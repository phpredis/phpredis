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

/* Local passthrough macro for command construction.  Given that these methods
 * are generic (so they work whether the caller is Redis or RedisCluster) we
 * will always have redis_sock, slot*, and */
#define REDIS_CMD_SPPRINTF(ret, kw, fmt, ...) \
    redis_spprintf(redis_sock, slot, ret, kw, fmt, ##__VA_ARGS__)

/* Generic commands based on method signature and what kind of things we're
 * processing.  Lots of Redis commands take something like key, value, or
 * key, value long.  Each unique signature like this is written only once */

/* A command that takes no arguments */
int redis_empty_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock,
                    char *kw, char **cmd, int *cmd_len, short *slot,
                    void **ctx)
{
    *cmd_len = REDIS_CMD_SPPRINTF(cmd, kw, "");
    return SUCCESS;
}

/* Helper to construct a raw command.  Given that the cluster and non cluster
 * versions are different (RedisCluster needs an additional argument to direct
 * the command) we take the start of our array and count */
int redis_build_raw_cmd(zval *z_args, int argc, char **cmd, int *cmd_len)
{
    smart_string cmdstr = {0};
    int i;

    /* Make sure our first argument is a string */
    if (Z_TYPE(z_args[0]) != IS_STRING) {
        php_error_docref(NULL, E_WARNING,
            "When sending a 'raw' command, the first argument must be a string!");
        return FAILURE;
    }

    /* Initialize our command string */
    redis_cmd_init_sstr(&cmdstr, argc-1, Z_STRVAL(z_args[0]), Z_STRLEN(z_args[0]));

    for (i = 1; i < argc; i++) {
       switch (Z_TYPE(z_args[i])) {
            case IS_STRING:
                redis_cmd_append_sstr(&cmdstr, Z_STRVAL(z_args[i]),
                    Z_STRLEN(z_args[i]));
                break;
            case IS_LONG:
                redis_cmd_append_sstr_long(&cmdstr,Z_LVAL(z_args[i]));
                break;
            case IS_DOUBLE:
                redis_cmd_append_sstr_dbl(&cmdstr,Z_DVAL(z_args[i]));
                break;
            default:
                php_error_docref(NULL, E_WARNING,
                    "Raw command arguments must be scalar values!");
                efree(cmdstr.c);
                return FAILURE;
        }
    }

    /* Push command and length to caller */
    *cmd = cmdstr.c;
    *cmd_len = cmdstr.len;

    return SUCCESS;
}

smart_string *
redis_build_script_cmd(smart_string *cmd, int argc, zval *z_args)
{
    int i;
    zend_string *zstr;

    if (Z_TYPE(z_args[0]) != IS_STRING) {
        return NULL;
    }
    // Branch based on the directive
    if (!strcasecmp(Z_STRVAL(z_args[0]), "kill")) {
        // Simple SCRIPT_KILL command
        REDIS_CMD_INIT_SSTR_STATIC(cmd, argc, "SCRIPT");
        redis_cmd_append_sstr(cmd, ZEND_STRL("KILL"));
    } else if (!strcasecmp(Z_STRVAL(z_args[0]), "flush")) {
        // Simple SCRIPT FLUSH [ASYNC | SYNC]
        if (argc > 1 && (
            Z_TYPE(z_args[1]) != IS_STRING ||
            strcasecmp(Z_STRVAL(z_args[1]), "sync") ||
            strcasecmp(Z_STRVAL(z_args[1]), "async")
        )) {
            return NULL;
        }
        REDIS_CMD_INIT_SSTR_STATIC(cmd, argc, "SCRIPT");
        redis_cmd_append_sstr(cmd, ZEND_STRL("FLUSH"));
        if (argc > 1) {
            redis_cmd_append_sstr(cmd, Z_STRVAL(z_args[1]), Z_STRLEN(z_args[1]));
        }
    } else if (!strcasecmp(Z_STRVAL(z_args[0]), "load")) {
        // Make sure we have a second argument, and it's not empty.  If it is
        // empty, we can just return an empty array (which is what Redis does)
        if (argc < 2 || Z_TYPE(z_args[1]) != IS_STRING || Z_STRLEN(z_args[1]) < 1) {
            return NULL;
        }
        // Format our SCRIPT LOAD command
        REDIS_CMD_INIT_SSTR_STATIC(cmd, argc, "SCRIPT");
        redis_cmd_append_sstr(cmd, ZEND_STRL("LOAD"));
        redis_cmd_append_sstr(cmd, Z_STRVAL(z_args[1]), Z_STRLEN(z_args[1]));
    } else if (!strcasecmp(Z_STRVAL(z_args[0]), "exists")) {
        // Make sure we have a second argument
        if (argc < 2) {
            return NULL;
        }
        /* Construct our SCRIPT EXISTS command */
        REDIS_CMD_INIT_SSTR_STATIC(cmd, argc, "SCRIPT");
        redis_cmd_append_sstr(cmd, ZEND_STRL("EXISTS"));

        for (i = 1; i < argc; ++i) {
            zstr = zval_get_string(&z_args[i]);
            redis_cmd_append_sstr(cmd, ZSTR_VAL(zstr), ZSTR_LEN(zstr));
            zend_string_release(zstr);
        }
    } else {
        /* Unknown directive */
        return NULL;
    }
    return cmd;
}

/* Command that takes one optional string */
int redis_opt_str_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock, char *kw,
                      char **cmd, int *cmd_len, short *slot, void **ctx)
{
    char *arg = NULL;
    size_t arglen;

    if (zend_parse_parameters(ZEND_NUM_ARGS(), "|s!", &arg, &arglen) == FAILURE) {
        return FAILURE;
    }

    if (arg != NULL) {
        *cmd_len = REDIS_CMD_SPPRINTF(cmd, kw, "s", arg, arglen);
    } else {
        *cmd_len = REDIS_CMD_SPPRINTF(cmd, kw, "");
    }

    return SUCCESS;
}

/* Generic command where we just take a string and do nothing to it*/
int redis_str_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock, char *kw,
                  char **cmd, int *cmd_len, short *slot, void **ctx)
{
    char *arg;
    size_t arg_len;

    // Parse args
    if (zend_parse_parameters(ZEND_NUM_ARGS(), "s", &arg, &arg_len)
                             ==FAILURE)
    {
        return FAILURE;
    }

    // Build the command without molesting the string
    *cmd_len = REDIS_CMD_SPPRINTF(cmd, kw, "s", arg, arg_len);

    return SUCCESS;
}

/* Key, long, zval (serialized) */
int redis_key_long_val_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock,
                           char *kw, char **cmd, int *cmd_len, short *slot,
                           void **ctx)
{
    char *key = NULL;
    size_t key_len;
    zend_long expire;
    zval *z_val;

    if (zend_parse_parameters(ZEND_NUM_ARGS(), "slz", &key, &key_len,
                             &expire, &z_val) == FAILURE)
    {
        return FAILURE;
    }

    *cmd_len = REDIS_CMD_SPPRINTF(cmd, kw, "klv", key, key_len, expire, z_val);

    return SUCCESS;
}

/* Generic key, long, string (unserialized) */
int redis_key_long_str_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock,
                           char *kw, char **cmd, int *cmd_len, short *slot,
                           void **ctx)
{
    char *key, *val;
    size_t key_len, val_len;
    zend_long lval;

    if (zend_parse_parameters(ZEND_NUM_ARGS(), "sls", &key, &key_len,
                             &lval, &val, &val_len) == FAILURE)
    {
        return FAILURE;
    }

    *cmd_len = REDIS_CMD_SPPRINTF(cmd, kw, "kds", key, key_len, (int)lval, val, val_len);

    return SUCCESS;
}

/* Generic command construction when we just take a key and value */
int redis_kv_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock,
                 char *kw, char **cmd, int *cmd_len, short *slot,
                 void **ctx)
{
    char *key;
    size_t key_len;
    zval *z_val;

    if (zend_parse_parameters(ZEND_NUM_ARGS(), "sz", &key, &key_len,
                             &z_val) == FAILURE)
    {
        return FAILURE;
    }

    *cmd_len = REDIS_CMD_SPPRINTF(cmd, kw, "kv", key, key_len, z_val);

    return SUCCESS;
}

/* Generic command that takes a key and an unserialized value */
int redis_key_str_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock,
                      char *kw, char **cmd, int *cmd_len, short *slot,
                      void **ctx)
{
    char *key, *val;
    size_t key_len, val_len;

    if (zend_parse_parameters(ZEND_NUM_ARGS(), "ss", &key, &key_len,
                             &val, &val_len) == FAILURE)
    {
        return FAILURE;
    }

    // Construct command
    *cmd_len = REDIS_CMD_SPPRINTF(cmd, kw, "ks", key, key_len, val, val_len);

    return SUCCESS;
}

/* Key, string, string without serialization (ZCOUNT, ZREMRANGEBYSCORE) */
int redis_key_str_str_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock,
                          char *kw, char **cmd, int *cmd_len, short *slot,
                          void **ctx)
{
    char *k, *v1, *v2;
    size_t klen, v1len, v2len;

    if (zend_parse_parameters(ZEND_NUM_ARGS(), "sss", &k, &klen,
                             &v1, &v1len, &v2, &v2len) == FAILURE)
    {
        return FAILURE;
    }

    *cmd_len = REDIS_CMD_SPPRINTF(cmd, kw, "kss", k, klen, v1, v1len, v2, v2len);

    // Success!
    return SUCCESS;
}

/* Generic command that takes two keys */
int redis_key_key_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock,
                      char *kw, char **cmd, int *cmd_len, short *slot,
                      void **ctx)
{
    zend_string *key1 = NULL, *key2 = NULL;
    smart_string cmdstr = {0};
    short slot2;

    ZEND_PARSE_PARAMETERS_START(2, 2)
        Z_PARAM_STR(key1)
        Z_PARAM_STR(key2)
    ZEND_PARSE_PARAMETERS_END_EX(return FAILURE);

    redis_cmd_init_sstr(&cmdstr, 2, kw, strlen(kw));
    redis_cmd_append_sstr_key_zstr(&cmdstr, key1, redis_sock, slot);
    redis_cmd_append_sstr_key_zstr(&cmdstr, key2, redis_sock, slot ? &slot2 : NULL);

    if (slot && *slot != slot2) {
        php_error_docref(0, E_WARNING, "Keys don't hash to the same slot");
        smart_string_free(&cmdstr);
        return FAILURE;
    }

    *cmd = cmdstr.c;
    *cmd_len = cmdstr.len;
    return SUCCESS;
}

/* Generic command construction where we take a key and a long */
int redis_key_long_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock,
                       char *kw, char **cmd, int *cmd_len, short *slot,
                       void **ctx)
{
    zend_string *key = NULL;
    zend_long lval = 0;

    ZEND_PARSE_PARAMETERS_START(2, 2)
        Z_PARAM_STR(key)
        Z_PARAM_LONG(lval)
    ZEND_PARSE_PARAMETERS_END_EX(return FAILURE);

    *cmd_len = REDIS_CMD_SPPRINTF(cmd, kw, "kl", ZSTR_VAL(key), ZSTR_LEN(key), lval);

    return SUCCESS;
}

/* long, long */
int redis_long_long_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock,
                        char *kw, char **cmd, int *cmd_len, short *slot,
                        void **ctx)
{
    zend_long l1 = 0, l2 = 0;

    ZEND_PARSE_PARAMETERS_START(2, 2)
        Z_PARAM_LONG(l1)
        Z_PARAM_LONG(l2)
    ZEND_PARSE_PARAMETERS_END_EX(return FAILURE);

    *cmd_len = REDIS_CMD_SPPRINTF(cmd, kw, "ll", l1, l2);

    return SUCCESS;
}

/* key, long, long */
int redis_key_long_long_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock,
                            char *kw, char **cmd, int *cmd_len, short *slot,
                            void **ctx)
{
    char *key;
    size_t key_len;
    zend_long val1, val2;

    if (zend_parse_parameters(ZEND_NUM_ARGS(), "sll", &key, &key_len,
                             &val1, &val2) == FAILURE)
    {
        return FAILURE;
    }

    *cmd_len = REDIS_CMD_SPPRINTF(cmd, kw, "kll", key, key_len, val1, val2);

    return SUCCESS;
}

/* Generic command where we take a single key */
int redis_key_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock,
                  char *kw, char **cmd, int *cmd_len, short *slot,
                  void **ctx)
{
    char *key;
    size_t key_len;

    if (zend_parse_parameters(ZEND_NUM_ARGS(), "s", &key, &key_len)
                             ==FAILURE)
    {
        return FAILURE;
    }

    *cmd_len = REDIS_CMD_SPPRINTF(cmd, kw, "k", key, key_len);

    return SUCCESS;
}

int
redis_failover_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock,
                   char **cmd, int *cmd_len, short *slot, void **ctx)
{
    int argc;
    smart_string cmdstr = {0};
    zend_bool abort = 0, force = 0;
    zend_long timeout = 0, port = 0;
    zend_string *zkey, *host = NULL;
    zval *z_to = NULL, *z_ele;

    if (zend_parse_parameters(ZEND_NUM_ARGS(), "|a!bl",
                              &z_to, &abort, &timeout) == FAILURE)
    {
        return FAILURE;
    }

    if (z_to != NULL) {
        ZEND_HASH_FOREACH_STR_KEY_VAL(Z_ARRVAL_P(z_to), zkey, z_ele) {
            if (zkey != NULL) {
                ZVAL_DEREF(z_ele);
                if (zend_string_equals_literal_ci(zkey, "host")) {
                    host = zval_get_string(z_ele);
                } else if (zend_string_equals_literal_ci(zkey, "port")) {
                    port = zval_get_long(z_ele);
                } else if (zend_string_equals_literal_ci(zkey, "force")) {
                    force = zval_is_true(z_ele);
                }
            }
        } ZEND_HASH_FOREACH_END();
        if (!host || !port) {
            php_error_docref(NULL, E_WARNING, "host and port must be provided!");
            if (host) zend_string_release(host);
            return FAILURE;
        }
    }

    argc = (host && port ? 3 + force : 0) + abort + (timeout > 0 ? 2 : 0);
    REDIS_CMD_INIT_SSTR_STATIC(&cmdstr, argc, "FAILOVER");

    if (host && port) {
        REDIS_CMD_APPEND_SSTR_STATIC(&cmdstr, "TO");
        redis_cmd_append_sstr_zstr(&cmdstr, host);
        redis_cmd_append_sstr_int(&cmdstr, port);
        if (force) {
            REDIS_CMD_APPEND_SSTR_STATIC(&cmdstr, "FORCE");
        }
        zend_string_release(host);
    }

    if (abort) {
        REDIS_CMD_APPEND_SSTR_STATIC(&cmdstr, "ABORT");
    }
    if (timeout > 0) {
        REDIS_CMD_APPEND_SSTR_STATIC(&cmdstr, "TIMEOUT");
        redis_cmd_append_sstr_long(&cmdstr, timeout);
    }

    *cmd = cmdstr.c;
    *cmd_len = cmdstr.len;

    return SUCCESS;
}

int redis_flush_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock,
               char *kw, char **cmd, int *cmd_len, short *slot, void **ctx)
{
    smart_string cmdstr = {0};
    zend_bool sync = 0;
    zend_bool is_null = 1;

    ZEND_PARSE_PARAMETERS_START(0, 1)
        Z_PARAM_OPTIONAL
        Z_PARAM_BOOL_OR_NULL(sync, is_null)
    ZEND_PARSE_PARAMETERS_END_EX(return FAILURE);

    redis_cmd_init_sstr(&cmdstr, !is_null, kw, strlen(kw));
    if (!is_null) {
        ZEND_ASSERT(sync == 0 || sync == 1);
        if (sync == 0) {
            REDIS_CMD_APPEND_SSTR_STATIC(&cmdstr, "ASYNC");
        } else {
            REDIS_CMD_APPEND_SSTR_STATIC(&cmdstr, "SYNC");
        }
    }

    *cmd = cmdstr.c;
    *cmd_len = cmdstr.len;

    return SUCCESS;
}

/* Generic command where we take a key and a double */
int redis_key_dbl_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock,
                      char *kw, char **cmd, int *cmd_len, short *slot,
                      void **ctx)
{
    char *key;
    size_t key_len;
    double val;

    if (zend_parse_parameters(ZEND_NUM_ARGS(), "sd", &key, &key_len,
                             &val) == FAILURE)
    {
        return FAILURE;
    }

    *cmd_len = REDIS_CMD_SPPRINTF(cmd, kw, "kf", key, key_len, val);

    return SUCCESS;
}

/* Generic to construct SCAN and variant commands */
int redis_fmt_scan_cmd(char **cmd, REDIS_SCAN_TYPE type, char *key, int key_len,
                       long it, char *pat, int pat_len, long count)
{
    static char *kw[] = {"SCAN","SSCAN","HSCAN","ZSCAN"};
    int argc;
    smart_string cmdstr = {0};

    // Figure out our argument count
    argc = 1 + (type!=TYPE_SCAN) + (pat_len>0?2:0) + (count>0?2:0);

    redis_cmd_init_sstr(&cmdstr, argc, kw[type], strlen(kw[type]));

    // Append our key if it's not a regular SCAN command
    if (type != TYPE_SCAN) {
        redis_cmd_append_sstr(&cmdstr, key, key_len);
    }

    // Append cursor
    redis_cmd_append_sstr_long(&cmdstr, it);

    // Append count if we've got one
    if (count) {
        redis_cmd_append_sstr(&cmdstr, ZEND_STRL("COUNT"));
        redis_cmd_append_sstr_long(&cmdstr, count);
    }

    // Append pattern if we've got one
    if (pat_len) {
        redis_cmd_append_sstr(&cmdstr, ZEND_STRL("MATCH"));
        redis_cmd_append_sstr(&cmdstr,pat,pat_len);
    }

    // Push command to the caller, return length
    *cmd = cmdstr.c;
    return cmdstr.len;
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
                dst->withscores = zval_is_true(zv);
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
static int redis_get_zcmd_flags(const char *kw) {
    size_t len = strlen(kw);

    if (REDIS_STRICMP_STATIC(kw, len, "ZRANGESTORE")) {
        return REDIS_ZCMD_HAS_DST_KEY |
               REDIS_ZCMD_HAS_WITHSCORES |
               REDIS_ZCMD_HAS_BY_LEX_SCORE |
               REDIS_ZCMD_HAS_REV |
               REDIS_ZCMD_HAS_LIMIT;
    } else if (REDIS_STRICMP_STATIC(kw, len, "ZRANGE")) {
        return REDIS_ZCMD_HAS_WITHSCORES |
               REDIS_ZCMD_HAS_BY_LEX_SCORE |
               REDIS_ZCMD_HAS_REV |
               REDIS_ZCMD_HAS_LIMIT;
    } else if (REDIS_STRICMP_STATIC(kw, len, "ZREVRANGE")) {
        return REDIS_ZCMD_HAS_WITHSCORES |
               REDIS_ZCMD_INT_RANGE;
    } else if (REDIS_STRICMP_STATIC(kw, len, "ZRANGEBYSCORE") ||
               REDIS_STRICMP_STATIC(kw, len, "ZREVRANGEBYSCORE"))
    {
        return REDIS_ZCMD_HAS_LIMIT |
               REDIS_ZCMD_HAS_WITHSCORES;
    } else if (REDIS_STRICMP_STATIC(kw, len, "ZRANGEBYLEX") ||
               REDIS_STRICMP_STATIC(kw, len, "ZREVRANGEBYLEX"))
    {
        return REDIS_ZCMD_HAS_LIMIT;
    } else if (REDIS_STRICMP_STATIC(kw, len, "ZDIFF")) {
        return REDIS_ZCMD_HAS_WITHSCORES;
    } else if (REDIS_STRICMP_STATIC(kw, len, "ZINTER") ||
               REDIS_STRICMP_STATIC(kw, len, "ZUNION"))
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

static int validate_zlex_arg_zval(zval *z) {
    return Z_TYPE_P(z) == IS_STRING && validate_zlex_arg(Z_STRVAL_P(z), Z_STRLEN_P(z));
}

int redis_zrange_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock,
                     char *kw, char **cmd, int *cmd_len, short *slot,
                     void **ctx)
{
    zval *zoptions = NULL, *zstart = NULL, *zend = NULL;
    zend_string *dst = NULL, *src = NULL;
    zend_long start = 0, end = 0;
    smart_string cmdstr = {0};
    redisZcmdOptions opt;
    int min_argc, flags;
    short slot2;

    flags = redis_get_zcmd_flags(kw);

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
    ZEND_PARSE_PARAMETERS_END_EX(return FAILURE);

    redis_get_zcmd_options(&opt, zoptions, flags);

    if (opt.bylex) {
        ZEND_ASSERT(!(flags & REDIS_ZCMD_INT_RANGE));
        if (!validate_zlex_arg_zval(zstart) || !validate_zlex_arg_zval(zend)) {
            php_error_docref(NULL, E_WARNING, "Legographical args must start with '[' or '(' or be '+' or '-'");
            return FAILURE;
        }
    }

    redis_cmd_init_sstr(&cmdstr, min_argc + !!opt.bylex + !!opt.byscore +
                                 !!opt.rev + !!opt.withscores +
                                 (opt.limit.enabled ? 3 : 0), kw, strlen(kw));

    if (flags & REDIS_ZCMD_HAS_DST_KEY)
        redis_cmd_append_sstr_key_zstr(&cmdstr, dst, redis_sock, slot);
    redis_cmd_append_sstr_key_zstr(&cmdstr, src, redis_sock, &slot2);

    /* Protect the user from crossslot errors */
    if ((flags & REDIS_ZCMD_HAS_DST_KEY) && slot && *slot != slot2) {
        php_error_docref(NULL, E_WARNING, "destination and source keys must map to the same slot");
        efree(cmdstr.c);
        return FAILURE;
    }

    if (flags & REDIS_ZCMD_INT_RANGE) {
        redis_cmd_append_sstr_long(&cmdstr, start);
        redis_cmd_append_sstr_long(&cmdstr, end);
    } else {
        redis_cmd_append_sstr_zval(&cmdstr, zstart, NULL);
        redis_cmd_append_sstr_zval(&cmdstr, zend, NULL);
    }

    REDIS_CMD_APPEND_SSTR_OPT_STATIC(&cmdstr, opt.byscore, "BYSCORE");
    REDIS_CMD_APPEND_SSTR_OPT_STATIC(&cmdstr, opt.bylex, "BYLEX");
    REDIS_CMD_APPEND_SSTR_OPT_STATIC(&cmdstr, opt.rev, "REV");

    if (opt.limit.enabled) {
        REDIS_CMD_APPEND_SSTR_STATIC(&cmdstr, "LIMIT");
        redis_cmd_append_sstr_long(&cmdstr, opt.limit.offset);
        redis_cmd_append_sstr_long(&cmdstr, opt.limit.count);
    }

    REDIS_CMD_APPEND_SSTR_OPT_STATIC(&cmdstr, opt.withscores, "WITHSCORES");

    if (slot) *slot = slot2;
    *ctx = opt.withscores ? PHPREDIS_CTX_PTR : NULL;
    *cmd = cmdstr.c;
    *cmd_len = cmdstr.len;

    return SUCCESS;
}

static int redis_build_config_get_cmd(smart_string *dst, zval *val) {
    zend_string *zstr;
    int ncfg;
    zval *zv;

    if (val == NULL || (Z_TYPE_P(val) != IS_STRING && Z_TYPE_P(val) != IS_ARRAY)) {
        php_error_docref(NULL, E_WARNING, "Must pass a string or array of values to CONFIG GET");
        return FAILURE;
    } else if (Z_TYPE_P(val) == IS_ARRAY && zend_hash_num_elements(Z_ARRVAL_P(val)) == 0) {
        php_error_docref(NULL, E_WARNING, "Cannot pass an empty array to CONFIG GET");
        return FAILURE;
    }

    ncfg = Z_TYPE_P(val) == IS_STRING ? 1 : zend_hash_num_elements(Z_ARRVAL_P(val));

    REDIS_CMD_INIT_SSTR_STATIC(dst, 1 + ncfg, "CONFIG");
    REDIS_CMD_APPEND_SSTR_STATIC(dst, "GET");

    if (Z_TYPE_P(val) == IS_STRING) {
        redis_cmd_append_sstr_zstr(dst, Z_STR_P(val));
    } else {
        ZEND_HASH_FOREACH_VAL(Z_ARRVAL_P(val), zv) {
            ZVAL_DEREF(zv);

            zstr = zval_get_string(zv);
            redis_cmd_append_sstr_zstr(dst, zstr);
            zend_string_release(zstr);
        } ZEND_HASH_FOREACH_END();
    }

    return SUCCESS;
}

static int redis_build_config_set_cmd(smart_string *dst, zval *key, zend_string *val) {
    zend_string *zkey, *zstr;
    zval *zv;

    /* Legacy case:  CONFIG SET <string> <string> */
    if (key != NULL && val != NULL) {
        REDIS_CMD_INIT_SSTR_STATIC(dst, 3, "CONFIG");
        REDIS_CMD_APPEND_SSTR_STATIC(dst, "SET");

        zstr = zval_get_string(key);
        redis_cmd_append_sstr_zstr(dst, zstr);
        zend_string_release(zstr);

        redis_cmd_append_sstr_zstr(dst, val);

        return SUCCESS;
    }

    /* Now we must have an array with at least one element */
    if (key == NULL || Z_TYPE_P(key) != IS_ARRAY || zend_hash_num_elements(Z_ARRVAL_P(key)) == 0) {
        php_error_docref(NULL, E_WARNING, "Must either pass two strings to CONFIG SET or a non-empty array of values");
        return FAILURE;
    }

    REDIS_CMD_INIT_SSTR_STATIC(dst, 1 + (2 * zend_hash_num_elements(Z_ARRVAL_P(key))), "CONFIG");
    REDIS_CMD_APPEND_SSTR_STATIC(dst, "SET");

    ZEND_HASH_FOREACH_STR_KEY_VAL(Z_ARRVAL_P(key), zkey, zv) {
        if (zkey == NULL)
            goto fail;

        ZVAL_DEREF(zv);

        redis_cmd_append_sstr_zstr(dst, zkey);

        zstr = zval_get_string(zv);
        redis_cmd_append_sstr_zstr(dst, zstr);
        zend_string_release(zstr);
    } ZEND_HASH_FOREACH_END();

    return SUCCESS;

fail:
    php_error_docref(NULL, E_WARNING, "Must pass an associate array of config keys and values");
    efree(dst->c);
    memset(dst, 0, sizeof(*dst));
    return FAILURE;
}

int
redis_config_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock,
                 char **cmd, int *cmd_len, short *slot, void **ctx)
{
    zend_string *op = NULL, *arg = NULL;
    smart_string cmdstr = {0};
    int res = FAILURE;
    zval *key = NULL;

    ZEND_PARSE_PARAMETERS_START(1, 3)
        Z_PARAM_STR(op)
        Z_PARAM_OPTIONAL
        Z_PARAM_ZVAL_OR_NULL(key)
        Z_PARAM_STR_OR_NULL(arg)
    ZEND_PARSE_PARAMETERS_END_EX(return FAILURE);

    if (zend_string_equals_literal_ci(op, "RESETSTAT") ||
        zend_string_equals_literal_ci(op, "REWRITE"))
    {
        REDIS_CMD_INIT_SSTR_STATIC(&cmdstr, 1, "CONFIG");
        redis_cmd_append_sstr_zstr(&cmdstr, op);
        *ctx = redis_boolean_response;
        res  = SUCCESS;
    } else if (zend_string_equals_literal_ci(op, "GET")) {
        res  = redis_build_config_get_cmd(&cmdstr, key);
        *ctx = redis_mbulk_reply_zipped_raw;
    } else if (zend_string_equals_literal_ci(op, "SET")) {
        res  = redis_build_config_set_cmd(&cmdstr, key, arg);
        *ctx = redis_boolean_response;
    } else {
        php_error_docref(NULL, E_WARNING, "Unknown operation '%s'", ZSTR_VAL(op));
        return FAILURE;
    }

    *cmd = cmdstr.c;
    *cmd_len = cmdstr.len;
    return res;
}

