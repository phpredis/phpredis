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
#include "php_ini.h"
#include "php_redis.h"
#include "redis_array.h"
#include <zend_exceptions.h>

#ifdef PHP_SESSION
#include "ext/session/php_session.h"
#endif

#include <ext/standard/php_smart_str.h>
#include <ext/standard/php_var.h>
#include <ext/standard/php_math.h>

#include "library.h"

#define R_SUB_CALLBACK_CLASS_TYPE 1
#define R_SUB_CALLBACK_FT_TYPE 2
#define R_SUB_CLOSURE_TYPE 3

int le_redis_sock;
extern int le_redis_array;

#ifdef PHP_SESSION
extern ps_module ps_mod_redis;
#endif

extern zend_class_entry *redis_array_ce;
zend_class_entry *redis_ce;
zend_class_entry *redis_exception_ce;
zend_class_entry *spl_ce_RuntimeException = NULL;

extern zend_function_entry redis_array_functions[];

PHP_INI_BEGIN()
	/* redis arrays */
	PHP_INI_ENTRY("redis.arrays.names", "", PHP_INI_ALL, NULL)
	PHP_INI_ENTRY("redis.arrays.hosts", "", PHP_INI_ALL, NULL)
	PHP_INI_ENTRY("redis.arrays.previous", "", PHP_INI_ALL, NULL)
	PHP_INI_ENTRY("redis.arrays.functions", "", PHP_INI_ALL, NULL)
	PHP_INI_ENTRY("redis.arrays.index", "", PHP_INI_ALL, NULL)
	PHP_INI_ENTRY("redis.arrays.autorehash", "", PHP_INI_ALL, NULL)
PHP_INI_END()

ZEND_DECLARE_MODULE_GLOBALS(redis)

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
     PHP_ME(Redis, resetStat, NULL, ZEND_ACC_PUBLIC)
     PHP_ME(Redis, select, NULL, ZEND_ACC_PUBLIC)
     PHP_ME(Redis, move, NULL, ZEND_ACC_PUBLIC)
     PHP_ME(Redis, bgrewriteaof, NULL, ZEND_ACC_PUBLIC)
     PHP_ME(Redis, slaveof, NULL, ZEND_ACC_PUBLIC)
     PHP_ME(Redis, object, NULL, ZEND_ACC_PUBLIC)
     PHP_ME(Redis, bitop, NULL, ZEND_ACC_PUBLIC)
     PHP_ME(Redis, bitcount, NULL, ZEND_ACC_PUBLIC)

     /* 1.1 */
     PHP_ME(Redis, mset, NULL, ZEND_ACC_PUBLIC)
     PHP_ME(Redis, msetnx, NULL, ZEND_ACC_PUBLIC)
     PHP_ME(Redis, rpoplpush, NULL, ZEND_ACC_PUBLIC)
     PHP_ME(Redis, brpoplpush, NULL, ZEND_ACC_PUBLIC)
     PHP_ME(Redis, zAdd, NULL, ZEND_ACC_PUBLIC)
     PHP_ME(Redis, zDelete, NULL, ZEND_ACC_PUBLIC)
     PHP_ME(Redis, zRange, NULL, ZEND_ACC_PUBLIC)
     PHP_ME(Redis, zReverseRange, NULL, ZEND_ACC_PUBLIC)
     PHP_ME(Redis, zRangeByScore, NULL, ZEND_ACC_PUBLIC)
     PHP_ME(Redis, zRevRangeByScore, NULL, ZEND_ACC_PUBLIC)
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

     PHP_ME(Redis, eval, NULL, ZEND_ACC_PUBLIC)
     PHP_ME(Redis, evalsha, NULL, ZEND_ACC_PUBLIC)
     PHP_ME(Redis, script, NULL, ZEND_ACC_PUBLIC)

     PHP_ME(Redis, dump, NULL, ZEND_ACC_PUBLIC)
     PHP_ME(Redis, restore, NULL, ZEND_ACC_PUBLIC)
     PHP_ME(Redis, migrate, NULL, ZEND_ACC_PUBLIC)

     PHP_ME(Redis, getLastError, NULL, ZEND_ACC_PUBLIC)
     PHP_ME(Redis, clearLastError, NULL, ZEND_ACC_PUBLIC)

     PHP_ME(Redis, _prefix, NULL, ZEND_ACC_PUBLIC)
     PHP_ME(Redis, _unserialize, NULL, ZEND_ACC_PUBLIC)

     PHP_ME(Redis, client, NULL, ZEND_ACC_PUBLIC)

     /* options */
     PHP_ME(Redis, getOption, NULL, ZEND_ACC_PUBLIC)
     PHP_ME(Redis, setOption, NULL, ZEND_ACC_PUBLIC)

     /* config */
     PHP_ME(Redis, config, NULL, ZEND_ACC_PUBLIC)

     /* slowlog */
     PHP_ME(Redis, slowlog, NULL, ZEND_ACC_PUBLIC)

     /* introspection */
     PHP_ME(Redis, getHost, NULL, ZEND_ACC_PUBLIC)
     PHP_ME(Redis, getPort, NULL, ZEND_ACC_PUBLIC)
     PHP_ME(Redis, getDBNum, NULL, ZEND_ACC_PUBLIC)
     PHP_ME(Redis, getTimeout, NULL, ZEND_ACC_PUBLIC)
     PHP_ME(Redis, getReadTimeout, NULL, ZEND_ACC_PUBLIC)
     PHP_ME(Redis, getPersistentID, NULL, ZEND_ACC_PUBLIC)
     PHP_ME(Redis, getAuth, NULL, ZEND_ACC_PUBLIC)
     PHP_ME(Redis, isConnected, NULL, ZEND_ACC_PUBLIC)

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
     PHP_MALIAS(Redis, zrevrange, zReverseRange, NULL, ZEND_ACC_PUBLIC)
     
     PHP_MALIAS(Redis, sendEcho, echo, NULL, ZEND_ACC_PUBLIC) 

     PHP_MALIAS(Redis, evaluate, eval, NULL, ZEND_ACC_PUBLIC)
     PHP_MALIAS(Redis, evaluateSha, evalsha, NULL, ZEND_ACC_PUBLIC)
     {NULL, NULL, NULL}
};

zend_module_entry redis_module_entry = {
#if ZEND_MODULE_API_NO >= 20010901
     STANDARD_MODULE_HEADER,
#endif
     "redis",
     NULL,
     PHP_MINIT(redis),
     PHP_MSHUTDOWN(redis),
     PHP_RINIT(redis),
     PHP_RSHUTDOWN(redis),
     PHP_MINFO(redis),
#if ZEND_MODULE_API_NO >= 20010901
     PHP_REDIS_VERSION,
#endif
     STANDARD_MODULE_PROPERTIES
};

#ifdef COMPILE_DL_REDIS
ZEND_GET_MODULE(redis)
#endif

PHPAPI zend_class_entry *redis_get_exception_base(int root TSRMLS_DC)
{
#if HAVE_SPL
        if (!root) {
                if (!spl_ce_RuntimeException) {
                        zend_class_entry **pce;

                        if (zend_hash_find(CG(class_table), "runtimeexception",
                                                           sizeof("RuntimeException"), (void **) &pce) == SUCCESS) {
                                spl_ce_RuntimeException = *pce;
                                return *pce;
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

/**
 * Send a static DISCARD in case we're in MULTI mode.
 */
static int send_discard_static(RedisSock *redis_sock TSRMLS_DC) {

	int result = FAILURE;
	char *cmd, *response;
   	int response_len, cmd_len;

   	/* format our discard command */
   	cmd_len = redis_cmd_format_static(&cmd, "DISCARD", "");

   	/* send our DISCARD command */
   	if (redis_sock_write(redis_sock, cmd, cmd_len TSRMLS_CC) >= 0 &&
   	   (response = redis_sock_read(redis_sock, &response_len TSRMLS_CC)) != NULL) {

   		/* success if we get OK */
   		result = (response_len == 3 && strncmp(response,"+OK", 3) == 0) ? SUCCESS : FAILURE;

   		/* free our response */
   		efree(response);
   	}

   	/* free our command */
   	efree(cmd);

   	/* return success/failure */
   	return result;
}

/**
 * redis_destructor_redis_sock
 */
static void redis_destructor_redis_sock(zend_rsrc_list_entry * rsrc TSRMLS_DC)
{
    RedisSock *redis_sock = (RedisSock *) rsrc->ptr;
    redis_sock_disconnect(redis_sock TSRMLS_CC);
    redis_free_socket(redis_sock);
}
/**
 * redis_sock_get
 */
PHPAPI int redis_sock_get(zval *id, RedisSock **redis_sock TSRMLS_DC, int no_throw)
{

    zval **socket;
    int resource_type;

    if (Z_TYPE_P(id) != IS_OBJECT || zend_hash_find(Z_OBJPROP_P(id), "socket",
                                  sizeof("socket"), (void **) &socket) == FAILURE) {
    	// Throw an exception unless we've been requested not to
        if(!no_throw) {
        	zend_throw_exception(redis_exception_ce, "Redis server went away", 0 TSRMLS_CC);
        }
        return -1;
    }

    *redis_sock = (RedisSock *) zend_list_find(Z_LVAL_PP(socket), &resource_type);

    if (!*redis_sock || resource_type != le_redis_sock) {
		// Throw an exception unless we've been requested not to
    	if(!no_throw) {
    		zend_throw_exception(redis_exception_ce, "Redis server went away", 0 TSRMLS_CC);
    	}
		return -1;
    }
    if ((*redis_sock)->lazy_connect)
    {
        (*redis_sock)->lazy_connect = 0;
        if (redis_sock_server_open(*redis_sock, 1 TSRMLS_CC) < 0) {
            return -1;
        }
    }

    return Z_LVAL_PP(socket);
}

/**
 * redis_sock_get_direct
 * Returns our attached RedisSock pointer if we're connected
 */
PHPAPI RedisSock *redis_sock_get_connected(INTERNAL_FUNCTION_PARAMETERS) {
    zval *object;
    RedisSock *redis_sock;

    // If we can't grab our object, or get a socket, or we're not connected, return NULL
    if((zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), "O", &object, redis_ce) == FAILURE) ||
       (redis_sock_get(object, &redis_sock TSRMLS_CC, 1) < 0) || redis_sock->status != REDIS_SOCK_STATUS_CONNECTED)
    {
        return NULL;
    }

    // Return our socket
    return redis_sock;
}


/**
 * PHP_MINIT_FUNCTION
 */
PHP_MINIT_FUNCTION(redis)
{
    zend_class_entry redis_class_entry;
    zend_class_entry redis_array_class_entry;
    zend_class_entry redis_exception_class_entry;

	REGISTER_INI_ENTRIES();
    
	/* Redis class */
	INIT_CLASS_ENTRY(redis_class_entry, "Redis", redis_functions);
    redis_ce = zend_register_internal_class(&redis_class_entry TSRMLS_CC);

	/* RedisArray class */
	INIT_CLASS_ENTRY(redis_array_class_entry, "RedisArray", redis_array_functions);
    redis_array_ce = zend_register_internal_class(&redis_array_class_entry TSRMLS_CC);

    le_redis_array = zend_register_list_destructors_ex(
        redis_destructor_redis_array,
        NULL,
        "Redis Array", module_number
    );

	/* RedisException class */
    INIT_CLASS_ENTRY(redis_exception_class_entry, "RedisException", NULL);
    redis_exception_ce = zend_register_internal_class_ex(
        &redis_exception_class_entry,
        redis_get_exception_base(0 TSRMLS_CC),
        NULL TSRMLS_CC
    );

    le_redis_sock = zend_register_list_destructors_ex(
        redis_destructor_redis_sock,
        NULL,
        redis_sock_name, module_number
    );

	add_constant_long(redis_ce, "REDIS_NOT_FOUND", REDIS_NOT_FOUND);
	add_constant_long(redis_ce, "REDIS_STRING", REDIS_STRING);
	add_constant_long(redis_ce, "REDIS_SET", REDIS_SET);
	add_constant_long(redis_ce, "REDIS_LIST", REDIS_LIST);
	add_constant_long(redis_ce, "REDIS_ZSET", REDIS_ZSET);
	add_constant_long(redis_ce, "REDIS_HASH", REDIS_HASH);

	add_constant_long(redis_ce, "ATOMIC", ATOMIC);
	add_constant_long(redis_ce, "MULTI", MULTI);
	add_constant_long(redis_ce, "PIPELINE", PIPELINE);

    /* options */
    add_constant_long(redis_ce, "OPT_SERIALIZER", REDIS_OPT_SERIALIZER);
    add_constant_long(redis_ce, "OPT_PREFIX", REDIS_OPT_PREFIX);
    add_constant_long(redis_ce, "OPT_READ_TIMEOUT", REDIS_OPT_READ_TIMEOUT);

    /* serializer */
    add_constant_long(redis_ce, "SERIALIZER_NONE", REDIS_SERIALIZER_NONE);
    add_constant_long(redis_ce, "SERIALIZER_PHP", REDIS_SERIALIZER_PHP);
#ifdef HAVE_REDIS_IGBINARY
    add_constant_long(redis_ce, "SERIALIZER_IGBINARY", REDIS_SERIALIZER_IGBINARY);
#endif

	zend_declare_class_constant_stringl(redis_ce, "AFTER", 5, "after", 5 TSRMLS_CC);
	zend_declare_class_constant_stringl(redis_ce, "BEFORE", 6, "before", 6 TSRMLS_CC);

#ifdef PHP_SESSION
    /* declare session handler */
    php_session_register_module(&ps_mod_redis);
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
 * PHP_RINIT_FUNCTION
 */
PHP_RINIT_FUNCTION(redis)
{
    return SUCCESS;
}

/**
 * PHP_RSHUTDOWN_FUNCTION
 */
PHP_RSHUTDOWN_FUNCTION(redis)
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
	if (redis_sock_get(getThis(), &redis_sock TSRMLS_CC, 1) < 0) {
		RETURN_FALSE;
	}

	// If we think we're in MULTI mode, send a discard
	if(redis_sock->mode == MULTI) {
		// Discard any multi commands, and free any callbacks that have been queued
		send_discard_static(redis_sock TSRMLS_CC);
		free_reply_callbacks(getThis(), redis_sock);
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
		/* reset multi/exec state if there is one. */
		RedisSock *redis_sock;
		if (redis_sock_get(getThis(), &redis_sock TSRMLS_CC, 0) < 0) {
			RETURN_FALSE;
		}

		RETURN_TRUE;
	}
}
/* }}} */

PHPAPI int redis_connect(INTERNAL_FUNCTION_PARAMETERS, int persistent) {
	zval *object;
	zval **socket;
	int host_len, id;
	char *host = NULL;
	long port = -1;
	long retry_interval = 0;

	char *persistent_id = NULL;
	int persistent_id_len = -1;
	
	double timeout = 0.0;
	RedisSock *redis_sock  = NULL;

#ifdef ZTS
	/* not sure how in threaded mode this works so disabled persistents at first */
    persistent = 0;
#endif

	if (zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), "Os|ldsl",
				&object, redis_ce, &host, &host_len, &port,
				&timeout, &persistent_id, &persistent_id_len,
				&retry_interval) == FAILURE) {
		return FAILURE;
	}

	if (timeout < 0L || timeout > INT_MAX) {
		zend_throw_exception(redis_exception_ce, "Invalid timeout", 0 TSRMLS_CC);
		return FAILURE;
	}

	if (retry_interval < 0L || retry_interval > INT_MAX) {
		zend_throw_exception(redis_exception_ce, "Invalid retry interval", 0 TSRMLS_CC);
		return FAILURE;
	}

	if(port == -1 && host_len && host[0] != '/') { /* not unix socket, set to default value */
		port = 6379;
	}

	/* if there is a redis sock already we have to remove it from the list */
	if (redis_sock_get(object, &redis_sock TSRMLS_CC, 1) > 0) {
		if (zend_hash_find(Z_OBJPROP_P(object), "socket",
					sizeof("socket"), (void **) &socket) == FAILURE)
		{
			/* maybe there is a socket but the id isn't known.. what to do? */
		} else {
			zend_list_delete(Z_LVAL_PP(socket)); /* the refcount should be decreased and the detructor called */
		}
	}

	redis_sock = redis_sock_create(host, host_len, port, timeout, persistent, persistent_id, retry_interval, 0);

	if (redis_sock_server_open(redis_sock, 1 TSRMLS_CC) < 0) {
		redis_free_socket(redis_sock);
		return FAILURE;
	}

#if PHP_VERSION_ID >= 50400
	id = zend_list_insert(redis_sock, le_redis_sock TSRMLS_CC);
#else
	id = zend_list_insert(redis_sock, le_redis_sock);
#endif
	add_property_resource(object, "socket", id);

	return SUCCESS;
}

/* {{{ proto boolean Redis::bitop(string op, string key, ...)
 */
PHP_METHOD(Redis, bitop)
{
    char *cmd;
    int cmd_len;

	zval **z_args;
	char **keys;
	int *keys_len;
	int argc = ZEND_NUM_ARGS(), i;
    RedisSock *redis_sock = NULL;
    smart_str buf = {0};
	int key_free = 0;

	/* get redis socket */
    if (redis_sock_get(getThis(), &redis_sock TSRMLS_CC, 0) < 0) {
        RETURN_FALSE;
    }

	/* fetch args */
    z_args = emalloc(argc * sizeof(zval*));
    if(zend_get_parameters_array(ht, argc, z_args) == FAILURE
			|| argc < 3 /* 3 args min. */
			|| Z_TYPE_P(z_args[0]) != IS_STRING /* operation must be a string. */
			) {
        efree(z_args);
		RETURN_FALSE;
    }


	keys = emalloc(argc * sizeof(char*));
	keys_len = emalloc(argc * sizeof(int));

	/* prefix keys */
	for(i = 0; i < argc; ++i) {
		convert_to_string(z_args[i]);

		keys[i] = Z_STRVAL_P(z_args[i]);
		keys_len[i] = Z_STRLEN_P(z_args[i]);
		if(i != 0)
			key_free = redis_key_prefix(redis_sock, &keys[i], &keys_len[i] TSRMLS_CC);
	}

	/* start building the command */
	smart_str_appendc(&buf, '*');
	smart_str_append_long(&buf, argc + 1); /* +1 for BITOP command */
	smart_str_appendl(&buf, _NL, sizeof(_NL) - 1);

	/* add command name */
	smart_str_appendc(&buf, '$');
	smart_str_append_long(&buf, 5);
	smart_str_appendl(&buf, _NL, sizeof(_NL) - 1);
	smart_str_appendl(&buf, "BITOP", 5);
	smart_str_appendl(&buf, _NL, sizeof(_NL) - 1);

	/* add keys */
	for(i = 0; i < argc; ++i) {
		smart_str_appendc(&buf, '$');
		smart_str_append_long(&buf, keys_len[i]);
		smart_str_appendl(&buf, _NL, sizeof(_NL) - 1);
		smart_str_appendl(&buf, keys[i], keys_len[i]);
		smart_str_appendl(&buf, _NL, sizeof(_NL) - 1);
	}
	/* end string */
	smart_str_0(&buf);
	cmd = buf.c;
	cmd_len = buf.len;

	/* cleanup */
    if(key_free)
	for(i = 1; i < argc; ++i) {
		efree(keys[i]);
	}
	efree(keys);
	efree(keys_len);
	efree(z_args);

	/* send */
	REDIS_PROCESS_REQUEST(redis_sock, cmd, cmd_len);
	IF_ATOMIC() {
		redis_long_response(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock, NULL, NULL);
	}
	REDIS_PROCESS_RESPONSE(redis_long_response);
}
/* }}} */

/* {{{ proto boolean Redis::bitcount(string key, [int start], [int end])
 */
PHP_METHOD(Redis, bitcount)
{
    zval *object;
    RedisSock *redis_sock;
    char *key = NULL, *cmd;
    int key_len, cmd_len, key_free;
    long start = 0, end = -1;

    if (zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), "Os|ll",
                                     &object, redis_ce,
                                     &key, &key_len, &start, &end) == FAILURE) {
        RETURN_FALSE;
    }

    if (redis_sock_get(object, &redis_sock TSRMLS_CC, 0) < 0) {
        RETURN_FALSE;
    }

	/* BITCOUNT key start end */
	key_free = redis_key_prefix(redis_sock, &key, &key_len TSRMLS_CC);
    cmd_len = redis_cmd_format_static(&cmd, "BITCOUNT", "sdd", key, key_len, (int)start, (int)end);
	if(key_free) efree(key);

	REDIS_PROCESS_REQUEST(redis_sock, cmd, cmd_len);
	IF_ATOMIC() {
		redis_long_response(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock, NULL, NULL);
	}
	REDIS_PROCESS_RESPONSE(redis_long_response);

}
/* }}} */

/* {{{ proto boolean Redis::close()
 */
PHP_METHOD(Redis, close)
{
    zval *object;
    RedisSock *redis_sock = NULL;

    if (zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), "O",
        &object, redis_ce) == FAILURE) {
        RETURN_FALSE;
    }

    if (redis_sock_get(object, &redis_sock TSRMLS_CC, 0) < 0) {
        RETURN_FALSE;
    }

    if (redis_sock_disconnect(redis_sock TSRMLS_CC)) {
        RETURN_TRUE;
    }

    RETURN_FALSE;
}
/* }}} */

/* {{{ proto boolean Redis::set(string key, mixed value, long timeout | array options) */
PHP_METHOD(Redis, set) {
    zval *object;
    RedisSock *redis_sock;
    char *key = NULL, *val = NULL, *cmd, *exp_type = NULL, *set_type = NULL;
    int key_len, val_len, cmd_len;
    long expire = -1;
    int val_free = 0, key_free = 0;
    zval *z_value, *z_opts = NULL;

    // Make sure the arguments are correct
    if(zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), "Osz|z",
                                    &object, redis_ce, &key, &key_len, &z_value,
                                    &z_opts) == FAILURE)
    {
        RETURN_FALSE;
    }

    // Ensure we can grab our redis socket
    if(redis_sock_get(object, &redis_sock TSRMLS_CC, 0) < 0) {
        RETURN_FALSE;
    }

    /* Our optional argument can either be a long (to support legacy SETEX */
    /* redirection), or an array with Redis >= 2.6.12 set options */
    if(z_opts && Z_TYPE_P(z_opts) != IS_LONG && Z_TYPE_P(z_opts) != IS_ARRAY
       && Z_TYPE_P(z_opts) != IS_NULL) 
    {
        RETURN_FALSE;
    }

    /* Serialization, key prefixing */
    val_free = redis_serialize(redis_sock, z_value, &val, &val_len TSRMLS_CC);
    key_free = redis_key_prefix(redis_sock, &key, &key_len TSRMLS_CC);

    if(z_opts && Z_TYPE_P(z_opts) == IS_ARRAY) {
        HashTable *kt = Z_ARRVAL_P(z_opts);
        int type;
        unsigned int ht_key_len;
        unsigned long idx;
        char *k;
        zval **v;

        /* Iterate our option array */
        for(zend_hash_internal_pointer_reset(kt);
            zend_hash_has_more_elements(kt) == SUCCESS;
            zend_hash_move_forward(kt))
        {
            // Grab key and value
            type = zend_hash_get_current_key_ex(kt, &k, &ht_key_len, &idx, 0, NULL);
            zend_hash_get_current_data(kt, (void**)&v);

            if(type == HASH_KEY_IS_STRING && (Z_TYPE_PP(v) == IS_LONG) &&
               (Z_LVAL_PP(v) > 0) && IS_EX_PX_ARG(k))
            {
                exp_type = k;
                expire = Z_LVAL_PP(v);
            } else if(Z_TYPE_PP(v) == IS_STRING && IS_NX_XX_ARG(Z_STRVAL_PP(v))) {
                set_type = Z_STRVAL_PP(v);
            }
        }
    } else if(z_opts && Z_TYPE_P(z_opts) == IS_LONG) {
        expire = Z_LVAL_P(z_opts);
    }

    /* Now let's construct the command we want */
    if(exp_type && set_type) {
        /* SET <key> <value> NX|XX PX|EX <timeout> */
        cmd_len = redis_cmd_format_static(&cmd, "SET", "ssssl", key, key_len,
                                          val, val_len, set_type, 2, exp_type,
                                          2, expire);
    } else if(exp_type) {
        /* SET <key> <value> PX|EX <timeout> */
        cmd_len = redis_cmd_format_static(&cmd, "SET", "sssl", key, key_len,
                                          val, val_len, exp_type, 2, expire);
    } else if(set_type) {
        /* SET <key> <value> NX|XX */
        cmd_len = redis_cmd_format_static(&cmd, "SET", "sss", key, key_len,
                                          val, val_len, set_type, 2);
    } else if(expire > 0) {
        /* Backward compatible SETEX redirection */
        cmd_len = redis_cmd_format_static(&cmd, "SETEX", "sls", key, key_len,
                                          expire, val, val_len);
    } else {
        /* SET <key> <value> */
        cmd_len = redis_cmd_format_static(&cmd, "SET", "ss", key, key_len,
                                          val, val_len);
    }

    /* Free our key or value if we prefixed/serialized */
    if(key_free) efree(key);
    if(val_free) efree(val);

    /* Kick off the command */
    REDIS_PROCESS_REQUEST(redis_sock, cmd, cmd_len);
    IF_ATOMIC() {
        redis_boolean_response(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock, NULL, NULL);
    }
    REDIS_PROCESS_RESPONSE(redis_boolean_response);
}

PHPAPI void redis_generic_setex(INTERNAL_FUNCTION_PARAMETERS, char *keyword) {

    zval *object;
    RedisSock *redis_sock;
    char *key = NULL, *val = NULL, *cmd;
    int key_len, val_len, cmd_len;
    long expire;
    int val_free = 0, key_free = 0;
    zval *z_value;

    if (zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), "Oslz",
                                     &object, redis_ce, &key, &key_len,
                                     &expire, &z_value) == FAILURE) {
        RETURN_FALSE;
    }

    if (redis_sock_get(object, &redis_sock TSRMLS_CC, 0) < 0) {
        RETURN_FALSE;
    }

    val_free = redis_serialize(redis_sock, z_value, &val, &val_len TSRMLS_CC);
	key_free = redis_key_prefix(redis_sock, &key, &key_len TSRMLS_CC);
    cmd_len = redis_cmd_format_static(&cmd, keyword, "sls", key, key_len, expire, val, val_len);
    if(val_free) efree(val);
    if(key_free) efree(key);

	REDIS_PROCESS_REQUEST(redis_sock, cmd, cmd_len);
	IF_ATOMIC() {
		redis_boolean_response(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock, NULL, NULL);
	}
	REDIS_PROCESS_RESPONSE(redis_boolean_response);
}

/* {{{ proto boolean Redis::setex(string key, long expire, string value)
 */
PHP_METHOD(Redis, setex)
{
	redis_generic_setex(INTERNAL_FUNCTION_PARAM_PASSTHRU, "SETEX");
}

/* {{{ proto boolean Redis::psetex(string key, long expire, string value)
 */
PHP_METHOD(Redis, psetex)
{
	redis_generic_setex(INTERNAL_FUNCTION_PARAM_PASSTHRU, "PSETEX");
}

/* {{{ proto boolean Redis::setnx(string key, string value)
 */
