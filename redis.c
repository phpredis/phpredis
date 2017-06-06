/* -*- Mode: C; tab-width: 4 -*- */
/*
  +----------------------------------------------------------------------+
  | PHP Version 5                                                        |
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

#include "common.h"
#include "ext/standard/info.h"
#include "php_redis.h"
#include "redis_commands.h"
#include "redis_array.h"
#include "redis_cluster.h"
#include <zend_exceptions.h>

#ifdef PHP_SESSION
#include "ext/session/php_session.h"
#endif

#include "library.h"

#ifdef PHP_SESSION
extern ps_module ps_mod_redis;
extern ps_module ps_mod_redis_cluster;
#endif

extern zend_class_entry *redis_array_ce;
extern zend_class_entry *redis_cluster_ce;
zend_class_entry *redis_ce;
zend_class_entry *redis_exception_ce;
extern zend_class_entry *redis_cluster_exception_ce;
static zend_class_entry *spl_ce_RuntimeException = NULL;

extern zend_function_entry redis_array_functions[];
extern zend_function_entry redis_cluster_functions[];

PHP_INI_BEGIN()
    /* redis arrays */
    PHP_INI_ENTRY("redis.arrays.names", "", PHP_INI_ALL, NULL)
    PHP_INI_ENTRY("redis.arrays.hosts", "", PHP_INI_ALL, NULL)
    PHP_INI_ENTRY("redis.arrays.previous", "", PHP_INI_ALL, NULL)
    PHP_INI_ENTRY("redis.arrays.functions", "", PHP_INI_ALL, NULL)
    PHP_INI_ENTRY("redis.arrays.index", "", PHP_INI_ALL, NULL)
    PHP_INI_ENTRY("redis.arrays.autorehash", "", PHP_INI_ALL, NULL)

    /* redis cluster */
    PHP_INI_ENTRY("redis.clusters.seeds", "", PHP_INI_ALL, NULL)
    PHP_INI_ENTRY("redis.clusters.timeout", "", PHP_INI_ALL, NULL)
    PHP_INI_ENTRY("redis.clusters.read_timeout", "", PHP_INI_ALL, NULL)
PHP_INI_END()

/**
 * Argument info for the SCAN proper
 */
ZEND_BEGIN_ARG_INFO_EX(arginfo_scan, 0, 0, 1)
    ZEND_ARG_INFO(1, i_iterator)
    ZEND_ARG_INFO(0, str_pattern)
    ZEND_ARG_INFO(0, i_count)
ZEND_END_ARG_INFO();

/**
 * Argument info for key scanning
 */
ZEND_BEGIN_ARG_INFO_EX(arginfo_kscan, 0, 0, 2)
    ZEND_ARG_INFO(0, str_key)
    ZEND_ARG_INFO(1, i_iterator)
    ZEND_ARG_INFO(0, str_pattern)
    ZEND_ARG_INFO(0, i_count)
ZEND_END_ARG_INFO();

#ifdef ZTS
ZEND_DECLARE_MODULE_GLOBALS(redis)
#endif

