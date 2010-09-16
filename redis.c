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
  +----------------------------------------------------------------------+
*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "common.h"
#include "ext/standard/info.h"
#include "php_redis.h"
#include <zend_exceptions.h>

#include "library.h"

#define _NL "\r\n"
#define R_SUB_CALLBACK_CLASS_TYPE 1
#define R_SUB_CALLBACK_FT_TYPE 2

static int le_redis_sock;
static int le_redis_multi_access_type;
int le_redis_multi_head;
int le_redis_multi_current;
int le_redis_pipeline_head;
int le_redis_pipeline_current;

static zend_class_entry *redis_ce;
static zend_class_entry *redis_exception_ce;
static zend_class_entry *spl_ce_RuntimeException = NULL;


ZEND_DECLARE_MODULE_GLOBALS(redis)

static zend_function_entry redis_functions[] = {
     PHP_ME(Redis, __construct, NULL, ZEND_ACC_PUBLIC)
     PHP_ME(Redis, connect, NULL, ZEND_ACC_PUBLIC)
     PHP_ME(Redis, close, NULL, ZEND_ACC_PUBLIC)
     PHP_ME(Redis, ping, NULL, ZEND_ACC_PUBLIC)
     PHP_ME(Redis, get, NULL, ZEND_ACC_PUBLIC)
     PHP_ME(Redis, set, NULL, ZEND_ACC_PUBLIC)
     PHP_ME(Redis, setex, NULL, ZEND_ACC_PUBLIC)
     PHP_ME(Redis, setnx, NULL, ZEND_ACC_PUBLIC)
     PHP_ME(Redis, getSet, NULL, ZEND_ACC_PUBLIC)
     PHP_ME(Redis, randomKey, NULL, ZEND_ACC_PUBLIC)
     PHP_ME(Redis, renameKey, NULL, ZEND_ACC_PUBLIC)
     PHP_ME(Redis, renameNx, NULL, ZEND_ACC_PUBLIC)
     PHP_ME(Redis, getMultiple, NULL, ZEND_ACC_PUBLIC)
     PHP_ME(Redis, exists, NULL, ZEND_ACC_PUBLIC)
     PHP_ME(Redis, delete, NULL, ZEND_ACC_PUBLIC)
     PHP_ME(Redis, incr, NULL, ZEND_ACC_PUBLIC)
     PHP_ME(Redis, decr, NULL, ZEND_ACC_PUBLIC)
     PHP_ME(Redis, type, NULL, ZEND_ACC_PUBLIC)
     PHP_ME(Redis, getKeys, NULL, ZEND_ACC_PUBLIC)
     PHP_ME(Redis, sort, NULL, ZEND_ACC_PUBLIC)
     PHP_ME(Redis, sortAsc, NULL, ZEND_ACC_PUBLIC)
     PHP_ME(Redis, sortAscAlpha, NULL, ZEND_ACC_PUBLIC)
     PHP_ME(Redis, sortDesc, NULL, ZEND_ACC_PUBLIC)
     PHP_ME(Redis, sortDescAlpha, NULL, ZEND_ACC_PUBLIC)
     PHP_ME(Redis, lPush, NULL, ZEND_ACC_PUBLIC)
     PHP_ME(Redis, rPush, NULL, ZEND_ACC_PUBLIC)
     PHP_ME(Redis, lPop, NULL, ZEND_ACC_PUBLIC)
     PHP_ME(Redis, rPop, NULL, ZEND_ACC_PUBLIC)
     PHP_ME(Redis, lSize, NULL, ZEND_ACC_PUBLIC)
     PHP_ME(Redis, lRemove, NULL, ZEND_ACC_PUBLIC)
     PHP_ME(Redis, listTrim, NULL, ZEND_ACC_PUBLIC)
     PHP_ME(Redis, lGet, NULL, ZEND_ACC_PUBLIC)
     PHP_ME(Redis, lGetRange, NULL, ZEND_ACC_PUBLIC)
     PHP_ME(Redis, lSet, NULL, ZEND_ACC_PUBLIC)
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
     PHP_ME(Redis, info, NULL, ZEND_ACC_PUBLIC)
     PHP_ME(Redis, select, NULL, ZEND_ACC_PUBLIC)
     PHP_ME(Redis, move, NULL, ZEND_ACC_PUBLIC)

     /* 1.1 */
     PHP_ME(Redis, mset, NULL, ZEND_ACC_PUBLIC)
     PHP_ME(Redis, rpoplpush, NULL, ZEND_ACC_PUBLIC)
     PHP_ME(Redis, zAdd, NULL, ZEND_ACC_PUBLIC)
     PHP_ME(Redis, zDelete, NULL, ZEND_ACC_PUBLIC)
     PHP_ME(Redis, zRange, NULL, ZEND_ACC_PUBLIC)
     PHP_ME(Redis, zReverseRange, NULL, ZEND_ACC_PUBLIC)
     PHP_ME(Redis, zRangeByScore, NULL, ZEND_ACC_PUBLIC)
     PHP_ME(Redis, zCount, NULL, ZEND_ACC_PUBLIC)
     PHP_ME(Redis, zDeleteRangeByScore, NULL, ZEND_ACC_PUBLIC)
     PHP_ME(Redis, zCard, NULL, ZEND_ACC_PUBLIC)
     PHP_ME(Redis, zScore, NULL, ZEND_ACC_PUBLIC)
     PHP_ME(Redis, zRank, NULL, ZEND_ACC_PUBLIC)
     PHP_ME(Redis, zRevRank, NULL, ZEND_ACC_PUBLIC)
     PHP_ME(Redis, zInter, NULL, ZEND_ACC_PUBLIC)
     PHP_ME(Redis, zUnion, NULL, ZEND_ACC_PUBLIC)
     PHP_ME(Redis, zIncrBy, NULL, ZEND_ACC_PUBLIC)
     PHP_ME(Redis, expireAt, NULL, ZEND_ACC_PUBLIC)

     /* 1.2 */
     PHP_ME(Redis, hGet, NULL, ZEND_ACC_PUBLIC)
     PHP_ME(Redis, hSet, NULL, ZEND_ACC_PUBLIC)
     PHP_ME(Redis, hDel, NULL, ZEND_ACC_PUBLIC)
     PHP_ME(Redis, hLen, NULL, ZEND_ACC_PUBLIC)
     PHP_ME(Redis, hKeys, NULL, ZEND_ACC_PUBLIC)
     PHP_ME(Redis, hVals, NULL, ZEND_ACC_PUBLIC)
     PHP_ME(Redis, hGetAll, NULL, ZEND_ACC_PUBLIC)
     PHP_ME(Redis, hExists, NULL, ZEND_ACC_PUBLIC)
     PHP_ME(Redis, hIncrBy, NULL, ZEND_ACC_PUBLIC)
     PHP_ME(Redis, hMset, NULL, ZEND_ACC_PUBLIC)

     PHP_ME(Redis, multi, NULL, ZEND_ACC_PUBLIC)
     PHP_ME(Redis, discard, NULL, ZEND_ACC_PUBLIC)
     PHP_ME(Redis, exec, NULL, ZEND_ACC_PUBLIC)
     PHP_ME(Redis, pipeline, NULL, ZEND_ACC_PUBLIC)
     PHP_ME(Redis, watch, NULL, ZEND_ACC_PUBLIC)
     PHP_ME(Redis, unwatch, NULL, ZEND_ACC_PUBLIC)

	 PHP_ME(Redis, publish, NULL, ZEND_ACC_PUBLIC)
	 PHP_ME(Redis, subscribe, NULL, ZEND_ACC_PUBLIC)
	 PHP_ME(Redis, unsubscribe, NULL, ZEND_ACC_PUBLIC)

     /* aliases */
     PHP_MALIAS(Redis, open, connect, NULL, ZEND_ACC_PUBLIC)
     PHP_MALIAS(Redis, lLen, lSize, NULL, ZEND_ACC_PUBLIC)
     PHP_MALIAS(Redis, sGetMembers, sMembers, NULL, ZEND_ACC_PUBLIC)
     PHP_MALIAS(Redis, mget, getMultiple, NULL, ZEND_ACC_PUBLIC)
     PHP_MALIAS(Redis, expire, setTimeout, NULL, ZEND_ACC_PUBLIC)
     PHP_MALIAS(Redis, zunionstore, zUnion, NULL, ZEND_ACC_PUBLIC)
     PHP_MALIAS(Redis, zinterstore, zInter, NULL, ZEND_ACC_PUBLIC)

     PHP_MALIAS(Redis, zRemove, zDelete, NULL, ZEND_ACC_PUBLIC)
     PHP_MALIAS(Redis, zRemoveRangeByScore, zDeleteRangeByScore, NULL, ZEND_ACC_PUBLIC)
     PHP_MALIAS(Redis, zSize, zCard, NULL, ZEND_ACC_PUBLIC)
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
PHPAPI int redis_sock_get(zval *id, RedisSock **redis_sock TSRMLS_DC)
{

    zval **socket;
    int resource_type;

    if (Z_TYPE_P(id) != IS_OBJECT || zend_hash_find(Z_OBJPROP_P(id), "socket",
                                  sizeof("socket"), (void **) &socket) == FAILURE) {
        return -1;
    }

    *redis_sock = (RedisSock *) zend_list_find(Z_LVAL_PP(socket), &resource_type);

    if (!*redis_sock || resource_type != le_redis_sock) {
            return -1;
    }

    return Z_LVAL_PP(socket);
}

/**
 * redis_destructor_multi_access
 */
static void redis_destructor_multi_access(zend_rsrc_list_entry * rsrc TSRMLS_DC)
{
}


/**
 * PHP_MINIT_FUNCTION
 */
PHP_MINIT_FUNCTION(redis)
{
    zend_class_entry redis_class_entry;
    INIT_CLASS_ENTRY(redis_class_entry, "Redis", redis_functions);
    redis_ce = zend_register_internal_class(&redis_class_entry TSRMLS_CC);

    zend_class_entry redis_exception_class_entry;
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

	le_redis_multi_access_type = zend_register_list_destructors_ex(
		redis_destructor_multi_access,
		NULL,
		redis_multi_access_type_name, module_number
	);

	le_redis_multi_head = zend_register_list_destructors_ex(
		redis_destructor_multi_access,
		NULL,
		redis_multi_access_type_name, module_number
	);

	le_redis_multi_current = zend_register_list_destructors_ex(
		redis_destructor_multi_access,
		NULL,
		redis_multi_access_type_name, module_number
	);

	le_redis_pipeline_head = zend_register_list_destructors_ex(
		redis_destructor_multi_access,
		NULL,
		redis_multi_access_type_name, module_number
	);

	le_redis_pipeline_current = zend_register_list_destructors_ex(
		redis_destructor_multi_access,
		NULL,
		redis_multi_access_type_name, module_number
	);

	add_constant_long(redis_ce, "REDIS_NOT_FOUND", REDIS_NOT_FOUND);
	add_constant_long(redis_ce, "REDIS_STRING", REDIS_STRING);
	add_constant_long(redis_ce, "REDIS_SET", REDIS_SET);
	add_constant_long(redis_ce, "REDIS_LIST", REDIS_LIST);

	add_constant_long(redis_ce, "ATOMIC", REDIS_ATOMIC);
	add_constant_long(redis_ce, "MULTI", REDIS_MULTI);
	add_constant_long(redis_ce, "PIPELINE", REDIS_PIPELINE);

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
    php_info_print_table_row(2, "Version", PHP_REDIS_VERSION);
    php_info_print_table_end();
}

/* {{{ proto Redis Redis::__construct()
    Public constructor */
PHP_METHOD(Redis, __construct)
{
	zval *object;

    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "") == FAILURE) {
        RETURN_FALSE;
    }
	int multi_flag = 0, id;
	object = getThis();
    id = zend_list_insert(&multi_flag,  le_redis_multi_access_type);
    add_property_resource(object, "multi_flag", id);

	set_flag(object, REDIS_ATOMIC TSRMLS_CC);
	set_multi_head(object, NULL);
	set_multi_current(object, NULL);
    set_pipeline_head(object, NULL);
    set_pipeline_current(object, NULL);

}
/* }}} */
PHPAPI int get_flag(zval *object TSRMLS_DC)
{
	zval **multi_flag;
	int flag, flag_result;

	zend_hash_find(Z_OBJPROP_P(object), "multi_flag", sizeof("multi_flag"), (void **) &multi_flag);
	flag = (int)(long)zend_list_find(Z_LVAL_PP(multi_flag), &flag_result);

	return flag;
}

PHPAPI void set_flag(zval *object, int new_flag TSRMLS_DC)
{
	zval **multi_flag = NULL;

	zend_hash_find(Z_OBJPROP_P(object), "multi_flag", sizeof("multi_flag"), (void **) &multi_flag);
	zend_list_delete(Z_LVAL_PP(multi_flag));

	int id = zend_list_insert((void *)(long)new_flag, le_redis_multi_access_type);
    add_property_resource(object, "multi_flag", id);

}

/* {{{ proto boolean Redis::connect(string host, int port [, int timeout])
 */
PHP_METHOD(Redis, connect)
{
    zval *object;
    int host_len, id;
    char *host = NULL;
    long port;

    struct timeval timeout = {0L, 0L};
    RedisSock *redis_sock  = NULL;

    if (zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), "Osl|l",
                                     &object, redis_ce, &host, &host_len, &port,
                                     &timeout.tv_sec) == FAILURE) {
       RETURN_FALSE;
    }

    if (timeout.tv_sec < 0L || timeout.tv_sec > INT_MAX) {
        zend_throw_exception(redis_exception_ce, "Invalid timeout", 0 TSRMLS_CC);
        RETURN_FALSE;
    }

    redis_sock = redis_sock_create(host, host_len, port, timeout.tv_sec);

    if (redis_sock_server_open(redis_sock, 1 TSRMLS_CC) < 0) {
        redis_free_socket(redis_sock);
        zend_throw_exception_ex(
            redis_exception_ce,
            0 TSRMLS_CC,
            "Can't connect to %s:%d",
            host,
            port
        );
        RETURN_FALSE;
    }

    id = zend_list_insert(redis_sock, le_redis_sock);
    add_property_resource(object, "socket", id);

    RETURN_TRUE;
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

    if (redis_sock_get(object, &redis_sock TSRMLS_CC) < 0) {
        RETURN_FALSE;
    }

    if (redis_sock_disconnect(redis_sock TSRMLS_CC)) {
        RETURN_TRUE;
    }

    RETURN_FALSE;
}
/* }}} */

/* {{{ proto boolean Redis::set(string key, string value)
 */
PHP_METHOD(Redis, set)
{
    zval *object;
    RedisSock *redis_sock;
    char *key = NULL, *val = NULL, *cmd;
    int key_len, val_len, cmd_len;

    if (zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), "Oss",
                                     &object, redis_ce, &key, &key_len,
                                     &val, &val_len) == FAILURE) {
        RETURN_FALSE;
    }

    if (redis_sock_get(object, &redis_sock TSRMLS_CC) < 0) {
        RETURN_FALSE;
    }

    cmd_len = redis_cmd_format(&cmd,
                    "*3" _NL
                    "$3" _NL
                    "SET" _NL

                    "$%d" _NL   /* key_len */
                    "%s" _NL    /* key */

                    "$%d" _NL   /* val_len */
                    "%s" _NL    /* val */

                    , key_len, key, key_len
                    , val_len, val, val_len);

	REDIS_PROCESS_REQUEST(redis_sock, cmd, cmd_len);
	IF_ATOMIC() {
		redis_boolean_response(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock, NULL);
	}
	REDIS_PROCESS_RESPONSE(redis_boolean_response);
}

