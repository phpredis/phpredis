/*
  +----------------------------------------------------------------------+
  | PHP Version 5                                                        |
  +----------------------------------------------------------------------+
  | Copyright (c) 1997-2009 The PHP Group                                |
  +----------------------------------------------------------------------+
  | This source file is subject to version 3.01 of the PHP license,      |
  | that is bundled with this package in the file LICENSE, and is        |
  | available through the world-wide-web at the following url:           |
  | http://www.php.net/license/3_01.txt                                  |
  | If you did not receive a copy of the PHP license and are unable to   |
  | obtain it through the world-wide-web, please send a note to          |
  | license@php.net so we can mail you a copy immediately.               |
  +----------------------------------------------------------------------+
  | Author: Michael Grunder <michael.grunder@gmail.com>                  |
  | Maintainer: Nicolas Favre-Felix <n.favre-felix@owlient.eu>           |
  +----------------------------------------------------------------------+
*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "common.h"
#include "php_redis.h"
#include "ext/standard/info.h"
#include "crc16.h"
#include "redis_cluster.h"
#include "redis_commands.h"
#include <zend_exceptions.h>
#include "library.h"

zend_class_entry *redis_cluster_ce;

/* Exception handler */
zend_class_entry *redis_cluster_exception_ce;
zend_class_entry *spl_rte_ce = NULL;

/* Argument info for HSCAN, SSCAN, HSCAN */
ZEND_BEGIN_ARG_INFO_EX(arginfo_kscan, 0, 0, 2)
    ZEND_ARG_INFO(0, str_key)
    ZEND_ARG_INFO(1, i_iterator)
    ZEND_ARG_INFO(0, str_pattern)
    ZEND_ARG_INFO(0, i_count)
ZEND_END_ARG_INFO();

/* Argument infor for SCAN */
ZEND_BEGIN_ARG_INFO_EX(arginfo_scan, 0, 0, 2)
    ZEND_ARG_INFO(1, i_iterator)
    ZEND_ARG_INFO(0, str_node)
    ZEND_ARG_INFO(0, str_pattern)
    ZEND_ARG_INFO(0, i_count)
ZEND_END_ARG_INFO();

/* Function table */
zend_function_entry redis_cluster_functions[] = {
    PHP_ME(RedisCluster, __construct, NULL, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, close, NULL, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, get, NULL, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, set, NULL, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, mget, NULL, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, mset, NULL, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, msetnx, NULL, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, del, NULL, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, setex, NULL, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, psetex, NULL, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, setnx, NULL, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, getset, NULL, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, exists, NULL, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, keys, NULL, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, type, NULL, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, lpop, NULL, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, rpop, NULL, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, lset, NULL, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, spop, NULL, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, lpush, NULL, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, rpush, NULL, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, blpop, NULL, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, brpop, NULL, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, rpushx, NULL, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, lpushx, NULL, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, linsert, NULL, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, lrem, NULL, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, brpoplpush, NULL, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, rpoplpush, NULL, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, llen, NULL, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, scard, NULL, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, smembers, NULL, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, sismember, NULL, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, sadd, NULL, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, srem, NULL, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, sunion, NULL, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, sunionstore, NULL, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, sinter, NULL, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, sinterstore, NULL, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, sdiff, NULL, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, sdiffstore, NULL, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, srandmember, NULL, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, strlen, NULL, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, persist, NULL, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, ttl, NULL, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, pttl, NULL, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, zcard, NULL, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, zcount, NULL, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, zremrangebyscore, NULL, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, zscore, NULL, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, zadd, NULL, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, zincrby, NULL, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, hlen, NULL, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, hkeys, NULL, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, hvals, NULL, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, hget, NULL, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, hgetall, NULL, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, hexists, NULL, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, hincrby, NULL, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, hset, NULL, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, hsetnx, NULL, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, hmget, NULL, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, hmset, NULL, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, hdel, NULL, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, hincrbyfloat, NULL, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, dump, NULL, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, zrank, NULL, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, zrevrank, NULL, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, incr, NULL, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, decr, NULL, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, incrby, NULL, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, decrby, NULL, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, incrbyfloat, NULL, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, expire, NULL, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, pexpire, NULL, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, expireat, NULL, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, pexpireat, NULL, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, append, NULL, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, getbit, NULL, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, setbit, NULL, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, bitop, NULL, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, bitpos, NULL, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, bitcount, NULL, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, lget, NULL, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, getrange, NULL, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, ltrim, NULL, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, lrange, NULL, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, zremrangebyrank, NULL, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, publish, NULL, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, rename, NULL, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, renamenx, NULL, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, pfcount, NULL, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, pfadd, NULL, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, pfmerge, NULL, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, setrange, NULL, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, restore, NULL, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, smove, NULL, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, zrange, NULL, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, zrevrange, NULL, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, zrangebyscore, NULL, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, zunionstore, NULL, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, zinterstore, NULL, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, zrem, NULL, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, sort, NULL, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, object, NULL, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, subscribe, NULL, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, psubscribe, NULL, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, unsubscribe, NULL, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, punsubscribe, NULL, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, eval, NULL, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, evalsha, NULL, ZEND_ACC_PUBLIC)

    PHP_ME(RedisCluster, scan, arginfo_scan, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, sscan, arginfo_kscan, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, zscan, arginfo_kscan, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, hscan, arginfo_kscan, ZEND_ACC_PUBLIC)

    PHP_ME(RedisCluster, getoption, NULL, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, setoption, NULL, ZEND_ACC_PUBLIC)

    PHP_ME(RedisCluster, _prefix, NULL, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, _serialize, NULL, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, _unserialize, NULL, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, _masters, NULL, ZEND_ACC_PUBLIC)
    
    PHP_ME(RedisCluster, multi, NULL, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, exec, NULL, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, discard, NULL, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, watch, NULL, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, unwatch, NULL, ZEND_ACC_PUBLIC)

    PHP_ME(RedisCluster, save, NULL, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, bgsave, NULL, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, flushdb, NULL, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, flushall, NULL, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, dbsize, NULL, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, bgrewriteaof, NULL, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, lastsave, NULL, ZEND_ACC_PUBLIC)
    {NULL, NULL, NULL}
};

/* Our context seeds will be a hash table with RedisSock* pointers */
static void ht_free_seed(void *data) {
    RedisSock *redis_sock = *(RedisSock**)data;
    if(redis_sock) redis_free_socket(redis_sock);
}

/* Free redisClusterNode objects we've stored */
static void ht_free_node(void *data) {
    redisClusterNode *node = *(redisClusterNode**)data;
    cluster_free_node(node);
}

/* Initialize/Register our RedisCluster exceptions */
PHPAPI zend_class_entry *rediscluster_get_exception_base(int root TSRMLS_DC) {
#if HAVE_SPL
    if(!root) {
        if(!spl_rte_ce) {
            zend_class_entry **pce;

            if(zend_hash_find(CG(class_table), "runtimeexception",
                              sizeof("runtimeexception"), (void**)&pce)
                              ==SUCCESS)
            {
                spl_rte_ce = *pce;
                return *pce;
            }
        } else {
            return spl_rte_ce;
        }
    }
#endif
#if (PHP_MAJOR_VERSION == 5) && (PHP_MINOR_VERSION < 2)
    return zend_exception_get_default();
#else
    return zend_exception_get_default(TSRMLS_C);
#endif
}

/* Create redisCluster context */
zend_object_value
create_cluster_context(zend_class_entry *class_type TSRMLS_DC) {
    zend_object_value retval;
    redisCluster *cluster;

    // Allocate our actual struct
    cluster = emalloc(sizeof(redisCluster));
    memset(cluster, 0, sizeof(redisCluster));

    // We're not currently subscribed anywhere
    cluster->subscribed_slot = -1;

    // Allocate our RedisSock we'll use to store prefix/serialization flags
    cluster->flags = ecalloc(1, sizeof(RedisSock));

    // Allocate our hash table for seeds
    ALLOC_HASHTABLE(cluster->seeds);
    zend_hash_init(cluster->seeds, 0, NULL, ht_free_seed, 0);

    // Allocate our hash table for connected Redis objects
    ALLOC_HASHTABLE(cluster->nodes);
    zend_hash_init(cluster->nodes, 0, NULL, ht_free_node, 0);

    // Initialize it
    zend_object_std_init(&cluster->std, class_type TSRMLS_CC);

#if PHP_VERSION_ID < 50399
    zval *tmp;

    zend_hash_copy(cluster->std.properties, &class_type->default_properties,
        (copy_ctor_func_t)zval_add_ref, (void*)&tmp, sizeof(zval*));
#endif

    retval.handle = zend_objects_store_put(cluster,
        (zend_objects_store_dtor_t)zend_objects_destroy_object,
        free_cluster_context, NULL TSRMLS_CC);

    retval.handlers = zend_get_std_object_handlers();

    return retval;
}

/* Free redisCluster context */
void free_cluster_context(void *object TSRMLS_DC) {
    redisCluster *cluster;

    // Grab context
    cluster = (redisCluster*)object;

    // Free any allocated prefix, as well as the struct
    if(cluster->flags->prefix) efree(cluster->flags->prefix);
    efree(cluster->flags);

    // Free seeds HashTable itself
    zend_hash_destroy(cluster->seeds);
    efree(cluster->seeds);

    // Destroy all Redis objects and free our nodes HashTable
    zend_hash_destroy(cluster->nodes);
    efree(cluster->nodes);

    if(cluster->err) efree(cluster->err);

    // Finally, free the redisCluster structure itself
    efree(cluster);
}

//
// PHP Methods
//

