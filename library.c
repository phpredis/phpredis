#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include "common.h"
#include "php_network.h"
#include <sys/types.h>
#ifndef _MSC_VER
#include <netinet/tcp.h>  /* TCP_NODELAY */
#include <sys/socket.h>
#endif
#ifdef HAVE_REDIS_IGBINARY
#include "igbinary/igbinary.h"
#endif
#include <zend_exceptions.h>
#include "php_redis.h"
#include "library.h"
#include "redis_commands.h"
#include <ext/standard/php_rand.h>

#define UNSERIALIZE_NONE 0
#define UNSERIALIZE_KEYS 1
#define UNSERIALIZE_VALS 2
#define UNSERIALIZE_ALL  3

#define SCORE_DECODE_NONE 0
#define SCORE_DECODE_INT  1
#define SCORE_DECODE_DOUBLE 2

#ifdef PHP_WIN32
    # if PHP_MAJOR_VERSION == 5 && PHP_MINOR_VERSION <= 4
        /* This proto is available from 5.5 on only */
        PHP_REDIS_API int usleep(unsigned int useconds);
    # endif
#endif

#if (PHP_MAJOR_VERSION < 7)
    int (*_add_next_index_string)(zval *, const char *, int) = &add_next_index_string;
    int (*_add_next_index_stringl)(zval *, const char *, uint, int) = &add_next_index_stringl;
    int (*_add_assoc_bool_ex)(zval *, const char *, uint, int) = &add_assoc_bool_ex;
    int (*_add_assoc_long_ex)(zval *, const char *, uint, long) = &add_assoc_long_ex;
    int (*_add_assoc_double_ex)(zval *, const char *, uint, double) = &add_assoc_double_ex;
    int (*_add_assoc_string_ex)(zval *, const char *, uint, char *, int) = &add_assoc_string_ex;
    int (*_add_assoc_stringl_ex)(zval *, const char *, uint, char *, uint, int) = &add_assoc_stringl_ex;
    int (*_add_assoc_zval_ex)(zval *, const char *, uint, zval *) = &add_assoc_zval_ex;
    void (*_php_var_serialize)(smart_str *, zval **, php_serialize_data_t * TSRMLS_DC) = &php_var_serialize;
    int (*_php_var_unserialize)(zval **, const unsigned char **, const unsigned char *, php_unserialize_data_t * TSRMLS_DC) = &php_var_unserialize;
#endif

extern zend_class_entry *redis_ce;
extern zend_class_entry *redis_exception_ce;

/* Helper to reselect the proper DB number when we reconnect */
static int reselect_db(RedisSock *redis_sock TSRMLS_DC) {
    char *cmd, *response;
    int cmd_len, response_len;

    cmd_len = redis_spprintf(redis_sock, NULL TSRMLS_CC, &cmd, "SELECT", "d",
                             redis_sock->dbNumber);

    if (redis_sock_write(redis_sock, cmd, cmd_len TSRMLS_CC) < 0) {
        efree(cmd);
        return -1;
    }

    efree(cmd);

    if ((response = redis_sock_read(redis_sock, &response_len TSRMLS_CC)) == NULL) {
        return -1;
    }

    if (strncmp(response, "+OK", 3)) {
        efree(response);
        return -1;
    }

    efree(response);
    return 0;
}

/* Helper to resend AUTH <password> in the case of a reconnect */
static int resend_auth(RedisSock *redis_sock TSRMLS_DC) {
    char *cmd, *response;
    int cmd_len, response_len;

    cmd_len = redis_spprintf(redis_sock, NULL TSRMLS_CC, &cmd, "AUTH", "s",
                             redis_sock->auth, strlen(redis_sock->auth));

    if (redis_sock_write(redis_sock, cmd, cmd_len TSRMLS_CC) < 0) {
        efree(cmd);
        return -1;
    }

    efree(cmd);

    response = redis_sock_read(redis_sock, &response_len TSRMLS_CC);
    if (response == NULL) {
        return -1;
    }

    if (strncmp(response, "+OK", 3)) {
        efree(response);
        return -1;
    }

    efree(response);
    return 0;
}

/* Helper function that will throw an exception for a small number of ERR codes
 * returned by Redis.  Typically we just return FALSE to the caller in the event
 * of an ERROR reply, but for the following error types:
 *    1) MASTERDOWN
 *    2) AUTH
 *    3) LOADING
 */
static void
redis_error_throw(RedisSock *redis_sock TSRMLS_DC)
{
    if (redis_sock != NULL && redis_sock->err != NULL &&
        memcmp(redis_sock->err, "ERR", sizeof("ERR") - 1) != 0 &&
        memcmp(redis_sock->err, "NOSCRIPT", sizeof("NOSCRIPT") - 1) != 0 &&
        memcmp(redis_sock->err, "WRONGTYPE", sizeof("WRONGTYPE") - 1) != 0
    ) {
        zend_throw_exception(redis_exception_ce, redis_sock->err, 0 TSRMLS_CC);
    }
}

PHP_REDIS_API void redis_stream_close(RedisSock *redis_sock TSRMLS_DC) {
    if (!redis_sock->persistent) {
        php_stream_close(redis_sock->stream);
    } else {
        php_stream_pclose(redis_sock->stream);
    }
}

PHP_REDIS_API int
redis_check_eof(RedisSock *redis_sock, int no_throw TSRMLS_DC)
{
    int count;

    if (!redis_sock->stream) {
        return -1;
    }

    /* NOITCE: set errno = 0 here
     *
     * There is a bug in php socket stream to check liveness of a connection:
     * if (0 >= recv(sock->socket, &buf, sizeof(buf), MSG_PEEK) && php_socket_errno() != EWOULDBLOCK) {
     *    alive = 0;
     * }
     * If last errno is EWOULDBLOCK and recv returns 0 because of connection closed, alive would not be
     * set to 0. However, the connection is close indeed. The php_stream_eof is not reliable. This will
     * cause a "read error on connection" exception when use a closed persistent connection.
     *
     * We work around this by set errno = 0 first.
     *
     * Bug fix of php: https://github.com/php/php-src/pull/1456
     * */
    errno = 0;
    if (php_stream_eof(redis_sock->stream) == 0) {
        /* Success */
        return 0;
    } else if (redis_sock->mode == MULTI || redis_sock->watching) {
        REDIS_STREAM_CLOSE_MARK_FAILED(redis_sock);
        if (!no_throw) {
            zend_throw_exception(redis_exception_ce,
                "Connection lost and socket is in MULTI/watching mode",
                0 TSRMLS_CC);
        }
        return -1;
    }
    /* TODO: configurable max retry count */
    for (count = 0; count < 10; ++count) {
        /* close existing stream before reconnecting */
        if (redis_sock->stream) {
            redis_stream_close(redis_sock TSRMLS_CC);
            redis_sock->stream = NULL;
        }
        // Wait for a while before trying to reconnect
        if (redis_sock->retry_interval) {
            // Random factor to avoid having several (or many) concurrent connections trying to reconnect at the same time
            long retry_interval = (count ? redis_sock->retry_interval : (php_rand(TSRMLS_C) % redis_sock->retry_interval));
            usleep(retry_interval);
        }
        /* reconnect */
        if (redis_sock_connect(redis_sock TSRMLS_CC) == 0) {
            /* check for EOF again. */
            errno = 0;
            if (php_stream_eof(redis_sock->stream) == 0) {
                /* If we're using a password, attempt a reauthorization */
                if (redis_sock->auth && resend_auth(redis_sock TSRMLS_CC) != 0) {
                    break;
                }
                /* If we're using a non-zero db, reselect it */
                if (redis_sock->dbNumber && reselect_db(redis_sock TSRMLS_CC) != 0) {
                    break;
                }
                /* Success */
                return 0;
            }
        }
    }
    /* close stream if still here */
    if (redis_sock->stream) {
        REDIS_STREAM_CLOSE_MARK_FAILED(redis_sock);
    }
    if (!no_throw) {
        zend_throw_exception(redis_exception_ce, "Connection lost", 0 TSRMLS_CC);
    }
    return -1;
}