/* {{{ proto boolean Redis::setex(string key, long expire, string value)
 */
PHP_METHOD(Redis, setex)
{
    zval *object;
    RedisSock *redis_sock;
    char *key = NULL, *val = NULL, *cmd;
    int key_len, val_len, cmd_len;
    long expire;

    if (zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), "Osls",
                                     &object, redis_ce, &key, &key_len,
                                     &expire, &val, &val_len) == FAILURE) {
        RETURN_FALSE;
    }

    if (redis_sock_get(object, &redis_sock TSRMLS_CC) < 0) {
        RETURN_FALSE;
    }

    cmd_len = redis_cmd_format(&cmd,
                    "*4" _NL
                    "$5" _NL
                    "SETEX" _NL

                    "$%d" _NL   /* key_len */
                    "%s" _NL    /* key */

                    "$%d" _NL   /* expire_len */
                    "%d" _NL    /* expire */

                    "$%d" _NL   /* val_len */
                    "%s" _NL    /* val */

                    , key_len, key, key_len
                    , integer_length(expire), expire
                    , val_len, val, val_len);

	REDIS_PROCESS_REQUEST(redis_sock, cmd, cmd_len);
	IF_ATOMIC() {
		redis_boolean_response(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock, NULL);
	}
	REDIS_PROCESS_RESPONSE(redis_boolean_response);
}

/* {{{ proto boolean Redis::setnx(string key, string value)
 */
PHP_METHOD(Redis, setnx)
{

    zval *object;
    RedisSock *redis_sock;
    char *key = NULL, *val = NULL, *cmd;
    int key_len, val_len, cmd_len;

    if (zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), "Oss",
                                     &object, redis_ce, &key, &key_len,
                                     &val, &val_len) == FAILURE) {
        RETURN_FALSE;
    }

    if (redis_sock_get(object, &redis_sock TSRMLS_CC) < 0) {
        RETURN_FALSE;
    }

    cmd_len = redis_cmd_format(&cmd,
                    "*3" _NL
                    "$5" _NL
                    "SETNX" _NL

                    "$%d" _NL   /* key_len */
                    "%s" _NL    /* key */

                    "$%d" _NL   /* val_len */
                    "%s" _NL    /* val */

                    , key_len, key, key_len
                    , val_len, val, val_len);

    REDIS_PROCESS_REQUEST(redis_sock, cmd, cmd_len);

    IF_ATOMIC() {
	  redis_1_response(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock, NULL);
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

    if (zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), "Oss",
                                     &object, redis_ce, &key, &key_len,
                                     &val, &val_len) == FAILURE) {
        RETURN_FALSE;
    }

    if (redis_sock_get(object, &redis_sock TSRMLS_CC) < 0) {
        RETURN_FALSE;
    }

    cmd_len = redis_cmd_format(&cmd,
                    "*3" _NL
                    "$6" _NL
                    "GETSET" _NL

                    "$%d" _NL   /* key_len */
                    "%s" _NL    /* key */

                    "$%d" _NL   /* val_len */
                    "%s" _NL    /* val */

                    , key_len, key, key_len
                    , val_len, val, val_len);

	REDIS_PROCESS_REQUEST(redis_sock, cmd, cmd_len);
	IF_ATOMIC() {
		redis_string_response(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock, NULL);
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

    if (redis_sock_get(object, &redis_sock TSRMLS_CC) < 0) {
        RETURN_FALSE;
    }

    cmd_len = redis_cmd_format(&cmd, "*1" _NL "$9" _NL "RANDOMKEY" _NL);

	REDIS_PROCESS_REQUEST(redis_sock, cmd, cmd_len);
	IF_ATOMIC() {
	  redis_string_response(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock, NULL);
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

    if (zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), "Oss",
                                     &object, redis_ce,
                                     &src, &src_len,
                                     &dst, &dst_len) == FAILURE) {
        RETURN_FALSE;
    }

    if (redis_sock_get(object, &redis_sock TSRMLS_CC) < 0) {
        RETURN_FALSE;
    }

    cmd_len = redis_cmd_format(&cmd,
                    "*3" _NL
                    "$6" _NL
                    "RENAME" _NL

                    "$%d" _NL   /* src_len */
                    "%s" _NL    /* src */

                    "$%d" _NL   /* dst_len */
                    "%s" _NL    /* dst */

                    , src_len, src, src_len
                    , dst_len, dst, dst_len);


	REDIS_PROCESS_REQUEST(redis_sock, cmd, cmd_len);
	IF_ATOMIC() {
		redis_boolean_response(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock, NULL);
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

    if (zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), "Oss",
                                     &object, redis_ce,
                                     &src, &src_len,
                                     &dst, &dst_len) == FAILURE) {
        RETURN_FALSE;
    }

    if (redis_sock_get(object, &redis_sock TSRMLS_CC) < 0) {
        RETURN_FALSE;
    }


    cmd_len = redis_cmd_format(&cmd,
                    "*3" _NL
                    "$8" _NL
                    "RENAMENX" _NL

                    "$%d" _NL   /* src_len */
                    "%s" _NL    /* src */

                    "$%d" _NL   /* dst_len */
                    "%s" _NL    /* dst */

                    , src_len, src, src_len
                    , dst_len, dst, dst_len);

	REDIS_PROCESS_REQUEST(redis_sock, cmd, cmd_len);
	IF_ATOMIC() {
		redis_boolean_response(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock, NULL);
	}
	REDIS_PROCESS_RESPONSE(redis_boolean_response);

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

    if (zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), "Os",
                                     &object, redis_ce,
                                     &key, &key_len) == FAILURE) {
        RETURN_FALSE;
    }

    if (redis_sock_get(object, &redis_sock TSRMLS_CC) < 0) {
        RETURN_FALSE;
    }

    cmd_len = redis_cmd_format(&cmd,
                    "*2" _NL
                    "$3" _NL
                    "GET" _NL

                    "$%d" _NL   /* src_len */
                    "%s" _NL    /* src */

                    , key_len, key, key_len);

	REDIS_PROCESS_REQUEST(redis_sock, cmd, cmd_len);
    IF_ATOMIC() {
	  redis_string_response(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock, NULL);
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
    int cmd_len;

    if (zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), "O",
                                     &object, redis_ce) == FAILURE) {
        RETURN_FALSE;
    }

    if (redis_sock_get(object, &redis_sock TSRMLS_CC) < 0) {
        RETURN_FALSE;
    }

    if (!redis_sock->stream) {
       php_error_docref(NULL TSRMLS_CC, E_ERROR, "The object is not connected");
       RETURN_FALSE;
    }

    char *cmd = estrdup("*1" _NL "$4" _NL "PING" _NL);
	cmd_len = strlen(cmd);

	REDIS_PROCESS_REQUEST(redis_sock, cmd, cmd_len);
    IF_ATOMIC() {
	  redis_string_response(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock, NULL);
    }
    REDIS_PROCESS_RESPONSE(redis_string_response);
}
/* }}} */

PHPAPI void redis_atomic_increment(INTERNAL_FUNCTION_PARAMETERS, char *keyword) {

    zval *object;
    RedisSock *redis_sock;
    char *key = NULL, *cmd;
    int key_len, cmd_len;
    long val = 1;
    int keyword_len = strlen(keyword);

    if (zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), "Os|l",
                                     &object, redis_ce,
                                     &key, &key_len, &val) == FAILURE) {
        RETURN_FALSE;
    }

    if (redis_sock_get(object, &redis_sock TSRMLS_CC) < 0) {
        RETURN_FALSE;
    }
    if (val <= 1) {
        cmd_len = redis_cmd_format(&cmd,
                        "*2" _NL
                        "$%d" _NL
                        "%s" _NL
                        "$%d" _NL
                        "%s" _NL
                            , keyword_len, keyword, keyword_len
                            , key_len, key, key_len);
    } else {
        int val_len = integer_length(val);
        cmd_len = redis_cmd_format(&cmd,
                        "*3" _NL
                        "$%d" _NL
                        "%sBY" _NL
                        "$%d" _NL
                        "%s" _NL
                        "$%d" _NL
                        "%d" _NL
                            , keyword_len+2, keyword, keyword_len
                            , key_len, key, key_len
                            , val_len, (int)val);
    }

	REDIS_PROCESS_REQUEST(redis_sock, cmd, cmd_len);
    IF_ATOMIC() {
		redis_long_response(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock, NULL);
    }
    REDIS_PROCESS_RESPONSE(redis_long_response);
}

/* {{{ proto boolean Redis::incr(string key [,int value])
 */
PHP_METHOD(Redis, incr)
{
    redis_atomic_increment(INTERNAL_FUNCTION_PARAM_PASSTHRU, "INCR");
}
/* }}} */

/* {{{ proto boolean Redis::decr(string key [,int value])
 */
PHP_METHOD(Redis, decr)
{
    redis_atomic_increment(INTERNAL_FUNCTION_PARAM_PASSTHRU, "DECR");
}
/* }}} */

/* {{{ proto array Redis::getMultiple(array keys)
 */
PHP_METHOD(Redis, getMultiple)
{
    zval *object, *array, **data;
    HashTable *arr_hash;
    HashPosition pointer;
    RedisSock *redis_sock;
    char *cmd = "", *old_cmd = NULL;
    int cmd_len, array_count;

    if (zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), "Oa",
                                     &object, redis_ce, &array) == FAILURE) {
        RETURN_FALSE;
    }

    if (redis_sock_get(object, &redis_sock TSRMLS_CC) < 0) {
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
    cmd_len = spprintf(&cmd, 0, "MGET %s\r\n", cmd);

    efree(old_cmd);

	REDIS_PROCESS_REQUEST(redis_sock, cmd, cmd_len);
    IF_ATOMIC() {
	    if (redis_sock_read_multibulk_reply(INTERNAL_FUNCTION_PARAM_PASSTHRU,
											redis_sock, NULL) < 0) {
    	    RETURN_FALSE;
	    }
    }
    REDIS_PROCESS_RESPONSE(redis_sock_read_multibulk_reply);

}
/* }}} */

/* {{{ proto boolean Redis::exists(string key)
 */
PHP_METHOD(Redis, exists)
{
    zval *object;
    RedisSock *redis_sock;
    char *key = NULL, *cmd;
    int key_len, cmd_len;

    if (zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), "Os",
                                     &object, redis_ce,
                                     &key, &key_len) == FAILURE) {
        RETURN_FALSE;
    }
    if (redis_sock_get(object, &redis_sock TSRMLS_CC) < 0) {
        RETURN_FALSE;
    }

    cmd_len = redis_cmd_format(&cmd,
                    "*2" _NL
                    "$6" _NL
                    "EXISTS" _NL

                    "$%d" _NL   /* key_len */
                    "%s" _NL    /* key */

                    , key_len, key, key_len);

    REDIS_PROCESS_REQUEST(redis_sock, cmd, cmd_len);
    IF_ATOMIC() {
	  redis_1_response(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock, NULL);
    }
    REDIS_PROCESS_RESPONSE(redis_1_response);

}
/* }}} */

/* {{{ proto boolean Redis::delete(string key)
 */
PHP_METHOD(Redis, delete)
{
    RedisSock *redis_sock;

    generic_multiple_args_cmd(INTERNAL_FUNCTION_PARAM_PASSTHRU,
                    "DEL", sizeof("DEL") - 1,
                    1, &redis_sock);
	zval * object = getThis();

    IF_ATOMIC() {
	  redis_long_response(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock, NULL);
    }
	REDIS_PROCESS_RESPONSE(redis_long_response);

}
/* }}} */

/* {{{ proto boolean Redis::watch(string key1, string key2...)
 */
PHP_METHOD(Redis, watch)
{
    RedisSock *redis_sock;

    generic_multiple_args_cmd(INTERNAL_FUNCTION_PARAM_PASSTHRU,
                    "WATCH", sizeof("WATCH") - 1,
                    1, &redis_sock);
	zval * object = getThis();

    IF_ATOMIC() {
	  redis_boolean_response(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock, NULL);
    }
	REDIS_PROCESS_RESPONSE(redis_boolean_response);

}
/* }}} */

/* {{{ proto boolean Redis::unwatch()
 */
PHP_METHOD(Redis, unwatch)
{
    char cmd[] = "*1" _NL "$7" _NL "UNWATCH" _NL;
    generic_empty_cmd(INTERNAL_FUNCTION_PARAM_PASSTHRU, estrdup(cmd), sizeof(cmd)-1);

}
/* }}} */

/* {{{ proto array Redis::getKeys(string pattern)
 */
PHP_METHOD(Redis, getKeys)
{
    zval *object;
    RedisSock *redis_sock;
    char *pattern = NULL, *cmd, *response;
    int pattern_len, cmd_len, response_len, count;
    char inbuf[1024];

    if (zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), "Os",
                                     &object, redis_ce,
                                     &pattern, &pattern_len) == FAILURE) {
        RETURN_NULL();
    }

    if (redis_sock_get(object, &redis_sock TSRMLS_CC) < 0) {
        RETURN_FALSE;
    }

    cmd_len = spprintf(&cmd, 0,
                    "*2" _NL
                    "$4" _NL
                    "KEYS" _NL
                    "$%d" _NL
                    "%s" _NL
                    , pattern_len, pattern, pattern_len);

    if (redis_sock_write(redis_sock, cmd, cmd_len) < 0) {
        efree(cmd);
        RETURN_FALSE;
    }
    efree(cmd);

    /* two cases:
     * 1 - old versions of redis (before 1st of March 2010): one space-separated line
     * 2 - newer versions of redis: multi-bulk data.
     *
     * The following code supports both.
     **/

    /* prepare for php_explode */
    array_init(return_value);

    /* read a first line. */
    redis_check_eof(redis_sock TSRMLS_CC);
    php_stream_gets(redis_sock->stream, inbuf, 1024);

    switch(inbuf[0]) { /* check what character it starts with. */
        case '$':
            response_len = atoi(inbuf + 1);
            response = redis_sock_read_bulk_reply(redis_sock, response_len);
            if(!response_len) { /* empty array in case of an empty string */
                efree(response);
                return;
            }
            zval *delimiter; /* delimiter */
            MAKE_STD_ZVAL(delimiter);
            ZVAL_STRING(delimiter, " ", 1);

            zval *keys; /* keys */
            MAKE_STD_ZVAL(keys);
            ZVAL_STRING(keys, response, 1);
            php_explode(delimiter, keys, return_value, -1);

            /* free memory */
            zval_dtor(keys);
            efree(keys);
            zval_dtor(delimiter);
            efree(delimiter);
            break;

        case '*':
            count = atoi(inbuf + 1);
            redis_sock_read_multibulk_reply_loop(INTERNAL_FUNCTION_PARAM_PASSTHRU,
                            redis_sock, return_value, count);
            break;
    }
}
/* }}} */

/* {{{ proto int Redis::type(string key)
 */
