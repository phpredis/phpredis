/* -*- Mode: C; tab-width: 4 -*- */
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
  | Original Author: Michael Grunder <michael.grunder@gmail.com          |
  | Maintainer: Nicolas Favre-Felix <n.favre-felix@owlient.eu>           |
  +----------------------------------------------------------------------+
*/

#include "hash/php_hash.h"
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "redis_commands.h"


#include "php_network.h"

#ifndef PHP_WIN32
#include <netinet/tcp.h> /* TCP_KEEPALIVE */
#else
#include <winsock.h>
#endif

#ifdef HAVE_REDIS_JSON
#include <ext/json/php_json.h>
#endif

#include <zend_exceptions.h>

/* Georadius sort type */
typedef enum geoSortType {
    SORT_NONE,
    SORT_ASC,
    SORT_DESC
} geoSortType;

/* Georadius store type */
typedef enum geoStoreType {
    STORE_NONE,
    STORE_COORD,
    STORE_DIST
} geoStoreType;

/* Georadius options structure */
typedef struct geoOptions {
    int withcoord;
    int withdist;
    int withhash;
    long count;
    zend_bool any;
    geoSortType sort;
    geoStoreType store;
    zend_string *key;
} geoOptions;

typedef enum geosearchShape {
    GEOSEARCH_SHAPE_RADIUS,
    GEOSEARCH_SHAPE_BOX,
    GEOSEARCH_SHAPE_POLYGON
} geosearchShape;

typedef enum geosearchPosition {
    GEOSEARCH_POSITION_NONE,
    GEOSEARCH_POSITION_MEMBER,
    GEOSEARCH_POSITION_LONLAT
} geosearchPosition;

typedef struct geoSearchOptions {
    geosearchShape shape_type;
    geosearchPosition position_type;
    union {
        double radius;
        HashTable *dimensions;
    } shape;
    union {
        zend_string *member;
        HashTable *coordinates;
    } position;
    int shape_argc;
    int position_argc;
} geoSearchOptions;

typedef struct redisLcsOptions {
    zend_bool len;
    zend_bool idx;
    zend_long minmatchlen;
    zend_bool withmatchlen;
} redisLcsOptions;

typedef struct redisRestoreOptions {
    zend_bool replace;
    zend_bool absttl;
    zend_long idletime;
    zend_long freq;
} redisRestoreOptions;

typedef enum redisSetType {
    REDIS_SET_NONE,
    REDIS_SET_NX,
    REDIS_SET_XX
} redisSetType;

typedef enum redisExpiryType {
    REDIS_EXPIRY_NONE,
    REDIS_EXPIRY_EX,
    REDIS_EXPIRY_PX,
    REDIS_EXPIRY_EXAT,
    REDIS_EXPIRY_PXAT,
} redisExpiryType;

typedef enum redisEqType {
    REDIS_IF_NONE,
    REDIS_IFEQ,
    REDIS_IFNE,
    REDIS_IFDEQ,
    REDIS_IFDNE
} redisEqType;

typedef struct redisExpiryOptions {
    redisExpiryType type;
    zend_long ttl;
    zend_bool keepttl;
} redisExpiryOptions;

typedef struct redisSetOptions {
    redisSetType type;
    zend_bool get;
    redisExpiryOptions expiry;
    struct {
        redisEqType type;
        zval *zval;
    } eq;
} redisSetOptions;


#define REDIS_ZCMD_HAS_DST_KEY      (1 << 0)
#define REDIS_ZCMD_HAS_WITHSCORES   (1 << 1)
#define REDIS_ZCMD_HAS_BY_LEX_SCORE (1 << 2)
#define REDIS_ZCMD_HAS_REV          (1 << 3)
#define REDIS_ZCMD_HAS_LIMIT        (1 << 4)
#define REDIS_ZCMD_INT_RANGE        (1 << 5)
#define REDIS_ZCMD_HAS_AGGREGATE    (1 << 6)

/* ZRANGE, ZRANGEBYSCORE, ZRANGESTORE options */
typedef struct redisZcmdOptions {
    zend_bool withscores;
    zend_bool byscore;
    zend_bool bylex;
    zend_bool rev;
    zend_string *aggregate;
    struct {
        zend_bool enabled;
        zend_long offset;
        zend_long count;
    } limit;
} redisZcmdOptions;

static zend_always_inline void redis_crosslot_warning(void) {
    php_error_docref(NULL, E_WARNING,
        "Warning, not all keys hash to the same slot!");
}

#define redis_cmd_try_cat_key(cmd, fn, ...) \
    do { \
        if (!fn((cmd), __VA_ARGS__)) { \
            redis_crosslot_warning(); \
            redis_cmd_free_ex(cmd, 1); \
            return NULL; \
        } \
    } while (0)

#define redis_cmd_try_cat_key_zstr(cmd, zstr) \
    redis_cmd_try_cat_key(cmd, redis_cmd_cat_key_zstr, zstr)
#define redis_cmd_try_cat_key_zval(cmd, zval) \
    redis_cmd_try_cat_key(cmd, redis_cmd_cat_key_zval, zval)

static zend_always_inline void
redis_cmd_cat_channel_zstr(RedisCmd *cmd, zend_string *channel) {
    zend_string *prefixed;

    if (UNEXPECTED(cmd->redis_sock == NULL)) {
        redis_cmd_cat_zstr(cmd, channel);
        return;
    }

    prefixed = redis_key_prefix_zstr(cmd->redis_sock, channel);

    if (cmd->redis_sock->type == REDIS_SOCK_CLUSTER) {
        cmd->slot = cluster_hash_key(ZSTR_VAL(prefixed), ZSTR_LEN(prefixed));
    }

    redis_cmd_cat_zstr(cmd, prefixed);
    zend_string_release(prefixed);
}

static zend_always_inline void
redis_cmd_cat_channel_zval(RedisCmd *cmd, zval *zv) {
    zend_string *zstr, *tmp;

    zstr = zval_get_tmp_string(zv, &tmp);
    redis_cmd_cat_channel_zstr(cmd, zstr);
    zend_tmp_string_release(tmp);
}

/* Generic commands based on method signature and what kind of things we're
 * processing.  Lots of Redis commands take something like key, value, or
 * key, value long.  Each unique signature like this is written only once */

/* A command that takes no arguments */
RedisCmd *redis_empty_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock, const char *kw, size_t kw_len)
{
    RedisCmd *cmd;

    if (zend_parse_parameters_none() == FAILURE) {
        return NULL;
    }

    cmd = redis_cmd_create(redis_sock, kw, kw_len);

    redis_cmd_randslot(cmd);

    return cmd;
}

/* Helper to construct a raw command.  Given that the cluster and non cluster
 * versions are different (RedisCluster needs an additional argument to direct
 * the command) we take the start of our array and count */
RedisCmd *redis_build_raw_cmd(zval *z_args, int argc) {
    RedisCmd *cmd;
    int i;

    /* Make sure our first argument is a string */
    if (Z_TYPE(z_args[0]) != IS_STRING) {
        php_error_docref(NULL, E_WARNING,
            "When sending a 'raw' command, the first argument must be a string!");
        return NULL;
    }

    cmd = redis_cmd_create(NULL, Z_STRVAL(z_args[0]), Z_STRLEN(z_args[0]));

    for (i = 1; i < argc; i++) {
       switch (Z_TYPE(z_args[i])) {
            case IS_STRING:
                redis_cmd_cat_zstr(cmd, Z_STR(z_args[i]));
                break;
            case IS_LONG:
                redis_cmd_cat_long(cmd, Z_LVAL(z_args[i]));
                break;
            case IS_DOUBLE:
                redis_cmd_cat_double(cmd, Z_DVAL(z_args[i]));
                break;
            default:
                php_error_docref(NULL, E_WARNING,
                    "Raw command arguments must be scalar values!");
                redis_cmd_free(cmd);
                return NULL;
        }
    }

    return cmd;
}

RedisCmd *redis_build_script_cmd(int argc, zval *z_args) {
    RedisCmd *cmd;
    int i;

    if (Z_TYPE(z_args[0]) != IS_STRING) {
        return NULL;
    }

    cmd = redis_cmd_create_literal(NULL, "SCRIPT");

    // Branch based on the directive
    if (zend_string_equals_literal_ci(Z_STR(z_args[0]), "kill")) {
        redis_cmd_cat_literal(cmd, "KILL");
    } else if (zend_string_equals_literal_ci(Z_STR(z_args[0]), "flush")) {
        // Simple SCRIPT FLUSH [ASYNC | SYNC]
        if (argc > 1 && (
            Z_TYPE(z_args[1]) != IS_STRING || (
                !zend_string_equals_literal_ci(Z_STR(z_args[1]), "sync") &&
                !zend_string_equals_literal_ci(Z_STR(z_args[1]), "async")
            )
        )) {
            goto error;
        }

        redis_cmd_cat_literal(cmd, "FLUSH");
        if (argc > 1) {
            redis_cmd_cat_zval_zstr(cmd, &z_args[1]);
        }
    } else if (zend_string_equals_literal_ci(Z_STR(z_args[0]), "load")) {
        // Make sure we have a second argument, and it's not empty.  If it is
        // empty, we can just return an empty array (which is what Redis does)
        if (argc < 2 || Z_TYPE(z_args[1]) != IS_STRING || Z_STRLEN(z_args[1]) < 1) {
            goto error;
        }

        redis_cmd_cat_literal(cmd, "LOAD");
        redis_cmd_cat_zval_zstr(cmd, &z_args[1]);
    } else if (zend_string_equals_literal_ci(Z_STR(z_args[0]), "exists")) {
        // Make sure we have a second argument
        if (argc < 2) {
            goto error;
        }

        redis_cmd_cat_literal(cmd, "EXISTS");

        for (i = 1; i < argc; ++i) {
            redis_cmd_cat_zval_zstr(cmd, &z_args[i]);
        }
    } else {
        goto error;
    }

    return cmd;

error:
    redis_cmd_free(cmd);
    return NULL;
}

/* Command that takes one optional string */
RedisCmd *redis_opt_str_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock, const char *kw, size_t kw_len)
{
    zend_string *arg = NULL;
    RedisCmd *cmd;

    ZEND_PARSE_PARAMETERS_START(0, 1)
        Z_PARAM_OPTIONAL
        Z_PARAM_STR_OR_NULL(arg)
    ZEND_PARSE_PARAMETERS_END_EX(return NULL);

    cmd = redis_cmd_create(redis_sock, kw, kw_len);
    if (arg != NULL)
        redis_cmd_cat_zstr(cmd, arg);

    return cmd;
}

/* Generic command where we just take a string and do nothing to it*/
RedisCmd *redis_str_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock, const char *kw, size_t kw_len)
{
    zend_string *arg;
    RedisCmd *cmd;

    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_STR(arg)
    ZEND_PARSE_PARAMETERS_END_EX(return NULL);

    cmd = redis_cmd_create(redis_sock, kw, kw_len);
    redis_cmd_cat_zstr(cmd, arg);

    return cmd;
}

/* Key, long, zval (serialized) */
RedisCmd *redis_key_long_val_cmd(INTERNAL_FUNCTION_PARAMETERS,
                                 RedisSock *redis_sock, const char *kw, size_t kw_len)
{
    zend_string *key = NULL;
    zend_long expire;
    zval *z_val;

    ZEND_PARSE_PARAMETERS_START(3, 3)
        Z_PARAM_STR(key)
        Z_PARAM_LONG(expire)
        Z_PARAM_ZVAL(z_val)
    ZEND_PARSE_PARAMETERS_END_EX(return NULL);

    return redis_cmd_fmt_ex(redis_sock, kw, kw_len, "Klv", key, expire, z_val);
}

/* Generic key, long, string (unserialized) */
RedisCmd *redis_key_long_str_cmd(INTERNAL_FUNCTION_PARAMETERS,
                                 RedisSock *redis_sock, const char *kw, size_t kw_len)
{
    zend_string *key, *val;
    zend_long lval;
    RedisCmd *cmd;

    ZEND_PARSE_PARAMETERS_START(3, 3)
        Z_PARAM_STR(key)
        Z_PARAM_LONG(lval)
        Z_PARAM_STR(val)
    ZEND_PARSE_PARAMETERS_END_EX(return NULL);

    cmd = redis_cmd_create(redis_sock, kw, kw_len);

    redis_cmd_cat_key_zstr(cmd, key);
    redis_cmd_cat_long(cmd, lval);
    redis_cmd_cat_zstr(cmd, val);

    return cmd;
}

/* Generic command construction when we just take a key and value */
RedisCmd *redis_kv_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock, const char *kw, size_t kw_len)
{
    zend_string *key;
    zval *zv;

    ZEND_PARSE_PARAMETERS_START(2, 2)
        Z_PARAM_STR(key)
        Z_PARAM_ZVAL(zv)
    ZEND_PARSE_PARAMETERS_END_EX(return NULL);

    return redis_cmd_fmt_ex(redis_sock, kw, kw_len, "Kv", key, zv);
}

/* Generic command that takes a key and an unserialized value */
RedisCmd *redis_key_str_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock, const char *kw, size_t kw_len)
{
    zend_string *key, *val;

    ZEND_PARSE_PARAMETERS_START(2, 2)
        Z_PARAM_STR(key)
        Z_PARAM_STR(val)
    ZEND_PARSE_PARAMETERS_END_EX(return NULL);

    return redis_cmd_fmt_ex(redis_sock, kw, kw_len, "KS", key, val);
}

/* Key, string, string without serialization (ZCOUNT, ZREMRANGEBYSCORE) */
RedisCmd *redis_key_str_str_cmd(INTERNAL_FUNCTION_PARAMETERS,
                                RedisSock *redis_sock, const char *kw, size_t kw_len)
{
    zend_string *key, *str1, *str2;
    RedisCmd *cmd;

    ZEND_PARSE_PARAMETERS_START(3, 3)
        Z_PARAM_STR(key)
        Z_PARAM_STR(str1)
        Z_PARAM_STR(str2)
    ZEND_PARSE_PARAMETERS_END_EX(return NULL);

    cmd = redis_cmd_create(redis_sock, kw, kw_len);

    redis_cmd_cat_key_zstr(cmd, key);
    redis_cmd_cat_zstr(cmd, str1);
    redis_cmd_cat_zstr(cmd, str2);

    return cmd;
}

/* Generic command that takes two keys */
RedisCmd *redis_key_key_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock, const char *kw, size_t kw_len)
{
    zend_string *key1 = NULL, *key2 = NULL;
    RedisCmd *cmd;

    ZEND_PARSE_PARAMETERS_START(2, 2)
        Z_PARAM_STR(key1)
        Z_PARAM_STR(key2)
    ZEND_PARSE_PARAMETERS_END_EX(return NULL);

    cmd = redis_cmd_create(redis_sock, kw, kw_len);
    redis_cmd_cat_key_zstr(cmd, key1);

    redis_cmd_try_cat_key_zstr(cmd, key2);

    return cmd;
}

/* Generic command construction where we take a key and a long */
RedisCmd *redis_key_long_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock, const char *kw, size_t kw_len)
{
    zend_string *key = NULL;
    zend_long lval = 0;

    ZEND_PARSE_PARAMETERS_START(2, 2)
        Z_PARAM_STR(key)
        Z_PARAM_LONG(lval)
    ZEND_PARSE_PARAMETERS_END_EX(return NULL);

    return redis_cmd_fmt_ex(redis_sock, kw, kw_len, "Kl", key, lval);
}

/* long, long */
RedisCmd *redis_long_long_cmd(INTERNAL_FUNCTION_PARAMETERS,
                              RedisSock *redis_sock, const char *kw, size_t kw_len)
{
    zend_long l1 = 0, l2 = 0;

    ZEND_PARSE_PARAMETERS_START(2, 2)
        Z_PARAM_LONG(l1)
        Z_PARAM_LONG(l2)
    ZEND_PARSE_PARAMETERS_END_EX(return NULL);

    return redis_cmd_fmt_ex(redis_sock, kw, kw_len, "ll", l1, l2);
}

/* key, long, long */
RedisCmd *redis_key_long_long_cmd(INTERNAL_FUNCTION_PARAMETERS,
                                  RedisSock *redis_sock, const char *kw, size_t kw_len)
{
    zend_long val1, val2;
    zend_string *key;

    ZEND_PARSE_PARAMETERS_START(3, 3)
        Z_PARAM_STR(key)
        Z_PARAM_LONG(val1)
        Z_PARAM_LONG(val2)
    ZEND_PARSE_PARAMETERS_END_EX(return NULL);

    return redis_cmd_fmt_ex(redis_sock, kw, kw_len, "Kll", key, val1, val2);
}

/* Generic command where we take a single key */
RedisCmd *redis_key_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock, const char *kw, size_t kw_len)
{
    zend_string *key;

    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_STR(key);
    ZEND_PARSE_PARAMETERS_END_EX(return NULL);

    return redis_cmd_fmt_ex(redis_sock, kw, kw_len, "K", key);
}

RedisCmd *
redis_failover_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock)
{
    zend_bool abort = 0, force = 0;
    zend_long timeout = 0, port = 0;
    zend_string *zkey, *host = NULL;
    zval *z_to = NULL, *z_ele;
    RedisCmd *cmd;

    ZEND_PARSE_PARAMETERS_START(0, 3)
        Z_PARAM_OPTIONAL
        Z_PARAM_ARRAY_OR_NULL(z_to)
        Z_PARAM_BOOL(abort)
        Z_PARAM_LONG(timeout)
    ZEND_PARSE_PARAMETERS_END_EX(return NULL);

    if (z_to != NULL) {
        ZEND_HASH_FOREACH_STR_KEY_VAL(Z_ARRVAL_P(z_to), zkey, z_ele) {
            if (zkey != NULL) {
                ZVAL_DEREF(z_ele);
                if (zend_string_equals_literal_ci(zkey, "host")) {
                    host = zval_get_string(z_ele);
                } else if (zend_string_equals_literal_ci(zkey, "port")) {
                    port = zval_get_long(z_ele);
                } else if (zend_string_equals_literal_ci(zkey, "force")) {
                    force = zend_is_true(z_ele);
                }
            }
        } ZEND_HASH_FOREACH_END();
        if (!host || !port) {
            php_error_docref(NULL, E_WARNING, "host and port must be provided!");
            if (host) zend_string_release(host);
            return NULL;
        }
    }

    cmd = redis_cmd_create_literal(redis_sock, "FAILOVER");

    if (host && port) {
        redis_cmd_cat_literal(cmd, "TO");
        redis_cmd_cat_zstr(cmd, host);
        redis_cmd_cat_long(cmd, port);
        if (force) {
            redis_cmd_cat_literal(cmd, "FORCE");
        }
        zend_string_release(host);
    }

    if (abort) {
        redis_cmd_cat_literal(cmd, "ABORT");
    }
    if (timeout > 0) {
        redis_cmd_cat_literal(cmd, "TIMEOUT");
        redis_cmd_cat_long(cmd, timeout);
    }

    return cmd;
}

RedisCmd *redis_flush_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock, const char *kw, size_t kw_len)
{
    zend_bool sync = 0;
    zend_bool is_null = 1;
    RedisCmd *cmd;

    ZEND_PARSE_PARAMETERS_START(0, 1)
        Z_PARAM_OPTIONAL
        Z_PARAM_BOOL_OR_NULL(sync, is_null)
    ZEND_PARSE_PARAMETERS_END_EX(return NULL);

    cmd = redis_cmd_create(redis_sock, kw, kw_len);
    if (!is_null) {
        ZEND_ASSERT(sync == 0 || sync == 1);
        if (sync == 0) {
            redis_cmd_cat_literal(cmd, "ASYNC");
        } else {
            redis_cmd_cat_literal(cmd, "SYNC");
        }
    }

    return cmd;
}

/* Generic command where we take a key and a double */
RedisCmd *redis_key_dbl_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock, const char *kw, size_t kw_len)
{
    zend_string *key;
    double val;

    ZEND_PARSE_PARAMETERS_START(2, 2)
        Z_PARAM_STR(key)
        Z_PARAM_DOUBLE(val)
    ZEND_PARSE_PARAMETERS_END_EX(return NULL);

    return redis_cmd_fmt_ex(redis_sock, kw, kw_len, "Kf", key, val);
}

/* Generic to construct SCAN and variant commands */
RedisCmd *
redis_fmt_scan_cmd(RedisSock *redis_sock, REDIS_SCAN_TYPE type, char *key,
                   int key_len, uint64_t it, char *pat, int pat_len,
                   long count)
{
    static char *kw[] = {"SCAN","SSCAN","HSCAN","ZSCAN"};
    RedisCmd *cmd;

    cmd = redis_cmd_create(redis_sock, kw[type], strlen(kw[type]));

    // Append our key if it's not a regular SCAN command
    if (type != TYPE_SCAN) {
        if (!redis_cmd_cat_key_str(cmd, key, key_len)) {
            redis_cmd_free(cmd);
            return NULL;
        }
    }

    // Append cursor
    redis_cmd_cat_u64(cmd, it);

    // Append count if we've got one
    if (count) {
        redis_cmd_cat_literal(cmd, "COUNT");
        redis_cmd_cat_long(cmd, count);
    }

    // Append pattern if we've got one
    if (pat_len) {
        redis_cmd_cat_literal(cmd, "MATCH");
        redis_cmd_cat_str(cmd, pat, pat_len);
    }

    return cmd;
}

void redis_get_zcmd_options(redisZcmdOptions *dst, zval *src, int flags) {
    zval *zv, *zoff, *zcnt;
    zend_string *key;

    ZEND_ASSERT(dst != NULL);

    memset(dst, 0, sizeof(*dst));

    if (src == NULL)
        return;

    if (Z_TYPE_P(src) != IS_ARRAY) {
        if (Z_TYPE_P(src) == IS_TRUE && (flags & REDIS_ZCMD_HAS_WITHSCORES))
            dst->withscores = 1;
        return;
    }

    ZEND_HASH_FOREACH_STR_KEY_VAL(Z_ARRVAL_P(src), key, zv) {
        ZVAL_DEREF(zv);

        if (key) {
            if ((flags & REDIS_ZCMD_HAS_WITHSCORES) && zend_string_equals_literal_ci(key, "WITHSCORES"))
                dst->withscores = zend_is_true(zv);
            else if ((flags & REDIS_ZCMD_HAS_LIMIT) && zend_string_equals_literal_ci(key, "LIMIT") &&
                     Z_TYPE_P(zv) == IS_ARRAY)
            {
                if ((zoff = zend_hash_index_find(Z_ARRVAL_P(zv), 0)) != NULL &&
                    (zcnt = zend_hash_index_find(Z_ARRVAL_P(zv), 1)) != NULL)
                {
                    dst->limit.enabled = 1;
                    dst->limit.offset = zval_get_long(zoff);
                    dst->limit.count = zval_get_long(zcnt);
                } else {
                    php_error_docref(NULL, E_WARNING, "LIMIT offset and count must be an array with twe elements");
                }
            } else if ((flags & REDIS_ZCMD_HAS_AGGREGATE && zend_string_equals_literal_ci(key, "AGGREGATE")) &&
                       Z_TYPE_P(zv) == IS_STRING)
            {
                if (Z_TYPE_P(zv) != IS_STRING || (!zend_string_equals_literal_ci(Z_STR_P(zv), "SUM") &&
                                                  !zend_string_equals_literal_ci(Z_STR_P(zv), "MIN") &&
                                                  !zend_string_equals_literal_ci(Z_STR_P(zv), "MAX")))
                {
                    php_error_docref(NULL, E_WARNING, "Valid AGGREGATE options are 'SUM', 'MIN', or 'MAX'");
                } else {
                    dst->aggregate = Z_STR_P(zv);
                }
            }
        } else if (Z_TYPE_P(zv) == IS_STRING) {
            key = Z_STR_P(zv);

            if ((flags & REDIS_ZCMD_HAS_BY_LEX_SCORE) && zend_string_equals_literal_ci(key, "BYSCORE"))
                dst->byscore = 1, dst->bylex = 0;
            else if ((flags & REDIS_ZCMD_HAS_BY_LEX_SCORE) && zend_string_equals_literal_ci(key, "BYLEX"))
                dst->bylex = 1, dst->byscore = 0;
            else if ((flags & REDIS_ZCMD_HAS_REV) && zend_string_equals_literal_ci(key, "REV"))
                dst->rev = 1;
            else if ((flags & REDIS_ZCMD_HAS_WITHSCORES && zend_string_equals_literal_ci(key, "WITHSCORES")))
                dst->withscores = 1;
        }
    } ZEND_HASH_FOREACH_END();
}

#if defined(__clang__) || \
    (defined(__GNUC__) && (__GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 3)))
#define PHPREDIS_HAVE_BSWAP32 1
#endif

static void redis_copy_fp32(char *dst, float src) {
#ifdef PHPREDIS_BIG_ENDIAN
#ifdef PHPREDIS_HAVE_BSWAP32
    uint32_t val;
    memcpy(&val, &src, sizeof(val));
    val = __builtin_bswap32(val);
    memcpy(dst, &val, sizeof(val));
#else
    union {
        float f;
        unsigned char b[4];
    } u;
    u.f = src;
    dst[0] = u.b[3];
    dst[1] = u.b[2];
    dst[2] = u.b[1];
    dst[3] = u.b[0];
#endif
#else
    memcpy(dst, &src, sizeof(src));
#endif
}

// + ZRANGE               key start stop [BYSCORE | BYLEX] [REV] [LIMIT offset count] [WITHSCORES]
// + ZRANGESTORE      dst src   min  max [BYSCORE | BYLEX] [REV] [LIMIT offset count]
// + ZREVRANGE            key start stop                                              [WITHSCORES]
// + ZRANGEBYSCORE        key   min  max                         [LIMIT offset count] [WITHSCORES]
// + ZREVRANGEBYSCORE     key   max  min                         [LIMIT offset count] [WITHSCORES]
// - ZRANGEBYLEX          key   min  max                         [LIMIT offset count]
// - ZREVRANGEBYLEX       key   max  min                         [LIMIT offset count]
// - ZDIFF                                                                            [WITHSCORES]
// - ZUNION                                                                           [WITHSCORES] [AGGREGATE X]
// - ZINTER                                                                           [WITHSCORES] [AGGREGATE X]
static int redis_get_zcmd_flags(const char *kw, size_t kw_len) {
    if (redis_str_ieq(kw, kw_len, ZEND_STRL("ZRANGESTORE"))) {
        return REDIS_ZCMD_HAS_DST_KEY |
               REDIS_ZCMD_HAS_WITHSCORES |
               REDIS_ZCMD_HAS_BY_LEX_SCORE |
               REDIS_ZCMD_HAS_REV |
               REDIS_ZCMD_HAS_LIMIT;
    } else if (redis_str_ieq(kw, kw_len, ZEND_STRL("ZRANGE"))) {
        return REDIS_ZCMD_HAS_WITHSCORES |
               REDIS_ZCMD_HAS_BY_LEX_SCORE |
               REDIS_ZCMD_HAS_REV |
               REDIS_ZCMD_HAS_LIMIT;
    } else if (redis_str_ieq(kw, kw_len, ZEND_STRL("ZREVRANGE"))) {
        return REDIS_ZCMD_HAS_WITHSCORES |
               REDIS_ZCMD_INT_RANGE;
    } else if (redis_str_ieq(kw, kw_len, ZEND_STRL("ZRANGEBYSCORE")) ||
               redis_str_ieq(kw, kw_len, ZEND_STRL("ZREVRANGEBYSCORE")))
    {
        return REDIS_ZCMD_HAS_LIMIT |
               REDIS_ZCMD_HAS_WITHSCORES;
    } else if (redis_str_ieq(kw, kw_len, ZEND_STRL("ZRANGEBYLEX")) ||
               redis_str_ieq(kw, kw_len, ZEND_STRL("ZREVRANGEBYLEX")))
    {
        return REDIS_ZCMD_HAS_LIMIT;
    } else if (redis_str_ieq(kw, kw_len, ZEND_STRL("ZDIFF"))) {
        return REDIS_ZCMD_HAS_WITHSCORES;
    } else if (redis_str_ieq(kw, kw_len, ZEND_STRL("ZINTER")) ||
               redis_str_ieq(kw, kw_len, ZEND_STRL("ZUNION")))
    {
        return REDIS_ZCMD_HAS_WITHSCORES |
               REDIS_ZCMD_HAS_AGGREGATE;
    }

    /* Reaching this line means a compile-time error */
    ZEND_ASSERT(0);
}

/* Validate ZLEX* min/max argument strings */
static int validate_zlex_arg(const char *str, size_t len) {
    return (len  > 1 && (*str == '[' || *str == '(')) ||
           (len == 1 && (*str == '+' || *str == '-'));
}

static zend_always_inline int validate_zlex_arg_zstr(zend_string *z) {
    return validate_zlex_arg(ZSTR_VAL(z), ZSTR_LEN(z));
}

static int validate_zlex_arg_zval(zval *z) {
    return Z_TYPE_P(z) == IS_STRING && validate_zlex_arg_zstr(Z_STR_P(z));
}

RedisCmd *redis_zrange_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock, const char *kw, size_t kw_len)
{
    zval *zoptions = NULL, *zstart = NULL, *zend = NULL;
    zend_string *dst = NULL, *src = NULL;
    zend_long start = 0, end = 0;
    redisZcmdOptions opt;
    int min_argc, flags;
    RedisCmd *cmd;

    flags = redis_get_zcmd_flags(kw, kw_len);

    min_argc = 3 + (flags & REDIS_ZCMD_HAS_DST_KEY);
    ZEND_PARSE_PARAMETERS_START(min_argc, min_argc + 1)
        if (flags & REDIS_ZCMD_HAS_DST_KEY) {
            Z_PARAM_STR(dst)
        }
        Z_PARAM_STR(src)
        if (flags & REDIS_ZCMD_INT_RANGE) {
            Z_PARAM_LONG(start)
            Z_PARAM_LONG(end)
        } else {
            Z_PARAM_ZVAL(zstart)
            Z_PARAM_ZVAL(zend)
        }
        Z_PARAM_OPTIONAL
        Z_PARAM_ZVAL_OR_NULL(zoptions)
    ZEND_PARSE_PARAMETERS_END_EX(return NULL);

    redis_get_zcmd_options(&opt, zoptions, flags);

    if (opt.bylex) {
        ZEND_ASSERT(!(flags & REDIS_ZCMD_INT_RANGE));
        if (!validate_zlex_arg_zval(zstart) || !validate_zlex_arg_zval(zend)) {
            php_error_docref(NULL, E_WARNING, "Legographical args must start with '[' or '(' or be '+' or '-'");
            return NULL;
        }
    }

    cmd = redis_cmd_create(redis_sock, kw, kw_len);

    if ((flags & REDIS_ZCMD_HAS_DST_KEY))
        redis_cmd_try_cat_key_zstr(cmd, dst);

    redis_cmd_try_cat_key_zstr(cmd, src);

    if (flags & REDIS_ZCMD_INT_RANGE) {
        redis_cmd_cat_long(cmd, start);
        redis_cmd_cat_long(cmd, end);
    } else {
        redis_cmd_cat_zval_zstr(cmd, zstart);
        redis_cmd_cat_zval_zstr(cmd, zend);
    }

    redis_cmd_cat_literal_if(cmd, opt.byscore, "BYSCORE");
    redis_cmd_cat_literal_if(cmd, opt.bylex, "BYLEX");
    redis_cmd_cat_literal_if(cmd, opt.rev, "REV");

    if (opt.limit.enabled) {
        redis_cmd_cat_literal(cmd, "LIMIT");
        redis_cmd_cat_long(cmd, opt.limit.offset);
        redis_cmd_cat_long(cmd, opt.limit.count);
    }

    redis_cmd_cat_literal_if(cmd, opt.withscores, "WITHSCORES");

    redis_cmd_set_ctx(cmd, opt.withscores ? PHPREDIS_CTX_PTR : NULL);

    return cmd;
}

static RedisCmd *redis_build_config_get_cmd(RedisSock *redis_sock, zval *val) {
    zend_string *zstr;
    RedisCmd *cmd;
    zval *zv;

    if (val == NULL || (Z_TYPE_P(val) != IS_STRING && Z_TYPE_P(val) != IS_ARRAY)) {
        php_error_docref(NULL, E_WARNING, "Must pass a string or array of values to CONFIG GET");
        return NULL;
    } else if (Z_TYPE_P(val) == IS_ARRAY && zend_hash_num_elements(Z_ARRVAL_P(val)) == 0) {
        php_error_docref(NULL, E_WARNING, "Cannot pass an empty array to CONFIG GET");
        return NULL;
    }

    cmd = redis_cmd_create_literal(redis_sock, "CONFIG");
    redis_cmd_cat_literal(cmd, "GET");

    if (Z_TYPE_P(val) == IS_STRING) {
        redis_cmd_cat_zstr(cmd, Z_STR_P(val));
    } else {
        ZEND_HASH_FOREACH_VAL(Z_ARRVAL_P(val), zv) {
            ZVAL_DEREF(zv);

            zstr = zval_get_string(zv);
            redis_cmd_cat_zstr(cmd, zstr);
            zend_string_release(zstr);
        } ZEND_HASH_FOREACH_END();
    }

    return cmd;
}

static RedisCmd *redis_build_config_set_cmd(RedisSock *redis_sock, zval *key,
                                            zend_string *val) {
    zend_string *zkey, *zstr;
    RedisCmd *cmd;
    zval *zv;

    /* Legacy case:  CONFIG SET <string> <string> */
    if (key != NULL && val != NULL) {
        cmd = redis_cmd_create_literal(redis_sock, "CONFIG");
        redis_cmd_cat_literal(cmd, "SET");

        zstr = zval_get_string(key);
        redis_cmd_cat_zstr(cmd, zstr);
        zend_string_release(zstr);

        redis_cmd_cat_zstr(cmd, val);

        return cmd;
    }

    /* Now we must have an array with at least one element */
    if (key == NULL || Z_TYPE_P(key) != IS_ARRAY || zend_hash_num_elements(Z_ARRVAL_P(key)) == 0) {
        php_error_docref(NULL, E_WARNING, "Must either pass two strings to CONFIG SET or a non-empty array of values");
        return NULL;
    }

    cmd = redis_cmd_create_literal(redis_sock, "CONFIG");
    redis_cmd_cat_literal(cmd, "SET");

    ZEND_HASH_FOREACH_STR_KEY_VAL(Z_ARRVAL_P(key), zkey, zv) {
        if (zkey == NULL)
            goto fail;

        ZVAL_DEREF(zv);

        redis_cmd_cat_zstr(cmd, zkey);

        zstr = zval_get_string(zv);
        redis_cmd_cat_zstr(cmd, zstr);
        zend_string_release(zstr);
    } ZEND_HASH_FOREACH_END();

    return cmd;

fail:
    php_error_docref(NULL, E_WARNING, "Must pass an associate array of config keys and values");
    redis_cmd_free(cmd);
    return NULL;
}