static zend_function_entry redis_functions[] = {
     PHP_ME(Redis, __construct, NULL, ZEND_ACC_CTOR | ZEND_ACC_PUBLIC)
     PHP_ME(Redis, __destruct, NULL, ZEND_ACC_DTOR | ZEND_ACC_PUBLIC)
     PHP_ME(Redis, connect, NULL, ZEND_ACC_PUBLIC)
     PHP_ME(Redis, pconnect, NULL, ZEND_ACC_PUBLIC)
     PHP_ME(Redis, close, NULL, ZEND_ACC_PUBLIC)
     PHP_ME(Redis, ping, NULL, ZEND_ACC_PUBLIC)
     PHP_ME(Redis, echo, NULL, ZEND_ACC_PUBLIC)
     PHP_ME(Redis, get, NULL, ZEND_ACC_PUBLIC)
     PHP_ME(Redis, set, NULL, ZEND_ACC_PUBLIC)
     PHP_ME(Redis, setex, NULL, ZEND_ACC_PUBLIC)
     PHP_ME(Redis, psetex, NULL, ZEND_ACC_PUBLIC)
     PHP_ME(Redis, setnx, NULL, ZEND_ACC_PUBLIC)
     PHP_ME(Redis, getSet, NULL, ZEND_ACC_PUBLIC)
     PHP_ME(Redis, randomKey, NULL, ZEND_ACC_PUBLIC)
     PHP_ME(Redis, renameKey, NULL, ZEND_ACC_PUBLIC)
     PHP_ME(Redis, renameNx, NULL, ZEND_ACC_PUBLIC)
     PHP_ME(Redis, getMultiple, NULL, ZEND_ACC_PUBLIC)
     PHP_ME(Redis, exists, NULL, ZEND_ACC_PUBLIC)
     PHP_ME(Redis, delete, NULL, ZEND_ACC_PUBLIC)
     PHP_ME(Redis, incr, NULL, ZEND_ACC_PUBLIC)
     PHP_ME(Redis, incrBy, NULL, ZEND_ACC_PUBLIC)
     PHP_ME(Redis, incrByFloat, NULL, ZEND_ACC_PUBLIC)
     PHP_ME(Redis, decr, NULL, ZEND_ACC_PUBLIC)
     PHP_ME(Redis, decrBy, NULL, ZEND_ACC_PUBLIC)
     PHP_ME(Redis, type, NULL, ZEND_ACC_PUBLIC)
     PHP_ME(Redis, append, NULL, ZEND_ACC_PUBLIC)
     PHP_ME(Redis, getRange, NULL, ZEND_ACC_PUBLIC)
     PHP_ME(Redis, setRange, NULL, ZEND_ACC_PUBLIC)
     PHP_ME(Redis, getBit, NULL, ZEND_ACC_PUBLIC)
     PHP_ME(Redis, setBit, NULL, ZEND_ACC_PUBLIC)
     PHP_ME(Redis, strlen, NULL, ZEND_ACC_PUBLIC)
     PHP_ME(Redis, getKeys, NULL, ZEND_ACC_PUBLIC)
     PHP_ME(Redis, sort, NULL, ZEND_ACC_PUBLIC)
     PHP_ME(Redis, sortAsc, NULL, ZEND_ACC_PUBLIC)
     PHP_ME(Redis, sortAscAlpha, NULL, ZEND_ACC_PUBLIC)
     PHP_ME(Redis, sortDesc, NULL, ZEND_ACC_PUBLIC)
     PHP_ME(Redis, sortDescAlpha, NULL, ZEND_ACC_PUBLIC)
     PHP_ME(Redis, lPush, NULL, ZEND_ACC_PUBLIC)
     PHP_ME(Redis, rPush, NULL, ZEND_ACC_PUBLIC)
     PHP_ME(Redis, lPushx, NULL, ZEND_ACC_PUBLIC)
     PHP_ME(Redis, rPushx, NULL, ZEND_ACC_PUBLIC)
     PHP_ME(Redis, lPop, NULL, ZEND_ACC_PUBLIC)
     PHP_ME(Redis, rPop, NULL, ZEND_ACC_PUBLIC)
     PHP_ME(Redis, blPop, NULL, ZEND_ACC_PUBLIC)
     PHP_ME(Redis, brPop, NULL, ZEND_ACC_PUBLIC)
     PHP_ME(Redis, lSize, NULL, ZEND_ACC_PUBLIC)
     PHP_ME(Redis, lRemove, NULL, ZEND_ACC_PUBLIC)
     PHP_ME(Redis, listTrim, NULL, ZEND_ACC_PUBLIC)
     PHP_ME(Redis, lGet, NULL, ZEND_ACC_PUBLIC)
     PHP_ME(Redis, lGetRange, NULL, ZEND_ACC_PUBLIC)
     PHP_ME(Redis, lSet, NULL, ZEND_ACC_PUBLIC)
     PHP_ME(Redis, lInsert, NULL, ZEND_ACC_PUBLIC)
     PHP_ME(Redis, sAdd, NULL, ZEND_ACC_PUBLIC)
     PHP_ME(Redis, sAddArray, NULL, ZEND_ACC_PUBLIC)
     PHP_ME(Redis, sSize, NULL, ZEND_ACC_PUBLIC)
     PHP_ME(Redis, sRemove, NULL, ZEND_ACC_PUBLIC)
     PHP_ME(Redis, sMove, NULL, ZEND_ACC_PUBLIC)
     PHP_ME(Redis, sPop, NULL, ZEND_ACC_PUBLIC)
     PHP_ME(Redis, sRandMember, NULL, ZEND_ACC_PUBLIC)
     PHP_ME(Redis, sContains, NULL, ZEND_ACC_PUBLIC)
     PHP_ME(Redis, sMembers, NULL, ZEND_ACC_PUBLIC)
     PHP_ME(Redis, sInter, NULL, ZEND_ACC_PUBLIC)
     PHP_ME(Redis, sInterStore, NULL, ZEND_ACC_PUBLIC)
     PHP_ME(Redis, sUnion, NULL, ZEND_ACC_PUBLIC)
     PHP_ME(Redis, sUnionStore, NULL, ZEND_ACC_PUBLIC)
     PHP_ME(Redis, sDiff, NULL, ZEND_ACC_PUBLIC)
     PHP_ME(Redis, sDiffStore, NULL, ZEND_ACC_PUBLIC)
     PHP_ME(Redis, setTimeout, NULL, ZEND_ACC_PUBLIC)
     PHP_ME(Redis, save, NULL, ZEND_ACC_PUBLIC)
     PHP_ME(Redis, bgSave, NULL, ZEND_ACC_PUBLIC)
     PHP_ME(Redis, lastSave, NULL, ZEND_ACC_PUBLIC)
     PHP_ME(Redis, flushDB, NULL, ZEND_ACC_PUBLIC)
     PHP_ME(Redis, flushAll, NULL, ZEND_ACC_PUBLIC)
     PHP_ME(Redis, dbSize, NULL, ZEND_ACC_PUBLIC)
     PHP_ME(Redis, auth, NULL, ZEND_ACC_PUBLIC)
     PHP_ME(Redis, ttl, NULL, ZEND_ACC_PUBLIC)
     PHP_ME(Redis, pttl, NULL, ZEND_ACC_PUBLIC)
     PHP_ME(Redis, persist, NULL, ZEND_ACC_PUBLIC)
     PHP_ME(Redis, info, NULL, ZEND_ACC_PUBLIC)
     PHP_ME(Redis, select, NULL, ZEND_ACC_PUBLIC)
     PHP_ME(Redis, move, NULL, ZEND_ACC_PUBLIC)
     PHP_ME(Redis, bgrewriteaof, NULL, ZEND_ACC_PUBLIC)
     PHP_ME(Redis, slaveof, NULL, ZEND_ACC_PUBLIC)
     PHP_ME(Redis, object, NULL, ZEND_ACC_PUBLIC)
     PHP_ME(Redis, bitop, NULL, ZEND_ACC_PUBLIC)
     PHP_ME(Redis, bitcount, NULL, ZEND_ACC_PUBLIC)
     PHP_ME(Redis, bitpos, NULL, ZEND_ACC_PUBLIC)

     /* 1.1 */
     PHP_ME(Redis, mset, NULL, ZEND_ACC_PUBLIC)
     PHP_ME(Redis, msetnx, NULL, ZEND_ACC_PUBLIC)
     PHP_ME(Redis, rpoplpush, NULL, ZEND_ACC_PUBLIC)
     PHP_ME(Redis, brpoplpush, NULL, ZEND_ACC_PUBLIC)
     PHP_ME(Redis, zAdd, NULL, ZEND_ACC_PUBLIC)
     PHP_ME(Redis, zDelete, NULL, ZEND_ACC_PUBLIC)
     PHP_ME(Redis, zRange, NULL, ZEND_ACC_PUBLIC)
     PHP_ME(Redis, zRevRange, NULL, ZEND_ACC_PUBLIC)
     PHP_ME(Redis, zRangeByScore, NULL, ZEND_ACC_PUBLIC)
     PHP_ME(Redis, zRevRangeByScore, NULL, ZEND_ACC_PUBLIC)
     PHP_ME(Redis, zRangeByLex, NULL, ZEND_ACC_PUBLIC)
     PHP_ME(Redis, zRevRangeByLex, NULL, ZEND_ACC_PUBLIC)
     PHP_ME(Redis, zLexCount, NULL, ZEND_ACC_PUBLIC)
     PHP_ME(Redis, zRemRangeByLex, NULL, ZEND_ACC_PUBLIC)
     PHP_ME(Redis, zCount, NULL, ZEND_ACC_PUBLIC)
     PHP_ME(Redis, zDeleteRangeByScore, NULL, ZEND_ACC_PUBLIC)
     PHP_ME(Redis, zDeleteRangeByRank, NULL, ZEND_ACC_PUBLIC)
     PHP_ME(Redis, zCard, NULL, ZEND_ACC_PUBLIC)
     PHP_ME(Redis, zScore, NULL, ZEND_ACC_PUBLIC)
     PHP_ME(Redis, zRank, NULL, ZEND_ACC_PUBLIC)
     PHP_ME(Redis, zRevRank, NULL, ZEND_ACC_PUBLIC)
     PHP_ME(Redis, zInter, NULL, ZEND_ACC_PUBLIC)
     PHP_ME(Redis, zUnion, NULL, ZEND_ACC_PUBLIC)
     PHP_ME(Redis, zIncrBy, NULL, ZEND_ACC_PUBLIC)
     PHP_ME(Redis, expireAt, NULL, ZEND_ACC_PUBLIC)
     PHP_ME(Redis, pexpire, NULL, ZEND_ACC_PUBLIC)
     PHP_ME(Redis, pexpireAt, NULL, ZEND_ACC_PUBLIC)

     /* 1.2 */
     PHP_ME(Redis, hGet, NULL, ZEND_ACC_PUBLIC)
     PHP_ME(Redis, hSet, NULL, ZEND_ACC_PUBLIC)
     PHP_ME(Redis, hSetNx, NULL, ZEND_ACC_PUBLIC)
     PHP_ME(Redis, hDel, NULL, ZEND_ACC_PUBLIC)
     PHP_ME(Redis, hLen, NULL, ZEND_ACC_PUBLIC)
     PHP_ME(Redis, hKeys, NULL, ZEND_ACC_PUBLIC)
     PHP_ME(Redis, hVals, NULL, ZEND_ACC_PUBLIC)
     PHP_ME(Redis, hGetAll, NULL, ZEND_ACC_PUBLIC)
     PHP_ME(Redis, hExists, NULL, ZEND_ACC_PUBLIC)
     PHP_ME(Redis, hIncrBy, NULL, ZEND_ACC_PUBLIC)
     PHP_ME(Redis, hIncrByFloat, NULL, ZEND_ACC_PUBLIC)
     PHP_ME(Redis, hMset, NULL, ZEND_ACC_PUBLIC)
     PHP_ME(Redis, hMget, NULL, ZEND_ACC_PUBLIC)
     PHP_ME(Redis, hStrLen, NULL, ZEND_ACC_PUBLIC)

     PHP_ME(Redis, multi, NULL, ZEND_ACC_PUBLIC)
     PHP_ME(Redis, discard, NULL, ZEND_ACC_PUBLIC)
     PHP_ME(Redis, exec, NULL, ZEND_ACC_PUBLIC)
     PHP_ME(Redis, pipeline, NULL, ZEND_ACC_PUBLIC)
     PHP_ME(Redis, watch, NULL, ZEND_ACC_PUBLIC)
     PHP_ME(Redis, unwatch, NULL, ZEND_ACC_PUBLIC)

     PHP_ME(Redis, publish, NULL, ZEND_ACC_PUBLIC)
     PHP_ME(Redis, subscribe, NULL, ZEND_ACC_PUBLIC)
     PHP_ME(Redis, psubscribe, NULL, ZEND_ACC_PUBLIC)
     PHP_ME(Redis, unsubscribe, NULL, ZEND_ACC_PUBLIC)
     PHP_ME(Redis, punsubscribe, NULL, ZEND_ACC_PUBLIC)

     PHP_ME(Redis, time, NULL, ZEND_ACC_PUBLIC)
     PHP_ME(Redis, role, NULL, ZEND_ACC_PUBLIC)
     PHP_ME(Redis, eval, NULL, ZEND_ACC_PUBLIC)
     PHP_ME(Redis, evalsha, NULL, ZEND_ACC_PUBLIC)
     PHP_ME(Redis, script, NULL, ZEND_ACC_PUBLIC)

     PHP_ME(Redis, debug, NULL, ZEND_ACC_PUBLIC)
     PHP_ME(Redis, dump, NULL, ZEND_ACC_PUBLIC)
     PHP_ME(Redis, restore, NULL, ZEND_ACC_PUBLIC)
     PHP_ME(Redis, migrate, NULL, ZEND_ACC_PUBLIC)

     PHP_ME(Redis, getLastError, NULL, ZEND_ACC_PUBLIC)
     PHP_ME(Redis, clearLastError, NULL, ZEND_ACC_PUBLIC)

     PHP_ME(Redis, _prefix, NULL, ZEND_ACC_PUBLIC)
     PHP_ME(Redis, _serialize, NULL, ZEND_ACC_PUBLIC)
     PHP_ME(Redis, _unserialize, NULL, ZEND_ACC_PUBLIC)

     PHP_ME(Redis, client, NULL, ZEND_ACC_PUBLIC)
     PHP_ME(Redis, command, NULL, ZEND_ACC_PUBLIC)

     /* SCAN and friends */
     PHP_ME(Redis, scan, arginfo_scan, ZEND_ACC_PUBLIC)
     PHP_ME(Redis, hscan, arginfo_kscan, ZEND_ACC_PUBLIC)
     PHP_ME(Redis, zscan, arginfo_kscan, ZEND_ACC_PUBLIC)
     PHP_ME(Redis, sscan, arginfo_kscan, ZEND_ACC_PUBLIC)

     /* HyperLogLog commands */
     PHP_ME(Redis, pfadd, NULL, ZEND_ACC_PUBLIC)
     PHP_ME(Redis, pfcount, NULL, ZEND_ACC_PUBLIC)
     PHP_ME(Redis, pfmerge, NULL, ZEND_ACC_PUBLIC)

     /* options */
     PHP_ME(Redis, getOption, NULL, ZEND_ACC_PUBLIC)
     PHP_ME(Redis, setOption, NULL, ZEND_ACC_PUBLIC)

     /* config */
     PHP_ME(Redis, config, NULL, ZEND_ACC_PUBLIC)

     /* slowlog */
     PHP_ME(Redis, slowlog, NULL, ZEND_ACC_PUBLIC)

     /* Send a raw command and read raw results */
     PHP_ME(Redis, rawcommand, NULL, ZEND_ACC_PUBLIC)

     /* geoadd and friends */
     PHP_ME(Redis, geoadd, NULL, ZEND_ACC_PUBLIC)
     PHP_ME(Redis, geohash, NULL, ZEND_ACC_PUBLIC)
     PHP_ME(Redis, geopos, NULL, ZEND_ACC_PUBLIC)
     PHP_ME(Redis, geodist, NULL, ZEND_ACC_PUBLIC)
     PHP_ME(Redis, georadius, NULL, ZEND_ACC_PUBLIC)
     PHP_ME(Redis, georadiusbymember, NULL, ZEND_ACC_PUBLIC)

     /* introspection */
     PHP_ME(Redis, getHost, NULL, ZEND_ACC_PUBLIC)
     PHP_ME(Redis, getPort, NULL, ZEND_ACC_PUBLIC)
     PHP_ME(Redis, getDBNum, NULL, ZEND_ACC_PUBLIC)
     PHP_ME(Redis, getTimeout, NULL, ZEND_ACC_PUBLIC)
     PHP_ME(Redis, getReadTimeout, NULL, ZEND_ACC_PUBLIC)
     PHP_ME(Redis, getPersistentID, NULL, ZEND_ACC_PUBLIC)
     PHP_ME(Redis, getAuth, NULL, ZEND_ACC_PUBLIC)
     PHP_ME(Redis, isConnected, NULL, ZEND_ACC_PUBLIC)
     PHP_ME(Redis, getMode, NULL, ZEND_ACC_PUBLIC)
     PHP_ME(Redis, wait, NULL, ZEND_ACC_PUBLIC)
     PHP_ME(Redis, pubsub, NULL, ZEND_ACC_PUBLIC)

     /* aliases */
     PHP_MALIAS(Redis, open, connect, NULL, ZEND_ACC_PUBLIC)
     PHP_MALIAS(Redis, popen, pconnect, NULL, ZEND_ACC_PUBLIC)
     PHP_MALIAS(Redis, lLen, lSize, NULL, ZEND_ACC_PUBLIC)
     PHP_MALIAS(Redis, sGetMembers, sMembers, NULL, ZEND_ACC_PUBLIC)
     PHP_MALIAS(Redis, mget, getMultiple, NULL, ZEND_ACC_PUBLIC)
     PHP_MALIAS(Redis, expire, setTimeout, NULL, ZEND_ACC_PUBLIC)
     PHP_MALIAS(Redis, zunionstore, zUnion, NULL, ZEND_ACC_PUBLIC)
     PHP_MALIAS(Redis, zinterstore, zInter, NULL, ZEND_ACC_PUBLIC)

     PHP_MALIAS(Redis, zRemove, zDelete, NULL, ZEND_ACC_PUBLIC)
     PHP_MALIAS(Redis, zRem, zDelete, NULL, ZEND_ACC_PUBLIC)
     PHP_MALIAS(Redis, zRemoveRangeByScore, zDeleteRangeByScore, NULL, ZEND_ACC_PUBLIC)
     PHP_MALIAS(Redis, zRemRangeByScore, zDeleteRangeByScore, NULL, ZEND_ACC_PUBLIC)
     PHP_MALIAS(Redis, zRemRangeByRank, zDeleteRangeByRank, NULL, ZEND_ACC_PUBLIC)
     PHP_MALIAS(Redis, zSize, zCard, NULL, ZEND_ACC_PUBLIC)
     PHP_MALIAS(Redis, substr, getRange, NULL, ZEND_ACC_PUBLIC)
     PHP_MALIAS(Redis, rename, renameKey, NULL, ZEND_ACC_PUBLIC)
     PHP_MALIAS(Redis, del, delete, NULL, ZEND_ACC_PUBLIC)
     PHP_MALIAS(Redis, keys, getKeys, NULL, ZEND_ACC_PUBLIC)
     PHP_MALIAS(Redis, lrem, lRemove, NULL, ZEND_ACC_PUBLIC)
     PHP_MALIAS(Redis, ltrim, listTrim, NULL, ZEND_ACC_PUBLIC)
     PHP_MALIAS(Redis, lindex, lGet, NULL, ZEND_ACC_PUBLIC)
     PHP_MALIAS(Redis, lrange, lGetRange, NULL, ZEND_ACC_PUBLIC)
     PHP_MALIAS(Redis, scard, sSize, NULL, ZEND_ACC_PUBLIC)
     PHP_MALIAS(Redis, srem, sRemove, NULL, ZEND_ACC_PUBLIC)
     PHP_MALIAS(Redis, sismember, sContains, NULL, ZEND_ACC_PUBLIC)
     PHP_MALIAS(Redis, zReverseRange, zRevRange, NULL, ZEND_ACC_PUBLIC)

     PHP_MALIAS(Redis, sendEcho, echo, NULL, ZEND_ACC_PUBLIC)

     PHP_MALIAS(Redis, evaluate, eval, NULL, ZEND_ACC_PUBLIC)
     PHP_MALIAS(Redis, evaluateSha, evalsha, NULL, ZEND_ACC_PUBLIC)
     PHP_FE_END
};

