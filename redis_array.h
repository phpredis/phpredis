#ifndef REDIS_ARRAY_H
#define REDIS_ARRAY_H

#include <stdint.h>
#include "common.h"

zend_class_entry *redis_array_ce;
void redis_destructor_redis_array(zend_rsrc_list_entry * rsrc TSRMLS_DC);

PHP_METHOD(RedisArray, __construct);
PHP_METHOD(RedisArray, __call);
PHP_METHOD(RedisArray, _hosts);


typedef struct RedisArray_ {
	
	int count;
	char **hosts;
	zval **redis;
	zval *z_fun;	/* key extractor */

	struct RedisArray_ *prev;
} RedisArray;

uint32_t crc32(const char *s, size_t sz);

RedisArray *ra_make_array(HashTable *hosts, zval *z_fun);
zval *ra_find_node(RedisArray *ra, const char *key, int key_len);

#endif
