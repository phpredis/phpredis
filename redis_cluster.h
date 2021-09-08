#ifndef REDIS_CLUSTER_H
#define REDIS_CLUSTER_H

#include "cluster_library.h"
#include <php.h>
#include <stddef.h>

/* Get attached object context */
#define GET_CONTEXT() PHPREDIS_ZVAL_GET_OBJECT(redisCluster, getThis())

/* Command building/processing is identical for every command */
#define CLUSTER_BUILD_CMD(name, c, cmd, cmd_len, slot) \
    redis_##name##_cmd(INTERNAL_FUNCTION_PARAM_PASSTHRU, c->flags, &cmd, \
                       &cmd_len, &slot)

/* Append information required to handle MULTI commands to the tail of our MULTI
 * linked list. */
#define CLUSTER_ENQUEUE_RESPONSE(c, slot, cb, ctx) \
    clusterFoldItem *_item; \
    _item = emalloc(sizeof(clusterFoldItem)); \
    _item->callback = cb; \
    _item->slot = slot; \
    _item->ctx = ctx; \
    _item->next = NULL; \
    if(c->multi_head == NULL) { \
        c->multi_head = _item; \
        c->multi_curr = _item; \
    } else { \
        c->multi_curr->next = _item; \
        c->multi_curr = _item; \
    } \

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

/* Simple 1-1 command -> response macro */
#define CLUSTER_PROCESS_CMD(cmdname, resp_func, readcmd) \
    redisCluster *c = GET_CONTEXT(); \
    c->readonly = CLUSTER_IS_ATOMIC(c) && readcmd; \
    char *cmd; int cmd_len; short slot; void *ctx=NULL; \
    if(redis_##cmdname##_cmd(INTERNAL_FUNCTION_PARAM_PASSTHRU,c->flags, &cmd, \
                             &cmd_len, &slot, &ctx)==FAILURE) { \
        RETURN_FALSE; \
    } \
    if(cluster_send_command(c,slot,cmd,cmd_len)<0 || c->err!=NULL) {\
        efree(cmd); \
        RETURN_FALSE; \
    } \
    efree(cmd); \
    if(c->flags->mode == MULTI) { \
        CLUSTER_ENQUEUE_RESPONSE(c, slot, resp_func, ctx); \
        RETURN_ZVAL(getThis(), 1, 0); \
    } \
    resp_func(INTERNAL_FUNCTION_PARAM_PASSTHRU, c, ctx);

/* More generic processing, where only the keyword differs */
#define CLUSTER_PROCESS_KW_CMD(kw, cmdfunc, resp_func, readcmd) \
    redisCluster *c = GET_CONTEXT(); \
    c->readonly = CLUSTER_IS_ATOMIC(c) && readcmd; \
    char *cmd; int cmd_len; short slot; void *ctx=NULL; \
    if(cmdfunc(INTERNAL_FUNCTION_PARAM_PASSTHRU, c->flags, kw, &cmd, &cmd_len,\
               &slot,&ctx)==FAILURE) { \
        RETURN_FALSE; \
    } \
    if(cluster_send_command(c,slot,cmd,cmd_len)<0 || c->err!=NULL) { \
        efree(cmd); \
        RETURN_FALSE; \
    } \
    efree(cmd); \
    if(c->flags->mode == MULTI) { \
        CLUSTER_ENQUEUE_RESPONSE(c, slot, resp_func, ctx); \
        RETURN_ZVAL(getThis(), 1, 0); \
    } \
    resp_func(INTERNAL_FUNCTION_PARAM_PASSTHRU, c, ctx);

extern const zend_function_entry *redis_cluster_get_methods(void);

/* Create cluster context */
zend_object *create_cluster_context(zend_class_entry *class_type);

/* Free cluster context struct */
void free_cluster_context(zend_object *object);

#endif
