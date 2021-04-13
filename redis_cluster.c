/*
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

/* Handlers for RedisCluster */
zend_object_handlers RedisCluster_handlers;

ZEND_BEGIN_ARG_INFO_EX(arginfo_ctor, 0, 0, 1)
    ZEND_ARG_INFO(0, name)
    ZEND_ARG_ARRAY_INFO(0, seeds, 0)
    ZEND_ARG_INFO(0, timeout)
    ZEND_ARG_INFO(0, read_timeout)
    ZEND_ARG_INFO(0, persistent)
    ZEND_ARG_INFO(0, auth)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_del, 0, 0, 1)
    ZEND_ARG_INFO(0, key)
    ZEND_ARG_VARIADIC_INFO(0, other_keys)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_mget, 0, 0, 1)
    ZEND_ARG_ARRAY_INFO(0, keys, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_keys, 0, 0, 1)
    ZEND_ARG_INFO(0, pattern)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_key_or_address, 0, 0, 1)
    ZEND_ARG_INFO(0, key_or_address)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_key_or_address_variadic, 0, 0, 1)
    ZEND_ARG_INFO(0, key_or_address)
    ZEND_ARG_INFO(0, arg)
    ZEND_ARG_VARIADIC_INFO(0, other_args)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_info, 0, 0, 1)
    ZEND_ARG_INFO(0, key_or_address)
    ZEND_ARG_INFO(0, option)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_flush, 0, 0, 1)
    ZEND_ARG_INFO(0, key_or_address)
    ZEND_ARG_INFO(0, async)
ZEND_END_ARG_INFO()

/* Argument info for HSCAN, SSCAN, HSCAN */
ZEND_BEGIN_ARG_INFO_EX(arginfo_kscan_cl, 0, 0, 2)
    ZEND_ARG_INFO(0, str_key)
    ZEND_ARG_INFO(1, i_iterator)
    ZEND_ARG_INFO(0, str_pattern)
    ZEND_ARG_INFO(0, i_count)
ZEND_END_ARG_INFO()

/* Argument info for SCAN */
ZEND_BEGIN_ARG_INFO_EX(arginfo_scan_cl, 0, 0, 2)
    ZEND_ARG_INFO(1, i_iterator)
    ZEND_ARG_INFO(0, str_node)
    ZEND_ARG_INFO(0, str_pattern)
    ZEND_ARG_INFO(0, i_count)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_acl_cl, 0, 0, 2)
    ZEND_ARG_INFO(0, key_or_address)
    ZEND_ARG_INFO(0, subcmd)
    ZEND_ARG_VARIADIC_INFO(0, args)
ZEND_END_ARG_INFO()

/* Function table */
zend_function_entry redis_cluster_functions[] = {
    PHP_ME(RedisCluster, __construct, arginfo_ctor, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, _masters, arginfo_void, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, _prefix, arginfo_key, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, _redir, arginfo_void, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, _serialize, arginfo_value, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, _unserialize, arginfo_value, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, acl, arginfo_acl_cl, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, append, arginfo_key_value, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, bgrewriteaof, arginfo_key_or_address, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, bgsave, arginfo_key_or_address, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, bitcount, arginfo_key, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, bitop, arginfo_bitop, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, bitpos, arginfo_bitpos, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, blpop, arginfo_blrpop, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, brpop, arginfo_blrpop, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, brpoplpush, arginfo_brpoplpush, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, clearlasterror, arginfo_void, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, bzpopmax, arginfo_blrpop, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, bzpopmin, arginfo_blrpop, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, client, arginfo_key_or_address_variadic, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, close, arginfo_void, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, cluster, arginfo_key_or_address_variadic, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, command, arginfo_command, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, config, arginfo_key_or_address_variadic, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, dbsize, arginfo_key_or_address, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, decr, arginfo_key, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, decrby, arginfo_key_value, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, del, arginfo_del, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, discard, arginfo_void, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, dump, arginfo_key, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, echo, arginfo_echo, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, eval, arginfo_eval, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, evalsha, arginfo_evalsha, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, exec, arginfo_void, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, exists, arginfo_key, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, expire, arginfo_expire, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, expireat, arginfo_key_timestamp, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, flushall, arginfo_flush, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, flushdb, arginfo_flush, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, geoadd, arginfo_geoadd, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, geodist, arginfo_geodist, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, geohash, arginfo_key_members, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, geopos, arginfo_key_members, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, georadius, arginfo_georadius, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, georadius_ro, arginfo_georadius, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, georadiusbymember, arginfo_georadiusbymember, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, georadiusbymember_ro, arginfo_georadiusbymember, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, get, arginfo_key, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, getbit, arginfo_key_offset, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, getlasterror, arginfo_void, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, getmode, arginfo_void, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, getoption, arginfo_getoption, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, getrange, arginfo_key_start_end, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, getset, arginfo_key_value, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, hdel, arginfo_key_members, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, hexists, arginfo_key_member, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, hget, arginfo_key_member, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, hgetall, arginfo_key, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, hincrby, arginfo_key_member_value, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, hincrbyfloat, arginfo_key_member_value, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, hkeys, arginfo_key, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, hlen, arginfo_key, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, hmget, arginfo_hmget, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, hmset, arginfo_hmset, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, hscan, arginfo_kscan_cl, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, hset, arginfo_key_member_value, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, hsetnx, arginfo_key_member_value, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, hstrlen, arginfo_key_member, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, hvals, arginfo_key, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, incr, arginfo_key, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, incrby, arginfo_key_value, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, incrbyfloat, arginfo_key_value, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, info, arginfo_info, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, keys, arginfo_keys, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, lastsave, arginfo_key_or_address, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, lget, arginfo_lindex, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, lindex, arginfo_lindex, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, linsert, arginfo_linsert, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, llen, arginfo_key, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, lpop, arginfo_key, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, lpush, arginfo_key_value, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, lpushx, arginfo_key_value, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, lrange, arginfo_key_start_end, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, lrem, arginfo_key_value, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, lset, arginfo_lset, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, ltrim, arginfo_ltrim, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, mget, arginfo_mget, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, mset, arginfo_pairs, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, msetnx, arginfo_pairs, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, multi, arginfo_void, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, object, arginfo_object, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, persist, arginfo_key, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, pexpire, arginfo_key_timestamp, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, pexpireat, arginfo_key_timestamp, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, pfadd, arginfo_pfadd, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, pfcount, arginfo_key, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, pfmerge, arginfo_pfmerge, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, ping, arginfo_key_or_address, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, psetex, arginfo_key_expire_value, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, psubscribe, arginfo_psubscribe, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, pttl, arginfo_key, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, publish, arginfo_publish, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, pubsub, arginfo_key_or_address_variadic, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, punsubscribe, arginfo_punsubscribe, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, randomkey, arginfo_key_or_address, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, rawcommand, arginfo_rawcommand, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, rename, arginfo_key_newkey, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, renamenx, arginfo_key_newkey, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, restore, arginfo_restore, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, role, arginfo_void, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, rpop, arginfo_key, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, rpoplpush, arginfo_rpoplpush, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, rpush, arginfo_key_value, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, rpushx, arginfo_key_value, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, sadd, arginfo_key_value, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, saddarray, arginfo_sadd_array, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, save, arginfo_key_or_address, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, scan, arginfo_scan_cl, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, scard, arginfo_key, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, script, arginfo_key_or_address_variadic, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, sdiff, arginfo_nkeys, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, sdiffstore, arginfo_dst_nkeys, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, set, arginfo_set, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, setbit, arginfo_key_offset_value, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, setex, arginfo_key_expire_value, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, setnx, arginfo_key_value, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, setoption, arginfo_setoption, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, setrange, arginfo_key_offset_value, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, sinter, arginfo_nkeys, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, sinterstore, arginfo_dst_nkeys, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, sismember, arginfo_key_value, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, slowlog, arginfo_key_or_address_variadic, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, smembers, arginfo_key, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, smove, arginfo_smove, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, sort, arginfo_sort, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, spop, arginfo_key, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, srandmember, arginfo_srand_member, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, srem, arginfo_key_value, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, sscan, arginfo_kscan_cl, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, strlen, arginfo_key, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, subscribe, arginfo_subscribe, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, sunion, arginfo_nkeys, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, sunionstore, arginfo_dst_nkeys, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, time, arginfo_key_or_address, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, ttl, arginfo_key, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, type, arginfo_key, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, unsubscribe, arginfo_unsubscribe, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, unlink, arginfo_del, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, unwatch, arginfo_void, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, watch, arginfo_watch, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, xack, arginfo_xack, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, xadd, arginfo_xadd, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, xclaim, arginfo_xclaim, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, xdel, arginfo_xdel, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, xgroup, arginfo_xgroup, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, xinfo, arginfo_xinfo, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, xlen, arginfo_key, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, xpending, arginfo_xpending, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, xrange, arginfo_xrange, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, xread, arginfo_xread, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, xreadgroup, arginfo_xreadgroup, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, xrevrange, arginfo_xrange, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, xtrim, arginfo_xtrim, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, zadd, arginfo_zadd, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, zcard, arginfo_key, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, zcount, arginfo_key_min_max, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, zincrby, arginfo_zincrby, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, zinterstore, arginfo_zstore, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, zlexcount, arginfo_key_min_max, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, zpopmax, arginfo_key, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, zpopmin, arginfo_key, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, zrange, arginfo_zrange, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, zrangebylex, arginfo_zrangebylex, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, zrangebyscore, arginfo_zrangebyscore, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, zrank, arginfo_key_member, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, zrem, arginfo_key_members, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, zremrangebylex, arginfo_key_min_max, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, zremrangebyrank, arginfo_key_min_max, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, zremrangebyscore, arginfo_key_min_max, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, zrevrange, arginfo_zrange, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, zrevrangebylex, arginfo_zrangebylex, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, zrevrangebyscore, arginfo_zrangebyscore, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, zrevrank, arginfo_key_member, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, zscan, arginfo_kscan_cl, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, zscore, arginfo_key_member, ZEND_ACC_PUBLIC)
    PHP_ME(RedisCluster, zunionstore, arginfo_zstore, ZEND_ACC_PUBLIC)
    PHP_FE_END
};

