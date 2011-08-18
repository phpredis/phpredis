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

ZEND_BEGIN_ARG_INFO_EX(__redis_array_call_args, 0, 0, 2)
	ZEND_ARG_INFO(0, function_name)
	ZEND_ARG_INFO(0, arguments)
ZEND_END_ARG_INFO()

zend_function_entry redis_array_functions[] = {
     PHP_ME(RedisArray, __construct, NULL, ZEND_ACC_PUBLIC)
     PHP_ME(RedisArray, __call, __redis_array_call_args, ZEND_ACC_PUBLIC)
     {NULL, NULL, NULL}
};

RedisArray *
ra_make_array(HashTable *hosts, const char *fun_name) {

	int i, host_len;
	int count = zend_hash_num_elements(hosts);
	char *host, *p;
	short port;
	zval **zpData;

	/* create object */
	RedisArray *ra = emalloc(sizeof(RedisArray));
	ra->cx = emalloc(count * sizeof(RedisSock*));
	ra->count = count;

	/* init connections */
	for(i = 0; i < count; ++i) {
		if(FAILURE == zend_hash_quick_find(hosts, NULL, 0, i, &zpData)) {
			efree(ra->cx);
			efree(ra);
			return NULL;
		}
		
		/* default values */
		host = Z_STRVAL_PP(zpData);
		host_len = Z_STRLEN_PP(zpData);
		port = 6379;

		if((p = strchr(host, ':'))) { /* found port */
			host_len = p - host;
			port = (short)atoi(p+1);
		}
		// printf("host(%d)=%s, port=%d\n", host_len, host,  (int)port);
		ra->cx[i] = redis_sock_create(host, host_len, port, 0, 0, NULL); /* TODO: persistence */
 
	}

	/* copy function if provided */
	if(fun_name) {
		ra->fun = estrdup(fun_name);
	}

	return ra;
}

/* {{{ proto RedisArray RedisArray::__construct()
    Public constructor */
PHP_METHOD(RedisArray, __construct)
{
	zval *z0;
	char *fun = NULL, *name = NULL;
	int fun_len;
	RedisArray *ra = NULL;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "z|s", &z0, &fun, &fun_len) == FAILURE) {
		RETURN_FALSE;
	}

	if(!fun || !fun_len) { /* either an array name or a list of hosts */
		switch(Z_TYPE_P(z0)) {
			case IS_STRING:
				name = Z_STRVAL_P(z0);
				// printf("name=%s\n", name);
				break;

			case IS_ARRAY:
				// printf("ARRAY OF HOSTS\n");
				ra = ra_make_array(Z_ARRVAL_P(z0), NULL);
				break;

			default:
				WRONG_PARAM_COUNT;
				break;
		}
	} else {
		if(Z_TYPE_P(z0) != IS_ARRAY) {
			WRONG_PARAM_COUNT;
		}
		// printf("ARRAY OF HOSTS, fun=%s\n", fun);
		ra = ra_make_array(Z_ARRVAL_P(z0), fun);
	}
}

PHP_METHOD(RedisArray, __call)
{
    zval *object, *array;

    printf("argc=%d\n", ZEND_NUM_ARGS());
}

