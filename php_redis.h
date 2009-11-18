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
  | Author: Alfonso Jimenez <yo@alfonsojimenez.com>                      |
  +----------------------------------------------------------------------+
*/

#ifndef PHP_REDIS_H
#define PHP_REDIS_H

extern zend_module_entry redis_module_entry;

PHP_METHOD(Redis, __construct);
PHP_METHOD(Redis, connect);
PHP_METHOD(Redis, close);
PHP_METHOD(Redis, ping);
PHP_METHOD(Redis, get);
PHP_METHOD(Redis, set);
PHP_METHOD(Redis, setnx);
PHP_METHOD(Redis, getSet);
PHP_METHOD(Redis, randomKey);
PHP_METHOD(Redis, add);
PHP_METHOD(Redis, getMultiple);
PHP_METHOD(Redis, exists);
PHP_METHOD(Redis, delete);
PHP_METHOD(Redis, incr);
PHP_METHOD(Redis, decr);
PHP_METHOD(Redis, type);
PHP_METHOD(Redis, getKeys);
PHP_METHOD(Redis, getSort);
PHP_METHOD(Redis, lPush);
PHP_METHOD(Redis, rPush);
PHP_METHOD(Redis, lPop);
PHP_METHOD(Redis, rPop);
PHP_METHOD(Redis, lSize);
PHP_METHOD(Redis, lRemove);
PHP_METHOD(Redis, listTrim);
PHP_METHOD(Redis, lGet);
PHP_METHOD(Redis, lGetRange);
PHP_METHOD(Redis, lSet);
PHP_METHOD(Redis, sAdd);
PHP_METHOD(Redis, sSize);
PHP_METHOD(Redis, sRemove);
PHP_METHOD(Redis, sMove);
PHP_METHOD(Redis, sPop);
PHP_METHOD(Redis, sContains);
PHP_METHOD(Redis, sGetMembers);
PHP_METHOD(Redis, setTimeout);

#ifdef PHP_WIN32
#define PHP_REDIS_API __declspec(dllexport)
#else
#define PHP_REDIS_API
#endif

#ifdef ZTS
#include "TSRM.h"
#endif

PHP_MINIT_FUNCTION(redis);
PHP_MSHUTDOWN_FUNCTION(redis);
PHP_RINIT_FUNCTION(redis);
PHP_RSHUTDOWN_FUNCTION(redis);
PHP_MINFO_FUNCTION(redis);

/* {{{ struct RedisSock */
typedef struct RedisSock_ {
    php_stream     *stream;
    char           *host;
    unsigned short port;
    long           timeout;
    int            failed;
    int            status;
} RedisSock;
/* }}} */

#define redis_sock_name "Redis Socket Buffer"

#define REDIS_SOCK_STATUS_FAILED 0
#define REDIS_SOCK_STATUS_DISCONNECTED 1
#define REDIS_SOCK_STATUS_UNKNOWN 2
#define REDIS_SOCK_STATUS_CONNECTED 3

/* {{{ internal function protos */
PHPAPI RedisSock* redis_sock_create(char *host, int host_len, unsigned short port, long timeout);
PHPAPI int redis_sock_connect(RedisSock *redis_sock TSRMLS_DC);
PHPAPI int redis_sock_disconnect(RedisSock *redis_sock TSRMLS_DC);
PHPAPI int redis_sock_server_open(RedisSock *redis_sock, int TSRMLS_DC);
PHPAPI char * redis_sock_read(RedisSock *redis_sock, int *buf_len TSRMLS_DC);
PHPAPI char * redis_sock_read_bulk_reply(RedisSock *redis_sock, int bytes);
PHPAPI int redis_sock_read_multibulk_reply(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock, int *buf_len TSRMLS_DC);
PHPAPI int redis_sock_write(RedisSock *redis_sock, char *cmd, size_t sz);
PHPAPI void redis_free_socket(RedisSock *redis_sock);
/* }}} */

#ifdef ZTS
#define REDIS_G(v) TSRMG(redis_globals_id, zend_redis_globals *, v)
#else
#define REDIS_G(v) (redis_globals.v)
#endif

#define PHP_REDIS_VERSION "0.1.0"

#endif

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
