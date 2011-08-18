#ifndef REDIS_ARRAY_H
#define REDIS_ARRAY_H

#include <stdint.h>
#include "common.h"

zend_class_entry *redis_array_ce;
void redis_destructor_redis_array(zend_rsrc_list_entry * rsrc TSRMLS_DC);

PHP_METHOD(RedisArray, __construct);
PHP_METHOD(RedisArray, __call);


typedef struct RedisArray_ {
	
	int count;
	zval **redis;
	char *fun;

	struct RedisArray_ *prev;
} RedisArray;

uint32_t crc32(const char *s, size_t sz);

RedisArray *ra_make_array(HashTable *hosts, const char *fun_name);
zval *ra_find_node(RedisArray *ra, const char *key, int key_len);

#endif