PHP_REDIS_API int
redis_sock_read_scan_reply(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock,
                           REDIS_SCAN_TYPE type, zend_long *iter)
{
    REDIS_REPLY_TYPE reply_type;
    long reply_info;
    char *p_iter;

    /* Our response should have two multibulk replies */
    if(redis_read_reply_type(redis_sock, &reply_type, &reply_info TSRMLS_CC)<0
       || reply_type != TYPE_MULTIBULK || reply_info != 2)
    {
        return -1;
    }

    /* The BULK response iterator */
    if(redis_read_reply_type(redis_sock, &reply_type, &reply_info TSRMLS_CC)<0
       || reply_type != TYPE_BULK)
    {
        return -1;
    }

    /* Attempt to read the iterator */
    if(!(p_iter = redis_sock_read_bulk_reply(redis_sock, reply_info TSRMLS_CC))) {
        return -1;
    }

    /* Push the iterator out to the caller */
    *iter = atol(p_iter);
    efree(p_iter);

    /* Read our actual keys/members/etc differently depending on what kind of
       scan command this is.  They all come back in slightly different ways */
    switch(type) {
        case TYPE_SCAN:
            return redis_mbulk_reply_raw(INTERNAL_FUNCTION_PARAM_PASSTHRU,
                redis_sock, NULL, NULL);
        case TYPE_SSCAN:
            return redis_sock_read_multibulk_reply(
                INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock, NULL, NULL);
        case TYPE_ZSCAN:
            return redis_mbulk_reply_zipped_keys_dbl(INTERNAL_FUNCTION_PARAM_PASSTHRU,
                redis_sock, NULL, NULL);
        case TYPE_HSCAN:
            return redis_mbulk_reply_zipped_vals(INTERNAL_FUNCTION_PARAM_PASSTHRU,
                redis_sock, NULL, NULL);
        default:
            return -1;
    }
}

PHP_REDIS_API int redis_subscribe_response(INTERNAL_FUNCTION_PARAMETERS,
                                    RedisSock *redis_sock, zval *z_tab,
                                    void *ctx)
{
    subscribeContext *sctx = (subscribeContext*)ctx;
    zval *z_tmp, z_resp;

    // Consume response(s) from subscribe, which will vary on argc
    while(sctx->argc--) {
        if (!redis_sock_read_multibulk_reply_zval(
            INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock, &z_resp)
        ) {
            efree(sctx);
            return -1;
        }

        // We'll need to find the command response
        if ((z_tmp = zend_hash_index_find(Z_ARRVAL(z_resp), 0)) == NULL) {
            zval_dtor(&z_resp);
            efree(sctx);
            return -1;
        }

        // Make sure the command response matches the command we called
        if(strcasecmp(Z_STRVAL_P(z_tmp), sctx->kw) !=0) {
            zval_dtor(&z_resp);
            efree(sctx);
            return -1;
        }

        zval_dtor(&z_resp);
    }

#if (PHP_MAJOR_VERSION < 7)
    zval *z_ret, **z_args[4];
    sctx->cb.retval_ptr_ptr = &z_ret;
#else
    zval z_ret, z_args[4];
    sctx->cb.retval = &z_ret;
#endif
    sctx->cb.params = z_args;
    sctx->cb.no_separation = 0;

    /* Multibulk response, {[pattern], type, channel, payload } */
    while(1) {
        zval *z_type, *z_chan, *z_pat = NULL, *z_data;
        HashTable *ht_tab;
        int tab_idx=1, is_pmsg;

        if (!redis_sock_read_multibulk_reply_zval(
            INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock, &z_resp)) break;

        ht_tab = Z_ARRVAL(z_resp);

        if ((z_type = zend_hash_index_find(ht_tab, 0)) == NULL ||
           Z_TYPE_P(z_type) != IS_STRING
        ) {
            break;
        }

        // Check for message or pmessage
        if(!strncmp(Z_STRVAL_P(z_type), "message", 7) ||
           !strncmp(Z_STRVAL_P(z_type), "pmessage", 8))
        {
            is_pmsg = *Z_STRVAL_P(z_type)=='p';
        } else {
            break;
        }

        // Extract pattern if it's a pmessage
        if(is_pmsg) {
            if ((z_pat = zend_hash_index_find(ht_tab, tab_idx++)) == NULL) {
                break;
            }
        }

        // Extract channel and data
        if ((z_chan = zend_hash_index_find(ht_tab, tab_idx++)) == NULL ||
            (z_data = zend_hash_index_find(ht_tab, tab_idx++)) == NULL
        ) {
            break;
        }

        // Different args for SUBSCRIBE and PSUBSCRIBE
#if (PHP_MAJOR_VERSION < 7)
        z_args[0] = &getThis();
        if(is_pmsg) {
            z_args[1] = &z_pat;
            z_args[2] = &z_chan;
            z_args[3] = &z_data;
        } else {
            z_args[1] = &z_chan;
            z_args[2] = &z_data;
        }
#else
        z_args[0] = *getThis();
        if(is_pmsg) {
            z_args[1] = *z_pat;
            z_args[2] = *z_chan;
            z_args[3] = *z_data;
        } else {
            z_args[1] = *z_chan;
            z_args[2] = *z_data;
        }
#endif

        // Set arg count
        sctx->cb.param_count = tab_idx;

        // Execute callback
        if(zend_call_function(&(sctx->cb), &(sctx->cb_cache) TSRMLS_CC)
                              ==FAILURE)
        {
            break;
        }

        // If we have a return value free it
        zval_ptr_dtor(&z_ret);
        zval_dtor(&z_resp);
    }

    // This is an error state, clean up
    zval_dtor(&z_resp);
    efree(sctx);

    return -1;
}

PHP_REDIS_API int redis_unsubscribe_response(INTERNAL_FUNCTION_PARAMETERS,
                                      RedisSock *redis_sock, zval *z_tab,
                                      void *ctx)
{
    subscribeContext *sctx = (subscribeContext*)ctx;
    zval *z_chan, zv, *z_ret = &zv, z_resp;
    int i;

    array_init(z_ret);

    for (i = 0; i < sctx->argc; i++) {
        if (!redis_sock_read_multibulk_reply_zval(
            INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock, &z_resp) ||
            (z_chan = zend_hash_index_find(Z_ARRVAL(z_resp), 1)) == NULL
        ) {
            zval_dtor(z_ret);
            return -1;
        }

        add_assoc_bool(z_ret, Z_STRVAL_P(z_chan), 1);

        zval_dtor(&z_resp);
    }

    efree(sctx);

    RETVAL_ZVAL(z_ret, 0, 1);

    // Success
    return 0;
}

PHP_REDIS_API zval *
redis_sock_read_multibulk_reply_zval(INTERNAL_FUNCTION_PARAMETERS,
                                     RedisSock *redis_sock, zval *z_tab)
{
    char inbuf[4096];
    int numElems;
    size_t len;

    if (redis_sock_gets(redis_sock, inbuf, sizeof(inbuf) - 1, &len TSRMLS_CC) < 0) {
        return NULL;
    }

    if(inbuf[0] != '*') {
        return NULL;
    }
    numElems = atoi(inbuf+1);

    array_init(z_tab);

    redis_mbulk_reply_loop(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock, z_tab,
        numElems, UNSERIALIZE_ALL);

    return z_tab;
}

/**
 * redis_sock_read_bulk_reply
 */
PHP_REDIS_API char *
redis_sock_read_bulk_reply(RedisSock *redis_sock, int bytes TSRMLS_DC)
{
    int offset = 0;
    char *reply, c[2];
    size_t got;

    if (-1 == bytes || -1 == redis_check_eof(redis_sock, 0 TSRMLS_CC)) {
        return NULL;
    }

    /* Allocate memory for string */
    reply = emalloc(bytes+1);

    /* Consume bulk string */
    while(offset < bytes) {
        got = php_stream_read(redis_sock->stream, reply + offset, bytes-offset);
        if (got == 0) break;
        offset += got;
    }

    /* Protect against reading too few bytes */
    if (offset < bytes) {
        /* Error or EOF */
        zend_throw_exception(redis_exception_ce,
            "socket error on read socket", 0 TSRMLS_CC);
        efree(reply);
        return NULL;
    }

    /* Consume \r\n and null terminate reply string */
    php_stream_read(redis_sock->stream, c, 2);
    reply[bytes] = '\0';

    return reply;
}

/**
 * redis_sock_read
 */
PHP_REDIS_API char *
redis_sock_read(RedisSock *redis_sock, int *buf_len TSRMLS_DC)
{
    char inbuf[4096];
    size_t len;

    *buf_len = 0;
    if (redis_sock_gets(redis_sock, inbuf, sizeof(inbuf) - 1, &len TSRMLS_CC) < 0) {
        return NULL;
    }

    switch(inbuf[0]) {
        case '-':
            redis_sock_set_err(redis_sock, inbuf+1, len);

            /* Filter our ERROR through the few that should actually throw */
            redis_error_throw(redis_sock TSRMLS_CC);

            return NULL;
        case '$':
            *buf_len = atoi(inbuf + 1);
            return redis_sock_read_bulk_reply(redis_sock, *buf_len TSRMLS_CC);

        case '*':
            /* For null multi-bulk replies (like timeouts from brpoplpush): */
            if(memcmp(inbuf + 1, "-1", 2) == 0) {
                return NULL;
            }
            /* fall through */

        case '+':
        case ':':
            /* Single Line Reply */
            /* +OK or :123 */
            if (len > 1) {
                *buf_len = len;
                return estrndup(inbuf, *buf_len);
            }
        default:
            zend_throw_exception_ex(
                redis_exception_ce,
                0 TSRMLS_CC,
                "protocol error, got '%c' as reply type byte\n",
                inbuf[0]
            );
    }

    return NULL;
}

