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

#include "common.h"

#ifndef PHP_REDIS_H
#define PHP_REDIS_H

#ifdef PHP_WIN32
#define PHP_REDIS_API __declspec(dllexport)
#else
#define PHP_REDIS_API
#endif

#ifdef ZTS
#include "TSRM.h"
#endif

PHP_MINIT_FUNCTION(redis);
PHP_MINFO_FUNCTION(redis);

PHPAPI int redis_connect(INTERNAL_FUNCTION_PARAMETERS, int persistent);
PHPAPI void redis_atomic_increment(INTERNAL_FUNCTION_PARAMETERS, char *keyword, int count);
PHPAPI int generic_multiple_args_cmd(INTERNAL_FUNCTION_PARAMETERS, char *keyword, int keyword_len,
									 int min_argc, RedisSock **redis_sock, int has_timeout);
PHPAPI void generic_sort_cmd(INTERNAL_FUNCTION_PARAMETERS, char *sort, int use_alpha);
PHPAPI void generic_empty_cmd(INTERNAL_FUNCTION_PARAMETERS, char *cmd, int cmd_len, ...);
PHPAPI void generic_empty_long_cmd(INTERNAL_FUNCTION_PARAMETERS, char *cmd, int cmd_len, ...);

PHPAPI void array_zip_values_and_scores(RedisSock *redis_sock, zval *z_tab, int use_atof TSRMLS_DC);
PHPAPI int redis_response_enqueued(RedisSock *redis_sock TSRMLS_DC);

PHPAPI int get_flag(zval *object TSRMLS_DC);
PHPAPI void set_flag(zval *object, int new_flag TSRMLS_DC);

PHPAPI int redis_sock_read_multibulk_multi_reply_loop(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock, zval *z_tab, int numElems);

/* pipeline */
PHPAPI request_item* get_pipeline_head(zval *object);
PHPAPI void set_pipeline_head(zval *object, request_item *head);
PHPAPI request_item* get_pipeline_current(zval *object);
PHPAPI void set_pipeline_current(zval *object, request_item *current);

ZEND_BEGIN_MODULE_GLOBALS(redis)
ZEND_END_MODULE_GLOBALS(redis)


struct redis_queued_item {

	/* reading function */
	zval * (*fun)(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock, ...);

	char *cmd; 
	int cmd_len;

	struct redis_queued_item *next;
};

extern zend_module_entry redis_module_entry;
#define redis_module_ptr &redis_module_entry

#define phpext_redis_ptr redis_module_ptr

#define PHP_REDIS_VERSION "2.1.3"

#endif

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