RedisCmd *
redis_config_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock)
{
    zend_string *op = NULL, *arg = NULL;
    RedisCmd *cmd = NULL;
    void *ctx = NULL;
    zval *key = NULL;

    ZEND_PARSE_PARAMETERS_START(1, 3)
        Z_PARAM_STR(op)
        Z_PARAM_OPTIONAL
        Z_PARAM_ZVAL_OR_NULL(key)
        Z_PARAM_STR_OR_NULL(arg)
    ZEND_PARSE_PARAMETERS_END_EX(return NULL);

    if (zend_string_equals_literal_ci(op, "RESETSTAT") ||
        zend_string_equals_literal_ci(op, "REWRITE"))
    {
        cmd = redis_cmd_create_literal(redis_sock, "CONFIG");
        redis_cmd_cat_zstr(cmd, op);
        ctx = redis_boolean_response;
    } else if (zend_string_equals_literal_ci(op, "GET")) {
        cmd = redis_build_config_get_cmd(redis_sock, key);
        ctx = redis_mbulk_reply_zipped_raw;
    } else if (zend_string_equals_literal_ci(op, "SET")) {
        cmd = redis_build_config_set_cmd(redis_sock, key, arg);
        ctx = redis_boolean_response;
    } else {
        php_error_docref(NULL, E_WARNING, "Unknown operation '%s'", ZSTR_VAL(op));
        return NULL;
    }

    if (cmd)
        redis_cmd_set_ctx(cmd, ctx);

    return cmd;
}

RedisCmd *
redis_function_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock)
{
    zend_string *op = NULL, *arg;
    zval *argv = NULL;
    RedisCmd *cmd;
    void *ctx = NULL;
    int i, argc = 0;

    ZEND_PARSE_PARAMETERS_START(1, -1)
        Z_PARAM_STR(op)
        Z_PARAM_OPTIONAL
        Z_PARAM_VARIADIC('*', argv, argc)
    ZEND_PARSE_PARAMETERS_END_EX(return NULL);

    for (i = 0; i < argc; ++i) {
        if (Z_TYPE(argv[i]) != IS_STRING) {
            php_error_docref(NULL, E_WARNING, "invalid argument");
            return NULL;
        }
    }

    if (zend_string_equals_literal_ci(op, "DELETE")) {
        if (argc < 1) {
            php_error_docref(NULL, E_WARNING, "argument required");
            return NULL;
        }
    } else if (zend_string_equals_literal_ci(op, "DUMP")) {
        ctx = PHPREDIS_CTX_PTR;
    } else if (zend_string_equals_literal_ci(op, "FLUSH")) {
        if (argc > 0 &&
            !zend_string_equals_literal_ci(Z_STR(argv[0]), "SYNC") &&
            !zend_string_equals_literal_ci(Z_STR(argv[0]), "ASYNC")
        ) {
            php_error_docref(NULL, E_WARNING, "invalid argument");
            return NULL;
        }
    } else if (zend_string_equals_literal_ci(op, "KILL")) {
        // noop
    } else if (zend_string_equals_literal_ci(op, "LIST")) {
        if (argc > 0) {
            if (zend_string_equals_literal_ci(Z_STR(argv[0]), "LIBRARYNAME")) {
                if (argc < 2) {
                    php_error_docref(NULL, E_WARNING, "argument required");
                    return NULL;
                }
            } else if (!zend_string_equals_literal_ci(Z_STR(argv[0]), "WITHCODE")) {
                php_error_docref(NULL, E_WARNING, "invalid argument");
                return NULL;
            }
        }
        ctx = PHPREDIS_CTX_PTR + 1;
    } else if (zend_string_equals_literal_ci(op, "LOAD")) {
        if (argc < 1 || (
            zend_string_equals_literal_ci(Z_STR(argv[0]), "REPLACE") && argc < 2
        )) {
            php_error_docref(NULL, E_WARNING, "argument required");
            return NULL;
        }
        ctx = PHPREDIS_CTX_PTR;
    } else if (zend_string_equals_literal_ci(op, "RESTORE")) {
        if (argc < 1 || (
            argc > 1 &&
            !zend_string_equals_literal_ci(Z_STR(argv[1]), "FLUSH") &&
            !zend_string_equals_literal_ci(Z_STR(argv[1]), "APPEND") &&
            !zend_string_equals_literal_ci(Z_STR(argv[1]), "REPLACE")
        )) {
            php_error_docref(NULL, E_WARNING, "invalid argument");
            return NULL;
        }
    } else if (zend_string_equals_literal_ci(op, "STATS")) {
        ctx = PHPREDIS_CTX_PTR + 1;
    } else {
        php_error_docref(NULL, E_WARNING, "Unknown operation '%s'", ZSTR_VAL(op));
        return NULL;
    }

    cmd = redis_cmd_create_literal(redis_sock, "FUNCTION");
    redis_cmd_cat_zstr(cmd, op);

    for (i = 0; i < argc; i++) {
        arg = zval_get_string(&argv[i]);
        redis_cmd_cat_zstr(cmd, arg);
        zend_string_release(arg);
    }

    redis_cmd_set_ctx(cmd, ctx);

    return cmd;
}

RedisCmd *
redis_fcall_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock, const char *kw, size_t kw_len)
{
    HashTable *keys = NULL, *args = NULL;
    zend_string *fn = NULL;
    RedisCmd *cmd;
    zval *zv;

    ZEND_PARSE_PARAMETERS_START(1, 3)
        Z_PARAM_STR(fn)
        Z_PARAM_OPTIONAL
        Z_PARAM_ARRAY_HT(keys)
        Z_PARAM_ARRAY_HT(args)
    ZEND_PARSE_PARAMETERS_END_EX(return NULL);

    cmd = redis_cmd_create(redis_sock, kw, kw_len);
    redis_cmd_cat_zstr(cmd, fn);
    redis_cmd_cat_long(cmd, keys ? zend_hash_num_elements(keys) : 0);

    if (keys != NULL) {
        ZEND_HASH_FOREACH_VAL(keys, zv) {
            redis_cmd_try_cat_key_zval(cmd, zv);
        } ZEND_HASH_FOREACH_END();
    }

    if (args != NULL) {
        ZEND_HASH_FOREACH_VAL(args, zv) {
            redis_cmd_cat_zval(cmd, zv);
        } ZEND_HASH_FOREACH_END();
    }

    return cmd;
}

RedisCmd *
redis_zrandmember_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock)
{
    zend_bool withscores = 0;
    zend_string *key, *zstr;
    HashTable *opts = NULL;
    RedisCmd *cmd;
    int count = 0;
    zval *z_ele;

    ZEND_PARSE_PARAMETERS_START(1, 2)
        Z_PARAM_STR(key)
        Z_PARAM_OPTIONAL
        Z_PARAM_ARRAY_HT(opts)
    ZEND_PARSE_PARAMETERS_END_EX(return NULL);

    if (opts != NULL) {
        ZEND_HASH_FOREACH_STR_KEY_VAL(opts, zstr, z_ele) {
            if (zstr != NULL) {
                ZVAL_DEREF(z_ele);
                if (zend_string_equals_literal_ci(zstr, "count")) {
                    count = zval_get_long(z_ele);
                } else if (zend_string_equals_literal_ci(zstr, "withscores")) {
                    withscores = zend_is_true(z_ele);
                }
            }
        } ZEND_HASH_FOREACH_END();
    }

    cmd = redis_cmd_create_literal(redis_sock, "ZRANDMEMBER");
    redis_cmd_cat_key_zstr(cmd, key);

    if (count != 0) {
        redis_cmd_cat_long(cmd, count);
        redis_cmd_set_ctx(cmd, PHPREDIS_CTX_PTR);
    }

    if (withscores) {
        redis_cmd_cat_literal(cmd, "WITHSCORES");
        redis_cmd_set_ctx(cmd, PHPREDIS_CTX_PTR + 1);
    }

    return cmd;
}

RedisCmd *
redis_zdiff_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock)
{
    zval *z_keys, *z_opts = NULL, *z_key;
    redisZcmdOptions opts = {0};
    RedisCmd *cmd;
    int numkeys, flags;

    ZEND_PARSE_PARAMETERS_START(1, 2)
        Z_PARAM_ARRAY(z_keys)
        Z_PARAM_OPTIONAL
        Z_PARAM_ARRAY(z_opts)
    ZEND_PARSE_PARAMETERS_END_EX(return NULL);

    if ((numkeys = zend_hash_num_elements(Z_ARRVAL_P(z_keys))) == 0) {
        return NULL;
    }

    flags = redis_get_zcmd_flags(ZEND_STRL("ZDIFF"));
    redis_get_zcmd_options(&opts, z_opts, flags);

    cmd = redis_cmd_create_literal(redis_sock, "ZDIFF");
    redis_cmd_cat_long(cmd, numkeys);

    ZEND_HASH_FOREACH_VAL(Z_ARRVAL_P(z_keys), z_key) {
        ZVAL_DEREF(z_key);
        redis_cmd_try_cat_key_zval(cmd, z_key);
    } ZEND_HASH_FOREACH_END();

    if (opts.withscores) {
        redis_cmd_cat_literal(cmd, "WITHSCORES");
        redis_cmd_set_ctx(cmd, PHPREDIS_CTX_PTR);
    }

    return cmd;
}

static int redis_cmd_cat_score(RedisCmd *cmd, zval *score) {
    zend_uchar type;
    zend_long lval;
    uint32_t argc;
    double dval;

    /* Get current command length */
    argc = cmd->argc;

    if (Z_TYPE_P(score) == IS_LONG) {
        redis_cmd_cat_long(cmd, Z_LVAL_P(score));
    } else if (Z_TYPE_P(score) == IS_DOUBLE) {
        redis_cmd_cat_double(cmd, Z_DVAL_P(score));
    } else if (Z_TYPE_P(score) == IS_STRING) {
        type = is_numeric_string(Z_STRVAL_P(score), Z_STRLEN_P(score), &lval, &dval, 0);
        if (type == IS_LONG) {
            redis_cmd_cat_long(cmd, lval);
        } else if (type == IS_DOUBLE) {
            redis_cmd_cat_double(cmd, dval);
        } else if (zend_string_equals_literal_ci(Z_STR_P(score), "-inf") ||
                   zend_string_equals_literal_ci(Z_STR_P(score), "+inf") ||
                   zend_string_equals_literal_ci(Z_STR_P(score), "inf"))
        {
            redis_cmd_cat_zstr(cmd, Z_STR_P(score));
        }
    }

    /* Success if we appended something */
    if (cmd->argc > argc)
        return SUCCESS;

    /* Nothing appended, failure */
    php_error_docref(NULL, E_WARNING, "scores must be numeric or '-inf', 'inf', '+inf'");
    return FAILURE;
}

RedisCmd *redis_intercard_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock, const char *kw, size_t kw_len)
{
    zend_long limit = -1;
    HashTable *keys;
    RedisCmd *cmd;
    zval *zv;

    ZEND_PARSE_PARAMETERS_START(1, 2)
        Z_PARAM_ARRAY_HT(keys)
        Z_PARAM_OPTIONAL
        Z_PARAM_LONG(limit)
    ZEND_PARSE_PARAMETERS_END_EX(return NULL);

    if (zend_hash_num_elements(keys) == 0) {
        php_error_docref(NULL, E_WARNING, "Must pass at least one key");
        return NULL;
    } else if (ZEND_NUM_ARGS() == 2 && limit < 0) {
        php_error_docref(NULL, E_WARNING, "LIMIT cannot be negative");
        return NULL;
    }

    cmd = redis_cmd_create(redis_sock, kw, kw_len);
    redis_cmd_cat_long(cmd, zend_hash_num_elements(keys));

    ZEND_HASH_FOREACH_VAL(keys, zv) {
        redis_cmd_try_cat_key_zval(cmd, zv);
    } ZEND_HASH_FOREACH_END();

    if (limit > 0) {
        redis_cmd_cat_literal(cmd, "LIMIT");
        redis_cmd_cat_long(cmd, limit);
    }

    return cmd;
}

RedisCmd *redis_replicaof_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock, const char *kw, size_t kw_len)
{
    zend_string *host = NULL;
    zend_long port = 6379;
    RedisCmd *cmd;

    ZEND_PARSE_PARAMETERS_START(0, 2)
        Z_PARAM_OPTIONAL
        Z_PARAM_STR(host)
        Z_PARAM_LONG(port)
    ZEND_PARSE_PARAMETERS_END_EX(return NULL);

    if (port < 0 || port > UINT16_MAX) {
        php_error_docref(NULL, E_WARNING, "Invalid port %ld", (long)port);
        return NULL;
    }

    cmd = redis_cmd_create(redis_sock, kw, kw_len);

    if (ZEND_NUM_ARGS() == 2) {
        redis_cmd_cat_zstr(cmd, host);
        redis_cmd_cat_long(cmd, port);
    } else {
        redis_cmd_cat_literal(cmd, "NO");
        redis_cmd_cat_literal(cmd, "ONE");
    }

    return cmd;
}

RedisCmd *
redis_zinterunion_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock, const char *kw, size_t kw_len)
{
    zval *z_keys, *z_weights = NULL, *z_opts = NULL, *z_ele;
    redisZcmdOptions opts = {0};
    RedisCmd *cmd;
    int numkeys, flags;

    ZEND_PARSE_PARAMETERS_START(1, 3)
        Z_PARAM_ARRAY(z_keys)
        Z_PARAM_OPTIONAL
        Z_PARAM_ARRAY_OR_NULL(z_weights)
        Z_PARAM_ARRAY(z_opts)
    ZEND_PARSE_PARAMETERS_END_EX(return NULL);

    if ((numkeys = zend_hash_num_elements(Z_ARRVAL_P(z_keys))) == 0) {
        return NULL;
    }

    if (z_weights && zend_hash_num_elements(Z_ARRVAL_P(z_weights)) != numkeys) {
        php_error_docref(NULL, E_WARNING, "WEIGHTS and keys array should be the same size!");
        return NULL;
    }

    flags = redis_get_zcmd_flags(kw, kw_len);
    redis_get_zcmd_options(&opts, z_opts, flags);

    cmd = redis_cmd_create(redis_sock, kw, kw_len);
    redis_cmd_cat_long(cmd, numkeys);

    ZEND_HASH_FOREACH_VAL(Z_ARRVAL_P(z_keys), z_ele) {
        ZVAL_DEREF(z_ele);
        redis_cmd_try_cat_key_zval(cmd, z_ele);
    } ZEND_HASH_FOREACH_END();

    if (z_weights) {
        redis_cmd_cat_literal(cmd, "WEIGHTS");
        ZEND_HASH_FOREACH_VAL(Z_ARRVAL_P(z_weights), z_ele) {
            ZVAL_DEREF(z_ele);
            if (redis_cmd_cat_score(cmd, z_ele) == FAILURE) {
                redis_cmd_free(cmd);
                return NULL;
            }
        } ZEND_HASH_FOREACH_END();
    }

    if (opts.aggregate) {
        redis_cmd_cat_literal(cmd, "AGGREGATE");
        redis_cmd_cat_zstr(cmd, opts.aggregate);
    }

    if (opts.withscores) {
        redis_cmd_cat_literal(cmd, "WITHSCORES");
        redis_cmd_set_ctx(cmd, PHPREDIS_CTX_PTR);
    }

    return cmd;
}

RedisCmd *
redis_zdiffstore_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock)
{
    zend_string *dst = NULL;
    HashTable *keys = NULL;
    zend_ulong nkeys;
    RedisCmd *cmd;
    zval *zkey;

    ZEND_PARSE_PARAMETERS_START(2, 2)
        Z_PARAM_STR(dst)
        Z_PARAM_ARRAY_HT(keys)
    ZEND_PARSE_PARAMETERS_END_EX(return NULL);

    nkeys = zend_hash_num_elements(keys);
    if (nkeys == 0)
        return NULL;

    cmd = redis_cmd_create_literal(redis_sock, "ZDIFFSTORE");
    redis_cmd_cat_key_zstr(cmd, dst);
    redis_cmd_cat_long(cmd, nkeys);

    ZEND_HASH_FOREACH_VAL(keys, zkey) {
        ZVAL_DEREF(zkey);
        redis_cmd_try_cat_key_zval(cmd, zkey);
    } ZEND_HASH_FOREACH_END();

    return cmd;
}

/* ZUNIONSTORE, ZINTERSTORE */
RedisCmd *
redis_zinterunionstore_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock, const char *kw, size_t kw_len)
{
    HashTable *keys = NULL, *weights = NULL;
    zend_string *dst = NULL;
    zend_string *agg = NULL;
    zend_ulong nkeys;
    zval *zv = NULL;
    RedisCmd *cmd;

    ZEND_PARSE_PARAMETERS_START(2, 4)
        Z_PARAM_STR(dst)
        Z_PARAM_ARRAY_HT(keys)
        Z_PARAM_OPTIONAL
        Z_PARAM_ARRAY_HT_OR_NULL(weights)
        Z_PARAM_STR_OR_NULL(agg)
    ZEND_PARSE_PARAMETERS_END_EX(return NULL);

    nkeys = zend_hash_num_elements(keys);
    if (nkeys == 0)
        return NULL;

    if (weights != NULL && zend_hash_num_elements(weights) != nkeys) {
        php_error_docref(NULL, E_WARNING, "WEIGHTS and keys array must be the same size!");
        return NULL;
    }

    // AGGREGATE option
    if (agg != NULL && (!zend_string_equals_literal_ci(agg, "SUM") &&
                        !zend_string_equals_literal_ci(agg, "MIN") &&
                        !zend_string_equals_literal_ci(agg, "MAX")))
    {
        php_error_docref(NULL, E_WARNING, "AGGREGATE option must be 'SUM', 'MIN', or 'MAX'");
        return NULL;
    }

    cmd = redis_cmd_create(redis_sock, kw, kw_len);
    redis_cmd_cat_key_zstr(cmd, dst);
    redis_cmd_cat_long(cmd, nkeys);

    ZEND_HASH_FOREACH_VAL(keys, zv) {
        ZVAL_DEREF(zv);
        redis_cmd_try_cat_key_zval(cmd, zv);
    } ZEND_HASH_FOREACH_END();

    if (weights) {
        redis_cmd_cat_literal(cmd, "WEIGHTS");
        ZEND_HASH_FOREACH_VAL(weights, zv) {
            ZVAL_DEREF(zv);
            if (redis_cmd_cat_score(cmd, zv) == FAILURE) {
                redis_cmd_free(cmd);
                return NULL;
            }
        } ZEND_HASH_FOREACH_END();
    }

    if (agg) {
        redis_cmd_cat_literal(cmd, "AGGREGATE");
        redis_cmd_cat_zstr(cmd, agg);
    }

    return cmd;
}

RedisCmd *redis_pubsub_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock)
{
    HashTable *channels = NULL;
    zend_string *op, *pattern = NULL;
    zval *arg = NULL, *z_chan;
    RedisCmd *cmd;
    void *ctx = NULL;

    ZEND_PARSE_PARAMETERS_START(1, 2)
        Z_PARAM_STR(op)
        Z_PARAM_OPTIONAL
        Z_PARAM_ZVAL(arg)
    ZEND_PARSE_PARAMETERS_END_EX(return NULL);

    if (zend_string_equals_literal_ci(op, "NUMPAT")) {
        ctx = NULL;
    } else if (zend_string_equals_literal_ci(op, "CHANNELS") ||
        zend_string_equals_literal_ci(op, "SHARDCHANNELS")
    ) {
        if (arg != NULL) {
            if (Z_TYPE_P(arg) != IS_STRING) {
                php_error_docref(NULL, E_WARNING, "Invalid pattern value");
                return NULL;
            }
            pattern = zval_get_string(arg);
        }
        ctx = PHPREDIS_CTX_PTR;
    } else if (zend_string_equals_literal_ci(op, "NUMSUB") ||
        zend_string_equals_literal_ci(op, "SHARDNUMSUB")
    ) {
        if (arg != NULL) {
            if (Z_TYPE_P(arg) != IS_ARRAY) {
                php_error_docref(NULL, E_WARNING, "Invalid channels value");
                return NULL;
            }
            channels = Z_ARRVAL_P(arg);
        }
        ctx = PHPREDIS_CTX_PTR + 1;
    } else {
        php_error_docref(NULL, E_WARNING, "Unknown PUBSUB operation '%s'", ZSTR_VAL(op));
        return NULL;
    }

    cmd = redis_cmd_create_literal(redis_sock, "PUBSUB");
    redis_cmd_cat_zstr(cmd, op);

    if (pattern != NULL) {
        redis_cmd_cat_zstr(cmd, pattern);
        zend_string_release(pattern);
    } else if (channels != NULL) {
        ZEND_HASH_FOREACH_VAL(channels, z_chan) {
            /* Prefix but don't cross slot protect */
            redis_cmd_cat_channel_zval(cmd, z_chan);
        } ZEND_HASH_FOREACH_END();
    }

    redis_cmd_set_ctx(cmd, ctx);

    return cmd;
}

/* SUBSCRIBE/PSUBSCRIBE/SSUBSCRIBE */
RedisCmd *
redis_subscribe_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock, const char *kw, size_t kw_len)
{
    zval *z_arr, *z_chan;
    HashTable *ht_chan;
    unsigned short shardslot = REDIS_CLUSTER_SLOTS;
    RedisCmd *cmd;
    subscribeContext *sctx;
    zend_fcall_info_cache fcc;
    zend_fcall_info fci;

    ZEND_PARSE_PARAMETERS_START(2, 2)
        Z_PARAM_ARRAY(z_arr)
        Z_PARAM_FUNC(fci, fcc)
    ZEND_PARSE_PARAMETERS_END_EX(return NULL);

    ht_chan = Z_ARRVAL_P(z_arr);
    if (zend_hash_num_elements(ht_chan) == 0)
        return NULL;

    sctx = ecalloc(1, sizeof(*sctx));
    sctx->kw = kw;
    sctx->cb.fci = fci;
    sctx->cb.fci_cache = fcc;
    sctx->argc = zend_hash_num_elements(ht_chan);

    if (strcasecmp(kw, "ssubscribe") == 0) {
        zend_hash_internal_pointer_reset(ht_chan);
        if ((z_chan = zend_hash_get_current_data(ht_chan)) == NULL) {
            php_error_docref(NULL, E_WARNING, "Internal Zend HashTable error");
            efree(sctx);
            return NULL;
        }
        shardslot = cluster_hash_key_zval(z_chan);
    }

    // Start command construction
    cmd = redis_cmd_create(redis_sock, kw, kw_len);

    // Iterate over channels
    ZEND_HASH_FOREACH_VAL(ht_chan, z_chan) {
        if (!redis_cmd_cat_key_zval(cmd, z_chan) ||
            (shardslot != REDIS_CLUSTER_SLOTS && cmd->slot != shardslot))
        {
            php_error_docref(NULL, E_WARNING, "All shard channels needs to belong to a single slot");
            redis_cmd_free(cmd);
            efree(sctx);
            return NULL;
        }
    } ZEND_HASH_FOREACH_END();

    redis_cmd_set_ctx_ex(cmd, sctx, redis_cmd_ctx_efree);

    if (shardslot != REDIS_CLUSTER_SLOTS) {
        cmd->slot = shardslot;
    } else {
        // Pick a slot at random
        redis_cmd_randslot(cmd);
    }

    return cmd;
}

/* UNSUBSCRIBE/PUNSUBSCRIBE/SUNSUBSCRIBE */
RedisCmd *redis_unsubscribe_cmd(INTERNAL_FUNCTION_PARAMETERS,
                                RedisSock *redis_sock, const char *kw, size_t kw_len)
{
    subscribeContext *sctx;
    HashTable *channels;
    zval *channel;
    RedisCmd *cmd;

    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_ARRAY_HT(channels)
    ZEND_PARSE_PARAMETERS_END_EX(return NULL);

    if (zend_hash_num_elements(channels) == 0)
        return NULL;

    sctx = ecalloc(1, sizeof(*sctx));
    sctx->kw = kw;
    sctx->argc = zend_hash_num_elements(channels);

    cmd = redis_cmd_create(redis_sock, kw, kw_len);
    redis_cmd_set_ctx_ex(cmd, sctx, redis_cmd_ctx_efree);

    ZEND_HASH_FOREACH_VAL(channels, channel) {
        /* Prefix but don't cross slot protect */
        redis_cmd_cat_channel_zval(cmd, channel);
    } ZEND_HASH_FOREACH_END();

    return cmd;
}

/* ZRANGEBYLEX/ZREVRANGEBYLEX */
RedisCmd *redis_zrangebylex_cmd(INTERNAL_FUNCTION_PARAMETERS,
                                RedisSock *redis_sock, const char *kw, size_t kw_len)
{
    zend_string *key, *min, *max;
    int argc = ZEND_NUM_ARGS();
    zend_long offset, count;
    RedisCmd *cmd;

    /* We need either 3 or 5 arguments for this to be valid */
    if (argc != 3 && argc != 5) {
        php_error_docref(0, E_WARNING, "Must pass either 3 or 5 arguments");
        return NULL;
    }

    ZEND_PARSE_PARAMETERS_START(3, 5)
        Z_PARAM_STR(key)
        Z_PARAM_STR(min)
        Z_PARAM_STR(max)
        Z_PARAM_OPTIONAL
        Z_PARAM_LONG(offset)
        Z_PARAM_LONG(count)
    ZEND_PARSE_PARAMETERS_END_EX(return NULL);

    /* min and max must start with '(' or '[', or be either '-' or '+' */
    if (!validate_zlex_arg_zstr(min) || !validate_zlex_arg_zstr(max)) {
        php_error_docref(NULL, E_WARNING,
            "Min/Max args can be '-' or '+', or start with '[' or '('");
        return NULL;
    }

    cmd = redis_cmd_fmt_ex(redis_sock, kw, kw_len, "KSS", key, min, max);

    if (argc == 5) {
        redis_cmd_cat_literal(cmd, "LIMIT");
        redis_cmd_cat_long(cmd, offset);
        redis_cmd_cat_long(cmd, count);
    }

    return cmd;
}

/* ZLEXCOUNT/ZREMRANGEBYLEX */
RedisCmd *redis_gen_zlex_cmd(INTERNAL_FUNCTION_PARAMETERS,
                             RedisSock *redis_sock, const char *kw, size_t kw_len)
{
    zend_string *key, *min, *max;

    ZEND_PARSE_PARAMETERS_START(3, 3)
        Z_PARAM_STR(key)
        Z_PARAM_STR(min)
        Z_PARAM_STR(max)
    ZEND_PARSE_PARAMETERS_END_EX(return NULL);

    /* Quick sanity check on min/max */
    if (!validate_zlex_arg_zstr(min) || !validate_zlex_arg_zstr(max)) {
        php_error_docref(NULL, E_WARNING,
            "Min/Max args can be '-' or '+', or start with '[' or '('");
        return NULL;
    }

    /* Construct command */
    return redis_cmd_fmt_ex(redis_sock, kw, kw_len, "KSS", key, min, max);
}

/* EVAL and EVALSHA */
RedisCmd *redis_eval_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock, const char *kw, size_t kw_len)
{
    zval *zv;
    HashTable *ht = NULL;
    zend_long num_keys = 0;
    zend_string *lua;
    RedisCmd *cmd;

    ZEND_PARSE_PARAMETERS_START(1, 3)
        Z_PARAM_STR(lua)
        Z_PARAM_OPTIONAL
        Z_PARAM_ARRAY_HT_OR_NULL(ht)
        Z_PARAM_LONG(num_keys)
    ZEND_PARSE_PARAMETERS_END_EX(return NULL);

    if (ht == NULL)
        ht = (HashTable*)&zend_empty_array;

    /* EVAL[SHA] {script || sha1} {num keys}  */
    cmd = redis_cmd_create(redis_sock, kw, kw_len);
    redis_cmd_cat_zstr(cmd, lua);
    redis_cmd_cat_long(cmd, num_keys);

    /* Pick a random slot if there are no keys */
    if (num_keys == 0)
        redis_cmd_randslot(cmd);

    ZEND_HASH_FOREACH_VAL(ht, zv) {
        if (num_keys-- > 0) {
            redis_cmd_try_cat_key_zval(cmd, zv);
        } else {
            redis_cmd_cat_zval_zstr(cmd, zv);
        }
    } ZEND_HASH_FOREACH_END();

    return cmd;
}

/* Commands that take a key followed by a variable list of serializable
 * values (RPUSH, LPUSH, SADD, SREM, etc...) */
RedisCmd *redis_key_varval_cmd(INTERNAL_FUNCTION_PARAMETERS,
                               RedisSock *redis_sock, const char *kw, size_t kw_len)
{
    zval *args = NULL;
    zend_string *key = NULL;
    RedisCmd *cmd;
    size_t i;
    int argc = 0;

    ZEND_PARSE_PARAMETERS_START(2, -1)
        Z_PARAM_STR(key)
        Z_PARAM_VARIADIC('*', args, argc)
    ZEND_PARSE_PARAMETERS_END_EX(return NULL);

    /* Initialize our command */
    cmd = redis_cmd_create(redis_sock, kw, kw_len);

    /* Append key */
    redis_cmd_cat_key_zstr(cmd, key);

    /* Add members */
    for (i = 0; i < argc; i++) {
        redis_cmd_cat_zval(cmd, &args[i]);
    }

    return cmd;
}

/* Commands that take a key and then an array of values */
static RedisCmd *gen_key_arr_cmd(INTERNAL_FUNCTION_PARAMETERS,
                                 RedisSock *redis_sock, const char *kw, size_t kw_len,
                                 zend_bool pack_values)
{
    HashTable *values = NULL;
    zend_string *key = NULL;
    RedisCmd *cmd;
    zval *zv;

    ZEND_PARSE_PARAMETERS_START(2, 2)
        Z_PARAM_STR(key)
        Z_PARAM_ARRAY_HT(values)
    ZEND_PARSE_PARAMETERS_END_EX(return NULL);

    if (zend_hash_num_elements(values) == 0)
        return NULL;

    cmd = redis_cmd_create(redis_sock, kw, kw_len);
    redis_cmd_cat_key_zstr(cmd, key);

    ZEND_HASH_FOREACH_VAL(values, zv) {
        if (pack_values) {
            redis_cmd_cat_zval(cmd, zv);
        } else {
            redis_cmd_cat_zval_zstr(cmd, zv);
        }
    } ZEND_HASH_FOREACH_END();

    return cmd;
}

RedisCmd *redis_key_val_arr_cmd(INTERNAL_FUNCTION_PARAMETERS,
                                RedisSock *redis_sock, const char *kw, size_t kw_len)
{
    return gen_key_arr_cmd(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock, kw, kw_len, 1);
}

RedisCmd *redis_key_str_arr_cmd(INTERNAL_FUNCTION_PARAMETERS,
                                RedisSock *redis_sock, const char *kw, size_t kw_len)
{
    return gen_key_arr_cmd(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock, kw, kw_len, 0);
}

/* Generic function that takes one or more non-serialized arguments */
RedisCmd *
gen_vararg_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock,
               uint32_t min_argc, const char *kw, size_t kw_len)
{
    zval *argv = NULL;
    RedisCmd *cmd;
    int argc = 0;
    uint32_t i;

    ZEND_PARSE_PARAMETERS_START(min_argc, -1)
        Z_PARAM_VARIADIC('*', argv, argc)
    ZEND_PARSE_PARAMETERS_END_EX(return NULL);

    cmd = redis_cmd_create(redis_sock, kw, kw_len);

    for (i = 0; i < argc; i++) {
        redis_cmd_cat_zval_zstr(cmd, &argv[i]);
    }

    return cmd;
}

static zend_bool
redis_cmd_cat_mset_kvals(RedisCmd *cmd, HashTable *kvals)
{
    zend_string *key;
    zend_ulong idx;
    zval *zv;

    ZEND_HASH_FOREACH_KEY_VAL(kvals, idx, key, zv) {
        ZVAL_DEREF(zv);
        if (key) {
            if (!redis_cmd_cat_key_zstr(cmd, key))
                return 0;
        } else {
            if (!redis_cmd_cat_key_long(cmd, idx))
                return 0;
        }
        redis_cmd_cat_zval(cmd, zv);
    } ZEND_HASH_FOREACH_END();

    return 1;
}

RedisCmd *redis_mset_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock, const char *kw, size_t kw_len)
{
    HashTable *kvals = NULL;
    RedisCmd *cmd;

    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_ARRAY_HT(kvals)
    ZEND_PARSE_PARAMETERS_END_EX(return NULL);

    if (zend_hash_num_elements(kvals) == 0)
        return NULL;

    cmd = redis_cmd_create(redis_sock, kw, kw_len);

    redis_cmd_try_cat_key(cmd, redis_cmd_cat_mset_kvals, kvals);

    return cmd;
}

/* Generic function that takes a variable number of keys, with an optional
 * timeout value.  This can handle various SUNION/SUNIONSTORE/BRPOP type
 * commands. */
static RedisCmd *
gen_varkey_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock,
               const char *kw, size_t kw_len, zend_bool has_timeout)