/* A simple union to store the various arg types we might handle in our
 * redis_spprintf command formatting function */
union resparg {
    char *str;
    zend_string *zstr;
    zval *zv;
    int ival;
    long lval;
    double dval;
};

/* A printf like method to construct a Redis RESP command.  It has been extended
 * to take a few different format specifiers that are convienient to phpredis.
 *
 * s - C string followed by length as a strlen_t
 * S - Pointer to a zend_string
 * k - Same as 's' but the value will be prefixed if phpredis is set up do do
 *     that and the working slot will be set if it has been passed.
 * v - A z_val which will be serialized if phpredis is configured to serialize.
 * f - A double value
 * F - Alias to 'f'
 * i - An integer
 * d - Alias to 'i'
 * l - A long
 * L - Alias to 'l'
 */
PHP_REDIS_API int
redis_spprintf(RedisSock *redis_sock, short *slot TSRMLS_DC, char **ret, char *kw, char *fmt, ...) {
    smart_string cmd = {0};
    va_list ap;
    union resparg arg;
    char *dup;
    int argfree;
    strlen_t arglen;

    va_start(ap, fmt);

    /* Header */
    redis_cmd_init_sstr(&cmd, strlen(fmt), kw, strlen(kw));

    while (*fmt) {
        switch (*fmt) {
            case 's':
                arg.str = va_arg(ap, char*);
                arglen = va_arg(ap, strlen_t);
                redis_cmd_append_sstr(&cmd, arg.str, arglen);
                break;
            case 'S':
                arg.zstr = va_arg(ap, zend_string*);
                redis_cmd_append_sstr(&cmd, arg.zstr->val, arg.zstr->len);
                break;
            case 'k':
                arg.str = va_arg(ap, char*);
                arglen = va_arg(ap, strlen_t);
                argfree = redis_key_prefix(redis_sock, &arg.str, &arglen);
                redis_cmd_append_sstr(&cmd, arg.str, arglen);
                if (slot) *slot = cluster_hash_key(arg.str, arglen);
                if (argfree) efree(arg.str);
                break;
            case 'v':
                arg.zv = va_arg(ap, zval*);
                argfree = redis_serialize(redis_sock, arg.zv, &dup, &arglen TSRMLS_CC);
                redis_cmd_append_sstr(&cmd, dup, arglen);
                if (argfree) efree(dup);
                break;
            case 'f':
            case 'F':
                arg.dval = va_arg(ap, double);
                redis_cmd_append_sstr_dbl(&cmd, arg.dval);
                break;
            case 'i':
            case 'd':
                arg.ival = va_arg(ap, int);
                redis_cmd_append_sstr_int(&cmd, arg.ival);
                break;
            case 'l':
            case 'L':
                arg.lval = va_arg(ap, long);
                redis_cmd_append_sstr_long(&cmd, arg.lval);
                break;
        }

        fmt++;
    }

    /* Null terminate */
    smart_string_0(&cmd);

    /* Push command string, return length */
    *ret = cmd.c;
    return cmd.len;
}

/*
 * Given a smart string, number of arguments, a keyword, and the length of the keyword
 * initialize our smart string with the proper Redis header for the command to follow
 */
int redis_cmd_init_sstr(smart_string *str, int num_args, char *keyword, int keyword_len) {
    smart_string_appendc(str, '*');
    smart_string_append_long(str, num_args + 1);
    smart_string_appendl(str, _NL, sizeof(_NL) -1);
    smart_string_appendc(str, '$');
    smart_string_append_long(str, keyword_len);
    smart_string_appendl(str, _NL, sizeof(_NL) - 1);
    smart_string_appendl(str, keyword, keyword_len);
    smart_string_appendl(str, _NL, sizeof(_NL) - 1);
    return str->len;
}

/*
 * Append a command sequence to a smart_string
 */
int redis_cmd_append_sstr(smart_string *str, char *append, int append_len) {
    smart_string_appendc(str, '$');
    smart_string_append_long(str, append_len);
    smart_string_appendl(str, _NL, sizeof(_NL) - 1);
    smart_string_appendl(str, append, append_len);
    smart_string_appendl(str, _NL, sizeof(_NL) - 1);

    /* Return our new length */
    return str->len;
}

/*
 * Append an integer to a smart string command
 */
int redis_cmd_append_sstr_int(smart_string *str, int append) {
    char int_buf[32];
    int int_len = snprintf(int_buf, sizeof(int_buf), "%d", append);
    return redis_cmd_append_sstr(str, int_buf, int_len);
}

/*
 * Append a long to a smart string command
 */
int redis_cmd_append_sstr_long(smart_string *str, long append) {
    char long_buf[32];
    int long_len = snprintf(long_buf, sizeof(long_buf), "%ld", append);
    return redis_cmd_append_sstr(str, long_buf, long_len);
}

/*
 * Append a double to a smart string command
 */
int
redis_cmd_append_sstr_dbl(smart_string *str, double value)
{
    char tmp[64];
    int len, retval;

    /* Convert to string */
    len = snprintf(tmp, sizeof(tmp), "%.16g", value);

    // Append the string
    retval = redis_cmd_append_sstr(str, tmp, len);

    /* Return new length */
    return retval;
}

/* Append a zval to a redis command.  The value will be serialized if we are
 * configured to do that */
int redis_cmd_append_sstr_zval(smart_string *str, zval *z, RedisSock *redis_sock TSRMLS_DC) {
    char *val;
    strlen_t vallen;
    int valfree, retval;

    valfree = redis_serialize(redis_sock, z, &val, &vallen TSRMLS_CC);
    retval = redis_cmd_append_sstr(str, val, vallen);
    if (valfree) efree(val);

    return retval;
}

/* Append a string key to a redis command.  This function takes care of prefixing the key
 * for the caller and setting the slot argument if it is passed non null */
int redis_cmd_append_sstr_key(smart_string *str, char *key, strlen_t len, RedisSock *redis_sock, short *slot) {
    int valfree, retval;

    valfree = redis_key_prefix(redis_sock, &key, &len);
    if (slot) *slot = cluster_hash_key(key, len);
    retval = redis_cmd_append_sstr(str, key, len);
    if (valfree) efree(key);

    return retval;
}

PHP_REDIS_API void redis_bulk_double_response(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock, zval *z_tab, void *ctx) {

    char *response;
    int response_len;
    double ret;

    if ((response = redis_sock_read(redis_sock, &response_len TSRMLS_CC)) == NULL) {
        IF_NOT_ATOMIC() {
            add_next_index_bool(z_tab, 0);
            return;
        } else {
            RETURN_FALSE;
        }
    }

    ret = atof(response);
    efree(response);
    IF_NOT_ATOMIC() {
        add_next_index_double(z_tab, ret);
    } else {
        RETURN_DOUBLE(ret);
    }
}

PHP_REDIS_API void redis_type_response(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock, zval *z_tab, void *ctx) {
    char *response;
    int response_len;
    long l;

    if ((response = redis_sock_read(redis_sock, &response_len TSRMLS_CC)) == NULL) {
        IF_NOT_ATOMIC() {
            add_next_index_bool(z_tab, 0);
            return;
        } else {
            RETURN_FALSE;
        }
    }

    if (strncmp(response, "+string", 7) == 0) {
        l = REDIS_STRING;
    } else if (strncmp(response, "+set", 4) == 0){
        l = REDIS_SET;
    } else if (strncmp(response, "+list", 5) == 0){
        l = REDIS_LIST;
    } else if (strncmp(response, "+zset", 5) == 0){
        l = REDIS_ZSET;
    } else if (strncmp(response, "+hash", 5) == 0){
        l = REDIS_HASH;
    } else {
        l = REDIS_NOT_FOUND;
    }

    efree(response);
    IF_NOT_ATOMIC() {
    add_next_index_long(z_tab, l);
    } else {
        RETURN_LONG(l);
    }
}

PHP_REDIS_API void redis_info_response(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock, zval *z_tab, void *ctx) {
    char *response;
    int response_len;
    zval zv = {{0}}, *z_ret = &zv;

    /* Read bulk response */
    if ((response = redis_sock_read(redis_sock, &response_len TSRMLS_CC)) == NULL) {
        RETURN_FALSE;
    }

    /* Parse it into a zval array */
    redis_parse_info_response(response, z_ret);

    /* Free source response */
    efree(response);

    IF_NOT_ATOMIC() {
        add_next_index_zval(z_tab, z_ret);
    } else {
        RETVAL_ZVAL(z_ret, 0, 1);
    }
}

