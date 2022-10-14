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
    int result = FAILURE;
    char *cmd, *resp;
    int resp_len, cmd_len;

    /* format our discard command */
    cmd_len = REDIS_SPPRINTF(&cmd, "DISCARD", "");

    /* send our DISCARD command */
    if (redis_sock_write(redis_sock, cmd, cmd_len) >= 0 &&
       (resp = redis_sock_read(redis_sock,&resp_len)) != NULL)
    {
        /* success if we get OK */
        result = (resp_len == 3 && strncmp(resp,"+OK", 3) == 0) ? SUCCESS:FAILURE;

        /* free our response */
        efree(resp);
    }

    /* free our command */
    efree(cmd);

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
        redis_sock_disconnect(redis->sock, 0);
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

/* Redis and RedisCluster objects share serialization/prefixing settings so
 * this is a generic function to add class constants to either */
static void add_class_constants(zend_class_entry *ce, int is_cluster) {
    zend_declare_class_constant_long(ce, ZEND_STRL("REDIS_NOT_FOUND"), REDIS_NOT_FOUND);
    zend_declare_class_constant_long(ce, ZEND_STRL("REDIS_STRING"), REDIS_STRING);
    zend_declare_class_constant_long(ce, ZEND_STRL("REDIS_SET"), REDIS_SET);
    zend_declare_class_constant_long(ce, ZEND_STRL("REDIS_LIST"), REDIS_LIST);
    zend_declare_class_constant_long(ce, ZEND_STRL("REDIS_ZSET"), REDIS_ZSET);
    zend_declare_class_constant_long(ce, ZEND_STRL("REDIS_HASH"), REDIS_HASH);
    zend_declare_class_constant_long(ce, ZEND_STRL("REDIS_STREAM"), REDIS_STREAM);

    /* Add common mode constants */
    zend_declare_class_constant_long(ce, ZEND_STRL("ATOMIC"), ATOMIC);
    zend_declare_class_constant_long(ce, ZEND_STRL("MULTI"), MULTI);

    /* options */
    zend_declare_class_constant_long(ce, ZEND_STRL("OPT_SERIALIZER"), REDIS_OPT_SERIALIZER);
    zend_declare_class_constant_long(ce, ZEND_STRL("OPT_PREFIX"), REDIS_OPT_PREFIX);
    zend_declare_class_constant_long(ce, ZEND_STRL("OPT_READ_TIMEOUT"), REDIS_OPT_READ_TIMEOUT);
    zend_declare_class_constant_long(ce, ZEND_STRL("OPT_TCP_KEEPALIVE"), REDIS_OPT_TCP_KEEPALIVE);
    zend_declare_class_constant_long(ce, ZEND_STRL("OPT_COMPRESSION"), REDIS_OPT_COMPRESSION);
    zend_declare_class_constant_long(ce, ZEND_STRL("OPT_REPLY_LITERAL"), REDIS_OPT_REPLY_LITERAL);
    zend_declare_class_constant_long(ce, ZEND_STRL("OPT_COMPRESSION_LEVEL"), REDIS_OPT_COMPRESSION_LEVEL);
    zend_declare_class_constant_long(ce, ZEND_STRL("OPT_NULL_MULTIBULK_AS_NULL"), REDIS_OPT_NULL_MBULK_AS_NULL);

    /* serializer */
    zend_declare_class_constant_long(ce, ZEND_STRL("SERIALIZER_NONE"), REDIS_SERIALIZER_NONE);
    zend_declare_class_constant_long(ce, ZEND_STRL("SERIALIZER_PHP"), REDIS_SERIALIZER_PHP);
#ifdef HAVE_REDIS_IGBINARY
    zend_declare_class_constant_long(ce, ZEND_STRL("SERIALIZER_IGBINARY"), REDIS_SERIALIZER_IGBINARY);
#endif
#ifdef HAVE_REDIS_MSGPACK
    zend_declare_class_constant_long(ce, ZEND_STRL("SERIALIZER_MSGPACK"), REDIS_SERIALIZER_MSGPACK);
#endif
    zend_declare_class_constant_long(ce, ZEND_STRL("SERIALIZER_JSON"), REDIS_SERIALIZER_JSON);

    /* compression */
    zend_declare_class_constant_long(ce, ZEND_STRL("COMPRESSION_NONE"), REDIS_COMPRESSION_NONE);
#ifdef HAVE_REDIS_LZF
    zend_declare_class_constant_long(ce, ZEND_STRL("COMPRESSION_LZF"), REDIS_COMPRESSION_LZF);
#endif
#ifdef HAVE_REDIS_ZSTD
    zend_declare_class_constant_long(ce, ZEND_STRL("COMPRESSION_ZSTD"), REDIS_COMPRESSION_ZSTD);
    zend_declare_class_constant_long(ce, ZEND_STRL("COMPRESSION_ZSTD_MIN"), 1);
#ifdef ZSTD_CLEVEL_DEFAULT
    zend_declare_class_constant_long(ce, ZEND_STRL("COMPRESSION_ZSTD_DEFAULT"), ZSTD_CLEVEL_DEFAULT);
#else
    zend_declare_class_constant_long(ce, ZEND_STRL("COMPRESSION_ZSTD_DEFAULT"), 3);
#endif
    zend_declare_class_constant_long(ce, ZEND_STRL("COMPRESSION_ZSTD_MAX"), ZSTD_maxCLevel());
#endif

#ifdef HAVE_REDIS_LZ4
    zend_declare_class_constant_long(ce, ZEND_STRL("COMPRESSION_LZ4"), REDIS_COMPRESSION_LZ4);
#endif

    /* scan options*/
    zend_declare_class_constant_long(ce, ZEND_STRL("OPT_SCAN"), REDIS_OPT_SCAN);
    zend_declare_class_constant_long(ce, ZEND_STRL("SCAN_RETRY"), REDIS_SCAN_RETRY);
    zend_declare_class_constant_long(ce, ZEND_STRL("SCAN_NORETRY"), REDIS_SCAN_NORETRY);
    zend_declare_class_constant_long(ce, ZEND_STRL("SCAN_PREFIX"), REDIS_SCAN_PREFIX);
    zend_declare_class_constant_long(ce, ZEND_STRL("SCAN_NOPREFIX"), REDIS_SCAN_NOPREFIX);

    zend_declare_class_constant_stringl(ce, "AFTER", 5, "after", 5);
    zend_declare_class_constant_stringl(ce, "BEFORE", 6, "before", 6);

    if (is_cluster) {
        /* Cluster option to allow for slave failover */
        zend_declare_class_constant_long(ce, ZEND_STRL("OPT_SLAVE_FAILOVER"), REDIS_OPT_FAILOVER);
        zend_declare_class_constant_long(ce, ZEND_STRL("FAILOVER_NONE"), REDIS_FAILOVER_NONE);
        zend_declare_class_constant_long(ce, ZEND_STRL("FAILOVER_ERROR"), REDIS_FAILOVER_ERROR);
        zend_declare_class_constant_long(ce, ZEND_STRL("FAILOVER_DISTRIBUTE"), REDIS_FAILOVER_DISTRIBUTE);
        zend_declare_class_constant_long(ce, ZEND_STRL("FAILOVER_DISTRIBUTE_SLAVES"), REDIS_FAILOVER_DISTRIBUTE_SLAVES);
    } else {
        /* Cluster doesn't support pipelining at this time */
        zend_declare_class_constant_long(ce, ZEND_STRL("PIPELINE"), PIPELINE);

        zend_declare_class_constant_stringl(ce, ZEND_STRL("LEFT"), ZEND_STRL("left"));
        zend_declare_class_constant_stringl(ce, ZEND_STRL("RIGHT"), ZEND_STRL("right"));
    }

    /* retry/backoff options*/
    zend_declare_class_constant_long(ce, ZEND_STRL("OPT_MAX_RETRIES"), REDIS_OPT_MAX_RETRIES);

    zend_declare_class_constant_long(ce, ZEND_STRL("OPT_BACKOFF_ALGORITHM"), REDIS_OPT_BACKOFF_ALGORITHM);
    zend_declare_class_constant_long(ce, ZEND_STRL("BACKOFF_ALGORITHM_DEFAULT"), REDIS_BACKOFF_ALGORITHM_DEFAULT);
    zend_declare_class_constant_long(ce, ZEND_STRL("BACKOFF_ALGORITHM_CONSTANT"), REDIS_BACKOFF_ALGORITHM_CONSTANT);
    zend_declare_class_constant_long(ce, ZEND_STRL("BACKOFF_ALGORITHM_UNIFORM"), REDIS_BACKOFF_ALGORITHM_UNIFORM);
    zend_declare_class_constant_long(ce, ZEND_STRL("BACKOFF_ALGORITHM_EXPONENTIAL"), REDIS_BACKOFF_ALGORITHM_EXPONENTIAL);
    zend_declare_class_constant_long(ce, ZEND_STRL("BACKOFF_ALGORITHM_FULL_JITTER"), REDIS_BACKOFF_ALGORITHM_FULL_JITTER);
    zend_declare_class_constant_long(ce, ZEND_STRL("BACKOFF_ALGORITHM_EQUAL_JITTER"), REDIS_BACKOFF_ALGORITHM_EQUAL_JITTER);
    zend_declare_class_constant_long(ce, ZEND_STRL("BACKOFF_ALGORITHM_DECORRELATED_JITTER"), REDIS_BACKOFF_ALGORITHM_DECORRELATED_JITTER);

    zend_declare_class_constant_long(ce, ZEND_STRL("OPT_BACKOFF_BASE"), REDIS_OPT_BACKOFF_BASE);

    zend_declare_class_constant_long(ce, ZEND_STRL("OPT_BACKOFF_CAP"), REDIS_OPT_BACKOFF_CAP);
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

    /* Add shared class constants to Redis and RedisCluster objects */
    add_class_constants(redis_ce, 0);
    add_class_constants(redis_cluster_ce, 1);

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
    redis_object *redis;
    zend_string *zkey;
    zval *val, *opts = NULL;

    if (zend_parse_parameters(ZEND_NUM_ARGS(), "|a", &opts) == FAILURE) {
        RETURN_THROWS();
    }

    if (opts != NULL) {
        redis = PHPREDIS_ZVAL_GET_OBJECT(redis_object, getThis());
        redis->sock = redis_sock_create("127.0.0.1", sizeof("127.0.0.1") - 1, 6379, 0, 0, 0, NULL, 0);

        ZEND_HASH_FOREACH_STR_KEY_VAL(Z_ARRVAL_P(opts), zkey, val) {
            if (zkey == NULL) {
                continue;
            }
            ZVAL_DEREF(val);
            if (zend_string_equals_literal_ci(zkey, "host")) {
                if (Z_TYPE_P(val) != IS_STRING) {
                    REDIS_VALUE_EXCEPTION("Invalid host");
                    RETURN_THROWS();
                }
                zend_string_release(redis->sock->host);
                redis->sock->host = zval_get_string(val);
            } else if (zend_string_equals_literal_ci(zkey, "port")) {
                if (Z_TYPE_P(val) != IS_LONG) {
                    REDIS_VALUE_EXCEPTION("Invalid port");
                    RETURN_THROWS();
                }
                redis->sock->port = zval_get_long(val);
            } else if (zend_string_equals_literal_ci(zkey, "connectTimeout")) {
                if (Z_TYPE_P(val) != IS_LONG && Z_TYPE_P(val) != IS_DOUBLE) {
                    REDIS_VALUE_EXCEPTION("Invalid connect timeout");
                    RETURN_THROWS();
                }
                redis->sock->timeout = zval_get_double(val);
            } else if (zend_string_equals_literal_ci(zkey, "readTimeout")) {
                if (Z_TYPE_P(val) != IS_LONG && Z_TYPE_P(val) != IS_DOUBLE) {
                    REDIS_VALUE_EXCEPTION("Invalid read timeout");
                    RETURN_THROWS();
                }
                redis->sock->read_timeout = zval_get_double(val);
            } else if (zend_string_equals_literal_ci(zkey, "persistent")) {
                if (Z_TYPE_P(val) == IS_STRING) {
                    if (redis->sock->persistent_id) zend_string_release(redis->sock->persistent_id);
                    redis->sock->persistent_id = zval_get_string(val);
                    redis->sock->persistent = 1;
                } else {
                    redis->sock->persistent = zval_is_true(val);
                }
            } else if (zend_string_equals_literal_ci(zkey, "retryInterval")) {
                if (Z_TYPE_P(val) != IS_LONG && Z_TYPE_P(val) != IS_DOUBLE) {
                    REDIS_VALUE_EXCEPTION("Invalid retry interval");
                    RETURN_THROWS();
                }
                redis->sock->retry_interval = zval_get_long(val);
            } else if (zend_string_equals_literal_ci(zkey, "ssl")) {
                if (redis_sock_set_stream_context(redis->sock, val) != SUCCESS) {
                    REDIS_VALUE_EXCEPTION("Invalid SSL context options");
                    RETURN_THROWS();
                }
            } else if (zend_string_equals_literal_ci(zkey, "auth")) {
                if (Z_TYPE_P(val) != IS_STRING && Z_TYPE_P(val) != IS_ARRAY) {
                    REDIS_VALUE_EXCEPTION("Invalid auth credentials");
                    RETURN_THROWS();
                }
                redis_sock_set_auth_zval(redis->sock, val);
            } else if (zend_string_equals_literal_ci(zkey, "backoff")) {
                if (redis_sock_set_backoff(redis->sock, val) != SUCCESS) {
                    REDIS_VALUE_EXCEPTION("Invalid backoff options");
                    RETURN_THROWS();
                }
            } else {
                php_error_docref(NULL, E_WARNING, "Skip unknown option '%s'", ZSTR_VAL(zkey));
            }
        } ZEND_HASH_FOREACH_END();
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
        redis_sock_disconnect(redis->sock, 0);
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

    if (redis_sock_server_open(redis->sock) < 0) {
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

    if (redis_sock_disconnect(redis_sock, 1) == SUCCESS) {
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
PHP_METHOD(Redis, mget)
{
    zval *object, *z_args, *z_ele;
    HashTable *hash;
    RedisSock *redis_sock;
    smart_string cmd = {0};
    int arg_count;

    /* Make sure we have proper arguments */
    if(zend_parse_method_parameters(ZEND_NUM_ARGS(), getThis(), "Oa",
                                    &object, redis_ce, &z_args) == FAILURE) {
        RETURN_FALSE;
    }

    /* We'll need the socket */
    if ((redis_sock = redis_sock_get(object, 0)) == NULL) {
        RETURN_FALSE;
    }

    /* Grab our array */
    hash = Z_ARRVAL_P(z_args);

    /* We don't need to do anything if there aren't any keys */
    if((arg_count = zend_hash_num_elements(hash)) == 0) {
        RETURN_FALSE;
    }

    /* Build our command header */
    redis_cmd_init_sstr(&cmd, arg_count, "MGET", 4);

    /* Iterate through and grab our keys */
    ZEND_HASH_FOREACH_VAL(hash, z_ele) {
        zend_string *zstr = zval_get_string(z_ele);
        redis_cmd_append_sstr_key(&cmd, ZSTR_VAL(zstr), ZSTR_LEN(zstr), redis_sock, NULL);
        zend_string_release(zstr);
    } ZEND_HASH_FOREACH_END();

    /* Kick off our command */
    REDIS_PROCESS_REQUEST(redis_sock, cmd.c, cmd.len);
    if (IS_ATOMIC(redis_sock)) {
        if(redis_sock_read_multibulk_reply(INTERNAL_FUNCTION_PARAM_PASSTHRU,
                                           redis_sock, NULL, NULL) < 0) {
            RETURN_FALSE;
        }
    }
    REDIS_PROCESS_RESPONSE(redis_sock_read_multibulk_reply);
}

/* {{{ proto boolean Redis::exists(string key)
 */
PHP_METHOD(Redis, exists)
{
    REDIS_PROCESS_CMD(exists, redis_long_response);
}
/* }}} */

/* {{{ proto boolean Redis::del(string key)
 */
PHP_METHOD(Redis, del)
{
    REDIS_PROCESS_CMD(del, redis_long_response);
}
/* }}} */

/* {{{ proto long Redis::unlink(string $key1, string $key2 [, string $key3...]) }}}
 * {{{ proto long Redis::unlink(array $keys) */
PHP_METHOD(Redis, unlink)
{
    REDIS_PROCESS_CMD(unlink, redis_long_response);
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
PHP_METHOD(Redis, watch)
{
    REDIS_PROCESS_CMD(watch, redis_watch_response);
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
    RedisSock *redis_sock;
    FailableResultCallback cb;
    zval *zargs;
    zend_string *op;
    char *cmd;
    int cmdlen, argc = ZEND_NUM_ARGS();

    if (argc < 1 || (redis_sock = redis_sock_get(getThis(), 0)) == NULL) {
        if (argc < 1) {
            php_error_docref(NULL, E_WARNING, "ACL command requires at least one argument");
        }
        RETURN_FALSE;
    }

    zargs = emalloc(argc * sizeof(*zargs));
    if (zend_get_parameters_array(ht, argc, zargs) == FAILURE) {
        efree(zargs);
        RETURN_FALSE;
    }

    /* Read the subcommand and set response callback */
    op = zval_get_string(&zargs[0]);
    if (zend_string_equals_literal_ci(op, "GETUSER")) {
        cb = redis_acl_getuser_reply;
    } else if (zend_string_equals_literal_ci(op, "LOG")) {
        cb = redis_acl_log_reply;
    } else {
        cb = redis_read_variant_reply;
    }

    /* Make our command and free args */
    cmd = redis_variadic_str_cmd("ACL", zargs, argc, &cmdlen);

    zend_string_release(op);
    efree(zargs);

    REDIS_PROCESS_REQUEST(redis_sock, cmd, cmdlen);
    if (IS_ATOMIC(redis_sock)) {
        if (cb(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock, NULL, NULL) < 0) {
            RETURN_FALSE;
        }
    }
    REDIS_PROCESS_RESPONSE(cb);
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

/* {{{ proto string Redis::lMove(string source, string destination, string wherefrom, string whereto) */
PHP_METHOD(Redis, lMove)
{
    REDIS_PROCESS_CMD(lmove, redis_string_response);
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
    char *cmd;
    int cmd_len;
    short have_count;
    RedisSock *redis_sock;

    // Grab our socket, validate call
    if ((redis_sock = redis_sock_get(getThis(), 0)) == NULL ||
       redis_srandmember_cmd(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock,
                             &cmd, &cmd_len, NULL, NULL, &have_count) == FAILURE)
    {
        RETURN_FALSE;
    }

    REDIS_PROCESS_REQUEST(redis_sock, cmd, cmd_len);
    if(have_count) {
        if (IS_ATOMIC(redis_sock)) {
            if(redis_sock_read_multibulk_reply(INTERNAL_FUNCTION_PARAM_PASSTHRU,
                                               redis_sock, NULL, NULL) < 0)
            {
                RETURN_FALSE;
            }
        }
        REDIS_PROCESS_RESPONSE(redis_sock_read_multibulk_reply);
    } else {
        if (IS_ATOMIC(redis_sock)) {
            redis_string_response(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock,
                NULL, NULL);
        }
        REDIS_PROCESS_RESPONSE(redis_string_response);
    }
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
    REDIS_PROCESS_CMD(sinter, redis_sock_read_multibulk_reply);
}
/* }}} */

PHP_METHOD(Redis, sintercard) {
    REDIS_PROCESS_KW_CMD("SINTERCARD", redis_intercard_cmd, redis_long_response);
}

/* {{{ proto array Redis::sInterStore(string dst, string key0,...string keyN) */
PHP_METHOD(Redis, sInterStore) {
    REDIS_PROCESS_CMD(sinterstore, redis_long_response);
}
/* }}} */

/* {{{ proto array Redis::sUnion(string key0, ... string keyN) */
PHP_METHOD(Redis, sUnion) {
    REDIS_PROCESS_CMD(sunion, redis_sock_read_multibulk_reply);
}
/* }}} */

/* {{{ proto array Redis::sUnionStore(string dst, string key0, ... keyN) */
PHP_METHOD(Redis, sUnionStore) {
    REDIS_PROCESS_CMD(sunionstore, redis_long_response);
}
/* }}} */

/* {{{ proto array Redis::sDiff(string key0, ... string keyN) */
PHP_METHOD(Redis, sDiff) {
    REDIS_PROCESS_CMD(sdiff, redis_sock_read_multibulk_reply);
}
/* }}} */

/* {{{ proto array Redis::sDiffStore(string dst, string key0, ... keyN) */
PHP_METHOD(Redis, sDiffStore) {
    REDIS_PROCESS_CMD(sdiffstore, redis_long_response);
}
/* }}} */


/* {{{ proto array Redis::sort(string key, array options) */
PHP_METHOD(Redis, sort) {
    char *cmd;
    int cmd_len, have_store;
    RedisSock *redis_sock;

    // Grab socket, handle command construction
    if ((redis_sock = redis_sock_get(getThis(), 0)) == NULL ||
       redis_sort_cmd(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock, &have_store,
                      &cmd, &cmd_len, NULL, NULL) == FAILURE)
    {
        RETURN_FALSE;
    }

    REDIS_PROCESS_REQUEST(redis_sock, cmd, cmd_len);
    if (IS_ATOMIC(redis_sock)) {
        if (redis_read_variant_reply(INTERNAL_FUNCTION_PARAM_PASSTHRU,
                                     redis_sock, NULL, NULL) < 0)
        {
            RETURN_FALSE;
        }
    }
    REDIS_PROCESS_RESPONSE(redis_read_variant_reply);
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
    smart_string cmdstr = {0};
    RedisSock *redis_sock;
    zend_string *section;
    zval *args = NULL;
    int i, argc = 0;

    ZEND_PARSE_PARAMETERS_START(0, -1)
        Z_PARAM_VARIADIC('+', args, argc)
    ZEND_PARSE_PARAMETERS_END();

    if ((redis_sock = redis_sock_get(getThis(), 0)) == NULL)
        RETURN_FALSE;

    REDIS_CMD_INIT_SSTR_STATIC(&cmdstr, argc, "INFO");
    for (i = 0; i < argc; i++) {
        section = zval_get_string(&args[i]);
        redis_cmd_append_sstr_zstr(&cmdstr, section);
        zend_string_release(section);
    }

    REDIS_PROCESS_REQUEST(redis_sock, cmdstr.c, cmdstr.len);
    if (IS_ATOMIC(redis_sock)) {
        redis_info_response(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock, NULL,
            NULL);
    }
    REDIS_PROCESS_RESPONSE(redis_info_response);
}
/* }}} */

/* {{{ proto bool Redis::select(long dbNumber) */
PHP_METHOD(Redis, select) {

    zval *object;
    RedisSock *redis_sock;

    char *cmd;
    int cmd_len;
    zend_long dbNumber;

    if (zend_parse_method_parameters(ZEND_NUM_ARGS(), getThis(), "Ol",
                                     &object, redis_ce, &dbNumber) == FAILURE) {
        RETURN_FALSE;
    }

    if (dbNumber < 0 || (redis_sock = redis_sock_get(object, 0)) == NULL) {
        RETURN_FALSE;
    }

    redis_sock->dbNumber = dbNumber;
    cmd_len = REDIS_SPPRINTF(&cmd, "SELECT", "d", dbNumber);

    REDIS_PROCESS_REQUEST(redis_sock, cmd, cmd_len);
    if (IS_ATOMIC(redis_sock)) {
        redis_boolean_response(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock,
            NULL, NULL);
    }
    REDIS_PROCESS_RESPONSE(redis_boolean_response);
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

static
void generic_mset(INTERNAL_FUNCTION_PARAMETERS, char *kw, FailableResultCallback fun)
{
    RedisSock *redis_sock;
    smart_string cmd = {0};
    zval *object, *z_array;
    HashTable *htargs;
    zend_string *zkey;
    zval *zmem;
    char buf[64];
    size_t keylen;
    zend_ulong idx;

    if (zend_parse_method_parameters(ZEND_NUM_ARGS(), getThis(), "Oa",
                                     &object, redis_ce, &z_array) == FAILURE)
    {
        RETURN_FALSE;
    }

    /* Make sure we can get our socket, and we were not passed an empty array */
    if ((redis_sock = redis_sock_get(object, 0)) == NULL ||
        zend_hash_num_elements(Z_ARRVAL_P(z_array)) == 0)
    {
        RETURN_FALSE;
    }

    /* Initialize our command */
    htargs = Z_ARRVAL_P(z_array);
    redis_cmd_init_sstr(&cmd, zend_hash_num_elements(htargs) * 2, kw, strlen(kw));

    ZEND_HASH_FOREACH_KEY_VAL(htargs, idx, zkey,  zmem) {
        /* Handle string or numeric keys */
        if (zkey) {
            redis_cmd_append_sstr_key(&cmd, ZSTR_VAL(zkey), ZSTR_LEN(zkey), redis_sock, NULL);
        } else {
            keylen = snprintf(buf, sizeof(buf), "%ld", (long)idx);
            redis_cmd_append_sstr_key(&cmd, buf, keylen, redis_sock, NULL);
        }

        /* Append our value */
        redis_cmd_append_sstr_zval(&cmd, zmem, redis_sock);
    } ZEND_HASH_FOREACH_END();

    REDIS_PROCESS_REQUEST(redis_sock, cmd.c, cmd.len);
    if (IS_ATOMIC(redis_sock)) {
        fun(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock, NULL, NULL);
    }

    REDIS_PROCESS_RESPONSE(fun);
}

/* {{{ proto bool Redis::mset(array (key => value, ...)) */
PHP_METHOD(Redis, mset) {
    generic_mset(INTERNAL_FUNCTION_PARAM_PASSTHRU, "MSET", redis_boolean_response);
}
/* }}} */


/* {{{ proto bool Redis::msetnx(array (key => value, ...)) */
PHP_METHOD(Redis, msetnx) {
    generic_mset(INTERNAL_FUNCTION_PARAM_PASSTHRU, "MSETNX", redis_1_response);
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
    REDIS_PROCESS_CMD(zadd, redis_long_response);
}
/* }}} */

/* Handle ZRANGE and ZREVRANGE as they're the same except for keyword */
static void generic_zrange_cmd(INTERNAL_FUNCTION_PARAMETERS, char *kw,
                               zrange_cb fun)
{
    char *cmd;
    int cmd_len;
    RedisSock *redis_sock;
    int withscores = 0;

    if ((redis_sock = redis_sock_get(getThis(), 0)) == NULL) {
        RETURN_FALSE;
    }

    if(fun(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock, kw, &cmd,
           &cmd_len, &withscores, NULL, NULL) == FAILURE)
    {
        RETURN_FALSE;
    }

    REDIS_PROCESS_REQUEST(redis_sock, cmd, cmd_len);
    if(withscores) {
        if (IS_ATOMIC(redis_sock)) {
            redis_mbulk_reply_zipped_keys_dbl(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock, NULL, NULL);
        }
        REDIS_PROCESS_RESPONSE(redis_mbulk_reply_zipped_keys_dbl);
    } else {
        if (IS_ATOMIC(redis_sock)) {
            if(redis_sock_read_multibulk_reply(INTERNAL_FUNCTION_PARAM_PASSTHRU,
                                               redis_sock, NULL, NULL) < 0)
            {
                RETURN_FALSE;
            }
        }
        REDIS_PROCESS_RESPONSE(redis_sock_read_multibulk_reply);
    }
}

/* {{{ proto array Redis::zRandMember(string key, array options) */
PHP_METHOD(Redis, zRandMember)
{
    REDIS_PROCESS_CMD(zrandmember, redis_zrandmember_response);
}
/* }}} */

/* {{{ proto array Redis::zRange(string key,int start,int end,bool scores = 0) */
PHP_METHOD(Redis, zRange)
{
    generic_zrange_cmd(INTERNAL_FUNCTION_PARAM_PASSTHRU, "ZRANGE",
        redis_zrange_cmd);
}
/* }}} */

/* {{{ proto array Redis::zRevRange(string k, long s, long e, bool scores = 0) */
PHP_METHOD(Redis, zRevRange) {
    generic_zrange_cmd(INTERNAL_FUNCTION_PARAM_PASSTHRU, "ZREVRANGE",
        redis_zrange_cmd);
}
/* }}} */

/* {{{ proto array Redis::zRangeByScore(string k,string s,string e,array opt) */
PHP_METHOD(Redis, zRangeByScore) {
    generic_zrange_cmd(INTERNAL_FUNCTION_PARAM_PASSTHRU, "ZRANGEBYSCORE",
        redis_zrangebyscore_cmd);
}
/* }}} */

/* {{{ proto array Redis::zRevRangeByScore(string key, string start, string end,
 *                                         array options) */
PHP_METHOD(Redis, zRevRangeByScore) {
    generic_zrange_cmd(INTERNAL_FUNCTION_PARAM_PASSTHRU, "ZREVRANGEBYSCORE",
        redis_zrangebyscore_cmd);
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
    char *resp, *cmd;
    int resp_len, cmd_len;
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
            cmd_len = REDIS_SPPRINTF(&cmd, "MULTI", "");
            if (IS_PIPELINE(redis_sock)) {
                PIPELINE_ENQUEUE_COMMAND(cmd, cmd_len);
                efree(cmd);
                REDIS_SAVE_CALLBACK(NULL, NULL);
                REDIS_ENABLE_MODE(redis_sock, MULTI);
            } else {
                SOCKET_WRITE_COMMAND(redis_sock, cmd, cmd_len)
                efree(cmd);
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

/* redis_sock_read_multibulk_multi_reply */
PHP_REDIS_API int redis_sock_read_multibulk_multi_reply(INTERNAL_FUNCTION_PARAMETERS,
                                      RedisSock *redis_sock)
{

    char inbuf[4096];
    int numElems;
    size_t len;

    if (redis_sock_gets(redis_sock, inbuf, sizeof(inbuf) - 1, &len) < 0) {
        return - 1;
    }

    /* number of responses */
    numElems = atoi(inbuf+1);

    if(numElems < 0) {
        return -1;
    }

    array_init(return_value);

    redis_sock_read_multibulk_multi_reply_loop(INTERNAL_FUNCTION_PARAM_PASSTHRU,
                    redis_sock, return_value, numElems);

    return 0;
}


/* exec */
PHP_METHOD(Redis, exec)
{
    RedisSock *redis_sock;
    char *cmd;
    int cmd_len, ret;
    zval *object;

    if (zend_parse_method_parameters(ZEND_NUM_ARGS(), getThis(),
                                     "O", &object, redis_ce) == FAILURE ||
        (redis_sock = redis_sock_get(object, 0)) == NULL
    ) {
        RETURN_FALSE;
    }

    if (IS_MULTI(redis_sock)) {
        cmd_len = REDIS_SPPRINTF(&cmd, "EXEC", "");
        if (IS_PIPELINE(redis_sock)) {
            PIPELINE_ENQUEUE_COMMAND(cmd, cmd_len);
            efree(cmd);
            REDIS_SAVE_CALLBACK(NULL, NULL);
            REDIS_DISABLE_MODE(redis_sock, MULTI);
            RETURN_ZVAL(getThis(), 1, 0);
        }
        SOCKET_WRITE_COMMAND(redis_sock, cmd, cmd_len)
        efree(cmd);

        ret = redis_sock_read_multibulk_multi_reply(
            INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock);
        free_reply_callbacks(redis_sock);
        REDIS_DISABLE_MODE(redis_sock, MULTI);
        redis_sock->watching = 0;
        if (ret < 0) {
            zval_dtor(return_value);
            RETURN_FALSE;
        }
    }

    if (IS_PIPELINE(redis_sock)) {
        if (redis_sock->pipeline_cmd == NULL) {
            /* Empty array when no command was run. */
            array_init(return_value);
        } else {
            if (redis_sock_write(redis_sock, ZSTR_VAL(redis_sock->pipeline_cmd),
                    ZSTR_LEN(redis_sock->pipeline_cmd)) < 0) {
                ZVAL_FALSE(return_value);
            } else {
                array_init(return_value);
                redis_sock_read_multibulk_multi_reply_loop(
                    INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock, return_value, 0);
            }
            zend_string_release(redis_sock->pipeline_cmd);
            redis_sock->pipeline_cmd = NULL;
        }
        free_reply_callbacks(redis_sock);
        REDIS_DISABLE_MODE(redis_sock, PIPELINE);
    }
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

/* TODO:  Investigate/fix the odd logic going on in here.  Looks like previous abort
 *        conditions that are now simply empty if { } { } blocks. */
PHP_REDIS_API int
redis_sock_read_multibulk_multi_reply_loop(INTERNAL_FUNCTION_PARAMETERS,
                                           RedisSock *redis_sock, zval *z_tab,
                                           int numElems)
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

        if (redis_sock_gets(redis_sock, inbuf, sizeof(inbuf) - 1, &len) < 0) {
        } else if (strncmp(inbuf, "+OK", 3) != 0) {
        }

        while ((fi = fi->next) && fi->fun) {
            if (redis_response_enqueued(redis_sock) == SUCCESS) {
            } else {
            }
        }

        if (redis_sock_gets(redis_sock, inbuf, sizeof(inbuf) - 1, &len) < 0) {
        }

        zval z_ret;
        array_init(&z_ret);
        add_next_index_zval(z_tab, &z_ret);

        int num = atol(inbuf + 1);

        if (num > 0 && redis_read_multibulk_recursive(redis_sock, num, 0, &z_ret) < 0) {
        }

        if (fi) fi = fi->next;
    }
    redis_sock->current = fi;
    return 0;
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

/* {{{ proto void Redis::subscribe(Array(channel1, channel2, ... channelN)) */
PHP_METHOD(Redis, subscribe) {
    REDIS_PROCESS_KW_CMD("SUBSCRIBE", redis_subscribe_cmd,
        redis_subscribe_response);
}

/**
 *  [p]unsubscribe channel_0 channel_1 ... channel_n
 *  [p]unsubscribe(array(channel_0, channel_1, ..., channel_n))
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

/* {{{ proto string Redis::bgrewriteaof() */
PHP_METHOD(Redis, bgrewriteaof)
{
    REDIS_PROCESS_KW_CMD("BGREWRITEAOF", redis_empty_cmd,
        redis_boolean_response);
}
/* }}} */

/* {{{ proto string Redis::slaveof([host, port]) */
PHP_METHOD(Redis, slaveof)
{
    zval *object;
    RedisSock *redis_sock;
    char *cmd = "", *host = NULL;
    size_t host_len;
    zend_long port = 6379;
    int cmd_len;

    if (zend_parse_method_parameters(ZEND_NUM_ARGS(), getThis(),
                                     "O|sl", &object, redis_ce, &host,
                                     &host_len, &port) == FAILURE)
    {
        RETURN_FALSE;
    }
    if (port < 0 || (redis_sock = redis_sock_get(object, 0)) == NULL) {
        RETURN_FALSE;
    }

    if (host && host_len) {
        cmd_len = REDIS_SPPRINTF(&cmd, "SLAVEOF", "sd", host, host_len, (int)port);
    } else {
        cmd_len = REDIS_SPPRINTF(&cmd, "SLAVEOF", "ss", "NO", 2, "ONE", 3);
    }

    REDIS_PROCESS_REQUEST(redis_sock, cmd, cmd_len);
    if (IS_ATOMIC(redis_sock)) {
      redis_boolean_response(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock,
          NULL, NULL);
    }
    REDIS_PROCESS_RESPONSE(redis_boolean_response);
}
/* }}} */

/* {{{ proto string Redis::object(key) */
PHP_METHOD(Redis, object)
{
    RedisSock *redis_sock;
    char *cmd; int cmd_len;
    REDIS_REPLY_TYPE rtype;

    if ((redis_sock = redis_sock_get(getThis(), 0)) == NULL) {
       RETURN_FALSE;
    }

    if(redis_object_cmd(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock, &rtype,
                        &cmd, &cmd_len, NULL, NULL)==FAILURE)
    {
       RETURN_FALSE;
    }

    REDIS_PROCESS_REQUEST(redis_sock, cmd, cmd_len);

    if(rtype == TYPE_INT) {
        if (IS_ATOMIC(redis_sock)) {
            redis_long_response(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock,
                NULL, NULL);
        }
        REDIS_PROCESS_RESPONSE(redis_long_response);
    } else {
        if (IS_ATOMIC(redis_sock)) {
            redis_string_response(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock,
                NULL, NULL);
        }
        REDIS_PROCESS_RESPONSE(redis_string_response);
    }
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
PHP_METHOD(Redis, config)
{
    zend_string *op, *key = NULL, *val = NULL;
    FailableResultCallback cb;
    RedisSock *redis_sock;
    zval *object;
    int cmd_len;
    char *cmd;

    if (zend_parse_method_parameters(ZEND_NUM_ARGS(), getThis(), "OS|SS", &object,
                                     redis_ce, &op, &key, &val) == FAILURE)
    {
        RETURN_FALSE;
    }

    if ((redis_sock = redis_sock_get(object, 0)) == NULL) {
        RETURN_FALSE;
    }

    if (zend_string_equals_literal_ci(op, "GET") && key != NULL) {
        cmd_len = REDIS_SPPRINTF(&cmd, "CONFIG", "SS", op, key);
        cb = redis_mbulk_reply_zipped_raw;
    } else if (zend_string_equals_literal_ci(op, "RESETSTAT") ||
               zend_string_equals_literal_ci(op, "REWRITE"))
    {
        cmd_len = REDIS_SPPRINTF(&cmd, "CONFIG", "s", ZSTR_VAL(op), ZSTR_LEN(op));
        cb = redis_boolean_response;
    } else if (zend_string_equals_literal_ci(op, "SET") && key != NULL && val != NULL) {
        cmd_len = REDIS_SPPRINTF(&cmd, "CONFIG", "SSS", op, key, val);
        cb = redis_boolean_response;
    } else {
        RETURN_FALSE;
    }

    REDIS_PROCESS_REQUEST(redis_sock, cmd, cmd_len)
    if (IS_ATOMIC(redis_sock)) {
        cb(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock, NULL, NULL);
    }
    REDIS_PROCESS_RESPONSE(redis_boolean_response);
}
/* }}} */


/* {{{ proto boolean Redis::slowlog(string arg, [int option]) */
PHP_METHOD(Redis, slowlog) {
    zval *object;
    RedisSock *redis_sock;
    char *arg, *cmd;
    int cmd_len;
    size_t arg_len;
    zend_long option = 0;
    enum {SLOWLOG_GET, SLOWLOG_LEN, SLOWLOG_RESET} mode;

    // Make sure we can get parameters
    if(zend_parse_method_parameters(ZEND_NUM_ARGS(), getThis(),
                                    "Os|l", &object, redis_ce, &arg, &arg_len,
                                    &option) == FAILURE)
    {
        RETURN_FALSE;
    }

    /* Figure out what kind of slowlog command we're executing */
    if(!strncasecmp(arg, "GET", 3)) {
        mode = SLOWLOG_GET;
    } else if(!strncasecmp(arg, "LEN", 3)) {
        mode = SLOWLOG_LEN;
    } else if(!strncasecmp(arg, "RESET", 5)) {
        mode = SLOWLOG_RESET;
    } else {
        /* This command is not valid */
        RETURN_FALSE;
    }

    /* Make sure we can grab our redis socket */
    if ((redis_sock = redis_sock_get(object, 0)) == NULL) {
        RETURN_FALSE;
    }

    // Create our command.  For everything except SLOWLOG GET (with an arg) it's
    // just two parts
    if (mode == SLOWLOG_GET && ZEND_NUM_ARGS() == 2) {
        cmd_len = REDIS_SPPRINTF(&cmd, "SLOWLOG", "sl", arg, arg_len, option);
    } else {
        cmd_len = REDIS_SPPRINTF(&cmd, "SLOWLOG", "s", arg, arg_len);
    }

    /* Kick off our command */
    REDIS_PROCESS_REQUEST(redis_sock, cmd, cmd_len);
    if (IS_ATOMIC(redis_sock)) {
        if(redis_read_variant_reply(INTERNAL_FUNCTION_PARAM_PASSTHRU,
                                    redis_sock, NULL, NULL) < 0)
        {
            RETURN_FALSE;
        }
    }
    REDIS_PROCESS_RESPONSE(redis_read_variant_reply);
}

/* {{{ proto Redis::wait(int num_slaves, int ms) }}} */
PHP_METHOD(Redis, wait) {
    zval *object;
    RedisSock *redis_sock;
    zend_long num_slaves, timeout;
    char *cmd;
    int cmd_len;

    /* Make sure arguments are valid */
    if(zend_parse_method_parameters(ZEND_NUM_ARGS(), getThis(), "Oll",
                                    &object, redis_ce, &num_slaves, &timeout)
                                    ==FAILURE)
    {
        RETURN_FALSE;
    }

    /* Don't even send this to Redis if our args are negative */
    if(num_slaves < 0 || timeout < 0) {
        RETURN_FALSE;
    }

    /* Grab our socket */
    if ((redis_sock = redis_sock_get(object, 0)) == NULL) {
        RETURN_FALSE;
    }

    // Construct the command
    cmd_len = REDIS_SPPRINTF(&cmd, "WAIT", "ll", num_slaves, timeout);

    /* Kick it off */
    REDIS_PROCESS_REQUEST(redis_sock, cmd, cmd_len);
    if (IS_ATOMIC(redis_sock)) {
        redis_long_response(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock, NULL,
            NULL);
    }
    REDIS_PROCESS_RESPONSE(redis_long_response);
}

/* Construct a PUBSUB command */
PHP_REDIS_API int
redis_build_pubsub_cmd(RedisSock *redis_sock, char **ret, PUBSUB_TYPE type,
                       zval *arg)
{
    HashTable *ht_chan;
    zval *z_ele;
    smart_string cmd = {0};

    if (type == PUBSUB_CHANNELS) {
        if (arg) {
            /* With a pattern */
            return REDIS_SPPRINTF(ret, "PUBSUB", "sk", "CHANNELS", sizeof("CHANNELS") - 1,
                                  Z_STRVAL_P(arg), Z_STRLEN_P(arg));
        } else {
            /* No pattern */
            return REDIS_SPPRINTF(ret, "PUBSUB", "s", "CHANNELS", sizeof("CHANNELS") - 1);
        }
    } else if (type == PUBSUB_NUMSUB) {
        ht_chan = Z_ARRVAL_P(arg);

        // Add PUBSUB and NUMSUB bits
        redis_cmd_init_sstr(&cmd, zend_hash_num_elements(ht_chan)+1, "PUBSUB", sizeof("PUBSUB")-1);
        redis_cmd_append_sstr(&cmd, "NUMSUB", sizeof("NUMSUB")-1);

        /* Iterate our elements */
        ZEND_HASH_FOREACH_VAL(ht_chan, z_ele) {
            zend_string *zstr = zval_get_string(z_ele);
            redis_cmd_append_sstr_key(&cmd, ZSTR_VAL(zstr), ZSTR_LEN(zstr), redis_sock, NULL);
            zend_string_release(zstr);
        } ZEND_HASH_FOREACH_END();

        /* Set return */
        *ret = cmd.c;
        return cmd.len;
    } else if (type == PUBSUB_NUMPAT) {
        return REDIS_SPPRINTF(ret, "PUBSUB", "s", "NUMPAT", sizeof("NUMPAT") - 1);
    }

    /* Shouldn't ever happen */
    return -1;
}

/*
 * {{{ proto Redis::pubsub("channels", pattern);
 *     proto Redis::pubsub("numsub", Array channels);
 *     proto Redis::pubsub("numpat"); }}}
 */
PHP_METHOD(Redis, pubsub) {
    zval *object;
    RedisSock *redis_sock;
    char *keyword, *cmd;
    int cmd_len;
    size_t kw_len;
    PUBSUB_TYPE type;
    zval *arg = NULL;

    // Parse arguments
    if(zend_parse_method_parameters(ZEND_NUM_ARGS(), getThis(),
                                    "Os|z", &object, redis_ce, &keyword,
                                    &kw_len, &arg)==FAILURE)
    {
        RETURN_FALSE;
    }

    /* Validate our sub command keyword, and that we've got proper arguments */
    if(!strncasecmp(keyword, "channels", sizeof("channels"))) {
        /* One (optional) string argument */
        if(arg && Z_TYPE_P(arg) != IS_STRING) {
            RETURN_FALSE;
        }
        type = PUBSUB_CHANNELS;
    } else if(!strncasecmp(keyword, "numsub", sizeof("numsub"))) {
        /* One array argument */
        if(ZEND_NUM_ARGS() < 2 || Z_TYPE_P(arg) != IS_ARRAY ||
           zend_hash_num_elements(Z_ARRVAL_P(arg)) == 0)
        {
            RETURN_FALSE;
        }
        type = PUBSUB_NUMSUB;
    } else if(!strncasecmp(keyword, "numpat", sizeof("numpat"))) {
        type = PUBSUB_NUMPAT;
    } else {
        /* Invalid keyword */
        RETURN_FALSE;
    }

    /* Grab our socket context object */
    if ((redis_sock = redis_sock_get(object, 0)) == NULL) {
        RETURN_FALSE;
    }

    /* Construct our "PUBSUB" command */
    cmd_len = redis_build_pubsub_cmd(redis_sock, &cmd, type, arg);

    REDIS_PROCESS_REQUEST(redis_sock, cmd, cmd_len);

    if(type == PUBSUB_NUMSUB) {
        if (IS_ATOMIC(redis_sock)) {
            if(redis_mbulk_reply_zipped_keys_int(INTERNAL_FUNCTION_PARAM_PASSTHRU,
                                                 redis_sock, NULL, NULL) < 0)
            {
                RETURN_FALSE;
            }
        }
        REDIS_PROCESS_RESPONSE(redis_mbulk_reply_zipped_keys_int);
    } else {
        if (IS_ATOMIC(redis_sock)) {
            if(redis_read_variant_reply(INTERNAL_FUNCTION_PARAM_PASSTHRU,
                                        redis_sock, NULL, NULL) < 0)
            {
                RETURN_FALSE;
            }
        }
        REDIS_PROCESS_RESPONSE(redis_read_variant_reply);
    }
}

/* {{{ proto variant Redis::eval(string script, [array keys, long num_keys]) */
PHP_METHOD(Redis, eval)
{
    REDIS_PROCESS_KW_CMD("EVAL", redis_eval_cmd, redis_read_raw_variant_reply);
}

/* {{{ proto variant Redis::evalsha(string sha1, [array keys, long num_keys]) */
PHP_METHOD(Redis, evalsha) {
    REDIS_PROCESS_KW_CMD("EVALSHA", redis_eval_cmd, redis_read_raw_variant_reply);
}

/* {{{ proto status Redis::script('flush')
 * {{{ proto status Redis::script('kill')
 * {{{ proto string Redis::script('load', lua_script)
 * {{{ proto int Reids::script('exists', script_sha1 [, script_sha2, ...])
 */
PHP_METHOD(Redis, script) {
    zval *z_args;
    RedisSock *redis_sock;
    smart_string cmd = {0};
    int argc = ZEND_NUM_ARGS();

    /* Attempt to grab our socket */
    if (argc < 1 || (redis_sock = redis_sock_get(getThis(), 0)) == NULL) {
        RETURN_FALSE;
    }

    /* Allocate an array big enough to store our arguments */
    z_args = ecalloc(argc, sizeof(zval));

    /* Make sure we can grab our arguments, we have a string directive */
    if (zend_get_parameters_array(ht, argc, z_args) == FAILURE ||
        redis_build_script_cmd(&cmd, argc, z_args) == NULL
    ) {
        efree(z_args);
        RETURN_FALSE;
    }

    /* Free our allocated arguments */
    efree(z_args);

    // Kick off our request
    REDIS_PROCESS_REQUEST(redis_sock, cmd.c, cmd.len);
    if (IS_ATOMIC(redis_sock)) {
        if(redis_read_variant_reply(INTERNAL_FUNCTION_PARAM_PASSTHRU,
                                    redis_sock, NULL, NULL) < 0)
        {
            RETURN_FALSE;
        }
    }
    REDIS_PROCESS_RESPONSE(redis_read_variant_reply);
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
    REDIS_PROCESS_CMD(command, redis_read_variant_reply);
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
                                        "Oz/|s!lS", &object, redis_ce, &z_iter,
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
    REDIS_PROCESS_KW_CMD("GEOADD", redis_key_varval_cmd, redis_long_response);
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
