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
#include <php_variables.h>
#include <SAPI.h>

zend_class_entry *redis_cluster_ce;

/* Exception handler */
zend_class_entry *redis_cluster_exception_ce;

static zend_class_entry *spl_rte_ce = NULL;
/* Handlers for RedisCluster */
zend_object_handlers RedisCluster_handlers;

/* Argument info for HSCAN, SSCAN, HSCAN */
ZEND_BEGIN_ARG_INFO_EX(arginfo_kscan_cl, 0, 0, 2)
    ZEND_ARG_INFO(0, str_key)
    ZEND_ARG_INFO(1, i_iterator)
    ZEND_ARG_INFO(0, str_pattern)
    ZEND_ARG_INFO(0, i_count)
ZEND_END_ARG_INFO();

/* Argument infor for SCAN */
ZEND_BEGIN_ARG_INFO_EX(arginfo_scan_cl, 0, 0, 2)
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
    PHP_ME(RedisCluster, lindex, NULL, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, lrem, NULL, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, brpoplpush, NULL, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, rpoplpush, NULL, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, llen, NULL, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, scard, NULL, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, smembers, NULL, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, sismember, NULL, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, sadd, NULL, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, saddarray, NULL, ZEND_ACC_PUBLIC)
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
    PHP_ME(RedisCluster, hstrlen, NULL, ZEND_ACC_PUBLIC)
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
    PHP_ME(RedisCluster, zrevrangebyscore, NULL, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, zrangebylex, NULL, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, zrevrangebylex, NULL, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, zlexcount, NULL, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, zremrangebylex, NULL, ZEND_ACC_PUBLIC)
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
    PHP_ME(RedisCluster, scan, arginfo_scan_cl, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, sscan, arginfo_kscan_cl, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, zscan, arginfo_kscan_cl, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, hscan, arginfo_kscan_cl, ZEND_ACC_PUBLIC)
   
    PHP_ME(RedisCluster, getmode, NULL, ZEND_ACC_PUBLIC) 
    PHP_ME(RedisCluster, getlasterror, NULL, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, clearlasterror, NULL, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, getoption, NULL, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, setoption, NULL, ZEND_ACC_PUBLIC)
    
    PHP_ME(RedisCluster, _prefix, NULL, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, _serialize, NULL, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, _unserialize, NULL, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, _masters, NULL, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, _redir, NULL, ZEND_ACC_PUBLIC)

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
    PHP_ME(RedisCluster, info, NULL, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, role, NULL, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, time, NULL, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, randomkey, NULL, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, ping, NULL, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, echo, NULL, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, command, NULL, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, rawcommand, NULL, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, cluster, NULL, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, client, NULL, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, config, NULL, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, pubsub, NULL, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, script, NULL, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, slowlog, NULL, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, geoadd, NULL, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, geohash, NULL, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, geopos, NULL, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, geodist, NULL, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, georadius, NULL, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, georadiusbymember, NULL, ZEND_ACC_PUBLIC)
    PHP_FE_END
};

/* Our context seeds will be a hash table with RedisSock* pointers */
#if (PHP_MAJOR_VERSION < 7)
static void ht_free_seed(void *data)
#else
static void ht_free_seed(zval *data)
#endif
{
    RedisSock *redis_sock = *(RedisSock**)data;
    if(redis_sock) redis_free_socket(redis_sock);
}

/* Free redisClusterNode objects we've stored */
#if (PHP_MAJOR_VERSION < 7)
static void ht_free_node(void *data)
#else
static void ht_free_node(zval *data)
#endif
{
    redisClusterNode *node = *(redisClusterNode**)data;
    cluster_free_node(node);
}