PHP_REDIS_API void
redis_parse_info_response(char *response, zval *z_ret)
{
    char *cur, *pos;

    array_init(z_ret);

    cur = response;
    while(1) {
        /* skip comments and empty lines */
        if (*cur == '#' || *cur == '\r') {
            if ((cur = strstr(cur, _NL)) == NULL) {
                break;
            }
            cur += 2;
            continue;
        }

        /* key */
        if ((pos = strchr(cur, ':')) == NULL) {
            break;
        }
        char *key = cur;
        int key_len = pos - cur;
        key[key_len] = '\0';

        /* value */
        cur = pos + 1;
        if ((pos = strstr(cur, _NL)) == NULL) {
            break;
        }
        char *value = cur;
        int value_len = pos - cur;
        value[value_len] = '\0';

        double dval;
        zend_long lval;
        zend_uchar type = is_numeric_string(value, value_len, &lval, &dval, 0);
        if (type == IS_LONG) {
            add_assoc_long_ex(z_ret, key, key_len, lval);
        } else if (type == IS_DOUBLE) {
            add_assoc_double_ex(z_ret, key, key_len, dval);
        } else {
            add_assoc_stringl_ex(z_ret, key, key_len, value, value_len);
        }

        cur = pos + 2; /* \r, \n */
    }
}

/*
 * Specialized handling of the CLIENT LIST output so it comes out in a simple way for PHP userland code
 * to handle.
 */
PHP_REDIS_API void redis_client_list_reply(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock, zval *z_tab) {
    char *resp;
    int resp_len;

    /* Make sure we can read the bulk response from Redis */
    if ((resp = redis_sock_read(redis_sock, &resp_len TSRMLS_CC)) == NULL) {
        RETURN_FALSE;
    }

    zval zv, *z_ret = &zv;
#if (PHP_MAJOR_VERSION < 7)
    MAKE_STD_ZVAL(z_ret);
#endif

    /* Parse it out */
    redis_parse_client_list_response(resp, z_ret);

    /* Free our response */
    efree(resp);

    /* Return or append depending if we're atomic */
    IF_NOT_ATOMIC() {
        add_next_index_zval(z_tab, z_ret);
    } else {
        RETVAL_ZVAL(z_ret, 0, 1);
    }
}

PHP_REDIS_API void
redis_parse_client_list_response(char *response, zval *z_ret)
{
    char *p, *lpos, *kpos = NULL, *vpos = NULL, *p2, *key, *value;
    int klen = 0, done = 0, is_numeric;

    // Allocate memory for our response
    array_init(z_ret);

    /* Allocate memory for one user (there should be at least one, namely us!) */
    zval zv, *z_sub_result = &zv;
#if (PHP_MAJOR_VERSION < 7)
    ALLOC_INIT_ZVAL(z_sub_result);
#endif
    array_init(z_sub_result);

    // Pointers for parsing
    p = response;
    lpos = response;

    /* While we've got more to parse */
    while(!done) {
        /* What character are we on */
        switch(*p) {
            /* We're done */
            case '\0':
                done = 1;
                break;
            /* \n, ' ' mean we can pull a k/v pair */
            case '\n':
            case ' ':
                /* Grab our value */
                vpos = lpos;

                /* There is some communication error or Redis bug if we don't
                   have a key and value, but check anyway. */
                if(kpos && vpos) {
                    /* Allocate, copy in our key */
                    key = estrndup(kpos, klen);

                    /* Allocate, copy in our value */
                    value = estrndup(lpos, p - lpos);

                    /* Treat numbers as numbers, strings as strings */
                    is_numeric = 1;
                    for(p2 = value; *p2; ++p2) {
                        if(*p2 < '0' || *p2 > '9') {
                            is_numeric = 0;
                            break;
                        }
                    }

                    /* Add as a long or string, depending */
                    if(is_numeric == 1) {
                        add_assoc_long(z_sub_result, key, atol(value));
                    } else {
                        add_assoc_string(z_sub_result, key, value);
                    }
                    efree(value);
                    // If we hit a '\n', then we can add this user to our list
                    if(*p == '\n') {
                        /* Add our user */
                        add_next_index_zval(z_ret, z_sub_result);

                        /* If we have another user, make another one */
                        if(*(p+1) != '\0') {
#if (PHP_MAJOR_VERSION < 7)
                            ALLOC_INIT_ZVAL(z_sub_result);
#endif
                            array_init(z_sub_result);
                        }
                    }

                    // Free our key
                    efree(key);
                } else {
                    // Something is wrong
                    zval_dtor(z_ret);
                    ZVAL_BOOL(z_ret, 0);
                    return;
                }

                /* Move forward */
                lpos = p + 1;

                break;
            /* We can pull the key and null terminate at our sep */
            case '=':
                /* Key, key length */
                kpos = lpos;
                klen = p - lpos;

                /* Move forward */
                lpos = p + 1;

                break;
        }

        /* Increment */
        p++;
    }
}

PHP_REDIS_API void
redis_boolean_response_impl(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock,
                            zval *z_tab, void *ctx,
                            SuccessCallback success_callback)
{

    char *response;
    int response_len;
    zend_bool ret = 0;

    if ((response = redis_sock_read(redis_sock, &response_len TSRMLS_CC)) != NULL) {
        ret = (*response == '+');
        efree(response);
    }

    if (ret && success_callback != NULL) {
        success_callback(redis_sock);
    }
    IF_NOT_ATOMIC() {
        add_next_index_bool(z_tab, ret);
    } else {
        RETURN_BOOL(ret);
    }
}

PHP_REDIS_API void redis_boolean_response(INTERNAL_FUNCTION_PARAMETERS,
                                   RedisSock *redis_sock, zval *z_tab,
                                   void *ctx)
{
    redis_boolean_response_impl(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock,
        z_tab, ctx, NULL);
}

PHP_REDIS_API void redis_long_response(INTERNAL_FUNCTION_PARAMETERS,
                                RedisSock *redis_sock, zval * z_tab,
                                void *ctx)
{

    char *response;
    int response_len;

    if ((response = redis_sock_read(redis_sock, &response_len TSRMLS_CC))
                                    == NULL)
    {
        IF_NOT_ATOMIC() {
            add_next_index_bool(z_tab, 0);
            return;
        } else {
            RETURN_FALSE;
        }
    }

    if(response[0] == ':') {
#ifdef PHP_WIN32
        __int64 ret = _atoi64(response + 1);
#else
        long long ret = atoll(response + 1);
#endif
        IF_NOT_ATOMIC() {
            if(ret > LONG_MAX) { /* overflow */
                add_next_index_stringl(z_tab, response + 1, response_len - 1);
            } else {
                add_next_index_long(z_tab, (long)ret);
            }
        } else {
            if(ret > LONG_MAX) { /* overflow */
                RETVAL_STRINGL(response + 1, response_len - 1);
            } else {
                RETVAL_LONG((long)ret);
            }
        }
    } else {
        IF_NOT_ATOMIC() {
          add_next_index_null(z_tab);
        } else {
            RETVAL_FALSE;
        }
    }
    efree(response);
}

/* Helper method to convert [key, value, key, value] into [key => value,
 * key => value] when returning data to the caller.  Depending on our decode
 * flag we'll convert the value data types */
static void array_zip_values_and_scores(RedisSock *redis_sock, zval *z_tab,
                                        int decode TSRMLS_DC)
{

    zval zv, *z_ret = &zv;
    HashTable *keytable;

    array_init(z_ret);
    keytable = Z_ARRVAL_P(z_tab);

    for(zend_hash_internal_pointer_reset(keytable);
        zend_hash_has_more_elements(keytable) == SUCCESS;
        zend_hash_move_forward(keytable)) {

        zval *z_key_p, *z_value_p;

        if ((z_key_p = zend_hash_get_current_data(keytable)) == NULL) {
            continue;   /* this should never happen, according to the PHP people. */
        }

        /* get current value, a key */
        zend_string *hkey = zval_get_string(z_key_p);

        /* move forward */
        zend_hash_move_forward(keytable);

        /* fetch again */
        if ((z_value_p = zend_hash_get_current_data(keytable)) == NULL) {
            zend_string_release(hkey);
            continue;   /* this should never happen, according to the PHP people. */
        }

        /* get current value, a hash value now. */
        char *hval = Z_STRVAL_P(z_value_p);

        /* Decode the score depending on flag */
        if (decode == SCORE_DECODE_INT && Z_STRLEN_P(z_value_p) > 0) {
            add_assoc_long_ex(z_ret, hkey->val, hkey->len, atoi(hval+1));
        } else if (decode == SCORE_DECODE_DOUBLE) {
            add_assoc_double_ex(z_ret, hkey->val, hkey->len, atof(hval));
        } else {
            zval zv0, *z = &zv0;
#if (PHP_MAJOR_VERSION < 7)
            MAKE_STD_ZVAL(z);
#endif
            ZVAL_ZVAL(z, z_value_p, 1, 0);
            add_assoc_zval_ex(z_ret, hkey->val, hkey->len, z);
        }
        zend_string_release(hkey);
    }

    /* replace */
    zval_dtor(z_tab);
    ZVAL_ZVAL(z_tab, z_ret, 1, 0);
    zval_dtor(z_ret);
}

