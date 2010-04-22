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


static int le_redis_sock;
static int le_redis_multi_access_type;

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
     PHP_ME(Redis, setnx, NULL, ZEND_ACC_PUBLIC)
     PHP_ME(Redis, getSet, NULL, ZEND_ACC_PUBLIC)
     PHP_ME(Redis, randomKey, NULL, ZEND_ACC_PUBLIC)
     PHP_ME(Redis, renameKey, NULL, ZEND_ACC_PUBLIC)
     PHP_ME(Redis, renameNx, NULL, ZEND_ACC_PUBLIC)
     PHP_ME(Redis, add, NULL, ZEND_ACC_PUBLIC)
     PHP_ME(Redis, getMultiple, NULL, ZEND_ACC_PUBLIC)
     PHP_ME(Redis, exists, NULL, ZEND_ACC_PUBLIC)
     PHP_ME(Redis, delete, NULL, ZEND_ACC_PUBLIC)
     PHP_ME(Redis, incr, NULL, ZEND_ACC_PUBLIC)
     PHP_ME(Redis, decr, NULL, ZEND_ACC_PUBLIC)
     PHP_ME(Redis, type, NULL, ZEND_ACC_PUBLIC)
     PHP_ME(Redis, getKeys, NULL, ZEND_ACC_PUBLIC)
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
     PHP_ME(Redis, zDeleteRangeByScore, NULL, ZEND_ACC_PUBLIC)
     PHP_ME(Redis, zCard, NULL, ZEND_ACC_PUBLIC)
     PHP_ME(Redis, zScore, NULL, ZEND_ACC_PUBLIC)
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

     PHP_ME(Redis, multi, NULL, ZEND_ACC_PUBLIC)
     PHP_ME(Redis, discard, NULL, ZEND_ACC_PUBLIC)
     PHP_ME(Redis, exec, NULL, ZEND_ACC_PUBLIC)
     PHP_ME(Redis, pipeline, NULL, ZEND_ACC_PUBLIC)

     /* aliases */
     PHP_MALIAS(Redis, open, connect, NULL, ZEND_ACC_PUBLIC)
     PHP_MALIAS(Redis, lLen, lSize, NULL, ZEND_ACC_PUBLIC)
     PHP_MALIAS(Redis, sGetMembers, sMembers, NULL, ZEND_ACC_PUBLIC)
     PHP_MALIAS(Redis, mget, getMultiple, NULL, ZEND_ACC_PUBLIC)
     PHP_MALIAS(Redis, expire, setTimeout, NULL, ZEND_ACC_PUBLIC)

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

	set_flag(object, REDIS_ATOMIC);

}
/* }}} */
PHPAPI int get_flag(zval *object)
{
	zval **multi_flag;
	int flag, flag_result;

	zend_hash_find(Z_OBJPROP_P(object), "multi_flag", sizeof("multi_flag"), (void **) &multi_flag);
	flag = (int)(long)zend_list_find(Z_LVAL_PP(multi_flag), &flag_result);

	return flag;
}

PHPAPI void set_flag(zval *object, int new_flag)
{
	zval **multi_flag;
	int flag_result;

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

    cmd_len = redis_cmd_format(&cmd, "SET %s %d\r\n%s\r\n", key, key_len, val_len, val, val_len);

	REDIS_PROCESS_REQUEST(redis_sock, cmd, cmd_len);
	
	IF_ATOMIC() {
		redis_boolean_response(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock, NULL TSRMLS_CC);
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

    cmd_len = redis_cmd_format(&cmd, "SETNX %s %d\r\n%s\r\n", key, key_len, val_len, val, val_len);
	
    REDIS_PROCESS_REQUEST(redis_sock, cmd, cmd_len);

    IF_ATOMIC() {
	  redis_1_response(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock, NULL TSRMLS_CC);
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
    char *key = NULL, *val = NULL, *cmd, *response;
    int key_len, val_len, cmd_len, response_len;

    if (zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), "Oss",
                                     &object, redis_ce, &key, &key_len,
                                     &val, &val_len) == FAILURE) {
        RETURN_FALSE;
    }

    if (redis_sock_get(object, &redis_sock TSRMLS_CC) < 0) {
        RETURN_FALSE;
    }

    cmd_len = redis_cmd_format(&cmd, "GETSET %s %d\r\n%s\r\n", key, key_len, val_len, val, val_len);
	
	REDIS_PROCESS_REQUEST(redis_sock, cmd, cmd_len);
	IF_ATOMIC() {
		redis_string_response(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock, NULL TSRMLS_CC);
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
    char *cmd, *response, *ret;
    int cmd_len, response_len;

    if (zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), "O",
                                     &object, redis_ce) == FAILURE) {
        RETURN_FALSE;
    }

    if (redis_sock_get(object, &redis_sock TSRMLS_CC) < 0) {
        RETURN_FALSE;
    }

    cmd_len = redis_cmd_format(&cmd, "RANDOMKEY\r\n");

	REDIS_PROCESS_REQUEST(redis_sock, cmd, cmd_len);
	IF_ATOMIC() {
	  redis_string_response(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock, NULL TSRMLS_CC);
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

    cmd_len = redis_cmd_format(&cmd, "RENAME %s %s\r\n",
                               src, src_len,
                               dst, dst_len);


	REDIS_PROCESS_REQUEST(redis_sock, cmd, cmd_len);
	IF_ATOMIC() {
		redis_boolean_response(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock, NULL TSRMLS_CC);
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

    cmd_len = redis_cmd_format(&cmd, "RENAMENX %s %s\r\n",
                               src, src_len,
                               dst, dst_len);

	REDIS_PROCESS_REQUEST(redis_sock, cmd, cmd_len);
	IF_ATOMIC() {
		redis_boolean_response(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock, NULL TSRMLS_CC);
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
    char *key = NULL, *cmd, *response;
    int key_len, cmd_len, response_len;

    if (zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), "Os",
                                     &object, redis_ce,
                                     &key, &key_len) == FAILURE) {
        RETURN_FALSE;
    }

    if (redis_sock_get(object, &redis_sock TSRMLS_CC) < 0) {
        RETURN_FALSE;
    }

    cmd_len = spprintf(&cmd, 0, "GET %s\r\n", key);

	REDIS_PROCESS_REQUEST(redis_sock, cmd, cmd_len);
    IF_ATOMIC() {
	  redis_string_response(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock, NULL TSRMLS_CC);
    }
    REDIS_PROCESS_RESPONSE(redis_string_response);

}
/* }}} */

/* {{{ proto boolean Redis::add(string key, string value)
 */
PHP_METHOD(Redis, add)
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

    cmd_len = redis_cmd_format(&cmd, "SADD %s %d\r\n%s\r\n",
                    key, key_len,
                    val_len,
                    val, val_len);

	REDIS_PROCESS_REQUEST(redis_sock, cmd, cmd_len);
    IF_ATOMIC() {
	  redis_1_response(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock, NULL TSRMLS_CC);
    }
    REDIS_PROCESS_RESPONSE(redis_1_response);

}
/* }}} */

/* {{{ proto string Redis::ping()
 */