/* Initialize/Register our RedisCluster exceptions */
PHPAPI zend_class_entry *rediscluster_get_exception_base(int root TSRMLS_DC) {
#if HAVE_SPL
    if(!root) {
        if(!spl_rte_ce) {
            zend_class_entry *pce;

            if ((pce = zend_hash_str_find_ptr(CG(class_table), "runtimeexception", sizeof("runtimeexception") - 1))) {
                spl_rte_ce = pce;
                return pce;
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
#if (PHP_MAJOR_VERSION < 7)
zend_object_value
create_cluster_context(zend_class_entry *class_type TSRMLS_DC) {
    redisCluster *cluster;

    // Allocate our actual struct
    cluster = ecalloc(1, sizeof(redisCluster));
#else
zend_object *
create_cluster_context(zend_class_entry *class_type TSRMLS_DC) {
    redisCluster *cluster;

    // Allocate our actual struct
    cluster = ecalloc(1, sizeof(redisCluster) + sizeof(zval) * (class_type->default_properties_count - 1));
#endif
    
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
#if (PHP_MAJOR_VERSION < 7)
    zend_object_value retval;
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
#else
	object_properties_init(&cluster->std, class_type);
	memcpy(&RedisCluster_handlers, zend_get_std_object_handlers(), sizeof(RedisCluster_handlers));
	RedisCluster_handlers.offset = XtOffsetOf(redisCluster, std);
    RedisCluster_handlers.free_obj = free_cluster_context;

	cluster->std.handlers = &RedisCluster_handlers;

	return &cluster->std;
#endif
}

/* Free redisCluster context */
void
#if (PHP_MAJOR_VERSION < 7)
free_cluster_context(void *object TSRMLS_DC) {
    redisCluster *cluster = (redisCluster*)object;
#else
free_cluster_context(zend_object *object) {
    redisCluster *cluster = (redisCluster*)((char*)(object) - XtOffsetOf(redisCluster, std));
#endif
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

    zend_object_std_dtor(&cluster->std TSRMLS_CC);

#if (PHP_MAJOR_VERSION < 7)
    efree(cluster);
#endif
}

/* Attempt to connect to a Redis cluster provided seeds and timeout options */
void redis_cluster_init(redisCluster *c, HashTable *ht_seeds, double timeout,
                        double read_timeout, int persistent TSRMLS_DC)
{
    // Validate timeout
    if(timeout < 0L || timeout > INT_MAX) {
        zend_throw_exception(redis_cluster_exception_ce, 
            "Invalid timeout", 0 TSRMLS_CC);
    }

    // Validate our read timeout
    if(read_timeout < 0L || read_timeout > INT_MAX) {
        zend_throw_exception(redis_cluster_exception_ce,
            "Invalid read timeout", 0 TSRMLS_CC);
    }

    /* Make sure there are some seeds */
    if(zend_hash_num_elements(ht_seeds)==0) {
        zend_throw_exception(redis_cluster_exception_ce,
            "Must pass seeds", 0 TSRMLS_CC);
    }
    /* Set our timeout and read_timeout which we'll pass through to the
     * socket type operations */
    c->timeout = timeout;
    c->read_timeout = read_timeout;
    
    /* Set our option to use or not use persistent connections */
    c->persistent = persistent;

    /* Calculate the number of miliseconds we will wait when bouncing around,
     * (e.g. a node goes down), which is not the same as a standard timeout. */
    c->waitms = (long)(timeout * 1000);
    
    // Initialize our RedisSock "seed" objects
    cluster_init_seeds(c, ht_seeds);

    // Create and map our key space
    cluster_map_keyspace(c TSRMLS_CC);
}

/* Attempt to load a named cluster configured in php.ini */
void redis_cluster_load(redisCluster *c, char *name, int name_len TSRMLS_DC) {
    zval z_seeds, z_timeout, z_read_timeout, z_persistent, *z_value;
    char *iptr;
    double timeout=0, read_timeout=0;
    int persistent = 0;
    HashTable *ht_seeds = NULL;

    /* Seeds */
    array_init(&z_seeds);
    if ((iptr = INI_STR("redis.clusters.seeds")) != NULL) {
        sapi_module.treat_data(PARSE_STRING, estrdup(iptr), &z_seeds TSRMLS_CC);
    }
    if ((z_value = zend_hash_str_find(Z_ARRVAL(z_seeds), name, name_len)) != NULL) {
        ht_seeds = Z_ARRVAL_P(z_value);
    } else {
        zval_dtor(&z_seeds);
        zend_throw_exception(redis_cluster_exception_ce, "Couldn't find seeds for cluster", 0 TSRMLS_CC);
        return;
    }
    
    /* Connection timeout */
    array_init(&z_timeout);
    if ((iptr = INI_STR("redis.clusters.timeout")) != NULL) {
        sapi_module.treat_data(PARSE_STRING, estrdup(iptr), &z_timeout TSRMLS_CC);
    }
    if ((z_value = zend_hash_str_find(Z_ARRVAL(z_timeout), name, name_len)) != NULL) {
        if (Z_TYPE_P(z_value) == IS_STRING) {
            timeout = atof(Z_STRVAL_P(z_value));
        } else if (Z_TYPE_P(z_value) == IS_DOUBLE) {
            timeout = Z_DVAL_P(z_value);
        } else if (Z_TYPE_P(z_value) == IS_LONG) {
            timeout = Z_LVAL_P(z_value);
        }
    }

    /* Read timeout */
    array_init(&z_read_timeout);
    if ((iptr = INI_STR("redis.clusters.read_timeout")) != NULL) {
        sapi_module.treat_data(PARSE_STRING, estrdup(iptr), &z_read_timeout TSRMLS_CC);
    }
    if ((z_value = zend_hash_str_find(Z_ARRVAL(z_read_timeout), name, name_len)) != NULL) {
        if (Z_TYPE_P(z_value) == IS_STRING) {
            read_timeout = atof(Z_STRVAL_P(z_value));
        } else if (Z_TYPE_P(z_value) == IS_DOUBLE) {
            read_timeout = Z_DVAL_P(z_value);
        } else if (Z_TYPE_P(z_value) == IS_LONG) {
            timeout = Z_LVAL_P(z_value);
        }
    }

    /* Persistent connections */
    array_init(&z_persistent);
    if ((iptr = INI_STR("redis.clusters.persistent")) != NULL) {
        sapi_module.treat_data(PARSE_STRING, estrdup(iptr), &z_persistent TSRMLS_CC);
    }
    if ((z_value = zend_hash_str_find(Z_ARRVAL(z_persistent), name, name_len)) != NULL) {
        if (Z_TYPE_P(z_value) == IS_STRING) {
            persistent = atoi(Z_STRVAL_P(z_value));
        } else if (Z_TYPE_P(z_value) == IS_LONG) {
            persistent = Z_LVAL_P(z_value);
        }
    }

    /* Attempt to create/connect to the cluster */
    redis_cluster_init(c, ht_seeds, timeout, read_timeout, persistent TSRMLS_CC);    

    /* Clean up our arrays */
    zval_dtor(&z_seeds);
    zval_dtor(&z_timeout);
    zval_dtor(&z_read_timeout);
    zval_dtor(&z_persistent);
}

/*
 * PHP Methods
 */

/* Create a RedisCluster Object */
PHP_METHOD(RedisCluster, __construct) {
    zval *object, *z_seeds=NULL;
    char *name;
    strlen_t name_len;
    double timeout = 0.0, read_timeout = 0.0;
    zend_bool persistent = 0;
    redisCluster *context = GET_CONTEXT();

    // Parse arguments
    if(zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(),
                                    "Os|addb", &object, redis_cluster_ce, &name,
                                    &name_len, &z_seeds, &timeout,
                                    &read_timeout, &persistent)==FAILURE)
    {
        RETURN_FALSE;
    }

    // Require a name
    if(name_len == 0 && ZEND_NUM_ARGS() < 2) {
        zend_throw_exception(redis_cluster_exception_ce,
            "You must specify a name or pass seeds!",
            0 TSRMLS_CC);
    }

    /* If we've been passed only one argument, the user is attempting to connect
     * to a named cluster, stored in php.ini, otherwise we'll need manual seeds */
    if (ZEND_NUM_ARGS() > 1) {
        redis_cluster_init(context, Z_ARRVAL_P(z_seeds), timeout, read_timeout,
            persistent TSRMLS_CC);
    } else {
        redis_cluster_load(context, name, name_len TSRMLS_CC);
    }
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
    CLUSTER_PROCESS_KW_CMD("GET", redis_key_cmd, cluster_bulk_resp, 1);
}
/* }}} */

/* {{{ proto bool RedisCluster::set(string key, string value) */
PHP_METHOD(RedisCluster, set) {
    CLUSTER_PROCESS_CMD(set, cluster_bool_resp, 0);
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
    strlen_t key_len;
    int key_free;
    short slot;

    char *val;
    strlen_t val_len;
    int val_free;
} clusterKeyValHT;

/* Helper to pull a key/value pair from a HashTable */
static int get_key_val_ht(redisCluster *c, HashTable *ht, HashPosition *ptr, 
                          clusterKeyValHT *kv TSRMLS_DC)
{
    zval *z_val;
	zend_ulong idx;

    // Grab the key, convert it to a string using provided kbuf buffer if it's
    // a LONG style key
#if (PHP_MAJOR_VERSION < 7)
	uint key_len;
	switch(zend_hash_get_current_key_ex(ht, &(kv->key), &key_len, &idx, 0, ptr)) {
		case HASH_KEY_IS_STRING:
			kv->key_len = (int)(key_len-1);
#else
	zend_string *zkey;
	switch (zend_hash_get_current_key_ex(ht, &zkey, &idx, ptr)) {
		case HASH_KEY_IS_STRING:
			kv->key_len = zkey->len;
			kv->key = zkey->val;
#endif
            break;
        case HASH_KEY_IS_LONG:
            kv->key_len = snprintf(kv->kbuf,sizeof(kv->kbuf),"%ld",(long)idx);
            kv->key     = kv->kbuf;
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
    if ((z_val = zend_hash_get_current_data_ex(ht, ptr)) == NULL) {
        zend_throw_exception(redis_cluster_exception_ce,
            "Internal Zend HashTable error", 0 TSRMLS_CC);
        return -1;
    }

    // Serialize our value if required
    kv->val_free = redis_serialize(c->flags,z_val,&(kv->val),&(kv->val_len)
        TSRMLS_CC);

    // Success
    return 0;
}

/* Helper to pull, prefix, and hash a key from a HashTable value */
static int get_key_ht(redisCluster *c, HashTable *ht, HashPosition *ptr,
                      clusterKeyValHT *kv TSRMLS_DC)
{
    zval *z_key;

    if ((z_key = zend_hash_get_current_data_ex(ht, ptr)) == NULL) {
        // Shouldn't happen, but check anyway
        zend_throw_exception(redis_cluster_exception_ce,
            "Internal Zend HashTable error", 0 TSRMLS_CC);
        return -1;
    }

    // Always want to work with strings
    convert_to_string(z_key);

    kv->key = Z_STRVAL_P(z_key);
    kv->key_len = Z_STRLEN_P(z_key);
    kv->key_free = redis_key_prefix(c->flags, &(kv->key), &(kv->key_len));

    // Hash our key
    kv->slot = cluster_hash_key(kv->key, kv->key_len);

    // Success
    return 0;
}

/* Turn variable arguments into a HashTable for processing */
static HashTable *method_args_to_ht(zval *z_args, int argc) {
    HashTable *ht_ret;
    int i;

    /* Allocate our hash table */
    ALLOC_HASHTABLE(ht_ret);
    zend_hash_init(ht_ret, argc, NULL, NULL, 0);

    /* Populate our return hash table with our arguments */
    for (i = 0; i < argc; i++) {
        zend_hash_next_index_insert(ht_ret, &z_args[i]);
    }

    /* Return our hash table */
    return ht_ret;
}

/* Handler for both MGET and DEL */
static int cluster_mkey_cmd(INTERNAL_FUNCTION_PARAMETERS, char *kw, int kw_len,
                            zval *z_ret, cluster_cb cb)
{
    redisCluster *c = GET_CONTEXT();
    clusterMultiCmd mc = {0};
    clusterKeyValHT kv;
    zval *z_args;
    HashTable *ht_arr;
    HashPosition ptr;
    int i=1, argc = ZEND_NUM_ARGS(), ht_free=0;
    short slot;

    /* If we don't have any arguments we're invalid */
    if (!argc) return -1;

    /* Extract our arguments into an array */
    z_args = ecalloc(argc, sizeof(zval));
    if (zend_get_parameters_array(ht, argc, z_args) == FAILURE) {
        efree(z_args);
        return -1;
    }

    /* Determine if we're working with a single array or variadic args */
    if (argc == 1 && Z_TYPE(z_args[0]) == IS_ARRAY) {
        ht_arr = Z_ARRVAL(z_args[0]);
        argc = zend_hash_num_elements(ht_arr);
        if (!argc) {
            efree(z_args);
            return -1;
        }
    } else {
        ht_arr = method_args_to_ht(z_args, argc);
        ht_free = 1;
    }

    /* MGET is readonly, DEL is not */
    c->readonly = kw_len == 4 && CLUSTER_IS_ATOMIC(c);

    // Initialize our "multi" command handler with command/len
    CLUSTER_MULTI_INIT(mc, kw, kw_len);

    // Process the first key outside of our loop, so we don't have to check if
    // it's the first iteration every time, needlessly
    zend_hash_internal_pointer_reset_ex(ht_arr, &ptr);
    if(get_key_ht(c, ht_arr, &ptr, &kv TSRMLS_CC)<0) {
		efree(z_args);
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
            if (ht_free) {
                zend_hash_destroy(ht_arr);
                efree(ht_arr);
            }
			efree(z_args);
            return -1;
        }
    
        // If the slots have changed, kick off the keys we've aggregated
        if(slot != kv.slot) {
            // Process this batch of MGET keys
            if(distcmd_resp_handler(INTERNAL_FUNCTION_PARAM_PASSTHRU, c, slot, 
                                    &mc, z_ret, i==argc, cb)<0)
            {
                cluster_multi_free(&mc);
                if (ht_free) {
                    zend_hash_destroy(ht_arr);
                    efree(ht_arr);
                }
				efree(z_args);
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
	efree(z_args);

    // If we've got straggler(s) process them
    if(mc.argc > 0) {
        if(distcmd_resp_handler(INTERNAL_FUNCTION_PARAM_PASSTHRU, c, slot, 
                                &mc, z_ret, 1, cb)<0)
        {
            cluster_multi_free(&mc);
            if (ht_free) {
                zend_hash_destroy(ht_arr);
                efree(ht_arr);
            }
            return -1;
        }
    }

    // Free our command
    cluster_multi_free(&mc);

    /* Clean up our hash table if we constructed it from variadic args */
    if (ht_free) {
        zend_hash_destroy(ht_arr);
        efree(ht_arr);
    }

    /* Return our object if we're in MULTI mode */
    if (!CLUSTER_IS_ATOMIC(c))
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

    /* This is a write command */
    c->readonly = 0;

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
    if(kv.val_free) efree(kv.val);

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
        if(kv.val_free) efree(kv.val);

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

    /* Return our object if we're in MULTI mode */
    if (!CLUSTER_IS_ATOMIC(c))
        RETVAL_ZVAL(getThis(), 1, 0);

    // Success
    return 0;
}

/* {{{ proto array RedisCluster::del(string key1, string key2, ... keyN) */
PHP_METHOD(RedisCluster, del) {
    zval *z_ret;

#if (PHP_MAJOR_VERSION < 7)
    MAKE_STD_ZVAL(z_ret);
#else
    z_ret = emalloc(sizeof(zval));
#endif

    // Initialize a LONG value to zero for our return
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
#if (PHP_MAJOR_VERSION < 7)
    MAKE_STD_ZVAL(z_ret);
#else
    z_ret = emalloc(sizeof(zval));
#endif
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
#if (PHP_MAJOR_VERSION < 7)
    MAKE_STD_ZVAL(z_ret);
#else
    z_ret = emalloc(sizeof(zval));
#endif
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
#if (PHP_MAJOR_VERSION < 7)
    MAKE_STD_ZVAL(z_ret);
#else
    z_ret = emalloc(sizeof(zval));
#endif
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
    CLUSTER_PROCESS_KW_CMD("SETEX", redis_key_long_val_cmd, cluster_bool_resp, 0);
}
/* }}} */

/* {{{ proto bool RedisCluster::psetex(string key, string value, int expiry) */
PHP_METHOD(RedisCluster, psetex) {
    CLUSTER_PROCESS_KW_CMD("PSETEX", redis_key_long_val_cmd, cluster_bool_resp, 0);
}
/* }}} */

/* {{{ proto bool RedisCluster::setnx(string key, string value) */
PHP_METHOD(RedisCluster, setnx) {
    CLUSTER_PROCESS_KW_CMD("SETNX", redis_kv_cmd, cluster_1_resp, 0);
}
/* }}} */

/* {{{ proto string RedisCluster::getSet(string key, string value) */
PHP_METHOD(RedisCluster, getset) {
    CLUSTER_PROCESS_KW_CMD("GETSET", redis_kv_cmd, cluster_bulk_resp, 0);
}
/* }}} */

/* {{{ proto int RedisCluster::exists(string key) */
PHP_METHOD(RedisCluster, exists) {
    CLUSTER_PROCESS_KW_CMD("EXISTS", redis_key_cmd, cluster_1_resp, 1);
}
/* }}} */

/* {{{ proto array Redis::keys(string pattern) */
PHP_METHOD(RedisCluster, keys) {
    redisCluster *c = GET_CONTEXT();
    redisClusterNode *node;
    strlen_t pat_len;
    char *pat, *cmd;
    clusterReply *resp;
    zval zv, *z_ret = &zv;
    int i, cmd_len;

    if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &pat, &pat_len)
                             ==FAILURE)
    {
        RETURN_FALSE;
    }

    /* Prefix and then build our command */
    cmd_len = redis_spprintf(c->flags, NULL TSRMLS_CC, &cmd, "KEYS", "k", pat, pat_len);

    array_init(z_ret);

    /* Treat as readonly */
    c->readonly = CLUSTER_IS_ATOMIC(c);

    /* Iterate over our known nodes */
    for(zend_hash_internal_pointer_reset(c->nodes);
        (node = zend_hash_get_current_data_ptr(c->nodes)) != NULL;
        zend_hash_move_forward(c->nodes))
    {
        if (cluster_send_slot(c, node->slot, cmd, cmd_len, TYPE_MULTIBULK
                             TSRMLS_CC)<0)
        {
            php_error_docref(0 TSRMLS_CC, E_ERROR, "Can't send KEYS to %s:%d",
                node->sock->host, node->sock->port);
            efree(cmd);
            RETURN_FALSE;
        }

        /* Ensure we can get a response */
        resp = cluster_read_resp(c TSRMLS_CC);
        if(!resp) {
            php_error_docref(0 TSRMLS_CC, E_WARNING, 
                "Can't read response from %s:%d", node->sock->host, 
                node->sock->port);
            continue;
        }

        /* Iterate keys, adding to our big array */
        for(i=0;i<resp->elements;i++) {
            /* Skip non bulk responses, they should all be bulk */
            if(resp->element[i]->type != TYPE_BULK) {
                continue;
            }

            add_next_index_stringl(z_ret, resp->element[i]->str,
                resp->element[i]->len);
        }

        /* Free response, don't free data */
        cluster_free_reply(resp, 0);
    }

    efree(cmd);

    /* Return our keys */
    RETURN_ZVAL(z_ret, 0, 1);
}
/* }}} */

/* {{{ proto int RedisCluster::type(string key) */
PHP_METHOD(RedisCluster, type) {
    CLUSTER_PROCESS_KW_CMD("TYPE", redis_key_cmd, cluster_type_resp, 1);
}
/* }}} */

/* {{{ proto string RedisCluster::pop(string key) */
PHP_METHOD(RedisCluster, lpop) {
    CLUSTER_PROCESS_KW_CMD("LPOP", redis_key_cmd, cluster_bulk_resp, 0);
}
/* }}} */

/* {{{ proto string RedisCluster::rpop(string key) */
PHP_METHOD(RedisCluster, rpop) {
    CLUSTER_PROCESS_KW_CMD("RPOP", redis_key_cmd, cluster_bulk_resp, 0);
}
/* }}} */

/* {{{ proto bool RedisCluster::lset(string key, long index, string val) */
PHP_METHOD(RedisCluster, lset) {
    CLUSTER_PROCESS_KW_CMD("LSET", redis_key_long_val_cmd, cluster_bool_resp, 0);
}
/* }}} */

/* {{{ proto string RedisCluster::spop(string key) */
PHP_METHOD(RedisCluster, spop) {
    if (ZEND_NUM_ARGS() == 1) {
        CLUSTER_PROCESS_KW_CMD("SPOP", redis_key_cmd, cluster_bulk_resp, 0);
    } else if (ZEND_NUM_ARGS() == 2) {
        CLUSTER_PROCESS_KW_CMD("SPOP", redis_key_long_cmd, cluster_mbulk_resp, 0);
    } else {
        ZEND_WRONG_PARAM_COUNT();
    }
}
/* }}} */

/* {{{ proto string|array RedisCluster::srandmember(string key, [long count]) */
PHP_METHOD(RedisCluster, srandmember) {
    redisCluster *c = GET_CONTEXT();
    cluster_cb cb;
    char *cmd; int cmd_len; short slot;
    short have_count;

    /* Treat as readonly */
    c->readonly = CLUSTER_IS_ATOMIC(c);

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

    cb = have_count ? cluster_mbulk_resp : cluster_bulk_resp;
    if (CLUSTER_IS_ATOMIC(c)) {
        cb(INTERNAL_FUNCTION_PARAM_PASSTHRU, c, NULL);
    } else {
        void *ctx = NULL;
        CLUSTER_ENQUEUE_RESPONSE(c, slot, cb, ctx);
        RETURN_ZVAL(getThis(), 1, 0);
    }
}

/* {{{ proto string RedisCluster::strlen(string key) */
PHP_METHOD(RedisCluster, strlen) {
    CLUSTER_PROCESS_KW_CMD("STRLEN", redis_key_cmd, cluster_long_resp, 1);
}

/* {{{ proto long RedisCluster::lpush(string key, string val1, ... valN) */
PHP_METHOD(RedisCluster, lpush) {
    CLUSTER_PROCESS_KW_CMD("LPUSH", redis_key_varval_cmd, cluster_long_resp, 0);
}
/* }}} */

/* {{{ proto long RedisCluster::rpush(string key, string val1, ... valN) */
PHP_METHOD(RedisCluster, rpush) {
    CLUSTER_PROCESS_KW_CMD("RPUSH", redis_key_varval_cmd, cluster_long_resp, 0);
}
/* }}} */

/* {{{ proto array RedisCluster::blpop(string key1, ... keyN, long timeout) */
PHP_METHOD(RedisCluster, blpop) {
    CLUSTER_PROCESS_CMD(blpop, cluster_mbulk_resp, 0);
}
/* }}} */

/* {{{ proto array RedisCluster::brpop(string key1, ... keyN, long timeout */
PHP_METHOD(RedisCluster, brpop) {
    CLUSTER_PROCESS_CMD(brpop, cluster_mbulk_resp, 0);
}
/* }}} */

/* {{{ proto long RedisCluster::rpushx(string key, mixed value) */
PHP_METHOD(RedisCluster, rpushx) {
    CLUSTER_PROCESS_KW_CMD("RPUSHX", redis_kv_cmd, cluster_long_resp, 0);
}
/* }}} */

/* {{{ proto long RedisCluster::lpushx(string key, mixed value) */
PHP_METHOD(RedisCluster, lpushx) {
    CLUSTER_PROCESS_KW_CMD("LPUSHX", redis_kv_cmd, cluster_long_resp, 0);
}
/* }}} */

/* {{{ proto long RedisCluster::linsert(string k,string pos,mix pvt,mix val) */
PHP_METHOD(RedisCluster, linsert) {
    CLUSTER_PROCESS_CMD(linsert, cluster_long_resp, 0);
}
/* }}} */

/* {{{ proto string RedisCluster::lindex(string key, long index) */
PHP_METHOD(RedisCluster, lindex) {
    CLUSTER_PROCESS_KW_CMD("LINDEX", redis_key_long_cmd, cluster_bulk_resp, 0);
}
/* }}} */

/* {{{ proto long RedisCluster::lrem(string key, long count, string val) */
PHP_METHOD(RedisCluster, lrem) {
    CLUSTER_PROCESS_CMD(lrem, cluster_long_resp, 0);
}
/* }}} */

/* {{{ proto string RedisCluster::rpoplpush(string key, string key) */
PHP_METHOD(RedisCluster, rpoplpush) {
    CLUSTER_PROCESS_KW_CMD("RPOPLPUSH", redis_key_key_cmd, cluster_bulk_resp, 0);
}
/* }}} */

/* {{{ proto string RedisCluster::brpoplpush(string key, string key, long tm) */
PHP_METHOD(RedisCluster, brpoplpush) {
    CLUSTER_PROCESS_CMD(brpoplpush, cluster_bulk_resp, 0);
}
/* }}} */

/* {{{ proto long RedisCluster::llen(string key)  */
PHP_METHOD(RedisCluster, llen) {
    CLUSTER_PROCESS_KW_CMD("LLEN", redis_key_cmd, cluster_long_resp, 1);
}
/* }}} */

/* {{{ proto long RedisCluster::scard(string key) */
PHP_METHOD(RedisCluster, scard) {
    CLUSTER_PROCESS_KW_CMD("SCARD", redis_key_cmd, cluster_long_resp, 1);
}
/* }}} */

/* {{{ proto array RedisCluster::smembers(string key) */
PHP_METHOD(RedisCluster, smembers) {
    CLUSTER_PROCESS_KW_CMD("SMEMBERS", redis_key_cmd, cluster_mbulk_resp, 1);
}
/* }}} */

/* {{{ proto long RedisCluster::sismember(string key) */
PHP_METHOD(RedisCluster, sismember) {
    CLUSTER_PROCESS_KW_CMD("SISMEMBER", redis_kv_cmd, cluster_1_resp, 1);
}
/* }}} */

/* {{{ proto long RedisCluster::sadd(string key, string val1 [, ...]) */
PHP_METHOD(RedisCluster, sadd) {
    CLUSTER_PROCESS_KW_CMD("SADD", redis_key_varval_cmd, cluster_long_resp, 0);
}
/* }}} */

/* {{{ proto long RedisCluster::saddarray(string key, array values) */
PHP_METHOD(RedisCluster, saddarray) {
    CLUSTER_PROCESS_KW_CMD("SADD", redis_key_arr_cmd, cluster_long_resp, 0);
}
/* }}} */

/* {{{ proto long RedisCluster::srem(string key, string val1 [, ...]) */
PHP_METHOD(RedisCluster, srem) {
    CLUSTER_PROCESS_KW_CMD("SREM", redis_key_varval_cmd, cluster_long_resp, 0);
}
/* }}} */

/* {{{ proto array RedisCluster::sunion(string key1, ... keyN) */
PHP_METHOD(RedisCluster, sunion) {
    CLUSTER_PROCESS_CMD(sunion, cluster_mbulk_resp, 0);
}
/* }}} */

/* {{{ proto long RedisCluster::sunionstore(string dst, string k1, ... kN) */
PHP_METHOD(RedisCluster, sunionstore) {
    CLUSTER_PROCESS_CMD(sunionstore, cluster_long_resp, 0);
}
/* }}} */

/* {{{ ptoto array RedisCluster::sinter(string k1, ... kN) */
PHP_METHOD(RedisCluster, sinter) {
    CLUSTER_PROCESS_CMD(sinter, cluster_mbulk_resp, 0);
}
/* }}} */

/* {{{ ptoto long RedisCluster::sinterstore(string dst, string k1, ... kN) */
PHP_METHOD(RedisCluster, sinterstore) {
    CLUSTER_PROCESS_CMD(sinterstore, cluster_long_resp, 0);
}
/* }}} */

/* {{{ proto array RedisCluster::sdiff(string k1, ... kN) */
PHP_METHOD(RedisCluster, sdiff) {
    CLUSTER_PROCESS_CMD(sdiff, cluster_mbulk_resp, 1);
}
/* }}} */

/* {{{ proto long RedisCluster::sdiffstore(string dst, string k1, ... kN) */
PHP_METHOD(RedisCluster, sdiffstore) {
    CLUSTER_PROCESS_CMD(sdiffstore, cluster_long_resp, 0);
}
/* }}} */

/* {{{ proto bool RedisCluster::smove(sting src, string dst, string mem) */
PHP_METHOD(RedisCluster, smove) {
    CLUSTER_PROCESS_CMD(smove, cluster_1_resp, 0);
}
/* }}} */

/* {{{ proto bool RedisCluster::persist(string key) */
PHP_METHOD(RedisCluster, persist) {
    CLUSTER_PROCESS_KW_CMD("PERSIST", redis_key_cmd, cluster_1_resp, 0);
}
/* }}} */

/* {{{ proto long RedisCluster::ttl(string key) */
PHP_METHOD(RedisCluster, ttl) {
    CLUSTER_PROCESS_KW_CMD("TTL", redis_key_cmd, cluster_long_resp, 1);
}
/* }}} */

/* {{{ proto long RedisCluster::pttl(string key) */
PHP_METHOD(RedisCluster, pttl) {
    CLUSTER_PROCESS_KW_CMD("PTTL", redis_key_cmd, cluster_long_resp, 1);
}
/* }}} */

/* {{{ proto long RedisCluster::zcard(string key) */
PHP_METHOD(RedisCluster, zcard) {
    CLUSTER_PROCESS_KW_CMD("ZCARD", redis_key_cmd, cluster_long_resp, 1);
}
/* }}} */

/* {{{ proto double RedisCluster::zscore(string key) */
PHP_METHOD(RedisCluster, zscore) {
    CLUSTER_PROCESS_KW_CMD("ZSCORE", redis_kv_cmd, cluster_dbl_resp, 1);
}
/* }}} */

/* {{{ proto long RedisCluster::zadd(string key,double score,string mem, ...) */
PHP_METHOD(RedisCluster, zadd) {
    CLUSTER_PROCESS_CMD(zadd, cluster_long_resp, 0);
}
/* }}} */

/* {{{ proto double RedisCluster::zincrby(string key, double by, string mem) */
PHP_METHOD(RedisCluster, zincrby) {
    CLUSTER_PROCESS_CMD(zincrby, cluster_dbl_resp, 0);
}
/* }}} */

/* {{{ proto RedisCluster::zremrangebyscore(string k, string s, string e) */
PHP_METHOD(RedisCluster, zremrangebyscore) {
    CLUSTER_PROCESS_KW_CMD("ZREMRANGEBYSCORE", redis_key_str_str_cmd,
        cluster_long_resp, 0);
}
/* }}} */

/* {{{ proto RedisCluster::zcount(string key, string s, string e) */
PHP_METHOD(RedisCluster, zcount) {
    CLUSTER_PROCESS_KW_CMD("ZCOUNT", redis_key_str_str_cmd, cluster_long_resp, 1);
}
/* }}} */

/* {{{ proto long RedisCluster::zrank(string key, mixed member) */
PHP_METHOD(RedisCluster, zrank) {
    CLUSTER_PROCESS_KW_CMD("ZRANK", redis_kv_cmd, cluster_long_resp, 1);
}
/* }}} */

/* {{{ proto long RedisCluster::zrevrank(string key, mixed member) */
PHP_METHOD(RedisCluster, zrevrank) {
    CLUSTER_PROCESS_KW_CMD("ZREVRANK", redis_kv_cmd, cluster_long_resp, 1);
}
/* }}} */

/* {{{ proto long RedisCluster::hlen(string key) */
PHP_METHOD(RedisCluster, hlen) {
    CLUSTER_PROCESS_KW_CMD("HLEN", redis_key_cmd, cluster_long_resp, 1);
}
/* }}} */

/* {{{ proto array RedisCluster::hkeys(string key) */
PHP_METHOD(RedisCluster, hkeys) {
    CLUSTER_PROCESS_KW_CMD("HKEYS", redis_key_cmd, cluster_mbulk_raw_resp, 1);
}
/* }}} */

/* {{{ proto array RedisCluster::hvals(string key) */
PHP_METHOD(RedisCluster, hvals) {
    CLUSTER_PROCESS_KW_CMD("HVALS", redis_key_cmd, cluster_mbulk_resp, 1);
}
/* }}} */

/* {{{ proto string RedisCluster::hget(string key, string mem) */
PHP_METHOD(RedisCluster, hget) {
    CLUSTER_PROCESS_KW_CMD("HGET", redis_key_str_cmd, cluster_bulk_resp, 1);
}
/* }}} */

/* {{{ proto bool RedisCluster::hset(string key, string mem, string val) */
PHP_METHOD(RedisCluster, hset) {
    CLUSTER_PROCESS_CMD(hset, cluster_long_resp, 0);
}
/* }}} */

/* {{{ proto bool RedisCluster::hsetnx(string key, string mem, string val) */
PHP_METHOD(RedisCluster, hsetnx) {
    CLUSTER_PROCESS_CMD(hsetnx, cluster_1_resp, 0);
}
/* }}} */

/* {{{ proto array RedisCluster::hgetall(string key) */
PHP_METHOD(RedisCluster, hgetall) {
    CLUSTER_PROCESS_KW_CMD("HGETALL", redis_key_cmd,
        cluster_mbulk_zipstr_resp, 1);
}
/* }}} */

/* {{{ proto bool RedisCluster::hexists(string key, string member) */
PHP_METHOD(RedisCluster, hexists) {
    CLUSTER_PROCESS_KW_CMD("HEXISTS", redis_key_str_cmd, cluster_1_resp, 1);
}
/* }}} */

/* {{{ proto long RedisCluster::hincr(string key, string mem, long val) */
PHP_METHOD(RedisCluster, hincrby) {
    CLUSTER_PROCESS_CMD(hincrby, cluster_long_resp, 0);
}
/* }}} */

/* {{{ proto double RedisCluster::hincrbyfloat(string k, string m, double v) */
PHP_METHOD(RedisCluster, hincrbyfloat) {
    CLUSTER_PROCESS_CMD(hincrbyfloat, cluster_dbl_resp, 0);
}
/* }}} */

/* {{{ proto bool RedisCluster::hmset(string key, array key_vals) */
PHP_METHOD(RedisCluster, hmset) {
    CLUSTER_PROCESS_CMD(hmset, cluster_bool_resp, 0);
}
/* }}} */

/* {{{ proto long RedisCluster::hdel(string key, string mem1, ... memN) */
PHP_METHOD(RedisCluster, hdel) {
    CLUSTER_PROCESS_CMD(hdel, cluster_long_resp, 0);
}
/* }}} */

/* {{{ proto array RedisCluster::hmget(string key, array members) */
PHP_METHOD(RedisCluster, hmget) {
    CLUSTER_PROCESS_CMD(hmget, cluster_mbulk_assoc_resp, 1);
}
/* }}} */

/* {{{ proto array RedisCluster::hstrlen(string key, string field) */
PHP_METHOD(RedisCluster, hstrlen) {
    CLUSTER_PROCESS_CMD(hstrlen, cluster_long_resp, 1);
}
/* }}} */


/* {{{ proto string RedisCluster::dump(string key) */
PHP_METHOD(RedisCluster, dump) {
    CLUSTER_PROCESS_KW_CMD("DUMP", redis_key_cmd, cluster_bulk_raw_resp, 1);
}

/* {{{ proto long RedisCluster::incr(string key) */
PHP_METHOD(RedisCluster, incr) {
    CLUSTER_PROCESS_CMD(incr, cluster_long_resp, 0);
}
/* }}} */

/* {{{ proto long RedisCluster::incrby(string key, long byval) */
PHP_METHOD(RedisCluster, incrby) {
    CLUSTER_PROCESS_KW_CMD("INCRBY", redis_key_long_cmd, cluster_long_resp, 0);
}
/* }}} */

/* {{{ proto long RedisCluster::decr(string key) */
PHP_METHOD(RedisCluster, decr) {
    CLUSTER_PROCESS_CMD(decr, cluster_long_resp, 0);
}
/* }}} */

/* {{{ proto long RedisCluster::decrby(string key, long byval) */
PHP_METHOD(RedisCluster, decrby) {
    CLUSTER_PROCESS_KW_CMD("DECRBY", redis_key_long_cmd, cluster_long_resp, 0);
}
/* }}} */

/* {{{ proto double RedisCluster::incrbyfloat(string key, double val) */
PHP_METHOD(RedisCluster, incrbyfloat) {
    CLUSTER_PROCESS_KW_CMD("INCRBYFLOAT", redis_key_dbl_cmd,
        cluster_dbl_resp, 0);
}
/* }}} */

/* {{{ proto double RedisCluster::decrbyfloat(string key, double val) */
PHP_METHOD(RedisCluster, decrbyfloat) {
    CLUSTER_PROCESS_KW_CMD("DECRBYFLOAT", redis_key_dbl_cmd,
        cluster_dbl_resp, 0);
}
/* }}} */

/* {{{ proto bool RedisCluster::expire(string key, long sec) */
PHP_METHOD(RedisCluster, expire) {
    CLUSTER_PROCESS_KW_CMD("EXPIRE", redis_key_long_cmd, cluster_1_resp, 0);
}
/* }}} */

/* {{{ proto bool RedisCluster::expireat(string key, long ts) */
PHP_METHOD(RedisCluster, expireat) {
    CLUSTER_PROCESS_KW_CMD("EXPIREAT", redis_key_long_cmd, cluster_1_resp, 0);
}

/* {{{ proto bool RedisCluster::pexpire(string key, long ms) */
PHP_METHOD(RedisCluster, pexpire) {
    CLUSTER_PROCESS_KW_CMD("PEXPIRE", redis_key_long_cmd, cluster_1_resp, 0);
}
/* }}} */

/* {{{ proto bool RedisCluster::pexpireat(string key, long ts) */
PHP_METHOD(RedisCluster, pexpireat) {
    CLUSTER_PROCESS_KW_CMD("PEXPIREAT", redis_key_long_cmd, cluster_1_resp, 0);
}
/* }}} */

/* {{{ proto long RedisCluster::append(string key, string val) */
PHP_METHOD(RedisCluster, append) {
    CLUSTER_PROCESS_KW_CMD("APPEND", redis_kv_cmd, cluster_long_resp, 0);
}
/* }}} */

/* {{{ proto long RedisCluster::getbit(string key, long val) */
PHP_METHOD(RedisCluster, getbit) {
    CLUSTER_PROCESS_KW_CMD("GETBIT", redis_key_long_cmd, cluster_long_resp, 1);
}
/* }}} */

/* {{{ proto long RedisCluster::setbit(string key, long offset, bool onoff) */
PHP_METHOD(RedisCluster, setbit) {
    CLUSTER_PROCESS_CMD(setbit, cluster_long_resp, 0);
}

/* {{{ proto long RedisCluster::bitop(string op,string key,[string key2,...]) */
PHP_METHOD(RedisCluster, bitop)
{
    CLUSTER_PROCESS_CMD(bitop, cluster_long_resp, 0);
}
/* }}} */

/* {{{ proto long RedisCluster::bitcount(string key, [int start, int end]) */
PHP_METHOD(RedisCluster, bitcount) {
    CLUSTER_PROCESS_CMD(bitcount, cluster_long_resp, 1);
}
/* }}} */

/* {{{ proto long RedisCluster::bitpos(string key, int bit, [int s, int end]) */
PHP_METHOD(RedisCluster, bitpos) {
    CLUSTER_PROCESS_CMD(bitpos, cluster_long_resp, 1);
}
/* }}} */

/* {{{ proto string Redis::lget(string key, long index) */
PHP_METHOD(RedisCluster, lget) {
    CLUSTER_PROCESS_KW_CMD("LINDEX", redis_key_long_cmd, cluster_bulk_resp, 1);
}
/* }}} */

/* {{{ proto string RedisCluster::getrange(string key, long start, long end) */
PHP_METHOD(RedisCluster, getrange) {
    CLUSTER_PROCESS_KW_CMD("GETRANGE", redis_key_long_long_cmd,
        cluster_bulk_resp, 1);
}
/* }}} */

/* {{{ proto string RedisCluster::ltrim(string key, long start, long end) */
PHP_METHOD(RedisCluster, ltrim) {
    CLUSTER_PROCESS_KW_CMD("LTRIM", redis_key_long_long_cmd, cluster_bool_resp, 0);
}
/* }}} */

/* {{{ proto array RedisCluster::lrange(string key, long start, long end) */
PHP_METHOD(RedisCluster, lrange) {
    CLUSTER_PROCESS_KW_CMD("LRANGE", redis_key_long_long_cmd,
        cluster_mbulk_resp, 1);
}
/* }}} */

/* {{{ proto long RedisCluster::zremrangebyrank(string k, long s, long e) */
PHP_METHOD(RedisCluster, zremrangebyrank) {
    CLUSTER_PROCESS_KW_CMD("ZREMRANGEBYRANK", redis_key_long_long_cmd,
        cluster_long_resp, 0);
}
/* }}} */

/* {{{ proto long RedisCluster::publish(string key, string msg) */
PHP_METHOD(RedisCluster, publish) {
    CLUSTER_PROCESS_KW_CMD("PUBLISH", redis_key_str_cmd, cluster_long_resp, 0);
}
/* }}} */

/* {{{ proto bool RedisCluster::rename(string key1, string key2) */
PHP_METHOD(RedisCluster, rename) {
    CLUSTER_PROCESS_KW_CMD("RENAME", redis_key_key_cmd, cluster_bool_resp, 0);
}
/* }}} */

/* {{{ proto bool RedisCluster::renamenx(string key1, string key2) */
PHP_METHOD(RedisCluster, renamenx) {
    CLUSTER_PROCESS_KW_CMD("RENAMENX", redis_key_key_cmd, cluster_1_resp, 0);
}
/* }}} */

/* {{{ proto long RedisCluster::pfcount(string key) */
PHP_METHOD(RedisCluster, pfcount) {
    CLUSTER_PROCESS_CMD(pfcount, cluster_long_resp, 1);
}
/* }}} */

/* {{{ proto bool RedisCluster::pfadd(string key, array vals) */
PHP_METHOD(RedisCluster, pfadd) {
    CLUSTER_PROCESS_CMD(pfadd, cluster_1_resp, 0);
}
/* }}} */

/* {{{ proto bool RedisCluster::pfmerge(string key, array keys) */
PHP_METHOD(RedisCluster, pfmerge) {
    CLUSTER_PROCESS_CMD(pfmerge, cluster_bool_resp, 0);
}
/* }}} */

/* {{{ proto boolean RedisCluster::restore(string key, long ttl, string val) */
PHP_METHOD(RedisCluster, restore) {
    CLUSTER_PROCESS_KW_CMD("RESTORE", redis_key_long_str_cmd,
        cluster_bool_resp, 0);
}
/* }}} */

/* {{{ proto long RedisCluster::setrange(string key, long offset, string val) */
PHP_METHOD(RedisCluster, setrange) {
    CLUSTER_PROCESS_KW_CMD("SETRANGE", redis_key_long_str_cmd,
        cluster_long_resp, 0);
}
/* }}} */

/* Generic implementation for ZRANGE, ZREVRANGE, ZRANGEBYSCORE, ZREVRANGEBYSCORE */
static void generic_zrange_cmd(INTERNAL_FUNCTION_PARAMETERS, char *kw,
                               zrange_cb fun)
{
    redisCluster *c = GET_CONTEXT();
    c->readonly = CLUSTER_IS_ATOMIC(c);
    cluster_cb cb;
    char *cmd; int cmd_len; short slot;
    int withscores=0;

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

    cb = withscores ? cluster_mbulk_zipdbl_resp : cluster_mbulk_resp;
    if (CLUSTER_IS_ATOMIC(c)) {
        cb(INTERNAL_FUNCTION_PARAM_PASSTHRU, c, NULL);
    } else {
        void *ctx = NULL;
        CLUSTER_ENQUEUE_RESPONSE(c, slot, cb, ctx);
        RETURN_ZVAL(getThis(), 1, 0);
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
    CLUSTER_PROCESS_KW_CMD("ZUNIONSTORE", redis_zinter_cmd, cluster_long_resp, 0);
}
/* }}} */

/* {{{ proto RedisCluster::zinterstore(string dst, array keys, [array weights,
 *                                     string agg]) */
PHP_METHOD(RedisCluster, zinterstore) {
    CLUSTER_PROCESS_KW_CMD("ZINTERSTORE", redis_zinter_cmd, cluster_long_resp, 0);
}
/* }}} */

/* {{{ proto RedisCluster::zrem(string key, string val1, ... valN) */
PHP_METHOD(RedisCluster, zrem) {
    CLUSTER_PROCESS_KW_CMD("ZREM", redis_key_varval_cmd, cluster_long_resp, 0);
}
/* }}} */

/* {{{ proto array
 *     RedisCluster::zrevrangebyscore(string k, long s, long e, array opts) */
PHP_METHOD(RedisCluster, zrevrangebyscore) {
    generic_zrange_cmd(INTERNAL_FUNCTION_PARAM_PASSTHRU, "ZREVRANGEBYSCORE",
        redis_zrangebyscore_cmd);
}
/* }}} */

/* {{{ proto array RedisCluster::zrangebylex(string key, string min, string max, 
 *                                           [offset, count]) */
PHP_METHOD(RedisCluster, zrangebylex) {
    CLUSTER_PROCESS_KW_CMD("ZRANGEBYLEX", redis_zrangebylex_cmd, 
        cluster_mbulk_resp, 1);
}
/* }}} */

/* {{{ proto array RedisCluster::zrevrangebylex(string key, string min,
 *                                              string min, [long off, long limit) */
PHP_METHOD(RedisCluster, zrevrangebylex) {
    CLUSTER_PROCESS_KW_CMD("ZREVRANGEBYLEX", redis_zrangebylex_cmd,
        cluster_mbulk_resp, 1);
}
/* }}} */

/* {{{ proto long RedisCluster::zlexcount(string key, string min, string max) */
PHP_METHOD(RedisCluster, zlexcount) {
    CLUSTER_PROCESS_KW_CMD("ZLEXCOUNT", redis_gen_zlex_cmd, cluster_long_resp, 1);
}
/* }}} */

/* {{{ proto long RedisCluster::zremrangebylex(string key, string min, string max) */
PHP_METHOD(RedisCluster, zremrangebylex) {
    CLUSTER_PROCESS_KW_CMD("ZREMRANGEBYLEX", redis_gen_zlex_cmd, 
        cluster_long_resp, 0);
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
    CLUSTER_PROCESS_KW_CMD("SUBSCRIBE", redis_subscribe_cmd, cluster_sub_resp, 0);
}
/* }}} */

/* {{{ proto null RedisCluster::psubscribe(array pats, callable cb) */
PHP_METHOD(RedisCluster, psubscribe) {
    CLUSTER_PROCESS_KW_CMD("PSUBSCRIBE", redis_subscribe_cmd, cluster_sub_resp, 0);
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
    if(cluster_send_slot(c, c->subscribed_slot, cmd, cmd_len, TYPE_MULTIBULK 
                         TSRMLS_CC) ==FAILURE)
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

/* {{{ proto mixed RedisCluster::eval(string script, [array args, int numkeys) */
PHP_METHOD(RedisCluster, eval) {
    CLUSTER_PROCESS_KW_CMD("EVAL", redis_eval_cmd, cluster_variant_resp, 0);
}
/* }}} */

/* {{{ proto mixed RedisCluster::evalsha(string sha, [array args, int numkeys]) */
PHP_METHOD(RedisCluster, evalsha) {
    CLUSTER_PROCESS_KW_CMD("EVALSHA", redis_eval_cmd, cluster_variant_resp, 0);
}
/* }}} */

/* Commands that do not interact with Redis, but just report stuff about
 * various options, etc */

/* {{{ proto string RedisCluster::getmode() */
PHP_METHOD(RedisCluster, getmode) {
    redisCluster *c = GET_CONTEXT();
    RETURN_LONG(c->flags->mode);
}
/* }}} */

/* {{{ proto string RedisCluster::getlasterror() */
PHP_METHOD(RedisCluster, getlasterror) {
    redisCluster *c = GET_CONTEXT();

    if(c->err != NULL && c->err_len > 0) {
        RETURN_STRINGL(c->err, c->err_len);
    } else {
        RETURN_NULL();
    }
}
/* }}} */

/* {{{ proto bool RedisCluster::clearlasterror() */
PHP_METHOD(RedisCluster, clearlasterror) {
    redisCluster *c = GET_CONTEXT();
    
    if (c->err) efree(c->err);
    c->err = NULL;
    c->err_len = 0;
    
    RETURN_TRUE;
}
/* }}} */

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
    redisClusterNode *node;
    zval zv, *z_ret = &zv;
    char *host;
    short port;

    array_init(z_ret);

    for(zend_hash_internal_pointer_reset(c->nodes);
        (node = zend_hash_get_current_data_ptr(c->nodes)) != NULL;
        zend_hash_move_forward(c->nodes))
    {
        host = node->sock->host;
        port = node->sock->port;

        zval z, *z_sub = &z;
#if (PHP_MAJOR_VERSION < 7)
        MAKE_STD_ZVAL(z_sub);
#endif
        array_init(z_sub);

        add_next_index_stringl(z_sub, host, strlen(host));
        add_next_index_long(z_sub, port);
        add_next_index_zval(z_ret, z_sub);
    }

    RETVAL_ZVAL(z_ret, 0, 1);
}

PHP_METHOD(RedisCluster, _redir) {
    redisCluster *c = GET_CONTEXT();
    char buf[255];
    size_t len;

    len = snprintf(buf, sizeof(buf), "%s:%d", c->redir_host, c->redir_port);
    if(*c->redir_host && c->redir_host_len) {
        RETURN_STRINGL(buf, len);
    } else {
        RETURN_NULL();
    }
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

    /* Flag that we're in MULTI mode */
    c->flags->mode = MULTI;

    /* Return our object so we can chain MULTI calls */
    RETVAL_ZVAL(getThis(), 1, 0);
}

/* {{{ proto bool RedisCluster::watch() */
PHP_METHOD(RedisCluster, watch) {
    redisCluster *c = GET_CONTEXT();
    HashTable *ht_dist;
    clusterDistList *dl;
    smart_string cmd = {0};
    zval *z_args;
    int argc = ZEND_NUM_ARGS(), i;
    zend_ulong slot;
    zend_string *zstr;

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
    z_args = emalloc(sizeof(zval) * argc);
    if (zend_get_parameters_array(ht, argc, z_args) == FAILURE) {
        efree(z_args);
        cluster_dist_free(ht_dist);
        RETURN_FALSE;
    }

    // Loop through arguments, prefixing if needed
    for(i=0;i<argc;i++) {
        // We'll need the key as a string
        zstr = zval_get_string(&z_args[i]);

        // Add this key to our distribution handler
        if (cluster_dist_add_key(c, ht_dist, zstr->val, zstr->len, NULL) == FAILURE) {
            zend_throw_exception(redis_cluster_exception_ce,
                "Can't issue WATCH command as the keyspace isn't fully mapped",
                0 TSRMLS_CC);
            zend_string_release(zstr);
            RETURN_FALSE;
        }
        zend_string_release(zstr);
    }

    // Iterate over each node we'll be sending commands to
    for(zend_hash_internal_pointer_reset(ht_dist);
		zend_hash_get_current_key(ht_dist, NULL, &slot) == HASH_KEY_IS_LONG;
        zend_hash_move_forward(ht_dist))
    {
        // Grab the clusterDistList pointer itself
        if ((dl = zend_hash_get_current_data_ptr(ht_dist)) == NULL) {
            zend_throw_exception(redis_cluster_exception_ce,
                "Internal error in a PHP HashTable", 0 TSRMLS_CC);
            cluster_dist_free(ht_dist);
            efree(z_args);
            efree(cmd.c);
            RETURN_FALSE;
        }

        // Construct our watch command for this node
        redis_cmd_init_sstr(&cmd, dl->len, "WATCH", sizeof("WATCH")-1);
        for (i = 0; i < dl->len; i++) {
            redis_cmd_append_sstr(&cmd, dl->entry[i].key, 
                dl->entry[i].key_len);
        }

        // If we get a failure from this, we have to abort
        if (cluster_send_command(c,(short)slot,cmd.c,cmd.len TSRMLS_CC)==-1) {
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
        php_error_docref(NULL TSRMLS_CC, E_WARNING, "RedisCluster is not in MULTI mode");
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
        php_error_docref(NULL TSRMLS_CC, E_WARNING, "Cluster is not in MULTI mode");
        RETURN_FALSE;
    }
    
    if(cluster_abort_exec(c TSRMLS_CC)<0) {
        CLUSTER_RESET_MULTI(c);
    }

    CLUSTER_FREE_QUEUE(c);

    RETURN_TRUE;
}

/* Get a slot either by key (string) or host/port array */
static short
cluster_cmd_get_slot(redisCluster *c, zval *z_arg TSRMLS_DC) 
{
    strlen_t key_len;
    int key_free;
    zval *z_host, *z_port;
    short slot;
    char *key;
    zend_string *zstr;

    /* If it's a string, treat it as a key.  Otherwise, look for a two
     * element array */
    if(Z_TYPE_P(z_arg)==IS_STRING || Z_TYPE_P(z_arg)==IS_LONG ||
       Z_TYPE_P(z_arg)==IS_DOUBLE) 
    {
        /* Allow for any scalar here */
        zstr = zval_get_string(z_arg);
        key = zstr->val;
        key_len = zstr->len;

        /* Hash it */
        key_free = redis_key_prefix(c->flags, &key, &key_len);
        slot = cluster_hash_key(key, key_len);
        zend_string_release(zstr);
        if(key_free) efree(key);
    } else if (Z_TYPE_P(z_arg) == IS_ARRAY && 
		(z_host = zend_hash_index_find(Z_ARRVAL_P(z_arg), 0)) != NULL &&
		(z_port = zend_hash_index_find(Z_ARRVAL_P(z_arg), 1)) != NULL &&
		Z_TYPE_P(z_host) == IS_STRING && Z_TYPE_P(z_port) == IS_LONG
	) {
        /* Attempt to find this specific node by host:port */
        slot = cluster_find_slot(c,(const char *)Z_STRVAL_P(z_host),
            (unsigned short)Z_LVAL_P(z_port));

        /* Inform the caller if they've passed bad data */
        if(slot < 0) { 
            php_error_docref(0 TSRMLS_CC, E_WARNING, "Unknown node %s:%ld",
                Z_STRVAL_P(z_host), Z_LVAL_P(z_port));
        }
    } else {
        php_error_docref(0 TSRMLS_CC, E_WARNING,
            "Direted commands musty be passed a key or [host,port] array");
        return -1;
    }

    return slot;
}

/* Generic handler for things we want directed at a given node, like SAVE,
 * BGSAVE, FLUSHDB, FLUSHALL, etc */
static void 
cluster_empty_node_cmd(INTERNAL_FUNCTION_PARAMETERS, char *kw, 
                       REDIS_REPLY_TYPE reply_type, cluster_cb cb)
{
    redisCluster *c = GET_CONTEXT();
    char *cmd; 
    int cmd_len;
    zval *z_arg;
    short slot;

    if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "z", &z_arg)==FAILURE) {
        RETURN_FALSE;
    }

    // One argument means find the node (treated like a key), and two means
    // send the command to a specific host and port
    slot = cluster_cmd_get_slot(c, z_arg TSRMLS_CC);
    if(slot<0) {
        RETURN_FALSE;
    }

    // Construct our command
    cmd_len = redis_spprintf(NULL, NULL TSRMLS_CC, &cmd, kw, "");

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

/* Generic routine for handling various commands which need to be directed at
 * a node, but have complex syntax.  We simply parse out the arguments and send
 * the command as constructed by the caller */
static void cluster_raw_cmd(INTERNAL_FUNCTION_PARAMETERS, char *kw, int kw_len) 
{
    redisCluster *c = GET_CONTEXT();
    smart_string cmd = {0};
    zval *z_args;
    short slot;
    int i, argc = ZEND_NUM_ARGS();

    /* Commands using this pass-thru don't need to be enabled in MULTI mode */
    if(!CLUSTER_IS_ATOMIC(c)) {
        php_error_docref(0 TSRMLS_CC, E_WARNING,
            "Command can't be issued in MULTI mode");
        RETURN_FALSE;
    }

    /* We at least need the key or [host,port] argument */
    if(argc<1) {
        php_error_docref(0 TSRMLS_CC, E_WARNING,
            "Command requires at least an argument to direct to a node");
        RETURN_FALSE;
    }

    /* Allocate an array to process arguments */
    z_args = emalloc(argc * sizeof(zval));

    /* Grab args */
    if (zend_get_parameters_array(ht, argc, z_args) == FAILURE) {
        efree(z_args);
        RETURN_FALSE;
    }

    /* First argument needs to be the "where" */
    if((slot = cluster_cmd_get_slot(c, &z_args[0] TSRMLS_CC))<0) {
        RETURN_FALSE;
    }

    /* Initialize our command */
    redis_cmd_init_sstr(&cmd, argc-1, kw, kw_len);

    /* Iterate, appending args */
    for(i=1;i<argc;i++) {
        zend_string *zstr = zval_get_string(&z_args[i]);
        redis_cmd_append_sstr(&cmd, zstr->val, zstr->len);
        zend_string_release(zstr);
    }

    /* Send it off */
    if(cluster_send_slot(c, slot, cmd.c, cmd.len, TYPE_EOF TSRMLS_CC)<0) {
        zend_throw_exception(redis_cluster_exception_ce,
            "Couldn't send command to node", 0 TSRMLS_CC);
        efree(cmd.c);
        efree(z_args);
        RETURN_FALSE;
    }

    /* Read the response variant */
    cluster_variant_resp(INTERNAL_FUNCTION_PARAM_PASSTHRU, c, NULL);

    efree(cmd.c);
    efree(z_args);
}

/* Generic method for HSCAN, SSCAN, and ZSCAN */
static void cluster_kscan_cmd(INTERNAL_FUNCTION_PARAMETERS, 
                              REDIS_SCAN_TYPE type)
{
    redisCluster *c = GET_CONTEXT();
    char *cmd, *pat=NULL, *key=NULL; 
    strlen_t key_len = 0, pat_len = 0;
    int cmd_len, key_free=0;
    short slot;
    zval *z_it;
    HashTable *hash;
    long it, num_ele;
    zend_long count = 0;

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

    /* Treat as readonly */
    c->readonly = 1;

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
        /* Free our return value if we're back in the loop */
        if (Z_TYPE_P(return_value) == IS_ARRAY) {
            zval_dtor(return_value);
            ZVAL_NULL(return_value);
        } 
    
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
    char *cmd, *pat=NULL;
    strlen_t pat_len = 0;
    int cmd_len;
    short slot;
    zval *z_it, *z_node;
    long it, num_ele;
    zend_long count = 0;

    /* Treat as read-only */
    c->readonly = CLUSTER_IS_ATOMIC(c);

    /* Can't be in MULTI mode */
    if(!CLUSTER_IS_ATOMIC(c)) {
        zend_throw_exception(redis_cluster_exception_ce,
            "SCAN type commands can't be called in MULTI mode", 0 TSRMLS_CC);
        RETURN_FALSE;
    }

    /* Parse arguments */
    if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "z/z|s!l", &z_it, 
                             &z_node, &pat, &pat_len, &count)==FAILURE)
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
        /* Free our return value if we're back in the loop */
        if (Z_TYPE_P(return_value) == IS_ARRAY) {
            zval_dtor(return_value);
            ZVAL_NULL(return_value);
        }
    
        /* Construct our command */
        cmd_len = redis_fmt_scan_cmd(&cmd, TYPE_SCAN, NULL, 0, it, pat, pat_len,
            count);
       
        if((slot = cluster_cmd_get_slot(c, z_node TSRMLS_CC))<0) {
           RETURN_FALSE;
        }

        // Send it to the node in question
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
/* }}} */

/* {{{ proto RedisCluster::sscan(string key, long it [string pat, long cnt]) */
PHP_METHOD(RedisCluster, sscan) {
    cluster_kscan_cmd(INTERNAL_FUNCTION_PARAM_PASSTHRU, TYPE_SSCAN);
}
/* }}} */

/* {{{ proto RedisCluster::zscan(string key, long it [string pat, long cnt]) */
PHP_METHOD(RedisCluster, zscan) {
    cluster_kscan_cmd(INTERNAL_FUNCTION_PARAM_PASSTHRU, TYPE_ZSCAN);
}
/* }}} */

/* {{{ proto RedisCluster::hscan(string key, long it [string pat, long cnt]) */
PHP_METHOD(RedisCluster, hscan) {
    cluster_kscan_cmd(INTERNAL_FUNCTION_PARAM_PASSTHRU, TYPE_HSCAN);
}
/* }}} */

/* {{{ proto RedisCluster::save(string key)
 *     proto RedisCluster::save(string host, long port) */
PHP_METHOD(RedisCluster, save) {
    cluster_empty_node_cmd(INTERNAL_FUNCTION_PARAM_PASSTHRU, "SAVE", TYPE_LINE,
        cluster_bool_resp);
}
/* }}} */

/* {{{ proto RedisCluster::bgsave(string key) 
 *     proto RedisCluster::bgsave(string host, long port) */
PHP_METHOD(RedisCluster, bgsave) {
    cluster_empty_node_cmd(INTERNAL_FUNCTION_PARAM_PASSTHRU, "BGSAVE", 
        TYPE_LINE, cluster_bool_resp);
}
/* }}} */

/* {{{ proto RedisCluster::flushdb(string key)
 *     proto RedisCluster::flushdb(string host, long port) */
PHP_METHOD(RedisCluster, flushdb) {
    cluster_empty_node_cmd(INTERNAL_FUNCTION_PARAM_PASSTHRU, "FLUSHDB",
        TYPE_LINE, cluster_bool_resp);
}
/* }}} */

/* {{{ proto RedisCluster::flushall(string key)
 *     proto RedisCluster::flushall(string host, long port) */
PHP_METHOD(RedisCluster, flushall) {
    cluster_empty_node_cmd(INTERNAL_FUNCTION_PARAM_PASSTHRU, "FLUSHALL",
        TYPE_LINE, cluster_bool_resp);
}
/* }}} */

/* {{{ proto RedisCluster::dbsize(string key)
 *     proto RedisCluster::dbsize(string host, long port) */
PHP_METHOD(RedisCluster, dbsize) {
    cluster_empty_node_cmd(INTERNAL_FUNCTION_PARAM_PASSTHRU, "DBSIZE",
        TYPE_INT, cluster_long_resp);
}
/* }}} */

/* {{{ proto RedisCluster::bgrewriteaof(string key)
 *     proto RedisCluster::bgrewriteaof(string host, long port) */
PHP_METHOD(RedisCluster, bgrewriteaof) {
    cluster_empty_node_cmd(INTERNAL_FUNCTION_PARAM_PASSTHRU, "BGREWRITEAOF",
        TYPE_LINE, cluster_bool_resp);
}
/* }}} */

/* {{{ proto RedisCluster::lastsave(string key)
 *     proto RedisCluster::lastsave(array $host_port) */
PHP_METHOD(RedisCluster, lastsave) {
    cluster_empty_node_cmd(INTERNAL_FUNCTION_PARAM_PASSTHRU, "LASTSAVE",
        TYPE_INT, cluster_long_resp);
}
/* }}} */

/* {{{ proto array RedisCluster::info(string key, [string $arg])
 *     proto array RedisCluster::info(array host_port, [string $arg]) */
PHP_METHOD(RedisCluster, info) {
    redisCluster *c = GET_CONTEXT();
    REDIS_REPLY_TYPE rtype;
    char *cmd, *opt=NULL;
    int cmd_len;
    strlen_t opt_len = 0;
    void *ctx = NULL;
    zval *z_arg;
    short slot;

    if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "z|s", &z_arg, &opt,
                             &opt_len)==FAILURE)
    {
        RETURN_FALSE;
    }

    /* Treat INFO as non read-only, as we probably want the master */
    c->readonly = 0;

    slot = cluster_cmd_get_slot(c, z_arg TSRMLS_CC);
    if (slot < 0) {
        RETURN_FALSE;
    }

    if (opt != NULL) {
        cmd_len = redis_spprintf(NULL, NULL TSRMLS_CC, &cmd, "INFO", "s", opt, opt_len);
    } else {
        cmd_len = redis_spprintf(NULL, NULL TSRMLS_CC, &cmd, "INFO", "");
    }

    rtype = CLUSTER_IS_ATOMIC(c) ? TYPE_BULK : TYPE_LINE;
    if (cluster_send_slot(c, slot, cmd, cmd_len, rtype TSRMLS_CC)<0) {
        zend_throw_exception(redis_cluster_exception_ce,
            "Unable to send INFO command to specific node", 0 TSRMLS_CC);
        efree(cmd);
        RETURN_FALSE;
    }

    if (CLUSTER_IS_ATOMIC(c)) {
        cluster_info_resp(INTERNAL_FUNCTION_PARAM_PASSTHRU, c, NULL);
    } else {
        CLUSTER_ENQUEUE_RESPONSE(c, slot, cluster_info_resp, ctx);
    }

    efree(cmd);
}
/* }}} */

/* {{{ proto array RedisCluster::client('list')
 *     proto bool RedisCluster::client('kill', $ipport)
 *     proto bool RedisCluster::client('setname', $name)
 *     proto string RedisCluster::client('getname')
 */
PHP_METHOD(RedisCluster, client) {
    redisCluster *c = GET_CONTEXT();
    char *cmd, *opt=NULL, *arg=NULL;
    int cmd_len;
    strlen_t opt_len, arg_len = 0;
    REDIS_REPLY_TYPE rtype;
    zval *z_node;
    short slot;
    cluster_cb cb;

    /* Parse args */
    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "zs|s", &z_node, &opt, 
                              &opt_len, &arg, &arg_len)==FAILURE)
    {
        RETURN_FALSE;
    }
    
    /* Make sure we can properly resolve the slot */
    slot = cluster_cmd_get_slot(c, z_node TSRMLS_CC);
    if(slot<0) RETURN_FALSE;

    /* Our return type and reply callback is different for all subcommands */
    if (opt_len == 4 && !strncasecmp(opt, "list", 4)) {
        rtype = CLUSTER_IS_ATOMIC(c) ? TYPE_BULK : TYPE_LINE;
        cb = cluster_client_list_resp;
    } else if ((opt_len == 4 && !strncasecmp(opt, "kill", 4)) ||
               (opt_len == 7 && !strncasecmp(opt, "setname", 7))) 
    {
        rtype = TYPE_LINE;
        cb = cluster_bool_resp;
    } else if (opt_len == 7 && !strncasecmp(opt, "getname", 7)) {
        rtype = CLUSTER_IS_ATOMIC(c) ? TYPE_BULK : TYPE_LINE;
        cb = cluster_bulk_resp;
    } else {
        php_error_docref(NULL TSRMLS_CC, E_WARNING,
            "Invalid CLIENT subcommand (LIST, KILL, GETNAME, and SETNAME are valid");
        RETURN_FALSE;
    }

    /* Construct the command */
    if (ZEND_NUM_ARGS() == 3) {
        cmd_len = redis_spprintf(NULL, NULL TSRMLS_CC, &cmd, "CLIENT", "ss",
            opt, opt_len, arg, arg_len);
    } else if(ZEND_NUM_ARGS() == 2) {
        cmd_len = redis_spprintf(NULL, NULL TSRMLS_CC, &cmd, "CLIENT", "s",
            opt, opt_len);
    } else {
        zend_wrong_param_count(TSRMLS_C);
        RETURN_FALSE;
    }

    /* Attempt to write our command */
    if (cluster_send_slot(c, slot, cmd, cmd_len, rtype TSRMLS_CC)<0) {
        zend_throw_exception(redis_cluster_exception_ce,
            "Unable to send CLIENT command to specific node", 0 TSRMLS_CC);
        efree(cmd);
        RETURN_FALSE;
    }

    /* Now enqueue or process response */
    if (CLUSTER_IS_ATOMIC(c)) {
        cb(INTERNAL_FUNCTION_PARAM_PASSTHRU, c, NULL);
    } else {
        void *ctx = NULL;
        CLUSTER_ENQUEUE_RESPONSE(c, slot, cb, ctx);
    }

    efree(cmd);
}

/* {{{ proto mixed RedisCluster::cluster(variant) */
PHP_METHOD(RedisCluster, cluster) {
    cluster_raw_cmd(INTERNAL_FUNCTION_PARAM_PASSTHRU, "CLUSTER", 
        sizeof("CLUSTER")-1);
}
/* }}} */

/* }}} */

/* {{{ proto mixed RedisCluster::config(string key, ...) 
 *     proto mixed RedisCluster::config(array host_port, ...) */
PHP_METHOD(RedisCluster, config) {
    cluster_raw_cmd(INTERNAL_FUNCTION_PARAM_PASSTHRU, "CONFIG",
        sizeof("CONFIG")-1);
}
/* }}} */

/* {{{ proto mixed RedisCluster::pubsub(string key, ...)
 *     proto mixed RedisCluster::pubsub(array host_port, ...) */
PHP_METHOD(RedisCluster, pubsub) {
    cluster_raw_cmd(INTERNAL_FUNCTION_PARAM_PASSTHRU, "PUBSUB",
        sizeof("PUBSUB")-1);
}
/* }}} */

/* {{{ proto mixed RedisCluster::script(string key, ...) 
 *     proto mixed RedisCluster::script(array host_port, ...) */
PHP_METHOD(RedisCluster, script) {
    cluster_raw_cmd(INTERNAL_FUNCTION_PARAM_PASSTHRU, "SCRIPT",
        sizeof("SCRIPT")-1);
}
/* }}} */

/* {{{ proto mixed RedisCluster::slowlog(string key, ...)
 *     proto mixed RedisCluster::slowlog(array host_port, ...) */
PHP_METHOD(RedisCluster, slowlog) {
    cluster_raw_cmd(INTERNAL_FUNCTION_PARAM_PASSTHRU, "SLOWLOG",
        sizeof("SLOWLOG")-1);
}
/* }}} */

/* {{{ proto int RedisCluster::geoadd(string key, float long float lat string mem, ...) */
PHP_METHOD(RedisCluster, geoadd) {
    CLUSTER_PROCESS_KW_CMD("GEOADD", redis_key_varval_cmd, cluster_long_resp, 0);
}

/* {{{ proto array RedisCluster::geohash(string key, string mem1, [string mem2...]) */
PHP_METHOD(RedisCluster, geohash) {
    CLUSTER_PROCESS_KW_CMD("GEOHASH", redis_key_varval_cmd, cluster_mbulk_raw_resp, 1);
}

/* {{{ proto array RedisCluster::geopos(string key, string mem1, [string mem2...]) */
PHP_METHOD(RedisCluster, geopos) {
    CLUSTER_PROCESS_KW_CMD("GEOPOS", redis_key_varval_cmd, cluster_variant_resp, 1);
}

/* {{{ proto array RedisCluster::geodist(string key, string mem1, string mem2 [string unit]) */
PHP_METHOD(RedisCluster, geodist) {
    CLUSTER_PROCESS_CMD(geodist, cluster_dbl_resp, 1);
}

/* {{{ proto array RedisCluster::georadius() }}} */
PHP_METHOD(RedisCluster, georadius) {
    CLUSTER_PROCESS_CMD(georadius, cluster_variant_resp, 1);
}

/* {{{ proto array RedisCluster::georadiusbymember() }}} */
PHP_METHOD(RedisCluster, georadiusbymember) {
    CLUSTER_PROCESS_CMD(georadiusbymember, cluster_variant_resp, 1)
}

/* {{{ proto array RedisCluster::role(string key)
 *     proto array RedisCluster::role(array host_port) */
PHP_METHOD(RedisCluster, role) {
    cluster_empty_node_cmd(INTERNAL_FUNCTION_PARAM_PASSTHRU, "ROLE",
        TYPE_MULTIBULK, cluster_variant_resp);
}

/* {{{ proto array RedisCluster::time(string key)
 *     proto array RedisCluster::time(array host_port) */
PHP_METHOD(RedisCluster, time) {
    cluster_empty_node_cmd(INTERNAL_FUNCTION_PARAM_PASSTHRU, "TIME",
        TYPE_MULTIBULK, cluster_variant_resp);
}
/* }}} */

/* {{{ proto string RedisCluster::randomkey(string key)
 *     proto string RedisCluster::randomkey(array host_port) */
PHP_METHOD(RedisCluster, randomkey) {
    cluster_empty_node_cmd(INTERNAL_FUNCTION_PARAM_PASSTHRU, "RANDOMKEY",
        TYPE_BULK, cluster_bulk_resp);
}
/* }}} */

/* {{{ proto bool RedisCluster::ping(string key)
 *     proto bool RedisCluster::ping(array host_port) */
PHP_METHOD(RedisCluster, ping) {
    cluster_empty_node_cmd(INTERNAL_FUNCTION_PARAM_PASSTHRU, "PING",
        TYPE_LINE, cluster_ping_resp);
}
/* }}} */

/* {{{ proto string RedisCluster::echo(string key, string msg)
 *     proto string RedisCluster::echo(array host_port, string msg) */
PHP_METHOD(RedisCluster, echo) {
    redisCluster *c = GET_CONTEXT();
    REDIS_REPLY_TYPE rtype;
    zval *z_arg;
    char *cmd, *msg;
    int cmd_len;
    strlen_t msg_len;
    short slot;
    
    if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "zs", &z_arg, &msg,
                             &msg_len)==FAILURE)
    {
        RETURN_FALSE;
    }

    /* Treat this as a readonly command */
    c->readonly = CLUSTER_IS_ATOMIC(c);

    /* Grab slot either by key or host/port */
    slot = cluster_cmd_get_slot(c, z_arg TSRMLS_CC);
    if(slot<0) {
        RETURN_FALSE;
    }

    /* Construct our command */
    cmd_len = redis_spprintf(NULL, NULL TSRMLS_CC, &cmd, "ECHO", "s", msg, msg_len);

    /* Send it off */
    rtype = CLUSTER_IS_ATOMIC(c) ? TYPE_BULK : TYPE_LINE;
    if(cluster_send_slot(c,slot,cmd,cmd_len,rtype TSRMLS_CC)<0) {
        zend_throw_exception(redis_cluster_exception_ce,
            "Unable to send commnad at the specificed node", 0 TSRMLS_CC);
        efree(cmd);
        RETURN_FALSE;
    }

    /* Process bulk response */
    if (CLUSTER_IS_ATOMIC(c)) {
        cluster_bulk_resp(INTERNAL_FUNCTION_PARAM_PASSTHRU, c, NULL);
    } else {
        void *ctx = NULL;
        CLUSTER_ENQUEUE_RESPONSE(c, slot, cluster_bulk_resp, ctx);
    } 

    efree(cmd);
}
/* }}} */

/* {{{ proto mixed RedisCluster::rawcommand(string $key, string $cmd, [ $argv1 .. $argvN])
 *     proto mixed RedisCluster::rawcommand(array $host_port, string $cmd, [ $argv1 .. $argvN]) */
PHP_METHOD(RedisCluster, rawcommand) {
    REDIS_REPLY_TYPE rtype;
    int argc = ZEND_NUM_ARGS(), cmd_len;
    redisCluster *c = GET_CONTEXT();
    char *cmd = NULL;
    zval *z_args;
    short slot;

    /* Sanity check on our arguments */
    if (argc < 2) {
        php_error_docref(NULL TSRMLS_CC, E_WARNING,
            "You must pass at least node information as well as at least a command.");
        RETURN_FALSE;
    }
    z_args = emalloc(argc * sizeof(zval));
    if (zend_get_parameters_array(ht, argc, z_args) == FAILURE) {
        php_error_docref(NULL TSRMLS_CC, E_WARNING,
            "Internal PHP error parsing method parameters.");
        efree(z_args);
        RETURN_FALSE;
    } else if (redis_build_raw_cmd(&z_args[1], argc-1, &cmd, &cmd_len TSRMLS_CC) ||
               (slot = cluster_cmd_get_slot(c, &z_args[0] TSRMLS_CC))<0) 
    {
        if (cmd) efree(cmd);
        efree(z_args);
        RETURN_FALSE;
    }

    /* Free argument array */
    efree(z_args);

    /* Direct the command */
    rtype = CLUSTER_IS_ATOMIC(c) ? TYPE_EOF : TYPE_LINE;
    if (cluster_send_slot(c,slot,cmd,cmd_len,rtype TSRMLS_CC)<0) {
        zend_throw_exception(redis_cluster_exception_ce,
            "Unable to send command to the specified node", 0 TSRMLS_CC);
        efree(cmd);
        RETURN_FALSE;
    }

    /* Process variant response */
    if (CLUSTER_IS_ATOMIC(c)) {
        cluster_variant_resp(INTERNAL_FUNCTION_PARAM_PASSTHRU, c, NULL);
    } else {
        void *ctx = NULL;
        CLUSTER_ENQUEUE_RESPONSE(c, slot, cluster_variant_resp, ctx);
    }

    efree(cmd);
}
/* }}} */

/* {{{ proto array RedisCluster::command()
 *     proto array RedisCluster::command('INFO', string cmd)
 *     proto array RedisCluster::command('GETKEYS', array cmd_args) */
PHP_METHOD(RedisCluster, command) {
    CLUSTER_PROCESS_CMD(command, cluster_variant_resp, 0);
}

/* vim: set tabstop=4 softtabstop=4 expandtab shiftwidth=4: */
