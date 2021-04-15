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
    geoSortType sort;
    geoStoreType store;
    zend_string *key;
} geoOptions;

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
    if (!strcasecmp(Z_STRVAL(z_args[0]), "flush") || !strcasecmp(Z_STRVAL(z_args[0]), "kill")) {
        // Simple SCRIPT FLUSH, or SCRIPT_KILL command
        REDIS_CMD_INIT_SSTR_STATIC(cmd, argc, "SCRIPT");
        redis_cmd_append_sstr(cmd, Z_STRVAL(z_args[0]), Z_STRLEN(z_args[0]));
    } else if (!strcasecmp(Z_STRVAL(z_args[0]), "load")) {
        // Make sure we have a second argument, and it's not empty.  If it is
        // empty, we can just return an empty array (which is what Redis does)
        if (argc < 2 || Z_TYPE(z_args[1]) != IS_STRING || Z_STRLEN(z_args[1]) < 1) {
            return NULL;
        }
        // Format our SCRIPT LOAD command
        REDIS_CMD_INIT_SSTR_STATIC(cmd, argc, "SCRIPT");
        redis_cmd_append_sstr(cmd, "LOAD", sizeof("LOAD") - 1);
        redis_cmd_append_sstr(cmd, Z_STRVAL(z_args[1]), Z_STRLEN(z_args[1]));
    } else if (!strcasecmp(Z_STRVAL(z_args[0]), "exists")) {
        // Make sure we have a second argument
        if (argc < 2) {
            return NULL;
        }
        /* Construct our SCRIPT EXISTS command */
        REDIS_CMD_INIT_SSTR_STATIC(cmd, argc, "SCRIPT");
        redis_cmd_append_sstr(cmd, "EXISTS", sizeof("EXISTS") - 1);

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
    char *k1, *k2;
    size_t k1len, k2len;
    int k1free, k2free;

    if (zend_parse_parameters(ZEND_NUM_ARGS(), "ss", &k1, &k1len,
                             &k2, &k2len) == FAILURE)
    {
        return FAILURE;
    }

    // Prefix both keys
    k1free = redis_key_prefix(redis_sock, &k1, &k1len);
    k2free = redis_key_prefix(redis_sock, &k2, &k2len);

    // If a slot is requested, we can test that they hash the same
    if (slot) {
        // Slots where these keys resolve
        short slot1 = cluster_hash_key(k1, k1len);
        short slot2 = cluster_hash_key(k2, k2len);

        // Check if Redis would give us a CROSSLOT error
        if (slot1 != slot2) {
            php_error_docref(0, E_WARNING, "Keys don't hash to the same slot");
            if (k1free) efree(k1);
            if (k2free) efree(k2);
            return FAILURE;
        }

        // They're both the same
        *slot = slot1;
    }

    /* Send keys as normal strings because we manually prefixed to check against
     * cross slot error. */
    *cmd_len = REDIS_CMD_SPPRINTF(cmd, kw, "ss", k1, k1len, k2, k2len);

    /* Clean keys up if we prefixed */
    if (k1free) efree(k1);
    if (k2free) efree(k2);

    return SUCCESS;
}

/* Generic command construction where we take a key and a long */
int redis_key_long_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock,
                       char *kw, char **cmd, int *cmd_len, short *slot,
                       void **ctx)
{
    char *key;
    size_t keylen;
    zend_long lval;

    if (zend_parse_parameters(ZEND_NUM_ARGS(), "sl", &key, &keylen, &lval)
                              ==FAILURE)
    {
        return FAILURE;
    }

    *cmd_len = REDIS_CMD_SPPRINTF(cmd, kw, "kl", key, keylen, lval);

    // Success!
    return SUCCESS;
}

/* long, long */
int redis_long_long_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock,
                        char *kw, char **cmd, int *cmd_len, short *slot,
                        void **ctx)
{
    zend_long v1, v2;

    if (zend_parse_parameters(ZEND_NUM_ARGS(), "ll", &v1, &v2)
                              == FAILURE)
    {
        return FAILURE;
    }

    *cmd_len = REDIS_CMD_SPPRINTF(cmd, kw, "ll", v1, v2);

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

int redis_flush_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock,
               char *kw, char **cmd, int *cmd_len, short *slot, void **ctx)
{
    zend_bool async = 0;

    if (zend_parse_parameters(ZEND_NUM_ARGS(), "|b", &async) == FAILURE) {
        return FAILURE;
    }

    if (async) {
        *cmd_len = REDIS_CMD_SPPRINTF(cmd, kw, "s", "ASYNC", sizeof("ASYNC") - 1);
    } else {
        *cmd_len = REDIS_CMD_SPPRINTF(cmd, kw, "");
    }

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
        redis_cmd_append_sstr(&cmdstr,"COUNT",sizeof("COUNT")-1);
        redis_cmd_append_sstr_long(&cmdstr, count);
    }

    // Append pattern if we've got one
    if (pat_len) {
        redis_cmd_append_sstr(&cmdstr,"MATCH",sizeof("MATCH")-1);
        redis_cmd_append_sstr(&cmdstr,pat,pat_len);
    }

    // Push command to the caller, return length
    *cmd = cmdstr.c;
    return cmdstr.len;
}

/* ZRANGE/ZREVRANGE */
int redis_zrange_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock,
                     char *kw, char **cmd, int *cmd_len, int *withscores,
                     short *slot, void **ctx)
{
    char *key;
    size_t key_len;
    zend_long start, end;
    zend_string *zkey;
    zval *z_ws = NULL, *z_ele;

    if (zend_parse_parameters(ZEND_NUM_ARGS(), "sll|z", &key, &key_len,
                             &start, &end, &z_ws) == FAILURE)
    {
        return FAILURE;
    }

    // Clear withscores arg
    *withscores = 0;

    /* Accept ['withscores' => true], or the legacy `true` value */
    if (z_ws) {
        if (Z_TYPE_P(z_ws) == IS_ARRAY) {
            ZEND_HASH_FOREACH_STR_KEY_VAL(Z_ARRVAL_P(z_ws), zkey, z_ele) {
                ZVAL_DEREF(z_ele);
                if (zend_string_equals_literal_ci(zkey, "withscores")) {
                    *withscores = zval_is_true(z_ele);
                    break;
                }
            } ZEND_HASH_FOREACH_END();
        } else if (Z_TYPE_P(z_ws) == IS_TRUE) {
            *withscores = Z_TYPE_P(z_ws) == IS_TRUE;
        }
    }

    if (*withscores) {
        *cmd_len = REDIS_CMD_SPPRINTF(cmd, kw, "kdds", key, key_len, start, end,
            "WITHSCORES", sizeof("WITHSCORES") - 1);
    } else {
        *cmd_len = REDIS_CMD_SPPRINTF(cmd, kw, "kdd", key, key_len, start, end);
    }

    return SUCCESS;
}

int redis_zrangebyscore_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock,
                            char *kw, char **cmd, int *cmd_len, int *withscores,
                            short *slot, void **ctx)
{
    char *key, *start, *end;
    int has_limit = 0;
    long offset, count;
    size_t key_len, start_len, end_len;
    zval *z_opt=NULL, *z_ele;
    zend_string *zkey;
    zend_ulong idx;
    HashTable *ht_opt;

    PHPREDIS_NOTUSED(idx);

    if (zend_parse_parameters(ZEND_NUM_ARGS(), "sss|a", &key, &key_len,
                             &start, &start_len, &end, &end_len, &z_opt)
                             ==FAILURE)
    {
        return FAILURE;
    }

    // Check for an options array
    if (z_opt && Z_TYPE_P(z_opt) == IS_ARRAY) {
        ht_opt = Z_ARRVAL_P(z_opt);
        ZEND_HASH_FOREACH_KEY_VAL(ht_opt, idx, zkey, z_ele) {
           /* All options require a string key type */
           if (!zkey) continue;
           ZVAL_DEREF(z_ele);
           /* Check for withscores and limit */
           if (zend_string_equals_literal_ci(zkey, "withscores")) {
               *withscores = zval_is_true(z_ele);
           } else if (zend_string_equals_literal_ci(zkey, "limit") && Z_TYPE_P(z_ele) == IS_ARRAY) {
                HashTable *htlimit = Z_ARRVAL_P(z_ele);
                zval *zoff, *zcnt;

                /* We need two arguments (offset and count) */
                if ((zoff = zend_hash_index_find(htlimit, 0)) != NULL &&
                    (zcnt = zend_hash_index_find(htlimit, 1)) != NULL
                ) {
                    /* Set our limit if we can get valid longs from both args */
                    offset = zval_get_long(zoff);
                    count = zval_get_long(zcnt);
                    has_limit = 1;
                }
           }
        } ZEND_HASH_FOREACH_END();
    }

    // Construct our command
    if (*withscores) {
        if (has_limit) {
            *cmd_len = REDIS_CMD_SPPRINTF(cmd, kw, "ksssdds", key, key_len,
                start, start_len, end, end_len, "LIMIT", 5, offset, count,
                "WITHSCORES", 10);
        } else {
            *cmd_len = REDIS_CMD_SPPRINTF(cmd, kw, "ksss", key, key_len, start,
                start_len, end, end_len, "WITHSCORES", 10);
        }
    } else {
        if (has_limit) {
            *cmd_len = REDIS_CMD_SPPRINTF(cmd, kw, "ksssdd", key, key_len, start,
                start_len, end, end_len, "LIMIT", 5, offset, count);
        } else {
            *cmd_len = REDIS_CMD_SPPRINTF(cmd, kw, "kss", key, key_len, start,
                start_len, end, end_len);
        }
    }

    return SUCCESS;
}
int
redis_zdiff_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock,
                char **cmd, int *cmd_len, short *slot, void **ctx)
{
    int numkeys;
    smart_string cmdstr = {0};
    zval *z_keys, *z_opts = NULL, *z_ele;
    zend_bool withscores = 0;

    if (zend_parse_parameters(ZEND_NUM_ARGS(), "a|a",
                              &z_keys, &z_opts) == FAILURE)
    {
        return FAILURE;
    }

    if ((numkeys = zend_hash_num_elements(Z_ARRVAL_P(z_keys))) == 0) {
        return FAILURE;
    }

    if (z_opts && Z_TYPE_P(z_opts) == IS_ARRAY) {
        zend_ulong idx;
        zend_string *zkey;
        ZEND_HASH_FOREACH_KEY_VAL(Z_ARRVAL_P(z_opts), idx, zkey, z_ele) {
            if (zkey != NULL) {
                ZVAL_DEREF(z_ele);
                if (zend_string_equals_literal_ci(zkey, "withscores")) {
                    withscores = zval_is_true(z_ele);
                }
            }
        } ZEND_HASH_FOREACH_END();
    }

    REDIS_CMD_INIT_SSTR_STATIC(&cmdstr, 1 + numkeys + withscores, "ZDIFF");
    redis_cmd_append_sstr_long(&cmdstr, numkeys);

    ZEND_HASH_FOREACH_VAL(Z_ARRVAL_P(z_keys), z_ele) {
        ZVAL_DEREF(z_ele);
        redis_cmd_append_sstr_zval(&cmdstr, z_ele, redis_sock);
    } ZEND_HASH_FOREACH_END();

    if (withscores) {
        REDIS_CMD_APPEND_SSTR_STATIC(&cmdstr, "WITHSCORES");
        *ctx = PHPREDIS_CTX_PTR;
    }

    *cmd = cmdstr.c;
    *cmd_len = cmdstr.len;
    return SUCCESS;
}

