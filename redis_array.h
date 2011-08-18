#ifndef REDIS_ARRAY_H
#define REDIS_ARRAY_H

#include "common.h"

zend_class_entry *redis_array_ce;

PHP_METHOD(RedisArray, __construct);
PHP_METHOD(RedisArray, __call);


typedef struct RedisArray_ {
	
	int count;
	RedisSock **cx;
	char *fun;

	struct RedisArray_ *prev;
} RedisArray;


RedisArray *ra_make_array(HashTable *hosts, const char *fun_name);

#endif
