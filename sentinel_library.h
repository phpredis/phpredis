#ifndef REDIS_SENTINEL_LIBRARY_H
#define REDIS_SENTINEL_LIBRARY_H

#include "common.h"
#include "library.h"

typedef redis_object redis_sentinel_object;

/* Multi-host fallback entry. One per user-provided Sentinel endpoint;
 * the full list hangs off RedisSock->sentinel_hosts (issue #2819). */
struct sentinel_host_entry {
    zend_string *host;
    int port;
};

zend_object *create_sentinel_object(zend_class_entry *ce);

PHP_REDIS_API int sentinel_mbulk_reply_zipped_assoc(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock, zval *z_tab, void *ctx);

int sentinel_parse_hosts_option(RedisSock *sock, zval *hosts_zv);
int sentinel_try_next_host(RedisSock *sock);
zend_bool sentinel_was_network_error(RedisSock *sock);
zend_bool sentinel_has_more_hosts(RedisSock *sock);
void sentinel_free_hosts(RedisSock *sock);

/* Resolve the RedisSentinel from PHP_METHOD context and wrap the command call
 * with retry-once-on-network-error. Command-level retry is bounded to 1; the
 * scan inside sentinel_try_next_host iterates the remaining host list.
 * No-op on single-host usage — sentinel_has_more_hosts short-circuits. */
#define SENTINEL_METHOD(cmd_call)                                              \
    do {                                                                       \
        redis_sentinel_object *__obj =                                         \
            PHPREDIS_ZVAL_GET_OBJECT(redis_sentinel_object, getThis());        \
        RedisSock *__sock = __obj->sock;                                       \
        cmd_call;                                                              \
        if (EG(exception) &&                                                   \
            sentinel_was_network_error(__sock) &&                              \
            sentinel_has_more_hosts(__sock)) {                                 \
            OBJ_RELEASE(EG(exception));                                        \
            EG(exception) = NULL;                                              \
            if (sentinel_try_next_host(__sock) == SUCCESS) {                   \
                cmd_call;                                                      \
            }                                                                  \
        }                                                                      \
    } while (0)

#endif /* REDIS_SENTINEL_LIBRARY_H */