PHP_METHOD(Redis, ping)
{
    zval *object;
    RedisSock *redis_sock;
    char *response;
    int cmd_len, response_len;

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

	char cmd[] = "PING\r\n";
	cmd_len = 6;

	/*@TODO : special structure, to refactor */
	IF_MULTI_OR_ATOMIC() {
		if(redis_sock_write(redis_sock, cmd, sizeof(cmd)-1) < 0) {
			RETURN_FALSE;
		}
	}
	IF_PIPELINE() {
		PIPELINE_ENQUEUE_COMMAND(cmd, cmd_len);
	}	

	MULTI_RESPONSE(redis_string_response);
	IF_ATOMIC() {
		redis_string_response(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock, NULL TSRMLS_CC);
	}
	ELSE_IF_MULTI()
	ELSE_IF_PIPELINE();

}
/* }}} */

PHPAPI void redis_atomic_increment(INTERNAL_FUNCTION_PARAMETERS, char *keyword TSRMLS_DC) {

    zval *object;
    RedisSock *redis_sock;
    char *key = NULL, *cmd;
    int key_len, cmd_len;
    long val = 1;

    if (zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), "Os|l",
                                     &object, redis_ce,
                                     &key, &key_len, &val) == FAILURE) {
        RETURN_FALSE;
    }

    if (redis_sock_get(object, &redis_sock TSRMLS_CC) < 0) {
        RETURN_FALSE;
    }
    if (val <= 1) {
        cmd_len = spprintf(&cmd, 0, "%s %s\r\n", keyword, key);
    } else {
        cmd_len = spprintf(&cmd, 0, "%sBY %s %d\r\n", keyword, key, (int)val);
    }

	REDIS_PROCESS_REQUEST(redis_sock, cmd, cmd_len);
    IF_ATOMIC() {
		redis_long_response(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock, NULL TSRMLS_CC);
    }
    REDIS_PROCESS_RESPONSE(redis_long_response);
}

/* {{{ proto boolean Redis::incr(string key [,int value])
 */
PHP_METHOD(Redis, incr)
{
    redis_atomic_increment(INTERNAL_FUNCTION_PARAM_PASSTHRU, "INCR" TSRMLS_CC);
}
/* }}} */

/* {{{ proto boolean Redis::decr(string key [,int value])
 */
PHP_METHOD(Redis, decr)
{
    redis_atomic_increment(INTERNAL_FUNCTION_PARAM_PASSTHRU, "DECR" TSRMLS_CC);
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
    int cmd_len, response_len, array_count;

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
											redis_sock, NULL TSRMLS_CC) < 0) {
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

    cmd_len = spprintf(&cmd, 0, "EXISTS %s\r\n", key);

    REDIS_PROCESS_REQUEST(redis_sock, cmd, cmd_len);
    IF_ATOMIC() {
	  redis_1_response(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock, NULL TSRMLS_CC);
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
                    1, &redis_sock TSRMLS_CC);
	zval * object = getThis();


    IF_ATOMIC() {
	  redis_long_response(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock, NULL TSRMLS_CC);
    }
	REDIS_PROCESS_RESPONSE(redis_long_response);

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

    cmd_len = spprintf(&cmd, 0, "KEYS %s\r\n", pattern);

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

    cmd_len = spprintf(&cmd, 0, "TYPE %s\r\n", key);

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

/* {{{ proto boolean Redis::lPush(string key , string value)
 */
PHP_METHOD(Redis, lPush)
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

    cmd_len = redis_cmd_format(&cmd, "LPUSH %s %d\r\n%s\r\n",
                               key, key_len,
                               val_len,
                               val, val_len);

	REDIS_PROCESS_REQUEST(redis_sock, cmd, cmd_len);
	IF_ATOMIC() {
		redis_long_response(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock, NULL TSRMLS_CC);
	} 
	REDIS_PROCESS_RESPONSE(redis_long_response);
}

/* {{{ proto boolean Redis::rPush(string key , string value)
 */
PHP_METHOD(Redis, rPush)
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

    cmd_len = redis_cmd_format(&cmd, "RPUSH %s %d\r\n%s\r\n",
                               key, key_len,
                               val_len,
                               val, val_len);

	REDIS_PROCESS_REQUEST(redis_sock, cmd, cmd_len);
	IF_ATOMIC() {
		redis_long_response(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock, NULL TSRMLS_CC);
	} 
	REDIS_PROCESS_RESPONSE(redis_long_response);
}
/* }}} */

/* {{{ proto string Redis::lPOP(string key)
 */
PHP_METHOD(Redis, lPop)
{
    zval *object;
    RedisSock *redis_sock;
    char *key = NULL, *cmd, *response;
    int key_len, cmd_len, response_len;
    long type = 0;

    if (zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), "Os",
                                     &object, redis_ce,
                                     &key, &key_len) == FAILURE) {
        RETURN_FALSE;
    }

    if (redis_sock_get(object, &redis_sock TSRMLS_CC) < 0) {
        RETURN_FALSE;
    }

    cmd_len = spprintf(&cmd, 0, "LPOP %s\r\n", key);

	REDIS_PROCESS_REQUEST(redis_sock, cmd, cmd_len);
	IF_ATOMIC() {
		redis_string_response(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock, NULL TSRMLS_CC);
	} 
	REDIS_PROCESS_RESPONSE(redis_string_response);
}

/* {{{ proto string Redis::rPOP(string key)
 */