{
    zval *argv = NULL, ztimeout = {0}, *zv;
    uint32_t min_argc;
    int single_array;
    RedisCmd *cmd;
    int argc = 0;

    min_argc = has_timeout ? 2 : 1;

    ZEND_PARSE_PARAMETERS_START(min_argc, -1)
        Z_PARAM_VARIADIC('*', argv, argc)
    ZEND_PARSE_PARAMETERS_END_EX(return NULL);

    single_array = argc == min_argc && Z_TYPE(argv[0]) == IS_ARRAY;

    if (has_timeout) {
        if (single_array)
            ZVAL_COPY_VALUE(&ztimeout, &argv[1]);
        else
            ZVAL_COPY_VALUE(&ztimeout, &argv[argc - 1]);

        if (Z_TYPE(ztimeout) != IS_LONG && Z_TYPE(ztimeout) != IS_DOUBLE) {
            php_error_docref(NULL, E_WARNING, "Timeout must be a long or double");
            return NULL;
        }
    }

    // If we're running a single array, rework args
    if (single_array) {
        /* Need at least one argument */
        argc = zend_hash_num_elements(Z_ARRVAL(argv[0]));
        if (argc == 0)
            return NULL;

        if (has_timeout) argc++;
    }

    // Begin construction of our command
    cmd = redis_cmd_create(redis_sock, kw, kw_len);

    if (single_array) {
        ZEND_HASH_FOREACH_VAL(Z_ARRVAL(argv[0]), zv) {
            redis_cmd_try_cat_key_zval(cmd, zv);
        } ZEND_HASH_FOREACH_END();
    } else {
        for (uint32_t i = 0; i < argc - !!has_timeout; i++) {
            redis_cmd_try_cat_key_zval(cmd, &argv[i]);
        }
    }

    if (Z_TYPE(ztimeout) == IS_DOUBLE) {
        redis_cmd_cat_double(cmd, Z_DVAL(ztimeout));
    } else if (Z_TYPE(ztimeout) == IS_LONG) {
        redis_cmd_cat_long(cmd, Z_LVAL(ztimeout));
    }

    return cmd;
}

RedisCmd *
redis_mpop_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock, const char *kw, size_t kw_len) {
    zend_string *from = NULL;
    HashTable *keys = NULL;
    int blocking, is_zmpop;
    double timeout = 0.0;
    zend_long count = 1;
    RedisCmd *cmd;
    zval *zv;

    /* Sanity check on our keyword */
    ZEND_ASSERT(kw != NULL && *kw != '\0' && *(kw+1) != '\0');

    blocking = tolower(*kw) == 'b';
    is_zmpop = tolower(kw[blocking]) == 'z';

    ZEND_PARSE_PARAMETERS_START(2 + blocking, 3 + blocking) {
        if (blocking) {
            Z_PARAM_DOUBLE(timeout)
        }
        Z_PARAM_ARRAY_HT(keys)
        Z_PARAM_STR(from);
        Z_PARAM_OPTIONAL
        Z_PARAM_LONG(count);
    } ZEND_PARSE_PARAMETERS_END_EX(return NULL);

    if (zend_hash_num_elements(keys) == 0) {
        php_error_docref(NULL, E_WARNING, "Must pass at least one key");
        return NULL;
    } else if (count < 1) {
        php_error_docref(NULL, E_WARNING, "Count must be > 0");
        return NULL;
    } else if (!is_zmpop && !(zend_string_equals_literal_ci(from, "LEFT") ||
                              zend_string_equals_literal_ci(from, "RIGHT")))
    {
        php_error_docref(NULL, E_WARNING, "from must be either 'LEFT' or 'RIGHT'");
        return NULL;
    } else if (is_zmpop && !(zend_string_equals_literal_ci(from, "MIN") ||
                             zend_string_equals_literal_ci(from, "MAX")))
    {
        php_error_docref(NULL, E_WARNING, "from must be either 'MIN' or 'MAX'");
        return NULL;
    }

    cmd = redis_cmd_create(redis_sock, kw, kw_len);

    if (blocking)
        redis_cmd_cat_double(cmd, timeout);
    redis_cmd_cat_long(cmd, zend_hash_num_elements(keys));

    ZEND_HASH_FOREACH_VAL(keys, zv) {
        redis_cmd_try_cat_key_zval(cmd, zv);
    } ZEND_HASH_FOREACH_END();

    redis_cmd_cat_zstr(cmd, from);

    if (count != 1) {
        redis_cmd_cat_literal(cmd, "COUNT");
        redis_cmd_cat_long(cmd, count);
    }

    redis_cmd_set_ctx(cmd, is_zmpop ? PHPREDIS_CTX_PTR : NULL);

    return cmd;
}

RedisCmd *redis_info_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock) {
    return gen_vararg_cmd(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock, 0,
                          ZEND_STRL("INFO"));
}

RedisCmd *redis_script_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock) {
    zval *argv = NULL;
    int argc = 0;

    ZEND_PARSE_PARAMETERS_START(1, -1)
        Z_PARAM_VARIADIC('*', argv, argc)
    ZEND_PARSE_PARAMETERS_END_EX(return NULL);

    return redis_build_script_cmd(argc, argv);
}

/* Generic handling of every blocking pop command (BLPOP, BZPOP[MIN/MAX], etc */
RedisCmd *
redis_blocking_pop_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock, const char *kw, size_t kw_len)
{
    return gen_varkey_cmd(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock, kw,
        kw_len, 1);
}

/*
 * Commands with specific signatures or that need unique functions because they
 * have specific processing (argument validation, etc) that make them unique
 */

RedisCmd *
redis_pop_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock, const char *kw, size_t kw_len)
{
    zend_string *key;
    zend_long count = 0;
    RedisCmd *cmd;

    ZEND_PARSE_PARAMETERS_START(1, 2)
        Z_PARAM_STR(key)
        Z_PARAM_OPTIONAL
        Z_PARAM_LONG(count)
    ZEND_PARSE_PARAMETERS_END_EX(return NULL);

    cmd = redis_cmd_create(redis_sock, kw, kw_len);
    redis_cmd_cat_key_zstr(cmd, key);
    if (count > 0) {
        redis_cmd_cat_long(cmd, count);
        redis_cmd_set_ctx(cmd, PHPREDIS_CTX_PTR);
    }

    return cmd;
}

RedisCmd *
redis_acl_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock) {
    zend_string *op, *zstr;
    zval *z_args = NULL;
    RedisCmd *cmd;
    void *ctx = NULL;
    int argc = 0, i;

    ZEND_PARSE_PARAMETERS_START(1, -1)
        Z_PARAM_STR(op)
        Z_PARAM_OPTIONAL
        Z_PARAM_VARIADIC('*', z_args, argc)
    ZEND_PARSE_PARAMETERS_END_EX(return NULL);

    if (zend_string_equals_literal_ci(op, "CAT") ||
        zend_string_equals_literal_ci(op, "LIST") ||
        zend_string_equals_literal_ci(op, "USERS")
    ) {
        ctx = NULL;
    } else if (zend_string_equals_literal_ci(op, "LOAD") ||
        zend_string_equals_literal_ci(op, "SAVE")
    ) {
        ctx = PHPREDIS_CTX_PTR;
    } else if (zend_string_equals_literal_ci(op, "GENPASS") ||
        zend_string_equals_literal_ci(op, "WHOAMI")
    ) {
        ctx = PHPREDIS_CTX_PTR + 1;
    } else if (zend_string_equals_literal_ci(op, "SETUSER")) {
        if (argc < 1) {
            php_error_docref(NULL, E_WARNING, "ACL SETUSER requires at least one argument");
            return NULL;
        }
        ctx = PHPREDIS_CTX_PTR;
    } else if (zend_string_equals_literal_ci(op, "DELUSER")) {
        if (argc < 1) {
            php_error_docref(NULL, E_WARNING, "ACL DELUSER requires at least one argument");
            return NULL;
        }
        ctx = PHPREDIS_CTX_PTR + 2;
    } else if (zend_string_equals_literal_ci(op, "GETUSER")) {
        if (argc < 1) {
            php_error_docref(NULL, E_WARNING, "ACL GETUSER requires at least one argument");
            return NULL;
        }
        ctx = PHPREDIS_CTX_PTR + 3;
    } else if (zend_string_equals_literal_ci(op, "DRYRUN")) {
        if (argc < 2) {
            php_error_docref(NULL, E_WARNING, "ACL DRYRUN requires at least two arguments");
            return NULL;
        }
        ctx = PHPREDIS_CTX_PTR;
    } else if (zend_string_equals_literal_ci(op, "LOG")) {
        if (argc > 0 && Z_TYPE(z_args[0]) == IS_STRING &&
            zend_string_equals_literal_ci(Z_STR(z_args[0]), "RESET"))
        {
            ctx = PHPREDIS_CTX_PTR;
        } else {
            ctx = PHPREDIS_CTX_PTR + 4;
        }
    } else {
        php_error_docref(NULL, E_WARNING, "Unknown ACL operation '%s'", ZSTR_VAL(op));
        return NULL;
    }

    cmd = redis_cmd_create_literal(redis_sock, "ACL");
    redis_cmd_cat_zstr(cmd, op);

    for (i = 0; i < argc; ++i) {
        zstr = zval_get_string(&z_args[i]);
        redis_cmd_cat_zstr(cmd, zstr);
        zend_string_release(zstr);
    }

    redis_cmd_set_ctx(cmd, ctx);

    return cmd;
}

RedisCmd *redis_waitaof_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock)
{
    zend_long numlocal, numreplicas, timeout;
    RedisCmd *cmd;

    ZEND_PARSE_PARAMETERS_START(3, 3)
        Z_PARAM_LONG(numlocal)
        Z_PARAM_LONG(numreplicas)
        Z_PARAM_LONG(timeout)
    ZEND_PARSE_PARAMETERS_END_EX(return NULL);

    if (numlocal < 0 || numreplicas < 0 || timeout < 0) {
        php_error_docref(NULL, E_WARNING, "No arguments can be negative");
        return NULL;
    }

    cmd = redis_cmd_create_literal(redis_sock, "WAITAOF");
    redis_cmd_cat_long(cmd, numlocal);
    redis_cmd_cat_long(cmd, numreplicas);
    redis_cmd_cat_long(cmd, timeout);

    return cmd;
}

/* Attempt to pull a long expiry from a zval.  We're more restrictave than zval_get_long
 * because that function will return integers from things like open file descriptors
 * which should simply fail as a TTL */
static int redis_try_get_expiry(zval *zv, zend_long *lval) {
    double dval;

    /* Success on an actual long or double */
    if (Z_TYPE_P(zv) == IS_LONG || Z_TYPE_P(zv) == IS_DOUBLE) {
        *lval = zval_get_long(zv);
        return SUCCESS;
    }

    /* Automatically fail if we're not a string */
    if (Z_TYPE_P(zv) != IS_STRING)
        return FAILURE;

    /* Attempt to get a long from the string */
    switch (is_numeric_string(Z_STRVAL_P(zv), Z_STRLEN_P(zv), lval, &dval, 0)) {
        case IS_DOUBLE:
            *lval = dval;
            REDIS_FALLTHROUGH;
        case IS_LONG:
            return SUCCESS;
        default:
            return FAILURE;
    }
}

static zend_bool zstr_to_expiry_type(redisExpiryType *dst, zend_string *src) {
    redisExpiryType tmp = REDIS_EXPIRY_NONE;

    if (zend_string_equals_literal_ci(src, "EX")) {
        tmp = REDIS_EXPIRY_EX;
    } else if (zend_string_equals_literal_ci(src, "PX")) {
        tmp = REDIS_EXPIRY_PX;
    } else if (zend_string_equals_literal_ci(src, "EXAT")) {
        tmp = REDIS_EXPIRY_EXAT;
    } else if (zend_string_equals_literal_ci(src, "PXAT")) {
        tmp = REDIS_EXPIRY_PXAT;
    } else {
        return 0;
    }

    *dst = tmp;
    return 1;
}

static zend_bool zstr_to_eq_type(redisEqType *dst, zend_string *src) {
    *dst = REDIS_IF_NONE;

    if (zend_string_equals_literal_ci(src, "IFEQ")) {
        *dst = REDIS_IFEQ;
    } else if (zend_string_equals_literal_ci(src, "IFNE")) {
        *dst = REDIS_IFNE;
    } else if (zend_string_equals_literal_ci(src, "IFDEQ")) {
        *dst = REDIS_IFDEQ;
    } else if (zend_string_equals_literal_ci(src, "IFDNE")) {
        *dst = REDIS_IFDNE;
    } else {
        return 0;
    }

    return 1;
}

static void fill_set_options_ht(redisSetOptions *dst, HashTable *ht) {
    zend_string *key;
    zval *zv;

    ZEND_HASH_FOREACH_STR_KEY_VAL(ht, key, zv) {
        if (key) {
            if (zstr_to_expiry_type(&dst->expiry.type, key)) {
                dst->expiry.ttl = zval_get_long(zv);
            } else if (zstr_to_eq_type(&dst->eq.type, key)) {
                dst->eq.zval = zv;
            } else if (zend_string_equals_literal_ci(key, "GET")) {
                dst->get = zend_is_true(zv);
            }
        } else if (Z_TYPE_P(zv) == IS_STRING) {
            key = Z_STR_P(zv);
            if (zend_string_equals_literal_ci(key, "NX")) {
                dst->type = REDIS_SET_NX;
            } else if (zend_string_equals_literal_ci(key, "XX")) {
                dst->type = REDIS_SET_XX;
            } else if (zend_string_equals_literal_ci(key, "GET")) {
                dst->get = 1;
            } else if (zend_string_equals_literal_ci(key, "KEEPTTL")) {
                dst->expiry.type = REDIS_EXPIRY_NONE;
                dst->expiry.keepttl = 1;
            }
        }
    } ZEND_HASH_FOREACH_END();
}

static int
fill_set_options_zval(redisSetOptions *dst, zval *zv, zend_bool legacy_set) {
    zend_long lval;

    memset(dst, 0, sizeof(*dst));

    if (zv == NULL)
        return SUCCESS;

    if (Z_TYPE_P(zv) == IS_ARRAY) {
        fill_set_options_ht(dst, Z_ARRVAL_P(zv));
        return SUCCESS;
    } else if (redis_try_get_expiry(zv, &lval) == SUCCESS && lval > 0) {
        if (!legacy_set)
            dst->expiry.type = REDIS_EXPIRY_EX;
        dst->expiry.ttl = lval;
        return SUCCESS;
    }

    php_error_docref(NULL, E_WARNING,
        "EXPIRY is invalid (must be an int, float, or numeric string >= 1)");

    return FAILURE;
}

static void
redis_cmd_cat_expiry(RedisCmd *cmd, redisExpiryOptions *e)
{
    if (e->type == REDIS_EXPIRY_NONE && !e->keepttl)
        return;

    if (e->keepttl) {
        redis_cmd_cat_literal(cmd, "KEEPTTL");
        return;
    } else if (e->type == REDIS_EXPIRY_EX) {
        redis_cmd_cat_literal(cmd, "EX");
    } else if (e->type == REDIS_EXPIRY_PX) {
        redis_cmd_cat_literal(cmd, "PX");
    } else if (e->type == REDIS_EXPIRY_EXAT) {
        redis_cmd_cat_literal(cmd, "EXAT");
    } else if (e->type == REDIS_EXPIRY_PXAT) {
        redis_cmd_cat_literal(cmd, "PXAT");
    }

    redis_cmd_cat_long(cmd, e->ttl);
}

static void
redis_cmd_cat_eq_clause(RedisCmd *cmd, redisEqType type, zval *zv)
{
    zend_bool pack;

    if (type == REDIS_IF_NONE)
        return;

    if (type == REDIS_IFEQ) {
        redis_cmd_cat_literal(cmd, "IFEQ");
    } else if (type == REDIS_IFNE) {
        redis_cmd_cat_literal(cmd, "IFNE");
    } else if (type == REDIS_IFDEQ) {
        redis_cmd_cat_literal(cmd, "IFDEQ");
    } else if (type == REDIS_IFDNE) {
        redis_cmd_cat_literal(cmd, "IFDNE");
    }

    pack = type == REDIS_IFEQ || type == REDIS_IFNE;
    if (pack) {
        redis_cmd_cat_zval(cmd, zv);
    } else {
        redis_cmd_cat_zval_zstr(cmd, zv);
    }
}

static void
redis_cmd_cat_set_type(RedisCmd *cmd, redisSetType type) {
    if (type == REDIS_SET_NX) {
        redis_cmd_cat_literal(cmd, "NX");
    } else if (type == REDIS_SET_XX) {
        redis_cmd_cat_literal(cmd, "XX");
    }
}

/* MSETEX */
RedisCmd *redis_msetex_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock)
{
    redisSetOptions opt = {0};
    zval *zexp = NULL;
    HashTable *kvals;
    RedisCmd *cmd;

    ZEND_PARSE_PARAMETERS_START(1, 2)
        Z_PARAM_ARRAY_HT(kvals)
        Z_PARAM_OPTIONAL
        Z_PARAM_ZVAL_OR_NULL(zexp)
    ZEND_PARSE_PARAMETERS_END_EX(return NULL);

    if (zend_hash_num_elements(kvals) == 0) {
        php_error_docref(NULL, E_WARNING, "No key/value pairs provided");
        return NULL;
    }

    if (fill_set_options_zval(&opt, zexp, 0) != SUCCESS)
        return NULL;

    cmd = redis_cmd_create_literal(redis_sock, "MSETEX");

    redis_cmd_cat_u64(cmd, zend_hash_num_elements(kvals));
    redis_cmd_try_cat_key(cmd, redis_cmd_cat_mset_kvals, kvals);
    redis_cmd_cat_set_type(cmd, opt.type);
    redis_cmd_cat_expiry(cmd, &opt.expiry);

    return cmd;
}

/* SET */
RedisCmd *redis_set_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock)
{
    zval *z_value = NULL, *z_opts = NULL;
    redisSetOptions opt = {0};
    zend_string *key;
    RedisCmd *cmd;

    ZEND_PARSE_PARAMETERS_START(2, 3)
        Z_PARAM_STR(key)
        Z_PARAM_ZVAL(z_value)
        Z_PARAM_OPTIONAL
        Z_PARAM_ZVAL_OR_NULL(z_opts)
    ZEND_PARSE_PARAMETERS_END_EX(return NULL);

    if (fill_set_options_zval(&opt, z_opts, 1) != SUCCESS)
        return NULL;

    /* You can't use IFEQ with NX or XX */
    if (opt.type && opt.eq.type != REDIS_IF_NONE) {
        php_error_docref(NULL, E_WARNING,
            "IF clauses can't be combined with NX/XX");
        return NULL;
    }

    /* Backward compatibility:  If we are passed no options except an EXPIRE ttl, we
     * actually execute the SETEX command */
    if (opt.expiry.ttl > 0 && opt.expiry.type == REDIS_EXPIRY_NONE && !opt.type)
    {
        return redis_cmd_fmt(redis_sock, "SETEX", "Klv", key, opt.expiry.ttl,
                             z_value);
    }

    /* Initial SET <key> <value> */
    cmd = redis_cmd_create_literal(redis_sock, "SET");
    redis_cmd_cat_key_zstr(cmd, key);
    redis_cmd_cat_zval(cmd, z_value);

    if (opt.eq.type != REDIS_IF_NONE) {
        redis_cmd_cat_eq_clause(cmd, opt.eq.type, opt.eq.zval);
    } else if (opt.type != REDIS_SET_NONE) {
        redis_cmd_cat_set_type(cmd, opt.type);
    }

    if (opt.get) {
        redis_cmd_cat_literal(cmd, "GET");
        redis_cmd_set_ctx(cmd, PHPREDIS_CTX_PTR);
    }

    redis_cmd_cat_expiry(cmd, &opt.expiry);

    return cmd;
}

/* MGET */
RedisCmd *redis_mget_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock)
{
    HashTable *keys = NULL;
    RedisCmd *cmd;
    zval *zkey;

    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_ARRAY_HT(keys)
    ZEND_PARSE_PARAMETERS_END_EX(return NULL);

    if (zend_hash_num_elements(keys) == 0)
        return NULL;

    cmd = redis_cmd_create_literal(redis_sock, "MGET");

    ZEND_HASH_FOREACH_VAL(keys, zkey) {
        ZVAL_DEREF(zkey);
        redis_cmd_try_cat_key_zval(cmd, zkey);
    } ZEND_HASH_FOREACH_END();

    return cmd;
}

RedisCmd *
redis_getex_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock)
{
    char *exp_type = NULL;
    zval *z_opts = NULL, *z_ele;
    zend_string *zkey, *key;
    zend_long expire = -1;
    zend_bool persist = 0;
    RedisCmd *cmd;

    ZEND_PARSE_PARAMETERS_START(1, 2)
        Z_PARAM_STR(key)
        Z_PARAM_OPTIONAL
        Z_PARAM_ARRAY(z_opts)
    ZEND_PARSE_PARAMETERS_END_EX(return NULL);

    if (z_opts != NULL) {
        ZEND_HASH_FOREACH_STR_KEY_VAL(Z_ARRVAL_P(z_opts), zkey, z_ele) {
            if (zkey != NULL) {
                ZVAL_DEREF(z_ele);
                if (zend_string_equals_literal_ci(zkey, "EX") ||
                    zend_string_equals_literal_ci(zkey, "PX") ||
                    zend_string_equals_literal_ci(zkey, "EXAT") ||
                    zend_string_equals_literal_ci(zkey, "PXAT")
                ) {
                    exp_type = ZSTR_VAL(zkey);
                    expire = zval_get_long(z_ele);
                    persist = 0;
                } else if (zend_string_equals_literal_ci(zkey, "PERSIST")) {
                    persist = zend_is_true(z_ele);
                    exp_type = NULL;
                }
            } else if (Z_TYPE_P(z_ele) == IS_STRING &&
                       zend_string_equals_literal_ci(Z_STR_P(z_ele), "PERSIST"))
            {
                persist = zend_is_true(z_ele);
                exp_type = NULL;
            }
        } ZEND_HASH_FOREACH_END();
    }

    if (exp_type != NULL && expire < 1) {
        php_error_docref(NULL, E_WARNING, "EXPIRE can't be < 1");
        return NULL;
    }

    cmd = redis_cmd_create_literal(redis_sock, "GETEX");
    redis_cmd_cat_key_zstr(cmd, key);

    if (exp_type != NULL) {
        redis_cmd_cat_str(cmd, exp_type, strlen(exp_type));
        redis_cmd_cat_long(cmd, expire);
    } else if (persist) {
        redis_cmd_cat_literal(cmd, "PERSIST");
    }

    return cmd;
}

/* BRPOPLPUSH */
RedisCmd *redis_brpoplpush_cmd(INTERNAL_FUNCTION_PARAMETERS,
                               RedisSock *redis_sock)
{
    zend_string *src = NULL, *dst = NULL;
    double timeout = 0;
    RedisCmd *cmd;

    ZEND_PARSE_PARAMETERS_START(3, 3)
        Z_PARAM_STR(src)
        Z_PARAM_STR(dst)
        Z_PARAM_DOUBLE(timeout)
    ZEND_PARSE_PARAMETERS_END_EX(return NULL);

    /* Consistency with Redis.  If timeout < 0 use RPOPLPUSH */
    if (timeout < 0) {
        cmd = redis_cmd_create_literal(redis_sock, "RPOPLPUSH");
    } else if (fabs(timeout - (long)timeout) < .0001) {
        cmd = redis_cmd_create_literal(redis_sock, "BRPOPLPUSH");
    } else {
        cmd = redis_cmd_create_literal(redis_sock, "BRPOPLPUSH");
    }

    redis_cmd_cat_key_zstr(cmd, src);
    redis_cmd_try_cat_key_zstr(cmd, dst);

    if (timeout >= 0) {
        if (fabs(timeout - (long)timeout) < .0001) {
            redis_cmd_cat_long(cmd, (long)timeout);
        } else {
            redis_cmd_cat_double(cmd, timeout);
        }
    }

    return cmd;
}

/* To maintain backward compatibility with earlier versions of phpredis, we
 * allow for an optional "increment by" argument for INCR and DECR even though
 * that's not how Redis proper works */
#define TYPE_INCR 0
#define TYPE_DECR 1

/* Handle INCR(BY) and DECR(BY) depending on optional increment value */
static RedisCmd *
redis_atomic_increment(INTERNAL_FUNCTION_PARAMETERS, int type,
                       RedisSock *redis_sock)
{
    zend_long val = 1;
    zend_string *key;

    ZEND_PARSE_PARAMETERS_START(1, 2)
        Z_PARAM_STR(key)
        Z_PARAM_OPTIONAL
        Z_PARAM_LONG(val)
    ZEND_PARSE_PARAMETERS_END_EX(return NULL);

    /* If our value is 1 we use INCR/DECR.  For other values, treat the call as
     * an INCRBY or DECRBY call */
    if (type == TYPE_INCR) {
        if (val == 1) {
            return redis_cmd_fmt(redis_sock, "INCR", "K", key);
        } else {
            return redis_cmd_fmt(redis_sock, "INCRBY", "Kl", key, val);
        }
    } else {
        if (val == 1) {
            return redis_cmd_fmt(redis_sock, "DECR", "K", key);
        } else {
            return redis_cmd_fmt(redis_sock, "DECRBY", "Kl", key, val);
        }
    }
}

/* INCR */
RedisCmd *redis_incr_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock)
{
    return redis_atomic_increment(INTERNAL_FUNCTION_PARAM_PASSTHRU,
        TYPE_INCR, redis_sock);
}

/* DECR */
RedisCmd *redis_decr_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock)
{
    return redis_atomic_increment(INTERNAL_FUNCTION_PARAM_PASSTHRU,
        TYPE_DECR, redis_sock);
}

/* INCREX - atomic increment with optional bounds, saturation, and expiration */
RedisCmd *redis_increx_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock)
{
    zval *z_increment = NULL, *z_opts = NULL;
    zend_bool byfloat = 0, saturate = 0, enx = 0, persist = 0;
    zend_long expire = -1;
    char *exp_type = NULL;
    zend_bool has_lbound = 0, has_ubound = 0;
    zend_long lbound_l = 0, ubound_l = 0;
    double lbound_d = 0, ubound_d = 0;
    zend_string *key, *zkey_opt;
    zval *z_ele;
    RedisCmd *cmd;

    ZEND_PARSE_PARAMETERS_START(1, 3)
        Z_PARAM_STR(key)
        Z_PARAM_OPTIONAL
        Z_PARAM_ZVAL(z_increment)
        Z_PARAM_OPTIONAL
        Z_PARAM_ZVAL_OR_NULL(z_opts)
    ZEND_PARSE_PARAMETERS_END_EX(return NULL);

    /* Determine increment type */
    if (z_increment != NULL && Z_TYPE_P(z_increment) == IS_DOUBLE) {
        byfloat = 1;
    }

    /* Parse options */
    if (z_opts != NULL && Z_TYPE_P(z_opts) == IS_ARRAY) {
        ZEND_HASH_FOREACH_STR_KEY_VAL(Z_ARRVAL_P(z_opts), zkey_opt, z_ele) {
            if (zkey_opt != NULL) {
                ZVAL_DEREF(z_ele);
                if (zend_string_equals_literal_ci(zkey_opt, "EX") ||
                    zend_string_equals_literal_ci(zkey_opt, "PX") ||
                    zend_string_equals_literal_ci(zkey_opt, "EXAT") ||
                    zend_string_equals_literal_ci(zkey_opt, "PXAT")
                ) {
                    exp_type = ZSTR_VAL(zkey_opt);
                    expire = zval_get_long(z_ele);
                    persist = 0;
                } else if (zend_string_equals_literal_ci(zkey_opt, "PERSIST")) {
                    persist = zend_is_true(z_ele);
                    exp_type = NULL;
                } else if (zend_string_equals_literal_ci(zkey_opt, "SATURATE")) {
                    saturate = zend_is_true(z_ele);
                } else if (zend_string_equals_literal_ci(zkey_opt, "ENX")) {
                    enx = zend_is_true(z_ele);
                } else if (zend_string_equals_literal_ci(zkey_opt, "LBOUND")) {
                    has_lbound = 1;
                    if (Z_TYPE_P(z_ele) == IS_DOUBLE) {
                        lbound_d = Z_DVAL_P(z_ele);
                        lbound_l = (zend_long)lbound_d;
                    } else {
                        lbound_l = zval_get_long(z_ele);
                        lbound_d = (double)lbound_l;
                    }
                } else if (zend_string_equals_literal_ci(zkey_opt, "UBOUND")) {
                    has_ubound = 1;
                    if (Z_TYPE_P(z_ele) == IS_DOUBLE) {
                        ubound_d = Z_DVAL_P(z_ele);
                        ubound_l = (zend_long)ubound_d;
                    } else {
                        ubound_l = zval_get_long(z_ele);
                        ubound_d = (double)ubound_l;
                    }
                }
            } else {
                ZVAL_DEREF(z_ele);
                if (Z_TYPE_P(z_ele) == IS_STRING) {
                    if (zend_string_equals_literal_ci(Z_STR_P(z_ele), "PERSIST")) {
                        persist = 1;
                        exp_type = NULL;
                    } else if (zend_string_equals_literal_ci(Z_STR_P(z_ele), "SATURATE")) {
                        saturate = 1;
                    } else if (zend_string_equals_literal_ci(Z_STR_P(z_ele), "ENX")) {
                        enx = 1;
                    }
                }
            }
        } ZEND_HASH_FOREACH_END();
    }

    /* Validate */
    if (exp_type != NULL && expire < 1) {
        php_error_docref(NULL, E_WARNING, "EXPIRE can't be < 1");
        return NULL;
    }

    if (enx && exp_type == NULL) {
        php_error_docref(NULL, E_WARNING,
            "ENX requires one of EX, PX, EXAT, or PXAT");
        return NULL;
    }

    /* Build INCREX command */
    cmd = redis_cmd_create_literal(redis_sock, "INCREX");
    redis_cmd_cat_key_zstr(cmd, key);

    /* BYFLOAT / BYINT */
    if (z_increment != NULL) {
        if (byfloat) {
            redis_cmd_cat_literal(cmd, "BYFLOAT");
            if (Z_TYPE_P(z_increment) == IS_LONG) {
                redis_cmd_cat_double(cmd, (double)Z_LVAL_P(z_increment));
            } else {
                redis_cmd_cat_double(cmd, Z_DVAL_P(z_increment));
            }
        } else {
            redis_cmd_cat_literal(cmd, "BYINT");
            redis_cmd_cat_long(cmd, zval_get_long(z_increment));
        }
    }

    /* LBOUND */
    if (has_lbound) {
        redis_cmd_cat_literal(cmd, "LBOUND");
        if (byfloat) {
            redis_cmd_cat_double(cmd, lbound_d);
        } else {
            redis_cmd_cat_long(cmd, lbound_l);
        }
    }

    /* UBOUND */
    if (has_ubound) {
        redis_cmd_cat_literal(cmd, "UBOUND");
        if (byfloat) {
            redis_cmd_cat_double(cmd, ubound_d);
        } else {
            redis_cmd_cat_long(cmd, ubound_l);
        }
    }

    /* SATURATE */
    if (saturate) {
        redis_cmd_cat_literal(cmd, "SATURATE");
    }

    /* Expiration */
    if (exp_type != NULL) {
        redis_cmd_cat_str(cmd, exp_type, strlen(exp_type));
        redis_cmd_cat_long(cmd, expire);
    } else if (persist) {
        redis_cmd_cat_literal(cmd, "PERSIST");
    }

    /* ENX */
    if (enx) {
        redis_cmd_cat_literal(cmd, "ENX");
    }

    return cmd;
}

typedef enum xdelExMode {
    REDIS_XDELEX_NONE,
    REDIS_XDELEX_KEEPREF,
    REDIS_XDELEX_DELREF,
    REDIS_XDELEX_ACKED,
} xdelExMode;

RedisCmd *redis_delex_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock)
{
    redisEqType type = REDIS_IF_NONE;
    zend_string *key, *ztype;
    HashTable *ht = NULL;
    zval *zv = NULL;
    RedisCmd *cmd;

    ZEND_PARSE_PARAMETERS_START(1, 2)
        Z_PARAM_STR(key)
        Z_PARAM_OPTIONAL
        Z_PARAM_ARRAY_HT_OR_NULL(ht)
    ZEND_PARSE_PARAMETERS_END_EX(return NULL);

    if (ht != NULL) {
        ZEND_HASH_FOREACH_STR_KEY_VAL(ht, ztype, zv) {
            if (ztype == NULL)
                continue;

            ZVAL_DEREF(zv);

            if (zstr_to_eq_type(&type, ztype))
                break;

            php_error_docref(NULL, E_WARNING, "Unknown option '%s'", ZSTR_VAL(ztype));
        } ZEND_HASH_FOREACH_END();
    }

    cmd = redis_cmd_create_literal(redis_sock, "DELEX");

    redis_cmd_cat_key_zstr(cmd, key);
    redis_cmd_cat_eq_clause(cmd, type, zv);

    return cmd;
}

/* HINCRBY */
RedisCmd *redis_hincrby_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock)
{
    zend_string *key, *mem;
    zend_long byval;

    ZEND_PARSE_PARAMETERS_START(3, 3)
        Z_PARAM_STR(key)
        Z_PARAM_STR(mem)
        Z_PARAM_LONG(byval)
    ZEND_PARSE_PARAMETERS_END_EX(return NULL);

    return redis_cmd_fmt(redis_sock, "HINCRBY", "KSl", key, mem, byval);
}

/* HINCRBYFLOAT */
RedisCmd *redis_hincrbyfloat_cmd(INTERNAL_FUNCTION_PARAMETERS,
                                 RedisSock *redis_sock)
{
    zend_string *key, *mem;
    double dval;

    ZEND_PARSE_PARAMETERS_START(3, 3)
        Z_PARAM_STR(key)
        Z_PARAM_STR(mem)
        Z_PARAM_DOUBLE(dval)
    ZEND_PARSE_PARAMETERS_END_EX(return NULL);

    return redis_cmd_fmt(redis_sock, "HINCRBYFLOAT", "KSf", key, mem, dval);
}

static inline zval *coerce_hash_field(zval *zv, zval *aux) {
    char buf[32];
    zend_long lv;
    size_t len;

    if (UNEXPECTED(Z_TYPE_P(zv) == IS_STRING &&
                   is_numeric_string(Z_STRVAL_P(zv),
                                     Z_STRLEN_P(zv), &lv, NULL, 0) == IS_LONG))
    {
        len = snprintf(buf, sizeof(buf), ZEND_LONG_FMT, lv);
        if (len == Z_STRLEN_P(zv) && redis_strncmp(Z_STRVAL_P(zv), buf, len) == 0) {
            ZVAL_LONG(aux, lv);
            return aux;
        }
    }

    return zv;
}