PHP_METHOD(Redis, type)
{
    zval *object;
    RedisSock *redis_sock;
    char *key = NULL, *cmd, *response;
    int key_len, cmd_len, response_len;

    if (zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), "Os",
                                     &object, redis_ce,
                                     &key, &key_len) == FAILURE) {
        RETURN_NULL();
    }

    if (redis_sock_get(object, &redis_sock TSRMLS_CC) < 0) {
        RETURN_FALSE;
    }

    cmd_len = redis_cmd_format(&cmd,
                    "*2" _NL
                    "$4" _NL
                    "TYPE" _NL
                    "$%d" _NL
                    "%s" _NL
                    , key_len, key, key_len);

	IF_MULTI_OR_ATOMIC() {
		SOCKET_WRITE_COMMAND(redis_sock, cmd, cmd_len);
	}
	IF_PIPELINE() {
		PIPELINE_ENQUEUE_COMMAND(cmd, cmd_len);
	}
    efree(cmd);

	MULTI_RESPONSE(redis_long_response);
	IF_ATOMIC() {
	    if ((response = redis_sock_read(redis_sock, &response_len TSRMLS_CC)) == NULL) {
    	    RETURN_FALSE;
	    }

    	long l;
	    if (strncmp(response, "+string", 7) == 0) {
    	   l = REDIS_STRING;
	    } else if (strncmp(response, "+set", 4) == 0){
    	   l = REDIS_SET;
	    } else if (strncmp(response, "+list", 5) == 0){
    	   l = REDIS_LIST;
	    } else {
    	   l = REDIS_NOT_FOUND;
	    }
    	efree(response);
	    RETURN_LONG(l);
	}
	ELSE_IF_MULTI()
	ELSE_IF_PIPELINE();


}
/* }}} */


PHPAPI void
generic_push_function(INTERNAL_FUNCTION_PARAMETERS, char *keyword, int keyword_len) {
    zval *object;
    RedisSock *redis_sock;
    char *cmd, *key, *val;
    int cmd_len, key_len, val_len;

    if (zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), "Oss",
                                     &object, redis_ce,
                                     &key, &key_len, &val, &val_len) == FAILURE) {
        RETURN_NULL();
    }

    if (redis_sock_get(object, &redis_sock TSRMLS_CC) < 0) {
        RETURN_FALSE;
    }

    cmd_len = redis_cmd_format(&cmd,
                    "*3" _NL
                    "$%d" _NL /* keyword_len */
                    "%s" _NL  /* keyword */

                    "$%d" _NL /* key_len */
                    "%s" _NL  /* key */

                    "$%d" _NL /* val_len */
                    "%s" _NL  /* val */

                    , keyword_len, keyword, keyword_len
                    , key_len, key, key_len
                    , val_len, val, val_len);

	REDIS_PROCESS_REQUEST(redis_sock, cmd, cmd_len);
	IF_ATOMIC() {
		redis_long_response(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock, NULL);
	}
	REDIS_PROCESS_RESPONSE(redis_long_response);
}

/* {{{ proto boolean Redis::lPush(string key , string value)
 */
PHP_METHOD(Redis, lPush)
{
    generic_push_function(INTERNAL_FUNCTION_PARAM_PASSTHRU, "LPUSH", sizeof("LPUSH")-1);
}
/* }}} */

/* {{{ proto boolean Redis::rPush(string key , string value)
 */
PHP_METHOD(Redis, rPush)
{
    generic_push_function(INTERNAL_FUNCTION_PARAM_PASSTHRU, "RPUSH", sizeof("RPUSH")-1);
}
/* }}} */

PHPAPI void
generic_pop_function(INTERNAL_FUNCTION_PARAMETERS, char *keyword, int keyword_len) {

    zval *object;
    RedisSock *redis_sock;
    char *key = NULL, *cmd;
    int key_len, cmd_len;

    if (zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), "Os",
                                     &object, redis_ce,
                                     &key, &key_len) == FAILURE) {
        RETURN_FALSE;
    }

    if (redis_sock_get(object, &redis_sock TSRMLS_CC) < 0) {
        RETURN_FALSE;
    }

    cmd_len = redis_cmd_format(&cmd,
                    "*2" _NL
                    "$%d" _NL /* keyword_len */
                    "%s" _NL /* keyword */

                    "$%d" _NL /* key_len */
                    "%s" _NL /* key */

                    , keyword_len, keyword, keyword_len
                    , key_len, key, key_len);

	REDIS_PROCESS_REQUEST(redis_sock, cmd, cmd_len);
	IF_ATOMIC() {
		redis_string_response(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock, NULL);
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

/* {{{ proto int Redis::lSize(string key)
 */
PHP_METHOD(Redis, lSize)
{
    zval *object;
    RedisSock *redis_sock;
    char *key = NULL, *cmd;
    int key_len, cmd_len;

    if (zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), "Os",
                                     &object, redis_ce,
                                     &key, &key_len) == FAILURE) {
        RETURN_FALSE;
    }

    if (redis_sock_get(object, &redis_sock TSRMLS_CC) < 0) {
        RETURN_FALSE;
    }

    cmd_len = redis_cmd_format(&cmd,
                    "*2" _NL
                    "$4" _NL
                    "LLEN" _NL

                    "$%d" _NL /* key_len */
                    "%s" _NL  /* key */

                    , key_len, key, key_len);

	REDIS_PROCESS_REQUEST(redis_sock, cmd, cmd_len);
	IF_ATOMIC() {
		redis_long_response(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock, NULL);
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

    if (zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), "Oss|l",
                                     &object, redis_ce,
                                     &key, &key_len, &val, &val_len, &count) == FAILURE) {
        RETURN_NULL();
    }

    if (redis_sock_get(object, &redis_sock TSRMLS_CC) < 0) {
        RETURN_FALSE;
    }

    /* LREM key count value */
    cmd_len = redis_cmd_format(&cmd,
                    "*4" _NL
                    "$4" _NL
                    "LREM" _NL

                    "$%d" _NL /* key_len */
                    "%s" _NL  /* key */

                    "$%d" _NL /* count_len */
                    "%d" _NL  /* count */

                    "$%d" _NL /* val_len */
                    "%s" _NL  /* val */

                    , key_len, key, key_len
                    , integer_length(count), count
                    , val_len, val, val_len);

	REDIS_PROCESS_REQUEST(redis_sock, cmd, cmd_len);
	IF_ATOMIC() {
		redis_long_response(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock, NULL);
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
    char *key = NULL, *val = NULL, *cmd;
    int key_len, cmd_len;
    long start, end;

    if (zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), "Osll",
                                     &object, redis_ce, &key, &key_len,
                                     &start, &end) == FAILURE) {
        RETURN_FALSE;
    }

    if (redis_sock_get(object, &redis_sock TSRMLS_CC) < 0) {
        RETURN_FALSE;
    }

    cmd_len = spprintf(&cmd, 0, "LTRIM %s %d %d\r\n", key, (int)start, (int)end);

	REDIS_PROCESS_REQUEST(redis_sock, cmd, cmd_len);
	IF_ATOMIC() {
		redis_boolean_response(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock, NULL);
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
    char *key = NULL, *cmd, *response;
    int key_len,cmd_len, response_len;
    long index;

    if (zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), "Osl",
                                     &object, redis_ce,
                                     &key, &key_len, &index) == FAILURE) {
        RETURN_NULL();
    }

    if (redis_sock_get(object, &redis_sock TSRMLS_CC) < 0) {
        RETURN_FALSE;
    }

    /* LINDEX key pos */
    cmd_len = redis_cmd_format(&cmd,
                    "*3" _NL
                    "$6" _NL
                    "LINDEX" _NL

                    "$%d" _NL /* key_len */
                    "%s" _NL  /* key */

                    "$%d" _NL /* index_len */
                    "%d" _NL  /* index */

                    , key_len, key, key_len
                    , integer_length(index), index);

	REDIS_PROCESS_REQUEST(redis_sock, cmd, cmd_len);
	IF_ATOMIC() {
		redis_string_response(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock, NULL);
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
    int key_len, cmd_len, response_len;
    long start, end;

    if (zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), "Osll",
                                     &object, redis_ce,
                                     &key, &key_len, &start, &end) == FAILURE) {
        RETURN_FALSE;
    }

    if (redis_sock_get(object, &redis_sock TSRMLS_CC) < 0) {
        RETURN_FALSE;
    }

    /* LRANGE key start end */
    cmd_len = redis_cmd_format(&cmd,
                    "*4" _NL
                    "$6" _NL
                    "LRANGE" _NL

                    "$%d" _NL /* key_len */
                    "%s" _NL  /* key */

                    "$%d" _NL /* start_len */
                    "%d" _NL  /* start */

                    "$%d" _NL /* end_len */
                    "%d" _NL  /* end */

                    , key_len, key, key_len
                    , integer_length(start), start
                    , integer_length(end), end);

	REDIS_PROCESS_REQUEST(redis_sock, cmd, cmd_len);
	IF_ATOMIC() {
		redis_sock_read_multibulk_reply(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock, NULL);
	}
	REDIS_PROCESS_RESPONSE(redis_sock_read_multibulk_reply);

}
/* }}} */

/* {{{ proto boolean Redis::sAdd(string key , string value)
 */
PHP_METHOD(Redis, sAdd)
{
    zval *object;
    RedisSock *redis_sock;
    char *key = NULL, *val = NULL, *cmd;
    int key_len, val_len, cmd_len;

    if (zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), "Oss",
                                     &object, redis_ce, &key, &key_len,
                                     &val, &val_len) == FAILURE) {
        RETURN_NULL();
    }

    if (redis_sock_get(object, &redis_sock TSRMLS_CC) < 0) {
        RETURN_FALSE;
    }

    cmd_len = redis_cmd_format(&cmd,
                    "*3" _NL
                    "$4" _NL
                    "SADD" _NL

                    "$%d" _NL   /* key_len */
                    "%s" _NL    /* key */
                    "$%d" _NL   /* val_len */
                    "%s" _NL    /* val */

                    , key_len, key, key_len
                    , val_len, val, val_len);

	REDIS_PROCESS_REQUEST(redis_sock, cmd, cmd_len);
	IF_ATOMIC() {
		redis_1_response(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock, NULL);
	}
	REDIS_PROCESS_RESPONSE(redis_1_response);
}
/* }}} */

/* {{{ proto int Redis::sSize(string key)
 */
PHP_METHOD(Redis, sSize)
{
    zval *object;
    RedisSock *redis_sock;
    char *key = NULL, *cmd;
    int key_len, cmd_len, response_len;

    if (zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), "Os",
                                     &object, redis_ce,
                                     &key, &key_len) == FAILURE) {
        RETURN_FALSE;
    }

    if (redis_sock_get(object, &redis_sock TSRMLS_CC) < 0) {
        RETURN_FALSE;
    }

    cmd_len = redis_cmd_format(&cmd,
                    "*2" _NL
                    "$5" _NL
                    "SCARD" _NL

                    "$%d" _NL   /* key_len */
                    "%s" _NL    /* key */

                    , key_len, key, key_len);

	REDIS_PROCESS_REQUEST(redis_sock, cmd, cmd_len);
	IF_ATOMIC() {
		redis_long_response(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock, NULL);
	}
	REDIS_PROCESS_RESPONSE(redis_long_response);
}
/* }}} */

/* {{{ proto boolean Redis::sRemove(string set, string value)
 */
PHP_METHOD(Redis, sRemove)
{
    zval *object;
    RedisSock *redis_sock;
    char *key = NULL, *val = NULL, *cmd;
    int key_len, val_len, cmd_len;

    if (zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), "Oss",
                                     &object, redis_ce,
                                     &key, &key_len, &val, &val_len) == FAILURE) {
        RETURN_FALSE;
    }

    if (redis_sock_get(object, &redis_sock TSRMLS_CC) < 0) {
        RETURN_FALSE;
    }

    cmd_len = redis_cmd_format(&cmd,
                    "*3" _NL
                    "$4" _NL
                    "SREM" _NL

                    "$%d" _NL   /* key_len */
                    "%s" _NL    /* key */

                    "$%d" _NL   /* val_len */
                    "%s" _NL    /* val */

                    , key_len, key, key_len
                    , val_len, val, val_len);

	REDIS_PROCESS_REQUEST(redis_sock, cmd, cmd_len);
	IF_ATOMIC() {
		redis_1_response(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock, NULL);
	}
	REDIS_PROCESS_RESPONSE(redis_1_response);
}
/* }}} */
/* {{{ proto boolean Redis::sMove(string set_src, string set_dst, string value)
 */
PHP_METHOD(Redis, sMove)
{
    zval *object;
    RedisSock *redis_sock;
    char *src = NULL, *dst = NULL, *val = NULL, *cmd;
    int src_len, dst_len, val_len, cmd_len;

    if (zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), "Osss",
                                     &object, redis_ce,
                                     &src, &src_len,
                                     &dst, &dst_len,
                                     &val, &val_len) == FAILURE) {
        RETURN_FALSE;
    }

    if (redis_sock_get(object, &redis_sock TSRMLS_CC) < 0) {
        RETURN_FALSE;
    }

    cmd_len = redis_cmd_format(&cmd,
                    "*4" _NL
                    "$5" _NL
                    "SMOVE" _NL

                    "$%d" _NL   /* src_len */
                    "%s" _NL    /* src */

                    "$%d" _NL   /* dst_len */
                    "%s" _NL    /* dst */

                    "$%d" _NL   /* val_len */
                    "%s" _NL    /* val */

                    , src_len, src, src_len
                    , dst_len, dst, dst_len
                    , val_len, val, val_len);

	REDIS_PROCESS_REQUEST(redis_sock, cmd, cmd_len);
	IF_ATOMIC() {
	  redis_1_response(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock, NULL);
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
/* {{{ proto string Redis::sRandMember(string key)
 */
PHP_METHOD(Redis, sRandMember)
{
    generic_pop_function(INTERNAL_FUNCTION_PARAM_PASSTHRU, "SRANDMEMBER", 11);
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

    if (zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), "Oss",
                                     &object, redis_ce,
                                     &key, &key_len, &val, &val_len) == FAILURE) {
        return;
    }

    if (redis_sock_get(object, &redis_sock TSRMLS_CC) < 0) {
        RETURN_FALSE;
    }

    cmd_len = redis_cmd_format(&cmd,
                               "*3" _NL
                               "$9" _NL
                               "SISMEMBER" _NL

                               "$%d" _NL /* key_len */
                               "%s" _NL  /* key */

                               "$%d" _NL /* val_len */
                               "%s" _NL  /* val */

                               , key_len, key, key_len
                               , val_len, val, val_len);

	REDIS_PROCESS_REQUEST(redis_sock, cmd, cmd_len);
	IF_ATOMIC() {
		redis_1_response(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock, NULL);
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
    int key_len, cmd_len, response_len;

    if (zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), "Os",
                                     &object, redis_ce,
                                     &key, &key_len) == FAILURE) {
        RETURN_FALSE;
    }

    if (redis_sock_get(object, &redis_sock TSRMLS_CC) < 0) {
        RETURN_FALSE;
    }

    cmd_len = redis_cmd_format(&cmd,
                               "*2" _NL
                               "$8" _NL
                               "SMEMBERS" _NL

                               "$%d" _NL /* key_len */
                               "%s" _NL  /* key */

                               , key_len, key, key_len);

	REDIS_PROCESS_REQUEST(redis_sock, cmd, cmd_len);
	IF_ATOMIC() {
	    if (redis_sock_read_multibulk_reply(INTERNAL_FUNCTION_PARAM_PASSTHRU,
    	                                    redis_sock, NULL) < 0) {
        	RETURN_FALSE;
	    }
	}
    REDIS_PROCESS_RESPONSE(redis_sock_read_multibulk_reply);
}
/* }}} */