PHP_METHOD(Redis, setnx)
{

    zval *object;
    RedisSock *redis_sock;
    char *key = NULL, *val = NULL, *cmd;
    int key_len, val_len, cmd_len;
    int val_free = 0, key_free = 0;
    zval *z_value;

    if (zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), "Osz",
                                     &object, redis_ce, &key, &key_len,
                                     &z_value) == FAILURE) {
        RETURN_FALSE;
    }

    if (redis_sock_get(object, &redis_sock TSRMLS_CC, 0) < 0) {
        RETURN_FALSE;
    }

    val_free = redis_serialize(redis_sock, z_value, &val, &val_len TSRMLS_CC);
	key_free = redis_key_prefix(redis_sock, &key, &key_len TSRMLS_CC);
    cmd_len = redis_cmd_format_static(&cmd, "SETNX", "ss", key, key_len, val, val_len);
    if(val_free) efree(val);
    if(key_free) efree(key);

    REDIS_PROCESS_REQUEST(redis_sock, cmd, cmd_len);

    IF_ATOMIC() {
	  redis_1_response(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock, NULL, NULL);
    }

    REDIS_PROCESS_RESPONSE(redis_1_response);

}
/* }}} */
/* {{{ proto string Redis::getSet(string key, string value)
 */
PHP_METHOD(Redis, getSet)
{

    zval *object;
    RedisSock *redis_sock;
    char *key = NULL, *val = NULL, *cmd;
    int key_len, val_len, cmd_len;
    int val_free = 0, key_free = 0;
    zval *z_value;

    if (zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), "Osz",
                                     &object, redis_ce, &key, &key_len,
                                     &z_value) == FAILURE) {
        RETURN_FALSE;
    }

    if (redis_sock_get(object, &redis_sock TSRMLS_CC, 0) < 0) {
        RETURN_FALSE;
    }

    val_free = redis_serialize(redis_sock, z_value, &val, &val_len TSRMLS_CC);
	key_free = redis_key_prefix(redis_sock, &key, &key_len TSRMLS_CC);
    cmd_len = redis_cmd_format_static(&cmd, "GETSET", "ss", key, key_len, val, val_len);
    if(val_free) efree(val);
    if(key_free) efree(key);

	REDIS_PROCESS_REQUEST(redis_sock, cmd, cmd_len);
	IF_ATOMIC() {
		redis_string_response(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock, NULL, NULL);
	}
	REDIS_PROCESS_RESPONSE(redis_string_response);

}
/* }}} */

/* {{{ proto string Redis::randomKey()
 */
PHP_METHOD(Redis, randomKey)
{

    zval *object;
    RedisSock *redis_sock;
    char *cmd;
    int cmd_len;

    if (zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), "O",
                                     &object, redis_ce) == FAILURE) {
        RETURN_FALSE;
    }

    if (redis_sock_get(object, &redis_sock TSRMLS_CC, 0) < 0) {
        RETURN_FALSE;
    }

    cmd_len = redis_cmd_format(&cmd, "*1" _NL "$9" _NL "RANDOMKEY" _NL);
	/* TODO: remove prefix from key */

	REDIS_PROCESS_REQUEST(redis_sock, cmd, cmd_len);
	IF_ATOMIC() {
	  redis_ping_response(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock, NULL, NULL);
	}
	REDIS_PROCESS_RESPONSE(redis_ping_response);
}
/* }}} */

/* {{{ proto string Redis::echo(string key)
 */
PHP_METHOD(Redis, echo)
{
    zval *object;
    RedisSock *redis_sock;
    char *key = NULL, *cmd;
    int key_len, cmd_len;
	int key_free;

    if (zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), "Os",
                                     &object, redis_ce,
                                     &key, &key_len) == FAILURE) {
        RETURN_FALSE;
    }

    if (redis_sock_get(object, &redis_sock TSRMLS_CC, 0) < 0) {
        RETURN_FALSE;
    }

	key_free = redis_key_prefix(redis_sock, &key, &key_len TSRMLS_CC);
    cmd_len = redis_cmd_format_static(&cmd, "ECHO", "s", key, key_len);
	if(key_free) efree(key);

	REDIS_PROCESS_REQUEST(redis_sock, cmd, cmd_len);
    IF_ATOMIC() {
	  redis_string_response(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock, NULL, NULL);
    }
    REDIS_PROCESS_RESPONSE(redis_string_response);

}
/* }}} */

/* {{{ proto string Redis::renameKey(string key_src, string key_dst)
 */
PHP_METHOD(Redis, renameKey)
{

    zval *object;
    RedisSock *redis_sock;
    char *cmd, *src, *dst;
    int cmd_len, src_len, dst_len;
	int src_free, dst_free;

    if (zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), "Oss",
                                     &object, redis_ce,
                                     &src, &src_len,
                                     &dst, &dst_len) == FAILURE) {
        RETURN_FALSE;
    }

    if (redis_sock_get(object, &redis_sock TSRMLS_CC, 0) < 0) {
        RETURN_FALSE;
    }

	src_free = redis_key_prefix(redis_sock, &src, &src_len TSRMLS_CC);
	dst_free = redis_key_prefix(redis_sock, &dst, &dst_len TSRMLS_CC);
    cmd_len = redis_cmd_format_static(&cmd, "RENAME", "ss", src, src_len, dst, dst_len);
    if(src_free) efree(src);
    if(dst_free) efree(dst);

	REDIS_PROCESS_REQUEST(redis_sock, cmd, cmd_len);
	IF_ATOMIC() {
		redis_boolean_response(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock, NULL, NULL);
	}
	REDIS_PROCESS_RESPONSE(redis_boolean_response);

}
/* }}} */

/* {{{ proto string Redis::renameNx(string key_src, string key_dst)
 */
PHP_METHOD(Redis, renameNx)
{

    zval *object;
    RedisSock *redis_sock;
    char *cmd, *src, *dst;
    int cmd_len, src_len, dst_len;
	int src_free, dst_free;

    if (zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), "Oss",
                                     &object, redis_ce,
                                     &src, &src_len,
                                     &dst, &dst_len) == FAILURE) {
        RETURN_FALSE;
    }

    if (redis_sock_get(object, &redis_sock TSRMLS_CC, 0) < 0) {
        RETURN_FALSE;
    }

	src_free = redis_key_prefix(redis_sock, &src, &src_len TSRMLS_CC);
	dst_free = redis_key_prefix(redis_sock, &dst, &dst_len TSRMLS_CC);
    cmd_len = redis_cmd_format_static(&cmd, "RENAMENX", "ss", src, src_len, dst, dst_len);
    if(src_free) efree(src);
    if(dst_free) efree(dst);

	REDIS_PROCESS_REQUEST(redis_sock, cmd, cmd_len);
	IF_ATOMIC() {
		redis_1_response(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock, NULL, NULL);
	}
	REDIS_PROCESS_RESPONSE(redis_1_response);

}
/* }}} */

/* {{{ proto string Redis::get(string key)
 */
PHP_METHOD(Redis, get)
{
    zval *object;
    RedisSock *redis_sock;
    char *key = NULL, *cmd;
    int key_len, cmd_len;
	int key_free;

    if (zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), "Os",
                                     &object, redis_ce,
                                     &key, &key_len) == FAILURE) {
        RETURN_FALSE;
    }

    if (redis_sock_get(object, &redis_sock TSRMLS_CC, 0) < 0) {
        RETURN_FALSE;
    }

	key_free = redis_key_prefix(redis_sock, &key, &key_len TSRMLS_CC);
    cmd_len = redis_cmd_format_static(&cmd, "GET", "s", key, key_len);
	if(key_free) efree(key);

	REDIS_PROCESS_REQUEST(redis_sock, cmd, cmd_len);
    IF_ATOMIC() {
	  redis_string_response(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock, NULL, NULL);
    }
    REDIS_PROCESS_RESPONSE(redis_string_response);

}
/* }}} */


/* {{{ proto string Redis::ping()
 */
PHP_METHOD(Redis, ping)
{
    zval *object;
    RedisSock *redis_sock;
    char *cmd;
    int cmd_len;

    if (zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), "O",
                                     &object, redis_ce) == FAILURE) {
        RETURN_FALSE;
    }

    if (redis_sock_get(object, &redis_sock TSRMLS_CC, 0) < 0) {
        RETURN_FALSE;
    }

    cmd_len = redis_cmd_format_static(&cmd, "PING", "");

	REDIS_PROCESS_REQUEST(redis_sock, cmd, cmd_len);
    IF_ATOMIC() {
	  redis_ping_response(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock, NULL, NULL);
    }
    REDIS_PROCESS_RESPONSE(redis_ping_response);
}
/* }}} */

PHPAPI void redis_atomic_increment(INTERNAL_FUNCTION_PARAMETERS, char *keyword, int count) {

    zval *object;
    RedisSock *redis_sock;
    char *key = NULL, *cmd;
    int key_len, cmd_len;
    long val = 1;
	int key_free;

    if (zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), "Os|l",
                                     &object, redis_ce,
                                     &key, &key_len, &val) == FAILURE) {
        RETURN_FALSE;
    }

    if (redis_sock_get(object, &redis_sock TSRMLS_CC, 0) < 0) {
        RETURN_FALSE;
    }
	key_free = redis_key_prefix(redis_sock, &key, &key_len TSRMLS_CC);
    if (val == 1) {
        cmd_len = redis_cmd_format_static(&cmd, keyword, "s", key, key_len);
    } else {
        cmd_len = redis_cmd_format_static(&cmd, keyword, "sl", key, key_len, val);
    }
	if(key_free) efree(key);

	REDIS_PROCESS_REQUEST(redis_sock, cmd, cmd_len);
    IF_ATOMIC() {
		redis_long_response(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock, NULL, NULL);
    }
    REDIS_PROCESS_RESPONSE(redis_long_response);
}

/* {{{ proto boolean Redis::incr(string key [,int value])
 */
PHP_METHOD(Redis, incr){

    zval *object;
    char *key = NULL;
    int key_len;
    long val = 1;

    if (zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), "Os|l",
                                     &object, redis_ce,
                                     &key, &key_len, &val) == FAILURE) {
        RETURN_FALSE;
    }

    if(val == 1) {
        redis_atomic_increment(INTERNAL_FUNCTION_PARAM_PASSTHRU, "INCR", 1);
    } else {
        redis_atomic_increment(INTERNAL_FUNCTION_PARAM_PASSTHRU, "INCRBY", val);
    }
}
/* }}} */

/* {{{ proto boolean Redis::incrBy(string key ,int value)
 */
PHP_METHOD(Redis, incrBy){

    zval *object;
    char *key = NULL;
    int key_len;
    long val = 1;

    if (zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), "Osl",
                                     &object, redis_ce,
                                     &key, &key_len, &val) == FAILURE) {
        RETURN_FALSE;
    }

    if(val == 1) {
        redis_atomic_increment(INTERNAL_FUNCTION_PARAM_PASSTHRU, "INCR", 1);
    } else {
        redis_atomic_increment(INTERNAL_FUNCTION_PARAM_PASSTHRU, "INCRBY", val);
    }
}
/* }}} */

/* {{{ proto float Redis::incrByFloat(string key, float value)
 */
PHP_METHOD(Redis, incrByFloat) {
	zval *object;
	RedisSock *redis_sock;
	char *key = NULL, *cmd;
	int key_len, cmd_len, key_free;
	double val;

	if(zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), "Osd",
									&object, redis_ce, &key, &key_len, &val) == FAILURE) {
		RETURN_FALSE;
	}

	if(redis_sock_get(object, &redis_sock TSRMLS_CC, 0) < 0) {
		RETURN_FALSE;
	}

	// Prefix key, format command, free old key if necissary
	key_free = redis_key_prefix(redis_sock, &key, &key_len TSRMLS_CC);
    cmd_len = redis_cmd_format_static(&cmd, "INCRBYFLOAT", "sf", key, key_len, val);
	if(key_free) efree(key);

	REDIS_PROCESS_REQUEST(redis_sock, cmd, cmd_len);
	IF_ATOMIC() {
		redis_bulk_double_response(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock, NULL, NULL);
	}
	REDIS_PROCESS_RESPONSE(redis_bulk_double_response);
}

/* {{{ proto boolean Redis::decr(string key [,int value])
 */
PHP_METHOD(Redis, decr)
{
    zval *object;
    char *key = NULL;
    int key_len;
    long val = 1;

    if (zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), "Os|l",
                                     &object, redis_ce,
                                     &key, &key_len, &val) == FAILURE) {
        RETURN_FALSE;
    }

    if(val == 1) {
        redis_atomic_increment(INTERNAL_FUNCTION_PARAM_PASSTHRU, "DECR", 1);
    } else {
        redis_atomic_increment(INTERNAL_FUNCTION_PARAM_PASSTHRU, "DECRBY", val);
    }
}
/* }}} */

/* {{{ proto boolean Redis::decrBy(string key ,int value)
 */
PHP_METHOD(Redis, decrBy){

    zval *object;
    char *key = NULL;
    int key_len;
    long val = 1;

    if (zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), "Osl",
                                     &object, redis_ce,
                                     &key, &key_len, &val) == FAILURE) {
        RETURN_FALSE;
    }

    if(val == 1) {
        redis_atomic_increment(INTERNAL_FUNCTION_PARAM_PASSTHRU, "DECR", 1);
    } else {
        redis_atomic_increment(INTERNAL_FUNCTION_PARAM_PASSTHRU, "DECRBY", val);
    }
}
/* }}} */

/* {{{ proto array Redis::getMultiple(array keys)
 */
PHP_METHOD(Redis, getMultiple)
{
    zval *object, *z_args, **z_ele;
    HashTable *hash;
    HashPosition ptr;
    RedisSock *redis_sock;
    smart_str cmd = {0};
    int arg_count;

    // Make sure we have proper arguments
    if(zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), "Oa",
                                    &object, redis_ce, &z_args) == FAILURE) {
        RETURN_FALSE;
    }

    // We'll need the socket
    if(redis_sock_get(object, &redis_sock TSRMLS_CC, 0) < 0) {
        RETURN_FALSE;
    }

    // Grab our array
    hash = Z_ARRVAL_P(z_args);

    // We don't need to do anything if there aren't any keys
    if((arg_count = zend_hash_num_elements(hash)) == 0) {
        RETURN_FALSE;
    }

    // Build our command header
    redis_cmd_init_sstr(&cmd, arg_count, "MGET", 4);

    // Iterate through and grab our keys
    for(zend_hash_internal_pointer_reset_ex(hash, &ptr);
        zend_hash_get_current_data_ex(hash, (void**)&z_ele, &ptr) == SUCCESS;
        zend_hash_move_forward_ex(hash, &ptr))
    {
        char *key;
        int key_len, key_free;
        zval *z_tmp = NULL;

        // If the key isn't a string, turn it into one
        if(Z_TYPE_PP(z_ele) == IS_STRING) {
            key = Z_STRVAL_PP(z_ele);
            key_len = Z_STRLEN_PP(z_ele);
        } else {
            MAKE_STD_ZVAL(z_tmp);
            *z_tmp = **z_ele;
            zval_copy_ctor(z_tmp);
            convert_to_string(z_tmp);

            key = Z_STRVAL_P(z_tmp);
            key_len = Z_STRLEN_P(z_tmp);
        }

        // Apply key prefix if necissary
        key_free = redis_key_prefix(redis_sock, &key, &key_len TSRMLS_CC);

        // Append this key to our command
        redis_cmd_append_sstr(&cmd, key, key_len);

        // Free our key if it was prefixed
        if(key_free) efree(key);

        // Free oour temporary ZVAL if we converted from a non-string
        if(z_tmp) {
            zval_dtor(z_tmp);
            efree(z_tmp);
            z_tmp = NULL;
        }
    }

    // Kick off our command
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
    zval *object;
    RedisSock *redis_sock;
    char *key = NULL, *cmd;
    int key_len, cmd_len;
	int key_free;

    if (zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), "Os",
                                     &object, redis_ce,
                                     &key, &key_len) == FAILURE) {
        RETURN_FALSE;
    }
    if (redis_sock_get(object, &redis_sock TSRMLS_CC, 0) < 0) {
        RETURN_FALSE;
    }

	key_free = redis_key_prefix(redis_sock, &key, &key_len TSRMLS_CC);
    cmd_len = redis_cmd_format_static(&cmd, "EXISTS", "s", key, key_len);
	if(key_free) efree(key);

    REDIS_PROCESS_REQUEST(redis_sock, cmd, cmd_len);
    IF_ATOMIC() {
	  redis_1_response(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock, NULL, NULL);
    }
    REDIS_PROCESS_RESPONSE(redis_1_response);

}
/* }}} */

/* {{{ proto boolean Redis::delete(string key)
 */
PHP_METHOD(Redis, delete)
{
    RedisSock *redis_sock;

    if(FAILURE == generic_multiple_args_cmd(INTERNAL_FUNCTION_PARAM_PASSTHRU,
                    "DEL", sizeof("DEL") - 1,
					1, &redis_sock, 0, 1, 1))
		return;

    IF_ATOMIC() {
	  redis_long_response(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock, NULL, NULL);
    }
	REDIS_PROCESS_RESPONSE(redis_long_response);

}
/* }}} */

PHPAPI void redis_set_watch(RedisSock *redis_sock)
{
    redis_sock->watching = 1;
}

PHPAPI void redis_watch_response(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock, zval *z_tab, void *ctx)
{
    redis_boolean_response_impl(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock, z_tab, ctx, redis_set_watch);
}

/* {{{ proto boolean Redis::watch(string key1, string key2...)
 */
PHP_METHOD(Redis, watch)
{
    RedisSock *redis_sock;

    generic_multiple_args_cmd(INTERNAL_FUNCTION_PARAM_PASSTHRU,
                    "WATCH", sizeof("WATCH") - 1,
					1, &redis_sock, 0, 1, 1);
    redis_sock->watching = 1;
    IF_ATOMIC() {
        redis_watch_response(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock, NULL, NULL);
    }
    REDIS_PROCESS_RESPONSE(redis_watch_response);

}
/* }}} */

PHPAPI void redis_clear_watch(RedisSock *redis_sock)
{
    redis_sock->watching = 0;
}

PHPAPI void redis_unwatch_response(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock, zval *z_tab, void *ctx)
{
    redis_boolean_response_impl(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock, z_tab, ctx, redis_clear_watch);
}

/* {{{ proto boolean Redis::unwatch()
 */
PHP_METHOD(Redis, unwatch)
{
    char cmd[] = "*1" _NL "$7" _NL "UNWATCH" _NL;
    generic_empty_cmd_impl(INTERNAL_FUNCTION_PARAM_PASSTHRU, estrdup(cmd), sizeof(cmd)-1, redis_unwatch_response);

}
/* }}} */

/* {{{ proto array Redis::getKeys(string pattern)
 */
PHP_METHOD(Redis, getKeys)
{
    zval *object;
    RedisSock *redis_sock;
    char *pattern = NULL, *cmd;
    int pattern_len, cmd_len, pattern_free;

    if (zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), "Os",
                                     &object, redis_ce,
                                     &pattern, &pattern_len) == FAILURE) {
        RETURN_NULL();
    }

    if (redis_sock_get(object, &redis_sock TSRMLS_CC, 0) < 0) {
        RETURN_FALSE;
    }

	pattern_free = redis_key_prefix(redis_sock, &pattern, &pattern_len TSRMLS_CC);
    cmd_len = redis_cmd_format_static(&cmd, "KEYS", "s", pattern, pattern_len);
	if(pattern_free) efree(pattern);

	REDIS_PROCESS_REQUEST(redis_sock, cmd, cmd_len);
    IF_ATOMIC() {
	    if (redis_sock_read_multibulk_reply_raw(INTERNAL_FUNCTION_PARAM_PASSTHRU,
											redis_sock, NULL, NULL) < 0) {
    	    RETURN_FALSE;
	    }
    }
    REDIS_PROCESS_RESPONSE(redis_sock_read_multibulk_reply_raw);
}
/* }}} */

/* {{{ proto int Redis::type(string key)
 */
PHP_METHOD(Redis, type)
{
    zval *object;
    RedisSock *redis_sock;
    char *key = NULL, *cmd;
    int key_len, cmd_len, key_free;

    if (zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), "Os",
                                     &object, redis_ce,
                                     &key, &key_len) == FAILURE) {
        RETURN_NULL();
    }

    if (redis_sock_get(object, &redis_sock TSRMLS_CC, 0) < 0) {
        RETURN_FALSE;
    }

	key_free = redis_key_prefix(redis_sock, &key, &key_len TSRMLS_CC);
    cmd_len = redis_cmd_format_static(&cmd, "TYPE", "s", key, key_len);
	if(key_free) efree(key);

	REDIS_PROCESS_REQUEST(redis_sock, cmd, cmd_len);
	IF_ATOMIC() {
	  redis_type_response(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock, NULL, NULL);
	}
	REDIS_PROCESS_RESPONSE(redis_type_response);
}
/* }}} */

PHP_METHOD(Redis, append)
{
	zval *object;
	RedisSock *redis_sock;
	char *cmd;
	int cmd_len, key_len, val_len, key_free;
	char *key, *val;

	if (zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), "Oss",
                                     &object, redis_ce,
                                     &key, &key_len, &val, &val_len) == FAILURE) {
		RETURN_NULL();
	}

	if (redis_sock_get(object, &redis_sock TSRMLS_CC, 0) < 0) {
		RETURN_FALSE;
	}

	key_free = redis_key_prefix(redis_sock, &key, &key_len TSRMLS_CC);
	cmd_len = redis_cmd_format_static(&cmd, "APPEND", "ss", key, key_len, val, val_len);
	if(key_free) efree(key);

	REDIS_PROCESS_REQUEST(redis_sock, cmd, cmd_len);
	IF_ATOMIC() {
		redis_long_response(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock, NULL, NULL);
	}
	REDIS_PROCESS_RESPONSE(redis_long_response);
}

PHP_METHOD(Redis, getRange)
{
	zval *object;
	RedisSock *redis_sock;
	char *key = NULL, *cmd;
	int key_len, cmd_len, key_free;
	long start, end;

	if (zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), "Osll",
                                     &object, redis_ce, &key, &key_len,
                                     &start, &end) == FAILURE) {
		RETURN_FALSE;
	}

	if (redis_sock_get(object, &redis_sock TSRMLS_CC, 0) < 0) {
		RETURN_FALSE;
	}

	key_free = redis_key_prefix(redis_sock, &key, &key_len TSRMLS_CC);
	cmd_len = redis_cmd_format_static(&cmd, "GETRANGE", "sdd", key, key_len, (int)start, (int)end);
	if(key_free) efree(key);
	REDIS_PROCESS_REQUEST(redis_sock, cmd, cmd_len);
	IF_ATOMIC() {
		redis_string_response(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock, NULL, NULL);
	}
	REDIS_PROCESS_RESPONSE(redis_string_response);
}

PHP_METHOD(Redis, setRange)
{
	zval *object;
	RedisSock *redis_sock;
	char *key = NULL, *val, *cmd;
	int key_len, val_len, cmd_len, key_free;
	long offset;

	if (zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), "Osls",
                                     &object, redis_ce, &key, &key_len,
                                     &offset, &val, &val_len) == FAILURE) {
		RETURN_FALSE;
	}

	if (redis_sock_get(object, &redis_sock TSRMLS_CC, 0) < 0) {
		RETURN_FALSE;
	}

	key_free = redis_key_prefix(redis_sock, &key, &key_len TSRMLS_CC);
	cmd_len = redis_cmd_format_static(&cmd, "SETRANGE", "sds", key, key_len, (int)offset, val, val_len);
	if(key_free) efree(key);
	REDIS_PROCESS_REQUEST(redis_sock, cmd, cmd_len);
	IF_ATOMIC() {
		redis_long_response(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock, NULL, NULL);
	}
	REDIS_PROCESS_RESPONSE(redis_long_response);
}

PHP_METHOD(Redis, getBit)
{
	zval *object;
	RedisSock *redis_sock;
	char *key = NULL, *cmd;
	int key_len, cmd_len, key_free;
	long offset;

	if (zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), "Osl",
                                     &object, redis_ce, &key, &key_len,
                                     &offset) == FAILURE) {
		RETURN_FALSE;
	}

	if (redis_sock_get(object, &redis_sock TSRMLS_CC, 0) < 0) {
		RETURN_FALSE;
	}

	// GETBIT and SETBIT only work for 0 - 2^32-1
	if(offset < BITOP_MIN_OFFSET || offset > BITOP_MAX_OFFSET) {
	    RETURN_FALSE;
	}

	key_free = redis_key_prefix(redis_sock, &key, &key_len TSRMLS_CC);
	cmd_len = redis_cmd_format_static(&cmd, "GETBIT", "sd", key, key_len, (int)offset);
	if(key_free) efree(key);
	REDIS_PROCESS_REQUEST(redis_sock, cmd, cmd_len);
	IF_ATOMIC() {
		redis_long_response(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock, NULL, NULL);
	}
	REDIS_PROCESS_RESPONSE(redis_long_response);
}

PHP_METHOD(Redis, setBit)
{
	zval *object;
	RedisSock *redis_sock;
	char *key = NULL, *cmd;
	int key_len, cmd_len, key_free;
	long offset;
	zend_bool val;

	if (zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), "Oslb",
                                     &object, redis_ce, &key, &key_len,
                                     &offset, &val) == FAILURE) {
		RETURN_FALSE;
	}

	if (redis_sock_get(object, &redis_sock TSRMLS_CC, 0) < 0) {
		RETURN_FALSE;
	}

	// GETBIT and SETBIT only work for 0 - 2^32-1
	if(offset < BITOP_MIN_OFFSET || offset > BITOP_MAX_OFFSET) {
	    RETURN_FALSE;
	}

	key_free = redis_key_prefix(redis_sock, &key, &key_len TSRMLS_CC);
	cmd_len = redis_cmd_format_static(&cmd, "SETBIT", "sdd", key, key_len, (int)offset, (int)val);
	if(key_free) efree(key);
	REDIS_PROCESS_REQUEST(redis_sock, cmd, cmd_len);
	IF_ATOMIC() {
		redis_long_response(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock, NULL, NULL);
	}
	REDIS_PROCESS_RESPONSE(redis_long_response);
}


PHP_METHOD(Redis, strlen)
{
	zval *object;
	RedisSock *redis_sock;
	char *cmd;
	int cmd_len, key_len, key_free;
	char *key;

	if (zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), "Os",
                                     &object, redis_ce,
                                     &key, &key_len) == FAILURE) {
		RETURN_NULL();
	}

	if (redis_sock_get(object, &redis_sock TSRMLS_CC, 0) < 0) {
		RETURN_FALSE;
	}

	key_free = redis_key_prefix(redis_sock, &key, &key_len TSRMLS_CC);
	cmd_len = redis_cmd_format_static(&cmd, "STRLEN", "s", key, key_len);
	if(key_free) efree(key);

	REDIS_PROCESS_REQUEST(redis_sock, cmd, cmd_len);
	IF_ATOMIC() {
		redis_long_response(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock, NULL, NULL);
	}
	REDIS_PROCESS_RESPONSE(redis_long_response);
}

PHPAPI void
generic_push_function(INTERNAL_FUNCTION_PARAMETERS, char *keyword, int keyword_len) {
    zval *object;
    RedisSock *redis_sock;
    char *cmd, *key, *val;
    int cmd_len, key_len, val_len;
    zval *z_value;
    int val_free, key_free = 0;

    if (zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), "Osz",
                                     &object, redis_ce,
                                     &key, &key_len, &z_value) == FAILURE) {
        RETURN_NULL();
    }

    if (redis_sock_get(object, &redis_sock TSRMLS_CC, 0) < 0) {
        RETURN_FALSE;
    }

    val_free = redis_serialize(redis_sock, z_value, &val, &val_len TSRMLS_CC);
	key_free = redis_key_prefix(redis_sock, &key, &key_len TSRMLS_CC);
    cmd_len = redis_cmd_format_static(&cmd, keyword, "ss", key, key_len, val, val_len);
    if(val_free) efree(val);
    if(key_free) efree(key);

	REDIS_PROCESS_REQUEST(redis_sock, cmd, cmd_len);
	IF_ATOMIC() {
		redis_long_response(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock, NULL, NULL);
	}
	REDIS_PROCESS_RESPONSE(redis_long_response);
}