/* A helper function to build HMGET, HGETEX, HGETDEL context arrays we curry
 * to the reply side to return to the user fields and values */
static HashTable *
build_hash_context_ht(HashTable *htsrc, zend_bool (*cb)(zval*)) {
    zend_string *key, *tmp;
    HashTable *ht;
    zval *zv, aux;

    ALLOC_HASHTABLE(ht);
    zend_hash_init(ht, zend_hash_num_elements(htsrc),
                   NULL, ZVAL_PTR_DTOR, 0);

    ZEND_HASH_FOREACH_VAL(htsrc, zv)
        ZVAL_DEREF(zv);
        if (cb && !cb(zv))
            continue;

        /* Legacy behavior: Integer strings become integer keys */
        zv = coerce_hash_field(zv, &aux);

        if (Z_TYPE_P(zv) == IS_LONG) {
            zend_hash_index_add_empty_element(ht, Z_LVAL_P(zv));
        } else {
            key = zval_get_tmp_string(zv, &tmp);
            zend_hash_add_empty_element(ht, key);
            zend_tmp_string_release(tmp);
        }
    ZEND_HASH_FOREACH_END();

    /* Sanity check that we have at least one value */
    if (zend_hash_num_elements(ht) == 0) {
        zend_hash_destroy(ht);
        FREE_HASHTABLE(ht);
        return NULL;
    }

    return ht;
}

/* Legacy HMGET behavior predicate */
static zend_bool hmget_filter(zval *zv) {
    return (Z_TYPE_P(zv) == IS_STRING && Z_STRLEN_P(zv) > 0) ||
           (Z_TYPE_P(zv) == IS_LONG);
}

static void redis_cmd_cat_hash_fields(RedisCmd *cmd, HashTable *ht) {
    zend_string *key;
    zend_ulong idx;

    ZEND_HASH_FOREACH_KEY(ht, idx, key) {
        if (key) {
            redis_cmd_cat_zstr(cmd, key);
        } else {
            redis_cmd_cat_long(cmd, idx);
        }
    } ZEND_HASH_FOREACH_END();
}

void redis_cmd_ctx_hash_dtor(void *ptr) {
    HashTable *ht = ptr;

    if (GC_REFCOUNT(ht) == 1) {
        zend_hash_destroy(ht);
        FREE_HASHTABLE(ht);
    } else {
        GC_DELREF(ht);
    }
}

RedisCmd *redis_hmget_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock) {
    HashTable *fields = NULL, *htctx = NULL;
    zend_string *key = NULL;
    RedisCmd *cmd;

    ZEND_PARSE_PARAMETERS_START(2, 2)
        Z_PARAM_STR(key)
        Z_PARAM_ARRAY_HT(fields)
    ZEND_PARSE_PARAMETERS_END_EX(return NULL);

    if (zend_hash_num_elements(fields) == 0)
        return NULL;

    htctx = build_hash_context_ht(fields, hmget_filter);
    if (htctx == NULL) {
        php_error_docref(NULL, E_WARNING, "No valid fields provided");
        return NULL;
    }

    cmd = redis_cmd_create_literal(redis_sock, "HMGET");
    redis_cmd_cat_key_zstr(cmd, key);

    redis_cmd_cat_hash_fields(cmd, htctx);

    redis_cmd_set_ctx_ex(cmd, htctx, redis_cmd_ctx_hash_dtor);

    return cmd;
}

/* HMSET */
RedisCmd *redis_hmset_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock)
{
    zend_string *key = NULL;
    HashTable *ht = NULL;
    uint32_t fields;
    RedisCmd *cmd;
    zend_ulong idx;
    zval *zv;

    ZEND_PARSE_PARAMETERS_START(2, 2)
        Z_PARAM_STR(key)
        Z_PARAM_ARRAY_HT(ht)
    ZEND_PARSE_PARAMETERS_END_EX(return NULL);

    fields = zend_hash_num_elements(ht);
    if (fields == 0)
        return NULL;

    cmd = redis_cmd_create_literal(redis_sock, "HMSET");
    redis_cmd_cat_key_zstr(cmd, key);

    ZEND_HASH_FOREACH_KEY_VAL(ht, idx, key, zv) {
        if (key) {
            redis_cmd_cat_zstr(cmd, key);
        } else {
            redis_cmd_cat_long(cmd, idx);
        }
        redis_cmd_cat_zval(cmd, zv);
    } ZEND_HASH_FOREACH_END();

    return cmd;
}

/* HSTRLEN */
RedisCmd *
redis_hstrlen_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock)
{
    zend_string *key, *field;

    ZEND_PARSE_PARAMETERS_START(2, 2)
        Z_PARAM_STR(key)
        Z_PARAM_STR(field)
    ZEND_PARSE_PARAMETERS_END_EX(return NULL);

    return redis_cmd_fmt(redis_sock, "HSTRLEN", "KS", key, field);
}

static void redis_get_lcs_options(redisLcsOptions *dst, HashTable *ht) {
    zend_string *key;
    zval *zv;

    ZEND_ASSERT(dst != NULL);

    memset(dst, 0, sizeof(*dst));

    if (ht == NULL)
        return;

    ZEND_HASH_FOREACH_STR_KEY_VAL(ht, key, zv) {
        if (key) {
            if (zend_string_equals_literal_ci(key, "LEN")) {
                dst->idx = 0;
                dst->len = zend_is_true(zv);
            } else if (zend_string_equals_literal_ci(key, "IDX")) {
                dst->len = 0;
                dst->idx = zend_is_true(zv);
            } else if (zend_string_equals_literal_ci(key, "MINMATCHLEN")) {
                dst->minmatchlen = zval_get_long(zv);
            } else if (zend_string_equals_literal_ci(key, "WITHMATCHLEN")) {
                dst->withmatchlen = zend_is_true(zv);
            } else {
                php_error_docref(NULL, E_WARNING, "Unknown LCS option '%s'", ZSTR_VAL(key));
            }
        } else if (Z_TYPE_P(zv) == IS_STRING) {
            if (zend_string_equals_literal_ci(Z_STR_P(zv), "LEN")) {
                dst->idx = 0;
                dst->len = 1;
            } else if (zend_string_equals_literal_ci(Z_STR_P(zv), "IDX")) {
                dst->idx = 1;
                dst->len = 0;
            } else if (zend_string_equals_literal_ci(Z_STR_P(zv), "WITHMATCHLEN")) {
                dst->withmatchlen = 1;
            }
        }
    } ZEND_HASH_FOREACH_END();
}

/* LCS */
RedisCmd *redis_lcs_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock)
{
    zend_string *key1 = NULL, *key2 = NULL;
    HashTable *ht = NULL;
    redisLcsOptions opt;
    RedisCmd *cmd;

    ZEND_PARSE_PARAMETERS_START(2, 3)
        Z_PARAM_STR(key1)
        Z_PARAM_STR(key2)
        Z_PARAM_OPTIONAL
        Z_PARAM_ARRAY_HT_OR_NULL(ht)
    ZEND_PARSE_PARAMETERS_END_EX(return NULL);

    redis_get_lcs_options(&opt, ht);

    cmd = redis_cmd_create_literal(redis_sock, "LCS");

    redis_cmd_cat_key_zstr(cmd, key1);
    redis_cmd_try_cat_key_zstr(cmd, key2);

    redis_cmd_cat_literal_if(cmd, opt.idx, "IDX");
    redis_cmd_cat_literal_if(cmd, opt.len, "LEN");
    redis_cmd_cat_literal_if(cmd, opt.withmatchlen, "WITHMATCHLEN");

    if (opt.minmatchlen) {
        redis_cmd_cat_literal(cmd, "MINMATCHLEN");
        redis_cmd_cat_long(cmd, opt.minmatchlen);
    }

    return cmd;
}

RedisCmd *redis_slowlog_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock)
{
    enum {SLOWLOG_GET, SLOWLOG_LEN, SLOWLOG_RESET} mode;
    zend_string *op = NULL;
    zend_long arg = 0;
    RedisCmd *cmd;

    ZEND_PARSE_PARAMETERS_START(1, 2)
        Z_PARAM_STR(op)
        Z_PARAM_OPTIONAL
        Z_PARAM_LONG(arg)
    ZEND_PARSE_PARAMETERS_END_EX(return NULL);

    if (zend_string_equals_literal_ci(op, "GET")) {
        mode = SLOWLOG_GET;
    } else if (zend_string_equals_literal_ci(op, "LEN")) {
        mode = SLOWLOG_LEN;
    } else if (zend_string_equals_literal_ci(op, "RESET")) {
        mode = SLOWLOG_RESET;
    } else {
        php_error_docref(NULL, E_WARNING, "Unknown SLOWLOG operation '%s'", ZSTR_VAL(op));
        return NULL;
    }

    cmd = redis_cmd_create_literal(redis_sock, "SLOWLOG");
    redis_cmd_cat_zstr(cmd, op);

    if (mode == SLOWLOG_GET && ZEND_NUM_ARGS() == 2)
        redis_cmd_cat_long(cmd, arg);

    return cmd;
}

void redis_get_restore_options(redisRestoreOptions *dst, HashTable *ht) {
    zend_string *key;
    zend_long lval;
    zval *zv;

    ZEND_ASSERT(dst != NULL);

    memset(dst, 0, sizeof(*dst));
    dst->idletime = dst->freq = -1;

    if (ht == NULL)
        return;

    ZEND_HASH_FOREACH_STR_KEY_VAL(ht, key, zv) {
        ZVAL_DEREF(zv);

        if (key) {
            if (zend_string_equals_literal_ci(key, "IDLETIME")) {
                lval = zval_get_long(zv);
                if (lval < 0) {
                    php_error_docref(NULL, E_WARNING, "IDLETIME must be >= 0");
                } else {
                    dst->idletime = lval;
                    dst->freq = -1;
                }
            } else if (zend_string_equals_literal_ci(key, "FREQ")) {
                lval = zval_get_long(zv);
                if (lval < 0 || lval > 255) {
                    php_error_docref(NULL, E_WARNING, "FREQ must be >= 0 and <= 255");
                } else {
                    dst->freq = lval;
                    dst->idletime = -1;
                }
            } else {
                php_error_docref(NULL, E_WARNING, "Unknown RESTORE option '%s'", ZSTR_VAL(key));
            }
        } else if (Z_TYPE_P(zv) == IS_STRING) {
            if (zend_string_equals_literal_ci(Z_STR_P(zv), "REPLACE")) {
                dst->replace = 1;
            } else if (zend_string_equals_literal_ci(Z_STR_P(zv), "ABSTTL")) {
                dst->absttl = 1;
            } else {
                php_error_docref(NULL, E_WARNING, "Unknown RESTORE option '%s'", Z_STRVAL_P(zv));
            }
        }
    } ZEND_HASH_FOREACH_END();
}

/* RESTORE */
RedisCmd *redis_restore_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock)
{
    zend_string *key, *value = NULL;
    HashTable *options = NULL;
    redisRestoreOptions opt;
    zend_long timeout = 0;
    RedisCmd *cmd;

    ZEND_PARSE_PARAMETERS_START(3, 4) {
        Z_PARAM_STR(key)
        Z_PARAM_LONG(timeout)
        Z_PARAM_STR(value)
        Z_PARAM_OPTIONAL
        Z_PARAM_ARRAY_HT_OR_NULL(options)
    } ZEND_PARSE_PARAMETERS_END_EX(return NULL);

    redis_get_restore_options(&opt, options);

    cmd = redis_cmd_create_literal(redis_sock, "RESTORE");

    redis_cmd_cat_key_zstr(cmd, key);
    redis_cmd_cat_long(cmd, timeout);
    redis_cmd_cat_zstr(cmd, value);

    redis_cmd_cat_literal_if(cmd, opt.replace, "REPLACE");
    redis_cmd_cat_literal_if(cmd, opt.absttl, "ABSTTL");

    if (opt.idletime > -1) {
        redis_cmd_cat_literal(cmd, "IDLETIME");
        redis_cmd_cat_long(cmd, opt.idletime);
    }

    if (opt.freq > -1) {
        redis_cmd_cat_literal(cmd, "FREQ");
        redis_cmd_cat_long(cmd, opt.freq);
    }

    return cmd;
}

/* BITPOS key bit [start [end [BYTE | BIT]]] */
RedisCmd *redis_bitpos_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock)
{
    zend_long start = 0, end = -1;
    zend_bool bit = 0, bybit = 0;
    zend_string *key = NULL;
    RedisCmd *cmd;

    ZEND_PARSE_PARAMETERS_START(2, 5)
        Z_PARAM_STR(key)
        Z_PARAM_BOOL(bit)
        Z_PARAM_OPTIONAL
        Z_PARAM_LONG(start)
        Z_PARAM_LONG(end)
        Z_PARAM_BOOL(bybit)
    ZEND_PARSE_PARAMETERS_END_EX(return NULL);

    cmd = redis_cmd_create_literal(redis_sock, "BITPOS");

    redis_cmd_cat_key_zstr(cmd, key);
    redis_cmd_cat_long(cmd, bit);

    /* Start and length if we were passed either */
    if (ZEND_NUM_ARGS() > 2) {
        redis_cmd_cat_long(cmd, start);
        redis_cmd_cat_long(cmd, end);
    }

    /* Finally, BIT or BYTE if we were passed that argument */
    redis_cmd_cat_literal_if(cmd, !!bybit, "BIT");

    return cmd;
}

/* BITOP */
RedisCmd *redis_bitop_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock) {
    zend_string *op, *dstkey, *srckey;
    RedisCmd *cmd;
    zval *args;
    int argc;

    ZEND_PARSE_PARAMETERS_START(3, -1)
        Z_PARAM_STR(op)
        Z_PARAM_STR(dstkey)
        Z_PARAM_STR(srckey)
        Z_PARAM_VARIADIC('*', args, argc)
    ZEND_PARSE_PARAMETERS_END_EX(return NULL);

    cmd = redis_cmd_create_literal(redis_sock, "BITOP");

    redis_cmd_cat_zstr(cmd, op);
    redis_cmd_cat_key_zstr(cmd, dstkey);
    redis_cmd_try_cat_key_zstr(cmd, srckey);

    for (int i = 0; i < argc; i++) {
        redis_cmd_try_cat_key_zval(cmd, &args[i]);
    }

    return cmd;
}

/* BITCOUNT */
RedisCmd *redis_bitcount_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock) {
    zend_long start = 0, end = -1;
    zend_bool isbit = 0;
    zend_string *key;
    RedisCmd *cmd;

    ZEND_PARSE_PARAMETERS_START(1, 4)
        Z_PARAM_STR(key)
        Z_PARAM_OPTIONAL
        Z_PARAM_LONG(start)
        Z_PARAM_LONG(end)
        Z_PARAM_BOOL(isbit)
    ZEND_PARSE_PARAMETERS_END_EX(return NULL);

    cmd = redis_cmd_create_literal(redis_sock, "BITCOUNT");

    redis_cmd_cat_key_zstr(cmd, key);
    redis_cmd_cat_long(cmd, start);
    redis_cmd_cat_long(cmd, end);

    redis_cmd_cat_literal_if(cmd, isbit, "BIT");

    return cmd;
}

/* PFADD and PFMERGE are the same except that in one case we serialize,
 * and in the other case we key prefix */
static RedisCmd *
redis_gen_pf_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock,
                 const char *kw, size_t kw_len, int is_keys)
{
    zend_string *key = NULL;
    HashTable *ht = NULL;
    RedisCmd *cmd;
    zval *z_ele;

    // Parse arguments
    ZEND_PARSE_PARAMETERS_START(2, 2)
        Z_PARAM_STR(key)
        Z_PARAM_ARRAY_HT(ht)
    ZEND_PARSE_PARAMETERS_END_EX(return NULL);

    // We need at least two arguments
    if (zend_hash_num_elements(ht) + 1 < 2) {
        return NULL;
    }

    cmd = redis_cmd_create(redis_sock, kw, kw_len);

    redis_cmd_cat_key_zstr(cmd, key);

    // Append our array of keys or serialized values */
    ZEND_HASH_FOREACH_VAL(ht, z_ele) {
        if (is_keys) {
            redis_cmd_try_cat_key_zval(cmd, z_ele);
        } else {
            redis_cmd_cat_zval(cmd, z_ele);
        }
    } ZEND_HASH_FOREACH_END();

    return cmd;
}

/* PFADD */
RedisCmd *redis_pfadd_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock) {
    return redis_gen_pf_cmd(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock,
                            ZEND_STRL("PFADD"), 0);
}

/* PFMERGE */
RedisCmd *redis_pfmerge_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock) {
    return redis_gen_pf_cmd(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock,
        ZEND_STRL("PFMERGE"), 1);
}

/* PFCOUNT */
RedisCmd *redis_pfcount_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock) {
    zval *zarg = NULL, *zv;
    uint32_t keys;
    RedisCmd *cmd;

    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_ZVAL(zarg)
    ZEND_PARSE_PARAMETERS_END_EX(return NULL);

    cmd = redis_cmd_create_literal(redis_sock, "PFCOUNT");

    if (Z_TYPE_P(zarg) == IS_STRING) {
        redis_cmd_cat_key_zstr(cmd, Z_STR_P(zarg));
    } else if (Z_TYPE_P(zarg) == IS_ARRAY) {
        keys = zend_hash_num_elements(Z_ARRVAL_P(zarg));
        if (keys == 0) {
            redis_cmd_free(cmd);
            return NULL;
        }

        ZEND_HASH_FOREACH_VAL(Z_ARRVAL_P(zarg), zv) {
            redis_cmd_try_cat_key_zval(cmd, zv);
        } ZEND_HASH_FOREACH_END();
    } else {
        php_error_docref(NULL, E_WARNING, "Argument must be either an array or a string");
        redis_cmd_free(cmd);
        return NULL;
    }

    return cmd;
}

RedisCmd *redis_auth_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock) {
    zend_string *user = NULL, *pass = NULL;
    RedisCmd *cmd;
    zval *ztest;

    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_ZVAL_OR_NULL(ztest)
    ZEND_PARSE_PARAMETERS_END_EX(return NULL);

    if (redis_extract_auth_info(ztest, &user, &pass) == FAILURE) {
        return NULL;
    }

    cmd = redis_cmd_create_literal(redis_sock, "AUTH");

    /* Construct either AUTH <user> <pass> or AUTH <pass> */
    if (user)
        redis_cmd_cat_zstr(cmd, user);
    if (pass)
        redis_cmd_cat_zstr(cmd, pass);

    redis_sock_set_auth(redis_sock, user, pass);

    if (user)
        zend_string_release(user);
    if (pass)
        zend_string_release(pass);

    return cmd;
}

/* SETBIT */
RedisCmd *redis_setbit_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock) {
    zend_long offset;
    zend_string *key;
    zend_bool val;
    RedisCmd *cmd;

    ZEND_PARSE_PARAMETERS_START(3, 3)
        Z_PARAM_STR(key)
        Z_PARAM_LONG(offset)
        Z_PARAM_BOOL(val)
    ZEND_PARSE_PARAMETERS_END_EX(return NULL);

    // Validate our offset
    if (offset < BITOP_MIN_OFFSET || offset > BITOP_MAX_OFFSET) {
        php_error_docref(0, E_WARNING,
            "Invalid OFFSET for bitop command (must be between 0-2^32-1)");
        return NULL;
    }

    cmd = redis_cmd_create_literal(redis_sock, "SETBIT");
    redis_cmd_cat_key_zstr(cmd, key);
    redis_cmd_cat_long(cmd, offset);
    redis_cmd_cat_long(cmd, val);

    return cmd;
}

RedisCmd *redis_lmove_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock, const char *kw, size_t kw_len)
{
    zend_string *src = NULL, *dst = NULL, *from = NULL, *to = NULL;
    double timeout = 0.0;
    RedisCmd *cmd;
    int blocking;

    blocking = toupper(*kw) == 'B';

    ZEND_PARSE_PARAMETERS_START(4 + !!blocking, 4 + !!blocking)
        Z_PARAM_STR(src)
        Z_PARAM_STR(dst)
        Z_PARAM_STR(from)
        Z_PARAM_STR(to)
        if (blocking) {
            Z_PARAM_DOUBLE(timeout)
        }
    ZEND_PARSE_PARAMETERS_END_EX(return NULL);

    if (!zend_string_equals_literal_ci(from, "LEFT") && !zend_string_equals_literal_ci(from, "RIGHT")) {
        php_error_docref(NULL, E_WARNING, "Wherefrom argument must be 'LEFT' or 'RIGHT'");
        return NULL;
    } else if (!zend_string_equals_literal_ci(to, "LEFT") && !zend_string_equals_literal_ci(to, "RIGHT")) {
        php_error_docref(NULL, E_WARNING, "Whereto argument must be 'LEFT' or 'RIGHT'");
        return NULL;
    }

    cmd = redis_cmd_create(redis_sock, kw, kw_len);
    redis_cmd_cat_key_zstr(cmd, src);
    redis_cmd_try_cat_key_zstr(cmd, dst);

    redis_cmd_cat_zstr(cmd, from);
    redis_cmd_cat_zstr(cmd, to);
    if (blocking) redis_cmd_cat_double(cmd, timeout);

    return cmd;
}

typedef enum redisLmovemCountType {
    REDIS_LMOVEM_COUNT_NONE,
    REDIS_LMOVEM_COUNT,
    REDIS_LMOVEM_EXACTLY
} redisLmovemCountType;

typedef enum redisLmovemMoveType {
    REDIS_LMOVEM_MOVE_NONE,
    REDIS_LMOVEM_MOVE_OBO,
    REDIS_LMOVEM_MOVE_BULK
} redisLmovemMoveType;

typedef struct redisLmovemOptions {
    redisLmovemCountType count_type;
    redisLmovemMoveType move_type;
    zend_long count;
} redisLmovemOptions;

static int redis_get_lmovem_options(redisLmovemOptions *dst, HashTable *options)
{
    zval *zv, *zcount, *zmove;
    zend_string *key;

    memset(dst, 0, sizeof(*dst));

    if (options == NULL)
        return SUCCESS;

    ZEND_HASH_FOREACH_STR_KEY_VAL(options, key, zv) {
        if (key == NULL || (dst->count_type != REDIS_LMOVEM_COUNT_NONE)) {
            php_error_docref(NULL, E_WARNING,
                "Exactly one of COUNT or EXACTLY may be specified");
            return FAILURE;
        }

        if (zend_string_equals_literal_ci(key, "COUNT")) {
            dst->count_type = REDIS_LMOVEM_COUNT;
        } else if (zend_string_equals_literal_ci(key, "EXACTLY")) {
            dst->count_type = REDIS_LMOVEM_EXACTLY;
        } else {
            php_error_docref(NULL, E_WARNING, "Unknown LMOVEM option '%s'",
                ZSTR_VAL(key));
            return FAILURE;
        }

        ZVAL_DEREF(zv);
        if (Z_TYPE_P(zv) != IS_ARRAY || zend_hash_num_elements(Z_ARRVAL_P(zv)) != 2 ||
            (zcount = zend_hash_index_find(Z_ARRVAL_P(zv), 0)) == NULL ||
            (zmove = zend_hash_index_find(Z_ARRVAL_P(zv), 1)) == NULL)
        {
            php_error_docref(NULL, E_WARNING,
                "%s must be an array containing a count and movement type",
                dst->count_type == REDIS_LMOVEM_COUNT ? "COUNT" : "EXACTLY");
            return FAILURE;
        }

        ZVAL_DEREF(zcount);
        if (Z_TYPE_P(zcount) != IS_LONG || Z_LVAL_P(zcount) <= 0) {
            php_error_docref(NULL, E_WARNING, "LMOVEM count must be a positive integer");
            return FAILURE;
        }
        dst->count = Z_LVAL_P(zcount);

        ZVAL_DEREF(zmove);
        if (Z_TYPE_P(zmove) != IS_STRING) {
            php_error_docref(NULL, E_WARNING, "Movement type must be 'OBO' or 'BULK'");
            return FAILURE;
        } else if (zend_string_equals_literal_ci(Z_STR_P(zmove), "OBO")) {
            dst->move_type = REDIS_LMOVEM_MOVE_OBO;
        } else if (zend_string_equals_literal_ci(Z_STR_P(zmove), "BULK")) {
            dst->move_type = REDIS_LMOVEM_MOVE_BULK;
        } else {
            php_error_docref(NULL, E_WARNING, "Movement type must be 'OBO' or 'BULK'");
            return FAILURE;
        }
    } ZEND_HASH_FOREACH_END();

    return SUCCESS;
}

static RedisCmd *redis_lmovem_cmd_impl(INTERNAL_FUNCTION_PARAMETERS,
    RedisSock *redis_sock, const char *kw, size_t kw_len, zend_bool blocking)
{
    zend_string *src, *dst, *from, *to;
    HashTable *options = NULL;
    redisLmovemOptions opt;
    double timeout = 0;
    RedisCmd *cmd;

    ZEND_PARSE_PARAMETERS_START(4 + blocking, 5 + blocking)
        Z_PARAM_STR(src)
        Z_PARAM_STR(dst)
        Z_PARAM_STR(from)
        Z_PARAM_STR(to)
        if (blocking) {
            Z_PARAM_DOUBLE(timeout)
        }
        Z_PARAM_OPTIONAL
        Z_PARAM_ARRAY_HT_OR_NULL(options)
    ZEND_PARSE_PARAMETERS_END_EX(return NULL);

    if (!zend_string_equals_literal_ci(from, "LEFT") &&
        !zend_string_equals_literal_ci(from, "RIGHT"))
    {
        php_error_docref(NULL, E_WARNING, "Wherefrom argument must be 'LEFT' or 'RIGHT'");
        return NULL;
    } else if (!zend_string_equals_literal_ci(to, "LEFT") &&
               !zend_string_equals_literal_ci(to, "RIGHT"))
    {
        php_error_docref(NULL, E_WARNING, "Whereto argument must be 'LEFT' or 'RIGHT'");
        return NULL;
    } else if (redis_get_lmovem_options(&opt, options) == FAILURE) {
        return NULL;
    }

    cmd = redis_cmd_create(redis_sock, kw, kw_len);
    redis_cmd_cat_key_zstr(cmd, src);
    redis_cmd_try_cat_key_zstr(cmd, dst);
    redis_cmd_cat_zstr(cmd, from);
    redis_cmd_cat_zstr(cmd, to);

    if (blocking)
        redis_cmd_cat_double(cmd, timeout);

    if (opt.count_type != REDIS_LMOVEM_COUNT_NONE) {
        if (opt.count_type == REDIS_LMOVEM_COUNT) {
            redis_cmd_cat_literal(cmd, "COUNT");
        } else {
            redis_cmd_cat_literal(cmd, "EXACTLY");
        }
        redis_cmd_cat_long(cmd, opt.count);
        if (opt.move_type == REDIS_LMOVEM_MOVE_OBO) {
            redis_cmd_cat_literal(cmd, "OBO");
        } else {
            redis_cmd_cat_literal(cmd, "BULK");
        }
    }

    return cmd;
}

RedisCmd *redis_lmovem_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock,
                           const char *kw, size_t kw_len)
{
    return redis_lmovem_cmd_impl(INTERNAL_FUNCTION_PARAM_PASSTHRU,
        redis_sock, kw, kw_len, 0);
}

RedisCmd *redis_blmovem_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock,
                            const char *kw, size_t kw_len)
{
    return redis_lmovem_cmd_impl(INTERNAL_FUNCTION_PARAM_PASSTHRU,
        redis_sock, kw, kw_len, 1);
}

/* LINSERT */
RedisCmd *
redis_linsert_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock) {
    zend_string *key, *pos;
    zval *z_val, *z_pivot;
    RedisCmd *cmd;

    ZEND_PARSE_PARAMETERS_START(4, 4)
        Z_PARAM_STR(key)
        Z_PARAM_STR(pos)
        Z_PARAM_ZVAL(z_pivot)
        Z_PARAM_ZVAL(z_val)
    ZEND_PARSE_PARAMETERS_END_EX(return NULL);

    // Validate position
    if (!zend_string_equals_literal_ci(pos, "BEFORE") &&
        !zend_string_equals_literal_ci(pos, "AFTER"))
    {
        /* Valid */
        php_error_docref(NULL, E_WARNING,
            "Position must be either 'BEFORE' or 'AFTER'");
        return NULL;
    }

    cmd = redis_cmd_create_literal(redis_sock, "LINSERT");
    redis_cmd_cat_key_zstr(cmd, key);
    redis_cmd_cat_zstr(cmd, pos);
    redis_cmd_cat_zval(cmd, z_pivot);
    redis_cmd_cat_zval(cmd, z_val);

    return cmd;
}

/* LREM */
RedisCmd *redis_lrem_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock) {
    zend_long count = 0;
    zend_string *key;
    RedisCmd *cmd;
    zval *z_val;

    ZEND_PARSE_PARAMETERS_START(2, 3)
        Z_PARAM_STR(key)
        Z_PARAM_ZVAL(z_val)
        Z_PARAM_OPTIONAL
        Z_PARAM_LONG(count)
    ZEND_PARSE_PARAMETERS_END_EX(return NULL);

    cmd = redis_cmd_create_literal(redis_sock, "LREM");

    redis_cmd_cat_key_zstr(cmd, key);
    redis_cmd_cat_long(cmd, count);
    redis_cmd_cat_zval(cmd, z_val);

    return cmd;
}

RedisCmd *
redis_lpos_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock) {
    zend_string *key;
    zend_bool withrank = 0;
    zend_long rank = 0, count = -1, maxlen = -1;
    zend_string *zkey;
    zval *z_val, *z_ele;
    HashTable *ht_opts = NULL;
    RedisCmd *cmd;

    ZEND_PARSE_PARAMETERS_START(2, 3)
        Z_PARAM_STR(key)
        Z_PARAM_ZVAL(z_val)
        Z_PARAM_OPTIONAL
        Z_PARAM_ARRAY_HT_OR_NULL(ht_opts)
    ZEND_PARSE_PARAMETERS_END_EX(return NULL);

    if (ht_opts != NULL) {
        ZEND_HASH_FOREACH_STR_KEY_VAL(ht_opts, zkey, z_ele) {
            if (zkey != NULL) {
                ZVAL_DEREF(z_ele);
                if (zend_string_equals_literal_ci(zkey, "count")) {
                    count = zval_get_long(z_ele);
                } else if (zend_string_equals_literal_ci(zkey, "maxlen")) {
                    maxlen = zval_get_long(z_ele);
                } else if (zend_string_equals_literal_ci(zkey, "rank")) {
                    rank = zval_get_long(z_ele);
                    withrank = 1;
                }
            }
        } ZEND_HASH_FOREACH_END();
    }

    cmd = redis_cmd_create_literal(redis_sock, "LPOS");

    redis_cmd_cat_key_zstr(cmd, key);
    redis_cmd_cat_zval(cmd, z_val);

    if (withrank) {
        redis_cmd_cat_literal(cmd, "RANK");
        redis_cmd_cat_long(cmd, rank);
    }

    if (count >= 0) {
        redis_cmd_cat_literal(cmd, "COUNT");
        redis_cmd_cat_long(cmd, count);
        redis_cmd_set_ctx(cmd, PHPREDIS_CTX_PTR);
    }

    if (maxlen >= 0) {
        redis_cmd_cat_literal(cmd, "MAXLEN");
        redis_cmd_cat_long(cmd, maxlen);
    }

    return cmd;
}

RedisCmd *redis_smove_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock) {
    zend_string *src = NULL, *dst = NULL;
    zval *zv = NULL;
    RedisCmd *cmd;

    ZEND_PARSE_PARAMETERS_START(3, 3) {
        Z_PARAM_STR(src)
        Z_PARAM_STR(dst)
        Z_PARAM_ZVAL(zv)
    } ZEND_PARSE_PARAMETERS_END_EX(return NULL);

    cmd = redis_cmd_create_literal(redis_sock, "SMOVE");

    redis_cmd_cat_key_zstr(cmd, src);
    redis_cmd_try_cat_key_zstr(cmd, dst);
    redis_cmd_cat_zval(cmd, zv);

    return cmd;
}

typedef struct sunioncardOptions {
    zend_long limit;
    zend_bool approx;
} sunioncardOptions;

int fill_sunioncard_options(sunioncardOptions *dst, HashTable *ht) {
    zend_string *key;
    zend_long lval;
    zval *zv;

    memset(dst, 0, sizeof(*dst));
    if (ht == NULL)
        return SUCCESS;

    ZEND_HASH_FOREACH_STR_KEY_VAL(ht, key, zv) {
        if (key != NULL) {
            if (zend_string_equals_literal_ci(key, "LIMIT")) {
                lval = zval_get_long(zv);
                if (lval < 0) {
                    php_error_docref(NULL, E_WARNING, "LIMIT must be >= 0");
                    return FAILURE;
                }

                dst->limit = lval;
            }
        } else if (Z_TYPE_P(zv) == IS_STRING && zend_string_equals_literal_ci(Z_STR_P(zv), "APPROX")) {
            dst->approx = 1;
        }
    } ZEND_HASH_FOREACH_END();

    return SUCCESS;
}

/* SUNIONCARD/SDIFFCARD */
static RedisCmd *
gen_suniondiffcard_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock,
                       const char *kw, size_t kw_len, zend_bool has_approx)
{
    HashTable *keys, *htopt = NULL;
    sunioncardOptions opt;
    RedisCmd *cmd;
    zval *zkey;

    ZEND_PARSE_PARAMETERS_START(1, 2) {
        Z_PARAM_ARRAY_HT(keys)
        Z_PARAM_OPTIONAL
        Z_PARAM_ARRAY_HT_OR_NULL(htopt)
    } ZEND_PARSE_PARAMETERS_END_EX(return NULL);

    if (zend_hash_num_elements(keys) == 0) {
        php_error_docref(NULL, E_WARNING, "At least one key must be provided");
        return NULL;
    }

    if (fill_sunioncard_options(&opt, htopt) == FAILURE)
        return NULL;

    cmd = redis_cmd_create(redis_sock, kw, kw_len);

    redis_cmd_cat_long(cmd, zend_hash_num_elements(keys));

    ZEND_HASH_FOREACH_VAL(keys, zkey) {
        redis_cmd_try_cat_key_zval(cmd, zkey);
    } ZEND_HASH_FOREACH_END();

    if (opt.limit > 0) {
        redis_cmd_cat_literal(cmd, "LIMIT");
        redis_cmd_cat_long(cmd, opt.limit);
    }

    redis_cmd_cat_literal_if(cmd, has_approx && opt.approx, "APPROX");

    return cmd;
}