PHPAPI int generic_multiple_args_cmd(INTERNAL_FUNCTION_PARAMETERS, char *keyword, int keyword_len,
                int min_argc, RedisSock **out_sock)
{
    zval *object, **z_args, *z_array;
    char **keys, *cmd;
    int cmd_len, *keys_len;
    int i, argc = ZEND_NUM_ARGS();
    int single_array = 0;
    RedisSock *redis_sock;

    if(argc < min_argc) {
        WRONG_PARAM_COUNT;
        RETURN_FALSE;
    }

    z_args = emalloc(argc * sizeof(zval*));
    if(zend_get_parameters_array(ht, argc, z_args) == FAILURE) {
        efree(z_args);
        RETURN_FALSE;
    }

    /* case of a single array */
    if(argc == 1 && Z_TYPE_P(z_args[0]) == IS_ARRAY) {
        single_array = 1;
        z_array = z_args[0];
        efree(z_args);
        z_args = NULL;

        /* new count */
        argc = zend_hash_num_elements(Z_ARRVAL_P(z_array));
    }

    /* prepare an array for the keys, and one for their lengths */
    keys = emalloc(argc * sizeof(char*));
    keys_len = emalloc(argc * sizeof(int));

    cmd_len = keyword_len; /* start computing the command length */

    if(single_array) { /* loop over the array */
        HashTable *keytable = Z_ARRVAL_P(z_array);

        for(i = 0, zend_hash_internal_pointer_reset(keytable);
            zend_hash_has_more_elements(keytable) == SUCCESS;
            zend_hash_move_forward(keytable), i++) {

            char *key;
            int key_len;
            unsigned long idx;
            int type;
            zval **z_value_pp;

            type = zend_hash_get_current_key_ex(keytable, &key, &key_len, &idx, 0, NULL);
            if(zend_hash_get_current_data(keytable, (void**)&z_value_pp) == FAILURE) {
                continue; 	/* this should never happen, according to the PHP people. */
            }

            /* get current value */
            keys[i] = Z_STRVAL_PP(z_value_pp);
            keys_len[i] = Z_STRLEN_PP(z_value_pp);
            cmd_len += keys_len[i] + 1; /* +1 for the preceding space. */
        }
    } else {
        for(i = 0; i < argc; ++i) { /* store each key */
            keys[i] = Z_STRVAL_P(z_args[i]);
            keys_len[i] = Z_STRLEN_P(z_args[i]);
            cmd_len += keys_len[i] + 1; /* +1 for the preceding space. */
        }
    }

    /* get redis socket */
    if (redis_sock_get(getThis(), out_sock TSRMLS_CC) < 0) {
        efree(keys);
        efree(keys_len);
        if(z_args) efree(z_args);
        RETURN_FALSE;
    }
    redis_sock = *out_sock;

    cmd_len += sizeof("\r\n") - 1;
    cmd = emalloc(cmd_len+1);

    memcpy(cmd, keyword, keyword_len);
    int pos = keyword_len;
    /* copy each key to its destination */
    for(i = 0; i < argc; ++i) {
        cmd[pos] = ' ';
        memcpy(cmd + pos + 1, keys[i], keys_len[i]);
        pos += 1+keys_len[i];
    }
    /* add the final new line. */
    memcpy(cmd + pos, "\r\n", 2);
    cmd[cmd_len] = '\0'; /* just in case we want to print it... */
    efree(keys);
    efree(keys_len);
    if(z_args) efree(z_args);

	object = getThis();
    REDIS_PROCESS_REQUEST(redis_sock, cmd, cmd_len);

}

/* {{{ proto array Redis::sInter(string key0, ... string keyN)
 */
PHP_METHOD(Redis, sInter) {

    RedisSock *redis_sock;

    generic_multiple_args_cmd(INTERNAL_FUNCTION_PARAM_PASSTHRU,
                    "SINTER", sizeof("SINTER") - 1,
                    0, &redis_sock);

	zval *object = getThis();

    IF_ATOMIC() {
    	if (redis_sock_read_multibulk_reply(INTERNAL_FUNCTION_PARAM_PASSTHRU,
											redis_sock, NULL) < 0) {
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

    generic_multiple_args_cmd(INTERNAL_FUNCTION_PARAM_PASSTHRU,
                    "SINTERSTORE", sizeof("SINTERSTORE") - 1,
                    1, &redis_sock);

	zval *object = getThis();

	IF_ATOMIC() {
		redis_long_response(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock, NULL);
	}
    REDIS_PROCESS_RESPONSE(redis_long_response);


}
/* }}} */

/* {{{ proto array Redis::sUnion(string key0, ... string keyN)
 */
PHP_METHOD(Redis, sUnion) {

    RedisSock *redis_sock;

    generic_multiple_args_cmd(INTERNAL_FUNCTION_PARAM_PASSTHRU,
                    "SUNION", sizeof("SUNION") - 1,
                    0, &redis_sock);
	zval *object = getThis();

	IF_ATOMIC() {
    	if (redis_sock_read_multibulk_reply(INTERNAL_FUNCTION_PARAM_PASSTHRU,
        	                                redis_sock, NULL) < 0) {
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

    generic_multiple_args_cmd(INTERNAL_FUNCTION_PARAM_PASSTHRU,
                    "SUNIONSTORE", sizeof("SUNIONSTORE") - 1,
                    1, &redis_sock);
	zval *object = getThis();

	IF_ATOMIC() {
		redis_long_response(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock, NULL);
	}
	REDIS_PROCESS_RESPONSE(redis_long_response);
}

/* }}} */

/* {{{ proto array Redis::sDiff(string key0, ... string keyN)
 */
PHP_METHOD(Redis, sDiff) {

    RedisSock *redis_sock;

    generic_multiple_args_cmd(INTERNAL_FUNCTION_PARAM_PASSTHRU,
                    "SDIFF", sizeof("SDIFF") - 1,
                    0, &redis_sock);

	zval *object = getThis();

	IF_ATOMIC() {
	    /* read multibulk reply */
    	if (redis_sock_read_multibulk_reply(INTERNAL_FUNCTION_PARAM_PASSTHRU,
											redis_sock, NULL) < 0) {
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

    generic_multiple_args_cmd(INTERNAL_FUNCTION_PARAM_PASSTHRU,
                    "SDIFFSTORE", sizeof("SDIFFSTORE") - 1,
                    1, &redis_sock);
	zval *object = getThis();

	IF_ATOMIC() {
	  redis_long_response(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock, NULL);
	}
	REDIS_PROCESS_RESPONSE(redis_long_response);
}
/* }}} */

PHP_METHOD(Redis, sort) {

    zval *object = getThis(), *z_array = NULL, **z_cur;
    char *cmd, *old_cmd = NULL, *key;
    int cmd_len, elements = 2, key_len;
    int i, argc = ZEND_NUM_ARGS();
    int using_store = 0;
    RedisSock *redis_sock;

    if (zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), "Os|a",
                                     &object, redis_ce,
                                     &key, &key_len, &z_array) == FAILURE) {
        RETURN_FALSE;
    }

    if (redis_sock_get(object, &redis_sock TSRMLS_CC) < 0) {
        RETURN_FALSE;
    }

    cmd_len = redis_cmd_format(&cmd, "$4" _NL "SORT" _NL "$%d" _NL "%s" _NL
                            , key_len, key, key_len);

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
                        && Z_TYPE_PP(z_cur) == IS_STRING) {

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
                  SUCCESS == zend_hash_index_find(Z_ARRVAL_PP(z_cur), 1, (void**)&z_count_pp) &&
                  Z_TYPE_PP(z_offset_pp) == IS_LONG &&
                  Z_TYPE_PP(z_count_pp) == IS_LONG) {

                    long limit_low = Z_LVAL_PP(z_offset_pp);
                    long limit_high = Z_LVAL_PP(z_count_pp);

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

    /* complete with prefix */
    old_cmd = cmd;
    cmd_len = redis_cmd_format(&cmd, "*%d" _NL "%s",
                    elements, cmd, cmd_len);
    efree(old_cmd);

    /* run command */
    REDIS_PROCESS_REQUEST(redis_sock, cmd, cmd_len);
    if(using_store) {
        IF_ATOMIC() {
            redis_long_response(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock, NULL);
        }
        REDIS_PROCESS_RESPONSE(redis_long_response);
    } else {
        IF_ATOMIC() {
            if (redis_sock_read_multibulk_reply(INTERNAL_FUNCTION_PARAM_PASSTHRU,
                                                redis_sock, NULL) < 0) {
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
    int key_len, pattern_len = -1, get_len = -1, store_len = -1, cmd_len;
    long sort_start = -1, sort_count = -1;

    int cmd_elements;

    long use_pound = 0;


    char *cmd_lines[30];
    int cmd_sizes[30];

    if (zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), "Os|sslls",
                                     &object, redis_ce,
                                     &key, &key_len, &pattern, &pattern_len,
                                     &get, &get_len, &sort_start, &sort_count, &store, &store_len) == FAILURE) {
        RETURN_FALSE;
    }

    if (redis_sock_get(object, &redis_sock TSRMLS_CC) < 0) {
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

    /* second line, key */
    cmd_sizes[3] = redis_cmd_format(&cmd_lines[3], "$%d", key_len);
    cmd_lines[4] = emalloc(key_len + 1);
    memcpy(cmd_lines[4], key, key_len);
    cmd_lines[4][key_len] = 0;
    cmd_sizes[4] = key_len;

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
    int sort_len = strlen(sort);
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
    int i;
    for(i = 0; i < cmd_elements; ++i) {
            cmd_len += cmd_sizes[i] + sizeof(_NL) - 1; /* each line followeb by _NL */
    }

    /* copy all lines into the final command. */
    cmd = emalloc(1 + cmd_len);
    int pos = 0;
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
											redis_sock, NULL) < 0) {
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
    char *key = NULL, *cmd;
    int key_len, cmd_len;
    long t;

    if (zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), "Osl",
                                     &object, redis_ce, &key, &key_len,
                                     &t) == FAILURE) {
        RETURN_FALSE;
    }

    if (redis_sock_get(object, &redis_sock TSRMLS_CC) < 0) {
        RETURN_FALSE;
    }

    cmd_len = redis_cmd_format(&cmd,
                    "*3" _NL
                    "$%d" _NL /* keyword_len */
                    "%s" _NL  /* keyword */

                    "$%d" _NL /* key_len */
                    "%s" _NL  /* key */

                    "$%d" _NL /* time_len */
                    "%d" _NL  /* time */

                    , keyword_len, keyword, keyword_len
                    , key_len, key, key_len
                    , integer_length(t), t);

	REDIS_PROCESS_REQUEST(redis_sock, cmd, cmd_len);
	IF_ATOMIC() {
		redis_1_response(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock, NULL);
	}
	REDIS_PROCESS_RESPONSE(redis_1_response);
}

/* {{{ proto array Redis::setTimeout(string key, int timeout)
 */
PHP_METHOD(Redis, setTimeout) {
    generic_expire_cmd(INTERNAL_FUNCTION_PARAM_PASSTHRU, "EXPIRE", sizeof("EXPIRE")-1);
}
/* }}} */

/* {{{ proto array Redis::expireAt(string key, int timestamp)
 */
PHP_METHOD(Redis, expireAt) {
    generic_expire_cmd(INTERNAL_FUNCTION_PARAM_PASSTHRU, "EXPIREAT", sizeof("EXPIREAT")-1);
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

    if (zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), "Osls",
                                     &object, redis_ce, &key, &key_len, &index, &val, &val_len) == FAILURE) {
        RETURN_FALSE;
    }

    if (redis_sock_get(object, &redis_sock TSRMLS_CC) < 0) {
        RETURN_FALSE;
    }

    cmd_len = redis_cmd_format(&cmd,
                    "*4" _NL
                    "$4" _NL
                    "LSET" _NL

                    "$%d" _NL /* key_len */
                    "%s" _NL  /* key */

                    "$%d" _NL /* index_len */
                    "%d" _NL  /* index */

                    "$%d" _NL /* val_len */
                    "%s" _NL  /* val */

                    , key_len, key, key_len
                    , integer_length(index), index
                    , val_len, val, val_len);

	REDIS_PROCESS_REQUEST(redis_sock, cmd, cmd_len);
	IF_ATOMIC() {
		redis_boolean_response(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock, NULL);
	}
	REDIS_PROCESS_RESPONSE(redis_boolean_response);
}
/* }}} */


PHPAPI void generic_empty_cmd(INTERNAL_FUNCTION_PARAMETERS, char *cmd, int cmd_len, ...) {
    zval *object;
    RedisSock *redis_sock;
    char ret;

    if (zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), "O",
                                     &object, redis_ce) == FAILURE) {
        RETURN_FALSE;
    }

    if (redis_sock_get(object, &redis_sock TSRMLS_CC) < 0) {
        RETURN_FALSE;
    }

	REDIS_PROCESS_REQUEST(redis_sock, cmd, cmd_len);
    IF_ATOMIC() {
	  redis_boolean_response(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock, NULL);
    }
    REDIS_PROCESS_RESPONSE(redis_boolean_response);
}

/* {{{ proto string Redis::save()
 */
PHP_METHOD(Redis, save)
{
    char cmd[] = "*1" _NL "$4" _NL "SAVE" _NL;
    generic_empty_cmd(INTERNAL_FUNCTION_PARAM_PASSTHRU, estrdup(cmd), sizeof(cmd)-1);

}
/* }}} */

/* {{{ proto string Redis::bgSave()
 */
PHP_METHOD(Redis, bgSave)
{
    char cmd[] = "*1" _NL "$6" _NL "BGSAVE" _NL;
    generic_empty_cmd(INTERNAL_FUNCTION_PARAM_PASSTHRU, estrdup(cmd), sizeof(cmd)-1);

}
/* }}} */

PHPAPI void generic_empty_long_cmd(INTERNAL_FUNCTION_PARAMETERS, char *cmd, int cmd_len, ...) {

    zval *object;
    RedisSock *redis_sock;

    if (zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), "O",
                                     &object, redis_ce) == FAILURE) {
        RETURN_FALSE;
    }

    if (redis_sock_get(object, &redis_sock TSRMLS_CC) < 0) {
        RETURN_FALSE;
    }

	REDIS_PROCESS_REQUEST(redis_sock, cmd, cmd_len);
    IF_ATOMIC() {
	  redis_long_response(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock, NULL);
    }
    REDIS_PROCESS_RESPONSE(redis_long_response);
}

/* {{{ proto integer Redis::lastSave()
 */
