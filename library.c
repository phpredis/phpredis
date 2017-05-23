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

    cmd_len = redis_cmd_format_static(&cmd, "SELECT", "d", redis_sock->dbNumber);

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

    cmd_len = redis_cmd_format_static(&cmd, "AUTH", "s", redis_sock->auth,
        strlen(redis_sock->auth));

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
static void redis_error_throw(char *err, size_t err_len TSRMLS_DC) {
    /* Handle stale data error (slave syncing with master) */
    if (err_len == sizeof(REDIS_ERR_SYNC_MSG) - 1 &&
        !memcmp(err,REDIS_ERR_SYNC_KW,sizeof(REDIS_ERR_SYNC_KW)-1))
    {
        zend_throw_exception(redis_exception_ce,
            "SYNC with master in progress or master down!", 0 TSRMLS_CC);
    } else if (err_len == sizeof(REDIS_ERR_LOADING_MSG) - 1 &&
               !memcmp(err,REDIS_ERR_LOADING_KW,sizeof(REDIS_ERR_LOADING_KW)-1))
    {
        zend_throw_exception(redis_exception_ce,
            "Redis is LOADING the dataset", 0 TSRMLS_CC);
    } else if (err_len == sizeof(REDIS_ERR_AUTH_MSG) -1 &&
               !memcmp(err,REDIS_ERR_AUTH_KW,sizeof(REDIS_ERR_AUTH_KW)-1))
    {
        zend_throw_exception(redis_exception_ce,
            "Failed to AUTH connection", 0 TSRMLS_CC);
    }
}

PHP_REDIS_API void redis_stream_close(RedisSock *redis_sock TSRMLS_DC) {
    if (!redis_sock->persistent) {
        php_stream_close(redis_sock->stream);
    } else {
        php_stream_pclose(redis_sock->stream);
    }
}

