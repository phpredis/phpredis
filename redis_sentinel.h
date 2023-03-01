#ifndef REDIS_SENTINEL_H
#define REDIS_SENTINEL_H

#include "sentinel_library.h"

#define PHP_REDIS_SENTINEL_VERSION "1.0"

extern zend_class_entry *redis_sentinel_ce;
extern PHP_MINIT_FUNCTION(redis_sentinel);

#endif /* REDIS_SENTINEL_H */