/* Create a RedisCluster Object */
PHP_METHOD(RedisCluster, __construct) {
    zval *object, *z_seeds=NULL;
    char *name;
    long name_len;
    double timeout = 0.0, read_timeout = 0.0;
    redisCluster *context = GET_CONTEXT();

    // Parse arguments
    if(zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(),
                                    "Os|add", &object, redis_cluster_ce, &name,
                                    &name_len, &z_seeds, &timeout,
                                    &read_timeout)==FAILURE)
    {
        RETURN_FALSE;
    }

    // Require a name
    if(name_len == 0) {
        zend_throw_exception(redis_cluster_exception_ce,
            "You must give this cluster a name!",
            0 TSRMLS_CC);
    }

    // Validate timeout
    if(timeout < 0L || timeout > INT_MAX) {
        zend_throw_exception(redis_cluster_exception_ce,
            "Invalid timeout", 0 TSRMLS_CC);
        RETURN_FALSE;
    }

    // Validate our read timeout
    if(read_timeout < 0L || read_timeout > INT_MAX) {
        zend_throw_exception(redis_cluster_exception_ce,
            "Invalid read timeout", 0 TSRMLS_CC);
        RETURN_FALSE;
    }

    // TODO: Implement seed retrieval from php.ini
    if(!z_seeds || zend_hash_num_elements(Z_ARRVAL_P(z_seeds))==0) {
        zend_throw_exception(redis_cluster_exception_ce,
            "Must pass seeds", 0 TSRMLS_CC);
        RETURN_FALSE;
    }

    // Initialize our RedisSock "seed" objects
    cluster_init_seeds(context, Z_ARRVAL_P(z_seeds));

    // Create and map our key space
    cluster_map_keyspace(context TSRMLS_CC);
}

/* 
 * RedisCluster method implementation
 */

/* {{{ proto bool RedisCluster::close() */
PHP_METHOD(RedisCluster, close) {
    cluster_disconnect(GET_CONTEXT() TSRMLS_CC);
    RETURN_TRUE;
}

/* {{{ proto string RedisCluster::get(string key) */
PHP_METHOD(RedisCluster, get) {
    CLUSTER_PROCESS_KW_CMD("GET", redis_key_cmd, cluster_bulk_resp);
}
/* }}} */

/* {{{ proto bool RedisCluster::set(string key, string value) */
PHP_METHOD(RedisCluster, set) {
    CLUSTER_PROCESS_CMD(set, cluster_bool_resp);
}
/* }}} */

/* Generic handler for MGET/MSET/MSETNX */
static int 
distcmd_resp_handler(INTERNAL_FUNCTION_PARAMETERS, redisCluster *c, short slot, 
                     clusterMultiCmd *mc, zval *z_ret, int last, cluster_cb cb)
{
    clusterMultiCtx *ctx;

    // Finalize multi command
    cluster_multi_fini(mc);

    // Spin up multi context
    ctx = emalloc(sizeof(clusterMultiCtx));
    ctx->z_multi = z_ret;
    ctx->count   = mc->argc;
    ctx->last    = last;

    // Attempt to send the command
    if(cluster_send_command(c,slot,mc->cmd.c,mc->cmd.len TSRMLS_CC)<0 ||
       c->err!=NULL)
    {
        cluster_multi_free(mc);
        zval_dtor(z_ret);
        efree(z_ret);
        efree(ctx);
        return -1;
    }

    if(CLUSTER_IS_ATOMIC(c)) {
        // Process response now
        cb(INTERNAL_FUNCTION_PARAM_PASSTHRU, c, (void*)ctx);
    } else {
        CLUSTER_ENQUEUE_RESPONSE(c, slot, cb, ctx);
    }

    // Clear out our command but retain allocated memory
    CLUSTER_MULTI_CLEAR(mc);

    return 0;
}

/* Container struct for a key/value pair pulled from an array */
typedef struct clusterKeyValHT {
    char kbuf[22];

    char  *key;
    int   key_len, key_free;
    short slot;

    char *val;
    int  val_len, val_free;
} clusterKeyValHT;

/* Helper to pull a key/value pair from a HashTable */
static int get_key_val_ht(redisCluster *c, HashTable *ht, HashPosition *ptr, 
                          clusterKeyValHT *kv TSRMLS_DC)
{
    zval **z_val;
    unsigned int key_len;
    ulong idx;

    // Grab the key, convert it to a string using provided kbuf buffer if it's
    // a LONG style key
    switch(zend_hash_get_current_key_ex(ht, &(kv->key), &key_len, &idx, 0, ptr)) 
    {
        case HASH_KEY_IS_LONG:
            kv->key_len = snprintf(kv->kbuf,sizeof(kv->kbuf),"%ld",(long)idx);
            kv->key     = kv->kbuf;
            break;
        case HASH_KEY_IS_STRING: 
            kv->key_len = (int)(key_len-1);
            break;
        default:
            zend_throw_exception(redis_cluster_exception_ce,
                "Internal Zend HashTable error", 0 TSRMLS_CC);
            return -1;
    }

    // Prefix our key if we need to, set the slot
    kv->key_free = redis_key_prefix(c->flags, &(kv->key), &(kv->key_len));
    kv->slot     = cluster_hash_key(kv->key, kv->key_len);

    // Now grab our value
    if(zend_hash_get_current_data_ex(ht, (void**)&z_val, ptr)==FAILURE) {
        zend_throw_exception(redis_cluster_exception_ce,
            "Internal Zend HashTable error", 0 TSRMLS_CC);
        return -1;
    }

    // Serialize our value if required
    kv->val_free = redis_serialize(c->flags,*z_val,&(kv->val),&(kv->val_len));

    // Success
    return 0;
}

/* Helper to pull, prefix, and hash a key from a HashTable value */
static int get_key_ht(redisCluster *c, HashTable *ht, HashPosition *ptr,
                      clusterKeyValHT *kv TSRMLS_DC)
{
    zval **z_key;

    if(zend_hash_get_current_data_ex(ht, (void**)&z_key, ptr)==FAILURE) {
        // Shouldn't happen, but check anyway
        zend_throw_exception(redis_cluster_exception_ce,
            "Internal Zend HashTable error", 0 TSRMLS_CC);
        return -1;
    }

    // Always want to work with strings
    convert_to_string(*z_key);

    kv->key = Z_STRVAL_PP(z_key);
    kv->key_len = Z_STRLEN_PP(z_key);
    kv->key_free = redis_key_prefix(c->flags, &(kv->key), &(kv->key_len));

    // Hash our key
    kv->slot = cluster_hash_key(kv->key, kv->key_len);

    // Success
    return 0;
}

/* Handler for both MGET and DEL */
static int cluster_mkey_cmd(INTERNAL_FUNCTION_PARAMETERS, char *kw, int kw_len,
                            zval *z_ret, cluster_cb cb)
{
    redisCluster *c = GET_CONTEXT();
    clusterMultiCmd mc = {0};
    clusterKeyValHT kv;
    zval *z_arr;
    HashTable *ht_arr;
    HashPosition ptr;
    int i=1, argc;
    short slot;

    // Parse our arguments
    if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "a", &z_arr)==FAILURE) {
        return -1;
    }

    // No reason to send zero arguments
    ht_arr = Z_ARRVAL_P(z_arr);
    if((argc = zend_hash_num_elements(ht_arr))==0) {
        return -1; 
    }

    // Initialize our "multi" command handler with command/len
    CLUSTER_MULTI_INIT(mc, kw, kw_len);

    // Process the first key outside of our loop, so we don't have to check if
    // it's the first iteration every time, needlessly
    zend_hash_internal_pointer_reset_ex(ht_arr, &ptr);
    if(get_key_ht(c, ht_arr, &ptr, &kv TSRMLS_CC)<0) {
        return -1;
    }

    // Process our key and add it to the command
    cluster_multi_add(&mc, kv.key, kv.key_len);

    // Free key if we prefixed
    if(kv.key_free) efree(kv.key);

    // Move to the next key
    zend_hash_move_forward_ex(ht_arr, &ptr);

    // Iterate over keys 2...N
    slot = kv.slot;
    while(zend_hash_has_more_elements_ex(ht_arr, &ptr)==SUCCESS) {
        if(get_key_ht(c, ht_arr, &ptr, &kv TSRMLS_CC)<0) {
            cluster_multi_free(&mc);
            return -1;
        }
    
        // If the slots have changed, kick off the keys we've aggregated
        if(slot != kv.slot) {
            // Process this batch of MGET keys
            if(distcmd_resp_handler(INTERNAL_FUNCTION_PARAM_PASSTHRU, c, slot, 
                                    &mc, z_ret, i==argc, cb)<0)
            {
                cluster_multi_free(&mc);
                return -1;
            }
        }

        // Add this key to the command
        cluster_multi_add(&mc, kv.key, kv.key_len);

        // Free key if we prefixed
        if(kv.key_free) efree(kv.key);

        // Update the last slot we encountered, and the key we're on
        slot = kv.slot; 
        i++;

        zend_hash_move_forward_ex(ht_arr, &ptr);
    }

    // If we've got straggler(s) process them
    if(mc.argc > 0) {
        if(distcmd_resp_handler(INTERNAL_FUNCTION_PARAM_PASSTHRU, c, slot, 
                                &mc, z_ret, 1, cb)<0)
        {
            cluster_multi_free(&mc);
            return -1;
        }
    }

    // Free our command
    cluster_multi_free(&mc);

    if(!CLUSTER_IS_ATOMIC(c))
        RETVAL_ZVAL(getThis(), 1, 0);

    // Success
    return 0;
}