RedisCmd *
redis_sunioncard_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock) {
    return gen_suniondiffcard_cmd(INTERNAL_FUNCTION_PARAM_PASSTHRU,
                                      redis_sock, ZEND_STRL("SUNIONCARD"), 1);
}

RedisCmd *
redis_sdiffcard_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock) {
    return gen_suniondiffcard_cmd(INTERNAL_FUNCTION_PARAM_PASSTHRU,
                                      redis_sock, ZEND_STRL("SDIFFCARD"), 0);
}

/* HSET */
RedisCmd *redis_hset_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock) {
    int i, argc;
    zend_string *key, *zkey;
    zval *args, *z_ele;
    zend_ulong idx;
    RedisCmd *cmd;

    ZEND_PARSE_PARAMETERS_START(2, -1)
        Z_PARAM_STR(key)
        Z_PARAM_VARIADIC('*', args, argc)
    ZEND_PARSE_PARAMETERS_END_EX(return NULL);

    if (argc == 1) {
        if (Z_TYPE_P(args) != IS_ARRAY || zend_hash_num_elements(Z_ARRVAL_P(args)) == 0) {
            return NULL;
        }

        cmd = redis_cmd_create_literal(redis_sock, "HSET");
        redis_cmd_cat_key_zstr(cmd, key);

        ZEND_HASH_FOREACH_KEY_VAL(Z_ARRVAL_P(args), idx, zkey, z_ele) {
            ZVAL_DEREF(z_ele);
            if (zkey == NULL) {
                redis_cmd_cat_long(cmd, idx);
            } else {
                redis_cmd_cat_zstr(cmd, zkey);
            }
            redis_cmd_cat_zval(cmd, z_ele);
        } ZEND_HASH_FOREACH_END();
    } else {
        if (argc % 2 != 0) {
            return NULL;
        }

        cmd = redis_cmd_create_literal(redis_sock, "HSET");

        redis_cmd_cat_key_zstr(cmd, key);

        for (i = 0; i < argc; ++i) {
            if (i % 2) {
                redis_cmd_cat_zval(cmd, &args[i]);
            } else {
                redis_cmd_cat_zval_zstr(cmd, &args[i]);
            }
        }
    }

    return cmd;
}

/* HSETNX */
RedisCmd *redis_hsetnx_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock) {
    zend_string *key, *mem;
    RedisCmd *cmd;
    zval *z_val;

    ZEND_PARSE_PARAMETERS_START(3, 3)
        Z_PARAM_STR(key)
        Z_PARAM_STR(mem)
        Z_PARAM_ZVAL(z_val)
    ZEND_PARSE_PARAMETERS_END_EX(return NULL);

    cmd = redis_cmd_create_literal(redis_sock, "HSETNX");
    redis_cmd_cat_key_zstr(cmd, key);
    redis_cmd_cat_zstr(cmd, mem);
    redis_cmd_cat_zval(cmd, z_val);

    return cmd;
}

RedisCmd *
redis_hrandfield_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock) {
    HashTable *ht_opts = NULL;
    zend_bool withvalues = 0;
    zend_string *zkey;
    zend_string *key;
    RedisCmd *cmd;
    int count = 0;
    zval *z_ele;

    ZEND_PARSE_PARAMETERS_START(1, 2)
        Z_PARAM_STR(key)
        Z_PARAM_OPTIONAL
        Z_PARAM_ARRAY_HT_OR_NULL(ht_opts)
    ZEND_PARSE_PARAMETERS_END_EX(return NULL);

    if (ht_opts != NULL) {
        ZEND_HASH_FOREACH_STR_KEY_VAL(ht_opts, zkey, z_ele) {
            if (zkey != NULL) {
                ZVAL_DEREF(z_ele);
                if (zend_string_equals_literal_ci(zkey, "count")) {
                    count = zval_get_long(z_ele);
                } else if (zend_string_equals_literal_ci(zkey, "withvalues")) {
                    withvalues = zend_is_true(z_ele);
                }
            } else if (Z_TYPE_P(z_ele) == IS_STRING) {
                if (zend_string_equals_literal_ci(Z_STR_P(z_ele), "WITHVALUES")) {
                    withvalues = 1;
                }
            }
        } ZEND_HASH_FOREACH_END();
    }

    /* If we're sending WITHVALUES we must also send a count */
    if (count == 0 && withvalues)
        count = 1;

    cmd = redis_cmd_create_literal(redis_sock, "HRANDFIELD");

    redis_cmd_cat_key_zstr(cmd, key);

    if (count != 0) {
        redis_cmd_cat_long(cmd, count);
        redis_cmd_set_ctx(cmd, PHPREDIS_CTX_PTR);
    }

    if (withvalues) {
        redis_cmd_cat_literal(cmd, "WITHVALUES");
        redis_cmd_set_ctx(cmd, PHPREDIS_CTX_PTR + 1);
    }

    return cmd;
}

RedisCmd *
redis_select_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock) {
    zend_long db = 0;
    RedisCmd *cmd;

    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_LONG(db)
    ZEND_PARSE_PARAMETERS_END_EX(return NULL);

    if (db < 0 || db > INT_MAX)
        return NULL;

    cmd = redis_cmd_create_literal(redis_sock, "SELECT");
    redis_cmd_cat_long(cmd, db);
    redis_cmd_set_ctx(cmd, (void*)(uintptr_t)db);

    return cmd;
}

/* SRANDMEMBER */
RedisCmd *
redis_randmember_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock, const char *kw, size_t kw_len)
{
    uint32_t argc = ZEND_NUM_ARGS();
    zend_string *key = NULL;
    zend_long count = 0;
    RedisCmd *cmd;

    ZEND_PARSE_PARAMETERS_START(1, 2)
        Z_PARAM_STR(key)
        Z_PARAM_OPTIONAL
        Z_PARAM_LONG(count)
    ZEND_PARSE_PARAMETERS_END_EX(return NULL);

    cmd = redis_cmd_create(redis_sock, kw, kw_len);

    redis_cmd_cat_key_zstr(cmd, key);
    if (argc == 2)
        redis_cmd_cat_long(cmd, count);

    if (argc == 2)
        redis_cmd_set_ctx(cmd, PHPREDIS_CTX_PTR);

    return cmd;
}

/* ZINCRBY */
RedisCmd *
redis_zincrby_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock) {
    zend_string *key;
    RedisCmd *cmd;
    double incrby;
    zval *z_val;

    ZEND_PARSE_PARAMETERS_START(3, 3)
        Z_PARAM_STR(key)
        Z_PARAM_DOUBLE(incrby)
        Z_PARAM_ZVAL(z_val)
    ZEND_PARSE_PARAMETERS_END_EX(return NULL);

    cmd = redis_cmd_create_literal(redis_sock, "ZINCRBY");
    redis_cmd_cat_key_zstr(cmd, key);
    redis_cmd_cat_double(cmd, incrby);
    redis_cmd_cat_zval(cmd, z_val);

    return cmd;
}

/* SORT */
RedisCmd *redis_sort_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock, const char *kw, size_t kw_len)
{
    HashTable *opts = NULL;
    zend_string *key;
    RedisCmd *cmd;
    zval *zv;

    ZEND_PARSE_PARAMETERS_START(1, 2)
        Z_PARAM_STR(key)
        Z_PARAM_OPTIONAL
        Z_PARAM_ARRAY_HT(opts)
    ZEND_PARSE_PARAMETERS_END_EX(return NULL);

    // If we don't have an options array, the command is quite simple
    if (!opts || zend_hash_num_elements(opts) == 0) {
        return redis_cmd_fmt_ex(redis_sock, kw, kw_len, "K", key);
    }

    cmd = redis_cmd_create(redis_sock, kw, kw_len);

    // SORT <key>
    redis_cmd_cat_key_zstr(cmd, key);

    // Handle BY pattern
    if (((zv = zend_hash_str_find(opts, ZEND_STRL("by"))) != NULL ||
         (zv = zend_hash_str_find(opts, ZEND_STRL("BY"))) != NULL
        ) && Z_TYPE_P(zv) == IS_STRING)
    {
        // "BY" option is disabled in cluster
        if (redis_sock->type == REDIS_SOCK_CLUSTER) {
            php_error_docref(NULL, E_WARNING,
                "SORT BY option is not allowed in Redis Cluster");
            redis_cmd_free(cmd);
            return NULL;
        }

        // ... BY <pattern>
        redis_cmd_cat_literal(cmd, "BY");
        redis_cmd_cat_zstr(cmd, Z_STR_P(zv));
    }

    // Handle ASC/DESC option
    if (((zv = zend_hash_str_find(opts, ZEND_STRL("sort"))) != NULL ||
         (zv = zend_hash_str_find(opts, ZEND_STRL("SORT"))) != NULL) &&
        Z_TYPE_P(zv) == IS_STRING)
    {
        // 'asc'|'desc'
        redis_cmd_cat_zstr(cmd, Z_STR_P(zv));
    }

    // STORE option
    if (((zv = zend_hash_str_find(opts, ZEND_STRL("store"))) != NULL ||
         (zv = zend_hash_str_find(opts, ZEND_STRL("STORE"))) != NULL) &&
        Z_TYPE_P(zv) == IS_STRING)
    {
        redis_cmd_cat_literal(cmd, "STORE");
        redis_cmd_try_cat_key_zval(cmd, zv);
        redis_cmd_set_ctx(cmd, PHPREDIS_CTX_PTR);
    }

    // GET option
    if (((zv = zend_hash_str_find(opts, ZEND_STRL("get"))) != NULL ||
         (zv = zend_hash_str_find(opts, ZEND_STRL("GET"))) != NULL) &&
        (Z_TYPE_P(zv) == IS_STRING || Z_TYPE_P(zv) == IS_ARRAY))
    {
        // Disabled in cluster
        if (redis_sock->type == REDIS_SOCK_CLUSTER) {
            php_error_docref(NULL, E_WARNING,
                "GET option for SORT disabled in Redis Cluster");
            redis_cmd_free(cmd);
            return NULL;
        }

        // If it's a string just add it
        if (Z_TYPE_P(zv) == IS_STRING) {
            redis_cmd_cat_literal(cmd, "GET");
            redis_cmd_cat_zstr(cmd, Z_STR_P(zv));
        } else {
            zend_bool added = 0;

            ZEND_HASH_FOREACH_VAL(Z_ARRVAL_P(zv), zv) {
                if (Z_TYPE_P(zv) != IS_STRING)
                    continue;

                redis_cmd_cat_literal(cmd, "GET");
                redis_cmd_cat_zval_zstr(cmd, zv);
                added = 1;
            } ZEND_HASH_FOREACH_END();

            if (added == 0) {
                php_error_docref(NULL, E_WARNING,
                    "Array of GET values requested, but none are valid");
                redis_cmd_free(cmd);
                return NULL;
            }
        }
    }

    // ALPHA
    if (((zv = zend_hash_str_find(opts, ZEND_STRL("alpha"))) != NULL ||
         (zv = zend_hash_str_find(opts, ZEND_STRL("ALPHA"))) != NULL) &&
        zend_is_true(zv))
    {
        redis_cmd_cat_literal(cmd, "ALPHA");
    }

    // LIMIT <offset> <count>
    if (((zv = zend_hash_str_find(opts, ZEND_STRL("limit"))) != NULL ||
         (zv = zend_hash_str_find(opts, ZEND_STRL("LIMIT"))) != NULL) &&
        Z_TYPE_P(zv) == IS_ARRAY)
    {
        HashTable *ht_off = Z_ARRVAL_P(zv);
        zval *zoff, *zcnt;

        if ((zoff = zend_hash_index_find(ht_off, 0)) != NULL &&
            (zcnt = zend_hash_index_find(ht_off, 1)) != NULL
        ) {
            if ((Z_TYPE_P(zoff) != IS_STRING && Z_TYPE_P(zoff) != IS_LONG) ||
                (Z_TYPE_P(zcnt) != IS_STRING && Z_TYPE_P(zcnt) != IS_LONG)
            ) {
                php_error_docref(NULL, E_WARNING,
                    "LIMIT options on SORT command must be longs or strings");
                redis_cmd_free(cmd);
                return NULL;
            }

            // Add LIMIT argument
            redis_cmd_cat_literal(cmd, "LIMIT");

            long low, high;
            if (Z_TYPE_P(zoff) == IS_STRING) {
                low = atol(Z_STRVAL_P(zoff));
            } else {
                low = Z_LVAL_P(zoff);
            }
            if (Z_TYPE_P(zcnt) == IS_STRING) {
                high = atol(Z_STRVAL_P(zcnt));
            } else {
                high = Z_LVAL_P(zcnt);
            }

            // Add our two LIMIT arguments
            redis_cmd_cat_long(cmd, low);
            redis_cmd_cat_long(cmd, high);
        }
    }

    return cmd;
}

/* HDEL */
RedisCmd *redis_hdel_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock) {
    zend_string *key = NULL;
    RedisCmd *cmd;
    int argc = 0;
    zval *args;
    int i;

    ZEND_PARSE_PARAMETERS_START(2, -1)
        Z_PARAM_STR(key)
        Z_PARAM_VARIADIC('*', args, argc)
    ZEND_PARSE_PARAMETERS_END_EX(return NULL);

    cmd = redis_cmd_create_literal(redis_sock, "HDEL");

    redis_cmd_cat_key_zstr(cmd, key);

    for (i = 0; i < argc; i++) {
        redis_cmd_cat_zval_zstr(cmd, &args[i]);
    }

    return cmd;
}

RedisCmd *redis_zadd_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock) {
    zend_string *zstr, *key = NULL, *exp_type = NULL, *range_type = NULL;
    zend_bool ch = 0, incr = 0;
    zval *argv = NULL, *z_opt;
    int argc = 0, pos = 0;
    RedisCmd *cmd;

    ZEND_PARSE_PARAMETERS_START(3, -1)
        Z_PARAM_STR(key)
        Z_PARAM_VARIADIC('*', argv, argc)
    ZEND_PARSE_PARAMETERS_END_EX(return NULL);

    // Need key, [NX|XX] [LT|GT] [CH] [INCR] score, value, [score, value...] */
    if (argc % 2 != 0) {
        if (Z_TYPE(argv[0]) != IS_ARRAY) {
            return NULL;
        }

        ZEND_HASH_FOREACH_VAL(Z_ARRVAL(argv[0]), z_opt) {
            if (Z_TYPE_P(z_opt) == IS_STRING) {
                zstr = Z_STR_P(z_opt);
                if (zend_string_equals_literal_ci(zstr, "NX") ||
                    zend_string_equals_literal_ci(zstr, "XX"))
                {
                    exp_type = Z_STR_P(z_opt);
                } else if (zend_string_equals_literal_ci(zstr, "LT") ||
                           zend_string_equals_literal_ci(zstr, "GT"))
                {
                    range_type = Z_STR_P(z_opt);
                } else if (zend_string_equals_literal_ci(zstr, "CH")) {
                    ch = 1;
                } else if (zend_string_equals_literal_ci(zstr, "INCR")) {
                    if (argc != 3) {
                        // Only one score-element pair can be specified in this mode.
                        return NULL;
                    }
                    incr = 1;
                }
            }
        } ZEND_HASH_FOREACH_END();

        pos++;
    }

    cmd = redis_cmd_create_literal(redis_sock, "ZADD");

    redis_cmd_cat_key_zstr(cmd, key);

    if (exp_type)
        redis_cmd_cat_zstr(cmd, exp_type);
    if (range_type)
        redis_cmd_cat_zstr(cmd, range_type);

    redis_cmd_cat_literal_if(cmd, ch, "CH");
    redis_cmd_cat_literal_if(cmd, incr, "INCR");

    // Now the rest of our arguments
    while (pos < argc) {
        // Append score and member
        if (redis_cmd_cat_score(cmd, &argv[pos]) == FAILURE) {
            redis_cmd_free(cmd);
            return NULL;
        }

        redis_cmd_cat_zval(cmd, &argv[pos+1]);

        pos += 2;
    }

    if (incr)
        redis_cmd_set_ctx(cmd, PHPREDIS_CTX_PTR);

    return cmd;
}

RedisCmd *redis_object_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock) {
    zend_string *subcmd = NULL, *key = NULL;
    void *ctx = NULL;
    RedisCmd *cmd;

    ZEND_PARSE_PARAMETERS_START(2, 2)
        Z_PARAM_STR(subcmd)
        Z_PARAM_STR(key)
    ZEND_PARSE_PARAMETERS_END_EX(return NULL);

    if (zend_string_equals_literal_ci(subcmd, "REFCOUNT") ||
        zend_string_equals_literal_ci(subcmd, "IDLETIME"))
    {
        ctx = PHPREDIS_CTX_PTR;
    } else if (zend_string_equals_literal_ci(subcmd, "ENCODING")) {
        ctx = PHPREDIS_CTX_PTR + 1;
    } else {
        php_error_docref(NULL, E_WARNING, "Invalid subcommand sent to OBJECT");
        return NULL;
    }

    cmd = redis_cmd_create_literal(redis_sock, "OBJECT");

    redis_cmd_cat_zstr(cmd, subcmd);
    redis_cmd_cat_key_zstr(cmd, key);
    redis_cmd_set_ctx(cmd, ctx);

    return cmd;
}

RedisCmd *
redis_geoadd_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock) {
    zend_string *mode = NULL;
    zval *args, *z_ele;
    zend_bool ch = 0;
    RedisCmd *cmd;
    int argc, i;

    // We at least need a key and three values
    if ((argc = ZEND_NUM_ARGS()) < 4 || (argc % 3 != 1 && argc % 3 != 2)) {
        zend_wrong_param_count();
        return NULL;
    }

    ZEND_PARSE_PARAMETERS_START(4, -1)
        Z_PARAM_VARIADIC('*', args, argc)
    ZEND_PARSE_PARAMETERS_END_EX(return NULL);

    if (argc % 3 == 2) {
        argc--;
        if (Z_TYPE(args[argc]) != IS_ARRAY) {
            php_error_docref(NULL, E_WARNING, "Invalid options value");
            return NULL;
        }

        ZEND_HASH_FOREACH_VAL(Z_ARRVAL(args[argc]), z_ele) {
            ZVAL_DEREF(z_ele);
            if (Z_TYPE_P(z_ele) == IS_STRING) {
                if (zend_string_equals_literal_ci(Z_STR_P(z_ele), "NX") ||
                    zend_string_equals_literal_ci(Z_STR_P(z_ele), "XX"))
                {
                    mode = Z_STR_P(z_ele);
                } else if (zend_string_equals_literal_ci(Z_STR_P(z_ele), "CH")) {
                    ch = 1;
                }
            }
        } ZEND_HASH_FOREACH_END();
    }

    cmd = redis_cmd_create_literal(redis_sock, "GEOADD");

    redis_cmd_cat_key_zval(cmd, &args[0]);

    if (mode != NULL)
        redis_cmd_cat_zstr(cmd, mode);

    redis_cmd_cat_literal_if(cmd, ch, "CH");

    /* Append members */
    for (i = 1; i < argc; ++i) {
        redis_cmd_cat_zval(cmd, &args[i]);
    }

    return cmd;
}

/* GEODIST */
RedisCmd *
redis_geodist_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock) {
    zend_string *key, *source, *dest, *unit = NULL;
    RedisCmd *cmd;

    ZEND_PARSE_PARAMETERS_START(3, 4)
        Z_PARAM_STR(key)
        Z_PARAM_STR(source)
        Z_PARAM_STR(dest)
        Z_PARAM_OPTIONAL
        Z_PARAM_STR(unit)
    ZEND_PARSE_PARAMETERS_END_EX(return NULL);

    cmd = redis_cmd_create_literal(redis_sock, "GEODIST");

    redis_cmd_cat_key_zstr(cmd, key);
    redis_cmd_cat_zstr(cmd, source);
    redis_cmd_cat_zstr(cmd, dest);

    if (unit)
        redis_cmd_cat_zstr(cmd, unit);

    return cmd;
}

geoStoreType get_georadius_store_type(zend_string *key) {
    if (zend_string_equals_literal_ci(key, "store")) {
        return STORE_COORD;
    } else if (zend_string_equals_literal_ci(key, "storedist")) {
        return STORE_DIST;
    }

    return STORE_NONE;
}

static int validate_geosearch_shape_position(zval *position, zval *shape,
                                             geoSearchOptions *opts)
{
    uint32_t nelems;

    memset(opts, 0, sizeof(*opts));

    if (Z_TYPE_P(shape) == IS_LONG || Z_TYPE_P(shape) == IS_DOUBLE) {
        opts->shape_type = GEOSEARCH_SHAPE_RADIUS;
        opts->shape.radius = zval_get_double(shape);
        opts->shape_argc = 2;
    } else if (Z_TYPE_P(shape) == IS_ARRAY) {
        nelems = zend_hash_num_elements(Z_ARRVAL_P(shape));
        opts->shape.dimensions = Z_ARRVAL_P(shape);

        if (nelems == 2) {
            opts->shape_type = GEOSEARCH_SHAPE_BOX;
            opts->shape_argc = 3;
        } else if (nelems >= 6 && nelems % 2 == 0) {
            opts->shape_type = GEOSEARCH_SHAPE_POLYGON;
            opts->position_type = GEOSEARCH_POSITION_NONE;
            opts->shape_argc = 2 + nelems;
            return 1;
        } else {
            php_error_docref(NULL, E_WARNING, "Invalid shape dimensions");
            return 0;
        }
    } else {
        php_error_docref(NULL, E_WARNING, "Invalid shape dimensions");
        return 0;
    }

    if (Z_TYPE_P(position) == IS_STRING && Z_STRLEN_P(position) > 0) {
        opts->position_type = GEOSEARCH_POSITION_MEMBER;
        opts->position.member = Z_STR_P(position);
        opts->position_argc = 2;
    } else if (Z_TYPE_P(position) == IS_ARRAY &&
               zend_hash_num_elements(Z_ARRVAL_P(position)) == 2)
    {
        opts->position_type = GEOSEARCH_POSITION_LONLAT;
        opts->position.coordinates = Z_ARRVAL_P(position);
        opts->position_argc = 3;
    } else {
        php_error_docref(NULL, E_WARNING, "Invalid position");
        return 0;
    }

    return 1;
}

static void
append_geosearch_position(RedisCmd *cmd, geoSearchOptions *opts) {
    zval *z_ele;

    switch (opts->position_type) {
        case GEOSEARCH_POSITION_MEMBER:
            redis_cmd_cat_literal(cmd, "FROMMEMBER");
            redis_cmd_cat_zstr(cmd, opts->position.member);
            break;
        case GEOSEARCH_POSITION_LONLAT:
            redis_cmd_cat_literal(cmd, "FROMLONLAT");
            ZEND_HASH_FOREACH_VAL(opts->position.coordinates, z_ele) {
                ZVAL_DEREF(z_ele);
                redis_cmd_cat_double(cmd, zval_get_double(z_ele));
            } ZEND_HASH_FOREACH_END();
            break;
        case GEOSEARCH_POSITION_NONE:
            break;
    }
}

static void
append_geosearch_shape(RedisCmd *cmd, geoSearchOptions *opts) {
    zval *z_ele;

    switch (opts->shape_type) {
        case GEOSEARCH_SHAPE_RADIUS:
            redis_cmd_cat_literal(cmd, "BYRADIUS");
            redis_cmd_cat_double(cmd, opts->shape.radius);
            break;
        case GEOSEARCH_SHAPE_BOX:
            redis_cmd_cat_literal(cmd, "BYBOX");
            ZEND_HASH_FOREACH_VAL(opts->shape.dimensions, z_ele) {
                ZVAL_DEREF(z_ele);
                redis_cmd_cat_double(cmd, zval_get_double(z_ele));
            } ZEND_HASH_FOREACH_END();
            break;
        case GEOSEARCH_SHAPE_POLYGON:
            redis_cmd_cat_literal(cmd, "BYPOLYGON");
            redis_cmd_cat_long(cmd,
                zend_hash_num_elements(opts->shape.dimensions) / 2);
            ZEND_HASH_FOREACH_VAL(opts->shape.dimensions, z_ele) {
                ZVAL_DEREF(z_ele);
                redis_cmd_cat_double(cmd, zval_get_double(z_ele));
            } ZEND_HASH_FOREACH_END();
            break;
    }
}

/* Helper function to get COUNT and possible ANY flag which is passable to
 * both GEORADIUS and GEOSEARCH */
static int get_georadius_count_options(zval *optval, geoOptions *opts) {
    zval *z_tmp;

    /* Short circuit on bad options */
    if (Z_TYPE_P(optval) != IS_ARRAY && Z_TYPE_P(optval) != IS_LONG)
        goto error;

    if (Z_TYPE_P(optval) == IS_ARRAY) {
        z_tmp = zend_hash_index_find(Z_ARRVAL_P(optval), 0);
        if (z_tmp) {
            if (Z_TYPE_P(z_tmp) != IS_LONG || Z_LVAL_P(z_tmp) <= 0)
                goto error;
            opts->count = Z_LVAL_P(z_tmp);
        }

        z_tmp = zend_hash_index_find(Z_ARRVAL_P(optval), 1);
        if (z_tmp) {
            opts->any = zend_is_true(z_tmp);
        }
    } else {
        if (Z_LVAL_P(optval) <= 0)
            goto error;
        opts->count = Z_LVAL_P(optval);
    }

    return SUCCESS;

error:
    php_error_docref(NULL, E_WARNING, "Invalid COUNT value");
    return FAILURE;
}

/* Helper function to extract optional arguments for GEORADIUS and GEORADIUSBYMEMBER */
static int get_georadius_opts(HashTable *ht, geoOptions *opts) {
    zend_string *zkey;
    char *optstr;
    zval *optval;

    /* Iterate over our argument array, collating which ones we have */
    ZEND_HASH_FOREACH_STR_KEY_VAL(ht, zkey, optval) {
        ZVAL_DEREF(optval);

        /* If the key is numeric it's a non value option */
        if (zkey) {
            if (zend_string_equals_literal_ci(zkey, "COUNT")) {
                if (get_georadius_count_options(optval, opts) == FAILURE) {
                    if (opts->key) zend_string_release(opts->key);
                    return FAILURE;
                }
            } else if (opts->store == STORE_NONE) {
                opts->store = get_georadius_store_type(zkey);
                if (opts->store != STORE_NONE) {
                    opts->key = zval_get_string(optval);
                }
            }
        } else {
            /* Option needs to be a string */
            if (Z_TYPE_P(optval) != IS_STRING) continue;

            optstr = Z_STRVAL_P(optval);

            if (!strcasecmp(optstr, "withcoord")) {
                opts->withcoord = 1;
            } else if (!strcasecmp(optstr, "withdist")) {
                opts->withdist = 1;
            } else if (!strcasecmp(optstr, "withhash")) {
                opts->withhash = 1;
            } else if (!strcasecmp(optstr, "asc")) {
                opts->sort = SORT_ASC;
            } else if (!strcasecmp(optstr, "desc")) {
                opts->sort = SORT_DESC;
            }
        }
    } ZEND_HASH_FOREACH_END();

    /* STORE and STOREDIST are not compatible with the WITH* options */
    if (opts->key != NULL && (opts->withcoord || opts->withdist || opts->withhash)) {
        php_error_docref(NULL, E_WARNING,
            "STORE[DIST] is not compatible with WITHCOORD, WITHDIST or WITHHASH");

        if (opts->key) zend_string_release(opts->key);
        return FAILURE;
    }

    /* Success */
    return SUCCESS;
}

/* Helper to append options to a GEORADIUS or GEORADIUSBYMEMBER command */
zend_bool append_georadius_opts(RedisCmd *cmd, geoOptions *opt)
{
    zend_bool res = 1;

    redis_cmd_cat_literal_if(cmd, opt->withcoord, "WITHCOORD");
    redis_cmd_cat_literal_if(cmd, opt->withdist, "WITHDIST");
    redis_cmd_cat_literal_if(cmd, opt->withhash, "WITHHASH");

    /* Append sort if it's not GEO_NONE */
    if (opt->sort == SORT_ASC) {
        redis_cmd_cat_literal(cmd, "ASC");
    } else if (opt->sort == SORT_DESC) {
        redis_cmd_cat_literal(cmd, "DESC");
    }

    /* Append our count if we've got one */
    if (opt->count) {
        redis_cmd_cat_literal(cmd, "COUNT");
        redis_cmd_cat_long(cmd, opt->count);
        redis_cmd_cat_literal_if(cmd, opt->any, "ANY");
    }

    /* Append store options if we've got them */
    if (opt->store != STORE_NONE && opt->key != NULL) {
        if (opt->store == STORE_COORD) {
            redis_cmd_cat_literal(cmd, "STORE");
        } else {
            redis_cmd_cat_literal(cmd, "STOREDIST");
        }

        res = redis_cmd_cat_key_zstr(cmd, opt->key);
    }

    return res;
}

/* GEORADIUS / GEORADIUS_RO */
RedisCmd *
redis_georadius_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock, const char *kw, size_t kw_len)
{
    zend_string *key = NULL, *unit = NULL;
    double lng = 0, lat = 0, radius = 0;
    HashTable *opts = NULL;
    geoOptions gopts = {0};
    RedisCmd *cmd;

    ZEND_PARSE_PARAMETERS_START(5, 6)
        Z_PARAM_STR(key)
        Z_PARAM_DOUBLE(lng)
        Z_PARAM_DOUBLE(lat)
        Z_PARAM_DOUBLE(radius)
        Z_PARAM_STR(unit)
        Z_PARAM_OPTIONAL
        Z_PARAM_ARRAY_HT_OR_NULL(opts)
    ZEND_PARSE_PARAMETERS_END_EX(return NULL);

    /* Parse any GEORADIUS options we have */
    if (opts != NULL && get_georadius_opts(opts, &gopts) != SUCCESS)
        return NULL;

    cmd = redis_cmd_create(redis_sock, kw, kw_len);
    redis_cmd_cat_key_zstr(cmd, key);

    /* Append required arguments */
    redis_cmd_cat_double(cmd, lng);
    redis_cmd_cat_double(cmd, lat);
    redis_cmd_cat_double(cmd, radius);
    redis_cmd_cat_zstr(cmd, unit);

    /* Append optional arguments */
    if (!append_georadius_opts(cmd, &gopts)) {
        redis_crosslot_warning();
        redis_cmd_free(cmd);
        cmd = NULL;
    }

    /* Free key if it was prefixed */
    if (gopts.key)
        zend_string_release(gopts.key);

    return cmd;
}

/* GEORADIUSBYMEMBER/GEORADIUSBYMEMBER_RO
 *    key member radius m|km|ft|mi [WITHCOORD] [WITHDIST] [WITHHASH] [COUNT count] */
RedisCmd *
redis_georadiusbymember_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock, const char *kw, size_t kw_len)
{
    zend_string *key, *mem, *unit;
    geoOptions gopts = {0};
    HashTable *opts = NULL;
    double radius;
    RedisCmd *cmd;

    ZEND_PARSE_PARAMETERS_START(4, 5)
        Z_PARAM_STR(key)
        Z_PARAM_STR(mem)
        Z_PARAM_DOUBLE(radius)
        Z_PARAM_STR(unit)
        Z_PARAM_OPTIONAL
        Z_PARAM_ARRAY_HT_OR_NULL(opts)
    ZEND_PARSE_PARAMETERS_END_EX(return NULL);

    if (opts != NULL) {
        /* Attempt to parse our options array */
        if (get_georadius_opts(opts, &gopts) == FAILURE) {
            return NULL;
        }
    }

    cmd = redis_cmd_create(redis_sock, kw, kw_len);

    /* Append required arguments */
    redis_cmd_cat_key_zstr(cmd, key);
    redis_cmd_cat_zstr(cmd, mem);
    redis_cmd_cat_long(cmd, radius);
    redis_cmd_cat_zstr(cmd, unit);

    /* Append options */
    if (!append_georadius_opts(cmd, &gopts)) {
        redis_crosslot_warning();
        redis_cmd_free(cmd);
        cmd = NULL;
    }

    if (gopts.key)
        zend_string_release(gopts.key);

    return cmd;
}

