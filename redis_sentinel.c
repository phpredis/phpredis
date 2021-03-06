/*
  +----------------------------------------------------------------------+
  | Copyright (c) The PHP Group                                          |
  +----------------------------------------------------------------------+
  | This source file is subject to version 3.01 of the PHP license,      |
  | that is bundled with this package in the file LICENSE, and is        |
  | available through the world-wide-web at the following url:           |
  | http://www.php.net/license/3_01.txt                                  |
  | If you did not receive a copy of the PHP license and are unable to   |
  | obtain it through the world-wide-web, please send a note to          |
  | license@php.net so we can mail you a copy immediately.               |
  +----------------------------------------------------------------------+
  | Author: Pavlo Yatsukhnenko <yatsukhnenko@gmail.com>                  |
  | Maintainer: Michael Grunder <michael.grunder@gmail.com>              |
  +----------------------------------------------------------------------+
*/

#include "php_redis.h"
#include "redis_commands.h"
#include "redis_sentinel.h"
#include <zend_exceptions.h>

zend_class_entry *redis_sentinel_ce;
extern zend_class_entry *redis_exception_ce;

ZEND_BEGIN_ARG_INFO_EX(arginfo_ctor, 0, 0, 1)
    ZEND_ARG_INFO(0, host)
    ZEND_ARG_INFO(0, port)
    ZEND_ARG_INFO(0, timeout)
    ZEND_ARG_INFO(0, persistent)
    ZEND_ARG_INFO(0, retry_interval)
    ZEND_ARG_INFO(0, read_timeout)
ZEND_END_ARG_INFO()

zend_function_entry redis_sentinel_functions[] = {
     PHP_ME(RedisSentinel, __construct, arginfo_ctor, ZEND_ACC_PUBLIC)
     PHP_ME(RedisSentinel, ckquorum, arginfo_value, ZEND_ACC_PUBLIC)
     PHP_ME(RedisSentinel, failover, arginfo_value, ZEND_ACC_PUBLIC)
     PHP_ME(RedisSentinel, flushconfig, arginfo_void, ZEND_ACC_PUBLIC)
     PHP_ME(RedisSentinel, getMasterAddrByName, arginfo_value, ZEND_ACC_PUBLIC)
     PHP_ME(RedisSentinel, master, arginfo_value, ZEND_ACC_PUBLIC)
     PHP_ME(RedisSentinel, masters, arginfo_void, ZEND_ACC_PUBLIC)
     PHP_ME(RedisSentinel, myid, arginfo_void, ZEND_ACC_PUBLIC)
     PHP_ME(RedisSentinel, ping, arginfo_void, ZEND_ACC_PUBLIC)
     PHP_ME(RedisSentinel, reset, arginfo_value, ZEND_ACC_PUBLIC)
     PHP_ME(RedisSentinel, sentinels, arginfo_value, ZEND_ACC_PUBLIC)
     PHP_ME(RedisSentinel, slaves, arginfo_value, ZEND_ACC_PUBLIC)
     PHP_FE_END
};

PHP_METHOD(RedisSentinel, __construct)
{
    int persistent = 0;
    char *persistent_id = NULL;
    double timeout = 0.0, read_timeout = 0.0;
    zend_long port = 26379, retry_interval = 0;
    redis_sentinel_object *obj;
    zend_string *host;
    zval *auth = NULL, *zv = NULL;

    if (zend_parse_parameters(ZEND_NUM_ARGS(), "S|ldz!ldz",
                                &host, &port, &timeout, &zv,
                                &retry_interval, &read_timeout,
                                &auth) == FAILURE) {
        RETURN_FALSE;
    }

    if (port < 0 || port > UINT16_MAX) {
        REDIS_THROW_EXCEPTION("Invalid port", 0);
        RETURN_FALSE;
    }

    if (timeout < 0L || timeout > INT_MAX) {
        REDIS_THROW_EXCEPTION("Invalid connect timeout", 0);
        RETURN_FALSE;
    }

    if (read_timeout < 0L || read_timeout > INT_MAX) {
        REDIS_THROW_EXCEPTION("Invalid read timeout", 0);
        RETURN_FALSE;
    }

    if (retry_interval < 0L || retry_interval > INT_MAX) {
        REDIS_THROW_EXCEPTION("Invalid retry interval", 0);
        RETURN_FALSE;
    }

    if (zv) {
        ZVAL_DEREF(zv);
        if (Z_TYPE_P(zv) == IS_STRING) {
            persistent_id = Z_STRVAL_P(zv);
            persistent = 1; /* even empty */
        } else {
            persistent = zval_is_true(zv);
        }
    }

    obj = PHPREDIS_ZVAL_GET_OBJECT(redis_sentinel_object, getThis());
    obj->sock = redis_sock_create(ZSTR_VAL(host), ZSTR_LEN(host), port,
        timeout, read_timeout, persistent, persistent_id, retry_interval);
    if (auth) {
        redis_sock_set_auth_zval(obj->sock, auth);
    }
}

PHP_METHOD(RedisSentinel, ckquorum)
{
    REDIS_PROCESS_KW_CMD("ckquorum", redis_sentinel_str_cmd, redis_boolean_response);
}

PHP_METHOD(RedisSentinel, failover)
{
    REDIS_PROCESS_KW_CMD("failover", redis_sentinel_str_cmd, redis_boolean_response);
}

PHP_METHOD(RedisSentinel, flushconfig)
{
    REDIS_PROCESS_KW_CMD("flushconfig", redis_sentinel_cmd, redis_boolean_response);
}

PHP_METHOD(RedisSentinel, getMasterAddrByName)
{
    REDIS_PROCESS_KW_CMD("get-master-addr-by-name", redis_sentinel_str_cmd, redis_mbulk_reply_raw);
}

PHP_METHOD(RedisSentinel, master)
{
    REDIS_PROCESS_KW_CMD("master", redis_sentinel_str_cmd, redis_mbulk_reply_zipped_raw);
}

PHP_METHOD(RedisSentinel, masters)
{
    REDIS_PROCESS_KW_CMD("masters", redis_sentinel_cmd, sentinel_mbulk_reply_zipped_assoc);
}

PHP_METHOD(RedisSentinel, myid)
{
    REDIS_PROCESS_KW_CMD("myid", redis_sentinel_cmd, redis_string_response);
}

PHP_METHOD(RedisSentinel, ping)
{
    REDIS_PROCESS_KW_CMD("ping", redis_empty_cmd, redis_boolean_response);
}

PHP_METHOD(RedisSentinel, reset)
{
    REDIS_PROCESS_KW_CMD("reset", redis_sentinel_str_cmd, redis_boolean_response);
}

PHP_METHOD(RedisSentinel, sentinels)
{
    REDIS_PROCESS_KW_CMD("sentinels", redis_sentinel_str_cmd, sentinel_mbulk_reply_zipped_assoc);
}

PHP_METHOD(RedisSentinel, slaves)
{
    REDIS_PROCESS_KW_CMD("slaves", redis_sentinel_str_cmd, sentinel_mbulk_reply_zipped_assoc);
}
