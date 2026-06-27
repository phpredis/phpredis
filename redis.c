/* -*- Mode: C; tab-width: 4 -*- */
/*
  +----------------------------------------------------------------------+
  | Copyright (c) 1997-2009 The PHP Group                                |
  +----------------------------------------------------------------------+
  | This source file is subject to version 3.01 of the PHP license,      |
  | that is bundled with this package in the file LICENSE, and is        |
  | available through the world-wide-web at the following url:           |
  | http://www.php.net/license/3_01.txt                                  |
  | If you did not receive a copy of the PHP license and are unable to   |
  | obtain it through the world-wide-web, please send a note to          |
  | license@php.net so we can mail you a copy immediately.               |
  +----------------------------------------------------------------------+
  | Original author: Alfonso Jimenez <yo@alfonsojimenez.com>             |
  | Maintainer: Nicolas Favre-Felix <n.favre-felix@owlient.eu>           |
  | Maintainer: Nasreddine Bouafif <n.bouafif@owlient.eu>                |
  | Maintainer: Michael Grunder <michael.grunder@gmail.com>              |
  +----------------------------------------------------------------------+
*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "php_redis.h"
#include "redis_array.h"
#include "redis_cluster.h"
#include "redis_commands.h"
#include "redis_sentinel.h"
#include <ext/spl/spl_exceptions.h>
#include <zend_exceptions.h>
#include <ext/standard/info.h>
#include <ext/hash/php_hash.h>

#ifdef PHP_SESSION
#include <ext/session/php_session.h>
#endif

#include "library.h"

#ifdef HAVE_REDIS_ZSTD
#include <zstd.h>
#endif

#ifdef HAVE_REDIS_LZ4
#include <lz4.h>
#endif

#ifdef PHP_SESSION
extern ps_module ps_mod_redis;
extern ps_module ps_mod_redis_cluster;
#endif

zend_class_entry *redis_ce;
zend_class_entry *redis_exception_ce;

extern RedisCmdCtx redis_empty_ctx;

#if PHP_VERSION_ID < 80000
#include "redis_legacy_arginfo.h"
#else
#include "zend_attributes.h"
#include "redis_arginfo.h"
#endif

extern int le_cluster_slot_cache;
int le_redis_pconnect;

PHP_INI_BEGIN()
    /* redis arrays */
    PHP_INI_ENTRY("redis.arrays.algorithm", "", PHP_INI_ALL, NULL)
    PHP_INI_ENTRY("redis.arrays.auth", "", PHP_INI_ALL, NULL)
    PHP_INI_ENTRY("redis.arrays.autorehash", "0", PHP_INI_ALL, NULL)
    PHP_INI_ENTRY("redis.arrays.connecttimeout", "0", PHP_INI_ALL, NULL)
    PHP_INI_ENTRY("redis.arrays.distributor", "", PHP_INI_ALL, NULL)
    PHP_INI_ENTRY("redis.arrays.functions", "", PHP_INI_ALL, NULL)
    PHP_INI_ENTRY("redis.arrays.hosts", "", PHP_INI_ALL, NULL)
    PHP_INI_ENTRY("redis.arrays.index", "0", PHP_INI_ALL, NULL)
    PHP_INI_ENTRY("redis.arrays.lazyconnect", "0", PHP_INI_ALL, NULL)
    PHP_INI_ENTRY("redis.arrays.names", "", PHP_INI_ALL, NULL)
    PHP_INI_ENTRY("redis.arrays.pconnect", "0", PHP_INI_ALL, NULL)
    PHP_INI_ENTRY("redis.arrays.previous", "", PHP_INI_ALL, NULL)
    PHP_INI_ENTRY("redis.arrays.readtimeout", "0", PHP_INI_ALL, NULL)
    PHP_INI_ENTRY("redis.arrays.retryinterval", "0", PHP_INI_ALL, NULL)
    PHP_INI_ENTRY("redis.arrays.consistent", "0", PHP_INI_ALL, NULL)

    /* redis cluster */
    PHP_INI_ENTRY("redis.clusters.cache_slots", "0", PHP_INI_ALL, NULL)
    PHP_INI_ENTRY("redis.clusters.auth", "", PHP_INI_ALL, NULL)
    PHP_INI_ENTRY("redis.clusters.persistent", "0", PHP_INI_ALL, NULL)
    PHP_INI_ENTRY("redis.clusters.read_timeout", "0", PHP_INI_ALL, NULL)
    PHP_INI_ENTRY("redis.clusters.seeds", "", PHP_INI_ALL, NULL)
    PHP_INI_ENTRY("redis.clusters.timeout", "0", PHP_INI_ALL, NULL)

    /* redis pconnect */
    PHP_INI_ENTRY("redis.pconnect.pooling_enabled", "1", PHP_INI_ALL, NULL)
    PHP_INI_ENTRY("redis.pconnect.connection_limit", "0", PHP_INI_ALL, NULL)
    PHP_INI_ENTRY("redis.pconnect.echo_check_liveness", "1", PHP_INI_ALL, NULL)
    PHP_INI_ENTRY("redis.pconnect.pool_detect_dirty", "0", PHP_INI_ALL, NULL)
    PHP_INI_ENTRY("redis.pconnect.pool_poll_timeout", "0", PHP_INI_ALL, NULL)
    PHP_INI_ENTRY("redis.pconnect.pool_pattern", "", PHP_INI_ALL, NULL)

    /* redis session */
    PHP_INI_ENTRY("redis.session.locking_enabled", "0", PHP_INI_ALL, NULL)
    PHP_INI_ENTRY("redis.session.lock_release_cmd", "eval", PHP_INI_ALL, NULL)
    PHP_INI_ENTRY("redis.session.lock_expire", "0", PHP_INI_ALL, NULL)
    PHP_INI_ENTRY("redis.session.lock_retries", "100", PHP_INI_ALL, NULL)
    PHP_INI_ENTRY("redis.session.lock_wait_time", "20000", PHP_INI_ALL, NULL)
    PHP_INI_ENTRY("redis.session.lock_failure_readonly", "0", PHP_INI_ALL, NULL)
    PHP_INI_ENTRY("redis.session.early_refresh", "0", PHP_INI_ALL, NULL)
    PHP_INI_ENTRY("redis.session.compression", "none", PHP_INI_ALL, NULL)
    PHP_INI_ENTRY("redis.session.compression_level", "3", PHP_INI_ALL, NULL)
PHP_INI_END()

static const zend_module_dep redis_deps[] = {
#ifdef HAVE_REDIS_IGBINARY
     ZEND_MOD_REQUIRED("igbinary")
#endif
#ifdef HAVE_REDIS_MSGPACK
     ZEND_MOD_REQUIRED("msgpack")
#endif
#ifdef HAVE_REDIS_JSON
     ZEND_MOD_REQUIRED("json")
#endif
#ifdef PHP_SESSION
     ZEND_MOD_REQUIRED("session")
#endif
     ZEND_MOD_END
};

ZEND_DECLARE_MODULE_GLOBALS(redis)

zend_module_entry redis_module_entry = {
     STANDARD_MODULE_HEADER_EX,
     NULL,
     redis_deps,
     "redis",
     NULL,
     PHP_MINIT(redis),
     NULL,
     NULL,
     NULL,
     PHP_MINFO(redis),
     PHP_REDIS_VERSION,
     PHP_MODULE_GLOBALS(redis),
     NULL,
     NULL,
     NULL,
     STANDARD_MODULE_PROPERTIES_EX
};

#ifdef COMPILE_DL_REDIS
ZEND_GET_MODULE(redis)
#endif

zend_object_handlers redis_object_handlers;

/* Send a static DISCARD in case we're in MULTI mode. */
static int
redis_send_discard(RedisSock *redis_sock)
{
    char *resp;
    int resp_len, result = FAILURE;

    /* send our DISCARD command */
    if (redis_sock_write(redis_sock, ZEND_STRL(RESP_DISCARD_CMD)) >= 0 &&
       (resp = redis_sock_read(redis_sock,&resp_len)) != NULL)
    {
        /* success if we get OK */
        result = (resp_len == 3 && redis_strncmp(resp, ZEND_STRL("+OK")) == 0) ? SUCCESS:FAILURE;

        /* free our response */
        efree(resp);
    }

    /* return success/failure */
    return result;
}

/* Passthru for destroying cluster cache */
static void cluster_cache_dtor(zend_resource *rsrc) {
    if (rsrc->ptr) {
        cluster_cache_free(rsrc->ptr);
    }
}

void
free_redis_object(zend_object *object)
{
    redis_object *redis = PHPREDIS_GET_OBJECT(redis_object, object);

    zend_object_std_dtor(&redis->std);
    if (redis->sock) {
        redis_sock_disconnect(redis->sock, 0, 1);
        redis_free_socket(redis->sock);
    }
}

zend_object *
create_redis_object(zend_class_entry *ce)
{
    redis_object *redis = ecalloc(1, sizeof(redis_object) + zend_object_properties_size(ce));

    redis->sock = NULL;

    zend_object_std_init(&redis->std, ce);
    object_properties_init(&redis->std, ce);

    redis->std.handlers = &redis_object_handlers;

    return &redis->std;
}

static zend_always_inline RedisSock *
redis_sock_get_instance(zval *id, int no_throw)
{
    redis_object *redis;

    if (Z_TYPE_P(id) == IS_OBJECT) {
        redis = PHPREDIS_ZVAL_GET_OBJECT(redis_object, id);
        if (redis->sock) {
            return redis->sock;
        }
    }
    // Throw an exception unless we've been requested not to
    if (!no_throw) {
        REDIS_THROW_EXCEPTION("Redis server went away", 0);
    }
    return NULL;
}

static zend_never_inline ZEND_COLD void redis_sock_throw_exception(RedisSock *redis_sock) {
    char *errmsg = NULL;
    if (redis_sock->status == REDIS_SOCK_STATUS_AUTHENTICATED) {
        if (redis_sock->err != NULL) {
            spprintf(&errmsg, 0, "Could not select database %ld '%s'", redis_sock->dbNumber, ZSTR_VAL(redis_sock->err));
        } else {
            spprintf(&errmsg, 0, "Could not select database %ld", redis_sock->dbNumber);
        }
    } else if (redis_sock->status == REDIS_SOCK_STATUS_CONNECTED) {
        if (redis_sock->err != NULL) {
            spprintf(&errmsg, 0, "Could not authenticate '%s'", ZSTR_VAL(redis_sock->err));
        } else {
            spprintf(&errmsg, 0, "Could not authenticate");
        }
    } else {
        if (redis_sock->port < 0) {
            spprintf(&errmsg, 0, "Redis server %s went away", ZSTR_VAL(redis_sock->host));
        } else {
            spprintf(&errmsg, 0, "Redis server %s:%d went away", ZSTR_VAL(redis_sock->host), redis_sock->port);
        }
    }
    REDIS_THROW_EXCEPTION(errmsg, 0);
    efree(errmsg);
}