int
redis_function_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock,
                   char **cmd, int *cmd_len, short *slot, void **ctx)
{
    smart_string cmdstr = {0};
    zend_string *op = NULL, *arg;
    zval *argv = NULL;
    int i, argc = 0;

    ZEND_PARSE_PARAMETERS_START(1, -1)
        Z_PARAM_STR(op)
        Z_PARAM_OPTIONAL
        Z_PARAM_VARIADIC('*', argv, argc)
    ZEND_PARSE_PARAMETERS_END_EX(return FAILURE);

    for (i = 0; i < argc; ++i) {
        if (Z_TYPE(argv[i]) != IS_STRING) {
            php_error_docref(NULL, E_WARNING, "invalid argument");
            return FAILURE;
        }
    }

    if (zend_string_equals_literal_ci(op, "DELETE")) {
        if (argc < 1) {
            php_error_docref(NULL, E_WARNING, "argument required");
            return FAILURE;
        }
    } else if (zend_string_equals_literal_ci(op, "DUMP")) {
        *ctx = PHPREDIS_CTX_PTR;
    } else if (zend_string_equals_literal_ci(op, "FLUSH")) {
        if (argc > 0 &&
            !zend_string_equals_literal_ci(Z_STR(argv[0]), "SYNC") &&
            !zend_string_equals_literal_ci(Z_STR(argv[0]), "ASYNC")
        ) {
            php_error_docref(NULL, E_WARNING, "invalid argument");
            return FAILURE;
        }
    } else if (zend_string_equals_literal_ci(op, "KILL")) {
        // noop
    } else if (zend_string_equals_literal_ci(op, "LIST")) {
        if (argc > 0) {
            if (zend_string_equals_literal_ci(Z_STR(argv[0]), "LIBRARYNAME")) {
                if (argc < 2) {
                    php_error_docref(NULL, E_WARNING, "argument required");
                    return FAILURE;
                }
            } else if (!zend_string_equals_literal_ci(Z_STR(argv[0]), "WITHCODE")) {
                php_error_docref(NULL, E_WARNING, "invalid argument");
                return FAILURE;
            }
        }
        *ctx = PHPREDIS_CTX_PTR + 1;
    } else if (zend_string_equals_literal_ci(op, "LOAD")) {
        if (argc < 1 || (
            zend_string_equals_literal_ci(Z_STR(argv[0]), "REPLACE") && argc < 2
        )) {
            php_error_docref(NULL, E_WARNING, "argument required");
            return FAILURE;
        }
        *ctx = PHPREDIS_CTX_PTR;
    } else if (zend_string_equals_literal_ci(op, "RESTORE")) {
        if (argc < 1 || (
            argc > 1 &&
            !zend_string_equals_literal_ci(Z_STR(argv[1]), "FLUSH") &&
            !zend_string_equals_literal_ci(Z_STR(argv[1]), "APPEND") &&
            !zend_string_equals_literal_ci(Z_STR(argv[1]), "REPLACE")
        )) {
            php_error_docref(NULL, E_WARNING, "invalid argument");
            return FAILURE;
        }
    } else if (zend_string_equals_literal_ci(op, "STATS")) {
        *ctx = PHPREDIS_CTX_PTR + 1;
    } else {
        php_error_docref(NULL, E_WARNING, "Unknown operation '%s'", ZSTR_VAL(op));
        return FAILURE;
    }

    REDIS_CMD_INIT_SSTR_STATIC(&cmdstr, 1 + argc, "FUNCTION");
    redis_cmd_append_sstr_zstr(&cmdstr, op);

    for (i = 0; i < argc; i++) {
        arg = zval_get_string(&argv[i]);
        redis_cmd_append_sstr_zstr(&cmdstr, arg);
        zend_string_release(arg);
    }

    *cmd = cmdstr.c;
    *cmd_len = cmdstr.len;

    return SUCCESS;
}

int
redis_fcall_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock,
                char *kw, char **cmd, int *cmd_len, short *slot, void **ctx)
{
    HashTable *keys = NULL, *args = NULL;
    smart_string cmdstr = {0};
    zend_string *fn = NULL;
    zval *zv;

    ZEND_PARSE_PARAMETERS_START(1, 3)
        Z_PARAM_STR(fn)
        Z_PARAM_OPTIONAL
        Z_PARAM_ARRAY_HT(keys)
        Z_PARAM_ARRAY_HT(args)
    ZEND_PARSE_PARAMETERS_END_EX(return FAILURE);

    redis_cmd_init_sstr(&cmdstr, 2 + (keys ? zend_hash_num_elements(keys) : 0) + 
        (args ? zend_hash_num_elements(args) : 0), kw, strlen(kw));
    redis_cmd_append_sstr_zstr(&cmdstr, fn);
    redis_cmd_append_sstr_long(&cmdstr, keys ? zend_hash_num_elements(keys) : 0);

    if (keys != NULL) {
        ZEND_HASH_FOREACH_VAL(keys, zv) {
            redis_cmd_append_sstr_key_zval(&cmdstr, zv, redis_sock, slot);
        } ZEND_HASH_FOREACH_END();
    }

    if (args != NULL) {
        ZEND_HASH_FOREACH_VAL(args, zv) {
            redis_cmd_append_sstr_zval(&cmdstr, zv, redis_sock);
        } ZEND_HASH_FOREACH_END();
    }

    *cmd = cmdstr.c;
    *cmd_len = cmdstr.len;

    return SUCCESS;
}

int
redis_zrandmember_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock,
                      char **cmd, int *cmd_len, short *slot, void **ctx)
{
    char *key;
    int count = 0;
    size_t key_len;
    smart_string cmdstr = {0};
    zend_bool withscores = 0;
    zval *z_opts = NULL, *z_ele;
    zend_string *zkey;

    if (zend_parse_parameters(ZEND_NUM_ARGS(), "s|a",
                              &key, &key_len, &z_opts) == FAILURE)
    {
        return FAILURE;
    }

    if (z_opts != NULL) {
        ZEND_HASH_FOREACH_STR_KEY_VAL(Z_ARRVAL_P(z_opts), zkey, z_ele) {
            if (zkey != NULL) {
                ZVAL_DEREF(z_ele);
                if (zend_string_equals_literal_ci(zkey, "count")) {
                    count = zval_get_long(z_ele);
                } else if (zend_string_equals_literal_ci(zkey, "withscores")) {
                    withscores = zval_is_true(z_ele);
                }
            }
        } ZEND_HASH_FOREACH_END();
    }

    REDIS_CMD_INIT_SSTR_STATIC(&cmdstr, 1 + (count != 0) + withscores, "ZRANDMEMBER");
    redis_cmd_append_sstr_key(&cmdstr, key, key_len, redis_sock, slot);

    if (count != 0) {
        redis_cmd_append_sstr_long(&cmdstr, count);
        *ctx = PHPREDIS_CTX_PTR;
    }

    if (withscores) {
        REDIS_CMD_APPEND_SSTR_STATIC(&cmdstr, "WITHSCORES");
        *ctx = PHPREDIS_CTX_PTR + 1;
    }


    *cmd = cmdstr.c;
    *cmd_len = cmdstr.len;
    return SUCCESS;
}

int
redis_zdiff_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock,
                char **cmd, int *cmd_len, short *slot, void **ctx)
{
    zval *z_keys, *z_opts = NULL, *z_key;
    redisZcmdOptions opts = {0};
    smart_string cmdstr = {0};
    int numkeys, flags;
    short s2 = 0;

    if (zend_parse_parameters(ZEND_NUM_ARGS(), "a|a",
                              &z_keys, &z_opts) == FAILURE)
    {
        return FAILURE;
    }

    if ((numkeys = zend_hash_num_elements(Z_ARRVAL_P(z_keys))) == 0) {
        return FAILURE;
    }

    flags = redis_get_zcmd_flags("ZDIFF");
    redis_get_zcmd_options(&opts, z_opts, flags);

    REDIS_CMD_INIT_SSTR_STATIC(&cmdstr, 1 + numkeys + opts.withscores, "ZDIFF");
    redis_cmd_append_sstr_long(&cmdstr, numkeys);

    ZEND_HASH_FOREACH_VAL(Z_ARRVAL_P(z_keys), z_key) {
        ZVAL_DEREF(z_key);
        redis_cmd_append_sstr_key_zval(&cmdstr, z_key, redis_sock, slot);

        if (slot && s2 && s2 != *slot) {
            php_error_docref(NULL, E_WARNING, "Not all keys map to the same slot!");
            efree(cmdstr.c);
            return FAILURE;
        }

        if (slot) s2 = *slot;
    } ZEND_HASH_FOREACH_END();

    if (opts.withscores) {
        REDIS_CMD_APPEND_SSTR_STATIC(&cmdstr, "WITHSCORES");
        *ctx = PHPREDIS_CTX_PTR;
    }

    *cmd = cmdstr.c;
    *cmd_len = cmdstr.len;
    return SUCCESS;
}

static int redis_cmd_append_sstr_score(smart_string *dst, zval *score) {
    zend_uchar type;
    zend_long lval;
    size_t cmdlen;
    double dval;

    /* Get current command length */
    cmdlen = dst->len;

    if (Z_TYPE_P(score) == IS_LONG) {
        redis_cmd_append_sstr_long(dst, Z_LVAL_P(score));
    } else if (Z_TYPE_P(score) == IS_DOUBLE) {
        redis_cmd_append_sstr_dbl(dst, Z_DVAL_P(score));
    } else if (Z_TYPE_P(score) == IS_STRING) {
        type = is_numeric_string(Z_STRVAL_P(score), Z_STRLEN_P(score), &lval, &dval, 0);
        if (type == IS_LONG) {
            redis_cmd_append_sstr_long(dst, lval);
        } else if (type == IS_DOUBLE) {
            redis_cmd_append_sstr_dbl(dst, dval);
        } else if (zend_string_equals_literal_ci(Z_STR_P(score), "-inf") ||
                   zend_string_equals_literal_ci(Z_STR_P(score), "+inf") ||
                   zend_string_equals_literal_ci(Z_STR_P(score), "inf"))
        {
            redis_cmd_append_sstr_zstr(dst, Z_STR_P(score));
        }
    }

    /* Success if we appended something */
    if (dst->len > cmdlen)
        return SUCCESS;

    /* Nothing appended, failure */
    php_error_docref(NULL, E_WARNING, "scores must be numeric or '-inf', 'inf', '+inf'");
    return FAILURE;
}

int redis_intercard_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock,
                        char *kw, char **cmd, int *cmd_len, short *slot,
                        void **ctx)
{
    smart_string cmdstr = {0};
    zend_long limit = -1;
    HashTable *keys;
    zend_string *key;
    zval *zv;

    ZEND_PARSE_PARAMETERS_START(1, 2)
        Z_PARAM_ARRAY_HT(keys)
        Z_PARAM_OPTIONAL
        Z_PARAM_LONG(limit)
    ZEND_PARSE_PARAMETERS_END_EX(return FAILURE);

    if (zend_hash_num_elements(keys) == 0) {
        php_error_docref(NULL, E_WARNING, "Must pass at least one key");
        return FAILURE;
    } else if (ZEND_NUM_ARGS() == 2 && limit < 0) {
        php_error_docref(NULL, E_WARNING, "LIMIT cannot be negative");
        return FAILURE;
    }

    redis_cmd_init_sstr(&cmdstr, 1 + zend_hash_num_elements(keys) + (limit > 0 ? 2 : 0), kw, strlen(kw));
    redis_cmd_append_sstr_long(&cmdstr, zend_hash_num_elements(keys));

    if (slot) *slot = -1;

    ZEND_HASH_FOREACH_VAL(keys, zv) {
        key = redis_key_prefix_zval(redis_sock, zv);

        if (slot) {
            if (*slot == -1) {
                *slot = cluster_hash_key_zstr(key);
            } else if (*slot != cluster_hash_key_zstr(key)) {
                php_error_docref(NULL, E_WARNING, "All keys don't hash to the same slot");
                efree(cmdstr.c);
                zend_string_release(key);
                return FAILURE;
            }
        }

        redis_cmd_append_sstr_zstr(&cmdstr, key);
        zend_string_release(key);
    } ZEND_HASH_FOREACH_END();

    if (limit > 0) {
        REDIS_CMD_APPEND_SSTR_STATIC(&cmdstr, "LIMIT");
        redis_cmd_append_sstr_long(&cmdstr, limit);
    }

    *cmd = cmdstr.c;
    *cmd_len = cmdstr.len;
    return SUCCESS;
}

int redis_replicaof_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock,
                        char *kw, char **cmd, int *cmd_len, short *slot,
                        void **ctx)
{
    zend_string *host = NULL;
    zend_long port = 6379;

    ZEND_PARSE_PARAMETERS_START(0, 2)
        Z_PARAM_OPTIONAL
        Z_PARAM_STR(host)
        Z_PARAM_LONG(port)
    ZEND_PARSE_PARAMETERS_END_EX(return FAILURE);

    if (port < 0 || port > UINT16_MAX) {
        php_error_docref(NULL, E_WARNING, "Invalid port %ld", (long)port);
        return FAILURE;
    }

    if (ZEND_NUM_ARGS() == 2) {
        *cmd_len = REDIS_SPPRINTF(cmd, kw, "Sd", host, (int)port);
    } else {
        *cmd_len = REDIS_SPPRINTF(cmd, kw, "ss", ZEND_STRL("NO"), ZEND_STRL("ONE"));
    }

    return SUCCESS;
}

int
redis_zinterunion_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock,
                      char *kw, char **cmd, int *cmd_len, short *slot,
                      void **ctx)
{
    zval *z_keys, *z_weights = NULL, *z_opts = NULL, *z_ele;
    redisZcmdOptions opts = {0};
    smart_string cmdstr = {0};
    int numkeys, flags;
    short s2 = 0;

    if (zend_parse_parameters(ZEND_NUM_ARGS(), "a|a!a",
                             &z_keys, &z_weights, &z_opts) == FAILURE)
    {
        return FAILURE;
    }

    if ((numkeys = zend_hash_num_elements(Z_ARRVAL_P(z_keys))) == 0) {
        return FAILURE;
    }

    if (z_weights && zend_hash_num_elements(Z_ARRVAL_P(z_weights)) != numkeys) {
        php_error_docref(NULL, E_WARNING, "WEIGHTS and keys array should be the same size!");
        return FAILURE;
    }

    flags = redis_get_zcmd_flags(kw);
    redis_get_zcmd_options(&opts, z_opts, flags);

    redis_cmd_init_sstr(&cmdstr, 1 + numkeys + (z_weights ? 1 + numkeys : 0) + (opts.aggregate ? 2 : 0) + opts.withscores, kw, strlen(kw));
    redis_cmd_append_sstr_long(&cmdstr, numkeys);

    ZEND_HASH_FOREACH_VAL(Z_ARRVAL_P(z_keys), z_ele) {
        ZVAL_DEREF(z_ele);
        redis_cmd_append_sstr_key_zval(&cmdstr, z_ele, redis_sock, slot);
        if (slot) {
            if (s2 && s2 != *slot) {
                php_error_docref(NULL, E_WARNING, "Not all keys hash to the same slot");
                efree(cmdstr.c);
                return FAILURE;
            }
            s2 = *slot;
        }
    } ZEND_HASH_FOREACH_END();

    if (z_weights) {
        REDIS_CMD_APPEND_SSTR_STATIC(&cmdstr, "WEIGHTS");
        ZEND_HASH_FOREACH_VAL(Z_ARRVAL_P(z_weights), z_ele) {
            ZVAL_DEREF(z_ele);
            if (redis_cmd_append_sstr_score(&cmdstr, z_ele) == FAILURE) {
                efree(cmdstr.c);
                return FAILURE;
            }
        } ZEND_HASH_FOREACH_END();
    }

    if (opts.aggregate) {
        REDIS_CMD_APPEND_SSTR_STATIC(&cmdstr, "AGGREGATE");
        redis_cmd_append_sstr_zstr(&cmdstr, opts.aggregate);
    }

    if (opts.withscores) {
        REDIS_CMD_APPEND_SSTR_STATIC(&cmdstr, "WITHSCORES");
        *ctx = PHPREDIS_CTX_PTR;
    }

    *cmd = cmdstr.c;
    *cmd_len = cmdstr.len;
    return SUCCESS;
}

int
redis_zdiffstore_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock,
                     char **cmd, int *cmd_len, short *slot, void **ctx)
{
    smart_string cmdstr = {0};
    zend_string *dst = NULL;
    HashTable *keys = NULL;
    zend_ulong nkeys;
    short s2 = 0;
    zval *zkey;

    ZEND_PARSE_PARAMETERS_START(2, 2)
        Z_PARAM_STR(dst)
        Z_PARAM_ARRAY_HT(keys)
    ZEND_PARSE_PARAMETERS_END_EX(return FAILURE);

    nkeys = zend_hash_num_elements(keys);
    if (nkeys == 0)
        return FAILURE;

    REDIS_CMD_INIT_SSTR_STATIC(&cmdstr, 2 + nkeys, "ZDIFFSTORE");
    redis_cmd_append_sstr_key_zstr(&cmdstr, dst, redis_sock, slot);
    redis_cmd_append_sstr_long(&cmdstr, nkeys);

    ZEND_HASH_FOREACH_VAL(keys, zkey) {
        ZVAL_DEREF(zkey);
        redis_cmd_append_sstr_key_zval(&cmdstr, zkey, redis_sock, slot ? &s2 : NULL);
        if (slot && *slot != s2) {
            php_error_docref(NULL, E_WARNING, "All keys must hash to the same slot");
            efree(cmdstr.c);
            return FAILURE;
        }
    } ZEND_HASH_FOREACH_END();

    *cmd = cmdstr.c;
    *cmd_len = cmdstr.len;
    return SUCCESS;
}

/* ZUNIONSTORE, ZINTERSTORE */
int
redis_zinterunionstore_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock,
                     char *kw, char **cmd, int *cmd_len, short *slot,
                     void **ctx)
{
    HashTable *keys = NULL, *weights = NULL;
    smart_string cmdstr = {0};
    zend_string *dst = NULL;
    zend_string *agg = NULL;
    zend_ulong nkeys;
    zval *zv = NULL;
    short s2 = 0;

    ZEND_PARSE_PARAMETERS_START(2, 4)
        Z_PARAM_STR(dst)
        Z_PARAM_ARRAY_HT(keys)
        Z_PARAM_OPTIONAL
        Z_PARAM_ARRAY_HT_OR_NULL(weights)
        Z_PARAM_STR_OR_NULL(agg)
    ZEND_PARSE_PARAMETERS_END_EX(return FAILURE);

    nkeys = zend_hash_num_elements(keys);
    if (nkeys == 0)
        return FAILURE;

    if (weights != NULL && zend_hash_num_elements(weights) != nkeys) {
        php_error_docref(NULL, E_WARNING, "WEIGHTS and keys array must be the same size!");
        return FAILURE;
    }

    // AGGREGATE option
    if (agg != NULL && (!zend_string_equals_literal_ci(agg, "SUM") &&
                        !zend_string_equals_literal_ci(agg, "MIN") &&
                        !zend_string_equals_literal_ci(agg, "MAX")))
    {
        php_error_docref(NULL, E_WARNING, "AGGREGATE option must be 'SUM', 'MIN', or 'MAX'");
        return FAILURE;
    }

    redis_cmd_init_sstr(&cmdstr, 2 + nkeys + (weights ? 1 + nkeys : 0) + (agg ? 2 : 0), kw, strlen(kw));
    redis_cmd_append_sstr_key_zstr(&cmdstr, dst, redis_sock, slot);
    redis_cmd_append_sstr_int(&cmdstr, nkeys);

    ZEND_HASH_FOREACH_VAL(keys, zv) {
        ZVAL_DEREF(zv);
        redis_cmd_append_sstr_key_zval(&cmdstr, zv, redis_sock, slot ? &s2 : NULL);
        if (slot && s2 != *slot) {
            php_error_docref(NULL, E_WARNING, "All keys don't hash to the same slot!");
            efree(cmdstr.c);
            return FAILURE;
        }
    } ZEND_HASH_FOREACH_END();

    if (weights) {
        REDIS_CMD_APPEND_SSTR_STATIC(&cmdstr, "WEIGHTS");
        ZEND_HASH_FOREACH_VAL(weights, zv) {
            ZVAL_DEREF(zv);
            if (redis_cmd_append_sstr_score(&cmdstr, zv) == FAILURE) {
                efree(cmdstr.c);
                return FAILURE;
            }
        } ZEND_HASH_FOREACH_END();
    }

    if (agg) {
        redis_cmd_append_sstr(&cmdstr, ZEND_STRL("AGGREGATE"));
        redis_cmd_append_sstr_zstr(&cmdstr, agg);
    }

    // Push out values
    *cmd = cmdstr.c;
    *cmd_len = cmdstr.len;

    return SUCCESS;
}

int redis_pubsub_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock,
                     char **cmd, int *cmd_len, short *slot, void **ctx)
{
    HashTable *channels = NULL;
    smart_string cmdstr = {0};
    zend_string *op, *pattern = NULL;
    zval *arg = NULL, *z_chan;

    ZEND_PARSE_PARAMETERS_START(1, 2)
        Z_PARAM_STR(op)
        Z_PARAM_OPTIONAL
        Z_PARAM_ZVAL(arg)
    ZEND_PARSE_PARAMETERS_END_EX(return FAILURE);

    if (zend_string_equals_literal_ci(op, "NUMPAT")) {
        *ctx = NULL;
    } else if (zend_string_equals_literal_ci(op, "CHANNELS") ||
        zend_string_equals_literal_ci(op, "SHARDCHANNELS")
    ) {
        if (arg != NULL) {
            if (Z_TYPE_P(arg) != IS_STRING) {
                php_error_docref(NULL, E_WARNING, "Invalid patern value");
                return FAILURE;
            }
            pattern = zval_get_string(arg);
        }
        *ctx = PHPREDIS_CTX_PTR;
    } else if (zend_string_equals_literal_ci(op, "NUMSUB") ||
        zend_string_equals_literal_ci(op, "SHARDNUMSUB")
    ) {
        if (arg != NULL) {
            if (Z_TYPE_P(arg) != IS_ARRAY) {
                php_error_docref(NULL, E_WARNING, "Invalid channels value");
                return FAILURE;
            }
            channels = Z_ARRVAL_P(arg);
        }
        *ctx = PHPREDIS_CTX_PTR + 1;
    } else {
        php_error_docref(NULL, E_WARNING, "Unknown PUBSUB operation '%s'", ZSTR_VAL(op));
        return FAILURE;
    }

    REDIS_CMD_INIT_SSTR_STATIC(&cmdstr, 1 + !!pattern + (channels ? zend_hash_num_elements(channels) : 0), "PUBSUB");
    redis_cmd_append_sstr_zstr(&cmdstr, op);

    if (pattern != NULL) {
        redis_cmd_append_sstr_zstr(&cmdstr, pattern);
        zend_string_release(pattern);
    } else if (channels != NULL) {
        ZEND_HASH_FOREACH_VAL(channels, z_chan) {
            redis_cmd_append_sstr_key_zval(&cmdstr, z_chan, redis_sock, slot);
        } ZEND_HASH_FOREACH_END();
    }

    *cmd = cmdstr.c;
    *cmd_len = cmdstr.len;

    return SUCCESS;
}

/* SUBSCRIBE/PSUBSCRIBE/SSUBSCRIBE */
int redis_subscribe_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock,
                        char *kw, char **cmd, int *cmd_len, short *slot,
                        void **ctx)
{
    zval *z_arr, *z_chan;
    HashTable *ht_chan;
    smart_string cmdstr = {0};
    subscribeContext *sctx = ecalloc(1, sizeof(*sctx));
    unsigned short shardslot = REDIS_CLUSTER_SLOTS;
    short s2;

    if (zend_parse_parameters(ZEND_NUM_ARGS(), "af", &z_arr,
                             &sctx->cb.fci, &sctx->cb.fci_cache) == FAILURE)
    {
        efree(sctx);
        return FAILURE;
    }

    ht_chan    = Z_ARRVAL_P(z_arr);
    sctx->kw   = kw;
    sctx->argc = zend_hash_num_elements(ht_chan);

    if (sctx->argc == 0) {
        efree(sctx);
        return FAILURE;
    }

    if (strcasecmp(kw, "ssubscribe") == 0) {
        zend_hash_internal_pointer_reset(ht_chan);
        if ((z_chan = zend_hash_get_current_data(ht_chan)) == NULL) {
            php_error_docref(NULL, E_WARNING, "Internal Zend HashTable error");
            efree(sctx);
            return FAILURE;
        }
        shardslot = cluster_hash_key_zval(z_chan);
    }

    // Start command construction
    redis_cmd_init_sstr(&cmdstr, sctx->argc, kw, strlen(kw));

    // Iterate over channels
    ZEND_HASH_FOREACH_VAL(ht_chan, z_chan) {
        redis_cmd_append_sstr_key_zval(&cmdstr, z_chan, redis_sock, slot ? &s2 : NULL);

        if (slot && (shardslot != REDIS_CLUSTER_SLOTS && s2 != shardslot)) {
            php_error_docref(NULL, E_WARNING, "All shard channels needs to belong to a single slot");
            smart_string_free(&cmdstr);
            efree(sctx);
            return FAILURE;
        }
    } ZEND_HASH_FOREACH_END();

    // Push values out
    *cmd_len = cmdstr.len;
    *cmd     = cmdstr.c;
    *ctx     = (void*)sctx;

    if (shardslot != REDIS_CLUSTER_SLOTS) {
        if (slot) *slot = shardslot;
    } else {
        // Pick a slot at random
        CMD_RAND_SLOT(slot);
    }

    return SUCCESS;
}

/* UNSUBSCRIBE/PUNSUBSCRIBE/SUNSUBSCRIBE */
int redis_unsubscribe_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock,
                          char *kw, char **cmd, int *cmd_len, short *slot,
                          void **ctx)
{
    smart_string cmdstr = {0};
    subscribeContext *sctx;
    HashTable *channels;
    zval *channel;

    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_ARRAY_HT(channels)
    ZEND_PARSE_PARAMETERS_END_EX(return FAILURE);

    if (zend_hash_num_elements(channels) == 0)
        return FAILURE;

    sctx = ecalloc(1, sizeof(*sctx));
    sctx->kw = kw;
    sctx->argc = zend_hash_num_elements(channels);

    redis_cmd_init_sstr(&cmdstr, sctx->argc, kw, strlen(kw));

    ZEND_HASH_FOREACH_VAL(channels, channel) {
        redis_cmd_append_sstr_key_zval(&cmdstr, channel, redis_sock, slot);
    } ZEND_HASH_FOREACH_END();

    *cmd_len = cmdstr.len;
    *cmd = cmdstr.c;
    *ctx = sctx;

    return SUCCESS;
}