/* Handler for both MSET and MSETNX */
static int cluster_mset_cmd(INTERNAL_FUNCTION_PARAMETERS, char *kw, int kw_len,
                            zval *z_ret, cluster_cb cb)
{
    redisCluster *c = GET_CONTEXT();
    clusterKeyValHT kv;
    clusterMultiCmd mc = {0};
    zval *z_arr;
    HashTable *ht_arr;
    HashPosition ptr;
    int i=1, argc;
    short slot;

    // Parse our arguments
    if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "a", &z_arr)==FAILURE) {
        return -1;
    }

    // No reason to send zero args
    ht_arr = Z_ARRVAL_P(z_arr);
    if((argc = zend_hash_num_elements(ht_arr))==0) {
        return -1;
    }

    // Set up our multi command handler
    CLUSTER_MULTI_INIT(mc, kw, kw_len);

    // Process the first key/value pair outside of our loop
    zend_hash_internal_pointer_reset_ex(ht_arr, &ptr);
    if(get_key_val_ht(c, ht_arr, &ptr, &kv TSRMLS_CC)==-1) return -1;
    zend_hash_move_forward_ex(ht_arr, &ptr);

    // Add this to our multi cmd, set slot, free key if we prefixed
    cluster_multi_add(&mc, kv.key, kv.key_len);
    cluster_multi_add(&mc, kv.val, kv.val_len);
    if(kv.key_free) efree(kv.key);
    if(kv.val_free) STR_FREE(kv.val);

    // While we've got more keys to set
    slot = kv.slot;
    while(zend_hash_has_more_elements_ex(ht_arr, &ptr)==SUCCESS) {
        // Pull the next key/value pair
        if(get_key_val_ht(c, ht_arr, &ptr, &kv TSRMLS_CC)==-1) {
            return -1;
        }

        // If the slots have changed, process responses
        if(slot != kv.slot) {
            if(distcmd_resp_handler(INTERNAL_FUNCTION_PARAM_PASSTHRU, c, 
                                    slot, &mc, z_ret, i==argc, cb)<0)
            {
                return -1;
            }
        }

        // Add this key and value to our command
        cluster_multi_add(&mc, kv.key, kv.key_len);
        cluster_multi_add(&mc, kv.val, kv.val_len);

        // Free our key and value if we need to
        if(kv.key_free) efree(kv.key);
        if(kv.val_free) STR_FREE(kv.val);

        // Update our slot, increment position
        slot = kv.slot;
        i++;

        // Move on
        zend_hash_move_forward_ex(ht_arr, &ptr);
    }

    // If we've got stragglers, process them too
    if(mc.argc > 0) {
        if(distcmd_resp_handler(INTERNAL_FUNCTION_PARAM_PASSTHRU, c, slot, &mc, 
                                z_ret, 1, cb)<0) 
        {
            return -1;
        }
    }

    // Free our command
    cluster_multi_free(&mc);

    // Success
    return 0;
}

/* {{{ proto array RedisCluster::del(string key1, string key2, ... keyN) */
PHP_METHOD(RedisCluster, del) {
    zval *z_ret;

    // Initialize a LONG value to zero for our return
    MAKE_STD_ZVAL(z_ret);
    ZVAL_LONG(z_ret, 0);

    // Parse args, process
    if(cluster_mkey_cmd(INTERNAL_FUNCTION_PARAM_PASSTHRU, "DEL", 
                        sizeof("DEL")-1, z_ret, cluster_del_resp)<0)
    {
        efree(z_ret);
        RETURN_FALSE;
    }
}

/* {{{ proto array RedisCluster::mget(array keys) */
PHP_METHOD(RedisCluster, mget) {
    zval *z_ret;

    // Array response
    MAKE_STD_ZVAL(z_ret);
    array_init(z_ret);

    // Parse args, process
    if(cluster_mkey_cmd(INTERNAL_FUNCTION_PARAM_PASSTHRU, "MGET",
                        sizeof("MGET")-1, z_ret, cluster_mbulk_mget_resp)<0)
    {
        zval_dtor(z_ret);
        efree(z_ret);
        RETURN_FALSE;
    }
}

/* {{{ proto bool RedisCluster::mset(array keyvalues) */
PHP_METHOD(RedisCluster, mset) {
    zval *z_ret;

    // Response, defaults to TRUE
    MAKE_STD_ZVAL(z_ret);
    ZVAL_TRUE(z_ret);

    // Parse args and process.  If we get a failure, free zval and return FALSE.
    if(cluster_mset_cmd(INTERNAL_FUNCTION_PARAM_PASSTHRU, "MSET", 
                        sizeof("MSET")-1, z_ret, cluster_mset_resp)==-1)
    {
        efree(z_ret);
        RETURN_FALSE;
    }
}

/* {{{ proto array RedisCluster::msetnx(array keyvalues) */
PHP_METHOD(RedisCluster, msetnx) {
    zval *z_ret;

    // Array response 
    MAKE_STD_ZVAL(z_ret);
    array_init(z_ret);

    // Parse args and process.  If we get a failure, free mem and return FALSE
    if(cluster_mset_cmd(INTERNAL_FUNCTION_PARAM_PASSTHRU, "MSETNX",
                        sizeof("MSETNX")-1, z_ret, cluster_msetnx_resp)==-1)
    {
        zval_dtor(z_ret);
        efree(z_ret);
        RETURN_FALSE;
    }
}
/* }}} */

/* {{{ proto bool RedisCluster::setex(string key, string value, int expiry) */
PHP_METHOD(RedisCluster, setex) {
    CLUSTER_PROCESS_KW_CMD("SETEX", redis_key_long_val_cmd, cluster_bool_resp);
}
/* }}} */

/* {{{ proto bool RedisCluster::psetex(string key, string value, int expiry) */
PHP_METHOD(RedisCluster, psetex) {
    CLUSTER_PROCESS_KW_CMD("PSETEX", redis_key_long_val_cmd, cluster_bool_resp);
}
/* }}} */

/* {{{ proto bool RedisCluster::setnx(string key, string value) */
PHP_METHOD(RedisCluster, setnx) {
    CLUSTER_PROCESS_KW_CMD("SETNX", redis_kv_cmd, cluster_1_resp);
}
/* }}} */

/* {{{ proto string RedisCluster::getSet(string key, string value) */
PHP_METHOD(RedisCluster, getset) {
    CLUSTER_PROCESS_KW_CMD("GETSET", redis_kv_cmd, cluster_bulk_resp);
}
/* }}} */

/* {{{ proto int RedisCluster::exists(string key) */
PHP_METHOD(RedisCluster, exists) {
    CLUSTER_PROCESS_KW_CMD("EXISTS", redis_key_cmd, cluster_long_resp);
}
/* }}} */

/* {{{ proto array Redis::keys(string pattern) */
PHP_METHOD(RedisCluster, keys) {
    // TODO: Figure out how to implement this, as we may want to send it across
    // all nodes (although that seems dangerous), or ask for a specified slot.
    zend_throw_exception(redis_cluster_exception_ce,
        "KEYS command not implemented", 0 TSRMLS_CC);
}
/* }}} */

/* {{{ proto int RedisCluster::type(string key) */
PHP_METHOD(RedisCluster, type) {
    CLUSTER_PROCESS_KW_CMD("TYPE", redis_key_cmd, cluster_type_resp);
}
/* }}} */

/* {{{ proto string RedisCluster::pop(string key) */
PHP_METHOD(RedisCluster, lpop) {
    CLUSTER_PROCESS_KW_CMD("LPOP", redis_key_cmd, cluster_bulk_resp);
}
/* }}} */

/* {{{ proto string RedisCluster::rpop(string key) */
PHP_METHOD(RedisCluster, rpop) {
    CLUSTER_PROCESS_KW_CMD("RPOP", redis_key_cmd, cluster_bulk_resp);
}
/* }}} */

/* {{{ proto bool RedisCluster::lset(string key, long index, string val) */
PHP_METHOD(RedisCluster, lset) {
    CLUSTER_PROCESS_KW_CMD("LSET", redis_key_long_val_cmd, cluster_bool_resp);
}
/* }}} */

/* {{{ proto string RedisCluster::spop(string key) */
PHP_METHOD(RedisCluster, spop) {
    CLUSTER_PROCESS_KW_CMD("SPOP", redis_key_cmd, cluster_bulk_resp);
}
/* }}} */

/* {{{ proto string|array RedisCluster::srandmember(string key, [long count]) */
PHP_METHOD(RedisCluster, srandmember) {
    redisCluster *c = GET_CONTEXT();
    char *cmd; int cmd_len; short slot;
    short have_count;

    if(redis_srandmember_cmd(INTERNAL_FUNCTION_PARAM_PASSTHRU, c->flags,
                             &cmd, &cmd_len, &slot, NULL, &have_count)
                             ==FAILURE)
    {
        RETURN_FALSE;
    }

    if(cluster_send_command(c,slot,cmd,cmd_len TSRMLS_CC)<0 || c->err!=NULL) {
        efree(cmd);
        RETURN_FALSE;
    }

    // Clean up command
    efree(cmd);

    // Response type differs if we use WITHSCORES or not
    if(have_count) {
        cluster_mbulk_resp(INTERNAL_FUNCTION_PARAM_PASSTHRU, c, NULL);
    } else {
        cluster_bulk_resp(INTERNAL_FUNCTION_PARAM_PASSTHRU, c, NULL);
    }
}

/* {{{ proto string RedisCluster::strlen(string key) */
PHP_METHOD(RedisCluster, strlen) {
    CLUSTER_PROCESS_KW_CMD("STRLEN", redis_key_cmd, cluster_bulk_resp);
}

/* {{{ proto long RedisCluster::lpush(string key, string val1, ... valN) */
PHP_METHOD(RedisCluster, lpush) {
    CLUSTER_PROCESS_KW_CMD("LPUSH", redis_key_varval_cmd, cluster_long_resp);
}
/* }}} */

/* {{{ proto long RedisCluster::rpush(string key, string val1, ... valN) */
PHP_METHOD(RedisCluster, rpush) {
    CLUSTER_PROCESS_KW_CMD("RPUSH", redis_key_varval_cmd, cluster_long_resp);
}
/* }}} */

/* {{{ proto array RedisCluster::blpop(string key1, ... keyN, long timeout) */
PHP_METHOD(RedisCluster, blpop) {
    CLUSTER_PROCESS_CMD(blpop, cluster_mbulk_resp);
}
/* }}} */

/* {{{ proto array RedisCluster::brpop(string key1, ... keyN, long timeout */
PHP_METHOD(RedisCluster, brpop) {
    CLUSTER_PROCESS_CMD(brpop, cluster_mbulk_resp);
}
/* }}} */

/* {{{ proto long RedisCluster::rpushx(string key, mixed value) */
PHP_METHOD(RedisCluster, rpushx) {
    CLUSTER_PROCESS_KW_CMD("RPUSHX", redis_kv_cmd, cluster_long_resp);
}
/* }}} */

/* {{{ proto long RedisCluster::lpushx(string key, mixed value) */
PHP_METHOD(RedisCluster, lpushx) {
    CLUSTER_PROCESS_KW_CMD("LPUSHX", redis_kv_cmd, cluster_long_resp);
}
/* }}} */

/* {{{ proto long RedisCluster::linsert(string k,string pos,mix pvt,mix val) */
PHP_METHOD(RedisCluster, linsert) {
    CLUSTER_PROCESS_CMD(linsert, cluster_long_resp);
}
/* }}} */