static int
redis_mbulk_reply_zipped(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock,
                         zval *z_tab, int unserialize, int decode)
{
    char inbuf[4096];
    int numElems;
    size_t len;

    if (redis_sock_gets(redis_sock, inbuf, sizeof(inbuf) - 1, &len TSRMLS_CC) < 0) {
        return -1;
    }

    if(inbuf[0] != '*') {
        IF_NOT_ATOMIC() {
            add_next_index_bool(z_tab, 0);
        } else {
            RETVAL_FALSE;
        }
        return -1;
    }
    numElems = atoi(inbuf+1);
    zval zv, *z_multi_result = &zv;
#if (PHP_MAJOR_VERSION < 7)
    MAKE_STD_ZVAL(z_multi_result);
#endif
    array_init(z_multi_result); /* pre-allocate array for multi's results. */

    /* Grab our key, value, key, value array */
    redis_mbulk_reply_loop(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock,
        z_multi_result, numElems, unserialize);

    /* Zip keys and values */
    array_zip_values_and_scores(redis_sock, z_multi_result, decode TSRMLS_CC);

    IF_NOT_ATOMIC() {
        add_next_index_zval(z_tab, z_multi_result);
    } else {
        RETVAL_ZVAL(z_multi_result, 0, 1);
    }

    return 0;
}

/* Zipped key => value reply but we don't touch anything (e.g. CONFIG GET) */
PHP_REDIS_API int redis_mbulk_reply_zipped_raw(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock, zval *z_tab, void *ctx)
{
    return redis_mbulk_reply_zipped(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock,
        z_tab, UNSERIALIZE_NONE, SCORE_DECODE_NONE);
}

/* Zipped key => value reply unserializing keys and decoding the score as an integer (PUBSUB) */
PHP_REDIS_API int redis_mbulk_reply_zipped_keys_int(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock,
                                               zval *z_tab, void *ctx)
{
    return redis_mbulk_reply_zipped(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock,
        z_tab, UNSERIALIZE_KEYS, SCORE_DECODE_INT);
}

/* Zipped key => value reply unserializing keys and decoding the score as a double (ZSET commands) */
PHP_REDIS_API int redis_mbulk_reply_zipped_keys_dbl(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock,
                                                    zval *z_tab, void *ctx)
{
    return redis_mbulk_reply_zipped(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock,
        z_tab, UNSERIALIZE_KEYS, SCORE_DECODE_DOUBLE);
}

/* Zipped key => value reply where only the values are unserialized (e.g. HMGET) */
PHP_REDIS_API int redis_mbulk_reply_zipped_vals(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock,
                                               zval *z_tab, void *ctx)
{
    return redis_mbulk_reply_zipped(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock,
        z_tab, UNSERIALIZE_VALS, SCORE_DECODE_NONE);
}

PHP_REDIS_API void
redis_1_response(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock, zval *z_tab, void *ctx)
{
    char *response;
    int response_len;
    zend_bool ret = 0;

    if ((response = redis_sock_read(redis_sock, &response_len TSRMLS_CC)) != NULL) {
        ret = (response[1] == '1');
        efree(response);
    }

    IF_NOT_ATOMIC() {
        add_next_index_bool(z_tab, ret);
    } else {
        RETURN_BOOL(ret);
    }
}

PHP_REDIS_API void redis_string_response(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock, zval *z_tab, void *ctx) {

    char *response;
    int response_len;

    if ((response = redis_sock_read(redis_sock, &response_len TSRMLS_CC))
                                    == NULL)
    {
        IF_NOT_ATOMIC() {
            add_next_index_bool(z_tab, 0);
        return;
        }
        RETURN_FALSE;
    }
    IF_NOT_ATOMIC() {
        zval zv, *z = &zv;
        if (redis_unserialize(redis_sock, response, response_len, z TSRMLS_CC)) {
#if (PHP_MAJOR_VERSION < 7)
            MAKE_STD_ZVAL(z);
            *z = zv;
#endif
            add_next_index_zval(z_tab, z);
        } else {
            add_next_index_stringl(z_tab, response, response_len);
        }
    } else {
        if (!redis_unserialize(redis_sock, response, response_len, return_value TSRMLS_CC)) {
            RETVAL_STRINGL(response, response_len);
        }
    }
    efree(response);
}

/* like string response, but never unserialized. */
PHP_REDIS_API void
redis_ping_response(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock,
                    zval *z_tab, void *ctx)
{

    char *response;
    int response_len;

    if ((response = redis_sock_read(redis_sock, &response_len TSRMLS_CC))
                                    == NULL)
    {
        IF_NOT_ATOMIC() {
            add_next_index_bool(z_tab, 0);
        return;
        }
        RETURN_FALSE;
    }
    IF_NOT_ATOMIC() {
        add_next_index_stringl(z_tab, response, response_len);
    } else {
        RETVAL_STRINGL(response, response_len);
    }
    efree(response);
}

/* Response for DEBUG object which is a formatted single line reply */
PHP_REDIS_API void redis_debug_response(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock,
                                        zval *z_tab, void *ctx)
{
    char *resp, *p, *p2, *p3, *p4;
    int is_numeric,  resp_len;

    /* Add or return false if we can't read from the socket */
    if((resp = redis_sock_read(redis_sock, &resp_len TSRMLS_CC))==NULL) {
        IF_NOT_ATOMIC() {
            add_next_index_bool(z_tab, 0);
            return;
        }
        RETURN_FALSE;
    }

    zval zv, *z_result = &zv;
#if (PHP_MAJOR_VERSION < 7)
    MAKE_STD_ZVAL(z_result);
#endif
    array_init(z_result);

    /* Skip the '+' */
    p = resp + 1;

    /* <info>:<value> <info2:value2> ... */
    while((p2 = strchr(p, ':'))!=NULL) {
        /* Null terminate at the ':' */
        *p2++ = '\0';

        /* Null terminate at the space if we have one */
        if((p3 = strchr(p2, ' '))!=NULL) {
            *p3++ = '\0';
        } else {
            p3 = resp + resp_len;
        }

        is_numeric = 1;
        for(p4=p2; *p4; ++p4) {
            if(*p4 < '0' || *p4 > '9') {
                is_numeric = 0;
                break;
            }
        }

        /* Add our value */
        if(is_numeric) {
            add_assoc_long(z_result, p, atol(p2));
        } else {
            add_assoc_string(z_result, p, p2);
        }

        p = p3;
    }

    efree(resp);

    IF_NOT_ATOMIC() {
        add_next_index_zval(z_tab, z_result);
    } else {
        RETVAL_ZVAL(z_result, 0, 1);
    }
}

/**
 * redis_sock_create
 */
PHP_REDIS_API RedisSock*
redis_sock_create(char *host, int host_len, unsigned short port,
                  double timeout, double read_timeout,
                  int persistent, char *persistent_id,
                  long retry_interval, zend_bool lazy_connect)
{
    RedisSock *redis_sock;

    redis_sock         = ecalloc(1, sizeof(RedisSock));
    redis_sock->host   = estrndup(host, host_len);
    redis_sock->stream = NULL;
    redis_sock->status = REDIS_SOCK_STATUS_DISCONNECTED;
    redis_sock->watching = 0;
    redis_sock->dbNumber = 0;
    redis_sock->retry_interval = retry_interval * 1000;
    redis_sock->persistent = persistent;
    redis_sock->lazy_connect = lazy_connect;
    redis_sock->persistent_id = NULL;

    if(persistent_id) {
        redis_sock->persistent_id = estrdup(persistent_id);
    }

    redis_sock->port    = port;
    redis_sock->timeout = timeout;
    redis_sock->read_timeout = read_timeout;

    redis_sock->serializer = REDIS_SERIALIZER_NONE;
    redis_sock->mode = ATOMIC;
    redis_sock->head = NULL;
    redis_sock->current = NULL;

    redis_sock->pipeline_cmd = NULL;
    redis_sock->pipeline_len = 0;

    redis_sock->err = NULL;
    redis_sock->err_len = 0;

    redis_sock->scan = REDIS_SCAN_NORETRY;

    redis_sock->readonly = 0;

    return redis_sock;
}

