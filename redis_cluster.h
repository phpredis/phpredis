#ifndef REDIS_CLUSTER_H
#define REDIS_CLUSTER_H

#include "cluster_library.h"
#include "redis_commands.h"
#include <php.h>
#include <stddef.h>

/* Get attached object context */
#define GET_CONTEXT() PHPREDIS_ZVAL_GET_OBJECT(redisCluster, getThis())

/* Simple macro to free our enqueued callbacks after we EXEC */
#define CLUSTER_FREE_QUEUE(c) \
    clusterFoldItem *_item = c->multi_head, *_tmp; \
    while(_item) { \
        _tmp = _item->next; \
        efree(_item); \
        _item = _tmp; \
    } \
    c->multi_head = c->multi_curr = NULL; \

/* Reset anything flagged as MULTI */
#define CLUSTER_RESET_MULTI(c) \
    redisClusterNode *_node; \
    ZEND_HASH_FOREACH_PTR(c->nodes, _node) { \
        if (_node == NULL) break; \
        _node->sock->watching = 0; \
        _node->sock->mode = ATOMIC; \
    } ZEND_HASH_FOREACH_END(); \
    c->flags->watching = 0; \
    c->flags->mode     = ATOMIC; \

void cluster_process_cmd(INTERNAL_FUNCTION_PARAMETERS, redisCluster *c,
    redis_cmd_cb cmd_cb, cluster_cb resp_cb, int readonly);

#define CLUSTER_PROCESS_CMD(cmdname, resp_func, readcmd) \
    cluster_process_cmd(INTERNAL_FUNCTION_PARAM_PASSTHRU, GET_CONTEXT(), \
                        redis_##cmdname##_cmd, resp_func, readcmd)

void cluster_process_kw_cmd(INTERNAL_FUNCTION_PARAMETERS, redisCluster *c,
    const char *kw, redis_kw_cmd_cb cmd_cb, cluster_cb resp_cb, int readonly);

#define CLUSTER_PROCESS_KW_CMD(kw, cmd_cb, resp_cb, readonly) \
    cluster_process_kw_cmd(INTERNAL_FUNCTION_PARAM_PASSTHRU, GET_CONTEXT(), \
                           kw, cmd_cb, resp_cb, readonly)

extern zend_class_entry *redis_cluster_ce;
extern zend_class_entry *redis_cluster_exception_ce;
extern PHP_MINIT_FUNCTION(redis_cluster);
extern zend_object * create_cluster_context(zend_class_entry *class_type);
extern void free_cluster_context(zend_object *object);

#endif