/* ZRANGEBYLEX/ZREVRANGEBYLEX */
int redis_zrangebylex_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock,
                          char *kw, char **cmd, int *cmd_len, short *slot,
                          void **ctx)
{
    char *key, *min, *max;
    size_t key_len, min_len, max_len;
    int argc = ZEND_NUM_ARGS();
    zend_long offset, count;

    /* We need either 3 or 5 arguments for this to be valid */
    if (argc != 3 && argc != 5) {
        php_error_docref(0, E_WARNING, "Must pass either 3 or 5 arguments");
        return FAILURE;
    }

    if (zend_parse_parameters(argc, "sss|ll", &key, &key_len, &min, &min_len,
                             &max, &max_len, &offset, &count) == FAILURE)
    {
        return FAILURE;
    }

    /* min and max must start with '(' or '[', or be either '-' or '+' */
    if (!validate_zlex_arg(min, min_len) || !validate_zlex_arg(max, max_len)) {
        php_error_docref(NULL, E_WARNING,
            "Min/Max args can be '-' or '+', or start with '[' or '('");
        return FAILURE;
    }

    /* Construct command */
    if (argc == 3) {
        *cmd_len = REDIS_CMD_SPPRINTF(cmd, kw, "kss", key, key_len, min, min_len,
            max, max_len);
    } else {
        *cmd_len = REDIS_CMD_SPPRINTF(cmd, kw, "ksssll", key, key_len, min, min_len,
            max, max_len, "LIMIT", 5, offset, count);
    }

    return SUCCESS;
}

/* ZLEXCOUNT/ZREMRANGEBYLEX */
int redis_gen_zlex_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock,
                       char *kw, char **cmd, int *cmd_len, short *slot,
                       void **ctx)
{
    char *key, *min, *max;
    size_t key_len, min_len, max_len;

    /* Parse args */
    if (zend_parse_parameters(ZEND_NUM_ARGS(), "sss", &key, &key_len,
                             &min, &min_len, &max, &max_len) == FAILURE)
    {
        return FAILURE;
    }

    /* Quick sanity check on min/max */
    if (!validate_zlex_arg(min, min_len) || !validate_zlex_arg(max, max_len)) {
        php_error_docref(NULL, E_WARNING,
            "Min/Max args can be '-' or '+', or start with '[' or '('");
        return FAILURE;
    }

    /* Construct command */
    *cmd_len = REDIS_CMD_SPPRINTF(cmd, kw, "kss", key, key_len, min, min_len,
        max, max_len);

    return SUCCESS;
}

/* EVAL and EVALSHA */
int redis_eval_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock, char *kw,
                   char **cmd, int *cmd_len, short *slot, void **ctx)
{
    char *lua;
    int argc = 0;
    zval *z_arr = NULL, *z_ele;
    HashTable *ht_arr;
    zend_long num_keys = 0;
    smart_string cmdstr = {0};
    size_t lua_len;
    zend_string *zstr;
    short prevslot = -1;

    /* Parse args */
    if (zend_parse_parameters(ZEND_NUM_ARGS(), "s|al", &lua, &lua_len,
                             &z_arr, &num_keys) == FAILURE)
    {
        return FAILURE;
    }

    /* Grab arg count */
    if (z_arr != NULL) {
        ht_arr = Z_ARRVAL_P(z_arr);
        argc = zend_hash_num_elements(ht_arr);
    }

    /* EVAL[SHA] {script || sha1} {num keys}  */
    redis_cmd_init_sstr(&cmdstr, 2 + argc, kw, strlen(kw));
    redis_cmd_append_sstr(&cmdstr, lua, lua_len);
    redis_cmd_append_sstr_long(&cmdstr, num_keys);

    // Iterate over our args if we have any
    if (argc > 0) {
        ZEND_HASH_FOREACH_VAL(ht_arr, z_ele) {
            zstr = zval_get_string(z_ele);

            /* If we're still on a key, prefix it check slot */
            if (num_keys-- > 0) {
                redis_cmd_append_sstr_key(&cmdstr, ZSTR_VAL(zstr), ZSTR_LEN(zstr), redis_sock, slot);

                /* If we have been passed a slot, all keys must match */
                if (slot) {
                    if (prevslot != -1 && prevslot != *slot) {
                        zend_string_release(zstr);
                        php_error_docref(0, E_WARNING, "All keys do not map to the same slot");
                        return FAILURE;
                    }
                    prevslot = *slot;
                }
            } else {
                redis_cmd_append_sstr(&cmdstr, ZSTR_VAL(zstr), ZSTR_LEN(zstr));
            }

            zend_string_release(zstr);
        } ZEND_HASH_FOREACH_END();
    } else {
        /* Any slot will do */
        CMD_RAND_SLOT(slot);
    }

    *cmd = cmdstr.c;
    *cmd_len = cmdstr.len;
    return SUCCESS;
}

/* Commands that take a key followed by a variable list of serializable
 * values (RPUSH, LPUSH, SADD, SREM, etc...) */
int redis_key_varval_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock,
                         char *kw, char **cmd, int *cmd_len, short *slot,
                         void **ctx)
{
    zval *z_args;
    smart_string cmdstr = {0};
    size_t i;
    int argc = ZEND_NUM_ARGS();

    // We at least need a key and one value
    if (argc < 2) {
        zend_wrong_param_count();
        return FAILURE;
    }

    // Make sure we at least have a key, and we can get other args
    z_args = emalloc(argc * sizeof(zval));
    if (zend_get_parameters_array(ht, argc, z_args) == FAILURE) {
        efree(z_args);
        return FAILURE;
    }

    /* Initialize our command */
    redis_cmd_init_sstr(&cmdstr, argc, kw, strlen(kw));

    /* Append key */
    zend_string *zstr = zval_get_string(&z_args[0]);
    redis_cmd_append_sstr_key(&cmdstr, ZSTR_VAL(zstr), ZSTR_LEN(zstr), redis_sock, slot);
    zend_string_release(zstr);

    /* Add members */
    for (i = 1; i < argc; i++ ){
        redis_cmd_append_sstr_zval(&cmdstr, &z_args[i], redis_sock);
    }

    // Push out values
    *cmd     = cmdstr.c;
    *cmd_len = cmdstr.len;

    // Cleanup arg array
    efree(z_args);

    // Success!
    return SUCCESS;
}

/* Commands that take a key and then an array of values */
static int gen_key_arr_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock,
                           char *kw, zend_bool pack_values, char **cmd, int *cmd_len,
                           short *slot, void **ctx)
{
    smart_string cmdstr = {0};
    HashTable *values = NULL;
    zend_string *key = NULL;
    zval *zv;

    ZEND_PARSE_PARAMETERS_START(2, 2)
        Z_PARAM_STR(key)
        Z_PARAM_ARRAY_HT(values)
    ZEND_PARSE_PARAMETERS_END_EX(return FAILURE);

    if (zend_hash_num_elements(values) == 0)
        return FAILURE;

    redis_cmd_init_sstr(&cmdstr, 1 + zend_hash_num_elements(values), kw, strlen(kw));
    redis_cmd_append_sstr_key_zstr(&cmdstr, key, redis_sock, slot);

    ZEND_HASH_FOREACH_VAL(values, zv) {
        redis_cmd_append_sstr_zval(&cmdstr, zv, pack_values ? redis_sock : NULL);
    } ZEND_HASH_FOREACH_END();

    *cmd = cmdstr.c;
    *cmd_len = cmdstr.len;

    return SUCCESS;
}

int redis_key_val_arr_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock,
                        char *kw, char **cmd, int *cmd_len, short *slot,
                        void **ctx)
{
    return gen_key_arr_cmd(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock, kw,
        1, cmd, cmd_len, slot, ctx);
}

int redis_key_str_arr_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock,
                        char *kw, char **cmd, int *cmd_len, short *slot,
                        void **ctx)
{
    return gen_key_arr_cmd(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock, kw,
        0, cmd, cmd_len, slot, ctx);
}

/* Generic function that takes one or more non-serialized arguments */
static int
gen_vararg_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock,
               uint32_t min_argc, char *kw, char **cmd, int *cmd_len,
               short *slot, void **ctx)
{
    smart_string cmdstr = {0};
    zval *argv = NULL;
    int argc = 0;
    uint32_t i;

    ZEND_PARSE_PARAMETERS_START(min_argc, -1)
        Z_PARAM_VARIADIC('*', argv, argc)
    ZEND_PARSE_PARAMETERS_END_EX(return FAILURE);

    redis_cmd_init_sstr(&cmdstr, argc, kw, strlen(kw));

    for (i = 0; i < argc; i++) {
        redis_cmd_append_sstr_zval(&cmdstr, &argv[i], NULL);
    }

    *cmd = cmdstr.c;
    *cmd_len = cmdstr.len;

    return SUCCESS;
}

int redis_mset_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock,
                   char *kw, char **cmd, int *cmd_len, short *slot,
                   void **ctx)
{
    smart_string cmdstr = {0};
    HashTable *kvals = NULL;
    zend_string *key;
    zend_ulong idx;
    zval *zv;

    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_ARRAY_HT(kvals)
    ZEND_PARSE_PARAMETERS_END_EX(return FAILURE);

    if (zend_hash_num_elements(kvals) == 0)
        return FAILURE;

    redis_cmd_init_sstr(&cmdstr, zend_hash_num_elements(kvals) * 2, kw, strlen(kw));

    ZEND_HASH_FOREACH_KEY_VAL(kvals, idx, key, zv) {
        ZVAL_DEREF(zv);
        if (key) {
            redis_cmd_append_sstr_key_zstr(&cmdstr, key, redis_sock, NULL);
        } else {
            redis_cmd_append_sstr_key_long(&cmdstr, idx, redis_sock, NULL);
        }
        redis_cmd_append_sstr_zval(&cmdstr, zv, redis_sock);
    } ZEND_HASH_FOREACH_END();

    *cmd = cmdstr.c;
    *cmd_len = cmdstr.len;

    return SUCCESS;
}

/* Generic function that takes a variable number of keys, with an optional
 * timeout value.  This can handle various SUNION/SUNIONSTORE/BRPOP type
 * commands. */
static int gen_varkey_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock,
                          char *kw, int kw_len, zend_bool has_timeout,
                          char **cmd, int *cmd_len, short *slot)
{
    zval *argv = NULL, ztimeout = {0}, *zv;
    smart_string cmdstr = {0};
    uint32_t min_argc;
    short kslot = -1;
    int single_array;
    int argc = 0;

    min_argc = has_timeout ? 2 : 1;

    ZEND_PARSE_PARAMETERS_START(min_argc, -1)
        Z_PARAM_VARIADIC('*', argv, argc)
    ZEND_PARSE_PARAMETERS_END_EX(return FAILURE);

    single_array = argc == min_argc && Z_TYPE(argv[0]) == IS_ARRAY;

    if (has_timeout) {
        if (single_array)
            ZVAL_COPY_VALUE(&ztimeout, &argv[1]);
        else
            ZVAL_COPY_VALUE(&ztimeout, &argv[argc - 1]);

        if (Z_TYPE(ztimeout) != IS_LONG && Z_TYPE(ztimeout) != IS_DOUBLE) {
            php_error_docref(NULL, E_WARNING, "Timeout must be a long or double");
            return FAILURE;
        }
    }

    // If we're running a single array, rework args
    if (single_array) {
        /* Need at least one argument */
        argc = zend_hash_num_elements(Z_ARRVAL(argv[0]));
        if (argc == 0)
            return FAILURE;

        if (has_timeout) argc++;
    }

    // Begin construction of our command
    redis_cmd_init_sstr(&cmdstr, argc, kw, kw_len);

    if (single_array) {
        ZEND_HASH_FOREACH_VAL(Z_ARRVAL(argv[0]), zv) {
            redis_cmd_append_sstr_key_zval(&cmdstr, zv, redis_sock, slot);
            if (slot) {
                if (kslot != -1 && *slot != kslot)
                    goto cross_slot;
                kslot = *slot;
            }
        } ZEND_HASH_FOREACH_END();
    } else {
        uint32_t i;
        for(i = 0; i < argc - !!has_timeout; i++) {
            redis_cmd_append_sstr_key_zval(&cmdstr, &argv[i], redis_sock, slot);
            if (slot) {
                if (kslot != -1 && *slot != kslot)
                    goto cross_slot;
                kslot = *slot;
            }
        }
    }

    if (Z_TYPE(ztimeout) == IS_DOUBLE) {
        redis_cmd_append_sstr_dbl(&cmdstr, Z_DVAL(ztimeout));
    } else if (Z_TYPE(ztimeout) == IS_LONG) {
        redis_cmd_append_sstr_long(&cmdstr, Z_LVAL(ztimeout));
    }

    // Push out parameters
    *cmd = cmdstr.c;
    *cmd_len = cmdstr.len;

    return SUCCESS;

cross_slot:
    efree(cmdstr.c);
    php_error_docref(NULL, E_WARNING, "Not all keys hash to the same slot!");
    return FAILURE;
}

int redis_mpop_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock, char *kw,
                   char **cmd, int *cmd_len, short *slot, void **ctx)
{
    int argc, blocking, is_zmpop;
    smart_string cmdstr = {0};
    zend_string *from = NULL;
    HashTable *keys = NULL;
    double timeout = 0.0;
    zend_long count = 1;
    short slot2 = -1;
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
    } ZEND_PARSE_PARAMETERS_END_EX(return FAILURE);

    if (zend_hash_num_elements(keys) == 0) {
        php_error_docref(NULL, E_WARNING, "Must pass at least one key");
        return FAILURE;
    } else if (count < 1) {
        php_error_docref(NULL, E_WARNING, "Count must be > 0");
        return FAILURE;
    } else if (!is_zmpop && !(zend_string_equals_literal_ci(from, "LEFT") ||
                              zend_string_equals_literal_ci(from, "RIGHT")))
    {
        php_error_docref(NULL, E_WARNING, "from must be either 'LEFT' or 'RIGHT'");
        return FAILURE;
    } else if (is_zmpop && !(zend_string_equals_literal_ci(from, "MIN") ||
                             zend_string_equals_literal_ci(from, "MAX")))
    {
        php_error_docref(NULL, E_WARNING, "from must be either 'MIN' or 'MAX'");
        return FAILURE;
    }

    argc = 2 + !!blocking + zend_hash_num_elements(keys) + (count != 1 ? 2 : 0);
    redis_cmd_init_sstr(&cmdstr, argc, kw, strlen(kw));

    if (blocking) redis_cmd_append_sstr_dbl(&cmdstr, timeout);
    redis_cmd_append_sstr_long(&cmdstr, zend_hash_num_elements(keys));

    if (slot) *slot = -1;

    ZEND_HASH_FOREACH_VAL(keys, zv) {
        redis_cmd_append_sstr_key_zval(&cmdstr, zv, redis_sock, slot);
        if (slot) {
            if (slot2 != -1 && *slot != slot2) {
                php_error_docref(NULL, E_WARNING, "All keys don't hash to the same slot");
                efree(cmdstr.c);
                return FAILURE;
            }
            slot2 = *slot;
        }
    } ZEND_HASH_FOREACH_END();

    redis_cmd_append_sstr_zstr(&cmdstr, from);

    if (count != 1) {
        REDIS_CMD_APPEND_SSTR_STATIC(&cmdstr, "COUNT");
        redis_cmd_append_sstr_long(&cmdstr, count);
    }

    *ctx = is_zmpop ? PHPREDIS_CTX_PTR : NULL;
    *cmd = cmdstr.c;
    *cmd_len = cmdstr.len;

    return SUCCESS;
}

int redis_info_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock,
                   char **cmd, int *cmd_len, short *slot, void **ctx)
{
    return gen_vararg_cmd(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock, 0,
                          "INFO", cmd, cmd_len, slot, ctx);
}

int redis_script_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock,
                   char **cmd, int *cmd_len, short *slot, void **ctx)
{
    return gen_vararg_cmd(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock, 1,
                          "SCRIPT", cmd, cmd_len, slot, ctx);
}

/* Generic handling of every blocking pop command (BLPOP, BZPOP[MIN/MAX], etc */
int redis_blocking_pop_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock,
                           char *kw, char **cmd, int *cmd_len, short *slot,
                           void **ctx)
{
    return gen_varkey_cmd(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock, kw,
        strlen(kw), 1, cmd, cmd_len, slot);
}

/*
 * Commands with specific signatures or that need unique functions because they
 * have specific processing (argument validation, etc) that make them unique
 */

int
redis_pop_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock,
              char *kw, char **cmd, int *cmd_len, short *slot, void **ctx)
{
    char *key;
    size_t key_len;
    smart_string cmdstr = {0};
    zend_long count = 0;

    // Make sure the function is being called correctly
    if (zend_parse_parameters(ZEND_NUM_ARGS(), "s|l",
                              &key, &key_len, &count) == FAILURE)
    {
        return FAILURE;
    }

    redis_cmd_init_sstr(&cmdstr, 1 + (count > 0), kw, strlen(kw));
    redis_cmd_append_sstr_key(&cmdstr, key, key_len, redis_sock, slot);
    if (count > 0) {
        redis_cmd_append_sstr_long(&cmdstr, (long)count);
        *ctx = PHPREDIS_CTX_PTR;
    }

    *cmd = cmdstr.c;
    *cmd_len = cmdstr.len;

    return SUCCESS;
}

int
redis_acl_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock,
              char **cmd, int *cmd_len, short *slot, void **ctx)
{
    smart_string cmdstr = {0};
    zend_string *op, *zstr;
    zval *z_args = NULL;
    int argc = 0, i;

    ZEND_PARSE_PARAMETERS_START(1, -1)
        Z_PARAM_STR(op)
        Z_PARAM_OPTIONAL
        Z_PARAM_VARIADIC('*', z_args, argc)
    ZEND_PARSE_PARAMETERS_END_EX(return FAILURE);

    if (zend_string_equals_literal_ci(op, "CAT") ||
        zend_string_equals_literal_ci(op, "LIST") ||
        zend_string_equals_literal_ci(op, "USERS")
    ) {
        *ctx = NULL;
    } else if (zend_string_equals_literal_ci(op, "LOAD") ||
        zend_string_equals_literal_ci(op, "SAVE")
    ) {
        *ctx = PHPREDIS_CTX_PTR;
    } else if (zend_string_equals_literal_ci(op, "GENPASS") ||
        zend_string_equals_literal_ci(op, "WHOAMI")
    ) {
        *ctx = PHPREDIS_CTX_PTR + 1;
    } else if (zend_string_equals_literal_ci(op, "SETUSER")) {
        if (argc < 1) {
            php_error_docref(NULL, E_WARNING, "ACL SETUSER requires at least one argument");
            return FAILURE;
        }
        *ctx = PHPREDIS_CTX_PTR;
    } else if (zend_string_equals_literal_ci(op, "DELUSER")) {
        if (argc < 1) {
            php_error_docref(NULL, E_WARNING, "ACL DELUSER requires at least one argument");
            return FAILURE;
        }
        *ctx = PHPREDIS_CTX_PTR + 2;
    } else if (zend_string_equals_literal_ci(op, "GETUSER")) {
        if (argc < 1) {
            php_error_docref(NULL, E_WARNING, "ACL GETUSER requires at least one argument");
            return FAILURE;
        }
        *ctx = PHPREDIS_CTX_PTR + 3;
    } else if (zend_string_equals_literal_ci(op, "DRYRUN")) {
        if (argc < 2) {
            php_error_docref(NULL, E_WARNING, "ACL DRYRUN requires at least two arguments");
            return FAILURE;
        }
        *ctx = PHPREDIS_CTX_PTR;
    } else if (zend_string_equals_literal_ci(op, "LOG")) {
        if (argc > 0 && Z_TYPE(z_args[0]) == IS_STRING && ZVAL_STRICMP_STATIC(&z_args[0], "RESET")) {
            *ctx = PHPREDIS_CTX_PTR;
        } else {
            *ctx = PHPREDIS_CTX_PTR + 4;
        }
    } else {
        php_error_docref(NULL, E_WARNING, "Unknown ACL operation '%s'", ZSTR_VAL(op));
        return FAILURE;
    }

    REDIS_CMD_INIT_SSTR_STATIC(&cmdstr, 1 + argc, "ACL");
    redis_cmd_append_sstr_zstr(&cmdstr, op);

    for (i = 0; i < argc; ++i) {
        zstr = zval_get_string(&z_args[i]);
        redis_cmd_append_sstr_zstr(&cmdstr, zstr);
        zend_string_release(zstr);
    }

    *cmd = cmdstr.c;
    *cmd_len = cmdstr.len;

    return SUCCESS;
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

/* SET */
int redis_set_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock,
                  char **cmd, int *cmd_len, short *slot, void **ctx)
{
    smart_string cmdstr = {0};
    zval *z_value, *z_opts=NULL;
    char *key = NULL, *exp_type = NULL, *set_type = NULL;
    long exp_set = 0, keep_ttl = 0;
    zend_long expire = -1;
    zend_bool get = 0;
    size_t key_len;

    // Make sure the function is being called correctly
    if (zend_parse_parameters(ZEND_NUM_ARGS(), "sz|z", &key, &key_len,
                             &z_value, &z_opts) == FAILURE)
    {
        return FAILURE;
    }

    // Check for an options array
    if (z_opts && Z_TYPE_P(z_opts) == IS_ARRAY) {
        HashTable *kt = Z_ARRVAL_P(z_opts);
        zend_string *zkey;
        zval *v;

        /* Iterate our option array */
        ZEND_HASH_FOREACH_STR_KEY_VAL(kt, zkey, v) {
            ZVAL_DEREF(v);
            /* Detect PX or EX argument and validate timeout */
            if (zkey && (zend_string_equals_literal_ci(zkey, "EX") ||
                         zend_string_equals_literal_ci(zkey, "PX") ||
                         zend_string_equals_literal_ci(zkey, "EXAT") ||
                         zend_string_equals_literal_ci(zkey, "PXAT"))
            ) {
                exp_set = 1;

                /* Set expire type */
                exp_type = ZSTR_VAL(zkey);

                /* Try to extract timeout */
                if (Z_TYPE_P(v) == IS_LONG) {
                    expire = Z_LVAL_P(v);
                } else if (Z_TYPE_P(v) == IS_STRING) {
                    expire = atol(Z_STRVAL_P(v));
                }
            } else if (Z_TYPE_P(v) == IS_STRING) {
                if (zend_string_equals_literal_ci(Z_STR_P(v), "KEEPTTL")) {
                    keep_ttl  = 1;
                } else if (zend_string_equals_literal_ci(Z_STR_P(v), "GET")) {
                    get = 1;
                } else if (zend_string_equals_literal_ci(Z_STR_P(v), "NX") ||
                           zend_string_equals_literal_ci(Z_STR_P(v), "XX"))
                {
                    set_type = Z_STRVAL_P(v);
                }
            }
        } ZEND_HASH_FOREACH_END();
    } else if (z_opts && Z_TYPE_P(z_opts) != IS_NULL) {
        if (redis_try_get_expiry(z_opts, &expire) == FAILURE) {
            php_error_docref(NULL, E_WARNING, "Expire must be a long, double, or a numeric string");
            return FAILURE;
        }

        exp_set = 1;
    }

    /* Protect the user from syntax errors but give them some info about what's wrong */
    if (exp_set && expire < 1) {
        php_error_docref(NULL, E_WARNING, "EXPIRE can't be < 1");
        return FAILURE;
    } else if (exp_type && keep_ttl) {
        php_error_docref(NULL, E_WARNING, "KEEPTTL can't be combined with EX or PX option");
        return FAILURE;
    }

    /* Backward compatibility:  If we are passed no options except an EXPIRE ttl, we
     * actually execute a SETEX command */
    if (expire > 0 && !exp_type && !set_type && !keep_ttl) {
        *cmd_len = REDIS_CMD_SPPRINTF(cmd, "SETEX", "klv", key, key_len, expire, z_value);
        return SUCCESS;
    }

    /* Calculate argc based on options set */
    int argc = 2 + (exp_type ? 2 : 0) + (set_type != NULL) + (keep_ttl != 0) + get;

    /* Initial SET <key> <value> */
    redis_cmd_init_sstr(&cmdstr, argc, "SET", 3);
    redis_cmd_append_sstr_key(&cmdstr, key, key_len, redis_sock, slot);
    redis_cmd_append_sstr_zval(&cmdstr, z_value, redis_sock);

    if (exp_type) {
        redis_cmd_append_sstr(&cmdstr, exp_type, strlen(exp_type));
        redis_cmd_append_sstr_long(&cmdstr, (long)expire);
    }

    if (set_type)
        redis_cmd_append_sstr(&cmdstr, set_type, strlen(set_type));
    if (keep_ttl)
        redis_cmd_append_sstr(&cmdstr, "KEEPTTL", 7);
    if (get) {
        REDIS_CMD_APPEND_SSTR_STATIC(&cmdstr, "GET");
        *ctx = PHPREDIS_CTX_PTR;
    }

    /* Push command and length to the caller */
    *cmd = cmdstr.c;
    *cmd_len = cmdstr.len;

    return SUCCESS;
}

/* MGET */
int redis_mget_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock,
                   char **cmd, int *cmd_len, short *slot, void **ctx)
{
    smart_string cmdstr = {0};
    HashTable *keys = NULL;
    zval *zkey;

    /* RedisCluster has a custom MGET implementation */
    ZEND_ASSERT(slot == NULL);

    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_ARRAY_HT(keys)
    ZEND_PARSE_PARAMETERS_END_EX(return FAILURE);

    if (zend_hash_num_elements(keys) == 0)
        return FAILURE;

    REDIS_CMD_INIT_SSTR_STATIC(&cmdstr, zend_hash_num_elements(keys), "MGET");

    ZEND_HASH_FOREACH_VAL(keys, zkey) {
        ZVAL_DEREF(zkey);
        redis_cmd_append_sstr_key_zval(&cmdstr, zkey, redis_sock, slot);
    } ZEND_HASH_FOREACH_END();

    *cmd = cmdstr.c;
    *cmd_len = cmdstr.len;

    return SUCCESS;
}