/**
 * redis_sock_connect
 */
PHP_REDIS_API int redis_sock_connect(RedisSock *redis_sock TSRMLS_DC)
{
    struct timeval tv, read_tv, *tv_ptr = NULL;
    char host[1024], *persistent_id = NULL;
    const char *fmtstr = "%s:%d";
    int host_len, err = 0;
    php_netstream_data_t *sock;
    int tcp_flag = 1;

    if (redis_sock->stream != NULL) {
        redis_sock_disconnect(redis_sock TSRMLS_CC);
    }

    tv.tv_sec  = (time_t)redis_sock->timeout;
    tv.tv_usec = (int)((redis_sock->timeout - tv.tv_sec) * 1000000);
    if(tv.tv_sec != 0 || tv.tv_usec != 0) {
        tv_ptr = &tv;
    }

    read_tv.tv_sec  = (time_t)redis_sock->read_timeout;
    read_tv.tv_usec = (int)((redis_sock->read_timeout-read_tv.tv_sec)*1000000);

    if(redis_sock->host[0] == '/' && redis_sock->port < 1) {
        host_len = snprintf(host, sizeof(host), "unix://%s", redis_sock->host);
    } else {
        if(redis_sock->port == 0)
            redis_sock->port = 6379;

#ifdef HAVE_IPV6
        /* If we've got IPv6 and find a colon in our address, convert to proper
         * IPv6 [host]:port format */
        if (strchr(redis_sock->host, ':') != NULL) {
            fmtstr = "[%s]:%d";
        }
#endif
        host_len = snprintf(host, sizeof(host), fmtstr, redis_sock->host, redis_sock->port);
    }

    if (redis_sock->persistent) {
        if (redis_sock->persistent_id) {
            spprintf(&persistent_id, 0, "phpredis:%s:%s", host,
                redis_sock->persistent_id);
        } else {
            spprintf(&persistent_id, 0, "phpredis:%s:%f", host,
                redis_sock->timeout);
        }
    }

    redis_sock->stream = php_stream_xport_create(host, host_len,
        0, STREAM_XPORT_CLIENT | STREAM_XPORT_CONNECT,
        persistent_id, tv_ptr, NULL, NULL, &err);

    if (persistent_id) {
        efree(persistent_id);
    }

    if (!redis_sock->stream) {
        return -1;
    }

    /* set TCP_NODELAY */
    sock = (php_netstream_data_t*)redis_sock->stream->abstract;
    setsockopt(sock->socket, IPPROTO_TCP, TCP_NODELAY, (char *) &tcp_flag,
        sizeof(int));

    php_stream_auto_cleanup(redis_sock->stream);

    if (read_tv.tv_sec != 0 || read_tv.tv_usec != 0) {
        php_stream_set_option(redis_sock->stream,PHP_STREAM_OPTION_READ_TIMEOUT,
            0, &read_tv);
    }
    php_stream_set_option(redis_sock->stream,
        PHP_STREAM_OPTION_WRITE_BUFFER, PHP_STREAM_BUFFER_NONE, NULL);

    redis_sock->status = REDIS_SOCK_STATUS_CONNECTED;

    return 0;
}

/**
 * redis_sock_server_open
 */
PHP_REDIS_API int
redis_sock_server_open(RedisSock *redis_sock TSRMLS_DC)
{
    int res = -1;

    switch (redis_sock->status) {
        case REDIS_SOCK_STATUS_DISCONNECTED:
            return redis_sock_connect(redis_sock TSRMLS_CC);
        case REDIS_SOCK_STATUS_CONNECTED:
            res = 0;
        break;
    }

    return res;
}

/**
 * redis_sock_disconnect
 */
PHP_REDIS_API int redis_sock_disconnect(RedisSock *redis_sock TSRMLS_DC)
{
    if (redis_sock == NULL) {
        return 1;
    }

    redis_sock->dbNumber = 0;
    if (redis_sock->stream != NULL) {
			redis_sock->status = REDIS_SOCK_STATUS_DISCONNECTED;
            redis_sock->watching = 0;

            /* Stil valid? */
            if (!redis_sock->persistent) {
                php_stream_close(redis_sock->stream);
            }
            redis_sock->stream = NULL;

            return 1;
    }

    return 0;
}

PHP_REDIS_API void redis_send_discard(INTERNAL_FUNCTION_PARAMETERS,
                               RedisSock *redis_sock)
{
    char *cmd;
    int response_len, cmd_len;
    char * response;

    cmd_len = redis_spprintf(redis_sock, NULL TSRMLS_CC, &cmd, "DISCARD", "");

    SOCKET_WRITE_COMMAND(redis_sock, cmd, cmd_len)
    efree(cmd);

    if ((response = redis_sock_read(redis_sock, &response_len TSRMLS_CC))
                                    == NULL)
    {
        RETURN_FALSE;
    }

    RETVAL_BOOL(response_len == 3 && strncmp(response, "+OK", 3) == 0);
    efree(response);
}

/**
 * redis_sock_set_err
 */
PHP_REDIS_API void
redis_sock_set_err(RedisSock *redis_sock, const char *msg, int msg_len)
{
    // Free our last error
    if (redis_sock->err != NULL) {
        efree(redis_sock->err);
    }

    if (msg != NULL && msg_len > 0) {
        // Copy in our new error message
        redis_sock->err = estrndup(msg, msg_len);
        redis_sock->err_len = msg_len;
    } else {
        // Set to null, with zero length
        redis_sock->err = NULL;
        redis_sock->err_len = 0;
    }
}

/**
 * redis_sock_read_multibulk_reply
 */
PHP_REDIS_API int redis_sock_read_multibulk_reply(INTERNAL_FUNCTION_PARAMETERS,
                                           RedisSock *redis_sock, zval *z_tab,
                                           void *ctx)
{
    char inbuf[4096];
    int numElems;
    size_t len;

    if (redis_sock_gets(redis_sock, inbuf, sizeof(inbuf) - 1, &len TSRMLS_CC) < 0) {
        return -1;
    }

    if(inbuf[0] != '*') {
        IF_NOT_ATOMIC() {
            add_next_index_bool(z_tab, 0);
        } else {
            if (inbuf[0] == '-') {
                redis_sock_set_err(redis_sock, inbuf+1, len);
            }
            RETVAL_FALSE;
        }
        return -1;
    }
    numElems = atoi(inbuf+1);
    zval zv, *z_multi_result = &zv;
#if (PHP_MAJOR_VERSION < 7)
    MAKE_STD_ZVAL(z_multi_result);
#endif
    array_init(z_multi_result); /* pre-allocate array for multi's results. */

    redis_mbulk_reply_loop(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock,
        z_multi_result, numElems, UNSERIALIZE_ALL);

    IF_NOT_ATOMIC() {
        add_next_index_zval(z_tab, z_multi_result);
    } else {
        RETVAL_ZVAL(z_multi_result, 0, 1);
    }
    /*zval_copy_ctor(return_value); */
    return 0;
}

/* Like multibulk reply, but don't touch the values, they won't be unserialized
 * (this is used by HKEYS). */
PHP_REDIS_API int
redis_mbulk_reply_raw(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock, zval *z_tab, void *ctx)
{
    char inbuf[4096];
    int numElems;
    size_t len;

    if (redis_sock_gets(redis_sock, inbuf, sizeof(inbuf) - 1, &len TSRMLS_CC) < 0) {
        return -1;
    }

    if(inbuf[0] != '*') {
        IF_NOT_ATOMIC() {
            add_next_index_bool(z_tab, 0);
        } else {
            if (inbuf[0] == '-') {
                redis_sock_set_err(redis_sock, inbuf+1, len);
            }
            RETVAL_FALSE;
        }
        return -1;
    }
    numElems = atoi(inbuf+1);
    zval zv, *z_multi_result = &zv;
#if (PHP_MAJOR_VERSION < 7)
    MAKE_STD_ZVAL(z_multi_result);
#endif
    array_init(z_multi_result); /* pre-allocate array for multi's results. */

    redis_mbulk_reply_loop(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock,
        z_multi_result, numElems, UNSERIALIZE_NONE);

    IF_NOT_ATOMIC() {
        add_next_index_zval(z_tab, z_multi_result);
    } else {
        RETVAL_ZVAL(z_multi_result, 0, 1);
    }
    /*zval_copy_ctor(return_value); */
    return 0;
}

