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
  | Maintainer: Michael Grunder <michael.grunder@gmail.com>              |
  | Maintainer: Nasreddine Bouafif <n.bouafif@owlient.eu>                |
  +----------------------------------------------------------------------+
*/

#include "common.h"

#ifndef PHP_REDIS_H
#define PHP_REDIS_H

/* phpredis version */
#define PHP_REDIS_VERSION "5.3.2"

/* For convenience we store the salt as a printable hex string which requires 2
 * characters per byte + 1 for the NULL terminator */
#define REDIS_SALT_BYTES 32
#define REDIS_SALT_SIZE ((2 * REDIS_SALT_BYTES) + 1)

ZEND_BEGIN_MODULE_GLOBALS(redis)
	char salt[REDIS_SALT_SIZE];
ZEND_END_MODULE_GLOBALS(redis)

ZEND_EXTERN_MODULE_GLOBALS(redis)
#define REDIS_G(v) ZEND_MODULE_GLOBALS_ACCESSOR(redis, v)

#ifdef ZTS
#include "TSRM.h"
#endif

PHP_MINIT_FUNCTION(redis);
PHP_MSHUTDOWN_FUNCTION(redis);
PHP_MINFO_FUNCTION(redis);

PHP_REDIS_API int redis_connect(INTERNAL_FUNCTION_PARAMETERS, int persistent);

PHP_REDIS_API int redis_response_enqueued(RedisSock *redis_sock);

PHP_REDIS_API int redis_sock_read_multibulk_multi_reply_loop(
    INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock, zval *z_tab,
    int numElems);

extern zend_module_entry redis_module_entry;

#define redis_module_ptr &redis_module_entry
#define phpext_redis_ptr redis_module_ptr

#endif

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
