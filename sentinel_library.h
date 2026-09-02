#ifndef REDIS_SENTINEL_LIBRARY_H
#define REDIS_SENTINEL_LIBRARY_H

#include "common.h"
#include "library.h"

typedef redis_object redis_sentinel_object;

void redis_sentinel_init_object_handlers(void);
zend_object *create_sentinel_object(zend_class_entry *ce);

PHP_REDIS_API int sentinel_mbulk_reply_zipped_assoc(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock, zval *z_tab, RedisCmdCtx ctx);

#endif /* REDIS_SENTINEL_LIBRARY_H */