PHP_METHOD(Redis, rPop)
{
    zval *object;
    RedisSock *redis_sock;
    char *key = NULL, *cmd, *response;
    int key_len, cmd_len, response_len;
    long type = 0;

    if (zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), "Os",
                                     &object, redis_ce,
                                     &key, &key_len) == FAILURE) {
        RETURN_FALSE;
    }

    if (redis_sock_get(object, &redis_sock TSRMLS_CC) < 0) {
        RETURN_FALSE;
    }

    cmd_len = spprintf(&cmd, 0, "RPOP %s\r\n", key);

	REDIS_PROCESS_REQUEST(redis_sock, cmd, cmd_len);
	IF_ATOMIC() {
		redis_string_response(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock, NULL TSRMLS_CC);
	} 
	REDIS_PROCESS_RESPONSE(redis_string_response);

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

    cmd_len = spprintf(&cmd, 0, "LLEN %s\r\n", key);

	REDIS_PROCESS_REQUEST(redis_sock, cmd, cmd_len);
	IF_ATOMIC() {
		redis_long_response(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock, NULL TSRMLS_CC);
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

    cmd_len = redis_cmd_format(&cmd, "LREM %s %d %d\r\n%s\r\n",
                       key, key_len,
                       count, 
                       val_len,
                       val, val_len);

	REDIS_PROCESS_REQUEST(redis_sock, cmd, cmd_len);
	IF_ATOMIC() {
		redis_long_response(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock, NULL TSRMLS_CC);
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
		redis_boolean_response(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock, NULL TSRMLS_CC);
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

    cmd_len = spprintf(&cmd, 0, "LINDEX %s %d\r\n", key, (int)index);

	REDIS_PROCESS_REQUEST(redis_sock, cmd, cmd_len);
	IF_ATOMIC() {
		redis_string_response(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock, NULL TSRMLS_CC);
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

    cmd_len = spprintf(&cmd, 0, "LRANGE %s %d %d\r\n\r\n", key, (int)start, (int)end);

	REDIS_PROCESS_REQUEST(redis_sock, cmd, cmd_len);
	IF_ATOMIC() {
		redis_sock_read_multibulk_reply(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock, NULL TSRMLS_CC);
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

    cmd_len = redis_cmd_format(&cmd, "SADD %s %d\r\n%s\r\n",
                    key, key_len,
                    val_len, 
                    val, val_len);

	REDIS_PROCESS_REQUEST(redis_sock, cmd, cmd_len);
	IF_ATOMIC() {
		redis_1_response(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock, NULL TSRMLS_CC);	
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

    cmd_len = spprintf(&cmd, 0, "SCARD %s\r\n", key);

	REDIS_PROCESS_REQUEST(redis_sock, cmd, cmd_len);
	IF_ATOMIC() {
		redis_long_response(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock, NULL TSRMLS_CC);
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

    cmd_len = redis_cmd_format(&cmd, "SREM %s %d\r\n%s\r\n",
                               key, key_len,
                               val_len, 
                               val, val_len);

	REDIS_PROCESS_REQUEST(redis_sock, cmd, cmd_len);
	IF_ATOMIC() {
		redis_1_response(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock, NULL TSRMLS_CC);
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

    cmd_len = redis_cmd_format(&cmd, "SMOVE %s %s %d\r\n%s\r\n",
                               src, src_len,
                               dst, dst_len,
                               val_len,
                               val, val_len);

	REDIS_PROCESS_REQUEST(redis_sock, cmd, cmd_len);
	IF_ATOMIC() {
	  redis_1_response(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock, NULL TSRMLS_CC);
	}
	REDIS_PROCESS_RESPONSE(redis_1_response);
}
/* }}} */

/* }}} */
/* {{{ proto string Redis::sPop(string key)
 */
PHP_METHOD(Redis, sPop)
{
    zval *object;
    RedisSock *redis_sock;
    char *key = NULL, *cmd, *response;
    int key_len, cmd_len, response_len;

    if (zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), "Os",
                                     &object, redis_ce,
                                     &key, &key_len) == FAILURE) {
        RETURN_FALSE;
    }

    if (redis_sock_get(object, &redis_sock TSRMLS_CC) < 0) {
        RETURN_FALSE;
    }

    cmd_len = redis_cmd_format(&cmd, "SPOP %s\r\n", key, key_len);

	REDIS_PROCESS_REQUEST(redis_sock, cmd, cmd_len);
	IF_ATOMIC() {
		redis_string_response(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock, NULL TSRMLS_CC);
	} 
	REDIS_PROCESS_RESPONSE(redis_string_response);
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

    cmd_len = redis_cmd_format(&cmd, "SISMEMBER %s %d\r\n%s\r\n",
                               key, key_len,
                               val_len, 
                               val, val_len);

	REDIS_PROCESS_REQUEST(redis_sock, cmd, cmd_len);
	IF_ATOMIC() {
		redis_1_response(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock, NULL TSRMLS_CC);
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

    cmd_len = spprintf(&cmd, 0, "SMEMBERS %s\r\n", key);

	REDIS_PROCESS_REQUEST(redis_sock, cmd, cmd_len);
	IF_ATOMIC() {
	    if (redis_sock_read_multibulk_reply(INTERNAL_FUNCTION_PARAM_PASSTHRU,
    	                                    redis_sock, NULL TSRMLS_CC) < 0) {
        	RETURN_FALSE;
	    }
	}
    REDIS_PROCESS_RESPONSE(redis_sock_read_multibulk_reply);
}
/* }}} */


PHPAPI int generic_multiple_args_cmd(INTERNAL_FUNCTION_PARAMETERS, char *keyword, int keyword_len,
                int min_argc, RedisSock **out_sock TSRMLS_DC)
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

            char *key, *val;
            int key_len, val_len;
            unsigned long idx;
            int type;
            zval **z_value_pp;

            type = zend_hash_get_current_key_ex(keytable, &key, &key_len, &idx, 0, NULL);
            if(zend_hash_get_current_data(keytable, (void**)&z_value_pp) == FAILURE) {
                continue; 	/* this should never happen, according to the PHP people. */
            }

            // get current value
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
    cmd = emalloc(cmd_len);

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
	cmd[cmd_len] = '\0';
    efree(keys);
    efree(keys_len);
    if(z_args) efree(z_args);

	object = getThis();
    printf("cmd=%p\n", (void*)cmd);
    REDIS_PROCESS_REQUEST(redis_sock, cmd, cmd_len);

}

/* {{{ proto array Redis::sInter(string key0, ... string keyN)
 */
PHP_METHOD(Redis, sInter) {

    int response_len;
    RedisSock *redis_sock;

    generic_multiple_args_cmd(INTERNAL_FUNCTION_PARAM_PASSTHRU,
                    "SINTER", sizeof("SINTER") - 1,
                    0, &redis_sock TSRMLS_CC);

	zval *object = getThis();

    IF_ATOMIC() {
    	if (redis_sock_read_multibulk_reply(INTERNAL_FUNCTION_PARAM_PASSTHRU,
											redis_sock, NULL TSRMLS_CC) < 0) {
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
                    1, &redis_sock TSRMLS_CC);

	zval *object = getThis();

	IF_ATOMIC() {
		redis_long_response(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock, NULL TSRMLS_CC);
	}
    REDIS_PROCESS_RESPONSE(redis_long_response);


}
/* }}} */

/* {{{ proto array Redis::sUnion(string key0, ... string keyN)
 */
PHP_METHOD(Redis, sUnion) {

    int response_len;
    RedisSock *redis_sock;

    generic_multiple_args_cmd(INTERNAL_FUNCTION_PARAM_PASSTHRU,
                    "SUNION", sizeof("SUNION") - 1,
                    0, &redis_sock TSRMLS_CC);
	zval *object = getThis();

	IF_ATOMIC() {
    	if (redis_sock_read_multibulk_reply(INTERNAL_FUNCTION_PARAM_PASSTHRU,
        	                                redis_sock, NULL TSRMLS_CC) < 0) {
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
                    1, &redis_sock TSRMLS_CC);
	zval *object = getThis();

	IF_ATOMIC() {
		redis_long_response(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock, NULL TSRMLS_CC);
	}
	REDIS_PROCESS_RESPONSE(redis_long_response);
}

/* }}} */

/* {{{ proto array Redis::sDiff(string key0, ... string keyN)
 */
PHP_METHOD(Redis, sDiff) {

    int response_len;
    RedisSock *redis_sock;

    generic_multiple_args_cmd(INTERNAL_FUNCTION_PARAM_PASSTHRU,
                    "SDiff", sizeof("SDiff") - 1,
                    0, &redis_sock TSRMLS_CC);

	zval *object = getThis();

	IF_ATOMIC() {
	    /* read multibulk reply */
    	if (redis_sock_read_multibulk_reply(INTERNAL_FUNCTION_PARAM_PASSTHRU,
											redis_sock, NULL TSRMLS_CC) < 0) {
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
                    1, &redis_sock TSRMLS_CC);
	zval *object = getThis();

	IF_ATOMIC() {
	  redis_long_response(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock, NULL TSRMLS_CC);
	}
	REDIS_PROCESS_RESPONSE(redis_long_response);
}
/* }}} */


PHPAPI void generic_sort_cmd(INTERNAL_FUNCTION_PARAMETERS, char *sort, int use_alpha TSRMLS_DC) {

    zval *object;
    RedisSock *redis_sock;
    char *key = NULL, *pattern = NULL, *get = NULL, *cmd;
    int key_len, pattern_len = -1, get_len = -1, cmd_len, response_len;
    long start = -1, end = -1;

    long use_pound = 0;

    char *by_cmd = "";
    char *by_arg = "";

    char *get_cmd = "";
    char *get_arg = "";
    char *get_pound = "";

    char *limit = "";

    char *alpha = "";

    if (zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), "Os|ssllb",
                                     &object, redis_ce,
                                     &key, &key_len, &pattern, &pattern_len,
                                     &get, &get_len, &start, &end, &use_pound) == FAILURE) {
        RETURN_FALSE;
    }

    if (redis_sock_get(object, &redis_sock TSRMLS_CC) < 0) {
        RETURN_FALSE;
    }
    if(key_len == 0) {
        RETURN_FALSE;
    }

    if(start >= 0 && end >= start) {
        redis_cmd_format(&limit, "LIMIT %d %d", start, end);
    }

    char format[] = "SORT "
            "%s " /* key, mandatory */
            "%s " /* BY, optional */
            "%s " /* argument for BY, optional */
            "%s " /* GET, optional */
            "%s " /* argument for GET, optional */
            "%s " /* "#" argument for GET, optional */
            "%s " /* LIMIT, optional. full string in this format: "LIMIT 42, 100" */
            "%s " /* ALPHA, optional */
            "%s " /* ASC or DESC, optional */
            "\r\n";

    if(pattern && pattern_len) {
        by_cmd = "BY";
        by_arg = pattern;
    }
    if(get && get_len) {
        get_cmd = "GET";
        get_arg = get;
        get_pound = use_pound ? "#" : "";
    }
    if(start >= 0 && end >= start) {
        spprintf(&limit, 0, "LIMIT %ld %ld", start, end);
    }
    if(use_alpha) {
        alpha = "ALPHA";
    }

    cmd_len = spprintf(&cmd, 0, format, key, by_cmd, by_arg, get_cmd, get_arg, get_pound, limit, alpha, sort);

    if(*limit) {
        efree(limit);
    }

	REDIS_PROCESS_REQUEST(redis_sock, cmd, cmd_len);
	IF_ATOMIC() {
    	if (redis_sock_read_multibulk_reply(INTERNAL_FUNCTION_PARAM_PASSTHRU,
											redis_sock, NULL TSRMLS_CC) < 0) {
        	RETURN_FALSE;
	    }
	}
	REDIS_PROCESS_RESPONSE(redis_sock_read_multibulk_reply);

}

/* {{{ proto array Redis::sortAsc(string key, string pattern, string get, int start, int end, bool getList])
 */
PHP_METHOD(Redis, sortAsc)
{
    generic_sort_cmd(INTERNAL_FUNCTION_PARAM_PASSTHRU, "ASC", 0 TSRMLS_CC);
}
/* }}} */

/* {{{ proto array Redis::sortAscAlpha(string key, string pattern, string get, int start, int end, bool getList])
 */
PHP_METHOD(Redis, sortAscAlpha)
{
    generic_sort_cmd(INTERNAL_FUNCTION_PARAM_PASSTHRU, "ASC", 1 TSRMLS_CC);
}
/* }}} */

/* {{{ proto array Redis::sortDesc(string key, string pattern, string get, int start, int end, bool getList])
 */
PHP_METHOD(Redis, sortDesc)
{
    generic_sort_cmd(INTERNAL_FUNCTION_PARAM_PASSTHRU, "DESC", 0 TSRMLS_CC);
}
/* }}} */

/* {{{ proto array Redis::sortDescAlpha(string key, string pattern, string get, int start, int end, bool getList])
 */
PHP_METHOD(Redis, sortDescAlpha)
{
    generic_sort_cmd(INTERNAL_FUNCTION_PARAM_PASSTHRU, "DESC", 1 TSRMLS_CC);
}
/* }}} */

PHPAPI void generic_expire_cmd(INTERNAL_FUNCTION_PARAMETERS, char *keyword, int keyword_len TSRMLS_DC) {
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

    cmd_len = redis_cmd_format(&cmd, "%s %s %d\r\n",
        keyword, keyword_len,
        key, key_len,
        (int)t);

	REDIS_PROCESS_REQUEST(redis_sock, cmd, cmd_len);
	IF_ATOMIC() {
		redis_1_response(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock, NULL TSRMLS_CC);
	} 
	REDIS_PROCESS_RESPONSE(redis_1_response);
}

/* {{{ proto array Redis::setTimeout(string key, int timeout)
 */
PHP_METHOD(Redis, setTimeout) {
    generic_expire_cmd(INTERNAL_FUNCTION_PARAM_PASSTHRU, "EXPIRE", sizeof("EXPIRE")-1 TSRMLS_CC);
}
/* }}} */

/* {{{ proto array Redis::expireAt(string key, int timestamp)
 */
PHP_METHOD(Redis, expireAt) {
    generic_expire_cmd(INTERNAL_FUNCTION_PARAM_PASSTHRU, "EXPIREAT", sizeof("EXPIREAT")-1 TSRMLS_CC);
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

    cmd_len = redis_cmd_format(&cmd, "LSET %s %d %d\r\n%s\r\n",
                               key, key_len,
                               (int)index,
                               val_len,
                               val, val_len);

	REDIS_PROCESS_REQUEST(redis_sock, cmd, cmd_len);
	IF_ATOMIC() {
		redis_boolean_response(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock, NULL TSRMLS_CC);
	}
	REDIS_PROCESS_RESPONSE(redis_boolean_response);
}
/* }}} */


PHPAPI void generic_empty_cmd(INTERNAL_FUNCTION_PARAMETERS, char *cmd, int cmd_len TSRMLS_DC, ...) {
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

	IF_MULTI_OR_ATOMIC() {
		if (redis_sock_write(redis_sock, cmd, cmd_len) < 0) {
			/* no efree here, cmd is an argument. */
			RETURN_FALSE;
		}
	}
	IF_PIPELINE() {
		PIPELINE_ENQUEUE_COMMAND(cmd, cmd_len);
	}	

	MULTI_RESPONSE(redis_boolean_response);
	IF_ATOMIC() {
	  redis_boolean_response(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock, NULL TSRMLS_CC);
	}
	REDIS_PROCESS_RESPONSE(redis_boolean_response);
}

/* {{{ proto string Redis::save()
 */
PHP_METHOD(Redis, save)
{
    generic_empty_cmd(INTERNAL_FUNCTION_PARAM_PASSTHRU, "SAVE\r\n", sizeof("SAVE\r\n")-1 TSRMLS_CC);

}
/* }}} */

/* {{{ proto string Redis::bgSave()
 */
PHP_METHOD(Redis, bgSave)
{
    generic_empty_cmd(INTERNAL_FUNCTION_PARAM_PASSTHRU, "BGSAVE\r\n", sizeof("BGSAVE\r\n")-1 TSRMLS_CC);

}
/* }}} */

PHPAPI void generic_empty_long_cmd(INTERNAL_FUNCTION_PARAMETERS, char *cmd, int cmd_len TSRMLS_DC, ...) {

    zval *object;
    RedisSock *redis_sock;

    if (zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), "O",
                                     &object, redis_ce) == FAILURE) {
        RETURN_FALSE;
    }

    if (redis_sock_get(object, &redis_sock TSRMLS_CC) < 0) {
        RETURN_FALSE;
    }

	IF_MULTI_OR_ATOMIC() {
		if (redis_sock_write(redis_sock, cmd, cmd_len) < 0) {
			/* no efree here, cmd is an argument. */
			RETURN_FALSE;
		}
	}
	IF_PIPELINE() {
		PIPELINE_ENQUEUE_COMMAND(cmd, cmd_len);
	}	

	MULTI_RESPONSE(redis_long_response);
	IF_ATOMIC() {
		redis_long_response(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock, NULL TSRMLS_CC);
	} 
	ELSE_IF_MULTI()
	ELSE_IF_PIPELINE();

}

/* {{{ proto integer Redis::lastSave()
 */
PHP_METHOD(Redis, lastSave)
{
    generic_empty_long_cmd(INTERNAL_FUNCTION_PARAM_PASSTHRU, "LASTSAVE\r\n", sizeof("LASTSAVE\r\n") TSRMLS_CC);
}
/* }}} */


/* {{{ proto bool Redis::flushDB()
 */
PHP_METHOD(Redis, flushDB)
{
    generic_empty_cmd(INTERNAL_FUNCTION_PARAM_PASSTHRU, "FLUSHDB\r\n", sizeof("FLUSHDB\r\n")-1 TSRMLS_CC);
}
/* }}} */

/* {{{ proto bool Redis::flushAll()
 */
PHP_METHOD(Redis, flushAll)
{
    generic_empty_cmd(INTERNAL_FUNCTION_PARAM_PASSTHRU, "FLUSHALL\r\n", sizeof("FLUSHALL\r\n")-1 TSRMLS_CC);
}
/* }}} */

/* {{{ proto int Redis::dbSize()
 */
PHP_METHOD(Redis, dbSize)
{
    generic_empty_long_cmd(INTERNAL_FUNCTION_PARAM_PASSTHRU, "DBSIZE\r\n", sizeof("DBSIZE\r\n") TSRMLS_CC);
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

    cmd_len = redis_cmd_format(&cmd, "AUTH %s\r\n", password, password_len);

	REDIS_PROCESS_REQUEST(redis_sock, cmd, cmd_len);
	IF_ATOMIC() {
		redis_boolean_response(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock, NULL TSRMLS_CC);
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

    cmd_len = redis_cmd_format(&cmd, "TTL %s\r\n", key, key_len);

	REDIS_PROCESS_REQUEST(redis_sock, cmd, cmd_len);
	IF_ATOMIC() {
	  redis_long_response(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock, NULL TSRMLS_CC);
	} 
	REDIS_PROCESS_RESPONSE(redis_long_response);
}
/* }}} */

/* {{{ proto array Redis::info()
 */
PHP_METHOD(Redis, info) {

    zval *object;
    RedisSock *redis_sock;

    char cmd[] = "INFO\r\n", *response, *key;
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

    cmd_len = redis_cmd_format(&cmd, "SELECT %d\r\n", dbNumber);

	REDIS_PROCESS_REQUEST(redis_sock, cmd, cmd_len);
	IF_ATOMIC() {
		redis_boolean_response(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock, NULL TSRMLS_CC);
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

    cmd_len = redis_cmd_format(&cmd, "MOVE %s %d\r\n",
                               key, key_len, dbNumber);

	REDIS_PROCESS_REQUEST(redis_sock, cmd, cmd_len);
	IF_ATOMIC() {
	  redis_1_response(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock, NULL TSRMLS_CC);
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
		redis_boolean_response(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock, NULL TSRMLS_CC);
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

    cmd_len = redis_cmd_format(&cmd, "RPOPLPUSH %s %s\r\n",
                               srckey, srckey_len,
                               dstkey, dstkey_len);

	REDIS_PROCESS_REQUEST(redis_sock, cmd, cmd_len);
	IF_ATOMIC() {
		redis_string_response(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock, NULL TSRMLS_CC);
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

    cmd_len = redis_cmd_format(&cmd, "ZADD %s %f %d\r\n%s\r\n",
                               key, key_len,
                               score,
                               val_len,
                               val, val_len);

	REDIS_PROCESS_REQUEST(redis_sock, cmd, cmd_len);
	IF_ATOMIC() {
		redis_long_response(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock, NULL TSRMLS_CC);
	} 
	REDIS_PROCESS_RESPONSE(redis_long_response);

}
/* }}} */
/* {{{ proto array Redis::zRange(string key, int start , int end)
 */
PHP_METHOD(Redis, zRange)
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

    cmd_len = redis_cmd_format(&cmd, "ZRANGE %s %d %d\r\n\r\n", key, key_len, start, end);

	REDIS_PROCESS_REQUEST(redis_sock, cmd, cmd_len);	
	IF_ATOMIC() {
	    if (redis_sock_read_multibulk_reply(INTERNAL_FUNCTION_PARAM_PASSTHRU,
    	                                    redis_sock, NULL TSRMLS_CC) < 0) {
	       	RETURN_FALSE;
    	}
	} 
	REDIS_PROCESS_RESPONSE(redis_sock_read_multibulk_reply);
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

    cmd_len = redis_cmd_format(&cmd, "ZREM %s %d\r\n%s\r\n", key, key_len, member_len, member, member_len);

	REDIS_PROCESS_REQUEST(redis_sock, cmd, cmd_len);
	IF_ATOMIC() {
		redis_long_response(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock, NULL TSRMLS_CC);
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

    cmd_len = spprintf(&cmd, 0, "ZREMRANGEBYSCORE %s %f %f\r\n\r\n", key, start, end);

	REDIS_PROCESS_REQUEST(redis_sock, cmd, cmd_len);
	IF_ATOMIC() {
		redis_long_response(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock, NULL TSRMLS_CC);
	}
	REDIS_PROCESS_RESPONSE(redis_long_response);

}
/* }}} */
/* {{{ proto array Redis::zReverseRange(string key, int start , int end)
 */
PHP_METHOD(Redis, zReverseRange)
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

    cmd_len = spprintf(&cmd, 0, "ZREVRANGE %s %f %f\r\n\r\n", key, start, end);

	REDIS_PROCESS_REQUEST(redis_sock, cmd, cmd_len);
	IF_ATOMIC() {
	    if (redis_sock_read_multibulk_reply(INTERNAL_FUNCTION_PARAM_PASSTHRU,
    	                                    redis_sock, NULL TSRMLS_CC) < 0) {
        	RETURN_FALSE;
	    }
	}
	REDIS_PROCESS_RESPONSE(redis_sock_read_multibulk_reply);

}
/* }}} */
/* {{{ proto array Redis::zRangeByScore(string key, int start , int end [,array options = NULL])
 */
PHP_METHOD(Redis, zRangeByScore)
{
    zval *object, *z_options = NULL, **z_limit_val_pp = NULL, **z_withscores_val_pp = NULL;

    RedisSock *redis_sock;
    char *key = NULL, *limit = NULL, *cmd;
    int key_len, cmd_len, response_len;
    zend_bool withscores = 0;
    double start, end;

    if (zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), "Osdd|a",
                                     &object, redis_ce,
                                     &key, &key_len, &start, &end, &z_options) == FAILURE) {
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

                    spprintf(&limit, 0, " LIMIT %ld %ld", Z_LVAL_PP(z_offset_pp), Z_LVAL_PP(z_count_pp));
                }
            }
        }
    }

    if(withscores) {
        cmd_len = spprintf(&cmd, 0, "ZRANGEBYSCORE %s %f %f%s WITHSCORES\r\n\r\n", key, start, end, limit?limit:"");
    } else {
        cmd_len = spprintf(&cmd, 0, "ZRANGEBYSCORE %s %f %f%s\r\n\r\n", key, start, end, limit?limit:"");
    }
    if(limit) {
        efree(limit);
    }

    if (redis_sock_write(redis_sock, cmd, cmd_len) < 0) {
        efree(cmd);
        RETURN_FALSE;
    }
    efree(cmd);

    if (redis_sock_read_multibulk_reply(INTERNAL_FUNCTION_PARAM_PASSTHRU,
                                        redis_sock, NULL TSRMLS_CC) < 0) {
        RETURN_FALSE;
    }

    if(!withscores) {
        return;
    }
    /* with scores! we have to transform the return array.
     * return_value currently holds this: [elt0, val0, elt1, val1 ... ]
     * we want [elt0 => val0, elt1 => val1], etc.
     */
	IF_NOT_MULTI() {
	    array_zip_values_and_scores(INTERNAL_FUNCTION_PARAM_PASSTHRU, 1);
	} ELSE_IF_MULTI()
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

    cmd_len = spprintf(&cmd, 0, "ZCARD %s\r\n", key);

	REDIS_PROCESS_REQUEST(redis_sock, cmd, cmd_len);
	IF_ATOMIC() {
	  redis_long_response(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock, NULL TSRMLS_CC);
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
    int key_len, member_len, cmd_len, count;

    if (zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), "Oss",
                                     &object, redis_ce, &key, &key_len,
                                     &member, &member_len) == FAILURE) {
        RETURN_FALSE;
    }

    if (redis_sock_get(object, &redis_sock TSRMLS_CC) < 0) {
        RETURN_FALSE;
    }

    cmd_len = redis_cmd_format(&cmd, "ZSCORE %s %d\r\n%s\r\n", key, key_len, member_len, member, member_len);

	REDIS_PROCESS_REQUEST(redis_sock, cmd, cmd_len);
	IF_ATOMIC() {
	    redis_bulk_double_response(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock TSRMLS_CC);
	} 
	REDIS_PROCESS_RESPONSE(redis_bulk_double_response);

}