/**
 * redis_sock_get
 */
PHP_REDIS_API RedisSock *
redis_sock_get(zval *id, int no_throw)
{
    RedisSock *redis_sock;

    if ((redis_sock = redis_sock_get_instance(id, no_throw)) == NULL) {
        return NULL;
    }

    if (UNEXPECTED(redis_sock_server_open(redis_sock) < 0)) {
        if (!no_throw) {
            redis_sock_throw_exception(redis_sock);
        }
        return NULL;
    }

    return redis_sock;
}

/**
 * redis_sock_get_direct
 * Returns our attached RedisSock pointer if we're connected
 */
PHP_REDIS_API RedisSock *redis_sock_get_connected(INTERNAL_FUNCTION_PARAMETERS) {
    RedisSock *redis_sock;

    // If we can't grab our object, or get a socket, or we're not connected,
    // return NULL
    if ((redis_sock = redis_sock_get(getThis(), 1)) == NULL ||
       redis_sock->status < REDIS_SOCK_STATUS_CONNECTED)
    {
        return NULL;
    }

    /* Return our socket */
    return redis_sock;
}

static ZEND_RSRC_DTOR_FUNC(redis_connections_pool_dtor)
{
    if (res->ptr) {
        ConnectionPool *p = res->ptr;
        zend_llist_destroy(&p->list);
        pefree(res->ptr, 1);
    }
}

static void redis_random_hex_bytes(char *dst, size_t dstsize) {
    char chunk[9], *ptr = dst;
    ssize_t rem = dstsize, len, clen;
    size_t bytes;

    /* We need two characters per hex byte */
    bytes = dstsize / 2;
    zend_string *s = zend_string_alloc(bytes, 0);

    /* First try to have PHP generate the bytes */
    if (php_random_bytes_silent(ZSTR_VAL(s), bytes) == SUCCESS) {
        php_hash_bin2hex(dst, (unsigned char *)ZSTR_VAL(s), bytes);
        zend_string_release(s);
        return;
    }

    /* PHP shouldn't have failed, but generate manually if it did. */
    while (rem > 0) {
        clen = snprintf(chunk, sizeof(chunk), "%08x", rand());
        len = rem >= clen ? clen : rem;
        memcpy(ptr, chunk, len);
        ptr += len; rem -= len;
    }

    zend_string_release(s);
}

static void redis_init_object_handlers(void) {
    memcpy(&redis_object_handlers, zend_get_std_object_handlers(),
           sizeof(redis_object_handlers));
    redis_object_handlers.offset = XtOffsetOf(redis_object, std);
    redis_object_handlers.free_obj = free_redis_object;
    redis_object_handlers.clone_obj = NULL;
}

/**
 * PHP_MINIT_FUNCTION
 */
PHP_MINIT_FUNCTION(redis)
{
    struct timeval tv;

    /* Seed random generator (for RedisCluster failover) */
    gettimeofday(&tv, NULL);
    srand(tv.tv_usec * tv.tv_sec);

    /* Generate our random salt */
    redis_random_hex_bytes(REDIS_G(salt), sizeof(REDIS_G(salt)) - 1);
    REDIS_G(salt)[sizeof(REDIS_G(salt)) - 1] = '\0';

    REGISTER_INI_ENTRIES();

    /* Redis class */
    redis_ce = register_class_Redis();
    redis_ce->create_object = create_redis_object;

    /* Redis object handler initialization */
    redis_init_object_handlers();

    /* RedisArray class */
    ZEND_MINIT(redis_array)(INIT_FUNC_ARGS_PASSTHRU);

    /* RedisCluster class */
    ZEND_MINIT(redis_cluster)(INIT_FUNC_ARGS_PASSTHRU);

    /* RedisSentinel class */
    ZEND_MINIT(redis_sentinel)(INIT_FUNC_ARGS_PASSTHRU);

    /* Register our cluster cache list item */
    le_cluster_slot_cache = zend_register_list_destructors_ex(NULL, cluster_cache_dtor,
                                                              "Redis cluster slot cache",
                                                              module_number);

    /* RedisException class */
    redis_exception_ce = register_class_RedisException(spl_ce_RuntimeException);

#ifdef PHP_SESSION
    php_session_register_module(&ps_mod_redis);
    php_session_register_module(&ps_mod_redis_cluster);
#endif

    /* Register resource destructors */
    le_redis_pconnect = zend_register_list_destructors_ex(NULL, redis_connections_pool_dtor,
        "phpredis persistent connections pool", module_number);

    return SUCCESS;
}

static const char *
get_available_serializers(void)
{
#ifdef HAVE_REDIS_JSON
    #ifdef HAVE_REDIS_IGBINARY
        #ifdef HAVE_REDIS_MSGPACK
            return "php, json, igbinary, msgpack";
        #else
            return "php, json, igbinary";
        #endif
    #else
        #ifdef HAVE_REDIS_MSGPACK
            return "php, json, msgpack";
        #else
            return "php, json";
        #endif
    #endif
#else
    #ifdef HAVE_REDIS_IGBINARY
        #ifdef HAVE_REDIS_MSGPACK
            return "php, igbinary, msgpack";
        #else
            return "php, igbinary";
        #endif
    #else
        #ifdef HAVE_REDIS_MSGPACK
            return "php, msgpack";
        #else
            return "php";
        #endif
    #endif
#endif
}

/**
 * PHP_MINFO_FUNCTION
 */
PHP_MINFO_FUNCTION(redis)
{
    smart_str names = {0,};

    php_info_print_table_start();
    php_info_print_table_header(2, "Redis Support", "enabled");
    php_info_print_table_row(2, "Redis Version", PHP_REDIS_VERSION);
    php_info_print_table_row(2, "Redis Sentinel Version", PHP_REDIS_SENTINEL_VERSION);
#ifdef GIT_REVISION
    php_info_print_table_row(2, "Git revision", "$Id: " GIT_REVISION " $");
#endif
    php_info_print_table_row(2, "Available serializers", get_available_serializers());
#ifdef HAVE_REDIS_LZF
    smart_str_appends(&names, "lzf");
#endif
#ifdef HAVE_REDIS_ZSTD
    if (names.s) {
        smart_str_appends(&names, ", ");
    }
    smart_str_appends(&names, "zstd");
#endif
#ifdef HAVE_REDIS_LZ4
    if (names.s) {
        smart_str_appends(&names, ", ");
    }
    smart_str_appends(&names, "lz4");
#endif
    if (names.s) {
        smart_str_0(&names);
        php_info_print_table_row(2, "Available compression", ZSTR_VAL(names.s));
    }
    smart_str_free(&names);
    php_info_print_table_end();

    DISPLAY_INI_ENTRIES();
}

PHP_METHOD(Redis, __construct)
{
    HashTable *opts = NULL;
    redis_object *redis;

    ZEND_PARSE_PARAMETERS_START(0, 1)
        Z_PARAM_OPTIONAL
        Z_PARAM_ARRAY_HT_OR_NULL(opts)
    ZEND_PARSE_PARAMETERS_END_EX(RETURN_THROWS());

    redis = PHPREDIS_ZVAL_GET_OBJECT(redis_object, getThis());
    redis->sock = redis_sock_create(REDIS_SOCK_STANDALONE, ZEND_STRL("127.0.0.1"),
                                    6379, 0, 0, 0, NULL, 0);
    if (opts != NULL && redis_sock_configure(redis->sock, opts) != SUCCESS) {
        RETURN_THROWS();
    }
}

PHP_METHOD(Redis, __destruct) {
    if (zend_parse_parameters_none() == FAILURE) {
        RETURN_FALSE;
    }

    // Grab our socket
    RedisSock *redis_sock;
    if ((redis_sock = redis_sock_get_instance(getThis(), 1)) == NULL) {
        RETURN_FALSE;
    }

    // If we think we're in MULTI mode, send a discard
    if (redis_sock_is_multi(redis_sock)) {
        if (!redis_sock_is_pipeline(redis_sock) && redis_sock->stream) {
            redis_send_discard(redis_sock);
        }
        redis_free_reply_callbacks(redis_sock);
    }
}

PHP_METHOD(Redis, connect)
{
    if (redis_connect(INTERNAL_FUNCTION_PARAM_PASSTHRU, 0) == FAILURE) {
        RETURN_FALSE;
    } else {
        RETURN_TRUE;
    }
}

PHP_METHOD(Redis, pconnect)
{
    if (redis_connect(INTERNAL_FUNCTION_PARAM_PASSTHRU, 1) == FAILURE) {
        RETURN_FALSE;
    } else {
        RETURN_TRUE;
    }
}

PHP_REDIS_API int
redis_connect(INTERNAL_FUNCTION_PARAMETERS, int persistent)
{
    zval *context = NULL, *ele;
    char *host = NULL, *persistent_id = NULL;
    zend_long port = -1, retry_interval = 0;
    size_t host_len, persistent_id_len;
    double timeout = 0.0, read_timeout = 0.0;
    redis_object *redis;
    int af_unix;

#ifdef ZTS
    /* not sure how in threaded mode this works so disabled persistence at
     * first */
    persistent = 0;
#endif

    ZEND_PARSE_PARAMETERS_START(1, 7)
        Z_PARAM_STRING(host, host_len)
        Z_PARAM_OPTIONAL
        Z_PARAM_LONG(port)
        Z_PARAM_DOUBLE(timeout)
        Z_PARAM_STRING_OR_NULL(persistent_id, persistent_id_len)
        Z_PARAM_LONG(retry_interval)
        Z_PARAM_DOUBLE(read_timeout)
        Z_PARAM_ARRAY_OR_NULL(context)
    ZEND_PARSE_PARAMETERS_END_EX(return FAILURE);

    /* Disregard persistent_id if we're not opening a persistent connection */
    if (!persistent) {
        persistent_id = NULL;
    }

    if (timeout > INT_MAX) {
        REDIS_VALUE_EXCEPTION("Invalid connect timeout");
        return FAILURE;
    }

    if (read_timeout > INT_MAX) {
        REDIS_VALUE_EXCEPTION("Invalid read timeout");
        return FAILURE;
    }

    if (retry_interval < 0L || retry_interval > INT_MAX) {
        REDIS_VALUE_EXCEPTION("Invalid retry interval");
        return FAILURE;
    }

    /* Does the host look like a unix socket */
    af_unix = (host_len > 0 && host[0] == '/') ||
              (host_len > 6 && (!strncasecmp(host, "unix://", sizeof("unix://") - 1) ||
                                !strncasecmp(host, "file://", sizeof("file://") - 1)));

    /* If it's not a unix socket, set to default */
    if (port == -1 && !af_unix) {
        port = 6379;
    }

    redis = PHPREDIS_ZVAL_GET_OBJECT(redis_object, getThis());

    /* if there is a redis sock already we have to remove it */
    if (redis->sock) {
        redis_sock_disconnect(redis->sock, 0, 1);
        redis_free_socket(redis->sock);
    }

    redis->sock = redis_sock_create(REDIS_SOCK_STANDALONE, host, host_len, port,
                                    timeout, read_timeout, persistent,
                                    persistent_id, retry_interval);

    if (context) {
        /* Stream context (e.g. TLS) */
        if ((ele = REDIS_HASH_STR_FIND_STATIC(Z_ARRVAL_P(context), "stream"))) {
            redis_sock_set_context_zval(redis->sock, ele);
        }

        /* AUTH */
        if ((ele = REDIS_HASH_STR_FIND_STATIC(Z_ARRVAL_P(context), "auth"))) {
            redis_sock_set_auth_zval(redis->sock, ele);
        }
    }

    if (redis_sock_connect(redis->sock) != SUCCESS) {
        if (redis->sock->err) {
            REDIS_THROW_EXCEPTION(ZSTR_VAL(redis->sock->err), 0);
        }
        redis_free_socket(redis->sock);
        redis->sock = NULL;
        return FAILURE;
    }

    return SUCCESS;
}