int
redis_zinterunion_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock,
                      char *kw, char **cmd, int *cmd_len, short *slot,
                      void **ctx)
{
    int numkeys;
    smart_string cmdstr = {0};
    zval *z_keys, *z_weights = NULL, *z_opts = NULL, *z_ele;
    zend_string *aggregate = NULL;
    zend_bool withscores = 0;

    if (zend_parse_parameters(ZEND_NUM_ARGS(), "a|a!a",
                             &z_keys, &z_weights, &z_opts) == FAILURE)
    {
        return FAILURE;
    }

    if ((numkeys = zend_hash_num_elements(Z_ARRVAL_P(z_keys))) == 0) {
        return FAILURE;
    }

    if (z_weights) {
        if (zend_hash_num_elements(Z_ARRVAL_P(z_weights)) != numkeys) {
            php_error_docref(NULL, E_WARNING,
                "WEIGHTS and keys array should be the same size!");
            return FAILURE;
        }
    }

    if (z_opts && Z_TYPE_P(z_opts) == IS_ARRAY) {
        zend_ulong idx;
        zend_string *zkey;
        ZEND_HASH_FOREACH_KEY_VAL(Z_ARRVAL_P(z_opts), idx, zkey, z_ele) {
            if (zkey != NULL) {
                ZVAL_DEREF(z_ele);
                if (zend_string_equals_literal_ci(zkey, "aggregate")) {
                    aggregate = zval_get_string(z_ele);
                    if (!zend_string_equals_literal_ci(aggregate, "sum") &&
                        !zend_string_equals_literal_ci(aggregate, "min") &&
                        !zend_string_equals_literal_ci(aggregate, "max")
                    ) {
                        php_error_docref(NULL, E_WARNING,
                            "Invalid AGGREGATE option provided!");
                        zend_string_release(aggregate);
                        return FAILURE;
                    }
                } else if (zend_string_equals_literal_ci(zkey, "withscores")) {
                    withscores = zval_is_true(z_ele);
                }
            }
        } ZEND_HASH_FOREACH_END();
    }

    redis_cmd_init_sstr(&cmdstr, 1 + numkeys + (z_weights ? 1 + numkeys : 0) + (aggregate ? 2 : 0) + withscores, kw, strlen(kw));
    redis_cmd_append_sstr_long(&cmdstr, numkeys);


    ZEND_HASH_FOREACH_VAL(Z_ARRVAL_P(z_keys), z_ele) {
        ZVAL_DEREF(z_ele);
        redis_cmd_append_sstr_zval(&cmdstr, z_ele, redis_sock);
    } ZEND_HASH_FOREACH_END();

    if (z_weights) {
        REDIS_CMD_APPEND_SSTR_STATIC(&cmdstr, "WEIGHTS");
        ZEND_HASH_FOREACH_VAL(Z_ARRVAL_P(z_weights), z_ele) {
            ZVAL_DEREF(z_ele);
            switch (Z_TYPE_P(z_ele)) {
                case IS_LONG:
                    redis_cmd_append_sstr_long(&cmdstr, Z_LVAL_P(z_ele));
                    break;
                case IS_DOUBLE:
                    redis_cmd_append_sstr_dbl(&cmdstr, Z_DVAL_P(z_ele));
                    break;
                case IS_STRING: {
                    double dval;
                    zend_long lval;
                    zend_uchar type = is_numeric_string(Z_STRVAL_P(z_ele), Z_STRLEN_P(z_ele), &lval, &dval, 0);
                    if (type == IS_LONG) {
                        redis_cmd_append_sstr_long(&cmdstr, lval);
                        break;
                    } else if (type == IS_DOUBLE) {
                        redis_cmd_append_sstr_dbl(&cmdstr, dval);
                        break;
                    } else if (strncasecmp(Z_STRVAL_P(z_ele), "-inf", sizeof("-inf") - 1) == 0 ||
                               strncasecmp(Z_STRVAL_P(z_ele), "+inf", sizeof("+inf") - 1) == 0 ||
                               strncasecmp(Z_STRVAL_P(z_ele), "inf", sizeof("inf") - 1) == 0
                    ) {
                        redis_cmd_append_sstr(&cmdstr, Z_STRVAL_P(z_ele), Z_STRLEN_P(z_ele));
                        break;
                    }
                    // fall through
                }
                default:
                    php_error_docref(NULL, E_WARNING,
                        "Weights must be numeric or '-inf','inf','+inf'");
                    if (aggregate) zend_string_release(aggregate);
                    efree(cmdstr.c);
                    return FAILURE;
            }
        } ZEND_HASH_FOREACH_END();
    }

    if (aggregate) {
        REDIS_CMD_APPEND_SSTR_STATIC(&cmdstr, "AGGREGATE");
        redis_cmd_append_sstr_zstr(&cmdstr, aggregate);
        zend_string_release(aggregate);
    }

    if (withscores) {
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
    char *dst;
    size_t dst_len;
    int numkeys;
    zval *z_keys, *z_ele;
    smart_string cmdstr = {0};

    if (zend_parse_parameters(ZEND_NUM_ARGS(), "sa",
                              &dst, &dst_len, &z_keys) == FAILURE)
    {
        return FAILURE;
    }

    if ((numkeys = zend_hash_num_elements(Z_ARRVAL_P(z_keys))) == 0) {
        return FAILURE;
    }

    REDIS_CMD_INIT_SSTR_STATIC(&cmdstr, 2 + numkeys, "ZDIFFSTORE");
    redis_cmd_append_sstr(&cmdstr, dst, dst_len);
    redis_cmd_append_sstr_long(&cmdstr, numkeys);

    ZEND_HASH_FOREACH_VAL(Z_ARRVAL_P(z_keys), z_ele) {
        ZVAL_DEREF(z_ele);
        redis_cmd_append_sstr_zval(&cmdstr, z_ele, redis_sock);
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
    char *key, *agg_op=NULL;
    int key_free, argc = 2, keys_count;
    size_t key_len, agg_op_len = 0;
    zval *z_keys, *z_weights=NULL, *z_ele;
    HashTable *ht_keys, *ht_weights=NULL;
    smart_string cmdstr = {0};

    // Parse args
    if (zend_parse_parameters(ZEND_NUM_ARGS(), "sa|a!s", &key,
                             &key_len, &z_keys, &z_weights, &agg_op,
                             &agg_op_len) == FAILURE)
    {
        return FAILURE;
    }

    // Grab our keys
    ht_keys = Z_ARRVAL_P(z_keys);

    // Nothing to do if there aren't any
    if ((keys_count = zend_hash_num_elements(ht_keys)) == 0) {
        return FAILURE;
    } else {
        argc += keys_count;
    }

    // Handle WEIGHTS
    if (z_weights != NULL) {
        ht_weights = Z_ARRVAL_P(z_weights);
        if (zend_hash_num_elements(ht_weights) != keys_count) {
            php_error_docref(NULL, E_WARNING,
                "WEIGHTS and keys array should be the same size!");
            return FAILURE;
        }

        // "WEIGHTS" + key count
        argc += keys_count + 1;
    }

    // AGGREGATE option
    if (agg_op_len != 0) {
        if (strncasecmp(agg_op, "SUM", sizeof("SUM")) &&
           strncasecmp(agg_op, "MIN", sizeof("MIN")) &&
           strncasecmp(agg_op, "MAX", sizeof("MAX")))
        {
            php_error_docref(NULL, E_WARNING,
                "Invalid AGGREGATE option provided!");
            return FAILURE;
        }

        // "AGGREGATE" + type
        argc += 2;
    }

    // Prefix key
    key_free = redis_key_prefix(redis_sock, &key, &key_len);

    // Start building our command
    redis_cmd_init_sstr(&cmdstr, argc, kw, strlen(kw));
    redis_cmd_append_sstr(&cmdstr, key, key_len);
    redis_cmd_append_sstr_int(&cmdstr, keys_count);

    // Set our slot, free the key if we prefixed it
    CMD_SET_SLOT(slot,key,key_len);
    if (key_free) efree(key);

    // Process input keys
    ZEND_HASH_FOREACH_VAL(ht_keys, z_ele) {
        zend_string *zstr = zval_get_string(z_ele);
        char *key = ZSTR_VAL(zstr);
        size_t key_len = ZSTR_LEN(zstr);

        // Prefix key if necissary
        int key_free = redis_key_prefix(redis_sock, &key, &key_len);

        // If we're in Cluster mode, verify the slot is the same
        if (slot && *slot != cluster_hash_key(key,key_len)) {
            php_error_docref(NULL, E_WARNING,
                "All keys don't hash to the same slot!");
            efree(cmdstr.c);
            zend_string_release(zstr);
            if (key_free) efree(key);
            return FAILURE;
        }

        // Append this input set
        redis_cmd_append_sstr(&cmdstr, key, key_len);

        // Cleanup
        zend_string_release(zstr);
        if (key_free) efree(key);
    } ZEND_HASH_FOREACH_END();

    // Weights
    if (ht_weights != NULL) {
        redis_cmd_append_sstr(&cmdstr, "WEIGHTS", sizeof("WEIGHTS")-1);

        // Process our weights
        ZEND_HASH_FOREACH_VAL(ht_weights, z_ele) {
            // Ignore non numeric args unless they're inf/-inf
            ZVAL_DEREF(z_ele);
            switch (Z_TYPE_P(z_ele)) {
                case IS_LONG:
                    redis_cmd_append_sstr_long(&cmdstr, Z_LVAL_P(z_ele));
                    break;
                case IS_DOUBLE:
                    redis_cmd_append_sstr_dbl(&cmdstr, Z_DVAL_P(z_ele));
                    break;
                case IS_STRING: {
                    double dval;
                    zend_long lval;
                    zend_uchar type = is_numeric_string(Z_STRVAL_P(z_ele), Z_STRLEN_P(z_ele), &lval, &dval, 0);
                    if (type == IS_LONG) {
                        redis_cmd_append_sstr_long(&cmdstr, lval);
                        break;
                    } else if (type == IS_DOUBLE) {
                        redis_cmd_append_sstr_dbl(&cmdstr, dval);
                        break;
                    } else if (strncasecmp(Z_STRVAL_P(z_ele), "-inf", sizeof("-inf") - 1) == 0 ||
                               strncasecmp(Z_STRVAL_P(z_ele), "+inf", sizeof("+inf") - 1) == 0 ||
                               strncasecmp(Z_STRVAL_P(z_ele), "inf", sizeof("inf") - 1) == 0
                    ) {
                        redis_cmd_append_sstr(&cmdstr, Z_STRVAL_P(z_ele), Z_STRLEN_P(z_ele));
                        break;
                    }
                    // fall through
                }
                default:
                    php_error_docref(NULL, E_WARNING,
                        "Weights must be numeric or '-inf','inf','+inf'");
                    efree(cmdstr.c);
                    return FAILURE;
            }
        } ZEND_HASH_FOREACH_END();
    }

    // AGGREGATE
    if (agg_op_len != 0) {
        redis_cmd_append_sstr(&cmdstr, "AGGREGATE", sizeof("AGGREGATE")-1);
        redis_cmd_append_sstr(&cmdstr, agg_op, agg_op_len);
    }

    // Push out values
    *cmd     = cmdstr.c;
    *cmd_len = cmdstr.len;

    return SUCCESS;
}

/* SUBSCRIBE/PSUBSCRIBE */
int redis_subscribe_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock,
                        char *kw, char **cmd, int *cmd_len, short *slot,
                        void **ctx)
{
    zval *z_arr, *z_chan;
    HashTable *ht_chan;
    smart_string cmdstr = {0};
    subscribeContext *sctx = ecalloc(1, sizeof(*sctx));
    size_t key_len;
    int key_free;
    char *key;

    if (zend_parse_parameters(ZEND_NUM_ARGS(), "af", &z_arr,
                             &(sctx->cb), &(sctx->cb_cache)) == FAILURE)
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

    // Start command construction
    redis_cmd_init_sstr(&cmdstr, sctx->argc, kw, strlen(kw));

    // Iterate over channels
    ZEND_HASH_FOREACH_VAL(ht_chan, z_chan) {
        // We want to deal with strings here
        zend_string *zstr = zval_get_string(z_chan);

        // Grab channel name, prefix if required
        key = ZSTR_VAL(zstr);
        key_len = ZSTR_LEN(zstr);
        key_free = redis_key_prefix(redis_sock, &key, &key_len);

        // Add this channel
        redis_cmd_append_sstr(&cmdstr, key, key_len);

        zend_string_release(zstr);
        // Free our key if it was prefixed
        if (key_free) efree(key);
    } ZEND_HASH_FOREACH_END();

    // Push values out
    *cmd_len = cmdstr.len;
    *cmd     = cmdstr.c;
    *ctx     = (void*)sctx;

    // Pick a slot at random
    CMD_RAND_SLOT(slot);

    return SUCCESS;
}

/* UNSUBSCRIBE/PUNSUBSCRIBE */
int redis_unsubscribe_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock,
                          char *kw, char **cmd, int *cmd_len, short *slot,
                          void **ctx)
{
    zval *z_arr, *z_chan;
    HashTable *ht_arr;
    smart_string cmdstr = {0};
    subscribeContext *sctx = ecalloc(1, sizeof(*sctx));

    if (zend_parse_parameters(ZEND_NUM_ARGS(), "a", &z_arr) == FAILURE) {
        efree(sctx);
        return FAILURE;
    }

    ht_arr = Z_ARRVAL_P(z_arr);

    sctx->argc = zend_hash_num_elements(ht_arr);
    if (sctx->argc == 0) {
        efree(sctx);
        return FAILURE;
    }

    redis_cmd_init_sstr(&cmdstr, sctx->argc, kw, strlen(kw));

    ZEND_HASH_FOREACH_VAL(ht_arr, z_chan) {
        char *key = Z_STRVAL_P(z_chan);
        size_t key_len = Z_STRLEN_P(z_chan);
        int key_free;

        key_free = redis_key_prefix(redis_sock, &key, &key_len);
        redis_cmd_append_sstr(&cmdstr, key, key_len);
        if (key_free) efree(key);
    } ZEND_HASH_FOREACH_END();

    // Push out vals
    *cmd_len = cmdstr.len;
    *cmd     = cmdstr.c;
    *ctx     = (void*)sctx;

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
    if (min_len < 1 || max_len < 1 ||
       (min[0] != '(' && min[0] != '[' &&
       (min[0] != '-' || min_len > 1) && (min[0] != '+' || min_len > 1)) ||
       (max[0] != '(' && max[0] != '[' &&
       (max[0] != '-' || max_len > 1) && (max[0] != '+' || max_len > 1)))
    {
        php_error_docref(0, E_WARNING,
            "min and max arguments must start with '[' or '('");
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

/* Validate ZLEX* min/max argument strings */
static int validate_zlex_arg(const char *arg, size_t len) {
    return (len  > 1 && (*arg == '[' || *arg == '(')) ||
           (len == 1 && (*arg == '+' || *arg == '-'));
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
#define VAL_TYPE_VALUES 1
#define VAL_TYPE_STRINGS 2
static int gen_key_arr_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock,
                           char *kw, int valtype, char **cmd, int *cmd_len,
                           short *slot, void **ctx)
{
    zval *z_arr, *z_val;
    HashTable *ht_arr;
    smart_string cmdstr = {0};
    zend_string *zstr;
    int key_free, val_free, argc = 1;
    size_t val_len, key_len;
    char *key, *val;

    if (zend_parse_parameters(ZEND_NUM_ARGS(), "sa", &key, &key_len,
                              &z_arr) == FAILURE ||
                              zend_hash_num_elements(Z_ARRVAL_P(z_arr)) == 0)
    {
        return FAILURE;
    }

    /* Start constructing our command */
    ht_arr = Z_ARRVAL_P(z_arr);
    argc += zend_hash_num_elements(ht_arr);
    redis_cmd_init_sstr(&cmdstr, argc, kw, strlen(kw));

    /* Prefix if required and append the key name */
    key_free = redis_key_prefix(redis_sock, &key, &key_len);
    redis_cmd_append_sstr(&cmdstr, key, key_len);
    CMD_SET_SLOT(slot, key, key_len);
    if (key_free) efree(key);

    /* Iterate our hash table, serializing and appending values */
    assert(valtype == VAL_TYPE_VALUES || valtype == VAL_TYPE_STRINGS);
    ZEND_HASH_FOREACH_VAL(ht_arr, z_val) {
        if (valtype == VAL_TYPE_VALUES) {
            val_free = redis_pack(redis_sock, z_val, &val, &val_len);
            redis_cmd_append_sstr(&cmdstr, val, val_len);
            if (val_free) efree(val);
        } else {
            zstr = zval_get_string(z_val);
            redis_cmd_append_sstr(&cmdstr, ZSTR_VAL(zstr), ZSTR_LEN(zstr));
            zend_string_release(zstr);
        }
    } ZEND_HASH_FOREACH_END();

    *cmd_len = cmdstr.len;
    *cmd = cmdstr.c;

    return SUCCESS;
}

int redis_key_val_arr_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock,
                        char *kw, char **cmd, int *cmd_len, short *slot,
                        void **ctx)
{
    return gen_key_arr_cmd(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock, kw,
        VAL_TYPE_VALUES, cmd, cmd_len, slot, ctx);
}

int redis_key_str_arr_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock,
                        char *kw, char **cmd, int *cmd_len, short *slot,
                        void **ctx)
{
    return gen_key_arr_cmd(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock, kw,
        VAL_TYPE_STRINGS, cmd, cmd_len, slot, ctx);
}