/* {{{ proto long RedisCluster::lrem(string key, long count, string val) */
PHP_METHOD(RedisCluster, lrem) {
    CLUSTER_PROCESS_CMD(lrem, cluster_long_resp);
}
/* }}} */

/* {{{ proto string RedisCluster::rpoplpush(string key, string key) */
PHP_METHOD(RedisCluster, rpoplpush) {
    CLUSTER_PROCESS_KW_CMD("RPOPLPUSH", redis_key_key_cmd, cluster_bulk_resp);
}
/* }}} */

/* {{{ proto string RedisCluster::brpoplpush(string key, string key, long tm) */
PHP_METHOD(RedisCluster, brpoplpush) {
    CLUSTER_PROCESS_CMD(brpoplpush, cluster_bulk_resp);
}
/* }}} */

/* {{{ proto long RedisCluster::llen(string key)  */
PHP_METHOD(RedisCluster, llen) {
    CLUSTER_PROCESS_KW_CMD("LLEN", redis_key_cmd, cluster_long_resp);
}
/* }}} */

/* {{{ proto long RedisCluster::scard(string key) */
PHP_METHOD(RedisCluster, scard) {
    CLUSTER_PROCESS_KW_CMD("SCARD", redis_key_cmd, cluster_long_resp);
}
/* }}} */

/* {{{ proto array RedisCluster::smembers(string key) */
PHP_METHOD(RedisCluster, smembers) {
    CLUSTER_PROCESS_KW_CMD("SMEMBERS", redis_key_cmd, cluster_mbulk_resp);
}
/* }}} */

/* {{{ proto long RedisCluster::sismember(string key) */
PHP_METHOD(RedisCluster, sismember) {
    CLUSTER_PROCESS_KW_CMD("SISMEMBER", redis_kv_cmd, cluster_1_resp);
}
/* }}} */

/* {{{ proto long RedisCluster::sadd(string key, string val1 [, ...]) */
PHP_METHOD(RedisCluster, sadd) {
    CLUSTER_PROCESS_KW_CMD("SADD", redis_key_varval_cmd, cluster_long_resp);
}
/* }}} */

/* {{{ proto long RedisCluster::srem(string key, string val1 [, ...]) */
PHP_METHOD(RedisCluster, srem) {
    CLUSTER_PROCESS_KW_CMD("SREM", redis_key_varval_cmd, cluster_long_resp);
}
/* }}} */

/* {{{ proto array RedisCluster::sunion(string key1, ... keyN) */
PHP_METHOD(RedisCluster, sunion) {
    CLUSTER_PROCESS_CMD(sunion, cluster_mbulk_resp);
}
/* }}} */

/* {{{ proto long RedisCluster::sunionstore(string dst, string k1, ... kN) */
PHP_METHOD(RedisCluster, sunionstore) {
    CLUSTER_PROCESS_CMD(sunionstore, cluster_long_resp);
}
/* }}} */

/* {{{ ptoto array RedisCluster::sinter(string k1, ... kN) */
PHP_METHOD(RedisCluster, sinter) {
    CLUSTER_PROCESS_CMD(sinter, cluster_mbulk_resp);
}
/* }}} */

/* {{{ ptoto long RedisCluster::sinterstore(string dst, string k1, ... kN) */
PHP_METHOD(RedisCluster, sinterstore) {
    CLUSTER_PROCESS_CMD(sinterstore, cluster_long_resp);
}
/* }}} */

/* {{{ proto array RedisCluster::sdiff(string k1, ... kN) */
PHP_METHOD(RedisCluster, sdiff) {
    CLUSTER_PROCESS_CMD(sdiff, cluster_mbulk_resp);
}
/* }}} */

/* {{{ proto long RedisCluster::sdiffstore(string dst, string k1, ... kN) */
PHP_METHOD(RedisCluster, sdiffstore) {
    CLUSTER_PROCESS_CMD(sdiffstore, cluster_long_resp);
}
/* }}} */

/* {{{ proto bool RedisCluster::smove(sting src, string dst, string mem) */
PHP_METHOD(RedisCluster, smove) {
    CLUSTER_PROCESS_CMD(smove, cluster_1_resp);
}
/* }}} */

/* {{{ proto bool RedisCluster::persist(string key) */
PHP_METHOD(RedisCluster, persist) {
    CLUSTER_PROCESS_KW_CMD("PERSIST", redis_key_cmd, cluster_1_resp);
}
/* }}} */

/* {{{ proto long RedisCluster::ttl(string key) */
PHP_METHOD(RedisCluster, ttl) {
    CLUSTER_PROCESS_KW_CMD("TTL", redis_key_cmd, cluster_long_resp);
}
/* }}} */

/* {{{ proto long RedisCluster::pttl(string key) */
PHP_METHOD(RedisCluster, pttl) {
    CLUSTER_PROCESS_KW_CMD("PTTL", redis_key_cmd, cluster_long_resp);
}
/* }}} */

/* {{{ proto long RedisCluster::zcard(string key) */
PHP_METHOD(RedisCluster, zcard) {
    CLUSTER_PROCESS_KW_CMD("ZCARD", redis_key_cmd, cluster_long_resp);
}
/* }}} */

/* {{{ proto double RedisCluster::zscore(string key) */
PHP_METHOD(RedisCluster, zscore) {
    CLUSTER_PROCESS_KW_CMD("ZSCORE", redis_kv_cmd, cluster_dbl_resp);
}
/* }}} */

/* {{{ proto long RedisCluster::zadd(string key,double score,string mem, ...) */
PHP_METHOD(RedisCluster, zadd) {
    CLUSTER_PROCESS_CMD(zadd, cluster_long_resp);
}
/* }}} */

/* {{{ proto double RedisCluster::zincrby(string key, double by, string mem) */
PHP_METHOD(RedisCluster, zincrby) {
    CLUSTER_PROCESS_CMD(zincrby, cluster_dbl_resp);
}
/* }}} */

/* {{{ proto RedisCluster::zremrangebyscore(string k, string s, string e) */
PHP_METHOD(RedisCluster, zremrangebyscore) {
    CLUSTER_PROCESS_KW_CMD("ZREMRANGEBYSCORE", redis_key_str_str_cmd,
        cluster_long_resp);
}
/* }}} */

/* {{{ proto RedisCluster::zcount(string key, string s, string e) */
PHP_METHOD(RedisCluster, zcount) {
    CLUSTER_PROCESS_KW_CMD("ZCOUNT", redis_key_str_str_cmd, cluster_long_resp);
}
/* }}} */

/* {{{ proto long RedisCluster::zrank(string key, mixed member) */
PHP_METHOD(RedisCluster, zrank) {
    CLUSTER_PROCESS_KW_CMD("ZRANK", redis_kv_cmd, cluster_long_resp);
}
/* }}} */

/* {{{ proto long RedisCluster::zrevrank(string key, mixed member) */
PHP_METHOD(RedisCluster, zrevrank) {
    CLUSTER_PROCESS_KW_CMD("ZREVRANK", redis_kv_cmd, cluster_long_resp);
}
/* }}} */

/* {{{ proto long RedisCluster::hlen(string key) */
PHP_METHOD(RedisCluster, hlen) {
    CLUSTER_PROCESS_KW_CMD("HLEN", redis_key_cmd, cluster_long_resp);
}
/* }}} */

/* {{{ proto array RedisCluster::hkeys(string key) */
PHP_METHOD(RedisCluster, hkeys) {
    CLUSTER_PROCESS_KW_CMD("HKEYS", redis_key_cmd, cluster_mbulk_raw_resp);
}
/* }}} */

/* {{{ proto array RedisCluster::hvals(string key) */
PHP_METHOD(RedisCluster, hvals) {
    CLUSTER_PROCESS_KW_CMD("HVALS", redis_key_cmd, cluster_mbulk_resp);
}
/* }}} */

/* {{{ proto string RedisCluster::hget(string key, string mem) */
PHP_METHOD(RedisCluster, hget) {
    CLUSTER_PROCESS_KW_CMD("HGET", redis_key_str_cmd, cluster_bulk_resp);
}
/* }}} */

/* {{{ proto bool RedisCluster::hset(string key, string mem, string val) */
PHP_METHOD(RedisCluster, hset) {
    CLUSTER_PROCESS_CMD(hset, cluster_long_resp);
}
/* }}} */

/* {{{ proto bool RedisCluster::hsetnx(string key, string mem, string val) */
PHP_METHOD(RedisCluster, hsetnx) {
    CLUSTER_PROCESS_CMD(hsetnx, cluster_1_resp);
}
/* }}} */

/* {{{ proto array RedisCluster::hgetall(string key) */
PHP_METHOD(RedisCluster, hgetall) {
    CLUSTER_PROCESS_KW_CMD("HGETALL", redis_key_cmd,
        cluster_mbulk_zipstr_resp);
}
/* }}} */

/* {{{ proto bool RedisCluster::hexists(string key, string member) */
PHP_METHOD(RedisCluster, hexists) {
    CLUSTER_PROCESS_KW_CMD("HEXISTS", redis_key_str_cmd, cluster_1_resp);
}
/* }}} */

/* {{{ proto long RedisCluster::hincr(string key, string mem, long val) */
PHP_METHOD(RedisCluster, hincrby) {
    CLUSTER_PROCESS_CMD(hincrby, cluster_long_resp);
}
/* }}} */

/* {{{ proto double RedisCluster::hincrbyfloat(string k, string m, double v) */
PHP_METHOD(RedisCluster, hincrbyfloat) {
    CLUSTER_PROCESS_CMD(hincrbyfloat, cluster_dbl_resp);
}
/* }}} */

/* {{{ proto bool RedisCluster::hmset(string key, array key_vals) */
PHP_METHOD(RedisCluster, hmset) {
    CLUSTER_PROCESS_CMD(hmset, cluster_bool_resp);
}
/* }}} */

/* {{{ proto long RedisCluster::hdel(string key, string mem1, ... memN) */
PHP_METHOD(RedisCluster, hdel) {
    CLUSTER_PROCESS_CMD(hdel, cluster_long_resp);
}
/* }}} */