static const zend_module_dep redis_deps[] = {
#ifdef HAVE_REDIS_IGBINARY
     ZEND_MOD_REQUIRED("igbinary")
#endif
     ZEND_MOD_END
};

zend_module_entry redis_module_entry = {
#if ZEND_MODULE_API_NO >= 20010901
     STANDARD_MODULE_HEADER_EX,
     NULL,
     redis_deps,
#endif
     "redis",
     NULL,
     PHP_MINIT(redis),
     PHP_MSHUTDOWN(redis),
     NULL,
     NULL,
     PHP_MINFO(redis),
#if ZEND_MODULE_API_NO >= 20010901
     PHP_REDIS_VERSION,
#endif
     STANDARD_MODULE_PROPERTIES
};

#ifdef COMPILE_DL_REDIS
ZEND_GET_MODULE(redis)
#endif

PHP_REDIS_API zend_class_entry *redis_get_exception_base(int root TSRMLS_DC)
{
#if HAVE_SPL
    if (!root) {
        if (!spl_ce_RuntimeException) {
            zend_class_entry *pce;

            if ((pce = zend_hash_str_find_ptr(CG(class_table), "runtimeexception",
                                              sizeof("RuntimeException") - 1)))
            {
                spl_ce_RuntimeException = pce;
                return pce;
            }
        } else {
            return spl_ce_RuntimeException;
        }
    }
#endif
#if (PHP_MAJOR_VERSION == 5) && (PHP_MINOR_VERSION < 2)
    return zend_exception_get_default();
#else
    return zend_exception_get_default(TSRMLS_C);
#endif
}

/* Send a static DISCARD in case we're in MULTI mode. */
static int send_discard_static(RedisSock *redis_sock TSRMLS_DC) {
    int result = FAILURE;
    char *cmd, *resp;
    int resp_len, cmd_len;

    /* format our discard command */
    cmd_len = REDIS_SPPRINTF(&cmd, "DISCARD", "");

    /* send our DISCARD command */
    if (redis_sock_write(redis_sock, cmd, cmd_len TSRMLS_CC) >= 0 &&
       (resp = redis_sock_read(redis_sock,&resp_len TSRMLS_CC)) != NULL)
    {
        /* success if we get OK */
        result = (resp_len == 3 && strncmp(resp,"+OK", 3)==0) ? SUCCESS:FAILURE;

        /* free our response */
        efree(resp);
    }

    /* free our command */
    efree(cmd);

    /* return success/failure */
    return result;
}

static void
free_reply_callbacks(RedisSock *redis_sock)
{
    fold_item *fi;

    for (fi = redis_sock->head; fi; ) {
        fold_item *fi_next = fi->next;
        free(fi);
        fi = fi_next;
    }
    redis_sock->head = NULL;
    redis_sock->current = NULL;
}

#if (PHP_MAJOR_VERSION < 7)
void
free_redis_object(void *object TSRMLS_DC)
{
    redis_object *redis = (redis_object *)object;

    zend_object_std_dtor(&redis->std TSRMLS_CC);
    if (redis->sock) {
        redis_sock_disconnect(redis->sock TSRMLS_CC);
        redis_free_socket(redis->sock);
    }
    efree(redis);
}

zend_object_value
create_redis_object(zend_class_entry *ce TSRMLS_DC)
{
    zend_object_value retval;
    redis_object *redis = ecalloc(1, sizeof(redis_object));

    memset(redis, 0, sizeof(redis_object));
    zend_object_std_init(&redis->std, ce TSRMLS_CC);

#if PHP_VERSION_ID < 50399
    zval *tmp;
    zend_hash_copy(redis->std.properties, &ce->default_properties,
        (copy_ctor_func_t)zval_add_ref, (void *)&tmp, sizeof(zval *));
#endif

    retval.handle = zend_objects_store_put(redis,
        (zend_objects_store_dtor_t)zend_objects_destroy_object,
        (zend_objects_free_object_storage_t)free_redis_object,
        NULL TSRMLS_CC);
    retval.handlers = zend_get_std_object_handlers();

    return retval;
}
#else
zend_object_handlers redis_object_handlers;

void
free_redis_object(zend_object *object)
{
    redis_object *redis = (redis_object *)((char *)(object) - XtOffsetOf(redis_object, std));

    zend_object_std_dtor(&redis->std TSRMLS_CC);
    if (redis->sock) {
        redis_sock_disconnect(redis->sock TSRMLS_CC);
        redis_free_socket(redis->sock);
    }
}

zend_object *
create_redis_object(zend_class_entry *ce TSRMLS_DC)
{
    redis_object *redis = ecalloc(1, sizeof(redis_object) + zend_object_properties_size(ce));

    redis->sock = NULL;

    zend_object_std_init(&redis->std, ce TSRMLS_CC);
    object_properties_init(&redis->std, ce);

    memcpy(&redis_object_handlers, zend_get_std_object_handlers(), sizeof(redis_object_handlers));
    redis_object_handlers.offset = XtOffsetOf(redis_object, std);
    redis_object_handlers.free_obj = free_redis_object;
    redis->std.handlers = &redis_object_handlers;

    return &redis->std;
}
#endif

static zend_always_inline RedisSock *
redis_sock_get_instance(zval *id TSRMLS_DC, int no_throw)
{
    redis_object *redis;

    if (Z_TYPE_P(id) == IS_OBJECT) {
#if (PHP_MAJOR_VERSION < 7)
        redis = (redis_object *)zend_objects_get_address(id TSRMLS_CC);
#else
        redis = (redis_object *)((char *)Z_OBJ_P(id) - XtOffsetOf(redis_object, std));
#endif
        if (redis->sock) {
            return redis->sock;
        }
    }
    // Throw an exception unless we've been requested not to
    if (!no_throw) {
        zend_throw_exception(redis_exception_ce, "Redis server went away", 0 TSRMLS_CC);
    }
    return NULL;
}

/**
 * redis_sock_get
 */
PHP_REDIS_API RedisSock *
redis_sock_get(zval *id TSRMLS_DC, int no_throw)
{
    RedisSock *redis_sock;

    if ((redis_sock = redis_sock_get_instance(id TSRMLS_CC, no_throw)) == NULL) {
        return NULL;
    }

    if (redis_sock->lazy_connect) {
        redis_sock->lazy_connect = 0;
        if (redis_sock_server_open(redis_sock TSRMLS_CC) < 0) {
            return NULL;
        }
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
    if((zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), "O",
       &object, redis_ce) == FAILURE) ||
       (redis_sock = redis_sock_get(object TSRMLS_CC, 1)) == NULL ||
       redis_sock->status != REDIS_SOCK_STATUS_CONNECTED)
    {
        return NULL;
    }

    /* Return our socket */
    return redis_sock;
}

/* Redis and RedisCluster objects share serialization/prefixing settings so
 * this is a generic function to add class constants to either */
static void add_class_constants(zend_class_entry *ce, int is_cluster TSRMLS_DC) {
    zend_declare_class_constant_long(ce, ZEND_STRL("REDIS_NOT_FOUND"), REDIS_NOT_FOUND TSRMLS_CC);
    zend_declare_class_constant_long(ce, ZEND_STRL("REDIS_STRING"), REDIS_STRING TSRMLS_CC);
    zend_declare_class_constant_long(ce, ZEND_STRL("REDIS_SET"), REDIS_SET TSRMLS_CC);
    zend_declare_class_constant_long(ce, ZEND_STRL("REDIS_LIST"), REDIS_LIST TSRMLS_CC);
    zend_declare_class_constant_long(ce, ZEND_STRL("REDIS_ZSET"), REDIS_ZSET TSRMLS_CC);
    zend_declare_class_constant_long(ce, ZEND_STRL("REDIS_HASH"), REDIS_HASH TSRMLS_CC);

    /* Cluster doesn't support pipelining at this time */
    if(!is_cluster) {
        zend_declare_class_constant_long(ce, ZEND_STRL("PIPELINE"), PIPELINE TSRMLS_CC);
    }

    /* Add common mode constants */
    zend_declare_class_constant_long(ce, ZEND_STRL("ATOMIC"), ATOMIC TSRMLS_CC);
    zend_declare_class_constant_long(ce, ZEND_STRL("MULTI"), MULTI TSRMLS_CC);

    /* options */
    zend_declare_class_constant_long(ce, ZEND_STRL("OPT_SERIALIZER"), REDIS_OPT_SERIALIZER TSRMLS_CC);
    zend_declare_class_constant_long(ce, ZEND_STRL("OPT_PREFIX"), REDIS_OPT_PREFIX TSRMLS_CC);
    zend_declare_class_constant_long(ce, ZEND_STRL("OPT_READ_TIMEOUT"), REDIS_OPT_READ_TIMEOUT TSRMLS_CC);

    /* serializer */
    zend_declare_class_constant_long(ce, ZEND_STRL("SERIALIZER_NONE"), REDIS_SERIALIZER_NONE TSRMLS_CC);
    zend_declare_class_constant_long(ce, ZEND_STRL("SERIALIZER_PHP"), REDIS_SERIALIZER_PHP TSRMLS_CC);
#ifdef HAVE_REDIS_IGBINARY
    zend_declare_class_constant_long(ce, ZEND_STRL("SERIALIZER_IGBINARY"), REDIS_SERIALIZER_IGBINARY TSRMLS_CC);
#endif

    /* scan options*/
    zend_declare_class_constant_long(ce, ZEND_STRL("OPT_SCAN"), REDIS_OPT_SCAN TSRMLS_CC);
    zend_declare_class_constant_long(ce, ZEND_STRL("SCAN_RETRY"), REDIS_SCAN_RETRY TSRMLS_CC);
    zend_declare_class_constant_long(ce, ZEND_STRL("SCAN_NORETRY"), REDIS_SCAN_NORETRY TSRMLS_CC);

    /* Cluster option to allow for slave failover */
    if (is_cluster) {
        zend_declare_class_constant_long(ce, ZEND_STRL("OPT_SLAVE_FAILOVER"), REDIS_OPT_FAILOVER TSRMLS_CC);
        zend_declare_class_constant_long(ce, ZEND_STRL("FAILOVER_NONE"), REDIS_FAILOVER_NONE TSRMLS_CC);
        zend_declare_class_constant_long(ce, ZEND_STRL("FAILOVER_ERROR"), REDIS_FAILOVER_ERROR TSRMLS_CC);
        zend_declare_class_constant_long(ce, ZEND_STRL("FAILOVER_DISTRIBUTE"), REDIS_FAILOVER_DISTRIBUTE TSRMLS_CC);
        zend_declare_class_constant_long(ce, ZEND_STRL("FAILOVER_DISTRIBUTE_SLAVES"), REDIS_FAILOVER_DISTRIBUTE_SLAVES TSRMLS_CC);
    }

    zend_declare_class_constant_stringl(ce, "AFTER", 5, "after", 5 TSRMLS_CC);
    zend_declare_class_constant_stringl(ce, "BEFORE", 6, "before", 6 TSRMLS_CC);
}

