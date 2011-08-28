#ifndef REDIS_ARRAY_H
#define REDIS_ARRAY_H

#include <stdint.h>
#include "common.h"

zend_class_entry *redis_array_ce;
void redis_destructor_redis_array(zend_rsrc_list_entry * rsrc TSRMLS_DC);

PHP_METHOD(RedisArray, __construct);
PHP_METHOD(RedisArray, __call);
PHP_METHOD(RedisArray, _hosts);
PHP_METHOD(RedisArray, _target);
PHP_METHOD(RedisArray, _function);
PHP_METHOD(RedisArray, _rehash);

PHP_METHOD(RedisArray, info);
PHP_METHOD(RedisArray, ping);
PHP_METHOD(RedisArray, mget);
PHP_METHOD(RedisArray, mset);
PHP_METHOD(RedisArray, del);


typedef struct RedisArray_ {
	
	int count;
	char **hosts;
	zval **redis;
	zend_bool index;
	zval *z_fun;	/* key extractor */
	zval *z_pure_cmds;	/* hash table */

	struct RedisArray_ *prev;
} RedisArray;

uint32_t crc32(const char *s, size_t sz);


#endif