/* {{{ proto boolean Redis::lPush(string key , string value)
 */
PHP_METHOD(Redis, lPush)
{
    RedisSock *redis_sock;

    if(FAILURE == generic_multiple_args_cmd(INTERNAL_FUNCTION_PARAM_PASSTHRU,
                    "LPUSH", sizeof("LPUSH") - 1,
                    2, &redis_sock, 0, 0, 1))
		return;

    IF_ATOMIC() {
        redis_long_response(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock, NULL, NULL);
    }
    REDIS_PROCESS_RESPONSE(redis_long_response);
}
/* }}} */

/* {{{ proto boolean Redis::rPush(string key , string value)
 */
PHP_METHOD(Redis, rPush)
{
    RedisSock *redis_sock;

    if(FAILURE == generic_multiple_args_cmd(INTERNAL_FUNCTION_PARAM_PASSTHRU,
                    "RPUSH", sizeof("RPUSH") - 1,
                    2, &redis_sock, 0, 0, 1))
		return;

    IF_ATOMIC() {
        redis_long_response(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock, NULL, NULL);
    }
    REDIS_PROCESS_RESPONSE(redis_long_response);
}
/* }}} */

PHP_METHOD(Redis, lInsert)
{

	zval *object;
	RedisSock *redis_sock;
	char *pivot, *position, *key, *val, *cmd;
	int pivot_len, position_len, key_len, val_len, cmd_len;
    int val_free, pivot_free, key_free;
    zval *z_value, *z_pivot;
	

	if (zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), "Osszz",
					&object, redis_ce,
					&key, &key_len, 
					&position, &position_len,
					&z_pivot,
					&z_value) == FAILURE) {
		RETURN_NULL();
	}

	if (redis_sock_get(object, &redis_sock TSRMLS_CC, 0) < 0) {
		RETURN_FALSE;
	}
	
	if(strncasecmp(position, "after", 5) == 0 || strncasecmp(position, "before", 6) == 0) {

		key_free = redis_key_prefix(redis_sock, &key, &key_len TSRMLS_CC);
        val_free = redis_serialize(redis_sock, z_value, &val, &val_len TSRMLS_CC);
        pivot_free = redis_serialize(redis_sock, z_pivot, &pivot, &pivot_len TSRMLS_CC);
        cmd_len = redis_cmd_format_static(&cmd, "LINSERT", "ssss", key, key_len, position, position_len, pivot, pivot_len, val, val_len);
        if(val_free) efree(val);
		if(key_free) efree(key);
        if(pivot_free) efree(pivot);

		REDIS_PROCESS_REQUEST(redis_sock, cmd, cmd_len); 
		IF_ATOMIC() { 
			redis_long_response(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock, NULL, NULL); 
		} 
		REDIS_PROCESS_RESPONSE(redis_long_response); 
	} else {
		php_error_docref(NULL TSRMLS_CC, E_ERROR, "Error on position");
	}
	
}

PHP_METHOD(Redis, lPushx)
{
	generic_push_function(INTERNAL_FUNCTION_PARAM_PASSTHRU, "LPUSHX", sizeof("LPUSHX")-1);
}

PHP_METHOD(Redis, rPushx)
{
	generic_push_function(INTERNAL_FUNCTION_PARAM_PASSTHRU, "RPUSHX", sizeof("RPUSHX")-1);
}

PHPAPI void
generic_pop_function(INTERNAL_FUNCTION_PARAMETERS, char *keyword, int keyword_len) {

    zval *object;
    RedisSock *redis_sock;
    char *key = NULL, *cmd;
    int key_len, cmd_len, key_free;

    if (zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), "Os",
                                     &object, redis_ce,
                                     &key, &key_len) == FAILURE) {
        RETURN_FALSE;
    }

    if (redis_sock_get(object, &redis_sock TSRMLS_CC, 0) < 0) {
        RETURN_FALSE;
    }

	key_free = redis_key_prefix(redis_sock, &key, &key_len TSRMLS_CC);
    cmd_len = redis_cmd_format_static(&cmd, keyword, "s", key, key_len);
	if(key_free) efree(key);

	REDIS_PROCESS_REQUEST(redis_sock, cmd, cmd_len);
	IF_ATOMIC() {
		redis_string_response(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock, NULL, NULL);
	}
	REDIS_PROCESS_RESPONSE(redis_string_response);
}

/* {{{ proto string Redis::lPOP(string key)
 */
PHP_METHOD(Redis, lPop)
{
        generic_pop_function(INTERNAL_FUNCTION_PARAM_PASSTHRU, "LPOP", sizeof("LPOP")-1);
}
/* }}} */

/* {{{ proto string Redis::rPOP(string key)
 */
PHP_METHOD(Redis, rPop)
{
        generic_pop_function(INTERNAL_FUNCTION_PARAM_PASSTHRU, "RPOP", sizeof("RPOP")-1);
}
/* }}} */

/* {{{ proto string Redis::blPop(string key1, string key2, ..., int timeout)
 */
PHP_METHOD(Redis, blPop)
{

    RedisSock *redis_sock;

    if(FAILURE == generic_multiple_args_cmd(INTERNAL_FUNCTION_PARAM_PASSTHRU,
                    "BLPOP", sizeof("BLPOP") - 1,
					2, &redis_sock, 1, 1, 1))
		return;

    IF_ATOMIC() {
    	if (redis_sock_read_multibulk_reply(INTERNAL_FUNCTION_PARAM_PASSTHRU,
											redis_sock, NULL, NULL) < 0) {
        	RETURN_FALSE;
	    }
    }
    REDIS_PROCESS_RESPONSE(redis_sock_read_multibulk_reply);
}
/* }}} */

/* {{{ proto string Redis::brPop(string key1, string key2, ..., int timeout)
 */
PHP_METHOD(Redis, brPop)
{
    RedisSock *redis_sock;

    if(FAILURE == generic_multiple_args_cmd(INTERNAL_FUNCTION_PARAM_PASSTHRU,
                    "BRPOP", sizeof("BRPOP") - 1,
					2, &redis_sock, 1, 1, 1))
		return;

    IF_ATOMIC() {
    	if (redis_sock_read_multibulk_reply(INTERNAL_FUNCTION_PARAM_PASSTHRU,
											redis_sock, NULL, NULL) < 0) {
        	RETURN_FALSE;
	    }
    }
    REDIS_PROCESS_RESPONSE(redis_sock_read_multibulk_reply);
}
/* }}} */


/* {{{ proto int Redis::lSize(string key)
 */
PHP_METHOD(Redis, lSize)
{
    zval *object;
    RedisSock *redis_sock;
    char *key = NULL, *cmd;
    int key_len, cmd_len, key_free;

    if (zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), "Os",
                                     &object, redis_ce,
                                     &key, &key_len) == FAILURE) {
        RETURN_FALSE;
    }

    if (redis_sock_get(object, &redis_sock TSRMLS_CC, 0) < 0) {
        RETURN_FALSE;
    }

	key_free = redis_key_prefix(redis_sock, &key, &key_len TSRMLS_CC);
    cmd_len = redis_cmd_format_static(&cmd, "LLEN", "s", key, key_len);
	if(key_free) efree(key);

	REDIS_PROCESS_REQUEST(redis_sock, cmd, cmd_len);
	IF_ATOMIC() {
		redis_long_response(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock, NULL, NULL);
	}
	REDIS_PROCESS_RESPONSE(redis_long_response);

}
/* }}} */

/* {{{ proto boolean Redis::lRemove(string list, string value, int count = 0)
 */
PHP_METHOD(Redis, lRemove)
{
    zval *object;
    RedisSock *redis_sock;
    char *cmd;
    int cmd_len, key_len, val_len;
    char *key, *val;
    long count = 0;
    zval *z_value;
    int val_free, key_free = 0;

    if (zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), "Osz|l",
                                     &object, redis_ce,
                                     &key, &key_len, &z_value, &count) == FAILURE) {
        RETURN_NULL();
    }

    if (redis_sock_get(object, &redis_sock TSRMLS_CC, 0) < 0) {
        RETURN_FALSE;
    }


    /* LREM key count value */
    val_free = redis_serialize(redis_sock, z_value, &val, &val_len TSRMLS_CC);
	key_free = redis_key_prefix(redis_sock, &key, &key_len TSRMLS_CC);
    cmd_len = redis_cmd_format_static(&cmd, "LREM", "sds", key, key_len, count, val, val_len);
    if(val_free) efree(val);
    if(key_free) efree(key);

	REDIS_PROCESS_REQUEST(redis_sock, cmd, cmd_len);
	IF_ATOMIC() {
		redis_long_response(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock, NULL, NULL);
	}
	REDIS_PROCESS_RESPONSE(redis_long_response);
}
/* }}} */

/* {{{ proto boolean Redis::listTrim(string key , int start , int end)
 */
PHP_METHOD(Redis, listTrim)
{
    zval *object;
    RedisSock *redis_sock;
    char *key = NULL, *cmd;
    int key_len, cmd_len, key_free;
    long start, end;

    if (zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), "Osll",
                                     &object, redis_ce, &key, &key_len,
                                     &start, &end) == FAILURE) {
        RETURN_FALSE;
    }

    if (redis_sock_get(object, &redis_sock TSRMLS_CC, 0) < 0) {
        RETURN_FALSE;
    }

	key_free = redis_key_prefix(redis_sock, &key, &key_len TSRMLS_CC);
    cmd_len = redis_cmd_format_static(&cmd, "LTRIM", "sdd", key, key_len, (int)start, (int)end);
	if(key_free) efree(key);

	REDIS_PROCESS_REQUEST(redis_sock, cmd, cmd_len);
	IF_ATOMIC() {
		redis_boolean_response(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock, NULL, NULL);
	}
	REDIS_PROCESS_RESPONSE(redis_boolean_response);

}
/* }}} */

/* {{{ proto string Redis::lGet(string key , int index)
 */
PHP_METHOD(Redis, lGet)
{
    zval *object;
    RedisSock *redis_sock;
    char *key = NULL, *cmd;
    int key_len, cmd_len, key_free;
    long index;

    if (zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), "Osl",
                                     &object, redis_ce,
                                     &key, &key_len, &index) == FAILURE) {
        RETURN_NULL();
    }

    if (redis_sock_get(object, &redis_sock TSRMLS_CC, 0) < 0) {
        RETURN_FALSE;
    }

    /* LINDEX key pos */
	key_free = redis_key_prefix(redis_sock, &key, &key_len TSRMLS_CC);
    cmd_len = redis_cmd_format_static(&cmd, "LINDEX", "sd", key, key_len, (int)index);
	if(key_free) efree(key);

	REDIS_PROCESS_REQUEST(redis_sock, cmd, cmd_len);
	IF_ATOMIC() {
		redis_string_response(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock, NULL, NULL);
	}
	REDIS_PROCESS_RESPONSE(redis_string_response);
}
/* }}} */

/* {{{ proto array Redis::lGetRange(string key, int start , int end)
 */
PHP_METHOD(Redis, lGetRange)
{
    zval *object;
    RedisSock *redis_sock;
    char *key = NULL, *cmd;
    int key_len, cmd_len, key_free;
    long start, end;

    if (zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), "Osll",
                                     &object, redis_ce,
                                     &key, &key_len, &start, &end) == FAILURE) {
        RETURN_FALSE;
    }

    if (redis_sock_get(object, &redis_sock TSRMLS_CC, 0) < 0) {
        RETURN_FALSE;
    }

    /* LRANGE key start end */
	key_free = redis_key_prefix(redis_sock, &key, &key_len TSRMLS_CC);
    cmd_len = redis_cmd_format_static(&cmd, "LRANGE", "sdd", key, key_len, (int)start, (int)end);
	if(key_free) efree(key);

	REDIS_PROCESS_REQUEST(redis_sock, cmd, cmd_len);
	IF_ATOMIC() {
		redis_sock_read_multibulk_reply(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock, NULL, NULL);
	}
	REDIS_PROCESS_RESPONSE(redis_sock_read_multibulk_reply);

}
/* }}} */

/* {{{ proto boolean Redis::sAdd(string key , mixed value)
 */
PHP_METHOD(Redis, sAdd)
{
    RedisSock *redis_sock;

    if(FAILURE == generic_multiple_args_cmd(INTERNAL_FUNCTION_PARAM_PASSTHRU,
                    "SADD", sizeof("SADD") - 1,
					2, &redis_sock, 0, 0, 1))
		return;

	IF_ATOMIC() {
	  redis_long_response(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock, NULL, NULL);
	}
	REDIS_PROCESS_RESPONSE(redis_long_response);
}
/* }}} */

/* {{{ proto int Redis::sSize(string key)
 */
PHP_METHOD(Redis, sSize)
{
    zval *object;
    RedisSock *redis_sock;
    char *key = NULL, *cmd;
    int key_len, cmd_len, key_free;

    if (zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), "Os",
                                     &object, redis_ce,
                                     &key, &key_len) == FAILURE) {
        RETURN_FALSE;
    }

    if (redis_sock_get(object, &redis_sock TSRMLS_CC, 0) < 0) {
        RETURN_FALSE;
    }

	key_free = redis_key_prefix(redis_sock, &key, &key_len TSRMLS_CC);
    cmd_len = redis_cmd_format_static(&cmd, "SCARD", "s", key, key_len);
	if(key_free) efree(key);

	REDIS_PROCESS_REQUEST(redis_sock, cmd, cmd_len);
	IF_ATOMIC() {
		redis_long_response(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock, NULL, NULL);
	}
	REDIS_PROCESS_RESPONSE(redis_long_response);
}
/* }}} */

/* {{{ proto boolean Redis::sRemove(string set, string value)
 */
PHP_METHOD(Redis, sRemove)
{
    RedisSock *redis_sock;

    if(FAILURE == generic_multiple_args_cmd(INTERNAL_FUNCTION_PARAM_PASSTHRU,
                    "SREM", sizeof("SREM") - 1,
					2, &redis_sock, 0, 0, 1))
		return;

	IF_ATOMIC() {
	  redis_long_response(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock, NULL, NULL);
	}
	REDIS_PROCESS_RESPONSE(redis_long_response);
}
/* }}} */
/* {{{ proto boolean Redis::sMove(string set_src, string set_dst, mixed value)
 */
PHP_METHOD(Redis, sMove)
{
    zval *object;
    RedisSock *redis_sock;
    char *src = NULL, *dst = NULL, *val = NULL, *cmd;
    int src_len, dst_len, val_len, cmd_len;
    int val_free, src_free, dst_free;
    zval *z_value;

    if (zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), "Ossz",
                                     &object, redis_ce,
                                     &src, &src_len,
                                     &dst, &dst_len,
                                     &z_value) == FAILURE) {
        RETURN_FALSE;
    }

    if (redis_sock_get(object, &redis_sock TSRMLS_CC, 0) < 0) {
        RETURN_FALSE;
    }

    val_free = redis_serialize(redis_sock, z_value, &val, &val_len TSRMLS_CC);
	src_free = redis_key_prefix(redis_sock, &src, &src_len TSRMLS_CC);
	dst_free = redis_key_prefix(redis_sock, &dst, &dst_len TSRMLS_CC);
    cmd_len = redis_cmd_format_static(&cmd, "SMOVE", "sss", src, src_len, dst, dst_len, val, val_len);
    if(val_free) efree(val);
    if(src_free) efree(src);
    if(dst_free) efree(dst);

	REDIS_PROCESS_REQUEST(redis_sock, cmd, cmd_len);
	IF_ATOMIC() {
	  redis_1_response(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock, NULL, NULL);
	}
	REDIS_PROCESS_RESPONSE(redis_1_response);
}
/* }}} */

/* }}} */
/* {{{ proto string Redis::sPop(string key)
 */
PHP_METHOD(Redis, sPop)
{
    generic_pop_function(INTERNAL_FUNCTION_PARAM_PASSTHRU, "SPOP", 4);
}
/* }}} */

/* }}} */
/* {{{ proto string Redis::sRandMember(string key [int count])
 */
PHP_METHOD(Redis, sRandMember)
{
    zval *object;
    RedisSock *redis_sock;
    char *key = NULL, *cmd;
    int key_len, cmd_len, key_free = 0;
    long count;

    // Parse our params
    if(zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), "Os|l",
                                    &object, redis_ce, &key, &key_len, &count) == FAILURE) {
        RETURN_FALSE;
    }

    // Get our redis socket
    if(redis_sock_get(object, &redis_sock TSRMLS_CC, 0) < 0) {
        RETURN_FALSE;
    }

    // Prefix our key if necissary
    key_free = redis_key_prefix(redis_sock, &key, &key_len TSRMLS_CC);

    // If we have two arguments, we're running with an optional COUNT, which will return
    // a multibulk reply.  Without the argument we'll return a string response
    if(ZEND_NUM_ARGS() == 2) {
        // Construct our command with count
        cmd_len = redis_cmd_format_static(&cmd, "SRANDMEMBER", "sl", key, key_len, count);
    } else {
        // Construct our command
        cmd_len = redis_cmd_format_static(&cmd, "SRANDMEMBER", "s", key, key_len);
    }

    // Free our key if we prefixed it
    if(key_free) efree(key);

    // Process our command
    REDIS_PROCESS_REQUEST(redis_sock, cmd, cmd_len);

    // Either bulk or multi-bulk depending on argument count
    if(ZEND_NUM_ARGS() == 2) {
        IF_ATOMIC() {
            if(redis_sock_read_multibulk_reply(INTERNAL_FUNCTION_PARAM_PASSTHRU,
                                               redis_sock, NULL, NULL) < 0)
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

/* {{{ proto boolean Redis::sContains(string set, string value)
 */
PHP_METHOD(Redis, sContains)
{
	zval *object;
    RedisSock *redis_sock;
    char *key = NULL, *val = NULL, *cmd;
    int key_len, val_len, cmd_len;
    int val_free, key_free = 0;
    zval *z_value;

    if (zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), "Osz",
                                     &object, redis_ce,
                                     &key, &key_len, &z_value) == FAILURE) {
        return;
    }

    if (redis_sock_get(object, &redis_sock TSRMLS_CC, 0) < 0) {
        RETURN_FALSE;
    }

    val_free = redis_serialize(redis_sock, z_value, &val, &val_len TSRMLS_CC);
	key_free = redis_key_prefix(redis_sock, &key, &key_len TSRMLS_CC);
    cmd_len = redis_cmd_format_static(&cmd, "SISMEMBER", "ss", key, key_len, val, val_len);
    if(val_free) efree(val);
    if(key_free) efree(key);

	REDIS_PROCESS_REQUEST(redis_sock, cmd, cmd_len);
	IF_ATOMIC() {
		redis_1_response(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock, NULL, NULL);
	}
	REDIS_PROCESS_RESPONSE(redis_1_response);
}
/* }}} */

/* {{{ proto array Redis::sMembers(string set)
 */
PHP_METHOD(Redis, sMembers)
{
    zval *object;
    RedisSock *redis_sock;
    char *key = NULL, *cmd;
    int key_len, cmd_len, key_free;

    if (zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), "Os",
                                     &object, redis_ce,
                                     &key, &key_len) == FAILURE) {
        RETURN_FALSE;
    }

    if (redis_sock_get(object, &redis_sock TSRMLS_CC, 0) < 0) {
        RETURN_FALSE;
    }

	key_free = redis_key_prefix(redis_sock, &key, &key_len TSRMLS_CC);
    cmd_len = redis_cmd_format_static(&cmd, "SMEMBERS", "s", key, key_len);
	if(key_free) efree(key);

	REDIS_PROCESS_REQUEST(redis_sock, cmd, cmd_len);
	IF_ATOMIC() {
	    if (redis_sock_read_multibulk_reply(INTERNAL_FUNCTION_PARAM_PASSTHRU,
    	                                    redis_sock, NULL, NULL) < 0) {
        	RETURN_FALSE;
	    }
	}
    REDIS_PROCESS_RESPONSE(redis_sock_read_multibulk_reply);
}
/* }}} */

PHPAPI int generic_multiple_args_cmd(INTERNAL_FUNCTION_PARAMETERS, char *keyword, int keyword_len,
									 int min_argc, RedisSock **out_sock, int has_timeout, int all_keys, int can_serialize)
{
    zval **z_args, *z_array;
    char **keys, *cmd;
    int cmd_len, *keys_len, *keys_to_free;
    int i, j, argc = ZEND_NUM_ARGS(), real_argc = 0;
    int single_array = 0;
	int timeout = 0;
	int pos;
	int array_size;

    RedisSock *redis_sock;

    if(argc < min_argc) {
		zend_wrong_param_count(TSRMLS_C);
		ZVAL_BOOL(return_value, 0);
		return FAILURE;
    }

	/* get redis socket */
    if (redis_sock_get(getThis(), out_sock TSRMLS_CC, 0) < 0) {
		ZVAL_BOOL(return_value, 0);
		return FAILURE;
    }
    redis_sock = *out_sock;

    z_args = emalloc(argc * sizeof(zval*));
    if(zend_get_parameters_array(ht, argc, z_args) == FAILURE) {
        efree(z_args);
		ZVAL_BOOL(return_value, 0);
		return FAILURE;
    }

    /* case of a single array */
	if(has_timeout == 0) {
	    if(argc == 1 && Z_TYPE_P(z_args[0]) == IS_ARRAY) {
    	    single_array = 1;
        	z_array = z_args[0];
	        efree(z_args);
    	    z_args = NULL;

        	/* new count */
	        argc = zend_hash_num_elements(Z_ARRVAL_P(z_array));
    	}
	} else if(has_timeout == 1) {
		if(argc == 2 && Z_TYPE_P(z_args[0]) == IS_ARRAY && Z_TYPE_P(z_args[1]) == IS_LONG) {
    	    single_array = 1;
        	z_array = z_args[0];
			timeout = Z_LVAL_P(z_args[1]);
	        efree(z_args);
    	    z_args = NULL;
        	/* new count */
	        argc = zend_hash_num_elements(Z_ARRVAL_P(z_array));
    	}
	}

	/* prepare an array for the keys, one for their lengths, one to mark the keys to free. */
	array_size = argc;
	if(has_timeout)
		array_size++;

	keys = emalloc(array_size * sizeof(char*));
	keys_len = emalloc(array_size * sizeof(int));
	keys_to_free = emalloc(array_size * sizeof(int));
	memset(keys_to_free, 0, array_size * sizeof(int));


    cmd_len = 1 + integer_length(keyword_len) + 2 +keyword_len + 2; /* start computing the command length */

    if(single_array) { /* loop over the array */
        HashTable *keytable = Z_ARRVAL_P(z_array);

        for(j = 0, zend_hash_internal_pointer_reset(keytable);
            zend_hash_has_more_elements(keytable) == SUCCESS;
            zend_hash_move_forward(keytable)) {

            char *key;
            unsigned int key_len;
            unsigned long idx;
            zval **z_value_pp;

            zend_hash_get_current_key_ex(keytable, &key, &key_len, &idx, 0, NULL);
            if(zend_hash_get_current_data(keytable, (void**)&z_value_pp) == FAILURE) {
                continue; 	/* this should never happen, according to the PHP people. */
            }


			if(!all_keys && j != 0) { /* not just operating on keys */

				if(can_serialize) {
					keys_to_free[j] = redis_serialize(redis_sock, *z_value_pp, &keys[j], &keys_len[j] TSRMLS_CC);
				} else {
					convert_to_string(*z_value_pp);
					keys[j] = Z_STRVAL_PP(z_value_pp);
					keys_len[j] = Z_STRLEN_PP(z_value_pp);
					keys_to_free[j] = 0;
				}

			} else {

				/* only accept strings */
				if(Z_TYPE_PP(z_value_pp) != IS_STRING) {
					convert_to_string(*z_value_pp);
				}

                /* get current value */
                keys[j] = Z_STRVAL_PP(z_value_pp);
                keys_len[j] = Z_STRLEN_PP(z_value_pp);

				keys_to_free[j] = redis_key_prefix(redis_sock, &keys[j], &keys_len[j] TSRMLS_CC); /* add optional prefix */
			}

            cmd_len += 1 + integer_length(keys_len[j]) + 2 + keys_len[j] + 2; /* $ + size + NL + string + NL */
            j++;
            real_argc++;
        }
		if(has_timeout) {
			keys_len[j] = spprintf(&keys[j], 0, "%d", timeout);
			cmd_len += 1 + integer_length(keys_len[j]) + 2 + keys_len[j] + 2; // $ + size + NL + string + NL 
			j++;
			real_argc++;
		}
    } else {
		if(has_timeout && Z_TYPE_P(z_args[argc - 1]) != IS_LONG) {
			php_error_docref(NULL TSRMLS_CC, E_ERROR, "Syntax error on timeout");
		}
			
        for(i = 0, j = 0; i < argc; ++i) { /* store each key */
			if(!all_keys && j != 0) { /* not just operating on keys */

				if(can_serialize) {
					keys_to_free[j] = redis_serialize(redis_sock, z_args[i], &keys[j], &keys_len[j] TSRMLS_CC);
				} else {
					convert_to_string(z_args[i]);
					keys[j] = Z_STRVAL_P(z_args[i]);
					keys_len[j] = Z_STRLEN_P(z_args[i]);
					keys_to_free[j] = 0;
				}

			} else {
				
				if(Z_TYPE_P(z_args[i]) != IS_STRING) {
					convert_to_string(z_args[i]);
				}

       	        keys[j] = Z_STRVAL_P(z_args[i]);
           	    keys_len[j] = Z_STRLEN_P(z_args[i]);

           	    // If we have a timeout it should be the last argument, which we do not want to prefix
				if(!has_timeout || i < argc-1) {
					keys_to_free[j] = redis_key_prefix(redis_sock, &keys[j], &keys_len[j] TSRMLS_CC); /* add optional prefix  TSRMLS_CC*/
				}
			}

            cmd_len += 1 + integer_length(keys_len[j]) + 2 + keys_len[j] + 2; /* $ + size + NL + string + NL */
            j++;
   	        real_argc++;
		}
    }

    cmd_len += 1 + integer_length(real_argc+1) + 2; // *count NL 
    cmd = emalloc(cmd_len+1);

    sprintf(cmd, "*%d" _NL "$%d" _NL "%s" _NL, 1+real_argc, keyword_len, keyword);
	
    pos = 1 +integer_length(real_argc + 1) + 2
          + 1 + integer_length(keyword_len) + 2
          + keyword_len + 2;

    /* copy each key to its destination */
    for(i = 0; i < real_argc; ++i) {
        sprintf(cmd + pos, "$%d" _NL, keys_len[i]);     // size
        pos += 1 + integer_length(keys_len[i]) + 2;
        memcpy(cmd + pos, keys[i], keys_len[i]);
        pos += keys_len[i];
        memcpy(cmd + pos, _NL, 2);
        pos += 2;
    }

	/* cleanup prefixed keys. */
	for(i = 0; i < real_argc + (has_timeout?-1:0); ++i) {
		if(keys_to_free[i])
			efree(keys[i]);
	}
	if(single_array && has_timeout) { /* cleanup string created to contain timeout value */
		efree(keys[real_argc-1]);
	}

    efree(keys);
	efree(keys_len);
	efree(keys_to_free);

    if(z_args) efree(z_args);

	/* call REDIS_PROCESS_REQUEST and skip void returns */
	IF_MULTI_OR_ATOMIC() {
		if(redis_sock_write(redis_sock, cmd, cmd_len TSRMLS_CC) < 0) {
			efree(cmd);
			return FAILURE;
		}
		efree(cmd);
	}
	IF_PIPELINE() {
		PIPELINE_ENQUEUE_COMMAND(cmd, cmd_len);
		efree(cmd);
	}

    return SUCCESS;
}

/* {{{ proto array Redis::sInter(string key0, ... string keyN)
 */