int
redis_getex_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock,
                char **cmd, int *cmd_len, short *slot, void **ctx)
{
    smart_string cmdstr = {0};
    char *key, *exp_type = NULL;
    zval *z_opts = NULL, *z_ele;
    zend_long expire = -1;
    zend_bool persist = 0;
    zend_string *zkey;
    size_t key_len;

    if (zend_parse_parameters(ZEND_NUM_ARGS(), "s|a",
                              &key, &key_len, &z_opts) == FAILURE)
    {
        return FAILURE;
    }

    if (z_opts != NULL) {
        ZEND_HASH_FOREACH_STR_KEY_VAL(Z_ARRVAL_P(z_opts), zkey, z_ele) {
            if (zkey != NULL) {
                ZVAL_DEREF(z_ele);
                if (ZSTR_STRICMP_STATIC(zkey, "EX") ||
                    ZSTR_STRICMP_STATIC(zkey, "PX") ||
                    ZSTR_STRICMP_STATIC(zkey, "EXAT") ||
                    ZSTR_STRICMP_STATIC(zkey, "PXAT")
                ) {
                    exp_type = ZSTR_VAL(zkey);
                    expire = zval_get_long(z_ele);
                    persist = 0;
                } else if (ZSTR_STRICMP_STATIC(zkey, "PERSIST")) {
                    persist = zval_is_true(z_ele);
                    exp_type = NULL;
                }
            }
        } ZEND_HASH_FOREACH_END();
    }

    if (exp_type != NULL && expire < 1) {
        php_error_docref(NULL, E_WARNING, "EXPIRE can't be < 1");
        return FAILURE;
    }

    REDIS_CMD_INIT_SSTR_STATIC(&cmdstr, 1 + (exp_type ? 2 : persist), "GETEX");
    redis_cmd_append_sstr_key(&cmdstr, key, key_len, redis_sock, slot);

    if (exp_type != NULL) {
        redis_cmd_append_sstr(&cmdstr, exp_type, strlen(exp_type));
        redis_cmd_append_sstr_long(&cmdstr, expire);
    } else if (persist) {
        REDIS_CMD_APPEND_SSTR_STATIC(&cmdstr, "PERSIST");
    }

    *cmd = cmdstr.c;
    *cmd_len = cmdstr.len;

    return SUCCESS;
}

/* BRPOPLPUSH */
int redis_brpoplpush_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock,
                         char **cmd, int *cmd_len, short *slot, void **ctx)
{
    zend_string *src = NULL, *dst = NULL;
    double timeout = 0;

    ZEND_PARSE_PARAMETERS_START(3, 3)
        Z_PARAM_STR(src)
        Z_PARAM_STR(dst)
        Z_PARAM_DOUBLE(timeout)
    ZEND_PARSE_PARAMETERS_END_EX(return FAILURE);

    src = redis_key_prefix_zstr(redis_sock, src);
    dst = redis_key_prefix_zstr(redis_sock, dst);

    if (slot && (*slot = cluster_hash_key_zstr(src)) != cluster_hash_key_zstr(dst)) {
        php_error_docref(NULL, E_WARNING, "Keys must hash to the same slot");
        zend_string_release(src);
        zend_string_release(dst);
        return FAILURE;
    }

    /* Consistency with Redis.  If timeout < 0 use RPOPLPUSH */
    if (timeout < 0) {
        *cmd_len = REDIS_CMD_SPPRINTF(cmd, "RPOPLPUSH", "SS", src, dst);
    } else if (fabs(timeout - (long)timeout) < .0001) {
        *cmd_len = REDIS_CMD_SPPRINTF(cmd, "BRPOPLPUSH", "SSd", src, dst, (long)timeout);
    } else {
        *cmd_len = REDIS_CMD_SPPRINTF(cmd, "BRPOPLPUSH", "SSf", src, dst, timeout);
    }

    zend_string_release(src);
    zend_string_release(dst);

    return SUCCESS;
}

/* To maintain backward compatibility with earlier versions of phpredis, we
 * allow for an optional "increment by" argument for INCR and DECR even though
 * that's not how Redis proper works */
#define TYPE_INCR 0
#define TYPE_DECR 1

/* Handle INCR(BY) and DECR(BY) depending on optional increment value */
static int
redis_atomic_increment(INTERNAL_FUNCTION_PARAMETERS, int type,
                       RedisSock *redis_sock, char **cmd, int *cmd_len,
                       short *slot, void **ctx)
{
    char *key;
    size_t key_len;
    zend_long val = 1;

    if (zend_parse_parameters(ZEND_NUM_ARGS(), "s|l", &key, &key_len,
                              &val) == FAILURE)
    {
        return FAILURE;
    }

    /* If our value is 1 we use INCR/DECR.  For other values, treat the call as
     * an INCRBY or DECRBY call */
    if (type == TYPE_INCR) {
        if (val == 1) {
            *cmd_len = REDIS_CMD_SPPRINTF(cmd, "INCR", "k", key, key_len);
        } else {
            *cmd_len = REDIS_CMD_SPPRINTF(cmd, "INCRBY", "kl", key, key_len, val);
        }
    } else {
        if (val == 1) {
            *cmd_len = REDIS_CMD_SPPRINTF(cmd, "DECR", "k", key, key_len);
        } else {
            *cmd_len = REDIS_CMD_SPPRINTF(cmd, "DECRBY", "kl", key, key_len, val);
        }
    }

    /* Success */
    return SUCCESS;
}

/* INCR */
int redis_incr_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock,
                   char **cmd, int *cmd_len, short *slot, void **ctx)
{
    return redis_atomic_increment(INTERNAL_FUNCTION_PARAM_PASSTHRU,
        TYPE_INCR, redis_sock, cmd, cmd_len, slot, ctx);
}

/* DECR */
int redis_decr_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock,
                   char **cmd, int *cmd_len, short *slot, void **ctx)
{
    return redis_atomic_increment(INTERNAL_FUNCTION_PARAM_PASSTHRU,
        TYPE_DECR, redis_sock, cmd, cmd_len, slot, ctx);
}

/* HINCRBY */
int redis_hincrby_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock,
                      char **cmd, int *cmd_len, short *slot, void **ctx)
{
    char *key, *mem;
    size_t key_len, mem_len;
    zend_long byval;

    if (zend_parse_parameters(ZEND_NUM_ARGS(), "ssl", &key, &key_len,
                             &mem, &mem_len, &byval) == FAILURE)
    {
        return FAILURE;
    }

    // Construct command
    *cmd_len = REDIS_CMD_SPPRINTF(cmd, "HINCRBY", "ksl", key, key_len, mem, mem_len, byval);

    // Success
    return SUCCESS;
}

/* HINCRBYFLOAT */
int redis_hincrbyfloat_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock,
                           char **cmd, int *cmd_len, short *slot, void **ctx)
{
    char *key, *mem;
    size_t key_len, mem_len;
    double byval;

    if (zend_parse_parameters(ZEND_NUM_ARGS(), "ssd", &key, &key_len,
                             &mem, &mem_len, &byval) == FAILURE)
    {
        return FAILURE;
    }

    // Construct command
    *cmd_len = REDIS_CMD_SPPRINTF(cmd, "HINCRBYFLOAT", "ksf", key, key_len, mem,
                                 mem_len, byval);

    // Success
    return SUCCESS;
}

/* HMGET */
int redis_hmget_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock,
                    char **cmd, int *cmd_len, short *slot, void **ctx)
{
    zval *field = NULL, *zctx = NULL;
    smart_string cmdstr = {0};
    HashTable *fields = NULL;
    zend_string *key = NULL;
    zend_ulong valid = 0, i;

    ZEND_PARSE_PARAMETERS_START(2, 2)
        Z_PARAM_STR(key)
        Z_PARAM_ARRAY_HT(fields)
    ZEND_PARSE_PARAMETERS_END_EX(return FAILURE);

    if (zend_hash_num_elements(fields) == 0)
        return FAILURE;

    zctx = ecalloc(1 + zend_hash_num_elements(fields), sizeof(*zctx));

    ZEND_HASH_FOREACH_VAL(fields, field) {
        ZVAL_DEREF(field);
        if (!((Z_TYPE_P(field) == IS_STRING && Z_STRLEN_P(field) > 0) || Z_TYPE_P(field) == IS_LONG))
            continue;

        ZVAL_COPY(&zctx[valid++], field);
    } ZEND_HASH_FOREACH_END();

    if (valid == 0) {
        efree(zctx);
        return FAILURE;
    }

    ZVAL_NULL(&zctx[valid]);

    REDIS_CMD_INIT_SSTR_STATIC(&cmdstr, 1 + valid, "HMGET");
    redis_cmd_append_sstr_key_zstr(&cmdstr, key, redis_sock, slot);

    for (i = 0; i < valid; i++) {
        redis_cmd_append_sstr_zval(&cmdstr, &zctx[i], NULL);
    }

    // Push out command, length, and key context
    *cmd     = cmdstr.c;
    *cmd_len = cmdstr.len;
    *ctx     = zctx;

    return SUCCESS;
}

/* HMSET */
int redis_hmset_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock,
                    char **cmd, int *cmd_len, short *slot, void **ctx)
{
    smart_string cmdstr = {0};
    zend_string *key = NULL;
    HashTable *ht = NULL;
    uint32_t fields;
    zend_ulong idx;
    zval *zv;

    ZEND_PARSE_PARAMETERS_START(2, 2)
        Z_PARAM_STR(key)
        Z_PARAM_ARRAY_HT(ht)
    ZEND_PARSE_PARAMETERS_END_EX(return FAILURE);

    fields = zend_hash_num_elements(ht);
    if (fields == 0)
        return FAILURE;

    REDIS_CMD_INIT_SSTR_STATIC(&cmdstr, 1 + (2 * fields), "HMSET");
    redis_cmd_append_sstr_key_zstr(&cmdstr, key, redis_sock, slot);

    ZEND_HASH_FOREACH_KEY_VAL(ht, idx, key, zv) {
        if (key) {
            redis_cmd_append_sstr_zstr(&cmdstr, key);
        } else {
            redis_cmd_append_sstr_long(&cmdstr, idx);
        }
        redis_cmd_append_sstr_zval(&cmdstr, zv, redis_sock);
    } ZEND_HASH_FOREACH_END();

    *cmd_len = cmdstr.len;
    *cmd = cmdstr.c;

    return SUCCESS;
}

/* HSTRLEN */
int
redis_hstrlen_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock,
                  char **cmd, int *cmd_len, short *slot, void **ctx)
{
    char *key, *field;
    size_t key_len, field_len;

    if (zend_parse_parameters(ZEND_NUM_ARGS(), "ss", &key, &key_len,
                              &field, &field_len) == FAILURE
    ) {
        return FAILURE;
    }

    *cmd_len = REDIS_CMD_SPPRINTF(cmd, "HSTRLEN", "ks", key, key_len, field, field_len);

    return SUCCESS;
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
                dst->len = zval_is_true(zv);
            } else if (zend_string_equals_literal_ci(key, "IDX")) {
                dst->len = 0;
                dst->idx = zval_is_true(zv);
            } else if (zend_string_equals_literal_ci(key, "MINMATCHLEN")) {
                dst->minmatchlen = zval_get_long(zv);
            } else if (zend_string_equals_literal_ci(key, "WITHMATCHLEN")) {
                dst->withmatchlen = zval_is_true(zv);
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
int redis_lcs_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock,
                  char **cmd, int *cmd_len, short *slot, void **ctx)
{
    zend_string *key1 = NULL, *key2 = NULL;
    smart_string cmdstr = {0};
    HashTable *ht = NULL;
    redisLcsOptions opt;
    int argc;

    ZEND_PARSE_PARAMETERS_START(2, 3)
        Z_PARAM_STR(key1)
        Z_PARAM_STR(key2)
        Z_PARAM_OPTIONAL
        Z_PARAM_ARRAY_HT_OR_NULL(ht)
    ZEND_PARSE_PARAMETERS_END_EX(return FAILURE);

    key1 = redis_key_prefix_zstr(redis_sock, key1);
    key2 = redis_key_prefix_zstr(redis_sock, key2);

    if (slot) {
        *slot = cluster_hash_key_zstr(key1);
        if (*slot != cluster_hash_key_zstr(key2)) {
            php_error_docref(NULL, E_WARNING, "Warning, not all keys hash to the same slot!");
            zend_string_release(key1);
            zend_string_release(key2);
            return FAILURE;
        }
    }

    redis_get_lcs_options(&opt, ht);

    argc = 2 + !!opt.idx + !!opt.len + !!opt.withmatchlen + (opt.minmatchlen ? 2 : 0);
    REDIS_CMD_INIT_SSTR_STATIC(&cmdstr, argc, "LCS");

    redis_cmd_append_sstr_zstr(&cmdstr, key1);
    redis_cmd_append_sstr_zstr(&cmdstr, key2);

    REDIS_CMD_APPEND_SSTR_OPT_STATIC(&cmdstr, opt.idx, "IDX");
    REDIS_CMD_APPEND_SSTR_OPT_STATIC(&cmdstr, opt.len, "LEN");
    REDIS_CMD_APPEND_SSTR_OPT_STATIC(&cmdstr, opt.withmatchlen, "WITHMATCHLEN");

    if (opt.minmatchlen) {
        REDIS_CMD_APPEND_SSTR_STATIC(&cmdstr, "MINMATCHLEN");
        redis_cmd_append_sstr_long(&cmdstr, opt.minmatchlen);
    }

    zend_string_release(key1);
    zend_string_release(key2);

    *cmd = cmdstr.c;
    *cmd_len = cmdstr.len;
    return SUCCESS;
}

int redis_slowlog_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock,
                      char **cmd, int *cmd_len, short *slot, void **ctx)
{
    enum {SLOWLOG_GET, SLOWLOG_LEN, SLOWLOG_RESET} mode;
    smart_string cmdstr = {0};
    zend_string *op = NULL;
    zend_long arg = 0;

    ZEND_PARSE_PARAMETERS_START(1, 2)
        Z_PARAM_STR(op)
        Z_PARAM_OPTIONAL
        Z_PARAM_LONG(arg)
    ZEND_PARSE_PARAMETERS_END_EX(return FAILURE);

    if (zend_string_equals_literal_ci(op, "GET")) {
        mode = SLOWLOG_GET;
    } else if (zend_string_equals_literal_ci(op, "LEN")) {
        mode = SLOWLOG_LEN;
    } else if (zend_string_equals_literal_ci(op, "RESET")) {
        mode = SLOWLOG_RESET;
    } else {
        php_error_docref(NULL, E_WARNING, "Unknown SLOWLOG operation '%s'", ZSTR_VAL(op));
        return FAILURE;
    }

    REDIS_CMD_INIT_SSTR_STATIC(&cmdstr, 1 + (mode == SLOWLOG_GET && ZEND_NUM_ARGS() == 2), "SLOWLOG");
    redis_cmd_append_sstr_zstr(&cmdstr, op);

    if (mode == SLOWLOG_GET && ZEND_NUM_ARGS() == 2)
        redis_cmd_append_sstr_long(&cmdstr, arg);

    *cmd = cmdstr.c;
    *cmd_len = cmdstr.len;

    return SUCCESS;
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
int redis_restore_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock,
                      char **cmd, int *cmd_len, short *slot, void **ctx)
{
    zend_string *key, *value = NULL;
    smart_string cmdstr = {0};
    HashTable *options = NULL;
    redisRestoreOptions opt;
    zend_long timeout = 0;
    int argc;

    ZEND_PARSE_PARAMETERS_START(3, 4) {
        Z_PARAM_STR(key)
        Z_PARAM_LONG(timeout)
        Z_PARAM_STR(value)
        Z_PARAM_OPTIONAL
        Z_PARAM_ARRAY_HT_OR_NULL(options)
    } ZEND_PARSE_PARAMETERS_END_EX(return FAILURE);

    redis_get_restore_options(&opt, options);

    argc = 3 + (opt.idletime>-1?2:0) + (opt.freq>-1?2:0) + !!opt.absttl + !!opt.replace;
    REDIS_CMD_INIT_SSTR_STATIC(&cmdstr, argc, "RESTORE");

    redis_cmd_append_sstr_key(&cmdstr, ZSTR_VAL(key), ZSTR_LEN(key), redis_sock, slot);
    redis_cmd_append_sstr_long(&cmdstr, timeout);
    redis_cmd_append_sstr_zstr(&cmdstr, value);

    REDIS_CMD_APPEND_SSTR_OPT_STATIC(&cmdstr, opt.replace, "REPLACE");
    REDIS_CMD_APPEND_SSTR_OPT_STATIC(&cmdstr, opt.absttl, "ABSTTL");

    if (opt.idletime > -1) {
        REDIS_CMD_APPEND_SSTR_STATIC(&cmdstr, "IDLETIME");
        redis_cmd_append_sstr_long(&cmdstr, opt.idletime);
    }

    if (opt.freq > -1) {
        REDIS_CMD_APPEND_SSTR_STATIC(&cmdstr, "FREQ");
        redis_cmd_append_sstr_long(&cmdstr, opt.freq);
    }

    *cmd = cmdstr.c;
    *cmd_len = cmdstr.len;

    return SUCCESS;
}

/* BITPOS key bit [start [end [BYTE | BIT]]] */
int redis_bitpos_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock,
                     char **cmd, int *cmd_len, short *slot, void **ctx)
{
    zend_long start = 0, end = -1;
    zend_bool bit = 0, bybit = 0;
    smart_string cmdstr = {0};
    zend_string *key = NULL;

    ZEND_PARSE_PARAMETERS_START(2, 5)
        Z_PARAM_STR(key)
        Z_PARAM_BOOL(bit)
        Z_PARAM_OPTIONAL
        Z_PARAM_LONG(start)
        Z_PARAM_LONG(end)
        Z_PARAM_BOOL(bybit)
    ZEND_PARSE_PARAMETERS_END_EX(return FAILURE);

    REDIS_CMD_INIT_SSTR_STATIC(&cmdstr, 2 + (ZEND_NUM_ARGS() > 2 ? 2 : 0) + !!bybit, "BITPOS");

    redis_cmd_append_sstr_key_zstr(&cmdstr, key, redis_sock, slot);
    redis_cmd_append_sstr_long(&cmdstr, bit);

    /* Start and length if we were passed either */
    if (ZEND_NUM_ARGS() > 2) {
        redis_cmd_append_sstr_long(&cmdstr, start);
        redis_cmd_append_sstr_long(&cmdstr, end);
    }

    /* Finally, BIT or BYTE if we were passed that argument */
    REDIS_CMD_APPEND_SSTR_OPT_STATIC(&cmdstr, !!bybit, "BIT");

    *cmd = cmdstr.c;
    *cmd_len = cmdstr.len;

    return SUCCESS;
}

/* BITOP */
int redis_bitop_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock,
                    char **cmd, int *cmd_len, short *slot, void **ctx)
{
    zval *z_args;
    int i, argc = ZEND_NUM_ARGS();
    smart_string cmdstr = {0};
    short s2;

    // Allocate space for args, parse them as an array
    z_args = emalloc(argc * sizeof(zval));
    if (zend_get_parameters_array(ht, argc, z_args) == FAILURE ||
       argc < 3 || Z_TYPE(z_args[0]) != IS_STRING)
    {
        efree(z_args);
        return FAILURE;
    }

    // If we were passed a slot pointer, init to a sentinel value
    if (slot) *slot = -1;

    // Initialize command construction, add our operation argument
    redis_cmd_init_sstr(&cmdstr, argc, ZEND_STRL("BITOP"));
    redis_cmd_append_sstr(&cmdstr, Z_STRVAL(z_args[0]), Z_STRLEN(z_args[0]));

    // Now iterate over our keys argument
    for (i = 1; i < argc; i++) {
        // Append the key
        redis_cmd_append_sstr_key_zval(&cmdstr, &z_args[i], redis_sock, slot ? &s2 : NULL);

        // Verify slot if this is a Cluster request
        if (slot) {
            if (*slot != -1 && s2 != *slot) {
                php_error_docref(NULL, E_WARNING, "Warning, not all keys hash to the same slot!");
                efree(z_args);
                efree(cmdstr.c);
                return FAILURE;
            }
            *slot = s2;
        }
    }

    // Free our argument array
    efree(z_args);

    // Push out variables
    *cmd = cmdstr.c;
    *cmd_len = cmdstr.len;

    return SUCCESS;
}

/* BITCOUNT */
int redis_bitcount_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock,
                     char **cmd, int *cmd_len, short *slot, void **ctx)
{
    char *key;
    size_t key_len;
    zend_long start = 0, end = -1;
    zend_bool isbit = 0;

    if (zend_parse_parameters(ZEND_NUM_ARGS(), "s|llb", &key, &key_len,
                             &start, &end, &isbit) == FAILURE)
    {
        return FAILURE;
    }

    if (isbit) {
        *cmd_len = REDIS_CMD_SPPRINTF(cmd, "BITCOUNT", "kdds", key, key_len,
                                     (int)start, (int)end, "BIT", 3);
    } else {
        *cmd_len = REDIS_CMD_SPPRINTF(cmd, "BITCOUNT", "kdd", key, key_len,
                                     (int)start, (int)end);
    }

    return SUCCESS;
}

/* PFADD and PFMERGE are the same except that in one case we serialize,
 * and in the other case we key prefix */
static int redis_gen_pf_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock,
                            char *kw, int kw_len, int is_keys, char **cmd,
                            int *cmd_len, short *slot)
{
    smart_string cmdstr = {0};
    zend_string *key = NULL;
    HashTable *ht = NULL;
    zval *z_ele;
    int argc=1;
    short s2;

    // Parse arguments
    ZEND_PARSE_PARAMETERS_START(2, 2)
        Z_PARAM_STR(key)
        Z_PARAM_ARRAY_HT(ht)
    ZEND_PARSE_PARAMETERS_END_EX(return FAILURE);

    argc += zend_hash_num_elements(ht);

    // We need at least two arguments
    if (argc < 2) {
        return FAILURE;
    }

    redis_cmd_init_sstr(&cmdstr, argc, kw, kw_len);
    redis_cmd_append_sstr_key_zstr(&cmdstr, key, redis_sock, slot);

    // Append our array of keys or serialized values */
    ZEND_HASH_FOREACH_VAL(ht, z_ele) {
        if (is_keys) {
            redis_cmd_append_sstr_key_zval(&cmdstr, z_ele, redis_sock, slot ? &s2 : NULL);
            if (slot && *slot != s2) {
                php_error_docref(0, E_WARNING, "All keys must hash to the same slot!");
                return FAILURE;
            }
        } else {
            redis_cmd_append_sstr_zval(&cmdstr, z_ele, redis_sock);
        }
    } ZEND_HASH_FOREACH_END();

    // Push output arguments
    *cmd = cmdstr.c;
    *cmd_len = cmdstr.len;

    return SUCCESS;
}

/* PFADD */
int redis_pfadd_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock,
                    char **cmd, int *cmd_len, short *slot, void **ctx)
{
    return redis_gen_pf_cmd(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock,
        ZEND_STRL("PFADD"), 0, cmd, cmd_len, slot);
}

/* PFMERGE */
int redis_pfmerge_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock,
                      char **cmd, int *cmd_len, short *slot, void **ctx)
{
    return redis_gen_pf_cmd(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock,
        ZEND_STRL("PFMERGE"), 1, cmd, cmd_len, slot);
}

/* PFCOUNT */
int redis_pfcount_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock,
                      char **cmd, int *cmd_len, short *slot, void **ctx)
{
    smart_string cmdstr = {0};
    zval *zarg = NULL, *zv;
    short slot2 = -1;
    uint32_t keys;

    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_ZVAL(zarg)
    ZEND_PARSE_PARAMETERS_END_EX(return FAILURE);

    if (Z_TYPE_P(zarg) == IS_STRING) {
        REDIS_CMD_INIT_SSTR_STATIC(&cmdstr, 1, "PFCOUNT");
        redis_cmd_append_sstr_key_zstr(&cmdstr, Z_STR_P(zarg), redis_sock, slot);
    } else if (Z_TYPE_P(zarg) == IS_ARRAY) {
        keys = zend_hash_num_elements(Z_ARRVAL_P(zarg));
        if (keys == 0)
            return FAILURE;

        REDIS_CMD_INIT_SSTR_STATIC(&cmdstr, keys, "PFCOUNT");

        ZEND_HASH_FOREACH_VAL(Z_ARRVAL_P(zarg), zv) {
            redis_cmd_append_sstr_key_zval(&cmdstr, zv, redis_sock, slot);
            if (slot) {
                if (slot2 != -1 && slot2 != *slot)
                    goto cross_slot;
                slot2 = *slot;
            }
        } ZEND_HASH_FOREACH_END();
    } else {
        php_error_docref(NULL, E_WARNING, "Argument must be either an array or a string");
        return FAILURE;
    }

    *cmd = cmdstr.c;
    *cmd_len = cmdstr.len;
    return SUCCESS;

cross_slot:
    php_error_docref(NULL, E_WARNING, "Not all keys hash to the same slot!");
    efree(cmdstr.c);
    return FAILURE;
}

int redis_auth_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock,
                   char **cmd, int *cmd_len, short *slot, void **ctx)
{
    zend_string *user = NULL, *pass = NULL;
    zval *ztest;

    if (zend_parse_parameters(ZEND_NUM_ARGS(), "z!", &ztest) == FAILURE ||
        redis_extract_auth_info(ztest, &user, &pass) == FAILURE)
    {
        return FAILURE;
    }

    /* Construct either AUTH <user> <pass> or AUTH <pass> */
    if (user && pass) {
        *cmd_len = REDIS_CMD_SPPRINTF(cmd, "AUTH", "SS", user, pass);
    } else {
        *cmd_len = REDIS_CMD_SPPRINTF(cmd, "AUTH", "S", pass);
    }

    redis_sock_set_auth(redis_sock, user, pass);

    if (user) zend_string_release(user);
    if (pass) zend_string_release(pass);

    return SUCCESS;
}

/* SETBIT */
int redis_setbit_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock,
                     char **cmd, int *cmd_len, short *slot, void **ctx)
{
    char *key;
    size_t key_len;
    zend_long offset;
    zend_bool val;

    if (zend_parse_parameters(ZEND_NUM_ARGS(), "slb", &key, &key_len,
                             &offset, &val) == FAILURE)
    {
        return FAILURE;
    }

    // Validate our offset
    if (offset < BITOP_MIN_OFFSET || offset > BITOP_MAX_OFFSET) {
        php_error_docref(0, E_WARNING,
            "Invalid OFFSET for bitop command (must be between 0-2^32-1)");
        return FAILURE;
    }

    *cmd_len = REDIS_CMD_SPPRINTF(cmd, "SETBIT", "kld", key, key_len, offset, (int)val);

    return SUCCESS;
}

int redis_lmove_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock,
                    char *kw, char **cmd, int *cmd_len, short *slot, void **ctx)
{
    zend_string *src = NULL, *dst = NULL, *from = NULL, *to = NULL;
    smart_string cmdstr = {0};
    double timeout = 0.0;
    short slot2 = 0;
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
    ZEND_PARSE_PARAMETERS_END_EX(return FAILURE);

    if (!zend_string_equals_literal_ci(from, "LEFT") && !zend_string_equals_literal_ci(from, "RIGHT")) {
        php_error_docref(NULL, E_WARNING, "Wherefrom argument must be 'LEFT' or 'RIGHT'");
        return FAILURE;
    } else if (!zend_string_equals_literal_ci(to, "LEFT") && !zend_string_equals_literal_ci(to, "RIGHT")) {
        php_error_docref(NULL, E_WARNING, "Whereto argument must be 'LEFT' or 'RIGHT'");
        return FAILURE;
    }

    redis_cmd_init_sstr(&cmdstr, 4 + !!blocking, kw, strlen(kw));
    redis_cmd_append_sstr_key_zstr(&cmdstr, src, redis_sock, slot);
    redis_cmd_append_sstr_key_zstr(&cmdstr, dst, redis_sock, slot ? &slot2 : NULL);

    /* Protect the user from CROSSLOT errors */
    if (slot && slot2 != *slot) {
        php_error_docref(NULL, E_WARNING, "Both keys must hash to the same slot!");
        efree(cmdstr.c);
        return FAILURE;
    }

    redis_cmd_append_sstr_zstr(&cmdstr, from);
    redis_cmd_append_sstr_zstr(&cmdstr, to);
    if (blocking) redis_cmd_append_sstr_dbl(&cmdstr, timeout);

    *cmd = cmdstr.c;
    *cmd_len = cmdstr.len;

    return SUCCESS;
}