PHP_REDIS_API void
redis_mbulk_reply_loop(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock,
                       zval *z_tab, int count, int unserialize)
{
    char *line;
    int i, len;

    for (i = 0; i < count; ++i) {
        if ((line = redis_sock_read(redis_sock, &len TSRMLS_CC)) == NULL) {
            add_next_index_bool(z_tab, 0);
            continue;
        }

        /* We will attempt unserialization, if we're unserializing everything,
         * or if we're unserializing keys and we're on a key, or we're
         * unserializing values and we're on a value! */
        int unwrap = (
            (unserialize == UNSERIALIZE_ALL) ||
            (unserialize == UNSERIALIZE_KEYS && i % 2 == 0) ||
            (unserialize == UNSERIALIZE_VALS && i % 2 != 0)
        );
        zval zv, *z = &zv;
        if (unwrap && redis_unserialize(redis_sock, line, len, z TSRMLS_CC)) {
#if (PHP_MAJOR_VERSION < 7)
            MAKE_STD_ZVAL(z);
            *z = zv;
#endif
            add_next_index_zval(z_tab, z);
        } else {
            add_next_index_stringl(z_tab, line, len);
        }
        efree(line);
    }
}

/* Specialized multibulk processing for HMGET where we need to pair requested
 * keys with their returned values */
PHP_REDIS_API int redis_mbulk_reply_assoc(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock, zval *z_tab, void *ctx)
{
    char inbuf[4096], *response;
    int response_len;
    int i, numElems;
    size_t len;

    zval *z_keys = ctx;

    if (redis_sock_gets(redis_sock, inbuf, sizeof(inbuf) - 1, &len TSRMLS_CC) < 0) {
        return -1;
    }

    if(inbuf[0] != '*') {
        IF_NOT_ATOMIC() {
            add_next_index_bool(z_tab, 0);
        } else {
            RETVAL_FALSE;
        }
        return -1;
    }
    numElems = atoi(inbuf+1);
    zval zv, *z_multi_result = &zv;
#if (PHP_MAJOR_VERSION < 7)
    MAKE_STD_ZVAL(z_multi_result);
#endif
    array_init(z_multi_result); /* pre-allocate array for multi's results. */

    for(i = 0; i < numElems; ++i) {
        response = redis_sock_read(redis_sock, &response_len TSRMLS_CC);
        if(response != NULL) {
            zval zv0, *z = &zv0;
            if (redis_unserialize(redis_sock, response, response_len, z TSRMLS_CC)) {
#if (PHP_MAJOR_VERSION < 7)
                MAKE_STD_ZVAL(z);
                *z = zv0;
#endif
                add_assoc_zval_ex(z_multi_result, Z_STRVAL(z_keys[i]), Z_STRLEN(z_keys[i]), z);
            } else {
                add_assoc_stringl_ex(z_multi_result, Z_STRVAL(z_keys[i]), Z_STRLEN(z_keys[i]), response, response_len);
            }
            efree(response);
        } else {
            add_assoc_bool_ex(z_multi_result, Z_STRVAL(z_keys[i]), Z_STRLEN(z_keys[i]), 0);
        }
        zval_dtor(&z_keys[i]);
    }
    efree(z_keys);

    IF_NOT_ATOMIC() {
        add_next_index_zval(z_tab, z_multi_result);
    } else {
        RETVAL_ZVAL(z_multi_result, 0, 1);
    }
    return 0;
}

/**
 * redis_sock_write
 */
PHP_REDIS_API int
redis_sock_write(RedisSock *redis_sock, char *cmd, size_t sz TSRMLS_DC)
{
    if (!redis_sock || redis_sock->status == REDIS_SOCK_STATUS_DISCONNECTED) {
        zend_throw_exception(redis_exception_ce, "Connection closed",
            0 TSRMLS_CC);
    } else if (redis_check_eof(redis_sock, 0 TSRMLS_CC) == 0 &&
               php_stream_write(redis_sock->stream, cmd, sz) == sz
    ) {
        return sz;
    }
    return -1;
}

/**
 * redis_free_socket
 */
PHP_REDIS_API void redis_free_socket(RedisSock *redis_sock)
{
    if(redis_sock->prefix) {
        efree(redis_sock->prefix);
    }
    if (redis_sock->pipeline_cmd) {
        efree(redis_sock->pipeline_cmd);
    }
    if(redis_sock->err) {
        efree(redis_sock->err);
    }
    if(redis_sock->auth) {
        efree(redis_sock->auth);
    }
    if(redis_sock->persistent_id) {
        efree(redis_sock->persistent_id);
    }
    efree(redis_sock->host);
    efree(redis_sock);
}

PHP_REDIS_API int
redis_serialize(RedisSock *redis_sock, zval *z, char **val, strlen_t *val_len
                TSRMLS_DC)
{
#if ZEND_MODULE_API_NO >= 20100000
    php_serialize_data_t ht;
#else
    HashTable ht;
#endif
    smart_str sstr = {0};
#ifdef HAVE_REDIS_IGBINARY
    size_t sz;
    uint8_t *val8;
#endif

    switch(redis_sock->serializer) {
        case REDIS_SERIALIZER_NONE:
            switch(Z_TYPE_P(z)) {

                case IS_STRING:
                    *val = Z_STRVAL_P(z);
                    *val_len = Z_STRLEN_P(z);
                    break;

                case IS_OBJECT:
                    *val = "Object";
                    *val_len = 6;
                    break;

                case IS_ARRAY:
                    *val = "Array";
                    *val_len = 5;
                    break;

                default: { /* copy */
                    zend_string *zstr = zval_get_string(z);
                    *val = estrndup(zstr->val, zstr->len);
                    *val_len = zstr->len;
                    zend_string_release(zstr);
                    return 1;
                }
            }
            break;
        case REDIS_SERIALIZER_PHP:

#if ZEND_MODULE_API_NO >= 20100000
            PHP_VAR_SERIALIZE_INIT(ht);
#else
            zend_hash_init(&ht, 10, NULL, NULL, 0);
#endif
            php_var_serialize(&sstr, z, &ht);
#if (PHP_MAJOR_VERSION < 7)
            *val = estrndup(sstr.c, sstr.len);
            *val_len = sstr.len;
#else
            *val = estrndup(sstr.s->val, sstr.s->len);
            *val_len = sstr.s->len;
#endif
            smart_str_free(&sstr);
#if ZEND_MODULE_API_NO >= 20100000
            PHP_VAR_SERIALIZE_DESTROY(ht);
#else
            zend_hash_destroy(&ht);
#endif

            return 1;

        case REDIS_SERIALIZER_IGBINARY:
#ifdef HAVE_REDIS_IGBINARY
            if(igbinary_serialize(&val8, (size_t *)&sz, z TSRMLS_CC) == 0) {
                *val = (char*)val8;
                *val_len = sz;
                return 1;
            }
#endif
            break;
    }
    return 0;
}

PHP_REDIS_API int
redis_unserialize(RedisSock* redis_sock, const char *val, int val_len,
                  zval *z_ret TSRMLS_DC)
{

    php_unserialize_data_t var_hash;
    int ret = 0;

    switch(redis_sock->serializer) {
        case REDIS_SERIALIZER_PHP:
#if ZEND_MODULE_API_NO >= 20100000
            PHP_VAR_UNSERIALIZE_INIT(var_hash);
#else
            memset(&var_hash, 0, sizeof(var_hash));
#endif
            if (php_var_unserialize(z_ret, (const unsigned char**)&val,
                    (const unsigned char*)val + val_len, &var_hash)
            ) {
                ret = 1;
            }
#if ZEND_MODULE_API_NO >= 20100000
            PHP_VAR_UNSERIALIZE_DESTROY(var_hash);
#else
            var_destroy(&var_hash);
#endif
            break;

        case REDIS_SERIALIZER_IGBINARY:
#ifdef HAVE_REDIS_IGBINARY
            /*
             * Check if the given string starts with an igbinary header.
             *
             * A modern igbinary string consists of the following format:
             *
             * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-
             * | header (4) | type (1) | ... (n) |  NUL (1) |
             * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-
             *
             * With header being either 0x00000001 or 0x00000002
             * (encoded as big endian).
             *
             * Not all versions contain the trailing NULL byte though, so
             * do not check for that.
             */
            if (val_len < 5
                    || (memcmp(val, "\x00\x00\x00\x01", 4) != 0
                    && memcmp(val, "\x00\x00\x00\x02", 4) != 0))
            {
                /* This is most definitely not an igbinary string, so do
                   not try to unserialize this as one. */
                break;
            }

#if (PHP_MAJOR_VERSION < 7)
            INIT_PZVAL(z_ret);
            ret = !igbinary_unserialize((const uint8_t *)val, (size_t)val_len, &z_ret TSRMLS_CC);
#else
            ret = !igbinary_unserialize((const uint8_t *)val, (size_t)val_len, z_ret TSRMLS_CC);
#endif

#endif
            break;
    }
    return ret;
}

