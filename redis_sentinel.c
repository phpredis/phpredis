#include "php_redis.h"
#include "redis_commands.h"
#include "redis_sentinel.h"

zend_class_entry *redis_sentinel_ce;

ZEND_BEGIN_ARG_INFO_EX(arginfo_ctor, 0, 0, 1)
    ZEND_ARG_INFO(0, host)
    ZEND_ARG_INFO(0, port)
ZEND_END_ARG_INFO()

zend_function_entry redis_sentinel_functions[] = {
     PHP_ME(RedisSentinel, __construct, arginfo_ctor, ZEND_ACC_PUBLIC)
     PHP_ME(RedisSentinel, ckquorum, arginfo_value, ZEND_ACC_PUBLIC)
     PHP_ME(RedisSentinel, failover, arginfo_value, ZEND_ACC_PUBLIC)
     PHP_ME(RedisSentinel, flushconfig, arginfo_void, ZEND_ACC_PUBLIC)
     PHP_ME(RedisSentinel, getMasterAddrByName, arginfo_value, ZEND_ACC_PUBLIC)
     PHP_ME(RedisSentinel, master, arginfo_value, ZEND_ACC_PUBLIC)
     PHP_ME(RedisSentinel, masters, arginfo_void, ZEND_ACC_PUBLIC)
     PHP_ME(RedisSentinel, ping, arginfo_void, ZEND_ACC_PUBLIC)
     PHP_ME(RedisSentinel, reset, arginfo_value, ZEND_ACC_PUBLIC)
     PHP_ME(RedisSentinel, sentinels, arginfo_value, ZEND_ACC_PUBLIC)
     PHP_ME(RedisSentinel, slaves, arginfo_value, ZEND_ACC_PUBLIC)
     PHP_FE_END
};

PHP_METHOD(RedisSentinel, __construct)
{
    redis_sentinel_object *obj;
    zend_long port = -1;
    zend_string *host;

    if (zend_parse_parameters(ZEND_NUM_ARGS(), "S|l", &host, &port) == FAILURE) {
        RETURN_FALSE;
    }

    /* If it's not a unix socket, set to default */
    if (port < 0 && ZSTR_LEN(host) > 0 && *ZSTR_VAL(host) != '/') {
        port = 26379;
    }

    obj = PHPREDIS_GET_OBJECT(redis_sentinel_object, getThis());
    obj->sock = redis_sock_create(ZSTR_VAL(host), ZSTR_LEN(host), port, 0, 0, 0, NULL, 0);
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

PHP_METHOD(RedisSentinel, ping)
{
    REDIS_PROCESS_KW_CMD("PING", redis_empty_cmd, redis_boolean_response);
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
