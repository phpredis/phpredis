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

/* Function table */
zend_function_entry redis_cluster_functions[] = {
    PHP_ME(RedisCluster, __construct, NULL, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, close, NULL, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, get, NULL, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, set, NULL, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, mget, NULL, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, mset, NULL, ZEND_ACC_PUBLIC)
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
    
    PHP_ME(RedisCluster, getoption, NULL, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, setoption, NULL, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, _prefix, NULL, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, _serialize, NULL, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, _unserialize, NULL, ZEND_ACC_PUBLIC)

    PHP_ME(RedisCluster, multi, NULL, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, exec, NULL, ZEND_ACC_PUBLIC)

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

/* {{{ proto array RedisCluster::mget(string key1, ... string keyN) */
PHP_METHOD(RedisCluster, mget) {
    php_error_docref(NULL TSRMLS_CC, E_ERROR, "Coming soon!");
}

/* {{{ proto bool RedisCluster::mset(string key1, string val1, ... kN, vN) */
PHP_METHOD(RedisCluster, mset) {
    php_error_docref(NULL TSRMLS_CC, E_ERROR, "Coming soon!");
}

/* {{{ proto bool RedisCluster::setex(string key, string value, int expiry) */
PHP_METHOD(RedisCluster, setex) {
    CLUSTER_PROCESS_KW_CMD("SETEX", redis_key_long_val_cmd,
        cluster_bool_resp);
}
/* }}} */

/* {{{ proto bool RedisCluster::psetex(string key, string value, int expiry) */
PHP_METHOD(RedisCluster, psetex) {
    CLUSTER_PROCESS_KW_CMD("PSETEX", redis_key_long_val_cmd,
        cluster_bool_resp);
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

/* {{{ proto Redis::object(string subcmd, string key) */
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
            SLOT_SOCK(c, fi->slot)->mode = ATOMIC;
        }
        fi = fi->next;
    }

    // MULTI multi-bulk response handler
    cluster_multi_mbulk_resp(INTERNAL_FUNCTION_PARAM_PASSTHRU, c, NULL);

    // Free our queue, and reset MULTI state
    CLUSTER_FREE_QUEUE(c);
    CLUSTER_RESET_MULTI(c);
}

/* vim: set tabstop=4 softtabstops=4 noexpandtab shiftwidth=4: */