/* {{{ proto array RedisCluster::hmget(string key, array members) */
PHP_METHOD(RedisCluster, hmget) {
    CLUSTER_PROCESS_CMD(hmget, cluster_mbulk_assoc_resp);
}
/* }}} */

/* {{{ proto string RedisCluster::dump(string key) */
PHP_METHOD(RedisCluster, dump) {
    CLUSTER_PROCESS_KW_CMD("DUMP", redis_key_cmd, cluster_bulk_raw_resp);
}

/* {{{ proto long RedisCluster::incr(string key) */
PHP_METHOD(RedisCluster, incr) {
    CLUSTER_PROCESS_KW_CMD("INCR", redis_key_cmd, cluster_long_resp);
}
/* }}} */

/* {{{ proto long RedisCluster::incrby(string key, long byval) */
PHP_METHOD(RedisCluster, incrby) {
    CLUSTER_PROCESS_KW_CMD("INCRBY", redis_key_long_cmd, cluster_long_resp);
}
/* }}} */

/* {{{ proto long RedisCluster::decr(string key) */
PHP_METHOD(RedisCluster, decr) {
    CLUSTER_PROCESS_KW_CMD("DECR", redis_key_cmd, cluster_long_resp);
}
/* }}} */

/* {{{ proto long RedisCluster::decrby(string key, long byval) */
PHP_METHOD(RedisCluster, decrby) {
    CLUSTER_PROCESS_KW_CMD("DECRBY", redis_key_long_cmd, cluster_long_resp);
}
/* }}} */

/* {{{ proto double RedisCluster::incrbyfloat(string key, double val) */
PHP_METHOD(RedisCluster, incrbyfloat) {
    CLUSTER_PROCESS_KW_CMD("INCRBYFLOAT", redis_key_dbl_cmd,
        cluster_dbl_resp);
}
/* }}} */

/* {{{ proto double RedisCluster::decrbyfloat(string key, double val) */
PHP_METHOD(RedisCluster, decrbyfloat) {
    CLUSTER_PROCESS_KW_CMD("DECRBYFLOAT", redis_key_dbl_cmd,
        cluster_dbl_resp);
}
/* }}} */

/* {{{ proto bool RedisCluster::expire(string key, long sec) */
PHP_METHOD(RedisCluster, expire) {
    CLUSTER_PROCESS_KW_CMD("EXPIRE", redis_key_long_cmd, cluster_1_resp);
}
/* }}} */

/* {{{ proto bool RedisCluster::expireat(string key, long ts) */
PHP_METHOD(RedisCluster, expireat) {
    CLUSTER_PROCESS_KW_CMD("EXPIREAT", redis_key_long_cmd, cluster_1_resp);
}

/* {{{ proto bool RedisCluster::pexpire(string key, long ms) */
PHP_METHOD(RedisCluster, pexpire) {
    CLUSTER_PROCESS_KW_CMD("PEXPIRE", redis_key_long_cmd, cluster_1_resp);
}
/* }}} */

/* {{{ proto bool RedisCluster::pexpireat(string key, long ts) */
PHP_METHOD(RedisCluster, pexpireat) {
    CLUSTER_PROCESS_KW_CMD("PEXPIREAT", redis_key_long_cmd, cluster_1_resp);
}
/* }}} */

/* {{{ proto long RedisCluster::append(string key, string val) */
PHP_METHOD(RedisCluster, append) {
    CLUSTER_PROCESS_KW_CMD("APPEND", redis_kv_cmd, cluster_long_resp);
}
/* }}} */

/* {{{ proto long RedisCluster::getbit(string key, long val) */
PHP_METHOD(RedisCluster, getbit) {
    CLUSTER_PROCESS_KW_CMD("GETBIT", redis_key_long_cmd, cluster_long_resp);
}
/* }}} */

/* {{{ proto long RedisCluster::setbit(string key, long offset, bool onoff) */
PHP_METHOD(RedisCluster, setbit) {
    CLUSTER_PROCESS_CMD(setbit, cluster_long_resp);
}

/* {{{ proto long RedisCluster::bitop(string op,string key,[string key2,...]) */
PHP_METHOD(RedisCluster, bitop)
{
    CLUSTER_PROCESS_CMD(bitop, cluster_long_resp);
}
/* }}} */

/* {{{ proto long RedisCluster::bitcount(string key, [int start, int end]) */
PHP_METHOD(RedisCluster, bitcount) {
    CLUSTER_PROCESS_CMD(bitcount, cluster_long_resp);
}
/* }}} */

/* {{{ proto long RedisCluster::bitpos(string key, int bit, [int s, int end]) */
PHP_METHOD(RedisCluster, bitpos) {
    CLUSTER_PROCESS_CMD(bitpos, cluster_long_resp);
}
/* }}} */

/* {{{ proto string Redis::lget(string key, long index) */
PHP_METHOD(RedisCluster, lget) {
    CLUSTER_PROCESS_KW_CMD("LGET", redis_key_long_cmd, cluster_bulk_resp);
}
/* }}} */

/* {{{ proto string RedisCluster::getrange(string key, long start, long end) */
PHP_METHOD(RedisCluster, getrange) {
    CLUSTER_PROCESS_KW_CMD("GETRANGE", redis_key_long_long_cmd,
        cluster_bulk_resp);
}
/* }}} */

/* {{{ proto string RedisCluster::ltrim(string key, long start, long end) */
PHP_METHOD(RedisCluster, ltrim) {
    CLUSTER_PROCESS_KW_CMD("LTRIM", redis_key_long_long_cmd, cluster_bool_resp);
}
/* }}} */

/* {{{ proto array RedisCluster::lrange(string key, long start, long end) */
PHP_METHOD(RedisCluster, lrange) {
    CLUSTER_PROCESS_KW_CMD("LRANGE", redis_key_long_long_cmd,
        cluster_mbulk_resp);
}
/* }}} */

/* {{{ proto long RedisCluster::zremrangebyrank(string k, long s, long e) */
PHP_METHOD(RedisCluster, zremrangebyrank) {
    CLUSTER_PROCESS_KW_CMD("ZREMRANGEBYRANK", redis_key_long_long_cmd,
        cluster_long_resp);
}
/* }}} */

/* {{{ proto long RedisCluster::publish(string key, string msg) */
PHP_METHOD(RedisCluster, publish) {
    CLUSTER_PROCESS_KW_CMD("PUBLISH", redis_key_str_cmd, cluster_long_resp);
}
/* }}} */

/* {{{ proto bool RedisCluster::rename(string key1, string key2) */
PHP_METHOD(RedisCluster, rename) {
    CLUSTER_PROCESS_KW_CMD("RENAME", redis_key_key_cmd, cluster_bool_resp);
}
/* }}} */

/* {{{ proto bool RedisCluster::renamenx(string key1, string key2) */
PHP_METHOD(RedisCluster, renamenx) {
    CLUSTER_PROCESS_KW_CMD("RENAMENX", redis_key_key_cmd, cluster_1_resp);
}
/* }}} */

/* {{{ proto long RedisCluster::pfcount(string key) */
PHP_METHOD(RedisCluster, pfcount) {
    CLUSTER_PROCESS_KW_CMD("PFCOUNT", redis_key_cmd, cluster_long_resp);
}
/* }}} */

/* {{{ proto bool RedisCluster::pfadd(string key, array vals) */
PHP_METHOD(RedisCluster, pfadd) {
    CLUSTER_PROCESS_CMD(pfadd, cluster_1_resp);
}
/* }}} */

/* {{{ proto bool RedisCluster::pfmerge(string key, array keys) */
PHP_METHOD(RedisCluster, pfmerge) {
    CLUSTER_PROCESS_CMD(pfmerge, cluster_bool_resp);
}
/* }}} */

/* {{{ proto boolean RedisCluster::restore(string key, long ttl, string val) */
PHP_METHOD(RedisCluster, restore) {
    CLUSTER_PROCESS_KW_CMD("RESTORE", redis_key_long_str_cmd,
        cluster_bool_resp);
}
/* }}} */

/* {{{ proto long RedisCluster::setrange(string key, long offset, string val) */
PHP_METHOD(RedisCluster, setrange) {
    CLUSTER_PROCESS_KW_CMD("SETRANGE", redis_key_long_str_cmd,
        cluster_long_resp);
}
/* }}} */

/* Generic implementation for ZRANGE, ZREVRANGE, ZRANGEBYSCORE,
 * ZREVRANGEBYSCORE */
static void generic_zrange_cmd(INTERNAL_FUNCTION_PARAMETERS, char *kw,
                               zrange_cb fun)
{
    redisCluster *c = GET_CONTEXT();
    char *cmd; int cmd_len; short slot;
    int withscores;

    if(fun(INTERNAL_FUNCTION_PARAM_PASSTHRU, c->flags, kw, &cmd, &cmd_len,
           &withscores, &slot, NULL)==FAILURE)
    {
        efree(cmd);
        RETURN_FALSE;
    }

    if(cluster_send_command(c,slot,cmd,cmd_len TSRMLS_CC)<0 || c->err!=NULL) {
        efree(cmd);
        RETURN_FALSE;
    }

    efree(cmd);

    // Response type differs if we use WITHSCORES or not
    if(!withscores) {
        cluster_mbulk_resp(INTERNAL_FUNCTION_PARAM_PASSTHRU, c, NULL);
    } else {
        cluster_mbulk_zipdbl_resp(INTERNAL_FUNCTION_PARAM_PASSTHRU, c, NULL);
    }
}

/* {{{ proto
 *     array RedisCluster::zrange(string k, long s, long e, bool score=0) */
PHP_METHOD(RedisCluster, zrange) {
    generic_zrange_cmd(INTERNAL_FUNCTION_PARAM_PASSTHRU, "ZRANGE",
        redis_zrange_cmd);
}
/* }}} */

/* {{{ proto
 *     array RedisCluster::zrevrange(string k,long s,long e,bool scores=0) */
PHP_METHOD(RedisCluster, zrevrange) {
    generic_zrange_cmd(INTERNAL_FUNCTION_PARAM_PASSTHRU, "ZREVRANGE",
        redis_zrange_cmd);
}
/* }}} */

/* {{{ proto array
 *     RedisCluster::zrangebyscore(string k, long s, long e, array opts) */