/* Generic function that takes a variable number of keys, with an optional
 * timeout value.  This can handle various SUNION/SUNIONSTORE/BRPOP type
 * commands. */
static int gen_varkey_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock,
                          char *kw, int kw_len, int min_argc, int has_timeout,
                          char **cmd, int *cmd_len, short *slot)
{
    zval *z_args, *z_ele;
    HashTable *ht_arr;
    char *key;
    int key_free, i, tail;
    size_t key_len;
    int single_array = 0, argc = ZEND_NUM_ARGS();
    smart_string cmdstr = {0};
    long timeout = 0;
    short kslot = -1;
    zend_string *zstr;

    if (argc < min_argc) {
        zend_wrong_param_count();
        return FAILURE;
    }

    // Allocate args
    z_args = emalloc(argc * sizeof(zval));
    if (zend_get_parameters_array(ht, argc, z_args) == FAILURE) {
        efree(z_args);
        return FAILURE;
    }

    // Handle our "single array" case
    if (has_timeout == 0) {
        single_array = argc==1 && Z_TYPE(z_args[0]) == IS_ARRAY;
    } else {
        single_array = argc==2 && Z_TYPE(z_args[0]) == IS_ARRAY &&
            Z_TYPE(z_args[1]) == IS_LONG;
        timeout = Z_LVAL(z_args[1]);
    }

    // If we're running a single array, rework args
    if (single_array) {
        ht_arr = Z_ARRVAL(z_args[0]);
        argc = zend_hash_num_elements(ht_arr);
        if (has_timeout) argc++;
        efree(z_args);
        z_args = NULL;

        /* If the array is empty, we can simply abort */
        if (argc == 0) return FAILURE;
    }

    // Begin construction of our command
    redis_cmd_init_sstr(&cmdstr, argc, kw, kw_len);

    if (single_array) {
        ZEND_HASH_FOREACH_VAL(ht_arr, z_ele) {
            zstr = zval_get_string(z_ele);
            key = ZSTR_VAL(zstr);
            key_len = ZSTR_LEN(zstr);
            key_free = redis_key_prefix(redis_sock, &key, &key_len);

            // Protect against CROSSLOT errors
            if (slot) {
                if (kslot == -1) {
                    kslot = cluster_hash_key(key, key_len);
                } else if (cluster_hash_key(key,key_len)!=kslot) {
                    zend_string_release(zstr);
                    if (key_free) efree(key);
                    php_error_docref(NULL, E_WARNING,
                        "Not all keys hash to the same slot!");
                    return FAILURE;
                }
            }

            // Append this key, free it if we prefixed
            redis_cmd_append_sstr(&cmdstr, key, key_len);
            zend_string_release(zstr);
            if (key_free) efree(key);
        } ZEND_HASH_FOREACH_END();
        if (has_timeout) {
            redis_cmd_append_sstr_long(&cmdstr, timeout);
        }
    } else {
        if (has_timeout && Z_TYPE(z_args[argc-1])!=IS_LONG) {
            php_error_docref(NULL, E_ERROR,
                "Timeout value must be a LONG");
            efree(z_args);
            return FAILURE;
        }

        tail = has_timeout ? argc-1 : argc;
        for(i = 0; i < tail; i++) {
            zstr = zval_get_string(&z_args[i]);
            key = ZSTR_VAL(zstr);
            key_len = ZSTR_LEN(zstr);

            key_free = redis_key_prefix(redis_sock, &key, &key_len);

            /* Protect against CROSSSLOT errors if we've got a slot */
            if (slot) {
                if ( kslot == -1) {
                    kslot = cluster_hash_key(key, key_len);
                } else if (cluster_hash_key(key,key_len)!=kslot) {
                    php_error_docref(NULL, E_WARNING,
                        "Not all keys hash to the same slot");
                    zend_string_release(zstr);
                    if (key_free) efree(key);
                    efree(z_args);
                    return FAILURE;
                }
            }

            // Append this key
            redis_cmd_append_sstr(&cmdstr, key, key_len);
            zend_string_release(zstr);
            if (key_free) efree(key);
        }
        if (has_timeout) {
            redis_cmd_append_sstr_long(&cmdstr, Z_LVAL(z_args[tail]));
        }

        // Cleanup args
        efree(z_args);
    }

    // Push out parameters
    if (slot) *slot = kslot;
    *cmd = cmdstr.c;
    *cmd_len = cmdstr.len;

    return SUCCESS;
}

