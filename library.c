#include "php_redis.h"

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "common.h"
#include "php_network.h"
#include <sys/types.h>

#ifdef HAVE_REDIS_IGBINARY
#include "igbinary/igbinary.h"
#endif
#ifdef HAVE_REDIS_MSGPACK
#include "msgpack/php_msgpack.h"
#endif

#ifdef HAVE_REDIS_LZF
#include <lzf.h>

    #ifndef LZF_MARGIN
        #define LZF_MARGIN 128
    #endif
#endif

#ifdef HAVE_REDIS_ZSTD
#include <zstd.h>
#endif

#ifdef HAVE_REDIS_LZ4
#include <lz4.h>
#include <lz4hc.h>

/* uint8_t crf + int length */
#define REDIS_LZ4_HDR_SIZE (sizeof(uint8_t) + sizeof(int))
#if defined(LZ4HC_CLEVEL_MAX)
/* version >= 1.7.5 */
#define REDIS_LZ4_MAX_CLEVEL LZ4HC_CLEVEL_MAX

#elif defined (LZ4HC_MAX_CLEVEL)
/* version >= 1.7.3 */
#define REDIS_LZ4_MAX_CLEVEL LZ4HC_MAX_CLEVEL

#else
/* older versions */
#define REDIS_LZ4_MAX_CLEVEL 12
#endif
#endif

#include <zend_exceptions.h>
#include "php_redis.h"
#include "library.h"
#include "redis_commands.h"

#ifdef HAVE_REDIS_JSON
#include <ext/json/php_json.h>
#endif

#include <ext/standard/php_rand.h>
#include <ext/hash/php_hash.h>

#define UNSERIALIZE_NONE 0
#define UNSERIALIZE_KEYS 1
#define UNSERIALIZE_VALS 2
#define UNSERIALIZE_ALL  3

#define SCORE_DECODE_NONE 0
#define SCORE_DECODE_INT  1
#define SCORE_DECODE_DOUBLE 2

#ifndef PHP_WIN32
    #include <netinet/tcp.h> /* TCP_NODELAY */
    #include <sys/socket.h>  /* SO_KEEPALIVE */
#else
    #include <winsock.h>
#endif

extern zend_class_entry *redis_ce;
extern zend_class_entry *redis_exception_ce;

extern int le_redis_pconnect;

static int redis_mbulk_reply_zipped_raw_variant(RedisSock *redis_sock, zval *zret, int count);

/* Register a persistent resource in a a way that works for every PHP 7 version. */
void redis_register_persistent_resource(zend_string *id, void *ptr, int le_id) {
#if PHP_VERSION_ID < 70300
    zend_resource res;
    res.type = le_id;
    res.ptr = ptr;

    zend_hash_str_update_mem(&EG(persistent_list), ZSTR_VAL(id), ZSTR_LEN(id), &res, sizeof(res));
#else
    zend_register_persistent_resource(ZSTR_VAL(id), ZSTR_LEN(id), ptr, le_id);
#endif
}

static ConnectionPool *
redis_sock_get_connection_pool(RedisSock *redis_sock)
{
    ConnectionPool *pool;
    zend_resource *le;
    zend_string *persistent_id;

    /* Generate our unique pool id depending on configuration */
    persistent_id = redis_pool_spprintf(redis_sock, INI_STR("redis.pconnect.pool_pattern"));

    /* Return early if we can find the pool */
    if ((le = zend_hash_find_ptr(&EG(persistent_list), persistent_id))) {
        zend_string_release(persistent_id);
        return le->ptr;
    }

    /* Create the pool and store it in our persistent list */
    pool = pecalloc(1, sizeof(*pool), 1);
    zend_llist_init(&pool->list, sizeof(php_stream *), NULL, 1);
    redis_register_persistent_resource(persistent_id, pool, le_redis_pconnect);

    zend_string_release(persistent_id);
    return pool;
}

/* Helper to reselect the proper DB number when we reconnect */
static int reselect_db(RedisSock *redis_sock) {
    char *cmd, *response;
    int cmd_len, response_len;

    cmd_len = redis_spprintf(redis_sock, NULL, &cmd, "SELECT", "d",
                             redis_sock->dbNumber);

    if (redis_sock_write(redis_sock, cmd, cmd_len) < 0) {
        efree(cmd);
        return -1;
    }

    efree(cmd);

    if ((response = redis_sock_read(redis_sock, &response_len)) == NULL) {
        return -1;
    }

    if (strncmp(response, "+OK", 3)) {
        efree(response);
        return -1;
    }

    efree(response);
    return 0;
}

/* Attempt to read a single +OK response */
static int redis_sock_read_ok(RedisSock *redis_sock) {
    char buf[64];
    size_t len;

    if (redis_sock_read_single_line(redis_sock, buf, sizeof(buf), &len, 0) < 0)
        return FAILURE;

    return REDIS_STRCMP_STATIC(buf, len, "OK") ? SUCCESS : FAILURE;
}

/* Append an AUTH command to a smart string if neccessary.  This will either
 * append the new style AUTH <user> <password>, old style AUTH <password>, or
 * append no command at all.  Function returns 1 if we appended a command
 * and 0 otherwise. */
static int redis_sock_append_auth(RedisSock *redis_sock, smart_string *str) {
    /* We need a password at least */
    if (redis_sock->pass == NULL)
        return 0;

    REDIS_CMD_INIT_SSTR_STATIC(str, !!redis_sock->user + !!redis_sock->pass, "AUTH");

    if (redis_sock->user)
        redis_cmd_append_sstr_zstr(str, redis_sock->user);

    redis_cmd_append_sstr_zstr(str, redis_sock->pass);

    /* We appended a command */
    return 1;
}

PHP_REDIS_API void
redis_sock_copy_auth(RedisSock *dst, RedisSock *src) {
    redis_sock_set_auth(dst, src->user, src->pass);
}

PHP_REDIS_API void
redis_sock_set_auth(RedisSock *redis_sock, zend_string *user, zend_string *pass)
{
    /* Release existing user/pass */
    redis_sock_free_auth(redis_sock);

    /* Set new user/pass */
    redis_sock->user = user ? zend_string_copy(user) : NULL;
    redis_sock->pass = pass ? zend_string_copy(pass) : NULL;
}


PHP_REDIS_API void
redis_sock_set_auth_zval(RedisSock *redis_sock, zval *zv) {
    zend_string *user, *pass;

    if (redis_extract_auth_info(zv, &user, &pass) == FAILURE)
        return;

    redis_sock_set_auth(redis_sock, user, pass);

    if (user) zend_string_release(user);
    if (pass) zend_string_release(pass);
}

PHP_REDIS_API void
redis_sock_free_auth(RedisSock *redis_sock) {
    if (redis_sock->user) {
        zend_string_release(redis_sock->user);
        redis_sock->user = NULL;
    }

    if (redis_sock->pass) {
        zend_string_release(redis_sock->pass);
        redis_sock->pass = NULL;
    }
}

PHP_REDIS_API char *
redis_sock_auth_cmd(RedisSock *redis_sock, int *cmdlen) {
    char *cmd;

    /* AUTH requires at least a password */
    if (redis_sock->pass == NULL)
        return NULL;

    if (redis_sock->user) {
        *cmdlen = redis_spprintf(redis_sock, NULL, &cmd, "AUTH", "SS", redis_sock->user, redis_sock->pass);
    } else {
        *cmdlen = redis_spprintf(redis_sock, NULL, &cmd, "AUTH", "S", redis_sock->pass);
    }

    return cmd;
}

/* Send Redis AUTH and process response */
PHP_REDIS_API int redis_sock_auth(RedisSock *redis_sock) {
    char *cmd;
    int cmdlen, rv = FAILURE;

    if ((cmd = redis_sock_auth_cmd(redis_sock, &cmdlen)) == NULL)
        return SUCCESS;

    if (redis_sock_write(redis_sock, cmd, cmdlen) < 0)
        goto cleanup;

    rv = redis_sock_read_ok(redis_sock) == SUCCESS ? SUCCESS : FAILURE;
cleanup:
    efree(cmd);
    return rv;
}

/* Helper function and macro to test a RedisSock error prefix. */
#define REDIS_SOCK_ERRCMP_STATIC(rs, s) redis_sock_errcmp(rs, s, sizeof(s)-1)
static int redis_sock_errcmp(RedisSock *redis_sock, const char *err, size_t errlen) {
    return ZSTR_LEN(redis_sock->err) >= errlen &&
           memcmp(ZSTR_VAL(redis_sock->err), err, errlen) == 0;
}

/* Helper function that will throw an exception for a small number of ERR codes
 * returned by Redis.  Typically we just return FALSE to the caller in the event
 * of an ERROR reply, but for the following error types:
 *    1) MASTERDOWN
 *    2) AUTH
 *    3) LOADING
 */
static void
redis_error_throw(RedisSock *redis_sock)
{
    /* Short circuit if we have no redis_sock or any error */
    if (redis_sock == NULL || redis_sock->err == NULL)
        return;

    /* Redis 6 decided to add 'ERR AUTH' which has a normal 'ERR' prefix
     * but is actually an authentication error that we will want to throw
     * an exception for, so just short circuit if this is any other 'ERR'
     * prefixed error. */
    if (REDIS_SOCK_ERRCMP_STATIC(redis_sock, "ERR") &&
        !REDIS_SOCK_ERRCMP_STATIC(redis_sock, "ERR AUTH")) return;

    /* We may want to flip this logic and check for MASTERDOWN, AUTH,
     * and LOADING but that may have side effects (esp for things like
     * Disque) */
    if (!REDIS_SOCK_ERRCMP_STATIC(redis_sock, "NOSCRIPT") &&
        !REDIS_SOCK_ERRCMP_STATIC(redis_sock, "NOQUORUM") &&
        !REDIS_SOCK_ERRCMP_STATIC(redis_sock, "NOGOODSLAVE") &&
        !REDIS_SOCK_ERRCMP_STATIC(redis_sock, "WRONGTYPE") &&
        !REDIS_SOCK_ERRCMP_STATIC(redis_sock, "BUSYGROUP") &&
        !REDIS_SOCK_ERRCMP_STATIC(redis_sock, "NOGROUP"))
    {
        REDIS_THROW_EXCEPTION(ZSTR_VAL(redis_sock->err), 0);
    }
}