PHP_METHOD(Redis, sInter) {

    RedisSock *redis_sock;

    if(FAILURE == generic_multiple_args_cmd(INTERNAL_FUNCTION_PARAM_PASSTHRU,
                    "SINTER", sizeof("SINTER") - 1,
					0, &redis_sock, 0, 1, 1))
		return;

    IF_ATOMIC() {
    	if (redis_sock_read_multibulk_reply(INTERNAL_FUNCTION_PARAM_PASSTHRU,
											redis_sock, NULL, NULL) < 0) {
        	RETURN_FALSE;
	    }
    }
    REDIS_PROCESS_RESPONSE(redis_sock_read_multibulk_reply);
}
/* }}} */

/* {{{ proto array Redis::sInterStore(string destination, string key0, ... string keyN)
 */
PHP_METHOD(Redis, sInterStore) {

    RedisSock *redis_sock;

    if(FAILURE == generic_multiple_args_cmd(INTERNAL_FUNCTION_PARAM_PASSTHRU,
                    "SINTERSTORE", sizeof("SINTERSTORE") - 1,
					1, &redis_sock, 0, 1, 1))
		return;

	IF_ATOMIC() {
		redis_long_response(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock, NULL, NULL);
	}
    REDIS_PROCESS_RESPONSE(redis_long_response);


}
/* }}} */

/* {{{ proto array Redis::sUnion(string key0, ... string keyN)
 */
PHP_METHOD(Redis, sUnion) {

    RedisSock *redis_sock;

    if(FAILURE == generic_multiple_args_cmd(INTERNAL_FUNCTION_PARAM_PASSTHRU,
                    "SUNION", sizeof("SUNION") - 1,
							  0, &redis_sock, 0, 1, 1))
		return;

	IF_ATOMIC() {
    	if (redis_sock_read_multibulk_reply(INTERNAL_FUNCTION_PARAM_PASSTHRU,
        	                                redis_sock, NULL, NULL) < 0) {
	        RETURN_FALSE;
    	}
	}
	REDIS_PROCESS_RESPONSE(redis_sock_read_multibulk_reply);
}
/* }}} */
/* {{{ proto array Redis::sUnionStore(string destination, string key0, ... string keyN)
 */
PHP_METHOD(Redis, sUnionStore) {

    RedisSock *redis_sock;

    if(FAILURE == generic_multiple_args_cmd(INTERNAL_FUNCTION_PARAM_PASSTHRU,
                    "SUNIONSTORE", sizeof("SUNIONSTORE") - 1,
					1, &redis_sock, 0, 1, 1))
		return;

	IF_ATOMIC() {
		redis_long_response(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock, NULL, NULL);
	}
	REDIS_PROCESS_RESPONSE(redis_long_response);
}

/* }}} */

/* {{{ proto array Redis::sDiff(string key0, ... string keyN)
 */
PHP_METHOD(Redis, sDiff) {

    RedisSock *redis_sock;

    if(FAILURE == generic_multiple_args_cmd(INTERNAL_FUNCTION_PARAM_PASSTHRU,
                    "SDIFF", sizeof("SDIFF") - 1,
					0, &redis_sock, 0, 1, 1))
		return;

	IF_ATOMIC() {
	    /* read multibulk reply */
    	if (redis_sock_read_multibulk_reply(INTERNAL_FUNCTION_PARAM_PASSTHRU,
											redis_sock, NULL, NULL) < 0) {
        	RETURN_FALSE;
	    }
	}
	REDIS_PROCESS_RESPONSE(redis_sock_read_multibulk_reply);
}
/* }}} */

/* {{{ proto array Redis::sDiffStore(string destination, string key0, ... string keyN)
 */
PHP_METHOD(Redis, sDiffStore) {

    RedisSock *redis_sock;

    if(FAILURE == generic_multiple_args_cmd(INTERNAL_FUNCTION_PARAM_PASSTHRU,
                    "SDIFFSTORE", sizeof("SDIFFSTORE") - 1,
					1, &redis_sock, 0, 1, 1))
		return;

	IF_ATOMIC() {
	  redis_long_response(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock, NULL, NULL);
	}
	REDIS_PROCESS_RESPONSE(redis_long_response);
}
/* }}} */

PHP_METHOD(Redis, sort) {

    zval *object = getThis(), *z_array = NULL, **z_cur;
    char *cmd, *old_cmd = NULL, *key;
    int cmd_len, elements = 2, key_len, key_free;
    int using_store = 0;
    RedisSock *redis_sock;

    if (zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), "Os|a",
                                     &object, redis_ce,
                                     &key, &key_len, &z_array) == FAILURE) {
        RETURN_FALSE;
    }

    if (redis_sock_get(object, &redis_sock TSRMLS_CC, 0) < 0) {
        RETURN_FALSE;
    }

    key_free = redis_key_prefix(redis_sock, &key, &key_len TSRMLS_CC);
    cmd_len = redis_cmd_format(&cmd, "$4" _NL "SORT" _NL "$%d" _NL "%s" _NL, key_len, key, key_len);
    if(key_free) efree(key);

    if(z_array) {
        if ((zend_hash_find(Z_ARRVAL_P(z_array), "by", sizeof("by"), (void **) &z_cur) == SUCCESS
         || zend_hash_find(Z_ARRVAL_P(z_array), "BY", sizeof("BY"), (void **) &z_cur) == SUCCESS)
                        && Z_TYPE_PP(z_cur) == IS_STRING) {

            old_cmd = cmd;
            cmd_len = redis_cmd_format(&cmd, "%s"
                                             "$2" _NL
                                             "BY" _NL
                                             "$%d" _NL
                                             "%s" _NL
                                             , cmd, cmd_len
                                             , Z_STRLEN_PP(z_cur), Z_STRVAL_PP(z_cur), Z_STRLEN_PP(z_cur));
            elements += 2;
            efree(old_cmd);

        }

        if ((zend_hash_find(Z_ARRVAL_P(z_array), "sort", sizeof("sort"), (void **) &z_cur) == SUCCESS
         || zend_hash_find(Z_ARRVAL_P(z_array), "SORT", sizeof("SORT"), (void **) &z_cur) == SUCCESS)
                        && Z_TYPE_PP(z_cur) == IS_STRING) {

            old_cmd = cmd;
            cmd_len = redis_cmd_format(&cmd, "%s"
                                             "$%d" _NL
                                             "%s" _NL
                                             , cmd, cmd_len
                                             , Z_STRLEN_PP(z_cur), Z_STRVAL_PP(z_cur), Z_STRLEN_PP(z_cur));
            elements += 1;
            efree(old_cmd);
        }

        if ((zend_hash_find(Z_ARRVAL_P(z_array), "store", sizeof("store"), (void **) &z_cur) == SUCCESS
         || zend_hash_find(Z_ARRVAL_P(z_array), "STORE", sizeof("STORE"), (void **) &z_cur) == SUCCESS)
                        && Z_TYPE_PP(z_cur) == IS_STRING) {

            using_store = 1;
            old_cmd = cmd;
            cmd_len = redis_cmd_format(&cmd, "%s"
                                             "$5" _NL
                                             "STORE" _NL
                                             "$%d" _NL
                                             "%s" _NL
                                             , cmd, cmd_len
                                             , Z_STRLEN_PP(z_cur), Z_STRVAL_PP(z_cur), Z_STRLEN_PP(z_cur));
            elements += 2;
            efree(old_cmd);
        }

        if ((zend_hash_find(Z_ARRVAL_P(z_array), "get", sizeof("get"), (void **) &z_cur) == SUCCESS
         || zend_hash_find(Z_ARRVAL_P(z_array), "GET", sizeof("GET"), (void **) &z_cur) == SUCCESS)
                        && (Z_TYPE_PP(z_cur) == IS_STRING || Z_TYPE_PP(z_cur) == IS_ARRAY)) {

            if(Z_TYPE_PP(z_cur) == IS_STRING) {
                old_cmd = cmd;
                cmd_len = redis_cmd_format(&cmd, "%s"
                                                 "$3" _NL
                                                 "GET" _NL
                                                 "$%d" _NL
                                                 "%s" _NL
                                                 , cmd, cmd_len
                                                 , Z_STRLEN_PP(z_cur), Z_STRVAL_PP(z_cur), Z_STRLEN_PP(z_cur));
                elements += 2;
                efree(old_cmd);
            } else if(Z_TYPE_PP(z_cur) == IS_ARRAY) { // loop over the strings in that array and add them as patterns

                HashTable *keytable = Z_ARRVAL_PP(z_cur);
                for(zend_hash_internal_pointer_reset(keytable);
                    zend_hash_has_more_elements(keytable) == SUCCESS;
                    zend_hash_move_forward(keytable)) {

                    char *key;
                    unsigned int key_len;
                    unsigned long idx;
                    zval **z_value_pp;

                    zend_hash_get_current_key_ex(keytable, &key, &key_len, &idx, 0, NULL);
                    if(zend_hash_get_current_data(keytable, (void**)&z_value_pp) == FAILURE) {
                        continue; 	/* this should never happen, according to the PHP people. */
                    }

                    if(Z_TYPE_PP(z_value_pp) == IS_STRING) {
                        old_cmd = cmd;
                        cmd_len = redis_cmd_format(&cmd, "%s"
                                                         "$3" _NL
                                                         "GET" _NL
                                                         "$%d" _NL
                                                         "%s" _NL
                                                         , cmd, cmd_len
                                                         , Z_STRLEN_PP(z_value_pp), Z_STRVAL_PP(z_value_pp), Z_STRLEN_PP(z_value_pp));
                        elements += 2;
                        efree(old_cmd);
                    }
                }
            }
        }

        if ((zend_hash_find(Z_ARRVAL_P(z_array), "alpha", sizeof("alpha"), (void **) &z_cur) == SUCCESS
         || zend_hash_find(Z_ARRVAL_P(z_array), "ALPHA", sizeof("ALPHA"), (void **) &z_cur) == SUCCESS)
                        && Z_TYPE_PP(z_cur) == IS_BOOL && Z_BVAL_PP(z_cur) == 1) {

            old_cmd = cmd;
            cmd_len = redis_cmd_format(&cmd, "%s"
                                             "$5" _NL
                                             "ALPHA" _NL
                                             , cmd, cmd_len);
            elements += 1;
            efree(old_cmd);
        }

        if ((zend_hash_find(Z_ARRVAL_P(z_array), "limit", sizeof("limit"), (void **) &z_cur) == SUCCESS
         || zend_hash_find(Z_ARRVAL_P(z_array), "LIMIT", sizeof("LIMIT"), (void **) &z_cur) == SUCCESS)
                        && Z_TYPE_PP(z_cur) == IS_ARRAY) {

            if(zend_hash_num_elements(Z_ARRVAL_PP(z_cur)) == 2) {
                zval **z_offset_pp, **z_count_pp;
                // get the two values from the table, check that they are indeed of LONG type
                if(SUCCESS == zend_hash_index_find(Z_ARRVAL_PP(z_cur), 0, (void**)&z_offset_pp) &&
                  SUCCESS == zend_hash_index_find(Z_ARRVAL_PP(z_cur), 1, (void**)&z_count_pp)) {

                    long limit_low, limit_high;
					if((Z_TYPE_PP(z_offset_pp) == IS_LONG || Z_TYPE_PP(z_offset_pp) == IS_STRING) &&
						(Z_TYPE_PP(z_count_pp) == IS_LONG || Z_TYPE_PP(z_count_pp) == IS_STRING)) {


						if(Z_TYPE_PP(z_offset_pp) == IS_LONG) {
							limit_low = Z_LVAL_PP(z_offset_pp);
						} else {
							limit_low = atol(Z_STRVAL_PP(z_offset_pp));
						}
						if(Z_TYPE_PP(z_count_pp) == IS_LONG) {
							limit_high = Z_LVAL_PP(z_count_pp);
						} else {
							limit_high = atol(Z_STRVAL_PP(z_count_pp));
						}

						old_cmd = cmd;
						cmd_len = redis_cmd_format(&cmd, "%s"
														 "$5" _NL
														 "LIMIT" _NL
														 "$%d" _NL
														 "%d" _NL
														 "$%d" _NL
														 "%d" _NL
														 , cmd, cmd_len
														 , integer_length(limit_low), limit_low
														 , integer_length(limit_high), limit_high);
						elements += 3;
						efree(old_cmd);
					}
                }
            }
        }

    }

    /* complete with prefix */

    old_cmd = cmd;
    cmd_len = redis_cmd_format(&cmd, "*%d" _NL "%s", elements, cmd, cmd_len);
    efree(old_cmd);

    /* run command */
    REDIS_PROCESS_REQUEST(redis_sock, cmd, cmd_len);
    if(using_store) {
        IF_ATOMIC() {
            redis_long_response(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock, NULL, NULL);
        }
        REDIS_PROCESS_RESPONSE(redis_long_response);
    } else {
        IF_ATOMIC() {
            if (redis_sock_read_multibulk_reply(INTERNAL_FUNCTION_PARAM_PASSTHRU,
                                                redis_sock, NULL, NULL) < 0) {
                RETURN_FALSE;
            }
        }
        REDIS_PROCESS_RESPONSE(redis_sock_read_multibulk_reply);
    }
}

PHPAPI void generic_sort_cmd(INTERNAL_FUNCTION_PARAMETERS, char *sort, int use_alpha) {

    zval *object;
    RedisSock *redis_sock;
    char *key = NULL, *pattern = NULL, *get = NULL, *store = NULL, *cmd;
    int key_len, pattern_len = -1, get_len = -1, store_len = -1, cmd_len, key_free;
    long sort_start = -1, sort_count = -1;

    int cmd_elements;

    char *cmd_lines[30];
    int cmd_sizes[30];

	int sort_len;
    int i, pos;

    if (zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), "Os|sslls",
                                     &object, redis_ce,
                                     &key, &key_len, &pattern, &pattern_len,
                                     &get, &get_len, &sort_start, &sort_count, &store, &store_len) == FAILURE) {
        RETURN_FALSE;
    }

    if (redis_sock_get(object, &redis_sock TSRMLS_CC, 0) < 0) {
        RETURN_FALSE;
    }
    if(key_len == 0) {
        RETURN_FALSE;
    }

    /* first line, sort. */
    cmd_lines[1] = estrdup("$4");
    cmd_sizes[1] = 2;
    cmd_lines[2] = estrdup("SORT");
    cmd_sizes[2] = 4;

    // Prefix our key if we need to
    key_free = redis_key_prefix(redis_sock, &key, &key_len TSRMLS_CC);

    /* second line, key */
    cmd_sizes[3] = redis_cmd_format(&cmd_lines[3], "$%d", key_len);
    cmd_lines[4] = emalloc(key_len + 1);
    memcpy(cmd_lines[4], key, key_len);
    cmd_lines[4][key_len] = 0;
    cmd_sizes[4] = key_len;

    // If we prefixed our key, free it
    if(key_free) efree(key);

    cmd_elements = 5;
    if(pattern && pattern_len) {
            /* BY */
            cmd_lines[cmd_elements] = estrdup("$2");
            cmd_sizes[cmd_elements] = 2;
            cmd_elements++;
            cmd_lines[cmd_elements] = estrdup("BY");
            cmd_sizes[cmd_elements] = 2;
            cmd_elements++;

            /* pattern */
            cmd_sizes[cmd_elements] = redis_cmd_format(&cmd_lines[cmd_elements], "$%d", pattern_len);
            cmd_elements++;
            cmd_lines[cmd_elements] = emalloc(pattern_len + 1);
            memcpy(cmd_lines[cmd_elements], pattern, pattern_len);
            cmd_lines[cmd_elements][pattern_len] = 0;
            cmd_sizes[cmd_elements] = pattern_len;
            cmd_elements++;
    }
    if(sort_start >= 0 && sort_count >= 0) {
            /* LIMIT */
            cmd_lines[cmd_elements] = estrdup("$5");
            cmd_sizes[cmd_elements] = 2;
            cmd_elements++;
            cmd_lines[cmd_elements] = estrdup("LIMIT");
            cmd_sizes[cmd_elements] = 5;
            cmd_elements++;

            /* start */
            cmd_sizes[cmd_elements] = redis_cmd_format(&cmd_lines[cmd_elements], "$%d", integer_length(sort_start));
            cmd_elements++;
            cmd_sizes[cmd_elements] = spprintf(&cmd_lines[cmd_elements], 0, "%d", (int)sort_start);
            cmd_elements++;

            /* count */
            cmd_sizes[cmd_elements] = redis_cmd_format(&cmd_lines[cmd_elements], "$%d", integer_length(sort_count));
            cmd_elements++;
            cmd_sizes[cmd_elements] = spprintf(&cmd_lines[cmd_elements], 0, "%d", (int)sort_count);
            cmd_elements++;
    }
    if(get && get_len) {
            /* GET */
            cmd_lines[cmd_elements] = estrdup("$3");
            cmd_sizes[cmd_elements] = 2;
            cmd_elements++;
            cmd_lines[cmd_elements] = estrdup("GET");
            cmd_sizes[cmd_elements] = 3;
            cmd_elements++;

            /* pattern */
            cmd_sizes[cmd_elements] = redis_cmd_format(&cmd_lines[cmd_elements], "$%d", get_len);
            cmd_elements++;
            cmd_lines[cmd_elements] = emalloc(get_len + 1);
            memcpy(cmd_lines[cmd_elements], get, get_len);
            cmd_lines[cmd_elements][get_len] = 0;
            cmd_sizes[cmd_elements] = get_len;
            cmd_elements++;
    }

    /* add ASC or DESC */
    sort_len = strlen(sort);
    cmd_sizes[cmd_elements] = redis_cmd_format(&cmd_lines[cmd_elements], "$%d", sort_len);
    cmd_elements++;
    cmd_lines[cmd_elements] = emalloc(sort_len + 1);
    memcpy(cmd_lines[cmd_elements], sort, sort_len);
    cmd_lines[cmd_elements][sort_len] = 0;
    cmd_sizes[cmd_elements] = sort_len;
    cmd_elements++;

    if(use_alpha) {
            /* ALPHA */
            cmd_lines[cmd_elements] = estrdup("$5");
            cmd_sizes[cmd_elements] = 2;
            cmd_elements++;
            cmd_lines[cmd_elements] = estrdup("ALPHA");
            cmd_sizes[cmd_elements] = 5;
            cmd_elements++;
    }
    if(store && store_len) {
            /* STORE */
            cmd_lines[cmd_elements] = estrdup("$5");
            cmd_sizes[cmd_elements] = 2;
            cmd_elements++;
            cmd_lines[cmd_elements] = estrdup("STORE");
            cmd_sizes[cmd_elements] = 5;
            cmd_elements++;

            /* store key */
            cmd_sizes[cmd_elements] = redis_cmd_format(&cmd_lines[cmd_elements], "$%d", store_len);
            cmd_elements++;
            cmd_lines[cmd_elements] = emalloc(store_len + 1);
            memcpy(cmd_lines[cmd_elements], store, store_len);
            cmd_lines[cmd_elements][store_len] = 0;
            cmd_sizes[cmd_elements] = store_len;
            cmd_elements++;
    }

    /* first line has the star */
    cmd_sizes[0] = spprintf(&cmd_lines[0], 0, "*%d", (cmd_elements-1)/2);

    /* compute the command size */
    cmd_len = 0;
    for(i = 0; i < cmd_elements; ++i) {
            cmd_len += cmd_sizes[i] + sizeof(_NL) - 1; /* each line followeb by _NL */
    }

    /* copy all lines into the final command. */
    cmd = emalloc(1 + cmd_len);
    pos = 0;
    for(i = 0; i < cmd_elements; ++i) {
        memcpy(cmd + pos, cmd_lines[i], cmd_sizes[i]);
        pos += cmd_sizes[i];
        memcpy(cmd + pos, _NL, sizeof(_NL) - 1);
        pos += sizeof(_NL) - 1;

        /* free every line */
        efree(cmd_lines[i]);
    }

	REDIS_PROCESS_REQUEST(redis_sock, cmd, cmd_len);
	IF_ATOMIC() {
    	if (redis_sock_read_multibulk_reply(INTERNAL_FUNCTION_PARAM_PASSTHRU,
											redis_sock, NULL, NULL) < 0) {
        	RETURN_FALSE;
	    }
	}
	REDIS_PROCESS_RESPONSE(redis_sock_read_multibulk_reply);

}

/* {{{ proto array Redis::sortAsc(string key, string pattern, string get, int start, int end, bool getList])
 */
PHP_METHOD(Redis, sortAsc)
{
    generic_sort_cmd(INTERNAL_FUNCTION_PARAM_PASSTHRU, "ASC", 0);
}
/* }}} */

/* {{{ proto array Redis::sortAscAlpha(string key, string pattern, string get, int start, int end, bool getList])
 */
PHP_METHOD(Redis, sortAscAlpha)
{
    generic_sort_cmd(INTERNAL_FUNCTION_PARAM_PASSTHRU, "ASC", 1);
}
/* }}} */

/* {{{ proto array Redis::sortDesc(string key, string pattern, string get, int start, int end, bool getList])
 */
PHP_METHOD(Redis, sortDesc)
{
    generic_sort_cmd(INTERNAL_FUNCTION_PARAM_PASSTHRU, "DESC", 0);
}
/* }}} */

/* {{{ proto array Redis::sortDescAlpha(string key, string pattern, string get, int start, int end, bool getList])
 */
PHP_METHOD(Redis, sortDescAlpha)
{
    generic_sort_cmd(INTERNAL_FUNCTION_PARAM_PASSTHRU, "DESC", 1);
}
/* }}} */

PHPAPI void generic_expire_cmd(INTERNAL_FUNCTION_PARAMETERS, char *keyword, int keyword_len) {
    zval *object;
    RedisSock *redis_sock;
    char *key = NULL, *cmd, *t;
    int key_len, cmd_len, key_free, t_len;
	int i;

    if (zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), "Oss",
                                     &object, redis_ce, &key, &key_len,
                                     &t, &t_len) == FAILURE) {
        RETURN_FALSE;
    }

    if (redis_sock_get(object, &redis_sock TSRMLS_CC, 0) < 0) {
        RETURN_FALSE;
    }

	/* check that we have a number */
	for(i = 0; i < t_len; ++i)
		if(t[i] < '0' || t[i] > '9')
			RETURN_FALSE;

	key_free = redis_key_prefix(redis_sock, &key, &key_len TSRMLS_CC);
    cmd_len = redis_cmd_format_static(&cmd, keyword, "ss", key, key_len, t, t_len);
	if(key_free) efree(key);

	REDIS_PROCESS_REQUEST(redis_sock, cmd, cmd_len);
	IF_ATOMIC() {
		redis_1_response(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock, NULL, NULL);
	}
	REDIS_PROCESS_RESPONSE(redis_1_response);
}

/* {{{ proto array Redis::setTimeout(string key, int timeout)
 */
PHP_METHOD(Redis, setTimeout) {
    generic_expire_cmd(INTERNAL_FUNCTION_PARAM_PASSTHRU, "EXPIRE", sizeof("EXPIRE")-1);
}

PHP_METHOD(Redis, pexpire) {
    generic_expire_cmd(INTERNAL_FUNCTION_PARAM_PASSTHRU, "PEXPIRE", sizeof("PEXPIRE")-1);
}
/* }}} */

/* {{{ proto array Redis::expireAt(string key, int timestamp)
 */
PHP_METHOD(Redis, expireAt) {
    generic_expire_cmd(INTERNAL_FUNCTION_PARAM_PASSTHRU, "EXPIREAT", sizeof("EXPIREAT")-1);
}
/* }}} */

/* {{{ proto array Redis::pexpireAt(string key, int timestamp)
 */
PHP_METHOD(Redis, pexpireAt) {
    generic_expire_cmd(INTERNAL_FUNCTION_PARAM_PASSTHRU, "PEXPIREAT", sizeof("PEXPIREAT")-1);
}
/* }}} */


/* {{{ proto array Redis::lSet(string key, int index, string value)
 */
PHP_METHOD(Redis, lSet) {

    zval *object;
    RedisSock *redis_sock;

    char *cmd;
    int cmd_len, key_len, val_len;
    long index;
    char *key, *val;
    int val_free, key_free = 0;
    zval *z_value;

    if (zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), "Oslz",
                                     &object, redis_ce, &key, &key_len, &index, &z_value) == FAILURE) {
        RETURN_FALSE;
    }

    if (redis_sock_get(object, &redis_sock TSRMLS_CC, 0) < 0) {
        RETURN_FALSE;
    }

    val_free = redis_serialize(redis_sock, z_value, &val, &val_len TSRMLS_CC);
	key_free = redis_key_prefix(redis_sock, &key, &key_len TSRMLS_CC);
    cmd_len = redis_cmd_format_static(&cmd, "LSET", "sds", key, key_len, index, val, val_len);
    if(val_free) efree(val);
    if(key_free) efree(key);

	REDIS_PROCESS_REQUEST(redis_sock, cmd, cmd_len);
	IF_ATOMIC() {
		redis_boolean_response(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock, NULL, NULL);
	}
	REDIS_PROCESS_RESPONSE(redis_boolean_response);
}
/* }}} */

PHPAPI void generic_empty_cmd_impl(INTERNAL_FUNCTION_PARAMETERS, char *cmd, int cmd_len, ResultCallback result_callback) {
    zval *object;
    RedisSock *redis_sock;

    if (zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), "O",
                                     &object, redis_ce) == FAILURE) {
        RETURN_FALSE;
    }

    if (redis_sock_get(object, &redis_sock TSRMLS_CC, 0) < 0) {
        RETURN_FALSE;
    }

	REDIS_PROCESS_REQUEST(redis_sock, cmd, cmd_len);
    IF_ATOMIC() {
        result_callback(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock, NULL, NULL);
    }
    REDIS_PROCESS_RESPONSE(result_callback);
}

PHPAPI void generic_empty_cmd(INTERNAL_FUNCTION_PARAMETERS, char *cmd, int cmd_len, ...) {
    generic_empty_cmd_impl(INTERNAL_FUNCTION_PARAM_PASSTHRU, cmd, cmd_len, redis_boolean_response);
}

/* {{{ proto string Redis::save()
 */
PHP_METHOD(Redis, save)
{
    char *cmd;
    int cmd_len = redis_cmd_format_static(&cmd, "SAVE", "");
    generic_empty_cmd(INTERNAL_FUNCTION_PARAM_PASSTHRU, cmd, cmd_len);

}
/* }}} */

/* {{{ proto string Redis::bgSave()
 */
PHP_METHOD(Redis, bgSave)
{
    char *cmd;
    int cmd_len = redis_cmd_format_static(&cmd, "BGSAVE", "");
    generic_empty_cmd(INTERNAL_FUNCTION_PARAM_PASSTHRU, cmd, cmd_len);

}
/* }}} */

PHPAPI void generic_empty_long_cmd(INTERNAL_FUNCTION_PARAMETERS, char *cmd, int cmd_len, ...) {

    zval *object;
    RedisSock *redis_sock;

    if (zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), "O",
                                     &object, redis_ce) == FAILURE) {
        RETURN_FALSE;
    }

    if (redis_sock_get(object, &redis_sock TSRMLS_CC, 0) < 0) {
        RETURN_FALSE;
    }

	REDIS_PROCESS_REQUEST(redis_sock, cmd, cmd_len);
    IF_ATOMIC() {
	  redis_long_response(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock, NULL, NULL);
    }
    REDIS_PROCESS_RESPONSE(redis_long_response);
}

/* {{{ proto integer Redis::lastSave()
 */
PHP_METHOD(Redis, lastSave)
{
    char *cmd;
    int cmd_len = redis_cmd_format_static(&cmd, "LASTSAVE", "");
    generic_empty_long_cmd(INTERNAL_FUNCTION_PARAM_PASSTHRU, cmd, cmd_len);
}
/* }}} */


/* {{{ proto bool Redis::flushDB()
 */