/* Generic handling of every blocking pop command (BLPOP, BZPOP[MIN/MAX], etc */
int redis_blocking_pop_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock,
                           char *kw, char **cmd, int *cmd_len, short *slot,
                           void **ctx)
{
    return gen_varkey_cmd(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock, kw,
        strlen(kw), 2, 2, cmd, cmd_len, slot);
}

/*
 * Commands with specific signatures or that need unique functions because they
 * have specific processing (argument validation, etc) that make them unique
 */

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
            /* fallthrough */
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
        zend_ulong idx;
        zval *v;

        PHPREDIS_NOTUSED(idx);

        /* Iterate our option array */
        ZEND_HASH_FOREACH_KEY_VAL(kt, idx, zkey, v) {
            ZVAL_DEREF(v);
            /* Detect PX or EX argument and validate timeout */
            if (zkey && ZSTR_IS_EX_PX_ARG(zkey)) {
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
                if (ZVAL_STRICMP_STATIC(v, "KEEPTTL")) {
                    keep_ttl  = 1;
                } else if (ZVAL_STRICMP_STATIC((v), "GET")) {
                    get = 1;
                } else if (ZVAL_IS_NX_XX_ARG(v)) {
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

/* BRPOPLPUSH */
int redis_brpoplpush_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock,
                         char **cmd, int *cmd_len, short *slot, void **ctx)
{
    char *key1, *key2;
    size_t key1_len, key2_len;
    int key1_free, key2_free;
    short slot1, slot2;
    zend_long timeout;

    if (zend_parse_parameters(ZEND_NUM_ARGS(), "ssl", &key1, &key1_len,
                             &key2, &key2_len, &timeout) == FAILURE)
    {
        return FAILURE;
    }

    // Key prefixing
    key1_free = redis_key_prefix(redis_sock, &key1, &key1_len);
    key2_free = redis_key_prefix(redis_sock, &key2, &key2_len);

    // In cluster mode, verify the slots match
    if (slot) {
        slot1 = cluster_hash_key(key1, key1_len);
        slot2 = cluster_hash_key(key2, key2_len);
        if (slot1 != slot2) {
            php_error_docref(NULL, E_WARNING,
               "Keys hash to different slots!");
            if (key1_free) efree(key1);
            if (key2_free) efree(key2);
            return FAILURE;
        }

        // Both slots are the same
        *slot = slot1;
    }

    // Consistency with Redis, if timeout < 0 use RPOPLPUSH
    if (timeout < 0) {
        *cmd_len = REDIS_CMD_SPPRINTF(cmd, "RPOPLPUSH", "ss", key1, key1_len,
                                     key2, key2_len);
    } else {
        *cmd_len = REDIS_CMD_SPPRINTF(cmd, "BRPOPLPUSH", "ssd", key1, key1_len,
                                     key2, key2_len, timeout);
    }

    if (key1_free) efree(key1);
    if (key2_free) efree(key2);
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
    char *key;
    zval *z_arr, *z_mems, *z_mem;
    int i, count, valid = 0, key_free;
    size_t key_len;
    HashTable *ht_arr;
    smart_string cmdstr = {0};

    // Parse arguments
    if (zend_parse_parameters(ZEND_NUM_ARGS(), "sa", &key, &key_len,
                             &z_arr) == FAILURE)
    {
        return FAILURE;
    }

    // Our HashTable
    ht_arr = Z_ARRVAL_P(z_arr);

    // We can abort if we have no elements
    if ((count = zend_hash_num_elements(ht_arr)) == 0) {
        return FAILURE;
    }

    // Allocate memory for mems+1 so we can have a sentinel
    z_mems = ecalloc(count + 1, sizeof(zval));

    // Iterate over our member array
    ZEND_HASH_FOREACH_VAL(ht_arr, z_mem) {
        ZVAL_DEREF(z_mem);
        // We can only handle string or long values here
        if ((Z_TYPE_P(z_mem) == IS_STRING && Z_STRLEN_P(z_mem) > 0)
            || Z_TYPE_P(z_mem) == IS_LONG
        ) {
            // Copy into our member array
            ZVAL_ZVAL(&z_mems[valid], z_mem, 1, 0);

            // Increment the member count to actually send
            valid++;
        }
    } ZEND_HASH_FOREACH_END();

    // If nothing was valid, fail
    if (valid == 0) {
        efree(z_mems);
        return FAILURE;
    }

    // Sentinel so we can free this even if it's used and then we discard
    // the transaction manually or there is a transaction failure
    ZVAL_NULL(&z_mems[valid]);

    // Start command construction
    redis_cmd_init_sstr(&cmdstr, valid+1, "HMGET", sizeof("HMGET")-1);

    // Prefix our key
    key_free = redis_key_prefix(redis_sock, &key, &key_len);

    redis_cmd_append_sstr(&cmdstr, key, key_len);

    // Iterate over members, appending as arguments
    for(i = 0; i< valid; i++) {
        zend_string *zstr = zval_get_string(&z_mems[i]);
        redis_cmd_append_sstr(&cmdstr, ZSTR_VAL(zstr), ZSTR_LEN(zstr));
        zend_string_release(zstr);
    }

    // Set our slot
    CMD_SET_SLOT(slot,key,key_len);

    // Free our key if we prefixed it
    if (key_free) efree(key);

    // Push out command, length, and key context
    *cmd     = cmdstr.c;
    *cmd_len = cmdstr.len;
    *ctx     = (void*)z_mems;

    // Success!
    return SUCCESS;
}

/* HMSET */
int redis_hmset_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock,
                    char **cmd, int *cmd_len, short *slot, void **ctx)
{
    char *key;
    int key_free, count;
    size_t key_len;
    zend_ulong idx;
    zval *z_arr;
    HashTable *ht_vals;
    smart_string cmdstr = {0};
    zend_string *zkey;
    zval *z_val;

    // Parse args
    if (zend_parse_parameters(ZEND_NUM_ARGS(), "sa", &key, &key_len,
                             &z_arr) == FAILURE)
    {
        return FAILURE;
    }

    // We can abort if we have no fields
    if ((count = zend_hash_num_elements(Z_ARRVAL_P(z_arr))) == 0) {
        return FAILURE;
    }

    // Prefix our key
    key_free = redis_key_prefix(redis_sock, &key, &key_len);

    // Grab our array as a HashTable
    ht_vals = Z_ARRVAL_P(z_arr);

    // Initialize our HMSET command (key + 2x each array entry), add key
    redis_cmd_init_sstr(&cmdstr, 1+(count*2), "HMSET", sizeof("HMSET")-1);
    redis_cmd_append_sstr(&cmdstr, key, key_len);

    // Start traversing our key => value array
    ZEND_HASH_FOREACH_KEY_VAL(ht_vals, idx, zkey, z_val) {
        char *mem, *val, kbuf[40];
        size_t val_len;
        int val_free;
        unsigned int mem_len;

        // If the hash key is an integer, convert it to a string
        if (zkey) {
            mem_len = ZSTR_LEN(zkey);
            mem = ZSTR_VAL(zkey);
        } else {
            mem_len = snprintf(kbuf, sizeof(kbuf), ZEND_LONG_FMT, idx);
            mem = (char*)kbuf;
        }

        // Serialize value (if directed)
        val_free = redis_pack(redis_sock, z_val, &val, &val_len);

        // Append the key and value to our command
        redis_cmd_append_sstr(&cmdstr, mem, mem_len);
        redis_cmd_append_sstr(&cmdstr, val, val_len);

        // Free our value if we serialized it
        if (val_free) efree(val);
    } ZEND_HASH_FOREACH_END();

    // Set slot if directed
    CMD_SET_SLOT(slot,key,key_len);

    // Free our key if we prefixed it
    if (key_free) efree(key);

    // Push return pointers
    *cmd_len = cmdstr.len;
    *cmd = cmdstr.c;

    // Success!
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

/* BITPOS */
int redis_bitpos_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock,
                     char **cmd, int *cmd_len, short *slot, void **ctx)
{
    char *key;
    int argc;
    zend_long bit, start, end;
    size_t key_len;

    argc = ZEND_NUM_ARGS();
    if (zend_parse_parameters(argc, "sl|ll", &key, &key_len, &bit,
                             &start, &end) == FAILURE)
    {
        return FAILURE;
    }

    // Prevalidate bit
    if (bit != 0 && bit != 1) {
        return FAILURE;
    }

    // Construct command based on arg count
    if (argc == 2) {
        *cmd_len = REDIS_CMD_SPPRINTF(cmd, "BITPOS", "kd", key, key_len, bit);
    } else if (argc == 3) {
        *cmd_len = REDIS_CMD_SPPRINTF(cmd, "BITPOS", "kdd", key, key_len, bit,
                                     start);
    } else {
        *cmd_len = REDIS_CMD_SPPRINTF(cmd, "BITPOS", "kddd", key, key_len, bit,
                                     start, end);
    }

    return SUCCESS;
}

/* BITOP */
int redis_bitop_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock,
                    char **cmd, int *cmd_len, short *slot, void **ctx)
{
    zval *z_args;
    char *key;
    size_t key_len;
    int i, key_free, argc = ZEND_NUM_ARGS();
    smart_string cmdstr = {0};
    short kslot;
    zend_string *zstr;

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
    redis_cmd_init_sstr(&cmdstr, argc, "BITOP", sizeof("BITOP")-1);
    redis_cmd_append_sstr(&cmdstr, Z_STRVAL(z_args[0]), Z_STRLEN(z_args[0]));

    // Now iterate over our keys argument
    for (i = 1; i < argc; i++) {
        // Make sure we've got a string
        zstr = zval_get_string(&z_args[i]);

        // Grab this key and length
        key = ZSTR_VAL(zstr);
        key_len = ZSTR_LEN(zstr);

        // Prefix key, append
        key_free = redis_key_prefix(redis_sock, &key, &key_len);
        redis_cmd_append_sstr(&cmdstr, key, key_len);

        // Verify slot if this is a Cluster request
        if (slot) {
            kslot = cluster_hash_key(key, key_len);
            if (*slot == -1 || kslot != *slot) {
                php_error_docref(NULL, E_WARNING,
                    "Warning, not all keys hash to the same slot!");
                zend_string_release(zstr);
                if (key_free) efree(key);
                efree(z_args);
                return FAILURE;
            }
            *slot = kslot;
        }

        zend_string_release(zstr);
        if (key_free) efree(key);
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

    if (zend_parse_parameters(ZEND_NUM_ARGS(), "s|ll", &key, &key_len,
                             &start, &end) == FAILURE)
    {
        return FAILURE;
    }

    *cmd_len = REDIS_CMD_SPPRINTF(cmd, "BITCOUNT", "kdd", key, key_len,
                                 (int)start, (int)end);

    return SUCCESS;
}

/* PFADD and PFMERGE are the same except that in one case we serialize,
 * and in the other case we key prefix */
static int redis_gen_pf_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock,
                            char *kw, int kw_len, int is_keys, char **cmd,
                            int *cmd_len, short *slot)
{
    zval *z_arr, *z_ele;
    HashTable *ht_arr;
    smart_string cmdstr = {0};
    char *mem, *key;
    int key_free, mem_free, argc=1;
    size_t key_len, mem_len;
    zend_string *zstr;

    // Parse arguments
    if (zend_parse_parameters(ZEND_NUM_ARGS(), "sa", &key, &key_len,
                             &z_arr) == FAILURE)
    {
        return FAILURE;
    }

    // Grab HashTable, count total argc
    ht_arr = Z_ARRVAL_P(z_arr);
    argc += zend_hash_num_elements(ht_arr);

    // We need at least two arguments
    if (argc < 2) {
        return FAILURE;
    }

    // Prefix key, set initial hash slot
    key_free = redis_key_prefix(redis_sock, &key, &key_len);
    if (slot) *slot = cluster_hash_key(key, key_len);

    // Start command construction
    redis_cmd_init_sstr(&cmdstr, argc, kw, kw_len);
    redis_cmd_append_sstr(&cmdstr, key, key_len);

    // Free key if we prefixed
    if (key_free) efree(key);

    // Now iterate over the rest of our keys or values
    ZEND_HASH_FOREACH_VAL(ht_arr, z_ele) {
        // Prefix keys, serialize values
        if (is_keys) {
            zstr = zval_get_string(z_ele);
            mem = ZSTR_VAL(zstr);
            mem_len = ZSTR_LEN(zstr);

            // Key prefix
            mem_free = redis_key_prefix(redis_sock, &mem, &mem_len);

            // Verify slot
            if (slot && *slot != cluster_hash_key(mem, mem_len)) {
                php_error_docref(0, E_WARNING,
                    "All keys must hash to the same slot!");
                zend_string_release(zstr);
                if (key_free) efree(key);
                return FAILURE;
            }
        } else {
            mem_free = redis_pack(redis_sock, z_ele, &mem, &mem_len);

            zstr = NULL;
            if (!mem_free) {
                zstr = zval_get_string(z_ele);
                mem = ZSTR_VAL(zstr);
                mem_len = ZSTR_LEN(zstr);
            }
        }

        // Append our key or member
        redis_cmd_append_sstr(&cmdstr, mem, mem_len);

        // Clean up any allocated memory
        if (zstr) zend_string_release(zstr);
        if (mem_free) efree(mem);
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
        "PFADD", sizeof("PFADD")-1, 0, cmd, cmd_len, slot);
}