PHPAPI void generic_incrby_method(INTERNAL_FUNCTION_PARAMETERS, char *keyword, int keyword_len TSRMLS_DC) {
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
    cmd_len = redis_cmd_format(&cmd, "%s %s %f %d\r\n%s\r\n",
                    keyword, keyword_len,
                    key, key_len, val, member_len, member, member_len);

	REDIS_PROCESS_REQUEST(redis_sock, cmd, cmd_len);
	IF_ATOMIC() {
	    redis_bulk_double_response(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock TSRMLS_CC);
	} 
	REDIS_PROCESS_RESPONSE(redis_bulk_double_response);

}

/* {{{ proto double Redis::zIncrBy(string key, double value, string member)
 */
PHP_METHOD(Redis, zIncrBy)
{
    generic_incrby_method(INTERNAL_FUNCTION_PARAM_PASSTHRU, "ZINCRBY", sizeof("ZINCRBY")-1 TSRMLS_CC);
}
/* }}} */
PHPAPI void generic_z_command(INTERNAL_FUNCTION_PARAMETERS, char *command TSRMLS_DC) {

	zval *object, *keys_array, *weights_array = NULL, **data;
	HashTable *arr_weights_hash, *arr_keys_hash;
	int key_output_len, array_weights_count, array_keys_count, operation_len = 0;
	char *key_output, *operation;
	RedisSock *redis_sock;

	HashPosition pointer;
	char *cmd = "";
	int cmd_len, response_len, array_count;

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

	cmd_len = spprintf(&cmd, 0, "%s %s %d", command, key_output, array_keys_count);

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
            cmd_len = spprintf(&cmd, 0, "%s %s", cmd, Z_STRVAL_PP(data));
            if(old_cmd) {
                efree(old_cmd);
            }
        }
    }

	/* weight */
	if(weights_array != NULL) {
		cmd_len = spprintf(&cmd, 0, "%s WEIGHTS", cmd);
		for (zend_hash_internal_pointer_reset_ex(arr_weights_hash, &pointer);
			zend_hash_get_current_data_ex(arr_weights_hash, (void**) &data, &pointer) == SUCCESS;
			zend_hash_move_forward_ex(arr_weights_hash, &pointer)) {
		
			if (Z_TYPE_PP(data) == IS_LONG) {
				char *old_cmd = NULL;
				if(*cmd) {
					old_cmd = cmd;
				}
				cmd_len = spprintf(&cmd, 0, "%s %ld", cmd, Z_LVAL_PP(data));
				if(old_cmd) {
					efree(old_cmd);
				}
			} else {
				/* error */
				free(cmd);
				RETURN_FALSE;
			}
		}
	}
	
 	if(operation_len != 0) { 
		char *old_cmd = NULL;
		old_cmd = cmd;
 		cmd_len = spprintf(&cmd, 0, "%s AGGREGATE %s", cmd, operation); 
		efree(old_cmd);
	} 

	char *old_cmd = cmd;
	cmd_len = spprintf(&cmd, 0, "%s \r\n", cmd);
	efree(old_cmd);

	REDIS_PROCESS_REQUEST(redis_sock, cmd, cmd_len);
	IF_ATOMIC() {
	  redis_long_response(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock, NULL TSRMLS_CC);
	} 
	REDIS_PROCESS_RESPONSE(redis_long_response);

}

