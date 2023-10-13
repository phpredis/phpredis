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
#include <standard/php_random.h>
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

#if PHP_VERSION_ID < 80000
#include "redis_legacy_arginfo.h"
#else
#include "zend_attributes.h"
#include "redis_arginfo.h"
#endif

extern const zend_function_entry *redis_get_methods(void)
{
    return class_Redis_methods;
}

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
    PHP_INI_ENTRY("redis.session.lock_expire", "0", PHP_INI_ALL, NULL)
    PHP_INI_ENTRY("redis.session.lock_retries", "100", PHP_INI_ALL, NULL)
    PHP_INI_ENTRY("redis.session.lock_wait_time", "20000", PHP_INI_ALL, NULL)
    PHP_INI_ENTRY("redis.session.early_refresh", "0", PHP_INI_ALL, NULL)
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
        result = (resp_len == 3 && strncmp(resp,"+OK", 3) == 0) ? SUCCESS:FAILURE;

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

    memcpy(&redis_object_handlers, zend_get_std_object_handlers(), sizeof(redis_object_handlers));
    redis_object_handlers.offset = XtOffsetOf(redis_object, std);
    redis_object_handlers.free_obj = free_redis_object;
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

    if (redis_sock_server_open(redis_sock) < 0) {
        if (!no_throw) {
            char *errmsg = NULL;
            if (redis_sock->port < 0) {
                spprintf(&errmsg, 0, "Redis server %s went away", ZSTR_VAL(redis_sock->host));
            } else {
                spprintf(&errmsg, 0, "Redis server %s:%d went away", ZSTR_VAL(redis_sock->host), redis_sock->port);
            }
            REDIS_THROW_EXCEPTION(errmsg, 0);
            efree(errmsg);
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
    zval *object;
    RedisSock *redis_sock;

    // If we can't grab our object, or get a socket, or we're not connected,
    // return NULL
    if((zend_parse_method_parameters(ZEND_NUM_ARGS(), getThis(), "O",
       &object, redis_ce) == FAILURE) ||
       (redis_sock = redis_sock_get(object, 1)) == NULL ||
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

/* {{{ proto Redis Redis::__construct(array $options = null)
    Public constructor */
PHP_METHOD(Redis, __construct)
{
    HashTable *opts = NULL;
    redis_object *redis;

    ZEND_PARSE_PARAMETERS_START(0, 1)
        Z_PARAM_OPTIONAL
        Z_PARAM_ARRAY_HT_OR_NULL(opts)
    ZEND_PARSE_PARAMETERS_END_EX(RETURN_THROWS());

    redis = PHPREDIS_ZVAL_GET_OBJECT(redis_object, getThis());
    redis->sock = redis_sock_create(ZEND_STRL("127.0.0.1"), 6379, 0, 0, 0, NULL, 0);
    if (opts != NULL && redis_sock_configure(redis->sock, opts) != SUCCESS) {
        RETURN_THROWS();
    }
}
/* }}} */

/* {{{ proto Redis Redis::__destruct()
    Public Destructor
 */
PHP_METHOD(Redis,__destruct) {
    if (zend_parse_parameters_none() == FAILURE) {
        RETURN_FALSE;
    }

    // Grab our socket
    RedisSock *redis_sock;
    if ((redis_sock = redis_sock_get_instance(getThis(), 1)) == NULL) {
        RETURN_FALSE;
    }

    // If we think we're in MULTI mode, send a discard
    if (IS_MULTI(redis_sock)) {
        if (!IS_PIPELINE(redis_sock) && redis_sock->stream) {
            // Discard any multi commands, and free any callbacks that have been
            // queued
            redis_send_discard(redis_sock);
        }
        free_reply_callbacks(redis_sock);
    }
}

/* {{{ proto boolean Redis::connect(string host, int port [, double timeout [, long retry_interval]])
 */
PHP_METHOD(Redis, connect)
{
    if (redis_connect(INTERNAL_FUNCTION_PARAM_PASSTHRU, 0) == FAILURE) {
        RETURN_FALSE;
    } else {
        RETURN_TRUE;
    }
}
/* }}} */

/* {{{ proto boolean Redis::pconnect(string host, int port [, double timeout])
 */
PHP_METHOD(Redis, pconnect)
{
    if (redis_connect(INTERNAL_FUNCTION_PARAM_PASSTHRU, 1) == FAILURE) {
        RETURN_FALSE;
    } else {
        RETURN_TRUE;
    }
}
/* }}} */

PHP_REDIS_API int
redis_connect(INTERNAL_FUNCTION_PARAMETERS, int persistent)
{
    zval *object, *context = NULL, *ele;
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

    if (zend_parse_method_parameters(ZEND_NUM_ARGS(), getThis(),
                                     "Os|lds!lda", &object, redis_ce, &host,
                                     &host_len, &port, &timeout, &persistent_id,
                                     &persistent_id_len, &retry_interval,
                                     &read_timeout, &context) == FAILURE)
    {
        return FAILURE;
    }

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
              (host_len > 6 && !strncasecmp(host, "unix://", sizeof("unix://") - 1)) ||
              (host_len > 6 && !strncasecmp(host, "file://", sizeof("file://") - 1));

    /* If it's not a unix socket, set to default */
    if (port == -1 && !af_unix) {
        port = 6379;
    }

    redis = PHPREDIS_ZVAL_GET_OBJECT(redis_object, object);

    /* if there is a redis sock already we have to remove it */
    if (redis->sock) {
        redis_sock_disconnect(redis->sock, 0, 1);
        redis_free_socket(redis->sock);
    }

    redis->sock = redis_sock_create(host, host_len, port, timeout, read_timeout, persistent,
        persistent_id, retry_interval);

    if (context) {
        /* Stream context (e.g. TLS) */
        if ((ele = REDIS_HASH_STR_FIND_STATIC(Z_ARRVAL_P(context), "stream"))) {
            redis_sock_set_stream_context(redis->sock, ele);
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

/* {{{ proto long Redis::bitop(string op, string key, ...) */
PHP_METHOD(Redis, bitop) {
    REDIS_PROCESS_CMD(bitop, redis_long_response);
}

/* }}} */

/* {{{ proto long Redis::bitcount(string key, [int start], [int end])
 */
PHP_METHOD(Redis, bitcount)
{
    REDIS_PROCESS_CMD(bitcount, redis_long_response);
}
/* }}} */

/* {{{ proto integer Redis::bitpos(string key, int bit, [int start, int end]) */
PHP_METHOD(Redis, bitpos)
{
    REDIS_PROCESS_CMD(bitpos, redis_long_response);
}
/* }}} */

/* {{{ proto boolean Redis::close()
 */
PHP_METHOD(Redis, close)
{
    RedisSock *redis_sock = redis_sock_get_connected(INTERNAL_FUNCTION_PARAM_PASSTHRU);

    if (redis_sock_disconnect(redis_sock, 1, 1) == SUCCESS) {
        RETURN_TRUE;
    }
    RETURN_FALSE;
}
/* }}} */

/* {{{ proto boolean Redis::set(string key, mixed val, long timeout,
 *                              [array opt) */
PHP_METHOD(Redis, set) {
    REDIS_PROCESS_CMD(set, redis_set_response);
}

/* {{{ proto boolean Redis::setex(string key, long expire, string value)
 */
PHP_METHOD(Redis, setex)
{
    REDIS_PROCESS_KW_CMD("SETEX", redis_key_long_val_cmd, redis_boolean_response);
}

/* {{{ proto boolean Redis::psetex(string key, long expire, string value)
 */
PHP_METHOD(Redis, psetex)
{
    REDIS_PROCESS_KW_CMD("PSETEX", redis_key_long_val_cmd, redis_boolean_response);
}

/* {{{ proto boolean Redis::setnx(string key, string value)
 */
PHP_METHOD(Redis, setnx)
{
    REDIS_PROCESS_KW_CMD("SETNX", redis_kv_cmd, redis_1_response);
}

/* }}} */

/* {{{ proto string Redis::getSet(string key, string value)
 */
PHP_METHOD(Redis, getset)
{
    REDIS_PROCESS_KW_CMD("GETSET", redis_kv_cmd, redis_string_response);
}
/* }}} */

/* {{{ proto string Redis::randomKey()
 */
PHP_METHOD(Redis, randomKey)
{
    REDIS_PROCESS_KW_CMD("RANDOMKEY", redis_empty_cmd, redis_ping_response);
}
/* }}} */

/* {{{ proto string Redis::echo(string msg)
 */
PHP_METHOD(Redis, echo)
{
    REDIS_PROCESS_KW_CMD("ECHO", redis_str_cmd, redis_string_response);
}
/* }}} */

/* {{{ proto string Redis::rename(string key_src, string key_dst)
 */
PHP_METHOD(Redis, rename)
{
    REDIS_PROCESS_KW_CMD("RENAME", redis_key_key_cmd, redis_boolean_response);
}
/* }}} */

/* {{{ proto string Redis::renameNx(string key_src, string key_dst)
 */
PHP_METHOD(Redis, renameNx)
{
    REDIS_PROCESS_KW_CMD("RENAMENX", redis_key_key_cmd, redis_1_response);
}
/* }}} */

/** {{{ proto bool Redis::reset()
 */
PHP_METHOD(Redis, reset)
{
    char *response;
    int response_len;
    RedisSock *redis_sock;
    smart_string cmd = {0};
    zend_bool ret = 0;

    if ((redis_sock = redis_sock_get(getThis(), 0)) == NULL) {
        RETURN_FALSE;
    }

    if (IS_PIPELINE(redis_sock)) {
        php_error_docref(NULL, E_ERROR, "Reset ins't allowed in pipeline mode!");
        RETURN_FALSE;
    }

    redis_cmd_init_sstr(&cmd, 0, "RESET", 5);

    REDIS_PROCESS_REQUEST(redis_sock, cmd.c, cmd.len);

    if ((response = redis_sock_read(redis_sock, &response_len)) != NULL) {
        ret = REDIS_STRCMP_STATIC(response, response_len, "+RESET");
        efree(response);
    }

    if (!ret) {
        if (IS_ATOMIC(redis_sock)) {
            RETURN_FALSE;
        }
        REDIS_THROW_EXCEPTION("Reset failed in multi mode!", 0);
        RETURN_ZVAL(getThis(), 1, 0);
    }

    free_reply_callbacks(redis_sock);
    redis_sock->status = REDIS_SOCK_STATUS_CONNECTED;
    redis_sock->mode = ATOMIC;
    redis_sock->dbNumber = 0;
    redis_sock->watching = 0;

    RETURN_TRUE;
}
/* }}} */

/* {{{ proto string Redis::get(string key)
 */
PHP_METHOD(Redis, get)
{
    REDIS_PROCESS_KW_CMD("GET", redis_key_cmd, redis_string_response);
}
/* }}} */

/* {{{ proto string Redis::getDel(string key)
 */
PHP_METHOD(Redis, getDel)
{
    REDIS_PROCESS_KW_CMD("GETDEL", redis_key_cmd, redis_string_response);
}
/* }}} */

/* {{{ proto string Redis::getEx(string key [, array $options = []])
 */
PHP_METHOD(Redis, getEx)
{
    REDIS_PROCESS_CMD(getex, redis_string_response);
}
/* }}} */

/* {{{ proto string Redis::ping()
 */
PHP_METHOD(Redis, ping)
{
    REDIS_PROCESS_KW_CMD("PING", redis_opt_str_cmd, redis_read_variant_reply);
}
/* }}} */

/* {{{ proto boolean Redis::incr(string key [,int value])
 */
PHP_METHOD(Redis, incr){
    REDIS_PROCESS_CMD(incr, redis_long_response);
}
/* }}} */

/* {{{ proto boolean Redis::incrBy(string key ,int value)
 */
PHP_METHOD(Redis, incrBy){
    REDIS_PROCESS_KW_CMD("INCRBY", redis_key_long_cmd, redis_long_response);
}
/* }}} */

/* {{{ proto float Redis::incrByFloat(string key, float value)
 */
PHP_METHOD(Redis, incrByFloat) {
    REDIS_PROCESS_KW_CMD("INCRBYFLOAT", redis_key_dbl_cmd, redis_bulk_double_response);
}
/* }}} */

/* {{{ proto boolean Redis::decr(string key) */
PHP_METHOD(Redis, decr)
{
    REDIS_PROCESS_CMD(decr, redis_long_response);
}
/* }}} */

/* {{{ proto boolean Redis::decrBy(string key ,int value)
 */
PHP_METHOD(Redis, decrBy){
    REDIS_PROCESS_KW_CMD("DECRBY", redis_key_long_cmd, redis_long_response);
}
/* }}} */

/* {{{ proto array Redis::mget(array keys)
 */
PHP_METHOD(Redis, mget) {
    REDIS_PROCESS_CMD(mget, redis_sock_read_multibulk_reply);
}

/* {{{ proto boolean Redis::exists(string $key, string ...$more_keys)
 */
PHP_METHOD(Redis, exists) {
    REDIS_PROCESS_KW_CMD("EXISTS", redis_varkey_cmd, redis_long_response);
}
/* }}} */

/* {{{ proto boolean Redis::touch(string $key, string ...$more_keys)
 */
PHP_METHOD(Redis, touch) {
    REDIS_PROCESS_KW_CMD("TOUCH", redis_varkey_cmd, redis_long_response);
}

/* }}} */
/* {{{ proto boolean Redis::del(string key)
 */
PHP_METHOD(Redis, del) {
    REDIS_PROCESS_KW_CMD("DEL", redis_varkey_cmd, redis_long_response);
}
/* }}} */

/* {{{ proto long Redis::unlink(string $key1, string $key2 [, string $key3...]) }}}
 * {{{ proto long Redis::unlink(array $keys) */
PHP_METHOD(Redis, unlink)
{
    REDIS_PROCESS_KW_CMD("UNLINK", redis_varkey_cmd, redis_long_response);
}

PHP_REDIS_API void redis_set_watch(RedisSock *redis_sock)
{
    redis_sock->watching = 1;
}

PHP_REDIS_API int redis_watch_response(INTERNAL_FUNCTION_PARAMETERS,
                                 RedisSock *redis_sock, zval *z_tab, void *ctx)
{
    return redis_boolean_response_impl(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock,
        z_tab, ctx, redis_set_watch);
}

/* {{{ proto boolean Redis::watch(string key1, string key2...)
 */
PHP_METHOD(Redis, watch) {
    REDIS_PROCESS_KW_CMD("WATCH", redis_varkey_cmd, redis_watch_response);
}
/* }}} */

PHP_REDIS_API void redis_clear_watch(RedisSock *redis_sock)
{
    redis_sock->watching = 0;
}

PHP_REDIS_API int redis_unwatch_response(INTERNAL_FUNCTION_PARAMETERS,
                                   RedisSock *redis_sock, zval *z_tab,
                                   void *ctx)
{
    return redis_boolean_response_impl(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock,
                                       z_tab, ctx, redis_clear_watch);
}

/* {{{ proto boolean Redis::unwatch()
 */
PHP_METHOD(Redis, unwatch)
{
    REDIS_PROCESS_KW_CMD("UNWATCH", redis_empty_cmd, redis_unwatch_response);
}
/* }}} */

/* {{{ proto array Redis::keys(string pattern)
 */
PHP_METHOD(Redis, keys)
{
    REDIS_PROCESS_KW_CMD("KEYS", redis_key_cmd, redis_mbulk_reply_raw);
}
/* }}} */

/* {{{ proto int Redis::type(string key)
 */
PHP_METHOD(Redis, type)
{
    REDIS_PROCESS_KW_CMD("TYPE", redis_key_cmd, redis_type_response);
}
/* }}} */

/* {{{ proto mixed Redis::acl(string $op, ...) }}} */
PHP_METHOD(Redis, acl) {
    REDIS_PROCESS_CMD(acl, redis_acl_response);
}

/* {{{ proto long Redis::append(string key, string val) */
PHP_METHOD(Redis, append)
{
    REDIS_PROCESS_KW_CMD("APPEND", redis_kv_cmd, redis_long_response);
}
/* }}} */

/* {{{ proto string Redis::GetRange(string key, long start, long end) */
PHP_METHOD(Redis, getRange)
{
    REDIS_PROCESS_KW_CMD("GETRANGE", redis_key_long_long_cmd,
        redis_string_response);
}
/* }}} */

/* {{{ proto mixed Redis::lcs(string $key1, string $key2, ?array $options = NULL); */
PHP_METHOD(Redis, lcs) {
    REDIS_PROCESS_CMD(lcs, redis_read_variant_reply);
}
/* }}} */

/* {{{ proto string Redis::setRange(string key, long start, string value) */
PHP_METHOD(Redis, setRange)
{
    REDIS_PROCESS_KW_CMD("SETRANGE", redis_key_long_str_cmd,
        redis_long_response);
}
/* }}} */

/* {{{ proto long Redis::getbit(string key, long idx) */
PHP_METHOD(Redis, getBit)
{
    REDIS_PROCESS_KW_CMD("GETBIT", redis_key_long_cmd, redis_long_response);
}
/* }}} */

/* {{{ proto long Redis::setbit(string key, long idx, bool|int value) */
PHP_METHOD(Redis, setBit)
{
    REDIS_PROCESS_CMD(setbit, redis_long_response);
}
/* }}} */

/* {{{ proto long Redis::strlen(string key) */
PHP_METHOD(Redis, strlen)
{
    REDIS_PROCESS_KW_CMD("STRLEN", redis_key_cmd, redis_long_response);
}
/* }}} */

/* {{{ proto boolean Redis::lPush(string key , string value)
 */
PHP_METHOD(Redis, lPush)
{
    REDIS_PROCESS_KW_CMD("LPUSH", redis_key_varval_cmd, redis_long_response);
}
/* }}} */

/* {{{ proto boolean Redis::rPush(string key , string value)
 */
PHP_METHOD(Redis, rPush)
{
    REDIS_PROCESS_KW_CMD("RPUSH", redis_key_varval_cmd, redis_long_response);
}
/* }}} */

PHP_METHOD(Redis, lInsert)
{
    REDIS_PROCESS_CMD(linsert, redis_long_response);
}

/* {{{ proto long Redis::lPushx(string key, mixed value) */
PHP_METHOD(Redis, lPushx)
{
    REDIS_PROCESS_KW_CMD("LPUSHX", redis_kv_cmd, redis_long_response);
}
/* }}} */

/* {{{ proto long Redis::rPushx(string key, mixed value) */
PHP_METHOD(Redis, rPushx)
{
    REDIS_PROCESS_KW_CMD("RPUSHX", redis_kv_cmd, redis_long_response);
}
/* }}} */

/* {{{ proto string Redis::lPop(string key, [int count = 0]) */
PHP_METHOD(Redis, lPop)
{
    REDIS_PROCESS_KW_CMD("LPOP", redis_pop_cmd, redis_pop_response);
}
/* }}} */

/* {{{ proto string Redis::lPos(string key, mixed value, [array options = null]) */
PHP_METHOD(Redis, lPos)
{
    REDIS_PROCESS_CMD(lpos, redis_lpos_response);
}
/* }}} */

/* {{{ proto string Redis::rPop(string key, [int count = 0]) */
PHP_METHOD(Redis, rPop)
{
    REDIS_PROCESS_KW_CMD("RPOP", redis_pop_cmd, redis_pop_response);
}
/* }}} */

/* {{{ proto string Redis::blPop(string key1, string key2, ..., int timeout) */
PHP_METHOD(Redis, blPop)
{
    REDIS_PROCESS_KW_CMD("BLPOP", redis_blocking_pop_cmd, redis_sock_read_multibulk_reply);
}
/* }}} */

/* {{{ proto string Redis::brPop(string key1, string key2, ..., int timeout) */
PHP_METHOD(Redis, brPop)
{
    REDIS_PROCESS_KW_CMD("BRPOP", redis_blocking_pop_cmd, redis_sock_read_multibulk_reply);
}
/* }}} */


/* {{{ proto int Redis::lLen(string key) */
PHP_METHOD(Redis, lLen)
{
    REDIS_PROCESS_KW_CMD("LLEN", redis_key_cmd, redis_long_response);
}
/* }}} */

/* {{{ proto string Redis::blMove(string source, string destination, string wherefrom, string whereto, double $timeout) */
PHP_METHOD(Redis, blmove) {
    REDIS_PROCESS_KW_CMD("BLMOVE", redis_lmove_cmd, redis_string_response);
}

/* {{{ proto string Redis::lMove(string source, string destination, string wherefrom, string whereto) */
PHP_METHOD(Redis, lMove) {
    REDIS_PROCESS_KW_CMD("LMOVE", redis_lmove_cmd, redis_string_response);
}

/* {{{ proto boolean Redis::lrem(string list, string value, int count = 0) */
PHP_METHOD(Redis, lrem)
{
    REDIS_PROCESS_CMD(lrem, redis_long_response);
}
/* }}} */

/* {{{ proto boolean Redis::ltrim(string key , int start , int end) */
PHP_METHOD(Redis, ltrim)
{
    REDIS_PROCESS_KW_CMD("LTRIM", redis_key_long_long_cmd,
        redis_boolean_response);
}
/* }}} */

/* {{{ proto string Redis::lindex(string key , int index) */
PHP_METHOD(Redis, lindex)
{
    REDIS_PROCESS_KW_CMD("LINDEX", redis_key_long_cmd, redis_string_response);
}
/* }}} */

/* {{{ proto array Redis::lrange(string key, int start , int end) */
PHP_METHOD(Redis, lrange)
{
    REDIS_PROCESS_KW_CMD("LRANGE", redis_key_long_long_cmd,
        redis_sock_read_multibulk_reply);
}
/* }}} */

/* {{{ proto long Redis::sAdd(string key , mixed value) */
PHP_METHOD(Redis, sAdd)
{
    REDIS_PROCESS_KW_CMD("SADD", redis_key_varval_cmd, redis_long_response);
}
/* }}} */

/* {{{ proto boolean Redis::sAddArray(string key, array $values) */
PHP_METHOD(Redis, sAddArray) {
    REDIS_PROCESS_KW_CMD("SADD", redis_key_val_arr_cmd, redis_long_response);
} /* }}} */

/* {{{ proto int Redis::scard(string key) */
PHP_METHOD(Redis, scard)
{
    REDIS_PROCESS_KW_CMD("SCARD", redis_key_cmd, redis_long_response);
}
/* }}} */

/* {{{ proto boolean Redis::srem(string set, string value) */
PHP_METHOD(Redis, srem)
{
    REDIS_PROCESS_KW_CMD("SREM", redis_key_varval_cmd, redis_long_response);
}
/* }}} */

/* {{{ proto boolean Redis::sMove(string src, string dst, mixed value) */
PHP_METHOD(Redis, sMove)
{
    REDIS_PROCESS_CMD(smove, redis_1_response);
}
/* }}} */

/* {{{ proto string Redis::sPop(string key) */
PHP_METHOD(Redis, sPop)
{
    if (ZEND_NUM_ARGS() == 1) {
        REDIS_PROCESS_KW_CMD("SPOP", redis_key_cmd, redis_string_response);
    } else if (ZEND_NUM_ARGS() == 2) {
        REDIS_PROCESS_KW_CMD("SPOP", redis_key_long_cmd, redis_sock_read_multibulk_reply);
    } else {
        ZEND_WRONG_PARAM_COUNT();
    }

}
/* }}} */

/* {{{ proto string Redis::sRandMember(string key [int count]) */
PHP_METHOD(Redis, sRandMember)
{
    REDIS_PROCESS_CMD(srandmember, redis_srandmember_response);
}
/* }}} */

/* {{{ proto boolean Redis::sismember(string set, string value) */
PHP_METHOD(Redis, sismember)
{
    REDIS_PROCESS_KW_CMD("SISMEMBER", redis_kv_cmd, redis_1_response);
}
/* }}} */

/* {{{ proto array Redis::sMembers(string set) */
PHP_METHOD(Redis, sMembers)
{
    REDIS_PROCESS_KW_CMD("SMEMBERS", redis_key_cmd,
        redis_sock_read_multibulk_reply);
}

/* {{{ proto array Redis::sMisMember(string key, string member0, ...memberN) */
PHP_METHOD(Redis, sMisMember)
{
    REDIS_PROCESS_KW_CMD("SMISMEMBER", redis_key_varval_cmd, redis_read_variant_reply);
}
/* }}} */

/* {{{ proto array Redis::sInter(string key0, ... string keyN) */
PHP_METHOD(Redis, sInter) {
    REDIS_PROCESS_KW_CMD("SINTER", redis_varkey_cmd, redis_sock_read_multibulk_reply);
}
/* }}} */

PHP_METHOD(Redis, sintercard) {
    REDIS_PROCESS_KW_CMD("SINTERCARD", redis_intercard_cmd, redis_long_response);
}

/* {{{ proto array Redis::sInterStore(string dst, string key0,...string keyN) */
PHP_METHOD(Redis, sInterStore) {
    REDIS_PROCESS_KW_CMD("SINTERSTORE", redis_varkey_cmd, redis_long_response);
}
/* }}} */

/* {{{ proto array Redis::sUnion(string key0, ... string keyN) */
PHP_METHOD(Redis, sUnion) {
    REDIS_PROCESS_KW_CMD("SUNION", redis_varkey_cmd, redis_sock_read_multibulk_reply);
}
/* }}} */

/* {{{ proto array Redis::sUnionStore(array|string $key, string ...$srckeys) */
PHP_METHOD(Redis, sUnionStore) {
    REDIS_PROCESS_KW_CMD("SUNIONSTORE", redis_varkey_cmd, redis_long_response);
}
/* }}} */

/* {{{ proto array Redis::sDiff(string key0, ... string keyN) */
PHP_METHOD(Redis, sDiff) {
    REDIS_PROCESS_KW_CMD("SDIFF", redis_varkey_cmd, redis_sock_read_multibulk_reply);
}
/* }}} */

/* {{{ proto array Redis::sDiffStore(string dst, string key0, ... keyN) */
PHP_METHOD(Redis, sDiffStore) {
    REDIS_PROCESS_KW_CMD("SDIFFSTORE", redis_varkey_cmd, redis_long_response);
}
/* }}} */

/* {{{ proto array Redis::sort(string key, array options) */
PHP_METHOD(Redis, sort) {
    REDIS_PROCESS_KW_CMD("SORT", redis_sort_cmd, redis_read_variant_reply);
}

/* {{{ proto array Redis::sort(string key, array options) */
PHP_METHOD(Redis, sort_ro) {
    REDIS_PROCESS_KW_CMD("SORT_RO", redis_sort_cmd, redis_read_variant_reply);
}

static void
generic_sort_cmd(INTERNAL_FUNCTION_PARAMETERS, int desc, int alpha)
{
    zval *object, *zele, *zget = NULL;
    RedisSock *redis_sock;
    zend_string *zpattern;
    char *key = NULL, *pattern = NULL, *store = NULL;
    size_t keylen, patternlen, storelen;
    zend_long offset = -1, count = -1;
    int argc = 1; /* SORT key is the simplest SORT command */
    smart_string cmd = {0};

    /* Parse myriad of sort arguments */
    if (zend_parse_method_parameters(ZEND_NUM_ARGS(), getThis(),
                                     "Os|s!z!lls", &object, redis_ce, &key,
                                     &keylen, &pattern, &patternlen, &zget,
                                     &offset, &count, &store, &storelen)
                                     == FAILURE)
    {
        RETURN_FALSE;
    }

    /* Ensure we're sorting something, and we can get context */
    if (keylen == 0 || !(redis_sock = redis_sock_get(object, 0)))
        RETURN_FALSE;

    /* Start calculating argc depending on input arguments */
    if (pattern && patternlen)     argc += 2; /* BY pattern */
    if (offset >= 0 && count >= 0) argc += 3; /* LIMIT offset count */
    if (alpha)                     argc += 1; /* ALPHA */
    if (store)                     argc += 2; /* STORE destination */
    if (desc)                      argc += 1; /* DESC (ASC is the default) */

    /* GET is special.  It can be 0 .. N arguments depending what we have */
    if (zget) {
        if (Z_TYPE_P(zget) == IS_ARRAY)
            argc += zend_hash_num_elements(Z_ARRVAL_P(zget));
        else if (Z_STRLEN_P(zget) > 0) {
            argc += 2; /* GET pattern */
        }
    }

    /* Start constructing final command and append key */
    redis_cmd_init_sstr(&cmd, argc, "SORT", 4);
    redis_cmd_append_sstr_key(&cmd, key, keylen, redis_sock, NULL);

    /* BY pattern */
    if (pattern && patternlen) {
        redis_cmd_append_sstr(&cmd, "BY", sizeof("BY") - 1);
        redis_cmd_append_sstr(&cmd, pattern, patternlen);
    }

    /* LIMIT offset count */
    if (offset >= 0 && count >= 0) {
        redis_cmd_append_sstr(&cmd, "LIMIT", sizeof("LIMIT") - 1);
        redis_cmd_append_sstr_long(&cmd, offset);
        redis_cmd_append_sstr_long(&cmd, count);
    }

    /* Handle any number of GET pattern arguments we've been passed */
    if (zget != NULL) {
        if (Z_TYPE_P(zget) == IS_ARRAY) {
            ZEND_HASH_FOREACH_VAL(Z_ARRVAL_P(zget), zele) {
                zpattern = zval_get_string(zele);
                redis_cmd_append_sstr(&cmd, "GET", sizeof("GET") - 1);
                redis_cmd_append_sstr(&cmd, ZSTR_VAL(zpattern), ZSTR_LEN(zpattern));
                zend_string_release(zpattern);
            } ZEND_HASH_FOREACH_END();
        } else {
            zpattern = zval_get_string(zget);
            redis_cmd_append_sstr(&cmd, "GET", sizeof("GET") - 1);
            redis_cmd_append_sstr(&cmd, ZSTR_VAL(zpattern), ZSTR_LEN(zpattern));
            zend_string_release(zpattern);
        }
    }

    /* Append optional DESC and ALPHA modifiers */
    if (desc)  redis_cmd_append_sstr(&cmd, "DESC", sizeof("DESC") - 1);
    if (alpha) redis_cmd_append_sstr(&cmd, "ALPHA", sizeof("ALPHA") - 1);

    /* Finally append STORE if we've got it */
    if (store && storelen) {
        redis_cmd_append_sstr(&cmd, "STORE", sizeof("STORE") - 1);
        redis_cmd_append_sstr_key(&cmd, store, storelen, redis_sock, NULL);
    }

    REDIS_PROCESS_REQUEST(redis_sock, cmd.c, cmd.len);
    if (IS_ATOMIC(redis_sock)) {
        if (redis_read_variant_reply(INTERNAL_FUNCTION_PARAM_PASSTHRU,
                                     redis_sock, NULL, NULL) < 0)
        {
            RETURN_FALSE;
        }
    }
    REDIS_PROCESS_RESPONSE(redis_read_variant_reply);
}

/* {{{ proto array Redis::sortAsc(string key, string pattern, string get,
 *                                int start, int end, bool getList]) */
PHP_METHOD(Redis, sortAsc)
{
    generic_sort_cmd(INTERNAL_FUNCTION_PARAM_PASSTHRU, 0, 0);
}
/* }}} */

/* {{{ proto array Redis::sortAscAlpha(string key, string pattern, string get,
 *                                     int start, int end, bool getList]) */
PHP_METHOD(Redis, sortAscAlpha)
{
    generic_sort_cmd(INTERNAL_FUNCTION_PARAM_PASSTHRU, 0, 1);
}
/* }}} */

/* {{{ proto array Redis::sortDesc(string key, string pattern, string get,
 *                                 int start, int end, bool getList]) */
PHP_METHOD(Redis, sortDesc)
{
    generic_sort_cmd(INTERNAL_FUNCTION_PARAM_PASSTHRU, 1, 0);
}
/* }}} */

/* {{{ proto array Redis::sortDescAlpha(string key, string pattern, string get,
 *                                      int start, int end, bool getList]) */
PHP_METHOD(Redis, sortDescAlpha)
{
    generic_sort_cmd(INTERNAL_FUNCTION_PARAM_PASSTHRU, 1, 1);
}
/* }}} */

/* {{{ proto array Redis::expire(string key, int timeout) */
PHP_METHOD(Redis, expire) {
    REDIS_PROCESS_KW_CMD("EXPIRE", redis_expire_cmd, redis_1_response);
}
/* }}} */

/* {{{ proto bool Redis::pexpire(string key, long ms) */
PHP_METHOD(Redis, pexpire) {
    REDIS_PROCESS_KW_CMD("PEXPIRE", redis_expire_cmd, redis_1_response);
}
/* }}} */

/* {{{ proto array Redis::expireAt(string key, int timestamp) */
PHP_METHOD(Redis, expireAt) {
    REDIS_PROCESS_KW_CMD("EXPIREAT", redis_expire_cmd, redis_1_response);
}
/* }}} */

/* {{{ proto array Redis::pexpireAt(string key, int timestamp) */
PHP_METHOD(Redis, pexpireAt) {
    REDIS_PROCESS_KW_CMD("PEXPIREAT", redis_expire_cmd, redis_1_response);
}
/* }}} */

/* {{{ proto Redis::expiretime(string $key): int */
PHP_METHOD(Redis, expiretime) {
    REDIS_PROCESS_KW_CMD("EXPIRETIME", redis_key_cmd, redis_long_response);
}
/* }}} */

/* {{{ proto Redis::expiretime(string $key): int */
PHP_METHOD(Redis, pexpiretime) {
    REDIS_PROCESS_KW_CMD("PEXPIRETIME", redis_key_cmd, redis_long_response);
}

/* }}} */
/* {{{ proto array Redis::lSet(string key, int index, string value) */
PHP_METHOD(Redis, lSet) {
    REDIS_PROCESS_KW_CMD("LSET", redis_key_long_val_cmd,
        redis_boolean_response);
}
/* }}} */

/* {{{ proto string Redis::save() */
PHP_METHOD(Redis, save)
{
    REDIS_PROCESS_KW_CMD("SAVE", redis_empty_cmd, redis_boolean_response);
}
/* }}} */

/* {{{ proto string Redis::bgSave() */
PHP_METHOD(Redis, bgSave)
{
    REDIS_PROCESS_KW_CMD("BGSAVE", redis_empty_cmd, redis_boolean_response);
}
/* }}} */

/* {{{ proto integer Redis::lastSave() */
PHP_METHOD(Redis, lastSave)
{
    REDIS_PROCESS_KW_CMD("LASTSAVE", redis_empty_cmd, redis_long_response);
}
/* }}} */

/* {{{ proto bool Redis::failover([array to [,bool abort [,int timeout]]] ) */
PHP_METHOD(Redis, failover)
{
    REDIS_PROCESS_CMD(failover, redis_boolean_response);
}
/* }}} */

/* {{{ proto bool Redis::flushDB([bool async]) */
PHP_METHOD(Redis, flushDB)
{
    REDIS_PROCESS_KW_CMD("FLUSHDB", redis_flush_cmd, redis_boolean_response);
}
/* }}} */

/* {{{ proto bool Redis::flushAll([bool async]) */
PHP_METHOD(Redis, flushAll)
{
    REDIS_PROCESS_KW_CMD("FLUSHALL", redis_flush_cmd, redis_boolean_response);
}
/* }}} */

/* {{{ proto mixed Redis::function(string op, mixed ...args) */
PHP_METHOD(Redis, function)
{
    REDIS_PROCESS_CMD(function, redis_function_response)
}

/* {{{ proto int Redis::dbSize() */
PHP_METHOD(Redis, dbSize)
{
    REDIS_PROCESS_KW_CMD("DBSIZE", redis_empty_cmd, redis_long_response);
}
/* }}} */

/* {{{ proto bool Redis::auth(string passwd) */
PHP_METHOD(Redis, auth) {
    REDIS_PROCESS_CMD(auth, redis_boolean_response);
}
/* }}} */

/* {{{ proto long Redis::persist(string key) */
PHP_METHOD(Redis, persist) {
    REDIS_PROCESS_KW_CMD("PERSIST", redis_key_cmd, redis_1_response);
}
/* }}} */


/* {{{ proto long Redis::ttl(string key) */
PHP_METHOD(Redis, ttl) {
    REDIS_PROCESS_KW_CMD("TTL", redis_key_cmd, redis_long_response);
}
/* }}} */

/* {{{ proto long Redis::pttl(string key) */
PHP_METHOD(Redis, pttl) {
    REDIS_PROCESS_KW_CMD("PTTL", redis_key_cmd, redis_long_response);
}
/* }}} */

/* {{{ proto array Redis::info() */
PHP_METHOD(Redis, info) {
    REDIS_PROCESS_CMD(info, redis_info_response);
}
/* }}} */

/* {{{ proto bool Redis::select(long dbNumber) */
PHP_METHOD(Redis, select) {
    REDIS_PROCESS_CMD(select, redis_select_response);
}
/* }}} */

/* {{{ proto bool Redis::swapdb(long srcdb, long dstdb) */
PHP_METHOD(Redis, swapdb) {
    REDIS_PROCESS_KW_CMD("SWAPDB", redis_long_long_cmd, redis_boolean_response);
}

/* {{{ proto bool Redis::move(string key, long dbindex) */
PHP_METHOD(Redis, move) {
    REDIS_PROCESS_KW_CMD("MOVE", redis_key_long_cmd, redis_1_response);
}
/* }}} */

/* {{{ proto bool Redis::mset(array (key => value, ...)) */
PHP_METHOD(Redis, mset) {
    REDIS_PROCESS_KW_CMD("MSET", redis_mset_cmd, redis_boolean_response);
}
/* }}} */


/* {{{ proto bool Redis::msetnx(array (key => value, ...)) */
PHP_METHOD(Redis, msetnx) {
    REDIS_PROCESS_KW_CMD("MSETNX", redis_mset_cmd, redis_1_response);
}
/* }}} */

/* {{{ proto string Redis::rpoplpush(string srckey, string dstkey) */
PHP_METHOD(Redis, rpoplpush)
{
    REDIS_PROCESS_KW_CMD("RPOPLPUSH", redis_key_key_cmd, redis_string_response);
}
/* }}} */

/* {{{ proto string Redis::brpoplpush(string src, string dst, int timeout) */
PHP_METHOD(Redis, brpoplpush) {
    REDIS_PROCESS_CMD(brpoplpush, redis_string_response);
}
/* }}} */

/* {{{ proto long Redis::zAdd(string key, int score, string value) */
PHP_METHOD(Redis, zAdd) {
    REDIS_PROCESS_CMD(zadd, redis_zadd_response);
}
/* }}} */

/* {{{ proto array Redis::zRandMember(string key, array options) */
PHP_METHOD(Redis, zRandMember) {
    REDIS_PROCESS_CMD(zrandmember, redis_zrandmember_response);
}
/* }}} */

/* {{{ proto array Redis::zRange(string key,int start,int end,bool scores = 0) */
PHP_METHOD(Redis, zRange) {
    REDIS_PROCESS_KW_CMD("ZRANGE", redis_zrange_cmd, redis_zrange_response);
}
/* }}} */

PHP_METHOD(Redis, zrangestore) {
    REDIS_PROCESS_KW_CMD("ZRANGESTORE", redis_zrange_cmd, redis_long_response);
}

/* {{{ proto array Redis::zRevRange(string k, long s, long e, bool scores = 0) */
PHP_METHOD(Redis, zRevRange) {
    REDIS_PROCESS_KW_CMD("ZREVRANGE", redis_zrange_cmd, redis_zrange_response);
}
/* }}} */

/* {{{ proto array Redis::zRangeByScore(string k,string s,string e,array opt) */
PHP_METHOD(Redis, zRangeByScore) {
    REDIS_PROCESS_KW_CMD("ZRANGEBYSCORE", redis_zrange_cmd, redis_zrange_response);
}
/* }}} */

/* {{{ proto array Redis::zRevRangeByScore(string key, string start, string end,
 *                                         array options) */
PHP_METHOD(Redis, zRevRangeByScore) {
    REDIS_PROCESS_KW_CMD("ZREVRANGEBYSCORE", redis_zrange_cmd, redis_zrange_response);
}
/* }}} */

/* {{{ proto array Redis::zRangeByLex(string key, string min, string max, [
 *                                    offset, limit]) */
PHP_METHOD(Redis, zRangeByLex) {
    REDIS_PROCESS_KW_CMD("ZRANGEBYLEX", redis_zrangebylex_cmd,
        redis_sock_read_multibulk_reply);
}
/* }}} */

PHP_METHOD(Redis, zRevRangeByLex) {
    REDIS_PROCESS_KW_CMD("ZREVRANGEBYLEX", redis_zrangebylex_cmd,
        redis_sock_read_multibulk_reply);
}
/* }}} */

/* {{{ proto long Redis::zLexCount(string key, string min, string max) */
PHP_METHOD(Redis, zLexCount) {
    REDIS_PROCESS_KW_CMD("ZLEXCOUNT", redis_gen_zlex_cmd, redis_long_response);
}
/* }}} */

/* {{{ proto long Redis::zRemRangeByLex(string key, string min, string max) */
PHP_METHOD(Redis, zRemRangeByLex) {
    REDIS_PROCESS_KW_CMD("ZREMRANGEBYLEX", redis_gen_zlex_cmd,
        redis_long_response);
}
/* }}} */

/* {{{ proto long Redis::zRem(string key, string member) */
PHP_METHOD(Redis, zRem)
{
    REDIS_PROCESS_KW_CMD("ZREM", redis_key_varval_cmd, redis_long_response);
}
/* }}} */

/* {{{ proto long Redis::zRemRangeByScore(string k, string s, string e) */
PHP_METHOD(Redis, zRemRangeByScore)
{
    REDIS_PROCESS_KW_CMD("ZREMRANGEBYSCORE", redis_key_str_str_cmd,
        redis_long_response);
}
/* }}} */

/* {{{ proto long Redis::zRemRangeByRank(string key, long start, long end) */
PHP_METHOD(Redis, zRemRangeByRank)
{
    REDIS_PROCESS_KW_CMD("ZREMRANGEBYRANK", redis_key_long_long_cmd,
        redis_long_response);
}
/* }}} */

/* {{{ proto array Redis::zCount(string key, string start , string end) */
PHP_METHOD(Redis, zCount)
{
    REDIS_PROCESS_KW_CMD("ZCOUNT", redis_key_str_str_cmd, redis_long_response);
}
/* }}} */

/* {{{ proto long Redis::zCard(string key) */
PHP_METHOD(Redis, zCard)
{
    REDIS_PROCESS_KW_CMD("ZCARD", redis_key_cmd, redis_long_response);
}
/* }}} */

/* {{{ proto double Redis::zScore(string key, mixed member) */
PHP_METHOD(Redis, zScore)
{
    REDIS_PROCESS_KW_CMD("ZSCORE", redis_kv_cmd,
        redis_bulk_double_response);
}
/* }}} */

/* {{{ proto array Redis::zMscore(string key, string member0, ...memberN) */
PHP_METHOD(Redis, zMscore)
{
    REDIS_PROCESS_KW_CMD("ZMSCORE", redis_key_varval_cmd, redis_mbulk_reply_double);
}
/* }}} */

/* {{{ proto long Redis::zRank(string key, string member) */
PHP_METHOD(Redis, zRank) {
    REDIS_PROCESS_KW_CMD("ZRANK", redis_kv_cmd, redis_long_response);
}
/* }}} */

/* {{{ proto long Redis::zRevRank(string key, string member) */
PHP_METHOD(Redis, zRevRank) {
    REDIS_PROCESS_KW_CMD("ZREVRANK", redis_kv_cmd, redis_long_response);
}
/* }}} */

/* {{{ proto double Redis::zIncrBy(string key, double value, mixed member) */
PHP_METHOD(Redis, zIncrBy)
{
    REDIS_PROCESS_CMD(zincrby, redis_bulk_double_response);
}
/* }}} */

/* {{{ proto array Redis::zdiff(array keys, array options) */
PHP_METHOD(Redis, zdiff) {
    REDIS_PROCESS_CMD(zdiff, redis_zdiff_response);
}
/* }}} */

/* {{{ proto array Redis::zinter(array keys, array|null weights, array options) */
PHP_METHOD(Redis, zinter) {
    REDIS_PROCESS_KW_CMD("ZINTER", redis_zinterunion_cmd, redis_zdiff_response);
}
/* }}} */

PHP_METHOD(Redis, zintercard) {
    REDIS_PROCESS_KW_CMD("ZINTERCARD", redis_intercard_cmd, redis_long_response);
}

/* {{{ proto array Redis::zunion(array keys, array|null weights, array options) */
PHP_METHOD(Redis, zunion) {
    REDIS_PROCESS_KW_CMD("ZUNION", redis_zinterunion_cmd, redis_zdiff_response);
}
/* }}} */

/* {{{ proto array Redis::zdiffstore(string destination, array keys) */
PHP_METHOD(Redis, zdiffstore) {
    REDIS_PROCESS_CMD(zdiffstore, redis_long_response);
}
/* }}} */

/* zinterstore */
PHP_METHOD(Redis, zinterstore) {
    REDIS_PROCESS_KW_CMD("ZINTERSTORE", redis_zinterunionstore_cmd, redis_long_response);
}

/* zunionstore */
PHP_METHOD(Redis, zunionstore) {
    REDIS_PROCESS_KW_CMD("ZUNIONSTORE", redis_zinterunionstore_cmd, redis_long_response);
}

/* {{{ proto array Redis::zPopMax(string key) */
PHP_METHOD(Redis, zPopMax)
{
    if (ZEND_NUM_ARGS() == 1) {
        REDIS_PROCESS_KW_CMD("ZPOPMAX", redis_key_cmd, redis_mbulk_reply_zipped_keys_dbl);
    } else if (ZEND_NUM_ARGS() == 2) {
        REDIS_PROCESS_KW_CMD("ZPOPMAX", redis_key_long_cmd, redis_mbulk_reply_zipped_keys_dbl);
    } else {
        ZEND_WRONG_PARAM_COUNT();
    }
}
/* }}} */

/* {{{ proto array Redis::zPopMin(string key) */
PHP_METHOD(Redis, zPopMin)
{
    if (ZEND_NUM_ARGS() == 1) {
        REDIS_PROCESS_KW_CMD("ZPOPMIN", redis_key_cmd, redis_mbulk_reply_zipped_keys_dbl);
    } else if (ZEND_NUM_ARGS() == 2) {
        REDIS_PROCESS_KW_CMD("ZPOPMIN", redis_key_long_cmd, redis_mbulk_reply_zipped_keys_dbl);
    } else {
        ZEND_WRONG_PARAM_COUNT();
    }
}
/* }}} */

/* {{{ proto Redis::bzPopMax(Array(keys) [, timeout]): Array */
PHP_METHOD(Redis, bzPopMax) {
    REDIS_PROCESS_KW_CMD("BZPOPMAX", redis_blocking_pop_cmd, redis_sock_read_multibulk_reply);
}
/* }}} */

/* {{{ proto Redis::bzPopMin(Array(keys) [, timeout]): Array */
PHP_METHOD(Redis, bzPopMin) {
    REDIS_PROCESS_KW_CMD("BZPOPMIN", redis_blocking_pop_cmd, redis_sock_read_multibulk_reply);
}
/* }}} */

/* {{{ proto Redis|array|false Redis::lmpop(array $keys, string $from, int $count = 1) */
PHP_METHOD(Redis, lmpop) {
    REDIS_PROCESS_KW_CMD("LMPOP", redis_mpop_cmd, redis_mpop_response);
}
/* }}} */

/* {{{ proto Redis|array|false Redis::blmpop(double $timeout, array $keys, string $from, int $count = 1) */
PHP_METHOD(Redis, blmpop) {
    REDIS_PROCESS_KW_CMD("BLMPOP", redis_mpop_cmd, redis_mpop_response);
}
/* }}} */

/* {{{ proto Redis|array|false Redis::zmpop(array $keys, string $from, int $count = 1) */
PHP_METHOD(Redis, zmpop) {
    REDIS_PROCESS_KW_CMD("ZMPOP", redis_mpop_cmd, redis_mpop_response);
}

/* {{{ proto Redis|array|false Redis::bzmpop(double $timeout, array $keys, string $from, int $count = 1) */
PHP_METHOD(Redis, bzmpop) {
    REDIS_PROCESS_KW_CMD("BZMPOP", redis_mpop_cmd, redis_mpop_response);
}

/* }}} */
/* hashes */

/* {{{ proto long Redis::hset(string key, string mem, string val) */
PHP_METHOD(Redis, hSet)
{
    REDIS_PROCESS_CMD(hset, redis_long_response);
}
/* }}} */

/* {{{ proto bool Redis::hSetNx(string key, string mem, string val) */
PHP_METHOD(Redis, hSetNx)
{
    REDIS_PROCESS_CMD(hsetnx, redis_1_response);
}
/* }}} */

/* {{{ proto string Redis::hget(string key, string mem) */
PHP_METHOD(Redis, hGet)
{
    REDIS_PROCESS_KW_CMD("HGET", redis_key_str_cmd, redis_string_response);
}
/* }}} */

/* {{{ proto long Redis::hLen(string key) */
PHP_METHOD(Redis, hLen)
{
    REDIS_PROCESS_KW_CMD("HLEN", redis_key_cmd, redis_long_response);
}
/* }}} */

/* {{{ proto long Redis::hDel(string key, string mem1, ... memN) */
PHP_METHOD(Redis, hDel)
{
    REDIS_PROCESS_CMD(hdel, redis_long_response);
}
/* }}} */

/* {{{ proto bool Redis::hExists(string key, string mem) */
PHP_METHOD(Redis, hExists)
{
    REDIS_PROCESS_KW_CMD("HEXISTS", redis_key_str_cmd, redis_1_response);
}

/* {{{ proto array Redis::hkeys(string key) */
PHP_METHOD(Redis, hKeys)
{
    REDIS_PROCESS_KW_CMD("HKEYS", redis_key_cmd, redis_mbulk_reply_raw);
}
/* }}} */

/* {{{ proto array Redis::hvals(string key) */
PHP_METHOD(Redis, hVals)
{
    REDIS_PROCESS_KW_CMD("HVALS", redis_key_cmd,
        redis_sock_read_multibulk_reply);
}

/* {{{ proto array Redis::hgetall(string key) */
PHP_METHOD(Redis, hGetAll) {
    REDIS_PROCESS_KW_CMD("HGETALL", redis_key_cmd, redis_mbulk_reply_zipped_vals);
}
/* }}} */

/* {{{ proto double Redis::hIncrByFloat(string k, string me, double v) */
PHP_METHOD(Redis, hIncrByFloat)
{
    REDIS_PROCESS_CMD(hincrbyfloat, redis_bulk_double_response);
}
/* }}} */

/* {{{ proto long Redis::hincrby(string key, string mem, long byval) */
PHP_METHOD(Redis, hIncrBy)
{
    REDIS_PROCESS_CMD(hincrby, redis_long_response);
}
/* }}} */

/* {{{ array Redis::hMget(string hash, array keys) */
PHP_METHOD(Redis, hMget) {
    REDIS_PROCESS_CMD(hmget, redis_mbulk_reply_assoc);
}
/* }}} */

/* {{{ proto bool Redis::hmset(string key, array keyvals) */
PHP_METHOD(Redis, hMset)
{
    REDIS_PROCESS_CMD(hmset, redis_boolean_response);
}
/* }}} */

/* {{{ proto bool Redis::hRandField(string key, [array $options]) */
PHP_METHOD(Redis, hRandField)
{
    REDIS_PROCESS_CMD(hrandfield, redis_hrandfield_response);
}
/* }}} */


/* {{{ proto long Redis::hstrlen(string key, string field) */
PHP_METHOD(Redis, hStrLen) {
    REDIS_PROCESS_CMD(hstrlen, redis_long_response);
}
/* }}} */

/* flag : get, set {ATOMIC, MULTI, PIPELINE} */

PHP_METHOD(Redis, multi)
{

    RedisSock *redis_sock;
    char *resp;
    int resp_len;
    zval *object;
    zend_long multi_value = MULTI;

    if (zend_parse_method_parameters(ZEND_NUM_ARGS(), getThis(),
                                     "O|l", &object, redis_ce, &multi_value)
                                     == FAILURE)
    {
        RETURN_FALSE;
    }

    /* if the flag is activated, send the command, the reply will be "QUEUED"
     * or -ERR */
    if ((redis_sock = redis_sock_get(object, 0)) == NULL) {
        RETURN_FALSE;
    }

    if (multi_value == PIPELINE) {
        /* Cannot enter pipeline mode in a MULTI block */
        if (IS_MULTI(redis_sock)) {
            php_error_docref(NULL, E_ERROR, "Can't activate pipeline in multi mode!");
            RETURN_FALSE;
        }

        /* Enable PIPELINE if we're not already in one */
        if (IS_ATOMIC(redis_sock)) {
            REDIS_ENABLE_MODE(redis_sock, PIPELINE);
        }
    } else if (multi_value == MULTI) {
        /* Don't want to do anything if we're already in MULTI mode */
        if (!IS_MULTI(redis_sock)) {
            if (IS_PIPELINE(redis_sock)) {
                PIPELINE_ENQUEUE_COMMAND(RESP_MULTI_CMD, sizeof(RESP_MULTI_CMD) - 1);
                REDIS_SAVE_CALLBACK(NULL, NULL);
                REDIS_ENABLE_MODE(redis_sock, MULTI);
            } else {
                SOCKET_WRITE_COMMAND(redis_sock, RESP_MULTI_CMD, sizeof(RESP_MULTI_CMD) - 1)
                if ((resp = redis_sock_read(redis_sock, &resp_len)) == NULL) {
                    RETURN_FALSE;
                } else if (strncmp(resp, "+OK", 3) != 0) {
                    efree(resp);
                    RETURN_FALSE;
                }
                efree(resp);
                REDIS_ENABLE_MODE(redis_sock, MULTI);
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
    zval *object;

    if (zend_parse_method_parameters(ZEND_NUM_ARGS(), getThis(), "O",
                                     &object, redis_ce) == FAILURE) {
        RETURN_FALSE;
    }

    if ((redis_sock = redis_sock_get(object, 0)) == NULL) {
        RETURN_FALSE;
    }

    if (IS_PIPELINE(redis_sock)) {
        ret = SUCCESS;
        if (redis_sock->pipeline_cmd) {
            zend_string_release(redis_sock->pipeline_cmd);
            redis_sock->pipeline_cmd = NULL;
        }
    } else if (IS_MULTI(redis_sock)) {
        ret = redis_send_discard(redis_sock);
    }
    if (ret == SUCCESS) {
        free_reply_callbacks(redis_sock);
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

    array_init(z_tab);

    return redis_sock_read_multibulk_multi_reply_loop(INTERNAL_FUNCTION_PARAM_PASSTHRU,
                    redis_sock, z_tab);
}


/* exec */
PHP_METHOD(Redis, exec)
{
    RedisSock *redis_sock;
    int ret;
    zval *object, z_ret;

    if (zend_parse_method_parameters(ZEND_NUM_ARGS(), getThis(),
                                     "O", &object, redis_ce) == FAILURE ||
        (redis_sock = redis_sock_get(object, 0)) == NULL
    ) {
        RETURN_FALSE;
    }

    ZVAL_FALSE(&z_ret);

    if (IS_MULTI(redis_sock)) {
        if (IS_PIPELINE(redis_sock)) {
            PIPELINE_ENQUEUE_COMMAND(RESP_EXEC_CMD, sizeof(RESP_EXEC_CMD) - 1);
            REDIS_SAVE_CALLBACK(NULL, NULL);
            REDIS_DISABLE_MODE(redis_sock, MULTI);
            RETURN_ZVAL(getThis(), 1, 0);
        }
        SOCKET_WRITE_COMMAND(redis_sock, RESP_EXEC_CMD, sizeof(RESP_EXEC_CMD) - 1)

        ret = redis_sock_read_multibulk_multi_reply(
            INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock, &z_ret);
        free_reply_callbacks(redis_sock);
        REDIS_DISABLE_MODE(redis_sock, MULTI);
        redis_sock->watching = 0;
        if (ret < 0) {
            zval_dtor(&z_ret);
            ZVAL_FALSE(&z_ret);
        }
    }

    if (IS_PIPELINE(redis_sock)) {
        if (redis_sock->pipeline_cmd == NULL) {
            /* Empty array when no command was run. */
            array_init(&z_ret);
        } else {
            if (redis_sock_write(redis_sock, ZSTR_VAL(redis_sock->pipeline_cmd),
                    ZSTR_LEN(redis_sock->pipeline_cmd)) < 0) {
                ZVAL_FALSE(&z_ret);
            } else {
                array_init(&z_ret);
                if (redis_sock_read_multibulk_multi_reply_loop(
                    INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock, &z_ret) != SUCCESS) {
                    zval_dtor(&z_ret);
                    ZVAL_FALSE(&z_ret);
                }
            }
            zend_string_release(redis_sock->pipeline_cmd);
            redis_sock->pipeline_cmd = NULL;
        }
        free_reply_callbacks(redis_sock);
        REDIS_DISABLE_MODE(redis_sock, PIPELINE);
    }
    RETURN_ZVAL(&z_ret, 0, 1);
}

PHP_REDIS_API int
redis_response_enqueued(RedisSock *redis_sock)
{
    char *resp;
    int resp_len, ret = FAILURE;

    if ((resp = redis_sock_read(redis_sock, &resp_len)) != NULL) {
        if (strncmp(resp, "+QUEUED", 7) == 0) {
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

    for (fi = redis_sock->head; fi; /* void */) {
        if (fi->fun) {
            fi->fun(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock, z_tab, fi->ctx);
            fi = fi->next;
            continue;
        }
        size_t len;
        char inbuf[255];

        if (redis_sock_gets(redis_sock, inbuf, sizeof(inbuf) - 1, &len) < 0 || strncmp(inbuf, "+OK", 3) != 0) {
            return FAILURE;
        }

        while ((fi = fi->next) && fi->fun) {
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

        if (num > 0 && redis_read_multibulk_recursive(redis_sock, num, 0, &z_ret) < 0) {
            return FAILURE;
        }

        if (fi) fi = fi->next;
    }
    redis_sock->current = fi;
    return SUCCESS;
}

PHP_METHOD(Redis, pipeline)
{
    RedisSock *redis_sock;
    zval *object;

    if (zend_parse_method_parameters(ZEND_NUM_ARGS(), getThis(),
                                     "O", &object, redis_ce) == FAILURE ||
        (redis_sock = redis_sock_get(object, 0)) == NULL
    ) {
        RETURN_FALSE;
    }

    /* User cannot enter MULTI mode if already in a pipeline */
    if (IS_MULTI(redis_sock)) {
        php_error_docref(NULL, E_ERROR, "Can't activate pipeline in multi mode!");
        RETURN_FALSE;
    }

    /* Enable pipeline mode unless we're already in that mode in which case this
     * is just a NO OP */
    if (IS_ATOMIC(redis_sock)) {
        /* NB : we keep the function fold, to detect the last function.
         * We need the response format of the n - 1 command. So, we can delete
         * when n > 2, the { 1 .. n - 2} commands */
        REDIS_ENABLE_MODE(redis_sock, PIPELINE);
    }

    RETURN_ZVAL(getThis(), 1, 0);
}

/* {{{ proto long Redis::publish(string channel, string msg) */
PHP_METHOD(Redis, publish)
{
    REDIS_PROCESS_KW_CMD("PUBLISH", redis_key_str_cmd, redis_long_response);
}
/* }}} */

/* {{{ proto void Redis::psubscribe(Array(pattern1, pattern2, ... patternN)) */
PHP_METHOD(Redis, psubscribe)
{
    REDIS_PROCESS_KW_CMD("PSUBSCRIBE", redis_subscribe_cmd,
        redis_subscribe_response);
}
/* }}} */

/* {{{ proto void Redis::ssubscribe(Array(shardchannel1, shardchannel2, ... shardchannelN)) */
PHP_METHOD(Redis, ssubscribe)
{
    REDIS_PROCESS_KW_CMD("SSUBSCRIBE", redis_subscribe_cmd,
        redis_subscribe_response);
}
/* }}} */

/* {{{ proto void Redis::subscribe(Array(channel1, channel2, ... channelN)) */
PHP_METHOD(Redis, subscribe) {
    REDIS_PROCESS_KW_CMD("SUBSCRIBE", redis_subscribe_cmd,
        redis_subscribe_response);
}

/**
 *  [ps]unsubscribe channel_0 channel_1 ... channel_n
 *  [ps]unsubscribe(array(channel_0, channel_1, ..., channel_n))
 * response format :
 * array(
 *    channel_0 => TRUE|FALSE,
 *    channel_1 => TRUE|FALSE,
 *    ...
 *    channel_n => TRUE|FALSE
 * );
 **/

PHP_METHOD(Redis, unsubscribe)
{
    REDIS_PROCESS_KW_CMD("UNSUBSCRIBE", redis_unsubscribe_cmd,
        redis_unsubscribe_response);
}

PHP_METHOD(Redis, punsubscribe)
{
    REDIS_PROCESS_KW_CMD("PUNSUBSCRIBE", redis_unsubscribe_cmd,
        redis_unsubscribe_response);
}

PHP_METHOD(Redis, sunsubscribe)
{
    REDIS_PROCESS_KW_CMD("SUNSUBSCRIBE", redis_unsubscribe_cmd,
        redis_unsubscribe_response);
}

/* {{{ proto string Redis::bgrewriteaof() */
PHP_METHOD(Redis, bgrewriteaof)
{
    REDIS_PROCESS_KW_CMD("BGREWRITEAOF", redis_empty_cmd,
        redis_boolean_response);
}
/* }}} */

/* {{{ public function slaveof(string $host = NULL, int $port = NULL): Redis|bool }}} */
PHP_METHOD(Redis, slaveof) {
    REDIS_PROCESS_KW_CMD("SLAVEOF", redis_replicaof_cmd, redis_boolean_response);
}
/* }}} */

/* {{{ public function replicaof(string $host = NULL, int $port = NULL): Redis|bool }}} */
PHP_METHOD(Redis, replicaof) {
    REDIS_PROCESS_KW_CMD("REPLICAOF", redis_replicaof_cmd, redis_boolean_response);
}

/* }}} */
/* {{{ proto string Redis::object(key) */
PHP_METHOD(Redis, object)
{
    REDIS_PROCESS_CMD(object, redis_object_response);
}
/* }}} */

/* {{{ proto string Redis::getOption($option) */
PHP_METHOD(Redis, getOption)
{
    RedisSock *redis_sock;

    if ((redis_sock = redis_sock_get_instance(getThis(), 0)) == NULL) {
        RETURN_FALSE;
    }

    redis_getoption_handler(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock, NULL);
}
/* }}} */

/* {{{ proto string Redis::setOption(string $option, mixed $value) */
PHP_METHOD(Redis, setOption)
{
    RedisSock *redis_sock;

    if ((redis_sock = redis_sock_get_instance(getThis(), 0)) == NULL) {
        RETURN_FALSE;
    }

    redis_setoption_handler(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock, NULL);
}
/* }}} */

/* {{{ proto boolean Redis::config(string op, string key [, mixed value]) */
/* {{{ proto public function config(string $op, string ...$args) }}} */
// CONFIG SET/GET
PHP_METHOD(Redis, config) {
    REDIS_PROCESS_CMD(config, redis_config_response);
}
/* }}} */


/* {{{ proto boolean Redis::slowlog(string arg, [int option]) */
PHP_METHOD(Redis, slowlog) {
    REDIS_PROCESS_CMD(slowlog, redis_read_variant_reply);
}

/* {{{ proto Redis::wait(int num_slaves, int ms) }}} */
PHP_METHOD(Redis, wait) {
    REDIS_PROCESS_KW_CMD("WAIT", redis_long_long_cmd, redis_long_response);
}

/*
 * {{{ proto Redis::pubsub("channels", pattern);
 *     proto Redis::pubsub("numsub", Array channels);
 *     proto Redis::pubsub("numpat"); }}}
 */
PHP_METHOD(Redis, pubsub) {
    REDIS_PROCESS_CMD(pubsub, redis_pubsub_response);
}

/* {{{ proto variant Redis::eval(string script, [array keys, long num_keys]) */
PHP_METHOD(Redis, eval) {
    REDIS_PROCESS_KW_CMD("EVAL", redis_eval_cmd, redis_read_raw_variant_reply);
}

/* {{{ proto variant Redis::eval_ro(string script, [array keys, long num_keys]) */
PHP_METHOD(Redis, eval_ro) {
    REDIS_PROCESS_KW_CMD("EVAL_RO", redis_eval_cmd, redis_read_raw_variant_reply);
}

/* {{{ proto variant Redis::evalsha(string sha1, [array keys, long num_keys]) */
PHP_METHOD(Redis, evalsha) {
    REDIS_PROCESS_KW_CMD("EVALSHA", redis_eval_cmd, redis_read_raw_variant_reply);
}

/* {{{ proto variant Redis::evalsha_ro(string sha1, [array keys, long num_keys]) */
PHP_METHOD(Redis, evalsha_ro) {
    REDIS_PROCESS_KW_CMD("EVALSHA_RO", redis_eval_cmd, redis_read_raw_variant_reply);
}

/* {{{ proto variant Redis::fcall(string fn [, array keys [, array args]]) */
PHP_METHOD(Redis, fcall) {
    REDIS_PROCESS_KW_CMD("FCALL", redis_fcall_cmd, redis_read_raw_variant_reply);
}

/* {{{ proto variant Redis::fcall_ro(string fn [, array keys [, array args]]) */
PHP_METHOD(Redis, fcall_ro) {
    REDIS_PROCESS_KW_CMD("FCALL_RO", redis_fcall_cmd, redis_read_raw_variant_reply);
}

/* {{{ public function script($args...): mixed }}} */
PHP_METHOD(Redis, script) {
    REDIS_PROCESS_CMD(script, redis_read_variant_reply);
}

/* {{{ proto DUMP key */
PHP_METHOD(Redis, dump) {
    REDIS_PROCESS_KW_CMD("DUMP", redis_key_cmd, redis_string_response);
}
/* }}} */

/* {{{ proto Redis::restore(ttl, key, value) */
PHP_METHOD(Redis, restore) {
    REDIS_PROCESS_CMD(restore, redis_boolean_response);
}
/* }}} */

/* {{{ proto Redis::debug(string key) */
PHP_METHOD(Redis, debug) {
    REDIS_PROCESS_KW_CMD("DEBUG", redis_key_cmd, redis_string_response);
}
/* }}} */

/* {{{ proto Redis::migrate(host port key dest-db timeout [bool copy,
 *                          bool replace]) */
PHP_METHOD(Redis, migrate) {
    REDIS_PROCESS_CMD(migrate, redis_boolean_response);
}

/* {{{ proto Redis::_prefix(key) */
PHP_METHOD(Redis, _prefix) {
    RedisSock *redis_sock;

    if ((redis_sock = redis_sock_get_instance(getThis(), 0)) == NULL) {
        RETURN_FALSE;
    }

    redis_prefix_handler(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock);
}

/* {{{ proto Redis::_serialize(value) */
PHP_METHOD(Redis, _serialize) {
    RedisSock *redis_sock;

    // Grab socket
    if ((redis_sock = redis_sock_get_instance(getThis(), 0)) == NULL) {
        RETURN_FALSE;
    }

    redis_serialize_handler(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock);
}

/* {{{ proto Redis::_unserialize(value) */
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

PHP_METHOD(Redis, _unpack) {
    RedisSock *redis_sock;

    // Grab socket
    if ((redis_sock = redis_sock_get_instance(getThis(), 0)) == NULL) {
        RETURN_FALSE;
    }

    redis_unpack_handler(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock);
}

/* {{{ proto Redis::getLastError() */
PHP_METHOD(Redis, getLastError) {
    zval *object;
    RedisSock *redis_sock;

    // Grab our object
    if(zend_parse_method_parameters(ZEND_NUM_ARGS(), getThis(), "O",
                                    &object, redis_ce) == FAILURE)
    {
        RETURN_FALSE;
    }

    // Grab socket
    if ((redis_sock = redis_sock_get_instance(object, 0)) == NULL) {
        RETURN_FALSE;
    }

    /* Return our last error or NULL if we don't have one */
    if (redis_sock->err) {
        RETURN_STRINGL(ZSTR_VAL(redis_sock->err), ZSTR_LEN(redis_sock->err));
    }
    RETURN_NULL();
}

/* {{{ proto Redis::clearLastError() */
PHP_METHOD(Redis, clearLastError) {
    zval *object;
    RedisSock *redis_sock;

    // Grab our object
    if(zend_parse_method_parameters(ZEND_NUM_ARGS(), getThis(), "O",
                                    &object, redis_ce) == FAILURE)
    {
        RETURN_FALSE;
    }
    // Grab socket
    if ((redis_sock = redis_sock_get_instance(object, 0)) == NULL) {
        RETURN_FALSE;
    }

    // Clear error message
    if (redis_sock->err) {
        zend_string_release(redis_sock->err);
        redis_sock->err = NULL;
    }

    RETURN_TRUE;
}

/*
 * {{{ proto long Redis::getMode()
 */
PHP_METHOD(Redis, getMode) {
    zval *object;
    RedisSock *redis_sock;

    /* Grab our object */
    if (zend_parse_method_parameters(ZEND_NUM_ARGS(), getThis(), "O", &object, redis_ce) == FAILURE) {
        RETURN_FALSE;
    }

    /* Grab socket */
    if ((redis_sock = redis_sock_get_instance(object, 0)) == NULL) {
        RETURN_FALSE;
    }

    if (IS_PIPELINE(redis_sock)) {
        RETVAL_LONG(PIPELINE);
    } else if (IS_MULTI(redis_sock)) {
        RETVAL_LONG(MULTI);
    } else {
        RETVAL_LONG(ATOMIC);
    }
}

/* {{{ proto Redis::time() */
PHP_METHOD(Redis, time) {
    REDIS_PROCESS_KW_CMD("TIME", redis_empty_cmd, redis_mbulk_reply_raw);
}

/* {{{ proto array Redis::role() */
PHP_METHOD(Redis, role) {
    REDIS_PROCESS_KW_CMD("ROLE", redis_empty_cmd, redis_read_variant_reply);
}

/*
 * Introspection stuff
 */

/* {{{ proto Redis::IsConnected */
PHP_METHOD(Redis, isConnected) {
    zval *object;
    RedisSock *redis_sock;

    /* Grab our object */
    if (zend_parse_method_parameters(ZEND_NUM_ARGS(), getThis(), "O", &object, redis_ce) == FAILURE) {
        RETURN_FALSE;
    }

    /* Grab socket */
    if ((redis_sock = redis_sock_get_instance(object, 1)) == NULL) {
        RETURN_FALSE;
    }

    RETURN_BOOL(redis_sock->status >= REDIS_SOCK_STATUS_CONNECTED);
}

/* {{{ proto Redis::getHost() */
PHP_METHOD(Redis, getHost) {
    RedisSock *redis_sock;

    if((redis_sock = redis_sock_get_connected(INTERNAL_FUNCTION_PARAM_PASSTHRU))) {
        RETURN_STRINGL(ZSTR_VAL(redis_sock->host), ZSTR_LEN(redis_sock->host));
    } else {
        RETURN_FALSE;
    }
}

/* {{{ proto Redis::getPort() */
PHP_METHOD(Redis, getPort) {
    RedisSock *redis_sock;

    if((redis_sock = redis_sock_get_connected(INTERNAL_FUNCTION_PARAM_PASSTHRU))) {
        /* Return our port */
        RETURN_LONG(redis_sock->port);
    } else {
        RETURN_FALSE;
    }
}

/* {{{ proto Redis::getDBNum */
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

/* {{{ proto Redis::getTimeout */
PHP_METHOD(Redis, getTimeout) {
    RedisSock *redis_sock;

    if((redis_sock = redis_sock_get_connected(INTERNAL_FUNCTION_PARAM_PASSTHRU))) {
        RETURN_DOUBLE(redis_sock->timeout);
    } else {
        RETURN_FALSE;
    }
}

/* {{{ proto Redis::getReadTimeout */
PHP_METHOD(Redis, getReadTimeout) {
    RedisSock *redis_sock;

    if((redis_sock = redis_sock_get_connected(INTERNAL_FUNCTION_PARAM_PASSTHRU))) {
        RETURN_DOUBLE(redis_sock->read_timeout);
    } else {
        RETURN_FALSE;
    }
}

/* {{{ proto Redis::getPersistentID */
PHP_METHOD(Redis, getPersistentID) {
    RedisSock *redis_sock;

    if ((redis_sock = redis_sock_get_connected(INTERNAL_FUNCTION_PARAM_PASSTHRU)) == NULL) {
        RETURN_FALSE;
    } else if (redis_sock->persistent_id == NULL) {
        RETURN_NULL();
    }
    RETURN_STRINGL(ZSTR_VAL(redis_sock->persistent_id), ZSTR_LEN(redis_sock->persistent_id));
}

/* {{{ proto Redis::getAuth */
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

/* {{{ proto mixed Redis::client(string $command, [ $arg1 ... $argN]) */
PHP_METHOD(Redis, client) {
    REDIS_PROCESS_CMD(client, redis_client_response);
}
/* }}} */

/* {{{ proto mixed Redis::rawcommand(string $command, [ $arg1 ... $argN]) */
PHP_METHOD(Redis, rawcommand) {
    int argc = ZEND_NUM_ARGS(), cmd_len;
    char *cmd = NULL;
    RedisSock *redis_sock;
    zval *z_args;

    /* Sanity check on arguments */
    if (argc < 1) {
        php_error_docref(NULL, E_WARNING,
            "Must pass at least one command keyword");
        RETURN_FALSE;
    }
    z_args = emalloc(argc * sizeof(zval));
    if (zend_get_parameters_array(ht, argc, z_args) == FAILURE) {
        php_error_docref(NULL, E_WARNING,
            "Internal PHP error parsing arguments");
        efree(z_args);
        RETURN_FALSE;
    } else if (redis_build_raw_cmd(z_args, argc, &cmd, &cmd_len) < 0 ||
               (redis_sock = redis_sock_get(getThis(), 0)) == NULL
    ) {
        if (cmd) efree(cmd);
        efree(z_args);
        RETURN_FALSE;
    }

    /* Clean up command array */
    efree(z_args);

    /* Execute our command */
    REDIS_PROCESS_REQUEST(redis_sock, cmd, cmd_len);
    if (IS_ATOMIC(redis_sock)) {
        redis_read_raw_variant_reply(INTERNAL_FUNCTION_PARAM_PASSTHRU,redis_sock,NULL,NULL);
    }
    REDIS_PROCESS_RESPONSE(redis_read_variant_reply);
}
/* }}} */

/* {{{ proto array Redis::command()
 *     proto array Redis::command('info', string cmd)
 *     proto array Redis::command('getkeys', array cmd_args) */
PHP_METHOD(Redis, command) {
    REDIS_PROCESS_CMD(command, redis_command_response);
}
/* }}} */

/* {{{ proto array Redis::copy(string $source, string $destination, array $options = null) */
PHP_METHOD(Redis, copy) {
    REDIS_PROCESS_CMD(copy, redis_1_response)
}
/* }}} */

/* Helper to format any combination of SCAN arguments */
PHP_REDIS_API int
redis_build_scan_cmd(char **cmd, REDIS_SCAN_TYPE type, char *key, int key_len,
                     long iter, char *pattern, int pattern_len, int count,
                     zend_string *match_type)
{
    smart_string cmdstr = {0};
    char *keyword;
    int argc;

    /* Count our arguments +1 for key if it's got one, and + 2 for pattern */
    /* or count given that they each carry keywords with them. */
    argc = 1 + (key_len > 0) + (pattern_len > 0 ? 2 : 0) + (count > 0 ? 2 : 0) + (match_type ? 2 : 0);

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
    redis_cmd_init_sstr(&cmdstr, argc, keyword, strlen(keyword));
    if (key_len) redis_cmd_append_sstr(&cmdstr, key, key_len);
    redis_cmd_append_sstr_long(&cmdstr, iter);

    /* Append COUNT if we've got it */
    if(count) {
        REDIS_CMD_APPEND_SSTR_STATIC(&cmdstr, "COUNT");
        redis_cmd_append_sstr_int(&cmdstr, count);
    }

    /* Append MATCH if we've got it */
    if(pattern_len) {
        REDIS_CMD_APPEND_SSTR_STATIC(&cmdstr, "MATCH");
        redis_cmd_append_sstr(&cmdstr, pattern, pattern_len);
    }

    if (match_type) {
        REDIS_CMD_APPEND_SSTR_STATIC(&cmdstr, "TYPE");
        redis_cmd_append_sstr(&cmdstr, ZSTR_VAL(match_type), ZSTR_LEN(match_type));
    }

    /* Return our command length */
    *cmd = cmdstr.c;
    return cmdstr.len;
}

/* {{{ proto redis::scan(&$iterator, [pattern, [count, [type]]]) */
PHP_REDIS_API void
generic_scan_cmd(INTERNAL_FUNCTION_PARAMETERS, REDIS_SCAN_TYPE type) {
    zval *object, *z_iter;
    RedisSock *redis_sock;
    HashTable *hash;
    char *pattern = NULL, *cmd, *key = NULL;
    int cmd_len, num_elements, key_free = 0, pattern_free = 0;
    size_t key_len = 0, pattern_len = 0;
    zend_string *match_type = NULL;
    zend_long count = 0, iter;

    /* Different prototype depending on if this is a key based scan */
    if(type != TYPE_SCAN) {
        // Requires a key
        if(zend_parse_method_parameters(ZEND_NUM_ARGS(), getThis(),
                                        "Os!z/|s!l", &object, redis_ce, &key,
                                        &key_len, &z_iter, &pattern,
                                        &pattern_len, &count)==FAILURE)
        {
            RETURN_FALSE;
        }
    } else {
        // Doesn't require a key
        if(zend_parse_method_parameters(ZEND_NUM_ARGS(), getThis(),
                                        "Oz/|s!lS!", &object, redis_ce, &z_iter,
                                        &pattern, &pattern_len, &count, &match_type)
                                        == FAILURE)
        {
            RETURN_FALSE;
        }
    }

    /* Grab our socket */
    if ((redis_sock = redis_sock_get(object, 0)) == NULL) {
        RETURN_FALSE;
    }

    /* Calling this in a pipeline makes no sense */
    if (!IS_ATOMIC(redis_sock)) {
        php_error_docref(NULL, E_ERROR,
            "Can't call SCAN commands in multi or pipeline mode!");
        RETURN_FALSE;
    }

    // The iterator should be passed in as NULL for the first iteration, but we
    // can treat any NON LONG value as NULL for these purposes as we've
    // separated the variable anyway.
    if(Z_TYPE_P(z_iter) != IS_LONG || Z_LVAL_P(z_iter) < 0) {
        /* Convert to long */
        convert_to_long(z_iter);
        iter = 0;
    } else if(Z_LVAL_P(z_iter) != 0) {
        /* Update our iterator value for the next passthru */
        iter = Z_LVAL_P(z_iter);
    } else {
        /* We're done, back to iterator zero */
        RETURN_FALSE;
    }

    /* Prefix our key if we've got one and we have a prefix set */
    if(key_len) {
        key_free = redis_key_prefix(redis_sock, &key, &key_len);
    }

    if (redis_sock->scan & REDIS_SCAN_PREFIX) {
        pattern_free = redis_key_prefix(redis_sock, &pattern, &pattern_len);
    }

    /**
     * Redis can return to us empty keys, especially in the case where there
     * are a large number of keys to scan, and we're matching against a
     * pattern.  phpredis can be set up to abstract this from the user, by
     * setting OPT_SCAN to REDIS_SCAN_RETRY.  Otherwise we will return empty
     * keys and the user will need to make subsequent calls with an updated
     * iterator.
     */
    do {
        /* Free our previous reply if we're back in the loop.  We know we are
         * if our return_value is an array */
        if (Z_TYPE_P(return_value) == IS_ARRAY) {
            zval_dtor(return_value);
            ZVAL_NULL(return_value);
        }

        // Format our SCAN command
        cmd_len = redis_build_scan_cmd(&cmd, type, key, key_len, (long)iter,
                                   pattern, pattern_len, count, match_type);

        /* Execute our command getting our new iterator value */
        REDIS_PROCESS_REQUEST(redis_sock, cmd, cmd_len);
        if(redis_sock_read_scan_reply(INTERNAL_FUNCTION_PARAM_PASSTHRU,
                                      redis_sock,type,&iter) < 0)
        {
            if(key_free) efree(key);
            RETURN_FALSE;
        }

        /* Get the number of elements */
        hash = Z_ARRVAL_P(return_value);
        num_elements = zend_hash_num_elements(hash);
    } while (redis_sock->scan & REDIS_SCAN_RETRY && iter != 0 &&
            num_elements == 0);

    /* Free our pattern if it was prefixed */
    if (pattern_free) efree(pattern);

    /* Free our key if it was prefixed */
    if(key_free) efree(key);

    /* Update our iterator reference */
    Z_LVAL_P(z_iter) = iter;
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

/*
 * HyperLogLog based commands
 */

/* {{{ proto Redis::pfAdd(string key, array elements) }}} */
PHP_METHOD(Redis, pfadd) {
    REDIS_PROCESS_CMD(pfadd, redis_long_response);
}

/* {{{ proto Redis::pfCount(string key) }}}*/
PHP_METHOD(Redis, pfcount) {
    REDIS_PROCESS_CMD(pfcount, redis_long_response);
}

/* {{{ proto Redis::pfMerge(string dstkey, array keys) }}}*/
PHP_METHOD(Redis, pfmerge) {
    REDIS_PROCESS_CMD(pfmerge, redis_boolean_response);
}

/*
 * Geo commands
 */

PHP_METHOD(Redis, geoadd) {
    REDIS_PROCESS_CMD(geoadd, redis_long_response);
}

PHP_METHOD(Redis, geohash) {
    REDIS_PROCESS_KW_CMD("GEOHASH", redis_key_varval_cmd, redis_mbulk_reply_raw);
}

PHP_METHOD(Redis, geopos) {
    REDIS_PROCESS_KW_CMD("GEOPOS", redis_key_varval_cmd, redis_read_variant_reply);
}

PHP_METHOD(Redis, geodist) {
    REDIS_PROCESS_CMD(geodist, redis_bulk_double_response);
}

PHP_METHOD(Redis, georadius) {
    REDIS_PROCESS_KW_CMD("GEORADIUS", redis_georadius_cmd, redis_read_variant_reply);
}

PHP_METHOD(Redis, georadius_ro) {
    REDIS_PROCESS_KW_CMD("GEORADIUS_RO", redis_georadius_cmd, redis_read_variant_reply);
}

PHP_METHOD(Redis, georadiusbymember) {
    REDIS_PROCESS_KW_CMD("GEORADIUSBYMEMBER", redis_georadiusbymember_cmd, redis_read_variant_reply);
}

PHP_METHOD(Redis, georadiusbymember_ro) {
    REDIS_PROCESS_KW_CMD("GEORADIUSBYMEMBER_RO", redis_georadiusbymember_cmd, redis_read_variant_reply);
}

PHP_METHOD(Redis, geosearch) {
    REDIS_PROCESS_CMD(geosearch, redis_geosearch_response);
}

PHP_METHOD(Redis, geosearchstore) {
    REDIS_PROCESS_CMD(geosearchstore, redis_long_response);
}

/*
 * Streams
 */

PHP_METHOD(Redis, xack) {
    REDIS_PROCESS_CMD(xack, redis_long_response);
}

PHP_METHOD(Redis, xadd) {
    REDIS_PROCESS_CMD(xadd, redis_read_variant_reply);
}

PHP_METHOD(Redis, xautoclaim) {
    REDIS_PROCESS_CMD(xautoclaim, redis_xclaim_reply);
}

PHP_METHOD(Redis, xclaim) {
    REDIS_PROCESS_CMD(xclaim, redis_xclaim_reply);
}

PHP_METHOD(Redis, xdel) {
    REDIS_PROCESS_KW_CMD("XDEL", redis_key_str_arr_cmd, redis_long_response);
}

PHP_METHOD(Redis, xgroup) {
    REDIS_PROCESS_CMD(xgroup, redis_read_variant_reply);
}

PHP_METHOD(Redis, xinfo) {
    REDIS_PROCESS_CMD(xinfo, redis_xinfo_reply);
}

PHP_METHOD(Redis, xlen) {
    REDIS_PROCESS_KW_CMD("XLEN", redis_key_cmd, redis_long_response);
}

PHP_METHOD(Redis, xpending) {
    REDIS_PROCESS_CMD(xpending, redis_read_variant_reply_strings);
}

PHP_METHOD(Redis, xrange) {
    REDIS_PROCESS_KW_CMD("XRANGE", redis_xrange_cmd, redis_xrange_reply);
}

PHP_METHOD(Redis, xread) {
    REDIS_PROCESS_CMD(xread, redis_xread_reply);
}

PHP_METHOD(Redis, xreadgroup) {
    REDIS_PROCESS_CMD(xreadgroup, redis_xread_reply);
}

PHP_METHOD(Redis, xrevrange) {
    REDIS_PROCESS_KW_CMD("XREVRANGE", redis_xrange_cmd, redis_xrange_reply);
}

PHP_METHOD(Redis, xtrim) {
    REDIS_PROCESS_CMD(xtrim, redis_long_response);
}

/* vim: set tabstop=4 softtabstop=4 expandtab shiftwidth=4: */
