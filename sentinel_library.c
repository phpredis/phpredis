#include "sentinel_library.h"

static zend_object_handlers redis_sentinel_object_handlers;

static void
free_redis_sentinel_object(zend_object *object)
{
    redis_sentinel_object *obj = PHPREDIS_GET_OBJECT(redis_sentinel_object, object);

    if (obj->sock) {
        redis_sock_disconnect(obj->sock, 0);
        redis_free_socket(obj->sock);
    }
    zend_object_std_dtor(&obj->std);
}

zend_object *
create_sentinel_object(zend_class_entry *ce)
{
    redis_sentinel_object *obj = ecalloc(1, sizeof(*obj) + zend_object_properties_size(ce));

    zend_object_std_init(&obj->std, ce);
    object_properties_init(&obj->std, ce);

    memcpy(&redis_sentinel_object_handlers, zend_get_std_object_handlers(), sizeof(redis_sentinel_object_handlers));
    redis_sentinel_object_handlers.offset = XtOffsetOf(redis_sentinel_object, std);
    redis_sentinel_object_handlers.free_obj = free_redis_sentinel_object;
    obj->std.handlers = &redis_sentinel_object_handlers;

    return &obj->std;
}

PHP_REDIS_API int
sentinel_mbulk_reply_zipped_assoc(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock, zval *z_tab, void *ctx)
{
    char inbuf[4096];
    int i, nelem;
    size_t len;
    zval z_ret;

    /* Throws exception on failure */
    if (redis_sock_gets(redis_sock, inbuf, sizeof(inbuf) - 1, &len) < 0) {
        RETVAL_FALSE;
        return FAILURE;
    }

    if (*inbuf != TYPE_MULTIBULK) {
        if (*inbuf == TYPE_ERR) {
            redis_sock_set_err(redis_sock, inbuf + 1, len - 1);
        }

        RETVAL_FALSE;
        return FAILURE;
    }
    array_init(&z_ret);
    nelem = atoi(inbuf + 1);
    for (i = 0; i < nelem; ++i) {
        /* redis_mbulk_reply_zipped_raw calls redis_mbulk_reply_zipped
         * which puts result into return_value via RETVAL_ZVAL */
        redis_mbulk_reply_zipped_raw(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock, z_tab, ctx);
        add_next_index_zval(&z_ret, return_value);
    }

    RETVAL_ZVAL(&z_ret, 0, 1);
    return SUCCESS;
}