PHP_REDIS_API int redis_check_eof(RedisSock *redis_sock, int no_throw TSRMLS_DC)
{
    int eof;
    int count = 0;

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
    eof = php_stream_eof(redis_sock->stream);
    for (; eof; count++) {
        if((MULTI == redis_sock->mode) || redis_sock->watching || count == 10) {
            /* too many failures */
            if(redis_sock->stream) { /* close stream if still here */
                REDIS_STREAM_CLOSE_MARK_FAILED(redis_sock);
            }
            if(!no_throw) {
                zend_throw_exception(redis_exception_ce, "Connection lost", 
                    0 TSRMLS_CC);
            }
        return -1;
        }
        if(redis_sock->stream) { /* close existing stream before reconnecting */
            redis_stream_close(redis_sock TSRMLS_CC);
            redis_sock->stream = NULL;
            redis_sock->mode   = ATOMIC;
            redis_sock->watching = 0;
        }
        // Wait for a while before trying to reconnect
        if (redis_sock->retry_interval) {
            // Random factor to avoid having several (or many) concurrent connections trying to reconnect at the same time
            long retry_interval = (count ? redis_sock->retry_interval : (php_rand(TSRMLS_C) % redis_sock->retry_interval));
            usleep(retry_interval);
        }
        redis_sock_connect(redis_sock TSRMLS_CC); /* reconnect */
        if(redis_sock->stream) { /*  check for EOF again. */
            errno = 0;
            eof = php_stream_eof(redis_sock->stream);
        }
    }

    /* We've connected if we have a count */
    if (count) {
        /* If we're using a password, attempt a reauthorization */
        if (redis_sock->auth && resend_auth(redis_sock TSRMLS_CC) != 0) {
            return -1;
        }

        /* If we're using a non-zero db, reselect it */
        if (redis_sock->dbNumber && reselect_db(redis_sock TSRMLS_CC) != 0) {
            return -1;
        }
    }

    /* Success */
    return 0;
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
    char inbuf[1024];
    int numElems;

    if(-1 == redis_check_eof(redis_sock, 0 TSRMLS_CC)) {
        return NULL;
    }

    if(php_stream_gets(redis_sock->stream, inbuf, 1024) == NULL) {
        REDIS_STREAM_CLOSE_MARK_FAILED(redis_sock);
        zend_throw_exception(redis_exception_ce, 
            "read error on connection", 0 TSRMLS_CC);
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
PHP_REDIS_API char *redis_sock_read_bulk_reply(RedisSock *redis_sock, int bytes TSRMLS_DC)
{
    int offset = 0;
    size_t got;

    char *reply, c[2];

    if (-1 == bytes || -1 == redis_check_eof(redis_sock, 0 TSRMLS_CC)) {
        return NULL;
    }
        reply = emalloc(bytes+1);

        while(offset < bytes) {
            got = php_stream_read(redis_sock->stream, reply + offset, 
                bytes-offset);
            if (got <= 0) {
                /* Error or EOF */
                zend_throw_exception(redis_exception_ce, 
                    "socket error on read socket", 0 TSRMLS_CC);
                break;
            }
            offset += got;
        }
	php_stream_read(redis_sock->stream, c, 2);

    reply[bytes] = 0;
    return reply;
}

/**
 * redis_sock_read
 */
PHP_REDIS_API char *redis_sock_read(RedisSock *redis_sock, int *buf_len TSRMLS_DC)
{
    char inbuf[1024];
    size_t err_len;

    *buf_len = 0;
    if(-1 == redis_check_eof(redis_sock, 0 TSRMLS_CC)) {
        return NULL;
    }

    if(php_stream_gets(redis_sock->stream, inbuf, 1024) == NULL) {
        REDIS_STREAM_CLOSE_MARK_FAILED(redis_sock);
        zend_throw_exception(redis_exception_ce, "read error on connection", 
                             0 TSRMLS_CC);
        return NULL;
    }

    switch(inbuf[0]) {
        case '-':
            err_len = strlen(inbuf+1) - 2;
            redis_sock_set_err(redis_sock, inbuf+1, err_len);

            /* Filter our ERROR through the few that should actually throw */
            redis_error_throw(inbuf + 1, err_len TSRMLS_CC);

            /* Handle stale data error */
            if(memcmp(inbuf + 1, "-ERR SYNC ", 10) == 0) {
                zend_throw_exception(redis_exception_ce, 
                    "SYNC with master in progress", 0 TSRMLS_CC);
            }
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
            /* :123\r\n */
            *buf_len = strlen(inbuf) - 2;
            if(*buf_len >= 2) {
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

int
integer_length(int i) {
    int sz = 0;
    int ci = abs(i);
    while (ci > 0) {
        ci /= 10;
        sz++;
    }
    if (i == 0) { /* log 0 doesn't make sense. */
        sz = 1;
    } else if (i < 0) { /* allow for neg sign as well. */
        sz++;
    }
    return sz;
}

int
redis_cmd_format_header(char **ret, char *keyword, int arg_count) {
	/* Our return buffer */
	smart_string buf = {0};

	/* Keyword length */
	int l = strlen(keyword);

    smart_string_appendc(&buf, '*');
    smart_string_append_long(&buf, arg_count + 1);
    smart_string_appendl(&buf, _NL, sizeof(_NL) -1);
    smart_string_appendc(&buf, '$');
    smart_string_append_long(&buf, l);
    smart_string_appendl(&buf, _NL, sizeof(_NL) -1);
    smart_string_appendl(&buf, keyword, l);
    smart_string_appendl(&buf, _NL, sizeof(_NL) - 1);

	/* Set our return pointer */
	*ret = buf.c;

	/* Return the length */
	return buf.len;
}

int
redis_cmd_format_static(char **ret, char *keyword, char *format, ...)
{
    char *p = format;
    va_list ap;
    smart_string buf = {0};
    int l = strlen(keyword);
    zend_string *dbl_str;

    va_start(ap, format);

    /* add header */
    smart_string_appendc(&buf, '*');
    smart_string_append_long(&buf, strlen(format) + 1);
    smart_string_appendl(&buf, _NL, sizeof(_NL) - 1);
    smart_string_appendc(&buf, '$');
    smart_string_append_long(&buf, l);
    smart_string_appendl(&buf, _NL, sizeof(_NL) - 1);
    smart_string_appendl(&buf, keyword, l);
    smart_string_appendl(&buf, _NL, sizeof(_NL) - 1);

    while (*p) {
        smart_string_appendc(&buf, '$');

        switch(*p) {
            case 's': {
                char *val = va_arg(ap, char*);
                int val_len = va_arg(ap, int);
                smart_string_append_long(&buf, val_len);
                smart_string_appendl(&buf, _NL, sizeof(_NL) - 1);
                smart_string_appendl(&buf, val, val_len);
                }
                break;
            case 'f':
            case 'F': {
                double d = va_arg(ap, double);
                REDIS_DOUBLE_TO_STRING(dbl_str, d);
                smart_string_append_long(&buf, dbl_str->len);
                smart_string_appendl(&buf, _NL, sizeof(_NL) - 1);
                smart_string_appendl(&buf, dbl_str->val, dbl_str->len);
                zend_string_release(dbl_str);
            }
                break;

            case 'i':
            case 'd': {
                int i = va_arg(ap, int);
                char tmp[32];
                int tmp_len = snprintf(tmp, sizeof(tmp), "%d", i);
                smart_string_append_long(&buf, tmp_len);
                smart_string_appendl(&buf, _NL, sizeof(_NL) - 1);
                smart_string_appendl(&buf, tmp, tmp_len);
            }
                break;
            case 'l':
            case 'L': {
                long l = va_arg(ap, long);
                char tmp[32];
                int tmp_len = snprintf(tmp, sizeof(tmp), "%ld", l);
                smart_string_append_long(&buf, tmp_len);
                smart_string_appendl(&buf, _NL, sizeof(_NL) -1);
                smart_string_appendl(&buf, tmp, tmp_len);
            }
                break;
        }
        p++;
        smart_string_appendl(&buf, _NL, sizeof(_NL) - 1);
    }
    smart_string_0(&buf);

    *ret = buf.c;

    return buf.len;
}

/**
 * This command behave somehow like printf, except that strings need 2 
 * arguments:
 *      Their data and their size (strlen).
 *      Supported formats are: %d, %i, %s, %l
 */
int
redis_cmd_format(char **ret, char *format, ...) {

    smart_string buf = {0};
    va_list ap;
    char *p = format;
    zend_string *dbl_str;

    va_start(ap, format);

    while (*p) {
        if (*p == '%') {
            switch (*(++p)) {
                case 's': {
                    char *tmp = va_arg(ap, char*);
                    int tmp_len = va_arg(ap, int);
                    smart_string_appendl(&buf, tmp, tmp_len);
                }
                    break;

                case 'F':
                case 'f': {
                    double d = va_arg(ap, double);
                    REDIS_DOUBLE_TO_STRING(dbl_str, d);
                    smart_string_append_long(&buf, dbl_str->len);
                    smart_string_appendl(&buf, _NL, sizeof(_NL) - 1);
                    smart_string_appendl(&buf, dbl_str->val, dbl_str->len);
                    zend_string_release(dbl_str);
                }
                    break;

                case 'd':
                case 'i': {
                    int i = va_arg(ap, int);
                    char tmp[32];
                    int tmp_len = snprintf(tmp, sizeof(tmp), "%d", i);
                    smart_string_appendl(&buf, tmp, tmp_len);
                }
                    break;
            }
        } else {
            smart_string_appendc(&buf, *p);
        }

        p++;
    }

    smart_string_0(&buf);

    *ret = buf.c;

    return buf.len;
}

/*
 * Append a command sequence to a Redis command
 */
int redis_cmd_append_str(char **cmd, int cmd_len, char *append, int append_len) {
	/* Smart string buffer */
	smart_string buf = {0};

	/* Append the current command to our smart_string */
	smart_string_appendl(&buf, *cmd, cmd_len);

	/* Append our new command sequence */
	smart_string_appendc(&buf, '$');
	smart_string_append_long(&buf, append_len);
	smart_string_appendl(&buf, _NL, sizeof(_NL) -1);
	smart_string_appendl(&buf, append, append_len);
	smart_string_appendl(&buf, _NL, sizeof(_NL) -1);

	/* Free our old command */
	efree(*cmd);

	/* Set our return pointer */
	*cmd = buf.c;

	/* Return new command length */
	return buf.len;
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
int redis_cmd_append_sstr_dbl(smart_string *str, double value) {
    zend_string *dbl_str;
    int retval;

    /* Convert to double */
    REDIS_DOUBLE_TO_STRING(dbl_str, value);

    // Append the string
    retval = redis_cmd_append_sstr(str, dbl_str->val, dbl_str->len);

    /* Free our double string */
    zend_string_release(dbl_str);

    /* Return new length */
    return retval;
}

/*
 * Append an integer command to a Redis command
 */
int redis_cmd_append_int(char **cmd, int cmd_len, int append) {
    char int_buf[32];
    int int_len;

    // Conver to an int, capture length
    int_len = snprintf(int_buf, sizeof(int_buf), "%d", append);

	/* Return the new length */
	return redis_cmd_append_str(cmd, cmd_len, int_buf, int_len);
}

PHP_REDIS_API void redis_bulk_double_response(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock, zval *z_tab, void *ctx) {

    char *response;
    int response_len;
    double ret;

    if ((response = redis_sock_read(redis_sock, &response_len TSRMLS_CC)) == NULL) {
        IF_MULTI_OR_PIPELINE() {
            add_next_index_bool(z_tab, 0);
            return;
        } else {
            RETURN_FALSE;
        }
    }

    ret = atof(response);
    efree(response);
    IF_MULTI_OR_PIPELINE() {
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
        IF_MULTI_OR_PIPELINE() {
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
    IF_MULTI_OR_PIPELINE() {
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

    IF_MULTI_OR_PIPELINE() {
        add_next_index_zval(z_tab, z_ret);
    } else {
        RETVAL_ZVAL(z_ret, 0, 1);
    }
}

PHP_REDIS_API void
redis_parse_info_response(char *response, zval *z_ret)
{
    char *key, *value, *p, *cur, *pos;
    int is_numeric;

    array_init(z_ret);

    cur = response;
    while(1) {
        /* skip comments and empty lines */
        if(*cur == '#' || *cur == '\r') {
            if(!(cur = strchr(cur, '\n')))
                break;
            cur++;
            continue;
        }

        /* key */
        pos = strchr(cur, ':');
        if(pos == NULL) {
            break;
        }
        key = estrndup(cur, pos - cur);

        /* value */
        cur = pos + 1;
        pos = strchr(cur, '\r');
        if(pos == NULL) {
            efree(key);
            break;
        }
        value = estrndup(cur, pos - cur);
        pos += 2; /* \r, \n */
        cur = pos;

        is_numeric = 1;
        for(p = value; *p; ++p) {
            if(*p < '0' || *p > '9') {
                is_numeric = 0;
                break;
            }
        }

        if(is_numeric == 1) {
            add_assoc_long(z_ret, key, atol(value));
        } else {
            add_assoc_string(z_ret, key, value);
        }
        efree(value);
        efree(key);
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
    IF_MULTI_OR_PIPELINE() {
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
    IF_MULTI_OR_PIPELINE() {
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
        IF_MULTI_OR_PIPELINE() {
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
        IF_MULTI_OR_PIPELINE() {
            if(ret > LONG_MAX) { /* overflow */
                add_next_index_stringl(z_tab, response + 1, response_len - 1);
            } else {
                efree(response);
                add_next_index_long(z_tab, (long)ret);
            }
        } else {
            if(ret > LONG_MAX) { /* overflow */
                RETURN_STRINGL(response+1, response_len-1);
            } else {
                efree(response);
                RETURN_LONG((long)ret);
            }
        }
    } else {
        efree(response);
        IF_MULTI_OR_PIPELINE() {
          add_next_index_null(z_tab);
        } else {
            RETURN_FALSE;
        }
    }
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
    char inbuf[1024];
    int numElems;

    if(-1 == redis_check_eof(redis_sock, 0 TSRMLS_CC)) {
        return -1;
    }
    if(php_stream_gets(redis_sock->stream, inbuf, 1024) == NULL) {
        REDIS_STREAM_CLOSE_MARK_FAILED(redis_sock);
        zend_throw_exception(redis_exception_ce, "read error on connection", 0 TSRMLS_CC);
        return -1;
    }

    if(inbuf[0] != '*') {
        IF_MULTI_OR_PIPELINE() {
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

    IF_MULTI_OR_PIPELINE() {
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

PHP_REDIS_API void redis_1_response(INTERNAL_FUNCTION_PARAMETERS, 
                             RedisSock *redis_sock, zval *z_tab, void *ctx) 
{

    char *response;
    int response_len;
    char ret;

    if ((response = redis_sock_read(redis_sock, &response_len TSRMLS_CC)) 
                                    == NULL) 
    {
        IF_MULTI_OR_PIPELINE() {
            add_next_index_bool(z_tab, 0);
            return;
        } else {
            RETURN_FALSE;
        }
    }
    ret = response[1];
    efree(response);

    IF_MULTI_OR_PIPELINE() {
        if(ret == '1') {
            add_next_index_bool(z_tab, 1);
        } else {
            add_next_index_bool(z_tab, 0);
        }
    } else {
        if (ret == '1') {
            RETURN_TRUE;
        } else {
            RETURN_FALSE;
        }
    }
}

PHP_REDIS_API void redis_string_response(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock, zval *z_tab, void *ctx) {

    char *response;
    int response_len;

    if ((response = redis_sock_read(redis_sock, &response_len TSRMLS_CC)) 
                                    == NULL) 
    {
        IF_MULTI_OR_PIPELINE() {
            add_next_index_bool(z_tab, 0);
        return;
        }
        RETURN_FALSE;
    }
    IF_MULTI_OR_PIPELINE() {
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
        IF_MULTI_OR_PIPELINE() {
            add_next_index_bool(z_tab, 0);
        return;
        }
        RETURN_FALSE;
    }
    IF_MULTI_OR_PIPELINE() {
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
        IF_MULTI_OR_PIPELINE() {
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

    IF_MULTI_OR_PIPELINE() {
        add_next_index_zval(z_tab, z_result);
    } else {
        RETVAL_ZVAL(z_result, 0, 1);
    }
}

/**
 * redis_sock_create
 */
PHP_REDIS_API RedisSock* 
redis_sock_create(char *host, int host_len, unsigned short port, double timeout, 
                  int persistent, char *persistent_id, long retry_interval,
                  zend_bool lazy_connect)
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
    redis_sock->read_timeout = timeout;

    redis_sock->serializer = REDIS_SERIALIZER_NONE;
    redis_sock->mode = ATOMIC;
    redis_sock->head = NULL;
    redis_sock->current = NULL;
    redis_sock->pipeline_head = NULL;
    redis_sock->pipeline_current = NULL;

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

    if(tv.tv_sec != 0 || tv.tv_usec != 0) {
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
redis_sock_server_open(RedisSock *redis_sock, int force_connect TSRMLS_DC)
{
    int res = -1;

    switch (redis_sock->status) {
        case REDIS_SOCK_STATUS_DISCONNECTED:
            return redis_sock_connect(redis_sock TSRMLS_CC);
        case REDIS_SOCK_STATUS_CONNECTED:
            res = 0;
        break;
        case REDIS_SOCK_STATUS_UNKNOWN:
            if (force_connect > 0 && redis_sock_connect(redis_sock TSRMLS_CC) < 0) {
                res = -1;
            } else {
                res = 0;

                redis_sock->status = REDIS_SOCK_STATUS_CONNECTED;
            }
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
            if(redis_sock->stream && !redis_sock->persistent) {
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

    cmd_len = redis_cmd_format_static(&cmd, "DISCARD", "");

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
    char inbuf[1024];
    int numElems, err_len;

    if(-1 == redis_check_eof(redis_sock, 0 TSRMLS_CC)) {
        return -1;
    }
    if(php_stream_gets(redis_sock->stream, inbuf, 1024) == NULL) {
        REDIS_STREAM_CLOSE_MARK_FAILED(redis_sock);
        zend_throw_exception(redis_exception_ce, "read error on connection", 0 
            TSRMLS_CC);
        return -1;
    }

    if(inbuf[0] != '*') {
        IF_MULTI_OR_PIPELINE() {
            add_next_index_bool(z_tab, 0);
        } else {
            if (inbuf[0] == '-') {
                err_len = strlen(inbuf+1) - 2;
                redis_sock_set_err(redis_sock, inbuf+1, err_len);
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
    
    IF_MULTI_OR_PIPELINE() {
        add_next_index_zval(z_tab, z_multi_result);
    } else {
        RETVAL_ZVAL(z_multi_result, 0, 1);
    }
    /*zval_copy_ctor(return_value); */
    return 0;
}

/* Like multibulk reply, but don't touch the values, they won't be unserialized
 * (this is used by HKEYS). */
PHP_REDIS_API int redis_mbulk_reply_raw(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock, zval *z_tab, void *ctx)
{
    char inbuf[1024];
    int numElems, err_len;

    if(-1 == redis_check_eof(redis_sock, 0 TSRMLS_CC)) {
        return -1;
    }
    if(php_stream_gets(redis_sock->stream, inbuf, 1024) == NULL) {
        REDIS_STREAM_CLOSE_MARK_FAILED(redis_sock);
        zend_throw_exception(redis_exception_ce, "read error on connection", 0 TSRMLS_CC);
        return -1;
    }

    if(inbuf[0] != '*') {
        IF_MULTI_OR_PIPELINE() {
            add_next_index_bool(z_tab, 0);
        } else {
            if (inbuf[0] == '-') {
                err_len = strlen(inbuf+1) - 2;
                redis_sock_set_err(redis_sock, inbuf+1, err_len);
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

    IF_MULTI_OR_PIPELINE() {
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
    int len;

    while(count > 0) {
        line = redis_sock_read(redis_sock, &len TSRMLS_CC);
        if (line != NULL) {
            zval zv, *z = &zv;
            int unwrap;

            /* We will attempt unserialization, if we're unserializing everything,
             * or if we're unserializing keys and we're on a key, or we're
             * unserializing values and we're on a value! */
            unwrap = unserialize == UNSERIALIZE_ALL ||
                (unserialize == UNSERIALIZE_KEYS && count % 2 == 0) ||
                (unserialize == UNSERIALIZE_VALS && count % 2 != 0);

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
        } else {
            add_next_index_bool(z_tab, 0);
        }

        count--;
    }
}

/* Specialized multibulk processing for HMGET where we need to pair requested
 * keys with their returned values */
PHP_REDIS_API int redis_mbulk_reply_assoc(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock, zval *z_tab, void *ctx)
{
    char inbuf[1024], *response;
    int response_len;
    int i, numElems;

    zval *z_keys = ctx;

    if(-1 == redis_check_eof(redis_sock, 0 TSRMLS_CC)) {
        return -1;
    }
    if(php_stream_gets(redis_sock->stream, inbuf, 1024) == NULL) {
        REDIS_STREAM_CLOSE_MARK_FAILED(redis_sock);
        zend_throw_exception(redis_exception_ce, "read error on connection", 0 TSRMLS_CC);
        return -1;
    }

    if(inbuf[0] != '*') {
        IF_MULTI_OR_PIPELINE() {
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

    IF_MULTI_OR_PIPELINE() {
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
    char inbuf[1024];
    size_t line_size;

	/* Attempt to read our single line reply */
	if(redis_sock_gets(redis_sock, inbuf, sizeof(inbuf), &line_size TSRMLS_CC) < 0) {
		return -1;
	}

    // If this is an error response, check if it is a SYNC error, and throw in 
    // that case
    if(reply_type == TYPE_ERR) {
        /* Handle throwable errors */
        redis_error_throw(inbuf, line_size TSRMLS_CC);

		/* Set our last error */
		redis_sock_set_err(redis_sock, inbuf, line_size);

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

	IF_MULTI_OR_PIPELINE() {
		add_next_index_zval(z_tab, z_ret);
	} else {
		/* Set our return value */
        RETVAL_ZVAL(z_ret, 0, 1);
	}

	/* Success */
	return 0;
}

/* vim: set tabstop=4 softtabstop=4 expandtab shiftwidth=4: */