PHP_REDIS_API int
redis_key_prefix(RedisSock *redis_sock, char **key, strlen_t *key_len) {
    int ret_len;
    char *ret;

    if(redis_sock->prefix == NULL || redis_sock->prefix_len == 0) {
        return 0;
    }

    ret_len = redis_sock->prefix_len + *key_len;
    ret = ecalloc(1 + ret_len, 1);
    memcpy(ret, redis_sock->prefix, redis_sock->prefix_len);
    memcpy(ret + redis_sock->prefix_len, *key, *key_len);

    *key = ret;
    *key_len = ret_len;
    return 1;
}

/*
 * Processing for variant reply types (think EVAL)
 */

PHP_REDIS_API int
redis_sock_gets(RedisSock *redis_sock, char *buf, int buf_size,
                size_t *line_size TSRMLS_DC)
{
    // Handle EOF
    if(-1 == redis_check_eof(redis_sock, 0 TSRMLS_CC)) {
        return -1;
    }

    if(php_stream_get_line(redis_sock->stream, buf, buf_size, line_size)
                           == NULL)
    {
        // Close, put our socket state into error
        REDIS_STREAM_CLOSE_MARK_FAILED(redis_sock);

        // Throw a read error exception
        zend_throw_exception(redis_exception_ce, "read error on connection",
            0 TSRMLS_CC);
        return -1;
    }

	/* We don't need \r\n */
	*line_size-=2;
	buf[*line_size]='\0';

	/* Success! */
	return 0;
}

PHP_REDIS_API int
redis_read_reply_type(RedisSock *redis_sock, REDIS_REPLY_TYPE *reply_type,
                      long *reply_info TSRMLS_DC)
{
    // Make sure we haven't lost the connection, even trying to reconnect
    if(-1 == redis_check_eof(redis_sock, 0 TSRMLS_CC)) {
        // Failure
        return -1;
    }

    // Attempt to read the reply-type byte
    if((*reply_type = php_stream_getc(redis_sock->stream)) == EOF) {
        zend_throw_exception(redis_exception_ce, "socket error on read socket",
            0 TSRMLS_CC);
        return -1;
    }

    // If this is a BULK, MULTI BULK, or simply an INTEGER response, we can
    // extract the value or size info here
    if(*reply_type == TYPE_INT || *reply_type == TYPE_BULK ||
       *reply_type == TYPE_MULTIBULK)
    {
        // Buffer to hold size information
        char inbuf[255];

		/* Read up to our newline */
		if(php_stream_gets(redis_sock->stream, inbuf, sizeof(inbuf)) == NULL) {
			return -1;
		}

		/* Set our size response */
		*reply_info = atol(inbuf);
	}

	/* Success! */
	return 0;
}

/*
 * Read a single line response, having already consumed the reply-type byte
 */
PHP_REDIS_API int
redis_read_variant_line(RedisSock *redis_sock, REDIS_REPLY_TYPE reply_type,
                        zval *z_ret TSRMLS_DC)
{
    // Buffer to read our single line reply
    char inbuf[4096];
    size_t line_size;

	/* Attempt to read our single line reply */
	if(redis_sock_gets(redis_sock, inbuf, sizeof(inbuf), &line_size TSRMLS_CC) < 0) {
		return -1;
	}

    // If this is an error response, check if it is a SYNC error, and throw in
    // that case
    if(reply_type == TYPE_ERR) {
		/* Set our last error */
		redis_sock_set_err(redis_sock, inbuf, line_size);

        /* Handle throwable errors */
        redis_error_throw(redis_sock TSRMLS_CC);

		/* Set our response to FALSE */
		ZVAL_FALSE(z_ret);
	} else {
		/* Set our response to TRUE */
		ZVAL_TRUE(z_ret);
	}

    return 0;
}

PHP_REDIS_API int
redis_read_variant_bulk(RedisSock *redis_sock, int size, zval *z_ret
                        TSRMLS_DC)
{
    // Attempt to read the bulk reply
    char *bulk_resp = redis_sock_read_bulk_reply(redis_sock, size TSRMLS_CC);

	/* Set our reply to FALSE on failure, and the string on success */
	if(bulk_resp == NULL) {
		ZVAL_FALSE(z_ret);
		return -1;
	}
    ZVAL_STRINGL(z_ret, bulk_resp, size);
    efree(bulk_resp);
    return 0;
}

PHP_REDIS_API int
redis_read_multibulk_recursive(RedisSock *redis_sock, int elements, zval *z_ret
                               TSRMLS_DC)
{
    long reply_info;
    REDIS_REPLY_TYPE reply_type;
    zval zv, *z_subelem = &zv;

    // Iterate while we have elements
    while(elements > 0) {
        // Attempt to read our reply type
        if(redis_read_reply_type(redis_sock, &reply_type, &reply_info
                                 TSRMLS_CC) < 0)
        {
            zend_throw_exception_ex(redis_exception_ce, 0 TSRMLS_CC,
                "protocol error, couldn't parse MULTI-BULK response\n",
                reply_type);
            return -1;
        }

        // Switch on our reply-type byte
        switch(reply_type) {
            case TYPE_ERR:
            case TYPE_LINE:
#if (PHP_MAJOR_VERSION < 7)
                ALLOC_INIT_ZVAL(z_subelem);
#endif
                redis_read_variant_line(redis_sock, reply_type, z_subelem
                    TSRMLS_CC);
                add_next_index_zval(z_ret, z_subelem);
                break;
            case TYPE_INT:
                // Add our long value
                add_next_index_long(z_ret, reply_info);
                break;
            case TYPE_BULK:
                // Init a zval for our bulk response, read and add it
#if (PHP_MAJOR_VERSION < 7)
                ALLOC_INIT_ZVAL(z_subelem);
#endif
                redis_read_variant_bulk(redis_sock, reply_info, z_subelem
                    TSRMLS_CC);
                add_next_index_zval(z_ret, z_subelem);
                break;
            case TYPE_MULTIBULK:
                // Construct an array for our sub element, and add it,
                // and recurse
#if (PHP_MAJOR_VERSION < 7)
                ALLOC_INIT_ZVAL(z_subelem);
#endif
                array_init(z_subelem);
                add_next_index_zval(z_ret, z_subelem);
                redis_read_multibulk_recursive(redis_sock, reply_info,
                    z_subelem TSRMLS_CC);
                break;
            default:
                // Stop the compiler from whinging
                break;
        }

		/* Decrement our element counter */
		elements--;
	}

    return 0;
}

PHP_REDIS_API int
redis_read_variant_reply(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock,
                         zval *z_tab, void *ctx)
{
    // Reply type, and reply size vars
    REDIS_REPLY_TYPE reply_type;
    long reply_info;
    //char *bulk_resp;

    // Attempt to read our header
    if(redis_read_reply_type(redis_sock,&reply_type,&reply_info TSRMLS_CC) < 0)
    {
        return -1;
    }

    zval zv, *z_ret = &zv;
#if (PHP_MAJOR_VERSION < 7)
    MAKE_STD_ZVAL(z_ret);
#endif
	/* Switch based on our top level reply type */
	switch(reply_type) {
		case TYPE_ERR:
		case TYPE_LINE:
			redis_read_variant_line(redis_sock, reply_type, z_ret TSRMLS_CC);
			break;
		case TYPE_INT:
			ZVAL_LONG(z_ret, reply_info);
			break;
		case TYPE_BULK:
			redis_read_variant_bulk(redis_sock, reply_info, z_ret TSRMLS_CC);
			break;
		case TYPE_MULTIBULK:
			/* Initialize an array for our multi-bulk response */
			array_init(z_ret);

            // If we've got more than zero elements, parse our multi bulk
            // response recursively
            if(reply_info > -1) {
                redis_read_multibulk_recursive(redis_sock, reply_info, z_ret
                    TSRMLS_CC);
            }
            break;
        default:
#if (PHP_MAJOR_VERSION < 7)
            efree(z_ret);
#endif
            // Protocol error
            zend_throw_exception_ex(redis_exception_ce, 0 TSRMLS_CC,
                "protocol error, got '%c' as reply-type byte\n", reply_type);
            return FAILURE;
    }

	IF_NOT_ATOMIC() {
		add_next_index_zval(z_tab, z_ret);
	} else {
		/* Set our return value */
        RETVAL_ZVAL(z_ret, 0, 1);
	}

	/* Success */
	return 0;
}

/* vim: set tabstop=4 softtabstop=4 expandtab shiftwidth=4: */