/* zInter */
PHP_METHOD(Redis, zInter) {						
	generic_z_command(INTERNAL_FUNCTION_PARAM_PASSTHRU, "zInter" TSRMLS_CC);
}

/* zUnion */
PHP_METHOD(Redis, zUnion) {
	generic_z_command(INTERNAL_FUNCTION_PARAM_PASSTHRU, "zUnion" TSRMLS_CC);
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
	  redis_1_response(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock, NULL TSRMLS_CC);
	}
	REDIS_PROCESS_RESPONSE(redis_1_response);
}
/* }}} */

/* hSet */
PHP_METHOD(Redis, hGet)
{
    zval *object;
    RedisSock *redis_sock;
    char *key = NULL, *cmd, *member, *response;
    int key_len, member_len, cmd_len, response_len;

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
		redis_string_response(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock, NULL TSRMLS_CC);
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
	  redis_long_response(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock, NULL TSRMLS_CC);
	}
	REDIS_PROCESS_RESPONSE(redis_long_response);

}
/* }}} */

PHPAPI RedisSock*
generic_hash_command_2(INTERNAL_FUNCTION_PARAMETERS, char *keyword, int keyword_len, char **out_cmd, int *out_len TSRMLS_DC) {

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
    RedisSock *redis_sock = generic_hash_command_2(INTERNAL_FUNCTION_PARAM_PASSTHRU, "HDEL", 4, &cmd, &cmd_len TSRMLS_CC);
	zval *object = getThis();

	REDIS_PROCESS_REQUEST(redis_sock, cmd, cmd_len);
	IF_ATOMIC() {
		redis_1_response(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock, NULL TSRMLS_CC);
	}
	REDIS_PROCESS_RESPONSE(redis_1_response);

}