/* Our context seeds will be a hash table with RedisSock* pointers */
static void ht_free_seed(zval *data) {
    RedisSock *redis_sock = *(RedisSock**)data;
    if (redis_sock) redis_free_socket(redis_sock);
}

/* Free redisClusterNode objects we've stored */
static void ht_free_node(zval *data) {
    redisClusterNode *node = *(redisClusterNode**)data;
    cluster_free_node(node);
}

/* Create redisCluster context */
zend_object * create_cluster_context(zend_class_entry *class_type) {
    redisCluster *cluster;

    // Allocate our actual struct
    cluster = ecalloc(1, sizeof(redisCluster) + zend_object_properties_size(class_type));

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
    zend_object_std_init(&cluster->std, class_type);

    object_properties_init(&cluster->std, class_type);
    memcpy(&RedisCluster_handlers, zend_get_std_object_handlers(), sizeof(RedisCluster_handlers));
    RedisCluster_handlers.offset = XtOffsetOf(redisCluster, std);
    RedisCluster_handlers.free_obj = free_cluster_context;

    cluster->std.handlers = &RedisCluster_handlers;

    return &cluster->std;
}

/* Free redisCluster context */
void free_cluster_context(zend_object *object) {
    redisCluster *cluster = PHPREDIS_GET_OBJECT(redisCluster, object);

    cluster_free(cluster, 0);
    zend_object_std_dtor(&cluster->std);
}

/* Take user provided seeds and return unique and valid ones */
/* Attempt to connect to a Redis cluster provided seeds and timeout options */
static void redis_cluster_init(redisCluster *c, HashTable *ht_seeds, double timeout,
                               double read_timeout, int persistent, zend_string *user,
                               zend_string *pass, zval *context)
{
    zend_string *hash = NULL, **seeds;
    redisCachedCluster *cc;
    uint32_t nseeds;
    char *err;

    /* Validate our arguments and get a sanitized seed array */
    seeds = cluster_validate_args(timeout, read_timeout, ht_seeds, &nseeds, &err);
    if (seeds == NULL) {
        CLUSTER_THROW_EXCEPTION(err, 0);
        return;
    }

    if (user && ZSTR_LEN(user))
        c->flags->user = zend_string_copy(user);
    if (pass && ZSTR_LEN(pass))
        c->flags->pass = zend_string_copy(pass);
    if (context) {
        redis_sock_set_stream_context(c->flags, context);
    }

    c->flags->timeout = timeout;
    c->flags->read_timeout = read_timeout;
    c->flags->persistent = persistent;
    c->waitms = timeout * 1000L;

    /* Attempt to load slots from cache if caching is enabled */
    if (CLUSTER_CACHING_ENABLED()) {
        /* Exit early if we can load from cache */
        hash = cluster_hash_seeds(seeds, nseeds);
        if ((cc = cluster_cache_load(hash))) {
            cluster_init_cache(c, cc);
            goto cleanup;
        }
    }

    /* Initialize seeds and attempt to map keyspace */
    cluster_init_seeds(c, seeds, nseeds);
    if (cluster_map_keyspace(c) == SUCCESS && hash)
        cluster_cache_store(hash, c->nodes);

cleanup:
    if (hash) zend_string_release(hash);
    free_seed_array(seeds, nseeds);
}


/* Attempt to load a named cluster configured in php.ini */
void redis_cluster_load(redisCluster *c, char *name, int name_len) {
    zval z_seeds, z_tmp, *z_value;
    zend_string *user = NULL, *pass = NULL;
    double timeout = 0, read_timeout = 0;
    int persistent = 0;
    char *iptr;
    HashTable *ht_seeds = NULL;

    /* Seeds */
    array_init(&z_seeds);
    if ((iptr = INI_STR("redis.clusters.seeds")) != NULL) {
        sapi_module.treat_data(PARSE_STRING, estrdup(iptr), &z_seeds);
    }
    if ((z_value = zend_hash_str_find(Z_ARRVAL(z_seeds), name, name_len)) != NULL) {
        ht_seeds = Z_ARRVAL_P(z_value);
    } else {
        zval_dtor(&z_seeds);
        CLUSTER_THROW_EXCEPTION("Couldn't find seeds for cluster", 0);
        return;
    }

    /* Connection timeout */
    if ((iptr = INI_STR("redis.clusters.timeout")) != NULL) {
        array_init(&z_tmp);
        sapi_module.treat_data(PARSE_STRING, estrdup(iptr), &z_tmp);
        redis_conf_double(Z_ARRVAL(z_tmp), name, name_len, &timeout);
        zval_dtor(&z_tmp);
    }

    /* Read timeout */
    if ((iptr = INI_STR("redis.clusters.read_timeout")) != NULL) {
        array_init(&z_tmp);
        sapi_module.treat_data(PARSE_STRING, estrdup(iptr), &z_tmp);
        redis_conf_double(Z_ARRVAL(z_tmp), name, name_len, &read_timeout);
        zval_dtor(&z_tmp);
    }

    /* Persistent connections */
    if ((iptr = INI_STR("redis.clusters.persistent")) != NULL) {
        array_init(&z_tmp);
        sapi_module.treat_data(PARSE_STRING, estrdup(iptr), &z_tmp);
        redis_conf_bool(Z_ARRVAL(z_tmp), name, name_len, &persistent);
        zval_dtor(&z_tmp);
    }

    if ((iptr = INI_STR("redis.clusters.auth"))) {
        array_init(&z_tmp);
        sapi_module.treat_data(PARSE_STRING, estrdup(iptr), &z_tmp);
        redis_conf_auth(Z_ARRVAL(z_tmp), name, name_len, &user, &pass);
        zval_dtor(&z_tmp);
    }

    /* Attempt to create/connect to the cluster */
    redis_cluster_init(c, ht_seeds, timeout, read_timeout, persistent, user, pass, NULL);

    /* Clean up */
    zval_dtor(&z_seeds);
    if (user) zend_string_release(user);
    if (pass) zend_string_release(pass);
}

/*
 * PHP Methods
 */

/* Create a RedisCluster Object */
PHP_METHOD(RedisCluster, __construct) {
    zval *object, *z_seeds = NULL, *z_auth = NULL, *context = NULL;
    zend_string *user = NULL, *pass = NULL;
    double timeout = 0.0, read_timeout = 0.0;
    size_t name_len;
    zend_bool persistent = 0;
    redisCluster *c = GET_CONTEXT();
    char *name;

    // Parse arguments
    if (zend_parse_method_parameters(ZEND_NUM_ARGS(), getThis(),
                                    "Os!|addbza!", &object, redis_cluster_ce, &name,
                                    &name_len, &z_seeds, &timeout, &read_timeout,
                                    &persistent, &z_auth, &context) == FAILURE)
    {
        RETURN_FALSE;
    }

    /* If we've got a string try to load from INI */
    if (ZEND_NUM_ARGS() < 2) {
        if (name_len == 0) { // Require a name
            CLUSTER_THROW_EXCEPTION("You must specify a name or pass seeds!", 0);
        }
        redis_cluster_load(c, name, name_len);
        return;
    }

    /* The normal case, loading from arguments */
    redis_extract_auth_info(z_auth, &user, &pass);
    redis_cluster_init(c, Z_ARRVAL_P(z_seeds), timeout, read_timeout,
                       persistent, user, pass, context);

    if (user) zend_string_release(user);
    if (pass) zend_string_release(pass);
}

/*
 * RedisCluster method implementation
 */

/* {{{ proto bool RedisCluster::close() */
PHP_METHOD(RedisCluster, close) {
    cluster_disconnect(GET_CONTEXT(), 1);
    RETURN_TRUE;
}

/* {{{ proto string RedisCluster::get(string key) */
PHP_METHOD(RedisCluster, get) {
    CLUSTER_PROCESS_KW_CMD("GET", redis_key_cmd, cluster_bulk_resp, 1);
}
/* }}} */