PHP_METHOD(Redis, flushDB)
{
    char *cmd;
    int cmd_len = redis_cmd_format_static(&cmd, "FLUSHDB", "");
    generic_empty_cmd(INTERNAL_FUNCTION_PARAM_PASSTHRU, cmd, cmd_len);
}
/* }}} */

/* {{{ proto bool Redis::flushAll()
 */
PHP_METHOD(Redis, flushAll)
{
    char *cmd;
    int cmd_len = redis_cmd_format_static(&cmd, "FLUSHALL", "");
    generic_empty_cmd(INTERNAL_FUNCTION_PARAM_PASSTHRU, cmd, cmd_len);
}
/* }}} */

/* {{{ proto int Redis::dbSize()
 */
PHP_METHOD(Redis, dbSize)
{
    char *cmd;
    int cmd_len = redis_cmd_format_static(&cmd, "DBSIZE", "");
    generic_empty_long_cmd(INTERNAL_FUNCTION_PARAM_PASSTHRU, cmd, cmd_len);
}
/* }}} */

/* {{{ proto bool Redis::auth(string passwd)
 */
PHP_METHOD(Redis, auth) {

    zval *object;
    RedisSock *redis_sock;

    char *cmd, *password;
    int cmd_len, password_len;

    if (zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), "Os",
                                     &object, redis_ce, &password, &password_len) == FAILURE) {
        RETURN_FALSE;
    }

    if (redis_sock_get(object, &redis_sock TSRMLS_CC, 0) < 0) {
        RETURN_FALSE;
    }

    cmd_len = redis_cmd_format_static(&cmd, "AUTH", "s", password, password_len);

    // Free previously stored auth if we have one, and store this password
    if(redis_sock->auth) efree(redis_sock->auth);
    redis_sock->auth = estrndup(password, password_len);

	REDIS_PROCESS_REQUEST(redis_sock, cmd, cmd_len);
	IF_ATOMIC() {
		redis_boolean_response(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock, NULL, NULL);
	}
	REDIS_PROCESS_RESPONSE(redis_boolean_response);
}
/* }}} */

/* {{{ proto long Redis::persist(string key)
 */
PHP_METHOD(Redis, persist) {

    zval *object;
    RedisSock *redis_sock;

    char *cmd, *key;
    int cmd_len, key_len, key_free;

    if (zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), "Os",
                                     &object, redis_ce, &key, &key_len) == FAILURE) {
        RETURN_FALSE;
    }

    if (redis_sock_get(object, &redis_sock TSRMLS_CC, 0) < 0) {
        RETURN_FALSE;
    }

	key_free = redis_key_prefix(redis_sock, &key, &key_len TSRMLS_CC);
    cmd_len = redis_cmd_format_static(&cmd, "PERSIST", "s", key, key_len);
	if(key_free) efree(key);

	REDIS_PROCESS_REQUEST(redis_sock, cmd, cmd_len);
	IF_ATOMIC() {
	  redis_1_response(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock, NULL, NULL);
	}
	REDIS_PROCESS_RESPONSE(redis_1_response);
}
/* }}} */

PHPAPI void generic_ttl(INTERNAL_FUNCTION_PARAMETERS, char *keyword) {
    zval *object;
    RedisSock *redis_sock;

    char *cmd, *key;
    int cmd_len, key_len, key_free;

    if (zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), "Os",
                                     &object, redis_ce, &key, &key_len) == FAILURE) {
        RETURN_FALSE;
    }

    if (redis_sock_get(object, &redis_sock TSRMLS_CC, 0) < 0) {
        RETURN_FALSE;
    }

	key_free = redis_key_prefix(redis_sock, &key, &key_len TSRMLS_CC);
    cmd_len = redis_cmd_format_static(&cmd, keyword, "s", key, key_len);
	if(key_free) efree(key);

	REDIS_PROCESS_REQUEST(redis_sock, cmd, cmd_len);
	IF_ATOMIC() {
	  redis_long_response(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock, NULL, NULL);
	}
	REDIS_PROCESS_RESPONSE(redis_long_response);
}

/* {{{ proto long Redis::ttl(string key)
 */
PHP_METHOD(Redis, ttl) {
	generic_ttl(INTERNAL_FUNCTION_PARAM_PASSTHRU, "TTL");
}
/* }}} */

/* {{{ proto long Redis::pttl(string key)
 */
PHP_METHOD(Redis, pttl) {
	generic_ttl(INTERNAL_FUNCTION_PARAM_PASSTHRU, "PTTL");
}
/* }}} */

/* {{{ proto array Redis::info()
 */
PHP_METHOD(Redis, info) {

    zval *object;
    RedisSock *redis_sock;
    char *cmd, *opt = NULL;
    int cmd_len, opt_len;

    if (zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), "O|s",
                                     &object, redis_ce, &opt, &opt_len) == FAILURE) {
        RETURN_FALSE;
    }

    if (redis_sock_get(object, &redis_sock TSRMLS_CC, 0) < 0) {
        RETURN_FALSE;
    }

    // Build a standalone INFO command or one with an option
    if(opt != NULL) {
    	cmd_len = redis_cmd_format_static(&cmd, "INFO", "s", opt, opt_len);
    } else {
    	cmd_len = redis_cmd_format_static(&cmd, "INFO", "");
    }

	REDIS_PROCESS_REQUEST(redis_sock, cmd, cmd_len);
	IF_ATOMIC() {
	  redis_info_response(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock, NULL, NULL);
	}
	REDIS_PROCESS_RESPONSE(redis_info_response);

}
/* }}} */

/* {{{ proto string Redis::resetStat()
 */
PHP_METHOD(Redis, resetStat)
{
	char *cmd;
	int cmd_len = redis_cmd_format_static(&cmd, "CONFIG", "s", "RESETSTAT", 9);
	generic_empty_cmd(INTERNAL_FUNCTION_PARAM_PASSTHRU, cmd, cmd_len);
}
/* }}} */

/* {{{ proto bool Redis::select(long dbNumber)
 */
PHP_METHOD(Redis, select) {

    zval *object;
    RedisSock *redis_sock;

    char *cmd;
    int cmd_len;
    long dbNumber;

    if (zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), "Ol",
                                     &object, redis_ce, &dbNumber) == FAILURE) {
        RETURN_FALSE;
    }

    if (redis_sock_get(object, &redis_sock TSRMLS_CC, 0) < 0) {
        RETURN_FALSE;
    }

    redis_sock->dbNumber = dbNumber;

    cmd_len = redis_cmd_format_static(&cmd, "SELECT", "d", dbNumber);

	REDIS_PROCESS_REQUEST(redis_sock, cmd, cmd_len);
	IF_ATOMIC() {
		redis_boolean_response(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock, NULL, NULL);
	}
	REDIS_PROCESS_RESPONSE(redis_boolean_response);
}
/* }}} */

/* {{{ proto bool Redis::move(string key, long dbindex)
 */
PHP_METHOD(Redis, move) {

    zval *object;
    RedisSock *redis_sock;

    char *cmd, *key;
    int cmd_len, key_len, key_free;
    long dbNumber;

    if (zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), "Osl",
                                     &object, redis_ce, &key, &key_len, &dbNumber) == FAILURE) {
        RETURN_FALSE;
    }

    if (redis_sock_get(object, &redis_sock TSRMLS_CC, 0) < 0) {
        RETURN_FALSE;
    }

	key_free = redis_key_prefix(redis_sock, &key, &key_len TSRMLS_CC);
    cmd_len = redis_cmd_format_static(&cmd, "MOVE", "sd", key, key_len, dbNumber);
	if(key_free) efree(key);

	REDIS_PROCESS_REQUEST(redis_sock, cmd, cmd_len);
	IF_ATOMIC() {
	  redis_1_response(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock, NULL, NULL);
	}
	REDIS_PROCESS_RESPONSE(redis_1_response);

}
/* }}} */

PHPAPI void
generic_mset(INTERNAL_FUNCTION_PARAMETERS, char *kw, void (*fun)(INTERNAL_FUNCTION_PARAMETERS, RedisSock *, zval *, void *)) {

    zval *object;
    RedisSock *redis_sock;

    char *cmd = NULL, *p = NULL;
    int cmd_len = 0, argc = 0, kw_len = strlen(kw);
	int step = 0;	// 0: compute size; 1: copy strings.
    zval *z_array;

	HashTable *keytable;

    if (zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), "Oa",
                                     &object, redis_ce, &z_array) == FAILURE) {
        RETURN_FALSE;
    }

    if (redis_sock_get(object, &redis_sock TSRMLS_CC, 0) < 0) {
        RETURN_FALSE;
    }

    if(zend_hash_num_elements(Z_ARRVAL_P(z_array)) == 0) {
        RETURN_FALSE;
    }

	for(step = 0; step < 2; ++step) {
		if(step == 1) {
			cmd_len += 1 + integer_length(1 + 2 * argc) + 2;	/* star + arg count + NL */
			cmd_len += 1 + integer_length(kw_len) + 2;			/* dollar + strlen(kw) + NL */
			cmd_len += kw_len + 2;								/* kw + NL */

			p = cmd = emalloc(cmd_len + 1);	/* alloc */
			p += sprintf(cmd, "*%d" _NL "$%d" _NL "%s" _NL, 1 + 2 * argc, kw_len, kw); /* copy header */
		}

		keytable = Z_ARRVAL_P(z_array);
		for(zend_hash_internal_pointer_reset(keytable);
				zend_hash_has_more_elements(keytable) == SUCCESS;
				zend_hash_move_forward(keytable)) {

			char *key, *val;
			unsigned int key_len;
			int val_len;
			unsigned long idx;
			int type;
			zval **z_value_pp;
			int val_free, key_free;
			char buf[32];

			type = zend_hash_get_current_key_ex(keytable, &key, &key_len, &idx, 0, NULL);
			if(zend_hash_get_current_data(keytable, (void**)&z_value_pp) == FAILURE) {
				continue; 	/* this should never happen, according to the PHP people. */
			}

			// If the key isn't a string, use the index value returned when grabbing the
			// key.  We typecast to long, because they could actually be negative.
			if(type != HASH_KEY_IS_STRING) {
				// Create string representation of our index
				key_len = snprintf(buf, sizeof(buf), "%ld", (long)idx);
				key = (char*)buf;
			} else if(key_len > 0) {
				// When not an integer key, the length will include the \0
				key_len--;
			}

			if(step == 0)
				argc++; /* found a valid arg */

			val_free = redis_serialize(redis_sock, *z_value_pp, &val, &val_len TSRMLS_CC);
			key_free = redis_key_prefix(redis_sock, &key, (int*)&key_len TSRMLS_CC);

			if(step == 0) { /* counting */
				cmd_len += 1 + integer_length(key_len) + 2
						+ key_len + 2
						+ 1 + integer_length(val_len) + 2
						+ val_len + 2;
			} else {
				p += sprintf(p, "$%d" _NL, key_len);	/* key len */
				memcpy(p, key, key_len); p += key_len;	/* key */
				memcpy(p, _NL, 2); p += 2;

				p += sprintf(p, "$%d" _NL, val_len);	/* val len */
				memcpy(p, val, val_len); p += val_len;	/* val */
				memcpy(p, _NL, 2); p += 2;
			}

			if(val_free) efree(val);
			if(key_free) efree(key);
		}
	}

	REDIS_PROCESS_REQUEST(redis_sock, cmd, cmd_len);

	IF_ATOMIC() {
		fun(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock, NULL, NULL);
	}
	REDIS_PROCESS_RESPONSE(fun);
}

/* {{{ proto bool Redis::mset(array (key => value, ...))
 */
PHP_METHOD(Redis, mset) {
	generic_mset(INTERNAL_FUNCTION_PARAM_PASSTHRU, "MSET", redis_boolean_response);
}
/* }}} */


/* {{{ proto bool Redis::msetnx(array (key => value, ...))
 */
PHP_METHOD(Redis, msetnx) {
	generic_mset(INTERNAL_FUNCTION_PARAM_PASSTHRU, "MSETNX", redis_1_response);
}
/* }}} */

PHPAPI void common_rpoplpush(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock,
		char *srckey, int srckey_len, char *dstkey, int dstkey_len, int timeout) {

	char *cmd;
	int cmd_len;

	int srckey_free = redis_key_prefix(redis_sock, &srckey, &srckey_len TSRMLS_CC);
	int dstkey_free = redis_key_prefix(redis_sock, &dstkey, &dstkey_len TSRMLS_CC);
	if(timeout < 0) {
		cmd_len = redis_cmd_format_static(&cmd, "RPOPLPUSH", "ss", srckey, srckey_len, dstkey, dstkey_len);
	} else {
		cmd_len = redis_cmd_format_static(&cmd, "BRPOPLPUSH", "ssd", srckey, srckey_len, dstkey, dstkey_len, timeout);
	}
	if(srckey_free) efree(srckey);
	if(dstkey_free) efree(dstkey);

	REDIS_PROCESS_REQUEST(redis_sock, cmd, cmd_len);
	IF_ATOMIC() {
		redis_string_response(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock, NULL, NULL);
	}
	REDIS_PROCESS_RESPONSE(redis_string_response);

}

/* {{{ proto string Redis::rpoplpush(string srckey, string dstkey)
 */
PHP_METHOD(Redis, rpoplpush)
{
    zval *object;
    RedisSock *redis_sock;
    char *srckey = NULL, *dstkey = NULL;
    int srckey_len, dstkey_len;

    if (zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), "Oss",
                                     &object, redis_ce, &srckey, &srckey_len,
                                     &dstkey, &dstkey_len) == FAILURE) {
        RETURN_FALSE;
    }

    if (redis_sock_get(object, &redis_sock TSRMLS_CC, 0) < 0) {
        RETURN_FALSE;
    }

	common_rpoplpush(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock, srckey, srckey_len, dstkey, dstkey_len, -1);
}
/* }}} */

/* {{{ proto string Redis::brpoplpush(string srckey, string dstkey)
 */
PHP_METHOD(Redis, brpoplpush)
{
    zval *object;
    RedisSock *redis_sock;
    char *srckey = NULL, *dstkey = NULL;
    int srckey_len, dstkey_len;
	long timeout = 0;

    if (zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), "Ossl",
                                     &object, redis_ce, &srckey, &srckey_len,
                                     &dstkey, &dstkey_len, &timeout) == FAILURE) {
        RETURN_FALSE;
    }

    if (redis_sock_get(object, &redis_sock TSRMLS_CC, 0) < 0) {
        RETURN_FALSE;
    }

	common_rpoplpush(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock, srckey, srckey_len, dstkey, dstkey_len, timeout);
}
/* }}} */

/* {{{ proto long Redis::zAdd(string key, int score, string value)
 */
PHP_METHOD(Redis, zAdd) {

    RedisSock *redis_sock;

    char *cmd;
    int cmd_len, key_len, val_len;
    double score;
    char *key, *val;
    int val_free, key_free = 0;
	char *dbl_str;
	int dbl_len;

	zval **z_args;
	int argc = ZEND_NUM_ARGS(), i;

	/* get redis socket */
    if (redis_sock_get(getThis(), &redis_sock TSRMLS_CC, 0) < 0) {
        RETURN_FALSE;
    }

    z_args = emalloc(argc * sizeof(zval*));
    if(zend_get_parameters_array(ht, argc, z_args) == FAILURE) {
        efree(z_args);
		RETURN_FALSE;
    }

	/* need key, score, value, [score, value...] */
	if(argc > 1) {
		convert_to_string(z_args[0]); // required string
	}
	if(argc < 3 || Z_TYPE_P(z_args[0]) != IS_STRING || (argc-1) % 2 != 0) {
		efree(z_args);
		RETURN_FALSE;
	}

	/* possibly serialize key */
	key = Z_STRVAL_P(z_args[0]);
	key_len = Z_STRLEN_P(z_args[0]);
	key_free = redis_key_prefix(redis_sock, &key, &key_len TSRMLS_CC);

	/* start building the command */
    smart_str buf = {0};
	smart_str_appendc(&buf, '*');
	smart_str_append_long(&buf, argc + 1); /* +1 for ZADD command */
	smart_str_appendl(&buf, _NL, sizeof(_NL) - 1);

	/* add command name */
	smart_str_appendc(&buf, '$');
	smart_str_append_long(&buf, 4);
	smart_str_appendl(&buf, _NL, sizeof(_NL) - 1);
	smart_str_appendl(&buf, "ZADD", 4);
	smart_str_appendl(&buf, _NL, sizeof(_NL) - 1);

	/* add key */
	smart_str_appendc(&buf, '$');
	smart_str_append_long(&buf, key_len);
	smart_str_appendl(&buf, _NL, sizeof(_NL) - 1);
	smart_str_appendl(&buf, key, key_len);
	smart_str_appendl(&buf, _NL, sizeof(_NL) - 1);

	for(i = 1; i < argc; i +=2) {
		convert_to_double(z_args[i]); // convert score to double
		val_free = redis_serialize(redis_sock, z_args[i+1], &val, &val_len TSRMLS_CC); // possibly serialize value.

		/* add score */
		score = Z_DVAL_P(z_args[i]);
		REDIS_DOUBLE_TO_STRING(dbl_str, dbl_len, score)
		smart_str_appendc(&buf, '$');
		smart_str_append_long(&buf, dbl_len);
		smart_str_appendl(&buf, _NL, sizeof(_NL) - 1);
		smart_str_appendl(&buf, dbl_str, dbl_len);
		smart_str_appendl(&buf, _NL, sizeof(_NL) - 1);
		efree(dbl_str);

		/* add value */
		smart_str_appendc(&buf, '$');
		smart_str_append_long(&buf, val_len);
		smart_str_appendl(&buf, _NL, sizeof(_NL) - 1);
		smart_str_appendl(&buf, val, val_len);
		smart_str_appendl(&buf, _NL, sizeof(_NL) - 1);

		if(val_free) efree(val);
	}

	/* end string */
	smart_str_0(&buf);
	cmd = buf.c;
	cmd_len = buf.len;
    if(key_free) efree(key);

	/* cleanup */
	efree(z_args);

	REDIS_PROCESS_REQUEST(redis_sock, cmd, cmd_len);
	IF_ATOMIC() {
		redis_long_response(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock, NULL, NULL);
	}
	REDIS_PROCESS_RESPONSE(redis_long_response);
}
/* }}} */
/* {{{ proto array Redis::zRange(string key, int start , int end, bool withscores = FALSE)
 */
PHP_METHOD(Redis, zRange)
{
    zval *object;
    RedisSock *redis_sock;
    char *key = NULL, *cmd;
    int key_len, cmd_len, key_free;
    long start, end;
    long withscores = 0;

    if (zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), "Osll|b",
                                     &object, redis_ce,
                                     &key, &key_len, &start, &end, &withscores) == FAILURE) {
        RETURN_FALSE;
    }

    if (redis_sock_get(object, &redis_sock TSRMLS_CC, 0) < 0) {
        RETURN_FALSE;
    }

	key_free = redis_key_prefix(redis_sock, &key, &key_len TSRMLS_CC);
    if(withscores) {
        cmd_len = redis_cmd_format_static(&cmd, "ZRANGE", "sdds", key, key_len, start, end, "WITHSCORES", 10);
    } else {
        cmd_len = redis_cmd_format_static(&cmd, "ZRANGE", "sdd", key, key_len, start, end);
    }
	if(key_free) efree(key);

	REDIS_PROCESS_REQUEST(redis_sock, cmd, cmd_len);
    if(withscores) {
            IF_ATOMIC() {
                redis_sock_read_multibulk_reply_zipped(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock, NULL, NULL);
            }
            REDIS_PROCESS_RESPONSE(redis_sock_read_multibulk_reply_zipped);
    } else {
            IF_ATOMIC() {
                if (redis_sock_read_multibulk_reply(INTERNAL_FUNCTION_PARAM_PASSTHRU,
                                                    redis_sock, NULL, NULL) < 0) {
                    RETURN_FALSE;
                }
            }
            REDIS_PROCESS_RESPONSE(redis_sock_read_multibulk_reply);
    }
}
/* }}} */
/* {{{ proto long Redis::zDelete(string key, string member)
 */
PHP_METHOD(Redis, zDelete)
{
    RedisSock *redis_sock;

    if(FAILURE == generic_multiple_args_cmd(INTERNAL_FUNCTION_PARAM_PASSTHRU,
                    "ZREM", sizeof("ZREM") - 1,
					2, &redis_sock, 0, 0, 1))
		return;

	IF_ATOMIC() {
	  redis_long_response(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock, NULL, NULL);
	}
	REDIS_PROCESS_RESPONSE(redis_long_response);
}
/* }}} */
/* {{{ proto long Redis::zDeleteRangeByScore(string key, string start, string end)
 */
PHP_METHOD(Redis, zDeleteRangeByScore)
{
    zval *object;
    RedisSock *redis_sock;
    char *key = NULL, *cmd;
    int key_len, cmd_len, key_free;
    char *start, *end;
    int start_len, end_len;

    if (zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), "Osss",
                                     &object, redis_ce,
                                     &key, &key_len, &start, &start_len, &end, &end_len) == FAILURE) {
        RETURN_FALSE;
    }

    if (redis_sock_get(object, &redis_sock TSRMLS_CC, 0) < 0) {
        RETURN_FALSE;
    }

	key_free = redis_key_prefix(redis_sock, &key, &key_len TSRMLS_CC);
    cmd_len = redis_cmd_format_static(&cmd, "ZREMRANGEBYSCORE", "sss", key, key_len, start, start_len, end, end_len);
	if(key_free) efree(key);

	REDIS_PROCESS_REQUEST(redis_sock, cmd, cmd_len);
	IF_ATOMIC() {
		redis_long_response(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock, NULL, NULL);
	}
	REDIS_PROCESS_RESPONSE(redis_long_response);

}
/* }}} */

/* {{{ proto long Redis::zDeleteRangeByRank(string key, long start, long end)
 */
PHP_METHOD(Redis, zDeleteRangeByRank)
{
    zval *object;
    RedisSock *redis_sock;
    char *key = NULL, *cmd;
    int key_len, cmd_len, key_free;
    long start, end;

    if (zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), "Osll",
                                     &object, redis_ce,
                                     &key, &key_len, &start, &end) == FAILURE) {
        RETURN_FALSE;
    }

    if (redis_sock_get(object, &redis_sock TSRMLS_CC, 0) < 0) {
        RETURN_FALSE;
    }

	key_free = redis_key_prefix(redis_sock, &key, &key_len TSRMLS_CC);
    cmd_len = redis_cmd_format_static(&cmd, "ZREMRANGEBYRANK", "sdd", key, key_len, (int)start, (int)end);
	if(key_free) efree(key);

	REDIS_PROCESS_REQUEST(redis_sock, cmd, cmd_len);
	IF_ATOMIC() {
		redis_long_response(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock, NULL, NULL);
	}
	REDIS_PROCESS_RESPONSE(redis_long_response);

}
/* }}} */

/* {{{ proto array Redis::zReverseRange(string key, int start , int end, bool withscores = FALSE)
 */
PHP_METHOD(Redis, zReverseRange)
{
    zval *object;
    RedisSock *redis_sock;
    char *key = NULL, *cmd;
    int key_len, cmd_len, key_free;
    long start, end;
    long withscores = 0;

    if (zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), "Osll|b",
                                     &object, redis_ce,
                                     &key, &key_len, &start, &end, &withscores) == FAILURE) {
        RETURN_FALSE;
    }

    if (redis_sock_get(object, &redis_sock TSRMLS_CC, 0) < 0) {
        RETURN_FALSE;
    }

	key_free = redis_key_prefix(redis_sock, &key, &key_len TSRMLS_CC);
    if(withscores) {
        cmd_len = redis_cmd_format_static(&cmd, "ZREVRANGE", "sdds", key, key_len, start, end, "WITHSCORES", 10);
    } else {
        cmd_len = redis_cmd_format_static(&cmd, "ZREVRANGE", "sdd", key, key_len, start, end);
    }
	if(key_free) efree(key);

    REDIS_PROCESS_REQUEST(redis_sock, cmd, cmd_len);
    if(withscores) {
    	IF_ATOMIC() {
    		redis_sock_read_multibulk_reply_zipped(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock, NULL, NULL);
    	}
    	REDIS_PROCESS_RESPONSE(redis_sock_read_multibulk_reply_zipped);
    } else {
    	IF_ATOMIC() {
            if (redis_sock_read_multibulk_reply(INTERNAL_FUNCTION_PARAM_PASSTHRU,
                                                redis_sock, NULL, NULL) < 0) {
                RETURN_FALSE;
            }
    	}
    	REDIS_PROCESS_RESPONSE(redis_sock_read_multibulk_reply);
    }
}
/* }}} */

PHPAPI void
redis_generic_zrange_by_score(INTERNAL_FUNCTION_PARAMETERS, char *keyword) {

    zval *object, *z_options = NULL, **z_limit_val_pp = NULL, **z_withscores_val_pp = NULL;

    RedisSock *redis_sock;
    char *key = NULL, *cmd;
    int key_len, cmd_len, key_free;
    zend_bool withscores = 0;
    char *start, *end;
    int start_len, end_len;
    int has_limit = 0;
    long limit_low, limit_high;

    if (zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), "Osss|a",
                                     &object, redis_ce,
                                     &key, &key_len,
                                     &start, &start_len,
                                     &end, &end_len,
                                     &z_options) == FAILURE) {
        RETURN_FALSE;
    }

    if (redis_sock_get(object, &redis_sock TSRMLS_CC, 0) < 0) {
        RETURN_FALSE;
    }

    /* options */
    if (z_options && Z_TYPE_P(z_options) == IS_ARRAY) {
        /* add scores */
        zend_hash_find(Z_ARRVAL_P(z_options), "withscores", sizeof("withscores"), (void**)&z_withscores_val_pp);
        withscores = (z_withscores_val_pp ? Z_BVAL_PP(z_withscores_val_pp) : 0);

        /* limit offset, count:
           z_limit_val_pp points to an array($longFrom, $longCount)
        */
        if(zend_hash_find(Z_ARRVAL_P(z_options), "limit", sizeof("limit"), (void**)&z_limit_val_pp)== SUCCESS) {;
            if(zend_hash_num_elements(Z_ARRVAL_PP(z_limit_val_pp)) == 2) {
                zval **z_offset_pp, **z_count_pp;
                // get the two values from the table, check that they are indeed of LONG type
                if(SUCCESS == zend_hash_index_find(Z_ARRVAL_PP(z_limit_val_pp), 0, (void**)&z_offset_pp) &&
                  SUCCESS == zend_hash_index_find(Z_ARRVAL_PP(z_limit_val_pp), 1, (void**)&z_count_pp) &&
                  Z_TYPE_PP(z_offset_pp) == IS_LONG &&
                  Z_TYPE_PP(z_count_pp) == IS_LONG) {

                    has_limit = 1;
                    limit_low = Z_LVAL_PP(z_offset_pp);
                    limit_high = Z_LVAL_PP(z_count_pp);
                }
            }
        }
    }

	key_free = redis_key_prefix(redis_sock, &key, &key_len TSRMLS_CC);
    if(withscores) {
        if(has_limit) {
            cmd_len = redis_cmd_format_static(&cmd, keyword, "ssssdds",
                            key, key_len, start, start_len, end, end_len, "LIMIT", 5, limit_low, limit_high, "WITHSCORES", 10);
        } else {
            cmd_len = redis_cmd_format_static(&cmd, keyword, "ssss",
                            key, key_len, start, start_len, end, end_len, "WITHSCORES", 10);
        }
    } else {
        if(has_limit) {
            cmd_len = redis_cmd_format_static(&cmd, keyword, "ssssdd",
                            key, key_len, start, start_len, end, end_len, "LIMIT", 5, limit_low, limit_high);
        } else {
            cmd_len = redis_cmd_format_static(&cmd, keyword, "sss", key, key_len, start, start_len, end, end_len);
        }
    }
	if(key_free) efree(key);

    REDIS_PROCESS_REQUEST(redis_sock, cmd, cmd_len);
    if(withscores) {
            /* with scores! we have to transform the return array.
             * return_value currently holds this: [elt0, val0, elt1, val1 ... ]
             * we want [elt0 => val0, elt1 => val1], etc.
             */
            IF_ATOMIC() {
                if(redis_sock_read_multibulk_reply_zipped(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock, NULL, NULL) < 0) {
                    RETURN_FALSE;
                }
            }
            REDIS_PROCESS_RESPONSE(redis_sock_read_multibulk_reply_zipped);
    } else {
            IF_ATOMIC() {
                if(redis_sock_read_multibulk_reply(INTERNAL_FUNCTION_PARAM_PASSTHRU,
                                                    redis_sock, NULL, NULL) < 0) {
                    RETURN_FALSE;
                }
            }
            REDIS_PROCESS_RESPONSE(redis_sock_read_multibulk_reply);
    }
}

