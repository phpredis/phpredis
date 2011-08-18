#ifndef REDIS_ARRAY_H
#define REDIS_ARRAY_H

#include "common.h"

zend_class_entry *redis_array_ce;

PHP_METHOD(RedisArray, __construct);

#endif