/* LINSERT */
int redis_linsert_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock,
                      char **cmd, int *cmd_len, short *slot, void **ctx)
{
    char *key, *pos;
    size_t key_len, pos_len;
    zval *z_val, *z_pivot;

    if (zend_parse_parameters(ZEND_NUM_ARGS(), "sszz", &key, &key_len,
                             &pos, &pos_len, &z_pivot, &z_val) == FAILURE)
    {
        return FAILURE;
    }

    // Validate position
    if (strcasecmp(pos, "after") && strcasecmp(pos, "before")) {
        php_error_docref(NULL, E_WARNING,
            "Position must be either 'BEFORE' or 'AFTER'");
        return FAILURE;
    }

    /* Construct command */
    *cmd_len = REDIS_CMD_SPPRINTF(cmd, "LINSERT", "ksvv", key, key_len, pos,
                                 pos_len, z_pivot, z_val);

    // Success
    return SUCCESS;
}

/* LREM */
int redis_lrem_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock,
                   char **cmd, int *cmd_len, short *slot, void **ctx)
{
    char *key;
    size_t key_len;
    zend_long count = 0;
    zval *z_val;

    if (zend_parse_parameters(ZEND_NUM_ARGS(), "sz|l", &key, &key_len,
                             &z_val, &count) == FAILURE)
    {
        return FAILURE;
    }

    /* Construct command */
    *cmd_len = REDIS_CMD_SPPRINTF(cmd, "LREM", "kdv", key, key_len, count, z_val);

    // Success!
    return SUCCESS;
}

int
redis_lpos_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock,
               char **cmd, int *cmd_len, short *slot, void **ctx)
{
    char *key;
    int argc = 2;
    size_t key_len;
    smart_string cmdstr = {0};
    zend_bool withrank = 0;
    zend_long rank = 0, count = -1, maxlen = -1;
    zend_string *zkey;
    zval *z_val, *z_ele, *z_opts = NULL;

    if (zend_parse_parameters(ZEND_NUM_ARGS(), "sz|a",
                              &key, &key_len, &z_val, &z_opts) == FAILURE)
    {
        return FAILURE;
    }

    if (z_opts != NULL) {
        ZEND_HASH_FOREACH_STR_KEY_VAL(Z_ARRVAL_P(z_opts), zkey, z_ele) {
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

    argc += (withrank ? 2 : 0) + (count >= 0 ? 2 : 0) + (maxlen >= 0 ? 2 : 0);
    REDIS_CMD_INIT_SSTR_STATIC(&cmdstr, argc, "LPOS");

    redis_cmd_append_sstr_key(&cmdstr, key, key_len, redis_sock, slot);
    redis_cmd_append_sstr_zval(&cmdstr, z_val, redis_sock);

    if (withrank) {
        REDIS_CMD_APPEND_SSTR_STATIC(&cmdstr, "RANK");
        redis_cmd_append_sstr_long(&cmdstr, rank);
    }

    if (count >= 0) {
        REDIS_CMD_APPEND_SSTR_STATIC(&cmdstr, "COUNT");
        redis_cmd_append_sstr_long(&cmdstr, count);
        *ctx = PHPREDIS_CTX_PTR;
    }

    if (maxlen >= 0) {
        REDIS_CMD_APPEND_SSTR_STATIC(&cmdstr, "MAXLEN");
        redis_cmd_append_sstr_long(&cmdstr, maxlen);
    }

    *cmd = cmdstr.c;
    *cmd_len = cmdstr.len;

    return SUCCESS;
}

int redis_smove_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock,
                    char **cmd, int *cmd_len, short *slot, void **ctx)
{
    zend_string *src = NULL, *dst = NULL;
    smart_string cmdstr = {0};
    zval *zv = NULL;
    short slot2;

    ZEND_PARSE_PARAMETERS_START(3, 3) {
        Z_PARAM_STR(src)
        Z_PARAM_STR(dst)
        Z_PARAM_ZVAL(zv)
    } ZEND_PARSE_PARAMETERS_END_EX(return FAILURE);

    REDIS_CMD_INIT_SSTR_STATIC(&cmdstr, 3, "SMOVE");
    redis_cmd_append_sstr_key_zstr(&cmdstr, src, redis_sock, slot);
    redis_cmd_append_sstr_key_zstr(&cmdstr, dst, redis_sock, slot ? &slot2 : NULL);
    redis_cmd_append_sstr_zval(&cmdstr, zv, redis_sock);

    if (slot && *slot != slot2) {
        php_error_docref(0, E_WARNING, "Source and destination keys don't hash to the same slot!");
        efree(cmdstr.c);
        return FAILURE;
    }

    *cmd = cmdstr.c;
    *cmd_len = cmdstr.len;

    return SUCCESS;
}

/* HSET */
int redis_hset_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock,
                   char **cmd, int *cmd_len, short *slot, void **ctx)
{
    int i, argc;
    smart_string cmdstr = {0};
    zend_string *zkey;
    zval *z_args, *z_ele;

    if ((argc = ZEND_NUM_ARGS()) < 2) {
        return FAILURE;
    }

    z_args = ecalloc(argc, sizeof(*z_args));
    if (zend_get_parameters_array(ht, argc, z_args) == FAILURE) {
        efree(z_args);
        return FAILURE;
    }

    if (argc == 2) {
        if (Z_TYPE(z_args[1]) != IS_ARRAY || zend_hash_num_elements(Z_ARRVAL(z_args[1])) == 0) {
            efree(z_args);
            return FAILURE;
        }

        /* Initialize our command */
        redis_cmd_init_sstr(&cmdstr, 1 + zend_hash_num_elements(Z_ARRVAL(z_args[1])), ZEND_STRL("HSET"));

        /* Append key */
        zkey = zval_get_string(&z_args[0]);
        redis_cmd_append_sstr_key(&cmdstr, ZSTR_VAL(zkey), ZSTR_LEN(zkey), redis_sock, slot);
        zend_string_release(zkey);

        ZEND_HASH_FOREACH_STR_KEY_VAL(Z_ARRVAL(z_args[1]), zkey, z_ele) {
            if (zkey != NULL) {
                ZVAL_DEREF(z_ele);
                redis_cmd_append_sstr(&cmdstr, ZSTR_VAL(zkey), ZSTR_LEN(zkey));
                redis_cmd_append_sstr_zval(&cmdstr, z_ele, redis_sock);
            }
        } ZEND_HASH_FOREACH_END();
    } else {
        if (argc % 2 == 0) {
            efree(z_args);
            return FAILURE;
        }
        /* Initialize our command */
        redis_cmd_init_sstr(&cmdstr, argc, ZEND_STRL("HSET"));

        /* Append key */
        zkey = zval_get_string(&z_args[0]);
        redis_cmd_append_sstr_key(&cmdstr, ZSTR_VAL(zkey), ZSTR_LEN(zkey), redis_sock, slot);
        zend_string_release(zkey);

        for (i = 1; i < argc; ++i) {
            if (i % 2) {
                zkey = zval_get_string(&z_args[i]);
                redis_cmd_append_sstr(&cmdstr, ZSTR_VAL(zkey), ZSTR_LEN(zkey));
                zend_string_release(zkey);
            } else {
                redis_cmd_append_sstr_zval(&cmdstr, &z_args[i], redis_sock);
            }
        }
    }

    // Push out values
    *cmd = cmdstr.c;
    *cmd_len = cmdstr.len;

    // Cleanup arg array
    efree(z_args);

    return SUCCESS;
}

/* HSETNX */
int redis_hsetnx_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock,
                     char **cmd, int *cmd_len, short *slot, void **ctx)
{
    char *key, *mem;
    size_t key_len, mem_len;
    zval *z_val;

    if (zend_parse_parameters(ZEND_NUM_ARGS(), "ssz", &key, &key_len,
                             &mem, &mem_len, &z_val) == FAILURE)
    {
        return FAILURE;
    }

    /* Construct command */
    *cmd_len = REDIS_CMD_SPPRINTF(cmd, "HSETNX", "ksv", key, key_len, mem, mem_len, z_val);

    // Success
    return SUCCESS;
}

int
redis_hrandfield_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock,
                      char **cmd, int *cmd_len, short *slot, void **ctx)
{
    char *key;
    int count = 0;
    size_t key_len;
    smart_string cmdstr = {0};
    zend_bool withvalues = 0;
    zval *z_opts = NULL, *z_ele;
    zend_string *zkey;

    if (zend_parse_parameters(ZEND_NUM_ARGS(), "s|a",
                              &key, &key_len, &z_opts) == FAILURE)
    {
        return FAILURE;
    }

    if (z_opts != NULL) {
        ZEND_HASH_FOREACH_STR_KEY_VAL(Z_ARRVAL_P(z_opts), zkey, z_ele) {
            if (zkey != NULL) {
                ZVAL_DEREF(z_ele);
                if (zend_string_equals_literal_ci(zkey, "count")) {
                    count = zval_get_long(z_ele);
                } else if (zend_string_equals_literal_ci(zkey, "withvalues")) {
                    withvalues = zval_is_true(z_ele);
                }
            }
        } ZEND_HASH_FOREACH_END();
    }

    REDIS_CMD_INIT_SSTR_STATIC(&cmdstr, 1 + (count != 0) + withvalues, "HRANDFIELD");
    redis_cmd_append_sstr_key(&cmdstr, key, key_len, redis_sock, slot);

    if (count != 0) {
        redis_cmd_append_sstr_long(&cmdstr, count);
        *ctx = PHPREDIS_CTX_PTR;
    }

    if (withvalues) {
        REDIS_CMD_APPEND_SSTR_STATIC(&cmdstr, "WITHVALUES");
        *ctx = PHPREDIS_CTX_PTR + 1;
    }


    *cmd = cmdstr.c;
    *cmd_len = cmdstr.len;
    return SUCCESS;
}

int redis_select_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock,
                     char **cmd, int *cmd_len, short *slot, void **ctx)
{
    zend_long db = 0;

    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_LONG(db)
    ZEND_PARSE_PARAMETERS_END_EX(return FAILURE);

    if (db < 0 || db > INT_MAX)
        return FAILURE;

    *ctx = (void*)(uintptr_t)db;
    *cmd_len = REDIS_CMD_SPPRINTF(cmd, "SELECT", "d", db);

    return SUCCESS;
}

/* SRANDMEMBER */
int redis_srandmember_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock,
                          char **cmd, int *cmd_len, short *slot, void **ctx)
{
    uint32_t argc = ZEND_NUM_ARGS();
    smart_string cmdstr = {0};
    zend_string *key = NULL;
    zend_long count = 0;

    ZEND_PARSE_PARAMETERS_START(1, 2)
        Z_PARAM_STR(key)
        Z_PARAM_OPTIONAL
        Z_PARAM_LONG(count)
    ZEND_PARSE_PARAMETERS_END_EX(return FAILURE);

    REDIS_CMD_INIT_SSTR_STATIC(&cmdstr, ZEND_NUM_ARGS(), "SRANDMEMBER");
    redis_cmd_append_sstr_key_zstr(&cmdstr, key, redis_sock, slot);
    if (argc == 2)
        redis_cmd_append_sstr_long(&cmdstr, count);

    *cmd = cmdstr.c;
    *cmd_len = cmdstr.len;
    *ctx = argc == 2 ? PHPREDIS_CTX_PTR : NULL;

    return SUCCESS;
}

/* ZINCRBY */
int redis_zincrby_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock,
                      char **cmd, int *cmd_len, short *slot, void **ctx)
{
    char *key;
    size_t key_len;
    double incrby;
    zval *z_val;

    if (zend_parse_parameters(ZEND_NUM_ARGS(), "sdz", &key, &key_len,
                             &incrby, &z_val) == FAILURE)
    {
        return FAILURE;
    }

    *cmd_len = REDIS_CMD_SPPRINTF(cmd, "ZINCRBY", "kfv", key, key_len, incrby, z_val);

    return SUCCESS;
}

/* SORT */
int redis_sort_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock,
                   char *kw, char **cmd, int *cmd_len, short *slot, void **ctx)
{
    zval *z_opts=NULL, *z_ele, z_argv;
    char *key;
    HashTable *ht_opts;
    smart_string cmdstr = {0};
    size_t key_len;
    int key_free;

    if (zend_parse_parameters(ZEND_NUM_ARGS(), "s|a", &key, &key_len,
                             &z_opts) == FAILURE)
    {
        return FAILURE;
    }

    // If we don't have an options array, the command is quite simple
    if (!z_opts || zend_hash_num_elements(Z_ARRVAL_P(z_opts)) == 0) {
        // Construct command
        *cmd_len = REDIS_CMD_SPPRINTF(cmd, kw, "k", key, key_len);

        return SUCCESS;
    }

    // Create our hash table to hold our sort arguments
    array_init(&z_argv);

    // SORT <key>
    key_free = redis_key_prefix(redis_sock, &key, &key_len);
    add_next_index_stringl(&z_argv, key, key_len);
    if (key_free) efree(key);

    // Set slot
    CMD_SET_SLOT(slot,key,key_len);

    // Grab the hash table
    ht_opts = Z_ARRVAL_P(z_opts);

    // Handle BY pattern
    if (((z_ele = zend_hash_str_find(ht_opts, "by", sizeof("by") - 1)) != NULL ||
         (z_ele = zend_hash_str_find(ht_opts, "BY", sizeof("BY") - 1)) != NULL
        ) && Z_TYPE_P(z_ele) == IS_STRING
    ) {
        // "BY" option is disabled in cluster
        if (slot) {
            php_error_docref(NULL, E_WARNING,
                "SORT BY option is not allowed in Redis Cluster");
            zval_dtor(&z_argv);
            return FAILURE;
        }

        // ... BY <pattern>
        add_next_index_stringl(&z_argv, "BY", sizeof("BY") - 1);
        add_next_index_stringl(&z_argv, Z_STRVAL_P(z_ele), Z_STRLEN_P(z_ele));
    }

    // Handle ASC/DESC option
    if (((z_ele = zend_hash_str_find(ht_opts, "sort", sizeof("sort") - 1)) != NULL ||
         (z_ele = zend_hash_str_find(ht_opts, "SORT", sizeof("SORT") - 1)) != NULL
        ) && Z_TYPE_P(z_ele) == IS_STRING
    ) {
        // 'asc'|'desc'
        add_next_index_stringl(&z_argv, Z_STRVAL_P(z_ele), Z_STRLEN_P(z_ele));
    }

    // STORE option
    if (((z_ele = zend_hash_str_find(ht_opts, "store", sizeof("store") - 1)) != NULL ||
         (z_ele = zend_hash_str_find(ht_opts, "STORE", sizeof("STORE") - 1)) != NULL
        ) && Z_TYPE_P(z_ele) == IS_STRING
    ) {
        // Slot verification
        int cross_slot = slot && *slot != cluster_hash_key(
            Z_STRVAL_P(z_ele), Z_STRLEN_P(z_ele));

        if (cross_slot) {
            php_error_docref(0, E_WARNING,
                "Error, SORT key and STORE key have different slots!");
            zval_dtor(&z_argv);
            return FAILURE;
        }

        // STORE <key>
        add_next_index_stringl(&z_argv, "STORE", sizeof("STORE") - 1);
        add_next_index_stringl(&z_argv, Z_STRVAL_P(z_ele), Z_STRLEN_P(z_ele));

        // We are using STORE
        *ctx = PHPREDIS_CTX_PTR;
    }

    // GET option
    if (((z_ele = zend_hash_str_find(ht_opts, "get", sizeof("get") - 1)) != NULL ||
         (z_ele = zend_hash_str_find(ht_opts, "GET", sizeof("GET") - 1)) != NULL
        ) && (Z_TYPE_P(z_ele) == IS_STRING || Z_TYPE_P(z_ele) == IS_ARRAY)
    ) {
        // Disabled in cluster
        if (slot) {
            php_error_docref(NULL, E_WARNING,
                "GET option for SORT disabled in Redis Cluster");
            zval_dtor(&z_argv);
            return FAILURE;
        }

        // If it's a string just add it
        if (Z_TYPE_P(z_ele) == IS_STRING) {
            add_next_index_stringl(&z_argv, "GET", sizeof("GET") - 1);
            add_next_index_stringl(&z_argv, Z_STRVAL_P(z_ele), Z_STRLEN_P(z_ele));
        } else {
            int added = 0;
            zval *z_key;

            ZEND_HASH_FOREACH_VAL(Z_ARRVAL_P(z_ele), z_key) {
                // If we can't get the data, or it's not a string, skip
                if (z_key == NULL || Z_TYPE_P(z_key) != IS_STRING) {
                    continue;
                }
                /* Add get per thing we're getting */
                add_next_index_stringl(&z_argv, "GET", sizeof("GET") - 1);

                // Add this key to our argv array
                add_next_index_stringl(&z_argv, Z_STRVAL_P(z_key), Z_STRLEN_P(z_key));
                added++;
            } ZEND_HASH_FOREACH_END();

            // Make sure we were able to add at least one
            if (added == 0) {
                php_error_docref(NULL, E_WARNING,
                    "Array of GET values requested, but none are valid");
                zval_dtor(&z_argv);
                return FAILURE;
            }
        }
    }

    // ALPHA
    if (((z_ele = zend_hash_str_find(ht_opts, "alpha", sizeof("alpha") - 1)) != NULL ||
         (z_ele = zend_hash_str_find(ht_opts, "ALPHA", sizeof("ALPHA") - 1)) != NULL) &&
         zval_is_true(z_ele)
    ) {
        add_next_index_stringl(&z_argv, "ALPHA", sizeof("ALPHA") - 1);
    }

    // LIMIT <offset> <count>
    if (((z_ele = zend_hash_str_find(ht_opts, "limit", sizeof("limit") - 1)) != NULL ||
         (z_ele = zend_hash_str_find(ht_opts, "LIMIT", sizeof("LIMIT") - 1)) != NULL
        ) && Z_TYPE_P(z_ele) == IS_ARRAY
    ) {
        HashTable *ht_off = Z_ARRVAL_P(z_ele);
        zval *z_off, *z_cnt;

        if ((z_off = zend_hash_index_find(ht_off, 0)) != NULL &&
            (z_cnt = zend_hash_index_find(ht_off, 1)) != NULL
        ) {
            if ((Z_TYPE_P(z_off) != IS_STRING && Z_TYPE_P(z_off) != IS_LONG) ||
                (Z_TYPE_P(z_cnt) != IS_STRING && Z_TYPE_P(z_cnt) != IS_LONG)
            ) {
                php_error_docref(NULL, E_WARNING,
                    "LIMIT options on SORT command must be longs or strings");
                zval_dtor(&z_argv);
                return FAILURE;
            }

            // Add LIMIT argument
            add_next_index_stringl(&z_argv, "LIMIT", sizeof("LIMIT") - 1);

            long low, high;
            if (Z_TYPE_P(z_off) == IS_STRING) {
                low = atol(Z_STRVAL_P(z_off));
            } else {
                low = Z_LVAL_P(z_off);
            }
            if (Z_TYPE_P(z_cnt) == IS_STRING) {
                high = atol(Z_STRVAL_P(z_cnt));
            } else {
                high = Z_LVAL_P(z_cnt);
            }

            // Add our two LIMIT arguments
            add_next_index_long(&z_argv, low);
            add_next_index_long(&z_argv, high);
        }
    }

    // Start constructing our command
    HashTable *ht_argv = Z_ARRVAL_P(&z_argv);
    redis_cmd_init_sstr(&cmdstr, zend_hash_num_elements(ht_argv), kw, strlen(kw));

    // Iterate through our arguments
    ZEND_HASH_FOREACH_VAL(ht_argv, z_ele) {
        // Args are strings or longs
        if (Z_TYPE_P(z_ele) == IS_STRING) {
            redis_cmd_append_sstr(&cmdstr,Z_STRVAL_P(z_ele),
                Z_STRLEN_P(z_ele));
        } else {
            redis_cmd_append_sstr_long(&cmdstr, Z_LVAL_P(z_ele));
        }
    } ZEND_HASH_FOREACH_END();

    /* Clean up our arguments array.  Note we don't have to free any prefixed
     * key as that we didn't duplicate the pointer if we prefixed */
    zval_dtor(&z_argv);

    // Push our length and command
    *cmd_len = cmdstr.len;
    *cmd     = cmdstr.c;

    // Success!
    return SUCCESS;
}

/* HDEL */
int redis_hdel_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock,
                   char **cmd, int *cmd_len, short *slot, void **ctx)
{
    zval *z_args;
    smart_string cmdstr = {0};
    char *arg;
    int arg_free, i;
    size_t arg_len;
    int argc = ZEND_NUM_ARGS();
    zend_string *zstr;

    // We need at least KEY and one member
    if (argc < 2) {
        return FAILURE;
    }

    // Grab arguments as an array
    z_args = emalloc(argc * sizeof(zval));
    if (zend_get_parameters_array(ht, argc, z_args) == FAILURE) {
        efree(z_args);
        return FAILURE;
    }

    // Get first argument (the key) as a string
    zstr = zval_get_string(&z_args[0]);
    arg = ZSTR_VAL(zstr);
    arg_len = ZSTR_LEN(zstr);

    // Prefix
    arg_free = redis_key_prefix(redis_sock, &arg, &arg_len);

    // Start command construction
    redis_cmd_init_sstr(&cmdstr, argc, ZEND_STRL("HDEL"));
    redis_cmd_append_sstr(&cmdstr, arg, arg_len);

    // Set our slot, free key if we prefixed it
    CMD_SET_SLOT(slot,arg,arg_len);
    zend_string_release(zstr);
    if (arg_free) efree(arg);

    // Iterate through the members we're removing
    for (i = 1; i < argc; i++) {
        zstr = zval_get_string(&z_args[i]);
        redis_cmd_append_sstr(&cmdstr, ZSTR_VAL(zstr), ZSTR_LEN(zstr));
        zend_string_release(zstr);
    }

    // Push out values
    *cmd     = cmdstr.c;
    *cmd_len = cmdstr.len;

    // Cleanup
    efree(z_args);

    // Success!
    return SUCCESS;
}

/* ZADD */
int redis_zadd_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock,
                   char **cmd, int *cmd_len, short *slot, void **ctx)
{
    zend_string *zstr, *key = NULL, *exp_type = NULL, *range_type = NULL;
    zend_bool ch = 0, incr = 0;
    smart_string cmdstr = {0};
    zval *argv = NULL, *z_opt;
    int argc = 0, pos = 0;

    ZEND_PARSE_PARAMETERS_START(3, -1)
        Z_PARAM_STR(key)
        Z_PARAM_VARIADIC('*', argv, argc)
    ZEND_PARSE_PARAMETERS_END_EX(return FAILURE);

    // Need key, [NX|XX] [LT|GT] [CH] [INCR] score, value, [score, value...] */
    if (argc % 2 != 0) {
        if (Z_TYPE(argv[0]) != IS_ARRAY) {
            return FAILURE;
        }

        ZEND_HASH_FOREACH_VAL(Z_ARRVAL(argv[0]), z_opt) {
            if (Z_TYPE_P(z_opt) == IS_STRING) {
                zstr = Z_STR_P(z_opt);
                if (zend_string_equals_literal_ci(zstr, "NX") || zend_string_equals_literal_ci(zstr, "XX")) {
                    exp_type = Z_STR_P(z_opt);
                } else if (zend_string_equals_literal_ci(zstr, "LT") || zend_string_equals_literal_ci(zstr, "GT")) {
                    range_type = Z_STR_P(z_opt);
                } else if (zend_string_equals_literal_ci(zstr, "CH")) {
                    ch = 1;
                } else if (zend_string_equals_literal_ci(zstr, "INCR")) {
                    if (argc != 3) {
                        // Only one score-element pair can be specified in this mode.
                        return FAILURE;
                    }
                    incr = 1;
                }
            }
        } ZEND_HASH_FOREACH_END();

        pos++;
    }

    // Start command construction
    REDIS_CMD_INIT_SSTR_STATIC(&cmdstr, 1 + (argc - pos) + !!exp_type + !!range_type + !!ch + !!incr, "ZADD");
    redis_cmd_append_sstr_key_zstr(&cmdstr, key, redis_sock, slot);

    if (exp_type) redis_cmd_append_sstr_zstr(&cmdstr, exp_type);
    if (range_type) redis_cmd_append_sstr_zstr(&cmdstr, range_type);
    REDIS_CMD_APPEND_SSTR_OPT_STATIC(&cmdstr, ch, "CH");
    REDIS_CMD_APPEND_SSTR_OPT_STATIC(&cmdstr, incr, "INCR");

    // Now the rest of our arguments
    while (pos < argc) {
        // Append score and member
        if (redis_cmd_append_sstr_score(&cmdstr, &argv[pos]) == FAILURE) {
            smart_string_free(&cmdstr);
            return FAILURE;
        }

        redis_cmd_append_sstr_zval(&cmdstr, &argv[pos+1], redis_sock);

        pos += 2;
    }

    // Push output values
    *cmd = cmdstr.c;
    *cmd_len = cmdstr.len;
    *ctx = incr ? PHPREDIS_CTX_PTR : NULL;

    return SUCCESS;
}

/* OBJECT */
int redis_object_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock,
                     char **cmd, int *cmd_len, short *slot, void **ctx)
{
    zend_string *subcmd = NULL, *key = NULL;
    smart_string cmdstr = {0};

    ZEND_PARSE_PARAMETERS_START(2, 2)
        Z_PARAM_STR(subcmd)
        Z_PARAM_STR(key)
    ZEND_PARSE_PARAMETERS_END_EX(return FAILURE);

    if (zend_string_equals_literal_ci(subcmd, "REFCOUNT") ||
        zend_string_equals_literal_ci(subcmd, "IDLETIME"))
    {
        *ctx = PHPREDIS_CTX_PTR;
    } else if (zend_string_equals_literal_ci(subcmd, "ENCODING")) {
        *ctx = PHPREDIS_CTX_PTR + 1;
    } else {
        php_error_docref(NULL, E_WARNING, "Invalid subcommand sent to OBJECT");
        return FAILURE;
    }

    REDIS_CMD_INIT_SSTR_STATIC(&cmdstr, 2, "OBJECT");
    redis_cmd_append_sstr_zstr(&cmdstr, subcmd);
    redis_cmd_append_sstr_key_zstr(&cmdstr, key, redis_sock, slot);

    *cmd = cmdstr.c;
    *cmd_len = cmdstr.len;
    return SUCCESS;
}