/* {{{ proto array Redis::zRangeByScore(string key, string start , string end [,array options = NULL])
 */
PHP_METHOD(Redis, zRangeByScore)
{
	redis_generic_zrange_by_score(INTERNAL_FUNCTION_PARAM_PASSTHRU, "ZRANGEBYSCORE");
}
/* }}} */
/* {{{ proto array Redis::zRevRangeByScore(string key, string start , string end [,array options = NULL])
 */
PHP_METHOD(Redis, zRevRangeByScore)
{
	redis_generic_zrange_by_score(INTERNAL_FUNCTION_PARAM_PASSTHRU, "ZREVRANGEBYSCORE");
}

/* {{{ proto array Redis::zCount(string key, string start , string end)
 */
PHP_METHOD(Redis, zCount)
{
    zval *object;

    RedisSock *redis_sock;
    char *key = NULL, *cmd;
    int key_len, cmd_len, key_free;
    char *start, *end;
    int start_len, end_len;

    if (zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), "Osss",
                                     &object, redis_ce,
                                     &key, &key_len,
                                     &start, &start_len,
                                     &end, &end_len) == FAILURE) {
        RETURN_FALSE;
    }

    if (redis_sock_get(object, &redis_sock TSRMLS_CC, 0) < 0) {
        RETURN_FALSE;
    }

	key_free = redis_key_prefix(redis_sock, &key, &key_len TSRMLS_CC);
    cmd_len = redis_cmd_format_static(&cmd, "ZCOUNT", "sss", key, key_len, start, start_len, end, end_len);
	if(key_free) efree(key);

	REDIS_PROCESS_REQUEST(redis_sock, cmd, cmd_len);
	IF_ATOMIC() {
	  redis_long_response(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock, NULL, NULL);
	}
	REDIS_PROCESS_RESPONSE(redis_long_response);
}
/* }}} */

/* {{{ proto long Redis::zCard(string key)
 */
PHP_METHOD(Redis, zCard)
{
    zval *object;
    RedisSock *redis_sock;
    char *key = NULL, *cmd;
    int key_len, cmd_len, key_free;

    if (zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), "Os",
                                     &object, redis_ce,
                                     &key, &key_len) == FAILURE) {
        RETURN_FALSE;
    }

    if (redis_sock_get(object, &redis_sock TSRMLS_CC, 0) < 0) {
        RETURN_FALSE;
    }

	key_free = redis_key_prefix(redis_sock, &key, &key_len TSRMLS_CC);
    cmd_len = redis_cmd_format_static(&cmd, "ZCARD", "s", key, key_len);
	if(key_free) efree(key);

	REDIS_PROCESS_REQUEST(redis_sock, cmd, cmd_len);
	IF_ATOMIC() {
	  redis_long_response(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock, NULL, NULL);
	}
	REDIS_PROCESS_RESPONSE(redis_long_response);

}
/* }}} */

/* {{{ proto double Redis::zScore(string key, mixed member)
 */
PHP_METHOD(Redis, zScore)
{
    zval *object;
    RedisSock *redis_sock;
    char *key = NULL, *val = NULL, *cmd;
    int key_len, val_len, cmd_len;
    int val_free, key_free = 0;
    zval *z_value;

    if (zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), "Osz",
                                     &object, redis_ce, &key, &key_len,
                                     &z_value) == FAILURE) {
        RETURN_FALSE;
    }

    if (redis_sock_get(object, &redis_sock TSRMLS_CC, 0) < 0) {
        RETURN_FALSE;
    }

    val_free = redis_serialize(redis_sock, z_value, &val, &val_len TSRMLS_CC);
	key_free = redis_key_prefix(redis_sock, &key, &key_len TSRMLS_CC);
    cmd_len = redis_cmd_format_static(&cmd, "ZSCORE", "ss", key, key_len, val, val_len);
    if(val_free) efree(val);
    if(key_free) efree(key);

	REDIS_PROCESS_REQUEST(redis_sock, cmd, cmd_len);
	IF_ATOMIC() {
	    redis_bulk_double_response(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock, NULL, NULL);
	}
	REDIS_PROCESS_RESPONSE(redis_bulk_double_response);
}
/* }}} */


PHPAPI void generic_rank_method(INTERNAL_FUNCTION_PARAMETERS, char *keyword, int keyword_len) {
    zval *object;
    RedisSock *redis_sock;
    char *key = NULL, *val = NULL, *cmd;
    int key_len, val_len, cmd_len;
    int val_free, key_free = 0;
    zval *z_value;

    if (zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), "Osz",
                                     &object, redis_ce, &key, &key_len,
                                     &z_value) == FAILURE) {
        RETURN_FALSE;
    }

    if (redis_sock_get(object, &redis_sock TSRMLS_CC, 0) < 0) {
        RETURN_FALSE;
    }

    val_free = redis_serialize(redis_sock, z_value, &val, &val_len TSRMLS_CC);
	key_free = redis_key_prefix(redis_sock, &key, &key_len TSRMLS_CC);
    cmd_len = redis_cmd_format_static(&cmd, keyword, "ss", key, key_len, val, val_len);
    if(val_free) efree(val);
    if(key_free) efree(key);

	REDIS_PROCESS_REQUEST(redis_sock, cmd, cmd_len);
	IF_ATOMIC() {
	    redis_long_response(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock, NULL, NULL);
	}
	REDIS_PROCESS_RESPONSE(redis_long_response);
}


/* {{{ proto long Redis::zRank(string key, string member)
 */
PHP_METHOD(Redis, zRank) {

        generic_rank_method(INTERNAL_FUNCTION_PARAM_PASSTHRU, "ZRANK", 5);
}
/* }}} */

/* {{{ proto long Redis::zRevRank(string key, string member)
 */
PHP_METHOD(Redis, zRevRank) {

        generic_rank_method(INTERNAL_FUNCTION_PARAM_PASSTHRU, "ZREVRANK", 8);
}
/* }}} */

PHPAPI void generic_incrby_method(INTERNAL_FUNCTION_PARAMETERS, char *keyword, int keyword_len) {
    zval *object;
    RedisSock *redis_sock;

    char *key = NULL, *cmd, *val;
    int key_len, val_len, cmd_len;
    double add;
    int val_free, key_free;
    zval *z_value;

    if (zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), "Osdz",
                                     &object, redis_ce,
                                     &key, &key_len, &add, &z_value) == FAILURE) {
        RETURN_FALSE;
    }

    if (redis_sock_get(object, &redis_sock TSRMLS_CC, 0) < 0) {
        RETURN_FALSE;
    }

    val_free = redis_serialize(redis_sock, z_value, &val, &val_len TSRMLS_CC);
	key_free = redis_key_prefix(redis_sock, &key, &key_len TSRMLS_CC);
    cmd_len = redis_cmd_format_static(&cmd, keyword, "sfs", key, key_len, add, val, val_len);
    if(val_free) efree(val);
    if(key_free) efree(key);

	REDIS_PROCESS_REQUEST(redis_sock, cmd, cmd_len);
	IF_ATOMIC() {
	    redis_bulk_double_response(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock, NULL, NULL);
	}
	REDIS_PROCESS_RESPONSE(redis_bulk_double_response);

}

/* {{{ proto double Redis::zIncrBy(string key, double value, mixed member)
 */
PHP_METHOD(Redis, zIncrBy)
{
    generic_incrby_method(INTERNAL_FUNCTION_PARAM_PASSTHRU, "ZINCRBY", sizeof("ZINCRBY")-1);
}
/* }}} */

PHPAPI void generic_z_command(INTERNAL_FUNCTION_PARAMETERS, char *command, int command_len) {
    zval *object, *z_keys, *z_weights = NULL, **z_data;
    HashTable *ht_keys, *ht_weights = NULL;
    RedisSock *redis_sock;
    smart_str cmd = {0};
    HashPosition ptr;
    char *store_key, *agg_op = NULL;
    int cmd_arg_count = 2, store_key_len, agg_op_len = 0, keys_count;

    // Grab our parameters
    if(zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), "Osa|a!s",
                                    &object, redis_ce, &store_key, &store_key_len,
                                    &z_keys, &z_weights, &agg_op, &agg_op_len) == FAILURE)
    {
        RETURN_FALSE;
    }

    // We'll need our socket
    if(redis_sock_get(object, &redis_sock TSRMLS_CC, 0) < 0) {
        RETURN_FALSE;
    }

    // Grab our keys argument as an array
    ht_keys = Z_ARRVAL_P(z_keys);

    // Nothing to do if there aren't any keys
    if((keys_count = zend_hash_num_elements(ht_keys)) == 0) {
        RETURN_FALSE;
    } else {
        // Increment our overall argument count
        cmd_arg_count += keys_count;
    }

    // Grab and validate our weights array
    if(z_weights != NULL) {
        ht_weights = Z_ARRVAL_P(z_weights);

        // This command is invalid if the weights array isn't the same size
        // as our keys array.
        if(zend_hash_num_elements(ht_weights) != keys_count) {
            RETURN_FALSE;
        }

        // Increment our overall argument count by the number of keys
        // plus one, for the "WEIGHTS" argument itself
        cmd_arg_count += keys_count + 1;
    }

    // AGGREGATE option
    if(agg_op_len != 0) {
        // Verify our aggregation option
        if(strncasecmp(agg_op, "SUM", sizeof("SUM")) &&
           strncasecmp(agg_op, "MIN", sizeof("MIN")) &&
           strncasecmp(agg_op, "MAX", sizeof("MAX")))
        {
            RETURN_FALSE;
        }

        // Two more arguments: "AGGREGATE" and agg_op
        cmd_arg_count += 2;
    }

    // Command header
    redis_cmd_init_sstr(&cmd, cmd_arg_count, command, command_len);

    // Prefix our key if necessary and add the output key
    int key_free = redis_key_prefix(redis_sock, &store_key, &store_key_len TSRMLS_CC);
    redis_cmd_append_sstr(&cmd, store_key, store_key_len);
    if(key_free) efree(store_key);

    // Number of input keys argument
    redis_cmd_append_sstr_int(&cmd, keys_count);

    // Process input keys
    for(zend_hash_internal_pointer_reset_ex(ht_keys, &ptr);
        zend_hash_get_current_data_ex(ht_keys, (void**)&z_data, &ptr)==SUCCESS;
        zend_hash_move_forward_ex(ht_keys, &ptr))
    {
        char *key;
        int key_free, key_len;
        zval *z_tmp = NULL;

        if(Z_TYPE_PP(z_data) == IS_STRING) {
            key = Z_STRVAL_PP(z_data);
            key_len = Z_STRLEN_PP(z_data);
        } else {
            MAKE_STD_ZVAL(z_tmp);
            *z_tmp = **z_data;
            convert_to_string(z_tmp);

            key = Z_STRVAL_P(z_tmp);
            key_len = Z_STRLEN_P(z_tmp);
        }

        // Apply key prefix if necessary
        key_free = redis_key_prefix(redis_sock, &key, &key_len TSRMLS_CC);

        // Append this input set
        redis_cmd_append_sstr(&cmd, key, key_len);

        // Free our key if it was prefixed
        if(key_free) efree(key);

        // Free our temporary z_val if it was converted
        if(z_tmp) {
            zval_dtor(z_tmp);
            efree(z_tmp);
            z_tmp = NULL;
        }
    }

    // Weights
    if(ht_weights != NULL) {
        // Append "WEIGHTS" argument
        redis_cmd_append_sstr(&cmd, "WEIGHTS", sizeof("WEIGHTS") - 1);

        // Process weights
        for(zend_hash_internal_pointer_reset_ex(ht_weights, &ptr);
            zend_hash_get_current_data_ex(ht_weights, (void**)&z_data, &ptr)==SUCCESS;
            zend_hash_move_forward_ex(ht_weights, &ptr))
        {
            // Ignore non numeric arguments, unless they're special Redis numbers
            if (Z_TYPE_PP(z_data) != IS_LONG && Z_TYPE_PP(z_data) != IS_DOUBLE &&
                 strncasecmp(Z_STRVAL_PP(z_data), "inf", sizeof("inf")) != 0 &&
                 strncasecmp(Z_STRVAL_PP(z_data), "-inf", sizeof("-inf")) != 0 &&
                 strncasecmp(Z_STRVAL_PP(z_data), "+inf", sizeof("+inf")) != 0)
            {
                // We should abort if we have an invalid weight, rather than pass
                // a different number of weights than the user is expecting
                efree(cmd.c);
                RETURN_FALSE;
            }

            // Append the weight based on the input type
            switch(Z_TYPE_PP(z_data)) {
                case IS_LONG:
                    redis_cmd_append_sstr_long(&cmd, Z_LVAL_PP(z_data));
                    break;
                case IS_DOUBLE:
                    redis_cmd_append_sstr_dbl(&cmd, Z_DVAL_PP(z_data));
                    break;
                case IS_STRING:
                    redis_cmd_append_sstr(&cmd, Z_STRVAL_PP(z_data), Z_STRLEN_PP(z_data));
                    break;
            }
        }
    }

    // Aggregation options, if we have them
    if(agg_op_len != 0) {
        redis_cmd_append_sstr(&cmd, "AGGREGATE", sizeof("AGGREGATE") - 1);
        redis_cmd_append_sstr(&cmd, agg_op, agg_op_len);
    }

    // Kick off our request
    REDIS_PROCESS_REQUEST(redis_sock, cmd.c, cmd.len);
    IF_ATOMIC() {
        redis_long_response(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock, NULL, NULL);
    }
    REDIS_PROCESS_RESPONSE(redis_long_response);
}

/* zInter */
PHP_METHOD(Redis, zInter) {
	generic_z_command(INTERNAL_FUNCTION_PARAM_PASSTHRU, "ZINTERSTORE", 11);
}

/* zUnion */
PHP_METHOD(Redis, zUnion) {
	generic_z_command(INTERNAL_FUNCTION_PARAM_PASSTHRU, "ZUNIONSTORE", 11);
}

/* hashes */

PHPAPI void
generic_hset(INTERNAL_FUNCTION_PARAMETERS, char *kw, void (*fun)(INTERNAL_FUNCTION_PARAMETERS, RedisSock *, zval *, void *)) {
    zval *object;
    RedisSock *redis_sock;
    char *key = NULL, *cmd, *member, *val;
    int key_len, member_len, cmd_len, val_len;
    int val_free, key_free = 0;
    zval *z_value;

    if (zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), "Ossz",
                                     &object, redis_ce,
                                     &key, &key_len, &member, &member_len, &z_value) == FAILURE) {
        RETURN_FALSE;
    }

    if (redis_sock_get(object, &redis_sock TSRMLS_CC, 0) < 0) {
        RETURN_FALSE;
    }

    val_free = redis_serialize(redis_sock, z_value, &val, &val_len TSRMLS_CC);
	key_free = redis_key_prefix(redis_sock, &key, &key_len TSRMLS_CC);
    cmd_len = redis_cmd_format_static(&cmd, kw, "sss", key, key_len, member, member_len, val, val_len);
    if(val_free) efree(val);
    if(key_free) efree(key);

	REDIS_PROCESS_REQUEST(redis_sock, cmd, cmd_len);
	IF_ATOMIC() {
	  fun(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock, NULL, NULL);
	}
	REDIS_PROCESS_RESPONSE(fun);
}
/* hSet */
PHP_METHOD(Redis, hSet)
{
	generic_hset(INTERNAL_FUNCTION_PARAM_PASSTHRU, "HSET", redis_long_response);
}
/* }}} */
/* hSetNx */
PHP_METHOD(Redis, hSetNx)
{
	generic_hset(INTERNAL_FUNCTION_PARAM_PASSTHRU, "HSETNX", redis_1_response);
}
/* }}} */


/* hGet */
PHP_METHOD(Redis, hGet)
{
    zval *object;
    RedisSock *redis_sock;
    char *key = NULL, *cmd, *member;
    int key_len, member_len, cmd_len, key_free;

    if (zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), "Oss",
                                     &object, redis_ce,
                                     &key, &key_len, &member, &member_len) == FAILURE) {
        RETURN_FALSE;
    }

    if (redis_sock_get(object, &redis_sock TSRMLS_CC, 0) < 0) {
        RETURN_FALSE;
    }
	key_free = redis_key_prefix(redis_sock, &key, &key_len TSRMLS_CC);
    cmd_len = redis_cmd_format_static(&cmd, "HGET", "ss", key, key_len, member, member_len);
	if(key_free) efree(key);

	REDIS_PROCESS_REQUEST(redis_sock, cmd, cmd_len);
	IF_ATOMIC() {
		redis_string_response(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock, NULL, NULL);
	}
	REDIS_PROCESS_RESPONSE(redis_string_response);

}
/* }}} */

/* hLen */
PHP_METHOD(Redis, hLen)
{
    zval *object;
    RedisSock *redis_sock;
    char *key = NULL, *cmd;
    int key_len, cmd_len, key_free;

    if (zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), "Os",
                                     &object, redis_ce,
                                     &key, &key_len) == FAILURE) {
        RETURN_FALSE;
    }

    if (redis_sock_get(object, &redis_sock TSRMLS_CC, 0) < 0) {
        RETURN_FALSE;
    }

	key_free = redis_key_prefix(redis_sock, &key, &key_len TSRMLS_CC);
    cmd_len = redis_cmd_format_static(&cmd, "HLEN", "s", key, key_len);
	if(key_free) efree(key);

	REDIS_PROCESS_REQUEST(redis_sock, cmd, cmd_len);
	IF_ATOMIC() {
	  redis_long_response(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock, NULL, NULL);
	}
	REDIS_PROCESS_RESPONSE(redis_long_response);

}
/* }}} */

PHPAPI RedisSock*
generic_hash_command_2(INTERNAL_FUNCTION_PARAMETERS, char *keyword, int keyword_len, char **out_cmd, int *out_len) {

    zval *object;
    RedisSock *redis_sock;
    char *key = NULL, *cmd, *member;
    int key_len, cmd_len, member_len, key_free;

    if (zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), "Oss",
                                     &object, redis_ce,
                                     &key, &key_len, &member, &member_len) == FAILURE) {
            ZVAL_BOOL(return_value, 0);
            return NULL;
    }

    if (redis_sock_get(object, &redis_sock TSRMLS_CC, 0) < 0) {
            ZVAL_BOOL(return_value, 0);
            return NULL;
    }
	key_free = redis_key_prefix(redis_sock, &key, &key_len TSRMLS_CC);
    cmd_len = redis_cmd_format_static(&cmd, keyword, "ss", key, key_len, member, member_len);
	if(key_free) efree(key);

    *out_cmd = cmd;
    *out_len = cmd_len;
    return redis_sock;
}

/* hDel */
PHP_METHOD(Redis, hDel)
{
    RedisSock *redis_sock;

    if(FAILURE == generic_multiple_args_cmd(INTERNAL_FUNCTION_PARAM_PASSTHRU,
                    "HDEL", sizeof("HDEL") - 1,
					2, &redis_sock, 0, 0, 0))
		return;

	IF_ATOMIC() {
	  redis_long_response(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock, NULL, NULL);
	}
	REDIS_PROCESS_RESPONSE(redis_long_response);
}

/* hExists */
PHP_METHOD(Redis, hExists)
{
    char *cmd;
    int cmd_len;
    RedisSock *redis_sock = generic_hash_command_2(INTERNAL_FUNCTION_PARAM_PASSTHRU, "HEXISTS", 7, &cmd, &cmd_len);
	if(!redis_sock)
		RETURN_FALSE;

	REDIS_PROCESS_REQUEST(redis_sock, cmd, cmd_len);
	IF_ATOMIC() {
	  redis_1_response(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock, NULL, NULL);
	}
	REDIS_PROCESS_RESPONSE(redis_1_response);

}

PHPAPI RedisSock*
generic_hash_command_1(INTERNAL_FUNCTION_PARAMETERS, char *keyword, int keyword_len) {

    zval *object;
    RedisSock *redis_sock;
    char *key = NULL, *cmd;
    int key_len, cmd_len, key_free;

    if (zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), "Os",
                                     &object, redis_ce,
                                     &key, &key_len) == FAILURE) {
            ZVAL_BOOL(return_value, 0);
            return NULL;
    }

    if (redis_sock_get(object, &redis_sock TSRMLS_CC, 0) < 0) {
            ZVAL_BOOL(return_value, 0);
            return NULL;
    }
	key_free = redis_key_prefix(redis_sock, &key, &key_len TSRMLS_CC);
    cmd_len = redis_cmd_format_static(&cmd, keyword, "s", key, key_len);
	if(key_free) efree(key);

	/* call REDIS_PROCESS_REQUEST(redis_sock, cmd, cmd_len) without breaking the return value */
	IF_MULTI_OR_ATOMIC() {
		if(redis_sock_write(redis_sock, cmd, cmd_len TSRMLS_CC) < 0) {
			efree(cmd);
			return NULL;
		}
		efree(cmd);
	}
	IF_PIPELINE() {
		PIPELINE_ENQUEUE_COMMAND(cmd, cmd_len);
		efree(cmd);
	}
    return redis_sock;
}

/* hKeys */
PHP_METHOD(Redis, hKeys)
{
    RedisSock *redis_sock = generic_hash_command_1(INTERNAL_FUNCTION_PARAM_PASSTHRU, "HKEYS", sizeof("HKEYS")-1);
	if(!redis_sock)
		RETURN_FALSE;

	IF_ATOMIC() {
	    if (redis_sock_read_multibulk_reply_raw(INTERNAL_FUNCTION_PARAM_PASSTHRU,
											redis_sock, NULL, NULL) < 0) {
    	    RETURN_FALSE;
	    }
	}
	REDIS_PROCESS_RESPONSE(redis_sock_read_multibulk_reply_raw);


}
/* hVals */
PHP_METHOD(Redis, hVals)
{
    RedisSock *redis_sock = generic_hash_command_1(INTERNAL_FUNCTION_PARAM_PASSTHRU, "HVALS", sizeof("HVALS")-1);
	if(!redis_sock)
		RETURN_FALSE;

	IF_ATOMIC() {
	    if (redis_sock_read_multibulk_reply(INTERNAL_FUNCTION_PARAM_PASSTHRU,
    	                                    redis_sock, NULL, NULL) < 0) {
        	RETURN_FALSE;
	    }
	}
	REDIS_PROCESS_RESPONSE(redis_sock_read_multibulk_reply);

}


PHP_METHOD(Redis, hGetAll) {

    RedisSock *redis_sock = generic_hash_command_1(INTERNAL_FUNCTION_PARAM_PASSTHRU, "HGETALL", sizeof("HGETALL")-1);
	if(!redis_sock)
		RETURN_FALSE;

	IF_ATOMIC() {
	    if (redis_sock_read_multibulk_reply_zipped_strings(INTERNAL_FUNCTION_PARAM_PASSTHRU,
    	                                    redis_sock, NULL, NULL) < 0) {
        	RETURN_FALSE;
	    }
	}
	REDIS_PROCESS_RESPONSE(redis_sock_read_multibulk_reply_zipped_strings);
}

PHPAPI void array_zip_values_and_scores(RedisSock *redis_sock, zval *z_tab, int use_atof TSRMLS_DC) {

    zval *z_ret;
	HashTable *keytable;
    
	MAKE_STD_ZVAL(z_ret);
    array_init(z_ret);
    keytable = Z_ARRVAL_P(z_tab);

    for(zend_hash_internal_pointer_reset(keytable);
        zend_hash_has_more_elements(keytable) == SUCCESS;
        zend_hash_move_forward(keytable)) {

        char *tablekey, *hkey, *hval;
        unsigned int tablekey_len;
        int hkey_len;
        unsigned long idx;
        zval **z_key_pp, **z_value_pp;

        zend_hash_get_current_key_ex(keytable, &tablekey, &tablekey_len, &idx, 0, NULL);
        if(zend_hash_get_current_data(keytable, (void**)&z_key_pp) == FAILURE) {
            continue; 	/* this should never happen, according to the PHP people. */
        }

        /* get current value, a key */
        convert_to_string(*z_key_pp);
        hkey = Z_STRVAL_PP(z_key_pp);
        hkey_len = Z_STRLEN_PP(z_key_pp);

        /* move forward */
        zend_hash_move_forward(keytable);

        /* fetch again */
        zend_hash_get_current_key_ex(keytable, &tablekey, &tablekey_len, &idx, 0, NULL);
        if(zend_hash_get_current_data(keytable, (void**)&z_value_pp) == FAILURE) {
            continue; 	/* this should never happen, according to the PHP people. */
        }

        /* get current value, a hash value now. */
        hval = Z_STRVAL_PP(z_value_pp);

        if(use_atof) { /* zipping a score */
            add_assoc_double_ex(z_ret, hkey, 1+hkey_len, atof(hval));
        } else { /* add raw copy */
            zval *z = NULL;
            MAKE_STD_ZVAL(z);
            *z = **z_value_pp;
            zval_copy_ctor(z);
            add_assoc_zval_ex(z_ret, hkey, 1+hkey_len, z);
        }
    }
    /* replace */
    zval_dtor(z_tab);
    *z_tab = *z_ret;
    zval_copy_ctor(z_tab);
    zval_dtor(z_ret);

    efree(z_ret);
}

PHP_METHOD(Redis, hIncrByFloat)
{
	zval *object;
	RedisSock *redis_sock;
	char *key = NULL, *cmd, *member;
	int key_len, member_len, cmd_len, key_free;
	double val;

	// Validate we have the right number of arguments
	if(zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), "Ossd",
									&object, redis_ce,
									&key, &key_len, &member, &member_len, &val) == FAILURE) {
		RETURN_FALSE;
	}

	// Grab our socket
	if(redis_sock_get(object, &redis_sock TSRMLS_CC, 0) < 0) {
		RETURN_FALSE;
	}

	key_free = redis_key_prefix(redis_sock, &key, &key_len TSRMLS_CC);
	cmd_len = redis_cmd_format_static(&cmd, "HINCRBYFLOAT", "ssf", key, key_len, member, member_len, val);
	if(key_free) efree(key);

	REDIS_PROCESS_REQUEST(redis_sock, cmd, cmd_len);
	IF_ATOMIC() {
	    redis_bulk_double_response(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock, NULL, NULL);
	}
	REDIS_PROCESS_RESPONSE(redis_bulk_double_response);
}

