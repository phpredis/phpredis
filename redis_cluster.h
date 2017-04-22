#ifndef REDIS_CLUSTER_H
#define REDIS_CLUSTER_H

#include "cluster_library.h"
#include <php.h>
#include <stddef.h>

/* Redis cluster hash slots and N-1 which we'll use to find it */
#define REDIS_CLUSTER_SLOTS 16384
#define REDIS_CLUSTER_MOD   (REDIS_CLUSTER_SLOTS-1)

/* Get attached object context */
#if (PHP_MAJOR_VERSION < 7)
#define GET_CONTEXT() \
    ((redisCluster*)zend_object_store_get_object(getThis() TSRMLS_CC))
#else
#define GET_CONTEXT() \
    ((redisCluster *)((char *)Z_OBJ_P(getThis()) - XtOffsetOf(redisCluster, std)))
#endif

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
    for(zend_hash_internal_pointer_reset(c->nodes); \
        (_node = zend_hash_get_current_data_ptr(c->nodes)) != NULL; \
        zend_hash_move_forward(c->nodes)) \
    { \
        _node->sock->watching = 0; \
        _node->sock->mode = ATOMIC; \
    } \
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
    if(cluster_send_command(c,slot,cmd,cmd_len TSRMLS_CC)<0 || c->err!=NULL) {\
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
    if(cluster_send_command(c,slot,cmd,cmd_len TSRMLS_CC)<0 || c->err!=NULL) { \
        efree(cmd); \
        RETURN_FALSE; \
    } \
    efree(cmd); \
    if(c->flags->mode == MULTI) { \
        CLUSTER_ENQUEUE_RESPONSE(c, slot, resp_func, ctx); \
        RETURN_ZVAL(getThis(), 1, 0); \
    } \
    resp_func(INTERNAL_FUNCTION_PARAM_PASSTHRU, c, ctx); 

/* For the creation of RedisCluster specific exceptions */
PHP_REDIS_API zend_class_entry *rediscluster_get_exception_base(int root TSRMLS_DC);

#if (PHP_MAJOR_VERSION < 7)
/* Create cluster context */
zend_object_value create_cluster_context(zend_class_entry *class_type TSRMLS_DC);
/* Free cluster context struct */
void free_cluster_context(void *object TSRMLS_DC);
#else
/* Create cluster context */
zend_object *create_cluster_context(zend_class_entry *class_type TSRMLS_DC);
/* Free cluster context struct */
void free_cluster_context(zend_object *object);
#endif


/* Inittialize our class with PHP */
void init_rediscluster(TSRMLS_D);

/* RedisCluster method implementation */
PHP_METHOD(RedisCluster, __construct);
PHP_METHOD(RedisCluster, close);
PHP_METHOD(RedisCluster, get);
PHP_METHOD(RedisCluster, set);
PHP_METHOD(RedisCluster, mget);
PHP_METHOD(RedisCluster, mset);
PHP_METHOD(RedisCluster, msetnx);
PHP_METHOD(RedisCluster, mset);
PHP_METHOD(RedisCluster, del);
PHP_METHOD(RedisCluster, dump);
PHP_METHOD(RedisCluster, setex);
PHP_METHOD(RedisCluster, psetex);
PHP_METHOD(RedisCluster, setnx);
PHP_METHOD(RedisCluster, getset);
PHP_METHOD(RedisCluster, exists);
PHP_METHOD(RedisCluster, keys);
PHP_METHOD(RedisCluster, type);
PHP_METHOD(RedisCluster, persist);
PHP_METHOD(RedisCluster, lpop);
PHP_METHOD(RedisCluster, rpop);
PHP_METHOD(RedisCluster, spop);
PHP_METHOD(RedisCluster, rpush);
PHP_METHOD(RedisCluster, lpush);
PHP_METHOD(RedisCluster, blpop);
PHP_METHOD(RedisCluster, brpop);
PHP_METHOD(RedisCluster, rpushx);
PHP_METHOD(RedisCluster, lpushx);
PHP_METHOD(RedisCluster, linsert);
PHP_METHOD(RedisCluster, lindex);
PHP_METHOD(RedisCluster, lrem);
PHP_METHOD(RedisCluster, brpoplpush);
PHP_METHOD(RedisCluster, rpoplpush);
PHP_METHOD(RedisCluster, llen);
PHP_METHOD(RedisCluster, scard);
PHP_METHOD(RedisCluster, smembers);
PHP_METHOD(RedisCluster, sismember);
PHP_METHOD(RedisCluster, sadd);
PHP_METHOD(RedisCluster, saddarray);
PHP_METHOD(RedisCluster, srem);
PHP_METHOD(RedisCluster, sunion);
PHP_METHOD(RedisCluster, sunionstore);
PHP_METHOD(RedisCluster, sinter);
PHP_METHOD(RedisCluster, sinterstore);
PHP_METHOD(RedisCluster, sdiff);
PHP_METHOD(RedisCluster, sdiffstore);
PHP_METHOD(RedisCluster, strlen);
PHP_METHOD(RedisCluster, ttl);
PHP_METHOD(RedisCluster, pttl);
PHP_METHOD(RedisCluster, zcard);
PHP_METHOD(RedisCluster, zscore);
PHP_METHOD(RedisCluster, zcount);
PHP_METHOD(RedisCluster, zrem);
PHP_METHOD(RedisCluster, zremrangebyscore);
PHP_METHOD(RedisCluster, zrank);
PHP_METHOD(RedisCluster, zrevrank);
PHP_METHOD(RedisCluster, zadd);
PHP_METHOD(RedisCluster, zincrby);
PHP_METHOD(RedisCluster, hlen);
PHP_METHOD(RedisCluster, hget);
PHP_METHOD(RedisCluster, hkeys);
PHP_METHOD(RedisCluster, hvals);
PHP_METHOD(RedisCluster, hmget);
PHP_METHOD(RedisCluster, hmset);
PHP_METHOD(RedisCluster, hdel);
PHP_METHOD(RedisCluster, hgetall);
PHP_METHOD(RedisCluster, hexists);
PHP_METHOD(RedisCluster, hincrby);
PHP_METHOD(RedisCluster, hincrbyfloat);
PHP_METHOD(RedisCluster, hset);
PHP_METHOD(RedisCluster, hsetnx);
PHP_METHOD(RedisCluster, hstrlen);
PHP_METHOD(RedisCluster, incr);
PHP_METHOD(RedisCluster, decr);
PHP_METHOD(RedisCluster, incrby);
PHP_METHOD(RedisCluster, decrby);
PHP_METHOD(RedisCluster, incrbyfloat);
PHP_METHOD(RedisCluster, expire);
PHP_METHOD(RedisCluster, expireat);
PHP_METHOD(RedisCluster, pexpire);
PHP_METHOD(RedisCluster, pexpireat);
PHP_METHOD(RedisCluster, append);
PHP_METHOD(RedisCluster, getbit);
PHP_METHOD(RedisCluster, setbit);
PHP_METHOD(RedisCluster, bitop);
PHP_METHOD(RedisCluster, bitpos);
PHP_METHOD(RedisCluster, bitcount);
PHP_METHOD(RedisCluster, lget);
PHP_METHOD(RedisCluster, getrange);
PHP_METHOD(RedisCluster, ltrim);
PHP_METHOD(RedisCluster, lrange);
PHP_METHOD(RedisCluster, zremrangebyrank);
PHP_METHOD(RedisCluster, publish);
PHP_METHOD(RedisCluster, lset);
PHP_METHOD(RedisCluster, rename);
PHP_METHOD(RedisCluster, renamenx);
PHP_METHOD(RedisCluster, pfcount);
PHP_METHOD(RedisCluster, pfadd);
PHP_METHOD(RedisCluster, pfmerge);
PHP_METHOD(RedisCluster, restore);
PHP_METHOD(RedisCluster, setrange);
PHP_METHOD(RedisCluster, smove);
PHP_METHOD(RedisCluster, srandmember);
PHP_METHOD(RedisCluster, zrange);
PHP_METHOD(RedisCluster, zrevrange);
PHP_METHOD(RedisCluster, zrangebyscore);
PHP_METHOD(RedisCluster, zrevrangebyscore);
PHP_METHOD(RedisCluster, zrangebylex);
PHP_METHOD(RedisCluster, zrevrangebylex);
PHP_METHOD(RedisCluster, zlexcount);
PHP_METHOD(RedisCluster, zremrangebylex);
PHP_METHOD(RedisCluster, zunionstore);
PHP_METHOD(RedisCluster, zinterstore);
PHP_METHOD(RedisCluster, sort);
PHP_METHOD(RedisCluster, object);
PHP_METHOD(RedisCluster, subscribe);
PHP_METHOD(RedisCluster, psubscribe);
PHP_METHOD(RedisCluster, unsubscribe);
PHP_METHOD(RedisCluster, punsubscribe);
PHP_METHOD(RedisCluster, eval);
PHP_METHOD(RedisCluster, evalsha);
PHP_METHOD(RedisCluster, info);
PHP_METHOD(RedisCluster, cluster);
PHP_METHOD(RedisCluster, client);
PHP_METHOD(RedisCluster, config);
PHP_METHOD(RedisCluster, pubsub);
PHP_METHOD(RedisCluster, script);
PHP_METHOD(RedisCluster, slowlog);
PHP_METHOD(RedisCluster, command);
PHP_METHOD(RedisCluster, geoadd);
PHP_METHOD(RedisCluster, geohash);
PHP_METHOD(RedisCluster, geopos);
PHP_METHOD(RedisCluster, geodist);
PHP_METHOD(RedisCluster, georadius);
PHP_METHOD(RedisCluster, georadiusbymember);

/* SCAN and friends */
PHP_METHOD(RedisCluster, scan);
PHP_METHOD(RedisCluster, zscan);
PHP_METHOD(RedisCluster, hscan);
PHP_METHOD(RedisCluster, sscan);

/* Transactions */
PHP_METHOD(RedisCluster, multi);
PHP_METHOD(RedisCluster, exec);
PHP_METHOD(RedisCluster, discard);
PHP_METHOD(RedisCluster, watch);
PHP_METHOD(RedisCluster, unwatch);

/* Commands we direct to a node */
PHP_METHOD(RedisCluster, save);
PHP_METHOD(RedisCluster, bgsave);
PHP_METHOD(RedisCluster, flushdb);
PHP_METHOD(RedisCluster, flushall);
PHP_METHOD(RedisCluster, dbsize);
PHP_METHOD(RedisCluster, bgrewriteaof);
PHP_METHOD(RedisCluster, lastsave);
PHP_METHOD(RedisCluster, role);
PHP_METHOD(RedisCluster, time);
PHP_METHOD(RedisCluster, randomkey);
PHP_METHOD(RedisCluster, ping);
PHP_METHOD(RedisCluster, echo);
PHP_METHOD(RedisCluster, rawcommand);

/* Introspection */
PHP_METHOD(RedisCluster, getmode);
PHP_METHOD(RedisCluster, getlasterror);
PHP_METHOD(RedisCluster, clearlasterror);
PHP_METHOD(RedisCluster, getoption);
PHP_METHOD(RedisCluster, setoption);
PHP_METHOD(RedisCluster, _prefix);
PHP_METHOD(RedisCluster, _serialize);
PHP_METHOD(RedisCluster, _unserialize);
PHP_METHOD(RedisCluster, _masters);
PHP_METHOD(RedisCluster, _redir);

#endif