RedisCmd *
redis_geosearch_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock)
{
    zval *position, *shape, *z_ele;
    geoSearchOptions sopts = {0};
    zend_string *zkey, *zstr;
    zend_string *key, *unit;
    geoOptions gopts = {0};
    HashTable *opts = NULL;
    RedisCmd *cmd;

    ZEND_PARSE_PARAMETERS_START(4, 5)
        Z_PARAM_STR(key)
        Z_PARAM_ZVAL(position)
        Z_PARAM_ZVAL(shape)
        Z_PARAM_STR(unit)
        Z_PARAM_OPTIONAL
        Z_PARAM_ARRAY_HT_OR_NULL(opts)
    ZEND_PARSE_PARAMETERS_END_EX(return NULL);

    if (!validate_geosearch_shape_position(position, shape, &sopts)) {
        return NULL;
    }

    /* Attempt to parse our options array */
    if (opts != NULL) {
        ZEND_HASH_FOREACH_STR_KEY_VAL(opts, zkey, z_ele) {
            ZVAL_DEREF(z_ele);
            if (zkey != NULL && zend_string_equals_literal_ci(zkey, "COUNT")) {
                if (get_georadius_count_options(z_ele, &gopts) == FAILURE) {
                    return NULL;
                }
            } else if (Z_TYPE_P(z_ele) == IS_STRING) {
                zstr = Z_STR_P(z_ele);
                if (zend_string_equals_literal_ci(zstr, "WITHCOORD")) {
                    gopts.withcoord = 1;
                } else if (zend_string_equals_literal_ci(zstr, "WITHDIST")) {
                    gopts.withdist = 1;
                } else if (zend_string_equals_literal_ci(zstr, "WITHHASH")) {
                    gopts.withhash = 1;
                } else if (zend_string_equals_literal_ci(zstr, "ASC")) {
                    gopts.sort = SORT_ASC;
                } else if (zend_string_equals_literal_ci(zstr, "DESC")) {
                    gopts.sort = SORT_DESC;
                }
            }
        } ZEND_HASH_FOREACH_END();
    }

    cmd = redis_cmd_create_literal(redis_sock, "GEOSEARCH");

    redis_cmd_cat_key_zstr(cmd, key);

    append_geosearch_position(cmd, &sopts);
    append_geosearch_shape(cmd, &sopts);

    if (sopts.shape_type != GEOSEARCH_SHAPE_POLYGON) {
        redis_cmd_cat_zstr(cmd, unit);
    }

    redis_cmd_cat_literal_if(cmd, gopts.withcoord, "WITHCOORD");
    redis_cmd_cat_literal_if(cmd, gopts.withdist, "WITHDIST");
    redis_cmd_cat_literal_if(cmd, gopts.withhash, "WITHHASH");

    /* Append sort if it's not GEO_NONE */
    if (gopts.sort == SORT_ASC) {
        redis_cmd_cat_literal(cmd, "ASC");
    } else if (gopts.sort == SORT_DESC) {
        redis_cmd_cat_literal(cmd, "DESC");
    }

    /* Append our count if we've got one */
    if (gopts.count) {
        redis_cmd_cat_literal(cmd, "COUNT");
        redis_cmd_cat_long(cmd, gopts.count);
        redis_cmd_cat_literal_if(cmd, gopts.any, "ANY");
    }

    if (gopts.withcoord + gopts.withdist + gopts.withhash > 0) {
        redis_cmd_set_ctx(cmd, PHPREDIS_CTX_PTR);
    }

    return cmd;
}

RedisCmd *
redis_geosearchstore_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock) {
    zval *position, *shape, *z_ele;
    zend_string *dest, *src, *unit;
    geoSearchOptions sopts = {0};
    HashTable *opts = NULL;
    geoOptions gopts = {0};
    zend_string *zkey;
    RedisCmd *cmd;

    ZEND_PARSE_PARAMETERS_START(5, 6)
        Z_PARAM_STR(dest)
        Z_PARAM_STR(src)
        Z_PARAM_ZVAL(position)
        Z_PARAM_ZVAL(shape)
        Z_PARAM_STR(unit)
        Z_PARAM_OPTIONAL
        Z_PARAM_ARRAY_HT_OR_NULL(opts)
    ZEND_PARSE_PARAMETERS_END_EX(return NULL);

    if (!validate_geosearch_shape_position(position, shape, &sopts)) {
        return NULL;
    }

    /* Attempt to parse our options array */
    if (opts != NULL) {
        ZEND_HASH_FOREACH_STR_KEY_VAL(opts, zkey, z_ele) {
            ZVAL_DEREF(z_ele);
            if (zkey != NULL) {
                if (zend_string_equals_literal_ci(zkey, "COUNT")) {
                    if (Z_TYPE_P(z_ele) != IS_LONG || Z_LVAL_P(z_ele) <= 0) {
                        php_error_docref(NULL, E_WARNING, "COUNT must be an integer > 0!");
                        return NULL;
                    }
                    gopts.count = Z_LVAL_P(z_ele);
                }
            } else if (Z_TYPE_P(z_ele) == IS_STRING) {
                if (!strcasecmp(Z_STRVAL_P(z_ele), "ASC")) {
                    gopts.sort = SORT_ASC;
                } else if (!strcasecmp(Z_STRVAL_P(z_ele), "DESC")) {
                    gopts.sort = SORT_DESC;
                } else if (!strcasecmp(Z_STRVAL_P(z_ele), "STOREDIST")) {
                    gopts.store = STORE_DIST;
                }
            }
        } ZEND_HASH_FOREACH_END();

    }

    cmd = redis_cmd_create_literal(redis_sock, "GEOSEARCHSTORE");

    redis_cmd_cat_key_zstr(cmd, dest);
    redis_cmd_try_cat_key_zstr(cmd, src);

    append_geosearch_position(cmd, &sopts);
    append_geosearch_shape(cmd, &sopts);

    if (sopts.shape_type != GEOSEARCH_SHAPE_POLYGON) {
        redis_cmd_cat_zstr(cmd, unit);
    }

    /* Append sort if it's not GEO_NONE */
    if (gopts.sort == SORT_ASC) {
        redis_cmd_cat_literal(cmd, "ASC");
    } else if (gopts.sort == SORT_DESC) {
        redis_cmd_cat_literal(cmd, "DESC");
    }

    /* Append our count if we've got one */
    if (gopts.count) {
        redis_cmd_cat_literal(cmd, "COUNT");
        redis_cmd_cat_long(cmd, gopts.count);
    }

    redis_cmd_cat_literal_if(cmd, gopts.store == STORE_DIST, "STOREDIST");

    return cmd;
}

/*  MIGRATE host port <key | ""> destination-db timeout [COPY] [REPLACE]
            [[AUTH password] | [AUTH2 username password]] [KEYS key [key ...]]

    Starting with Redis version 3.0.0: Added the COPY and REPLACE options.
    Starting with Redis version 3.0.6: Added the KEYS option.
    Starting with Redis version 4.0.7: Added the AUTH option.
    Starting with Redis version 6.0.0: Added the AUTH2 option.
*/

RedisCmd *
redis_httl_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock, const char *kw, size_t kw_len) {
    HashTable *fields;
    zend_string *key;
    RedisCmd *cmd;
    zval *zv;

    ZEND_PARSE_PARAMETERS_START(2, 2)
        Z_PARAM_STR(key)
        Z_PARAM_ARRAY_HT(fields)
    ZEND_PARSE_PARAMETERS_END_EX(return NULL);

    if (zend_hash_num_elements(fields) < 1) {
        php_error_docref(NULL, E_WARNING, "Must pass at least one field");
        return NULL;
    }

    cmd = redis_cmd_create(redis_sock, kw, kw_len);

    redis_cmd_cat_key_zstr(cmd, key);

    redis_cmd_cat_literal(cmd, "FIELDS");
    redis_cmd_cat_long(cmd, zend_hash_num_elements(fields));

    ZEND_HASH_FOREACH_VAL(fields, zv)
        redis_cmd_cat_zval_zstr(cmd, zv);
    ZEND_HASH_FOREACH_END();

    return cmd;
}

typedef struct redisHGetExOptions {
    zend_string *exp_type;
    zend_long exp_arg;
} redisHGetExOptions;

static int get_hgetex_expiry_opts(redisHGetExOptions *dst, zval *zv) {
    zend_string *zkey;
    HashTable *ht;
    zval *zexp;

    *dst = (redisHGetExOptions) {
        .exp_type = NULL,
        .exp_arg = -1
    };

    if (zv == NULL)
        return SUCCESS;

    if (Z_TYPE_P(zv) != IS_STRING && Z_TYPE_P(zv) != IS_ARRAY) {
        php_error_docref(NULL, E_WARNING,
            "Expiry value must be a string or an array of strings");
        return FAILURE;
    }

    if (Z_TYPE_P(zv) == IS_STRING) {
        dst->exp_type = Z_STR_P(zv);
        dst->exp_arg = -1;
        return SUCCESS;
    }

    ht = Z_ARRVAL_P(zv);

    ZEND_HASH_FOREACH_STR_KEY_VAL(ht, zkey, zexp)
        if (zkey == NULL)
            continue;

        if (Z_TYPE_P(zexp) != IS_LONG || Z_LVAL_P(zexp) < 0) {
            php_error_docref(NULL, E_WARNING,
                "Expiry must be an integer >= 0");
            return FAILURE;
        }

        dst->exp_type = zkey;
        dst->exp_arg = Z_LVAL_P(zexp);
    ZEND_HASH_FOREACH_END();

    return SUCCESS;
}

RedisCmd *redis_hgetex_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock) {
    redisHGetExOptions opts = {0};
    HashTable *fields, *htctx;
    zval *zexpiry = NULL;
    zend_string *key;
    RedisCmd *cmd;

    ZEND_PARSE_PARAMETERS_START(2, 3)
        Z_PARAM_STR(key)
        Z_PARAM_ARRAY_HT(fields)
        Z_PARAM_OPTIONAL
        Z_PARAM_ZVAL_OR_NULL(zexpiry);
    ZEND_PARSE_PARAMETERS_END_EX(return NULL);

    if (zend_hash_num_elements(fields) == 0) {
        php_error_docref(NULL, E_WARNING, "Must pass at least one field");
        return NULL;
    } else if (get_hgetex_expiry_opts(&opts, zexpiry) == FAILURE) {
        return NULL;
    }

    htctx = build_hash_context_ht(fields, hmget_filter);
    if (htctx == NULL) {
        php_error_docref(NULL, E_WARNING,
            "Failed to build context hash table");
        return NULL;
    }

    cmd = redis_cmd_create_literal(redis_sock, "HGETEX");
    redis_cmd_cat_key_zstr(cmd, key);

    if (opts.exp_type) {
        redis_cmd_cat_zstr(cmd, opts.exp_type);
        if (opts.exp_arg >= 0)
            redis_cmd_cat_long(cmd, opts.exp_arg);
    }

    redis_cmd_cat_literal(cmd, "FIELDS");
    redis_cmd_cat_long(cmd, zend_hash_num_elements(htctx));

    redis_cmd_cat_hash_fields(cmd, htctx);
    redis_cmd_set_ctx_ex(cmd, htctx, redis_cmd_ctx_hash_dtor);

    return cmd;
}

typedef struct redisHSetExOptions {
    zend_string *set_mode;
    zend_string *exp_type;
    zend_long exp_arg;
} redisHSetExOptions;

void get_hsetex_expiry_options(redisHSetExOptions *dst, HashTable *src) {
    zend_string *key;
    zend_long lval;
    zval *zv;

    *dst = (redisHSetExOptions) {
        .exp_arg = -1,
    };

    if (src == NULL)
        return;

    ZEND_HASH_FOREACH_STR_KEY_VAL(src, key, zv) {
        if (key == NULL) {
            if (Z_TYPE_P(zv) == IS_STRING) {
                key = Z_STR_P(zv);
                if (zend_string_equals_literal_ci(key, "FNX") ||
                    zend_string_equals_literal_ci(key, "FXX"))
                {
                    dst->set_mode = Z_STR_P(zv);
                } else if (zend_string_equals_literal_ci(key, "KEEPTTL")) {
                    dst->exp_type = key;
                    dst->exp_arg = -1;
                }
            }
        } else if (zend_string_equals_literal_ci(key, "EX") ||
                   zend_string_equals_literal_ci(key, "PX") ||
                   zend_string_equals_literal_ci(key, "EXAT") ||
                   zend_string_equals_literal_ci(key, "PXAT"))
        {
            lval = zval_get_long(zv);
            if (lval >= 0) {
                dst->exp_type = key;
                dst->exp_arg = lval;
            } else {
                php_error_docref(NULL, E_WARNING, "Timeouts must be >= 0");
            }
        }
    } ZEND_HASH_FOREACH_END();
}

RedisCmd *redis_hsetex_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock) {
    HashTable *fields, *expiry = NULL;
    redisHSetExOptions opts = {0};
    zend_string *key;
    zend_ulong idx;
    RedisCmd *cmd;
    zval *zv;

    ZEND_PARSE_PARAMETERS_START(2, 3)
        Z_PARAM_STR(key)
        Z_PARAM_ARRAY_HT(fields)
        Z_PARAM_OPTIONAL
        Z_PARAM_ARRAY_HT_OR_NULL(expiry);
    ZEND_PARSE_PARAMETERS_END_EX(return NULL);

    if (zend_hash_num_elements(fields) == 0) {
        php_error_docref(NULL, E_WARNING, "Must pass at least one field");
        return NULL;
    }

    get_hsetex_expiry_options(&opts, expiry);

    cmd = redis_cmd_create_literal(redis_sock, "HSETEX");
    redis_cmd_cat_key_zstr(cmd, key);

    if (opts.set_mode)
        redis_cmd_cat_zstr(cmd, opts.set_mode);
    if (opts.exp_type) {
        redis_cmd_cat_zstr(cmd, opts.exp_type);
        if (opts.exp_arg >= 0)
            redis_cmd_cat_long(cmd, opts.exp_arg);
    }

    redis_cmd_cat_literal(cmd, "FIELDS");
    redis_cmd_cat_long(cmd, zend_hash_num_elements(fields));

    ZEND_HASH_FOREACH_KEY_VAL(fields, idx, key, zv)
        if (key) {
            redis_cmd_cat_zstr(cmd, key);
        } else {
            redis_cmd_cat_long(cmd, idx);
        }

        redis_cmd_cat_zval(cmd, zv);
    ZEND_HASH_FOREACH_END();

    return cmd;
}

RedisCmd *redis_hgetdel_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock) {
    HashTable *fields, *htctx;
    zend_string *key;
    RedisCmd *cmd;

    ZEND_PARSE_PARAMETERS_START(2, 2)
        Z_PARAM_STR(key);
        Z_PARAM_ARRAY_HT(fields);
    ZEND_PARSE_PARAMETERS_END_EX(return NULL);

    if (zend_hash_num_elements(fields) == 0) {
        php_error_docref(NULL, E_WARNING, "Must pass at least one field");
        return NULL;
    }

    htctx = build_hash_context_ht(fields, hmget_filter);
    if (htctx == NULL) {
        php_error_docref(NULL, E_WARNING,
            "Failed to build context hash table");
        return NULL;
    }

    cmd = redis_cmd_create_literal(redis_sock, "HGETDEL");
    redis_cmd_cat_key_zstr(cmd, key);

    redis_cmd_cat_literal(cmd, "FIELDS");
    redis_cmd_cat_long(cmd, zend_hash_num_elements(htctx));
    redis_cmd_cat_hash_fields(cmd, htctx);

    redis_cmd_set_ctx_ex(cmd, htctx, redis_cmd_ctx_hash_dtor);

    return cmd;
}

RedisCmd *
redis_hexpire_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock, const char *kw, size_t kw_len)

{
    zend_string *key, *option = NULL;
    HashTable *fields;
    RedisCmd *cmd;
    zend_long ttl;
    zval *zv;

    ZEND_PARSE_PARAMETERS_START(3, 4)
        Z_PARAM_STR(key)
        Z_PARAM_LONG(ttl)
        Z_PARAM_ARRAY_HT(fields)
        Z_PARAM_OPTIONAL
        Z_PARAM_STR_OR_NULL(option)
    ZEND_PARSE_PARAMETERS_END_EX(return NULL);

    if (zend_hash_num_elements(fields) < 1) {
        php_error_docref(NULL, E_WARNING, "Must pass at least one field");
        return NULL;
    }

    cmd = redis_cmd_create(redis_sock, kw, kw_len);

    redis_cmd_cat_key_zstr(cmd, key);
    redis_cmd_cat_long(cmd, ttl);
    redis_cmd_cat_zstr_if(cmd, option, option);

    redis_cmd_cat_literal(cmd, "FIELDS");
    redis_cmd_cat_long(cmd, zend_hash_num_elements(fields));

    ZEND_HASH_FOREACH_VAL(fields, zv)
        redis_cmd_cat_zval_zstr(cmd, zv);
    ZEND_HASH_FOREACH_END();

    return cmd;
}

/* MIGRATE */
RedisCmd *redis_migrate_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock) {
    zend_string *host = NULL, *user = NULL, *pass = NULL;
    zend_long destdb = 0, port = 0, timeout = 0;
    zval *zkeys = NULL, *zkey, *zauth = NULL;
    zend_bool copy = 0, replace = 0;
    RedisCmd *cmd;

    ZEND_PARSE_PARAMETERS_START(5, 8)
        Z_PARAM_STR(host)
        Z_PARAM_LONG(port)
        Z_PARAM_ZVAL(zkeys)
        Z_PARAM_LONG(destdb)
        Z_PARAM_LONG(timeout)
        Z_PARAM_OPTIONAL
        Z_PARAM_BOOL(copy)
        Z_PARAM_BOOL(replace)
        Z_PARAM_ZVAL_OR_NULL(zauth)
    ZEND_PARSE_PARAMETERS_END_EX(return NULL);

    /* Sanity check on our optional AUTH argument */
    if (zauth && redis_extract_auth_info(zauth, &user, &pass) == FAILURE) {
        php_error_docref(NULL, E_WARNING,
            "AUTH must be a string or an array with one or two strings");
        user = pass = NULL;
    }

    /* Protect against being passed an array with zero elements */
    if (Z_TYPE_P(zkeys) == IS_ARRAY &&
        zend_hash_num_elements(Z_ARRVAL_P(zkeys)) == 0)
    {
        php_error_docref(NULL, E_WARNING, "Keys array cannot be empty");
        return NULL;
    }

    cmd = redis_cmd_create_literal(redis_sock, "MIGRATE");

    redis_cmd_cat_zstr(cmd, host);
    redis_cmd_cat_long(cmd, port);

    /* If passed a keys array the keys come later, otherwise pass the key to
     * migrate here */
    if (Z_TYPE_P(zkeys) == IS_ARRAY) {
        redis_cmd_cat_literal(cmd, "");
    } else {
        if (!redis_cmd_cat_key_zval(cmd, zkeys)) {
            redis_crosslot_warning();
            redis_cmd_free(cmd);
            if (user) zend_string_release(user);
            if (pass) zend_string_release(pass);
            return NULL;
        }
    }

    redis_cmd_cat_long(cmd, destdb);
    redis_cmd_cat_long(cmd, timeout);
    redis_cmd_cat_literal_if(cmd, copy, "COPY");
    redis_cmd_cat_literal_if(cmd, replace, "REPLACE");

    if (user && pass) {
        redis_cmd_cat_literal(cmd, "AUTH2");
        redis_cmd_cat_zstr(cmd, user);
        redis_cmd_cat_zstr(cmd, pass);
    } else if (pass) {
        redis_cmd_cat_literal(cmd, "AUTH");
        redis_cmd_cat_zstr(cmd, pass);
    }

    if (user) zend_string_release(user);
    if (pass) zend_string_release(pass);

    /* Append actual keys if we've got a keys array */
    if (Z_TYPE_P(zkeys) == IS_ARRAY) {
        redis_cmd_cat_literal(cmd, "KEYS");

        ZEND_HASH_FOREACH_VAL(Z_ARRVAL_P(zkeys), zkey) {
            redis_cmd_try_cat_key_zval(cmd, zkey);
        } ZEND_HASH_FOREACH_END();
    }

    return cmd;
}

/* A generic passthru function for variadic key commands that take one or more
 * keys.  This is essentially all of them except ones that STORE data. */
RedisCmd *
redis_varkey_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock, const char *kw, size_t kw_len)
{
    return gen_varkey_cmd(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock,
                          kw, kw_len, 0);
}

static RedisCmd *
redis_build_client_list_command(RedisSock *redis_sock, int argc, zval *z_args)
{
    zend_string *zkey;
    zval *z_ele, *type = NULL, *id = NULL;
    RedisCmd *cmd;

    if (argc > 0) {
        if (Z_TYPE(z_args[0]) != IS_ARRAY) {
            return NULL;
        }
        ZEND_HASH_FOREACH_STR_KEY_VAL(Z_ARRVAL(z_args[0]), zkey, z_ele) {
            if (zkey != NULL) {
                ZVAL_DEREF(z_ele);
                if (zend_string_equals_literal_ci(zkey, "type")) {
                    if (Z_TYPE_P(z_ele) != IS_STRING || (
                        !zend_string_equals_literal_ci(Z_STR_P(z_ele), "normal") &&
                        !zend_string_equals_literal_ci(Z_STR_P(z_ele), "master") &&
                        !zend_string_equals_literal_ci(Z_STR_P(z_ele), "replica") &&
                        !zend_string_equals_literal_ci(Z_STR_P(z_ele), "pubsub")
                    )) {
                        return NULL;
                    }
                    type = z_ele;
                } else if (zend_string_equals_literal_ci(zkey, "id")) {
                    if (Z_TYPE_P(z_ele) != IS_STRING && (
                        Z_TYPE_P(z_ele) != IS_ARRAY ||
                        !zend_hash_num_elements(Z_ARRVAL_P(z_ele))
                    )) {
                        return NULL;
                    }
                    id = z_ele;
                }
            }
        } ZEND_HASH_FOREACH_END();
    }
    cmd = redis_cmd_create_literal(redis_sock, "CLIENT");
    redis_cmd_cat_literal(cmd, "LIST");
    if (type != NULL) {
        redis_cmd_cat_literal(cmd, "TYPE");
        redis_cmd_cat_zstr(cmd, Z_STR_P(type));
    }
    if (id != NULL) {
        redis_cmd_cat_literal(cmd, "ID");
        if (Z_TYPE_P(id) == IS_ARRAY) {
            ZEND_HASH_FOREACH_VAL(Z_ARRVAL_P(id), z_ele) {
                if (Z_TYPE_P(z_ele) == IS_STRING) {
                    redis_cmd_cat_zstr(cmd, Z_STR_P(z_ele));
                } else {
                    zkey = zval_get_string(z_ele);
                    redis_cmd_cat_zstr(cmd, zkey);
                    zend_string_release(zkey);
                }
            } ZEND_HASH_FOREACH_END();
        } else {
            redis_cmd_cat_zstr(cmd, Z_STR_P(id));
        }
    }
    return cmd;
}

static RedisCmd *
redis_build_client_kill_command(RedisSock *redis_sock, int argc, zval *z_args)
{
    zend_string *zkey;
    zval *z_ele, *id = NULL, *type = NULL, *address = NULL, *opts = NULL,
        *user = NULL, *addr = NULL, *laddr = NULL, *skipme = NULL;
    RedisCmd *cmd;

    if (argc > 0) {
        if (argc > 1) {
            if (Z_TYPE(z_args[0]) != IS_STRING || Z_TYPE(z_args[1]) != IS_ARRAY) {
                return NULL;
            }
            address = &z_args[0];
            opts = &z_args[1];
        } else if (Z_TYPE(z_args[0]) == IS_STRING) {
            address = &z_args[0];
        } else if (Z_TYPE(z_args[0]) == IS_ARRAY) {
            opts = &z_args[0];
        } else {
            return NULL;
        }
        if (opts != NULL) {
            ZEND_HASH_FOREACH_STR_KEY_VAL(Z_ARRVAL_P(opts), zkey, z_ele) {
                if (zkey != NULL) {
                    ZVAL_DEREF(z_ele);
                    if (Z_TYPE_P(z_ele) != IS_STRING) {
                        return NULL;
                    }
                    if (zend_string_equals_literal_ci(zkey, "id")) {
                        id = z_ele;
                    } else if (zend_string_equals_literal_ci(zkey, "type")) {
                        if (!zend_string_equals_literal_ci(Z_STR_P(z_ele), "normal") &&
                            !zend_string_equals_literal_ci(Z_STR_P(z_ele), "master") &&
                            !zend_string_equals_literal_ci(Z_STR_P(z_ele), "slave") &&
                            !zend_string_equals_literal_ci(Z_STR_P(z_ele), "replica") &&
                            !zend_string_equals_literal_ci(Z_STR_P(z_ele), "pubsub")
                        ) {
                            return NULL;
                        }
                        type = z_ele;
                    } else if (zend_string_equals_literal_ci(zkey, "user")) {
                        user = z_ele;
                    } else if (zend_string_equals_literal_ci(zkey, "addr")) {
                        addr = z_ele;
                    } else if (zend_string_equals_literal_ci(zkey, "laddr")) {
                        laddr = z_ele;
                    } else if (zend_string_equals_literal_ci(zkey, "skipme")) {
                        if (!zend_string_equals_literal_ci(Z_STR_P(z_ele), "yes") &&
                            !zend_string_equals_literal_ci(Z_STR_P(z_ele), "no")
                        ) {
                            return NULL;
                        }
                        skipme = z_ele;
                    }
                }
            } ZEND_HASH_FOREACH_END();
        }
    }
    cmd = redis_cmd_create_literal(redis_sock, "CLIENT");
    redis_cmd_cat_literal(cmd, "KILL");
    if (address != NULL) {
        redis_cmd_cat_zstr(cmd, Z_STR_P(address));
    }
    if (id != NULL) {
        redis_cmd_cat_literal(cmd, "ID");
        redis_cmd_cat_zstr(cmd, Z_STR_P(id));
    }
    if (type != NULL) {
        redis_cmd_cat_literal(cmd, "TYPE");
        redis_cmd_cat_zstr(cmd, Z_STR_P(type));
    }
    if (user != NULL) {
        redis_cmd_cat_literal(cmd, "USER");
        redis_cmd_cat_zstr(cmd, Z_STR_P(user));
    }
    if (addr != NULL) {
        redis_cmd_cat_literal(cmd, "ADDR");
        redis_cmd_cat_zstr(cmd, Z_STR_P(addr));
    }
    if (laddr != NULL) {
        redis_cmd_cat_literal(cmd, "LADDR");
        redis_cmd_cat_zstr(cmd, Z_STR_P(laddr));
    }
    if (skipme != NULL) {
        redis_cmd_cat_literal(cmd, "SKIPME");
        redis_cmd_cat_zstr(cmd, Z_STR_P(skipme));
    }
    return cmd;
}

static RedisCmd *
redis_build_client_tracking_command(RedisSock *redis_sock, int argc, zval *z_args)
{
    zend_string *zkey;
    zval *z_ele, *redirect = NULL, *prefix = NULL;
    zend_bool bcast = 0, optin = 0, optout = 0, noloop = 0;
    RedisCmd *cmd;

    if (argc < 1) {
        return NULL;
    }
    if (argc > 1) {
        if (Z_TYPE(z_args[1]) != IS_ARRAY) {
            return NULL;
        }
        ZEND_HASH_FOREACH_STR_KEY_VAL(Z_ARRVAL(z_args[1]), zkey, z_ele) {
            if (zkey != NULL) {
                ZVAL_DEREF(z_ele);
                if (zend_string_equals_literal_ci(zkey, "redirect")) {
                    if (Z_TYPE_P(z_ele) != IS_STRING) {
                        return NULL;
                    }
                    redirect = z_ele;
                } else if (zend_string_equals_literal_ci(zkey, "prefix")) {
                    if (Z_TYPE_P(z_ele) != IS_STRING && Z_TYPE_P(z_ele) != IS_ARRAY) {
                        return NULL;
                    }
                    prefix = z_ele;
                } else if (zend_string_equals_literal_ci(zkey, "bcast")) {
                    bcast = zend_is_true(z_ele);
                } else if (zend_string_equals_literal_ci(zkey, "optin")) {
                    optin = zend_is_true(z_ele);
                } else if (zend_string_equals_literal_ci(zkey, "optout")) {
                    optout = zend_is_true(z_ele);
                } else if (zend_string_equals_literal_ci(zkey, "noloop")) {
                    noloop = zend_is_true(z_ele);
                }
            }
        } ZEND_HASH_FOREACH_END();
    }
    cmd = redis_cmd_create_literal(redis_sock, "CLIENT");
    redis_cmd_cat_literal(cmd, "TRACKING");
    if (Z_TYPE(z_args[0]) == IS_STRING && (
        zend_string_equals_literal_ci(Z_STR(z_args[0]), "on") ||
        zend_string_equals_literal_ci(Z_STR(z_args[0]), "off")
    )) {
        redis_cmd_cat_zstr(cmd, Z_STR(z_args[0]));
    } else if (zend_is_true(&z_args[0])) {
        redis_cmd_cat_literal(cmd, "ON");
    } else {
        redis_cmd_cat_literal(cmd, "OFF");
    }
    if (redirect != NULL) {
        redis_cmd_cat_literal(cmd, "REDIRECT");
        redis_cmd_cat_zstr(cmd, Z_STR_P(redirect));
    }
    if (prefix != NULL) {
        if (Z_TYPE_P(prefix) == IS_ARRAY) {
            ZEND_HASH_FOREACH_VAL(Z_ARRVAL_P(prefix), z_ele) {
                redis_cmd_cat_literal(cmd, "PREFIX");
                if (Z_TYPE_P(z_ele) == IS_STRING) {
                    redis_cmd_cat_zstr(cmd, Z_STR_P(z_ele));
                } else {
                    zkey = zval_get_string(z_ele);
                    redis_cmd_cat_zstr(cmd, zkey);
                    zend_string_release(zkey);
                }
            } ZEND_HASH_FOREACH_END();
        } else {
            redis_cmd_cat_literal(cmd, "PREFIX");
            redis_cmd_cat_zstr(cmd, Z_STR_P(prefix));
        }
    }
    redis_cmd_cat_literal_if(cmd, bcast, "BCAST");
    redis_cmd_cat_literal_if(cmd, optin, "OPTIN");
    redis_cmd_cat_literal_if(cmd, optout, "OPTOUT");
    redis_cmd_cat_literal_if(cmd, noloop, "NOLOOP");

    return cmd;
}

RedisCmd *
redis_client_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock)
{
    zend_string *op = NULL;
    zval *z_args = NULL;
    RedisCmd *cmd = NULL;
    void *ctx = NULL;
    int argc = 0;

    ZEND_PARSE_PARAMETERS_START(1, -1)
        Z_PARAM_STR(op)
        Z_PARAM_OPTIONAL
        Z_PARAM_VARIADIC('*', z_args, argc)
    ZEND_PARSE_PARAMETERS_END_EX(return NULL);

    if (zend_string_equals_literal_ci(op, "INFO")) {
        cmd = redis_cmd_create_literal(redis_sock, "CLIENT");
        redis_cmd_cat_literal(cmd, "INFO");
    } else if (zend_string_equals_literal_ci(op, "LIST")) {
        cmd = redis_build_client_list_command(redis_sock, argc, z_args);
        ctx = PHPREDIS_CTX_PTR;
    } else if (zend_string_equals_literal_ci(op, "CACHING")) {
        if (argc < 1) {
            return NULL;
        }
        cmd = redis_cmd_create_literal(redis_sock, "CLIENT");
        redis_cmd_cat_literal(cmd, "CACHING");
        if (Z_TYPE(z_args[0]) == IS_STRING && (
            zend_string_equals_literal_ci(Z_STR(z_args[0]), "yes") ||
            zend_string_equals_literal_ci(Z_STR(z_args[0]), "no")
        )) {
            redis_cmd_cat_zstr(cmd, Z_STR(z_args[0]));
        } else if (zend_is_true(&z_args[0])) {
            redis_cmd_cat_literal(cmd, "YES");
        } else {
            redis_cmd_cat_literal(cmd, "NO");
        }
        ctx = PHPREDIS_CTX_PTR + 1;
    } else if (zend_string_equals_literal_ci(op, "GETNAME")) {
        cmd = redis_cmd_create_literal(redis_sock, "CLIENT");
        redis_cmd_cat_literal(cmd, "GETNAME");
        ctx = PHPREDIS_CTX_PTR + 3;
    } else if (zend_string_equals_literal_ci(op, "GETREDIR") || zend_string_equals_literal_ci(op, "ID")) {
        cmd = redis_cmd_create_literal(redis_sock, "CLIENT");
        redis_cmd_cat_zstr(cmd, op);
        ctx = PHPREDIS_CTX_PTR + 2;
    } else if (zend_string_equals_literal_ci(op, "KILL")) {
        cmd = redis_build_client_kill_command(redis_sock, argc, z_args);
        ctx = PHPREDIS_CTX_PTR + 1;
    } else if (zend_string_equals_literal_ci(op, "NO-EVICT")) {
        if (argc < 1) {
            return NULL;
        }
        cmd = redis_cmd_create_literal(redis_sock, "CLIENT");
        redis_cmd_cat_literal(cmd, "NO-EVICT");
        if (Z_TYPE(z_args[0]) == IS_STRING && (
            zend_string_equals_literal_ci(Z_STR(z_args[0]), "on") ||
            zend_string_equals_literal_ci(Z_STR(z_args[0]), "off")
        )) {
            redis_cmd_cat_zstr(cmd, Z_STR(z_args[0]));
        } else if (zend_is_true(&z_args[0])) {
            redis_cmd_cat_literal(cmd, "ON");
        } else {
            redis_cmd_cat_literal(cmd, "OFF");
        }
        ctx = PHPREDIS_CTX_PTR + 1;
    } else if (zend_string_equals_literal_ci(op, "PAUSE")) {
        if (argc < 1 || Z_TYPE(z_args[0]) != IS_LONG || (
            argc > 1 && (
                Z_TYPE(z_args[1]) != IS_STRING || (
                    !zend_string_equals_literal_ci(Z_STR(z_args[1]), "write") &&
                    !zend_string_equals_literal_ci(Z_STR(z_args[1]), "all")
                )
            )
        )) {
            return NULL;
        }
        cmd = redis_cmd_create_literal(redis_sock, "CLIENT");
        redis_cmd_cat_literal(cmd, "PAUSE");
        redis_cmd_cat_long(cmd, Z_LVAL(z_args[0]));
        if (argc > 1) {
            redis_cmd_cat_zstr(cmd, Z_STR(z_args[1]));
        }
        ctx = PHPREDIS_CTX_PTR + 1;
    } else if (zend_string_equals_literal_ci(op, "REPLY")) {
        if (argc > 0 && (
            Z_TYPE(z_args[0]) != IS_STRING || (
                !zend_string_equals_literal_ci(Z_STR(z_args[0]), "on") &&
                !zend_string_equals_literal_ci(Z_STR(z_args[0]), "off") &&
                !zend_string_equals_literal_ci(Z_STR(z_args[0]), "skip")
            )
        )) {
            return NULL;
        }
        cmd = redis_cmd_create_literal(redis_sock, "CLIENT");
        redis_cmd_cat_literal(cmd, "REPLY");
        if (argc > 0) {
            redis_cmd_cat_zstr(cmd, Z_STR(z_args[0]));
        }
        ctx = PHPREDIS_CTX_PTR + 1;
    } else if (zend_string_equals_literal_ci(op, "SETNAME")) {
        if (argc < 1 || Z_TYPE(z_args[0]) != IS_STRING) {
            return NULL;
        }
        cmd = redis_cmd_create_literal(redis_sock, "CLIENT");
        redis_cmd_cat_literal(cmd, "SETNAME");
        redis_cmd_cat_zstr(cmd, Z_STR(z_args[0]));
        ctx = PHPREDIS_CTX_PTR + 1;
    } else if (zend_string_equals_literal_ci(op, "TRACKING")) {
        cmd = redis_build_client_tracking_command(redis_sock, argc, z_args);
        ctx = PHPREDIS_CTX_PTR + 1;
    } else if (zend_string_equals_literal_ci(op, "TRACKINGINFO")) {
        cmd = redis_cmd_create_literal(redis_sock, "CLIENT");
        redis_cmd_cat_literal(cmd, "TRACKINGINFO");
        ctx = PHPREDIS_CTX_PTR + 4;
    } else if (zend_string_equals_literal_ci(op, "UNBLOCK")) {
        if (argc < 1 || Z_TYPE(z_args[0]) != IS_STRING || (
            argc > 1 && (
                Z_TYPE(z_args[1]) != IS_STRING || (
                    !zend_string_equals_literal_ci(Z_STR(z_args[1]), "timeout") &&
                    !zend_string_equals_literal_ci(Z_STR(z_args[1]), "error")
                )
            )
        )) {
            return NULL;
        }
        cmd = redis_cmd_create_literal(redis_sock, "CLIENT");
        redis_cmd_cat_literal(cmd, "UNBLOCK");
        redis_cmd_cat_zstr(cmd, Z_STR(z_args[0]));
        if (argc > 1) {
            redis_cmd_cat_zstr(cmd, Z_STR(z_args[1]));
        }
        ctx = PHPREDIS_CTX_PTR + 2;
    } else if (zend_string_equals_literal_ci(op, "UNPAUSE")) {
        cmd = redis_cmd_create_literal(redis_sock, "CLIENT");
        redis_cmd_cat_literal(cmd, "UNPAUSE");
        ctx = PHPREDIS_CTX_PTR + 1;
    } else {
        return NULL;
    }

    if (cmd == NULL)
        return NULL;

    redis_cmd_set_ctx(cmd, ctx);

    return cmd;
}

