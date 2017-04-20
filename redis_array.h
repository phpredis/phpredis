#ifndef REDIS_ARRAY_H
#define REDIS_ARRAY_H

#ifdef PHP_WIN32
#include "win32/php_stdint.h"
#else
#include <stdint.h>
#endif
#include "common.h"

PHP_METHOD(RedisArray, __construct);
PHP_METHOD(RedisArray, __call);
PHP_METHOD(RedisArray, _hosts);
PHP_METHOD(RedisArray, _target);
PHP_METHOD(RedisArray, _instance);
PHP_METHOD(RedisArray, _function);
PHP_METHOD(RedisArray, _distributor);
PHP_METHOD(RedisArray, _rehash);

PHP_METHOD(RedisArray, select);
PHP_METHOD(RedisArray, info);
PHP_METHOD(RedisArray, ping);
PHP_METHOD(RedisArray, flushdb);
PHP_METHOD(RedisArray, flushall);
PHP_METHOD(RedisArray, mget);
PHP_METHOD(RedisArray, mset);
PHP_METHOD(RedisArray, del);
PHP_METHOD(RedisArray, keys);
PHP_METHOD(RedisArray, getOption);
PHP_METHOD(RedisArray, setOption);
PHP_METHOD(RedisArray, save);
PHP_METHOD(RedisArray, bgsave);

PHP_METHOD(RedisArray, multi);
PHP_METHOD(RedisArray, exec);
PHP_METHOD(RedisArray, discard);
PHP_METHOD(RedisArray, unwatch);


typedef struct RedisArray_ {

	int count;
	char **hosts;			/* array of host:port strings */
	zval *redis;			/* array of Redis instances */
	zval *z_multi_exec;		/* Redis instance to be used in multi-exec */
	zend_bool index;		/* use per-node index */
	zend_bool auto_rehash; 	/* migrate keys on read operations */
	zend_bool pconnect;     /* should we use pconnect */
	zval z_fun;             /* key extractor, callable */
	zval z_dist;            /* key distributor, callable */
	zval z_pure_cmds;		/* hash table */
	double connect_timeout; /* socket connect timeout */
	double read_timeout;    /* socket read timeout */

	struct RedisArray_ *prev;
} RedisArray;

#if (PHP_MAJOR_VERSION < 7)
zend_object_value create_redis_array_object(zend_class_entry *ce TSRMLS_DC);
void free_redis_array_object(void *object TSRMLS_DC);
#else
zend_object *create_redis_array_object(zend_class_entry *ce TSRMLS_DC);
void free_redis_array_object(zend_object *object);
#endif


#endif