/* hExists */
PHP_METHOD(Redis, hExists)
{
    char *cmd;
    int cmd_len;
    RedisSock *redis_sock = generic_hash_command_2(INTERNAL_FUNCTION_PARAM_PASSTHRU, "HEXISTS", 7, &cmd, &cmd_len TSRMLS_CC);

	zval *object = getThis();

	REDIS_PROCESS_REQUEST(redis_sock, cmd, cmd_len);
	IF_ATOMIC() {
	  redis_1_response(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock, NULL TSRMLS_CC);
	} 
	REDIS_PROCESS_RESPONSE(redis_1_response);

}

PHPAPI RedisSock*
generic_hash_command_1(INTERNAL_FUNCTION_PARAMETERS, char *keyword, int keyword_len TSRMLS_DC) {

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
    RedisSock *redis_sock = generic_hash_command_1(INTERNAL_FUNCTION_PARAM_PASSTHRU, "HKEYS", sizeof("HKEYS")-1 TSRMLS_CC);
	zval *object = getThis();

	IF_ATOMIC() {
	    if (redis_sock_read_multibulk_reply(INTERNAL_FUNCTION_PARAM_PASSTHRU,
											redis_sock, NULL TSRMLS_CC) < 0) {
    	    RETURN_FALSE;
	    }
	}
	REDIS_PROCESS_RESPONSE(redis_sock_read_multibulk_reply);


}
/* hVals */
PHP_METHOD(Redis, hVals)
{
    RedisSock *redis_sock = generic_hash_command_1(INTERNAL_FUNCTION_PARAM_PASSTHRU, "HVALS", sizeof("HVALS")-1 TSRMLS_CC);
	zval *object = getThis();

	IF_ATOMIC() {
	    if (redis_sock_read_multibulk_reply(INTERNAL_FUNCTION_PARAM_PASSTHRU,
    	                                    redis_sock, NULL TSRMLS_CC) < 0) {
        	RETURN_FALSE;
	    }
	} 
	REDIS_PROCESS_RESPONSE(redis_sock_read_multibulk_reply);

}