PHP_METHOD(Redis, hIncrBy)
{
    zval *object;
    RedisSock *redis_sock;
    char *key = NULL, *cmd, *member, *val;
    int key_len, member_len, cmd_len, val_len, key_free;
	int i;

    if (zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), "Osss",
                                     &object, redis_ce,
                                     &key, &key_len, &member, &member_len, &val, &val_len) == FAILURE) {
        RETURN_FALSE;
    }

    if (redis_sock_get(object, &redis_sock TSRMLS_CC, 0) < 0) {
        RETURN_FALSE;
    }

	/* check for validity of numeric string */
	i = 0;
	if(val_len && val[0] == '-') { /* negative case */
		i++;
	}
	for(; i < val_len; ++i) {
		if(val[i] < '0' || val[i] > '9') {
			RETURN_FALSE;
		}
	}

    /* HINCRBY key member amount */
	key_free = redis_key_prefix(redis_sock, &key, &key_len TSRMLS_CC);
    cmd_len = redis_cmd_format_static(&cmd, "HINCRBY", "sss", key, key_len, member, member_len, val, val_len);
	if(key_free) efree(key);

	REDIS_PROCESS_REQUEST(redis_sock, cmd, cmd_len);
	IF_ATOMIC() {
	  redis_long_response(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock, NULL, NULL);
	}
	REDIS_PROCESS_RESPONSE(redis_long_response);

}

/* {{{ array Redis::hMget(string hash, array keys) */
PHP_METHOD(Redis, hMget) {
    zval *object;
    RedisSock *redis_sock;
    char *key = NULL;
    zval *z_array, **z_keys, **data;
    int field_count, i, valid, key_len, key_free;
    HashTable *ht_array;
    HashPosition ptr;
    smart_str cmd = {0};

    // Make sure we can grab our arguments properly
    if(zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), "Osa",
                                    &object, redis_ce, &key, &key_len, &z_array)
                                    == FAILURE)
    {
        RETURN_FALSE;
    }

    // We'll need our socket
    if(redis_sock_get(object, &redis_sock TSRMLS_CC, 0) < 0) {
        RETURN_FALSE;
    }

    // Grab member count and abort if we don't have any
    if((field_count = zend_hash_num_elements(Z_ARRVAL_P(z_array))) == 0) {
        RETURN_FALSE;
    }

    // Prefix our key if we need to
    key_free = redis_key_prefix(redis_sock, &key, &key_len TSRMLS_CC);

    // Allocate enough memory for the number of keys being requested
    z_keys = ecalloc(field_count, sizeof(zval *));

    // Grab our HashTable
    ht_array = Z_ARRVAL_P(z_array);

    // Iterate through our keys, grabbing members that are valid
    for(valid=0, zend_hash_internal_pointer_reset_ex(ht_array, &ptr);
        zend_hash_get_current_data_ex(ht_array, (void**)&data, &ptr)==SUCCESS;
        zend_hash_move_forward_ex(ht_array, &ptr))
    {
        // Make sure the data is a long or string, and if it's a string that
        // it isn't empty.  There is no reason to send empty length members.
        if((Z_TYPE_PP(data) == IS_STRING && Z_STRLEN_PP(data)>0) ||
            Z_TYPE_PP(data) == IS_LONG) 
        {
            // This is a key we can ask for, copy it and set it in our array
            MAKE_STD_ZVAL(z_keys[valid]);
            *z_keys[valid] = **data;
            zval_copy_ctor(z_keys[valid]);
            convert_to_string(z_keys[valid]);

            // Increment the number of valid keys we've encountered
            valid++;
        }
    }

    // If we don't have any valid keys, we can abort here
    if(valid == 0) {
        if(key_free) efree(key);
        efree(z_keys);
        RETURN_FALSE;
    }

    // Build command header.  One extra argument for the hash key itself
    redis_cmd_init_sstr(&cmd, valid+1, "HMGET", sizeof("HMGET")-1);

    // Add the hash key
    redis_cmd_append_sstr(&cmd, key, key_len);

    // Free key memory if it was prefixed
    if(key_free) efree(key);

    // Iterate our keys, appending them as arguments
    for(i=0;i<valid;i++) {
        redis_cmd_append_sstr(&cmd, Z_STRVAL_P(z_keys[i]), Z_STRLEN_P(z_keys[i]));
    }

    // Kick off our request
    REDIS_PROCESS_REQUEST(redis_sock, cmd.c, cmd.len);
    IF_ATOMIC() {
        redis_sock_read_multibulk_reply_assoc(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock, NULL, z_keys);
    }
    REDIS_PROCESS_RESPONSE_CLOSURE(redis_sock_read_multibulk_reply_assoc, z_keys);
}

PHP_METHOD(Redis, hMset)
{
    zval *object;
    RedisSock *redis_sock;
    char *key = NULL, *cmd, *old_cmd = NULL;
    int key_len, cmd_len, key_free, i, element_count = 2;
    zval *z_hash;
    HashTable *ht_hash;
    smart_str set_cmds = {0};

    if (zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), "Osa",
                                     &object, redis_ce,
                                     &key, &key_len, &z_hash) == FAILURE) {
        RETURN_FALSE;
    }

    if (redis_sock_get(object, &redis_sock TSRMLS_CC, 0) < 0) {
        RETURN_FALSE;
    }

    ht_hash = Z_ARRVAL_P(z_hash);

    if (zend_hash_num_elements(ht_hash) == 0) {
        RETURN_FALSE;
    }

    key_free = redis_key_prefix(redis_sock, &key, &key_len TSRMLS_CC);
    cmd_len = redis_cmd_format(&cmd,
                    "$5" _NL "HMSET" _NL
                    "$%d" _NL "%s" _NL
                    , key_len, key, key_len);
    if(key_free) efree(key);

    /* looping on each item of the array */
    for(i =0, zend_hash_internal_pointer_reset(ht_hash);
        zend_hash_has_more_elements(ht_hash) == SUCCESS;
        i++, zend_hash_move_forward(ht_hash)) {

        char *hkey, hkey_str[40];
        unsigned int hkey_len;
        unsigned long idx;
        int type;
        zval **z_value_p;
        
        char *hval;
        int hval_len, hval_free;

        type = zend_hash_get_current_key_ex(ht_hash, &hkey, &hkey_len, &idx, 0, NULL);

        if(zend_hash_get_current_data(ht_hash, (void**)&z_value_p) == FAILURE) {
            continue;   /* this should never happen */
        }

        if(type != HASH_KEY_IS_STRING) { /* convert to string */
            hkey_len = 1 + sprintf(hkey_str, "%ld", idx);
            hkey = (char*)hkey_str;
        }
        element_count += 2;

        /* key is set. */
        hval_free = redis_serialize(redis_sock, *z_value_p, &hval, &hval_len TSRMLS_CC);

        // Append our member and value in place
        redis_cmd_append_sstr(&set_cmds, hkey, hkey_len - 1);
        redis_cmd_append_sstr(&set_cmds, hval, hval_len);

        if(hval_free) efree(hval);
    }

    // Now construct the entire command
    old_cmd = cmd;
    cmd_len = redis_cmd_format(&cmd, "*%d" _NL "%s%s", element_count, cmd, cmd_len, set_cmds.c, set_cmds.len);
    efree(old_cmd);

    // Free the HMSET bits
    efree(set_cmds.c);

    REDIS_PROCESS_REQUEST(redis_sock, cmd, cmd_len);
    IF_ATOMIC() {
        redis_boolean_response(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock, NULL, NULL);
    }
    REDIS_PROCESS_RESPONSE(redis_boolean_response);
}


PHPAPI int redis_response_enqueued(RedisSock *redis_sock TSRMLS_DC) {

	char *response;
	int response_len, ret = 0;

	if ((response = redis_sock_read(redis_sock, &response_len TSRMLS_CC)) == NULL) {
		return 0;
    }

    if(strncmp(response, "+QUEUED", 7) == 0) {
        ret = 1;
    }
    efree(response);
    return ret;
}

/* flag : get, set {ATOMIC, MULTI, PIPELINE} */

PHP_METHOD(Redis, multi)
{

    RedisSock *redis_sock;
    char *cmd;
	int response_len, cmd_len;
	char * response;
	zval *object;
	long multi_value = MULTI;

	if (zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), "O|l",
                                     &object, redis_ce, &multi_value) == FAILURE) {
        RETURN_FALSE;
    }

    /* if the flag is activated, send the command, the reply will be "QUEUED" or -ERR */

    if (redis_sock_get(object, &redis_sock TSRMLS_CC, 0) < 0) {
        RETURN_FALSE;
    }

	if(multi_value == MULTI || multi_value == PIPELINE) {
		redis_sock->mode = multi_value;
	} else {
        RETURN_FALSE;
	}

    redis_sock->current = NULL;

	IF_MULTI() {
        cmd_len = redis_cmd_format_static(&cmd, "MULTI", "");

		if (redis_sock_write(redis_sock, cmd, cmd_len TSRMLS_CC) < 0) {
        	efree(cmd);
	        RETURN_FALSE;
    	}
	    efree(cmd);

    	if ((response = redis_sock_read(redis_sock, &response_len TSRMLS_CC)) == NULL) {
        	RETURN_FALSE;
    	}

        if(strncmp(response, "+OK", 3) == 0) {
            efree(response);
			RETURN_ZVAL(getThis(), 1, 0);
		}
        efree(response);
		RETURN_FALSE;
	}
	IF_PIPELINE() {
        free_reply_callbacks(getThis(), redis_sock);
		RETURN_ZVAL(getThis(), 1, 0);
	}
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

    if (redis_sock_get(object, &redis_sock TSRMLS_CC, 0) < 0) {
        RETURN_FALSE;
    }

	redis_sock->mode = ATOMIC;
	redis_send_discard(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock);
}

PHPAPI int redis_sock_read_multibulk_pipeline_reply(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock)
{
    zval *z_tab;
    MAKE_STD_ZVAL(z_tab);
    array_init(z_tab);

    redis_sock_read_multibulk_multi_reply_loop(INTERNAL_FUNCTION_PARAM_PASSTHRU,
                    redis_sock, z_tab, 0);

    *return_value = *z_tab;
    efree(z_tab);

    /* free allocated function/request memory */
    free_reply_callbacks(getThis(), redis_sock);

    return 0;

}
/* redis_sock_read_multibulk_multi_reply */
PHPAPI int redis_sock_read_multibulk_multi_reply(INTERNAL_FUNCTION_PARAMETERS,
                                      RedisSock *redis_sock)
{

    char inbuf[1024];
	int numElems;
	zval *z_tab;

    redis_check_eof(redis_sock TSRMLS_CC);

    php_stream_gets(redis_sock->stream, inbuf, 1024);
    if(inbuf[0] != '*') {
        return -1;
    }

	/* number of responses */
    numElems = atoi(inbuf+1);

    if(numElems < 0) {
        return -1;
    }

    zval_dtor(return_value);

    MAKE_STD_ZVAL(z_tab);
    array_init(z_tab);

    redis_sock_read_multibulk_multi_reply_loop(INTERNAL_FUNCTION_PARAM_PASSTHRU,
                    redis_sock, z_tab, numElems);

    *return_value = *z_tab;
    efree(z_tab);

    return 0;
}

void
free_reply_callbacks(zval *z_this, RedisSock *redis_sock) {

	fold_item *fi;
    fold_item *head = redis_sock->head;
	request_item *ri;
    
	for(fi = head; fi; ) {
        fold_item *fi_next = fi->next;
        free(fi);
        fi = fi_next;
    }
    redis_sock->head = NULL;
    redis_sock->current = NULL;

    for(ri = redis_sock->pipeline_head; ri; ) {
        struct request_item *ri_next = ri->next;
        free(ri->request_str);
        free(ri);
        ri = ri_next;
    }
    redis_sock->pipeline_head = NULL;
    redis_sock->pipeline_current = NULL;
}

/* exec */
PHP_METHOD(Redis, exec)
{

    RedisSock *redis_sock;
    char *cmd;
	int cmd_len;
	zval *object;
    struct request_item *ri;

	if (zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), "O",
                                     &object, redis_ce) == FAILURE) {
        RETURN_FALSE;
    }
   	if (redis_sock_get(object, &redis_sock TSRMLS_CC, 0) < 0) {
       	RETURN_FALSE;
    }

	IF_MULTI() {

        cmd_len = redis_cmd_format_static(&cmd, "EXEC", "");

		if (redis_sock_write(redis_sock, cmd, cmd_len TSRMLS_CC) < 0) {
			efree(cmd);
			RETURN_FALSE;
		}
		efree(cmd);

	    if (redis_sock_read_multibulk_multi_reply(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock) < 0) {
            zval_dtor(return_value);
            free_reply_callbacks(object, redis_sock);
            redis_sock->mode = ATOMIC;
            redis_sock->watching = 0;
			RETURN_FALSE;
	    }
        free_reply_callbacks(object, redis_sock);
		redis_sock->mode = ATOMIC;
        redis_sock->watching = 0;
	}

	IF_PIPELINE() {

		char *request = NULL;
		int total = 0;
		int offset = 0;

        /* compute the total request size */
		for(ri = redis_sock->pipeline_head; ri; ri = ri->next) {
            total += ri->request_size;
		}
        if(total) {
		    request = malloc(total);
        }

        /* concatenate individual elements one by one in the target buffer */
		for(ri = redis_sock->pipeline_head; ri; ri = ri->next) {
			memcpy(request + offset, ri->request_str, ri->request_size);
			offset += ri->request_size;
		}

		if(request != NULL) {
		    if (redis_sock_write(redis_sock, request, total TSRMLS_CC) < 0) {
    		    free(request);
                free_reply_callbacks(object, redis_sock);
                redis_sock->mode = ATOMIC;
        		RETURN_FALSE;
		    }
		   	free(request);
		} else {
                redis_sock->mode = ATOMIC;
                free_reply_callbacks(object, redis_sock);
                array_init(return_value); /* empty array when no command was run. */
                return;
        }

	    if (redis_sock_read_multibulk_pipeline_reply(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock) < 0) {
			redis_sock->mode = ATOMIC;
            free_reply_callbacks(object, redis_sock);
			RETURN_FALSE;
	    }
		redis_sock->mode = ATOMIC;
        free_reply_callbacks(object, redis_sock);
	}
}

PHPAPI void fold_this_item(INTERNAL_FUNCTION_PARAMETERS, fold_item *item, RedisSock *redis_sock, zval *z_tab) {
	item->fun(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock, z_tab, item->ctx TSRMLS_CC);
}

PHPAPI int redis_sock_read_multibulk_multi_reply_loop(INTERNAL_FUNCTION_PARAMETERS,
							RedisSock *redis_sock, zval *z_tab, int numElems)
{

    fold_item *head = redis_sock->head;
    fold_item *current = redis_sock->current;
    for(current = head; current; current = current->next) {
		fold_this_item(INTERNAL_FUNCTION_PARAM_PASSTHRU, current, redis_sock, z_tab);
    }
    redis_sock->current = current;
    return 0;
}

PHP_METHOD(Redis, pipeline)
{
    RedisSock *redis_sock;
	zval *object;

	if (zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), "O",
                                     &object, redis_ce) == FAILURE) {
        RETURN_FALSE;
    }

    /* if the flag is activated, send the command, the reply will be "QUEUED" or -ERR */
    if (redis_sock_get(object, &redis_sock TSRMLS_CC, 0) < 0) {
        RETURN_FALSE;
    }
	redis_sock->mode = PIPELINE;

	/*
		NB : we keep the function fold, to detect the last function.
		We need the response format of the n - 1 command. So, we can delete when n > 2, the { 1 .. n - 2} commands
	*/

    free_reply_callbacks(getThis(), redis_sock);

	RETURN_ZVAL(getThis(), 1, 0);
}

/* 
	publish channel message 
	@return the number of subscribers 
*/
PHP_METHOD(Redis, publish)
{
    zval *object;
    RedisSock *redis_sock;
    char *cmd, *key, *val;
    int cmd_len, key_len, val_len, key_free;

    if (zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), "Oss",
                                     &object, redis_ce,
                                     &key, &key_len, &val, &val_len) == FAILURE) {
        RETURN_NULL();
    }

    if (redis_sock_get(object, &redis_sock TSRMLS_CC, 0) < 0) {
        RETURN_FALSE;
    }

	key_free = redis_key_prefix(redis_sock, &key, &key_len TSRMLS_CC);
    cmd_len = redis_cmd_format_static(&cmd, "PUBLISH", "ss", key, key_len, val, val_len);
	if(key_free) efree(key);

	REDIS_PROCESS_REQUEST(redis_sock, cmd, cmd_len);
    IF_ATOMIC() {
		redis_long_response(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock, NULL, NULL);
    }
    REDIS_PROCESS_RESPONSE(redis_long_response);
}

PHPAPI void generic_subscribe_cmd(INTERNAL_FUNCTION_PARAMETERS, char *sub_cmd)
{
	zval *object, *array, **data;
    HashTable *arr_hash;
    HashPosition pointer;
    RedisSock *redis_sock;
    char *cmd = "", *old_cmd = NULL, *key;
    int cmd_len, array_count, key_len, key_free;
	zval *z_tab, **tmp;
	char *type_response;
	
	// Function call information
	zend_fcall_info z_callback;
	zend_fcall_info_cache z_callback_cache;

	zval *z_ret, **z_args[4];
	
	if (zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), "Oaf",
									 &object, redis_ce, &array, &z_callback, &z_callback_cache) == FAILURE) {
		RETURN_FALSE;	
	}

    if (redis_sock_get(object, &redis_sock TSRMLS_CC, 0) < 0) {
        RETURN_FALSE;
    }

    arr_hash    = Z_ARRVAL_P(array);
    array_count = zend_hash_num_elements(arr_hash);

    if (array_count == 0) {
        RETURN_FALSE;
    }
    for (zend_hash_internal_pointer_reset_ex(arr_hash, &pointer);
         zend_hash_get_current_data_ex(arr_hash, (void**) &data,
                                       &pointer) == SUCCESS;
         zend_hash_move_forward_ex(arr_hash, &pointer)) {

        if (Z_TYPE_PP(data) == IS_STRING) {
            char *old_cmd = NULL;
            if(*cmd) {
                old_cmd = cmd;
            }

            // Grab our key and len
            key = Z_STRVAL_PP(data);
            key_len = Z_STRLEN_PP(data);

            // Prefix our key if neccisary
            key_free = redis_key_prefix(redis_sock, &key, &key_len TSRMLS_CC);

            cmd_len = spprintf(&cmd, 0, "%s %s", cmd, key);

            if(old_cmd) {
                efree(old_cmd);
            }

            // Free our key if it was prefixed
            if(key_free) {
            	efree(key);
            }
        }
    }

    old_cmd = cmd;
    cmd_len = spprintf(&cmd, 0, "%s %s\r\n", sub_cmd, cmd);
    efree(old_cmd);
    if (redis_sock_write(redis_sock, cmd, cmd_len TSRMLS_CC) < 0) {
        efree(cmd);
        RETURN_FALSE;
    }
    efree(cmd);
	
	/* read the status of the execution of the command `subscribe` */
	
    z_tab = redis_sock_read_multibulk_reply_zval(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock);
	if(z_tab == NULL) {
		RETURN_FALSE;
	}

	if (zend_hash_index_find(Z_ARRVAL_P(z_tab), 0, (void**)&tmp) == SUCCESS) {
		type_response = Z_STRVAL_PP(tmp);
		if(strcmp(type_response, sub_cmd) != 0) {
			efree(tmp);
			efree(z_tab);	
			RETURN_FALSE;
		} 
	} else {
		efree(z_tab);	
		RETURN_FALSE;
	}
	efree(z_tab);	

	// Set a pointer to our return value and to our arguments.
	z_callback.retval_ptr_ptr = &z_ret;
	z_callback.params = z_args;
	z_callback.no_separation = 0;

	/* Multibulk Response, format : {message type, originating channel, message payload} */
	while(1) {		
		/* call the callback with this z_tab in argument */
		zval **type, **channel, **pattern, **data;
	    z_tab = redis_sock_read_multibulk_reply_zval(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock);
	    int is_pmsg, tab_idx = 1;
		
		if(z_tab == NULL || Z_TYPE_P(z_tab) != IS_ARRAY) {
			//ERROR
			break;
		}

		if (zend_hash_index_find(Z_ARRVAL_P(z_tab), 0, (void**)&type) == FAILURE || Z_TYPE_PP(type) != IS_STRING) {
			break;
		}

		// Make sure we have a message or pmessage
		if(!strncmp(Z_STRVAL_PP(type), "message", 7) || !strncmp(Z_STRVAL_PP(type), "pmessage", 8)) {
			// Is this a pmessage
			is_pmsg = *Z_STRVAL_PP(type) == 'p';
		} else {
			continue;  // It's not a message or pmessage
		}

		// If this is a pmessage, we'll want to extract the pattern first
		if(is_pmsg) {
			// Extract pattern
			if(zend_hash_index_find(Z_ARRVAL_P(z_tab), tab_idx++, (void**)&pattern) == FAILURE) {
				break;
			}
		}

		// Extract channel and data
		if (zend_hash_index_find(Z_ARRVAL_P(z_tab), tab_idx++, (void**)&channel) == FAILURE) {
			break;
		}
		if (zend_hash_index_find(Z_ARRVAL_P(z_tab), tab_idx++, (void**)&data) == FAILURE) {
			break;
		}

		// Always pass the Redis object through
		z_args[0] = &getThis();

		// Set up our callback args depending on the message type
		if(is_pmsg) {
			z_args[1] = pattern;
			z_args[2] = channel;
			z_args[3] = data;
		} else {
			z_args[1] = channel;
			z_args[2] = data;
		}

		// Set our argument information
		z_callback.param_count = tab_idx;

		// Break if we can't call the function
		if(zend_call_function(&z_callback, &z_callback_cache TSRMLS_CC) != SUCCESS) {
			break;
		}

		// If we have a return value, free it.  Note, we could use the return value to break the subscribe loop
		if(z_ret) zval_ptr_dtor(&z_ret);

        /* TODO: provide a way to break out of the loop. */
		zval_dtor(z_tab);
		efree(z_tab);
	}
}

/* {{{ proto void Redis::psubscribe(Array(pattern1, pattern2, ... patternN))
 */
PHP_METHOD(Redis, psubscribe)
{
	generic_subscribe_cmd(INTERNAL_FUNCTION_PARAM_PASSTHRU, "psubscribe");
}

/* {{{ proto void Redis::subscribe(Array(channel1, channel2, ... channelN))
 */
PHP_METHOD(Redis, subscribe) {
	generic_subscribe_cmd(INTERNAL_FUNCTION_PARAM_PASSTHRU, "subscribe");
}

/** 
 *	[p]unsubscribe channel_0 channel_1 ... channel_n
 *  [p]unsubscribe(array(channel_0, channel_1, ..., channel_n))
 * response format :
 * array(
 * 	channel_0 => TRUE|FALSE,
 *	channel_1 => TRUE|FALSE,
 *	...
 *	channel_n => TRUE|FALSE
 * );
 **/

PHPAPI void generic_unsubscribe_cmd(INTERNAL_FUNCTION_PARAMETERS, char *unsub_cmd)
{
    zval *object, *array, **data;
    HashTable *arr_hash;
    HashPosition pointer;
    RedisSock *redis_sock;
    char *cmd = "", *old_cmd = NULL;
    int cmd_len, array_count;
	
	int i;
	zval *z_tab, **z_channel;
	
	if (zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), "Oa", 
									 &object, redis_ce, &array) == FAILURE) {
		RETURN_FALSE;	
	}
    if (redis_sock_get(object, &redis_sock TSRMLS_CC, 0) < 0) {
        RETURN_FALSE;
    }

    arr_hash    = Z_ARRVAL_P(array);
    array_count = zend_hash_num_elements(arr_hash);

    if (array_count == 0) {
        RETURN_FALSE;
    }

    for (zend_hash_internal_pointer_reset_ex(arr_hash, &pointer);
         zend_hash_get_current_data_ex(arr_hash, (void**) &data,
                                       &pointer) == SUCCESS;
         zend_hash_move_forward_ex(arr_hash, &pointer)) {

        if (Z_TYPE_PP(data) == IS_STRING) {
            char *old_cmd = NULL;
            if(*cmd) {
                old_cmd = cmd;
            }
            cmd_len = spprintf(&cmd, 0, "%s %s", cmd, Z_STRVAL_PP(data));
            if(old_cmd) {
                efree(old_cmd);
            }
        }
    }

    old_cmd = cmd;
    cmd_len = spprintf(&cmd, 0, "%s %s\r\n", unsub_cmd, cmd);
    efree(old_cmd);

    if (redis_sock_write(redis_sock, cmd, cmd_len TSRMLS_CC) < 0) {
        efree(cmd);
        RETURN_FALSE;
    }
    efree(cmd);

	i = 1;
	array_init(return_value);

	while( i <= array_count) {
	    z_tab = redis_sock_read_multibulk_reply_zval(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock);

		if(Z_TYPE_P(z_tab) == IS_ARRAY) { 
			if (zend_hash_index_find(Z_ARRVAL_P(z_tab), 1, (void**)&z_channel) == FAILURE) {
				RETURN_FALSE;
			}		
			add_assoc_bool(return_value, Z_STRVAL_PP(z_channel), 1);
		} else {
			//error
			efree(z_tab);
			RETURN_FALSE;
		}
		efree(z_tab);
		i ++;
	}
}

PHP_METHOD(Redis, unsubscribe)
{
	generic_unsubscribe_cmd(INTERNAL_FUNCTION_PARAM_PASSTHRU, "UNSUBSCRIBE");
}

PHP_METHOD(Redis, punsubscribe)
{
	generic_unsubscribe_cmd(INTERNAL_FUNCTION_PARAM_PASSTHRU, "PUNSUBSCRIBE");
}

/* {{{ proto string Redis::bgrewriteaof()
 */
PHP_METHOD(Redis, bgrewriteaof)
{
    char *cmd;
    int cmd_len = redis_cmd_format_static(&cmd, "BGREWRITEAOF", "");
    generic_empty_cmd(INTERNAL_FUNCTION_PARAM_PASSTHRU, cmd, cmd_len);

}
/* }}} */

/* {{{ proto string Redis::slaveof([host, port])
 */
PHP_METHOD(Redis, slaveof)
{
    zval *object;
    RedisSock *redis_sock;
    char *cmd = "", *host = NULL;
    int cmd_len, host_len;
    long port = 6379;

	if (zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), "O|sl",
									 &object, redis_ce, &host, &host_len, &port) == FAILURE) {
		RETURN_FALSE;
	}
    if (redis_sock_get(object, &redis_sock TSRMLS_CC, 0) < 0) {
        RETURN_FALSE;
    }

    if(host && host_len) {
        cmd_len = redis_cmd_format_static(&cmd, "SLAVEOF", "sd", host, host_len, (int)port);
    } else {
        cmd_len = redis_cmd_format_static(&cmd, "SLAVEOF", "ss", "NO", 2, "ONE", 3);
    }

	REDIS_PROCESS_REQUEST(redis_sock, cmd, cmd_len);
    IF_ATOMIC() {
	  redis_boolean_response(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock, NULL, NULL);
    }
    REDIS_PROCESS_RESPONSE(redis_boolean_response);
}
/* }}} */

/* {{{ proto string Redis::object(key)
 */
PHP_METHOD(Redis, object)
{
    zval *object;
    RedisSock *redis_sock;
    char *cmd = "", *info = NULL, *key = NULL;
    int cmd_len, info_len, key_len;

	if (zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), "Oss",
									 &object, redis_ce, &info, &info_len, &key, &key_len) == FAILURE) {
		RETURN_FALSE;
	}
    if (redis_sock_get(object, &redis_sock TSRMLS_CC, 0) < 0) {
        RETURN_FALSE;
    }

    cmd_len = redis_cmd_format_static(&cmd, "OBJECT", "ss", info, info_len, key, key_len);
	REDIS_PROCESS_REQUEST(redis_sock, cmd, cmd_len);

	if(info_len == 8 && (strncasecmp(info, "refcount", 8) == 0 || strncasecmp(info, "idletime", 8) == 0)) {
		IF_ATOMIC() {
		  redis_long_response(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock, NULL, NULL);
		}
		REDIS_PROCESS_RESPONSE(redis_long_response);
	} else if(info_len == 8 && strncasecmp(info, "encoding", 8) == 0) {
		IF_ATOMIC() {
		  redis_string_response(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock, NULL, NULL);
		}
		REDIS_PROCESS_RESPONSE(redis_string_response);
	} else { /* fail */
		IF_ATOMIC() {
		  redis_boolean_response(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock, NULL, NULL);
		}
		REDIS_PROCESS_RESPONSE(redis_boolean_response);
	}
}
/* }}} */