static zend_always_inline void
pipeline_enqueue_cmd_strl(RedisSock *redis_sock, const char *cmd, int len) {
    smart_string_appendl(&redis_sock->pipeline_cmd, cmd, len);
}

static zend_always_inline void
pipeline_enqueue_cmd(RedisSock *redis_sock, RedisCmd *cmd) {
    pipeline_enqueue_cmd_strl(redis_sock, redis_cmd_str(cmd),
                              redis_cmd_len(cmd));
}

static void
redis_save_callback(RedisSock *redis_sock, FailableResultCallback cb,
                    RedisCmdCtx ctx)
{
    fold_item *fi;

    fi = redis_add_reply_callback(redis_sock);
    fi->fun = cb;
    fi->flags = redis_sock->flags;
    fi->ctx = ctx;
}

static int
redis_process_request_strl(RedisSock *redis_sock, const char *cmd, int len) {
    int res = SUCCESS;

    if (redis_sock_is_pipeline(redis_sock)) {
        pipeline_enqueue_cmd_strl(redis_sock, cmd, len);
    } else if (UNEXPECTED(redis_sock_write(redis_sock, cmd, len) < 0)) {
        res = FAILURE;
    }

    return res;
}

static int
redis_process_request(RedisSock *redis_sock, RedisCmd *cmd) {
    return redis_process_request_strl(redis_sock, redis_cmd_str(cmd),
                                      redis_cmd_len(cmd));
}

static void
redis_process_cmd(INTERNAL_FUNCTION_PARAMETERS, redis_cmd_cb *cmd_cb,
                  FailableResultCallback resp_cb)
{
    RedisSock *redis_sock;
    RedisCmdCtx ctx;
    RedisCmd *cmd;
    int res;

    redis_sock = redis_sock_get(getThis(), 0);
    if (UNEXPECTED(redis_sock == NULL)) {
        RETURN_FALSE;
    }

    cmd = cmd_cb(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock);
    if (UNEXPECTED(cmd == NULL)) {
        RETURN_FALSE;
    }

    ctx = redis_cmd_pop_ctx(cmd);

    if (redis_sock_is_pipeline(redis_sock)) {
        pipeline_enqueue_cmd(redis_sock, cmd);
        res = SUCCESS;
    } else {
        res = redis_sock_write_cmd(redis_sock, cmd) < 0 ? FAILURE : SUCCESS;
    }

    redis_cmd_free(cmd);

    if (UNEXPECTED(res != SUCCESS)) {
        redis_cmd_ctx_free(ctx);
        RETURN_FALSE;
    }

    if (redis_sock_is_atomic(redis_sock)) {
        resp_cb(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock, NULL, ctx);
        redis_cmd_ctx_free(ctx);
    } else {
        if (!redis_sock_is_pipeline(redis_sock)) {
            if (redis_response_enqueued(redis_sock) != SUCCESS) {
                redis_cmd_ctx_free(ctx);
                RETURN_FALSE;
            }
        }

        redis_save_callback(redis_sock, resp_cb, ctx);
        RETURN_ZVAL(getThis(), 1, 0);
    }
}

#define REDIS_METHOD(method, cmd_cb, resp_func) \
    PHP_METHOD(Redis, method) { \
        redis_process_cmd(INTERNAL_FUNCTION_PARAM_PASSTHRU, cmd_cb, resp_func); \
    }

#define REDIS_KW_METHOD(method, kw, cmd_cb, resp_func) \
    PHP_METHOD(Redis, method) { \
        redis_process_kw_cmd(INTERNAL_FUNCTION_PARAM_PASSTHRU, kw, cmd_cb, \
                             resp_func); \
    }

void
redis_process_kw_cmd(INTERNAL_FUNCTION_PARAMETERS, const char *kw,
                     redis_kw_cmd_cb *cmd_cb, FailableResultCallback resp_cb)
{
    RedisSock *redis_sock;
    RedisCmdCtx ctx;
    RedisCmd *cmd;
    int res;

    redis_sock = redis_sock_get(getThis(), 0);
    if (UNEXPECTED(redis_sock == NULL)) {
        RETURN_FALSE;
    }

    cmd = cmd_cb(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock, (char*)kw);
    if (UNEXPECTED(cmd == NULL)) {
        RETURN_FALSE;
    }

    ctx = redis_cmd_pop_ctx(cmd);

    if (redis_sock_is_pipeline(redis_sock)) {
        pipeline_enqueue_cmd(redis_sock, cmd);
        res = SUCCESS;
    } else {
        res = redis_sock_write_cmd(redis_sock, cmd) < 0 ? FAILURE : SUCCESS;
    }

    redis_cmd_free(cmd);

    if (UNEXPECTED(res != SUCCESS)) {
        redis_cmd_ctx_free(ctx);
        RETURN_FALSE;
    }

    if (redis_sock_is_atomic(redis_sock)) {
        resp_cb(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock, NULL, ctx);
        redis_cmd_ctx_free(ctx);
    } else {
        if (!redis_sock_is_pipeline(redis_sock)) {
            if (redis_response_enqueued(redis_sock) != SUCCESS) {
                redis_cmd_ctx_free(ctx);
                RETURN_FALSE;
            }
        }

        redis_save_callback(redis_sock, resp_cb, ctx);
        RETURN_ZVAL(getThis(), 1, 0);
    }
}

PHP_METHOD(Redis, close)
{
    RedisSock *redis_sock = redis_sock_get_connected(INTERNAL_FUNCTION_PARAM_PASSTHRU);

    if (redis_sock_disconnect(redis_sock, 1, 1) == SUCCESS) {
        RETURN_TRUE;
    }
    RETURN_FALSE;
}

PHP_METHOD(Redis, reset)
{
    char *response;
    int response_len;
    RedisSock *redis_sock;
    RedisCmd *cmd;
    zend_bool ret = 0;

    if ((redis_sock = redis_sock_get(getThis(), 0)) == NULL) {
        RETURN_FALSE;
    }

    if (redis_sock_is_pipeline(redis_sock)) {
        php_error_docref(NULL, E_ERROR,
            "Reset isn't allowed in pipeline mode!");
        RETURN_FALSE;
    }

    cmd = redis_cmd_create_literal(NULL, "RESET");

    if (redis_process_request(redis_sock, cmd) != SUCCESS) {
        redis_cmd_free(cmd);
        RETURN_FALSE;
    }
    redis_cmd_free(cmd);

    if ((response = redis_sock_read(redis_sock, &response_len)) != NULL) {
        ret = redis_str_eq(response, response_len, ZEND_STRL("+RESET"));
        efree(response);
    }

    if (!ret) {
        if (redis_sock_is_atomic(redis_sock)) {
            RETURN_FALSE;
        }
        REDIS_THROW_EXCEPTION("Reset failed in multi mode!", 0);
        RETURN_ZVAL(getThis(), 1, 0);
    }

    redis_free_reply_callbacks(redis_sock);
    redis_sock->status = REDIS_SOCK_STATUS_CONNECTED;
    redis_sock->mode = ATOMIC;
    redis_sock->dbNumber = 0;
    redis_sock->watching = 0;

    RETURN_TRUE;
}

PHP_REDIS_API void redis_set_watch(RedisSock *redis_sock)
{
    redis_sock->watching = 1;
}

PHP_REDIS_API int
redis_watch_response(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock,
                     zval *z_tab, RedisCmdCtx ctx)
{
    return redis_boolean_response_impl(INTERNAL_FUNCTION_PARAM_PASSTHRU,
                                       redis_sock, z_tab, ctx, redis_set_watch);
}

PHP_REDIS_API void redis_clear_watch(RedisSock *redis_sock)
{
    redis_sock->watching = 0;
}

PHP_REDIS_API int redis_unwatch_response(INTERNAL_FUNCTION_PARAMETERS,
                                   RedisSock *redis_sock, zval *z_tab,
                                   RedisCmdCtx ctx)
{
    return redis_boolean_response_impl(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock,
                                       z_tab, ctx, redis_clear_watch);
}

PHP_METHOD(Redis, sPop)
{
    if (ZEND_NUM_ARGS() == 1) {
        REDIS_PROCESS_KW_CMD("SPOP", redis_key_cmd, redis_string_response);
    } else if (ZEND_NUM_ARGS() == 2) {
        REDIS_PROCESS_KW_CMD("SPOP", redis_key_long_cmd, redis_sock_read_multibulk_reply);
    } else {
        zend_wrong_param_count();
    }

}

static RedisCmd *
generic_sort_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock, int desc,
                 int alpha)
{
    zend_string *key = NULL, *pattern = NULL, *store = NULL, *zpattern;
    zval *zele, *zget = NULL;
    zend_long offset = -1, count = -1;
    RedisCmd *cmd;

    ZEND_PARSE_PARAMETERS_START(1, 6)
        Z_PARAM_STR(key)
        Z_PARAM_OPTIONAL
        Z_PARAM_STR_OR_NULL(pattern)
        Z_PARAM_ZVAL_OR_NULL(zget)
        Z_PARAM_LONG(offset)
        Z_PARAM_LONG(count)
        Z_PARAM_STR(store)
    ZEND_PARSE_PARAMETERS_END_EX(return NULL);

    /* Ensure we're sorting something, and we can get context */
    if (ZSTR_LEN(key) == 0)
        return NULL;

    /* Start constructing final command and append key */
    cmd = redis_cmd_create_literal(redis_sock, "SORT");
    if (!redis_cmd_cat_key_zstr(cmd, key)) {
        redis_cmd_free(cmd);
        return NULL;
    }

    /* BY pattern */
    if (pattern && ZSTR_LEN(pattern)) {
        redis_cmd_cat_literal(cmd, "BY");
        redis_cmd_cat_zstr(cmd, pattern);
    }

    /* LIMIT offset count */
    if (offset >= 0 && count >= 0) {
        redis_cmd_cat_literal(cmd, "LIMIT");
        redis_cmd_cat_long(cmd, offset);
        redis_cmd_cat_long(cmd, count);
    }

    /* Handle any number of GET pattern arguments we've been passed */
    if (zget != NULL) {
        if (Z_TYPE_P(zget) == IS_ARRAY) {
            ZEND_HASH_FOREACH_VAL(Z_ARRVAL_P(zget), zele) {
                zpattern = zval_get_string(zele);
                redis_cmd_cat_literal(cmd, "GET");
                redis_cmd_cat_zstr(cmd, zpattern);
                zend_string_release(zpattern);
            } ZEND_HASH_FOREACH_END();
        } else {
            zpattern = zval_get_string(zget);
            redis_cmd_cat_literal(cmd, "GET");
            redis_cmd_cat_zstr(cmd, zpattern);
            zend_string_release(zpattern);
        }
    }

    /* Append optional DESC and ALPHA modifiers */
    if (desc)  redis_cmd_cat_literal(cmd, "DESC");
    if (alpha) redis_cmd_cat_literal(cmd, "ALPHA");

    /* Finally append STORE if we've got it */
    if (store && ZSTR_LEN(store)) {
        redis_cmd_cat_literal(cmd, "STORE");
        if (!redis_cmd_cat_key_zstr(cmd, store)) {
            redis_cmd_free(cmd);
            return NULL;
        }
    }

    return cmd;
}

