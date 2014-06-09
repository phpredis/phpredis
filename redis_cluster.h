#ifndef REDIS_CLUSTER_H
#define REDIS_CLUSTER_H

#include "cluster_library.h"
#include <php.h>
#include <stddef.h>

/* Redis cluster hash slots and N-1 which we'll use to find it */
#define REDIS_CLUSTER_SLOTS 16384
#define REDIS_CLUSTER_MOD   (REDIS_CLUSTER_SLOTS-1)

/* Get attached object context */
#define GET_CONTEXT() \
    (redisCluster*)zend_object_store_get_object(getThis() TSRMLS_CC)

/* Command building/processing is identical for every command */
#define CLUSTER_BUILD_CMD(name, c, cmd, cmd_len, slot) \
    redis_##name##_cmd(INTERNAL_FUNCTION_PARAM_PASSTHRU, c->flags, &cmd, \
                       &cmd_len, &slot)

/* Simple 1-1 command -> response macro */
#define CLUSTER_PROCESS_CMD(cmdname, resp_func) \
    redisCluster *c = GET_CONTEXT(); \
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
    resp_func(INTERNAL_FUNCTION_PARAM_PASSTHRU, c, ctx); 
        
/* More generic processing, where only the keyword differs */
#define CLUSTER_PROCESS_KW_CMD(kw, cmdfunc, resp_func) \
    redisCluster *c = GET_CONTEXT(); \
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
    resp_func(INTERNAL_FUNCTION_PARAM_PASSTHRU, c, ctx); 

/* For the creation of RedisCluster specific exceptions */
PHPAPI zend_class_entry *rediscluster_get_exception_base(int root TSRMLS_DC);

/* Create cluster context */
zend_object_value create_cluster_context(zend_class_entry *class_type 
                                         TSRMLS_DC);

/* Free cluster context struct */
void free_cluster_context(void *object TSRMLS_DC);

/* Inittialize our class with PHP */
void init_rediscluster(TSRMLS_D);

/* RedisCluster method implementation */
PHP_METHOD(RedisCluster, __construct);
PHP_METHOD(RedisCluster, get);
PHP_METHOD(RedisCluster, set);
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
PHP_METHOD(RedisCluster, rpushx);
PHP_METHOD(RedisCluster, lpushx);
PHP_METHOD(RedisCluster, linsert);
PHP_METHOD(RedisCluster, lrem);
PHP_METHOD(RedisCluster, brpoplpush);
PHP_METHOD(RedisCluster, rpoplpush);
PHP_METHOD(RedisCluster, llen);
PHP_METHOD(RedisCluster, scard);
PHP_METHOD(RedisCluster, smembers);
PHP_METHOD(RedisCluster, sismember);
PHP_METHOD(RedisCluster, sadd);
PHP_METHOD(RedisCluster, srem);
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
PHP_METHOD(RedisCluster, zunionstore);
PHP_METHOD(RedisCluster, zinterstore);
PHP_METHOD(RedisCluster, sort);
#endif