/* COMMAND */
RedisCmd *redis_command_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock)
{
    zend_string *op = NULL, *zstr;
    zval *z_args = NULL;
    RedisCmd *cmd;
    void *ctx = NULL;
    int i, argc = 0;

    ZEND_PARSE_PARAMETERS_START(0, -1)
        Z_PARAM_OPTIONAL
        Z_PARAM_STR(op)
        Z_PARAM_VARIADIC('*', z_args, argc)
    ZEND_PARSE_PARAMETERS_END_EX(return NULL);

    if (op == NULL) {
        ctx = NULL;
        argc = 0;
    } else if (zend_string_equals_literal_ci(op, "COUNT")) {
        ctx = PHPREDIS_CTX_PTR;
        argc = 0;
    } else if (zend_string_equals_literal_ci(op, "DOCS") ||
        zend_string_equals_literal_ci(op, "INFO")
    ) {
        ctx = NULL;
    } else if (zend_string_equals_literal_ci(op, "GETKEYS") ||
        zend_string_equals_literal_ci(op, "LIST")
    ) {
        ctx = PHPREDIS_CTX_PTR + 1;
    } else if (zend_string_equals_literal_ci(op, "GETKEYSANDFLAGS")) {
        ctx = PHPREDIS_CTX_PTR + 2;
    } else {
        php_error_docref(NULL, E_WARNING, "Unknown COMMAND operation '%s'", ZSTR_VAL(op));
        return NULL;
    }

    cmd = redis_cmd_create_literal(redis_sock, "COMMAND");
    if (op) redis_cmd_cat_zstr(cmd, op);

    for (i = 0; i < argc; ++i) {
        zstr = zval_get_string(&z_args[i]);
        redis_cmd_cat_zstr(cmd, zstr);
        zend_string_release(zstr);
    }

    /* Any slot will do */
    redis_cmd_randslot(cmd);
    redis_cmd_set_ctx(cmd, ctx);

    return cmd;
}

RedisCmd *
redis_copy_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock)
{
    zend_string *src = NULL, *dst = NULL;
    HashTable *opts = NULL;
    zend_bool replace = 0;
    zend_string *zkey;
    zend_long db = -1;
    RedisCmd *cmd;
    zval *zv;

    ZEND_PARSE_PARAMETERS_START(2, 3)
        Z_PARAM_STR(src)
        Z_PARAM_STR(dst)
        Z_PARAM_OPTIONAL
        Z_PARAM_ARRAY_HT_OR_NULL(opts)
    ZEND_PARSE_PARAMETERS_END_EX(return NULL);

    if (opts != NULL) {
        ZEND_HASH_FOREACH_STR_KEY_VAL(opts, zkey, zv) {
            if (zkey == NULL)
                continue;

            ZVAL_DEREF(zv);
            if (zend_string_equals_literal_ci(zkey, "db")) {
                db = zval_get_long(zv);
            } else if (zend_string_equals_literal_ci(zkey, "replace")) {
                replace = zend_is_true(zv);
            }
        } ZEND_HASH_FOREACH_END();
    }

    if (redis_sock && redis_sock->type == REDIS_SOCK_CLUSTER && db != -1) {
        php_error_docref(NULL, E_WARNING, "Can't copy to a specific DB in cluster mode");
        return NULL;
    }

    cmd = redis_cmd_create_literal(redis_sock, "COPY");
    redis_cmd_cat_key_zstr(cmd, src);
    redis_cmd_try_cat_key_zstr(cmd, dst);

    if (db > -1) {
        redis_cmd_cat_literal(cmd, "DB");
        redis_cmd_cat_long(cmd, db);
    }
    redis_cmd_cat_literal_if(cmd, replace, "REPLACE");

    return cmd;
}

static xdelExMode zstr_to_xdelex_mode(zend_string *s) {
    if (s == NULL)
        return REDIS_XDELEX_NONE;
    else if (zend_string_equals_literal_ci(s, "KEEPREF"))
        return REDIS_XDELEX_KEEPREF;
    else if (zend_string_equals_literal_ci(s, "DELREF"))
        return REDIS_XDELEX_DELREF;
    else if (zend_string_equals_literal_ci(s, "ACKED"))
        return REDIS_XDELEX_ACKED;

    php_error_docref(NULL, E_WARNING, "Unknown mode '%s'", ZSTR_VAL(s));

    return REDIS_XDELEX_NONE;
}

void redis_cmd_cat_delex_mode(RedisCmd *cmd, xdelExMode mode) {
    if (mode == REDIS_XDELEX_KEEPREF) {
        redis_cmd_cat_literal(cmd, "KEEPREF");
    } else if (mode == REDIS_XDELEX_DELREF) {
        redis_cmd_cat_literal(cmd, "DELREF");
    } else if (mode == REDIS_XDELEX_ACKED) {
        redis_cmd_cat_literal(cmd, "ACKED");
    }
}

RedisCmd *
redis_xdelex_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock) {
    xdelExMode mode = REDIS_XDELEX_NONE;
    zend_string *key, *mstr = NULL;
    HashTable *ids;
    RedisCmd *cmd;
    zval *id;

    ZEND_PARSE_PARAMETERS_START(2, 3)
        Z_PARAM_STR(key)
        Z_PARAM_ARRAY_HT(ids)
        Z_PARAM_OPTIONAL
        Z_PARAM_STR(mstr)
    ZEND_PARSE_PARAMETERS_END_EX(return NULL);

    if (zend_hash_num_elements(ids) == 0) {
        php_error_docref(NULL, E_WARNING, "At least one ID must be specified");
        return NULL;
    }

    mode = zstr_to_xdelex_mode(mstr);

    cmd = redis_cmd_create_literal(redis_sock, "XDELEX");
    redis_cmd_cat_key_zstr(cmd, key);

    redis_cmd_cat_delex_mode(cmd, mode);

    redis_cmd_cat_literal(cmd, "IDS");
    redis_cmd_cat_long(cmd, zend_hash_num_elements(ids));

    ZEND_HASH_FOREACH_VAL(ids, id) {
        redis_cmd_cat_zval_zstr(cmd, id);
    } ZEND_HASH_FOREACH_END();

    return cmd;
}

// XACKDEL key group [KEEPREF | DELREF | ACKED] IDS numids id [id ...]
RedisCmd *
redis_xackdel_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock) {
    zend_string *key, *group, *mode = NULL;
    xdelExMode dtype = REDIS_XDELEX_NONE;
    HashTable *ids;
    RedisCmd *cmd;
    zval *id;

    ZEND_PARSE_PARAMETERS_START(3, 4)
        Z_PARAM_STR(key)
        Z_PARAM_STR(group)
        Z_PARAM_ARRAY_HT(ids)
        Z_PARAM_OPTIONAL
        Z_PARAM_STR(mode)
    ZEND_PARSE_PARAMETERS_END_EX(return NULL);

    if (zend_hash_num_elements(ids) == 0) {
        php_error_docref(NULL, E_WARNING, "At least one ID must be specified");
        return NULL;
    }

    dtype = zstr_to_xdelex_mode(mode);

    cmd = redis_cmd_create_literal(redis_sock, "XACKDEL");

    redis_cmd_cat_key_zstr(cmd, key);
    redis_cmd_cat_zstr(cmd, group);
    redis_cmd_cat_delex_mode(cmd, dtype);

    redis_cmd_cat_literal(cmd, "IDS");
    redis_cmd_cat_long(cmd, zend_hash_num_elements(ids));

    ZEND_HASH_FOREACH_VAL(ids, id) {
        redis_cmd_cat_zval_zstr(cmd, id);
    } ZEND_HASH_FOREACH_END();

    return cmd;
}


/* XADD */
RedisCmd *redis_xadd_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock) {
    zend_string *key, *id, *arrkey;
    zend_long maxlen = 0;
    zend_bool approx = 0, nomkstream = 0;
    zend_ulong idx;
    HashTable *fields;
    RedisCmd *cmd;
    zval *value;
    int fcount;

    ZEND_PARSE_PARAMETERS_START(3, 6)
        Z_PARAM_STR(key)
        Z_PARAM_STR(id)
        Z_PARAM_ARRAY_HT(fields)
        Z_PARAM_OPTIONAL
        Z_PARAM_LONG(maxlen)
        Z_PARAM_BOOL(approx)
        Z_PARAM_BOOL(nomkstream)
    ZEND_PARSE_PARAMETERS_END_EX(return NULL);

    /* At least one field and string are required */
    if ((fcount = zend_hash_num_elements(fields)) == 0) {
        return NULL;
    }

    if (maxlen < 0 || (maxlen == 0 && approx != 0)) {
        php_error_docref(NULL, E_WARNING,
            "Warning:  Invalid MAXLEN argument or approximate flag");
    }

    /* XADD key ID field string [field string ...] */
    cmd = redis_cmd_create_literal(redis_sock, "XADD");
    redis_cmd_cat_key_zstr(cmd, key);

    redis_cmd_cat_literal_if(cmd, nomkstream, "NOMKSTREAM");

    /* Now append our MAXLEN bits if we've got them */
    if (maxlen > 0) {
        redis_cmd_cat_literal(cmd, "MAXLEN");
        redis_cmd_cat_literal_if(cmd, approx, "~");
        redis_cmd_cat_long(cmd, maxlen);
    }

    /* Now append ID and field(s) */
    redis_cmd_cat_zstr(cmd, id);
    ZEND_HASH_FOREACH_KEY_VAL(fields, idx, arrkey, value) {
        redis_cmd_cat_arrkey(cmd, arrkey, idx);
        redis_cmd_cat_zval(cmd, value);
    } ZEND_HASH_FOREACH_END();

    return cmd;
}

// XPENDING key group [start end count [consumer] [idle]]
RedisCmd *redis_xpending_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock)
{
    zend_string *key = NULL, *group = NULL, *start = NULL, *end = NULL,
                *consumer = NULL;
    zend_long count = -1, idle = 0;
    RedisCmd *cmd;

    ZEND_PARSE_PARAMETERS_START(2, 7)
        Z_PARAM_STR(key)
        Z_PARAM_STR(group)
        Z_PARAM_OPTIONAL
        Z_PARAM_STR_OR_NULL(start)
        Z_PARAM_STR_OR_NULL(end)
        Z_PARAM_LONG(count)
        Z_PARAM_STR_OR_NULL(consumer)
        Z_PARAM_LONG(idle)
    ZEND_PARSE_PARAMETERS_END_EX(return NULL);

    /* If we've been passed a start argument, we also need end and count */
    if (start != NULL && (end == NULL || count < 0)) {
        php_error_docref(NULL, E_WARNING, "'$start' must be accompanied by '$end' and '$count' arguments");
        return NULL;
    }

    /* Construct command and add required arguments */
    cmd = redis_cmd_create_literal(redis_sock, "XPENDING");
    redis_cmd_cat_key_zstr(cmd, key);
    redis_cmd_cat_zstr(cmd, group);

    /* Add optional argumentst */
    if (start) {
        if (idle != 0) {
            redis_cmd_cat_literal(cmd, "IDLE");
            redis_cmd_cat_long(cmd, idle);
        }
        redis_cmd_cat_zstr(cmd, start);
        redis_cmd_cat_zstr(cmd, end);
        redis_cmd_cat_long(cmd, count);

        /* Finally add consumer if we have it */
        if (consumer) redis_cmd_cat_zstr(cmd, consumer);
    }

    return cmd;
}

/* X[REV]RANGE key start end [COUNT count] */
static RedisCmd *
redis_xrange_generic_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock, const char *kw, size_t kw_len, zend_bool have_count_literal)
{
    zend_string *key, *start, *end;
    zend_long count = -1;
    RedisCmd *cmd;

    ZEND_PARSE_PARAMETERS_START(3, 4)
        Z_PARAM_STR(key)
        Z_PARAM_STR(start)
        Z_PARAM_STR(end)
        Z_PARAM_OPTIONAL
        Z_PARAM_LONG(count)
    ZEND_PARSE_PARAMETERS_END_EX(return NULL);

    cmd = redis_cmd_create(redis_sock, kw, kw_len);

    redis_cmd_cat_key_zstr(cmd, key);
    redis_cmd_cat_zstr(cmd, start);
    redis_cmd_cat_zstr(cmd, end);

    if (count > -1) {
        redis_cmd_cat_literal_if(cmd, have_count_literal, "COUNT");
        redis_cmd_cat_long(cmd, count);
    }

    return cmd;
}

RedisCmd *redis_xrange_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock, const char *kw, size_t kw_len)
{
    return redis_xrange_generic_cmd(INTERNAL_FUNCTION_PARAM_PASSTHRU,
                                    redis_sock, kw, kw_len, 1);
}

RedisCmd *redis_vrange_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock, const char *kw, size_t kw_len)
{
    return redis_xrange_generic_cmd(INTERNAL_FUNCTION_PARAM_PASSTHRU,
                                    redis_sock, kw, kw_len, 0);
}

/* Helper function to take an associative array and append the Redis
 * STREAMS stream [stream...] id [id ...] arguments to a command string. */
static zend_bool redis_cmd_cat_stream_args(RedisCmd *cmd, HashTable *ht) {
    zend_string *key;
    zend_ulong idx;
    zval *zid;

    /* Append STREAM qualifier */
    redis_cmd_cat_literal(cmd, "STREAMS");

    /* Iterate over our stream => id array appending streams and retaining each
     * value for final arguments */
    ZEND_HASH_FOREACH_KEY_VAL(ht, idx, key, zid) {
        if (!redis_cmd_cat_key_arrkey(cmd, key, idx)) {
            redis_crosslot_warning();
            return 0;
        }
    } ZEND_HASH_FOREACH_END();

    ZEND_HASH_FOREACH_VAL(ht, zid) {
        redis_cmd_cat_zval_zstr(cmd, zid);
    } ZEND_HASH_FOREACH_END();

    return 1;
}

/* XREAD [COUNT count] [BLOCK ms] STREAMS key [key ...] id [id ...] */
RedisCmd *redis_xread_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock) {
    zend_long count = -1, block = -1;
    HashTable *streams;
    RedisCmd *cmd;
    int scount;

    ZEND_PARSE_PARAMETERS_START(1, -1)
        Z_PARAM_ARRAY_HT(streams)
        Z_PARAM_OPTIONAL
        Z_PARAM_LONG(count)
        Z_PARAM_LONG(block)
    ZEND_PARSE_PARAMETERS_END_EX(return NULL);

    /* At least one stream and ID is required */
    if ((scount = zend_hash_num_elements(streams)) < 1) {
        return NULL;
    }

    /* Calculate argc and start constructing command */
    cmd = redis_cmd_create_literal(redis_sock, "XREAD");

    /* Append COUNT if we have it */
    if (count > -1) {
        redis_cmd_cat_literal(cmd, "COUNT");
        redis_cmd_cat_long(cmd, count);
    }

    /* Append BLOCK if we have it */
    if (block > -1) {
        redis_cmd_cat_literal(cmd, "BLOCK");
        redis_cmd_cat_long(cmd, block);
    }

    /* Append final STREAM key [key ...] id [id ...] arguments */
    if (!redis_cmd_cat_stream_args(cmd, streams)) {
        redis_cmd_free(cmd);
        return NULL;
    }

    return cmd;
}

/* XREADGROUP GROUP group consumer [COUNT count] [BLOCK ms]
 * STREAMS key [key ...] id [id ...] */
RedisCmd *redis_xreadgroup_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock) {
    zend_bool count_is_null = 1, block_is_null = 1;
    zend_string *group, *consumer;
    zend_long count, block;
    HashTable *streams;
    RedisCmd *cmd;
    int scount;

    ZEND_PARSE_PARAMETERS_START(3, 5)
        Z_PARAM_STR(group)
        Z_PARAM_STR(consumer)
        Z_PARAM_ARRAY_HT(streams)
        Z_PARAM_OPTIONAL
        Z_PARAM_LONG_OR_NULL(count, count_is_null)
        Z_PARAM_LONG_OR_NULL(block, block_is_null)
    ZEND_PARSE_PARAMETERS_END_EX(return NULL);

    /* Negative COUNT or BLOCK is illegal so abort immediately */
    if ((!count_is_null && count < 0) || (!block_is_null && block < 0)) {
        php_error_docref(NULL, E_WARNING, "Negative values for COUNT or BLOCK are illegal.");
        return NULL;
    }

    /* Redis requires at least one stream */
    if ((scount = zend_hash_num_elements(streams)) < 1) {
        return NULL;
    }

    /* Calculate argc and start constructing commands */
    cmd = redis_cmd_create_literal(redis_sock, "XREADGROUP");

    /* Group and consumer */
    redis_cmd_cat_literal(cmd, "GROUP");
    redis_cmd_cat_zstr(cmd, group);
    redis_cmd_cat_zstr(cmd, consumer);

    /* Append COUNT if we have it */
    if (!count_is_null) {
        redis_cmd_cat_literal(cmd, "COUNT");
        redis_cmd_cat_long(cmd, count);
    }

    /* Append BLOCK argument if we have it */
    if (!block_is_null) {
        redis_cmd_cat_literal(cmd, "BLOCK");
        redis_cmd_cat_long(cmd, block);
    }

    /* Finally append stream and id args */
    if (!redis_cmd_cat_stream_args(cmd, streams)) {
        redis_cmd_free(cmd);
        return NULL;
    }

    return cmd;
}

/* XACK key group id [id ...] */
RedisCmd *redis_xack_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock)
{
    zend_string *key, *group;
    zval *z_ids, *z_id;
    HashTable *ht_ids;
    RedisCmd *cmd;
    int idcount;

    ZEND_PARSE_PARAMETERS_START(3, 3)
        Z_PARAM_STR(key)
        Z_PARAM_STR(group)
        Z_PARAM_ARRAY(z_ids)
    ZEND_PARSE_PARAMETERS_END_EX(return NULL);

    ht_ids = Z_ARRVAL_P(z_ids);
    if ((idcount = zend_hash_num_elements(ht_ids)) < 1) {
        return NULL;
    }

    /* Create command and add initial arguments */
    cmd = redis_cmd_create_literal(redis_sock, "XACK");
    redis_cmd_cat_key_zstr(cmd, key);
    redis_cmd_cat_zstr(cmd, group);

    /* Append IDs */
    ZEND_HASH_FOREACH_VAL(ht_ids, z_id) {
        redis_cmd_cat_zval_zstr(cmd, z_id);
    } ZEND_HASH_FOREACH_END();

    return cmd;
}

/* XCLAIM options container */
typedef struct xclaimOptions {
    struct {
        char *type;
        int64_t time;
    } idle;
    zend_long retrycount;
    int force;
    int justid;
} xclaimOptions;

/* Attempt to extract an int64_t from the provided zval */
static int zval_get_i64(zval *zv, int64_t *retval) {
    if (Z_TYPE_P(zv) == IS_LONG) {
        *retval = (int64_t)Z_LVAL_P(zv);
        return SUCCESS;
    } else if (Z_TYPE_P(zv) == IS_DOUBLE) {
        *retval = (int64_t)Z_DVAL_P(zv);
        return SUCCESS;
    } else if (Z_TYPE_P(zv) == IS_STRING) {
        zend_long lval;
        double dval;

        switch (is_numeric_string(Z_STRVAL_P(zv), Z_STRLEN_P(zv), &lval, &dval, 1)) {
            case IS_LONG:
                *retval = (int64_t)lval;
                return SUCCESS;
            case IS_DOUBLE:
                *retval = (int64_t)dval;
                return SUCCESS;
        }
    }

    /* If we make it here we have failed */
    return FAILURE;
}

/* Helper function to get an integer XCLAIM argument.  This can overflow a
 * 32-bit PHP long so we have to extract it as an int64_t.  If the value is
 * not a valid number or negative, we'll inform the user of the problem and
 * that the argument is being ignored. */
static int64_t get_xclaim_i64_arg(const char *key, zval *zv) {
    int64_t retval = -1;

    /* Extract an i64, and if we can't let the user know there is an issue. */
    if (zval_get_i64(zv, &retval) == FAILURE || retval < 0) {
        php_error_docref(NULL, E_WARNING,
            "Invalid XCLAIM option '%s' will be ignored", key);
    }

    return retval;
}

/* Helper to extract XCLAIM options */
static void get_xclaim_options(xclaimOptions *dst, HashTable *ht) {
    zend_string *zkey;
    zval *zv;

    /* Initialize options array to sane defaults */
    memset(dst, 0, sizeof(*dst));
    dst->retrycount = -1;
    dst->idle.time = -1;

    /* Early return if we don't have any options */
    if (ht == NULL)
        return;

    /* Iterate over our options array */
    ZEND_HASH_FOREACH_STR_KEY_VAL(ht, zkey, zv) {
        if (zkey) {
            if (zend_string_equals_literal_ci(zkey, "TIME")) {
                dst->idle.type = "TIME";
                dst->idle.time = get_xclaim_i64_arg("TIME", zv);
            } else if (zend_string_equals_literal_ci(zkey, "IDLE")) {
                dst->idle.type = "IDLE";
                dst->idle.time = get_xclaim_i64_arg("IDLE", zv);
            } else if (zend_string_equals_literal_ci(zkey, "RETRYCOUNT")) {
                dst->retrycount = zval_get_long(zv);
            }
        } else if (Z_TYPE_P(zv) == IS_STRING) {
            if (zend_string_equals_literal_ci(Z_STR_P(zv), "FORCE")) {
                dst->force = 1;
            } else if (zend_string_equals_literal_ci(Z_STR_P(zv), "JUSTID")) {
                dst->justid = 1;
            }
        }
    } ZEND_HASH_FOREACH_END();
}

/* Append XCLAIM options */
static void redis_cmd_cat_xclaim_options(RedisCmd *cmd, xclaimOptions *opt) {
    /* IDLE/TIME long */
    if (opt->idle.type != NULL && opt->idle.time != -1) {
        redis_cmd_cat_str(cmd, opt->idle.type, strlen(opt->idle.type));
        redis_cmd_cat_long(cmd, opt->idle.time);
    }

    /* RETRYCOUNT */
    if (opt->retrycount != -1) {
        redis_cmd_cat_literal(cmd, "RETRYCOUNT");
        redis_cmd_cat_long(cmd, opt->retrycount);
    }

    /* FORCE and JUSTID */
    redis_cmd_cat_literal_if(cmd, opt->force, "FORCE");
    redis_cmd_cat_literal_if(cmd, opt->justid, "JUSTID");
}


RedisCmd *
redis_xautoclaim_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock) {
    zend_string *key, *group, *consumer, *start;
    zend_long min_idle, count = -1;
    zend_bool justid = 0;
    RedisCmd *cmd;

    ZEND_PARSE_PARAMETERS_START(5, 7)
        Z_PARAM_STR(key)
        Z_PARAM_STR(group)
        Z_PARAM_STR(consumer)
        Z_PARAM_LONG(min_idle)
        Z_PARAM_STR(start)
        Z_PARAM_OPTIONAL
        Z_PARAM_LONG(count)
        Z_PARAM_BOOL(justid)
    ZEND_PARSE_PARAMETERS_END_EX(return NULL);

    cmd = redis_cmd_create_literal(redis_sock, "XAUTOCLAIM");
    redis_cmd_cat_key_zstr(cmd, key);
    redis_cmd_cat_zstr(cmd, group);
    redis_cmd_cat_zstr(cmd, consumer);
    redis_cmd_cat_long(cmd, min_idle);
    redis_cmd_cat_zstr(cmd, start);

    if (count > 0) {
        redis_cmd_cat_literal(cmd, "COUNT");
        redis_cmd_cat_long(cmd, count);
    }

    redis_cmd_cat_literal_if(cmd, justid, "JUSTID");

    redis_cmd_set_ctx(cmd, PHPREDIS_CTX_PTR);

    return cmd;
}

/* XCLAIM <key> <group> <consumer> <min-idle-time> <ID-1> <ID-2>
          [IDLE <milliseconds>] [TIME <mstime>] [RETRYCOUNT <count>]
          [FORCE] [JUSTID] */
RedisCmd *redis_xclaim_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock)
{
    zend_string *key, *group, *consumer;
    HashTable *ids, *htopts = NULL;
    zend_long min_idle;
    xclaimOptions opts;
    RedisCmd *cmd;
    int id_count;
    zval *zv;

    ZEND_PARSE_PARAMETERS_START(5, 6)
        Z_PARAM_STR(key)
        Z_PARAM_STR(group)
        Z_PARAM_STR(consumer)
        Z_PARAM_LONG(min_idle)
        Z_PARAM_ARRAY_HT(ids)
        Z_PARAM_OPTIONAL
        Z_PARAM_ARRAY_HT_OR_NULL(htopts)
    ZEND_PARSE_PARAMETERS_END_EX(return NULL);

    /* At least one id is required */
    if ((id_count = zend_hash_num_elements(ids)) < 1) {
        return NULL;
    }

    /* Extract options array if we've got them */
    get_xclaim_options(&opts, htopts);

    /* Start constructing our command */
    cmd = redis_cmd_create_literal(redis_sock, "XCLAIM");
    redis_cmd_cat_key_zstr(cmd, key);
    redis_cmd_cat_zstr(cmd, group);
    redis_cmd_cat_zstr(cmd, consumer);
    redis_cmd_cat_long(cmd, min_idle);

    /* Add IDs */
    ZEND_HASH_FOREACH_VAL(ids, zv) {
        redis_cmd_cat_zval_zstr(cmd, zv);
    } ZEND_HASH_FOREACH_END();

    /* Finally add our options */
    redis_cmd_cat_xclaim_options(cmd, &opts);

	return cmd;
}

/* XGROUP HELP
 * XGROUP CREATE          key group id       [MKSTREAM] [ENTRIESREAD <n>]
 * XGROUP SETID           key group id                  [ENTRIESREAD <n>]
 * XGROUP CREATECONSUMER  key group consumer
 * XGROUP DELCONSUMER     key group consumer
 * XGROUP DESTROY         key group
 */
RedisCmd *redis_xgroup_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock)
{
    zend_string *op = NULL, *key = NULL, *group = NULL, *id_or_consumer = NULL;
    int nargs, is_create = 0, is_setid = 0;
    zend_long entries_read = -2;
    zend_bool mkstream = 0;
    RedisCmd *cmd;

    ZEND_PARSE_PARAMETERS_START(1, 6)
        Z_PARAM_STR(op)
        Z_PARAM_OPTIONAL
        Z_PARAM_STR(key)
        Z_PARAM_STR(group)
        Z_PARAM_STR(id_or_consumer)
        Z_PARAM_BOOL(mkstream)
        Z_PARAM_LONG(entries_read)
    ZEND_PARSE_PARAMETERS_END_EX(return NULL);

    if (zend_string_equals_literal_ci(op, "HELP")) {
        nargs = 0;
    } else if ((is_create = zend_string_equals_literal_ci(op, "CREATE")) ||
               (is_setid  = zend_string_equals_literal_ci(op, "SETID")) ||
                            zend_string_equals_literal_ci(op, "CREATECONSUMER") ||
                            zend_string_equals_literal_ci(op, "DELCONSUMER"))
    {
        nargs = 3;
    } else if (zend_string_equals_literal_ci(op, "DESTROY")) {
        nargs = 2;
    } else {
        php_error_docref(NULL, E_WARNING, "Unknown XGROUP operation '%s'", ZSTR_VAL(op));
        return NULL;
    }

    if (ZEND_NUM_ARGS() < nargs) {
        php_error_docref(NULL, E_WARNING, "Operation '%s' requires %d arguments", ZSTR_VAL(op), nargs);
        return NULL;
    }

    mkstream &= is_create;
    if (!(is_create || is_setid))
        entries_read = -2;

    cmd = redis_cmd_create_literal(redis_sock, "XGROUP");
    redis_cmd_cat_zstr(cmd, op);

    if (nargs-- > 0) redis_cmd_cat_key_zstr(cmd, key);
    if (nargs-- > 0) redis_cmd_cat_zstr(cmd, group);
    if (nargs-- > 0) redis_cmd_cat_zstr(cmd, id_or_consumer);

    redis_cmd_cat_literal_if(cmd, !!mkstream, "MKSTREAM");
    if (entries_read != -2) {
        redis_cmd_cat_literal(cmd, "ENTRIESREAD");
        redis_cmd_cat_long(cmd, entries_read);
    }

    return cmd;
}

/* XINFO CONSUMERS key group
 * XINFO GROUPS key
 * XINFO STREAM key [FULL [COUNT N]]
 * XINFO HELP */
RedisCmd *redis_xinfo_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock) {
    zend_string *op = NULL, *key = NULL, *arg = NULL;
    zend_long count = -1;
    RedisCmd *cmd;

    ZEND_PARSE_PARAMETERS_START(1, 4)
        Z_PARAM_STR(op)
        Z_PARAM_OPTIONAL
        Z_PARAM_STR_OR_NULL(key)
        Z_PARAM_STR_OR_NULL(arg)
        Z_PARAM_LONG(count)
    ZEND_PARSE_PARAMETERS_END_EX(return NULL);

    if ((arg != NULL && key == NULL) || (count != -1 && (key == NULL || arg == NULL))) {
        php_error_docref(NULL, E_WARNING,
            "Cannot pass a non-null optional argument after a NULL one.");
        return NULL;
    }

    cmd = redis_cmd_create_literal(redis_sock, "XINFO");
    redis_cmd_cat_zstr(cmd, op);

    if (key != NULL)
        redis_cmd_cat_key_zstr(cmd, key);
    if (arg != NULL)
        redis_cmd_cat_zstr(cmd, arg);

    if (count > -1) {
        redis_cmd_cat_literal(cmd, "COUNT");
        redis_cmd_cat_long(cmd, count);
    }

    return cmd;
}

// XTRIM key <MAXLEN | MINID> [= | ~] threshold [LIMIT count]
RedisCmd *redis_xtrim_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock) {
    zend_string *key = NULL, *threshold = NULL;
    zend_bool approx = 0, minid = 0;
    zend_long limit = -1;
    RedisCmd *cmd;

    ZEND_PARSE_PARAMETERS_START(2, 5)
        Z_PARAM_STR(key)
        Z_PARAM_STR(threshold)
        Z_PARAM_OPTIONAL
        Z_PARAM_BOOL(approx)
        Z_PARAM_BOOL(minid)
        Z_PARAM_LONG(limit)
    ZEND_PARSE_PARAMETERS_END_EX(return NULL);

    cmd = redis_cmd_create_literal(redis_sock, "XTRIM");

    redis_cmd_cat_key_zstr(cmd, key);

    if (minid) {
        redis_cmd_cat_literal(cmd, "MINID");
    } else {
        redis_cmd_cat_literal(cmd, "MAXLEN");
    }

    if (approx) {
        redis_cmd_cat_literal(cmd, "~");
    } else {
        redis_cmd_cat_literal(cmd, "=");
    }

    redis_cmd_cat_zstr(cmd, threshold);

    if (limit > -1 && approx) {
        redis_cmd_cat_literal(cmd, "LIMIT");
        redis_cmd_cat_long(cmd, limit);
    } else if (limit > -1) {
        php_error_docref(NULL, E_WARNING,
            "Cannot use LIMIT without an approximate match, ignoring");
    } else if (ZEND_NUM_ARGS() == 5) {
        php_error_docref(NULL, E_WARNING, "Limit must be >= 0");
    }

    return cmd;
}

// [P]EXPIRE[AT] [NX | XX | GT | LT]
RedisCmd *redis_expire_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock, const char *kw, size_t kw_len)
{
    zend_string *key = NULL, *mode = NULL;
    zend_long timeout = 0;
    RedisCmd *cmd;

    ZEND_PARSE_PARAMETERS_START(2, 3)
        Z_PARAM_STR(key)
        Z_PARAM_LONG(timeout)
        Z_PARAM_OPTIONAL
        Z_PARAM_STR_OR_NULL(mode)
    ZEND_PARSE_PARAMETERS_END_EX(return NULL);

    if (mode != NULL && !(zend_string_equals_literal_ci(mode, "NX") ||
                          zend_string_equals_literal_ci(mode, "XX") ||
                          zend_string_equals_literal_ci(mode, "LT") ||
                          zend_string_equals_literal_ci(mode, "GT")))
    {
        php_error_docref(NULL, E_WARNING, "Unknown expiration modifier '%s'", ZSTR_VAL(mode));
        return NULL;
    }

    cmd = redis_cmd_create(redis_sock, kw, kw_len);
    redis_cmd_cat_key_zstr(cmd, key);
    redis_cmd_cat_long(cmd, timeout);
    if (mode != NULL) redis_cmd_cat_zstr(cmd, mode);

    return cmd;
}