PHP_METHOD(Redis, zPopMax)
{
    if (ZEND_NUM_ARGS() == 1) {
        REDIS_PROCESS_KW_CMD("ZPOPMAX", redis_key_cmd, redis_mbulk_reply_zipped_keys_dbl);
    } else if (ZEND_NUM_ARGS() == 2) {
        REDIS_PROCESS_KW_CMD("ZPOPMAX", redis_key_long_cmd, redis_mbulk_reply_zipped_keys_dbl);
    } else {
        zend_wrong_param_count();
    }
}

PHP_METHOD(Redis, zPopMin)
{
    if (ZEND_NUM_ARGS() == 1) {
        REDIS_PROCESS_KW_CMD("ZPOPMIN", redis_key_cmd, redis_mbulk_reply_zipped_keys_dbl);
    } else if (ZEND_NUM_ARGS() == 2) {
        REDIS_PROCESS_KW_CMD("ZPOPMIN", redis_key_long_cmd, redis_mbulk_reply_zipped_keys_dbl);
    } else {
        zend_wrong_param_count();
    }
}

/* flag : get, set {ATOMIC, MULTI, PIPELINE} */

PHP_METHOD(Redis, multi)
{

    RedisSock *redis_sock;
    char *resp;
    int resp_len;
    zend_long multi_value = MULTI;

    ZEND_PARSE_PARAMETERS_START(0, 1)
        Z_PARAM_OPTIONAL
        Z_PARAM_LONG(multi_value)
    ZEND_PARSE_PARAMETERS_END_EX(RETURN_FALSE);

    /* if the flag is activated, send the command, the reply will be "QUEUED"
     * or -ERR */
    if ((redis_sock = redis_sock_get(getThis(), 0)) == NULL) {
        RETURN_FALSE;
    }

    if (multi_value == PIPELINE) {
        /* Cannot enter pipeline mode in a MULTI block */
        if (redis_sock_is_multi(redis_sock)) {
            php_error_docref(NULL, E_ERROR, "Can't activate pipeline in multi mode!");
            RETURN_FALSE;
        }

        /* Enable PIPELINE if we're not already in one */
        if (redis_sock_is_atomic(redis_sock)) {
            redis_sock->mode |= PIPELINE;
        }
    } else if (multi_value == MULTI) {
        /* Don't want to do anything if we're already in MULTI mode */
        if (!redis_sock_is_multi(redis_sock)) {
            if (redis_sock_is_pipeline(redis_sock)) {
                pipeline_enqueue_cmd_strl(redis_sock, ZEND_STRL(RESP_MULTI_CMD));
                redis_save_callback(redis_sock, NULL, redis_empty_ctx);
                redis_sock->mode |= MULTI;
            } else {
                if (redis_sock_write(redis_sock, ZEND_STRL(RESP_MULTI_CMD)) < 0) {
                    RETURN_FALSE;
                }
                if ((resp = redis_sock_read(redis_sock, &resp_len)) == NULL) {
                    RETURN_FALSE;
                } else if (redis_strncmp(resp, ZEND_STRL("+OK")) != 0) {
                    efree(resp);
                    RETURN_FALSE;
                }
                efree(resp);
                redis_sock->mode |= MULTI;
            }
        }
    } else {
        php_error_docref(NULL, E_WARNING, "Unknown mode sent to Redis::multi");
        RETURN_FALSE;
    }

    RETURN_ZVAL(getThis(), 1, 0);
}

/* discard */
PHP_METHOD(Redis, discard)
{
    int ret = FAILURE;
    RedisSock *redis_sock;

    if (zend_parse_parameters_none() == FAILURE) {
        RETURN_FALSE;
    }

    if ((redis_sock = redis_sock_get(getThis(), 0)) == NULL) {
        RETURN_FALSE;
    }

    if (redis_sock_is_pipeline(redis_sock)) {
        ret = SUCCESS;
        smart_string_free(&redis_sock->pipeline_cmd);
    } else if (redis_sock_is_multi(redis_sock)) {
        ret = redis_send_discard(redis_sock);
    }
    if (ret == SUCCESS) {
        redis_free_reply_callbacks(redis_sock);
        redis_sock->mode = ATOMIC;
        RETURN_TRUE;
    }
    RETURN_FALSE;
}

PHP_REDIS_API int
redis_sock_read_multibulk_multi_reply(INTERNAL_FUNCTION_PARAMETERS,
                                      RedisSock *redis_sock, zval *z_tab)
{

    char inbuf[4096];
    size_t len;

    if (redis_sock_gets(redis_sock, inbuf, sizeof(inbuf) - 1, &len) < 0 ||
        *inbuf != TYPE_MULTIBULK || atoi(inbuf + 1) < 0
    ) {
        return FAILURE;
    }

    // No command issued, return empty immutable array
    if (redis_sock->reply_callback == NULL) {
        ZVAL_EMPTY_ARRAY(z_tab);
        return SUCCESS;
    }

    array_init(z_tab);

    return redis_sock_read_multibulk_multi_reply_loop(INTERNAL_FUNCTION_PARAM_PASSTHRU,
                    redis_sock, z_tab);
}


/* exec */
PHP_METHOD(Redis, exec)
{
    RedisSock *redis_sock;
    int ret;
    zval z_ret;

    if ((redis_sock = redis_sock_get(getThis(), 0)) == NULL) {
        RETURN_FALSE;
    }

    ZVAL_FALSE(&z_ret);

    if (redis_sock_is_multi(redis_sock)) {
        if (redis_sock_is_pipeline(redis_sock)) {
            pipeline_enqueue_cmd_strl(redis_sock, ZEND_STRL(RESP_EXEC_CMD));
            redis_save_callback(redis_sock, NULL, redis_empty_ctx);
            redis_sock->mode &= ~MULTI;
            RETURN_ZVAL(getThis(), 1, 0);
        }
        if (redis_sock_write(redis_sock, ZEND_STRL(RESP_EXEC_CMD)) < 0) {
            RETURN_FALSE;
        }
        ret = redis_sock_read_multibulk_multi_reply(
            INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock, &z_ret);
        redis_free_reply_callbacks(redis_sock);
        redis_sock->mode &= ~MULTI;
        redis_sock->watching = 0;
        if (ret < 0) {
            zval_ptr_dtor_nogc(&z_ret);
            ZVAL_FALSE(&z_ret);
        }
    }

    if (redis_sock_is_pipeline(redis_sock)) {
        if (redis_sock->pipeline_cmd.len == 0) {
            /* Empty array when no command was run. */
            ZVAL_EMPTY_ARRAY(&z_ret);
        } else {
            if (redis_sock_write(redis_sock, redis_sock->pipeline_cmd.c,
                    redis_sock->pipeline_cmd.len) < 0) {
                ZVAL_FALSE(&z_ret);
            } else {
                array_init(&z_ret);
                if (redis_sock_read_multibulk_multi_reply_loop(
                    INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock, &z_ret) != SUCCESS) {
                    zval_ptr_dtor_nogc(&z_ret);
                    ZVAL_FALSE(&z_ret);
                }
            }
            smart_string_free(&redis_sock->pipeline_cmd);
        }
        redis_free_reply_callbacks(redis_sock);
        redis_sock->mode &= ~PIPELINE;
    }
    RETURN_ZVAL(&z_ret, 0, 1);
}

PHP_REDIS_API int
redis_response_enqueued(RedisSock *redis_sock)
{
    char *resp;
    int resp_len, ret = FAILURE;

    if ((resp = redis_sock_read(redis_sock, &resp_len)) != NULL) {
        if (redis_strncmp(resp, ZEND_STRL("+QUEUED")) == 0) {
            ret = SUCCESS;
        }
        efree(resp);
    }
    return ret;
}

PHP_REDIS_API int
redis_sock_read_multibulk_multi_reply_loop(INTERNAL_FUNCTION_PARAMETERS,
                                           RedisSock *redis_sock, zval *z_tab)
{
    fold_item *fi;
    uint8_t flags;
    size_t i;

    flags = redis_sock->flags;
    for (i = 0; i < redis_sock->reply_callback_count; i++) {
        fi = &redis_sock->reply_callback[i];
        if (fi->fun) {
            redis_sock->flags = fi->flags;
            fi->fun(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock, z_tab, fi->ctx);
            redis_sock->flags = flags;
            continue;
        }
        size_t len;
        char inbuf[255];

        if (redis_sock_gets(redis_sock, inbuf, sizeof(inbuf) - 1, &len) < 0 ||
            redis_strncmp(inbuf, ZEND_STRL("+OK")) != 0)
        {
            return FAILURE;
        }

        while (redis_sock->reply_callback[++i].fun) {
            if (redis_response_enqueued(redis_sock) != SUCCESS) {
                return FAILURE;
            }
        }

        if (redis_sock_gets(redis_sock, inbuf, sizeof(inbuf) - 1, &len) < 0) {
            return FAILURE;
        }

        zval z_ret;
        array_init(&z_ret);
        add_next_index_zval(z_tab, &z_ret);

        int num = atol(inbuf + 1);

        if (num > 0 && redis_read_multibulk_recursive(redis_sock, num, 0, &z_ret) != SUCCESS) {
            return FAILURE;
        }
    }
    return SUCCESS;
}

PHP_METHOD(Redis, pipeline)
{
    RedisSock *redis_sock;

    if ((redis_sock = redis_sock_get(getThis(), 0)) == NULL) {
        RETURN_FALSE;
    }

    /* User cannot enter MULTI mode if already in a pipeline */
    if (redis_sock_is_multi(redis_sock)) {
        php_error_docref(NULL, E_ERROR, "Can't activate pipeline in multi mode!");
        RETURN_FALSE;
    }

    /* Enable pipeline mode unless we're already in that mode in which case this
     * is just a NO OP */
    if (redis_sock_is_atomic(redis_sock)) {
        redis_sock->mode |= PIPELINE;
    }

    RETURN_ZVAL(getThis(), 1, 0);
}