PHP_METHOD(Redis, hGetAll) {

    RedisSock *redis_sock = generic_hash_command_1(INTERNAL_FUNCTION_PARAM_PASSTHRU, "HGETALL", sizeof("HGETALL")-1 TSRMLS_CC);

	zval *object = getThis();

	IF_ATOMIC() {
	    if (redis_sock_read_multibulk_reply(INTERNAL_FUNCTION_PARAM_PASSTHRU,
    	                                    redis_sock, NULL TSRMLS_CC) < 0) {
        	RETURN_FALSE;
	    }

    	/* return_value now holds an array in the following format:
	     * [k0, v0, k1, v1, k2, v2]
    	 *
	     * The following call takes care of all this.
    	 */
	    array_zip_values_and_scores(INTERNAL_FUNCTION_PARAM_PASSTHRU, 0);
	} 
	REDIS_PROCESS_RESPONSE(array_zip_values_and_scores);
}

PHPAPI void array_zip_values_and_scores(INTERNAL_FUNCTION_PARAMETERS, int use_atof TSRMLS_DC) {

    zval *z_ret;
    MAKE_STD_ZVAL(z_ret);
    *z_ret = *return_value; /* copy */
    array_init(return_value);

    HashTable *keytable = Z_ARRVAL_P(z_ret);
	int i = 0;
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

        // get current value, a key
        hkey = Z_STRVAL_PP(z_value_pp);
        hkey_len = Z_STRLEN_PP(z_value_pp);

        // move forward
        zend_hash_move_forward(keytable);

        // fetch again
        type = zend_hash_get_current_key_ex(keytable, &tablekey, &tablekey_len, &idx, 0, NULL);
        if(zend_hash_get_current_data(keytable, (void**)&z_value_pp) == FAILURE) {
            continue; 	/* this should never happen, according to the PHP people. */
        }

        // get current value, a hash value now.
        hval = Z_STRVAL_PP(z_value_pp);
        hval_len = Z_STRLEN_PP(z_value_pp);

        if(use_atof) {
            add_assoc_double_ex(return_value, hkey, 1+hkey_len, atof(hval));
        } else {
            add_assoc_stringl_ex(return_value, hkey, 1+hkey_len, hval, hval_len, 1);
        }
    }
    zval_dtor(z_ret);
    efree(z_ret);
}

PHP_METHOD(Redis, hIncrBy)
{
    generic_incrby_method(INTERNAL_FUNCTION_PARAM_PASSTHRU, "HINCRBY", sizeof("HINCRBY")-1 TSRMLS_CC);
}

PHPAPI int redis_response_enqueued(RedisSock *redis_sock TSRMLS_DC) {

	char *response, *ret;
	int response_len;

	if ((response = redis_sock_read(redis_sock, &response_len TSRMLS_CC)) == NULL) {
		return 0;
    }

    if(response[0] == '+') {
		if(strncmp(response, "QUEUED", 6 )) {
			return 1;
		} else {
			return 1;
		}
    } else {
		efree(response);
		return 0;
	}
}

/* flag : get, set {REDIS_ATOMIC, REDIS_MULTI, REDIS_PIPELINE} */

