#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "common.h"
#include "ext/standard/info.h"
#include "php_ini.h"
#include "php_redis.h"
#include "redis_array.h"
#include <zend_exceptions.h>

#include "library.h"
#include "redis_array.h"

zend_function_entry redis_array_functions[] = {
     PHP_ME(RedisArray, __construct, NULL, ZEND_ACC_PUBLIC)
     {NULL, NULL, NULL}
};

/* {{{ proto RedisArray RedisArray::__construct()
    Public constructor */
PHP_METHOD(RedisArray, __construct)
{
	zval *z0;
	char *fun = NULL, *name = NULL;
	int fun_len;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "z|s", &z0, &fun, &fun_len) == FAILURE) {
		RETURN_FALSE;
	}

	if(!fun || !fun_len) { /* either an array name or a list of hosts */
		switch(Z_TYPE_P(z0)) {
			case IS_STRING:
				name = Z_STRVAL_P(z0);
				printf("name=%s\n", name);
				break;

			case IS_ARRAY:
				printf("ARRAY OF HOSTS\n");
				break;

			default:
				WRONG_PARAM_COUNT;
				break;
		}
	} else {
		if(Z_TYPE_P(z0) != IS_ARRAY) {
			WRONG_PARAM_COUNT;
		}
		printf("ARRAY OF HOSTS, fun=%s\n", fun);
	}
}