/* {{{ proto bool RedisCluster::set(string key, string value) */
PHP_METHOD(RedisCluster, set) {
    CLUSTER_PROCESS_CMD(set, cluster_set_resp, 0);
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
    if (cluster_send_command(c,slot,mc->cmd.c,mc->cmd.len) < 0 || c->err != NULL) {
        efree(ctx);
        return -1;
    }

    if (CLUSTER_IS_ATOMIC(c)) {
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
    size_t key_len;
    int key_free;
    short slot;

    char *val;
    size_t val_len;
    int val_free;
} clusterKeyValHT;

/* Helper to pull a key/value pair from a HashTable */
static int get_key_val_ht(redisCluster *c, HashTable *ht, HashPosition *ptr,
                          clusterKeyValHT *kv)
{
    zval *z_val;
    zend_ulong idx;

    // Grab the key, convert it to a string using provided kbuf buffer if it's
    // a LONG style key
    zend_string *zkey;
    switch (zend_hash_get_current_key_ex(ht, &zkey, &idx, ptr)) {
        case HASH_KEY_IS_STRING:
            kv->key_len = ZSTR_LEN(zkey);
            kv->key = ZSTR_VAL(zkey);
            break;
        case HASH_KEY_IS_LONG:
            kv->key_len = snprintf(kv->kbuf,sizeof(kv->kbuf),"%ld",(long)idx);
            kv->key     = kv->kbuf;
            break;
        default:
            CLUSTER_THROW_EXCEPTION("Internal Zend HashTable error", 0);
            return -1;
    }

    // Prefix our key if we need to, set the slot
    kv->key_free = redis_key_prefix(c->flags, &(kv->key), &(kv->key_len));
    kv->slot     = cluster_hash_key(kv->key, kv->key_len);

    // Now grab our value
    if ((z_val = zend_hash_get_current_data_ex(ht, ptr)) == NULL) {
        CLUSTER_THROW_EXCEPTION("Internal Zend HashTable error", 0);
        return -1;
    }

    // Serialize our value if required
    kv->val_free = redis_pack(c->flags,z_val,&(kv->val),&(kv->val_len));

    // Success
    return 0;
}

/* Helper to pull, prefix, and hash a key from a HashTable value */
static int get_key_ht(redisCluster *c, HashTable *ht, HashPosition *ptr,
                      clusterKeyValHT *kv)
{
    zval *z_key;

    if ((z_key = zend_hash_get_current_data_ex(ht, ptr)) == NULL) {
        // Shouldn't happen, but check anyway
        CLUSTER_THROW_EXCEPTION("Internal Zend HashTable error", 0);
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

/* Convenience handler for commands that take multiple keys such as
 * MGET, DEL, and UNLINK */
static int cluster_mkey_cmd(INTERNAL_FUNCTION_PARAMETERS, char *kw, int kw_len,
                            zval *z_ret, cluster_cb cb)
{
    redisCluster *c = GET_CONTEXT();
    clusterMultiCmd mc = {0};
    clusterKeyValHT kv;
    zval *z_args;
    HashTable *ht_arr;
    HashPosition ptr;
    int i = 1, argc = ZEND_NUM_ARGS(), ht_free = 0;
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
    if (get_key_ht(c, ht_arr, &ptr, &kv) < 0) {
        efree(z_args);
        return -1;
    }

    // Process our key and add it to the command
    cluster_multi_add(&mc, kv.key, kv.key_len);

    // Free key if we prefixed
    if (kv.key_free) efree(kv.key);

    // Move to the next key
    zend_hash_move_forward_ex(ht_arr, &ptr);

    // Iterate over keys 2...N
    slot = kv.slot;
    while (zend_hash_has_more_elements_ex(ht_arr, &ptr) ==SUCCESS) {
        if (get_key_ht(c, ht_arr, &ptr, &kv) < 0) {
            cluster_multi_free(&mc);
            if (ht_free) {
                zend_hash_destroy(ht_arr);
                efree(ht_arr);
            }
            efree(z_args);
            return -1;
        }

        // If the slots have changed, kick off the keys we've aggregated
        if (slot != kv.slot) {
            // Process this batch of MGET keys
            if (distcmd_resp_handler(INTERNAL_FUNCTION_PARAM_PASSTHRU, c, slot,
                                    &mc, z_ret, i == argc, cb) < 0)
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
        if (kv.key_free) efree(kv.key);

        // Update the last slot we encountered, and the key we're on
        slot = kv.slot;
        i++;

        zend_hash_move_forward_ex(ht_arr, &ptr);
    }
    efree(z_args);

    // If we've got straggler(s) process them
    if (mc.argc > 0) {
        if (distcmd_resp_handler(INTERNAL_FUNCTION_PARAM_PASSTHRU, c, slot,
                                &mc, z_ret, 1, cb) < 0)
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
    int i = 1, argc;
    short slot;

    // Parse our arguments
    if (zend_parse_parameters(ZEND_NUM_ARGS(), "a", &z_arr) == FAILURE) {
        return -1;
    }

    // No reason to send zero args
    ht_arr = Z_ARRVAL_P(z_arr);
    if ((argc = zend_hash_num_elements(ht_arr)) == 0) {
        return -1;
    }

    /* This is a write command */
    c->readonly = 0;

    // Set up our multi command handler
    CLUSTER_MULTI_INIT(mc, kw, kw_len);

    // Process the first key/value pair outside of our loop
    zend_hash_internal_pointer_reset_ex(ht_arr, &ptr);
    if (get_key_val_ht(c, ht_arr, &ptr, &kv) ==-1) return -1;
    zend_hash_move_forward_ex(ht_arr, &ptr);

    // Add this to our multi cmd, set slot, free key if we prefixed
    cluster_multi_add(&mc, kv.key, kv.key_len);
    cluster_multi_add(&mc, kv.val, kv.val_len);
    if (kv.key_free) efree(kv.key);
    if (kv.val_free) efree(kv.val);

    // While we've got more keys to set
    slot = kv.slot;
    while (zend_hash_has_more_elements_ex(ht_arr, &ptr) ==SUCCESS) {
        // Pull the next key/value pair
        if (get_key_val_ht(c, ht_arr, &ptr, &kv) ==-1) {
            return -1;
        }

        // If the slots have changed, process responses
        if (slot != kv.slot) {
            if (distcmd_resp_handler(INTERNAL_FUNCTION_PARAM_PASSTHRU, c,
                                    slot, &mc, z_ret, i == argc, cb) < 0)
            {
                cluster_multi_free(&mc);
                return -1;
            }
        }

        // Add this key and value to our command
        cluster_multi_add(&mc, kv.key, kv.key_len);
        cluster_multi_add(&mc, kv.val, kv.val_len);

        // Free our key and value if we need to
        if (kv.key_free) efree(kv.key);
        if (kv.val_free) efree(kv.val);

        // Update our slot, increment position
        slot = kv.slot;
        i++;

        // Move on
        zend_hash_move_forward_ex(ht_arr, &ptr);
    }

    // If we've got stragglers, process them too
    if (mc.argc > 0) {
        if (distcmd_resp_handler(INTERNAL_FUNCTION_PARAM_PASSTHRU, c, slot, &mc,
                                z_ret, 1, cb) < 0)
        {
            cluster_multi_free(&mc);
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

/* Generic passthru for DEL and UNLINK which act identically */
static void cluster_generic_delete(INTERNAL_FUNCTION_PARAMETERS,
                                   char *kw, int kw_len)
{
    zval *z_ret = emalloc(sizeof(*z_ret));

    // Initialize a LONG value to zero for our return
    ZVAL_LONG(z_ret, 0);

    // Parse args, process
    if (cluster_mkey_cmd(INTERNAL_FUNCTION_PARAM_PASSTHRU, kw, kw_len, z_ret,
                        cluster_del_resp) < 0)
    {
        efree(z_ret);
        RETURN_FALSE;
    }
}

/* {{{ proto array RedisCluster::del(string key1, string key2, ... keyN) */
PHP_METHOD(RedisCluster, del) {
    cluster_generic_delete(INTERNAL_FUNCTION_PARAM_PASSTHRU, "DEL", sizeof("DEL") - 1);
}

/* {{{ proto array RedisCluster::unlink(string key1, string key2, ... keyN) */
PHP_METHOD(RedisCluster, unlink) {
    cluster_generic_delete(INTERNAL_FUNCTION_PARAM_PASSTHRU, "UNLINK", sizeof("UNLINK") - 1);
}

/* {{{ proto array RedisCluster::mget(array keys) */
PHP_METHOD(RedisCluster, mget) {
    zval *z_ret = emalloc(sizeof(*z_ret));

    array_init(z_ret);

    // Parse args, process
    if (cluster_mkey_cmd(INTERNAL_FUNCTION_PARAM_PASSTHRU, "MGET",
                        sizeof("MGET")-1, z_ret, cluster_mbulk_mget_resp) < 0)
    {
        zval_dtor(z_ret);
        efree(z_ret);
        RETURN_FALSE;
    }
}

/* {{{ proto bool RedisCluster::mset(array keyvalues) */
PHP_METHOD(RedisCluster, mset) {
    zval *z_ret = emalloc(sizeof(*z_ret));

    ZVAL_TRUE(z_ret);

    // Parse args and process.  If we get a failure, free zval and return FALSE.
    if (cluster_mset_cmd(INTERNAL_FUNCTION_PARAM_PASSTHRU, "MSET",
                        sizeof("MSET")-1, z_ret, cluster_mset_resp) ==-1)
    {
        efree(z_ret);
        RETURN_FALSE;
    }
}

/* {{{ proto array RedisCluster::msetnx(array keyvalues) */
PHP_METHOD(RedisCluster, msetnx) {
    zval *z_ret = emalloc(sizeof(*z_ret));

    array_init(z_ret);

    // Parse args and process.  If we get a failure, free mem and return FALSE
    if (cluster_mset_cmd(INTERNAL_FUNCTION_PARAM_PASSTHRU, "MSETNX",
                         sizeof("MSETNX")-1, z_ret, cluster_msetnx_resp) ==-1)
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
    CLUSTER_PROCESS_CMD(exists, cluster_long_resp, 1);
}
/* }}} */

/* {{{ proto array Redis::keys(string pattern) */
PHP_METHOD(RedisCluster, keys) {
    redisCluster *c = GET_CONTEXT();
    redisClusterNode *node;
    size_t pat_len;
    char *pat, *cmd;
    clusterReply *resp;
    int i, cmd_len;

    if (zend_parse_parameters(ZEND_NUM_ARGS(), "s", &pat, &pat_len)
                             == FAILURE)
    {
        RETURN_FALSE;
    }

    /* Prefix and then build our command */
    cmd_len = redis_spprintf(c->flags, NULL, &cmd, "KEYS", "k", pat, pat_len);

    array_init(return_value);

    /* Treat as readonly */
    c->readonly = CLUSTER_IS_ATOMIC(c);

    /* Iterate over our known nodes */
    ZEND_HASH_FOREACH_PTR(c->nodes, node) {
        if (node == NULL) continue;
        if (cluster_send_slot(c, node->slot, cmd, cmd_len, TYPE_MULTIBULK
                            ) < 0)
        {
            php_error_docref(0, E_ERROR, "Can't send KEYS to %s:%d",
                ZSTR_VAL(node->sock->host), node->sock->port);
            zval_dtor(return_value);
            efree(cmd);
            RETURN_FALSE;
        }

        /* Ensure we can get a response */
        resp = cluster_read_resp(c, 0);
        if (!resp) {
            php_error_docref(0, E_WARNING,
                "Can't read response from %s:%d", ZSTR_VAL(node->sock->host),
                node->sock->port);
            continue;
        }

        /* Iterate keys, adding to our big array */
        for(i = 0; i < resp->elements; i++) {
            /* Skip non bulk responses, they should all be bulk */
            if (resp->element[i]->type != TYPE_BULK) {
                continue;
            }

            add_next_index_stringl(return_value, resp->element[i]->str,
                resp->element[i]->len);
        }

        /* Free response, don't free data */
        cluster_free_reply(resp, 1);
    } ZEND_HASH_FOREACH_END();

    efree(cmd);
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

    if (redis_srandmember_cmd(INTERNAL_FUNCTION_PARAM_PASSTHRU, c->flags,
                             &cmd, &cmd_len, &slot, NULL, &have_count)
                             == FAILURE)
    {
        RETURN_FALSE;
    }

    if (cluster_send_command(c,slot,cmd,cmd_len) < 0 || c->err != NULL) {
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
    CLUSTER_PROCESS_KW_CMD("BLPOP", redis_blocking_pop_cmd, cluster_mbulk_resp, 0);
}
/* }}} */

/* {{{ proto array RedisCluster::brpop(string key1, ... keyN, long timeout */
PHP_METHOD(RedisCluster, brpop) {
    CLUSTER_PROCESS_KW_CMD("BRPOP", redis_blocking_pop_cmd, cluster_mbulk_resp, 0);
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
    CLUSTER_PROCESS_KW_CMD("SADD", redis_key_val_arr_cmd, cluster_long_resp, 0);
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
    int withscores = 0;

    if (fun(INTERNAL_FUNCTION_PARAM_PASSTHRU, c->flags, kw, &cmd, &cmd_len,
           &withscores, &slot, NULL) == FAILURE)
    {
        efree(cmd);
        RETURN_FALSE;
    }

    if (cluster_send_command(c,slot,cmd,cmd_len) < 0 || c->err != NULL) {
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
 *     array RedisCluster::zrange(string k, long s, long e, bool score = 0) */
PHP_METHOD(RedisCluster, zrange) {
    generic_zrange_cmd(INTERNAL_FUNCTION_PARAM_PASSTHRU, "ZRANGE",
        redis_zrange_cmd);
}
/* }}} */

/* {{{ proto
 *     array RedisCluster::zrevrange(string k,long s,long e,bool scores = 0) */
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
    CLUSTER_PROCESS_KW_CMD("ZUNIONSTORE", redis_zinterunionstore_cmd, cluster_long_resp, 0);
}
/* }}} */

/* {{{ proto RedisCluster::zinterstore(string dst, array keys, [array weights,
 *                                     string agg]) */
PHP_METHOD(RedisCluster, zinterstore) {
    CLUSTER_PROCESS_KW_CMD("ZINTERSTORE", redis_zinterunionstore_cmd, cluster_long_resp, 0);
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

/* {{{ proto array RedisCluster::zpopmax(string key) */
PHP_METHOD(RedisCluster, zpopmax) {
    if (ZEND_NUM_ARGS() == 1) {
        CLUSTER_PROCESS_KW_CMD("ZPOPMAX", redis_key_cmd, cluster_mbulk_zipdbl_resp, 0);
    } else if (ZEND_NUM_ARGS() == 2) {
        CLUSTER_PROCESS_KW_CMD("ZPOPMAX", redis_key_long_cmd, cluster_mbulk_zipdbl_resp, 0);
    } else {
        ZEND_WRONG_PARAM_COUNT();
    }
}
/* }}} */

/* {{{ proto array RedisCluster::zpopmin(string key) */
PHP_METHOD(RedisCluster, zpopmin) {
    if (ZEND_NUM_ARGS() == 1) {
        CLUSTER_PROCESS_KW_CMD("ZPOPMIN", redis_key_cmd, cluster_mbulk_zipdbl_resp, 0);
    } else if (ZEND_NUM_ARGS() == 2) {
        CLUSTER_PROCESS_KW_CMD("ZPOPMIN", redis_key_long_cmd, cluster_mbulk_zipdbl_resp, 0);
    } else {
        ZEND_WRONG_PARAM_COUNT();
    }
}
/* }}} */

/* {{{ proto array RedisCluster::bzPopMin(Array keys [, timeout]) }}} */
PHP_METHOD(RedisCluster, bzpopmax) {
    CLUSTER_PROCESS_KW_CMD("BZPOPMAX", redis_blocking_pop_cmd, cluster_mbulk_resp, 0);
}

/* {{{ proto array RedisCluster::bzPopMax(Array keys [, timeout]) }}} */
PHP_METHOD(RedisCluster, bzpopmin) {
    CLUSTER_PROCESS_KW_CMD("BZPOPMIN", redis_blocking_pop_cmd, cluster_mbulk_resp, 0);
}

/* {{{ proto RedisCluster::sort(string key, array options) */
PHP_METHOD(RedisCluster, sort) {
    redisCluster *c = GET_CONTEXT();
    char *cmd; int cmd_len, have_store; short slot;

    if (redis_sort_cmd(INTERNAL_FUNCTION_PARAM_PASSTHRU, c->flags, &have_store,
                      &cmd, &cmd_len, &slot, NULL) == FAILURE)
    {
        RETURN_FALSE;
    }

    if (cluster_send_command(c,slot,cmd,cmd_len) < 0 || c->err != NULL) {
        efree(cmd);
        RETURN_FALSE;
    }

    efree(cmd);

    // Response type differs based on presence of STORE argument
    if (!have_store) {
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

    if (redis_object_cmd(INTERNAL_FUNCTION_PARAM_PASSTHRU, c->flags, &rtype,
                        &cmd, &cmd_len, &slot, NULL) == FAILURE)
    {
        RETURN_FALSE;
    }

     if (cluster_send_command(c,slot,cmd,cmd_len) < 0 || c->err != NULL) {
        efree(cmd);
        RETURN_FALSE;
    }

    efree(cmd);

    // Use the correct response type
    if (rtype == TYPE_INT) {
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
    if (c->subscribed_slot == -1) {
        php_error_docref(0, E_WARNING,
            "You can't unsubscribe outside of a subscribe loop");
        RETURN_FALSE;
    }

    // Call directly because we're going to set the slot manually
    if (redis_unsubscribe_cmd(INTERNAL_FUNCTION_PARAM_PASSTHRU, c->flags, kw,
                             &cmd, &cmd_len, &slot, &ctx)
                             == FAILURE)
    {
        RETURN_FALSE;
    }

    // This has to operate on our subscribe slot
    if (cluster_send_slot(c, c->subscribed_slot, cmd, cmd_len, TYPE_MULTIBULK
                        ) == FAILURE)
    {
        CLUSTER_THROW_EXCEPTION("Failed to UNSUBSCRIBE within our subscribe loop!", 0);
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
    CLUSTER_PROCESS_KW_CMD("EVAL", redis_eval_cmd, cluster_variant_raw_resp, 0);
}
/* }}} */

/* {{{ proto mixed RedisCluster::evalsha(string sha, [array args, int numkeys]) */
PHP_METHOD(RedisCluster, evalsha) {
    CLUSTER_PROCESS_KW_CMD("EVALSHA", redis_eval_cmd, cluster_variant_raw_resp, 0);
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

    if (c->err) {
        RETURN_STRINGL(ZSTR_VAL(c->err), ZSTR_LEN(c->err));
    }
    RETURN_NULL();
}
/* }}} */

/* {{{ proto bool RedisCluster::clearlasterror() */
PHP_METHOD(RedisCluster, clearlasterror) {
    redisCluster *c = GET_CONTEXT();

    if (c->err) {
        zend_string_release(c->err);
        c->err = NULL;
    }

    RETURN_TRUE;
}
/* }}} */

/* {{{ proto long RedisCluster::getOption(long option */
PHP_METHOD(RedisCluster, getoption) {
    redisCluster *c = GET_CONTEXT();
    redis_getoption_handler(INTERNAL_FUNCTION_PARAM_PASSTHRU, c->flags, c);
}
/* }}} */

/* {{{ proto bool RedisCluster::setOption(long option, mixed value) */
PHP_METHOD(RedisCluster, setoption) {
    redisCluster *c = GET_CONTEXT();
    redis_setoption_handler(INTERNAL_FUNCTION_PARAM_PASSTHRU, c->flags, c);
}
/* }}} */

/* {{{ proto string RedisCluster::_prefix(string key) */
PHP_METHOD(RedisCluster, _prefix) {
    redisCluster *c = GET_CONTEXT();
    redis_prefix_handler(INTERNAL_FUNCTION_PARAM_PASSTHRU, c->flags);
}
/* }}} */

/* {{{ proto string RedisCluster::_serialize(mixed val) */
PHP_METHOD(RedisCluster, _serialize) {
    redisCluster *c = GET_CONTEXT();
    redis_serialize_handler(INTERNAL_FUNCTION_PARAM_PASSTHRU, c->flags);
}
/* }}} */

/* {{{ proto mixed RedisCluster::_unserialize(string val) */
PHP_METHOD(RedisCluster, _unserialize) {
    redisCluster *c = GET_CONTEXT();
    redis_unserialize_handler(INTERNAL_FUNCTION_PARAM_PASSTHRU,
        c->flags, redis_cluster_exception_ce);
}
/* }}} */

/* {{{ proto array RedisCluster::_masters() */
PHP_METHOD(RedisCluster, _masters) {
    redisCluster *c = GET_CONTEXT();
    redisClusterNode *node;

    array_init(return_value);

    ZEND_HASH_FOREACH_PTR(c->nodes, node) {
        if (node == NULL) break;

        zval z_sub;

        array_init(&z_sub);

        add_next_index_stringl(&z_sub, ZSTR_VAL(node->sock->host), ZSTR_LEN(node->sock->host));
        add_next_index_long(&z_sub, node->sock->port);
        add_next_index_zval(return_value, &z_sub);
    } ZEND_HASH_FOREACH_END();
}

PHP_METHOD(RedisCluster, _redir) {
    redisCluster *c = GET_CONTEXT();
    char buf[255];
    size_t len;

    len = snprintf(buf, sizeof(buf), "%s:%d", c->redir_host, c->redir_port);
    if (*c->redir_host && c->redir_host_len) {
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

    if (c->flags->mode == MULTI) {
        php_error_docref(NULL, E_WARNING,
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
    if (c->flags->mode == MULTI) {
        php_error_docref(NULL, E_WARNING,
            "WATCH command not allowed in MULTI mode");
        RETURN_FALSE;
    }

    // Don't need to process zero arguments
    if (!argc) RETURN_FALSE;

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
    for(i = 0 ; i < argc; i++) {
        // We'll need the key as a string
        zstr = zval_get_string(&z_args[i]);

        // Add this key to our distribution handler
        if (cluster_dist_add_key(c, ht_dist, ZSTR_VAL(zstr), ZSTR_LEN(zstr), NULL) == FAILURE) {
            CLUSTER_THROW_EXCEPTION("Can't issue WATCH command as the keyspace isn't fully mapped", 0);
            zend_string_release(zstr);
            RETURN_FALSE;
        }
        zend_string_release(zstr);
    }

    // Iterate over each node we'll be sending commands to
    ZEND_HASH_FOREACH_PTR(ht_dist, dl) {
        // Grab the clusterDistList pointer itself
        if (dl == NULL) {
            CLUSTER_THROW_EXCEPTION("Internal error in a PHP HashTable", 0);
            cluster_dist_free(ht_dist);
            efree(z_args);
            efree(cmd.c);
            RETURN_FALSE;
        } else if (zend_hash_get_current_key(ht_dist, NULL, &slot) != HASH_KEY_IS_LONG) {
            break;
        }

        // Construct our watch command for this node
        redis_cmd_init_sstr(&cmd, dl->len, "WATCH", sizeof("WATCH")-1);
        for (i = 0; i < dl->len; i++) {
            redis_cmd_append_sstr(&cmd, dl->entry[i].key,
                dl->entry[i].key_len);
        }

        // If we get a failure from this, we have to abort
        if (cluster_send_command(c,(short)slot,cmd.c,cmd.len) ==-1) {
            RETURN_FALSE;
        }

        // This node is watching
        SLOT_SOCK(c, (short)slot)->watching = 1;

        // Zero out our command buffer
        cmd.len = 0;
    } ZEND_HASH_FOREACH_END();

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
    for(slot = 0; slot < REDIS_CLUSTER_SLOTS; slot++) {
        if (c->master[slot] && SLOT_SOCK(c,slot)->watching) {
            if (cluster_send_slot(c, slot, RESP_UNWATCH_CMD,
                                 sizeof(RESP_UNWATCH_CMD)-1,
                                 TYPE_LINE) ==-1)
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
    if (CLUSTER_IS_ATOMIC(c)) {
        php_error_docref(NULL, E_WARNING, "RedisCluster is not in MULTI mode");
        RETURN_FALSE;
    }

    // First pass, send EXEC and abort on failure
    fi = c->multi_head;
    while (fi) {
        if (SLOT_SOCK(c, fi->slot)->mode == MULTI) {
            if ( cluster_send_exec(c, fi->slot) < 0) {
                cluster_abort_exec(c);
                CLUSTER_THROW_EXCEPTION("Error processing EXEC across the cluster", 0);

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

    if (CLUSTER_IS_ATOMIC(c)) {
        php_error_docref(NULL, E_WARNING, "Cluster is not in MULTI mode");
        RETURN_FALSE;
    }

    if (cluster_abort_exec(c) < 0) {
        CLUSTER_RESET_MULTI(c);
    }

    CLUSTER_FREE_QUEUE(c);

    RETURN_TRUE;
}

/* Get a slot either by key (string) or host/port array */
static short
cluster_cmd_get_slot(redisCluster *c, zval *z_arg)
{
    size_t key_len;
    int key_free;
    zval *z_host, *z_port;
    short slot;
    char *key;
    zend_string *zstr;

    /* If it's a string, treat it as a key.  Otherwise, look for a two
     * element array */
    if (Z_TYPE_P(z_arg) ==IS_STRING || Z_TYPE_P(z_arg) ==IS_LONG ||
       Z_TYPE_P(z_arg) ==IS_DOUBLE)
    {
        /* Allow for any scalar here */
        zstr = zval_get_string(z_arg);
        key = ZSTR_VAL(zstr);
        key_len = ZSTR_LEN(zstr);

        /* Hash it */
        key_free = redis_key_prefix(c->flags, &key, &key_len);
        slot = cluster_hash_key(key, key_len);
        zend_string_release(zstr);
        if (key_free) efree(key);
    } else if (Z_TYPE_P(z_arg) == IS_ARRAY &&
        (z_host = zend_hash_index_find(Z_ARRVAL_P(z_arg), 0)) != NULL &&
        (z_port = zend_hash_index_find(Z_ARRVAL_P(z_arg), 1)) != NULL &&
        Z_TYPE_P(z_host) == IS_STRING && Z_TYPE_P(z_port) == IS_LONG
    ) {
        /* Attempt to find this specific node by host:port */
        slot = cluster_find_slot(c,(const char *)Z_STRVAL_P(z_host),
            (unsigned short)Z_LVAL_P(z_port));

        /* Inform the caller if they've passed bad data */
        if (slot < 0) {
            php_error_docref(0, E_WARNING, "Unknown node %s:" ZEND_LONG_FMT,
                Z_STRVAL_P(z_host), Z_LVAL_P(z_port));
        }
    } else {
        php_error_docref(0, E_WARNING,
            "Directed commands must be passed a key or [host,port] array");
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

    if (zend_parse_parameters(ZEND_NUM_ARGS(), "z", &z_arg) == FAILURE) {
        RETURN_FALSE;
    }

    // One argument means find the node (treated like a key), and two means
    // send the command to a specific host and port
    slot = cluster_cmd_get_slot(c, z_arg);
    if (slot < 0) {
        RETURN_FALSE;
    }

    // Construct our command
    cmd_len = redis_spprintf(NULL, NULL, &cmd, kw, "");

    // Kick off our command
    if (cluster_send_slot(c, slot, cmd, cmd_len, reply_type) < 0) {
        CLUSTER_THROW_EXCEPTION("Unable to send command at a specific node", 0);
        efree(cmd);
        RETURN_FALSE;
    }

    // Our response callback
    cb(INTERNAL_FUNCTION_PARAM_PASSTHRU, c, NULL);

    // Free our command
    efree(cmd);
}

static void
cluster_flush_cmd(INTERNAL_FUNCTION_PARAMETERS, char *kw, REDIS_REPLY_TYPE reply_type, cluster_cb cb)
{
    redisCluster *c = GET_CONTEXT();
    char *cmd;
    int cmd_len;
    zval *z_arg;
    zend_bool async = 0;
    short slot;

    if (zend_parse_parameters(ZEND_NUM_ARGS(), "z|b", &z_arg, &async) == FAILURE) {
        RETURN_FALSE;
    }

    // One argument means find the node (treated like a key), and two means
    // send the command to a specific host and port
    slot = cluster_cmd_get_slot(c, z_arg);
    if (slot < 0) {
        RETURN_FALSE;
    }

    // Construct our command
    if (async) {
        cmd_len = redis_spprintf(NULL, NULL, &cmd, kw, "s", "ASYNC", sizeof("ASYNC") - 1);
    } else {
        cmd_len = redis_spprintf(NULL, NULL, &cmd, kw, "");
    }


    // Kick off our command
    if (cluster_send_slot(c, slot, cmd, cmd_len, reply_type) < 0) {
        CLUSTER_THROW_EXCEPTION("Unable to send command at a specific node", 0);
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
    if (!CLUSTER_IS_ATOMIC(c)) {
        php_error_docref(0, E_WARNING,
            "Command can't be issued in MULTI mode");
        RETURN_FALSE;
    }

    /* We at least need the key or [host,port] argument */
    if (argc < 1) {
        php_error_docref(0, E_WARNING,
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
    if ((slot = cluster_cmd_get_slot(c, &z_args[0])) < 0) {
        efree(z_args);
        RETURN_FALSE;
    }

    /* Initialize our command */
    redis_cmd_init_sstr(&cmd, argc-1, kw, kw_len);

    /* Iterate, appending args */
    for(i = 1; i < argc; i++) {
        zend_string *zstr = zval_get_string(&z_args[i]);
        redis_cmd_append_sstr(&cmd, ZSTR_VAL(zstr), ZSTR_LEN(zstr));
        zend_string_release(zstr);
    }

    /* Send it off */
    if (cluster_send_slot(c, slot, cmd.c, cmd.len, TYPE_EOF) < 0) {
        CLUSTER_THROW_EXCEPTION("Couldn't send command to node", 0);
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
    char *cmd, *pat = NULL, *key = NULL;
    size_t key_len = 0, pat_len = 0, pat_free = 0;
    int cmd_len, key_free = 0;
    short slot;
    zval *z_it;
    HashTable *hash;
    long it, num_ele;
    zend_long count = 0;

    // Can't be in MULTI mode
    if (!CLUSTER_IS_ATOMIC(c)) {
        CLUSTER_THROW_EXCEPTION("SCAN type commands can't be called in MULTI mode!", 0);
        RETURN_FALSE;
    }

    /* Parse arguments */
    if (zend_parse_parameters(ZEND_NUM_ARGS(), "sz/|s!l", &key,
                             &key_len, &z_it, &pat, &pat_len, &count) == FAILURE)
    {
        RETURN_FALSE;
    }

    /* Treat as readonly */
    c->readonly = 1;

    // Convert iterator to long if it isn't, update our long iterator if it's
    // set and >0, and finish if it's back to zero
    if (Z_TYPE_P(z_it) != IS_LONG || Z_LVAL_P(z_it) < 0) {
        convert_to_long(z_it);
        it = 0;
    } else if (Z_LVAL_P(z_it) != 0) {
        it = Z_LVAL_P(z_it);
    } else {
        RETURN_FALSE;
    }

    // Apply any key prefix we have, get the slot
    key_free = redis_key_prefix(c->flags, &key, &key_len);
    slot = cluster_hash_key(key, key_len);

    if (c->flags->scan & REDIS_SCAN_PREFIX) {
        pat_free = redis_key_prefix(c->flags, &pat, &pat_len);
    }

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
        if (cluster_send_command(c, slot, cmd, cmd_len) == FAILURE)
        {
            CLUSTER_THROW_EXCEPTION("Couldn't send SCAN command", 0);
            if (key_free) efree(key);
            efree(cmd);
            RETURN_FALSE;
        }

        // Read response
        if (cluster_scan_resp(INTERNAL_FUNCTION_PARAM_PASSTHRU, c, type,
                              &it) == FAILURE)
        {
            CLUSTER_THROW_EXCEPTION("Couldn't read SCAN response", 0);
            if (key_free) efree(key);
            efree(cmd);
            RETURN_FALSE;
        }

        // Count the elements we got back
        hash = Z_ARRVAL_P(return_value);
        num_ele = zend_hash_num_elements(hash);

        // Free our command
        efree(cmd);
    } while (c->flags->scan & REDIS_SCAN_RETRY && it != 0 && num_ele == 0);

    // Free our pattern
    if (pat_free) efree(pat);

    // Free our key
    if (key_free) efree(key);

    // Update iterator reference
    Z_LVAL_P(z_it) = it;
}

static int redis_acl_op_readonly(zend_string *op) {
    /* Only return read-only for operations we know to be */
    if (ZSTR_STRICMP_STATIC(op, "LIST") ||
        ZSTR_STRICMP_STATIC(op, "USERS") ||
        ZSTR_STRICMP_STATIC(op, "GETUSER") ||
        ZSTR_STRICMP_STATIC(op, "CAT") ||
        ZSTR_STRICMP_STATIC(op, "GENPASS") ||
        ZSTR_STRICMP_STATIC(op, "WHOAMI") ||
        ZSTR_STRICMP_STATIC(op, "LOG")) return 1;

    return 0;
}

PHP_METHOD(RedisCluster, acl) {
    redisCluster *c = GET_CONTEXT();
    smart_string cmdstr = {0};
    int argc = ZEND_NUM_ARGS(), i, readonly;
    cluster_cb cb;
    zend_string *zs;
    zval *zargs;
    void *ctx = NULL;
    short slot;

    /* ACL in cluster needs a slot argument, and then at least the op */
    if (argc < 2) {
        WRONG_PARAM_COUNT;
        RETURN_FALSE;
    }

    /* Grab all our arguments and determine the command slot */
    zargs = emalloc(argc * sizeof(*zargs));
    if (zend_get_parameters_array(ht, argc, zargs) == FAILURE ||
        (slot = cluster_cmd_get_slot(c, &zargs[0]) < 0))
    {
        efree(zargs);
        RETURN_FALSE;
    }

    REDIS_CMD_INIT_SSTR_STATIC(&cmdstr, argc - 1, "ACL");

    /* Read the op, determin if it's readonly, and add it */
    zs = zval_get_string(&zargs[1]);
    readonly = redis_acl_op_readonly(zs);
    redis_cmd_append_sstr_zstr(&cmdstr, zs);

    /* We have specialized handlers for GETUSER and LOG, whereas every
     * other ACL command can be handled generically */
    if (zend_string_equals_literal_ci(zs, "GETUSER")) {
        cb = cluster_acl_getuser_resp;
    } else if (zend_string_equals_literal_ci(zs, "LOG")) {
        cb = cluster_acl_log_resp;
    } else {
        cb = cluster_variant_resp;
    }

    zend_string_release(zs);

    /* Process remaining args */
    for (i = 2; i < argc; i++) {
        zs = zval_get_string(&zargs[i]);
        redis_cmd_append_sstr_zstr(&cmdstr, zs);
        zend_string_release(zs);
    }

    /* Can we use replicas? */
    c->readonly = readonly && CLUSTER_IS_ATOMIC(c);

    /* Kick off our command */
    if (cluster_send_slot(c, slot, cmdstr.c, cmdstr.len, TYPE_EOF) < 0) {
        CLUSTER_THROW_EXCEPTION("Unabler to send ACL command", 0);
        efree(zargs);
        RETURN_FALSE;
    }

    if (CLUSTER_IS_ATOMIC(c)) {
        cb(INTERNAL_FUNCTION_PARAM_PASSTHRU, c, NULL);
    } else {
        CLUSTER_ENQUEUE_RESPONSE(c, slot, cb, ctx);
    }

    efree(cmdstr.c);
    efree(zargs);
}

/* {{{ proto RedisCluster::scan(string master, long it [, string pat, long cnt]) */
PHP_METHOD(RedisCluster, scan) {
    redisCluster *c = GET_CONTEXT();
    char *cmd, *pat = NULL;
    size_t pat_len = 0;
    int cmd_len;
    short slot;
    zval *z_it, *z_node;
    long it, num_ele, pat_free = 0;
    zend_long count = 0;

    /* Treat as read-only */
    c->readonly = CLUSTER_IS_ATOMIC(c);

    /* Can't be in MULTI mode */
    if (!CLUSTER_IS_ATOMIC(c)) {
        CLUSTER_THROW_EXCEPTION("SCAN type commands can't be called in MULTI mode", 0);
        RETURN_FALSE;
    }

    /* Parse arguments */
    if (zend_parse_parameters(ZEND_NUM_ARGS(), "z/z|s!l", &z_it,
                             &z_node, &pat, &pat_len, &count) == FAILURE)
    {
        RETURN_FALSE;
    }

    /* Convert or update iterator */
    if (Z_TYPE_P(z_it) != IS_LONG || Z_LVAL_P(z_it) < 0) {
        convert_to_long(z_it);
        it = 0;
    } else if (Z_LVAL_P(z_it) != 0) {
        it = Z_LVAL_P(z_it);
    } else {
        RETURN_FALSE;
    }

    if (c->flags->scan & REDIS_SCAN_PREFIX) {
        pat_free = redis_key_prefix(c->flags, &pat, &pat_len);
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

        if ((slot = cluster_cmd_get_slot(c, z_node)) < 0) {
           RETURN_FALSE;
        }

        // Send it to the node in question
        if (cluster_send_command(c, slot, cmd, cmd_len) < 0)
        {
            CLUSTER_THROW_EXCEPTION("Couldn't send SCAN to node", 0);
            efree(cmd);
            RETURN_FALSE;
        }

        if (cluster_scan_resp(INTERNAL_FUNCTION_PARAM_PASSTHRU, c, TYPE_SCAN,
                           &it) == FAILURE || Z_TYPE_P(return_value)!=IS_ARRAY)
        {
            CLUSTER_THROW_EXCEPTION("Couldn't process SCAN response from node", 0);
            efree(cmd);
            RETURN_FALSE;
        }

        efree(cmd);

        num_ele = zend_hash_num_elements(Z_ARRVAL_P(return_value));
    } while (c->flags->scan & REDIS_SCAN_RETRY && it != 0 && num_ele == 0);

    if (pat_free) efree(pat);

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
 *     proto RedisCluster::save(array host_port) */
PHP_METHOD(RedisCluster, save) {
    cluster_empty_node_cmd(INTERNAL_FUNCTION_PARAM_PASSTHRU, "SAVE", TYPE_LINE,
        cluster_bool_resp);
}
/* }}} */

/* {{{ proto RedisCluster::bgsave(string key)
 *     proto RedisCluster::bgsave(array host_port) */
PHP_METHOD(RedisCluster, bgsave) {
    cluster_empty_node_cmd(INTERNAL_FUNCTION_PARAM_PASSTHRU, "BGSAVE",
        TYPE_LINE, cluster_bool_resp);
}
/* }}} */

/* {{{ proto RedisCluster::flushdb(string key, [bool async])
 *     proto RedisCluster::flushdb(array host_port, [bool async]) */
PHP_METHOD(RedisCluster, flushdb) {
    cluster_flush_cmd(INTERNAL_FUNCTION_PARAM_PASSTHRU, "FLUSHDB",
        TYPE_LINE, cluster_bool_resp);
}
/* }}} */

/* {{{ proto RedisCluster::flushall(string key, [bool async])
 *     proto RedisCluster::flushall(array host_port, [bool async]) */
PHP_METHOD(RedisCluster, flushall) {
    cluster_flush_cmd(INTERNAL_FUNCTION_PARAM_PASSTHRU, "FLUSHALL",
        TYPE_LINE, cluster_bool_resp);
}
/* }}} */

/* {{{ proto RedisCluster::dbsize(string key)
 *     proto RedisCluster::dbsize(array host_port) */
PHP_METHOD(RedisCluster, dbsize) {
    cluster_empty_node_cmd(INTERNAL_FUNCTION_PARAM_PASSTHRU, "DBSIZE",
        TYPE_INT, cluster_long_resp);
}
/* }}} */

/* {{{ proto RedisCluster::bgrewriteaof(string key)
 *     proto RedisCluster::bgrewriteaof(array host_port) */
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
    char *cmd, *opt = NULL;
    int cmd_len;
    size_t opt_len = 0;
    void *ctx = NULL;

    zval *z_arg;
    short slot;

    if (zend_parse_parameters(ZEND_NUM_ARGS(), "z|s", &z_arg, &opt,
                             &opt_len) == FAILURE)
    {
        RETURN_FALSE;
    }

    /* Treat INFO as non read-only, as we probably want the master */
    c->readonly = 0;

    slot = cluster_cmd_get_slot(c, z_arg);
    if (slot < 0) {
        RETURN_FALSE;
    }

    if (opt != NULL) {
        cmd_len = redis_spprintf(NULL, NULL, &cmd, "INFO", "s", opt, opt_len);
    } else {
        cmd_len = redis_spprintf(NULL, NULL, &cmd, "INFO", "");
    }

    rtype = CLUSTER_IS_ATOMIC(c) ? TYPE_BULK : TYPE_LINE;
    if (cluster_send_slot(c, slot, cmd, cmd_len, rtype) < 0) {
        CLUSTER_THROW_EXCEPTION("Unable to send INFO command to specific node", 0);
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
    char *cmd, *opt = NULL, *arg = NULL;
    int cmd_len;
    size_t opt_len, arg_len = 0;
    REDIS_REPLY_TYPE rtype;
    zval *z_node;
    short slot;
    cluster_cb cb;

    /* Parse args */
    if (zend_parse_parameters(ZEND_NUM_ARGS(), "zs|s", &z_node, &opt,
                              &opt_len, &arg, &arg_len) == FAILURE)
    {
        RETURN_FALSE;
    }

    /* Make sure we can properly resolve the slot */
    slot = cluster_cmd_get_slot(c, z_node);
    if (slot < 0) RETURN_FALSE;

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
        php_error_docref(NULL, E_WARNING,
            "Invalid CLIENT subcommand (LIST, KILL, GETNAME, and SETNAME are valid");
        RETURN_FALSE;
    }

    /* Construct the command */
    if (ZEND_NUM_ARGS() == 3) {
        cmd_len = redis_spprintf(NULL, NULL, &cmd, "CLIENT", "ss",
            opt, opt_len, arg, arg_len);
    } else if (ZEND_NUM_ARGS() == 2) {
        cmd_len = redis_spprintf(NULL, NULL, &cmd, "CLIENT", "s",
            opt, opt_len);
    } else {
        zend_wrong_param_count();
        RETURN_FALSE;
    }

    /* Attempt to write our command */
    if (cluster_send_slot(c, slot, cmd, cmd_len, rtype) < 0) {
        CLUSTER_THROW_EXCEPTION("Unable to send CLIENT command to specific node", 0);
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
    redisCluster *c = GET_CONTEXT();
    smart_string cmd = {0};
    zval *z_args;
    short slot;
    int argc = ZEND_NUM_ARGS();

    /* Commands using this pass-thru don't need to be enabled in MULTI mode */
    if (!CLUSTER_IS_ATOMIC(c)) {
        php_error_docref(0, E_WARNING,
            "Command can't be issued in MULTI mode");
        RETURN_FALSE;
    }

    /* We at least need the key or [host,port] argument */
    if (argc < 2) {
        php_error_docref(0, E_WARNING,
            "Command requires at least an argument to direct to a node");
        RETURN_FALSE;
    }

    /* Allocate an array to process arguments */
    z_args = ecalloc(argc, sizeof(zval));

    /* Grab args */
    if (zend_get_parameters_array(ht, argc, z_args) == FAILURE ||
        (slot = cluster_cmd_get_slot(c, &z_args[0])) < 0 ||
        redis_build_script_cmd(&cmd, argc - 1, &z_args[1]) == NULL
    ) {
        efree(z_args);
        RETURN_FALSE;
    }

    /* Send it off */
    if (cluster_send_slot(c, slot, cmd.c, cmd.len, TYPE_EOF) < 0) {
        CLUSTER_THROW_EXCEPTION("Couldn't send command to node", 0);
        efree(cmd.c);
        efree(z_args);
        RETURN_FALSE;
    }

    /* Read the response variant */
    cluster_variant_resp(INTERNAL_FUNCTION_PARAM_PASSTHRU, c, NULL);

    efree(cmd.c);
    efree(z_args);
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
    CLUSTER_PROCESS_KW_CMD("GEORADIUS", redis_georadius_cmd, cluster_variant_resp, 1);
}

/* {{{ proto array RedisCluster::georadius() }}} */
PHP_METHOD(RedisCluster, georadius_ro) {
    CLUSTER_PROCESS_KW_CMD("GEORADIUS_RO", redis_georadius_cmd, cluster_variant_resp, 1);
}

/* {{{ proto array RedisCluster::georadiusbymember() }}} */
PHP_METHOD(RedisCluster, georadiusbymember) {
    CLUSTER_PROCESS_KW_CMD("GEORADIUSBYMEMBER", redis_georadiusbymember_cmd, cluster_variant_resp, 1);
}

/* {{{ proto array RedisCluster::georadiusbymember() }}} */
PHP_METHOD(RedisCluster, georadiusbymember_ro) {
    CLUSTER_PROCESS_KW_CMD("GEORADIUSBYMEMBER_RO", redis_georadiusbymember_cmd, cluster_variant_resp, 1);
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

/* {{{ proto bool RedisCluster::ping(string key| string msg)
 *     proto bool RedisCluster::ping(array host_port| string msg) */
PHP_METHOD(RedisCluster, ping) {
    redisCluster *c = GET_CONTEXT();
    REDIS_REPLY_TYPE rtype;
    void *ctx = NULL;
    zval *z_node;
    char *cmd, *arg = NULL;
    int cmdlen;
    size_t arglen;
    short slot;

    if (zend_parse_parameters(ZEND_NUM_ARGS(), "z|s!", &z_node, &arg,
                              &arglen) == FAILURE)
    {
        RETURN_FALSE;
    }

    /* Treat this as a readonly command */
    c->readonly = CLUSTER_IS_ATOMIC(c);

    /* Grab slot either by key or host/port */
    slot = cluster_cmd_get_slot(c, z_node);
    if (slot < 0) {
        RETURN_FALSE;
    }

    /* Construct our command */
    if (arg != NULL) {
        cmdlen = redis_spprintf(NULL, NULL, &cmd, "PING", "s", arg, arglen);
    } else {
        cmdlen = redis_spprintf(NULL, NULL, &cmd, "PING", "");
    }

    /* Send it off */
    rtype = CLUSTER_IS_ATOMIC(c) && arg != NULL ? TYPE_BULK : TYPE_LINE;
    if (cluster_send_slot(c, slot, cmd, cmdlen, rtype) < 0) {
        CLUSTER_THROW_EXCEPTION("Unable to send command at the specified node", 0);
        efree(cmd);
        RETURN_FALSE;
    }

    /* We're done with our command */
    efree(cmd);

    /* Process response */
    if (CLUSTER_IS_ATOMIC(c)) {
        if (arg != NULL) {
            cluster_bulk_resp(INTERNAL_FUNCTION_PARAM_PASSTHRU, c, NULL);
        } else {
            /* If we're atomic and didn't send an argument then we have already
             * processed the reply (which must have been successful. */
            RETURN_TRUE;
        }
    } else {
        if (arg != NULL) {
            CLUSTER_ENQUEUE_RESPONSE(c, slot, cluster_bulk_resp, ctx);
        } else {
            CLUSTER_ENQUEUE_RESPONSE(c, slot, cluster_variant_resp, ctx);
        }

        RETURN_ZVAL(getThis(), 1, 0);
    }
}
/* }}} */

/* {{{ proto long RedisCluster::xack(string key, string group, array ids) }}} */
PHP_METHOD(RedisCluster, xack) {
    CLUSTER_PROCESS_CMD(xack, cluster_long_resp, 0);
}

/* {{{ proto string RedisCluster::xadd(string key, string id, array field_values) }}} */
PHP_METHOD(RedisCluster, xadd) {
    CLUSTER_PROCESS_CMD(xadd, cluster_bulk_raw_resp, 0);
}

/* {{{ proto array RedisCluster::xclaim(string key, string group, string consumer,
 *                                      long min_idle_time, array ids, array options) */
PHP_METHOD(RedisCluster, xclaim) {
    CLUSTER_PROCESS_CMD(xclaim, cluster_xclaim_resp, 0);
}

PHP_METHOD(RedisCluster, xdel) {
    CLUSTER_PROCESS_KW_CMD("XDEL", redis_key_str_arr_cmd, cluster_long_resp, 0);
}

/* {{{ proto variant RedisCluster::xgroup(string op, [string key, string arg1, string arg2]) }}} */
PHP_METHOD(RedisCluster, xgroup) {
    CLUSTER_PROCESS_CMD(xgroup, cluster_variant_resp, 0);
}

/* {{{ proto variant RedisCluster::xinfo(string op, [string arg1, string arg2]); */
PHP_METHOD(RedisCluster, xinfo) {
    CLUSTER_PROCESS_CMD(xinfo, cluster_xinfo_resp, 0);
}

/* {{{ proto string RedisCluster::xlen(string key) }}} */
PHP_METHOD(RedisCluster, xlen) {
    CLUSTER_PROCESS_KW_CMD("XLEN", redis_key_cmd, cluster_long_resp, 1);
}

PHP_METHOD(RedisCluster, xpending) {
    CLUSTER_PROCESS_CMD(xpending, cluster_variant_resp_strings, 1);
}

PHP_METHOD(RedisCluster, xrange) {
    CLUSTER_PROCESS_KW_CMD("XRANGE", redis_xrange_cmd, cluster_xrange_resp, 1);
}

PHP_METHOD(RedisCluster, xrevrange) {
    CLUSTER_PROCESS_KW_CMD("XREVRANGE", redis_xrange_cmd, cluster_xrange_resp, 1);
}

PHP_METHOD(RedisCluster, xread) {
    CLUSTER_PROCESS_CMD(xread, cluster_xread_resp, 1);
}

PHP_METHOD(RedisCluster, xreadgroup) {
    CLUSTER_PROCESS_CMD(xreadgroup, cluster_xread_resp, 0);
}

PHP_METHOD(RedisCluster, xtrim) {
    CLUSTER_PROCESS_CMD(xtrim, cluster_long_resp, 0);
}



/* {{{ proto string RedisCluster::echo(string key, string msg)
 *     proto string RedisCluster::echo(array host_port, string msg) */
PHP_METHOD(RedisCluster, echo) {
    redisCluster *c = GET_CONTEXT();
    REDIS_REPLY_TYPE rtype;
    zval *z_arg;
    char *cmd, *msg;
    int cmd_len;
    size_t msg_len;
    short slot;

    if (zend_parse_parameters(ZEND_NUM_ARGS(), "zs", &z_arg, &msg,
                             &msg_len) == FAILURE)
    {
        RETURN_FALSE;
    }

    /* Treat this as a readonly command */
    c->readonly = CLUSTER_IS_ATOMIC(c);

    /* Grab slot either by key or host/port */
    slot = cluster_cmd_get_slot(c, z_arg);
    if (slot < 0) {
        RETURN_FALSE;
    }

    /* Construct our command */
    cmd_len = redis_spprintf(NULL, NULL, &cmd, "ECHO", "s", msg, msg_len);

    /* Send it off */
    rtype = CLUSTER_IS_ATOMIC(c) ? TYPE_BULK : TYPE_LINE;
    if (cluster_send_slot(c,slot,cmd,cmd_len,rtype) < 0) {
        CLUSTER_THROW_EXCEPTION("Unable to send command at the specified node", 0);
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
        php_error_docref(NULL, E_WARNING,
            "You must pass at least node information as well as at least a command.");
        RETURN_FALSE;
    }
    z_args = emalloc(argc * sizeof(zval));
    if (zend_get_parameters_array(ht, argc, z_args) == FAILURE) {
        php_error_docref(NULL, E_WARNING,
            "Internal PHP error parsing method parameters.");
        efree(z_args);
        RETURN_FALSE;
    } else if (redis_build_raw_cmd(&z_args[1], argc-1, &cmd, &cmd_len) ||
               (slot = cluster_cmd_get_slot(c, &z_args[0])) < 0)
    {
        if (cmd) efree(cmd);
        efree(z_args);
        RETURN_FALSE;
    }

    /* Free argument array */
    efree(z_args);

    /* Direct the command */
    rtype = CLUSTER_IS_ATOMIC(c) ? TYPE_EOF : TYPE_LINE;
    if (cluster_send_slot(c,slot,cmd,cmd_len,rtype) < 0) {
        CLUSTER_THROW_EXCEPTION("Unable to send command to the specified node", 0);
        efree(cmd);
        RETURN_FALSE;
    }

    /* Process variant response */
    if (CLUSTER_IS_ATOMIC(c)) {
        cluster_variant_raw_resp(INTERNAL_FUNCTION_PARAM_PASSTHRU, c, NULL);
    } else {
        void *ctx = NULL;
        CLUSTER_ENQUEUE_RESPONSE(c, slot, cluster_variant_raw_resp, ctx);
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