PHP_METHOD(Redis, getOption)
{
    RedisSock *redis_sock;

    if ((redis_sock = redis_sock_get_instance(getThis(), 0)) == NULL) {
        RETURN_FALSE;
    }

    redis_getoption_handler(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock, NULL);
}

PHP_METHOD(Redis, setOption)
{
    RedisSock *redis_sock;

    if ((redis_sock = redis_sock_get_instance(getThis(), 0)) == NULL) {
        RETURN_FALSE;
    }

    redis_setoption_handler(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock, NULL);
}

PHP_METHOD(Redis, _prefix) {
    RedisSock *redis_sock;

    if ((redis_sock = redis_sock_get_instance(getThis(), 0)) == NULL) {
        RETURN_FALSE;
    }

    redis_prefix_handler(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock);
}

PHP_METHOD(Redis, _serialize) {
    RedisSock *redis_sock;

    // Grab socket
    if ((redis_sock = redis_sock_get_instance(getThis(), 0)) == NULL) {
        RETURN_FALSE;
    }

    redis_serialize_handler(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock);
}

PHP_METHOD(Redis, _unserialize) {
    RedisSock *redis_sock;

    // Grab socket
    if ((redis_sock = redis_sock_get_instance(getThis(), 0)) == NULL) {
        RETURN_FALSE;
    }

    redis_unserialize_handler(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock,
        redis_exception_ce);
}

PHP_METHOD(Redis, _compress) {
    RedisSock *redis_sock;

    // Grab socket
    if ((redis_sock = redis_sock_get_instance(getThis(), 0)) == NULL) {
        RETURN_FALSE;
    }

    redis_compress_handler(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock);
}

PHP_METHOD(Redis, _uncompress) {
    RedisSock *redis_sock;

    // Grab socket
    if ((redis_sock = redis_sock_get_instance(getThis(), 0)) == NULL) {
        RETURN_FALSE;
    }

    redis_uncompress_handler(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock,
        redis_exception_ce);
}

PHP_METHOD(Redis, _pack) {
    RedisSock *redis_sock;

    // Grab socket
    if ((redis_sock = redis_sock_get_instance(getThis(), 0)) == NULL) {
        RETURN_FALSE;
    }

    redis_pack_handler(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock);
}

PHP_METHOD(Redis, _digest) {
    RedisSock *redis_sock;

    redis_sock = redis_sock_get_instance(getThis(), 0);
    if (redis_sock == NULL) {
        RETURN_FALSE;
    }

    redis_digest_handler(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock,
                         redis_exception_ce);
}

PHP_METHOD(Redis, _unpack) {
    RedisSock *redis_sock;

    // Grab socket
    if ((redis_sock = redis_sock_get_instance(getThis(), 0)) == NULL) {
        RETURN_FALSE;
    }

    redis_unpack_handler(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock);
}

PHP_METHOD(Redis, getLastError) {
    RedisSock *redis_sock;

    // Grab socket
    if ((redis_sock = redis_sock_get_instance(getThis(), 0)) == NULL) {
        RETURN_FALSE;
    }

    /* Return our last error or NULL if we don't have one */
    if (redis_sock->err) {
        RETURN_STRINGL(ZSTR_VAL(redis_sock->err), ZSTR_LEN(redis_sock->err));
    }
    RETURN_NULL();
}

PHP_METHOD(Redis, clearLastError) {
    RedisSock *redis_sock;

    // Grab socket
    if ((redis_sock = redis_sock_get_instance(getThis(), 0)) == NULL) {
        RETURN_FALSE;
    }

    redis_sock_clear_err(redis_sock);

    RETURN_TRUE;
}

PHP_METHOD(Redis, getMode) {
    RedisSock *redis_sock;

    /* Grab socket */
    if ((redis_sock = redis_sock_get_instance(getThis(), 0)) == NULL) {
        RETURN_FALSE;
    }

    if (redis_sock_is_pipeline(redis_sock)) {
        RETVAL_LONG(PIPELINE);
    } else if (redis_sock_is_multi(redis_sock)) {
        RETVAL_LONG(MULTI);
    } else {
        RETVAL_LONG(ATOMIC);
    }
}

/*
 * Introspection stuff
 */

PHP_METHOD(Redis, isConnected) {
    RedisSock *redis_sock;

    if (zend_parse_parameters_none() == FAILURE) {
        RETURN_FALSE;
    }

    /* Grab socket */
    if ((redis_sock = redis_sock_get_instance(getThis(), 1)) == NULL) {
        RETURN_FALSE;
    }

    RETURN_BOOL(redis_sock->status >= REDIS_SOCK_STATUS_CONNECTED);
}

PHP_METHOD(Redis, getHost) {
    RedisSock *redis_sock;

    if((redis_sock = redis_sock_get_connected(INTERNAL_FUNCTION_PARAM_PASSTHRU))) {
        RETURN_STRINGL(ZSTR_VAL(redis_sock->host), ZSTR_LEN(redis_sock->host));
    } else {
        RETURN_FALSE;
    }
}

PHP_METHOD(Redis, getPort) {
    RedisSock *redis_sock;

    if((redis_sock = redis_sock_get_connected(INTERNAL_FUNCTION_PARAM_PASSTHRU))) {
        /* Return our port */
        RETURN_LONG(redis_sock->port);
    } else {
        RETURN_FALSE;
    }
}

PHP_METHOD(Redis, serverName) {
    RedisSock *rs;

    if ((rs = redis_sock_get_instance(getThis(), 1)) == NULL) {
        RETURN_FALSE;
    } else if (!redis_sock_is_atomic(rs)) {
        php_error_docref(NULL, E_ERROR,
            "Can't call serverName in multi or pipeline mode!");
        RETURN_FALSE;
    } else if (rs->hello.server != NULL) {
        RETURN_STR_COPY(rs->hello.server);
    }

    REDIS_PROCESS_KW_CMD("HELLO", redis_empty_cmd, redis_hello_server_response);
}

PHP_METHOD(Redis, serverVersion) {
    RedisSock *rs;

    if ((rs = redis_sock_get_instance(getThis(), 1)) == NULL) {
        RETURN_FALSE;
    } else if (!redis_sock_is_atomic(rs)) {
        php_error_docref(NULL, E_ERROR,
            "Can't call serverVersion in multi or pipeline mode!");
        RETURN_FALSE;
    } else if (rs->hello.version != NULL) {
        RETURN_STR_COPY(rs->hello.version);
    }

    REDIS_PROCESS_KW_CMD("HELLO", redis_empty_cmd, redis_hello_version_response);
}

PHP_METHOD(Redis, getDBNum) {
    RedisSock *redis_sock;

    if((redis_sock = redis_sock_get_connected(INTERNAL_FUNCTION_PARAM_PASSTHRU))) {
        /* Return our db number */
        RETURN_LONG(redis_sock->dbNumber);
    } else {
        RETURN_FALSE;
    }
}

PHP_METHOD(Redis, getTransferredBytes) {
    RedisSock *redis_sock;

    if ((redis_sock = redis_sock_get_instance(getThis(), 0)) == NULL) {
        RETURN_THROWS();
    }

    array_init_size(return_value, 2);
    add_next_index_long(return_value, redis_sock->txBytes);
    add_next_index_long(return_value, redis_sock->rxBytes);
}

PHP_METHOD(Redis, clearTransferredBytes) {
    RedisSock *redis_sock;

    if ((redis_sock = redis_sock_get_instance(getThis(), 0)) == NULL) {
        RETURN_THROWS();
    }

    redis_sock->txBytes = 0;
    redis_sock->rxBytes = 0;
}

PHP_METHOD(Redis, getTimeout) {
    RedisSock *redis_sock;

    if((redis_sock = redis_sock_get_connected(INTERNAL_FUNCTION_PARAM_PASSTHRU))) {
        RETURN_DOUBLE(redis_sock->timeout);
    } else {
        RETURN_FALSE;
    }
}

PHP_METHOD(Redis, getReadTimeout) {
    RedisSock *redis_sock;

    if((redis_sock = redis_sock_get_connected(INTERNAL_FUNCTION_PARAM_PASSTHRU))) {
        RETURN_DOUBLE(redis_sock->read_timeout);
    } else {
        RETURN_FALSE;
    }
}

PHP_METHOD(Redis, getPersistentID) {
    RedisSock *redis_sock;

    if ((redis_sock = redis_sock_get_connected(INTERNAL_FUNCTION_PARAM_PASSTHRU)) == NULL) {
        RETURN_FALSE;
    } else if (redis_sock->persistent_id == NULL) {
        RETURN_NULL();
    }
    RETURN_STRINGL(ZSTR_VAL(redis_sock->persistent_id), ZSTR_LEN(redis_sock->persistent_id));
}

PHP_METHOD(Redis, getAuth) {
    RedisSock *redis_sock;
    zval zret;

    if (zend_parse_parameters_none() == FAILURE) {
        RETURN_FALSE;
    }

    redis_sock = redis_sock_get_connected(INTERNAL_FUNCTION_PARAM_PASSTHRU);
    if (redis_sock == NULL)
        RETURN_FALSE;

    if (redis_sock->user && redis_sock->pass) {
        array_init(&zret);
        add_next_index_str(&zret, zend_string_copy(redis_sock->user));
        add_next_index_str(&zret, zend_string_copy(redis_sock->pass));
        RETURN_ZVAL(&zret, 0, 0);
    } else if (redis_sock->pass) {
        RETURN_STR_COPY(redis_sock->pass);
    } else {
        RETURN_NULL();
    }
}

static RedisCmd *
redis_rawcommand_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock) {
    zval *argv;
    int argc;

    ZEND_PARSE_PARAMETERS_START(1, -1)
        Z_PARAM_VARIADIC('+', argv, argc)
    ZEND_PARSE_PARAMETERS_END_EX(return NULL);

    return redis_build_raw_cmd(argv, argc);
}