int
redis_geoadd_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock,
                 char **cmd, int *cmd_len, short *slot, void **ctx)
{
    zval *z_args, *z_ele;
    smart_string cmdstr = {0};
    zend_bool ch = 0;
    zend_string *zstr;
    char *mode = NULL;
    int argc, i;

    // We at least need a key and three values
    if ((argc = ZEND_NUM_ARGS()) < 4 || (argc % 3 != 1 && argc % 3 != 2)) {
        zend_wrong_param_count();
        return FAILURE;
    }

    // Make sure we at least have a key, and we can get other args
    z_args = ecalloc(argc, sizeof(*z_args));
    if (zend_get_parameters_array(ht, argc, z_args) == FAILURE) {
        efree(z_args);
        return FAILURE;
    }

    if (argc % 3 == 2) {
        argc--;
        if (Z_TYPE(z_args[argc]) != IS_ARRAY) {
            php_error_docref(NULL, E_WARNING, "Invalid options value");
            efree(z_args);
            return FAILURE;
        }
        ZEND_HASH_FOREACH_VAL(Z_ARRVAL(z_args[argc]), z_ele) {
            ZVAL_DEREF(z_ele);
            if (Z_TYPE_P(z_ele) == IS_STRING) {
                if (zend_string_equals_literal_ci(Z_STR_P(z_ele), "NX") ||
                    zend_string_equals_literal_ci(Z_STR_P(z_ele), "XX"))
                {
                    mode = Z_STRVAL_P(z_ele);
                } else if (zend_string_equals_literal_ci(Z_STR_P(z_ele), "CH")) {
                    ch = 1;
                }
            }
        } ZEND_HASH_FOREACH_END();
    }

    /* Initialize our command */
    REDIS_CMD_INIT_SSTR_STATIC(&cmdstr, argc + (mode != NULL) + ch, "GEOADD");

    /* Append key */
    zstr = zval_get_string(&z_args[0]);
    redis_cmd_append_sstr_key(&cmdstr, ZSTR_VAL(zstr), ZSTR_LEN(zstr), redis_sock, slot);
    zend_string_release(zstr);

    /* Append options */
    if (mode != NULL) {
        redis_cmd_append_sstr(&cmdstr, mode, strlen(mode));
    }
    REDIS_CMD_APPEND_SSTR_OPT_STATIC(&cmdstr, ch, "CH");

    /* Append members */
    for (i = 1; i < argc; ++i) {
        redis_cmd_append_sstr_zval(&cmdstr, &z_args[i], redis_sock);
    }

    // Cleanup arg array
    efree(z_args);

    // Push out values
    *cmd = cmdstr.c;
    *cmd_len = cmdstr.len;

    return SUCCESS;
}

/* GEODIST */
int redis_geodist_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock,
                      char **cmd, int *cmd_len, short *slot, void **ctx)
{
    char *key, *source, *dest, *unit = NULL;
    size_t keylen, sourcelen, destlen, unitlen;

    if (zend_parse_parameters(ZEND_NUM_ARGS(), "sss|s", &key, &keylen,
                              &source, &sourcelen, &dest, &destlen, &unit,
                              &unitlen) == FAILURE)
    {
        return FAILURE;
    }

    /* Construct command */
    if (unit != NULL) {
        *cmd_len = REDIS_CMD_SPPRINTF(cmd, "GEODIST", "ksss", key, keylen, source,
                                     sourcelen, dest, destlen, unit, unitlen);
    } else {
        *cmd_len = REDIS_CMD_SPPRINTF(cmd, "GEODIST", "kss", key, keylen, source,
                                     sourcelen, dest, destlen);
    }

    return SUCCESS;
}

geoStoreType get_georadius_store_type(zend_string *key) {
    if (ZSTR_LEN(key) == 5 && !strcasecmp(ZSTR_VAL(key), "store")) {
        return STORE_COORD;
    } else if (ZSTR_LEN(key) == 9 && !strcasecmp(ZSTR_VAL(key), "storedist")) {
        return STORE_DIST;
    }

    return STORE_NONE;
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
            opts->any = zval_is_true(z_tmp);
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
void append_georadius_opts(RedisSock *redis_sock, smart_string *str, short *slot,
                           geoOptions *opt)
{
    if (opt->withcoord)
        REDIS_CMD_APPEND_SSTR_STATIC(str, "WITHCOORD");
    if (opt->withdist)
        REDIS_CMD_APPEND_SSTR_STATIC(str, "WITHDIST");
    if (opt->withhash)
        REDIS_CMD_APPEND_SSTR_STATIC(str, "WITHHASH");

    /* Append sort if it's not GEO_NONE */
    if (opt->sort == SORT_ASC) {
        REDIS_CMD_APPEND_SSTR_STATIC(str, "ASC");
    } else if (opt->sort == SORT_DESC) {
        REDIS_CMD_APPEND_SSTR_STATIC(str, "DESC");
    }

    /* Append our count if we've got one */
    if (opt->count) {
        REDIS_CMD_APPEND_SSTR_STATIC(str, "COUNT");
        redis_cmd_append_sstr_long(str, opt->count);
        if (opt->any) {
            REDIS_CMD_APPEND_SSTR_STATIC(str, "ANY");
        }
    }

    /* Append store options if we've got them */
    if (opt->store != STORE_NONE && opt->key != NULL) {
        if (opt->store == STORE_COORD) {
            REDIS_CMD_APPEND_SSTR_STATIC(str, "STORE");
        } else {
            REDIS_CMD_APPEND_SSTR_STATIC(str, "STOREDIST");
        }

        redis_cmd_append_sstr_key_zstr(str, opt->key, redis_sock, slot);
    }
}

/* GEORADIUS / GEORADIUS_RO */
int redis_georadius_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock,
                        char *kw, char **cmd, int *cmd_len, short *slot,
                        void **ctx)
{
    zend_string *key = NULL, *unit = NULL;
    double lng = 0, lat = 0, radius = 0;
    smart_string cmdstr = {0};
    HashTable *opts = NULL;
    geoOptions gopts = {0};
    short store_slot = -1;
    uint32_t argc;

    ZEND_PARSE_PARAMETERS_START(5, 6)
        Z_PARAM_STR(key)
        Z_PARAM_DOUBLE(lng)
        Z_PARAM_DOUBLE(lat)
        Z_PARAM_DOUBLE(radius)
        Z_PARAM_STR(unit)
        Z_PARAM_OPTIONAL
        Z_PARAM_ARRAY_HT_OR_NULL(opts)
    ZEND_PARSE_PARAMETERS_END_EX(return FAILURE);

    /* Parse any GEORADIUS options we have */
    if (opts != NULL && get_georadius_opts(opts, &gopts) != SUCCESS)
        return FAILURE;

    /* Increment argc depending on options */
    argc = 5 + gopts.withcoord + gopts.withdist + gopts.withhash +
               (gopts.sort != SORT_NONE) + (gopts.count ? 2 + gopts.any : 0) +
               (gopts.store != STORE_NONE ? 2 : 0);

    /* Begin construction of our command */
    redis_cmd_init_sstr(&cmdstr, argc, kw, strlen(kw));
    redis_cmd_append_sstr_key_zstr(&cmdstr, key, redis_sock, slot);

    /* Append required arguments */
    redis_cmd_append_sstr_dbl(&cmdstr, lng);
    redis_cmd_append_sstr_dbl(&cmdstr, lat);
    redis_cmd_append_sstr_dbl(&cmdstr, radius);
    redis_cmd_append_sstr_zstr(&cmdstr, unit);

    /* Append optional arguments */
    append_georadius_opts(redis_sock, &cmdstr, slot ? &store_slot : NULL, &gopts);

    /* Free key if it was prefixed */
    if (gopts.key) zend_string_release(gopts.key);

    /* Protect the user from CROSSSLOT if we're in cluster */
    if (slot && gopts.store != STORE_NONE && *slot != store_slot) {
        php_error_docref(NULL, E_WARNING,
            "Key and STORE[DIST] key must hash to the same slot");
        efree(cmdstr.c);
        return FAILURE;
    }

    /* Set slot, command and len, and return */
    *cmd = cmdstr.c;
    *cmd_len = cmdstr.len;

    return SUCCESS;
}

/* GEORADIUSBYMEMBER/GEORADIUSBYMEMBER_RO
 *    key member radius m|km|ft|mi [WITHCOORD] [WITHDIST] [WITHHASH] [COUNT count] */
int redis_georadiusbymember_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock,
                                char *kw, char **cmd, int *cmd_len, short *slot,
                                void **ctx)
{
    char *key, *mem, *unit;
    size_t keylen, memlen, unitlen;
    short store_slot = 0;
    int keyfree, argc = 4;
    double radius;
    geoOptions gopts = {0};
    zval *opts = NULL;
    smart_string cmdstr = {0};

    if (zend_parse_parameters(ZEND_NUM_ARGS(), "ssds|a", &key, &keylen,
                              &mem, &memlen, &radius, &unit, &unitlen, &opts) == FAILURE)
    {
        return FAILURE;
    }

    if (opts != NULL) {
        /* Attempt to parse our options array */
        if (get_georadius_opts(Z_ARRVAL_P(opts), &gopts) == FAILURE) {
            return FAILURE;
        }
    }

    /* Increment argc based on options */
    argc += gopts.withcoord + gopts.withdist + gopts.withhash +
            (gopts.sort != SORT_NONE) + (gopts.count ? 2 + gopts.any : 0) +
            (gopts.store != STORE_NONE ? 2 : 0);

    /* Begin command construction*/
    redis_cmd_init_sstr(&cmdstr, argc, kw, strlen(kw));

    /* Prefix our key if we're prefixing and set the slot */
    keyfree = redis_key_prefix(redis_sock, &key, &keylen);
    CMD_SET_SLOT(slot, key, keylen);

    /* Append required arguments */
    redis_cmd_append_sstr(&cmdstr, key, keylen);
    redis_cmd_append_sstr(&cmdstr, mem, memlen);
    redis_cmd_append_sstr_long(&cmdstr, radius);
    redis_cmd_append_sstr(&cmdstr, unit, unitlen);

    /* Append options */
    append_georadius_opts(redis_sock, &cmdstr, slot ? &store_slot : NULL, &gopts);

    /* Free key if we prefixed */
    if (keyfree) efree(key);
    if (gopts.key) zend_string_release(gopts.key);

    /* Protect the user from CROSSSLOT if we're in cluster */
    if (slot && gopts.store != STORE_NONE && *slot != store_slot) {
        php_error_docref(NULL, E_WARNING,
            "Key and STORE[DIST] key must hash to the same slot");
        efree(cmdstr.c);
        return FAILURE;
    }

    *cmd = cmdstr.c;
    *cmd_len = cmdstr.len;

    return SUCCESS;
}