/* {{{ proto string Redis::getOption($option)
 */
PHP_METHOD(Redis, getOption)  {
    RedisSock *redis_sock;
    zval *object;
    long option;

	if (zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), "Ol",
									 &object, redis_ce, &option) == FAILURE) {
		RETURN_FALSE;
	}

    if (redis_sock_get(object, &redis_sock TSRMLS_CC, 0) < 0) {
        RETURN_FALSE;
    }

    switch(option) {

            case REDIS_OPT_SERIALIZER:
                    RETURN_LONG(redis_sock->serializer);

            case REDIS_OPT_PREFIX:
					if(redis_sock->prefix) {
						RETURN_STRINGL(redis_sock->prefix, redis_sock->prefix_len, 1);
					}
                    RETURN_NULL();

            case REDIS_OPT_READ_TIMEOUT:
                    RETURN_DOUBLE(redis_sock->read_timeout);

            default:
                    RETURN_FALSE;

    }
}
/* }}} */

/* {{{ proto string Redis::setOption(string $option, mixed $value)
 */
PHP_METHOD(Redis, setOption) {
    RedisSock *redis_sock;
    zval *object;
    long option, val_long;
	char *val_str;
	int val_len;
    struct timeval read_tv;

	if (zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), "Ols",
									 &object, redis_ce, &option, &val_str, &val_len) == FAILURE) {
		RETURN_FALSE;
	}

    if (redis_sock_get(object, &redis_sock TSRMLS_CC, 0) < 0) {
        RETURN_FALSE;
    }

    switch(option) {
            case REDIS_OPT_SERIALIZER:
					val_long = atol(val_str);
                    if(val_long == REDIS_SERIALIZER_NONE
#ifdef HAVE_REDIS_IGBINARY
						 	|| val_long == REDIS_SERIALIZER_IGBINARY
#endif
							|| val_long == REDIS_SERIALIZER_PHP) {
                            redis_sock->serializer = val_long;
                            RETURN_TRUE;
                    } else {
                            RETURN_FALSE;
                    }
                    break;

			case REDIS_OPT_PREFIX:
					if(redis_sock->prefix) {
						efree(redis_sock->prefix);
					}
					if(val_len == 0) {
						redis_sock->prefix = NULL;
						redis_sock->prefix_len = 0;
					} else {
						redis_sock->prefix_len = val_len;
						redis_sock->prefix = ecalloc(1+val_len, 1);
						memcpy(redis_sock->prefix, val_str, val_len);
					}
					RETURN_TRUE;

            case REDIS_OPT_READ_TIMEOUT:
                    redis_sock->read_timeout = atof(val_str);
                    if(redis_sock->stream) {
                        read_tv.tv_sec  = (time_t)redis_sock->read_timeout;
                        read_tv.tv_usec = (int)((redis_sock->read_timeout - read_tv.tv_sec) * 1000000);
                        php_stream_set_option(redis_sock->stream, PHP_STREAM_OPTION_READ_TIMEOUT,
                                              0, &read_tv);
                    }
                    RETURN_TRUE;

            default:
                    RETURN_FALSE;
    }
}
/* }}} */

/* {{{ proto boolean Redis::config(string op, string key [, mixed value])
 */
PHP_METHOD(Redis, config)
{
    zval *object;
    RedisSock *redis_sock;
    char *key = NULL, *val = NULL, *cmd, *op = NULL;
    int key_len, val_len, cmd_len, op_len;
	enum {CFG_GET, CFG_SET} mode;

    if (zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), "Oss|s",
                                     &object, redis_ce, &op, &op_len, &key, &key_len,
                                     &val, &val_len) == FAILURE) {
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

    if (redis_sock_get(object, &redis_sock TSRMLS_CC, 0) < 0) {
        RETURN_FALSE;
    }

    if (mode == CFG_GET && val == NULL) {
        cmd_len = redis_cmd_format_static(&cmd, "CONFIG", "ss", op, op_len, key, key_len);

		REDIS_PROCESS_REQUEST(redis_sock, cmd, cmd_len)
		IF_ATOMIC() {
			redis_sock_read_multibulk_reply_zipped_strings(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock, NULL, NULL);
		}
		REDIS_PROCESS_RESPONSE(redis_sock_read_multibulk_reply_zipped_strings);

    } else if(mode == CFG_SET && val != NULL) {
        cmd_len = redis_cmd_format_static(&cmd, "CONFIG", "sss", op, op_len, key, key_len, val, val_len);

		REDIS_PROCESS_REQUEST(redis_sock, cmd, cmd_len)
		IF_ATOMIC() {
			redis_boolean_response(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock, NULL, NULL);
		}
		REDIS_PROCESS_RESPONSE(redis_boolean_response);
    } else {
		RETURN_FALSE;
	}
}
/* }}} */


/* {{{ proto boolean Redis::slowlog(string arg, [int option])
 */
PHP_METHOD(Redis, slowlog) {
    zval *object;
    RedisSock *redis_sock;
    char *arg, *cmd;
    int arg_len, cmd_len;
    long option;
    enum {SLOWLOG_GET, SLOWLOG_LEN, SLOWLOG_RESET} mode;

    // Make sure we can get parameters
    if(zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), "Os|l",
                                    &object, redis_ce, &arg, &arg_len, &option) == FAILURE)
    {
        RETURN_FALSE;
    }

    // Figure out what kind of slowlog command we're executing
    if(!strncasecmp(arg, "GET", 3)) {
        mode = SLOWLOG_GET;
    } else if(!strncasecmp(arg, "LEN", 3)) {
        mode = SLOWLOG_LEN;
    } else if(!strncasecmp(arg, "RESET", 5)) {
        mode = SLOWLOG_RESET;
    } else {
        // This command is not valid
        RETURN_FALSE;
    }

    // Make sure we can grab our redis socket
    if(redis_sock_get(object, &redis_sock TSRMLS_CC, 0) < 0) {
        RETURN_FALSE;
    }

    // Create our command.  For everything except SLOWLOG GET (with an arg) it's just two parts
    if(mode == SLOWLOG_GET && ZEND_NUM_ARGS() == 2) {
        cmd_len = redis_cmd_format_static(&cmd, "SLOWLOG", "sl", arg, arg_len, option);
    } else {
        cmd_len = redis_cmd_format_static(&cmd, "SLOWLOG", "s", arg, arg_len);
    }

    // Kick off our command
    REDIS_PROCESS_REQUEST(redis_sock, cmd, cmd_len);
    IF_ATOMIC() {
        if(redis_read_variant_reply(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock, NULL) < 0) {
            RETURN_FALSE;
        }
    }
    REDIS_PROCESS_RESPONSE(redis_read_variant_reply);
}

// Construct an EVAL or EVALSHA command, with option argument array and number of arguments that are keys parameter
PHPAPI int
redis_build_eval_cmd(RedisSock *redis_sock, char **ret, char *keyword, char *value, int val_len, zval *args, int keys_count TSRMLS_DC) {
	zval **elem;
	HashTable *args_hash;
	HashPosition hash_pos;
	int cmd_len, args_count = 0;
	int eval_cmd_count = 2;

	// If we've been provided arguments, we'll want to include those in our eval command
	if(args != NULL) {
		// Init our hash array value, and grab the count
	    args_hash = Z_ARRVAL_P(args);
	    args_count = zend_hash_num_elements(args_hash);

	    // We only need to process the arguments if the array is non empty
	    if(args_count >  0) {
	    	// Header for our EVAL command
	    	cmd_len = redis_cmd_format_header(ret, keyword, eval_cmd_count + args_count);

	    	// Now append the script itself, and the number of arguments to treat as keys
	    	cmd_len = redis_cmd_append_str(ret, cmd_len, value, val_len);
	    	cmd_len = redis_cmd_append_int(ret, cmd_len, keys_count);

			// Iterate the values in our "keys" array
			for(zend_hash_internal_pointer_reset_ex(args_hash, &hash_pos);
				zend_hash_get_current_data_ex(args_hash, (void **)&elem, &hash_pos) == SUCCESS;
				zend_hash_move_forward_ex(args_hash, &hash_pos))
			{
				zval *z_tmp = NULL;
				char *key, *old_cmd;
				int key_len, key_free;

				if(Z_TYPE_PP(elem) == IS_STRING) {
					key = Z_STRVAL_PP(elem);
					key_len = Z_STRLEN_PP(elem);
				} else {
					// Convert it to a string
					MAKE_STD_ZVAL(z_tmp);
					*z_tmp = **elem;
					zval_copy_ctor(z_tmp);
					convert_to_string(z_tmp);

					key = Z_STRVAL_P(z_tmp);
					key_len = Z_STRLEN_P(z_tmp);
				}

				// Keep track of the old command pointer
				old_cmd = *ret;

				// If this is still a key argument, prefix it if we've been set up to prefix keys
				key_free = keys_count-- > 0 ? redis_key_prefix(redis_sock, &key, &key_len TSRMLS_CC) : 0;

				// Append this key to our EVAL command, free our old command
				cmd_len = redis_cmd_format(ret, "%s$%d" _NL "%s" _NL, *ret, cmd_len, key_len, key, key_len);
				efree(old_cmd);

				// Free our key, old command if we need to
				if(key_free) efree(key);

				// Free our temporary zval (converted from non string) if we've got one
				if(z_tmp) {
					zval_dtor(z_tmp);
					efree(z_tmp);
				}
			}
	    }
	}

	// If there weren't any arguments (none passed, or an empty array), construct a standard no args command
	if(args_count < 1) {
		cmd_len = redis_cmd_format_static(ret, keyword, "sd", value, val_len, 0);
	}

	// Return our command length
	return cmd_len;
}

/* {{{ proto variant Redis::evalsha(string script_sha1, [array keys, int num_key_args])
 */
PHP_METHOD(Redis, evalsha)
{
	zval *object, *args= NULL;
	char *cmd, *sha;
	int cmd_len, sha_len;
	long keys_count = 0;
	RedisSock *redis_sock;

	if(zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), "Os|al",
								    &object, redis_ce, &sha, &sha_len, &args, &keys_count) == FAILURE) {
		RETURN_FALSE;
	}

	// Attempt to grab socket
	if(redis_sock_get(object, &redis_sock TSRMLS_CC, 0) < 0) {
		RETURN_FALSE;
	}

	// Construct our EVALSHA command
	cmd_len = redis_build_eval_cmd(redis_sock, &cmd, "EVALSHA", sha, sha_len, args, keys_count TSRMLS_CC);

	REDIS_PROCESS_REQUEST(redis_sock, cmd, cmd_len);
	IF_ATOMIC() {
		if(redis_read_variant_reply(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock, NULL) < 0) {
			RETURN_FALSE;
		}
	}
	REDIS_PROCESS_RESPONSE(redis_read_variant_reply);
}

/* {{{ proto variant Redis::eval(string script, [array keys, int num_key_args])
 */
PHP_METHOD(Redis, eval)
{
	zval *object, *args = NULL;
	RedisSock *redis_sock;
	char *script, *cmd = "";
	int script_len, cmd_len;
	long keys_count = 0;

	// Attempt to parse parameters
	if(zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), "Os|al",
							 &object, redis_ce, &script, &script_len, &args, &keys_count) == FAILURE) {
		RETURN_FALSE;
	}

	// Attempt to grab socket
    if (redis_sock_get(object, &redis_sock TSRMLS_CC, 0) < 0) {
        RETURN_FALSE;
    }

    // Construct our EVAL command
    cmd_len = redis_build_eval_cmd(redis_sock, &cmd, "EVAL", script, script_len, args, keys_count TSRMLS_CC);

    REDIS_PROCESS_REQUEST(redis_sock, cmd, cmd_len);
    IF_ATOMIC() {
    	if(redis_read_variant_reply(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock, NULL) < 0) {
    		RETURN_FALSE;
    	}
    }
    REDIS_PROCESS_RESPONSE(redis_read_variant_reply);
}

PHPAPI int
redis_build_script_exists_cmd(char **ret, zval **argv, int argc) {
	// Our command length and iterator
	int cmd_len = 0, i;

	// Start building our command
	cmd_len = redis_cmd_format_header(ret, "SCRIPT", argc + 1); // +1 for "EXISTS"
	cmd_len = redis_cmd_append_str(ret, cmd_len, "EXISTS", 6);

	// Iterate our arguments
	for(i=0;i<argc;i++) {
		// Convert our argument to a string if we need to
		convert_to_string(argv[i]);

		// Append this script sha to our SCRIPT EXISTS command
		cmd_len = redis_cmd_append_str(ret, cmd_len, Z_STRVAL_P(argv[i]), Z_STRLEN_P(argv[i]));
	}

	// Success
	return cmd_len;
}

/* {{{ proto status Redis::script('flush')
 * {{{ proto status Redis::script('kill')
 * {{{ proto string Redis::script('load', lua_script)
 * {{{ proto int Reids::script('exists', script_sha1 [, script_sha2, ...])
 */
PHP_METHOD(Redis, script) {
	zval **z_args;
	RedisSock *redis_sock;
	int cmd_len, argc;
	char *cmd;

	// Attempt to grab our socket
	if(redis_sock_get(getThis(), &redis_sock TSRMLS_CC, 0) < 0) {
		RETURN_FALSE;
	}

	// Grab the number of arguments
	argc = ZEND_NUM_ARGS();

	// Allocate an array big enough to store our arguments
	z_args = emalloc(argc * sizeof(zval*));

	// Make sure we can grab our arguments, we have a string directive
	if(zend_get_parameters_array(ht, argc, z_args) == FAILURE ||
	   (argc < 1 || Z_TYPE_P(z_args[0]) != IS_STRING))
	{
		efree(z_args);
		RETURN_FALSE;
	}

	// Branch based on the directive
	if(!strcasecmp(Z_STRVAL_P(z_args[0]), "flush") || !strcasecmp(Z_STRVAL_P(z_args[0]), "kill")) {
		// Simple SCRIPT FLUSH, or SCRIPT_KILL command
		cmd_len = redis_cmd_format_static(&cmd, "SCRIPT", "s", Z_STRVAL_P(z_args[0]), Z_STRLEN_P(z_args[0]));
	} else if(!strcasecmp(Z_STRVAL_P(z_args[0]), "load")) {
		// Make sure we have a second argument, and it's not empty.  If it is
		// empty, we can just return an empty array (which is what Redis does)
		if(argc < 2 || Z_TYPE_P(z_args[1]) != IS_STRING || Z_STRLEN_P(z_args[1]) < 1) {
			// Free our args
			efree(z_args);
			RETURN_FALSE;
		}

		// Format our SCRIPT LOAD command
		cmd_len = redis_cmd_format_static(&cmd, "SCRIPT", "ss", "LOAD", 4, Z_STRVAL_P(z_args[1]), Z_STRLEN_P(z_args[1]));
	} else if(!strcasecmp(Z_STRVAL_P(z_args[0]), "exists")) {
		// Construct our SCRIPT EXISTS command
		cmd_len = redis_build_script_exists_cmd(&cmd, &(z_args[1]), argc-1);
	} else {
		// Unknown directive
		efree(z_args);
		RETURN_FALSE;
	}

	// Free our alocated arguments
	efree(z_args);

	// Kick off our request
	REDIS_PROCESS_REQUEST(redis_sock, cmd, cmd_len);
	IF_ATOMIC() {
		if(redis_read_variant_reply(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock, NULL) < 0) {
			RETURN_FALSE;
		}
	}
	REDIS_PROCESS_RESPONSE(redis_read_variant_reply);
}

/* {{{ proto DUMP key
 */
PHP_METHOD(Redis, dump) {
	zval *object;
	RedisSock *redis_sock;
	char *cmd, *key;
	int cmd_len, key_len, key_free;

	// Parse our arguments
	if(zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), "Os", &object, redis_ce,
									&key, &key_len) == FAILURE) {
		RETURN_FALSE;
	}

	// Grab our socket
	if(redis_sock_get(object, &redis_sock TSRMLS_CC, 0) < 0) {
		RETURN_FALSE;
	}

	// Prefix our key if we need to
	key_free = redis_key_prefix(redis_sock, &key, &key_len TSRMLS_CC);
	cmd_len = redis_cmd_format_static(&cmd, "DUMP", "s", key, key_len);
	if(key_free) efree(key);

	// Kick off our request
	REDIS_PROCESS_REQUEST(redis_sock, cmd, cmd_len);
	IF_ATOMIC() {
		redis_ping_response(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock, NULL, NULL);
	}
	REDIS_PROCESS_RESPONSE(redis_ping_response);
}

/*
 * {{{ proto Redis::restore(ttl, key, value)
 */
PHP_METHOD(Redis, restore) {
	zval *object;
	RedisSock *redis_sock;
	char *cmd, *key, *value;
	int cmd_len, key_len, value_len, key_free;
	long ttl;

	// Parse our arguments
	if(zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), "Osls", &object, redis_ce,
									&key, &key_len, &ttl, &value, &value_len) == FAILURE) {
		RETURN_FALSE;
	}

	// Grab our socket
	if(redis_sock_get(object, &redis_sock TSRMLS_CC, 0) < 0) {
		RETURN_FALSE;
	}

	// Prefix the key if we need to
	key_free = redis_key_prefix(redis_sock, &key, &key_len TSRMLS_CC);
	cmd_len = redis_cmd_format_static(&cmd, "RESTORE", "sls", key, key_len, ttl, value, value_len);
	if(key_free) efree(key);

	// Kick off our restore request
	REDIS_PROCESS_REQUEST(redis_sock, cmd, cmd_len);
	IF_ATOMIC() {
		redis_boolean_response(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock, NULL, NULL);
	}
	REDIS_PROCESS_RESPONSE(redis_boolean_response);
}

/*
 * {{{ proto Redis::migrate(host port key dest-db timeout)
 */
PHP_METHOD(Redis, migrate) {
	zval *object;
	RedisSock *redis_sock;
	char *cmd, *host, *key;
	int cmd_len, host_len, key_len, key_free;
	long port, dest_db, timeout;

	// Parse arguments
	if(zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), "Oslsll", &object, redis_ce,
									&host, &host_len, &port, &key, &key_len, &dest_db, &timeout) == FAILURE) {
		RETURN_FALSE;
	}

	// Grabg our socket
	if(redis_sock_get(object, &redis_sock TSRMLS_CC, 0) < 0) {
		RETURN_FALSE;
	}

	// Prefix our key if we need to, build our command
	key_free = redis_key_prefix(redis_sock, &key, &key_len TSRMLS_CC);
	cmd_len = redis_cmd_format_static(&cmd, "MIGRATE", "sdsdd", host, host_len, port, key, key_len, dest_db, timeout);
	if(key_free) efree(key);

	// Kick off our MIGRATE request
	REDIS_PROCESS_REQUEST(redis_sock, cmd, cmd_len);
	IF_ATOMIC() {
		redis_boolean_response(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock, NULL, NULL);
	}
	REDIS_PROCESS_RESPONSE(redis_boolean_response);
}

/*
 * {{{ proto Redis::_prefix(key)
 */
PHP_METHOD(Redis, _prefix) {
	zval *object;
	RedisSock *redis_sock;
	char *key;
	int key_len;

	// Parse our arguments
	if(zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), "Os", &object, redis_ce,
								    &key, &key_len) == FAILURE) {
		RETURN_FALSE;
	}
	// Grab socket
	if(redis_sock_get(object, &redis_sock TSRMLS_CC, 0) < 0) {
		RETURN_FALSE;
	}

	// Prefix our key if we need to
	if(redis_sock->prefix != NULL && redis_sock->prefix_len > 0) {
		redis_key_prefix(redis_sock, &key, &key_len TSRMLS_CC);
		RETURN_STRINGL(key, key_len, 0);
	} else {
		RETURN_STRINGL(key, key_len, 1);
	}
}

/*
 * {{{ proto Redis::_unserialize(value)
 */
PHP_METHOD(Redis, _unserialize) {
	zval *object;
	RedisSock *redis_sock;
	char *value;
	int value_len;

	// Parse our arguments
	if(zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), "Os", &object, redis_ce,
									&value, &value_len) == FAILURE) {
		RETURN_FALSE;
	}
	// Grab socket
	if(redis_sock_get(object, &redis_sock TSRMLS_CC, 0) < 0) {
		RETURN_FALSE;
	}

	// We only need to attempt unserialization if we have a serializer running
	if(redis_sock->serializer != REDIS_SERIALIZER_NONE) {
		zval *z_ret = NULL;
		if(redis_unserialize(redis_sock, value, value_len, &z_ret TSRMLS_CC) == 0) {
			// Badly formed input, throw an execption
			zend_throw_exception(redis_exception_ce, "Invalid serialized data, or unserialization error", 0 TSRMLS_CC);
			RETURN_FALSE;
		}
		RETURN_ZVAL(z_ret, 0, 1);
	} else {
		// Just return the value that was passed to us
		RETURN_STRINGL(value, value_len, 1);
	}
}

/*
 * {{{ proto Redis::getLastError()
 */
PHP_METHOD(Redis, getLastError) {
	zval *object;
	RedisSock *redis_sock;

	// Grab our object
	if(zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), "O", &object, redis_ce) == FAILURE) {
		RETURN_FALSE;
	}
	// Grab socket
	if(redis_sock_get(object, &redis_sock TSRMLS_CC, 0) < 0) {
		RETURN_FALSE;
	}

	// Return our last error or NULL if we don't have one
	if(redis_sock->err != NULL && redis_sock->err_len > 0) {
		RETURN_STRINGL(redis_sock->err, redis_sock->err_len, 1);
	} else {
		RETURN_NULL();
	}
}

/*
 * {{{ proto Redis::clearLastError()
 */
PHP_METHOD(Redis, clearLastError) {
	zval *object;
	RedisSock *redis_sock;

	// Grab our object
	if(zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), "O", &object, redis_ce) == FAILURE) {
		RETURN_FALSE;
	}
	// Grab socket
	if(redis_sock_get(object, &redis_sock TSRMLS_CC, 0) < 0) {
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
 * {{{ proto Redis::time()
 */
PHP_METHOD(Redis, time) {
	zval *object;
	RedisSock *redis_sock;
	char *cmd;
	int cmd_len;

	// Grab our object
	if(zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), "O", &object, redis_ce) == FAILURE) {
		RETURN_FALSE;
	}
	// Grab socket
	if(redis_sock_get(object, &redis_sock TSRMLS_CC, 0) < 0) {
		RETURN_FALSE;
	}

	// Build TIME command
	cmd_len = redis_cmd_format_static(&cmd, "TIME", "");

	// Execute or queue command
	REDIS_PROCESS_REQUEST(redis_sock, cmd, cmd_len);
	IF_ATOMIC() {
		if(redis_sock_read_multibulk_reply_raw(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock, NULL, NULL) < 0) {
			RETURN_FALSE;
		}
	}
	REDIS_PROCESS_RESPONSE(redis_sock_read_multibulk_reply_raw);
}

/*
 * Introspection stuff
 */

/*
 * {{{ proto Redis::IsConnected
 */
PHP_METHOD(Redis, isConnected) {
    RedisSock *redis_sock;

    if((redis_sock = redis_sock_get_connected(INTERNAL_FUNCTION_PARAM_PASSTHRU))) {
        RETURN_TRUE;
    } else {
        RETURN_FALSE;
    }
}

/*
 * {{{ proto Redis::getHost()
 */
PHP_METHOD(Redis, getHost) {
    RedisSock *redis_sock;

    if((redis_sock = redis_sock_get_connected(INTERNAL_FUNCTION_PARAM_PASSTHRU))) {
        RETURN_STRING(redis_sock->host, 1);
    } else {
        RETURN_FALSE;
    }
}

/*
 * {{{ proto Redis::getPort()
 */
PHP_METHOD(Redis, getPort) {
    RedisSock *redis_sock;

    if((redis_sock = redis_sock_get_connected(INTERNAL_FUNCTION_PARAM_PASSTHRU))) {
        // Return our port
        RETURN_LONG(redis_sock->port);
    } else {
        RETURN_FALSE;
    }
}

/*
 * {{{ proto Redis::getDBNum
 */
PHP_METHOD(Redis, getDBNum) {
    RedisSock *redis_sock;

    if((redis_sock = redis_sock_get_connected(INTERNAL_FUNCTION_PARAM_PASSTHRU))) {
        // Return our db number
        RETURN_LONG(redis_sock->dbNumber);
    } else {
        RETURN_FALSE;
    }
}

/*
 * {{{ proto Redis::getTimeout
 */
PHP_METHOD(Redis, getTimeout) {
    RedisSock *redis_sock;

    if((redis_sock = redis_sock_get_connected(INTERNAL_FUNCTION_PARAM_PASSTHRU))) {
        RETURN_DOUBLE(redis_sock->timeout);
    } else {
        RETURN_FALSE;
    }
}

/*
 * {{{ proto Redis::getReadTimeout
 */
PHP_METHOD(Redis, getReadTimeout) {
    RedisSock *redis_sock;

    if((redis_sock = redis_sock_get_connected(INTERNAL_FUNCTION_PARAM_PASSTHRU))) {
        RETURN_DOUBLE(redis_sock->read_timeout);
    } else {
        RETURN_FALSE;
    }
}

/*
 * {{{ proto Redis::getPersistentID
 */
PHP_METHOD(Redis, getPersistentID) {
    RedisSock *redis_sock;

    if((redis_sock = redis_sock_get_connected(INTERNAL_FUNCTION_PARAM_PASSTHRU))) {
        if(redis_sock->persistent_id != NULL) {
            RETURN_STRING(redis_sock->persistent_id, 1);
        } else {
            RETURN_NULL();
        }
    } else {
        RETURN_FALSE;
    }
}

/*
 * {{{ proto Redis::getAuth
 */
PHP_METHOD(Redis, getAuth) {
    RedisSock *redis_sock;

    if((redis_sock = redis_sock_get_connected(INTERNAL_FUNCTION_PARAM_PASSTHRU))) {
        if(redis_sock->auth != NULL) {
            RETURN_STRING(redis_sock->auth, 1);
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
    int cmd_len, opt_len, arg_len;

    // Parse our method parameters
    if(zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), "Os|s", 
        &object, redis_ce, &opt, &opt_len, &arg, &arg_len) == FAILURE) 
    {
        RETURN_FALSE;
    }

    // Grab our socket
    if(redis_sock_get(object, &redis_sock TSRMLS_CC, 0) < 0) {
        RETURN_FALSE;
    }

    // Build our CLIENT command
    if(ZEND_NUM_ARGS() == 2) {
        cmd_len = redis_cmd_format_static(&cmd, "CLIENT", "ss", opt, opt_len,
                                          arg, arg_len); 
    } else {
        cmd_len = redis_cmd_format_static(&cmd, "CLIENT", "s", opt, opt_len);
    }

    // Execute our queue command
    REDIS_PROCESS_REQUEST(redis_sock, cmd, cmd_len);

    // We handle CLIENT LIST with a custom response function
    if(!strncasecmp(opt, "list", 4)) {
        IF_ATOMIC() {
            redis_client_list_reply(INTERNAL_FUNCTION_PARAM_PASSTHRU,redis_sock,NULL);
        }
        REDIS_PROCESS_RESPONSE(redis_client_list_reply);
    } else {
        IF_ATOMIC() {
            redis_read_variant_reply(INTERNAL_FUNCTION_PARAM_PASSTHRU,redis_sock,NULL);
        }
        REDIS_PROCESS_RESPONSE(redis_read_variant_reply);
    }
}

/* vim: set tabstop=4 softtabstops=4 noexpandtab shiftwidth=4: */