PHP_METHOD(RedisCluster, zrangebyscore) {
    generic_zrange_cmd(INTERNAL_FUNCTION_PARAM_PASSTHRU, "ZRANGEBYSCORE",
        redis_zrangebyscore_cmd);
}
/* }}} */

/* {{{ proto RedisCluster::zunionstore(string dst, array keys, [array weights,
 *                                     string agg]) */
PHP_METHOD(RedisCluster, zunionstore) {
    CLUSTER_PROCESS_KW_CMD("ZUNIONSTORE", redis_zinter_cmd, cluster_long_resp);
}
/* }}} */

/* {{{ proto RedisCluster::zinterstore(string dst, array keys, [array weights,
 *                                     string agg]) */
PHP_METHOD(RedisCluster, zinterstore) {
    CLUSTER_PROCESS_KW_CMD("ZINTERSTORE", redis_zinter_cmd, cluster_long_resp);
}
/* }}} */

/* {{{ proto RedisCluster::zrem(string key, string val1, ... valN) */
PHP_METHOD(RedisCluster, zrem) {
    CLUSTER_PROCESS_KW_CMD("ZREM", redis_key_varval_cmd, cluster_long_resp);
}
/* }}} */

/* {{{ proto array
 *     RedisCluster::zrevrangebyscore(string k, long s, long e, array opts) */
PHP_METHOD(RedisCluster, zrevrangebyscore) {
    generic_zrange_cmd(INTERNAL_FUNCTION_PARAM_PASSTHRU, "ZREVRANGEBYSCORE",
        redis_zrangebyscore_cmd);
}
/* }}} */


/* {{{ proto RedisCluster::sort(string key, array options) */
PHP_METHOD(RedisCluster, sort) {
    redisCluster *c = GET_CONTEXT();
    char *cmd; int cmd_len, have_store; short slot;

    if(redis_sort_cmd(INTERNAL_FUNCTION_PARAM_PASSTHRU, c->flags, &have_store,
                      &cmd, &cmd_len, &slot, NULL)==FAILURE)
    {
        RETURN_FALSE;
    }

    if(cluster_send_command(c,slot,cmd,cmd_len TSRMLS_CC)<0 || c->err!=NULL) {
        efree(cmd);
        RETURN_FALSE;
    }

    efree(cmd);

    // Response type differs based on presence of STORE argument
    if(!have_store) {
        cluster_mbulk_resp(INTERNAL_FUNCTION_PARAM_PASSTHRU, c, NULL);
    } else {
        cluster_long_resp(INTERNAL_FUNCTION_PARAM_PASSTHRU, c, NULL);
    }
}

/* {{{ proto RedisCluster::object(string subcmd, string key) */
PHP_METHOD(RedisCluster, object) {
    redisCluster *c = GET_CONTEXT();
    char *cmd; int cmd_len; short slot;
    REDIS_REPLY_TYPE rtype;

    if(redis_object_cmd(INTERNAL_FUNCTION_PARAM_PASSTHRU, c->flags, &rtype,
                        &cmd, &cmd_len, &slot, NULL)==FAILURE)
    {
        RETURN_FALSE;
    }

    if(cluster_send_command(c,slot,cmd,cmd_len TSRMLS_CC)<0 || c->err!=NULL) {
        efree(cmd);
        RETURN_FALSE;
    }

    efree(cmd);

    // Use the correct response type
    if(rtype == TYPE_INT) {
        cluster_long_resp(INTERNAL_FUNCTION_PARAM_PASSTHRU, c, NULL);
    } else {
        cluster_bulk_resp(INTERNAL_FUNCTION_PARAM_PASSTHRU, c, NULL);
    }
}

/* {{{ proto null RedisCluster::subscribe(array chans, callable cb) */
PHP_METHOD(RedisCluster, subscribe) {
    CLUSTER_PROCESS_KW_CMD("SUBSCRIBE", redis_subscribe_cmd, cluster_sub_resp);
}
/* }}} */

/* {{{ proto null RedisCluster::psubscribe(array pats, callable cb) */
PHP_METHOD(RedisCluster, psubscribe) {
    CLUSTER_PROCESS_KW_CMD("PSUBSCRIBE", redis_subscribe_cmd, cluster_sub_resp);
}
/* }}} */

static void generic_unsub_cmd(INTERNAL_FUNCTION_PARAMETERS, redisCluster *c,
                              char *kw)
{
    char *cmd;
    int cmd_len;
    void *ctx;
    short slot;

    // There is not reason to unsubscribe outside of a subscribe loop
    if(c->subscribed_slot == -1) {
        php_error_docref(0 TSRMLS_CC, E_WARNING,
            "You can't unsubscribe outside of a subscribe loop");
        RETURN_FALSE;
    }

    // Call directly because we're going to set the slot manually
    if(redis_unsubscribe_cmd(INTERNAL_FUNCTION_PARAM_PASSTHRU, c->flags, kw, 
                             &cmd, &cmd_len, &slot, &ctx)
                             ==FAILURE)
    {
        RETURN_FALSE;
    }

    // This has to operate on our subscribe slot
    if(cluster_send_slot(c, c->subscribed_slot, cmd, cmd_len, TYPE_MULTIBULK)
                         ==FAILURE)
    {
        zend_throw_exception(redis_cluster_exception_ce,
            "Failed to UNSUBSCRIBE within our subscribe loop!", 0 TSRMLS_CC);
        RETURN_FALSE;
    }

    // Now process response from the slot we're subscribed on
    cluster_unsub_resp(INTERNAL_FUNCTION_PARAM_PASSTHRU, c, ctx);

    // Cleanup our command
    efree(cmd);
}

/* {{{ proto array RedisCluster::unsubscribe(array chans) */
PHP_METHOD(RedisCluster, unsubscribe) {
    generic_unsub_cmd(INTERNAL_FUNCTION_PARAM_PASSTHRU, GET_CONTEXT(), 
        "UNSUBSCRIBE");
}
/* }}} */

/* {{{ proto array RedisCluster::punsubscribe(array pats) */
PHP_METHOD(RedisCluster, punsubscribe) {
    generic_unsub_cmd(INTERNAL_FUNCTION_PARAM_PASSTHRU, GET_CONTEXT(),
        "PUNSUBSCRIBE");
}
/* }}} */

/* Parse arguments for EVAL or EVALSHA in the context of cluster.  If we aren't
 * provided any "keys" as arguments, the only choice is to send the command to
 * a random node in the cluster.  If we are passed key arguments the best we
 * can do is make sure they all map to the same "node", as we don't know what
 * the user is actually doing in the LUA source itself. */
/* EVAL/EVALSHA */
static void cluster_eval_cmd(INTERNAL_FUNCTION_PARAMETERS, redisCluster *c,
                            char *kw, int kw_len)
{
    redisClusterNode *node=NULL;
    char *lua, *key;
    int key_free, args_count=0, lua_len, key_len;
    zval *z_arr=NULL, **z_ele;
    HashTable *ht_arr;
    HashPosition ptr;
    long num_keys = 0;
    short slot;
    smart_str cmdstr = {0};

    /* Parse args */
    if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s|al", &lua, &lua_len,
                             &z_arr, &num_keys)==FAILURE)
    {
        RETURN_FALSE;
    }

    /* Grab arg count */
    if(z_arr != NULL) {
        ht_arr = Z_ARRVAL_P(z_arr);
        args_count = zend_hash_num_elements(ht_arr);
    }

    /* Format header, add script or SHA, and the number of args which are keys */
    redis_cmd_init_sstr(&cmdstr, 2 + args_count, kw, kw_len);
    redis_cmd_append_sstr(&cmdstr, lua, lua_len);
    redis_cmd_append_sstr_long(&cmdstr, num_keys);

    // Iterate over our args if we have any
    if(args_count > 0) {
        for(zend_hash_internal_pointer_reset_ex(ht_arr, &ptr);
            zend_hash_get_current_data_ex(ht_arr, (void**)&z_ele, &ptr)==SUCCESS;
            zend_hash_move_forward_ex(ht_arr, &ptr))
        {
            convert_to_string(*z_ele);
            key = Z_STRVAL_PP(z_ele);
            key_len = Z_STRLEN_PP(z_ele);

            /* If we're still on a key, prefix it check node */
            if(num_keys-- > 0) {
                key_free = redis_key_prefix(c->flags, &key, &key_len);
                slot = cluster_hash_key(key, key_len);

                /* validate that this key maps to the same node */
                if(node && c->master[slot] != node) {
                    php_error_docref(NULL TSRMLS_CC, E_WARNING,
                        "Keys appear to map to different nodes");
                    RETURN_FALSE;
                }

                node = c->master[slot];
            } else {
                key_free = 0;
            }

            /* Append this key/argument */
            redis_cmd_append_sstr(&cmdstr, key, key_len);

            /* Free key if we prefixed */
            if(key_free) efree(key);
        }
    } else {
        /* Pick a slot at random, we're being told there are no keys */
        slot = rand() % REDIS_CLUSTER_MOD;
    }

    if(cluster_send_command(c, slot, cmdstr.c, cmdstr.len TSRMLS_CC)<0) {
        efree(cmdstr.c);
        RETURN_FALSE;
    }

    if(CLUSTER_IS_ATOMIC(c)) {
        cluster_variant_resp(INTERNAL_FUNCTION_PARAM_PASSTHRU, c, NULL);
    } else {
        void *ctx = NULL;
        CLUSTER_ENQUEUE_RESPONSE(c, slot, cluster_variant_resp, ctx);
        RETURN_ZVAL(getThis(), 1, 0);
    }

    efree(cmdstr.c);
}

/* {{{ proto mixed RedisCluster::eval(string script, [array args, int numkeys) */
PHP_METHOD(RedisCluster, eval) {
    cluster_eval_cmd(INTERNAL_FUNCTION_PARAM_PASSTHRU, GET_CONTEXT(), 
        "EVAL", 4);
}
/* }}} */

/* {{{ proto mixed RedisCluster::evalsha(string sha, [array args, int numkeys]) */
PHP_METHOD(RedisCluster, evalsha) {
    cluster_eval_cmd(INTERNAL_FUNCTION_PARAM_PASSTHRU, GET_CONTEXT(), 
        "EVALSHA", 7);
}
/* }}} */