PHP_METHOD(Redis, multi)
{

    RedisSock *redis_sock;
    char *cmd;
	int response_len, cmd_len;
	char * response;
	zval *object;
	double multi_value;

	if (zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), "Od",
                                     &object, redis_ce, &multi_value) == FAILURE) {
        RETURN_FALSE;
    }

	// if the flag is activated, send the command, the reply will be "QUEUED" or -ERR

    if (redis_sock_get(object, &redis_sock TSRMLS_CC) < 0) {
        RETURN_FALSE;
    }

	if(multi_value == REDIS_MULTI || multi_value == REDIS_PIPELINE) {
		set_flag(object, multi_value);
	} else {
		php_printf("error[%f] !! \n", multi_value);
		exit(-1);
	}

    /*
	head = malloc(sizeof(fold_item));
	current = head;
	current->function_name = strdup("__begin__");
    */
    current = NULL;
	
	IF_MULTI() {
	    cmd_len = redis_cmd_format(&cmd, "MULTI \r\n");

    	if (redis_sock_write(redis_sock, cmd, cmd_len) < 0) {
        	efree(cmd);
	        RETURN_FALSE;
    	}
	    efree(cmd);

    	if ((response = redis_sock_read(redis_sock, &response_len TSRMLS_CC)) == NULL) {
        	RETURN_FALSE;
    	}

		if(response_len == 3 & response[0] == '+' && response[1] == 'O' && response[2] == 'K') {
			RETURN_ZVAL(getThis(), 1, 0);
		}
		RETURN_FALSE;
	}
	IF_PIPELINE() {
            /*
		head_request = malloc(sizeof(request_item));
		current_request = head_request;		
		current_request->function_name = strdup("__begin__");
		current_request->request_str = NULL;
        */
        head_request = current_request = NULL;
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

	set_flag(object, 0);

    if (redis_sock_get(object, &redis_sock TSRMLS_CC) < 0) {
        RETURN_FALSE;
    }

    cmd_len = redis_cmd_format(&cmd, "DISCARD \r\n");

    if (redis_sock_write(redis_sock, cmd, cmd_len) < 0) {
        efree(cmd);
        RETURN_FALSE;
    }
    efree(cmd);

    if ((response = redis_sock_read(redis_sock, &response_len TSRMLS_CC)) == NULL) {
        RETURN_FALSE;
    }

	if(response_len == 3 & response[0] == '+' && response[1] == 'O' && response[2] == 'K') {
		RETURN_TRUE;
	}
	RETURN_FALSE;
}
PHPAPI int redis_sock_read_multibulk_pipeline_reply(INTERNAL_FUNCTION_PARAMETERS,
                                      RedisSock *redis_sock TSRMLS_DC)
{

    zval *z_tab;
    MAKE_STD_ZVAL(z_tab);
    array_init(z_tab);

    redis_sock_read_multibulk_multi_reply_loop(INTERNAL_FUNCTION_PARAM_PASSTHRU,
                    redis_sock, z_tab, NULL);

    *return_value = *z_tab;

	// free allocated function/request memory	
	fold_item *tmp1;
	current = head;

	while(current) {
		tmp1 = current;
		current = current->next;
		free(tmp1);
	}
	
	request_item *tmp;
	current_request = head_request;
	while(current_request) {
		tmp = current_request;
		current_request = current_request->next;
		free(tmp);
	}
	
    return 0;
	
}
/* redis_sock_read_multibulk_multi_reply */
PHPAPI int redis_sock_read_multibulk_multi_reply(INTERNAL_FUNCTION_PARAMETERS,
                                      RedisSock *redis_sock TSRMLS_DC)
{

    char inbuf[1024], *response;
    int response_len;

    redis_check_eof(redis_sock TSRMLS_CC);

    php_stream_gets(redis_sock->stream, inbuf, 1024);
    if(inbuf[0] != '*') {
        return -1;
    }

	/* number of responses */
    int numElems = atoi(inbuf+1);
    zval *z_tab;
    MAKE_STD_ZVAL(z_tab);
    array_init(z_tab);

    redis_sock_read_multibulk_multi_reply_loop(INTERNAL_FUNCTION_PARAM_PASSTHRU,
                    redis_sock, z_tab, numElems);

    *return_value = *z_tab;
    return 0;
}

/* exec */
PHP_METHOD(Redis, exec)
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
   	if (redis_sock_get(object, &redis_sock TSRMLS_CC) < 0) {
       	RETURN_FALSE;
    }

	IF_MULTI() {

        /*
		fold_item *f1 = malloc(sizeof(fold_item));
		f1->function_name = strdup("___end___");
		f1->fun = (void *)NULL;
		f1->next = NULL;
		current = f1;	
        */
        current = NULL;



		cmd_len = redis_cmd_format(&cmd, "EXEC \r\n");

		if (redis_sock_write(redis_sock, cmd, cmd_len) < 0) {
			efree(cmd);
			RETURN_FALSE;
		}
		efree(cmd);

	    if (redis_sock_read_multibulk_multi_reply(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock TSRMLS_CC) < 0) {
			RETURN_FALSE;
	    }
		set_flag(object, REDIS_ATOMIC);
	}

	IF_PIPELINE() {
	  
        /*
		fold_item *f1 = malloc(sizeof(fold_item));
		f1->function_name = strdup("___end___");
		f1->fun = (void *)NULL;
		f1->next = NULL;
		current->next = f1;
		current = current->next;
        */
        current = NULL;
	
		current_request = head_request;
		char *request;
		int total = 0;
		int offset = 0;
		
	
		/** il faut calculer au préalable la taille de request */
		/* compute the total request size */
		current_request = head_request;
		while(current_request != NULL) {
			if(current_request->request_str == NULL) {
				current_request = current_request->next;
				continue;
			}
		  	total += current_request->request_size;
			current_request = current_request->next;

		}
		request = malloc(total * sizeof(char *));

		current_request = head_request;		
		while(current_request != NULL) {
			if(current_request->request_str == NULL) {
				current_request = current_request->next;
				continue;
			}
			memcpy(request + offset, current_request->request_str, current_request->request_size);
			offset += current_request->request_size;
			current_request = current_request->next;
		}
		request[offset] = '\0';

		if(request != NULL) {
		    if (redis_sock_write(redis_sock, request, total) < 0) {
    		    free(request);
        		RETURN_FALSE;
		    }
		   	free(request);
		}

	    if (redis_sock_read_multibulk_pipeline_reply(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock TSRMLS_CC) < 0) {
			set_flag(object, REDIS_ATOMIC);
			RETURN_FALSE;
	    }
		set_flag(object, REDIS_ATOMIC);
	}
}

zval *fold_this_item(INTERNAL_FUNCTION_PARAMETERS, fold_item *item, RedisSock *redis_sock, zval *z_tab TSRMLS_DC) 
{
	zval *ret = malloc(sizeof(zval *));
	ret = item->fun(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock TSRMLS_DC, z_tab);	
	return ret;
}

PHPAPI int redis_sock_read_multibulk_multi_reply_loop(INTERNAL_FUNCTION_PARAMETERS, 
							RedisSock *redis_sock, zval *z_tab, int numElems TSRMLS_DC) 
{

	zval *z_response;
	current = head;
	if(numElems == 0) {
		while(current != NULL) {
			numElems ++;
			current = current->next;
		}
	}
	current = head;

    while(numElems > 0) {		
            /*
		if(strcmp(current->function_name, "__begin__") == 0) {
			current = current->next;
		}	
        */
		fold_this_item(INTERNAL_FUNCTION_PARAM_PASSTHRU, current, redis_sock, z_tab TSRMLS_DC);	
		current  = current->next;
        numElems --;
    }
    return 0;
}

PHP_METHOD(Redis, pipeline) 
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

	// if the flag is activated, send the command, the reply will be "QUEUED" or -ERR
    if (redis_sock_get(object, &redis_sock TSRMLS_CC) < 0) {
        RETURN_FALSE;
    }
	set_flag(object, REDIS_PIPELINE);	

	/* 
		NB : we keep the function fold, to detect the last function . 
		We need the response format of the n - 1 command. So, we can delete when n > 2, the { 1 .. n - 2} commands
	*/
	
    /*
	head = malloc(sizeof(fold_item));
	current = head;
	current->function_name = strdup("__begin__");	
    */
    current = NULL;

	RETURN_ZVAL(getThis(), 1, 0);
}
/* vim: set tabstop=4 expandtab: */