int
redis_geosearch_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock,
                    char **cmd, int *cmd_len, short *slot, void **ctx)
{
    char *key, *unit;
    int argc = 2;
    size_t keylen, unitlen;
    geoOptions gopts = {0};
    smart_string cmdstr = {0};
    zval *position, *shape, *opts = NULL, *z_ele;
    zend_string *zkey, *zstr;

    if (zend_parse_parameters(ZEND_NUM_ARGS(), "szzs|a",
                              &key, &keylen, &position, &shape,
                              &unit, &unitlen, &opts) == FAILURE)
    {
        return FAILURE;
    }

    if (Z_TYPE_P(position) == IS_STRING && Z_STRLEN_P(position) > 0) {
        argc += 2;
    } else if (Z_TYPE_P(position) == IS_ARRAY && zend_hash_num_elements(Z_ARRVAL_P(position)) == 2) {
        argc += 3;
    } else {
        php_error_docref(NULL, E_WARNING, "Invalid position");
        return FAILURE;
    }

    if (Z_TYPE_P(shape) == IS_LONG || Z_TYPE_P(shape) == IS_DOUBLE) {
        argc += 2;
    } else if (Z_TYPE_P(shape) == IS_ARRAY) {
        argc += 3;
    } else {
        php_error_docref(NULL, E_WARNING, "Invalid shape dimensions");
        return FAILURE;
    }

    /* Attempt to parse our options array */
    if (opts != NULL) {
        ZEND_HASH_FOREACH_STR_KEY_VAL(Z_ARRVAL_P(opts), zkey, z_ele) {
            ZVAL_DEREF(z_ele);
            if (zkey != NULL && zend_string_equals_literal_ci(zkey, "COUNT")) {
                if (get_georadius_count_options(z_ele, &gopts) == FAILURE) {
                    return FAILURE;
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

    /* Increment argc based on options */
    argc += gopts.withcoord + gopts.withdist + gopts.withhash
         + (gopts.sort != SORT_NONE) + (gopts.count ? 2 + gopts.any : 0);

    REDIS_CMD_INIT_SSTR_STATIC(&cmdstr, argc, "GEOSEARCH");
    redis_cmd_append_sstr_key(&cmdstr, key, keylen, redis_sock, slot);

    if (Z_TYPE_P(position) == IS_ARRAY) {
        REDIS_CMD_APPEND_SSTR_STATIC(&cmdstr, "FROMLONLAT");
        ZEND_HASH_FOREACH_VAL(Z_ARRVAL_P(position), z_ele) {
            ZVAL_DEREF(z_ele);
            redis_cmd_append_sstr_dbl(&cmdstr, zval_get_double(z_ele));
        } ZEND_HASH_FOREACH_END();
    } else {
        REDIS_CMD_APPEND_SSTR_STATIC(&cmdstr, "FROMMEMBER");
        redis_cmd_append_sstr(&cmdstr, Z_STRVAL_P(position), Z_STRLEN_P(position));
    }

    if (Z_TYPE_P(shape) == IS_ARRAY) {
        REDIS_CMD_APPEND_SSTR_STATIC(&cmdstr, "BYBOX");
        ZEND_HASH_FOREACH_VAL(Z_ARRVAL_P(shape), z_ele) {
            ZVAL_DEREF(z_ele);
            redis_cmd_append_sstr_dbl(&cmdstr, zval_get_double(z_ele));
        } ZEND_HASH_FOREACH_END();
    } else {
        REDIS_CMD_APPEND_SSTR_STATIC(&cmdstr, "BYRADIUS");
        redis_cmd_append_sstr_dbl(&cmdstr, zval_get_double(shape));
    }
    redis_cmd_append_sstr(&cmdstr, unit, unitlen);

    /* Append optional arguments */
    if (gopts.withcoord) REDIS_CMD_APPEND_SSTR_STATIC(&cmdstr, "WITHCOORD");
    if (gopts.withdist) REDIS_CMD_APPEND_SSTR_STATIC(&cmdstr, "WITHDIST");
    if (gopts.withhash) REDIS_CMD_APPEND_SSTR_STATIC(&cmdstr, "WITHHASH");

    /* Append sort if it's not GEO_NONE */
    if (gopts.sort == SORT_ASC) {
        REDIS_CMD_APPEND_SSTR_STATIC(&cmdstr, "ASC");
    } else if (gopts.sort == SORT_DESC) {
        REDIS_CMD_APPEND_SSTR_STATIC(&cmdstr, "DESC");
    }

    /* Append our count if we've got one */
    if (gopts.count) {
        REDIS_CMD_APPEND_SSTR_STATIC(&cmdstr, "COUNT");
        redis_cmd_append_sstr_long(&cmdstr, gopts.count);
        if (gopts.any) {
            REDIS_CMD_APPEND_SSTR_STATIC(&cmdstr, "ANY");
        }
    }

    if ((argc = gopts.withcoord + gopts.withdist + gopts.withhash) > 0) {
        *ctx = PHPREDIS_CTX_PTR;
    }

    *cmd = cmdstr.c;
    *cmd_len = cmdstr.len;

    return SUCCESS;
}

int
redis_geosearchstore_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock,
                         char **cmd, int *cmd_len, short *slot, void **ctx)
{
    int argc = 3;
    char *dest, *src, *unit;
    size_t destlen, srclen, unitlen;
    geoOptions gopts = {0};
    smart_string cmdstr = {0};
    zval *position, *shape, *opts = NULL, *z_ele;
    zend_string *zkey;
    short s2 = 0;

    if (zend_parse_parameters(ZEND_NUM_ARGS(), "sszzs|a",
                              &dest, &destlen, &src, &srclen, &position, &shape,
                              &unit, &unitlen, &opts) == FAILURE)
    {
        return FAILURE;
    }

    if (Z_TYPE_P(position) == IS_STRING && Z_STRLEN_P(position) > 0) {
        argc += 2;
    } else if (Z_TYPE_P(position) == IS_ARRAY && zend_hash_num_elements(Z_ARRVAL_P(position)) == 2) {
        argc += 3;
    } else {
        php_error_docref(NULL, E_WARNING, "Invalid position");
        return FAILURE;
    }

    if (Z_TYPE_P(shape) == IS_LONG || Z_TYPE_P(shape) == IS_DOUBLE) {
        argc += 2;
    } else if (Z_TYPE_P(shape) == IS_ARRAY) {
        argc += 3;
    } else {
        php_error_docref(NULL, E_WARNING, "Invalid shape dimensions");
        return FAILURE;
    }

    /* Attempt to parse our options array */
    if (opts != NULL) {
        ZEND_HASH_FOREACH_STR_KEY_VAL(Z_ARRVAL_P(opts), zkey, z_ele) {
            ZVAL_DEREF(z_ele);
            if (zkey != NULL) {
                if (zend_string_equals_literal_ci(zkey, "COUNT")) {
                    if (Z_TYPE_P(z_ele) != IS_LONG || Z_LVAL_P(z_ele) <= 0) {
                        php_error_docref(NULL, E_WARNING, "COUNT must be an integer > 0!");
                        return FAILURE;
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

    /* Increment argc based on options */
    argc += gopts.withcoord + gopts.withdist + gopts.withhash
         + (gopts.sort != SORT_NONE) + (gopts.count ? 2 : 0)
         + (gopts.store != STORE_NONE);

    REDIS_CMD_INIT_SSTR_STATIC(&cmdstr, argc, "GEOSEARCHSTORE");
    redis_cmd_append_sstr_key(&cmdstr, dest, destlen, redis_sock, slot);
    redis_cmd_append_sstr_key(&cmdstr, src, srclen, redis_sock, slot ? &s2 : NULL);

    if (slot && *slot != s2) {
        php_error_docref(NULL, E_WARNING, "All keys must hash to the same slot");
        efree(cmdstr.c);
        return FAILURE;
    }

    if (Z_TYPE_P(position) == IS_ARRAY) {
        REDIS_CMD_APPEND_SSTR_STATIC(&cmdstr, "FROMLONLAT");
        ZEND_HASH_FOREACH_VAL(Z_ARRVAL_P(position), z_ele) {
            ZVAL_DEREF(z_ele);
            redis_cmd_append_sstr_dbl(&cmdstr, zval_get_double(z_ele));
        } ZEND_HASH_FOREACH_END();
    } else {
        REDIS_CMD_APPEND_SSTR_STATIC(&cmdstr, "FROMMEMBER");
        redis_cmd_append_sstr(&cmdstr, Z_STRVAL_P(position), Z_STRLEN_P(position));
    }

    if (Z_TYPE_P(shape) == IS_ARRAY) {
        REDIS_CMD_APPEND_SSTR_STATIC(&cmdstr, "BYBOX");
        ZEND_HASH_FOREACH_VAL(Z_ARRVAL_P(shape), z_ele) {
            ZVAL_DEREF(z_ele);
            redis_cmd_append_sstr_dbl(&cmdstr, zval_get_double(z_ele));
        } ZEND_HASH_FOREACH_END();
    } else {
        REDIS_CMD_APPEND_SSTR_STATIC(&cmdstr, "BYRADIUS");
        redis_cmd_append_sstr_dbl(&cmdstr, zval_get_double(shape));
    }
    redis_cmd_append_sstr(&cmdstr, unit, unitlen);

    /* Append sort if it's not GEO_NONE */
    if (gopts.sort == SORT_ASC) {
        REDIS_CMD_APPEND_SSTR_STATIC(&cmdstr, "ASC");
    } else if (gopts.sort == SORT_DESC) {
        REDIS_CMD_APPEND_SSTR_STATIC(&cmdstr, "DESC");
    }

    /* Append our count if we've got one */
    if (gopts.count) {
        REDIS_CMD_APPEND_SSTR_STATIC(&cmdstr, "COUNT");
        redis_cmd_append_sstr_long(&cmdstr, gopts.count);
    }

    if (gopts.store == STORE_DIST) {
        REDIS_CMD_APPEND_SSTR_STATIC(&cmdstr, "STOREDIST");
    }

    *cmd = cmdstr.c;
    *cmd_len = cmdstr.len;

    return SUCCESS;
}

/*  MIGRATE host port <key | ""> destination-db timeout [COPY] [REPLACE]
            [[AUTH password] | [AUTH2 username password]] [KEYS key [key ...]]

    Starting with Redis version 3.0.0: Added the COPY and REPLACE options.
    Starting with Redis version 3.0.6: Added the KEYS option.
    Starting with Redis version 4.0.7: Added the AUTH option.
    Starting with Redis version 6.0.0: Added the AUTH2 option.
*/

/* MIGRATE */
int redis_migrate_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock,
                      char **cmd, int *cmd_len, short *slot, void **ctx)
{
    zend_string *host = NULL, *key = NULL, *user = NULL, *pass = NULL;
    zend_long destdb = 0, port = 0, timeout = 0;
    zval *zkeys = NULL, *zkey, *zauth = NULL;
    zend_bool copy = 0, replace = 0;
    smart_string cmdstr = {0};
    int argc;

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
    ZEND_PARSE_PARAMETERS_END_EX(return FAILURE);

    /* Sanity check on our optional AUTH argument */
    if (zauth && redis_extract_auth_info(zauth, &user, &pass) == FAILURE) {
        php_error_docref(NULL, E_WARNING, "AUTH must be a string or an array with one or two strings");
        user = pass = NULL;
    }

    /* Protect against being passed an array with zero elements */
    if (Z_TYPE_P(zkeys) == IS_ARRAY && zend_hash_num_elements(Z_ARRVAL_P(zkeys)) == 0) {
        php_error_docref(NULL, E_WARNING, "Keys array cannot be empty");
        return FAILURE;
    }

    /* host, port, key|"", dest-db, timeout, [copy, replace] [KEYS key1..keyN] */
    argc = 5 + copy + replace + (user||pass ? 1 : 0) + (user != NULL) + (pass != NULL);
    if (Z_TYPE_P(zkeys) == IS_ARRAY) {
        /* +1 for the "KEYS" argument itself */
        argc += 1 + zend_hash_num_elements(Z_ARRVAL_P(zkeys));
    }

    /* Initialize MIGRATE command with host and port */
    REDIS_CMD_INIT_SSTR_STATIC(&cmdstr, argc, "MIGRATE");
    redis_cmd_append_sstr_zstr(&cmdstr, host);
    redis_cmd_append_sstr_long(&cmdstr, port);

    /* If passed a keys array the keys come later, otherwise pass the key to
     * migrate here */
    if (Z_TYPE_P(zkeys) == IS_ARRAY) {
        REDIS_CMD_APPEND_SSTR_STATIC(&cmdstr, "");
    } else {
        key = redis_key_prefix_zval(redis_sock, zkeys);
        redis_cmd_append_sstr_zstr(&cmdstr, key);
        zend_string_release(key);
    }

    redis_cmd_append_sstr_long(&cmdstr, destdb);
    redis_cmd_append_sstr_long(&cmdstr, timeout);
    REDIS_CMD_APPEND_SSTR_OPT_STATIC(&cmdstr, copy, "COPY");
    REDIS_CMD_APPEND_SSTR_OPT_STATIC(&cmdstr, replace, "REPLACE");

    if (user && pass) {
        REDIS_CMD_APPEND_SSTR_STATIC(&cmdstr, "AUTH2");
        redis_cmd_append_sstr_zstr(&cmdstr, user);
        redis_cmd_append_sstr_zstr(&cmdstr, pass);
    } else if (pass) {
        REDIS_CMD_APPEND_SSTR_STATIC(&cmdstr, "AUTH");
        redis_cmd_append_sstr_zstr(&cmdstr, pass);
    }

    /* Append actual keys if we've got a keys array */
    if (Z_TYPE_P(zkeys) == IS_ARRAY) {
        REDIS_CMD_APPEND_SSTR_STATIC(&cmdstr, "KEYS");

        ZEND_HASH_FOREACH_VAL(Z_ARRVAL_P(zkeys), zkey) {
            key = redis_key_prefix_zval(redis_sock, zkey);
            redis_cmd_append_sstr_zstr(&cmdstr, key);
            zend_string_release(key);
        } ZEND_HASH_FOREACH_END();
    }

    if (user) zend_string_release(user);
    if (pass) zend_string_release(pass);

    *cmd = cmdstr.c;
    *cmd_len = cmdstr.len;
    return SUCCESS;
}

/* A generic passthru function for variadic key commands that take one or more
 * keys.  This is essentially all of them except ones that STORE data. */
int redis_varkey_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock,
                     char *kw, char **cmd, int *cmd_len, short *slot, void **ctx)
{
    return gen_varkey_cmd(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock,
                          kw, strlen(kw), 0, cmd, cmd_len, slot);
}

static int
redis_build_client_list_command(smart_string *cmdstr, int argc, zval *z_args)
{
    zend_string *zkey;
    zval *z_ele, *type = NULL, *id = NULL;

    if (argc > 0) {
        if (Z_TYPE(z_args[0]) != IS_ARRAY) {
            return FAILURE;
        }
        ZEND_HASH_FOREACH_STR_KEY_VAL(Z_ARRVAL(z_args[0]), zkey, z_ele) {
            if (zkey != NULL) {
                ZVAL_DEREF(z_ele);
                if (zend_string_equals_literal_ci(zkey, "type")) {
                    if (Z_TYPE_P(z_ele) != IS_STRING || (
                        !ZVAL_STRICMP_STATIC(z_ele, "normal") &&
                        !ZVAL_STRICMP_STATIC(z_ele, "master") &&
                        !ZVAL_STRICMP_STATIC(z_ele, "replica") &&
                        !ZVAL_STRICMP_STATIC(z_ele, "pubsub")
                    )) {
                        return FAILURE;
                    }
                    type = z_ele;
                } else if (zend_string_equals_literal_ci(zkey, "id")) {
                    if (Z_TYPE_P(z_ele) != IS_STRING && (
                        Z_TYPE_P(z_ele) != IS_ARRAY ||
                        !zend_hash_num_elements(Z_ARRVAL_P(z_ele))
                    )) {
                        return FAILURE;
                    }
                    id = z_ele;
                }
            }
        } ZEND_HASH_FOREACH_END();
    }
    REDIS_CMD_INIT_SSTR_STATIC(cmdstr, 1 + (type ? 2 : 0) + (
        id ? (Z_TYPE_P(id) == IS_ARRAY ? 1 + zend_hash_num_elements(Z_ARRVAL_P(id)) : 2) : 0
    ), "CLIENT");
    REDIS_CMD_APPEND_SSTR_STATIC(cmdstr, "LIST");
    if (type != NULL) {
        REDIS_CMD_APPEND_SSTR_STATIC(cmdstr, "TYPE");
        redis_cmd_append_sstr(cmdstr, Z_STRVAL_P(type), Z_STRLEN_P(type));
    }
    if (id != NULL) {
        REDIS_CMD_APPEND_SSTR_STATIC(cmdstr, "ID");
        if (Z_TYPE_P(id) == IS_ARRAY) {
            ZEND_HASH_FOREACH_VAL(Z_ARRVAL_P(id), z_ele) {
                if (Z_TYPE_P(z_ele) == IS_STRING) {
                    redis_cmd_append_sstr(cmdstr, Z_STRVAL_P(z_ele), Z_STRLEN_P(z_ele));
                } else {
                    zkey = zval_get_string(z_ele);
                    redis_cmd_append_sstr(cmdstr, ZSTR_VAL(zkey), ZSTR_LEN(zkey));
                    zend_string_release(zkey);
                }
            } ZEND_HASH_FOREACH_END();
        } else {
            redis_cmd_append_sstr(cmdstr, Z_STRVAL_P(id), Z_STRLEN_P(id));
        }
    }
    return SUCCESS;
}

static int
redis_build_client_kill_command(smart_string *cmdstr, int argc, zval *z_args)
{
    zend_string *zkey;
    zval *z_ele, *id = NULL, *type = NULL, *address = NULL, *opts = NULL,
        *user = NULL, *addr = NULL, *laddr = NULL, *skipme = NULL;

    if (argc > 0) {
        if (argc > 1) {
            if (Z_TYPE(z_args[0]) != IS_STRING || Z_TYPE(z_args[1]) != IS_ARRAY) {
                return FAILURE;
            }
            address = &z_args[0];
            opts = &z_args[1];
        } else if (Z_TYPE(z_args[0]) == IS_STRING) {
            address = &z_args[0];
        } else if (Z_TYPE(z_args[0]) == IS_ARRAY) {
            opts = &z_args[0];
        } else {
            return FAILURE;
        }
        if (opts != NULL) {
            ZEND_HASH_FOREACH_STR_KEY_VAL(Z_ARRVAL_P(opts), zkey, z_ele) {
                if (zkey != NULL) {
                    ZVAL_DEREF(z_ele);
                    if (Z_TYPE_P(z_ele) != IS_STRING) {
                        return FAILURE;
                    }
                    if (zend_string_equals_literal_ci(zkey, "id")) {
                        id = z_ele;
                    } else if (zend_string_equals_literal_ci(zkey, "type")) {
                        if (!ZVAL_STRICMP_STATIC(z_ele, "normal") &&
                            !ZVAL_STRICMP_STATIC(z_ele, "master") &&
                            !ZVAL_STRICMP_STATIC(z_ele, "slave") &&
                            !ZVAL_STRICMP_STATIC(z_ele, "replica") &&
                            !ZVAL_STRICMP_STATIC(z_ele, "pubsub")
                        ) {
                            return FAILURE;
                        }
                        type = z_ele;
                    } else if (zend_string_equals_literal_ci(zkey, "user")) {
                        user = z_ele;
                    } else if (zend_string_equals_literal_ci(zkey, "addr")) {
                        addr = z_ele;
                    } else if (zend_string_equals_literal_ci(zkey, "laddr")) {
                        laddr = z_ele;
                    } else if (zend_string_equals_literal_ci(zkey, "skipme")) {
                        if (!ZVAL_STRICMP_STATIC(z_ele, "yes") &&
                            !ZVAL_STRICMP_STATIC(z_ele, "no")
                        ) {
                            return FAILURE;
                        }
                        skipme = z_ele;
                    }
                }
            } ZEND_HASH_FOREACH_END();
        }
    }
    REDIS_CMD_INIT_SSTR_STATIC(cmdstr, 1 + (address != 0) + (id ? 2 : 0)
        + (type ? 2 : 0) + (user ? 2 : 0) + (addr ? 2 : 0) + (laddr ? 2 : 0)
        + (skipme ? 2 : 0), "CLIENT");
    REDIS_CMD_APPEND_SSTR_STATIC(cmdstr, "KILL");
    if (address != NULL) {
        redis_cmd_append_sstr(cmdstr, Z_STRVAL_P(address), Z_STRLEN_P(address));
    }
    if (id != NULL) {
        REDIS_CMD_APPEND_SSTR_STATIC(cmdstr, "ID");
        redis_cmd_append_sstr(cmdstr, Z_STRVAL_P(id), Z_STRLEN_P(id));
    }
    if (type != NULL) {
        REDIS_CMD_APPEND_SSTR_STATIC(cmdstr, "TYPE");
        redis_cmd_append_sstr(cmdstr, Z_STRVAL_P(type), Z_STRLEN_P(type));
    }
    if (user != NULL) {
        REDIS_CMD_APPEND_SSTR_STATIC(cmdstr, "USER");
        redis_cmd_append_sstr(cmdstr, Z_STRVAL_P(user), Z_STRLEN_P(user));
    }
    if (addr != NULL) {
        REDIS_CMD_APPEND_SSTR_STATIC(cmdstr, "ADDR");
        redis_cmd_append_sstr(cmdstr, Z_STRVAL_P(addr), Z_STRLEN_P(addr));
    }
    if (laddr != NULL) {
        REDIS_CMD_APPEND_SSTR_STATIC(cmdstr, "LADDR");
        redis_cmd_append_sstr(cmdstr, Z_STRVAL_P(laddr), Z_STRLEN_P(laddr));
    }
    if (skipme != NULL) {
        REDIS_CMD_APPEND_SSTR_STATIC(cmdstr, "SKIPME");
        redis_cmd_append_sstr(cmdstr, Z_STRVAL_P(skipme), Z_STRLEN_P(skipme));
    }
    return SUCCESS;
}

static int
redis_build_client_tracking_command(smart_string *cmdstr, int argc, zval *z_args)
{
    zend_string *zkey;
    zval *z_ele, *redirect = NULL, *prefix = NULL;
    zend_bool bcast = 0, optin = 0, optout = 0, noloop = 0;

    if (argc < 1) {
        return FAILURE;
    }
    if (argc > 1) {
        if (Z_TYPE(z_args[1]) != IS_ARRAY) {
            return FAILURE;
        }
        ZEND_HASH_FOREACH_STR_KEY_VAL(Z_ARRVAL(z_args[1]), zkey, z_ele) {
            if (zkey != NULL) {
                ZVAL_DEREF(z_ele);
                if (zend_string_equals_literal_ci(zkey, "redirect")) {
                    if (Z_TYPE_P(z_ele) != IS_STRING) {
                        return FAILURE;
                    }
                    redirect = z_ele;
                } else if (zend_string_equals_literal_ci(zkey, "prefix")) {
                    if (Z_TYPE_P(z_ele) != IS_STRING && Z_TYPE_P(z_ele) != IS_ARRAY) {
                        return FAILURE;
                    }
                    prefix = z_ele;
                } else if (zend_string_equals_literal_ci(zkey, "bcast")) {
                    bcast = zval_is_true(z_ele);
                } else if (zend_string_equals_literal_ci(zkey, "optin")) {
                    optin = zval_is_true(z_ele);
                } else if (zend_string_equals_literal_ci(zkey, "optout")) {
                    optout = zval_is_true(z_ele);
                } else if (zend_string_equals_literal_ci(zkey, "noloop")) {
                    noloop = zval_is_true(z_ele);
                }
            }
        } ZEND_HASH_FOREACH_END();
    }
    REDIS_CMD_INIT_SSTR_STATIC(cmdstr, 2 + (redirect ? 2 : 0)
        + (prefix ? 2 * zend_hash_num_elements(Z_ARRVAL_P(prefix)) : 0)
        + bcast + optin + optout + noloop, "CLIENT");
    REDIS_CMD_APPEND_SSTR_STATIC(cmdstr, "TRACKING");
    if (Z_TYPE(z_args[0]) == IS_STRING && (
        ZVAL_STRICMP_STATIC(&z_args[0], "on") ||
        ZVAL_STRICMP_STATIC(&z_args[0], "off")
    )) {
        redis_cmd_append_sstr(cmdstr, Z_STRVAL(z_args[0]), Z_STRLEN(z_args[0]));
    } else if (zval_is_true(&z_args[0])) {
        REDIS_CMD_APPEND_SSTR_STATIC(cmdstr, "ON");
    } else {
        REDIS_CMD_APPEND_SSTR_STATIC(cmdstr, "OFF");
    }
    if (redirect != NULL) {
        REDIS_CMD_APPEND_SSTR_STATIC(cmdstr, "REDIRECT");
        redis_cmd_append_sstr(cmdstr, Z_STRVAL_P(redirect), Z_STRLEN_P(redirect));
    }
    if (prefix != NULL) {
        if (Z_TYPE_P(prefix) == IS_ARRAY) {
            ZEND_HASH_FOREACH_VAL(Z_ARRVAL_P(prefix), z_ele) {
                REDIS_CMD_APPEND_SSTR_STATIC(cmdstr, "PREFIX");
                if (Z_TYPE_P(z_ele) == IS_STRING) {
                    redis_cmd_append_sstr(cmdstr, Z_STRVAL_P(z_ele), Z_STRLEN_P(z_ele));
                } else {
                    zkey = zval_get_string(z_ele);
                    redis_cmd_append_sstr(cmdstr, ZSTR_VAL(zkey), ZSTR_LEN(zkey));
                    zend_string_release(zkey);
                }
            } ZEND_HASH_FOREACH_END();
        } else {
            REDIS_CMD_APPEND_SSTR_STATIC(cmdstr, "PREFIX");
            redis_cmd_append_sstr(cmdstr, Z_STRVAL_P(prefix), Z_STRLEN_P(prefix));
        }
    }
    if (bcast) {
        REDIS_CMD_APPEND_SSTR_STATIC(cmdstr, "BCAST");
    }
    if (optin) {
        REDIS_CMD_APPEND_SSTR_STATIC(cmdstr, "OPTIN");
    }
    if (optout) {
        REDIS_CMD_APPEND_SSTR_STATIC(cmdstr, "OPTOUT");
    }
    if (noloop) {
        REDIS_CMD_APPEND_SSTR_STATIC(cmdstr, "NOLOOP");
    }
    return SUCCESS;
}

int
redis_client_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock,
                 char **cmd, int *cmd_len, short *slot, void **ctx)
{
    smart_string cmdstr = {0};
    zend_string *op = NULL;
    zval *z_args = NULL;
    int argc = 0;

    ZEND_PARSE_PARAMETERS_START(1, -1)
        Z_PARAM_STR(op)
        Z_PARAM_OPTIONAL
        Z_PARAM_VARIADIC('*', z_args, argc)
    ZEND_PARSE_PARAMETERS_END_EX(return FAILURE);

    if (zend_string_equals_literal_ci(op, "INFO")) {
        REDIS_CMD_INIT_SSTR_STATIC(&cmdstr, 1, "CLIENT");
        REDIS_CMD_APPEND_SSTR_STATIC(&cmdstr, "INFO");
    } else if (zend_string_equals_literal_ci(op, "LIST")) {
        if (redis_build_client_list_command(&cmdstr, argc, z_args) != 0) {
            return FAILURE;
        }
        *ctx = PHPREDIS_CTX_PTR;
    } else if (zend_string_equals_literal_ci(op, "CACHING")) {
        if (argc < 1) {
            return FAILURE;
        }
        REDIS_CMD_INIT_SSTR_STATIC(&cmdstr, 2, "CLIENT");
        REDIS_CMD_APPEND_SSTR_STATIC(&cmdstr, "CACHING");
        if (Z_TYPE(z_args[0]) == IS_STRING && (
            ZVAL_STRICMP_STATIC(&z_args[0], "yes") ||
            ZVAL_STRICMP_STATIC(&z_args[0], "no")
        )) {
            redis_cmd_append_sstr(&cmdstr, Z_STRVAL(z_args[0]), Z_STRLEN(z_args[0]));
        } else if (zval_is_true(&z_args[0])) {
            REDIS_CMD_APPEND_SSTR_STATIC(&cmdstr, "YES");
        } else {
            REDIS_CMD_APPEND_SSTR_STATIC(&cmdstr, "NO");
        }
        *ctx = PHPREDIS_CTX_PTR + 1;
    } else if (zend_string_equals_literal_ci(op, "GETNAME")) {
        REDIS_CMD_INIT_SSTR_STATIC(&cmdstr, 1, "CLIENT");
        REDIS_CMD_APPEND_SSTR_STATIC(&cmdstr, "GETNAME");
        *ctx = PHPREDIS_CTX_PTR + 3;
    } else if (zend_string_equals_literal_ci(op, "GETREDIR") || zend_string_equals_literal_ci(op, "ID")) {
        REDIS_CMD_INIT_SSTR_STATIC(&cmdstr, 1, "CLIENT");
        redis_cmd_append_sstr(&cmdstr, ZSTR_VAL(op), ZSTR_LEN(op));
        *ctx = PHPREDIS_CTX_PTR + 2;
    } else if (zend_string_equals_literal_ci(op, "KILL")) {
        if (redis_build_client_kill_command(&cmdstr, argc, z_args) != 0) {
            return FAILURE;
        }
        *ctx = PHPREDIS_CTX_PTR + 1;
    } else if (zend_string_equals_literal_ci(op, "NO-EVICT")) {
        if (argc < 1) {
            return FAILURE;
        }
        REDIS_CMD_INIT_SSTR_STATIC(&cmdstr, 2, "CLIENT");
        REDIS_CMD_APPEND_SSTR_STATIC(&cmdstr, "NO-EVICT");
        if (Z_TYPE(z_args[0]) == IS_STRING && (
            ZVAL_STRICMP_STATIC(&z_args[0], "on") ||
            ZVAL_STRICMP_STATIC(&z_args[0], "off")
        )) {
            redis_cmd_append_sstr(&cmdstr, Z_STRVAL(z_args[0]), Z_STRLEN(z_args[0]));
        } else if (zval_is_true(&z_args[0])) {
            REDIS_CMD_APPEND_SSTR_STATIC(&cmdstr, "ON");
        } else {
            REDIS_CMD_APPEND_SSTR_STATIC(&cmdstr, "OFF");
        }
        *ctx = PHPREDIS_CTX_PTR + 1;
    } else if (zend_string_equals_literal_ci(op, "PAUSE")) {
        if (argc < 1 || Z_TYPE(z_args[0]) != IS_LONG || (
            argc > 1 && (
                Z_TYPE(z_args[1]) != IS_STRING || (
                    !ZVAL_STRICMP_STATIC(&z_args[1], "write") &&
                    !ZVAL_STRICMP_STATIC(&z_args[1], "all")
                )
            )
        )) {
            return FAILURE;
        }
        REDIS_CMD_INIT_SSTR_STATIC(&cmdstr, argc > 1 ? 3 : 2, "CLIENT");
        REDIS_CMD_APPEND_SSTR_STATIC(&cmdstr, "PAUSE");
        redis_cmd_append_sstr_long(&cmdstr, Z_LVAL(z_args[0]));
        if (argc > 1) {
            redis_cmd_append_sstr(&cmdstr, Z_STRVAL(z_args[1]), Z_STRLEN(z_args[1]));
        }
        *ctx = PHPREDIS_CTX_PTR + 1;
    } else if (zend_string_equals_literal_ci(op, "REPLY")) {
        if (argc > 0 && (
            Z_TYPE(z_args[0]) != IS_STRING || (
                !ZVAL_STRICMP_STATIC(&z_args[0], "on") &&
                !ZVAL_STRICMP_STATIC(&z_args[0], "off") &&
                !ZVAL_STRICMP_STATIC(&z_args[0], "skip")
            )
        )) {
            return FAILURE;
        }
        REDIS_CMD_INIT_SSTR_STATIC(&cmdstr, argc > 0 ? 2 : 1, "CLIENT");
        REDIS_CMD_APPEND_SSTR_STATIC(&cmdstr, "REPLY");
        if (argc > 0) {
            redis_cmd_append_sstr(&cmdstr, Z_STRVAL(z_args[0]), Z_STRLEN(z_args[0]));
        }
        *ctx = PHPREDIS_CTX_PTR + 1;
    } else if (zend_string_equals_literal_ci(op, "SETNAME")) {
        if (argc < 1 || Z_TYPE(z_args[0]) != IS_STRING) {
            return FAILURE;
        }
        REDIS_CMD_INIT_SSTR_STATIC(&cmdstr, 2, "CLIENT");
        REDIS_CMD_APPEND_SSTR_STATIC(&cmdstr, "SETNAME");
        redis_cmd_append_sstr(&cmdstr, Z_STRVAL(z_args[0]), Z_STRLEN(z_args[0]));
        *ctx = PHPREDIS_CTX_PTR + 1;
    } else if (zend_string_equals_literal_ci(op, "TRACKING")) {
        if (redis_build_client_tracking_command(&cmdstr, argc, z_args) != 0) {
            return FAILURE;
        }
        *ctx = PHPREDIS_CTX_PTR + 1;
    } else if (zend_string_equals_literal_ci(op, "TRACKINGINFO")) {
        REDIS_CMD_INIT_SSTR_STATIC(&cmdstr, 1, "CLIENT");
        REDIS_CMD_APPEND_SSTR_STATIC(&cmdstr, "TRACKINGINFO");
        *ctx = PHPREDIS_CTX_PTR + 4;
    } else if (zend_string_equals_literal_ci(op, "UNBLOCK")) {
        if (argc < 1 || Z_TYPE(z_args[0]) != IS_STRING || (
            argc > 1 && (
                Z_TYPE(z_args[1]) != IS_STRING || (
                    !ZVAL_STRICMP_STATIC(&z_args[1], "timeout") &&
                    !ZVAL_STRICMP_STATIC(&z_args[1], "error")
                )
            )
        )) {
            return FAILURE;
        }
        REDIS_CMD_INIT_SSTR_STATIC(&cmdstr, argc > 1 ? 3 : 2, "CLIENT");
        REDIS_CMD_APPEND_SSTR_STATIC(&cmdstr, "UNBLOCK");
        redis_cmd_append_sstr(&cmdstr, Z_STRVAL(z_args[0]), Z_STRLEN(z_args[0]));
        if (argc > 1) {
            redis_cmd_append_sstr(&cmdstr, Z_STRVAL(z_args[1]), Z_STRLEN(z_args[1]));
        }
        *ctx = PHPREDIS_CTX_PTR + 2;
    } else if (zend_string_equals_literal_ci(op, "UNPAUSE")) {
        REDIS_CMD_INIT_SSTR_STATIC(&cmdstr, 2, "CLIENT");
        REDIS_CMD_APPEND_SSTR_STATIC(&cmdstr, "UNPAUSE");
        *ctx = PHPREDIS_CTX_PTR + 1;
    } else {
        return FAILURE;
    }

    // Push out values
    *cmd = cmdstr.c;
    *cmd_len = cmdstr.len;

    return SUCCESS;
}

/* COMMAND */
int redis_command_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock,
                      char **cmd, int *cmd_len, short *slot, void **ctx)
{
    smart_string cmdstr = {0};
    zend_string *op = NULL, *zstr;
    zval *z_args = NULL;
    int i, argc = 0;

    ZEND_PARSE_PARAMETERS_START(0, -1)
        Z_PARAM_OPTIONAL
        Z_PARAM_STR(op)
        Z_PARAM_VARIADIC('*', z_args, argc)
    ZEND_PARSE_PARAMETERS_END_EX(return FAILURE);

    if (op == NULL) {
        *ctx = NULL;
        argc = 0;
    } else if (zend_string_equals_literal_ci(op, "COUNT")) {
        *ctx = PHPREDIS_CTX_PTR;
        argc = 0;
    } else if (zend_string_equals_literal_ci(op, "DOCS") ||
        zend_string_equals_literal_ci(op, "INFO")
    ) {
        *ctx = NULL;
    } else if (zend_string_equals_literal_ci(op, "GETKEYS") ||
        zend_string_equals_literal_ci(op, "LIST")
    ) {
        *ctx = PHPREDIS_CTX_PTR + 1;
    } else if (zend_string_equals_literal_ci(op, "GETKEYSANDFLAGS")) {
        *ctx = PHPREDIS_CTX_PTR + 2;
    } else {
        php_error_docref(NULL, E_WARNING, "Unknown COMMAND operation '%s'", ZSTR_VAL(op));
        return FAILURE;
    }

    REDIS_CMD_INIT_SSTR_STATIC(&cmdstr, !!op + argc, "COMMAND");
    if (op) redis_cmd_append_sstr_zstr(&cmdstr, op);

    for (i = 0; i < argc; ++i) {
        zstr = zval_get_string(&z_args[i]);
        redis_cmd_append_sstr(&cmdstr, ZSTR_VAL(zstr), ZSTR_LEN(zstr));
        zend_string_release(zstr);
    }

    // Push out values
    *cmd = cmdstr.c;
    *cmd_len = cmdstr.len;

    /* Any slot will do */
    CMD_RAND_SLOT(slot);

    return SUCCESS;
}

int
redis_copy_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock,
               char **cmd, int *cmd_len, short *slot, void **ctx)
{
    zend_string *src = NULL, *dst = NULL;
    smart_string cmdstr = {0};
    HashTable *opts = NULL;
    zend_bool replace = 0;
    zend_string *zkey;
    zend_long db = -1;
    short slot2;
    zval *zv;

    ZEND_PARSE_PARAMETERS_START(2, 3)
        Z_PARAM_STR(src)
        Z_PARAM_STR(dst)
        Z_PARAM_OPTIONAL
        Z_PARAM_ARRAY_HT_OR_NULL(opts)
    ZEND_PARSE_PARAMETERS_END_EX(return FAILURE);

    if (opts != NULL) {
        ZEND_HASH_FOREACH_STR_KEY_VAL(opts, zkey, zv) {
            if (zkey == NULL)
                continue;

            ZVAL_DEREF(zv);
            if (zend_string_equals_literal_ci(zkey, "db")) {
                db = zval_get_long(zv);
            } else if (zend_string_equals_literal_ci(zkey, "replace")) {
                replace = zval_is_true(zv);
            }
        } ZEND_HASH_FOREACH_END();
    }

    if (slot && db != -1) {
        php_error_docref(NULL, E_WARNING, "Cant copy to a specific DB in cluster mode");
        return FAILURE;
    }

    REDIS_CMD_INIT_SSTR_STATIC(&cmdstr, 2 + (db > -1 ? 2 : 0) + replace, "COPY");
    redis_cmd_append_sstr_key_zstr(&cmdstr, src, redis_sock, slot);
    redis_cmd_append_sstr_key_zstr(&cmdstr, dst, redis_sock, slot ? &slot2 : NULL);

    if (slot && *slot != slot2) {
        php_error_docref(NULL, E_WARNING, "Keys must hash to the same slot!");
        efree(cmdstr.c);
        return FAILURE;
    }

    if (db > -1) {
        REDIS_CMD_APPEND_SSTR_STATIC(&cmdstr, "DB");
        redis_cmd_append_sstr_long(&cmdstr, db);
    }
    REDIS_CMD_APPEND_SSTR_OPT_STATIC(&cmdstr, replace, "REPLACE");

    *cmd = cmdstr.c;
    *cmd_len = cmdstr.len;
    return SUCCESS;
}

/* XADD */
int redis_xadd_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock,
                   char **cmd, int *cmd_len, short *slot, void **ctx)
{
    smart_string cmdstr = {0};
    zend_string *arrkey;
    zval *z_fields, *value;
    zend_long maxlen = 0;
    zend_bool approx = 0, nomkstream = 0;
    zend_ulong idx;
    HashTable *ht_fields;
    int fcount, argc;
    char *key, *id;
    size_t keylen, idlen;

    if (zend_parse_parameters(ZEND_NUM_ARGS(), "ssa|lbb", &key, &keylen,
                              &id, &idlen, &z_fields, &maxlen, &approx,
                              &nomkstream) == FAILURE)
    {
        return FAILURE;
    }

    /* At least one field and string are required */
    ht_fields = Z_ARRVAL_P(z_fields);
    if ((fcount = zend_hash_num_elements(ht_fields)) == 0) {
        return FAILURE;
    }

    if (maxlen < 0 || (maxlen == 0 && approx != 0)) {
        php_error_docref(NULL, E_WARNING,
            "Warning:  Invalid MAXLEN argument or approximate flag");
    }


    /* Calculate argc for XADD.  It's a bit complex because we've got
     * an optional MAXLEN argument which can either take the form MAXLEN N
     * or MAXLEN ~ N */
    argc = 2 + nomkstream + (fcount * 2) + (maxlen > 0 ? (approx ? 3 : 2) : 0);

    /* XADD key ID field string [field string ...] */
    REDIS_CMD_INIT_SSTR_STATIC(&cmdstr, argc, "XADD");
    redis_cmd_append_sstr_key(&cmdstr, key, keylen, redis_sock, slot);

    if (nomkstream) {
        REDIS_CMD_APPEND_SSTR_STATIC(&cmdstr, "NOMKSTREAM");
    }

    /* Now append our MAXLEN bits if we've got them */
    if (maxlen > 0) {
        REDIS_CMD_APPEND_SSTR_STATIC(&cmdstr, "MAXLEN");
        REDIS_CMD_APPEND_SSTR_OPT_STATIC(&cmdstr, approx, "~");
        redis_cmd_append_sstr_long(&cmdstr, maxlen);
    }

    /* Now append ID and field(s) */
    redis_cmd_append_sstr(&cmdstr, id, idlen);
    ZEND_HASH_FOREACH_KEY_VAL(ht_fields, idx, arrkey, value) {
        redis_cmd_append_sstr_arrkey(&cmdstr, arrkey, idx);
        redis_cmd_append_sstr_zval(&cmdstr, value, redis_sock);
    } ZEND_HASH_FOREACH_END();

    *cmd = cmdstr.c;
    *cmd_len = cmdstr.len;
    return SUCCESS;
}

// XPENDING key group [start end count [consumer] [idle]]
int redis_xpending_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock,
                       char **cmd, int *cmd_len, short *slot, void **ctx)
{
    zend_string *key = NULL, *group = NULL, *start = NULL, *end = NULL,
                *consumer = NULL;
    zend_long count = -1, idle = 0;
    smart_string cmdstr = {0};
    int argc;

    ZEND_PARSE_PARAMETERS_START(2, 7)
        Z_PARAM_STR(key)
        Z_PARAM_STR(group)
        Z_PARAM_OPTIONAL
        Z_PARAM_STR_OR_NULL(start)
        Z_PARAM_STR_OR_NULL(end)
        Z_PARAM_LONG(count)
        Z_PARAM_STR_OR_NULL(consumer)
        Z_PARAM_LONG(idle)
    ZEND_PARSE_PARAMETERS_END_EX(return FAILURE);

    /* If we've been passed a start argument, we also need end and count */
    if (start != NULL && (end == NULL || count < 0)) {
        php_error_docref(NULL, E_WARNING, "'$start' must be accompanied by '$end' and '$count' arguments");
        return FAILURE;
    }

    /* Calculate argc.  It's either 2, 5, 6 or 7 */
    argc = 2 + (start != NULL ? 3 + (consumer != NULL) + (idle != 0) : 0);

    /* Construct command and add required arguments */
    REDIS_CMD_INIT_SSTR_STATIC(&cmdstr, argc, "XPENDING");
    redis_cmd_append_sstr_key_zstr(&cmdstr, key, redis_sock, slot);
    redis_cmd_append_sstr_zstr(&cmdstr, group);

    /* Add optional argumentst */
    if (start) {
        if (idle != 0) {
            REDIS_CMD_APPEND_SSTR_STATIC(&cmdstr, "IDLE");
            redis_cmd_append_sstr_long(&cmdstr, (long)idle);
        }
        redis_cmd_append_sstr_zstr(&cmdstr, start);
        redis_cmd_append_sstr_zstr(&cmdstr, end);
        redis_cmd_append_sstr_long(&cmdstr, (long)count);

        /* Finally add consumer if we have it */
        if (consumer) redis_cmd_append_sstr_zstr(&cmdstr, consumer);
    }

    *cmd = cmdstr.c;
    *cmd_len = cmdstr.len;
    return SUCCESS;
}

/* X[REV]RANGE key start end [COUNT count] */
int redis_xrange_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock,
                     char *kw, char **cmd, int *cmd_len, short *slot,
                     void **ctx)
{
    smart_string cmdstr = {0};
    char *key, *start, *end;
    size_t keylen, startlen, endlen;
    zend_long count = -1;

    if (zend_parse_parameters(ZEND_NUM_ARGS(), "sss|l", &key, &keylen,
                              &start, &startlen, &end, &endlen, &count)
                              == FAILURE)
    {
        return FAILURE;
    }

    redis_cmd_init_sstr(&cmdstr, 3 + (2 * (count > -1)), kw, strlen(kw));
    redis_cmd_append_sstr_key(&cmdstr, key, keylen, redis_sock, slot);
    redis_cmd_append_sstr(&cmdstr, start, startlen);
    redis_cmd_append_sstr(&cmdstr, end, endlen);

    if (count > -1) {
        redis_cmd_append_sstr(&cmdstr, ZEND_STRL("COUNT"));
        redis_cmd_append_sstr_long(&cmdstr, count);
    }

    *cmd = cmdstr.c;
    *cmd_len = cmdstr.len;
    return SUCCESS;
}