static RedisCmd *
generic_expiremember_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock,
                         const char *kw, size_t kw_len, int has_unit)
{
    zend_string *key, *mem, *unit = NULL;
    zend_long expiry;
    RedisCmd *cmd;

    ZEND_PARSE_PARAMETERS_START(3, has_unit ? 4 : 3)
        Z_PARAM_STR(key)
        Z_PARAM_STR(mem)
        Z_PARAM_LONG(expiry)
        if (has_unit) {
            Z_PARAM_OPTIONAL
            Z_PARAM_STR_OR_NULL(unit)
        }
    ZEND_PARSE_PARAMETERS_END_EX(return NULL);

    cmd = redis_cmd_create(redis_sock, kw, kw_len);
    redis_cmd_cat_key_zstr(cmd, key);
    redis_cmd_cat_zstr(cmd, mem);
    redis_cmd_cat_long(cmd, expiry);

    if (unit != NULL) {
        redis_cmd_cat_zstr(cmd, unit);
    }

    return cmd;
}


RedisCmd *
redis_expiremember_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock) {
    return generic_expiremember_cmd(INTERNAL_FUNCTION_PARAM_PASSTHRU,
                                    redis_sock, ZEND_STRL("EXPIREMEMBER"), 1);
}

RedisCmd *
redis_expirememberat_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock) {
    return generic_expiremember_cmd(INTERNAL_FUNCTION_PARAM_PASSTHRU,
                                    redis_sock, ZEND_STRL("EXPIREMEMBERAT"), 0);
}

RedisCmd *
redis_sentinel_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock, const char *kw, size_t kw_len)
{
    if (zend_parse_parameters_none() == FAILURE) {

        return NULL;
    }
    return redis_cmd_fmt(redis_sock, "SENTINEL", "s", kw, kw_len);
}

RedisCmd *
redis_sentinel_str_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock, const char *kw, size_t kw_len)
{
    zend_string *name;

    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_STR(name)
    ZEND_PARSE_PARAMETERS_END_EX(return NULL);

    return redis_cmd_fmt(redis_sock, "SENTINEL", "sS", kw, kw_len, name);
}

typedef enum redisVAddQuant {
    REDIS_QUANT_NONE,
    REDIS_QUANT_NOQUANT,
    REDIS_QUANT_Q8,
    REDIS_QUANT_BIN,
} redisVAddQuant;

typedef struct redisVAddOptions {
    enum redisVAddQuant quant;
    zend_long reduce;
    zend_bool values;
    zend_bool cas;
    zend_long ef;
    zend_string *attributes;
    zend_long numlinks;
} redisVAddOptions;

static zend_bool validate_vadd_integer(zend_string *name, zval *zv, zend_long min) {
    if (Z_TYPE_P(zv) != IS_LONG || Z_LVAL_P(zv) < min) {
        php_error_docref(NULL, E_WARNING, "%s must be >= "
            ZEND_LONG_FMT, ZSTR_VAL(name), min);
        return 0;
    }

    return 1;
}

static zend_string *zval_to_vattr(zval *zv) {
    if (Z_TYPE_P(zv) == IS_STRING) {
        return zval_get_string(zv);
#ifdef HAVE_REDIS_JSON
    } else if (Z_TYPE_P(zv) == IS_ARRAY) {
        smart_str aux = {0};
        php_json_encode(&aux, zv, PHP_JSON_OBJECT_AS_ARRAY);
        return aux.s;
    } else {
        php_error_docref(NULL, E_WARNING, "SETATTR must be a string or array");
    }
#else
    } else {
        php_error_docref(NULL, E_WARNING, "SETATTR must be a string");
    }
#endif

    return NULL;
}

static void parse_vadd_options(redisVAddOptions *dst, HashTable *ht) {
    zend_string *key;
    zval *zv;

    *dst = (redisVAddOptions){
        .ef = -1,
        .numlinks = -1,
        .reduce = -1,
    };

    if (ht == NULL)
        return;

    ZEND_HASH_FOREACH_STR_KEY_VAL(ht, key, zv) {
        if (key != NULL) {
            if (zend_string_equals_literal_ci(key, "REDUCE")) {
                if (validate_vadd_integer(key, zv, 1))
                    dst->reduce = Z_LVAL_P(zv);
            } else if (zend_string_equals_literal_ci(key, "M")) {
                if (validate_vadd_integer(key, zv, 1))
                    dst->numlinks = Z_LVAL_P(zv);
            } else if (zend_string_equals_literal_ci(key, "EF")) {
                if (validate_vadd_integer(key, zv, 1))
                    dst->ef = Z_LVAL_P(zv);
            } else if (zend_string_equals_literal_ci(key, "CAS")) {
                dst->cas = zend_is_true(zv);
            } else if (zend_string_equals_literal_ci(key, "VALUES")) {
                dst->values = zend_is_true(zv);
            } else if (zend_string_equals_literal_ci(key, "SETATTR")) {
                dst->attributes = zval_to_vattr(zv);
            }
        } else if (Z_TYPE_P(zv) == IS_STRING) {
            if (zend_string_equals_literal_ci(Z_STR_P(zv), "VALUES")) {
                dst->values = 1;
            } else if (zend_string_equals_literal_ci(Z_STR_P(zv), "FP32")) {
                dst->values = 0;
            } else if (zend_string_equals_literal_ci(Z_STR_P(zv), "NOQUANT")) {
                dst->quant = REDIS_QUANT_NOQUANT;
            } else if (zend_string_equals_literal_ci(Z_STR_P(zv), "Q8")) {
                dst->quant = REDIS_QUANT_Q8;
            } else if (zend_string_equals_literal_ci(Z_STR_P(zv), "BIN")) {
                dst->quant = REDIS_QUANT_BIN;
            }
        }
    } ZEND_HASH_FOREACH_END();
}

static void free_vadd_options(redisVAddOptions *o) {
    if (o->attributes)
        zend_string_release(o->attributes);
}

static void redis_cmd_cat_fp32(RedisCmd *cmd, HashTable *ht) {
    uint32_t i = 0;
    char *aux;
    zval *zv;

    aux = ecalloc(1, zend_hash_num_elements(ht) * sizeof(float));

    ZEND_HASH_FOREACH_VAL(ht, zv) {
        redis_copy_fp32(&aux[4 * i++], zval_get_double(zv));
    } ZEND_HASH_FOREACH_END();

    redis_cmd_cat_str(cmd, aux, 4 * i);

    efree(aux);
}

static void redis_cmd_cat_fp32_values(RedisCmd *dst, HashTable *ht) {
    zval *zv;

    ZEND_HASH_FOREACH_VAL(ht, zv) {
        redis_cmd_cat_double(dst, zval_get_double(zv));
    } ZEND_HASH_FOREACH_END();
}

RedisCmd *redis_vadd_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock) {
    HashTable *options = NULL, *vector;
    redisVAddOptions opts;
    zend_string *key;
    zval *element;
    RedisCmd *cmd;

    ZEND_PARSE_PARAMETERS_START(3, 4)
        Z_PARAM_STR(key)
        Z_PARAM_ARRAY_HT(vector)
        Z_PARAM_ZVAL(element)
        Z_PARAM_OPTIONAL
        Z_PARAM_ARRAY_HT_OR_NULL(options)
    ZEND_PARSE_PARAMETERS_END_EX(return NULL);

    if (zend_hash_num_elements(vector) < 1) {
        php_error_docref(NULL, E_WARNING,
            "At least one vector element is required");
        return NULL;
    }

    parse_vadd_options(&opts, options);

    cmd = redis_cmd_create_literal(redis_sock, "VADD");
    redis_cmd_cat_key_zstr(cmd, key);

    if (opts.reduce > -1) {
        redis_cmd_cat_literal(cmd, "REDUCE");
        redis_cmd_cat_long(cmd, opts.reduce);
    }

    if (opts.values) {
        redis_cmd_cat_literal(cmd, "VALUES");
        redis_cmd_cat_long(cmd, zend_hash_num_elements(vector));
        redis_cmd_cat_fp32_values(cmd, vector);
    } else {
        redis_cmd_cat_literal(cmd, "FP32");
        redis_cmd_cat_fp32(cmd, vector);
    }

    redis_cmd_cat_zval(cmd, element);

    redis_cmd_cat_literal_if(cmd, opts.cas, "CAS");

    redis_cmd_cat_literal_if(cmd, opts.quant == REDIS_QUANT_Q8, "Q8");
    redis_cmd_cat_literal_if(cmd, opts.quant == REDIS_QUANT_BIN, "BIN");
    redis_cmd_cat_literal_if(cmd, opts.quant == REDIS_QUANT_NOQUANT, "NOQUANT");

    if (opts.ef > -1) {
        redis_cmd_cat_literal(cmd, "EF");
        redis_cmd_cat_long(cmd, opts.ef);
    }

    if (opts.attributes != NULL) {
        redis_cmd_cat_literal(cmd, "SETATTR");
        redis_cmd_cat_zstr(cmd, opts.attributes);
    }

    if (opts.numlinks > -1) {
        redis_cmd_cat_literal(cmd, "M");
        redis_cmd_cat_long(cmd, opts.numlinks);
    }

    free_vadd_options(&opts);

    return cmd;
}

typedef enum redisVSimMemberType {
    REDIS_VSIM_NONE,
    REDIS_VSIM_FP32,
    REDIS_VSIM_VALUES,
    REDIS_VSIM_ELE,
} redisVSimMemberType;

typedef struct redisVSimOptions {
    redisVSimMemberType type;
    zend_long count;
    zend_long ef;
    zend_string *filter;
    zend_long filter_ef;
    zend_bool truth;
    zend_bool nothread;
    zend_bool withscores;
} redisVSimOptions;

static void free_vsim_options(redisVSimOptions *opts) {
    if (opts->filter != NULL)
        zend_string_release(opts->filter);
}

static void parse_vsim_options(redisVSimOptions *dst, HashTable *ht) {
    zend_string *key;
    zval *zv;

    *dst = (redisVSimOptions){
        .filter_ef = -1,
        .ef = -1,
    };

    if (ht == NULL)
        return;

    ZEND_HASH_FOREACH_STR_KEY_VAL(ht, key, zv) {
        if (key != NULL) {
            if (zend_string_equals_literal_ci(key, "EF")) {
                if (validate_vadd_integer(key, zv, 1))
                    dst->ef = Z_LVAL_P(zv);
            } else if (zend_string_equals_literal_ci(key, "FILTER")) {
                dst->filter = zval_get_string(zv);
            } else if (zend_string_equals_literal_ci(key, "FILTER-EF")) {
                if (validate_vadd_integer(key, zv, 1))
                    dst->filter_ef = Z_LVAL_P(zv);
            } else if (zend_string_equals_literal_ci(key, "COUNT")) {
                if (validate_vadd_integer(key, zv, 1))
                    dst->count = Z_LVAL_P(zv);
            } else if (zend_string_equals_literal_ci(key, "WITHSCORES")) {
                dst->withscores = zend_is_true(zv);
            }
        } else if (Z_TYPE_P(zv) == IS_STRING) {
            if (zend_string_equals_literal_ci(Z_STR_P(zv), "FP32")) {
                dst->type = REDIS_VSIM_FP32;
            } else if (zend_string_equals_literal_ci(Z_STR_P(zv), "VALUES")) {
                dst->type = REDIS_VSIM_VALUES;
            } else if (zend_string_equals_literal_ci(Z_STR_P(zv), "ELE")) {
                dst->type = REDIS_VSIM_ELE;
            } else if (zend_string_equals_literal_ci(Z_STR_P(zv), "NOTHREAD")) {
                dst->nothread = 1;
            } else if (zend_string_equals_literal_ci(Z_STR_P(zv), "TRUTH")) {
                dst->truth = 1;
            } else if (zend_string_equals_literal_ci(Z_STR_P(zv), "WITHSCORES")) {
                dst->withscores = 1;
            }
        }
    } ZEND_HASH_FOREACH_END();
}

static redisVSimMemberType detect_vsim_type(zval *zv) {
    zval *ele;

    if (Z_TYPE_P(zv) != IS_ARRAY)
        return REDIS_VSIM_ELE;

    ZEND_HASH_FOREACH_VAL(Z_ARRVAL_P(zv), ele) {
        if (Z_TYPE_P(ele) != IS_LONG && Z_TYPE_P(ele) != IS_DOUBLE)
            return REDIS_VSIM_ELE;
    } ZEND_HASH_FOREACH_END();

    return REDIS_VSIM_FP32;
}

RedisCmd *
redis_vsim_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock) {
    HashTable *options = NULL;
    redisVSimOptions opts;
    zend_string *key;
    RedisCmd *cmd;
    zval *member;

    ZEND_PARSE_PARAMETERS_START(2, 3) {
        Z_PARAM_STR(key)
        Z_PARAM_ZVAL(member)
        Z_PARAM_OPTIONAL
        Z_PARAM_ARRAY_HT_OR_NULL(options)
    } ZEND_PARSE_PARAMETERS_END_EX(return NULL);

    parse_vsim_options(&opts, options);

    if (opts.type == REDIS_VSIM_NONE)
        opts.type = detect_vsim_type(member);

    if (opts.type == REDIS_VSIM_NONE) {
        zend_error_noreturn(E_ERROR,
            "Internal error. type shouldn't be REDIS_VSIM_NONE!");
        return NULL;
    }

    /* Sanity check on the member type */
    if (opts.type != REDIS_VSIM_ELE && Z_TYPE_P(member) != IS_ARRAY) {
        php_error_docref(NULL, E_WARNING,
            "member must be an array when not querying in ELE mode");
        free_vsim_options(&opts);
        return NULL;
    }

    cmd = redis_cmd_create_literal(redis_sock, "VSIM");

    redis_cmd_cat_key_zstr(cmd, key);

    if (opts.type == REDIS_VSIM_FP32) {
        redis_cmd_cat_literal(cmd, "FP32");
        redis_cmd_cat_fp32(cmd, Z_ARRVAL_P(member));
    } else if (opts.type == REDIS_VSIM_VALUES) {
        redis_cmd_cat_literal(cmd, "VALUES");
        redis_cmd_cat_long(cmd, zend_hash_num_elements(Z_ARRVAL_P(member)));
        redis_cmd_cat_fp32_values(cmd, Z_ARRVAL_P(member));
    } else {
        redis_cmd_cat_literal(cmd, "ELE");
        redis_cmd_cat_zval(cmd, member);
    }

    if (opts.withscores) {
        redis_cmd_cat_literal(cmd, "WITHSCORES");
        redis_cmd_set_ctx(cmd, PHPREDIS_CTX_PTR);
    }

    if (opts.count > 0) {
        redis_cmd_cat_literal(cmd, "COUNT");
        redis_cmd_cat_long(cmd, opts.count);
    }

    if (opts.ef > 0) {
        redis_cmd_cat_literal(cmd, "EF");
        redis_cmd_cat_long(cmd, opts.ef);
    }

    if (opts.filter != NULL) {
        redis_cmd_cat_literal(cmd, "FILTER");
        redis_cmd_cat_zstr(cmd, opts.filter);
    }

    if (opts.filter_ef > 0) {
        redis_cmd_cat_literal(cmd, "FILTER-EF");
        redis_cmd_cat_long(cmd, opts.filter_ef);
    }

    redis_cmd_cat_literal_if(cmd, opts.truth, "TRUTH");
    redis_cmd_cat_literal_if(cmd, opts.nothread, "NOTHREAD");

    free_vsim_options(&opts);

    return cmd;
}

RedisCmd *
redis_vemb_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock) {
    zend_bool raw = 0;
    zend_string *key;
    RedisCmd *cmd;
    zval *member;

    ZEND_PARSE_PARAMETERS_START(2, 3) {
        Z_PARAM_STR(key)
        Z_PARAM_ZVAL(member);
        Z_PARAM_OPTIONAL
        Z_PARAM_BOOL(raw)
    } ZEND_PARSE_PARAMETERS_END_EX(return NULL);

    cmd = redis_cmd_create_literal(redis_sock, "VEMB");
    redis_cmd_cat_key_zstr(cmd, key);
    redis_cmd_cat_zval(cmd, member);

    redis_cmd_cat_literal_if(cmd, raw, "RAW");

    return cmd;
}

RedisCmd *
redis_vlinks_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock) {
    zend_bool withscores = 0;
    zend_string *key;
    RedisCmd *cmd;
    zval *member;

    ZEND_PARSE_PARAMETERS_START(2, 3) {
        Z_PARAM_STR(key)
        Z_PARAM_ZVAL(member)
        Z_PARAM_OPTIONAL
        Z_PARAM_BOOL(withscores)
    } ZEND_PARSE_PARAMETERS_END_EX(return NULL);

    cmd = redis_cmd_create_literal(redis_sock, "VLINKS");

    redis_cmd_cat_key_zstr(cmd, key);
    redis_cmd_cat_zval(cmd, member);

    if (withscores) {
        redis_cmd_cat_literal(cmd, "WITHSCORES");
        redis_cmd_set_ctx(cmd, PHPREDIS_CTX_PTR);
    }

    return cmd;
}

RedisCmd *redis_vgetattr_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock) {
    zend_bool raw = 0;
    zend_string *key;
    RedisCmd *cmd;
    zval *member;

    ZEND_PARSE_PARAMETERS_START(2, 3) {
        Z_PARAM_STR(key)
        Z_PARAM_ZVAL(member)
        Z_PARAM_OPTIONAL
        Z_PARAM_BOOL(raw)
    } ZEND_PARSE_PARAMETERS_END_EX(return NULL);

    cmd = redis_cmd_create_literal(redis_sock, "VGETATTR");
    redis_cmd_cat_key_zstr(cmd, key);
    redis_cmd_cat_zval(cmd, member);

    redis_cmd_set_ctx(cmd, raw ? NULL : PHPREDIS_CTX_PTR);

    return cmd;
}

RedisCmd *redis_vsetattr_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock) {
    zend_string *key, *attr;
    zval *member, *zattr;
    RedisCmd *cmd;

    ZEND_PARSE_PARAMETERS_START(3, 3) {
        Z_PARAM_STR(key)
        Z_PARAM_ZVAL(member)
        Z_PARAM_ZVAL(zattr)
    } ZEND_PARSE_PARAMETERS_END_EX(return NULL);

    attr = zval_to_vattr(zattr);
    if (zattr == NULL)
        return NULL;

    cmd = redis_cmd_create_literal(redis_sock, "VSETATTR");
    redis_cmd_cat_key_zstr(cmd, key);
    redis_cmd_cat_zval(cmd, member);
    redis_cmd_cat_zstr(cmd, attr);

    zend_string_release(attr);

    return cmd;
}

// GCRA key max-burst requests-per-period period [NUM_REQUESTS count]
RedisCmd *redis_gcra_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock)  {
    zend_long max_burst, req_per_period, period, tokens = 0;
    zend_string *key;
    RedisCmd *cmd;

    ZEND_PARSE_PARAMETERS_START(4, 5)
        Z_PARAM_STR(key)
        Z_PARAM_LONG(max_burst)
        Z_PARAM_LONG(req_per_period)
        Z_PARAM_LONG(period)
        Z_PARAM_OPTIONAL
        Z_PARAM_LONG(tokens)
    ZEND_PARSE_PARAMETERS_END_EX(return NULL);

    cmd = redis_cmd_create_literal(redis_sock, "GCRA");
    redis_cmd_cat_key_zstr(cmd, key);
    redis_cmd_cat_long(cmd, max_burst);
    redis_cmd_cat_long(cmd, req_per_period);
    redis_cmd_cat_long(cmd, period);

    if (tokens > 0) {
        redis_cmd_cat_literal(cmd, "TOKENS");
        redis_cmd_cat_long(cmd, tokens);
    }

    return cmd;
}

/*
 * Redis commands that don't deal with the server at all.  The RedisSock*
 * pointer is the only thing retrieved differently, so we just take that
 * in addition to the standard INTERNAL_FUNCTION_PARAMETERS for arg parsing,
 * return value handling, and thread safety. */

void redis_getoption_handler(INTERNAL_FUNCTION_PARAMETERS,
                             RedisSock *redis_sock, redisCluster *c)
{
    zend_long option;

    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_LONG(option)
    ZEND_PARSE_PARAMETERS_END_EX(RETURN_FALSE);

    // Return the requested option
    switch(option) {
        case REDIS_OPT_SERIALIZER:
            RETURN_LONG(redis_sock->serializer);
        case REDIS_OPT_COMPRESSION:
            RETURN_LONG(redis_sock->compression);
        case REDIS_OPT_COMPRESSION_LEVEL:
            RETURN_LONG(redis_sock->compression_level);
        case REDIS_OPT_PACK_IGNORE_NUMBERS:
            RETURN_BOOL(redis_sock->pack_ignore_numbers);
        case REDIS_OPT_PREFIX:
            if (redis_sock->prefix) {
                RETURN_STRINGL(ZSTR_VAL(redis_sock->prefix), ZSTR_LEN(redis_sock->prefix));
            }
            RETURN_NULL();
        case REDIS_OPT_READ_TIMEOUT:
            RETURN_DOUBLE(redis_sock->read_timeout);
        case REDIS_OPT_TCP_KEEPALIVE:
            RETURN_LONG(redis_sock->tcp_keepalive);
        case REDIS_OPT_SCAN:
            RETURN_LONG(redis_sock->scan);
        case REDIS_OPT_REPLY_LITERAL:
            RETURN_LONG(redis_sock->reply_literal);
        case REDIS_OPT_NULL_MBULK_AS_NULL:
            RETURN_LONG(redis_sock->null_mbulk_as_null);
        case REDIS_OPT_FAILOVER:
            RETURN_LONG(c->failover);
        case REDIS_OPT_MAX_RETRIES:
            RETURN_LONG(redis_sock->max_retries);
        case REDIS_OPT_BACKOFF_ALGORITHM:
            RETURN_LONG(redis_sock->backoff.algorithm);
        case REDIS_OPT_BACKOFF_BASE:
            RETURN_LONG(redis_sock->backoff.base / 1000);
        case REDIS_OPT_BACKOFF_CAP:
            RETURN_LONG(redis_sock->backoff.cap / 1000);
        default:
            RETURN_FALSE;
    }
}

void redis_setoption_handler(INTERNAL_FUNCTION_PARAMETERS,
                             RedisSock *redis_sock, redisCluster *c)
{
    zend_long val_long, option;
    zval *val;
    zend_string *val_str;
    struct timeval read_tv;
    int tcp_keepalive = 0;
    php_netstream_data_t *sock;

    ZEND_PARSE_PARAMETERS_START(2, 2)
        Z_PARAM_LONG(option)
        Z_PARAM_ZVAL(val)
    ZEND_PARSE_PARAMETERS_END_EX(RETURN_FALSE);

    switch(option) {
        case REDIS_OPT_SERIALIZER:
            val_long = zval_get_long(val);
            if (val_long == REDIS_SERIALIZER_NONE
                || val_long == REDIS_SERIALIZER_PHP
                || val_long == REDIS_SERIALIZER_JSON
#ifdef HAVE_REDIS_IGBINARY
                || val_long == REDIS_SERIALIZER_IGBINARY
#endif
#ifdef HAVE_REDIS_MSGPACK
                || val_long == REDIS_SERIALIZER_MSGPACK
#endif
            ) {
                redis_sock->serializer = val_long;
                RETURN_TRUE;
            }
            break;
        case REDIS_OPT_REPLY_LITERAL:
            val_long = zval_get_long(val);
            redis_sock->reply_literal = val_long != 0;
            RETURN_TRUE;
        case REDIS_OPT_NULL_MBULK_AS_NULL:
            val_long = zval_get_long(val);
            redis_sock->null_mbulk_as_null = val_long != 0;
            RETURN_TRUE;
        case REDIS_OPT_COMPRESSION:
            val_long = zval_get_long(val);
            if (val_long == REDIS_COMPRESSION_NONE
#ifdef HAVE_REDIS_LZF
                || val_long == REDIS_COMPRESSION_LZF
#endif
#ifdef HAVE_REDIS_ZSTD
                || val_long == REDIS_COMPRESSION_ZSTD
#endif
#ifdef HAVE_REDIS_LZ4
                || val_long == REDIS_COMPRESSION_LZ4
#endif
            ) {
                redis_sock->compression = val_long;
                RETURN_TRUE;
            }
            break;
        case REDIS_OPT_PACK_IGNORE_NUMBERS:
            redis_sock->pack_ignore_numbers = zend_is_true(val);
            RETURN_TRUE;
        case REDIS_OPT_COMPRESSION_LEVEL:
            val_long = zval_get_long(val);
            redis_sock->compression_level = val_long;
            RETURN_TRUE;
        case REDIS_OPT_PREFIX:
            if (redis_sock->prefix) {
                zend_string_release(redis_sock->prefix);
                redis_sock->prefix = NULL;
            }
            val_str = zval_get_string(val);
            if (ZSTR_LEN(val_str) > 0) {
                redis_sock->prefix = val_str;
            } else {
                zend_string_release(val_str);
            }
            RETURN_TRUE;
        case REDIS_OPT_READ_TIMEOUT:
            redis_sock->read_timeout = zval_get_double(val);
            if (redis_sock->stream) {
                read_tv.tv_sec  = (time_t)redis_sock->read_timeout;
                read_tv.tv_usec = (int)((redis_sock->read_timeout -
                                         read_tv.tv_sec) * 1000000);
                php_stream_set_option(redis_sock->stream,
                                      PHP_STREAM_OPTION_READ_TIMEOUT, 0,
                                      &read_tv);
            }
            RETURN_TRUE;
        case REDIS_OPT_TCP_KEEPALIVE:

            /* Don't set TCP_KEEPALIVE if we're using a unix socket. */
            if (ZSTR_VAL(redis_sock->host)[0] == '/' && redis_sock->port < 1) {
                RETURN_FALSE;
            }
            tcp_keepalive = zval_get_long(val) > 0 ? 1 : 0;
            if (redis_sock->tcp_keepalive == tcp_keepalive) {
                RETURN_TRUE;
            }
            if (redis_sock->stream) {
                /* set TCP_KEEPALIVE */
                sock = (php_netstream_data_t*)redis_sock->stream->abstract;
                if (setsockopt(sock->socket, SOL_SOCKET, SO_KEEPALIVE, (char*)&tcp_keepalive,
                            sizeof(tcp_keepalive)) == -1) {
                    RETURN_FALSE;
                }
                redis_sock->tcp_keepalive = tcp_keepalive;
            }
            RETURN_TRUE;
        case REDIS_OPT_SCAN:
            val_long = zval_get_long(val);
            if (val_long == REDIS_SCAN_NORETRY) {
                redis_sock->scan &= ~REDIS_SCAN_RETRY;
            } else if (val_long == REDIS_SCAN_NOPREFIX) {
                redis_sock->scan &= ~REDIS_SCAN_PREFIX;
            } else if (val_long == REDIS_SCAN_RETRY || val_long == REDIS_SCAN_PREFIX) {
                redis_sock->scan |= val_long;
            } else {
                break;
            }
            RETURN_TRUE;
        case REDIS_OPT_FAILOVER:
            if (c == NULL) RETURN_FALSE;
            val_long = zval_get_long(val);
            if (val_long == REDIS_FAILOVER_NONE ||
                val_long == REDIS_FAILOVER_ERROR ||
                val_long == REDIS_FAILOVER_DISTRIBUTE ||
                val_long == REDIS_FAILOVER_DISTRIBUTE_SLAVES)
            {
                c->failover = val_long;
                RETURN_TRUE;
            }
            break;
        case REDIS_OPT_MAX_RETRIES:
            val_long = zval_get_long(val);
            if(val_long >= 0) {
                redis_sock->max_retries = val_long;
                RETURN_TRUE;
            }
            break;
        case REDIS_OPT_BACKOFF_ALGORITHM:
            val_long = zval_get_long(val);
            if(val_long >= 0 &&
               val_long < REDIS_BACKOFF_ALGORITHMS) {
                redis_sock->backoff.algorithm = val_long;
                RETURN_TRUE;
            }
            break;
        case REDIS_OPT_BACKOFF_BASE:
            val_long = zval_get_long(val);
            if(val_long >= 0) {
                redis_sock->backoff.base = val_long * 1000;
                RETURN_TRUE;
            }
            break;
        case REDIS_OPT_BACKOFF_CAP:
            val_long = zval_get_long(val);
            if(val_long >= 0) {
                redis_sock->backoff.cap = val_long * 1000;
                RETURN_TRUE;
            }
            break;
        default:
            php_error_docref(NULL, E_WARNING, "Unknown option '" ZEND_LONG_FMT "'", option);
            break;
    }
    RETURN_FALSE;
}

void redis_prefix_handler(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock) {
    zend_string *key, *prefixed;

    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_STR(key)
    ZEND_PARSE_PARAMETERS_END_EX(RETURN_FALSE);

    prefixed = redis_key_prefix_zstr(redis_sock, key);

    RETURN_STR(prefixed);
}

void redis_serialize_handler(INTERNAL_FUNCTION_PARAMETERS,
                             RedisSock *redis_sock)
{
    zval *z_val;
    char *val;
    size_t val_len;

    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_ZVAL(z_val)
    ZEND_PARSE_PARAMETERS_END_EX(RETURN_FALSE);

    int val_free = redis_serialize(redis_sock, z_val, &val, &val_len);

    RETVAL_STRINGL(val, val_len);
    if (val_free) efree(val);
}

void redis_unserialize_handler(INTERNAL_FUNCTION_PARAMETERS,
                               RedisSock *redis_sock, zend_class_entry *ex)
{
    char *value;
    size_t value_len;

    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_STRING(value, value_len)
    ZEND_PARSE_PARAMETERS_END_EX(RETURN_FALSE);

    // We only need to attempt unserialization if we have a serializer running
    if (redis_sock->serializer == REDIS_SERIALIZER_NONE) {
        // Just return the value that was passed to us
        RETURN_STRINGL(value, value_len);
    }

    zval z_ret;
    if (!redis_unserialize(redis_sock, value, value_len, &z_ret)) {
        // Badly formed input, throw an exception
        zend_throw_exception(ex, "Invalid serialized data, or unserialization error", 0);
        RETURN_FALSE;
    }
    RETURN_ZVAL(&z_ret, 0, 0);
}

void redis_compress_handler(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock) {
    zend_string *zstr;
    size_t len;
    char *buf;
    int cmp_free;

    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_STR(zstr)
    ZEND_PARSE_PARAMETERS_END_EX(RETURN_FALSE);

    cmp_free = redis_compress(redis_sock, &buf, &len, ZSTR_VAL(zstr), ZSTR_LEN(zstr));
    RETVAL_STRINGL(buf, len);
    if (cmp_free) efree(buf);
}

void redis_uncompress_handler(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock,
                              zend_class_entry *ex)
{
    zend_string *zstr;
    size_t len;
    char *buf;

    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_STR(zstr)
    ZEND_PARSE_PARAMETERS_END_EX(RETURN_FALSE);

    if (ZSTR_LEN(zstr) == 0 || redis_sock->compression == REDIS_COMPRESSION_NONE) {
        RETURN_STR_COPY(zstr);
    }

    if (!redis_uncompress(redis_sock, &buf, &len, ZSTR_VAL(zstr), ZSTR_LEN(zstr))) {
        zend_throw_exception(ex, "Invalid compressed data or uncompression error", 0);
        RETURN_FALSE;
    }

    RETVAL_STRINGL(buf, len);
    efree(buf);
}

void redis_pack_handler(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock) {
    int valfree;
    size_t len;
    char *val;
    zval *zv;

    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_ZVAL(zv)
    ZEND_PARSE_PARAMETERS_END_EX(RETURN_FALSE);

    valfree = redis_pack(redis_sock, zv, &val, &len);
    RETVAL_STRINGL(val, len);
    if (valfree) efree(val);
}

#if PHP_VERSION_ID >= 80100
static zend_string *redis_xxh3_digest(RedisSock *redis_sock, zval *zv) {
    zend_string *algo, *hex;
    const php_hash_ops *ops;
    unsigned char *digest;
    int valfree;
    size_t len;
    char *val;
    void *ctx;

    algo = zend_string_init(ZEND_STRL("XXH3"), 0);
    ops  = php_hash_fetch_ops(algo);
    if (ops == NULL) {
        zend_string_release(algo);
        return NULL;
    }

    valfree = redis_pack(redis_sock, zv, &val, &len);

    ctx = emalloc(ops->context_size);
    ops->hash_init(ctx, NULL);
    ops->hash_update(ctx, (const unsigned char *)val, len);

    digest = emalloc(ops->digest_size);
    ops->hash_final(digest, ctx);

    hex = zend_string_safe_alloc(ops->digest_size, 2, 0, 0);
    zend_bin2hex(ZSTR_VAL(hex), digest, ops->digest_size);
    ZSTR_VAL(hex)[ZSTR_LEN(hex)] = '\0';

    efree(ctx);
    efree(digest);
    if (valfree) efree(val);
    zend_string_release(algo);

    return hex;
}
#endif

void redis_digest_handler(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock,
                          zend_class_entry *exception_ce)
{
    zend_string *digest;
    zval *zv;

    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_ZVAL(zv)
    ZEND_PARSE_PARAMETERS_END_EX(return );

    #if PHP_VERSION_ID >= 80100
        digest = redis_xxh3_digest(redis_sock, zv);

        if (digest == NULL) {
            zend_throw_exception(exception_ce, "XXH3 hashing not available?", 0);
            RETURN_THROWS();
        }

        RETURN_STR(digest);
    #else
        zend_throw_exception(exception_ce, "Method requires PHP >= 8.1", 0);
        RETURN_FALSE;
    #endif
}

void redis_unpack_handler(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock) {
    zend_string *str;

    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_STR(str)
    ZEND_PARSE_PARAMETERS_END_EX(RETURN_FALSE);

    redis_unpack(redis_sock, ZSTR_VAL(str), ZSTR_LEN(str), return_value);
}
/* vim: set tabstop=4 softtabstop=4 expandtab shiftwidth=4: */