/**
 * PHP_MINIT_FUNCTION
 */
PHP_MINIT_FUNCTION(redis)
{
    struct timeval tv;

    zend_class_entry redis_class_entry;
    zend_class_entry redis_array_class_entry;
    zend_class_entry redis_cluster_class_entry;
    zend_class_entry redis_exception_class_entry;
    zend_class_entry redis_cluster_exception_class_entry;

    /* Seed random generator (for RedisCluster failover) */
    gettimeofday(&tv, NULL);
    srand(tv.tv_usec * tv.tv_sec);

    REGISTER_INI_ENTRIES();

    /* Redis class */
    INIT_CLASS_ENTRY(redis_class_entry, "Redis", redis_functions);
    redis_ce = zend_register_internal_class(&redis_class_entry TSRMLS_CC);
    redis_ce->create_object = create_redis_object;

    /* RedisArray class */
    INIT_CLASS_ENTRY(redis_array_class_entry, "RedisArray", redis_array_functions);
    redis_array_ce = zend_register_internal_class(&redis_array_class_entry TSRMLS_CC);
    redis_array_ce->create_object = create_redis_array_object;

    /* RedisCluster class */
    INIT_CLASS_ENTRY(redis_cluster_class_entry, "RedisCluster", redis_cluster_functions);
    redis_cluster_ce = zend_register_internal_class(&redis_cluster_class_entry TSRMLS_CC);
    redis_cluster_ce->create_object = create_cluster_context;

    /* RedisException class */
    INIT_CLASS_ENTRY(redis_exception_class_entry, "RedisException", NULL);
    redis_exception_ce = zend_register_internal_class_ex(
        &redis_exception_class_entry,
#if (PHP_MAJOR_VERSION < 7)
        redis_get_exception_base(0 TSRMLS_CC),
        NULL TSRMLS_CC
#else
        redis_get_exception_base(0)
#endif
    );

    /* RedisClusterException class */
    INIT_CLASS_ENTRY(redis_cluster_exception_class_entry,
        "RedisClusterException", NULL);
    redis_cluster_exception_ce = zend_register_internal_class_ex(
        &redis_cluster_exception_class_entry,
#if (PHP_MAJOR_VERSION < 7)
        rediscluster_get_exception_base(0 TSRMLS_CC),
        NULL TSRMLS_CC
#else
        rediscluster_get_exception_base(0)
#endif
    );

    /* Add shared class constants to Redis and RedisCluster objects */
    add_class_constants(redis_ce, 0 TSRMLS_CC);
    add_class_constants(redis_cluster_ce, 1 TSRMLS_CC);

#ifdef PHP_SESSION
    php_session_register_module(&ps_mod_redis);
    php_session_register_module(&ps_mod_redis_cluster);
#endif

    return SUCCESS;
}

/**
 * PHP_MSHUTDOWN_FUNCTION
 */
PHP_MSHUTDOWN_FUNCTION(redis)
{
    return SUCCESS;
}

/**
 * PHP_MINFO_FUNCTION
 */
PHP_MINFO_FUNCTION(redis)
{
    php_info_print_table_start();
    php_info_print_table_header(2, "Redis Support", "enabled");
    php_info_print_table_row(2, "Redis Version", PHP_REDIS_VERSION);
#ifdef HAVE_REDIS_IGBINARY
    php_info_print_table_row(2, "Available serializers", "php, igbinary");
#else
    php_info_print_table_row(2, "Available serializers", "php");
#endif
    php_info_print_table_end();
}

/* {{{ proto Redis Redis::__construct()
    Public constructor */
PHP_METHOD(Redis, __construct)
{
    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "") == FAILURE) {
        RETURN_FALSE;
    }
}
/* }}} */

/* {{{ proto Redis Redis::__destruct()
    Public Destructor
 */
PHP_METHOD(Redis,__destruct) {
    if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "") == FAILURE) {
        RETURN_FALSE;
    }

    // Grab our socket
    RedisSock *redis_sock;
    if ((redis_sock = redis_sock_get(getThis() TSRMLS_CC, 1)) == NULL) {
        RETURN_FALSE;
    }

    // If we think we're in MULTI mode, send a discard
    IF_MULTI() {
        // Discard any multi commands, and free any callbacks that have been
        // queued
        send_discard_static(redis_sock TSRMLS_CC);
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
        /* FIXME: should we remove whole `else` block? */
        /* reset multi/exec state if there is one. */
        RedisSock *redis_sock;
        if ((redis_sock = redis_sock_get(getThis() TSRMLS_CC, 0)) == NULL) {
            RETURN_FALSE;
        }

        RETURN_TRUE;
    }
}
/* }}} */

PHP_REDIS_API int
redis_connect(INTERNAL_FUNCTION_PARAMETERS, int persistent)
{
    zval *object;
    char *host = NULL, *persistent_id = NULL;
    zend_long port = -1, retry_interval = 0;
    strlen_t host_len, persistent_id_len;
    double timeout = 0.0, read_timeout = 0.0;
    redis_object *redis;

#ifdef ZTS
    /* not sure how in threaded mode this works so disabled persistence at
     * first */
    persistent = 0;
#endif

    if (zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(),
                                     "Os|ldsld", &object, redis_ce, &host,
                                     &host_len, &port, &timeout, &persistent_id,
                                     &persistent_id_len, &retry_interval,
                                     &read_timeout) == FAILURE)
    {
        return FAILURE;
    } else if (!persistent) {
        persistent_id = NULL;
    }

    if (timeout < 0L || timeout > INT_MAX) {
        zend_throw_exception(redis_exception_ce,
            "Invalid connect timeout", 0 TSRMLS_CC);
        return FAILURE;
    }

    if (read_timeout < 0L || read_timeout > INT_MAX) {
        zend_throw_exception(redis_exception_ce,
            "Invalid read timeout", 0 TSRMLS_CC);
        return FAILURE;
    }

    if (retry_interval < 0L || retry_interval > INT_MAX) {
        zend_throw_exception(redis_exception_ce, "Invalid retry interval",
            0 TSRMLS_CC);
        return FAILURE;
    }

    /* If it's not a unix socket, set to default */
    if(port == -1 && host_len && host[0] != '/') {
        port = 6379;
    }

#if (PHP_MAJOR_VERSION < 7)
    redis = (redis_object *)zend_objects_get_address(object TSRMLS_CC);
#else
    redis = (redis_object *)((char *)Z_OBJ_P(object) - XtOffsetOf(redis_object, std));
#endif
    /* if there is a redis sock already we have to remove it */
    if (redis->sock) {
        redis_sock_disconnect(redis->sock TSRMLS_CC);
        redis_free_socket(redis->sock);
    }

    redis->sock = redis_sock_create(host, host_len, port, timeout, read_timeout, persistent,
        persistent_id, retry_interval, 0);

    if (redis_sock_server_open(redis->sock TSRMLS_CC) < 0) {
        redis_free_socket(redis->sock);
        redis->sock = NULL;
        return FAILURE;
    }

    return SUCCESS;
}

/* {{{ proto long Redis::bitop(string op, string key, ...) */
PHP_METHOD(Redis, bitop)
{
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

    if (redis_sock && redis_sock_disconnect(redis_sock TSRMLS_CC)) {
        RETURN_TRUE;
    }
    RETURN_FALSE;
}
/* }}} */

/* {{{ proto boolean Redis::set(string key, mixed val, long timeout,
 *                              [array opt) */