/* Commands that do not interact with Redis, but just report stuff about
 * various options, etc */

/* {{{ proto long RedisCluster::getOption(long option */
PHP_METHOD(RedisCluster, getoption) {
    redis_getoption_handler(INTERNAL_FUNCTION_PARAM_PASSTHRU, 
        GET_CONTEXT()->flags, GET_CONTEXT());
}
/* }}} */

/* {{{ proto bool RedisCluster::setOption(long option, mixed value) */
PHP_METHOD(RedisCluster, setoption) {
    redis_setoption_handler(INTERNAL_FUNCTION_PARAM_PASSTHRU, 
        GET_CONTEXT()->flags, GET_CONTEXT());
}
/* }}} */

/* {{{ proto string RedisCluster::_prefix(string key) */
PHP_METHOD(RedisCluster, _prefix) {
    redis_prefix_handler(INTERNAL_FUNCTION_PARAM_PASSTHRU, 
        GET_CONTEXT()->flags);
}
/* }}} */

/* {{{ proto string RedisCluster::_serialize(mixed val) */
PHP_METHOD(RedisCluster, _serialize) {
    redis_serialize_handler(INTERNAL_FUNCTION_PARAM_PASSTHRU,
        GET_CONTEXT()->flags);
}
/* }}} */

/* {{{ proto mixed RedisCluster::_unserialize(string val) */
PHP_METHOD(RedisCluster, _unserialize) {
    redis_unserialize_handler(INTERNAL_FUNCTION_PARAM_PASSTHRU,
        GET_CONTEXT()->flags, redis_cluster_exception_ce);
}
/* }}} */

/* {{{ proto array RedisCluster::_masters() */
PHP_METHOD(RedisCluster, _masters) {
    redisCluster *c = GET_CONTEXT();
    zval *z_ret;
    redisClusterNode **node;
    char buf[1024];
    size_t len;

    MAKE_STD_ZVAL(z_ret);
    array_init(z_ret);

    for(zend_hash_internal_pointer_reset(c->nodes);
        zend_hash_get_current_data(c->nodes, (void**)&node)==SUCCESS;
        zend_hash_move_forward(c->nodes))
    {
        len = snprintf(buf, sizeof(buf), "%s:%d", (*node)->sock->host,
            (*node)->sock->port);
        add_next_index_stringl(z_ret, buf, (int)len, 1);
    }

    *return_value = *z_ret;
    efree(z_ret);
}

/*
 * Transaction handling
 */

/* {{{ proto bool RedisCluster::multi() */
PHP_METHOD(RedisCluster, multi) {
    redisCluster *c = GET_CONTEXT();

    if(c->flags->mode == MULTI) {
        php_error_docref(NULL TSRMLS_CC, E_WARNING,
            "RedisCluster is already in MULTI mode, ignoring");
        RETURN_FALSE;
    }

    // Go into MULTI mode
    c->flags->mode = MULTI;

    // Success
    RETURN_TRUE;
}

/* {{{ proto bool RedisCluster::watch() */
PHP_METHOD(RedisCluster, watch) {
    redisCluster *c = GET_CONTEXT();
    HashTable *ht_dist;
    clusterDistList **dl;
    smart_str cmd = {0};
    zval **z_args;
    int argc = ZEND_NUM_ARGS(), i;
    ulong slot;

    // Disallow in MULTI mode
    if(c->flags->mode == MULTI) {
        php_error_docref(NULL TSRMLS_CC, E_WARNING,
            "WATCH command not allowed in MULTI mode");
        RETURN_FALSE;
    }

    // Don't need to process zero arguments
    if(!argc) RETURN_FALSE;

    // Create our distribution HashTable
    ht_dist = cluster_dist_create();

    // Allocate args, and grab them
    z_args = emalloc(sizeof(zval*)*argc);
    if(zend_get_parameters_array(ht, argc, z_args)==FAILURE) {
        efree(z_args);
        cluster_dist_free(ht_dist);
        RETURN_FALSE;
    }

    // Loop through arguments, prefixing if needed
    for(i=0;i<argc;i++) {
        // We'll need the key as a string
        convert_to_string(z_args[i]);

        // Add this key to our distribution handler
        if(cluster_dist_add_key(c, ht_dist, Z_STRVAL_P(z_args[i]), 
                                Z_STRLEN_P(z_args[i]), NULL) == FAILURE)
        {
            zend_throw_exception(redis_cluster_exception_ce,
                "Can't issue WATCH command as the keyspace isn't fully mapped",
                0 TSRMLS_CC);
            RETURN_FALSE;
        }
    }

    // Iterate over each node we'll be sending commands to
    for(zend_hash_internal_pointer_reset(ht_dist);
        zend_hash_get_current_key(ht_dist,NULL,&slot, 0)==HASH_KEY_IS_LONG;
        zend_hash_move_forward(ht_dist))
    {
        // Grab the clusterDistList pointer itself
        if(zend_hash_get_current_data(ht_dist, (void**)&dl)==FAILURE) {
            zend_throw_exception(redis_cluster_exception_ce,
                "Internal error in a PHP HashTable", 0 TSRMLS_CC);
            cluster_dist_free(ht_dist);
            efree(z_args);
            efree(cmd.c);
            RETURN_FALSE;
        }

        // Construct our watch command for this node
        redis_cmd_init_sstr(&cmd, (*dl)->len, "WATCH", sizeof("WATCH")-1);
        for(i=0;i<(*dl)->len;i++) {
            redis_cmd_append_sstr(&cmd, (*dl)->entry[i].key, 
                (*dl)->entry[i].key_len);
        }

        // If we get a failure from this, we have to abort
        if((slot = cluster_send_command(c, (short)slot, cmd.c, cmd.len 
                                        TSRMLS_CC))==-1)
        {
            RETURN_FALSE;
        }

        // This node is watching
        SLOT_SOCK(c, (short)slot)->watching = 1;

        // Zero out our command buffer
        cmd.len = 0;
    }

    // Cleanup
    cluster_dist_free(ht_dist);
    efree(z_args);
    efree(cmd.c);

    RETURN_TRUE;
}

/* {{{ proto bool RedisCluster::unwatch() */
PHP_METHOD(RedisCluster, unwatch) {
    redisCluster *c = GET_CONTEXT();
    short slot;

    // Send UNWATCH to nodes that need it
    for(slot=0;slot<REDIS_CLUSTER_SLOTS;slot++) {
        if(c->master[slot] && SLOT_SOCK(c,slot)->watching) {
            if(cluster_send_slot(c, slot, RESP_UNWATCH_CMD, 
                                 sizeof(RESP_UNWATCH_CMD)-1,
                                 TYPE_LINE TSRMLS_CC)==-1)
            {
                CLUSTER_RETURN_BOOL(c, 0);
            }

            // No longer watching
            SLOT_SOCK(c,slot)->watching = 0;
        }
    }

    CLUSTER_RETURN_BOOL(c, 1);
}

/* {{{ proto array RedisCluster::exec() */
PHP_METHOD(RedisCluster, exec) {
    redisCluster *c = GET_CONTEXT();
    clusterFoldItem *fi;

    // Verify we are in fact in multi mode
    if(CLUSTER_IS_ATOMIC(c)) {
        php_error_docref(NULL TSRMLS_CC, E_WARNING, 
            "RedisCluster is not in MULTI mode");
        RETURN_FALSE;
    }

    // First pass, send EXEC and abort on failure
    fi = c->multi_head;
    while(fi) {
        if(SLOT_SOCK(c, fi->slot)->mode == MULTI) {
            if(cluster_send_exec(c, fi->slot TSRMLS_CC)<0) {
                cluster_abort_exec(c TSRMLS_CC);
            
                zend_throw_exception(redis_cluster_exception_ce,
                    "Error processing EXEC across the cluster",
                    0 TSRMLS_CC);

                // Free our queue, reset MULTI state
                CLUSTER_FREE_QUEUE(c);
                CLUSTER_RESET_MULTI(c);
            
                RETURN_FALSE;
            }
            SLOT_SOCK(c, fi->slot)->mode     = ATOMIC;
            SLOT_SOCK(c, fi->slot)->watching = 0;
        }
        fi = fi->next;
    }

    // MULTI multi-bulk response handler
    cluster_multi_mbulk_resp(INTERNAL_FUNCTION_PARAM_PASSTHRU, c, NULL);

    // Free our callback queue, any enqueued distributed command context items
    // and reset our MULTI state.
    CLUSTER_FREE_QUEUE(c);
    CLUSTER_RESET_MULTI(c);
}

/* {{{ proto bool RedisCluster::discard() */
PHP_METHOD(RedisCluster, discard) {
    redisCluster *c = GET_CONTEXT();

    if(CLUSTER_IS_ATOMIC(c)) {
        php_error_docref(NULL TSRMLS_CC, E_WARNING,
            "Cluster is not in MULTI mode");
        RETURN_FALSE;
    }
    
    if(cluster_abort_exec(c TSRMLS_CC)<0) {
        CLUSTER_RESET_MULTI(c);
    }

    CLUSTER_FREE_QUEUE(c);

    RETURN_TRUE;
}

/* Generic handler for things we want directed at a given node, like SAVE,
 * BGSAVE, FLUSHDB, FLUSHALL, etc */
static void 
cluster_empty_node_cmd(INTERNAL_FUNCTION_PARAMETERS, char *kw, 
                       REDIS_REPLY_TYPE reply_type, cluster_cb cb)
{
    redisCluster *c = GET_CONTEXT();
    char *cmd, *arg1; 
    int arg1_len, cmd_len, arg1_free; 
    short slot;
    long arg2;

    if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s|l", &arg1, &arg1_len,
                             &arg2)==FAILURE)
    {
        RETURN_FALSE;
    }

    // One argument means find the node (treated like a key), and two means
    // send the command to a specific host and port
    if(ZEND_NUM_ARGS() == 1) {
        // Treat our argument like a key, and look for the slot that way
        arg1_free = redis_key_prefix(c->flags, &arg1, &arg1_len);
        slot = cluster_hash_key(arg1, arg1_len);
        if(arg1_free) efree(arg1);        
    } else {
        // Find the slot by IP/port
        slot = cluster_find_slot(c, (const char *)arg1, (unsigned short)arg2);
        if(slot<0) {
            php_error_docref(0 TSRMLS_CC, E_WARNING, "Unknown node %s:%ld", 
                arg1, arg2);
            RETURN_FALSE;
        }
    }

    // Construct our command
    cmd_len = redis_cmd_format_static(&cmd, kw, "");

    // Kick off our command
    if(cluster_send_slot(c, slot, cmd, cmd_len, reply_type TSRMLS_CC)<0) {
        zend_throw_exception(redis_cluster_exception_ce,
            "Unable to send command at a specific node", 0 TSRMLS_CC);
        efree(cmd);
        RETURN_FALSE;
    }

    // Our response callback
    cb(INTERNAL_FUNCTION_PARAM_PASSTHRU, c, NULL);

    // Free our command
    efree(cmd);
}

