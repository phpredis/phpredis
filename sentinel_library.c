#include "sentinel_library.h"
#include <zend_exceptions.h>

extern zend_class_entry *redis_exception_ce;

/* Upper bound on hosts list length. Real HA deployments have 3-7 Sentinels;
 * this is purely a DoS guard against runaway allocation. */
#define SENTINEL_MAX_HOSTS 1024

static zend_object_handlers redis_sentinel_object_handlers;

void
sentinel_free_hosts(RedisSock *sock)
{
    size_t i;
    if (sock == NULL || sock->sentinel_hosts == NULL) return;
    for (i = 0; i < sock->sentinel_hosts_count; i++) {
        if (sock->sentinel_hosts[i].host) {
            zend_string_release(sock->sentinel_hosts[i].host);
        }
    }
    efree(sock->sentinel_hosts);
    sock->sentinel_hosts = NULL;
    sock->sentinel_hosts_count = 0;
    sock->sentinel_current_host_idx = 0;
}

static void
free_redis_sentinel_object(zend_object *object)
{
    redis_sentinel_object *obj = PHPREDIS_GET_OBJECT(redis_sentinel_object, object);

    if (obj->sock) {
        sentinel_free_hosts(obj->sock);
        redis_sock_disconnect(obj->sock, 0, 1);
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

static int
parse_one_host_entry(zval *entry, size_t i, sentinel_host_entry *out)
{
    zval *host_zv, *port_zv;

    if (Z_TYPE_P(entry) != IS_ARRAY) {
        zend_throw_exception_ex(redis_exception_ce, 0,
            "RedisSentinel: 'hosts' entry at index %zu must be an array", i);
        return FAILURE;
    }

    host_zv = zend_hash_str_find(Z_ARRVAL_P(entry), ZEND_STRL("host"));
    if (host_zv == NULL) {
        zend_throw_exception_ex(redis_exception_ce, 0,
            "RedisSentinel: 'hosts' entry at index %zu missing required 'host' key", i);
        return FAILURE;
    }
    if (Z_TYPE_P(host_zv) != IS_STRING) {
        zend_throw_exception_ex(redis_exception_ce, 0,
            "RedisSentinel: 'hosts' entry at index %zu: 'host' must be string", i);
        return FAILURE;
    }

    port_zv = zend_hash_str_find(Z_ARRVAL_P(entry), ZEND_STRL("port"));
    if (port_zv == NULL) {
        out->port = 26379;
    } else if (Z_TYPE_P(port_zv) == IS_LONG) {
        out->port = (int) Z_LVAL_P(port_zv);
    } else {
        zend_throw_exception_ex(redis_exception_ce, 0,
            "RedisSentinel: 'hosts' entry at index %zu: 'port' must be int", i);
        return FAILURE;
    }

    out->host = zend_string_copy(Z_STR_P(host_zv));
    return SUCCESS;
}

int
sentinel_parse_hosts_option(RedisSock *sock, zval *hosts_zv)
{
    HashTable *ht;
    zval *entry;
    size_t n, i = 0;

    if (Z_TYPE_P(hosts_zv) != IS_ARRAY) {
        REDIS_THROW_EXCEPTION("RedisSentinel: 'hosts' must be an array", 0);
        return FAILURE;
    }

    ht = Z_ARRVAL_P(hosts_zv);
    n = zend_hash_num_elements(ht);
    if (n == 0) {
        REDIS_THROW_EXCEPTION("RedisSentinel: 'hosts' must not be empty", 0);
        return FAILURE;
    }
    if (n > SENTINEL_MAX_HOSTS) {
        zend_throw_exception_ex(redis_exception_ce, 0,
            "RedisSentinel: 'hosts' has %zu entries; max is %d", n, SENTINEL_MAX_HOSTS);
        return FAILURE;
    }

    sock->sentinel_hosts = ecalloc(n, sizeof(sentinel_host_entry));
    sock->sentinel_hosts_count = n;
    sock->sentinel_current_host_idx = 0;

    ZEND_HASH_FOREACH_VAL(ht, entry) {
        if (parse_one_host_entry(entry, i, &sock->sentinel_hosts[i]) != SUCCESS) {
            sentinel_free_hosts(sock);
            return FAILURE;
        }
        i++;
    } ZEND_HASH_FOREACH_END();

    return SUCCESS;
}

zend_bool
sentinel_was_network_error(RedisSock *sock)
{
    if (sock == NULL) return 1;
    if (sock->stream == NULL) return 1;
    if (sock->status == REDIS_SOCK_STATUS_FAILED) return 1;
    if (sock->status == REDIS_SOCK_STATUS_DISCONNECTED) return 1;
    return 0;
}

zend_bool
sentinel_has_more_hosts(RedisSock *sock)
{
    if (sock == NULL || sock->sentinel_hosts == NULL) return 0;
    return (sock->sentinel_current_host_idx + 1) < sock->sentinel_hosts_count;
}

int
sentinel_try_next_host(RedisSock *sock)
{
    size_t i;

    if (sock == NULL || sock->sentinel_hosts == NULL) return FAILURE;

    if (sock->stream) {
        redis_sock_disconnect(sock, 0, 1);
    }

    for (i = sock->sentinel_current_host_idx + 1; i < sock->sentinel_hosts_count; i++) {
        /* Clear any exception set by a prior iteration's failed connect so it
         * doesn't contaminate the next redis_sock_server_open attempt or leak
         * into the caller if this iteration succeeds. */
        if (EG(exception)) zend_clear_exception();

        if (sock->host) zend_string_release(sock->host);
        sock->host = zend_string_copy(sock->sentinel_hosts[i].host);
        sock->port = sock->sentinel_hosts[i].port;

        if (redis_sock_server_open(sock) == SUCCESS) {
            sock->sentinel_current_host_idx = i;
            if (EG(exception)) zend_clear_exception();
            return SUCCESS;
        }
    }

    {
        sentinel_host_entry *last = &sock->sentinel_hosts[sock->sentinel_hosts_count - 1];
        if (EG(exception)) zend_clear_exception();
        zend_throw_exception_ex(redis_exception_ce, 0,
            "Failed to connect to any of %zu Sentinel hosts (last attempted: %s:%d)",
            sock->sentinel_hosts_count, ZSTR_VAL(last->host), last->port);
    }
    return FAILURE;
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