PHP_METHOD(Redis, lastSave)
{
    char cmd[] = "*1" _NL "$8" _NL "LASTSAVE" _NL;
    generic_empty_long_cmd(INTERNAL_FUNCTION_PARAM_PASSTHRU, estrdup(cmd), sizeof(cmd)-1);
}
/* }}} */


/* {{{ proto bool Redis::flushDB()
 */
PHP_METHOD(Redis, flushDB)
{
    char cmd[] = "*1" _NL "$7" _NL "FLUSHDB" _NL;
    generic_empty_cmd(INTERNAL_FUNCTION_PARAM_PASSTHRU, estrdup(cmd), sizeof(cmd)-1);
}
/* }}} */

/* {{{ proto bool Redis::flushAll()
 */
PHP_METHOD(Redis, flushAll)
{
    char cmd[] = "*1" _NL "$8" _NL "FLUSHALL" _NL;
    generic_empty_cmd(INTERNAL_FUNCTION_PARAM_PASSTHRU, estrdup(cmd), sizeof(cmd)-1);
}
/* }}} */

/* {{{ proto int Redis::dbSize()
 */
PHP_METHOD(Redis, dbSize)
{
    char cmd[] = "*1" _NL "$6" _NL "DBSIZE" _NL;
    generic_empty_long_cmd(INTERNAL_FUNCTION_PARAM_PASSTHRU, estrdup(cmd), sizeof(cmd)-1);
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

    if (redis_sock_get(object, &redis_sock TSRMLS_CC) < 0) {
        RETURN_FALSE;
    }

    cmd_len = redis_cmd_format(&cmd,
                               "*2" _NL
                               "$4" _NL
                               "AUTH" _NL

                               "$%d" _NL /* password_len */
                               "%s" _NL  /* password */

                               , password_len, password, password_len);

	REDIS_PROCESS_REQUEST(redis_sock, cmd, cmd_len);
	IF_ATOMIC() {
		redis_boolean_response(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock, NULL);
	}
	REDIS_PROCESS_RESPONSE(redis_boolean_response);
}
/* }}} */

/* {{{ proto long Redis::ttl(string key)
 */
PHP_METHOD(Redis, ttl) {

    zval *object;
    RedisSock *redis_sock;

    char *cmd, *key;
    int cmd_len, key_len;

    if (zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), "Os",
                                     &object, redis_ce, &key, &key_len) == FAILURE) {
        RETURN_FALSE;
    }

    if (redis_sock_get(object, &redis_sock TSRMLS_CC) < 0) {
        RETURN_FALSE;
    }

    cmd_len = redis_cmd_format(&cmd,
                               "*2" _NL
                               "$3" _NL
                               "TTL" _NL

                               "$%d" _NL /* key_len */
                               "%s" _NL  /* key */

                               , key_len, key, key_len);

	REDIS_PROCESS_REQUEST(redis_sock, cmd, cmd_len);
	IF_ATOMIC() {
	  redis_long_response(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock, NULL);
	}
	REDIS_PROCESS_RESPONSE(redis_long_response);
}
/* }}} */

/* {{{ proto array Redis::info()
 */
PHP_METHOD(Redis, info) {

    zval *object;
    RedisSock *redis_sock;

    char cmd[] = "*1" _NL "$4" _NL "INFO" _NL, *response, *key;
    int cmd_len = sizeof(cmd)-1, response_len;
    long ttl;
    char *cur, *pos, *value;
    int is_numeric;
    char *p;

    if (zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), "O",
                                     &object, redis_ce) == FAILURE) {
        RETURN_FALSE;
    }

    if (redis_sock_get(object, &redis_sock TSRMLS_CC) < 0) {
        RETURN_FALSE;
    }

    if (redis_sock_write(redis_sock, cmd, cmd_len) < 0) {
        /* no efree(cmd) here, it's on the stack. */
        RETURN_FALSE;
    }

    if ((response = redis_sock_read(redis_sock, &response_len TSRMLS_CC)) == NULL) {
        RETURN_FALSE;
    }

    array_init(return_value);
    /* response :: [response_line]
     * response_line :: key ':' value CRLF
     */

    cur = response;
    while(1) {
        /* key */
        pos = strchr(cur, ':');
        if(pos == NULL) {
            break;
        }
        key = emalloc(pos - cur + 1);
        memcpy(key, cur, pos-cur);
        key[pos-cur] = 0;

        /* value */
        cur = pos + 1;
        pos = strchr(cur, '\r');
        if(pos == NULL) {
            break;
        }
        value = emalloc(pos - cur + 1);
        memcpy(value, cur, pos-cur);
        value[pos-cur] = 0;
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
            add_assoc_long(return_value, key, atol(value));
            efree(value);
        } else {
            add_assoc_string(return_value, key, value, 0);
        }
        efree(key);
    }
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

    if (redis_sock_get(object, &redis_sock TSRMLS_CC) < 0) {
        RETURN_FALSE;
    }

    cmd_len = redis_cmd_format(&cmd,
                    "*2" _NL
                    "$6" _NL
                    "SELECT" _NL
                    "$%d" _NL
                    "%d" _NL
                    , integer_length(dbNumber), dbNumber);

	REDIS_PROCESS_REQUEST(redis_sock, cmd, cmd_len);
	IF_ATOMIC() {
		redis_boolean_response(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock, NULL);
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
    int cmd_len, key_len;
    long dbNumber;

    if (zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), "Osl",
                                     &object, redis_ce, &key, &key_len, &dbNumber) == FAILURE) {
        RETURN_FALSE;
    }

    if (redis_sock_get(object, &redis_sock TSRMLS_CC) < 0) {
        RETURN_FALSE;
    }

    cmd_len = redis_cmd_format(&cmd,
                    "*3" _NL
                    "$4" _NL
                    "MOVE" _NL
                    "$%d" _NL /* key_len */
                    "%s" _NL  /* key */
                    "$%d" _NL /* db_len */
                    "%d" _NL  /* db */

                    , key_len, key, key_len
                    , integer_length(dbNumber), dbNumber);

	REDIS_PROCESS_REQUEST(redis_sock, cmd, cmd_len);
	IF_ATOMIC() {
	  redis_1_response(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock, NULL);
	}
	REDIS_PROCESS_RESPONSE(redis_1_response);

}
/* }}} */

/* {{{ proto bool Redis::mset(array (key => value, ...))
 */
PHP_METHOD(Redis, mset) {
    zval *object;
    RedisSock *redis_sock;

    char *cmd;
    int cmd_len;
    zval *z_array;

    if (zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), "Oa",
                                     &object, redis_ce, &z_array) == FAILURE) {
        RETURN_FALSE;
    }

    if (redis_sock_get(object, &redis_sock TSRMLS_CC) < 0) {
        RETURN_FALSE;
    }

    if(zend_hash_num_elements(Z_ARRVAL_P(z_array)) == 0) {
        RETURN_FALSE;
    }

    cmd_len = redis_cmd_format(&cmd, "*%d\r\n$4\r\nMSET\r\n", 1 + 2 * zend_hash_num_elements(Z_ARRVAL_P(z_array)));

    HashTable *keytable = Z_ARRVAL_P(z_array);
    for(zend_hash_internal_pointer_reset(keytable);
        zend_hash_has_more_elements(keytable) == SUCCESS;
        zend_hash_move_forward(keytable)) {

        char *key, *val;
        int key_len, val_len;
        unsigned long idx;
        int type;
        zval **z_value_pp;

        type = zend_hash_get_current_key_ex(keytable, &key, &key_len, &idx, 0, NULL);
        if(zend_hash_get_current_data(keytable, (void**)&z_value_pp) == FAILURE) {
            continue; 	/* this should never happen, according to the PHP people. */
        }

        if(type != HASH_KEY_IS_STRING) { /* ignore non-string keys */
            continue;
        }

        val = Z_STRVAL_PP(z_value_pp);
        val_len = Z_STRLEN_PP(z_value_pp);

        if(key_len > 0) {
            key_len--;
        }

        cmd_len = redis_cmd_format(&cmd,
                                   "%s"
                                   "$%d" "\r\n" /* key_len */
                                   "%s" "\r\n"  /* key */
                                   "$%d" "\r\n" /* val_len */
                                   "%s" "\r\n"  /* val */
                                   , cmd, cmd_len
                                   , key_len, key, key_len
                                   , val_len, val, val_len);

    }
	REDIS_PROCESS_REQUEST(redis_sock, cmd, cmd_len);

	IF_ATOMIC() {
		redis_boolean_response(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock, NULL);
	}
	REDIS_PROCESS_RESPONSE(redis_boolean_response);
}
/* }}} */


/* {{{ proto string Redis::rpoplpush(string srckey, string dstkey)
 */
PHP_METHOD(Redis, rpoplpush)
{
    zval *object;
    RedisSock *redis_sock;
    char *srckey = NULL, *dstkey = NULL, *cmd, *response;
    int srckey_len, dstkey_len, cmd_len, response_len;

    if (zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), "Oss",
                                     &object, redis_ce, &srckey, &srckey_len,
                                     &dstkey, &dstkey_len) == FAILURE) {
        RETURN_FALSE;
    }

    if (redis_sock_get(object, &redis_sock TSRMLS_CC) < 0) {
        RETURN_FALSE;
    }

    cmd_len = redis_cmd_format(&cmd,
                               "*3" _NL
                               "$9" _NL
                               "RPOPLPUSH" _NL
                               "$%d" _NL    /* src_len */
                               "%s" _NL     /* src */
                               "$%d" _NL    /* dst_len */
                               "%s" _NL     /* dst */
                               , srckey_len, srckey, srckey_len
                               , dstkey_len, dstkey, dstkey_len);

	REDIS_PROCESS_REQUEST(redis_sock, cmd, cmd_len);
	IF_ATOMIC() {
		redis_string_response(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock, NULL);
	}
	REDIS_PROCESS_RESPONSE(redis_string_response);
}
/* }}} */
/* {{{ proto long Redis::zAdd(string key, int score, string value)
 */
PHP_METHOD(Redis, zAdd) {

    zval *object;
    RedisSock *redis_sock;

    char *cmd;
    int cmd_len, key_len, val_len;
    double score;
    char *key, *val;

    if (zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), "Osds",
                                     &object, redis_ce, &key, &key_len, &score, &val, &val_len) == FAILURE) {
        RETURN_FALSE;
    }

    if (redis_sock_get(object, &redis_sock TSRMLS_CC) < 0) {
        RETURN_FALSE;
    }

    cmd_len = redis_cmd_format(&cmd,
                               "*4" _NL
                               "$4" _NL
                               "ZADD" _NL

                               "$%d" _NL    /* key_len */
                               "%s" _NL     /* key */

                               "$%d" _NL    /* score_len */
                               "%F" _NL     /* score */

                               "$%d" _NL    /* val_len */
                               "%s" _NL     /* val */

                               , key_len, key, key_len
                               , double_length(score), score
                               , val_len, val, val_len);

	REDIS_PROCESS_REQUEST(redis_sock, cmd, cmd_len);
	IF_ATOMIC() {
		redis_long_response(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock, NULL);
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
    int key_len, cmd_len;
    long start, end;
    long withscores = 0;

    if (zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), "Osll|b",
                                     &object, redis_ce,
                                     &key, &key_len, &start, &end, &withscores) == FAILURE) {
        RETURN_FALSE;
    }

    if (redis_sock_get(object, &redis_sock TSRMLS_CC) < 0) {
        RETURN_FALSE;
    }

#define ZRANGE_FORMAT "$6" _NL\
                    "ZRANGE" _NL\
                    "$%d" _NL\
                    "%s" _NL\
                    "$%d" _NL\
                    "%d" _NL\
                    "$%d" _NL\
                    "%d" _NL\

    if(withscores) {
        cmd_len = redis_cmd_format(&cmd,
                    "*5" _NL
                    ZRANGE_FORMAT
                    "$10" _NL
                    "WITHSCORES" _NL

                    , key_len, key, key_len
                    , integer_length(start), start
                    , integer_length(end), end);
    } else {

        cmd_len = redis_cmd_format(&cmd,
                    "*4" _NL
                    ZRANGE_FORMAT
                    , key_len, key, key_len
                    , integer_length(start), start
                    , integer_length(end), end);
    }

	REDIS_PROCESS_REQUEST(redis_sock, cmd, cmd_len);
    if(withscores) {
            IF_ATOMIC() {
                redis_sock_read_multibulk_reply_zipped(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock, NULL);
            }
            REDIS_PROCESS_RESPONSE(redis_sock_read_multibulk_reply_zipped);
    } else {
            IF_ATOMIC() {
                if (redis_sock_read_multibulk_reply(INTERNAL_FUNCTION_PARAM_PASSTHRU,
                                                    redis_sock, NULL) < 0) {
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
    zval *object;
    RedisSock *redis_sock;
    char *key = NULL, *member = NULL, *cmd;
    int key_len, member_len, cmd_len, count; 

    if (zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), "Oss",
                                     &object, redis_ce, &key, &key_len,
                                     &member, &member_len) == FAILURE) {
        RETURN_FALSE;
    }

    if (redis_sock_get(object, &redis_sock TSRMLS_CC) < 0) {
        RETURN_FALSE;
    }

    cmd_len = redis_cmd_format(&cmd,
                            "*3" _NL
                            "$4" _NL
                            "ZREM" _NL

                            "$%d" _NL /* key_len */
                            "%s" _NL  /* key */

                            "$%d" _NL /* member_len */
                            "%s" _NL  /* member */

                            , key_len, key, key_len
                            , member_len, member, member_len);

	REDIS_PROCESS_REQUEST(redis_sock, cmd, cmd_len);
	IF_ATOMIC() {
		redis_long_response(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock, NULL);
	}
	REDIS_PROCESS_RESPONSE(redis_long_response);


}
/* }}} */
/* {{{ proto long Redis::zDeleteRangeByScore(string key, int start, int end)
 */
PHP_METHOD(Redis, zDeleteRangeByScore)
{
    zval *object;
    RedisSock *redis_sock;
    char *key = NULL, *cmd;
    int key_len, cmd_len, response_len;
    double start, end;

    if (zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), "Osdd",
                                     &object, redis_ce,
                                     &key, &key_len, &start, &end) == FAILURE) {
        RETURN_FALSE;
    }

    if (redis_sock_get(object, &redis_sock TSRMLS_CC) < 0) {
        RETURN_FALSE;
    }

    cmd_len = redis_cmd_format(&cmd,
                    "*4" _NL
                    "$16" _NL
                    "ZREMRANGEBYSCORE" _NL

                    "$%d" _NL /* key_len */
                    "%s" _NL  /* key */

                    "$%d" _NL /* start_len */
                    "%F" _NL  /* start */

                    "$%d" _NL /* end_len */
                    "%F" _NL  /* end */

                    , key_len, key, key_len
                    , double_length(start), start
                    , double_length(end), end);

	REDIS_PROCESS_REQUEST(redis_sock, cmd, cmd_len);
	IF_ATOMIC() {
		redis_long_response(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock, NULL);
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
    int key_len, cmd_len, response_len;
    long start, end;
    long withscores = 0;

    if (zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), "Osll|b",
                                     &object, redis_ce,
                                     &key, &key_len, &start, &end, &withscores) == FAILURE) {
        RETURN_FALSE;
    }

    if (redis_sock_get(object, &redis_sock TSRMLS_CC) < 0) {
        RETURN_FALSE;
    }