/* Generic method for HSCAN, SSCAN, and ZSCAN */
static void cluster_kscan_cmd(INTERNAL_FUNCTION_PARAMETERS, 
                              REDIS_SCAN_TYPE type)
{
    redisCluster *c = GET_CONTEXT();
    char *cmd, *pat=NULL, *key=NULL; 
    int cmd_len, key_len=0, pat_len=0, key_free=0;
    short slot;
    zval *z_it;
    HashTable *hash;
    long it, num_ele, count=0;

    // Can't be in MULTI mode
    if(!CLUSTER_IS_ATOMIC(c)) {
        zend_throw_exception(redis_cluster_exception_ce,
            "SCAN type commands can't be called in MULTI mode!", 0 TSRMLS_CC);
        RETURN_FALSE;
    }

    /* Parse arguments */
    if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "sz/|s!l", &key, 
                             &key_len, &z_it, &pat, &pat_len, &count)==FAILURE)
    {
        RETURN_FALSE;
    }

    // Convert iterator to long if it isn't, update our long iterator if it's
    // set and >0, and finish if it's back to zero
    if(Z_TYPE_P(z_it) != IS_LONG || Z_LVAL_P(z_it)<0) {
        convert_to_long(z_it);
        it = 0;
    } else if(Z_LVAL_P(z_it)!=0) {
        it = Z_LVAL_P(z_it);
    } else {
        RETURN_FALSE;
    }

    // Apply any key prefix we have, get the slot
    key_free = redis_key_prefix(c->flags, &key, &key_len);
    slot = cluster_hash_key(key, key_len);

    // If SCAN_RETRY is set, loop until we get a zero iterator or until
    // we get non-zero elements.  Otherwise we just send the command once.
    do {
        // Create command
        cmd_len = redis_fmt_scan_cmd(&cmd, type, key, key_len, it, pat, pat_len,
            count); 

        // Send it off
        if(cluster_send_command(c, slot, cmd, cmd_len TSRMLS_CC)==FAILURE)
        {
            zend_throw_exception(redis_cluster_exception_ce,
                "Couldn't send SCAN command", 0 TSRMLS_CC);
            if(key_free) efree(key);
            efree(cmd);
            RETURN_FALSE;
        }

        // Read response
        if(cluster_scan_resp(INTERNAL_FUNCTION_PARAM_PASSTHRU, c, type, 
                              &it)==FAILURE)
        {
            zend_throw_exception(redis_cluster_exception_ce,
                "Couldn't read SCAN response", 0 TSRMLS_CC);
            if(key_free) efree(key);
            efree(cmd);
            RETURN_FALSE;
        }

        // Count the elements we got back
        hash = Z_ARRVAL_P(return_value);
        num_ele = zend_hash_num_elements(hash);

        // Free our command
        efree(cmd);
    } while(c->flags->scan == REDIS_SCAN_RETRY && it != 0 && num_ele == 0);

    // Free our key
    if(key_free) efree(key);

    // Update iterator reference
    Z_LVAL_P(z_it) = it;
}

/* {{{ proto RedisCluster::scan(string master, long it [, string pat, long cnt]) */
PHP_METHOD(RedisCluster, scan) {
    redisCluster *c = GET_CONTEXT();
    redisClusterNode **n;
    char *cmd, *node, *pat=NULL, *key=NULL;
    int pat_len=0, node_len, cmd_len;
    short slot;
    zval *z_it;
    HashTable *hash;
    long it, num_ele, count=0;

    /* Can't be in MULTI mode */
    if(!CLUSTER_IS_ATOMIC(c)) {
        zend_throw_exception(redis_cluster_exception_ce,
            "SCAN type commands can't be called in MULTI mode", 0 TSRMLS_CC);
        RETURN_FALSE;
    }

    /* Parse arguments */
    if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "z/s|s!l", &z_it, &node, 
                             &node_len, &pat, &pat_len, &count)==FAILURE)
    {
        RETURN_FALSE;
    }

    /* Convert or update iterator */
    if(Z_TYPE_P(z_it) != IS_LONG || Z_LVAL_P(z_it)<0) {
        convert_to_long(z_it);
        it = 0;
    } else if(Z_LVAL_P(z_it)!=0) {
        it = Z_LVAL_P(z_it);
    } else {
        RETURN_FALSE;
    }

    /* With SCAN_RETRY on, loop until we get some keys, otherwise just return
     * what Redis does, as it does */
    do {
        /* Construct our command */
        cmd_len = redis_fmt_scan_cmd(&cmd, TYPE_SCAN, NULL, 0, it, pat, pat_len,
            count);
        
        /* Find this slot by node */
        if(zend_hash_find(c->nodes, node, node_len+1, (void**)&n)==FAILURE) {
            zend_throw_exception(redis_cluster_exception_ce, 
                "Unknown host:port passed to SCAN command", 0 TSRMLS_CC);
            efree(cmd);
            RETURN_FALSE;
        }

        // Send it to the node in question
        slot = (*n)->slot;
        if(cluster_send_command(c, slot, cmd, cmd_len TSRMLS_CC)<0) 
        {
            zend_throw_exception(redis_cluster_exception_ce,
                "Couldn't send SCAN to node", 0 TSRMLS_CC);
            efree(cmd);
            RETURN_FALSE;
        }

        if(cluster_scan_resp(INTERNAL_FUNCTION_PARAM_PASSTHRU, c, TYPE_SCAN,
                           &it)==FAILURE || Z_TYPE_P(return_value)!=IS_ARRAY)
        {
            zend_throw_exception(redis_cluster_exception_ce,
                "Couldn't process SCAN response from node", 0 TSRMLS_CC);
            efree(cmd);
            RETURN_FALSE;
        }

        efree(cmd);

        num_ele = zend_hash_num_elements(Z_ARRVAL_P(return_value));
    } while(c->flags->scan == REDIS_SCAN_RETRY && it != 0 && num_ele == 0);

    Z_LVAL_P(z_it) = it;
}

/* {{{ proto RedisCluster::sscan(string key, long it [string pat, long cnt]) */
PHP_METHOD(RedisCluster, sscan) {
    cluster_kscan_cmd(INTERNAL_FUNCTION_PARAM_PASSTHRU, TYPE_SSCAN);
}

/* {{{ proto RedisCluster::zscan(string key, long it [string pat, long cnt]) */
PHP_METHOD(RedisCluster, zscan) {
    cluster_kscan_cmd(INTERNAL_FUNCTION_PARAM_PASSTHRU, TYPE_ZSCAN);
}

/* {{{ proto RedisCluster::hscan(string key, long it [string pat, long cnt]) */
PHP_METHOD(RedisCluster, hscan) {
    cluster_kscan_cmd(INTERNAL_FUNCTION_PARAM_PASSTHRU, TYPE_HSCAN);
}


/* {{{ proto RedisCluster::save(string key)
 *     proto RedisCluster::save(string host, long port) */
PHP_METHOD(RedisCluster, save) {
    cluster_empty_node_cmd(INTERNAL_FUNCTION_PARAM_PASSTHRU, "SAVE", TYPE_LINE,
        cluster_bool_resp);
}

/* {{{ proto RedisCluster::bgsave(string key) 
 *     proto RedisCluster::bgsave(string host, long port) */
PHP_METHOD(RedisCluster, bgsave) {
    cluster_empty_node_cmd(INTERNAL_FUNCTION_PARAM_PASSTHRU, "BGSAVE", 
        TYPE_LINE, cluster_bool_resp);
}

/* {{{ proto RedisCluster::flushdb(string key)
 *     proto RedisCluster::flushdb(string host, long port) */
PHP_METHOD(RedisCluster, flushdb) {
    cluster_empty_node_cmd(INTERNAL_FUNCTION_PARAM_PASSTHRU, "FLUSHDB",
        TYPE_LINE, cluster_bool_resp);
}

/* {{{ proto RedisCluster::flushall(string key)
 *     proto RedisCluster::flushall(string host, long port) */
PHP_METHOD(RedisCluster, flushall) {
    cluster_empty_node_cmd(INTERNAL_FUNCTION_PARAM_PASSTHRU, "FLUSHALL",
        TYPE_LINE, cluster_bool_resp);
}

/* {{{ proto RedisCluster::dbsize(string key)
 *     proto RedisCluster::dbsize(string host, long port) */
PHP_METHOD(RedisCluster, dbsize) {
    cluster_empty_node_cmd(INTERNAL_FUNCTION_PARAM_PASSTHRU, "DBSIZE",
        TYPE_LINE, cluster_bool_resp);
}

/* {{{ proto RedisCluster::bgrewriteaof(string key)
 *     proto RedisCluster::bgrewriteaof(string host, long port) */
PHP_METHOD(RedisCluster, bgrewriteaof) {
    cluster_empty_node_cmd(INTERNAL_FUNCTION_PARAM_PASSTHRU, "BGREWRITEAOF",
        TYPE_LINE, cluster_bool_resp);
}

/* {{{ proto RedisCluster::lastsave(string key)
 *     proto RedisCluster::lastsave(string host, long port) */
PHP_METHOD(RedisCluster, lastsave) {
    cluster_empty_node_cmd(INTERNAL_FUNCTION_PARAM_PASSTHRU, "LASTSAVE",
        TYPE_INT, cluster_long_resp);
}


/* vim: set tabstop=4 softtabstops=4 noexpandtab shiftwidth=4: */