/* PFMERGE */
int redis_pfmerge_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock,
                      char **cmd, int *cmd_len, short *slot, void **ctx)
{
    return redis_gen_pf_cmd(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock,
        "PFMERGE", sizeof("PFMERGE")-1, 1, cmd, cmd_len, slot);
}

/* PFCOUNT */
int redis_pfcount_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock,
                      char **cmd, int *cmd_len, short *slot, void **ctx)
{
    zval *z_keys, *z_key;
    HashTable *ht_keys;
    smart_string cmdstr = {0};
    int num_keys, key_free;
    size_t key_len;
    char *key;
    short kslot=-1;
    zend_string *zstr;

    if (zend_parse_parameters(ZEND_NUM_ARGS(),"z",&z_keys) == FAILURE) {
        return FAILURE;
    }

    /* If we were passed an array of keys, iterate through them prefixing if
     * required and capturing lengths and if we need to free them.  Otherwise
     * attempt to treat the argument as a string and just pass one */
    if (Z_TYPE_P(z_keys) == IS_ARRAY) {
        /* Grab key hash table and the number of keys */
        ht_keys = Z_ARRVAL_P(z_keys);
        num_keys = zend_hash_num_elements(ht_keys);

        /* There is no reason to send zero keys */
        if (num_keys == 0) {
            return FAILURE;
        }

        /* Initialize the command with our number of arguments */
        redis_cmd_init_sstr(&cmdstr, num_keys, "PFCOUNT", sizeof("PFCOUNT")-1);

        /* Append our key(s) */
        ZEND_HASH_FOREACH_VAL(ht_keys, z_key) {
            /* Turn our value into a string if it isn't one */
            zstr = zval_get_string(z_key);
            key = ZSTR_VAL(zstr);
            key_len = ZSTR_LEN(zstr);

            /* Append this key to our command */
            key_free = redis_key_prefix(redis_sock, &key, &key_len);
            redis_cmd_append_sstr(&cmdstr, key, key_len);

            /* Protect against CROSSLOT errors */
            if (slot) {
                if (kslot == -1) {
                    kslot = cluster_hash_key(key, key_len);
                } else if (cluster_hash_key(key,key_len)!=kslot) {
                    zend_string_release(zstr);
                    if (key_free) efree(key);
                    efree(cmdstr.c);

                    php_error_docref(NULL, E_WARNING,
                        "Not all keys hash to the same slot!");
                    return FAILURE;
                }
            }

            /* Cleanup */
            zend_string_release(zstr);
            if (key_free) efree(key);
        } ZEND_HASH_FOREACH_END();
    } else {
        /* Construct our whole command */
        redis_cmd_init_sstr(&cmdstr, 1, "PFCOUNT", sizeof("PFCOUNT")-1);

        /* Turn our key into a string if it's a different type */
        zstr = zval_get_string(z_keys);
        key = ZSTR_VAL(zstr);
        key_len = ZSTR_LEN(zstr);
        key_free = redis_key_prefix(redis_sock, &key, &key_len);
        redis_cmd_append_sstr(&cmdstr, key, key_len);

        /* Hash our key */
        CMD_SET_SLOT(slot, key, key_len);

        /* Cleanup */
        zend_string_release(zstr);
        if (key_free) efree(key);
    }

    /* Push our command and length to the caller */
    *cmd = cmdstr.c;
    *cmd_len = cmdstr.len;

    return SUCCESS;
}

char *redis_variadic_str_cmd(char *kw, zval *argv, int argc, int *cmd_len) {
    smart_string cmdstr = {0};
    zend_string *zstr;
    int i;

    redis_cmd_init_sstr(&cmdstr, argc, kw, strlen(kw));

    for (i = 0; i < argc; i++) {
        zstr = zval_get_string(&argv[i]);
        redis_cmd_append_sstr_zstr(&cmdstr, zstr);
        zend_string_release(zstr);
    }

    *cmd_len = cmdstr.len;
    return cmdstr.c;
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

int
redis_lmove_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock,
                char **cmd, int *cmd_len, short *slot, void **ctx)
{
    char *src, *dst, *from, *to;
    size_t src_len, dst_len, from_len, to_len;

    if (zend_parse_parameters(ZEND_NUM_ARGS(), "ssss",
                                &src, &src_len, &dst, &dst_len,
                                &from, &from_len, &to, &to_len) == FAILURE
    ) {
        return FAILURE;
    }

    // Validate wherefrom/whereto
    if (strcasecmp(from, "left") != 0 && strcasecmp(from, "right") != 0) {
        php_error_docref(NULL, E_WARNING,
            "Wherefrom argument must be either 'LEFT' or 'RIGHT'");
        return FAILURE;
    } else if (strcasecmp(to, "left") != 0 && strcasecmp(to, "right") != 0) {
        php_error_docref(NULL, E_WARNING,
            "Whereto argument must be either 'LEFT' or 'RIGHT'");
        return FAILURE;
    }

    /* Construct command */
    *cmd_len = REDIS_CMD_SPPRINTF(cmd, "LMOVE", "kkss",
                                  src, src_len, dst, dst_len,
                                  from, from_len, to, to_len);

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

int redis_smove_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock,
                    char **cmd, int *cmd_len, short *slot, void **ctx)
{
    char *src, *dst;
    size_t src_len, dst_len;
    int src_free, dst_free;
    zval *z_val;

    if (zend_parse_parameters(ZEND_NUM_ARGS(), "ssz", &src, &src_len,
                             &dst, &dst_len, &z_val) == FAILURE)
    {
        return FAILURE;
    }

    src_free = redis_key_prefix(redis_sock, &src, &src_len);
    dst_free = redis_key_prefix(redis_sock, &dst, &dst_len);

    // Protect against a CROSSSLOT error
    if (slot) {
        short slot1 = cluster_hash_key(src, src_len);
        short slot2 = cluster_hash_key(dst, dst_len);
        if (slot1 != slot2) {
            php_error_docref(0, E_WARNING,
                "Source and destination keys don't hash to the same slot!");
            if (src_free) efree(src);
            if (dst_free) efree(dst);
            return FAILURE;
        }
        *slot = slot1;
    }

    // Construct command
    *cmd_len = REDIS_CMD_SPPRINTF(cmd, "SMOVE", "ssv", src, src_len, dst,
        dst_len, z_val);

    // Cleanup
    if (src_free) efree(src);
    if (dst_free) efree(dst);

    // Success!
    return SUCCESS;
}

/* Generic command construction for HSET and HSETNX */
static int gen_hset_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock,
                        char *kw, char **cmd, int *cmd_len, short *slot)
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
    *cmd_len = REDIS_CMD_SPPRINTF(cmd, kw, "ksv", key, key_len, mem, mem_len, z_val);

    // Success
    return SUCCESS;
}