#define ZREVRANGE_FORMAT "$9" _NL\
                    "ZREVRANGE" _NL\
                    "$%d" _NL\
                    "%s" _NL\
                    "$%d" _NL\
                    "%d" _NL\
                    "$%d" _NL\
                    "%d" _NL

    if(withscores) {
        cmd_len = redis_cmd_format(&cmd,
                    "*5" _NL
                    ZREVRANGE_FORMAT
                    "$10" _NL
                    "WITHSCORES" _NL
                    , key_len, key, key_len
                    , integer_length(start), start
                    , integer_length(end), end);
    } else {
        cmd_len = redis_cmd_format(&cmd,
                    "*4" _NL
                    ZREVRANGE_FORMAT
                    , key_len, key, key_len
                    , integer_length(start), start
                    , integer_length(end), end);


    }
#undef ZREVRANGE_FORMAT

    REDIS_PROCESS_REQUEST(redis_sock, cmd, cmd_len);
    if(withscores) {
    	IF_ATOMIC() {
    		redis_sock_read_multibulk_reply_zipped(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock, NULL);
    	}
    	REDIS_PROCESS_RESPONSE(redis_sock_read_multibulk_reply_zipped);
    } else {
    	IF_ATOMIC() {
            if (redis_sock_read_multibulk_reply(INTERNAL_FUNCTION_PARAM_PASSTHRU,
                                                redis_sock, NULL) < 0) {
                RETURN_FALSE;
            }
    	}
    	REDIS_PROCESS_RESPONSE(redis_sock_read_multibulk_reply);
    }
}
/* }}} */
/* {{{ proto array Redis::zRangeByScore(string key, string start , string end [,array options = NULL])
 */
PHP_METHOD(Redis, zRangeByScore)
{
    zval *object, *z_options = NULL, **z_limit_val_pp = NULL, **z_withscores_val_pp = NULL;

    RedisSock *redis_sock;
    char *key = NULL, *limit = NULL, *cmd;
    int key_len, cmd_len, response_len;
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

    if (redis_sock_get(object, &redis_sock TSRMLS_CC) < 0) {
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

#define BASIC_FORMAT\
                       "$13" _NL\
                       "ZRANGEBYSCORE" _NL\
                       \
                       "$%d" _NL /* key_len */\
                       "%s" _NL  /* key */\
                       \
                       "$%d" _NL /* start_len */\
                       "%s" _NL  /* start */\
                       \
                       "$%d" _NL /* end_len */\
                       "%s" _NL  /* end */
#define BASIC_FORMAT_WITH_LIMIT BASIC_FORMAT\
                       "$5" _NL\
                       "LIMIT" _NL\
                       \
                       "$%d" _NL /* limit_low_len */\
                       "%d" _NL  /* limit_low */\
                       \
                       "$%d" _NL /* limit_high_len */\
                       "%d" _NL  /* limit_high */

    if(withscores) {
        if(has_limit) {
            cmd_len = redis_cmd_format(&cmd,
                            "*8" _NL
                            BASIC_FORMAT_WITH_LIMIT
                            "$10" _NL
                            "WITHSCORES" _NL
                            , key_len, key, key_len
                            , start_len, start, start_len
                            , end_len, end, end_len
                            , integer_length(limit_low), limit_low
                            , integer_length(limit_high), limit_high);
        } else {
            cmd_len = redis_cmd_format(&cmd,
                            "*5" _NL
                            BASIC_FORMAT
                            "$10" _NL
                            "WITHSCORES" _NL
                            , key_len, key, key_len
                            , start_len, start, start_len
                            , end_len, end, end_len);
        }
    } else {

        if(has_limit) {

            cmd_len = redis_cmd_format(&cmd,
                            "*7" _NL
                            BASIC_FORMAT_WITH_LIMIT
                            , key_len, key, key_len
                            , start_len, start, start_len
                            , end_len, end, end_len
                            , integer_length(limit_low), limit_low
                            , integer_length(limit_high), limit_high);
        } else {
            cmd_len = redis_cmd_format(&cmd,
                            "*4" _NL
                            BASIC_FORMAT
                            , key_len, key, key_len
                            , start_len, start, start_len
                            , end_len, end, end_len);
        }
    }
#undef BASIC_FORMAT
#undef BASIC_FORMAT_WITH_LIMIT

    REDIS_PROCESS_REQUEST(redis_sock, cmd, cmd_len);
    if(withscores) {
            /* with scores! we have to transform the return array.
             * return_value currently holds this: [elt0, val0, elt1, val1 ... ]
             * we want [elt0 => val0, elt1 => val1], etc.
             */
            IF_ATOMIC() {
                if(redis_sock_read_multibulk_reply_zipped(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock, NULL) < 0) {
                    RETURN_FALSE;
                }
            }
            REDIS_PROCESS_RESPONSE(redis_sock_read_multibulk_reply_zipped);
    } else {
            IF_ATOMIC() {
                if(redis_sock_read_multibulk_reply(INTERNAL_FUNCTION_PARAM_PASSTHRU,
                                                    redis_sock, NULL) < 0) {
                    RETURN_FALSE;
                }
            }
            REDIS_PROCESS_RESPONSE(redis_sock_read_multibulk_reply);
    }
}
/* }}} */

/* {{{ proto array Redis::zCount(string key, string start , string end)
 */
PHP_METHOD(Redis, zCount)
{
    zval *object;

    RedisSock *redis_sock;
    char *key = NULL, *cmd;
    int key_len, cmd_len, response_len;
    char *start, *end;
    int start_len, end_len;

    if (zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), "Osss",
                                     &object, redis_ce,
                                     &key, &key_len,
                                     &start, &start_len,
                                     &end, &end_len) == FAILURE) {
        RETURN_FALSE;
    }

    if (redis_sock_get(object, &redis_sock TSRMLS_CC) < 0) {
        RETURN_FALSE;
    }

    cmd_len = redis_cmd_format(&cmd,
                    "*4" _NL

                    "$6" _NL
                    "ZCOUNT" _NL

                    "$%d" _NL /* key_len */
                    "%s" _NL  /* key */

                    "$%d" _NL /* start_len */
                    "%s" _NL  /* start */

                    "$%d" _NL /* end_len */
                    "%s" _NL  /* end */

                    , key_len, key, key_len
                    , start_len, start, start_len
                    , end_len, end, end_len);

	REDIS_PROCESS_REQUEST(redis_sock, cmd, cmd_len);
	IF_ATOMIC() {
	  redis_long_response(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock, NULL);
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
    int key_len, cmd_len, response_len;

    if (zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), "Os",
                                     &object, redis_ce,
                                     &key, &key_len) == FAILURE) {
        RETURN_FALSE;
    }

    if (redis_sock_get(object, &redis_sock TSRMLS_CC) < 0) {
        RETURN_FALSE;
    }

    cmd_len = redis_cmd_format(&cmd,
                    "*2" _NL
                    "$5" _NL
                    "ZCARD" _NL
                    "$%d" _NL
                    "%s" _NL
                    , key_len, key, key_len);

	REDIS_PROCESS_REQUEST(redis_sock, cmd, cmd_len);
	IF_ATOMIC() {
	  redis_long_response(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock, NULL);
	}
	REDIS_PROCESS_RESPONSE(redis_long_response);

}
/* }}} */

/* {{{ proto double Redis::zScore(string key, string member)
 */
PHP_METHOD(Redis, zScore)
{
    zval *object;
    RedisSock *redis_sock;
    char *key = NULL, *member = NULL, *cmd;
    int key_len, member_len, cmd_len;

    if (zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), "Oss",
                                     &object, redis_ce, &key, &key_len,
                                     &member, &member_len) == FAILURE) {
        RETURN_FALSE;
    }

    if (redis_sock_get(object, &redis_sock TSRMLS_CC) < 0) {
        RETURN_FALSE;
    }

    cmd_len = redis_cmd_format(&cmd,
                    "*3" _NL
                    "$6" _NL
                    "ZSCORE" _NL
                    "$%d" _NL /* key_len */
                    "%s" _NL  /* key */
                    "$%d" _NL /* member_len */
                    "%s" _NL  /* member */
                    , key_len, key, key_len
                    , member_len, member, member_len);

	REDIS_PROCESS_REQUEST(redis_sock, cmd, cmd_len);
	IF_ATOMIC() {
	    redis_bulk_double_response(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock, NULL);
	}
	REDIS_PROCESS_RESPONSE(redis_bulk_double_response);
}
/* }}} */


PHPAPI void generic_rank_method(INTERNAL_FUNCTION_PARAMETERS, char *keyword, int keyword_len) {
    zval *object;
    RedisSock *redis_sock;
    char *key = NULL, *member = NULL, *cmd;
    int key_len, member_len, cmd_len;

    if (zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), "Oss",
                                     &object, redis_ce, &key, &key_len,
                                     &member, &member_len) == FAILURE) {
        RETURN_FALSE;
    }

    if (redis_sock_get(object, &redis_sock TSRMLS_CC) < 0) {
        RETURN_FALSE;
    }

    cmd_len = redis_cmd_format(&cmd,
                    "*3" _NL
                    "$%d" _NL
                    "%s" _NL
                    "$%d" _NL /* key_len */
                    "%s" _NL  /* key */
                    "$%d" _NL /* member_len */
                    "%s" _NL  /* member */
                    , keyword_len, keyword, keyword_len
                    , key_len, key, key_len
                    , member_len, member, member_len);

	REDIS_PROCESS_REQUEST(redis_sock, cmd, cmd_len);
	IF_ATOMIC() {
	    redis_long_response(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock, NULL);
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
    char *key = NULL, *cmd, *member, *response;
    int key_len, member_len, cmd_len, response_len;
    double val;

    if (zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), "Osds",
                                     &object, redis_ce,
                                     &key, &key_len, &val, &member, &member_len) == FAILURE) {
        RETURN_FALSE;
    }

    if (redis_sock_get(object, &redis_sock TSRMLS_CC) < 0) {
        RETURN_FALSE;
    }
    cmd_len = redis_cmd_format(&cmd,
                        "*4" _NL

                        "$%d" _NL
                        "%s" _NL  /* keyword */

                        "$%d" _NL
                        "%s" _NL /* key */

                        "$%d" _NL
                        "%F" _NL /* val */

                        "$%d" _NL
                        "%s" _NL /* member */

                        , keyword_len, keyword, keyword_len
                        , key_len, key, key_len
                        , double_length(val), val
                        , member_len, member, member_len);

	REDIS_PROCESS_REQUEST(redis_sock, cmd, cmd_len);
	IF_ATOMIC() {
	    redis_bulk_double_response(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock, NULL);
	}
	REDIS_PROCESS_RESPONSE(redis_bulk_double_response);

}

/* {{{ proto double Redis::zIncrBy(string key, double value, string member)
 */
PHP_METHOD(Redis, zIncrBy)
{
    generic_incrby_method(INTERNAL_FUNCTION_PARAM_PASSTHRU, "ZINCRBY", sizeof("ZINCRBY")-1);
}
/* }}} */
PHPAPI void generic_z_command(INTERNAL_FUNCTION_PARAMETERS, char *command, int command_len) {

	zval *object, *keys_array, *weights_array = NULL, **data;
	HashTable *arr_weights_hash, *arr_keys_hash;
	int key_output_len, array_weights_count, array_keys_count, operation_len = 0;
	char *key_output, *operation;
	RedisSock *redis_sock;

	HashPosition pointer;
	char *cmd = "";
	int cmd_len, cmd_elements;

	if(zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), "Osa|as",
					&object, redis_ce,
					&key_output, &key_output_len, &keys_array, &weights_array, &operation, &operation_len) == FAILURE) {
		RETURN_FALSE;
	}

    if(redis_sock_get(object, &redis_sock TSRMLS_CC) < 0) {
		RETURN_FALSE;
    }

    arr_keys_hash    = Z_ARRVAL_P(keys_array);
	array_keys_count = zend_hash_num_elements(arr_keys_hash);

    if (array_keys_count == 0) {
        RETURN_FALSE;
    }

	if(weights_array != NULL) {
		arr_weights_hash    = Z_ARRVAL_P(weights_array);
		array_weights_count = zend_hash_num_elements(arr_weights_hash);
		if (array_weights_count == 0) {
        	RETURN_FALSE;
    	}
		if((array_weights_count != 0) && (array_weights_count != array_keys_count)) {
			RETURN_FALSE;
		}

	}

    cmd_elements = 3;
	cmd_len = redis_cmd_format(&cmd,
                    "$%d" _NL /* command_len */
                    "%s" _NL  /* command */

                    "$%d" _NL /* key_output_len */
                    "%s" _NL  /* key_output */

                    "$%d" _NL
                    "%d" _NL  /* array_keys_count */

                    , command_len, command, command_len
                    , key_output_len, key_output, key_output_len
                    , integer_length(array_keys_count), array_keys_count);

	/* keys */
    for (zend_hash_internal_pointer_reset_ex(arr_keys_hash, &pointer);
         zend_hash_get_current_data_ex(arr_keys_hash, (void**) &data,
                                       &pointer) == SUCCESS;
         zend_hash_move_forward_ex(arr_keys_hash, &pointer)) {

        if (Z_TYPE_PP(data) == IS_STRING) {
            char *old_cmd = NULL;
            if(*cmd) {
                old_cmd = cmd;
            }
            cmd_len = redis_cmd_format(&cmd,
                            "%s" /* cmd */
                            "$%d" _NL
                            "%s" _NL
                            , cmd, cmd_len
                            , Z_STRLEN_PP(data), Z_STRVAL_PP(data), Z_STRLEN_PP(data));
            cmd_elements++;
            if(old_cmd) {
                efree(old_cmd);
            }
        }
    }

	/* weight */
	if(weights_array != NULL) {
        cmd_len = redis_cmd_format(&cmd,
                        "%s" /* cmd */
                        "$7" _NL
                        "WEIGHTS" _NL
                        , cmd, cmd_len);
        cmd_elements++;

		for (zend_hash_internal_pointer_reset_ex(arr_weights_hash, &pointer);
			zend_hash_get_current_data_ex(arr_weights_hash, (void**) &data, &pointer) == SUCCESS;
			zend_hash_move_forward_ex(arr_weights_hash, &pointer)) {

			if (Z_TYPE_PP(data) == IS_LONG) {
				char *old_cmd = NULL;
				if(*cmd) {
					old_cmd = cmd;
				}
				cmd_len = redis_cmd_format(&cmd,
                                "%s" /* cmd */
                                "$%d" _NL /* data_len */
                                "%d" _NL  /* data */

                                , cmd, cmd_len
                                , integer_length(Z_LVAL_PP(data)), Z_LVAL_PP(data));
                cmd_elements++;
				if(old_cmd) {
					efree(old_cmd);
				}
			} else {
				/* error */
				efree(cmd);
				RETURN_FALSE;
			}
		}
	}

 	if(operation_len != 0) {
		char *old_cmd = NULL;
		old_cmd = cmd;
        cmd_len = redis_cmd_format(&cmd,
                        "%s" /* cmd */
                        "$9" _NL
                        "AGGREGATE" _NL
                        "$%d" _NL
                        "%s" _NL
                        , cmd, cmd_len
                        , operation_len, operation, operation_len);
        cmd_elements += 2;
		efree(old_cmd);
	}

	char *old_cmd = cmd;
	cmd_len = redis_cmd_format(&cmd,
                    "*%d" _NL
                    "%s"
                    , cmd_elements
                    , cmd, cmd_len);
	efree(old_cmd);

	REDIS_PROCESS_REQUEST(redis_sock, cmd, cmd_len);
	IF_ATOMIC() {
	  redis_long_response(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock, NULL);
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

/* hGet */
PHP_METHOD(Redis, hSet)
{
    zval *object;
    RedisSock *redis_sock;
    char *key = NULL, *cmd, *member, *val;
    int key_len, member_len, cmd_len, val_len;

    if (zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), "Osss",
                                     &object, redis_ce,
                                     &key, &key_len, &member, &member_len, &val, &val_len) == FAILURE) {
        RETURN_FALSE;
    }

    if (redis_sock_get(object, &redis_sock TSRMLS_CC) < 0) {
        RETURN_FALSE;
    }
    cmd_len = redis_cmd_format(&cmd,
                    "*4\r\n"
                    "$4\r\n"
                    "HSET\r\n"

                    "$%d\r\n"   /* key */
                    "%s\r\n"

                    "$%d\r\n"   /* field */
                    "%s\r\n"

                    "$%d\r\n"   /* value */
                    "%s\r\n"
                    ,
                    key_len, key, key_len,
                    member_len, member, member_len,
                    val_len, val, val_len);

	REDIS_PROCESS_REQUEST(redis_sock, cmd, cmd_len);
	IF_ATOMIC() {
	  redis_long_response(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock, NULL);
	}
	REDIS_PROCESS_RESPONSE(redis_long_response);
}
/* }}} */

