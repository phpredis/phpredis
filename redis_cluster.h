#ifndef REDIS_CLUSTER_H
#define REDIS_CLUSTER_H

#include "cluster_library.h"
#include <php.h>
#include <stddef.h>

/* Redis cluster hash slots and N-1 which we'll use to find it */
#define REDIS_CLUSTER_SLOTS 16384
#define REDIS_CLUSTER_MOD   (REDIS_CLUSTER_SLOTS-1)

/* Get attached object context */
#define GET_CONTEXT() (redisCluster*)zend_object_store_get_object(getThis() TSRMLS_CC)

/* For the creation of RedisCluster specific exceptions */
PHPAPI zend_class_entry *rediscluster_get_exception_base(int root TSRMLS_DC);

/* Initialization and cleanup of our attached object */
zend_object_value create_cluster_context(zend_class_entry *class_type TSRMLS_DC);
void free_cluster_context(void *object TSRMLS_DC);

/* Inittialize our class with PHP */
void init_rediscluster(TSRMLS_D);

PHP_METHOD(RedisCluster, __construct);
PHP_METHOD(RedisCluster, get);

#endif