PHP_REDIS_API int
redis_check_eof(RedisSock *redis_sock, int no_throw)
{
    int count;
    char *errmsg;

    if (!redis_sock || !redis_sock->stream || redis_sock->status == REDIS_SOCK_STATUS_FAILED) {
        if (!no_throw) {
            REDIS_THROW_EXCEPTION( "Connection closed", 0);
        }
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
        errmsg = "Connection lost and socket is in MULTI/watching mode";
    } else {
        errmsg = "Connection lost";
        /* TODO: configurable max retry count */
        for (count = 0; count < 10; ++count) {
            /* close existing stream before reconnecting */
            if (redis_sock->stream) {
                redis_sock_disconnect(redis_sock, 1);
            }
            // Wait for a while before trying to reconnect
            if (redis_sock->retry_interval) {
                // Random factor to avoid having several (or many) concurrent connections trying to reconnect at the same time
                long retry_interval = (count ? redis_sock->retry_interval : (php_rand() % redis_sock->retry_interval));
                usleep(retry_interval);
            }
            /* reconnect */
            if (redis_sock_connect(redis_sock) == 0) {
                /* check for EOF again. */
                errno = 0;
                if (php_stream_eof(redis_sock->stream) == 0) {
                    if (redis_sock_auth(redis_sock) != SUCCESS) {
                        errmsg = "AUTH failed while reconnecting";
                        break;
                    }

                    redis_sock->status = REDIS_SOCK_STATUS_READY;
                    /* If we're using a non-zero db, reselect it */
                    if (redis_sock->dbNumber && reselect_db(redis_sock) != 0) {
                        errmsg = "SELECT failed while reconnecting";
                        break;
                    }
                    /* Success */
                    return 0;
                }
            }
        }
    }
    /* close stream and mark socket as failed */
    redis_sock_disconnect(redis_sock, 1);
    redis_sock->status = REDIS_SOCK_STATUS_FAILED;
    if (!no_throw) {
        REDIS_THROW_EXCEPTION( errmsg, 0);
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
    if(redis_read_reply_type(redis_sock, &reply_type, &reply_info)<0
       || reply_type != TYPE_MULTIBULK || reply_info != 2)
    {
        return -1;
    }

    /* The BULK response iterator */
    if(redis_read_reply_type(redis_sock, &reply_type, &reply_info)<0
       || reply_type != TYPE_BULK)
    {
        return -1;
    }

    /* Attempt to read the iterator */
    if(!(p_iter = redis_sock_read_bulk_reply(redis_sock, reply_info))) {
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

    zval z_ret, z_args[4];
    sctx->cb.retval = &z_ret;
    sctx->cb.params = z_args;

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
        z_args[0] = *getThis();
        if(is_pmsg) {
            z_args[1] = *z_pat;
            z_args[2] = *z_chan;
            z_args[3] = *z_data;
        } else {
            z_args[1] = *z_chan;
            z_args[2] = *z_data;
        }

        // Set arg count
        sctx->cb.param_count = tab_idx;

        // Execute callback
        if(zend_call_function(&(sctx->cb), &(sctx->cb_cache))
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

    ZVAL_NULL(z_tab);
    if (redis_sock_gets(redis_sock, inbuf, sizeof(inbuf) - 1, &len) < 0) {
        return NULL;
    }

    if(inbuf[0] != '*') {
        return NULL;
    }
    numElems = atoi(inbuf+1);

    array_init(z_tab);

    redis_mbulk_reply_loop(redis_sock, z_tab, numElems, UNSERIALIZE_ALL);

    return z_tab;
}

/**
 * redis_sock_read_bulk_reply
 */
PHP_REDIS_API char *
redis_sock_read_bulk_reply(RedisSock *redis_sock, int bytes)
{
    int offset = 0, nbytes;
    char *reply;
    size_t got;

    if (-1 == bytes || -1 == redis_check_eof(redis_sock, 0)) {
        return NULL;
    }

    nbytes = bytes + 2;
    /* Allocate memory for string */
    reply = emalloc(nbytes);

    /* Consume bulk string */
    while (offset < nbytes) {
        got = php_stream_read(redis_sock->stream, reply + offset, nbytes - offset);
        if (got == 0 && php_stream_eof(redis_sock->stream)) break;
        offset += got;
    }

    /* Protect against reading too few bytes */
    if (offset < nbytes) {
        /* Error or EOF */
        REDIS_THROW_EXCEPTION("socket error on read socket", 0);
        efree(reply);
        return NULL;
    }

    /* Null terminate reply string */
    reply[bytes] = '\0';

    return reply;
}

/**
 * redis_sock_read
 */
PHP_REDIS_API char *
redis_sock_read(RedisSock *redis_sock, int *buf_len)
{
    char inbuf[4096];
    size_t len;

    *buf_len = 0;
    if (redis_sock_gets(redis_sock, inbuf, sizeof(inbuf) - 1, &len) < 0) {
        return NULL;
    }

    switch(inbuf[0]) {
        case '-':
            redis_sock_set_err(redis_sock, inbuf+1, len);

            /* Filter our ERROR through the few that should actually throw */
            redis_error_throw(redis_sock);

            return NULL;
        case '$':
            *buf_len = atoi(inbuf + 1);
            return redis_sock_read_bulk_reply(redis_sock, *buf_len);

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
            zend_throw_exception_ex(redis_exception_ce, 0,
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

static zend_string *redis_hash_auth(zend_string *user, zend_string *pass) {
    zend_string *algo, *hex;
    smart_str salted = {0};
    const php_hash_ops *ops;
    unsigned char *digest;
    void *ctx;

    /* No op if there is not username/password */
    if (user == NULL && pass == NULL)
        return NULL;

    /* Theoretically inpossible but check anyway */
    algo = zend_string_init("sha256", sizeof("sha256") - 1, 0);
    if ((ops = redis_hash_fetch_ops(algo)) == NULL) {
        zend_string_release(algo);
        return NULL;
    }

    /* Hash username + password with our salt global */
    smart_str_alloc(&salted, 256, 0);
    if (user) smart_str_append_ex(&salted, user, 0);
    if (pass) smart_str_append_ex(&salted, pass, 0);
    smart_str_appendl_ex(&salted, REDIS_G(salt), sizeof(REDIS_G(salt)), 0);

    ctx = emalloc(ops->context_size);
#if PHP_VERSION_ID >= 80100
    ops->hash_init(ctx,NULL);
#else
    ops->hash_init(ctx);
#endif
    ops->hash_update(ctx, (const unsigned char *)ZSTR_VAL(salted.s), ZSTR_LEN(salted.s));

    digest = emalloc(ops->digest_size);
    ops->hash_final(digest, ctx);
    efree(ctx);

    hex = zend_string_safe_alloc(ops->digest_size, 2, 0, 0);
    php_hash_bin2hex(ZSTR_VAL(hex), digest, ops->digest_size);
    ZSTR_VAL(hex)[2 * ops->digest_size] = 0;

    efree(digest);
    zend_string_release(algo);
    smart_str_free(&salted);

    return hex;
}

static void append_auth_hash(smart_str *dst, zend_string *user, zend_string *pass) {
    zend_string *s;

    if ((s = redis_hash_auth(user, pass)) != NULL) {
        smart_str_appendc(dst, ':');
        smart_str_append_ex(dst, s, 0);
        zend_string_release(s);
    }
}

/* A printf like function to generate our connection pool hash value. */
PHP_REDIS_API zend_string *
redis_pool_spprintf(RedisSock *redis_sock, char *fmt, ...) {
    smart_str str = {0};

    smart_str_alloc(&str, 128, 0);

    /* We always include phpredis_<host>:<port> */
    smart_str_appendl(&str, "phpredis_", sizeof("phpredis_") - 1);
    smart_str_append_ex(&str, redis_sock->host, 0);
    smart_str_appendc(&str, ':');
    smart_str_append_long(&str, (zend_long)redis_sock->port);

    /* Short circuit if we don't have a pattern */
    if (fmt == NULL) {
        smart_str_0(&str);
        return str.s;
    }

    while (*fmt) {
        switch (*fmt) {
            case 'i':
                if (redis_sock->persistent_id) {
                    smart_str_appendc(&str, ':');
                    smart_str_append_ex(&str, redis_sock->persistent_id, 0);
                }
                break;
            case 'u':
                smart_str_appendc(&str, ':');
                if (redis_sock->user) {
                    smart_str_append_ex(&str, redis_sock->user, 0);
                }
                break;
            case 'p':
                append_auth_hash(&str, NULL, redis_sock->pass);
                break;
            case 'a':
                append_auth_hash(&str, redis_sock->user, redis_sock->pass);
                break;
            default:
                /* Maybe issue a php_error_docref? */
                break;
        }

        fmt++;
    }

    smart_str_0(&str);
    return str.s;
}

/* A printf like method to construct a Redis RESP command.  It has been extended
 * to take a few different format specifiers that are convenient to phpredis.
 *
 * s - C string followed by length as a
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
redis_spprintf(RedisSock *redis_sock, short *slot, char **ret, char *kw, char *fmt, ...) {
    smart_string cmd = {0};
    va_list ap;
    union resparg arg;
    char *dup;
    int argfree;
    size_t arglen;

    va_start(ap, fmt);

    /* Header */
    redis_cmd_init_sstr(&cmd, strlen(fmt), kw, strlen(kw));

    while (*fmt) {
        switch (*fmt) {
            case 's':
                arg.str = va_arg(ap, char*);
                arglen = va_arg(ap, size_t);
                redis_cmd_append_sstr(&cmd, arg.str, arglen);
                break;
            case 'S':
                arg.zstr = va_arg(ap, zend_string*);
                redis_cmd_append_sstr(&cmd, ZSTR_VAL(arg.zstr), ZSTR_LEN(arg.zstr));
                break;
            case 'k':
                arg.str = va_arg(ap, char*);
                arglen = va_arg(ap, size_t);
                argfree = redis_key_prefix(redis_sock, &arg.str, &arglen);
                redis_cmd_append_sstr(&cmd, arg.str, arglen);
                if (slot) *slot = cluster_hash_key(arg.str, arglen);
                if (argfree) efree(arg.str);
                break;
            case 'v':
                arg.zv = va_arg(ap, zval*);
                argfree = redis_pack(redis_sock, arg.zv, &dup, &arglen);
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
    /* varargs cleanup */
    va_end(ap);

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
 * Append a 64-bit integer to our command
 */
int redis_cmd_append_sstr_i64(smart_string *str, int64_t append) {
    char nbuf[64];
    int len = snprintf(nbuf, sizeof(nbuf), "%" PRId64, append);
    return redis_cmd_append_sstr(str, nbuf, len);
}

/*
 * Append a double to a smart string command
 */
int
redis_cmd_append_sstr_dbl(smart_string *str, double value)
{
    char tmp[64], *p;
    int len;

    /* Convert to string */
    len = snprintf(tmp, sizeof(tmp), "%.17g", value);

    /* snprintf depends on locale, replace comma with point */
    if ((p = strchr(tmp, ',')) != NULL) *p = '.';

    // Append the string
    return redis_cmd_append_sstr(str, tmp, len);
}

/* Append a zval to a redis command.  The value will be serialized if we are
 * configured to do that */
int redis_cmd_append_sstr_zval(smart_string *str, zval *z, RedisSock *redis_sock) {
    char *val;
    size_t vallen;
    int valfree, retval;

    valfree = redis_pack(redis_sock, z, &val, &vallen);
    retval = redis_cmd_append_sstr(str, val, vallen);
    if (valfree) efree(val);

    return retval;
}

int redis_cmd_append_sstr_zstr(smart_string *str, zend_string *zstr) {
    return redis_cmd_append_sstr(str, ZSTR_VAL(zstr), ZSTR_LEN(zstr));
}

/* Append a string key to a redis command.  This function takes care of prefixing the key
 * for the caller and setting the slot argument if it is passed non null */
int redis_cmd_append_sstr_key(smart_string *str, char *key, size_t len, RedisSock *redis_sock, short *slot) {
    int valfree, retval;

    valfree = redis_key_prefix(redis_sock, &key, &len);
    if (slot) *slot = cluster_hash_key(key, len);
    retval = redis_cmd_append_sstr(str, key, len);
    if (valfree) efree(key);

    return retval;
}

/* Append an array key to a redis smart string command.  This function
 * handles the boilerplate conditionals around string or integer keys */
int redis_cmd_append_sstr_arrkey(smart_string *cmd, zend_string *kstr, zend_ulong idx)
{
    char *arg, kbuf[128];
    int len;

    if (kstr) {
        len = ZSTR_LEN(kstr);
        arg = ZSTR_VAL(kstr);
    } else {
        len = snprintf(kbuf, sizeof(kbuf), "%ld", (long)idx);
        arg = (char*)kbuf;
    }

    return redis_cmd_append_sstr(cmd, arg, len);
}

PHP_REDIS_API int
redis_bulk_double_response(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock, zval *z_tab, void *ctx) {

    char *response;
    int response_len;
    double ret;

    if ((response = redis_sock_read(redis_sock, &response_len)) == NULL) {
        if (IS_ATOMIC(redis_sock)) {
            RETVAL_FALSE;
        } else {
            add_next_index_bool(z_tab, 0);
        }
        return FAILURE;
    }

    ret = atof(response);
    efree(response);
    if (IS_ATOMIC(redis_sock)) {
        RETVAL_DOUBLE(ret);
    } else {
        add_next_index_double(z_tab, ret);
    }

    return SUCCESS;
}

PHP_REDIS_API int redis_type_response(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock, zval *z_tab, void *ctx) {
    char *response;
    int response_len;
    long l;

    if ((response = redis_sock_read(redis_sock, &response_len)) == NULL) {
        if (IS_ATOMIC(redis_sock)) {
            RETVAL_FALSE;
        } else {
            add_next_index_bool(z_tab, 0);
        }
        return FAILURE;
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
    } else if (strncmp(response, "+stream", 7) == 0) {
        l = REDIS_STREAM;
    } else {
        l = REDIS_NOT_FOUND;
    }

    efree(response);
    if (IS_ATOMIC(redis_sock)) {
        RETVAL_LONG(l);
    } else {
        add_next_index_long(z_tab, l);
    }

    return SUCCESS;
}

PHP_REDIS_API int redis_info_response(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock, zval *z_tab, void *ctx) {
    char *response;
    int response_len;
    zval z_ret;

    /* Read bulk response */
    if ((response = redis_sock_read(redis_sock, &response_len)) == NULL) {
        RETVAL_FALSE;
        return FAILURE;
    }

    /* Parse it into a zval array */
    ZVAL_UNDEF(&z_ret);
    redis_parse_info_response(response, &z_ret);

    /* Free source response */
    efree(response);

    if (IS_ATOMIC(redis_sock)) {
        RETVAL_ZVAL(&z_ret, 0, 1);
    } else {
        add_next_index_zval(z_tab, &z_ret);
    }

    return SUCCESS;
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
PHP_REDIS_API int
redis_client_list_reply(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock, zval *z_tab, void *ctx) {
    char *resp;
    int resp_len;
    zval z_ret;

    /* Make sure we can read the bulk response from Redis */
    if ((resp = redis_sock_read(redis_sock, &resp_len)) == NULL) {
        RETVAL_FALSE;
        return FAILURE;
    }

    /* Parse it out */
    redis_parse_client_list_response(resp, &z_ret);

    /* Free our response */
    efree(resp);

    /* Return or append depending if we're atomic */
    if (IS_ATOMIC(redis_sock)) {
        RETVAL_ZVAL(&z_ret, 0, 1);
    } else {
        add_next_index_zval(z_tab, &z_ret);
    }

    return SUCCESS;
}

PHP_REDIS_API void
redis_parse_client_list_response(char *response, zval *z_ret)
{
    char *p, *lpos, *kpos = NULL, *vpos = NULL, *p2, *key, *value;
    int klen = 0, done = 0, is_numeric;
    zval z_sub_result;

    /* Allocate for response and our user */
    array_init(z_ret);
    array_init(&z_sub_result);

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
                        add_assoc_long(&z_sub_result, key, atol(value));
                    } else {
                        add_assoc_string(&z_sub_result, key, value);
                    }
                    efree(value);
                    // If we hit a '\n', then we can add this user to our list
                    if(*p == '\n') {
                        /* Add our user */
                        add_next_index_zval(z_ret, &z_sub_result);

                        /* If we have another user, make another one */
                        if(*(p+1) != '\0') {
                            array_init(&z_sub_result);
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

PHP_REDIS_API int
redis_zdiff_response(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock, zval *z_tab, void *ctx)
{
    if (ctx == NULL) {
        return redis_mbulk_reply_raw(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock, z_tab, NULL);
    }
    return redis_mbulk_reply_zipped_keys_dbl(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock, z_tab, NULL);
}

PHP_REDIS_API int
redis_set_response(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock, zval *z_tab, void *ctx)
{
    if (ctx == NULL) {
        return redis_boolean_response(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock, z_tab, NULL);
    }
    return redis_string_response(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock, z_tab, NULL);
}

PHP_REDIS_API int
redis_boolean_response_impl(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock,
                            zval *z_tab, void *ctx,
                            SuccessCallback success_callback)
{

    char *response;
    int response_len;
    zend_bool ret = 0;

    if ((response = redis_sock_read(redis_sock, &response_len)) != NULL) {
        ret = (*response == '+');
        efree(response);
    }

    if (ret && success_callback != NULL) {
        success_callback(redis_sock);
    }
    if (IS_ATOMIC(redis_sock)) {
        RETVAL_BOOL(ret);
    } else {
        add_next_index_bool(z_tab, ret);
    }

    return ret ? SUCCESS : FAILURE;
}

PHP_REDIS_API int redis_boolean_response(INTERNAL_FUNCTION_PARAMETERS,
                                   RedisSock *redis_sock, zval *z_tab,
                                   void *ctx)
{
    return redis_boolean_response_impl(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock,
                                       z_tab, ctx, NULL);
}

PHP_REDIS_API int redis_long_response(INTERNAL_FUNCTION_PARAMETERS,
                                      RedisSock *redis_sock, zval * z_tab,
                                      void *ctx)
{

    char *response;
    int response_len;

    if ((response = redis_sock_read(redis_sock, &response_len)) == NULL) {
        if (IS_ATOMIC(redis_sock)) {
            RETVAL_FALSE;
        } else {
            add_next_index_bool(z_tab, 0);
        }

        return FAILURE;
    }

    if(response[0] == ':') {
        int64_t ret = phpredis_atoi64(response + 1);

        if (IS_ATOMIC(redis_sock)) {
            if(ret > LONG_MAX) { /* overflow */
                RETVAL_STRINGL(response + 1, response_len - 1);
            } else {
                RETVAL_LONG((long)ret);
            }
        } else {
            if(ret > LONG_MAX) { /* overflow */
                add_next_index_stringl(z_tab, response + 1, response_len - 1);
            } else {
                add_next_index_long(z_tab, (long)ret);
            }
        }
    } else {
        if (IS_ATOMIC(redis_sock)) {
            RETVAL_FALSE;
        } else {
            add_next_index_null(z_tab);
        }
        efree(response);
        return FAILURE;
    }

    efree(response);
    return SUCCESS;
}

/* Helper method to convert [key, value, key, value] into [key => value,
 * key => value] when returning data to the caller.  Depending on our decode
 * flag we'll convert the value data types */
static void array_zip_values_and_scores(RedisSock *redis_sock, zval *z_tab,
                                        int decode)
{

    zval z_ret, z_sub;
    HashTable *keytable;

    array_init(&z_ret);
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
            add_assoc_long_ex(&z_ret, ZSTR_VAL(hkey), ZSTR_LEN(hkey), atoi(hval+1));
        } else if (decode == SCORE_DECODE_DOUBLE) {
            add_assoc_double_ex(&z_ret, ZSTR_VAL(hkey), ZSTR_LEN(hkey), atof(hval));
        } else {
            ZVAL_ZVAL(&z_sub, z_value_p, 1, 0);
            add_assoc_zval_ex(&z_ret, ZSTR_VAL(hkey), ZSTR_LEN(hkey), &z_sub);
        }
        zend_string_release(hkey);
    }

    /* replace */
    zval_dtor(z_tab);
    ZVAL_ZVAL(z_tab, &z_ret, 0, 0);
}

static int
read_mbulk_header(RedisSock *redis_sock, int *nelem)
{
    char line[4096];
    size_t len;

    /* Throws exception on failure */
    if (redis_sock_gets(redis_sock, line, sizeof(line)-1, &len) < 0)
        return -1;

    if (line[0] != '*') {
        if (IS_ATOMIC(redis_sock)) {
            if (line[0] == '-') {
                redis_sock_set_err(redis_sock, line+1, len-1);
            }
        }
        return -1;
    }

    *nelem = atoi(line+1);

    return 0;
}

static int
redis_mbulk_reply_zipped(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock,
                         zval *z_tab, int unserialize, int decode)
{
    char inbuf[4096];
    int numElems;
    size_t len;

    if (redis_sock_gets(redis_sock, inbuf, sizeof(inbuf) - 1, &len) < 0) {
        if (IS_ATOMIC(redis_sock)) {
            RETVAL_FALSE;
        } else {
            add_next_index_bool(z_tab, 0);
        }
        return FAILURE;
    }

    if(inbuf[0] != '*') {
        if (IS_ATOMIC(redis_sock)) {
            RETVAL_FALSE;
        } else {
            add_next_index_bool(z_tab, 0);
        }
        if (*inbuf == TYPE_ERR) {
            redis_sock_set_err(redis_sock, inbuf + 1, len - 1);
        }
        return -1;
    }
    numElems = atoi(inbuf+1);
    zval z_multi_result;
    array_init(&z_multi_result); /* pre-allocate array for multi's results. */

    /* Grab our key, value, key, value array */
    redis_mbulk_reply_loop(redis_sock, &z_multi_result, numElems, unserialize);

    /* Zip keys and values */
    array_zip_values_and_scores(redis_sock, &z_multi_result, decode);

    if (IS_ATOMIC(redis_sock)) {
        RETVAL_ZVAL(&z_multi_result, 0, 1);
    } else {
        add_next_index_zval(z_tab, &z_multi_result);
    }

    return 0;
}

/* Consume message ID */
PHP_REDIS_API int
redis_sock_read_single_line(RedisSock *redis_sock, char *buffer, size_t buflen,
                            size_t *linelen, int set_err)
{
    REDIS_REPLY_TYPE type;
    long info;

    if (redis_read_reply_type(redis_sock, &type, &info) < 0 ||
        (type != TYPE_LINE && type != TYPE_ERR))
    {
        return -1;
    }

    if (redis_sock_gets(redis_sock, buffer, buflen, linelen) < 0) {
        return -1;
    }

    if (set_err && type == TYPE_ERR) {
        if (IS_ATOMIC(redis_sock)) {
            redis_sock_set_err(redis_sock, buffer, *linelen);
        }
    }

    return type == TYPE_LINE ? 0 : -1;
}

/* Helper function to consume Redis stream message data.  This is useful for
 * multiple stream callers (e.g. XREAD[GROUP], and X[REV]RANGE handlers). */
PHP_REDIS_API int
redis_read_stream_messages(RedisSock *redis_sock, int count, zval *z_ret
                          )
{
    zval z_message;
    int i, mhdr, fields;
    char *id = NULL;
    int idlen;

    /* Iterate over each message */
    for (i = 0; i < count; i++) {
        /* Consume inner multi-bulk header, message ID itself and finally
         * the multi-bulk header for field and values */
        if ((read_mbulk_header(redis_sock, &mhdr) < 0 || mhdr != 2) ||
            ((id = redis_sock_read(redis_sock, &idlen)) == NULL) ||
            (read_mbulk_header(redis_sock, &fields) < 0 ||
            (fields > 0 && fields % 2 != 0)))
        {
            if (id) efree(id);
            return -1;
        }

        if (fields < 0) {
            add_assoc_null_ex(z_ret, id, idlen);
        } else {
            array_init(&z_message);
            redis_mbulk_reply_loop(redis_sock, &z_message, fields, UNSERIALIZE_VALS);
            array_zip_values_and_scores(redis_sock, &z_message, SCORE_DECODE_NONE);
            add_assoc_zval_ex(z_ret, id, idlen, &z_message);
        }
        efree(id);
    }

    return 0;
}

PHP_REDIS_API int
redis_xrange_reply(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock,
                   zval *z_tab, void *ctx)
{
    zval z_messages;
    int messages;

    array_init(&z_messages);

    if (read_mbulk_header(redis_sock, &messages) < 0 ||
        redis_read_stream_messages(redis_sock, messages, &z_messages) < 0)
    {
        zval_dtor(&z_messages);
        if (IS_ATOMIC(redis_sock)) {
            RETVAL_FALSE;
        } else {
            add_next_index_bool(z_tab, 0);
        }
        return -1;
    }

    if (IS_ATOMIC(redis_sock)) {
        RETVAL_ZVAL(&z_messages, 0, 1);
    } else {
        add_next_index_zval(z_tab, &z_messages);
    }

    return 0;
}

PHP_REDIS_API int
redis_read_stream_messages_multi(RedisSock *redis_sock, int count, zval *z_streams
                                )
{
    zval z_messages;
    int i, shdr, messages;
    char *id = NULL;
    int idlen;

    for (i = 0; i < count; i++) {
        if ((read_mbulk_header(redis_sock, &shdr) < 0 || shdr != 2) ||
            (id = redis_sock_read(redis_sock, &idlen)) == NULL ||
            read_mbulk_header(redis_sock, &messages) < 0)
        {
            if (id) efree(id);
            return -1;
        }

        array_init(&z_messages);

        if (redis_read_stream_messages(redis_sock, messages, &z_messages) < 0)
            goto failure;

        add_assoc_zval_ex(z_streams, id, idlen, &z_messages);
        efree(id);
    }

    return 0;
failure:
    efree(id);
    zval_dtor(&z_messages);
    return -1;
}

PHP_REDIS_API int
redis_xread_reply(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock,
                  zval *z_tab, void *ctx)
{
    zval z_rv;
    int streams;

    if (read_mbulk_header(redis_sock, &streams) < 0)
        goto failure;

    if (streams == -1 && redis_sock->null_mbulk_as_null) {
        ZVAL_NULL(&z_rv);
    } else {
        array_init(&z_rv);
        if (redis_read_stream_messages_multi(redis_sock, streams, &z_rv) < 0)
            goto cleanup;
    }

    if (IS_ATOMIC(redis_sock)) {
        RETVAL_ZVAL(&z_rv, 0, 1);
    } else {
        add_next_index_zval(z_tab, &z_rv);
    }
    return 0;

cleanup:
    zval_dtor(&z_rv);
failure:
    if (IS_ATOMIC(redis_sock)) {
        RETVAL_FALSE;
    } else {
        add_next_index_bool(z_tab, 0);
    }
    return -1;
}

/* This helper function does that actual XCLAIM response handling, which can be used by both
 * Redis and RedisCluster.  Note that XCLAIM is somewhat unique in that its reply type depends
 * on whether or not it was called with the JUSTID option */
PHP_REDIS_API int
redis_read_xclaim_response(RedisSock *redis_sock, int count, zval *rv) {
    zval z_msg;
    REDIS_REPLY_TYPE type;
    char *id = NULL;
    int i, fields, idlen;
    long li;

    for (i = 0; i < count; i++) {
        /* Consume inner reply type */
        if (redis_read_reply_type(redis_sock, &type, &li) < 0 ||
            (type != TYPE_BULK && type != TYPE_MULTIBULK) ||
            (type == TYPE_BULK && li <= 0)) return -1;

        /* TYPE_BULK is the JUSTID variant, otherwise it's standard xclaim response */
        if (type == TYPE_BULK) {
            if ((id = redis_sock_read_bulk_reply(redis_sock, (size_t)li)) == NULL)
                return -1;

            add_next_index_stringl(rv, id, li);
            efree(id);
        } else {
            if ((li != 2 || (id = redis_sock_read(redis_sock, &idlen)) == NULL) ||
                (read_mbulk_header(redis_sock, &fields) < 0 || fields % 2 != 0))
            {
                if (id) efree(id);
                return -1;
            }

            array_init(&z_msg);

            redis_mbulk_reply_loop(redis_sock, &z_msg, fields, UNSERIALIZE_VALS);
            array_zip_values_and_scores(redis_sock, &z_msg, SCORE_DECODE_NONE);
            add_assoc_zval_ex(rv, id, idlen, &z_msg);
            efree(id);
        }
    }

    return 0;
}

PHP_REDIS_API int
redis_xclaim_reply(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock,
                   zval *z_tab, void *ctx)
{
    zval z_ret;
    int messages;

    /* All XCLAIM responses start multibulk */
    if (read_mbulk_header(redis_sock, &messages) < 0)
        goto failure;

    array_init(&z_ret);

    if (redis_read_xclaim_response(redis_sock, messages, &z_ret) < 0) {
        zval_dtor(&z_ret);
        goto failure;
    }

    if (IS_ATOMIC(redis_sock)) {
        RETVAL_ZVAL(&z_ret, 0, 1);
    } else {
        add_next_index_zval(z_tab, &z_ret);
    }
    return 0;

failure:
    if (IS_ATOMIC(redis_sock)) {
        RETVAL_FALSE;
    } else {
        add_next_index_bool(z_tab, 0);
    }
    return -1;
}

PHP_REDIS_API int
redis_read_xinfo_response(RedisSock *redis_sock, zval *z_ret, int elements)
{
    zval zv;
    int i, len = 0;
    char *key = NULL, *data;
    REDIS_REPLY_TYPE type;
    long li;

    for (i = 0; i < elements; ++i) {
        if (redis_read_reply_type(redis_sock, &type, &li) < 0) {
            goto failure;
        }
        switch (type) {
        case TYPE_BULK:
            if ((data = redis_sock_read_bulk_reply(redis_sock, li)) == NULL) {
                if (!key) goto failure;
                add_assoc_null_ex(z_ret, key, len);
                efree(key);
                key = NULL;
            } else if (key) {
                add_assoc_stringl_ex(z_ret, key, len, data, li);
                efree(data);
                efree(key);
                key = NULL;
            } else {
                key = data;
                len = li;
            }
            break;
        case TYPE_INT:
            if (key) {
                add_assoc_long_ex(z_ret, key, len, li);
                efree(key);
                key = NULL;
            } else {
                len = spprintf(&key, 0, "%ld", li);
            }
            break;
        case TYPE_MULTIBULK:
            array_init(&zv);
            if (redis_read_xinfo_response(redis_sock, &zv, li) != SUCCESS) {
                zval_dtor(&zv);
                goto failure;
            }
            if (key) {
                add_assoc_zval_ex(z_ret, key, len, &zv);
                efree(key);
                key = NULL;
            } else {
                add_next_index_zval(z_ret, &zv);
            }
            break;
        default:
            goto failure;
        }
    }

    return SUCCESS;

failure:
    if (key) efree(key);
    return FAILURE;
}

PHP_REDIS_API int
redis_xinfo_reply(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock, zval *z_tab, void *ctx)
{
    zval z_ret;
    int elements;

    if (read_mbulk_header(redis_sock, &elements) == SUCCESS) {
        array_init(&z_ret);
        if (redis_read_xinfo_response(redis_sock, &z_ret, elements) == SUCCESS) {
            if (IS_ATOMIC(redis_sock)) {
                RETVAL_ZVAL(&z_ret, 0, 1);
            } else {
                add_next_index_zval(z_tab, &z_ret);
            }
            return SUCCESS;
        }
        zval_dtor(&z_ret);
    }
    if (IS_ATOMIC(redis_sock)) {
        RETVAL_FALSE;
    } else {
        add_next_index_bool(z_tab, 0);
    }
    return FAILURE;
}

PHP_REDIS_API int
redis_read_acl_log_reply(RedisSock *redis_sock, zval *zret, long count) {
    zval zsub;
    int i, nsub;

    for (i = 0; i < count; i++) {
        if (read_mbulk_header(redis_sock, &nsub) < 0 || nsub % 2 != 0)
            return FAILURE;

        array_init(&zsub);
        if (redis_mbulk_reply_zipped_raw_variant(redis_sock, &zsub, nsub) == FAILURE)
            return FAILURE;

        add_next_index_zval(zret, &zsub);
    }

    return SUCCESS;
}

PHP_REDIS_API int
redis_read_acl_getuser_reply(RedisSock *redis_sock, zval *zret, long count) {
    REDIS_REPLY_TYPE type;
    zval zv;
    char *key, *val;
    long vlen;
    int klen, i;

    for (i = 0; i < count; i += 2) {
        if (!(key = redis_sock_read(redis_sock, &klen)) ||
            redis_read_reply_type(redis_sock, &type, &vlen) < 0 ||
            (type != TYPE_BULK && type != TYPE_MULTIBULK) ||
            vlen > INT_MAX)
        {
            if (key) efree(key);
            return FAILURE;
        }

        if (type == TYPE_BULK) {
            if (!(val = redis_sock_read_bulk_reply(redis_sock, (int)vlen)))
                return FAILURE;
            add_assoc_stringl_ex(zret, key, klen, val, vlen);
            efree(val);
        } else {
            array_init(&zv);
            redis_mbulk_reply_loop(redis_sock, &zv, (int)vlen, UNSERIALIZE_NONE);
            add_assoc_zval_ex(zret, key, klen, &zv);
        }

        efree(key);
    }

    return SUCCESS;
}

int redis_acl_custom_reply(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock, zval *z_tab, void *ctx,
                           int (*cb)(RedisSock*, zval*, long)) {
    REDIS_REPLY_TYPE type;
    int res = FAILURE;
    zval zret;
    long len;

    if (redis_read_reply_type(redis_sock, &type, &len) == 0 && type == TYPE_MULTIBULK) {
        array_init(&zret);

        res = cb(redis_sock, &zret, len);
        if (res == FAILURE) {
            zval_dtor(&zret);
            ZVAL_FALSE(&zret);
        }
    } else {
        ZVAL_FALSE(&zret);
    }

    if (IS_ATOMIC(redis_sock)) {
        RETVAL_ZVAL(&zret, 0, 0);
    } else {
        add_next_index_zval(z_tab, &zret);
    }

    return res;
}

int redis_acl_getuser_reply(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock, zval *z_tab, void *ctx) {
    return redis_acl_custom_reply(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock, z_tab, ctx,
                                  redis_read_acl_getuser_reply);
}

int redis_acl_log_reply(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock, zval *z_tab, void *ctx) {
    return redis_acl_custom_reply(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock, z_tab, ctx,
                                  redis_read_acl_log_reply);
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

PHP_REDIS_API int
redis_1_response(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock, zval *z_tab, void *ctx)
{
    char *response;
    int response_len;
    zend_bool ret = 0;

    if ((response = redis_sock_read(redis_sock, &response_len)) != NULL) {
        ret = (response[1] == '1');
        efree(response);
    }

    if (IS_ATOMIC(redis_sock)) {
        RETVAL_BOOL(ret);
    } else {
        add_next_index_bool(z_tab, ret);
    }

    return ret ? SUCCESS : FAILURE;
}

PHP_REDIS_API int redis_string_response(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock, zval *z_tab, void *ctx) {

    char *response;
    int response_len;

    if ((response = redis_sock_read(redis_sock, &response_len))
                                    == NULL)
    {
        if (IS_ATOMIC(redis_sock)) {
            RETVAL_FALSE;
        } else {
            add_next_index_bool(z_tab, 0);
        }
        return FAILURE;
    }
    if (IS_ATOMIC(redis_sock)) {
        if (!redis_unpack(redis_sock, response, response_len, return_value)) {
            RETVAL_STRINGL(response, response_len);
        }
    } else {
        zval z_unpacked;
        if (redis_unpack(redis_sock, response, response_len, &z_unpacked)) {
            add_next_index_zval(z_tab, &z_unpacked);
        } else {
            add_next_index_stringl(z_tab, response, response_len);
        }
    }

    efree(response);
    return SUCCESS;
}

PHP_REDIS_API
void redis_single_line_reply(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock,
                             zval *z_tab, void *ctx)
{
    char buffer[4096];
    size_t len;

    if (redis_sock_read_single_line(redis_sock, buffer, sizeof(buffer), &len, 1) < 0) {
        if (IS_ATOMIC(redis_sock)) {
            RETURN_FALSE;
        } else {
            add_next_index_bool(z_tab, 0);
        }
        return;
    }

    //str = estrndup(buffer, len);
    if (IS_ATOMIC(redis_sock)) {
        RETVAL_STRINGL(buffer, len);
    } else {
        add_next_index_stringl(z_tab, buffer, len);
    }
}

/* like string response, but never unserialized. */
PHP_REDIS_API int
redis_ping_response(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock,
                    zval *z_tab, void *ctx)
{

    char *response;
    int response_len;

    if ((response = redis_sock_read(redis_sock, &response_len))
                                    == NULL)
    {
        if (IS_ATOMIC(redis_sock)) {
            RETVAL_FALSE;
        } else {
            add_next_index_bool(z_tab, 0);
        }
        return FAILURE;
    }
    if (IS_ATOMIC(redis_sock)) {
        RETVAL_STRINGL(response, response_len);
    } else {
        add_next_index_stringl(z_tab, response, response_len);
    }

    efree(response);
    return SUCCESS;
}

/* Response for DEBUG object which is a formatted single line reply */
PHP_REDIS_API void redis_debug_response(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock,
                                        zval *z_tab, void *ctx)
{
    char *resp, *p, *p2, *p3, *p4;
    int is_numeric,  resp_len;

    /* Add or return false if we can't read from the socket */
    if((resp = redis_sock_read(redis_sock, &resp_len))==NULL) {
        if (IS_ATOMIC(redis_sock)) {
            RETURN_FALSE;
        }
        add_next_index_bool(z_tab, 0);
        return;
    }

    zval z_result;
    array_init(&z_result);

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
            add_assoc_long(&z_result, p, atol(p2));
        } else {
            add_assoc_string(&z_result, p, p2);
        }

        p = p3;
    }

    efree(resp);

    if (IS_ATOMIC(redis_sock)) {
        RETVAL_ZVAL(&z_result, 0, 1);
    } else {
        add_next_index_zval(z_tab, &z_result);
    }
}

/**
 * redis_sock_create
 */
PHP_REDIS_API RedisSock*
redis_sock_create(char *host, int host_len, int port,
                  double timeout, double read_timeout,
                  int persistent, char *persistent_id,
                  long retry_interval)
{
    RedisSock *redis_sock;

    redis_sock = ecalloc(1, sizeof(RedisSock));
    redis_sock->host = zend_string_init(host, host_len, 0);
    redis_sock->status = REDIS_SOCK_STATUS_DISCONNECTED;
    redis_sock->retry_interval = retry_interval * 1000;
    redis_sock->persistent = persistent;

    if (persistent && persistent_id != NULL) {
        redis_sock->persistent_id = zend_string_init(persistent_id, strlen(persistent_id), 0);
    }

    redis_sock->port    = port;
    redis_sock->timeout = timeout;
    redis_sock->read_timeout = read_timeout;

    redis_sock->serializer = REDIS_SERIALIZER_NONE;
    redis_sock->compression = REDIS_COMPRESSION_NONE;
    redis_sock->mode = ATOMIC;

    return redis_sock;
}

static int redis_uniqid(char *buf, size_t buflen) {
    struct timeval tv;
    gettimeofday(&tv, NULL);

    return snprintf(buf, buflen, "phpredis:%08lx%05lx:%08lx",
                    (long)tv.tv_sec, (long)tv.tv_usec, (long)php_rand());
}

static int redis_stream_liveness_check(php_stream *stream) {
    return php_stream_set_option(stream, PHP_STREAM_OPTION_CHECK_LIVENESS,
                                 0, NULL) == PHP_STREAM_OPTION_RETURN_OK ?
                                 SUCCESS : FAILURE;
}

static int
redis_sock_check_liveness(RedisSock *redis_sock)
{
    char id[64], inbuf[4096];
    int idlen, auth;
    smart_string cmd = {0};
    size_t len;

    /* Short circuit if we detect the stream has gone bad or if the user has
     * configured persistent connection "YOLO mode". */
    if (redis_stream_liveness_check(redis_sock->stream) != SUCCESS) {
        goto failure;
    }

    redis_sock->status = REDIS_SOCK_STATUS_CONNECTED;
    if (!INI_INT("redis.pconnect.echo_check_liveness")) {
        return SUCCESS;
    }

    /* AUTH (if we need it) */
    auth = redis_sock_append_auth(redis_sock, &cmd);

    /* ECHO challenge/response */
    idlen = redis_uniqid(id, sizeof(id));
    REDIS_CMD_INIT_SSTR_STATIC(&cmd, 1, "ECHO");
    redis_cmd_append_sstr(&cmd, id, idlen);

    /* Send command(s) and make sure we can consume reply(ies) */
    if (redis_sock_write(redis_sock, cmd.c, cmd.len) < 0) {
        smart_string_free(&cmd);
        goto failure;
    }
    smart_string_free(&cmd);

    if (redis_sock_gets(redis_sock, inbuf, sizeof(inbuf) - 1, &len) < 0) {
        goto failure;
    }

    if (auth) {
        if (strncmp(inbuf, "+OK", 3) == 0 || strncmp(inbuf, "-ERR Client sent AUTH", 21) == 0) {
            /* successfully authenticated or authentication isn't required */
            if (redis_sock_gets(redis_sock, inbuf, sizeof(inbuf) - 1, &len) < 0) {
                goto failure;
            }
        } else if (strncmp(inbuf, "-NOAUTH", 7) == 0) {
            /* connection is fine but authentication failed, next command must fails too */
            if (redis_sock_gets(redis_sock, inbuf, sizeof(inbuf) - 1, &len) < 0 || strncmp(inbuf, "-NOAUTH", 7) != 0) {
                goto failure;
            }
            return SUCCESS;
        } else {
            goto failure;
        }
        redis_sock->status = REDIS_SOCK_STATUS_READY;
    } else {
        if (strncmp(inbuf, "-NOAUTH", 7) == 0) {
            /* connection is fine but authentication required */
            return SUCCESS;
        }
    }

    /* check echo response */
    if (*inbuf != TYPE_BULK || atoi(inbuf + 1) != idlen ||
        redis_sock_gets(redis_sock, inbuf, sizeof(inbuf) - 1, &len) < 0 ||
        strncmp(inbuf, id, idlen) != 0
    ) {
        goto failure;
    }

    return SUCCESS;
failure:
    redis_sock->status = REDIS_SOCK_STATUS_DISCONNECTED;
    if (redis_sock->stream) {
        php_stream_pclose(redis_sock->stream);
        redis_sock->stream = NULL;
    }
    return FAILURE;
}

/**
 * redis_sock_connect
 */
PHP_REDIS_API int redis_sock_connect(RedisSock *redis_sock)
{
    struct timeval tv, read_tv, *tv_ptr = NULL;
    zend_string *persistent_id = NULL, *estr = NULL;
    char host[1024], *pos, *address, *scheme = NULL;
    const char *fmtstr = "%s://%s:%d";
    int host_len, usocket = 0, err = 0, tcp_flag = 1, scheme_free = 0;
    ConnectionPool *p = NULL;

    if (redis_sock->stream != NULL) {
        redis_sock_disconnect(redis_sock, 0);
    }

    address = ZSTR_VAL(redis_sock->host);
    if ((pos = strstr(address, "://")) == NULL) {
        scheme = redis_sock->stream_ctx ? "ssl" : "tcp";
    } else {
        scheme = estrndup(address, pos - address);
        address = pos + sizeof("://") - 1;
        scheme_free = 1;
    }
    if (address[0] == '/' && redis_sock->port < 1) {
        host_len = snprintf(host, sizeof(host), "unix://%s", address);
        usocket = 1;
    } else {
        if(redis_sock->port == 0)
            redis_sock->port = 6379;

#ifdef HAVE_IPV6
        /* If we've got IPv6 and find a colon in our address, convert to proper
         * IPv6 [host]:port format */
        if (strchr(address, ':') != NULL) {
            fmtstr = "%s://[%s]:%d";
        }
#endif
        host_len = snprintf(host, sizeof(host), fmtstr, scheme, address, redis_sock->port);
    }
    if (scheme_free) efree(scheme);

    if (redis_sock->persistent) {
        if (INI_INT("redis.pconnect.pooling_enabled")) {
            p = redis_sock_get_connection_pool(redis_sock);
            if (zend_llist_count(&p->list) > 0) {
                redis_sock->stream = *(php_stream **)zend_llist_get_last(&p->list);
                zend_llist_remove_tail(&p->list);

                if (redis_sock_check_liveness(redis_sock) == SUCCESS) {
                    return SUCCESS;
                }
                p->nb_active--;
            }

            int limit = INI_INT("redis.pconnect.connection_limit");
            if (limit > 0 && p->nb_active >= limit) {
                redis_sock_set_err(redis_sock, "Connection limit reached", sizeof("Connection limit reached") - 1);
                return FAILURE;
            }

            gettimeofday(&tv, NULL);
            persistent_id = strpprintf(0, "phpredis_%ld%ld", tv.tv_sec, tv.tv_usec);
        } else {
            if (redis_sock->persistent_id) {
                persistent_id = strpprintf(0, "phpredis:%s:%s", host, ZSTR_VAL(redis_sock->persistent_id));
            } else {
                persistent_id = strpprintf(0, "phpredis:%s:%f", host, redis_sock->timeout);
            }
        }
    }

    tv.tv_sec  = (time_t)redis_sock->timeout;
    tv.tv_usec = (int)((redis_sock->timeout - tv.tv_sec) * 1000000);
    if (tv.tv_sec != 0 || tv.tv_usec != 0) {
        tv_ptr = &tv;
    }

    redis_sock->stream = php_stream_xport_create(host, host_len,
        0, STREAM_XPORT_CLIENT | STREAM_XPORT_CONNECT,
        persistent_id ? ZSTR_VAL(persistent_id) : NULL,
        tv_ptr, redis_sock->stream_ctx, &estr, &err);

    if (persistent_id) {
        zend_string_release(persistent_id);
    }

    if (!redis_sock->stream) {
        if (estr) {
            redis_sock_set_err(redis_sock, ZSTR_VAL(estr), ZSTR_LEN(estr));
            zend_string_release(estr);
        }
        return FAILURE;
    }

    if (p) p->nb_active++;

    /* Attempt to set TCP_NODELAY/TCP_KEEPALIVE if we're not using a unix socket. */
    if (!usocket) {
        php_netstream_data_t *sock = (php_netstream_data_t*)redis_sock->stream->abstract;
        err = setsockopt(sock->socket, IPPROTO_TCP, TCP_NODELAY, (char*) &tcp_flag, sizeof(tcp_flag));
        PHPREDIS_NOTUSED(err);
        err = setsockopt(sock->socket, SOL_SOCKET, SO_KEEPALIVE, (char*) &redis_sock->tcp_keepalive, sizeof(redis_sock->tcp_keepalive));
        PHPREDIS_NOTUSED(err);
    }

    php_stream_auto_cleanup(redis_sock->stream);

    read_tv.tv_sec  = (time_t)redis_sock->read_timeout;
    read_tv.tv_usec = (int)((redis_sock->read_timeout - read_tv.tv_sec) * 1000000);

    if (read_tv.tv_sec != 0 || read_tv.tv_usec != 0) {
        php_stream_set_option(redis_sock->stream,PHP_STREAM_OPTION_READ_TIMEOUT,
            0, &read_tv);
    }
    php_stream_set_option(redis_sock->stream,
        PHP_STREAM_OPTION_WRITE_BUFFER, PHP_STREAM_BUFFER_NONE, NULL);

    redis_sock->status = REDIS_SOCK_STATUS_CONNECTED;

    return SUCCESS;
}

/**
 * redis_sock_server_open
 */
PHP_REDIS_API int
redis_sock_server_open(RedisSock *redis_sock)
{
    if (redis_sock) {
        switch (redis_sock->status) {
        case REDIS_SOCK_STATUS_DISCONNECTED:
            if (redis_sock_connect(redis_sock) != SUCCESS) {
                break;
            } else if (redis_sock->status == REDIS_SOCK_STATUS_READY) {
                return SUCCESS;
            }
            // fall through
        case REDIS_SOCK_STATUS_CONNECTED:
            if (redis_sock_auth(redis_sock) != SUCCESS)
                break;
            redis_sock->status = REDIS_SOCK_STATUS_READY;
            // fall through
        case REDIS_SOCK_STATUS_READY:
            return SUCCESS;
        default:
            return FAILURE;
        }
    }
    return FAILURE;
}

/**
 * redis_sock_disconnect
 */
PHP_REDIS_API int
redis_sock_disconnect(RedisSock *redis_sock, int force)
{
    if (redis_sock == NULL) {
        return FAILURE;
    } else if (redis_sock->stream) {
        if (redis_sock->persistent) {
            ConnectionPool *p = NULL;
            if (INI_INT("redis.pconnect.pooling_enabled")) {
                p = redis_sock_get_connection_pool(redis_sock);
            }
            if (force) {
                php_stream_pclose(redis_sock->stream);
                if (p) p->nb_active--;
            } else if (p) {
                zend_llist_prepend_element(&p->list, &redis_sock->stream);
            }
        } else {
            php_stream_close(redis_sock->stream);
        }
        redis_sock->stream = NULL;
    }
    redis_sock->mode = ATOMIC;
    redis_sock->status = REDIS_SOCK_STATUS_DISCONNECTED;
    redis_sock->watching = 0;

    return SUCCESS;
}

/**
 * redis_sock_set_err
 */
PHP_REDIS_API void
redis_sock_set_err(RedisSock *redis_sock, const char *msg, int msg_len)
{
    // Free our last error
    if (redis_sock->err != NULL) {
        zend_string_release(redis_sock->err);
        redis_sock->err = NULL;
    }

    if (msg != NULL && msg_len > 0) {
        // Copy in our new error message
        redis_sock->err = zend_string_init(msg, msg_len, 0);
    }
}

PHP_REDIS_API int
redis_sock_set_stream_context(RedisSock *redis_sock, zval *options)
{
    zend_string *zkey;
    zval *z_ele;

    if (!redis_sock || Z_TYPE_P(options) != IS_ARRAY)
        return FAILURE;

    if (!redis_sock->stream_ctx)
        redis_sock->stream_ctx = php_stream_context_alloc();

    ZEND_HASH_FOREACH_STR_KEY_VAL(Z_ARRVAL_P(options), zkey, z_ele) {
        php_stream_context_set_option(redis_sock->stream_ctx, "ssl", ZSTR_VAL(zkey), z_ele);
    } ZEND_HASH_FOREACH_END();

    return SUCCESS;
}

/**
 * redis_sock_read_multibulk_reply
 */
PHP_REDIS_API int redis_sock_read_multibulk_reply(INTERNAL_FUNCTION_PARAMETERS,
                                           RedisSock *redis_sock, zval *z_tab,
                                           void *ctx)
{
    zval z_multi_result;
    char inbuf[4096];
    int numElems;
    size_t len;

    if (redis_sock_gets(redis_sock, inbuf, sizeof(inbuf) - 1, &len) < 0) {
        return -1;
    }

    if(inbuf[0] != '*') {
        if (IS_ATOMIC(redis_sock)) {
            if (inbuf[0] == '-') {
                redis_sock_set_err(redis_sock, inbuf+1, len);
            }
            RETVAL_FALSE;
        } else {
            add_next_index_bool(z_tab, 0);
        }
        return -1;
    }

    numElems = atoi(inbuf+1);

    if (numElems == -1 && redis_sock->null_mbulk_as_null) {
        ZVAL_NULL(&z_multi_result);
    } else {
        array_init(&z_multi_result);
        redis_mbulk_reply_loop(redis_sock, &z_multi_result, numElems, UNSERIALIZE_ALL);
    }

    if (IS_ATOMIC(redis_sock)) {
        RETVAL_ZVAL(&z_multi_result, 0, 1);
    } else {
        add_next_index_zval(z_tab, &z_multi_result);
    }

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

    if (redis_sock_gets(redis_sock, inbuf, sizeof(inbuf) - 1, &len) < 0) {
        if (IS_ATOMIC(redis_sock)) {
            RETVAL_FALSE;
        } else {
            add_next_index_bool(z_tab, 0);
        }
        return FAILURE;
    }

    if(inbuf[0] != '*') {
        if (IS_ATOMIC(redis_sock)) {
            if (inbuf[0] == '-') {
                redis_sock_set_err(redis_sock, inbuf+1, len);
            }
            RETVAL_FALSE;
        } else {
            add_next_index_bool(z_tab, 0);
        }
        return -1;
    }
    numElems = atoi(inbuf+1);
    zval z_multi_result;
    array_init(&z_multi_result); /* pre-allocate array for multi's results. */

    redis_mbulk_reply_loop(redis_sock, &z_multi_result, numElems, UNSERIALIZE_NONE);

    if (IS_ATOMIC(redis_sock)) {
        RETVAL_ZVAL(&z_multi_result, 0, 1);
    } else {
        add_next_index_zval(z_tab, &z_multi_result);
    }

    return 0;
}

PHP_REDIS_API int
redis_mbulk_reply_double(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock, zval *z_tab, void *ctx)
{
    char inbuf[4096], *line;
    int i, numElems, len;
    size_t buf_len;
    zval z_multi_result;

    if (redis_sock_gets(redis_sock, inbuf, sizeof(inbuf) - 1, &buf_len) < 0) {
        return FAILURE;
    }

    if (*inbuf != TYPE_MULTIBULK) {
        if (IS_ATOMIC(redis_sock)) {
            if (*inbuf == TYPE_ERR) {
                redis_sock_set_err(redis_sock, inbuf + 1, buf_len);
            }
            RETVAL_FALSE;
        } else {
            add_next_index_bool(z_tab, 0);
        }
        return FAILURE;
    }
    numElems = atoi(inbuf + 1);

    array_init(&z_multi_result);
    for (i = 0; i < numElems; ++i) {
        if ((line = redis_sock_read(redis_sock, &len)) == NULL) {
            add_next_index_bool(&z_multi_result, 0);
            continue;
        }
        add_next_index_double(&z_multi_result, atof(line));
        efree(line);
    }

    if (IS_ATOMIC(redis_sock)) {
        RETVAL_ZVAL(&z_multi_result, 0, 1);
    } else {
        add_next_index_zval(z_tab, &z_multi_result);
    }

    return SUCCESS;
}

PHP_REDIS_API void
redis_mbulk_reply_loop(RedisSock *redis_sock, zval *z_tab, int count,
                       int unserialize)
{
    zval z_unpacked;
    char *line;
    int i, len;

    for (i = 0; i < count; ++i) {
        if ((line = redis_sock_read(redis_sock, &len)) == NULL) {
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

        if (unwrap && redis_unpack(redis_sock, line, len, &z_unpacked)) {
            add_next_index_zval(z_tab, &z_unpacked);
        } else {
            add_next_index_stringl(z_tab, line, len);
        }
        efree(line);
    }
}

static int
redis_mbulk_reply_zipped_raw_variant(RedisSock *redis_sock, zval *zret, int count) {
    REDIS_REPLY_TYPE type;
    char *key, *val;
    int keylen, i;
    zend_long lval;
    double dval;
    long vallen;

    for (i = 0; i < count; i+= 2) {
        /* Keys should always be bulk strings */
        if ((key = redis_sock_read(redis_sock, &keylen)) == NULL)
            return FAILURE;

        /* This can vary */
        if (redis_read_reply_type(redis_sock, &type, &vallen) < 0)
            return FAILURE;

        if (type == TYPE_BULK) {
            if (vallen > INT_MAX || (val = redis_sock_read_bulk_reply(redis_sock, (int)vallen)) == NULL) {
                efree(key);
                return FAILURE;
            }

            /* Possibly overkill, but provides really nice types */
            switch (is_numeric_string(val, vallen, &lval, &dval, 0)) {
                case IS_LONG:
                    add_assoc_long_ex(zret, key, keylen, lval);
                    break;
                case IS_DOUBLE:
                    add_assoc_double_ex(zret, key, keylen, dval);
                    break;
                default:
                    add_assoc_stringl_ex(zret, key, keylen, val, vallen);
            }

            efree(val);
        } else if (type == TYPE_INT) {
            add_assoc_long_ex(zret, key, keylen, (zend_long)vallen);
        } else {
            add_assoc_null_ex(zret, key, keylen);
        }

        efree(key);
    }

    return SUCCESS;
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

    if (redis_sock_gets(redis_sock, inbuf, sizeof(inbuf) - 1, &len) < 0) {
        goto failure;
    }

    if (*inbuf != TYPE_MULTIBULK) {
        if (IS_ATOMIC(redis_sock)) {
            RETVAL_FALSE;
        } else {
            add_next_index_bool(z_tab, 0);
        }
        if (*inbuf == TYPE_ERR) {
            redis_sock_set_err(redis_sock, inbuf + 1, len - 1);
        }
        goto failure;
    }
    numElems = atoi(inbuf+1);
    zval z_multi_result;
    array_init(&z_multi_result); /* pre-allocate array for multi's results. */

    for(i = 0; i < numElems; ++i) {
        zend_string *zstr = zval_get_string(&z_keys[i]);
        response = redis_sock_read(redis_sock, &response_len);
        if(response != NULL) {
            zval z_unpacked;
            if (redis_unpack(redis_sock, response, response_len, &z_unpacked)) {
                add_assoc_zval_ex(&z_multi_result, ZSTR_VAL(zstr), ZSTR_LEN(zstr), &z_unpacked);
            } else {
                add_assoc_stringl_ex(&z_multi_result, ZSTR_VAL(zstr), ZSTR_LEN(zstr), response, response_len);
            }
            efree(response);
        } else {
            add_assoc_bool_ex(&z_multi_result, ZSTR_VAL(zstr), ZSTR_LEN(zstr), 0);
        }
        zend_string_release(zstr);
        zval_dtor(&z_keys[i]);
    }
    efree(z_keys);

    if (IS_ATOMIC(redis_sock)) {
        RETVAL_ZVAL(&z_multi_result, 0, 1);
    } else {
        add_next_index_zval(z_tab, &z_multi_result);
    }
    return SUCCESS;
failure:
    if (z_keys != NULL) {
        for (i = 0; Z_TYPE(z_keys[i]) != IS_NULL; ++i) {
            zval_dtor(&z_keys[i]);
        }
        efree(z_keys);
    }
    return FAILURE;
}

/**
 * redis_sock_write
 */
PHP_REDIS_API int
redis_sock_write(RedisSock *redis_sock, char *cmd, size_t sz)
{
    if (redis_check_eof(redis_sock, 0) == 0 &&
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
    if (redis_sock->prefix) {
        zend_string_release(redis_sock->prefix);
    }
    if (redis_sock->pipeline_cmd) {
        zend_string_release(redis_sock->pipeline_cmd);
    }
    if (redis_sock->err) {
        zend_string_release(redis_sock->err);
    }
    if (redis_sock->persistent_id) {
        zend_string_release(redis_sock->persistent_id);
    }
    if (redis_sock->host) {
        zend_string_release(redis_sock->host);
    }
    redis_sock_free_auth(redis_sock);
    efree(redis_sock);
}

#ifdef HAVE_REDIS_LZ4
/* Implementation of CRC8 for our LZ4 checksum value */
static uint8_t crc8(unsigned char *input, size_t len) {
    size_t i;
    uint8_t crc = 0xFF;

    while (len--) {
        crc ^= *input++;
        for (i = 0; i < 8; i++) {
            if (crc & 0x80)
                crc = (uint8_t)(crc << 1) ^ 0x31;
            else
                crc <<= 1;
        }
    }

    return crc;
}
#endif

PHP_REDIS_API int
redis_pack(RedisSock *redis_sock, zval *z, char **val, size_t *val_len)
{
    char *buf;
    int valfree;
    size_t len;

    valfree = redis_serialize(redis_sock, z, &buf, &len);
    if (redis_sock->compression == REDIS_COMPRESSION_NONE) {
        *val = buf;
        *val_len = len;
        return valfree;
    }

    switch (redis_sock->compression) {
        case REDIS_COMPRESSION_LZF:
#ifdef HAVE_REDIS_LZF
            {
                char *data;
                uint32_t res;
                double size;

                /* preserve compatibility with PECL lzf_compress margin (greater of 4% and LZF_MARGIN) */
                size = len + MIN(UINT_MAX - len, MAX(LZF_MARGIN, len / 25));
                data = emalloc(size);
                if ((res = lzf_compress(buf, len, data, size)) > 0) {
                    if (valfree) efree(buf);
                    *val = data;
                    *val_len = res;
                    return 1;
                }
                efree(data);
            }
#endif
            break;
        case REDIS_COMPRESSION_ZSTD:
#ifdef HAVE_REDIS_ZSTD
            {
                char *data;
                size_t size;
                int level;

                if (redis_sock->compression_level < 1) {
#ifdef ZSTD_CLEVEL_DEFAULT
                    level = ZSTD_CLEVEL_DEFAULT;
#else
                    level = 3;
#endif
                } else if (redis_sock->compression_level > ZSTD_maxCLevel()) {
                    level = ZSTD_maxCLevel();
                } else {
                    level = redis_sock->compression_level;
                }

                size = ZSTD_compressBound(len);
                data = emalloc(size);
                size = ZSTD_compress(data, size, buf, len, level);
                if (!ZSTD_isError(size)) {
                    if (valfree) efree(buf);
                    data = erealloc(data, size);
                    *val = data;
                    *val_len = size;
                    return 1;
                }
                efree(data);
            }
#endif
            break;
        case REDIS_COMPRESSION_LZ4:
#ifdef HAVE_REDIS_LZ4
            {
                /* Compressing empty data is pointless */
                if (len < 1)
                    break;

                /* Compressing more than INT_MAX bytes would require multiple blocks */
                if (len > INT_MAX) {
                    php_error_docref(NULL, E_WARNING,
                        "LZ4: compressing > %d bytes not supported", INT_MAX);
                    break;
                }

                int old_len = len, lz4len, lz4bound;
                uint8_t crc = crc8((unsigned char*)&old_len, sizeof(old_len));
                char *lz4buf, *lz4pos;

                lz4bound = LZ4_compressBound(len);
                lz4buf = emalloc(REDIS_LZ4_HDR_SIZE + lz4bound);
                lz4pos = lz4buf;

                /* Copy and move past crc8 length checksum */
                memcpy(lz4pos, &crc, sizeof(crc));
                lz4pos += sizeof(crc);

                /* Copy and advance past length */
                memcpy(lz4pos, &old_len, sizeof(old_len));
                lz4pos += sizeof(old_len);

                if (redis_sock->compression_level <= 0 || redis_sock->compression_level > REDIS_LZ4_MAX_CLEVEL) {
                    lz4len = LZ4_compress_default(buf, lz4pos, old_len, lz4bound);
                } else {
                    lz4len = LZ4_compress_HC(buf, lz4pos, old_len, lz4bound, redis_sock->compression_level);
                }

                if (lz4len <= 0) {
                    efree(lz4buf);
                    break;
                }

                if (valfree) efree(buf);
                *val = lz4buf;
                *val_len = lz4len + REDIS_LZ4_HDR_SIZE;
                return 1;
            }
#endif
            break;
    }
    *val = buf;
    *val_len = len;
    return valfree;
}

PHP_REDIS_API int
redis_unpack(RedisSock *redis_sock, const char *val, int val_len, zval *z_ret)
{
    switch (redis_sock->compression) {
        case REDIS_COMPRESSION_LZF:
#ifdef HAVE_REDIS_LZF
            {
                char *data;
                int i;
                uint32_t res;

                if (val_len == 0)
                    break;

                /* start from two-times bigger buffer and
                 * increase it exponentially  if needed */
                errno = E2BIG;
                for (i = 2; errno == E2BIG; i *= 2) {
                    data = emalloc(i * val_len);
                    if ((res = lzf_decompress(val, val_len, data, i * val_len)) == 0) {
                        /* errno != E2BIG will brake for loop */
                        efree(data);
                        continue;
                    } else if (redis_unserialize(redis_sock, data, res, z_ret) == 0) {
                        ZVAL_STRINGL(z_ret, data, res);
                    }
                    efree(data);
                    return 1;
                }
            }
#endif
            break;
        case REDIS_COMPRESSION_ZSTD:
#ifdef HAVE_REDIS_ZSTD
            {
                char *data;
                unsigned long long len;

                len = ZSTD_getFrameContentSize(val, val_len);

                if (len != ZSTD_CONTENTSIZE_ERROR && len != ZSTD_CONTENTSIZE_UNKNOWN && len <= INT_MAX)
                {
                    size_t zlen;

                    data = emalloc(len);
                    zlen = ZSTD_decompress(data, len, val, val_len);
                    if (ZSTD_isError(zlen) || zlen != len) {
                        efree(data);
                        break;
                    } else if (redis_unserialize(redis_sock, data, zlen, z_ret) == 0) {
                        ZVAL_STRINGL(z_ret, data, zlen);
                    }
                    efree(data);
                    return 1;
                }
            }
#endif
            break;
        case REDIS_COMPRESSION_LZ4:
#ifdef HAVE_REDIS_LZ4
            {
                char *data;
                int datalen;
                uint8_t lz4crc;

                /* We must have at least enough bytes for our header, and can't have more than
                 * INT_MAX + our header size. */
                if (val_len < REDIS_LZ4_HDR_SIZE || val_len > INT_MAX + REDIS_LZ4_HDR_SIZE)
                    break;

                /* Operate on copies in case our CRC fails */
                const char *copy = val;
                size_t copylen = val_len;

                /* Read in our header bytes */
                memcpy(&lz4crc, copy, sizeof(uint8_t));
                copy += sizeof(uint8_t); copylen -= sizeof(uint8_t);
                memcpy(&datalen, copy, sizeof(int));
                copy += sizeof(int); copylen -= sizeof(int);

                /* Make sure our CRC matches (TODO:  Maybe issue a docref error?) */
                if (crc8((unsigned char*)&datalen, sizeof(datalen)) != lz4crc)
                    break;

                /* Finally attempt decompression */
                data = emalloc(datalen);
                if (LZ4_decompress_safe(copy, data, copylen, datalen) > 0) {
                    if (redis_unserialize(redis_sock, data, datalen, z_ret) == 0) {
                        ZVAL_STRINGL(z_ret, data, datalen);
                    }
                    efree(data);
                    return 1;
                }
                efree(data);
            }
#endif
            break;
    }
    return redis_unserialize(redis_sock, val, val_len, z_ret);
}

PHP_REDIS_API int
redis_serialize(RedisSock *redis_sock, zval *z, char **val, size_t *val_len
               )
{
    php_serialize_data_t ht;

    smart_str sstr = {0};
#ifdef HAVE_REDIS_IGBINARY
    size_t sz;
    uint8_t *val8;
#endif

    *val = "";
    *val_len = 0;
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
                    *val = estrndup(ZSTR_VAL(zstr), ZSTR_LEN(zstr));
                    *val_len = ZSTR_LEN(zstr);
                    zend_string_release(zstr);
                    return 1;
                }
            }
            break;
        case REDIS_SERIALIZER_PHP:
            PHP_VAR_SERIALIZE_INIT(ht);
            php_var_serialize(&sstr, z, &ht);

            *val = estrndup(ZSTR_VAL(sstr.s), ZSTR_LEN(sstr.s));
            *val_len = ZSTR_LEN(sstr.s);

            smart_str_free(&sstr);
            PHP_VAR_SERIALIZE_DESTROY(ht);

            return 1;

        case REDIS_SERIALIZER_MSGPACK:
#ifdef HAVE_REDIS_MSGPACK
            php_msgpack_serialize(&sstr, z);
            *val = estrndup(ZSTR_VAL(sstr.s), ZSTR_LEN(sstr.s));
            *val_len = ZSTR_LEN(sstr.s);
            smart_str_free(&sstr);

            return 1;
#endif
            break;
        case REDIS_SERIALIZER_IGBINARY:
#ifdef HAVE_REDIS_IGBINARY
            if(igbinary_serialize(&val8, (size_t *)&sz, z) == 0) {
                *val = (char*)val8;
                *val_len = sz;
                return 1;
            }
#endif
            break;
        case REDIS_SERIALIZER_JSON:
#ifdef HAVE_REDIS_JSON
            php_json_encode(&sstr, z, PHP_JSON_OBJECT_AS_ARRAY);
            *val = estrndup(ZSTR_VAL(sstr.s), ZSTR_LEN(sstr.s));
            *val_len = ZSTR_LEN(sstr.s);
            smart_str_free(&sstr);
            return 1;
#endif
            break;
        EMPTY_SWITCH_DEFAULT_CASE()
    }

    return 0;
}

PHP_REDIS_API int
redis_unserialize(RedisSock* redis_sock, const char *val, int val_len,
                  zval *z_ret)
{

    php_unserialize_data_t var_hash;
    int ret = 0;

    switch(redis_sock->serializer) {
        case REDIS_SERIALIZER_NONE:
            /* Nothing to do */
            break;
        case REDIS_SERIALIZER_PHP:
            PHP_VAR_UNSERIALIZE_INIT(var_hash);

            ret = php_var_unserialize(z_ret, (const unsigned char **)&val,
                                      (const unsigned char *)val + val_len,
                                       &var_hash);

            PHP_VAR_UNSERIALIZE_DESTROY(var_hash);
            break;

        case REDIS_SERIALIZER_MSGPACK:
#ifdef HAVE_REDIS_MSGPACK
            ret = !php_msgpack_unserialize(z_ret, (char *)val, (size_t)val_len);
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

            ret = !igbinary_unserialize((const uint8_t *)val, (size_t)val_len, z_ret);
#endif
            break;
        case REDIS_SERIALIZER_JSON:
#ifdef HAVE_REDIS_JSON
    #if (PHP_MAJOR_VERSION == 7 && PHP_MINOR_VERSION < 1)
            JSON_G(error_code) = PHP_JSON_ERROR_NONE;
            php_json_decode(z_ret, (char*)val, val_len, 1, PHP_JSON_PARSER_DEFAULT_DEPTH);
            ret = JSON_G(error_code) == PHP_JSON_ERROR_NONE;
    #else
            ret = !php_json_decode(z_ret, (char *)val, val_len, 1, PHP_JSON_PARSER_DEFAULT_DEPTH);
    #endif
#endif
            break;
        EMPTY_SWITCH_DEFAULT_CASE()
    }

    return ret;
}

PHP_REDIS_API int
redis_key_prefix(RedisSock *redis_sock, char **key, size_t *key_len) {
    int ret_len;
    char *ret;

    if (redis_sock->prefix == NULL) {
        return 0;
    }

    ret_len = ZSTR_LEN(redis_sock->prefix) + *key_len;
    ret = ecalloc(1 + ret_len, 1);
    memcpy(ret, ZSTR_VAL(redis_sock->prefix), ZSTR_LEN(redis_sock->prefix));
    memcpy(ret + ZSTR_LEN(redis_sock->prefix), *key, *key_len);

    *key = ret;
    *key_len = ret_len;
    return 1;
}

/*
 * Processing for variant reply types (think EVAL)
 */

PHP_REDIS_API int
redis_sock_gets(RedisSock *redis_sock, char *buf, int buf_size,
                size_t *line_size)
{
    // Handle EOF
    if(-1 == redis_check_eof(redis_sock, 0)) {
        return -1;
    }

    if(php_stream_get_line(redis_sock->stream, buf, buf_size, line_size)
                           == NULL)
    {
        char *errmsg = NULL;

        if (redis_sock->port < 0) {
            spprintf(&errmsg, 0, "read error on connection to %s", ZSTR_VAL(redis_sock->host));
        } else {
            spprintf(&errmsg, 0, "read error on connection to %s:%d", ZSTR_VAL(redis_sock->host), redis_sock->port);
        }
        // Close our socket
        redis_sock_disconnect(redis_sock, 1);

        // Throw a read error exception
        REDIS_THROW_EXCEPTION(errmsg, 0);
        efree(errmsg);
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
                      long *reply_info)
{
    // Make sure we haven't lost the connection, even trying to reconnect
    if(-1 == redis_check_eof(redis_sock, 0)) {
        // Failure
        *reply_type = EOF;
        return -1;
    }

    // Attempt to read the reply-type byte
    if((*reply_type = php_stream_getc(redis_sock->stream)) == EOF) {
        REDIS_THROW_EXCEPTION( "socket error on read socket", 0);
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
        if (php_stream_gets(redis_sock->stream, inbuf, sizeof(inbuf)) == NULL) {
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
static int
redis_read_variant_line(RedisSock *redis_sock, REDIS_REPLY_TYPE reply_type,
                        int as_string, zval *z_ret)
{
    // Buffer to read our single line reply
    char inbuf[4096];
    size_t len;

    /* Attempt to read our single line reply */
    if(redis_sock_gets(redis_sock, inbuf, sizeof(inbuf), &len) < 0) {
        return -1;
    }

    /* Throw exception on SYNC error otherwise just set error string */
    if(reply_type == TYPE_ERR) {
        redis_sock_set_err(redis_sock, inbuf, len);
        redis_error_throw(redis_sock);
        ZVAL_FALSE(z_ret);
    } else if (as_string) {
        ZVAL_STRINGL(z_ret, inbuf, len);
    } else {
        ZVAL_TRUE(z_ret);
    }

    return 0;
}

PHP_REDIS_API int
redis_read_variant_bulk(RedisSock *redis_sock, int size, zval *z_ret
                       )
{
    // Attempt to read the bulk reply
    char *bulk_resp = redis_sock_read_bulk_reply(redis_sock, size);

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
redis_read_multibulk_recursive(RedisSock *redis_sock, long long elements, int status_strings,
                               zval *z_ret)
{
    long reply_info;
    REDIS_REPLY_TYPE reply_type;
    zval z_subelem;

    // Iterate while we have elements
    while(elements > 0) {
        // Attempt to read our reply type
        if(redis_read_reply_type(redis_sock, &reply_type, &reply_info
                                ) < 0)
        {
            zend_throw_exception_ex(redis_exception_ce, 0,
                "protocol error, couldn't parse MULTI-BULK response\n");
            return FAILURE;
        }

        // Switch on our reply-type byte
        switch(reply_type) {
            case TYPE_ERR:
            case TYPE_LINE:
                redis_read_variant_line(redis_sock, reply_type, status_strings,
                                        &z_subelem);
                add_next_index_zval(z_ret, &z_subelem);
                break;
            case TYPE_INT:
                // Add our long value
                add_next_index_long(z_ret, reply_info);
                break;
            case TYPE_BULK:
                // Init a zval for our bulk response, read and add it
                redis_read_variant_bulk(redis_sock, reply_info, &z_subelem);
                add_next_index_zval(z_ret, &z_subelem);
                break;
            case TYPE_MULTIBULK:
                if (reply_info < 0 && redis_sock->null_mbulk_as_null) {
                    add_next_index_null(z_ret);
                } else {
                    array_init(&z_subelem);
                    if (reply_info > 0) {
                        redis_read_multibulk_recursive(redis_sock, reply_info, status_strings, &z_subelem);
                    }
                    add_next_index_zval(z_ret, &z_subelem);
                }
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

static int
variant_reply_generic(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock,
                      int status_strings, int null_mbulk_as_null,
                      zval *z_tab, void *ctx)
{
    // Reply type, and reply size vars
    REDIS_REPLY_TYPE reply_type;
    long reply_info;
    zval z_ret;

    // Attempt to read our header
    if(redis_read_reply_type(redis_sock,&reply_type,&reply_info) < 0) {
        return -1;
    }

    /* Switch based on our top level reply type */
    switch(reply_type) {
        case TYPE_ERR:
        case TYPE_LINE:
            redis_read_variant_line(redis_sock, reply_type, status_strings, &z_ret);
            break;
        case TYPE_INT:
            ZVAL_LONG(&z_ret, reply_info);
            break;
        case TYPE_BULK:
            redis_read_variant_bulk(redis_sock, reply_info, &z_ret);
            break;
        case TYPE_MULTIBULK:
            if (reply_info > -1) {
                array_init(&z_ret);
                redis_read_multibulk_recursive(redis_sock, reply_info, status_strings, &z_ret);
            } else {
                if (null_mbulk_as_null) {
                    ZVAL_NULL(&z_ret);
                } else {
                    array_init(&z_ret);
                }
            }
            break;
        default:
            zend_throw_exception_ex(redis_exception_ce, 0,
                "protocol error, got '%c' as reply-type byte\n", reply_type);
            return FAILURE;
    }

    if (IS_ATOMIC(redis_sock)) {
        /* Set our return value */
        RETVAL_ZVAL(&z_ret, 0, 1);
    } else {
        add_next_index_zval(z_tab, &z_ret);
    }

    /* Success */
    return 0;
}

PHP_REDIS_API int
redis_read_raw_variant_reply(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock,
                             zval *z_tab, void *ctx)
{
    return variant_reply_generic(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock,
                                 redis_sock->reply_literal,
                                 redis_sock->null_mbulk_as_null,
                                 z_tab, ctx);
}

PHP_REDIS_API int
redis_read_variant_reply(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock,
                         zval *z_tab, void *ctx)
{
    return variant_reply_generic(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock, 0,
                                 redis_sock->null_mbulk_as_null, z_tab, ctx);
}

PHP_REDIS_API int
redis_read_variant_reply_strings(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock,
                                 zval *z_tab, void *ctx)
{
    return variant_reply_generic(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock, 1, 0, z_tab, ctx);
}

PHP_REDIS_API
int redis_extract_auth_info(zval *ztest, zend_string **user, zend_string **pass) {
    zval *zv;
    HashTable *ht;
    int num;

    /* The user may wish to send us something like [NULL, 'password'] or
     * [false, 'password'] so don't convert NULL or FALSE into "". */
    #define TRY_SET_AUTH_ARG(zv, ppzstr) \
        do { \
            if (Z_TYPE_P(zv) != IS_NULL && Z_TYPE_P(zv) != IS_FALSE) { \
                *(ppzstr) = zval_get_string(zv); \
            } \
        } while (0)

    /* Null out user and password */
    *user = *pass = NULL;

    /* User passed nothing */
    if (ztest == NULL)
        return FAILURE;

    /* Handle a non-array first */
    if (Z_TYPE_P(ztest) != IS_ARRAY) {
        *pass = zval_get_string(ztest);
        return SUCCESS;
    }

    /* Handle the array case */
    ht = Z_ARRVAL_P(ztest);
    num = zend_hash_num_elements(ht);

    /* Something other than one or two entries makes no sense */
    if (num != 1 && num != 2) {
        php_error_docref(NULL, E_WARNING, "When passing an array as auth it must have one or two elements!");
        return FAILURE;
    }

    if (num == 2) {
        if ((zv = REDIS_HASH_STR_FIND_STATIC(ht, "user")) ||
            (zv = zend_hash_index_find(ht, 0)))
        {
            TRY_SET_AUTH_ARG(zv, user);
        }

        if ((zv = REDIS_HASH_STR_FIND_STATIC(ht, "pass")) ||
            (zv = zend_hash_index_find(ht, 1)))
        {
            TRY_SET_AUTH_ARG(zv, pass);
        }
    } else if ((zv = REDIS_HASH_STR_FIND_STATIC(ht, "pass")) ||
               (zv = zend_hash_index_find(ht, 0)))
    {
        TRY_SET_AUTH_ARG(zv, pass);
    }

    /* If we at least have a password, we're good */
    if (*pass != NULL)
        return SUCCESS;

    /* Failure, clean everything up so caller doesn't need to care */
    if (*user) zend_string_release(*user);
    *user = NULL;

    return FAILURE;
}

/* Helper methods to extract configuration settings from a hash table */

zval *redis_hash_str_find_type(HashTable *ht, const char *key, int keylen, int type) {
    zval *zv = zend_hash_str_find(ht, key, keylen);
    if (zv == NULL || Z_TYPE_P(zv) != type)
        return NULL;

    return zv;
}

void redis_conf_double(HashTable *ht, const char *key, int keylen, double *dval) {
    zval *zv = zend_hash_str_find(ht, key, keylen);
    if (zv == NULL)
        return;

    *dval = zval_get_double(zv);
}

void redis_conf_bool(HashTable *ht, const char *key, int keylen, int *ival) {
    zend_string *zstr = NULL;

    redis_conf_string(ht, key, keylen, &zstr);
    if (zstr == NULL)
        return;

    *ival = zend_string_equals_literal_ci(zstr, "true") ||
            zend_string_equals_literal_ci(zstr, "yes") ||
            zend_string_equals_literal_ci(zstr, "1");

    zend_string_release(zstr);
}

void redis_conf_zend_bool(HashTable *ht, const char *key, int keylen, zend_bool *bval) {
    zval *zv = zend_hash_str_find(ht, key, keylen);
    if (zv == NULL)
        return;

    *bval = zend_is_true(zv);
}

void redis_conf_long(HashTable *ht, const char *key, int keylen, zend_long *lval) {
    zval *zv = zend_hash_str_find(ht, key, keylen);
    if (zv == NULL)
        return;

    *lval = zval_get_long(zv);
}

void redis_conf_int(HashTable *ht, const char *key, int keylen, int *ival) {
    zval *zv = zend_hash_str_find(ht, key, keylen);
    if (zv == NULL)
        return;

    *ival = zval_get_long(zv);
}

void redis_conf_string(HashTable *ht, const char *key, size_t keylen,
                       zend_string **sval)
{
    zval *zv = zend_hash_str_find(ht, key, keylen);
    if (zv == NULL)
        return;

    *sval = zval_get_string(zv);
}

void redis_conf_zval(HashTable *ht, const char *key, size_t keylen, zval *zret,
                     int copy, int dtor)
{
    zval *zv = zend_hash_str_find(ht, key, keylen);
    if (zv == NULL)
        return;

    ZVAL_ZVAL(zret, zv, copy, dtor);
}

void redis_conf_auth(HashTable *ht, const char *key, size_t keylen,
                     zend_string **user, zend_string **pass)
{
    zval *zv = zend_hash_str_find(ht, key, keylen);
    if (zv == NULL)
        return;

    redis_extract_auth_info(zv, user, pass);
}

/* vim: set tabstop=4 softtabstop=4 expandtab shiftwidth=4: */