/* hSet */
PHP_METHOD(Redis, hGet)
{
    zval *object;
    RedisSock *redis_sock;
    char *key = NULL, *cmd, *member;
    int key_len, member_len, cmd_len;

    if (zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), "Oss",
                                     &object, redis_ce,
                                     &key, &key_len, &member, &member_len) == FAILURE) {
        RETURN_FALSE;
    }

    if (redis_sock_get(object, &redis_sock TSRMLS_CC) < 0) {
        RETURN_FALSE;
    }
    cmd_len = redis_cmd_format(&cmd,
                    "*3\r\n"
                    "$4\r\n"
                    "HGET\r\n"

                    "$%d\r\n"   /* key */
                    "%s\r\n"

                    "$%d\r\n"   /* field */
                    "%s\r\n"
                    ,
                    key_len, key, key_len,
                    member_len, member, member_len);

	REDIS_PROCESS_REQUEST(redis_sock, cmd, cmd_len);
	IF_ATOMIC() {
		redis_string_response(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock, NULL);
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
    int key_len, cmd_len;

    if (zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), "Os",
                                     &object, redis_ce,
                                     &key, &key_len) == FAILURE) {
        RETURN_FALSE;
    }

    if (redis_sock_get(object, &redis_sock TSRMLS_CC) < 0) {
        RETURN_FALSE;
    }
    cmd_len = redis_cmd_format(&cmd,
                    "*2\r\n"
                    "$4\r\n"
                    "HLEN\r\n"

                    "$%d\r\n"   /* key */
                    "%s\r\n"
                    ,
                    key_len, key, key_len);

	REDIS_PROCESS_REQUEST(redis_sock, cmd, cmd_len);
	IF_ATOMIC() {
	  redis_long_response(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock, NULL);
	}
	REDIS_PROCESS_RESPONSE(redis_long_response);

}
/* }}} */

PHPAPI RedisSock*
generic_hash_command_2(INTERNAL_FUNCTION_PARAMETERS, char *keyword, int keyword_len, char **out_cmd, int *out_len) {

    zval *object;
    RedisSock *redis_sock;
    char *key = NULL, *cmd, *member;
    int key_len, cmd_len, *member_len;

    if (zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), "Oss",
                                     &object, redis_ce,
                                     &key, &key_len, &member, &member_len) == FAILURE) {
        RETURN_FALSE;
    }

    if (redis_sock_get(object, &redis_sock TSRMLS_CC) < 0) {
        RETURN_FALSE;
    }
    cmd_len = redis_cmd_format(&cmd,
                    "*3\r\n"
                    "$%d\r\n"

                    "%s\r\n"

                    "$%d\r\n"   /* key */
                    "%s\r\n"

                    "$%d\r\n"   /* member */
                    "%s\r\n"
                    ,
                    keyword_len,
                    keyword, keyword_len,
                    key_len, key, key_len,
                    member_len, member, member_len);

    *out_cmd = cmd;
    *out_len = cmd_len;
    return redis_sock;
}

/* hDel */
PHP_METHOD(Redis, hDel)
{
    char *cmd;
    int cmd_len;
    RedisSock *redis_sock = generic_hash_command_2(INTERNAL_FUNCTION_PARAM_PASSTHRU, "HDEL", 4, &cmd, &cmd_len);
	zval *object = getThis();

	REDIS_PROCESS_REQUEST(redis_sock, cmd, cmd_len);
	IF_ATOMIC() {
		redis_1_response(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock, NULL);
	}
	REDIS_PROCESS_RESPONSE(redis_1_response);

}

/* hExists */
PHP_METHOD(Redis, hExists)
{
    char *cmd;
    int cmd_len;
    RedisSock *redis_sock = generic_hash_command_2(INTERNAL_FUNCTION_PARAM_PASSTHRU, "HEXISTS", 7, &cmd, &cmd_len);

	zval *object = getThis();

	REDIS_PROCESS_REQUEST(redis_sock, cmd, cmd_len);
	IF_ATOMIC() {
	  redis_1_response(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock, NULL);
	}
	REDIS_PROCESS_RESPONSE(redis_1_response);

}

PHPAPI RedisSock*
generic_hash_command_1(INTERNAL_FUNCTION_PARAMETERS, char *keyword, int keyword_len) {

    zval *object;
    RedisSock *redis_sock;
    char *key = NULL, *cmd;
    int key_len, cmd_len;

    if (zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), "Os",
                                     &object, redis_ce,
                                     &key, &key_len) == FAILURE) {
        RETURN_FALSE;
    }

    if (redis_sock_get(object, &redis_sock TSRMLS_CC) < 0) {
        RETURN_FALSE;
    }
    cmd_len = redis_cmd_format(&cmd,
                    "*2\r\n"
                    "$%d\r\n"

                    "%s\r\n"

                    "$%d\r\n"   /* key */
                    "%s\r\n"
                    ,
                    keyword_len,
                    keyword, keyword_len,
                    key_len, key, key_len);

	REDIS_PROCESS_REQUEST(redis_sock, cmd, cmd_len);
    return redis_sock;
}

/* hKeys */
PHP_METHOD(Redis, hKeys)
{
    RedisSock *redis_sock = generic_hash_command_1(INTERNAL_FUNCTION_PARAM_PASSTHRU, "HKEYS", sizeof("HKEYS")-1);
	zval *object = getThis();

	IF_ATOMIC() {
	    if (redis_sock_read_multibulk_reply(INTERNAL_FUNCTION_PARAM_PASSTHRU,
											redis_sock, NULL) < 0) {
    	    RETURN_FALSE;
	    }
	}
	REDIS_PROCESS_RESPONSE(redis_sock_read_multibulk_reply);


}
/* hVals */
PHP_METHOD(Redis, hVals)
{
    RedisSock *redis_sock = generic_hash_command_1(INTERNAL_FUNCTION_PARAM_PASSTHRU, "HVALS", sizeof("HVALS")-1);
	zval *object = getThis();

	IF_ATOMIC() {
	    if (redis_sock_read_multibulk_reply(INTERNAL_FUNCTION_PARAM_PASSTHRU,
    	                                    redis_sock, NULL) < 0) {
        	RETURN_FALSE;
	    }
	}
	REDIS_PROCESS_RESPONSE(redis_sock_read_multibulk_reply);

}


PHP_METHOD(Redis, hGetAll) {

    RedisSock *redis_sock = generic_hash_command_1(INTERNAL_FUNCTION_PARAM_PASSTHRU, "HGETALL", sizeof("HGETALL")-1);

	zval *object = getThis();

	IF_ATOMIC() {
	    if (redis_sock_read_multibulk_reply_zipped_strings(INTERNAL_FUNCTION_PARAM_PASSTHRU,
    	                                    redis_sock, NULL) < 0) {
        	RETURN_FALSE;
	    }
	}
	REDIS_PROCESS_RESPONSE(redis_sock_read_multibulk_reply_zipped_strings);
}

PHPAPI void array_zip_values_and_scores(zval *z_tab, int use_atof TSRMLS_DC) {

    zval *z_ret;
    MAKE_STD_ZVAL(z_ret);
    array_init(z_ret);

    HashTable *keytable = Z_ARRVAL_P(z_tab);

    for(zend_hash_internal_pointer_reset(keytable);
        zend_hash_has_more_elements(keytable) == SUCCESS;
        zend_hash_move_forward(keytable)) {

        char *tablekey, *hkey, *hval;
        int tablekey_len, hkey_len, hval_len;
        unsigned long idx;
        int type;
        zval **z_value_pp;

        type = zend_hash_get_current_key_ex(keytable, &tablekey, &tablekey_len, &idx, 0, NULL);
        if(zend_hash_get_current_data(keytable, (void**)&z_value_pp) == FAILURE) {
            continue; 	/* this should never happen, according to the PHP people. */
        }

        /* get current value, a key */
        hkey = Z_STRVAL_PP(z_value_pp);
        hkey_len = Z_STRLEN_PP(z_value_pp);

        /* move forward */
        zend_hash_move_forward(keytable);

        /* fetch again */
        type = zend_hash_get_current_key_ex(keytable, &tablekey, &tablekey_len, &idx, 0, NULL);
        if(zend_hash_get_current_data(keytable, (void**)&z_value_pp) == FAILURE) {
            continue; 	/* this should never happen, according to the PHP people. */
        }

        /* get current value, a hash value now. */
        hval = Z_STRVAL_PP(z_value_pp);
        hval_len = Z_STRLEN_PP(z_value_pp);

        if(use_atof) {
            add_assoc_double_ex(z_ret, hkey, 1+hkey_len, atof(hval));
        } else {
            add_assoc_stringl_ex(z_ret, hkey, 1+hkey_len, hval, hval_len, 1);
        }
    }
    /* replace */
    zval_dtor(z_tab);
    *z_tab = *z_ret;
    zval_copy_ctor(z_tab);

    efree(z_ret);
}

PHP_METHOD(Redis, hIncrBy)
{
    zval *object;
    RedisSock *redis_sock;
    char *key = NULL, *cmd, *member;
    int key_len, member_len, cmd_len;
    long val;

    if (zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), "Ossl",
                                     &object, redis_ce,
                                     &key, &key_len, &member, &member_len, &val) == FAILURE) {
        RETURN_FALSE;
    }

    if (redis_sock_get(object, &redis_sock TSRMLS_CC) < 0) {
        RETURN_FALSE;
    }

    /* HINCRBY key member amount */
    cmd_len = redis_cmd_format(&cmd,
                    "*4" _NL
                    "$7" _NL
                    "HINCRBY" _NL

                    "$%d" _NL /* key_len */
                    "%s" _NL  /* key */

                    "$%d" _NL /* member_len */
                    "%s" _NL  /* member */

                    "$%d" _NL /* val_len */
                    "%d" _NL  /* val */

                    , key_len, key, key_len
                    , member_len, member, member_len
                    , integer_length(val), val);

	REDIS_PROCESS_REQUEST(redis_sock, cmd, cmd_len);
	IF_ATOMIC() {
	  redis_long_response(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock, NULL);
	}
	REDIS_PROCESS_RESPONSE(redis_long_response);

}

PHP_METHOD(Redis, hMset)
{
    zval *object;
    RedisSock *redis_sock;
    char *key = NULL, *cmd, *member;
    int key_len, member_len, cmd_len;
    zval *z_hash, *data;
    HashTable *ht_hash;
    HashPosition pointer;
    int i;
    int element_count = 2;
    char *old_cmd = NULL;

    if (zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), "Osa",
                                     &object, redis_ce,
                                     &key, &key_len, &z_hash) == FAILURE) {
        RETURN_FALSE;
    }

    if (redis_sock_get(object, &redis_sock TSRMLS_CC) < 0) {
        RETURN_FALSE;
    }

    ht_hash = Z_ARRVAL_P(z_hash);

    if (zend_hash_num_elements(ht_hash) == 0) {
        RETURN_FALSE;
    }

    cmd_len = redis_cmd_format(&cmd, 
                    "$5" _NL "HMSET" _NL
                    "$%d" _NL "%s" _NL
                    , key_len, key, key_len);

    /* looping on each item of the array */
	for(i =0, zend_hash_internal_pointer_reset(ht_hash);
		zend_hash_has_more_elements(ht_hash) == SUCCESS;
		i++, zend_hash_move_forward(ht_hash)) {

		char *hkey;
		unsigned int hkey_len;
		unsigned long idx;
		int type;
		zval *z_value, **z_value_p;

		type = zend_hash_get_current_key_ex(ht_hash, &hkey, &hkey_len, &idx, 0, NULL);

		if(zend_hash_get_current_data(ht_hash, (void**)&z_value_p) == FAILURE) {
			continue; 	/* this should never happen */
		}

		if(type != HASH_KEY_IS_STRING) {
                continue;
        }
        element_count += 2;

        /* key is set. */
        zval *z_copy;
        MAKE_STD_ZVAL(z_copy);
        switch(Z_TYPE_PP(z_value_p)) {

            case IS_OBJECT:
                ZVAL_STRINGL(z_copy, "Object", 6, 1);
                break;

            case IS_ARRAY:
                ZVAL_STRINGL(z_copy, "Array", 5, 1);
                break;

            default:
                *z_copy = **z_value_p;
                zval_copy_ctor(z_copy);
                if(Z_TYPE_PP(z_value_p) != IS_STRING) {
                    convert_to_string(z_copy);
                }
        }

        old_cmd = cmd;
        cmd_len = redis_cmd_format(&cmd, "%s"
                        "$%d" _NL "%s" _NL
                        "$%d" _NL "%s" _NL
                        , cmd, cmd_len
                        , hkey_len-1, hkey, hkey_len-1
                        , Z_STRLEN_P(z_copy), Z_STRVAL_P(z_copy), Z_STRLEN_P(z_copy));
        efree(old_cmd);
        zval_dtor(z_copy);
        efree(z_copy);
    }

    old_cmd = cmd;
    cmd_len = redis_cmd_format(&cmd, "*%d" _NL "%s"
                    , element_count, cmd, cmd_len);
    efree(old_cmd);

	REDIS_PROCESS_REQUEST(redis_sock, cmd, cmd_len);
	IF_ATOMIC() {
		redis_boolean_response(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock, NULL);
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

/* flag : get, set {REDIS_ATOMIC, REDIS_MULTI, REDIS_PIPELINE} */

PHP_METHOD(Redis, multi)
{

    RedisSock *redis_sock;
    char *cmd;
	int response_len, cmd_len;
	char * response;
	zval *object;
	long multi_value = REDIS_MULTI;

	if (zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), "O|l",
                                     &object, redis_ce, &multi_value) == FAILURE) {
        RETURN_FALSE;
    }

    /* if the flag is activated, send the command, the reply will be "QUEUED" or -ERR */

    if (redis_sock_get(object, &redis_sock TSRMLS_CC) < 0) {
        RETURN_FALSE;
    }

	if(multi_value == REDIS_MULTI || multi_value == REDIS_PIPELINE) {
		set_flag(object, multi_value TSRMLS_CC);
	} else {
        RETURN_FALSE;
	}

    set_multi_current(getThis(), NULL); /* current = NULL; */

	IF_MULTI() {
	    cmd_len = redis_cmd_format(&cmd, "*1" _NL "$5" _NL "MULTI" _NL);

    	if (redis_sock_write(redis_sock, cmd, cmd_len) < 0) {
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
        free_reply_callbacks(getThis());
		RETURN_ZVAL(getThis(), 1, 0);
	}
}