/* HSET */
int redis_hset_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock,
                   char **cmd, int *cmd_len, short *slot, void **ctx)
{
    return gen_hset_cmd(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock, "HSET",
        cmd, cmd_len, slot);
}

/* HSETNX */
int redis_hsetnx_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock,
                     char **cmd, int *cmd_len, short *slot, void **ctx)
{
    return gen_hset_cmd(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock, "HSETNX",
        cmd, cmd_len, slot);
}

/* SRANDMEMBER */
int redis_srandmember_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock,
                          char **cmd, int *cmd_len, short *slot, void **ctx,
                          short *have_count)
{
    char *key;
    size_t key_len;
    zend_long count;

    if (zend_parse_parameters(ZEND_NUM_ARGS(), "s|l", &key, &key_len,
                             &count) == FAILURE)
    {
        return FAILURE;
    }

    // Set our have count flag
    *have_count = ZEND_NUM_ARGS() == 2;

    // Two args means we have the optional COUNT
    if (*have_count) {
        *cmd_len = REDIS_CMD_SPPRINTF(cmd, "SRANDMEMBER", "kl", key, key_len, count);
    } else {
        *cmd_len = REDIS_CMD_SPPRINTF(cmd, "SRANDMEMBER", "k", key, key_len);
    }

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
                   int *using_store, char **cmd, int *cmd_len, short *slot,
                   void **ctx)
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

    // Default that we're not using store
    *using_store = 0;

    // If we don't have an options array, the command is quite simple
    if (!z_opts || zend_hash_num_elements(Z_ARRVAL_P(z_opts)) == 0) {
        // Construct command
        *cmd_len = REDIS_CMD_SPPRINTF(cmd, "SORT", "k", key, key_len);

        /* Not storing */
        *using_store = 0;

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
        *using_store = 1;
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
    redis_cmd_init_sstr(&cmdstr, zend_hash_num_elements(ht_argv), "SORT",
        sizeof("SORT")-1);

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
    redis_cmd_init_sstr(&cmdstr, argc, "HDEL", sizeof("HDEL")-1);
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
    zval *z_args;
    char *key, *val, *exp_type = NULL;
    size_t key_len, val_len;
    int key_free, val_free;
    int num = ZEND_NUM_ARGS(), i = 1, argc;
    zend_bool ch = 0, incr = 0;
    smart_string cmdstr = {0};
    zend_string *zstr;

    if (num < 3) return FAILURE;
    z_args = ecalloc(num, sizeof(zval));
    if (zend_get_parameters_array(ht, num, z_args) == FAILURE) {
        efree(z_args);
        return FAILURE;
    }

    // Need key, [NX|XX] [CH] [INCR] score, value, [score, value...] */
    if (num % 2 == 0) {
        if (Z_TYPE(z_args[1]) != IS_ARRAY) {
            efree(z_args);
            return FAILURE;
        }
        zval *z_opt;
        ZEND_HASH_FOREACH_VAL(Z_ARRVAL(z_args[1]), z_opt) {
            if (Z_TYPE_P(z_opt) == IS_STRING) {
                if (ZVAL_IS_NX_XX_ARG(z_opt)) {
                    exp_type = Z_STRVAL_P(z_opt);
                } else if (ZVAL_STRICMP_STATIC(z_opt, "CH")) {
                    ch = 1;
                } else if (ZVAL_STRICMP_STATIC(z_opt, "INCR")) {
                    if (num > 4) {
                        // Only one score-element pair can be specified in this mode.
                        efree(z_args);
                        return FAILURE;
                    }
                    incr = 1;
                }

            }
        } ZEND_HASH_FOREACH_END();
        argc  = num - 1;
        if (exp_type) argc++;
        argc += ch + incr;
        i++;
    } else {
        argc = num;
    }

    // Prefix our key
    zstr = zval_get_string(&z_args[0]);
    key = ZSTR_VAL(zstr);
    key_len = ZSTR_LEN(zstr);
    key_free = redis_key_prefix(redis_sock, &key, &key_len);

    // Start command construction
    redis_cmd_init_sstr(&cmdstr, argc, "ZADD", sizeof("ZADD")-1);
    redis_cmd_append_sstr(&cmdstr, key, key_len);

    // Set our slot, free key if we prefixed it
    CMD_SET_SLOT(slot,key,key_len);
    zend_string_release(zstr);
    if (key_free) efree(key);

    if (exp_type) redis_cmd_append_sstr(&cmdstr, exp_type, 2);
    if (ch) redis_cmd_append_sstr(&cmdstr, "CH", 2);
    if (incr) redis_cmd_append_sstr(&cmdstr, "INCR", 4);

    // Now the rest of our arguments
    while (i < num) {
        // Append score and member
        switch (Z_TYPE(z_args[i])) {
        case IS_LONG:
        case IS_DOUBLE:
            redis_cmd_append_sstr_dbl(&cmdstr, zval_get_double(&z_args[i]));
            break;
        case IS_STRING:
            /* The score values must be the string representation of a double
             * precision floating point number. +inf and -inf values are valid
             * values as well. */
            if (strncasecmp(Z_STRVAL(z_args[i]), "-inf", 4) == 0 ||
                strncasecmp(Z_STRVAL(z_args[i]), "+inf", 4) == 0 ||
                is_numeric_string(Z_STRVAL(z_args[i]), Z_STRLEN(z_args[i]), NULL, NULL, 0) != 0
            ) {
                redis_cmd_append_sstr(&cmdstr, Z_STRVAL(z_args[i]), Z_STRLEN(z_args[i]));
                break;
            }
            // fall through
        default:
            php_error_docref(NULL, E_WARNING, "Scores must be numeric or '-inf','+inf'");
            smart_string_free(&cmdstr);
            efree(z_args);
            return FAILURE;
        }
        // serialize value if requested
        val_free = redis_pack(redis_sock, &z_args[i+1], &val, &val_len);
        redis_cmd_append_sstr(&cmdstr, val, val_len);

        // Free value if we serialized
        if (val_free) efree(val);
        i += 2;
    }

    // Push output values
    *cmd     = cmdstr.c;
    *cmd_len = cmdstr.len;

    // Cleanup args
    efree(z_args);

    return SUCCESS;
}