/* Helper to format any combination of SCAN arguments */
static RedisCmd *
redis_build_scan_cmd(REDIS_SCAN_TYPE type, zend_string *key, uint64_t cursor,
                     zend_string *pattern, int count, zend_string *match_type)
{
    RedisCmd *cmd;
    char *keyword;

    /* Turn our type into a keyword */
    switch(type) {
        case TYPE_SCAN:
            keyword = "SCAN";
            break;
        case TYPE_SSCAN:
            keyword = "SSCAN";
            break;
        case TYPE_HSCAN:
            keyword = "HSCAN";
            break;
        case TYPE_ZSCAN:
        default:
            keyword = "ZSCAN";
            break;
    }


    /* Start the command */
    cmd = redis_cmd_create(NULL, keyword, strlen(keyword));
    if (key && ZSTR_LEN(key) > 0) redis_cmd_cat_zstr(cmd, key);
    redis_cmd_cat_u64(cmd, cursor);

    /* Append COUNT if we've got it */
    if(count) {
        redis_cmd_cat_literal(cmd, "COUNT");
        redis_cmd_cat_long(cmd, count);
    }

    /* Append MATCH if we've got it */
    if(pattern && ZSTR_LEN(pattern) > 0) {
        redis_cmd_cat_literal(cmd, "MATCH");
        redis_cmd_cat_zstr(cmd, pattern);
    }

    if (match_type) {
        redis_cmd_cat_literal(cmd, "TYPE");
        redis_cmd_cat_zstr(cmd, match_type);
    }

    return cmd;
}

PHP_REDIS_API void
generic_scan_cmd(INTERNAL_FUNCTION_PARAMETERS, REDIS_SCAN_TYPE type) {
    zend_string *key = NULL, *pattern = NULL;
    zend_string *match_type = NULL;
    zval *z_cursor;
    RedisSock *redis_sock;
    zend_bool pattern_free = 0;
    zend_long count = 0;
    zend_bool completed;
    HashTable *hash;
    int num_elements;
    uint64_t cursor;
    RedisCmd *cmd;

    ZEND_PARSE_PARAMETERS_START(1 + (type != TYPE_SCAN), 4)
        if (type != TYPE_SCAN) {
            Z_PARAM_STR_OR_NULL(key)
        }
        Z_PARAM_ZVAL_EX(z_cursor, 0, 1)
        Z_PARAM_OPTIONAL
        Z_PARAM_STR_OR_NULL(pattern)
        Z_PARAM_LONG(count)
        if (type == TYPE_SCAN) {
            Z_PARAM_STR_OR_NULL(match_type)
        }
    ZEND_PARSE_PARAMETERS_END_EX(RETURN_FALSE);

    /* Grab our socket */
    if ((redis_sock = redis_sock_get(getThis(), 0)) == NULL) {
        RETURN_FALSE;
    }

    /* Calling this in a pipeline makes no sense */
    if (!redis_sock_is_atomic(redis_sock)) {
        php_error_docref(NULL, E_ERROR,
            "Can't call SCAN commands in multi or pipeline mode!");
        RETURN_FALSE;
    }

    /* Get our SCAN cursor short circuiting if we're done */
    cursor = redisGetScanCursor(z_cursor, &completed);
    if (completed)
        RETURN_FALSE;

    if(key)
        key = redis_key_prefix_zstr(redis_sock, key);

    if (pattern && redis_sock->scan & REDIS_SCAN_PREFIX) {
        pattern = redis_key_prefix_zstr(redis_sock, pattern);
        pattern_free = 1;
    }

    /**
     * Redis can return to us empty keys, especially in the case where there
     * are a large number of keys to scan, and we're matching against a
     * pattern.  phpredis can be set up to abstract this from the user, by
     * setting OPT_SCAN to REDIS_SCAN_RETRY.  Otherwise we will return empty
     * keys and the user will need to make subsequent calls with an updated
     * cursor.
     */
    do {
        /* Free our previous reply if we're back in the loop.  We know we are
         * if our return_value is an array */
        if (Z_TYPE_P(return_value) == IS_ARRAY) {
            zval_ptr_dtor_nogc(return_value);
            ZVAL_NULL(return_value);
        }

        // Format our SCAN command
        cmd = redis_build_scan_cmd(type, key, cursor, pattern, count, match_type);

        if (redis_process_request(redis_sock, cmd) != SUCCESS ||
            redis_sock_read_scan_reply(INTERNAL_FUNCTION_PARAM_PASSTHRU,
                                      redis_sock, type, &cursor) < 0)
        {
            if (key)
                zend_string_release(key);
            if (pattern_free)
                zend_string_release(pattern);
            redis_cmd_free(cmd);
            RETURN_FALSE;
        }

        /* Get the number of elements */
        hash = Z_ARRVAL_P(return_value);
        num_elements = zend_hash_num_elements(hash);

        redis_cmd_free(cmd);
    } while (redis_sock->scan & REDIS_SCAN_RETRY && cursor != 0 &&
             num_elements == 0);

    if (pattern_free)
        zend_string_release(pattern);
    if(key)
        zend_string_release(key);

    /* Update our cursor reference */
    redisSetScanCursor(z_cursor, cursor);
}

PHP_METHOD(Redis, scan) {
    generic_scan_cmd(INTERNAL_FUNCTION_PARAM_PASSTHRU, TYPE_SCAN);
}
PHP_METHOD(Redis, hscan) {
    generic_scan_cmd(INTERNAL_FUNCTION_PARAM_PASSTHRU, TYPE_HSCAN);
}
PHP_METHOD(Redis, sscan) {
    generic_scan_cmd(INTERNAL_FUNCTION_PARAM_PASSTHRU, TYPE_SSCAN);
}
PHP_METHOD(Redis, zscan) {
    generic_scan_cmd(INTERNAL_FUNCTION_PARAM_PASSTHRU, TYPE_ZSCAN);
}

#define REDIS_SORT_CMD_FUNC(name, desc, alpha) \
    RedisCmd * \
    redis_##name##_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock) { \
        return generic_sort_cmd(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock, \
                                desc, alpha); \
    }

REDIS_SORT_CMD_FUNC(sort_asc_numeric, 0, 0)
REDIS_SORT_CMD_FUNC(sort_asc_alpha, 0, 1)
REDIS_SORT_CMD_FUNC(sort_desc_numeric, 1, 0)
REDIS_SORT_CMD_FUNC(sort_desc_alpha, 1, 1)

/* REDIS_METHOD wrappers */

REDIS_METHOD(acl, redis_acl_cmd, redis_acl_response);
REDIS_METHOD(auth, redis_auth_cmd, redis_boolean_response);
REDIS_METHOD(bitcount, redis_bitcount_cmd, redis_long_response);
REDIS_METHOD(bitop, redis_bitop_cmd, redis_long_response);
REDIS_METHOD(bitpos, redis_bitpos_cmd, redis_long_response);
REDIS_METHOD(brpoplpush, redis_brpoplpush_cmd, redis_string_response);
REDIS_METHOD(client, redis_client_cmd, redis_client_response);
REDIS_METHOD(command, redis_command_cmd, redis_command_response);
REDIS_METHOD(config, redis_config_cmd, redis_config_response);
REDIS_METHOD(copy, redis_copy_cmd, redis_1_response);
REDIS_METHOD(decr, redis_decr_cmd, redis_long_response);
REDIS_METHOD(delex, redis_delex_cmd, redis_long_response);
REDIS_METHOD(expiremember, redis_expiremember_cmd, redis_long_response);
REDIS_METHOD(expirememberat, redis_expirememberat_cmd, redis_long_response);
REDIS_METHOD(failover, redis_failover_cmd, redis_boolean_response);
REDIS_METHOD(function, redis_function_cmd, redis_function_response);
REDIS_METHOD(gcra, redis_gcra_cmd, redis_read_variant_reply);
REDIS_METHOD(geoadd, redis_geoadd_cmd, redis_long_response);
REDIS_METHOD(geodist, redis_geodist_cmd, redis_bulk_double_response);
REDIS_METHOD(geosearch, redis_geosearch_cmd, redis_geosearch_response);
REDIS_METHOD(geosearchstore, redis_geosearchstore_cmd, redis_long_response);
REDIS_METHOD(getEx, redis_getex_cmd, redis_string_response);
REDIS_METHOD(hDel, redis_hdel_cmd, redis_long_response);
REDIS_METHOD(hIncrBy, redis_hincrby_cmd, redis_long_response);
REDIS_METHOD(hIncrByFloat, redis_hincrbyfloat_cmd, redis_bulk_double_response);
REDIS_METHOD(hMget, redis_hmget_cmd, redis_mbulk_reply_assoc);
REDIS_METHOD(hMset, redis_hmset_cmd, redis_boolean_response);
REDIS_METHOD(hRandField, redis_hrandfield_cmd, redis_hrandfield_response);
REDIS_METHOD(hSet, redis_hset_cmd, redis_long_response);
REDIS_METHOD(hSetNx, redis_hsetnx_cmd, redis_1_response);
REDIS_METHOD(hStrLen, redis_hstrlen_cmd, redis_long_response);
REDIS_METHOD(hgetdel, redis_hgetdel_cmd, redis_mbulk_reply_assoc);
REDIS_METHOD(hgetex, redis_hgetex_cmd, redis_mbulk_reply_assoc);
REDIS_METHOD(hsetex, redis_hsetex_cmd, redis_long_response);
REDIS_METHOD(incr, redis_incr_cmd, redis_long_response);
REDIS_METHOD(info, redis_info_cmd, redis_info_response);
REDIS_METHOD(lInsert, redis_linsert_cmd, redis_long_response);
REDIS_METHOD(lPos, redis_lpos_cmd, redis_lpos_response);
REDIS_METHOD(lcs, redis_lcs_cmd, redis_read_variant_reply);
REDIS_METHOD(lrem, redis_lrem_cmd, redis_long_response);
REDIS_METHOD(mget, redis_mget_cmd, redis_sock_read_multibulk_reply);
REDIS_METHOD(migrate, redis_migrate_cmd, redis_boolean_response);
REDIS_METHOD(msetex, redis_msetex_cmd, redis_long_response);
REDIS_METHOD(object, redis_object_cmd, redis_object_response);
REDIS_METHOD(pfadd, redis_pfadd_cmd, redis_long_response);
REDIS_METHOD(pfcount, redis_pfcount_cmd, redis_long_response);
REDIS_METHOD(pfmerge, redis_pfmerge_cmd, redis_boolean_response);
REDIS_METHOD(pubsub, redis_pubsub_cmd, redis_pubsub_response);
REDIS_METHOD(rawcommand, redis_rawcommand_cmd, redis_read_raw_variant_reply);
REDIS_METHOD(restore, redis_restore_cmd, redis_boolean_response);
REDIS_METHOD(sMove, redis_smove_cmd, redis_1_response);
REDIS_METHOD(script, redis_script_cmd, redis_read_variant_reply);
REDIS_METHOD(select, redis_select_cmd, redis_select_response);
REDIS_METHOD(set, redis_set_cmd, redis_set_response);
REDIS_METHOD(setBit, redis_setbit_cmd, redis_long_response);
REDIS_METHOD(slowlog, redis_slowlog_cmd, redis_read_variant_reply);
REDIS_METHOD(sortAsc, redis_sort_asc_numeric_cmd, redis_read_variant_reply);
REDIS_METHOD(sortAscAlpha, redis_sort_asc_alpha_cmd, redis_read_variant_reply);
REDIS_METHOD(sortDesc, redis_sort_desc_numeric_cmd, redis_read_variant_reply);
REDIS_METHOD(sortDescAlpha, redis_sort_desc_alpha_cmd, redis_read_variant_reply);
REDIS_METHOD(vadd, redis_vadd_cmd, redis_long_response);
REDIS_METHOD(vemb, redis_vemb_cmd, redis_vemb_reply);
REDIS_METHOD(vgetattr, redis_vgetattr_cmd, redis_vgetattr_reply);
REDIS_METHOD(vlinks, redis_vlinks_cmd, redis_vlinks_reply);
REDIS_METHOD(vsetattr, redis_vsetattr_cmd, redis_long_response);
REDIS_METHOD(vsim, redis_vsim_cmd, redis_zrange_response);
REDIS_METHOD(waitaof, redis_waitaof_cmd, redis_read_variant_reply);
REDIS_METHOD(xack, redis_xack_cmd, redis_long_response);
REDIS_METHOD(xadd, redis_xadd_cmd, redis_read_variant_reply);
REDIS_METHOD(xautoclaim, redis_xautoclaim_cmd, redis_xclaim_reply);
REDIS_METHOD(xclaim, redis_xclaim_cmd, redis_xclaim_reply);
REDIS_METHOD(xdelex, redis_xdelex_cmd, redis_read_variant_reply);
REDIS_METHOD(xgroup, redis_xgroup_cmd, redis_read_variant_reply);
REDIS_METHOD(xinfo, redis_xinfo_cmd, redis_xinfo_reply);
REDIS_METHOD(xpending, redis_xpending_cmd, redis_read_variant_reply_strings);
REDIS_METHOD(xread, redis_xread_cmd, redis_xread_reply);
REDIS_METHOD(xreadgroup, redis_xreadgroup_cmd, redis_xread_reply);
REDIS_METHOD(xtrim, redis_xtrim_cmd, redis_long_response);
REDIS_METHOD(zAdd, redis_zadd_cmd, redis_zadd_response);
REDIS_METHOD(zIncrBy, redis_zincrby_cmd, redis_bulk_double_response);
REDIS_METHOD(zRandMember, redis_zrandmember_cmd, redis_zrandmember_response);
REDIS_METHOD(zdiff, redis_zdiff_cmd, redis_zdiff_response);
REDIS_METHOD(zdiffstore, redis_zdiffstore_cmd, redis_long_response);

