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

#if PHP_VERSION_ID < 80000
#include "redis_sentinel_legacy_arginfo.h"
#else
#include "zend_attributes.h"
#include "redis_sentinel_arginfo.h"
#endif

PHP_MINIT_FUNCTION(redis_sentinel)
{
    /* RedisSentinel class */
    redis_sentinel_ce = register_class_RedisSentinel();
    redis_sentinel_ce->create_object = create_sentinel_object;

    return SUCCESS;
}

PHP_METHOD(RedisSentinel, __construct)
{
    HashTable *opts = NULL;
    redis_sentinel_object *sentinel;

    ZEND_PARSE_PARAMETERS_START(0, 1)
        Z_PARAM_OPTIONAL
        Z_PARAM_ARRAY_HT_OR_NULL(opts)
    ZEND_PARSE_PARAMETERS_END_EX(RETURN_THROWS());

    sentinel = PHPREDIS_ZVAL_GET_OBJECT(redis_sentinel_object, getThis());
    sentinel->sock = redis_sock_create(ZEND_STRL("127.0.0.1"), 26379, 0, 0, 0, NULL, 0);
    if (opts != NULL && redis_sock_configure(sentinel->sock, opts) != SUCCESS) {
        RETURN_THROWS();
    }
    sentinel->sock->sentinel = 1;
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
    REDIS_PROCESS_KW_CMD("reset", redis_sentinel_str_cmd, redis_long_response);
}

PHP_METHOD(RedisSentinel, sentinels)
{
    REDIS_PROCESS_KW_CMD("sentinels", redis_sentinel_str_cmd, sentinel_mbulk_reply_zipped_assoc);
}

PHP_METHOD(RedisSentinel, slaves)
{
    REDIS_PROCESS_KW_CMD("slaves", redis_sentinel_str_cmd, sentinel_mbulk_reply_zipped_assoc);
}