/* Helper function to take an associative array and append the Redis
 * STREAMS stream [stream...] id [id ...] arguments to a command string. */
static int
append_stream_args(smart_string *cmdstr, HashTable *ht, RedisSock *redis_sock,
                   short *slot)
{
    char *kptr, kbuf[40];
    int klen, i, pos = 0;
    zend_string *key, *idstr;
    short oldslot = -1;
    zval **id;
    zend_ulong idx;

    /* Append STREAM qualifier */
    REDIS_CMD_APPEND_SSTR_STATIC(cmdstr, "STREAMS");

    /* Allocate memory to keep IDs */
    id = emalloc(sizeof(*id) * zend_hash_num_elements(ht));

    /* Iterate over our stream => id array appending streams and retaining each
     * value for final arguments */
    ZEND_HASH_FOREACH_KEY_VAL(ht, idx, key, id[pos++]) {
        if (key) {
            klen = ZSTR_LEN(key);
            kptr = ZSTR_VAL(key);
        } else {
            klen = snprintf(kbuf, sizeof(kbuf), "%ld", (long)idx);
            kptr = (char*)kbuf;
        }

        /* Append stream key */
        redis_cmd_append_sstr_key(cmdstr, kptr, klen, redis_sock, slot);

        /* Protect the user against CROSSSLOT to avoid confusion */
        if (slot) {
            if (oldslot != -1 && *slot != oldslot) {
                php_error_docref(NULL, E_WARNING,
                    "Warning, not all keys hash to the same slot!");
                efree(id);
                return FAILURE;
            }
            oldslot = *slot;
        }
    } ZEND_HASH_FOREACH_END();

    /* Add our IDs */
    for (i = 0; i < pos; i++) {
        idstr = zval_get_string(id[i]);
        redis_cmd_append_sstr(cmdstr, ZSTR_VAL(idstr), ZSTR_LEN(idstr));
        zend_string_release(idstr);
    }

    /* Clean up ID container array */
    efree(id);

    return 0;
}

/* XREAD [COUNT count] [BLOCK ms] STREAMS key [key ...] id [id ...] */
int redis_xread_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock,
                    char **cmd, int *cmd_len, short *slot, void **ctx)
{
    smart_string cmdstr = {0};
    zend_long count = -1, block = -1;
    zval *z_streams;
    int argc, scount;
    HashTable *kt;

    if (zend_parse_parameters(ZEND_NUM_ARGS(), "a|ll", &z_streams,
                              &count, &block) == FAILURE)
    {
        return FAILURE;
    }

    /* At least one stream and ID is required */
    kt = Z_ARRVAL_P(z_streams);
    if ((scount = zend_hash_num_elements(kt)) < 1) {
        return FAILURE;
    }

    /* Calculate argc and start constructing command */
    argc = 1 + (2 * scount) + (2 * (count > -1)) + (2 * (block > -1));
    REDIS_CMD_INIT_SSTR_STATIC(&cmdstr, argc, "XREAD");

    /* Append COUNT if we have it */
    if (count > -1) {
        REDIS_CMD_APPEND_SSTR_STATIC(&cmdstr, "COUNT");
        redis_cmd_append_sstr_long(&cmdstr, count);
    }

    /* Append BLOCK if we have it */
    if (block > -1) {
        REDIS_CMD_APPEND_SSTR_STATIC(&cmdstr, "BLOCK");
        redis_cmd_append_sstr_long(&cmdstr, block);
    }

    /* Append final STREAM key [key ...] id [id ...] arguments */
    if (append_stream_args(&cmdstr, kt, redis_sock, slot) < 0) {
        efree(cmdstr.c);
        return FAILURE;
    }

    *cmd = cmdstr.c;
    *cmd_len = cmdstr.len;
    return SUCCESS;
}

/* XREADGROUP GROUP group consumer [COUNT count] [BLOCK ms]
 * STREAMS key [key ...] id [id ...] */
int redis_xreadgroup_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock,
                         char **cmd, int *cmd_len, short *slot, void **ctx)
{
    smart_string cmdstr = {0};
    zval *z_streams;
    HashTable *kt;
    char *group, *consumer;
    size_t grouplen, consumerlen;
    int scount, argc;
    zend_long count, block;
    zend_bool no_count = 1, no_block = 1;

    if (zend_parse_parameters(ZEND_NUM_ARGS(), "ssa|l!l!", &group,
                              &grouplen, &consumer, &consumerlen, &z_streams,
                              &count, &no_count, &block, &no_block) == FAILURE)
    {
        return FAILURE;
    }

    /* Negative COUNT or BLOCK is illegal so abort immediately */
    if ((!no_count && count < 0) || (!no_block && block < 0)) {
        php_error_docref(NULL, E_WARNING, "Negative values for COUNT or BLOCK are illegal.");
        return FAILURE;
    }

    /* Redis requires at least one stream */
    kt = Z_ARRVAL_P(z_streams);
    if ((scount = zend_hash_num_elements(kt)) < 1) {
        return FAILURE;
    }

    /* Calculate argc and start constructing commands */
    argc = 4 + (2 * scount) + (2 * !no_count) + (2 * !no_block);
    REDIS_CMD_INIT_SSTR_STATIC(&cmdstr, argc, "XREADGROUP");

    /* Group and consumer */
    REDIS_CMD_APPEND_SSTR_STATIC(&cmdstr, "GROUP");
    redis_cmd_append_sstr(&cmdstr, group, grouplen);
    redis_cmd_append_sstr(&cmdstr, consumer, consumerlen);

    /* Append COUNT if we have it */
    if (!no_count) {
        REDIS_CMD_APPEND_SSTR_STATIC(&cmdstr, "COUNT");
        redis_cmd_append_sstr_long(&cmdstr, count);
    }

    /* Append BLOCK argument if we have it */
    if (!no_block) {
        REDIS_CMD_APPEND_SSTR_STATIC(&cmdstr, "BLOCK");
        redis_cmd_append_sstr_long(&cmdstr, block);
    }

    /* Finally append stream and id args */
    if (append_stream_args(&cmdstr, kt, redis_sock, slot) < 0) {
        efree(cmdstr.c);
        return FAILURE;
    }

    *cmd = cmdstr.c;
    *cmd_len = cmdstr.len;
    return SUCCESS;
}

/* XACK key group id [id ...] */
int redis_xack_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock,
                   char **cmd, int *cmd_len, short *slot, void **ctx)
{
    smart_string cmdstr = {0};
    char *key, *group;
    size_t keylen, grouplen;
    zend_string *idstr;
    zval *z_ids, *z_id;
    HashTable *ht_ids;
    int idcount;

    if (zend_parse_parameters(ZEND_NUM_ARGS(), "ssa", &key, &keylen,
                              &group, &grouplen, &z_ids) == FAILURE)
    {
        return FAILURE;
    }

    ht_ids = Z_ARRVAL_P(z_ids);
    if ((idcount = zend_hash_num_elements(ht_ids)) < 1) {
        return FAILURE;
    }

    /* Create command and add initial arguments */
    REDIS_CMD_INIT_SSTR_STATIC(&cmdstr, 2 + idcount, "XACK");
    redis_cmd_append_sstr_key(&cmdstr, key, keylen, redis_sock, slot);
    redis_cmd_append_sstr(&cmdstr, group, grouplen);

    /* Append IDs */
    ZEND_HASH_FOREACH_VAL(ht_ids, z_id) {
        idstr = zval_get_string(z_id);
        redis_cmd_append_sstr(&cmdstr, ZSTR_VAL(idstr), ZSTR_LEN(idstr));
        zend_string_release(idstr);
    } ZEND_HASH_FOREACH_END();

    *cmd = cmdstr.c;
    *cmd_len = cmdstr.len;
    return SUCCESS;
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
static void get_xclaim_options(zval *z_arr, xclaimOptions *opt) {
    zend_string *zkey;
    HashTable *ht;
    zval *zv;

    /* Initialize options array to sane defaults */
    memset(opt, 0, sizeof(*opt));
    opt->retrycount = -1;
    opt->idle.time = -1;

    /* Early return if we don't have any options */
    if (z_arr == NULL)
        return;

    /* Iterate over our options array */
    ht = Z_ARRVAL_P(z_arr);
    ZEND_HASH_FOREACH_STR_KEY_VAL(ht, zkey, zv) {
        if (zkey) {
            if (zend_string_equals_literal_ci(zkey, "TIME")) {
                opt->idle.type = "TIME";
                opt->idle.time = get_xclaim_i64_arg("TIME", zv);
            } else if (zend_string_equals_literal_ci(zkey, "IDLE")) {
                opt->idle.type = "IDLE";
                opt->idle.time = get_xclaim_i64_arg("IDLE", zv);
            } else if (zend_string_equals_literal_ci(zkey, "RETRYCOUNT")) {
                opt->retrycount = zval_get_long(zv);
            }
        } else if (Z_TYPE_P(zv) == IS_STRING) {
            if (zend_string_equals_literal_ci(Z_STR_P(zv), "FORCE")) {
                opt->force = 1;
            } else if (zend_string_equals_literal_ci(Z_STR_P(zv), "JUSTID")) {
                opt->justid = 1;
            }
        }
    } ZEND_HASH_FOREACH_END();
}

/* Count argc for any options we may have */
static int xclaim_options_argc(xclaimOptions *opt) {
    int argc = 0;

    if (opt->idle.type != NULL && opt->idle.time != -1)
        argc += 2;
    if (opt->retrycount != -1)
        argc += 2;
    if (opt->force)
        argc++;
    if (opt->justid)
        argc++;

    return argc;
}

/* Append XCLAIM options */
static void append_xclaim_options(smart_string *cmd, xclaimOptions *opt) {
    /* IDLE/TIME long */
    if (opt->idle.type != NULL && opt->idle.time != -1) {
        redis_cmd_append_sstr(cmd, opt->idle.type, strlen(opt->idle.type));
        redis_cmd_append_sstr_i64(cmd, opt->idle.time);
    }

    /* RETRYCOUNT */
    if (opt->retrycount != -1) {
        REDIS_CMD_APPEND_SSTR_STATIC(cmd, "RETRYCOUNT");
        redis_cmd_append_sstr_long(cmd, opt->retrycount);
    }

    /* FORCE and JUSTID */
    if (opt->force)
        REDIS_CMD_APPEND_SSTR_STATIC(cmd, "FORCE");
    if (opt->justid)
        REDIS_CMD_APPEND_SSTR_STATIC(cmd, "JUSTID");
}


int
redis_xautoclaim_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock,
                     char **cmd, int *cmd_len, short *slot, void **ctx)
{
    smart_string cmdstr = {0};
    char *key, *group, *consumer, *start;
    size_t keylen, grouplen, consumerlen, startlen;
    zend_long min_idle, count = -1;
    zend_bool justid = 0;
    int argc;

    if (zend_parse_parameters(ZEND_NUM_ARGS(), "sssls|lb", &key, &keylen,
                              &group, &grouplen, &consumer, &consumerlen,
                              &min_idle, &start, &startlen, &count, &justid
                              ) == FAILURE)
    {
        return FAILURE;
    }

    argc = 5 + (count > 0 ? 1 + count : 0) + justid;

    REDIS_CMD_INIT_SSTR_STATIC(&cmdstr, argc, "XAUTOCLAIM");
    redis_cmd_append_sstr_key(&cmdstr, key, keylen, redis_sock, slot);
    redis_cmd_append_sstr(&cmdstr, group, grouplen);
    redis_cmd_append_sstr(&cmdstr, consumer, consumerlen);
    redis_cmd_append_sstr_long(&cmdstr, min_idle);
    redis_cmd_append_sstr(&cmdstr, start, startlen);

    if (count > 0) {
        REDIS_CMD_APPEND_SSTR_STATIC(&cmdstr, "COUNT");
        redis_cmd_append_sstr_long(&cmdstr, count);
    }

    if (justid) {
        REDIS_CMD_APPEND_SSTR_STATIC(&cmdstr, "JUSTID");
    }

    // Set the context to distinguish XCLAIM from XAUTOCLAIM which
    // have slightly different reply structures.
    *ctx = PHPREDIS_CTX_PTR;

    *cmd = cmdstr.c;
    *cmd_len = cmdstr.len;
	return SUCCESS;
}

/* XCLAIM <key> <group> <consumer> <min-idle-time> <ID-1> <ID-2>
          [IDLE <milliseconds>] [TIME <mstime>] [RETRYCOUNT <count>]
          [FORCE] [JUSTID] */
int redis_xclaim_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock,
					 char **cmd, int *cmd_len, short *slot, void **ctx)
{
    smart_string cmdstr = {0};
    char *key, *group, *consumer;
    size_t keylen, grouplen, consumerlen;
    zend_long min_idle;
    int argc, id_count;
    zval *z_ids, *z_id, *z_opts = NULL;
    zend_string *zstr;
    HashTable *ht_ids;
    xclaimOptions opts;

    if (zend_parse_parameters(ZEND_NUM_ARGS(), "sssla|a", &key, &keylen,
                              &group, &grouplen, &consumer, &consumerlen, &min_idle,
                              &z_ids, &z_opts) == FAILURE)
    {
        return FAILURE;
    }

    /* At least one id is required */
    ht_ids = Z_ARRVAL_P(z_ids);
    if ((id_count = zend_hash_num_elements(ht_ids)) < 1) {
        return FAILURE;
    }

    /* Extract options array if we've got them */
    get_xclaim_options(z_opts, &opts);

    /* Now we have enough information to calculate argc */
    argc = 4 + id_count + xclaim_options_argc(&opts);

    /* Start constructing our command */
    REDIS_CMD_INIT_SSTR_STATIC(&cmdstr, argc, "XCLAIM");
    redis_cmd_append_sstr_key(&cmdstr, key, keylen, redis_sock, slot);
    redis_cmd_append_sstr(&cmdstr, group, grouplen);
    redis_cmd_append_sstr(&cmdstr, consumer, consumerlen);
    redis_cmd_append_sstr_long(&cmdstr, min_idle);

    /* Add IDs */
    ZEND_HASH_FOREACH_VAL(ht_ids, z_id) {
        zstr = zval_get_string(z_id);
        redis_cmd_append_sstr(&cmdstr, ZSTR_VAL(zstr), ZSTR_LEN(zstr));
        zend_string_release(zstr);
    } ZEND_HASH_FOREACH_END();

    /* Finally add our options */
    append_xclaim_options(&cmdstr, &opts);

    /* Success */
    *cmd = cmdstr.c;
    *cmd_len = cmdstr.len;
	return SUCCESS;
}

/* XGROUP HELP
 * XGROUP CREATE          key group id       [MKSTREAM] [ENTRIESREAD <n>]
 * XGROUP SETID           key group id                  [ENTRIESREAD <n>]
 * XGROUP CREATECONSUMER  key group consumer
 * XGROUP DELCONSUMER     key group consumer
 * XGROUP DESTROY         key group
 */
int redis_xgroup_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock,
                     char **cmd, int *cmd_len, short *slot, void **ctx)
{
    zend_string *op = NULL, *key = NULL, *group = NULL, *id_or_consumer = NULL;
    int nargs, is_create = 0, is_setid = 0;
    zend_long entries_read = -2;
    smart_string cmdstr = {0};
    zend_bool mkstream = 0;

    ZEND_PARSE_PARAMETERS_START(1, 6)
        Z_PARAM_STR(op)
        Z_PARAM_OPTIONAL
        Z_PARAM_STR(key)
        Z_PARAM_STR(group)
        Z_PARAM_STR(id_or_consumer)
        Z_PARAM_BOOL(mkstream)
        Z_PARAM_LONG(entries_read)
    ZEND_PARSE_PARAMETERS_END_EX(return FAILURE);

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
        return FAILURE;
    }

    if (ZEND_NUM_ARGS() < nargs) {
        php_error_docref(NULL, E_WARNING, "Operation '%s' requires %d arguments", ZSTR_VAL(op), nargs);
        return FAILURE;
    }

    mkstream &= is_create;
    if (!(is_create || is_setid))
        entries_read = -2;

    REDIS_CMD_INIT_SSTR_STATIC(&cmdstr, 1 + nargs + !!mkstream + (entries_read != -2 ? 2 : 0), "XGROUP");
    redis_cmd_append_sstr_zstr(&cmdstr, op);

    if (nargs-- > 0) redis_cmd_append_sstr_key_zstr(&cmdstr, key, redis_sock, slot);
    if (nargs-- > 0) redis_cmd_append_sstr_zstr(&cmdstr, group);
    if (nargs-- > 0) redis_cmd_append_sstr_zstr(&cmdstr, id_or_consumer);

    REDIS_CMD_APPEND_SSTR_OPT_STATIC(&cmdstr, !!mkstream, "MKSTREAM");
    if (entries_read != -2) {
        REDIS_CMD_APPEND_SSTR_STATIC(&cmdstr, "ENTRIESREAD");
        redis_cmd_append_sstr_long(&cmdstr, entries_read);
    }

    *cmd = cmdstr.c;
    *cmd_len = cmdstr.len;

    return SUCCESS;
}

/* XINFO CONSUMERS key group
 * XINFO GROUPS key
 * XINFO STREAM key [FULL [COUNT N]]
 * XINFO HELP */
int redis_xinfo_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock,
                     char **cmd, int *cmd_len, short *slot, void **ctx)
{
    zend_string *op = NULL, *key = NULL, *arg = NULL;
    smart_string cmdstr = {0};
    zend_long count = -1;

    ZEND_PARSE_PARAMETERS_START(1, 4)
        Z_PARAM_STR(op)
        Z_PARAM_OPTIONAL
        Z_PARAM_STR_OR_NULL(key)
        Z_PARAM_STR_OR_NULL(arg)
        Z_PARAM_LONG(count)
    ZEND_PARSE_PARAMETERS_END_EX(return FAILURE);

    if ((arg != NULL && key == NULL) || (count != -1 && (key == NULL || arg == NULL))) {
        php_error_docref(NULL, E_WARNING, "Cannot pass a non-null optional argument after a NULL one.");
        return FAILURE;
    }

    REDIS_CMD_INIT_SSTR_STATIC(&cmdstr, 1 + (key != NULL) + (arg != NULL) + (count > -1 ? 2 : 0), "XINFO");
    redis_cmd_append_sstr_zstr(&cmdstr, op);

    if (key != NULL)
        redis_cmd_append_sstr_key(&cmdstr, ZSTR_VAL(key), ZSTR_LEN(key), redis_sock, slot);
    if (arg != NULL)
        redis_cmd_append_sstr_zstr(&cmdstr, arg);

    if (count > -1) {
        REDIS_CMD_APPEND_SSTR_STATIC(&cmdstr, "COUNT");
        redis_cmd_append_sstr_long(&cmdstr, count);
    }

    *cmd = cmdstr.c;
    *cmd_len = cmdstr.len;
    return SUCCESS;
}

// XTRIM key <MAXLEN | MINID> [= | ~] threshold [LIMIT count]
int redis_xtrim_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock,
                    char **cmd, int *cmd_len, short *slot, void **ctx)
{
    zend_string *key = NULL, *threshold = NULL;
    zend_bool approx = 0, minid = 0;
    smart_string cmdstr = {0};
    zend_long limit = -1;
    int argc;

    ZEND_PARSE_PARAMETERS_START(2, 5)
        Z_PARAM_STR(key)
        Z_PARAM_STR(threshold)
        Z_PARAM_OPTIONAL
        Z_PARAM_BOOL(approx)
        Z_PARAM_BOOL(minid)
        Z_PARAM_LONG(limit)
    ZEND_PARSE_PARAMETERS_END_EX(return FAILURE);

    argc = 4 + (approx && limit > -1 ? 2 : 0);
    REDIS_CMD_INIT_SSTR_STATIC(&cmdstr, argc, "XTRIM");

    redis_cmd_append_sstr_key_zstr(&cmdstr, key, redis_sock, slot);

    if (minid) {
        REDIS_CMD_APPEND_SSTR_STATIC(&cmdstr, "MINID");
    } else {
        REDIS_CMD_APPEND_SSTR_STATIC(&cmdstr, "MAXLEN");
    }

    if (approx) {
        REDIS_CMD_APPEND_SSTR_STATIC(&cmdstr, "~");
    } else {
        REDIS_CMD_APPEND_SSTR_STATIC(&cmdstr, "=");
    }

    redis_cmd_append_sstr_zstr(&cmdstr, threshold);

    if (limit > -1 && approx) {
        REDIS_CMD_APPEND_SSTR_STATIC(&cmdstr, "LIMIT");
        redis_cmd_append_sstr_long(&cmdstr, limit);
    } else if (limit > -1) {
        php_error_docref(NULL, E_WARNING, "Cannot use LIMIT without an approximate match, ignoring");
    } else if (ZEND_NUM_ARGS() == 5) {
        php_error_docref(NULL, E_WARNING, "Limit must be >= 0");
    }

    *cmd = cmdstr.c;
    *cmd_len = cmdstr.len;
    return SUCCESS;
}

// [P]EXPIRE[AT] [NX | XX | GT | LT]
int redis_expire_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock,
                     char *kw, char **cmd, int *cmd_len, short *slot,
                     void **ctx)
{
    zend_string *key = NULL, *mode = NULL;
    smart_string cmdstr = {0};
    zend_long timeout = 0;

    ZEND_PARSE_PARAMETERS_START(2, 3)
        Z_PARAM_STR(key)
        Z_PARAM_LONG(timeout)
        Z_PARAM_OPTIONAL
        Z_PARAM_STR_OR_NULL(mode)
    ZEND_PARSE_PARAMETERS_END_EX(return FAILURE);

    if (mode != NULL && !(zend_string_equals_literal_ci(mode, "NX") ||
                          zend_string_equals_literal_ci(mode, "XX") ||
                          zend_string_equals_literal_ci(mode, "LT") ||
                          zend_string_equals_literal_ci(mode, "GT")))
    {
        php_error_docref(NULL, E_WARNING, "Unknown expiration modifier '%s'", ZSTR_VAL(mode));
        return FAILURE;
    }

    redis_cmd_init_sstr(&cmdstr, 2 + (mode != NULL), kw, strlen(kw));
    redis_cmd_append_sstr_key_zstr(&cmdstr, key, redis_sock, slot);
    redis_cmd_append_sstr_long(&cmdstr, timeout);
    if (mode != NULL) redis_cmd_append_sstr_zstr(&cmdstr, mode);

    *cmd = cmdstr.c;
    *cmd_len = cmdstr.len;

    return SUCCESS;
}

int
redis_sentinel_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock,
                    char *kw, char **cmd, int *cmd_len, short *slot, void **ctx)
{
    if (zend_parse_parameters_none() == FAILURE) {

        return FAILURE;
    }
    *cmd_len = REDIS_CMD_SPPRINTF(cmd, "SENTINEL", "s", kw, strlen(kw));
    return SUCCESS;
}

int
redis_sentinel_str_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock,
                    char *kw, char **cmd, int *cmd_len, short *slot, void **ctx)
{
    zend_string *name;

    if (zend_parse_parameters(ZEND_NUM_ARGS(), "S", &name) == FAILURE) {
        return FAILURE;
    }
    *cmd_len = REDIS_CMD_SPPRINTF(cmd, "SENTINEL", "sS", kw, strlen(kw), name);
    return SUCCESS;
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

    if (zend_parse_parameters(ZEND_NUM_ARGS(), "l", &option)
                              == FAILURE)
    {
        RETURN_FALSE;
    }

    // Return the requested option
    switch(option) {
        case REDIS_OPT_SERIALIZER:
            RETURN_LONG(redis_sock->serializer);
        case REDIS_OPT_COMPRESSION:
            RETURN_LONG(redis_sock->compression);
        case REDIS_OPT_COMPRESSION_LEVEL:
            RETURN_LONG(redis_sock->compression_level);
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

    if (zend_parse_parameters(ZEND_NUM_ARGS(), "lz", &option,
                              &val) == FAILURE)
    {
        RETURN_FALSE;
    }

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
    char *key;
    size_t key_len;

    if (zend_parse_parameters(ZEND_NUM_ARGS(), "s", &key, &key_len)
                             ==FAILURE)
    {
        RETURN_FALSE;
    }

    if (redis_sock->prefix) {
        int keyfree = redis_key_prefix(redis_sock, &key, &key_len);
        RETVAL_STRINGL(key, key_len);
        if (keyfree) efree(key);
    } else {
        RETURN_STRINGL(key, key_len);
    }
}

void redis_serialize_handler(INTERNAL_FUNCTION_PARAMETERS,
                             RedisSock *redis_sock)
{
    zval *z_val;
    char *val;
    size_t val_len;

    if (zend_parse_parameters(ZEND_NUM_ARGS(), "z", &z_val) == FAILURE) {
        RETURN_FALSE;
    }

    int val_free = redis_serialize(redis_sock, z_val, &val, &val_len);

    RETVAL_STRINGL(val, val_len);
    if (val_free) efree(val);
}

void redis_unserialize_handler(INTERNAL_FUNCTION_PARAMETERS,
                               RedisSock *redis_sock, zend_class_entry *ex)
{
    char *value;
    size_t value_len;

    // Parse our arguments
    if (zend_parse_parameters(ZEND_NUM_ARGS(), "s", &value, &value_len)
                                    == FAILURE)
    {
        RETURN_FALSE;
    }

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

    if (zend_parse_parameters(ZEND_NUM_ARGS(), "S", &zstr) == FAILURE) {
        RETURN_FALSE;
    }

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

    if (zend_parse_parameters(ZEND_NUM_ARGS(), "S", &zstr) == FAILURE) {
        RETURN_FALSE;
    } else if (ZSTR_LEN(zstr) == 0 || redis_sock->compression == REDIS_COMPRESSION_NONE) {
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

    if (zend_parse_parameters(ZEND_NUM_ARGS(), "z", &zv) == FAILURE) {
        RETURN_FALSE;
    }

    valfree = redis_pack(redis_sock, zv, &val, &len);
    RETVAL_STRINGL(val, len);
    if (valfree) efree(val);
}

void redis_unpack_handler(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock) {
    zend_string *str;

    if (zend_parse_parameters(ZEND_NUM_ARGS(), "S", &str) == FAILURE) {
        RETURN_FALSE;
    }

    if (redis_unpack(redis_sock, ZSTR_VAL(str), ZSTR_LEN(str), return_value) == 0) {
        RETURN_STR_COPY(str);
    }
}
/* vim: set tabstop=4 softtabstop=4 expandtab shiftwidth=4: */
