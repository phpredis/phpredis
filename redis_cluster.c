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
    PHP_ME(RedisCluster, get, NULL, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, set, NULL, ZEND_ACC_PUBLIC)
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
    PHP_ME(RedisCluster, rpushx, NULL, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, lpushx, NULL, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, llen, NULL, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, scard, NULL, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, smembers, NULL, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, sismember, NULL, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, strlen, NULL, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, persist, NULL, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, ttl, NULL, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, pttl, NULL, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, zcard, NULL, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, zscore, NULL, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, hlen, NULL, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, hkeys, NULL, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, hvals, NULL, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, hgetall, NULL, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, hexists, NULL, ZEND_ACC_PUBLIC)
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
    PHP_ME(RedisCluster, lget, NULL, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, getrange, NULL, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, ltrim, NULL, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, lrange, NULL, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, zremrangebyrank, NULL, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, publish, NULL, ZEND_ACC_PUBLIC)
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
 * RedisCluster methods
 */

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

/* {{{ proto string RedisCluster::strlen(string key) */
PHP_METHOD(RedisCluster, strlen) {
    CLUSTER_PROCESS_KW_CMD("STRLEN", redis_key_cmd, cluster_bulk_resp);
}

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
    CLUSTER_PROCESS_KW_CMD("HKEYS", redis_key_cmd, cluster_mbulk_resp_raw);
}
/* }}} */

/* {{{ proto array RedisCluster::hvals(string key) */
PHP_METHOD(RedisCluster, hvals) { 
    CLUSTER_PROCESS_KW_CMD("HVALS", redis_key_cmd, cluster_mbulk_resp);
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


/* vim: set tabstop=4 softtabstops=4 noexpandtab shiftwidth=4: */