/* REDIS_KW_METHOD wrappers */

REDIS_KW_METHOD(append, "APPEND", redis_kv_cmd, redis_long_response);
REDIS_KW_METHOD(bgSave, "BGSAVE", redis_empty_cmd, redis_boolean_response);
REDIS_KW_METHOD(bgrewriteaof, "BGREWRITEAOF", redis_empty_cmd, redis_boolean_response);
REDIS_KW_METHOD(blPop, "BLPOP", redis_blocking_pop_cmd, redis_sock_read_multibulk_reply);
REDIS_KW_METHOD(blmove, "BLMOVE", redis_lmove_cmd, redis_string_response);
REDIS_KW_METHOD(blmpop, "BLMPOP", redis_mpop_cmd, redis_mpop_response);
REDIS_KW_METHOD(brPop, "BRPOP", redis_blocking_pop_cmd, redis_sock_read_multibulk_reply);
REDIS_KW_METHOD(bzPopMax, "BZPOPMAX", redis_blocking_pop_cmd, redis_sock_read_multibulk_reply);
REDIS_KW_METHOD(bzPopMin, "BZPOPMIN", redis_blocking_pop_cmd, redis_sock_read_multibulk_reply);
REDIS_KW_METHOD(bzmpop, "BZMPOP", redis_mpop_cmd, redis_mpop_response);
REDIS_KW_METHOD(dbSize, "DBSIZE", redis_empty_cmd, redis_long_response);
REDIS_KW_METHOD(debug, "DEBUG", redis_key_cmd, redis_string_response);
REDIS_KW_METHOD(decrBy, "DECRBY", redis_key_long_cmd, redis_long_response);
REDIS_KW_METHOD(del, "DEL", redis_varkey_cmd, redis_long_response);
REDIS_KW_METHOD(delifeq, "DELIFEQ", redis_kv_cmd, redis_long_response);
REDIS_KW_METHOD(digest, "DIGEST", redis_key_cmd, redis_ping_response);
REDIS_KW_METHOD(dump, "DUMP", redis_key_cmd, redis_string_response);
REDIS_KW_METHOD(echo, "ECHO", redis_str_cmd, redis_string_response);
REDIS_KW_METHOD(eval, "EVAL", redis_eval_cmd, redis_read_raw_variant_reply);
REDIS_KW_METHOD(eval_ro, "EVAL_RO", redis_eval_cmd, redis_read_raw_variant_reply);
REDIS_KW_METHOD(evalsha, "EVALSHA", redis_eval_cmd, redis_read_raw_variant_reply);
REDIS_KW_METHOD(evalsha_ro, "EVALSHA_RO", redis_eval_cmd, redis_read_raw_variant_reply);
REDIS_KW_METHOD(exists, "EXISTS", redis_varkey_cmd, redis_long_response);
REDIS_KW_METHOD(expire, "EXPIRE", redis_expire_cmd, redis_1_response);
REDIS_KW_METHOD(expireAt, "EXPIREAT", redis_expire_cmd, redis_1_response);
REDIS_KW_METHOD(expiretime, "EXPIRETIME", redis_key_cmd, redis_long_response);
REDIS_KW_METHOD(fcall, "FCALL", redis_fcall_cmd, redis_read_raw_variant_reply);
REDIS_KW_METHOD(fcall_ro, "FCALL_RO", redis_fcall_cmd, redis_read_raw_variant_reply);
REDIS_KW_METHOD(flushAll, "FLUSHALL", redis_flush_cmd, redis_boolean_response);
REDIS_KW_METHOD(flushDB, "FLUSHDB", redis_flush_cmd, redis_boolean_response);
REDIS_KW_METHOD(geohash, "GEOHASH", redis_key_varval_cmd, redis_mbulk_reply_raw);
REDIS_KW_METHOD(geopos, "GEOPOS", redis_key_varval_cmd, redis_read_variant_reply);
REDIS_KW_METHOD(georadius, "GEORADIUS", redis_georadius_cmd, redis_read_variant_reply);
REDIS_KW_METHOD(georadius_ro, "GEORADIUS_RO", redis_georadius_cmd, redis_read_variant_reply);
REDIS_KW_METHOD(georadiusbymember, "GEORADIUSBYMEMBER", redis_georadiusbymember_cmd, redis_read_variant_reply);
REDIS_KW_METHOD(georadiusbymember_ro, "GEORADIUSBYMEMBER_RO", redis_georadiusbymember_cmd, redis_read_variant_reply);
REDIS_KW_METHOD(get, "GET", redis_key_cmd, redis_string_response);
REDIS_KW_METHOD(getBit, "GETBIT", redis_key_long_cmd, redis_long_response);
REDIS_KW_METHOD(getDel, "GETDEL", redis_key_cmd, redis_string_response);
REDIS_KW_METHOD(getRange, "GETRANGE", redis_key_long_long_cmd, redis_string_response);
REDIS_KW_METHOD(getWithMeta, "GET", redis_key_cmd, redis_bulk_withmeta_response);
REDIS_KW_METHOD(getset, "GETSET", redis_kv_cmd, redis_string_response);
REDIS_KW_METHOD(hExists, "HEXISTS", redis_key_str_cmd, redis_1_response);
REDIS_KW_METHOD(hGet, "HGET", redis_key_str_cmd, redis_string_response);
REDIS_KW_METHOD(hGetAll, "HGETALL", redis_key_cmd, redis_mbulk_reply_zipped_vals);
REDIS_KW_METHOD(hGetWithMeta, "HGET", redis_key_str_cmd, redis_bulk_withmeta_response);
REDIS_KW_METHOD(hKeys, "HKEYS", redis_key_cmd, redis_mbulk_reply_raw);
REDIS_KW_METHOD(hLen, "HLEN", redis_key_cmd, redis_long_response);
REDIS_KW_METHOD(hVals, "HVALS", redis_key_cmd, redis_sock_read_multibulk_reply);
REDIS_KW_METHOD(hexpire, "HEXPIRE", redis_hexpire_cmd, redis_read_variant_reply);
REDIS_KW_METHOD(hexpireat, "HEXPIREAT", redis_hexpire_cmd, redis_read_variant_reply);
REDIS_KW_METHOD(hexpiretime, "HEXPIRETIME", redis_httl_cmd, redis_read_variant_reply);
REDIS_KW_METHOD(hpersist, "HPERSIST", redis_httl_cmd, redis_read_variant_reply);
REDIS_KW_METHOD(hpexpire, "HPEXPIRE", redis_hexpire_cmd, redis_read_variant_reply);
REDIS_KW_METHOD(hpexpireat, "HPEXPIREAT", redis_hexpire_cmd, redis_read_variant_reply);
REDIS_KW_METHOD(hpexpiretime, "HPEXPIRETIME", redis_httl_cmd, redis_read_variant_reply);
REDIS_KW_METHOD(hpttl, "HPTTL", redis_httl_cmd, redis_read_variant_reply);
REDIS_KW_METHOD(httl, "HTTL", redis_httl_cmd, redis_read_variant_reply);
REDIS_KW_METHOD(incrBy, "INCRBY", redis_key_long_cmd, redis_long_response);
REDIS_KW_METHOD(incrByFloat, "INCRBYFLOAT", redis_key_dbl_cmd, redis_bulk_double_response);
REDIS_KW_METHOD(keys, "KEYS", redis_key_cmd, redis_mbulk_reply_raw);
REDIS_KW_METHOD(lLen, "LLEN", redis_key_cmd, redis_long_response);
REDIS_KW_METHOD(lMove, "LMOVE", redis_lmove_cmd, redis_string_response);
REDIS_KW_METHOD(lPop, "LPOP", redis_pop_cmd, redis_pop_response);
REDIS_KW_METHOD(lPush, "LPUSH", redis_key_varval_cmd, redis_long_response);
REDIS_KW_METHOD(lPushx, "LPUSHX", redis_kv_cmd, redis_long_response);
REDIS_KW_METHOD(lSet, "LSET", redis_key_long_val_cmd, redis_boolean_response);
REDIS_KW_METHOD(lastSave, "LASTSAVE", redis_empty_cmd, redis_long_response);
REDIS_KW_METHOD(lindex, "LINDEX", redis_key_long_cmd, redis_string_response);
REDIS_KW_METHOD(lmpop, "LMPOP", redis_mpop_cmd, redis_mpop_response);
REDIS_KW_METHOD(lrange, "LRANGE", redis_key_long_long_cmd, redis_sock_read_multibulk_reply);
REDIS_KW_METHOD(ltrim, "LTRIM", redis_key_long_long_cmd, redis_boolean_response);
REDIS_KW_METHOD(move, "MOVE", redis_key_long_cmd, redis_1_response);
REDIS_KW_METHOD(mset, "MSET", redis_mset_cmd, redis_boolean_response);
REDIS_KW_METHOD(msetnx, "MSETNX", redis_mset_cmd, redis_1_response);
REDIS_KW_METHOD(persist, "PERSIST", redis_key_cmd, redis_1_response);
REDIS_KW_METHOD(pexpire, "PEXPIRE", redis_expire_cmd, redis_1_response);
REDIS_KW_METHOD(pexpireAt, "PEXPIREAT", redis_expire_cmd, redis_1_response);
REDIS_KW_METHOD(pexpiretime, "PEXPIRETIME", redis_key_cmd, redis_long_response);
REDIS_KW_METHOD(ping, "PING", redis_opt_str_cmd, redis_read_variant_reply);
REDIS_KW_METHOD(psetex, "PSETEX", redis_key_long_val_cmd, redis_boolean_response);
REDIS_KW_METHOD(psubscribe, "PSUBSCRIBE", redis_subscribe_cmd, redis_subscribe_response);
REDIS_KW_METHOD(pttl, "PTTL", redis_key_cmd, redis_long_response);
REDIS_KW_METHOD(publish, "PUBLISH", redis_key_str_cmd, redis_long_response);
REDIS_KW_METHOD(punsubscribe, "PUNSUBSCRIBE", redis_unsubscribe_cmd, redis_unsubscribe_response);
REDIS_KW_METHOD(rPop, "RPOP", redis_pop_cmd, redis_pop_response);
REDIS_KW_METHOD(rPush, "RPUSH", redis_key_varval_cmd, redis_long_response);
REDIS_KW_METHOD(rPushx, "RPUSHX", redis_kv_cmd, redis_long_response);
REDIS_KW_METHOD(randomKey, "RANDOMKEY", redis_empty_cmd, redis_ping_response);
REDIS_KW_METHOD(rename, "RENAME", redis_key_key_cmd, redis_boolean_response);
REDIS_KW_METHOD(renameNx, "RENAMENX", redis_key_key_cmd, redis_1_response);
REDIS_KW_METHOD(replicaof, "REPLICAOF", redis_replicaof_cmd, redis_boolean_response);
REDIS_KW_METHOD(role, "ROLE", redis_empty_cmd, redis_read_variant_reply);
REDIS_KW_METHOD(rpoplpush, "RPOPLPUSH", redis_key_key_cmd, redis_string_response);
REDIS_KW_METHOD(sAdd, "SADD", redis_key_varval_cmd, redis_long_response);
REDIS_KW_METHOD(sAddArray, "SADD", redis_key_val_arr_cmd, redis_long_response);
REDIS_KW_METHOD(sDiff, "SDIFF", redis_varkey_cmd, redis_sock_read_multibulk_reply);
REDIS_KW_METHOD(sDiffStore, "SDIFFSTORE", redis_varkey_cmd, redis_long_response);
REDIS_KW_METHOD(sInter, "SINTER", redis_varkey_cmd, redis_sock_read_multibulk_reply);
REDIS_KW_METHOD(sInterStore, "SINTERSTORE", redis_varkey_cmd, redis_long_response);
REDIS_KW_METHOD(sMembers, "SMEMBERS", redis_key_cmd, redis_sock_read_multibulk_reply);
REDIS_KW_METHOD(sMisMember, "SMISMEMBER", redis_key_varval_cmd, redis_read_variant_reply);
REDIS_KW_METHOD(sRandMember, "SRANDMEMBER", redis_randmember_cmd, redis_randmember_response);
REDIS_KW_METHOD(sUnion, "SUNION", redis_varkey_cmd, redis_sock_read_multibulk_reply);
REDIS_KW_METHOD(sUnionStore, "SUNIONSTORE", redis_varkey_cmd, redis_long_response);
REDIS_KW_METHOD(save, "SAVE", redis_empty_cmd, redis_boolean_response);
REDIS_KW_METHOD(scard, "SCARD", redis_key_cmd, redis_long_response);
REDIS_KW_METHOD(setRange, "SETRANGE", redis_key_long_str_cmd, redis_long_response);
REDIS_KW_METHOD(setex, "SETEX", redis_key_long_val_cmd, redis_boolean_response);
REDIS_KW_METHOD(setnx, "SETNX", redis_kv_cmd, redis_1_response);
REDIS_KW_METHOD(sintercard, "SINTERCARD", redis_intercard_cmd, redis_long_response);
REDIS_KW_METHOD(sismember, "SISMEMBER", redis_kv_cmd, redis_1_response);
REDIS_KW_METHOD(slaveof, "SLAVEOF", redis_replicaof_cmd, redis_boolean_response);
REDIS_KW_METHOD(sort, "SORT", redis_sort_cmd, redis_read_variant_reply);
REDIS_KW_METHOD(sort_ro, "SORT_RO", redis_sort_cmd, redis_read_variant_reply);
REDIS_KW_METHOD(srem, "SREM", redis_key_varval_cmd, redis_long_response);
REDIS_KW_METHOD(ssubscribe, "SSUBSCRIBE", redis_subscribe_cmd, redis_subscribe_response);
REDIS_KW_METHOD(strlen, "STRLEN", redis_key_cmd, redis_long_response);
REDIS_KW_METHOD(subscribe, "SUBSCRIBE", redis_subscribe_cmd, redis_subscribe_response);
REDIS_KW_METHOD(sunsubscribe, "SUNSUBSCRIBE", redis_unsubscribe_cmd, redis_unsubscribe_response);
REDIS_KW_METHOD(swapdb, "SWAPDB", redis_long_long_cmd, redis_boolean_response);
REDIS_KW_METHOD(time, "TIME", redis_empty_cmd, redis_mbulk_reply_raw);
REDIS_KW_METHOD(touch, "TOUCH", redis_varkey_cmd, redis_long_response);
REDIS_KW_METHOD(ttl, "TTL", redis_key_cmd, redis_long_response);
REDIS_KW_METHOD(type, "TYPE", redis_key_cmd, redis_type_response);
REDIS_KW_METHOD(unlink, "UNLINK", redis_varkey_cmd, redis_long_response);
REDIS_KW_METHOD(unsubscribe, "UNSUBSCRIBE", redis_unsubscribe_cmd, redis_unsubscribe_response);
REDIS_KW_METHOD(unwatch, "UNWATCH", redis_empty_cmd, redis_unwatch_response);
REDIS_KW_METHOD(vcard, "VCARD", redis_key_cmd, redis_long_response);
REDIS_KW_METHOD(vdim, "VDIM", redis_key_cmd, redis_long_response);
REDIS_KW_METHOD(vinfo, "VINFO", redis_key_cmd, redis_vinfo_reply);
REDIS_KW_METHOD(vismember, "VISMEMBER", redis_kv_cmd, redis_1_response);
REDIS_KW_METHOD(vrandmember, "VRANDMEMBER", redis_randmember_cmd, redis_randmember_response);
REDIS_KW_METHOD(vrange, "VRANGE", redis_vrange_cmd, redis_sock_read_multibulk_reply);
REDIS_KW_METHOD(vrem, "VREM", redis_kv_cmd, redis_long_response);
REDIS_KW_METHOD(wait, "WAIT", redis_long_long_cmd, redis_long_response);
REDIS_KW_METHOD(watch, "WATCH", redis_varkey_cmd, redis_watch_response);
REDIS_KW_METHOD(xdel, "XDEL", redis_key_str_arr_cmd, redis_long_response);
REDIS_KW_METHOD(xlen, "XLEN", redis_key_cmd, redis_long_response);
REDIS_KW_METHOD(xrange, "XRANGE", redis_xrange_cmd, redis_xrange_reply);
REDIS_KW_METHOD(xrevrange, "XREVRANGE", redis_xrange_cmd, redis_xrange_reply);
REDIS_KW_METHOD(zCard, "ZCARD", redis_key_cmd, redis_long_response);
REDIS_KW_METHOD(zCount, "ZCOUNT", redis_key_str_str_cmd, redis_long_response);
REDIS_KW_METHOD(zLexCount, "ZLEXCOUNT", redis_gen_zlex_cmd, redis_long_response);
REDIS_KW_METHOD(zMscore, "ZMSCORE", redis_key_varval_cmd, redis_mbulk_reply_double);
REDIS_KW_METHOD(zRange, "ZRANGE", redis_zrange_cmd, redis_zrange_response);
REDIS_KW_METHOD(zRangeByLex, "ZRANGEBYLEX", redis_zrangebylex_cmd, redis_sock_read_multibulk_reply);
REDIS_KW_METHOD(zRangeByScore, "ZRANGEBYSCORE", redis_zrange_cmd, redis_zrange_response);
REDIS_KW_METHOD(zRank, "ZRANK", redis_kv_cmd, redis_long_response);
REDIS_KW_METHOD(zRem, "ZREM", redis_key_varval_cmd, redis_long_response);
REDIS_KW_METHOD(zRemRangeByLex, "ZREMRANGEBYLEX", redis_gen_zlex_cmd, redis_long_response);
REDIS_KW_METHOD(zRemRangeByRank, "ZREMRANGEBYRANK", redis_key_long_long_cmd, redis_long_response);
REDIS_KW_METHOD(zRemRangeByScore, "ZREMRANGEBYSCORE", redis_key_str_str_cmd, redis_long_response);
REDIS_KW_METHOD(zRevRange, "ZREVRANGE", redis_zrange_cmd, redis_zrange_response);
REDIS_KW_METHOD(zRevRangeByLex, "ZREVRANGEBYLEX", redis_zrangebylex_cmd, redis_sock_read_multibulk_reply);
REDIS_KW_METHOD(zRevRangeByScore, "ZREVRANGEBYSCORE", redis_zrange_cmd, redis_zrange_response);
REDIS_KW_METHOD(zRevRank, "ZREVRANK", redis_kv_cmd, redis_long_response);
REDIS_KW_METHOD(zScore, "ZSCORE", redis_kv_cmd, redis_bulk_double_response);
REDIS_KW_METHOD(zinter, "ZINTER", redis_zinterunion_cmd, redis_zdiff_response);
REDIS_KW_METHOD(zintercard, "ZINTERCARD", redis_intercard_cmd, redis_long_response);
REDIS_KW_METHOD(zinterstore, "ZINTERSTORE", redis_zinterunionstore_cmd, redis_long_response);
REDIS_KW_METHOD(zmpop, "ZMPOP", redis_mpop_cmd, redis_mpop_response);
REDIS_KW_METHOD(zrangestore, "ZRANGESTORE", redis_zrange_cmd, redis_long_response);
REDIS_KW_METHOD(zunion, "ZUNION", redis_zinterunion_cmd, redis_zdiff_response);
REDIS_KW_METHOD(zunionstore, "ZUNIONSTORE", redis_zinterunionstore_cmd, redis_long_response);

/* vim: set tabstop=4 softtabstop=4 expandtab shiftwidth=4: */