PHP_METHOD(Redis, set) {
    REDIS_PROCESS_CMD(set, redis_boolean_response);
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
PHP_METHOD(Redis, getSet)
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

/* {{{ proto string Redis::renameKey(string key_src, string key_dst)
 */
PHP_METHOD(Redis, renameKey)
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

/* }}} */

/* {{{ proto string Redis::get(string key)
 */
PHP_METHOD(Redis, get)
{
    REDIS_PROCESS_KW_CMD("GET", redis_key_cmd, redis_string_response);
}
/* }}} */


/* {{{ proto string Redis::ping()
 */
PHP_METHOD(Redis, ping)
{
    REDIS_PROCESS_KW_CMD("PING", redis_empty_cmd, redis_ping_response);
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
    REDIS_PROCESS_KW_CMD("INCRBYFLOAT", redis_key_dbl_cmd,
        redis_bulk_double_response);
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

/* {{{ proto array Redis::getMultiple(array keys)
 */
PHP_METHOD(Redis, getMultiple)
{
    zval *object, *z_args, *z_ele;
    HashTable *hash;
    RedisSock *redis_sock;
    smart_string cmd = {0};
    int arg_count;

    /* Make sure we have proper arguments */
    if(zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), "Oa",
                                    &object, redis_ce, &z_args) == FAILURE) {
        RETURN_FALSE;
    }

    /* We'll need the socket */
    if ((redis_sock = redis_sock_get(object TSRMLS_CC, 0)) == NULL) {
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
        redis_cmd_append_sstr_key(&cmd, zstr->val, zstr->len, redis_sock, NULL);
        zend_string_release(zstr);
    } ZEND_HASH_FOREACH_END();

    /* Kick off our command */
    REDIS_PROCESS_REQUEST(redis_sock, cmd.c, cmd.len);
    IF_ATOMIC() {
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
    REDIS_PROCESS_KW_CMD("EXISTS", redis_key_cmd, redis_1_response);
}
/* }}} */

/* {{{ proto boolean Redis::delete(string key)
 */
PHP_METHOD(Redis, delete)
{
    REDIS_PROCESS_CMD(del, redis_long_response);
}
/* }}} */

PHP_REDIS_API void redis_set_watch(RedisSock *redis_sock)
{
    redis_sock->watching = 1;
}

PHP_REDIS_API void redis_watch_response(INTERNAL_FUNCTION_PARAMETERS,
                                 RedisSock *redis_sock, zval *z_tab, void *ctx)
{
    redis_boolean_response_impl(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock,
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

PHP_REDIS_API void redis_unwatch_response(INTERNAL_FUNCTION_PARAMETERS,
                                   RedisSock *redis_sock, zval *z_tab,
                                   void *ctx)
{
    redis_boolean_response_impl(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock,
        z_tab, ctx, redis_clear_watch);
}

/* {{{ proto boolean Redis::unwatch()
 */
PHP_METHOD(Redis, unwatch)
{
    REDIS_PROCESS_KW_CMD("UNWATCH", redis_empty_cmd, redis_unwatch_response);
}
/* }}} */

/* {{{ proto array Redis::getKeys(string pattern)
 */
PHP_METHOD(Redis, getKeys)
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

PHP_METHOD(Redis, setRange)
{
    REDIS_PROCESS_KW_CMD("SETRANGE", redis_key_long_str_cmd,
        redis_long_response);
}

/* {{{ proto long Redis::getbit(string key, long idx) */
PHP_METHOD(Redis, getBit)
{
    REDIS_PROCESS_KW_CMD("GETBIT", redis_key_long_cmd, redis_long_response);
}
/* }}} */

PHP_METHOD(Redis, setBit)
{
    REDIS_PROCESS_CMD(setbit, redis_long_response);
}

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

/* {{{ proto string Redis::lPOP(string key) */
PHP_METHOD(Redis, lPop)
{
    REDIS_PROCESS_KW_CMD("LPOP", redis_key_cmd, redis_string_response);
}
/* }}} */

/* {{{ proto string Redis::rPOP(string key) */
PHP_METHOD(Redis, rPop)
{
    REDIS_PROCESS_KW_CMD("RPOP", redis_key_cmd, redis_string_response);
}
/* }}} */

/* {{{ proto string Redis::blPop(string key1, string key2, ..., int timeout) */
PHP_METHOD(Redis, blPop)
{
    REDIS_PROCESS_CMD(blpop, redis_sock_read_multibulk_reply);
}
/* }}} */

/* {{{ proto string Redis::brPop(string key1, string key2, ..., int timeout) */
PHP_METHOD(Redis, brPop)
{
    REDIS_PROCESS_CMD(brpop, redis_sock_read_multibulk_reply);
}
/* }}} */


/* {{{ proto int Redis::lSize(string key) */
PHP_METHOD(Redis, lSize)
{
    REDIS_PROCESS_KW_CMD("LLEN", redis_key_cmd, redis_long_response);
}
/* }}} */

/* {{{ proto boolean Redis::lRemove(string list, string value, int count = 0) */
PHP_METHOD(Redis, lRemove)
{
    REDIS_PROCESS_CMD(lrem, redis_long_response);
}
/* }}} */

/* {{{ proto boolean Redis::listTrim(string key , int start , int end) */
PHP_METHOD(Redis, listTrim)
{
    REDIS_PROCESS_KW_CMD("LTRIM", redis_key_long_long_cmd,
        redis_boolean_response);
}
/* }}} */

/* {{{ proto string Redis::lGet(string key , int index) */
PHP_METHOD(Redis, lGet)
{
    REDIS_PROCESS_KW_CMD("LINDEX", redis_key_long_cmd, redis_string_response);
}
/* }}} */

/* {{{ proto array Redis::lGetRange(string key, int start , int end) */
PHP_METHOD(Redis, lGetRange)
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
    REDIS_PROCESS_KW_CMD("SADD", redis_key_arr_cmd, redis_long_response);
} /* }}} */

/* {{{ proto int Redis::sSize(string key) */
PHP_METHOD(Redis, sSize)
{
    REDIS_PROCESS_KW_CMD("SCARD", redis_key_cmd, redis_long_response);
}
/* }}} */

/* {{{ proto boolean Redis::sRemove(string set, string value) */
PHP_METHOD(Redis, sRemove)
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
    if ((redis_sock = redis_sock_get(getThis() TSRMLS_CC, 0)) == NULL ||
       redis_srandmember_cmd(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock,
                             &cmd, &cmd_len, NULL, NULL, &have_count)==FAILURE)
    {
        RETURN_FALSE;
    }

    REDIS_PROCESS_REQUEST(redis_sock, cmd, cmd_len);
    if(have_count) {
        IF_ATOMIC() {
            if(redis_sock_read_multibulk_reply(INTERNAL_FUNCTION_PARAM_PASSTHRU,
                                               redis_sock, NULL, NULL)<0)
            {
                RETURN_FALSE;
            }
        }
        REDIS_PROCESS_RESPONSE(redis_sock_read_multibulk_reply);
    } else {
        IF_ATOMIC() {
            redis_string_response(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock,
                NULL, NULL);
        }
        REDIS_PROCESS_RESPONSE(redis_string_response);
    }
}
/* }}} */

/* {{{ proto boolean Redis::sContains(string set, string value) */
PHP_METHOD(Redis, sContains)
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
/* }}} */

/* {{{ proto array Redis::sInter(string key0, ... string keyN) */
PHP_METHOD(Redis, sInter) {
    REDIS_PROCESS_CMD(sinter, redis_sock_read_multibulk_reply);
}
/* }}} */

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
    if ((redis_sock = redis_sock_get(getThis() TSRMLS_CC, 0)) == NULL ||
       redis_sort_cmd(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock, &have_store,
                      &cmd, &cmd_len, NULL, NULL)==FAILURE)
    {
        RETURN_FALSE;
    }

    REDIS_PROCESS_REQUEST(redis_sock, cmd, cmd_len);
    IF_ATOMIC() {
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
    strlen_t keylen, patternlen, storelen;
    zend_long offset = -1, count = -1;
    int argc = 1; /* SORT key is the simplest SORT command */
    smart_string cmd = {0};

    /* Parse myriad of sort arguments */
    if (zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(),
                                     "Os|s!z!lls", &object, redis_ce, &key,
                                     &keylen, &pattern, &patternlen, &zget,
                                     &offset, &count, &store, &storelen)
                                     == FAILURE)
    {
        RETURN_FALSE;
    }

    /* Ensure we're sorting something, and we can get context */
    if (keylen == 0 || !(redis_sock = redis_sock_get(object TSRMLS_CC, 0)))
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
                redis_cmd_append_sstr(&cmd, zpattern->val, zpattern->len);
                zend_string_release(zpattern);
            } ZEND_HASH_FOREACH_END();
        } else {
            zpattern = zval_get_string(zget);
            redis_cmd_append_sstr(&cmd, "GET", sizeof("GET") - 1);
            redis_cmd_append_sstr(&cmd, zpattern->val, zpattern->len);
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
    IF_ATOMIC() {
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

/* {{{ proto array Redis::setTimeout(string key, int timeout) */
PHP_METHOD(Redis, setTimeout) {
    REDIS_PROCESS_KW_CMD("EXPIRE", redis_key_long_cmd, redis_1_response);
}
/* }}} */

/* {{{ proto bool Redis::pexpire(string key, long ms) */
PHP_METHOD(Redis, pexpire) {
    REDIS_PROCESS_KW_CMD("PEXPIRE", redis_key_long_cmd, redis_1_response);
}
/* }}} */

/* {{{ proto array Redis::expireAt(string key, int timestamp) */
PHP_METHOD(Redis, expireAt) {
    REDIS_PROCESS_KW_CMD("EXPIREAT", redis_key_long_cmd, redis_1_response);
}
/* }}} */

/* {{{ proto array Redis::pexpireAt(string key, int timestamp) */
PHP_METHOD(Redis, pexpireAt) {
    REDIS_PROCESS_KW_CMD("PEXPIREAT", redis_key_long_cmd, redis_1_response);
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

/* {{{ proto bool Redis::flushDB() */
PHP_METHOD(Redis, flushDB)
{
    REDIS_PROCESS_KW_CMD("FLUSHDB", redis_empty_cmd, redis_boolean_response);
}
/* }}} */

/* {{{ proto bool Redis::flushAll() */
PHP_METHOD(Redis, flushAll)
{
    REDIS_PROCESS_KW_CMD("FLUSHALL", redis_empty_cmd, redis_boolean_response);
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

    zval *object;
    RedisSock *redis_sock;
    char *cmd, *opt = NULL;
    strlen_t opt_len;
    int cmd_len;

    if (zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(),
                                     "O|s", &object, redis_ce, &opt, &opt_len)
                                     == FAILURE)
    {
        RETURN_FALSE;
    }

    if ((redis_sock = redis_sock_get(object TSRMLS_CC, 0)) == NULL) {
        RETURN_FALSE;
    }

    /* Build a standalone INFO command or one with an option */
    if (opt != NULL) {
        cmd_len = REDIS_SPPRINTF(&cmd, "INFO", "s", opt, opt_len);
    } else {
        cmd_len = REDIS_SPPRINTF(&cmd, "INFO", "");
    }

    REDIS_PROCESS_REQUEST(redis_sock, cmd, cmd_len);
    IF_ATOMIC() {
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

    if (zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), "Ol",
                                     &object, redis_ce, &dbNumber) == FAILURE) {
        RETURN_FALSE;
    }

    if (dbNumber < 0 || (redis_sock = redis_sock_get(object TSRMLS_CC, 0)) == NULL) {
        RETURN_FALSE;
    }

    redis_sock->dbNumber = dbNumber;
    cmd_len = REDIS_SPPRINTF(&cmd, "SELECT", "d", dbNumber);

    REDIS_PROCESS_REQUEST(redis_sock, cmd, cmd_len);
    IF_ATOMIC() {
        redis_boolean_response(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock,
            NULL, NULL);
    }
    REDIS_PROCESS_RESPONSE(redis_boolean_response);
}
/* }}} */

/* {{{ proto bool Redis::move(string key, long dbindex) */
PHP_METHOD(Redis, move) {
    REDIS_PROCESS_KW_CMD("MOVE", redis_key_long_cmd, redis_1_response);
}
/* }}} */

static
void generic_mset(INTERNAL_FUNCTION_PARAMETERS, char *kw, ResultCallback fun)
{
    RedisSock *redis_sock;
    smart_string cmd = {0};
    zval *object, *z_array;
    HashTable *htargs;
    zend_string *zkey;
    zval *zmem;
    char buf[64];
    size_t keylen;
    ulong idx;

    if (zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), "Oa",
                                     &object, redis_ce, &z_array) == FAILURE)
    {
        RETURN_FALSE;
    }

    /* Make sure we can get our socket, and we were not passed an empty array */
    if ((redis_sock = redis_sock_get(object TSRMLS_CC, 0)) == NULL ||
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
            redis_cmd_append_sstr_key(&cmd, zkey->val, zkey->len, redis_sock, NULL);
        } else {
            keylen = snprintf(buf, sizeof(buf), "%ld", (long)idx);
            redis_cmd_append_sstr_key(&cmd, buf, (strlen_t)keylen, redis_sock, NULL);
        }

        /* Append our value */
        redis_cmd_append_sstr_zval(&cmd, zmem, redis_sock TSRMLS_CC);
    } ZEND_HASH_FOREACH_END();

    REDIS_PROCESS_REQUEST(redis_sock, cmd.c, cmd.len);
    IF_ATOMIC() {
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
    int withscores=0;

    if ((redis_sock = redis_sock_get(getThis() TSRMLS_CC, 0)) == NULL) {
        RETURN_FALSE;
    }

    if(fun(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock, kw, &cmd,
           &cmd_len, &withscores, NULL, NULL)==FAILURE)
    {
        RETURN_FALSE;
    }

    REDIS_PROCESS_REQUEST(redis_sock, cmd, cmd_len);
    if(withscores) {
        IF_ATOMIC() {
            redis_mbulk_reply_zipped_keys_dbl(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock, NULL, NULL);
        }
        REDIS_PROCESS_RESPONSE(redis_mbulk_reply_zipped_keys_dbl);
    } else {
        IF_ATOMIC() {
            if(redis_sock_read_multibulk_reply(INTERNAL_FUNCTION_PARAM_PASSTHRU,
                                               redis_sock, NULL, NULL)<0)
            {
                RETURN_FALSE;
            }
        }
        REDIS_PROCESS_RESPONSE(redis_sock_read_multibulk_reply);
    }
}