/* OBJECT */
int redis_object_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock,
                     REDIS_REPLY_TYPE *rtype, char **cmd, int *cmd_len,
                     short *slot, void **ctx)
{
    char *key, *subcmd;
    size_t key_len, subcmd_len;

    if (zend_parse_parameters(ZEND_NUM_ARGS(), "ss", &subcmd,
                             &subcmd_len, &key, &key_len) == FAILURE)
    {
        return FAILURE;
    }

    // Format our command
    *cmd_len = REDIS_CMD_SPPRINTF(cmd, "OBJECT", "sk", subcmd, subcmd_len, key, key_len);

    // Push the reply type to our caller
    if (subcmd_len == 8 && (!strncasecmp(subcmd,"refcount",8) ||
                           !strncasecmp(subcmd,"idletime",8)))
    {
        *rtype = TYPE_INT;
    } else if (subcmd_len == 8 && !strncasecmp(subcmd, "encoding", 8)) {
        *rtype = TYPE_BULK;
    } else {
        php_error_docref(NULL, E_WARNING,
            "Invalid subcommand sent to OBJECT");
        efree(*cmd);
        return FAILURE;
    }

    // Success
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

/* Helper function to extract optional arguments for GEORADIUS and GEORADIUSBYMEMBER */
static int get_georadius_opts(HashTable *ht, geoOptions *opts) {
    zend_ulong idx;
    char *optstr;
    zend_string *zkey;
    zval *optval;

    PHPREDIS_NOTUSED(idx);

    /* Iterate over our argument array, collating which ones we have */
    ZEND_HASH_FOREACH_KEY_VAL(ht, idx, zkey, optval) {
        ZVAL_DEREF(optval);

        /* If the key is numeric it's a non value option */
        if (zkey) {
            if (ZSTR_LEN(zkey) == 5 && !strcasecmp(ZSTR_VAL(zkey), "count")) {
                if (Z_TYPE_P(optval) != IS_LONG || Z_LVAL_P(optval) <= 0) {
                    php_error_docref(NULL, E_WARNING,
                            "COUNT must be an integer > 0!");
                    if (opts->key) zend_string_release(opts->key);
                    return FAILURE;
                }

                /* Set our count */
                opts->count = Z_LVAL_P(optval);
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
    char *key;
    size_t keylen;
    int keyfree;

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
    }

    /* Append store options if we've got them */
    if (opt->store != STORE_NONE && opt->key != NULL) {
        /* Grab string bits and prefix if requested */
        key = ZSTR_VAL(opt->key);
        keylen = ZSTR_LEN(opt->key);
        keyfree = redis_key_prefix(redis_sock, &key, &keylen);

        if (opt->store == STORE_COORD) {
            REDIS_CMD_APPEND_SSTR_STATIC(str, "STORE");
        } else {
            REDIS_CMD_APPEND_SSTR_STATIC(str, "STOREDIST");
        }

        redis_cmd_append_sstr(str, key, keylen);

        CMD_SET_SLOT(slot, key, keylen);
        if (keyfree) free(key);
    }
}

/* GEORADIUS / GEORADIUS_RO */
int redis_georadius_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock,
                        char *kw, char **cmd, int *cmd_len, short *slot,
                        void **ctx)
{
    char *key, *unit;
    short store_slot = 0;
    size_t keylen, unitlen;
    int argc = 5, keyfree;
    double lng, lat, radius;
    zval *opts = NULL;
    geoOptions gopts = {0};
    smart_string cmdstr = {0};

    if (zend_parse_parameters(ZEND_NUM_ARGS(), "sddds|a", &key, &keylen,
                              &lng, &lat, &radius, &unit, &unitlen, &opts)
                              == FAILURE)
    {
        return FAILURE;
    }

    /* Parse any GEORADIUS options we have */
    if (opts != NULL) {
        /* Attempt to parse our options array */
        if (get_georadius_opts(Z_ARRVAL_P(opts), &gopts) != SUCCESS)
        {
            return FAILURE;
        }
    }

    /* Increment argc depending on options */
    argc += gopts.withcoord + gopts.withdist + gopts.withhash +
            (gopts.sort != SORT_NONE) + (gopts.count ? 2 : 0) +
            (gopts.store != STORE_NONE ? 2 : 0);

    /* Begin construction of our command */
    redis_cmd_init_sstr(&cmdstr, argc, kw, strlen(kw));

    /* Prefix and set slot */
    keyfree = redis_key_prefix(redis_sock, &key, &keylen);
    CMD_SET_SLOT(slot, key, keylen);

    /* Append required arguments */
    redis_cmd_append_sstr(&cmdstr, key, keylen);
    redis_cmd_append_sstr_dbl(&cmdstr, lng);
    redis_cmd_append_sstr_dbl(&cmdstr, lat);
    redis_cmd_append_sstr_dbl(&cmdstr, radius);
    redis_cmd_append_sstr(&cmdstr, unit, unitlen);

    /* Append optional arguments */
    append_georadius_opts(redis_sock, &cmdstr, slot ? &store_slot : NULL, &gopts);

    /* Free key if it was prefixed */
    if (keyfree) efree(key);
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
            (gopts.sort != SORT_NONE) + (gopts.count ? 2 : 0) +
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

/* MIGRATE */
int redis_migrate_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock,
                      char **cmd, int *cmd_len, short *slot, void **ctx)
{
    smart_string cmdstr = {0};
    char *host, *key;
    int argc, keyfree;
    zval *z_keys, *z_key;
    size_t hostlen, keylen;
    zend_long destdb, port, timeout;
    zend_bool copy = 0, replace = 0;
    zend_string *zstr;

    if (zend_parse_parameters(ZEND_NUM_ARGS(), "slzll|bb", &host, &hostlen, &port,
                              &z_keys, &destdb, &timeout, &copy, &replace) == FAILURE)
    {
        return FAILURE;
    }

    /* Protect against being passed an array with zero elements */
    if (Z_TYPE_P(z_keys) == IS_ARRAY && zend_hash_num_elements(Z_ARRVAL_P(z_keys)) == 0) {
        php_error_docref(NULL, E_WARNING, "Keys array cannot be empty");
        return FAILURE;
    }

    /* host, port, key|"", dest-db, timeout, [copy, replace] [KEYS key1..keyN] */
    argc = 5 + copy + replace;
    if (Z_TYPE_P(z_keys) == IS_ARRAY) {
        /* +1 for the "KEYS" argument itself */
        argc += 1 + zend_hash_num_elements(Z_ARRVAL_P(z_keys));
    }

    /* Initialize MIGRATE command with host and port */
    REDIS_CMD_INIT_SSTR_STATIC(&cmdstr, argc, "MIGRATE");
    redis_cmd_append_sstr(&cmdstr, host, hostlen);
    redis_cmd_append_sstr_long(&cmdstr, port);

    /* If passed a keys array the keys come later, otherwise pass the key to
     * migrate here */
    if (Z_TYPE_P(z_keys) == IS_ARRAY) {
        REDIS_CMD_APPEND_SSTR_STATIC(&cmdstr, "");
    } else {
        /* Grab passed value as a string */
        zstr = zval_get_string(z_keys);

        /* We may need to prefix our string */
        key = ZSTR_VAL(zstr);
        keylen = ZSTR_LEN(zstr);
        keyfree = redis_key_prefix(redis_sock, &key, &keylen);

        /* Add key to migrate */
        redis_cmd_append_sstr(&cmdstr, key, keylen);

        zend_string_release(zstr);
        if (keyfree) efree(key);
    }

    redis_cmd_append_sstr_long(&cmdstr, destdb);
    redis_cmd_append_sstr_long(&cmdstr, timeout);
    REDIS_CMD_APPEND_SSTR_OPT_STATIC(&cmdstr, copy, "COPY");
    REDIS_CMD_APPEND_SSTR_OPT_STATIC(&cmdstr, replace, "REPLACE");

    /* Append actual keys if we've got a keys array */
    if (Z_TYPE_P(z_keys) == IS_ARRAY) {
        REDIS_CMD_APPEND_SSTR_STATIC(&cmdstr, "KEYS");

        ZEND_HASH_FOREACH_VAL(Z_ARRVAL_P(z_keys), z_key) {
            zstr = zval_get_string(z_key);

            key = ZSTR_VAL(zstr);
            keylen = ZSTR_LEN(zstr);
            keyfree = redis_key_prefix(redis_sock, &key, &keylen);

            /* Append the key */
            redis_cmd_append_sstr(&cmdstr, key, keylen);

            zend_string_release(zstr);
            if (keyfree) efree(key);
        } ZEND_HASH_FOREACH_END();
    }

    *cmd = cmdstr.c;
    *cmd_len = cmdstr.len;
    return SUCCESS;
}

/* EXISTS */
int redis_exists_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock,
                     char **cmd, int *cmd_len, short *slot, void **ctx)
{
    return gen_varkey_cmd(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock,
        "EXISTS", sizeof("EXISTS") - 1, 0, 0, cmd, cmd_len, slot);
}

/* DEL */
int redis_del_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock,
                  char **cmd, int *cmd_len, short *slot, void **ctx)
{
    return gen_varkey_cmd(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock,
        "DEL", sizeof("DEL")-1, 1, 0, cmd, cmd_len, slot);
}

/* UNLINK */
int redis_unlink_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock,
                     char **cmd, int *cmd_len, short *slot, void **ctx)
{
    return gen_varkey_cmd(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock,
        "UNLINK", sizeof("UNLINK")-1, 1, 0, cmd, cmd_len, slot);
}

/* WATCH */
int redis_watch_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock,
                    char **cmd, int *cmd_len, short *slot, void **ctx)
{
    return gen_varkey_cmd(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock,
        "WATCH", sizeof("WATCH")-1, 1, 0, cmd, cmd_len, slot);
}

/* SINTER */
int redis_sinter_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock,
                     char **cmd, int *cmd_len, short *slot, void **ctx)
{
    return gen_varkey_cmd(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock,
        "SINTER", sizeof("SINTER")-1, 1, 0, cmd, cmd_len, slot);
}

/* SINTERSTORE */
int redis_sinterstore_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock,
                          char **cmd, int *cmd_len, short *slot, void **ctx)
{
    return gen_varkey_cmd(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock,
        "SINTERSTORE", sizeof("SINTERSTORE")-1, 1, 0, cmd, cmd_len, slot);
}

/* SUNION */
int redis_sunion_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock,
                     char **cmd, int *cmd_len, short *slot, void **ctx)
{
    return gen_varkey_cmd(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock,
        "SUNION", sizeof("SUNION")-1, 1, 0, cmd, cmd_len, slot);
}

/* SUNIONSTORE */
int redis_sunionstore_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock,
                          char **cmd, int *cmd_len, short *slot, void **ctx)
{
    return gen_varkey_cmd(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock,
        "SUNIONSTORE", sizeof("SUNIONSTORE")-1, 2, 0, cmd, cmd_len, slot);
}

/* SDIFF */
int redis_sdiff_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock,
                    char **cmd, int *cmd_len, short *slot, void **ctx)
{
    return gen_varkey_cmd(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock, "SDIFF",
        sizeof("SDIFF")-1, 1, 0, cmd, cmd_len, slot);
}

/* SDIFFSTORE */
int redis_sdiffstore_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock,
                         char **cmd, int *cmd_len, short *slot, void **ctx)
{
    return gen_varkey_cmd(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock,
        "SDIFFSTORE", sizeof("SDIFFSTORE")-1, 1, 0, cmd, cmd_len, slot);
}

/* COMMAND */
int redis_command_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock,
                      char **cmd, int *cmd_len, short *slot, void **ctx)
{
    char *kw=NULL;
    zval *z_arg;
    size_t kw_len;

    /* Parse our args */
    if (zend_parse_parameters(ZEND_NUM_ARGS(), "|sz", &kw, &kw_len,
                             &z_arg) == FAILURE)
    {
        return FAILURE;
    }

    /* Construct our command */
    if (!kw) {
        *cmd_len = REDIS_CMD_SPPRINTF(cmd, "COMMAND", "");
    } else if (!z_arg) {
        /* Sanity check */
        if (strncasecmp(kw, "count", sizeof("count") - 1)) {
            return FAILURE;
        }
        /* COMMAND COUNT */
        *cmd_len = REDIS_CMD_SPPRINTF(cmd, "COMMAND", "s", "COUNT", sizeof("COUNT") - 1);
    } else if (Z_TYPE_P(z_arg) == IS_STRING) {
        /* Sanity check */
        if (strncasecmp(kw, "info", sizeof("info") - 1)) {
            return FAILURE;
        }

        /* COMMAND INFO <cmd> */
        *cmd_len = REDIS_CMD_SPPRINTF(cmd, "COMMAND", "ss", "INFO", sizeof("INFO") - 1,
            Z_STRVAL_P(z_arg), Z_STRLEN_P(z_arg));
    } else {
        int arr_len;

        /* Sanity check on args */
        if (strncasecmp(kw, "getkeys", sizeof("getkeys")-1) ||
           Z_TYPE_P(z_arg)!=IS_ARRAY ||
           (arr_len=zend_hash_num_elements(Z_ARRVAL_P(z_arg)))<1)
        {
            return FAILURE;
        }

        zval *z_ele;
        HashTable *ht_arr = Z_ARRVAL_P(z_arg);
        smart_string cmdstr = {0};

        redis_cmd_init_sstr(&cmdstr, 1 + arr_len, "COMMAND", sizeof("COMMAND")-1);
        redis_cmd_append_sstr(&cmdstr, "GETKEYS", sizeof("GETKEYS")-1);

        ZEND_HASH_FOREACH_VAL(ht_arr, z_ele) {
            zend_string *zstr = zval_get_string(z_ele);
            redis_cmd_append_sstr(&cmdstr, ZSTR_VAL(zstr), ZSTR_LEN(zstr));
            zend_string_release(zstr);
        } ZEND_HASH_FOREACH_END();

        *cmd = cmdstr.c;
        *cmd_len = cmdstr.len;
    }

    /* Any slot will do */
    CMD_RAND_SLOT(slot);

    return SUCCESS;
}

