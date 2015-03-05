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
  | Author: Axel Etcheverry <axel@etcheverry.biz>                        |
  +----------------------------------------------------------------------+
*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "common.h"
#include "ext/standard/info.h"
#include "php_ini.h"
#include "php_redis.h"
#include "redis_sentinel.h"
#include <zend_exceptions.h>
#include "library.h"

extern int le_redis_sock;
extern zend_class_entry *redis_ce;
extern zend_class_entry *redis_exception_ce;
zend_class_entry *redis_sentinel_ce;

ZEND_BEGIN_ARG_INFO_EX(arginfo_redis_sentinel_construct, 0, 0, 1)
    ZEND_ARG_INFO(0, host)
    ZEND_ARG_INFO(0, port)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO(arginfo_redis_sentinel_destruct, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO(arginfo_redis_sentinel_connect, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO(arginfo_redis_sentinel_ping, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO(arginfo_redis_sentinel_masters, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO(arginfo_redis_sentinel_master, 0)
ZEND_ARG_INFO(0, name)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO(arginfo_redis_sentinel_slaves, 0)
ZEND_ARG_INFO(0, name)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO(arginfo_redis_sentinel_get_master_addr_by_name, 0)
ZEND_ARG_INFO(0, name)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO(arginfo_redis_sentinel_reset, 0)
ZEND_ARG_INFO(0, pattern)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO(arginfo_redis_sentinel_failover, 0)
ZEND_ARG_INFO(0, name)
ZEND_END_ARG_INFO()

zend_function_entry redis_sentinel_functions[] = {
    PHP_ME(RedisSentinel, __construct,          arginfo_redis_sentinel_construct, ZEND_ACC_CTOR | ZEND_ACC_PUBLIC)
    PHP_ME(RedisSentinel, __destruct,           arginfo_redis_sentinel_destruct, ZEND_ACC_DTOR | ZEND_ACC_PUBLIC)
    PHP_ME(RedisSentinel, connect,              arginfo_redis_sentinel_connect, ZEND_ACC_PUBLIC)
    PHP_ME(RedisSentinel, ping,                 arginfo_redis_sentinel_ping, ZEND_ACC_PUBLIC)
    PHP_ME(RedisSentinel, masters,              arginfo_redis_sentinel_masters, ZEND_ACC_PUBLIC)
    PHP_ME(RedisSentinel, master,               arginfo_redis_sentinel_master, ZEND_ACC_PUBLIC)
    PHP_ME(RedisSentinel, slaves,               arginfo_redis_sentinel_slaves, ZEND_ACC_PUBLIC)
    PHP_ME(RedisSentinel, getMasterAddrByName,  arginfo_redis_sentinel_get_master_addr_by_name, ZEND_ACC_PUBLIC)
    PHP_ME(RedisSentinel, reset,                arginfo_redis_sentinel_reset, ZEND_ACC_PUBLIC)
    PHP_ME(RedisSentinel, failover,             arginfo_redis_sentinel_failover, ZEND_ACC_PUBLIC)
    PHP_FE_END
};


/* {{{ proto RedisSentinel RedisSentinel::__construct(string host, int port [, double timeout])
    Public constructor */
PHP_METHOD(RedisSentinel, __construct)
{
    int host_len;
    char *host = NULL;
    long port = -1;
    double timeout = 1.0;

    if (zend_parse_parameters(
        ZEND_NUM_ARGS() TSRMLS_CC,
        "s|ld",
        &host,
        &host_len,
        &port,
        &timeout
    ) == FAILURE) {
        return;
    }

    if (timeout < 0L || timeout > INT_MAX) {
        zend_throw_exception(redis_exception_ce, "Invalid timeout", 0 TSRMLS_CC);
        return;
    }

    if (port == -1 && host_len && host[0] != '/') { /* not unix socket, set to default value */
        port = 26379;
    }

    zval *object = getThis();

    zend_update_property_stringl(
        redis_sentinel_ce,
        object,
        ZEND_STRS("host")-1,
        host,
        host_len TSRMLS_CC
    );

    zend_update_property_long(
        redis_sentinel_ce,
        object,
        ZEND_STRS("port")-1,
        port TSRMLS_CC
    );

    zend_update_property_double(
        redis_sentinel_ce,
        object,
        ZEND_STRS("timeout")-1,
        timeout TSRMLS_CC
    );
}
/* }}} */


/* {{{ proto RedisSentinel RedisSentinel::__destruct()
    Public Destructor
 */
PHP_METHOD(RedisSentinel, __destruct)
{
    zval *object;
    RedisSock *redis_sock = NULL;

    if (zend_parse_method_parameters(
        ZEND_NUM_ARGS() TSRMLS_CC,
        getThis(),
        "O",
        &object,
        redis_sentinel_ce
    ) == FAILURE) {
        RETURN_FALSE;
    }

    if (
        (redis_sock_get(object, &redis_sock TSRMLS_CC, 1) < 0) ||
        (redis_sock->status != REDIS_SOCK_STATUS_CONNECTED)
    ) {
        RETURN_FALSE;
    }

    if (redis_sock_disconnect(redis_sock TSRMLS_CC)) {
        RETURN_TRUE;
    }

    RETURN_FALSE;
}
/* }}} */


/* {{{ proto string RedisSentinel::ping()
 */
PHP_METHOD(RedisSentinel, ping)
{
    zval *object;
    RedisSock *redis_sock;
    char *cmd;
    int cmd_len;

    if (zend_parse_method_parameters(
        ZEND_NUM_ARGS() TSRMLS_CC,
        getThis(),
        "O",
        &object,
        redis_sentinel_ce
    ) == FAILURE) {
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


/* {{{ proto boolean RedisSentinel::connect()
 */
PHP_METHOD(RedisSentinel, connect)
{
    zval *object;
    zval **socket;
    int id;
    RedisSock *redis_sock  = NULL;

    if (zend_parse_method_parameters(
        ZEND_NUM_ARGS() TSRMLS_CC,
        getThis(),
        "O",
        &object,
        redis_sentinel_ce
    ) == FAILURE) {
        RETURN_FALSE;
    }

    zval *host = zend_read_property(redis_sentinel_ce, getThis(), ZEND_STRS("host")-1, 1 TSRMLS_CC);
    zval *port = zend_read_property(redis_sentinel_ce, getThis(), ZEND_STRS("port")-1, 1 TSRMLS_CC);
    zval *timeout = zend_read_property(redis_sentinel_ce, getThis(), ZEND_STRS("timeout")-1, 1 TSRMLS_CC);

    /* if there is a redis sock already we have to remove it from the list */
    if (redis_sock_get(object, &redis_sock TSRMLS_CC, 1) > 0) {
        if (zend_hash_find(
            Z_OBJPROP_P(object),
            "socket",
            sizeof("socket"),
            (void **)&socket
        ) == FAILURE) {
            /* maybe there is a socket but the id isn't known.. what to do? */
        } else {
            zend_list_delete(Z_LVAL_PP(socket)); /* the refcount should be decreased and the detructor called */
        }
    }

    redis_sock = redis_sock_create(
        Z_STRVAL_P(host),
        Z_STRLEN_P(host),
        Z_LVAL_P(port),
        Z_DVAL_P(timeout),
        0,
        NULL,
        0,
        0
    );

    if (redis_sock_server_open(redis_sock, 1 TSRMLS_CC) < 0) {
        redis_free_socket(redis_sock);
        RETURN_FALSE;
    }

#if PHP_VERSION_ID >= 50400
    id = zend_list_insert(redis_sock, le_redis_sock TSRMLS_CC);
#else
    id = zend_list_insert(redis_sock, le_redis_sock);
#endif
    add_property_resource(object, "socket", id);

    RETURN_TRUE;
}
/* }}} */


/* {{{ proto array RedisSentinel::masters()
 */
PHP_METHOD(RedisSentinel, masters)
{
    zval *object;
    RedisSock *redis_sock;
    char *cmd;
    int cmd_len;

    if (zend_parse_method_parameters(
        ZEND_NUM_ARGS() TSRMLS_CC,
        getThis(),
        "O",
        &object,
        redis_sentinel_ce
    ) == FAILURE) {
        RETURN_FALSE;
    }

    if (redis_sock_get(object, &redis_sock TSRMLS_CC, 0) < 0) {
        RETURN_FALSE;
    }

    cmd_len = redis_cmd_format(&cmd, "SENTINEL masters" _NL, "");

    REDIS_PROCESS_REQUEST(redis_sock, cmd, cmd_len);
    IF_ATOMIC() {
        if (redis_sock_read_sentinel_servers_reply(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock, NULL) < 0) {
            RETURN_FALSE;
        }
    }
    REDIS_PROCESS_RESPONSE(redis_sock_read_sentinel_servers_reply);
}
/* }}} */


/* {{{ proto array RedisSentinel::master(string name)
 */
PHP_METHOD(RedisSentinel, master)
{
    zval *object;
    RedisSock *redis_sock;
    char *cmd;
    int cmd_len;
    int name_len;
    char *name = NULL;

    if (zend_parse_method_parameters(
        ZEND_NUM_ARGS() TSRMLS_CC,
        getThis(),
        "Os",
        &object,
        redis_sentinel_ce,
        &name,
        &name_len
    ) == FAILURE) {
        RETURN_FALSE;
    }

    if (redis_sock_get(object, &redis_sock TSRMLS_CC, 0) < 0) {
        RETURN_FALSE;
    }

    cmd_len = redis_cmd_format(&cmd, "SENTINEL master %s" _NL, name, name_len);

    REDIS_PROCESS_REQUEST(redis_sock, cmd, cmd_len);
    IF_ATOMIC() {
        if (redis_sock_read_sentinel_server_reply(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock, NULL) < 0) {
            RETURN_FALSE;
        }
    }
    REDIS_PROCESS_RESPONSE(redis_sock_read_sentinel_server_reply);
}
/* }}} */


/* {{{ proto array RedisSentinel::slaves(string master)
 */
PHP_METHOD(RedisSentinel, slaves)
{
    zval *object;
    RedisSock *redis_sock;
    char *cmd;
    int cmd_len;
    int name_len;
    char *name = NULL;

    if (zend_parse_method_parameters(
        ZEND_NUM_ARGS() TSRMLS_CC,
        getThis(),
        "Os",
        &object,
        redis_sentinel_ce,
        &name,
        &name_len
    ) == FAILURE) {
        RETURN_FALSE;
    }

    if (redis_sock_get(object, &redis_sock TSRMLS_CC, 0) < 0) {
        RETURN_FALSE;
    }

    cmd_len = redis_cmd_format(&cmd, "SENTINEL slaves %s" _NL, name, name_len);

    REDIS_PROCESS_REQUEST(redis_sock, cmd, cmd_len);
    IF_ATOMIC() {
        if (redis_sock_read_sentinel_servers_reply(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock, NULL) < 0) {
            RETURN_FALSE;
        }
    }
    REDIS_PROCESS_RESPONSE(redis_sock_read_sentinel_servers_reply);
}
/* }}} */


/* {{{ proto array RedisSentinel::getMasterAddrByName(string master)
 */
PHP_METHOD(RedisSentinel, getMasterAddrByName)
{
    zval *object;
    RedisSock *redis_sock;
    char *cmd;
    int cmd_len;
    int name_len;
    char *name = NULL;

    if (zend_parse_method_parameters(
        ZEND_NUM_ARGS() TSRMLS_CC,
        getThis(),
        "Os",
        &object,
        redis_sentinel_ce,
        &name,
        &name_len
    ) == FAILURE) {
        RETURN_FALSE;
    }

    if (redis_sock_get(object, &redis_sock TSRMLS_CC, 0) < 0) {
        RETURN_FALSE;
    }

    cmd_len = redis_cmd_format(&cmd, "SENTINEL get-master-addr-by-name %s" _NL, name, name_len);

    REDIS_PROCESS_REQUEST(redis_sock, cmd, cmd_len);
    IF_ATOMIC() {
        redis_sock_read_multibulk_reply(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock, NULL, NULL);
    }
    REDIS_PROCESS_RESPONSE(redis_sock_read_multibulk_reply);
}
/* }}} */


/* {{{ proto integer RedisSentinel::reset(string pattern)
 */
PHP_METHOD(RedisSentinel, reset)
{
    zval *object;
    RedisSock *redis_sock;
    char *cmd;
    int cmd_len;
    int name_len;
    char *name = NULL;

    if (zend_parse_method_parameters(
        ZEND_NUM_ARGS() TSRMLS_CC,
        getThis(),
        "Os",
        &object,
        redis_sentinel_ce,
        &name,
        &name_len
    ) == FAILURE) {
        RETURN_FALSE;
    }

    if (redis_sock_get(object, &redis_sock TSRMLS_CC, 0) < 0) {
        RETURN_FALSE;
    }

    cmd_len = redis_cmd_format(&cmd, "SENTINEL reset %s" _NL, name, name_len);

    REDIS_PROCESS_REQUEST(redis_sock, cmd, cmd_len);
    IF_ATOMIC() {
        redis_long_response(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock, NULL, NULL);
    }
    REDIS_PROCESS_RESPONSE(redis_long_response);
}
/* }}} */


/* {{{ proto boolean RedisSentinel::failover(string master)
 */
PHP_METHOD(RedisSentinel, failover)
{
    zval *object;
    RedisSock *redis_sock;
    char *cmd;
    int cmd_len;
    int name_len;
    char *name = NULL;

    if (zend_parse_method_parameters(
        ZEND_NUM_ARGS() TSRMLS_CC,
        getThis(),
        "Os",
        &object,
        redis_sentinel_ce,
        &name,
        &name_len
    ) == FAILURE) {
        RETURN_FALSE;
    }

    if (redis_sock_get(object, &redis_sock TSRMLS_CC, 0) < 0) {
        RETURN_FALSE;
    }

    cmd_len = redis_cmd_format(&cmd, "SENTINEL failover %s" _NL, name, name_len);

    REDIS_PROCESS_REQUEST(redis_sock, cmd, cmd_len);
    IF_ATOMIC() {
        redis_boolean_response(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock, NULL, NULL);
    }
    REDIS_PROCESS_RESPONSE(redis_boolean_response);
}
/* }}} */