/* {{{ proto array Redis::zRange(string key,int start,int end,bool scores=0) */
PHP_METHOD(Redis, zRange)
{
    generic_zrange_cmd(INTERNAL_FUNCTION_PARAM_PASSTHRU, "ZRANGE",
        redis_zrange_cmd);
}

/* {{{ proto array Redis::zRevRange(string k, long s, long e, bool scores=0) */
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

/* {{{ proto long Redis::zDelete(string key, string member) */
PHP_METHOD(Redis, zDelete)
{
    REDIS_PROCESS_KW_CMD("ZREM", redis_key_varval_cmd, redis_long_response);
}
/* }}} */

/* {{{ proto long Redis::zDeleteRangeByScore(string k, string s, string e) */
PHP_METHOD(Redis, zDeleteRangeByScore)
{
    REDIS_PROCESS_KW_CMD("ZREMRANGEBYSCORE", redis_key_str_str_cmd,
        redis_long_response);
}
/* }}} */

/* {{{ proto long Redis::zDeleteRangeByRank(string key, long start, long end) */
PHP_METHOD(Redis, zDeleteRangeByRank)
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

/* zInter */
PHP_METHOD(Redis, zInter) {
    REDIS_PROCESS_KW_CMD("ZINTERSTORE", redis_zinter_cmd, redis_long_response);
}

/* zUnion */
PHP_METHOD(Redis, zUnion) {
    REDIS_PROCESS_KW_CMD("ZUNIONSTORE", redis_zinter_cmd, redis_long_response);
}

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

    if (zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(),
                                     "O|l", &object, redis_ce, &multi_value)
                                     == FAILURE)
    {
        RETURN_FALSE;
    }

    /* if the flag is activated, send the command, the reply will be "QUEUED"
     * or -ERR */
    if ((redis_sock = redis_sock_get(object TSRMLS_CC, 0)) == NULL) {
        RETURN_FALSE;
    }

    if (multi_value == PIPELINE) {
        IF_PIPELINE() {
            php_error_docref(NULL TSRMLS_CC, E_WARNING, "Already in pipeline mode");
        } else IF_MULTI() {
            php_error_docref(NULL TSRMLS_CC, E_ERROR, "Can't activate pipeline in multi mode!");
            RETURN_FALSE;
        } else {
            free_reply_callbacks(redis_sock);
            redis_sock->mode = PIPELINE;
        }
    } else if (multi_value == MULTI) {
        IF_MULTI() {
            php_error_docref(NULL TSRMLS_CC, E_WARNING, "Already in multi mode");
        } else IF_PIPELINE() {
            php_error_docref(NULL TSRMLS_CC, E_ERROR, "Can't activate multi in pipeline mode!");
            RETURN_FALSE;
        } else {
            cmd_len = REDIS_SPPRINTF(&cmd, "MULTI", "");
            SOCKET_WRITE_COMMAND(redis_sock, cmd, cmd_len)
            efree(cmd);

            if ((resp = redis_sock_read(redis_sock, &resp_len TSRMLS_CC)) == NULL) {
                RETURN_FALSE;
            } else if (strncmp(resp, "+OK", 3) != 0) {
                efree(resp);
                RETURN_FALSE;
            }
            efree(resp);
            redis_sock->mode = MULTI;
        }
    } else {
        RETURN_FALSE;
    }
    RETURN_ZVAL(getThis(), 1, 0);
}

/* discard */
PHP_METHOD(Redis, discard)
{
    RedisSock *redis_sock;
    zval *object;

    if (zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), "O",
                                     &object, redis_ce) == FAILURE) {
        RETURN_FALSE;
    }

    if ((redis_sock = redis_sock_get(object TSRMLS_CC, 0)) == NULL) {
        RETURN_FALSE;
    }

    redis_sock->mode = ATOMIC;
    free_reply_callbacks(redis_sock);
    redis_send_discard(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock);
}

/* redis_sock_read_multibulk_multi_reply */
PHP_REDIS_API int redis_sock_read_multibulk_multi_reply(INTERNAL_FUNCTION_PARAMETERS,
                                      RedisSock *redis_sock)
{

    char inbuf[4096];
    int numElems;
    size_t len;

    if (redis_sock_gets(redis_sock, inbuf, sizeof(inbuf) - 1, &len TSRMLS_CC) < 0) {
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

    if (zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(),
                                     "O", &object, redis_ce) == FAILURE ||
        (redis_sock = redis_sock_get(object TSRMLS_CC, 0)) == NULL
    ) {
        RETURN_FALSE;
    }

    IF_MULTI() {
        cmd_len = REDIS_SPPRINTF(&cmd, "EXEC", "");
        SOCKET_WRITE_COMMAND(redis_sock, cmd, cmd_len)
        efree(cmd);

        ret = redis_sock_read_multibulk_multi_reply(
            INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock);
        free_reply_callbacks(redis_sock);
        redis_sock->mode = ATOMIC;
        redis_sock->watching = 0;
        if (ret < 0) {
            zval_dtor(return_value);
            RETURN_FALSE;
        }
    }

    IF_PIPELINE() {
        if (redis_sock->pipeline_cmd == NULL) {
            /* Empty array when no command was run. */
            array_init(return_value);
        } else {
            if (redis_sock_write(redis_sock, redis_sock->pipeline_cmd,
                    redis_sock->pipeline_len TSRMLS_CC) < 0) {
                ZVAL_FALSE(return_value);
            } else {
                array_init(return_value);
                redis_sock_read_multibulk_multi_reply_loop(
                    INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock, return_value, 0);
            }
            efree(redis_sock->pipeline_cmd);
            redis_sock->pipeline_cmd = NULL;
            redis_sock->pipeline_len = 0;
        }
        free_reply_callbacks(redis_sock);
        redis_sock->mode = ATOMIC;
    }
}

PHP_REDIS_API int
redis_response_enqueued(RedisSock *redis_sock TSRMLS_DC)
{
    char *resp;
    int resp_len, ret = FAILURE;

    if ((resp = redis_sock_read(redis_sock, &resp_len TSRMLS_CC)) != NULL) {
        if (strncmp(resp, "+QUEUED", 7) == 0) {
            ret = SUCCESS;
        }
        efree(resp);
    }
    return ret;
}

PHP_REDIS_API int
redis_sock_read_multibulk_multi_reply_loop(INTERNAL_FUNCTION_PARAMETERS,
                                           RedisSock *redis_sock, zval *z_tab,
                                           int numElems)
{
    fold_item *fi;

    for (fi = redis_sock->head; fi; fi = fi->next) {
        fi->fun(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock, z_tab,
            fi->ctx TSRMLS_CC);
    }
    redis_sock->current = fi;
    return 0;
}

PHP_METHOD(Redis, pipeline)
{
    RedisSock *redis_sock;
    zval *object;

    if (zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(),
                                     "O", &object, redis_ce) == FAILURE ||
        (redis_sock = redis_sock_get(object TSRMLS_CC, 0)) == NULL
    ) {
        RETURN_FALSE;
    }

    IF_MULTI() {
        php_error_docref(NULL TSRMLS_CC, E_ERROR,
            "Can't activate pipeline in multi mode!");
        RETURN_FALSE;
    } else IF_PIPELINE() {
       php_error_docref(NULL TSRMLS_CC, E_WARNING,
            "Already in pipeline mode");
    } else {
        /* NB : we keep the function fold, to detect the last function.
         * We need the response format of the n - 1 command. So, we can delete
         * when n > 2, the { 1 .. n - 2} commands */
        free_reply_callbacks(redis_sock);
        redis_sock->mode = PIPELINE;
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
 *     channel_0 => TRUE|FALSE,
 *    channel_1 => TRUE|FALSE,
 *    ...
 *    channel_n => TRUE|FALSE
 * );
 **/