/* discard */
PHP_METHOD(Redis, discard)
{
    RedisSock *redis_sock;
    char *cmd;
	int response_len, cmd_len;
	char * response;
	zval *object;

	if (zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), "O",
                                     &object, redis_ce) == FAILURE) {
        RETURN_FALSE;
    }

	set_flag(object, 0 TSRMLS_CC);

    if (redis_sock_get(object, &redis_sock TSRMLS_CC) < 0) {
        RETURN_FALSE;
    }

    cmd_len = redis_cmd_format(&cmd, "*1" _NL "$7" _NL "DISCARD" _NL);

    if (redis_sock_write(redis_sock, cmd, cmd_len) < 0) {
        efree(cmd);
        RETURN_FALSE;
    }
    efree(cmd);

    if ((response = redis_sock_read(redis_sock, &response_len TSRMLS_CC)) == NULL) {
        RETURN_FALSE;
    }

	if(response_len == 3 && strncmp(response, "+OK", 3) == 0) {
		RETURN_TRUE;
	}
	RETURN_FALSE;
}
PHPAPI int redis_sock_read_multibulk_pipeline_reply(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock)
{
    zval *z_tab;
    MAKE_STD_ZVAL(z_tab);
    array_init(z_tab);

    redis_sock_read_multibulk_multi_reply_loop(INTERNAL_FUNCTION_PARAM_PASSTHRU,
                    redis_sock, z_tab, NULL);

    *return_value = *z_tab;
    efree(z_tab);

    /* free allocated function/request memory */
    free_reply_callbacks(getThis());

    return 0;

}
/* redis_sock_read_multibulk_multi_reply */
PHPAPI int redis_sock_read_multibulk_multi_reply(INTERNAL_FUNCTION_PARAMETERS,
                                      RedisSock *redis_sock)
{

    char inbuf[1024];

    redis_check_eof(redis_sock TSRMLS_CC);

    php_stream_gets(redis_sock->stream, inbuf, 1024);
    if(inbuf[0] != '*') {
        return -1;
    }

	/* number of responses */
    int numElems = atoi(inbuf+1);

    if(numElems < 0) {
        return -1;
    }

    zval_dtor(return_value);
    zval *z_tab;
    MAKE_STD_ZVAL(z_tab);
    array_init(z_tab);

    redis_sock_read_multibulk_multi_reply_loop(INTERNAL_FUNCTION_PARAM_PASSTHRU,
                    redis_sock, z_tab, numElems);

    *return_value = *z_tab;
    efree(z_tab);

    return 0;
}

void
free_reply_callbacks(zval *z_this) {

	fold_item *fi;
    fold_item *head = get_multi_head(z_this);
    for(fi = head; fi; ) {
        fold_item *fi_next = fi->next;
        free(fi);
        fi = fi_next;
    }
    set_multi_head(z_this, NULL);
    set_multi_current(z_this, NULL);


	request_item *ri;
    for(ri = get_pipeline_head(z_this); ri; ) {
        struct request_item *ri_next = ri->next;
        free(ri->request_str);
        free(ri);
        ri = ri_next;
    }
    set_pipeline_head(z_this, NULL);
    set_pipeline_current(z_this, NULL);

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
   	if (redis_sock_get(object, &redis_sock TSRMLS_CC) < 0) {
       	RETURN_FALSE;
    }

	IF_MULTI() {

		cmd_len = redis_cmd_format(&cmd, "*1" _NL "$4" _NL "EXEC" _NL);

		if (redis_sock_write(redis_sock, cmd, cmd_len) < 0) {
			efree(cmd);
			RETURN_FALSE;
		}
		efree(cmd);

	    if (redis_sock_read_multibulk_multi_reply(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock) < 0) {
            zval_dtor(return_value);
            free_reply_callbacks(object);
            set_flag(object, REDIS_ATOMIC TSRMLS_CC);
			RETURN_FALSE;
	    }
        free_reply_callbacks(object);
		set_flag(object, REDIS_ATOMIC TSRMLS_CC);
	}

	IF_PIPELINE() {

		char *request = NULL;
		int total = 0;
		int offset = 0;

        /* compute the total request size */
		for(ri = get_pipeline_head(object); ri; ri = ri->next) {
            total += ri->request_size;
		}
        if(total) {
		    request = malloc(total);
        }

        /* concatenate individual elements one by one in the target buffer */
		for(ri = get_pipeline_head(object); ri; ri = ri->next) {
			memcpy(request + offset, ri->request_str, ri->request_size);
			offset += ri->request_size;
		}

		if(request != NULL) {
		    if (redis_sock_write(redis_sock, request, total) < 0) {
    		    free(request);
                free_reply_callbacks(object);
                set_flag(object, REDIS_ATOMIC TSRMLS_CC);
        		RETURN_FALSE;
		    }
		   	free(request);
		} else {
                set_flag(object, REDIS_ATOMIC TSRMLS_CC);
                free_reply_callbacks(object);
                array_init(return_value); /* empty array when no command was run. */
                return;
        }

	    if (redis_sock_read_multibulk_pipeline_reply(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock) < 0) {
			set_flag(object, REDIS_ATOMIC TSRMLS_CC);
            free_reply_callbacks(object);
			RETURN_FALSE;
	    }
		set_flag(object, REDIS_ATOMIC TSRMLS_CC);
        free_reply_callbacks(object);
	}
}

PHPAPI void fold_this_item(INTERNAL_FUNCTION_PARAMETERS, fold_item *item, RedisSock *redis_sock, zval *z_tab) {
	item->fun(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock, z_tab TSRMLS_CC);
}

PHPAPI int redis_sock_read_multibulk_multi_reply_loop(INTERNAL_FUNCTION_PARAMETERS,
							RedisSock *redis_sock, zval *z_tab, int numElems)
{

    fold_item *head = get_multi_head(getThis());
    fold_item *current = get_multi_current(getThis());
    for(current = head; current; current = current->next) {
		fold_this_item(INTERNAL_FUNCTION_PARAM_PASSTHRU, current, redis_sock, z_tab);
    }
    set_multi_current(getThis(), current);
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
    if (redis_sock_get(object, &redis_sock TSRMLS_CC) < 0) {
        RETURN_FALSE;
    }
	set_flag(object, REDIS_PIPELINE TSRMLS_CC);

	/*
		NB : we keep the function fold, to detect the last function.
		We need the response format of the n - 1 command. So, we can delete when n > 2, the { 1 .. n - 2} commands
	*/

    free_reply_callbacks(getThis());

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
    int cmd_len, key_len, val_len;

    if (zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), "Oss",
                                     &object, redis_ce,
                                     &key, &key_len, &val, &val_len) == FAILURE) {
        RETURN_NULL();
    }

    if (redis_sock_get(object, &redis_sock TSRMLS_CC) < 0) {
        RETURN_FALSE;
    }

    cmd_len = redis_cmd_format(&cmd, "PUBLISH %s %d\r\n%s\r\n",
                               key, key_len,
                               val_len,
                               val, val_len);

    if (redis_sock_write(redis_sock, cmd, cmd_len) < 0) {
        efree(cmd);
        RETURN_FALSE;
    }
    efree(cmd);
	redis_long_response(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock, NULL);
}

/* 
	subscribe channel_1 channel_2 ... channel_n 
	subscribe(array(channel_1, channel_2, ..., channel_n), callback) 
*/
PHP_METHOD(Redis, subscribe)
{
	zval *z_callback,*object, *array, **data;
    HashTable *arr_hash;
    HashPosition pointer;
    RedisSock *redis_sock;
    char *cmd = "", *old_cmd = NULL, *callback_ft_name;
    int cmd_len, array_count, callback_ft_name_len;

	
	if (zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), "Oasz", 
									 &object, redis_ce, &array, &callback_ft_name, &callback_ft_name_len, &z_callback) == FAILURE) {
		RETURN_FALSE;	
	}

    if (redis_sock_get(object, &redis_sock TSRMLS_CC) < 0) {
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
    cmd_len = spprintf(&cmd, 0, "SUBSCRIBE %s\r\n", cmd);
    efree(old_cmd);
    if (redis_sock_write(redis_sock, cmd, cmd_len) < 0) {
        efree(cmd);
        RETURN_FALSE;
    }
    efree(cmd);
	
	/* read the status of the execution of the command `subscribe` */
	zval *z_tab, **tmp;
	char *type_response;
	
    z_tab = redis_sock_read_multibulk_reply_zval(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock);

	if (zend_hash_index_find(Z_ARRVAL_P(z_tab), 0, (void**)&tmp) == SUCCESS) {
		type_response = Z_STRVAL_PP(tmp);
		if(strcmp(type_response, "subscribe") != 0) {
			efree(tmp);
			efree(z_tab);	
			RETURN_FALSE;
		} 
	} else {
		efree(z_tab);	
		RETURN_FALSE;
	}
	efree(z_tab);	
	
	int callback_type;
	zval *z_o, *z_fun,*z_ret, *z_args[3];
	char *class_name, *method_name;
	zend_class_entry **class_entry_pp, *ce;

	MAKE_STD_ZVAL(z_ret);	

	/* verify the callback */
	if(Z_TYPE_P(z_callback) == IS_ARRAY) {

		if (zend_hash_index_find(Z_ARRVAL_P(z_callback), 0, (void**)&tmp) == FAILURE) {
			RETURN_FALSE;
		}

		class_name = Z_STRVAL_PP(tmp);

		if (zend_hash_index_find(Z_ARRVAL_P(z_callback), 1, (void**)&tmp) == FAILURE) {
			RETURN_FALSE;
		}

		method_name = Z_STRVAL_PP(tmp);	
		if(zend_lookup_class(class_name, strlen(class_name), &class_entry_pp TSRMLS_CC) == FAILURE) {
			/* The class didn't exist */
			/* generate error */
			RETURN_FALSE;
		}


		ce = *class_entry_pp;
		// create an empty object.                                                                                                                                                                                                               
		MAKE_STD_ZVAL(z_o);
		object_init_ex(z_o, ce);

		ALLOC_INIT_ZVAL(z_fun);
		ZVAL_STRING(z_fun, method_name, 1);
		callback_type = R_SUB_CALLBACK_CLASS_TYPE;

	} else if(Z_TYPE_P(z_callback) == IS_STRING) {
		callback_ft_name = Z_STRVAL_P(z_callback);
		callback_ft_name_len = strlen(callback_ft_name);
		callback_type = R_SUB_CALLBACK_FT_TYPE;
	}

	/* Multibulk Response, format : {message type, originating channel, message payload} */
	while(1) {		
		/* call the callback with this z_tab in argument */
	    z_tab = redis_sock_read_multibulk_reply_zval(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock);
		zval **type, **channel, **data;
		
		if(Z_TYPE_P(z_tab) != IS_ARRAY) {
			//ERROR
			break;
		}

		if (zend_hash_index_find(Z_ARRVAL_P(z_tab), 0, (void**)&type) == FAILURE) {
			break;
		}
		if (zend_hash_index_find(Z_ARRVAL_P(z_tab), 1, (void**)&channel) == FAILURE) {
			break;
		}		
		if (zend_hash_index_find(Z_ARRVAL_P(z_tab), 2, (void**)&data) == FAILURE) {
			break;
		}		
		
		z_args[0] = getThis();
		z_args[1] = *channel;
		z_args[2] = *data;
	
		switch(callback_type) {
			case R_SUB_CALLBACK_CLASS_TYPE:
				call_user_function(&ce->function_table, &z_o, z_fun, z_ret, 3, z_args TSRMLS_CC);							
				//efree(z_o);				
		        //efree(z_fun);
				//zval_dtor(z_ret); efree(z_ret);
				//free(z_args[0]); free(z_args[1]); free(z_args[2]);
				//free(z_args);
				
				break;
			case R_SUB_CALLBACK_FT_TYPE:
		       	MAKE_STD_ZVAL(z_ret);
				MAKE_STD_ZVAL(z_fun);	
				ZVAL_STRINGL(z_fun, callback_ft_name, callback_ft_name_len, 0);
	        	call_user_function(EG(function_table), NULL, z_fun, z_ret, 3, z_args TSRMLS_CC);
		        efree(z_fun);
				//free(z_args[0]); free(z_args[1]); free(z_args[2]);
				//free(z_args);
				break;
		}

		if(Z_TYPE_P(z_ret) == IS_BOOL) {
			// the callback function return TRUE if we want to continue listening on the channel
			// or FALSE if we need to stop listeneing		
			if(!Z_BVAL_P(z_ret)) {
				efree(z_o);				
		        efree(z_fun);
				zval_dtor(z_tab);
				efree(z_tab);
				zval_dtor(z_ret);
				efree(z_ret);
				break;	
			}
		} else {
			//error : the callback must return BOOL reponse
			efree(z_o);				
	        efree(z_fun);
			zval_dtor(z_tab);
			efree(z_tab);
			zval_dtor(z_ret);
			efree(z_ret);			
			RETURN_FALSE;
		}
		zval_dtor(z_tab);
		efree(z_tab);
	}	
	/*@TODO : collect all the returned data and return it */
}

/** 
 *	unsubscribe channel_0 channel_1 ... channel_n
 *  unsubscribe(array(channel_0, channel_1, ..., channel_n))
 * response format :
 * array(
 * 	channel_0 => TRUE|FALSE,
 *	channel_1 => TRUE|FALSE,
 *	...
 *	channel_n => TRUE|FALSE
 * );
 **/

PHP_METHOD(Redis, unsubscribe) 
{
    zval *object, *array, **data;
    HashTable *arr_hash;
    HashPosition pointer;
    RedisSock *redis_sock;
    char *cmd = "", *old_cmd = NULL;
    int cmd_len, array_count;
	
	if (zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), "Oa", 
									 &object, redis_ce, &array) == FAILURE) {
		RETURN_FALSE;	
	}
    if (redis_sock_get(object, &redis_sock TSRMLS_CC) < 0) {
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
    cmd_len = spprintf(&cmd, 0, "UNSUBSCRIBE %s\r\n", cmd);
    efree(old_cmd);

    if (redis_sock_write(redis_sock, cmd, cmd_len) < 0) {
        efree(cmd);
        RETURN_FALSE;
    }
    efree(cmd);

	int i = 1;
	zval *z_tab, **z_channel;

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

/* vim: set tabstop=4 expandtab: */