int
redis_copy_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock,
               char **cmd, int *cmd_len, short *slot, void **ctx)
{
    smart_string cmdstr = {0};
    char *src, *dst;
    size_t src_len, dst_len;
    zend_long db = -1;
    zend_bool replace = 0;
    zval *opts = NULL;

    if (zend_parse_parameters(ZEND_NUM_ARGS(), "ss|a",
                              &src, &src_len, &dst, &dst_len, &opts) == FAILURE)
    {
        return FAILURE;
    }

    if (opts != NULL && Z_TYPE_P(opts) == IS_ARRAY) {
        zend_ulong idx;
        zend_string *zkey;
        zval *z_ele;
        ZEND_HASH_FOREACH_KEY_VAL(Z_ARRVAL_P(opts), idx, zkey, z_ele) {
            if (zkey != NULL) {
                ZVAL_DEREF(z_ele);
                if (zend_string_equals_literal_ci(zkey, "db")) {
                    db = zval_get_long(z_ele);
                } else if (zend_string_equals_literal_ci(zkey, "replace")) {
                    replace = zval_is_true(z_ele);
                }
            }
        } ZEND_HASH_FOREACH_END();
    }

    REDIS_CMD_INIT_SSTR_STATIC(&cmdstr, 2 + (db > -1) + replace, "COPY");
    redis_cmd_append_sstr(&cmdstr, src, src_len);
    redis_cmd_append_sstr(&cmdstr, dst, dst_len);

    if (db > -1) {
        REDIS_CMD_APPEND_SSTR_STATIC(&cmdstr, "DB");
        redis_cmd_append_sstr_long(&cmdstr, db);
    }
    if (replace) {
        REDIS_CMD_APPEND_SSTR_STATIC(&cmdstr, "REPLACE");
    }

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
    zend_bool approx = 0;
    zend_ulong idx;
    HashTable *ht_fields;
    int fcount, argc;
    char *key, *id;
    size_t keylen, idlen;

    if (zend_parse_parameters(ZEND_NUM_ARGS(), "ssa|lb", &key, &keylen,
                              &id, &idlen, &z_fields, &maxlen, &approx) == FAILURE)
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
    argc = 2 + (fcount*2) + (maxlen > 0 ? (approx ? 3 : 2) : 0);

    /* XADD key ID field string [field string ...] */
    REDIS_CMD_INIT_SSTR_STATIC(&cmdstr, argc, "XADD");
    redis_cmd_append_sstr_key(&cmdstr, key, keylen, redis_sock, slot);

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
    smart_string cmdstr = {0};
    char *key, *group, *start = NULL, *end = NULL, *consumer = NULL;
    size_t keylen, grouplen, startlen, endlen, consumerlen;
    int argc;
    zend_long count = -1, idle = 0;

    // XPENDING mystream group55 - + 10 consumer-123
    if (zend_parse_parameters(ZEND_NUM_ARGS(), "ss|sslsl", &key,
                              &keylen, &group, &grouplen, &start, &startlen,
                              &end, &endlen, &count, &consumer, &consumerlen,
                              &idle) == FAILURE)
    {
        return FAILURE;
    }

    /* If we've been passed a start argument, we also need end and count */
    if (start != NULL && (end == NULL || count < 0)) {
        return FAILURE;
    }

    /* Calculate argc.  It's either 2, 5, 6 or 7 */
    argc = 2 + (start != NULL ? 3 + (consumer != NULL) + (idle != 0) : 0);

    /* Construct command and add required arguments */
    REDIS_CMD_INIT_SSTR_STATIC(&cmdstr, argc, "XPENDING");
    redis_cmd_append_sstr_key(&cmdstr, key, keylen, redis_sock, slot);
    redis_cmd_append_sstr(&cmdstr, group, grouplen);

    /* Add optional argumentst */
    if (start) {
        if (idle != 0) {
            REDIS_CMD_APPEND_SSTR_STATIC(&cmdstr, "IDLE");
            redis_cmd_append_sstr_long(&cmdstr, (long)idle);
        }
        redis_cmd_append_sstr(&cmdstr, start, startlen);
        redis_cmd_append_sstr(&cmdstr, end, endlen);
        redis_cmd_append_sstr_long(&cmdstr, (long)count);

        /* Finally add consumer if we have it */
        if (consumer) redis_cmd_append_sstr(&cmdstr, consumer, consumerlen);
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
        redis_cmd_append_sstr(&cmdstr, "COUNT", sizeof("COUNT")-1);
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
    short oldslot;
    zval **id;
    zend_ulong idx;

    /* Append STREAM qualifier */
    REDIS_CMD_APPEND_SSTR_STATIC(cmdstr, "STREAMS");

    /* Sentinel slot */
    if (slot) oldslot = -1;

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
    HashTable *ht;
    zend_string *zkey;
    char *kval;
    size_t klen;
    zend_ulong idx;
    zval *zv;

    PHPREDIS_NOTUSED(idx);

    /* Initialize options array to sane defaults */
    memset(opt, 0, sizeof(*opt));
    opt->retrycount = -1;
    opt->idle.time = -1;

    /* Early return if we don't have any options */
    if (z_arr == NULL)
        return;

    /* Iterate over our options array */
    ht = Z_ARRVAL_P(z_arr);
    ZEND_HASH_FOREACH_KEY_VAL(ht, idx, zkey, zv) {
        if (zkey) {
            kval = ZSTR_VAL(zkey);
            klen = ZSTR_LEN(zkey);

            if (klen == 4) {
                if (!strncasecmp(kval, "TIME", 4)) {
                    opt->idle.type = "TIME";
                    opt->idle.time = get_xclaim_i64_arg("TIME", zv);
                } else if (!strncasecmp(kval, "IDLE", 4)) {
                    opt->idle.type = "IDLE";
                    opt->idle.time = get_xclaim_i64_arg("IDLE", zv);
                }
            } else if (klen == 10 && !strncasecmp(kval, "RETRYCOUNT", 10)) {
                opt->retrycount = zval_get_long(zv);
            }
        } else {
            if (Z_TYPE_P(zv) == IS_STRING) {
                kval = Z_STRVAL_P(zv);
                klen = Z_STRLEN_P(zv);
                if (klen == 5 && !strncasecmp(kval, "FORCE", 5)) {
                    opt->force = 1;
                } else if (klen == 6 && !strncasecmp(kval, "JUSTID", 6)) {
                    opt->justid = 1;
                }
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
 * XGROUP CREATE key groupname id [MKSTREAM]
 * XGROUP SETID key group id
 * XGROUP DESTROY key groupname
 * XGROUP DELCONSUMER key groupname consumername */
int redis_xgroup_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock,
                     char **cmd, int *cmd_len, short *slot, void **ctx)
{
    char *op, *key = NULL, *arg1 = NULL, *arg2 = NULL;
    size_t oplen, keylen, arg1len, arg2len;
    zend_bool mkstream = 0;
    int argc = ZEND_NUM_ARGS();

    if (zend_parse_parameters(ZEND_NUM_ARGS(), "s|sssb", &op, &oplen,
                              &key, &keylen, &arg1, &arg1len, &arg2, &arg2len,
                              &mkstream) == FAILURE)
    {
        return FAILURE;
    }

    if (argc == 1 && oplen == 4 && !strncasecmp(op, "HELP", 4)) {
        *cmd_len = REDIS_CMD_SPPRINTF(cmd, "XGROUP", "s", "HELP", 4);
        return SUCCESS;
    } else if (argc >= 4 && (oplen == 6 && !strncasecmp(op, "CREATE", 6))) {
        if (mkstream) {
            *cmd_len = REDIS_CMD_SPPRINTF(cmd, "XGROUP", "sksss", op, oplen, key, keylen,
                                          arg1, arg1len, arg2, arg2len, "MKSTREAM",
                                          sizeof("MKSTREAM") - 1);
        } else {
            *cmd_len = REDIS_CMD_SPPRINTF(cmd, "XGROUP", "skss", op, oplen, key, keylen,
                                          arg1, arg1len, arg2, arg2len);
        }
        return SUCCESS;
    } else if (argc == 4 && ((oplen == 5 && !strncasecmp(op, "SETID", 5)) ||
                             (oplen == 11 && !strncasecmp(op, "DELCONSUMER", 11))))
    {
        *cmd_len = REDIS_CMD_SPPRINTF(cmd, "XGROUP", "skss", op, oplen, key, keylen,
                                      arg1, arg1len, arg2, arg2len);
        return SUCCESS;
    } else if (argc == 3 && ((oplen == 7 && !strncasecmp(op, "DESTROY", 7)))) {
        *cmd_len = REDIS_CMD_SPPRINTF(cmd, "XGROUP", "sks", op, oplen, key,
                                      keylen, arg1, arg1len);
        return SUCCESS;
    }

    /* Didn't detect any valid XGROUP command pattern */
    return FAILURE;
}

/* XINFO CONSUMERS key group
 * XINFO GROUPS key
 * XINFO STREAM key [FULL [COUNT N]]
 * XINFO HELP */
int redis_xinfo_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock,
                     char **cmd, int *cmd_len, short *slot, void **ctx)
{
    char *op, *key, *arg = NULL;
    size_t oplen, keylen, arglen;
    zend_long count = -1;
    int argc = ZEND_NUM_ARGS();
    char fmt[] = "skssl";

    if (argc > 4 || zend_parse_parameters(ZEND_NUM_ARGS(), "s|ssl",
                                          &op, &oplen, &key, &keylen, &arg,
                                          &arglen, &count) == FAILURE)
    {
        return FAILURE;
    }

    /* Handle everything except XINFO STREAM */
    if (strncasecmp(op, "STREAM", 6) != 0) {
        fmt[argc] = '\0';
        *cmd_len = REDIS_CMD_SPPRINTF(cmd, "XINFO", fmt, op, oplen, key, keylen,
                                      arg, arglen);
        return SUCCESS;
    }

    /* 'FULL' is the only legal option to XINFO STREAM */
    if (argc > 2 && strncasecmp(arg, "FULL", 4) != 0) {
        php_error_docref(NULL, E_WARNING, "'%s' is not a valid option for XINFO STREAM", arg);
        return FAILURE;
    }

    /* If we have a COUNT bump the argument count to account for the 'COUNT' literal */
    if (argc == 4) argc++;

    fmt[argc] = '\0';

    /* Build our XINFO STREAM variant */
    *cmd_len = REDIS_CMD_SPPRINTF(cmd, "XINFO", fmt, "STREAM", 6, key, keylen,
                                  "FULL", 4, "COUNT", 5, count);

    return SUCCESS;
}

/* XTRIM MAXLEN [~] count */
int redis_xtrim_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock,
                    char **cmd, int *cmd_len, short *slot, void **ctx)
{
    char *key;
    size_t keylen;
    zend_long maxlen;
    zend_bool approx = 0;

    if (zend_parse_parameters(ZEND_NUM_ARGS(), "sl|b", &key, &keylen,
                              &maxlen, &approx) == FAILURE)
    {
        return FAILURE;
    }

    if (approx) {
        *cmd_len = REDIS_CMD_SPPRINTF(cmd, "XTRIM", "kssl", key, keylen,
                                      "MAXLEN", 6, "~", 1, maxlen);
    } else {
        *cmd_len = REDIS_CMD_SPPRINTF(cmd, "XTRIM", "ksl", key, keylen,
                                      "MAXLEN", 6, maxlen);
    }

    return SUCCESS;
}

int
redis_sentinel_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock,
                    char *kw, char **cmd, int *cmd_len, short *slot, void **ctx)
{
    if (zend_parse_parameters(ZEND_NUM_ARGS(), "") == FAILURE) {
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
        EMPTY_SWITCH_DEFAULT_CASE()
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

/* vim: set tabstop=4 softtabstop=4 expandtab shiftwidth=4: */