PHP_REDIS_API void generic_unsubscribe_cmd(INTERNAL_FUNCTION_PARAMETERS,
                                    char *unsub_cmd)
{
    zval *object, *array, *data;
    HashTable *arr_hash;
    RedisSock *redis_sock;
    char *cmd = "", *old_cmd = NULL;
    int cmd_len, array_count;

    int i;
    zval z_tab, *z_channel;

    if (zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), "Oa",
                                     &object, redis_ce, &array) == FAILURE) {
        RETURN_FALSE;
    }
    if ((redis_sock = redis_sock_get(object TSRMLS_CC, 0)) == NULL) {
        RETURN_FALSE;
    }

    arr_hash    = Z_ARRVAL_P(array);
    array_count = zend_hash_num_elements(arr_hash);

    if (array_count == 0) {
        RETURN_FALSE;
    }

    ZEND_HASH_FOREACH_VAL(arr_hash, data) {
        ZVAL_DEREF(data);
        if (Z_TYPE_P(data) == IS_STRING) {
            char *old_cmd = NULL;
            if(*cmd) {
                old_cmd = cmd;
            }
            spprintf(&cmd, 0, "%s %s", cmd, Z_STRVAL_P(data));
            if(old_cmd) {
                efree(old_cmd);
            }
        }
    } ZEND_HASH_FOREACH_END();

    old_cmd = cmd;
    cmd_len = spprintf(&cmd, 0, "%s %s\r\n", unsub_cmd, cmd);
    efree(old_cmd);

    SOCKET_WRITE_COMMAND(redis_sock, cmd, cmd_len)
    efree(cmd);

    array_init(return_value);
    for (i = 1; i <= array_count; i++) {
        redis_sock_read_multibulk_reply_zval(
            INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock, &z_tab);

        if (Z_TYPE(z_tab) == IS_ARRAY) {
            if ((z_channel = zend_hash_index_find(Z_ARRVAL(z_tab), 1)) == NULL) {
                RETURN_FALSE;
            }
            add_assoc_bool(return_value, Z_STRVAL_P(z_channel), 1);
        } else {
            //error
            zval_dtor(&z_tab);
            RETURN_FALSE;
        }
        zval_dtor(&z_tab);
    }
}

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
    strlen_t host_len;
    zend_long port = 6379;
    int cmd_len;

    if (zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(),
                                     "O|sl", &object, redis_ce, &host,
                                     &host_len, &port) == FAILURE)
    {
        RETURN_FALSE;
    }
    if (port < 0 || (redis_sock = redis_sock_get(object TSRMLS_CC, 0)) == NULL) {
        RETURN_FALSE;
    }

    if (host && host_len) {
        cmd_len = REDIS_SPPRINTF(&cmd, "SLAVEOF", "sd", host, host_len, (int)port);
    } else {
        cmd_len = REDIS_SPPRINTF(&cmd, "SLAVEOF", "ss", "NO", 2, "ONE", 3);
    }

    REDIS_PROCESS_REQUEST(redis_sock, cmd, cmd_len);
    IF_ATOMIC() {
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

    if ((redis_sock = redis_sock_get(getThis() TSRMLS_CC, 0)) == NULL) {
       RETURN_FALSE;
    }

    if(redis_object_cmd(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock, &rtype,
                        &cmd, &cmd_len, NULL, NULL)==FAILURE)
    {
       RETURN_FALSE;
    }

    REDIS_PROCESS_REQUEST(redis_sock, cmd, cmd_len);

    if(rtype == TYPE_INT) {
        IF_ATOMIC() {
            redis_long_response(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock,
                NULL, NULL);
        }
        REDIS_PROCESS_RESPONSE(redis_long_response);
    } else {
        IF_ATOMIC() {
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

    if ((redis_sock = redis_sock_get_instance(getThis() TSRMLS_CC, 0)) == NULL) {
        RETURN_FALSE;
    }

    redis_getoption_handler(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock, NULL);
}
/* }}} */

/* {{{ proto string Redis::setOption(string $option, mixed $value) */
PHP_METHOD(Redis, setOption)
{
    RedisSock *redis_sock;

    if ((redis_sock = redis_sock_get_instance(getThis() TSRMLS_CC, 0)) == NULL) {
        RETURN_FALSE;
    }

    redis_setoption_handler(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock, NULL);
}
/* }}} */

/* {{{ proto boolean Redis::config(string op, string key [, mixed value]) */
PHP_METHOD(Redis, config)
{
    zval *object;
    RedisSock *redis_sock;
    char *key = NULL, *val = NULL, *cmd, *op = NULL;
    strlen_t key_len, val_len, op_len;
    enum {CFG_GET, CFG_SET} mode;
    int cmd_len;

    if (zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(),
                                     "Oss|s", &object, redis_ce, &op, &op_len,
                                     &key, &key_len, &val, &val_len) == FAILURE)
    {
        RETURN_FALSE;
    }

    /* op must be GET or SET */
    if(strncasecmp(op, "GET", 3) == 0) {
        mode = CFG_GET;
    } else if(strncasecmp(op, "SET", 3) == 0) {
        mode = CFG_SET;
    } else {
        RETURN_FALSE;
    }

    if ((redis_sock = redis_sock_get(object TSRMLS_CC, 0)) == NULL) {
        RETURN_FALSE;
    }

    if (mode == CFG_GET && val == NULL) {
        cmd_len = REDIS_SPPRINTF(&cmd, "CONFIG", "ss", op, op_len, key, key_len);

        REDIS_PROCESS_REQUEST(redis_sock, cmd, cmd_len)
        IF_ATOMIC() {
            redis_mbulk_reply_zipped_raw(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock, NULL, NULL);
        }
        REDIS_PROCESS_RESPONSE(redis_mbulk_reply_zipped_raw);

    } else if(mode == CFG_SET && val != NULL) {
        cmd_len = REDIS_SPPRINTF(&cmd, "CONFIG", "sss", op, op_len, key, key_len, val, val_len);

        REDIS_PROCESS_REQUEST(redis_sock, cmd, cmd_len)
        IF_ATOMIC() {
            redis_boolean_response(
                INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock, NULL, NULL);
        }
        REDIS_PROCESS_RESPONSE(redis_boolean_response);
    } else {
        RETURN_FALSE;
    }
}
/* }}} */


/* {{{ proto boolean Redis::slowlog(string arg, [int option]) */
PHP_METHOD(Redis, slowlog) {
    zval *object;
    RedisSock *redis_sock;
    char *arg, *cmd;
    int cmd_len;
    strlen_t arg_len;
    zend_long option = 0;
    enum {SLOWLOG_GET, SLOWLOG_LEN, SLOWLOG_RESET} mode;

    // Make sure we can get parameters
    if(zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(),
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
    if ((redis_sock = redis_sock_get(object TSRMLS_CC, 0)) == NULL) {
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
    IF_ATOMIC() {
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
    if(zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), "Oll",
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
    if ((redis_sock = redis_sock_get(object TSRMLS_CC, 0)) == NULL) {
        RETURN_FALSE;
    }

    // Construct the command
    cmd_len = REDIS_SPPRINTF(&cmd, "WAIT", "ll", num_slaves, timeout);

    /* Kick it off */
    REDIS_PROCESS_REQUEST(redis_sock, cmd, cmd_len);
    IF_ATOMIC() {
        redis_long_response(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock, NULL,
            NULL);
    }
    REDIS_PROCESS_RESPONSE(redis_long_response);
}

/* Construct a PUBSUB command */
PHP_REDIS_API int
redis_build_pubsub_cmd(RedisSock *redis_sock, char **ret, PUBSUB_TYPE type,
                       zval *arg TSRMLS_DC)
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
            redis_cmd_append_sstr_key(&cmd, zstr->val, zstr->len, redis_sock, NULL);
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
    strlen_t kw_len;
    PUBSUB_TYPE type;
    zval *arg=NULL;

    // Parse arguments
    if(zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(),
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
           zend_hash_num_elements(Z_ARRVAL_P(arg))==0)
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
    if ((redis_sock = redis_sock_get(object TSRMLS_CC, 0)) == NULL) {
        RETURN_FALSE;
    }

    /* Construct our "PUBSUB" command */
    cmd_len = redis_build_pubsub_cmd(redis_sock, &cmd, type, arg TSRMLS_CC);

    REDIS_PROCESS_REQUEST(redis_sock, cmd, cmd_len);

    if(type == PUBSUB_NUMSUB) {
        IF_ATOMIC() {
            if(redis_mbulk_reply_zipped_keys_int(INTERNAL_FUNCTION_PARAM_PASSTHRU,
                                                 redis_sock, NULL, NULL)<0)
            {
                RETURN_FALSE;
            }
        }
        REDIS_PROCESS_RESPONSE(redis_mbulk_reply_zipped_keys_int);
    } else {
        IF_ATOMIC() {
            if(redis_read_variant_reply(INTERNAL_FUNCTION_PARAM_PASSTHRU,
                                        redis_sock, NULL, NULL)<0)
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
    REDIS_PROCESS_KW_CMD("EVAL", redis_eval_cmd, redis_read_variant_reply);
}

/* {{{ proto variant Redis::evalsha(string sha1, [array keys, long num_keys]) */
PHP_METHOD(Redis, evalsha) {
    REDIS_PROCESS_KW_CMD("EVALSHA", redis_eval_cmd, redis_read_variant_reply);
}

PHP_REDIS_API int
redis_build_script_exists_cmd(char **ret, zval *argv, int argc) {
	smart_string cmd = {0};
    zend_string *zstr;
    int i;

    // Start building our command
    REDIS_CMD_INIT_SSTR_STATIC(&cmd, 1 + argc, "SCRIPT");
    redis_cmd_append_sstr(&cmd, "EXISTS", 6);

    for (i = 0; i < argc; i++) {
        zstr = zval_get_string(&argv[i]);
        redis_cmd_append_sstr(&cmd, zstr->val, zstr->len);
        zend_string_release(zstr);
    }

	/* Success */
    *ret = cmd.c;
	return cmd.len;
}

/* {{{ proto status Redis::script('flush')
 * {{{ proto status Redis::script('kill')
 * {{{ proto string Redis::script('load', lua_script)
 * {{{ proto int Reids::script('exists', script_sha1 [, script_sha2, ...])
 */
PHP_METHOD(Redis, script) {
    zval *z_args;
    RedisSock *redis_sock;
    int cmd_len, argc;
    char *cmd;

	/* Attempt to grab our socket */
    if ((redis_sock = redis_sock_get(getThis() TSRMLS_CC, 0)) == NULL) {
		RETURN_FALSE;
	}

	/* Grab the number of arguments */
	argc = ZEND_NUM_ARGS();

	/* Allocate an array big enough to store our arguments */
	z_args = emalloc(argc * sizeof(zval));

	/* Make sure we can grab our arguments, we have a string directive */
	if (zend_get_parameters_array(ht, argc, z_args) == FAILURE ||
	   (argc < 1 || Z_TYPE(z_args[0]) != IS_STRING))
	{
		efree(z_args);
		RETURN_FALSE;
	}

    // Branch based on the directive
    if(!strcasecmp(Z_STRVAL(z_args[0]), "flush") ||
       !strcasecmp(Z_STRVAL(z_args[0]), "kill"))
    {
        // Simple SCRIPT FLUSH, or SCRIPT_KILL command
        cmd_len = REDIS_SPPRINTF(&cmd, "SCRIPT", "s", Z_STRVAL(z_args[0]), Z_STRLEN(z_args[0]));
    } else if(!strcasecmp(Z_STRVAL(z_args[0]), "load")) {
        // Make sure we have a second argument, and it's not empty.  If it is
        // empty, we can just return an empty array (which is what Redis does)
        if(argc < 2 || Z_TYPE(z_args[1]) != IS_STRING ||
           Z_STRLEN(z_args[1]) < 1)
        {
            // Free our args
            efree(z_args);
            RETURN_FALSE;
        }

        // Format our SCRIPT LOAD command
        cmd_len = REDIS_SPPRINTF(&cmd, "SCRIPT", "ss", "LOAD", 4, Z_STRVAL(z_args[1]),
                                 Z_STRLEN(z_args[1]));
	} else if(!strcasecmp(Z_STRVAL(z_args[0]), "exists")) {
		/* Construct our SCRIPT EXISTS command */
		cmd_len = redis_build_script_exists_cmd(&cmd, &(z_args[1]), argc-1);
	} else {
		/* Unknown directive */
		efree(z_args);
		RETURN_FALSE;
	}

	/* Free our alocated arguments */
	efree(z_args);

    // Kick off our request
    REDIS_PROCESS_REQUEST(redis_sock, cmd, cmd_len);
    IF_ATOMIC() {
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
    REDIS_PROCESS_KW_CMD("RESTORE", redis_key_long_val_cmd,
        redis_boolean_response);
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

    if ((redis_sock = redis_sock_get(getThis() TSRMLS_CC, 0)) == NULL) {
        RETURN_FALSE;
    }

    redis_prefix_handler(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock);
}

/* {{{ proto Redis::_serialize(value) */
PHP_METHOD(Redis, _serialize) {
    RedisSock *redis_sock;

    // Grab socket
    if ((redis_sock = redis_sock_get(getThis() TSRMLS_CC, 0)) == NULL) {
        RETURN_FALSE;
    }

    redis_serialize_handler(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock);
}

/* {{{ proto Redis::_unserialize(value) */
PHP_METHOD(Redis, _unserialize) {
    RedisSock *redis_sock;

    // Grab socket
    if ((redis_sock = redis_sock_get(getThis() TSRMLS_CC, 0)) == NULL) {
        RETURN_FALSE;
    }

    redis_unserialize_handler(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock,
        redis_exception_ce);
}

/* {{{ proto Redis::getLastError() */
PHP_METHOD(Redis, getLastError) {
    zval *object;
    RedisSock *redis_sock;

    // Grab our object
    if(zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), "O",
                                    &object, redis_ce) == FAILURE)
    {
        RETURN_FALSE;
    }

    // Grab socket
    if ((redis_sock = redis_sock_get(object TSRMLS_CC, 0)) == NULL) {
        RETURN_FALSE;
    }

	/* Return our last error or NULL if we don't have one */
	if(redis_sock->err != NULL && redis_sock->err_len > 0) {
		RETURN_STRINGL(redis_sock->err, redis_sock->err_len);
	} else {
		RETURN_NULL();
	}
}

/* {{{ proto Redis::clearLastError() */
PHP_METHOD(Redis, clearLastError) {
    zval *object;
    RedisSock *redis_sock;

    // Grab our object
    if(zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), "O",
                                    &object, redis_ce) == FAILURE)
    {
        RETURN_FALSE;
    }
    // Grab socket
    if ((redis_sock = redis_sock_get(object TSRMLS_CC, 0)) == NULL) {
        RETURN_FALSE;
    }

    // Clear error message
    if(redis_sock->err) {
        efree(redis_sock->err);
    }
    redis_sock->err = NULL;

    RETURN_TRUE;
}

/*
 * {{{ proto long Redis::getMode()
 */
PHP_METHOD(Redis, getMode) {
    zval *object;
    RedisSock *redis_sock;

    /* Grab our object */
    if (zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), "O", &object, redis_ce) == FAILURE) {
        RETURN_FALSE;
    }

    /* Grab socket */
    if ((redis_sock = redis_sock_get(object TSRMLS_CC, 0)) == NULL) {
        RETURN_FALSE;
    }

    RETVAL_LONG(redis_sock->mode);
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
    RedisSock *redis_sock;

    if((redis_sock = redis_sock_get_connected(INTERNAL_FUNCTION_PARAM_PASSTHRU))) {
        RETURN_TRUE;
    } else {
        RETURN_FALSE;
    }
}

/* {{{ proto Redis::getHost() */
PHP_METHOD(Redis, getHost) {
    RedisSock *redis_sock;

    if((redis_sock = redis_sock_get_connected(INTERNAL_FUNCTION_PARAM_PASSTHRU))) {
        RETURN_STRING(redis_sock->host);
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

    if((redis_sock = redis_sock_get_connected(INTERNAL_FUNCTION_PARAM_PASSTHRU))) {
        if(redis_sock->persistent_id != NULL) {
            RETURN_STRING(redis_sock->persistent_id);
        } else {
            RETURN_NULL();
        }
    } else {
        RETURN_FALSE;
    }
}

/* {{{ proto Redis::getAuth */
PHP_METHOD(Redis, getAuth) {
    RedisSock *redis_sock;

    if((redis_sock = redis_sock_get_connected(INTERNAL_FUNCTION_PARAM_PASSTHRU))) {
        if(redis_sock->auth != NULL) {
            RETURN_STRING(redis_sock->auth);
        } else {
            RETURN_NULL();
        }
    } else {
        RETURN_FALSE;
    }
}

/*
 * $redis->client('list');
 * $redis->client('kill', <ip:port>);
 * $redis->client('setname', <name>);
 * $redis->client('getname');
 */
PHP_METHOD(Redis, client) {
    zval *object;
    RedisSock *redis_sock;
    char *cmd, *opt=NULL, *arg=NULL;
    strlen_t opt_len, arg_len;
    int cmd_len;

    // Parse our method parameters
    if(zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(),
                                    "Os|s", &object, redis_ce, &opt, &opt_len,
                                    &arg, &arg_len) == FAILURE)
    {
        RETURN_FALSE;
    }

    /* Grab our socket */
    if ((redis_sock = redis_sock_get(object TSRMLS_CC, 0)) == NULL) {
        RETURN_FALSE;
    }

    /* Build our CLIENT command */
    if (ZEND_NUM_ARGS() == 2) {
        cmd_len = REDIS_SPPRINTF(&cmd, "CLIENT", "ss", opt, opt_len, arg, arg_len);
    } else {
        cmd_len = REDIS_SPPRINTF(&cmd, "CLIENT", "s", opt, opt_len);
    }

    /* Execute our queue command */
    REDIS_PROCESS_REQUEST(redis_sock, cmd, cmd_len);

    /* We handle CLIENT LIST with a custom response function */
    if(!strncasecmp(opt, "list", 4)) {
        IF_ATOMIC() {
            redis_client_list_reply(INTERNAL_FUNCTION_PARAM_PASSTHRU,redis_sock,
                NULL);
        }
        REDIS_PROCESS_RESPONSE(redis_client_list_reply);
    } else {
        IF_ATOMIC() {
            redis_read_variant_reply(INTERNAL_FUNCTION_PARAM_PASSTHRU,
                redis_sock,NULL,NULL);
        }
        REDIS_PROCESS_RESPONSE(redis_read_variant_reply);
    }
}

/* {{{ proto mixed Redis::rawcommand(string $command, [ $arg1 ... $argN]) */
PHP_METHOD(Redis, rawcommand) {
    int argc = ZEND_NUM_ARGS(), cmd_len;
    char *cmd = NULL;
    RedisSock *redis_sock;
    zval *z_args;

    /* Sanity check on arguments */
    if (argc < 1) {
        php_error_docref(NULL TSRMLS_CC, E_WARNING,
            "Must pass at least one command keyword");
        RETURN_FALSE;
    }
    z_args = emalloc(argc * sizeof(zval));
    if (zend_get_parameters_array(ht, argc, z_args) == FAILURE) {
        php_error_docref(NULL TSRMLS_CC, E_WARNING,
            "Internal PHP error parsing arguments");
        efree(z_args);
        RETURN_FALSE;
    } else if (redis_build_raw_cmd(z_args, argc, &cmd, &cmd_len TSRMLS_CC) < 0 ||
               (redis_sock = redis_sock_get(getThis() TSRMLS_CC, 0)) == NULL
    ) {
        if (cmd) efree(cmd);
        efree(z_args);
        RETURN_FALSE;
    }

    /* Clean up command array */
    efree(z_args);

    /* Execute our command */
    REDIS_PROCESS_REQUEST(redis_sock, cmd, cmd_len);
    IF_ATOMIC() {
        redis_read_variant_reply(INTERNAL_FUNCTION_PARAM_PASSTHRU,redis_sock,NULL,NULL);
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

/* Helper to format any combination of SCAN arguments */
PHP_REDIS_API int
redis_build_scan_cmd(char **cmd, REDIS_SCAN_TYPE type, char *key, int key_len,
                     int iter, char *pattern, int pattern_len, int count)
{
    smart_string cmdstr = {0};
    char *keyword;
    int argc;

    /* Count our arguments +1 for key if it's got one, and + 2 for pattern */
    /* or count given that they each carry keywords with them. */
    argc = 1 + (key_len > 0) + (pattern_len > 0 ? 2 : 0) + (count > 0 ? 2 : 0);

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
    redis_cmd_append_sstr_int(&cmdstr, iter);

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

    /* Return our command length */
    *cmd = cmdstr.c;
    return cmdstr.len;
}

/* {{{ proto redis::scan(&$iterator, [pattern, [count]]) */
PHP_REDIS_API void
generic_scan_cmd(INTERNAL_FUNCTION_PARAMETERS, REDIS_SCAN_TYPE type) {
    zval *object, *z_iter;
    RedisSock *redis_sock;
    HashTable *hash;
    char *pattern=NULL, *cmd, *key=NULL;
    int cmd_len, num_elements, key_free=0;
    strlen_t key_len = 0, pattern_len = 0;
    zend_long count=0, iter;

    /* Different prototype depending on if this is a key based scan */
    if(type != TYPE_SCAN) {
        // Requires a key
        if(zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(),
                                        "Osz/|s!l", &object, redis_ce, &key,
                                        &key_len, &z_iter, &pattern,
                                        &pattern_len, &count)==FAILURE)
        {
            RETURN_FALSE;
        }
    } else {
        // Doesn't require a key
        if(zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(),
                                        "Oz/|s!l", &object, redis_ce, &z_iter,
                                        &pattern, &pattern_len, &count)
                                        == FAILURE)
        {
            RETURN_FALSE;
        }
    }

    /* Grab our socket */
    if ((redis_sock = redis_sock_get(object TSRMLS_CC, 0)) == NULL) {
        RETURN_FALSE;
    }

    /* Calling this in a pipeline makes no sense */
    IF_NOT_ATOMIC() {
        php_error_docref(NULL TSRMLS_CC, E_ERROR,
            "Can't call SCAN commands in multi or pipeline mode!");
        RETURN_FALSE;
    }

    // The iterator should be passed in as NULL for the first iteration, but we
    // can treat any NON LONG value as NULL for these purposes as we've
    // seperated the variable anyway.
    if(Z_TYPE_P(z_iter) != IS_LONG || Z_LVAL_P(z_iter)<0) {
        /* Convert to long */
        convert_to_long(z_iter);
        iter = 0;
    } else if(Z_LVAL_P(z_iter)!=0) {
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
        cmd_len = redis_build_scan_cmd(&cmd, type, key, key_len, (int)iter,
                                   pattern, pattern_len, count);

        /* Execute our command getting our new iterator value */
        REDIS_PROCESS_REQUEST(redis_sock, cmd, cmd_len);
        if(redis_sock_read_scan_reply(INTERNAL_FUNCTION_PARAM_PASSTHRU,
                                      redis_sock,type,&iter)<0)
        {
            if(key_free) efree(key);
            RETURN_FALSE;
        }

        /* Get the number of elements */
        hash = Z_ARRVAL_P(return_value);
        num_elements = zend_hash_num_elements(hash);
    } while(redis_sock->scan == REDIS_SCAN_RETRY && iter != 0 &&
            num_elements == 0);

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
    REDIS_PROCESS_CMD(georadius, redis_read_variant_reply);
}

PHP_METHOD(Redis, georadiusbymember) {
    REDIS_PROCESS_CMD(georadiusbymember, redis_read_variant_reply);
}

/* vim: set tabstop=4 softtabstop=4 expandtab shiftwidth=4: */
