/* -*- Mode: C; tab-width: 4 -*- */
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
  | Original Author: Michael Grunder <michael.grunder@gmail.com          |
  | Maintainer: Nicolas Favre-Felix <n.favre-felix@owlient.eu>           |
  +----------------------------------------------------------------------+
*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "redis_commands.h"
#include <zend_exceptions.h>

/* Local passthrough macro for command construction.  Given that these methods
 * are generic (so they work whether the caller is Redis or RedisCluster) we
 * will always have redis_sock, slot*, and TSRMLS_CC */
#define REDIS_CMD_SPPRINTF(ret, kw, fmt, ...) \
    redis_spprintf(redis_sock, slot TSRMLS_CC, ret, kw, fmt, ##__VA_ARGS__)

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
int redis_build_raw_cmd(zval *z_args, int argc, char **cmd, int *cmd_len TSRMLS_DC)
{
    smart_string cmdstr = {0};
    int i;

    /* Make sure our first argument is a string */
    if (Z_TYPE(z_args[0]) != IS_STRING) {
        php_error_docref(NULL TSRMLS_CC, E_WARNING,
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
                php_error_docref(NULL TSRMLS_CC, E_WARNING,
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

/* Generic command where we just take a string and do nothing to it*/
int redis_str_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock, char *kw,
                  char **cmd, int *cmd_len, short *slot, void **ctx)
{
    char *arg;
    strlen_t arg_len;

    // Parse args
    if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &arg, &arg_len)
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
    strlen_t key_len;
    zend_long expire;
    zval *z_val;

    if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "slz", &key, &key_len,
                             &expire, &z_val)==FAILURE)
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
    strlen_t key_len, val_len;
    zend_long lval;

    if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "sls", &key, &key_len,
                             &lval, &val, &val_len)==FAILURE)
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
    strlen_t key_len;
    zval *z_val;

    if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "sz", &key, &key_len,
                             &z_val)==FAILURE)
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
    strlen_t key_len, val_len;

    if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "ss", &key, &key_len,
                             &val, &val_len)==FAILURE)
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
    strlen_t klen, v1len, v2len;

    if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "sss", &k, &klen,
                             &v1, &v1len, &v2, &v2len)==FAILURE)
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
    strlen_t k1len, k2len;
    int k1free, k2free;

    if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "ss", &k1, &k1len,
                             &k2, &k2len)==FAILURE)
    {
        return FAILURE;
    }

    // Prefix both keys
    k1free = redis_key_prefix(redis_sock, &k1, &k1len);
    k2free = redis_key_prefix(redis_sock, &k2, &k2len);

    // If a slot is requested, we can test that they hash the same
    if(slot) {
        // Slots where these keys resolve
        short slot1 = cluster_hash_key(k1, k1len);
        short slot2 = cluster_hash_key(k2, k2len);

        // Check if Redis would give us a CROSSLOT error
        if(slot1 != slot2) {
            php_error_docref(0 TSRMLS_CC, E_WARNING, "Keys don't hash to the same slot");
            if(k1free) efree(k1);
            if(k2free) efree(k2);
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
    strlen_t keylen;
    zend_long lval;

    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "sl", &key, &keylen, &lval)
                              ==FAILURE)
    {
        return FAILURE;
    }

    *cmd_len = REDIS_CMD_SPPRINTF(cmd, kw, "kl", key, keylen, lval);

    // Success!
    return SUCCESS;
}

/* key, long, long */
int redis_key_long_long_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock,
                            char *kw, char **cmd, int *cmd_len, short *slot,
                            void **ctx)
{
    char *key;
    strlen_t key_len;
    zend_long val1, val2;

    if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "sll", &key, &key_len,
                             &val1, &val2)==FAILURE)
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
    strlen_t key_len;

    if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &key, &key_len)
                             ==FAILURE)
    {
        return FAILURE;
    }

    *cmd_len = REDIS_CMD_SPPRINTF(cmd, kw, "k", key, key_len);

    return SUCCESS;
}

/* Generic command where we take a key and a double */
int redis_key_dbl_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock,
                      char *kw, char **cmd, int *cmd_len, short *slot,
                      void **ctx)
{
    char *key;
    strlen_t key_len;
    double val;

    if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "sd", &key, &key_len,
                             &val)==FAILURE)
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
    if(type != TYPE_SCAN) {
        redis_cmd_append_sstr(&cmdstr, key, key_len);
    }

    // Append cursor
    redis_cmd_append_sstr_long(&cmdstr, it);

    // Append count if we've got one
    if(count) {
        redis_cmd_append_sstr(&cmdstr,"COUNT",sizeof("COUNT")-1);
        redis_cmd_append_sstr_long(&cmdstr, count);
    }

    // Append pattern if we've got one
    if(pat_len) {
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
    strlen_t key_len;
    zend_long start, end;
    zend_bool ws=0;

    if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "sll|b", &key, &key_len,
                             &start, &end, &ws)==FAILURE)
    {
        return FAILURE;
    }

    if(ws) {
        *cmd_len = REDIS_CMD_SPPRINTF(cmd, kw, "kdds", key, key_len, start, end,
            "WITHSCORES", sizeof("WITHSCORES") - 1);
    } else {
        *cmd_len = REDIS_CMD_SPPRINTF(cmd, kw, "kdd", key, key_len, start, end);
    }

    // Push out WITHSCORES option
    *withscores = ws;

    return SUCCESS;
}

/* ZRANGEBYSCORE/ZREVRANGEBYSCORE */
#define IS_WITHSCORES_ARG(s, l) \
    (l == sizeof("withscores") - 1 && !strncasecmp(s, "withscores", l))
#define IS_LIMIT_ARG(s, l) \
    (l == sizeof("limit") - 1 && !strncasecmp(s,"limit", l))

int redis_zrangebyscore_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock,
                            char *kw, char **cmd, int *cmd_len, int *withscores,
                            short *slot, void **ctx)
{
    char *key, *start, *end;
    int has_limit=0;
    long offset, count;
    strlen_t key_len, start_len, end_len;
    zval *z_opt=NULL, *z_ele;
    zend_string *zkey;
    ulong idx;
    HashTable *ht_opt;

    PHPREDIS_NOTUSED(idx);

    if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "sss|a", &key, &key_len,
                             &start, &start_len, &end, &end_len, &z_opt)
                             ==FAILURE)
    {
        return FAILURE;
    }

    // Check for an options array
    if(z_opt && Z_TYPE_P(z_opt)==IS_ARRAY) {
        ht_opt = Z_ARRVAL_P(z_opt);
        ZEND_HASH_FOREACH_KEY_VAL(ht_opt, idx, zkey, z_ele) {
           /* All options require a string key type */
           if (!zkey) continue;
           ZVAL_DEREF(z_ele);
           /* Check for withscores and limit */
           if (IS_WITHSCORES_ARG(zkey->val, zkey->len)) {
               *withscores = zval_is_true(z_ele);
           } else if (IS_LIMIT_ARG(zkey->val, zkey->len) && Z_TYPE_P(z_ele) == IS_ARRAY) {
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

/* ZUNIONSTORE, ZINTERSTORE */
int redis_zinter_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock,
                     char *kw, char **cmd, int *cmd_len, short *slot,
                     void **ctx)
{
    char *key, *agg_op=NULL;
    int key_free, argc = 2, keys_count;
    strlen_t key_len, agg_op_len = 0;
    zval *z_keys, *z_weights=NULL, *z_ele;
    HashTable *ht_keys, *ht_weights=NULL;
    smart_string cmdstr = {0};

    // Parse args
    if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "sa|a!s", &key,
                             &key_len, &z_keys, &z_weights, &agg_op,
                             &agg_op_len)==FAILURE)
    {
        return FAILURE;
    }

    // Grab our keys
    ht_keys = Z_ARRVAL_P(z_keys);

    // Nothing to do if there aren't any
    if((keys_count = zend_hash_num_elements(ht_keys))==0) {
        return FAILURE;
    } else {
        argc += keys_count;
    }

    // Handle WEIGHTS
    if(z_weights != NULL) {
        ht_weights = Z_ARRVAL_P(z_weights);
        if(zend_hash_num_elements(ht_weights) != keys_count) {
            php_error_docref(NULL TSRMLS_CC, E_WARNING,
                "WEIGHTS and keys array should be the same size!");
            return FAILURE;
        }

        // "WEIGHTS" + key count
        argc += keys_count + 1;
    }

    // AGGREGATE option
    if(agg_op_len != 0) {
        if(strncasecmp(agg_op, "SUM", sizeof("SUM")) &&
           strncasecmp(agg_op, "MIN", sizeof("MIN")) &&
           strncasecmp(agg_op, "MAX", sizeof("MAX")))
        {
            php_error_docref(NULL TSRMLS_CC, E_WARNING,
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
    if(key_free) efree(key);

    // Process input keys
    ZEND_HASH_FOREACH_VAL(ht_keys, z_ele) {
        zend_string *zstr = zval_get_string(z_ele);
        char *key = zstr->val;
        strlen_t key_len = zstr->len;

        // Prefix key if necissary
        int key_free = redis_key_prefix(redis_sock, &key, &key_len);

        // If we're in Cluster mode, verify the slot is the same
        if(slot && *slot != cluster_hash_key(key,key_len)) {
            php_error_docref(NULL TSRMLS_CC, E_WARNING,
                "All keys don't hash to the same slot!");
            efree(cmdstr.c);
            zend_string_release(zstr);
            if(key_free) efree(key);
            return FAILURE;
        }

        // Append this input set
        redis_cmd_append_sstr(&cmdstr, key, key_len);

        // Cleanup
        zend_string_release(zstr);
        if(key_free) efree(key);
    } ZEND_HASH_FOREACH_END();

    // Weights
    if(ht_weights != NULL) {
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
                    php_error_docref(NULL TSRMLS_CC, E_WARNING,
                        "Weights must be numeric or '-inf','inf','+inf'");
                    efree(cmdstr.c);
                    return FAILURE;
            }
        } ZEND_HASH_FOREACH_END();
    }

    // AGGREGATE
    if(agg_op_len != 0) {
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
    subscribeContext *sctx = emalloc(sizeof(subscribeContext));
    strlen_t key_len;
    int key_free;
    char *key;

    if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "af", &z_arr,
                             &(sctx->cb), &(sctx->cb_cache))==FAILURE)
    {
        efree(sctx);
        return FAILURE;
    }

    ht_chan    = Z_ARRVAL_P(z_arr);
    sctx->kw   = kw;
    sctx->argc = zend_hash_num_elements(ht_chan);

    if(sctx->argc==0) {
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
        key = zstr->val;
        key_len = zstr->len;
        key_free = redis_key_prefix(redis_sock, &key, &key_len);

        // Add this channel
        redis_cmd_append_sstr(&cmdstr, key, key_len);

        zend_string_release(zstr);
        // Free our key if it was prefixed
        if(key_free) efree(key);
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
    subscribeContext *sctx = emalloc(sizeof(subscribeContext));

    if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "a", &z_arr)==FAILURE) {
        efree(sctx);
        return FAILURE;
    }

    ht_arr = Z_ARRVAL_P(z_arr);

    sctx->argc = zend_hash_num_elements(ht_arr);
    if(sctx->argc == 0) {
        efree(sctx);
        return FAILURE;
    }

    redis_cmd_init_sstr(&cmdstr, sctx->argc, kw, strlen(kw));

    ZEND_HASH_FOREACH_VAL(ht_arr, z_chan) {
        char *key = Z_STRVAL_P(z_chan);
        strlen_t key_len = Z_STRLEN_P(z_chan);
        int key_free;

        key_free = redis_key_prefix(redis_sock, &key, &key_len);
        redis_cmd_append_sstr(&cmdstr, key, key_len);
        if(key_free) efree(key);
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
    strlen_t key_len, min_len, max_len;
    int argc = ZEND_NUM_ARGS();
    zend_long offset, count;

    /* We need either 3 or 5 arguments for this to be valid */
    if (argc != 3 && argc != 5) {
        php_error_docref(0 TSRMLS_CC, E_WARNING, "Must pass either 3 or 5 arguments");
        return FAILURE;
    }

    if(zend_parse_parameters(argc TSRMLS_CC, "sss|ll", &key, &key_len, &min, &min_len,
                             &max, &max_len, &offset, &count)==FAILURE)
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
        php_error_docref(0 TSRMLS_CC, E_WARNING,
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

/* ZLEXCOUNT/ZREMRANGEBYLEX */
int redis_gen_zlex_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock,
                       char *kw, char **cmd, int *cmd_len, short *slot,
                       void **ctx)
{
    char *key, *min, *max;
    strlen_t key_len, min_len, max_len;

    /* Parse args */
    if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "sss", &key, &key_len,
                             &min, &min_len, &max, &max_len)==FAILURE)
    {
        return FAILURE;
    }

    /* Quick sanity check on min/max */
    if(min_len<1 || max_len<1 || (min[0]!='(' && min[0]!='[') ||
       (max[0]!='(' && max[0]!='['))
    {
        php_error_docref(NULL TSRMLS_CC, E_WARNING,
            "Min and Max arguments must begin with '(' or '['");
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
    strlen_t lua_len;
    zend_string *zstr;
    short prevslot = -1;

    /* Parse args */
    if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s|al", &lua, &lua_len,
                             &z_arr, &num_keys)==FAILURE)
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
                redis_cmd_append_sstr_key(&cmdstr, zstr->val, zstr->len, redis_sock, slot);

                /* If we have been passed a slot, all keys must match */
                if (slot) {
                    if (prevslot != -1 && prevslot != *slot) {
                        zend_string_release(zstr);
                        php_error_docref(0 TSRMLS_CC, E_WARNING, "All keys do not map to the same slot");
                        return FAILURE;
                    }
                    prevslot = *slot;
                }
            } else {
                redis_cmd_append_sstr(&cmdstr, zstr->val, zstr->len);
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
    strlen_t i;
    int argc = ZEND_NUM_ARGS();

    // We at least need a key and one value
    if(argc < 2) {
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
    redis_cmd_append_sstr_key(&cmdstr, zstr->val, zstr->len, redis_sock, slot);
    zend_string_release(zstr);

    /* Add members */
    for (i = 1; i < argc; i++ ){
        redis_cmd_append_sstr_zval(&cmdstr, &z_args[i], redis_sock TSRMLS_CC);
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
int redis_key_arr_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock,
                      char *kw, char **cmd, int *cmd_len, short *slot,
                      void **ctx)
{
    zval *z_arr, *z_val;
    HashTable *ht_arr;
    smart_string cmdstr = {0};
    int key_free, val_free, argc = 1;
    strlen_t val_len, key_len;
    char *key, *val;

    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "sa", &key, &key_len,
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
    ZEND_HASH_FOREACH_VAL(ht_arr, z_val) {
        val_free = redis_serialize(redis_sock, z_val, &val, &val_len TSRMLS_CC);
        redis_cmd_append_sstr(&cmdstr, val, val_len);
        if (val_free) efree(val);
    } ZEND_HASH_FOREACH_END();

    *cmd_len = cmdstr.len;
    *cmd = cmdstr.c;

    return SUCCESS;
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
    strlen_t key_len;
    int single_array = 0, argc = ZEND_NUM_ARGS();
    smart_string cmdstr = {0};
    long timeout = 0;
    short kslot = -1;
    zend_string *zstr;

    if(argc < min_argc) {
        zend_wrong_param_count(TSRMLS_C);
        return FAILURE;
    }

    // Allocate args
    z_args = emalloc(argc * sizeof(zval));
    if (zend_get_parameters_array(ht, argc, z_args) == FAILURE) {
        efree(z_args);
        return FAILURE;
    }

    // Handle our "single array" case
    if(has_timeout == 0) {
        single_array = argc==1 && Z_TYPE(z_args[0])==IS_ARRAY;
    } else {
        single_array = argc==2 && Z_TYPE(z_args[0])==IS_ARRAY &&
            Z_TYPE(z_args[1])==IS_LONG;
        timeout = Z_LVAL(z_args[1]);
    }

    // If we're running a single array, rework args
    if(single_array) {
        ht_arr = Z_ARRVAL(z_args[0]);
        argc = zend_hash_num_elements(ht_arr);
        if(has_timeout) argc++;
        efree(z_args);
        z_args = NULL;

        /* If the array is empty, we can simply abort */
        if (argc == 0) return FAILURE;
    }

    // Begin construction of our command
    redis_cmd_init_sstr(&cmdstr, argc, kw, kw_len);

    if(single_array) {
        ZEND_HASH_FOREACH_VAL(ht_arr, z_ele) {
            zstr = zval_get_string(z_ele);
            key = zstr->val;
            key_len = zstr->len;
            key_free = redis_key_prefix(redis_sock, &key, &key_len);

            // Protect against CROSSLOT errors
            if(slot) {
                if(kslot == -1) {
                    kslot = cluster_hash_key(key, key_len);
                } else if(cluster_hash_key(key,key_len)!=kslot) {
                    zend_string_release(zstr);
                    if(key_free) efree(key);
                    php_error_docref(NULL TSRMLS_CC, E_WARNING,
                        "Not all keys hash to the same slot!");
                    return FAILURE;
                }
            }

            // Append this key, free it if we prefixed
            redis_cmd_append_sstr(&cmdstr, key, key_len);
            zend_string_release(zstr);
            if(key_free) efree(key);
        } ZEND_HASH_FOREACH_END();
        if(has_timeout) {
            redis_cmd_append_sstr_long(&cmdstr, timeout);
        }
    } else {
        if(has_timeout && Z_TYPE(z_args[argc-1])!=IS_LONG) {
            php_error_docref(NULL TSRMLS_CC, E_ERROR,
                "Timeout value must be a LONG");
            efree(z_args);
            return FAILURE;
        }

        tail = has_timeout ? argc-1 : argc;
        for(i=0;i<tail;i++) {
            zstr = zval_get_string(&z_args[i]);
            key = zstr->val;
            key_len = zstr->len;

            key_free = redis_key_prefix(redis_sock, &key, &key_len);

            /* Protect against CROSSSLOT errors if we've got a slot */
            if (slot) {
                if( kslot == -1) {
                    kslot = cluster_hash_key(key, key_len);
                } else if(cluster_hash_key(key,key_len)!=kslot) {
                    php_error_docref(NULL TSRMLS_CC, E_WARNING,
                        "Not all keys hash to the same slot");
                    zend_string_release(zstr);
                    if(key_free) efree(key);
                    efree(z_args);
                    return FAILURE;
                }
            }

            // Append this key
            redis_cmd_append_sstr(&cmdstr, key, key_len);
            zend_string_release(zstr);
            if(key_free) efree(key);
        }
        if(has_timeout) {
            redis_cmd_append_sstr_long(&cmdstr, Z_LVAL(z_args[tail]));
        }

        // Cleanup args
        efree(z_args);
    }

    // Push out parameters
    if(slot) *slot = kslot;
    *cmd = cmdstr.c;
    *cmd_len = cmdstr.len;

    return SUCCESS;
}

/*
 * Commands with specific signatures or that need unique functions because they
 * have specific processing (argument validation, etc) that make them unique
 */

/* SET */
int redis_set_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock,
                  char **cmd, int *cmd_len, short *slot, void **ctx)
{
    zval *z_value, *z_opts=NULL;
    char *key = NULL, *exp_type = NULL, *set_type = NULL;
    long expire = -1;
    strlen_t key_len;

    // Make sure the function is being called correctly
    if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "sz|z", &key, &key_len,
                             &z_value, &z_opts)==FAILURE)
    {
        return FAILURE;
    }

    /* Our optional argument can either be a long (to support legacy SETEX */
    /* redirection), or an array with Redis >= 2.6.12 set options */
    if(z_opts && Z_TYPE_P(z_opts) != IS_LONG && Z_TYPE_P(z_opts) != IS_ARRAY
       && Z_TYPE_P(z_opts) != IS_NULL)
    {
        return FAILURE;
    }

    // Check for an options array
    if(z_opts && Z_TYPE_P(z_opts) == IS_ARRAY) {
        HashTable *kt = Z_ARRVAL_P(z_opts);
        zend_string *zkey;
        ulong idx;
        zval *v;

        PHPREDIS_NOTUSED(idx);

        /* Iterate our option array */
        ZEND_HASH_FOREACH_KEY_VAL(kt, idx, zkey, v) {
            ZVAL_DEREF(v);
            /* Detect PX or EX argument and validate timeout */
            if (zkey && IS_EX_PX_ARG(zkey->val)) {
                /* Set expire type */
                exp_type = zkey->val;

                /* Try to extract timeout */
                if (Z_TYPE_P(v) == IS_LONG) {
                    expire = Z_LVAL_P(v);
                } else if (Z_TYPE_P(v) == IS_STRING) {
                    expire = atol(Z_STRVAL_P(v));
                }

                /* Expiry can't be set < 1 */
                if (expire < 1) {
                    return FAILURE;
                }
            } else if (Z_TYPE_P(v) == IS_STRING && IS_NX_XX_ARG(Z_STRVAL_P(v))) {
                set_type = Z_STRVAL_P(v);
            }
        } ZEND_HASH_FOREACH_END();
    } else if(z_opts && Z_TYPE_P(z_opts) == IS_LONG) {
        /* Grab expiry and fail if it's < 1 */
        expire = Z_LVAL_P(z_opts);
        if (expire < 1) {
            return FAILURE;
        }
    }

    /* Now let's construct the command we want */
    if(exp_type && set_type) {
        /* SET <key> <value> NX|XX PX|EX <timeout> */
        *cmd_len = REDIS_CMD_SPPRINTF(cmd, "SET", "kvsls", key, key_len, z_value,
                                     exp_type, 2, expire, set_type, 2);
    } else if(exp_type) {
        /* SET <key> <value> PX|EX <timeout> */
        *cmd_len = REDIS_CMD_SPPRINTF(cmd, "SET", "kvsl", key, key_len, z_value,
                                     exp_type, 2, expire);
    } else if(set_type) {
        /* SET <key> <value> NX|XX */
        *cmd_len = REDIS_CMD_SPPRINTF(cmd, "SET", "kvs", key, key_len, z_value,
                                     set_type, 2);
    } else if(expire > 0) {
        /* Backward compatible SETEX redirection */
        *cmd_len = REDIS_CMD_SPPRINTF(cmd, "SETEX", "klv", key, key_len, expire,
                                     z_value);
    } else {
        /* SET <key> <value> */
        *cmd_len = REDIS_CMD_SPPRINTF(cmd, "SET", "kv", key, key_len, z_value);
    }

    return SUCCESS;
}

/* BRPOPLPUSH */
int redis_brpoplpush_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock,
                         char **cmd, int *cmd_len, short *slot, void **ctx)
{
    char *key1, *key2;
    strlen_t key1_len, key2_len;
    int key1_free, key2_free;
    short slot1, slot2;
    zend_long timeout;

    if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "ssl", &key1, &key1_len,
                             &key2, &key2_len, &timeout)==FAILURE)
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
        if(slot1 != slot2) {
            php_error_docref(NULL TSRMLS_CC, E_WARNING,
               "Keys hash to different slots!");
            if(key1_free) efree(key1);
            if(key2_free) efree(key2);
            return FAILURE;
        }

        // Both slots are the same
        *slot = slot1;
    }

    // Consistency with Redis, if timeout < 0 use RPOPLPUSH
    if(timeout < 0) {
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
    strlen_t key_len;
    zend_long val = 1;

    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s|l", &key, &key_len,
                              &val)==FAILURE)
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
    strlen_t key_len, mem_len;
    zend_long byval;

    if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "ssl", &key, &key_len,
                             &mem, &mem_len, &byval)==FAILURE)
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
    strlen_t key_len, mem_len;
    double byval;

    if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "ssd", &key, &key_len,
                             &mem, &mem_len, &byval)==FAILURE)
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
    int i, count, valid=0, key_free;
    strlen_t key_len;
    HashTable *ht_arr;
    smart_string cmdstr = {0};

    // Parse arguments
    if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "sa", &key, &key_len,
                             &z_arr)==FAILURE)
    {
        return FAILURE;
    }

    // Our HashTable
    ht_arr = Z_ARRVAL_P(z_arr);

    // We can abort if we have no elements
    if((count = zend_hash_num_elements(ht_arr))==0) {
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
            convert_to_string(&z_mems[valid]);

            // Increment the member count to actually send
            valid++;
        }
    } ZEND_HASH_FOREACH_END();

    // If nothing was valid, fail
    if(valid == 0) {
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
    for(i=0;i<valid;i++) {
        redis_cmd_append_sstr(&cmdstr, Z_STRVAL(z_mems[i]),
            Z_STRLEN(z_mems[i]));
    }

    // Set our slot
    CMD_SET_SLOT(slot,key,key_len);

    // Free our key if we prefixed it
    if(key_free) efree(key);

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
    strlen_t key_len;
    ulong idx;
    zval *z_arr;
    HashTable *ht_vals;
    smart_string cmdstr = {0};
    zend_string *zkey;
    zval *z_val;

    // Parse args
    if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "sa", &key, &key_len,
                             &z_arr)==FAILURE)
    {
        return FAILURE;
    }

    // We can abort if we have no fields
    if((count = zend_hash_num_elements(Z_ARRVAL_P(z_arr)))==0) {
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
        strlen_t val_len;
        int val_free;
        unsigned int mem_len;

        // If the hash key is an integer, convert it to a string
        if (zkey) {
            mem_len = zkey->len;
            mem = zkey->val;
        } else {
            mem_len = snprintf(kbuf, sizeof(kbuf), "%ld", (long)idx);
            mem = (char*)kbuf;
        }

        // Serialize value (if directed)
        val_free = redis_serialize(redis_sock, z_val, &val, &val_len TSRMLS_CC);

        // Append the key and value to our command
        redis_cmd_append_sstr(&cmdstr, mem, mem_len);
        redis_cmd_append_sstr(&cmdstr, val, val_len);

        // Free our value if we serialized it
        if (val_free) efree(val);
    } ZEND_HASH_FOREACH_END();

    // Set slot if directed
    CMD_SET_SLOT(slot,key,key_len);

    // Free our key if we prefixed it
    if(key_free) efree(key);

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
    strlen_t key_len, field_len;

    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "ss", &key, &key_len,
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
    strlen_t key_len;

    argc = ZEND_NUM_ARGS();
    if(zend_parse_parameters(argc TSRMLS_CC, "sl|ll", &key, &key_len, &bit,
                             &start, &end)==FAILURE)
    {
        return FAILURE;
    }

    // Prevalidate bit
    if(bit != 0 && bit != 1) {
        return FAILURE;
    }

    // Construct command based on arg count
    if(argc == 2) {
        *cmd_len = REDIS_CMD_SPPRINTF(cmd, "BITPOS", "kd", key, key_len, bit);
    } else if(argc == 3) {
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
    strlen_t key_len;
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
    if(slot) *slot = -1;

    // Initialize command construction, add our operation argument
    redis_cmd_init_sstr(&cmdstr, argc, "BITOP", sizeof("BITOP")-1);
    redis_cmd_append_sstr(&cmdstr, Z_STRVAL(z_args[0]), Z_STRLEN(z_args[0]));

    // Now iterate over our keys argument
    for(i=1;i<argc;i++) {
        // Make sure we've got a string
        zstr = zval_get_string(&z_args[i]);

        // Grab this key and length
        key = zstr->val;
        key_len = zstr->len;

        // Prefix key, append
        key_free = redis_key_prefix(redis_sock, &key, &key_len);
        redis_cmd_append_sstr(&cmdstr, key, key_len);

        // Verify slot if this is a Cluster request
        if(slot) {
            kslot = cluster_hash_key(key, key_len);
            if(*slot == -1 || kslot != *slot) {
                php_error_docref(NULL TSRMLS_CC, E_WARNING,
                    "Warning, not all keys hash to the same slot!");
                zend_string_release(zstr);
                if(key_free) efree(key);
                efree(z_args);
                return FAILURE;
            }
            *slot = kslot;
        }

        zend_string_release(zstr);
        if(key_free) efree(key);
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
    strlen_t key_len;
    zend_long start = 0, end = -1;

    if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s|ll", &key, &key_len,
                             &start, &end)==FAILURE)
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
    strlen_t key_len, mem_len;
    zend_string *zstr;

    // Parse arguments
    if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "sa", &key, &key_len,
                             &z_arr)==FAILURE)
    {
        return FAILURE;
    }

    // Grab HashTable, count total argc
    ht_arr = Z_ARRVAL_P(z_arr);
    argc += zend_hash_num_elements(ht_arr);

    // We need at least two arguments
    if(argc < 2) {
        return FAILURE;
    }

    // Prefix key, set initial hash slot
    key_free = redis_key_prefix(redis_sock, &key, &key_len);
    if(slot) *slot = cluster_hash_key(key, key_len);

    // Start command construction
    redis_cmd_init_sstr(&cmdstr, argc, kw, kw_len);
    redis_cmd_append_sstr(&cmdstr, key, key_len);

    // Free key if we prefixed
    if(key_free) efree(key);

    // Now iterate over the rest of our keys or values
    ZEND_HASH_FOREACH_VAL(ht_arr, z_ele) {
        // Prefix keys, serialize values
        if(is_keys) {
            zstr = zval_get_string(z_ele);
            mem = zstr->val;
            mem_len = zstr->len;

            // Key prefix
            mem_free = redis_key_prefix(redis_sock, &mem, &mem_len);

            // Verify slot
            if(slot && *slot != cluster_hash_key(mem, mem_len)) {
                php_error_docref(0 TSRMLS_CC, E_WARNING,
                    "All keys must hash to the same slot!");
                zend_string_release(zstr);
                if(key_free) efree(key);
                return FAILURE;
            }
        } else {
            mem_free = redis_serialize(redis_sock, z_ele, &mem, &mem_len
                TSRMLS_CC);

            zstr = NULL;
            if(!mem_free) {
                zstr = zval_get_string(z_ele);
                mem = zstr->val;
                mem_len = zstr->len;
            }
        }

        // Append our key or member
        redis_cmd_append_sstr(&cmdstr, mem, mem_len);

        // Clean up any allocated memory
        if (zstr) zend_string_release(zstr);
        if(mem_free) efree(mem);
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
    strlen_t key_len;
    char *key;
    short kslot=-1;
    zend_string *zstr;

    if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC,"z",&z_keys)==FAILURE) {
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
            key = zstr->val;
            key_len = zstr->len;

            /* Append this key to our command */
            key_free = redis_key_prefix(redis_sock, &key, &key_len);
            redis_cmd_append_sstr(&cmdstr, key, key_len);

            /* Protect against CROSSLOT errors */
            if (slot) {
                if (kslot == -1) {
                    kslot = cluster_hash_key(key, key_len);
                } else if(cluster_hash_key(key,key_len)!=kslot) {
                    zend_string_release(zstr);
                    if (key_free) efree(key);
                    efree(cmdstr.c);

                    php_error_docref(NULL TSRMLS_CC, E_WARNING,
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
        key = zstr->val;
        key_len = zstr->len;
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

int redis_auth_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock,
                   char **cmd, int *cmd_len, short *slot, void **ctx)
{
    char *pw;
    strlen_t pw_len;

    if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &pw, &pw_len)
                             ==FAILURE)
    {
        return FAILURE;
    }

    // Construct our AUTH command
    *cmd_len = REDIS_CMD_SPPRINTF(cmd, "AUTH", "s", pw, pw_len);

    // Free previously allocated password, and update
    if(redis_sock->auth) efree(redis_sock->auth);
    redis_sock->auth = estrndup(pw, pw_len);

    // Success
    return SUCCESS;
}

/* SETBIT */
int redis_setbit_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock,
                     char **cmd, int *cmd_len, short *slot, void **ctx)
{
    char *key;
    strlen_t key_len;
    zend_long offset;
    zend_bool val;

    if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "slb", &key, &key_len,
                             &offset, &val)==FAILURE)
    {
        return FAILURE;
    }

    // Validate our offset
    if(offset < BITOP_MIN_OFFSET || offset > BITOP_MAX_OFFSET) {
        php_error_docref(0 TSRMLS_CC, E_WARNING,
            "Invalid OFFSET for bitop command (must be between 0-2^32-1)");
        return FAILURE;
    }

    *cmd_len = REDIS_CMD_SPPRINTF(cmd, "SETBIT", "kld", key, key_len, offset, (int)val);

    return SUCCESS;
}

/* LINSERT */
int redis_linsert_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock,
                      char **cmd, int *cmd_len, short *slot, void **ctx)
{
    char *key, *pos;
    strlen_t key_len, pos_len;
    zval *z_val, *z_pivot;

    if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "sszz", &key, &key_len,
                             &pos, &pos_len, &z_pivot, &z_val)==FAILURE)
    {
        return FAILURE;
    }

    // Validate position
    if(strncasecmp(pos, "after", 5) && strncasecmp(pos, "before", 6)) {
        php_error_docref(NULL TSRMLS_CC, E_WARNING,
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
    strlen_t key_len;
    zend_long count = 0;
    zval *z_val;

    if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "sz|l", &key, &key_len,
                             &z_val, &count)==FAILURE)
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
    strlen_t src_len, dst_len;
    int src_free, dst_free;
    zval *z_val;

    if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "ssz", &src, &src_len,
                             &dst, &dst_len, &z_val)==FAILURE)
    {
        return FAILURE;
    }

    src_free = redis_key_prefix(redis_sock, &src, &src_len);
    dst_free = redis_key_prefix(redis_sock, &dst, &dst_len);

    // Protect against a CROSSSLOT error
    if (slot) {
        short slot1 = cluster_hash_key(src, src_len);
        short slot2 = cluster_hash_key(dst, dst_len);
        if(slot1 != slot2) {
            php_error_docref(0 TSRMLS_CC, E_WARNING,
                "Source and destination keys don't hash to the same slot!");
            if(src_free) efree(src);
            if(dst_free) efree(dst);
            return FAILURE;
        }
        *slot = slot1;
    }

    // Construct command
    *cmd_len = REDIS_CMD_SPPRINTF(cmd, "SMOVE", "ssv", src, src_len, dst,
        dst_len, z_val);

    // Cleanup
    if(src_free) efree(src);
    if(dst_free) efree(dst);

    // Succcess!
    return SUCCESS;
}

/* Generic command construction for HSET and HSETNX */
static int gen_hset_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock,
                        char *kw, char **cmd, int *cmd_len, short *slot)
{
    char *key, *mem;
    strlen_t key_len, mem_len;
    zval *z_val;

    if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "ssz", &key, &key_len,
                             &mem, &mem_len, &z_val)==FAILURE)
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
    strlen_t key_len;
    zend_long count;

    if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s|l", &key, &key_len,
                             &count)==FAILURE)
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
    strlen_t key_len;
    double incrby;
    zval *z_val;

    if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "sdz", &key, &key_len,
                             &incrby, &z_val)==FAILURE)
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
    strlen_t key_len;
    int key_free;

    if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s|a", &key, &key_len,
                             &z_opts)==FAILURE)
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
        if(slot) {
            php_error_docref(NULL TSRMLS_CC, E_WARNING,
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

        if(cross_slot) {
            php_error_docref(0 TSRMLS_CC, E_WARNING,
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
        if(slot) {
            php_error_docref(NULL TSRMLS_CC, E_WARNING,
                "GET option for SORT disabled in Redis Cluster");
            zval_dtor(&z_argv);
            return FAILURE;
        }

        // If it's a string just add it
        if (Z_TYPE_P(z_ele) == IS_STRING) {
            add_next_index_stringl(&z_argv, "GET", sizeof("GET") - 1);
            add_next_index_stringl(&z_argv, Z_STRVAL_P(z_ele), Z_STRLEN_P(z_ele));
        } else {
            HashTable *ht_keys = Z_ARRVAL_P(z_ele);
            int added=0;

            for(zend_hash_internal_pointer_reset(ht_keys);
                zend_hash_has_more_elements(ht_keys)==SUCCESS;
                zend_hash_move_forward(ht_keys))
            {
                zval *z_key;

                // If we can't get the data, or it's not a string, skip
                if ((z_key = zend_hash_get_current_data(ht_keys)) == NULL || Z_TYPE_P(z_key) != IS_STRING) {
                    continue;
                }
                /* Add get per thing we're getting */
                add_next_index_stringl(&z_argv, "GET", sizeof("GET") - 1);

                // Add this key to our argv array
                add_next_index_stringl(&z_argv, Z_STRVAL_P(z_key), Z_STRLEN_P(z_key));
                added++;
            }

            // Make sure we were able to add at least one
            if(added==0) {
                php_error_docref(NULL TSRMLS_CC, E_WARNING,
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
                php_error_docref(NULL TSRMLS_CC, E_WARNING,
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
    strlen_t arg_len;
    int argc = ZEND_NUM_ARGS();
    zend_string *zstr;

    // We need at least KEY and one member
    if(argc < 2) {
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
    arg = zstr->val;
    arg_len = zstr->len;

    // Prefix
    arg_free = redis_key_prefix(redis_sock, &arg, &arg_len);

    // Start command construction
    redis_cmd_init_sstr(&cmdstr, argc, "HDEL", sizeof("HDEL")-1);
    redis_cmd_append_sstr(&cmdstr, arg, arg_len);

    // Set our slot, free key if we prefixed it
    CMD_SET_SLOT(slot,arg,arg_len);
    zend_string_release(zstr);
    if(arg_free) efree(arg);

    // Iterate through the members we're removing
    for(i=1;i<argc;i++) {
        zstr = zval_get_string(&z_args[i]);
        redis_cmd_append_sstr(&cmdstr, zstr->val, zstr->len);
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
    strlen_t key_len, val_len;
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
                if (Z_STRLEN_P(z_opt) == 2) {
                    if (IS_NX_XX_ARG(Z_STRVAL_P(z_opt))) {
                        exp_type = Z_STRVAL_P(z_opt);
                    } else if (strncasecmp(Z_STRVAL_P(z_opt), "ch", 2) == 0) {
                        ch = 1;
                    }
                } else if (Z_STRLEN_P(z_opt) == 4 &&
                    strncasecmp(Z_STRVAL_P(z_opt), "incr", 4) == 0
                ) {
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
    key = zstr->val;
    key_len = zstr->len;
    key_free = redis_key_prefix(redis_sock, &key, &key_len);

    // Start command construction
    redis_cmd_init_sstr(&cmdstr, argc, "ZADD", sizeof("ZADD")-1);
    redis_cmd_append_sstr(&cmdstr, key, key_len);

    // Set our slot, free key if we prefixed it
    CMD_SET_SLOT(slot,key,key_len);
    zend_string_release(zstr);
    if(key_free) efree(key);

    if (exp_type) redis_cmd_append_sstr(&cmdstr, exp_type, 2);
    if (ch) redis_cmd_append_sstr(&cmdstr, "CH", 2);
    if (incr) redis_cmd_append_sstr(&cmdstr, "INCR", 4);

    // Now the rest of our arguments
    while (i < num) {
        // Append score and member
        if (Z_TYPE(z_args[i]) == IS_STRING && (
            /* The score values should be the string representation of a double
             * precision floating point number. +inf and -inf values are valid
             * values as well. */
            strncasecmp(Z_STRVAL(z_args[i]), "-inf", 4) == 0 ||
            strncasecmp(Z_STRVAL(z_args[i]), "+inf", 4) == 0
        )) {
            redis_cmd_append_sstr(&cmdstr, Z_STRVAL(z_args[i]), Z_STRLEN(z_args[i]));
        } else {
            redis_cmd_append_sstr_dbl(&cmdstr, zval_get_double(&z_args[i]));
        }
        // serialize value if requested
        val_free = redis_serialize(redis_sock, &z_args[i+1], &val, &val_len
            TSRMLS_CC);
        redis_cmd_append_sstr(&cmdstr, val, val_len);

        // Free value if we serialized
        if(val_free) efree(val);
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
    strlen_t key_len, subcmd_len;

    if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "ss", &subcmd,
                             &subcmd_len, &key, &key_len)==FAILURE)
    {
        return FAILURE;
    }

    // Format our command
    *cmd_len = REDIS_CMD_SPPRINTF(cmd, "OBJECT", "sk", subcmd, subcmd_len, key, key_len);

    // Push the reply type to our caller
    if(subcmd_len == 8 && (!strncasecmp(subcmd,"refcount",8) ||
                           !strncasecmp(subcmd,"idletime",8)))
    {
        *rtype = TYPE_INT;
    } else if(subcmd_len == 8 && !strncasecmp(subcmd, "encoding", 8)) {
        *rtype = TYPE_BULK;
    } else {
        php_error_docref(NULL TSRMLS_CC, E_WARNING,
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
    strlen_t keylen, sourcelen, destlen, unitlen;

    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "sss|s", &key, &keylen,
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

/* Helper function to extract optional arguments for GEORADIUS and GEORADIUSBYMEMBER */
static void get_georadius_opts(HashTable *ht, int *withcoord, int *withdist,
                               int *withhash, long *count, geoSortType *sort)
{
    ulong idx;
    char *optstr;
    zend_string *zkey;
    zval *optval;

    PHPREDIS_NOTUSED(idx);

    /* Iterate over our argument array, collating which ones we have */
    ZEND_HASH_FOREACH_KEY_VAL(ht, idx, zkey, optval) {
        ZVAL_DEREF(optval);
        /* If the key is numeric it's a non value option */
        if (zkey) {
            if (zkey->len == 5 && !strcasecmp(zkey->val, "count") && Z_TYPE_P(optval) == IS_LONG) {
                *count = Z_LVAL_P(optval);
            }
        } else {
            /* Option needs to be a string */
            if (Z_TYPE_P(optval) != IS_STRING) continue;

            optstr = Z_STRVAL_P(optval);

            if (!strcasecmp(optstr, "withcoord")) {
                *withcoord = 1;
            } else if (!strcasecmp(optstr, "withdist")) {
                *withdist = 1;
            } else if (!strcasecmp(optstr, "withhash")) {
                *withhash = 1;
            } else if (!strcasecmp(optstr, "asc")) {
                *sort = SORT_ASC;
            } else if (!strcasecmp(optstr, "desc")) {
                *sort = SORT_DESC;
            }
        }
    } ZEND_HASH_FOREACH_END();
}

/* Helper to append options to a GEORADIUS or GEORADIUSBYMEMBER command */
void append_georadius_opts(smart_string *str, int withcoord, int withdist,
                           int withhash, long count, geoSortType sort)
{
    /* WITHCOORD option */
    if (withcoord)
        REDIS_CMD_APPEND_SSTR_STATIC(str, "WITHCOORD");

    /* WITHDIST option */
    if (withdist)
        REDIS_CMD_APPEND_SSTR_STATIC(str, "WITHDIST");

    /* WITHHASH option */
    if (withhash)
        REDIS_CMD_APPEND_SSTR_STATIC(str, "WITHHASH");

    /* Append sort if it's not GEO_NONE */
    if (sort == SORT_ASC) {
        REDIS_CMD_APPEND_SSTR_STATIC(str, "ASC");
    } else if (sort == SORT_DESC) {
        REDIS_CMD_APPEND_SSTR_STATIC(str, "DESC");
    }

    /* Append our count if we've got one */
    if (count > 0) {
        REDIS_CMD_APPEND_SSTR_STATIC(str, "COUNT");
        redis_cmd_append_sstr_long(str, count);
    }
}

/* GEORADIUS */
int redis_georadius_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock,
                        char **cmd, int *cmd_len, short *slot, void **ctx)
{
    char *key, *unit;
    strlen_t keylen, unitlen;
    int keyfree, withcoord = 0, withdist = 0, withhash = 0;
    long count = 0;
    geoSortType sort = SORT_NONE;
    double lng, lat, radius;
    zval *opts = NULL;
    smart_string cmdstr = {0};
    int argc;

    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "sddds|a", &key, &keylen,
                              &lng, &lat, &radius, &unit, &unitlen, &opts)
                              == FAILURE)
    {
        return FAILURE;
    }

    /* Parse any GEORADIUS options we have */
    if (opts != NULL) {
        get_georadius_opts(Z_ARRVAL_P(opts), &withcoord, &withdist, &withhash,
            &count, &sort);
    }

    /* Calculate the number of arguments we're going to send, five required plus
     * options. */
    argc = 5 + withcoord + withdist + withhash + (sort != SORT_NONE);
    if (count != 0) argc += 2;

    /* Begin construction of our command */
    REDIS_CMD_INIT_SSTR_STATIC(&cmdstr, argc, "GEORADIUS");

    /* Apply any key prefix */
    keyfree = redis_key_prefix(redis_sock, &key, &keylen);

    /* Append required arguments */
    redis_cmd_append_sstr(&cmdstr, key, keylen);
    redis_cmd_append_sstr_dbl(&cmdstr, lng);
    redis_cmd_append_sstr_dbl(&cmdstr, lat);
    redis_cmd_append_sstr_dbl(&cmdstr, radius);
    redis_cmd_append_sstr(&cmdstr, unit, unitlen);

    /* Append optional arguments */
    append_georadius_opts(&cmdstr, withcoord, withdist, withhash, count, sort);

    /* Free key if it was prefixed */
    if (keyfree) efree(key);

    /* Set slot, command and len, and return */
    CMD_SET_SLOT(slot, key, keylen);
    *cmd = cmdstr.c;
    *cmd_len = cmdstr.len;

    return SUCCESS;
}

/* GEORADIUSBYMEMBER key member radius m|km|ft|mi [WITHCOORD] [WITHDIST] [WITHHASH] [COUNT count] */
int redis_georadiusbymember_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock,
                                char **cmd, int *cmd_len, short *slot, void **ctx)
{
    char *key, *mem, *unit;
    strlen_t keylen, memlen, unitlen;
    int keyfree, argc, withcoord = 0, withdist = 0, withhash = 0;
    long count = 0;
    double radius;
    geoSortType sort = SORT_NONE;
    zval *opts = NULL;
    smart_string cmdstr = {0};

    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "ssds|a", &key, &keylen,
                              &mem, &memlen, &radius, &unit, &unitlen, &opts) == FAILURE)
    {
        return FAILURE;
    }

    if (opts != NULL) {
        get_georadius_opts(Z_ARRVAL_P(opts), &withcoord, &withdist, &withhash, &count, &sort);
    }

    /* Calculate argc */
    argc = 4 + withcoord + withdist + withhash + (sort != SORT_NONE);
    if (count != 0) argc += 2;

    /* Begin command construction*/
    REDIS_CMD_INIT_SSTR_STATIC(&cmdstr, argc, "GEORADIUSBYMEMBER");

    /* Prefix our key if we're prefixing */
    keyfree = redis_key_prefix(redis_sock, &key, &keylen);

    /* Append required arguments */
    redis_cmd_append_sstr(&cmdstr, key, keylen);
    redis_cmd_append_sstr(&cmdstr, mem, memlen);
    redis_cmd_append_sstr_long(&cmdstr, radius);
    redis_cmd_append_sstr(&cmdstr, unit, unitlen);

    /* Append options */
    append_georadius_opts(&cmdstr, withcoord, withdist, withhash, count, sort);

    /* Free key if we prefixed */
    if (keyfree) efree(key);

    CMD_SET_SLOT(slot, key, keylen);
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
    strlen_t hostlen, keylen;
    zend_long destdb, port, timeout;
    zend_bool copy = 0, replace = 0;
    zend_string *zstr;

    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "slzll|bb", &host, &hostlen, &port,
                              &z_keys, &destdb, &timeout, &copy, &replace) == FAILURE)
    {
        return FAILURE;
    }

    /* Protect against being passed an array with zero elements */
    if (Z_TYPE_P(z_keys) == IS_ARRAY && zend_hash_num_elements(Z_ARRVAL_P(z_keys)) == 0) {
        php_error_docref(NULL TSRMLS_CC, E_WARNING, "Keys array cannot be empty");
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
        key = zstr->val;
        keylen = zstr->len;
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

            key = zstr->val;
            keylen = zstr->len;
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

/* DEL */
int redis_del_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock,
                  char **cmd, int *cmd_len, short *slot, void **ctx)
{
    return gen_varkey_cmd(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock,
        "DEL", sizeof("DEL")-1, 1, 0, cmd, cmd_len, slot);
}

/* WATCH */
int redis_watch_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock,
                    char **cmd, int *cmd_len, short *slot, void **ctx)
{
    return gen_varkey_cmd(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock,
        "WATCH", sizeof("WATCH")-1, 1, 0, cmd, cmd_len, slot);
}

/* BLPOP */
int redis_blpop_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock,
                    char **cmd, int *cmd_len, short *slot, void **ctx)
{
    return gen_varkey_cmd(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock,
        "BLPOP", sizeof("BLPOP")-1, 2, 1, cmd, cmd_len, slot);
}

/* BRPOP */
int redis_brpop_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock,
                    char **cmd, int *cmd_len, short *slot, void **ctx)
{
    return gen_varkey_cmd(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock,
        "BRPOP", sizeof("BRPOP")-1, 1, 1, cmd, cmd_len, slot);
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
    strlen_t kw_len;

    /* Parse our args */
    if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "|sz", &kw, &kw_len,
                             &z_arg)==FAILURE)
    {
        return FAILURE;
    }

    /* Construct our command */
    if(!kw) {
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
        if(strncasecmp(kw, "getkeys", sizeof("getkeys")-1) ||
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
            redis_cmd_append_sstr(&cmdstr, zstr->val, zstr->len);
            zend_string_release(zstr);
        } ZEND_HASH_FOREACH_END();

        *cmd = cmdstr.c;
        *cmd_len = cmdstr.len;
    }

    /* Any slot will do */
    CMD_RAND_SLOT(slot);

    return SUCCESS;
}

/*
 * Redis commands that don't deal with the server at all.  The RedisSock*
 * pointer is the only thing retreived differently, so we just take that
 * in additon to the standard INTERNAL_FUNCTION_PARAMETERS for arg parsing,
 * return value handling, and thread safety. */

void redis_getoption_handler(INTERNAL_FUNCTION_PARAMETERS,
                             RedisSock *redis_sock, redisCluster *c)
{
    zend_long option;

    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "l", &option)
                              == FAILURE)
    {
        RETURN_FALSE;
    }

    // Return the requested option
    switch(option) {
        case REDIS_OPT_SERIALIZER:
            RETURN_LONG(redis_sock->serializer);
        case REDIS_OPT_PREFIX:
            if(redis_sock->prefix) {
                RETURN_STRINGL(redis_sock->prefix, redis_sock->prefix_len);
            }
            RETURN_NULL();
        case REDIS_OPT_READ_TIMEOUT:
            RETURN_DOUBLE(redis_sock->read_timeout);
        case REDIS_OPT_SCAN:
            RETURN_LONG(redis_sock->scan);
        case REDIS_OPT_FAILOVER:
            RETURN_LONG(c->failover);
        default:
            RETURN_FALSE;
    }
}

void redis_setoption_handler(INTERNAL_FUNCTION_PARAMETERS,
                             RedisSock *redis_sock, redisCluster *c)
{
    long val_long;
    zend_long option;
    char *val_str;
    struct timeval read_tv;
    strlen_t val_len;

    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "ls", &option,
                              &val_str, &val_len) == FAILURE)
    {
        RETURN_FALSE;
    }

    switch(option) {
        case REDIS_OPT_SERIALIZER:
            val_long = atol(val_str);
            if (val_long == REDIS_SERIALIZER_NONE || val_long == REDIS_SERIALIZER_PHP
#ifdef HAVE_REDIS_IGBINARY
                || val_long == REDIS_SERIALIZER_IGBINARY
#endif
            ) {
                redis_sock->serializer = val_long;
                RETURN_TRUE;
            }
            break;
        case REDIS_OPT_PREFIX:
            if(redis_sock->prefix) {
                efree(redis_sock->prefix);
            }
            if(val_len == 0) {
                redis_sock->prefix = NULL;
                redis_sock->prefix_len = 0;
            } else {
                redis_sock->prefix = estrndup(val_str, val_len);
                redis_sock->prefix_len = val_len;
            }
            RETURN_TRUE;
        case REDIS_OPT_READ_TIMEOUT:
            redis_sock->read_timeout = atof(val_str);
            if(redis_sock->stream) {
                read_tv.tv_sec  = (time_t)redis_sock->read_timeout;
                read_tv.tv_usec = (int)((redis_sock->read_timeout -
                                         read_tv.tv_sec) * 1000000);
                php_stream_set_option(redis_sock->stream,
                                      PHP_STREAM_OPTION_READ_TIMEOUT, 0,
                                      &read_tv);
            }
            RETURN_TRUE;
        case REDIS_OPT_SCAN:
            val_long = atol(val_str);
            if(val_long==REDIS_SCAN_NORETRY || val_long==REDIS_SCAN_RETRY) {
                redis_sock->scan = val_long;
                RETURN_TRUE;
            }
            break;
        case REDIS_OPT_FAILOVER:
            val_long = atol(val_str);
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
    strlen_t key_len;

    if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &key, &key_len)
                             ==FAILURE)
    {
        RETURN_FALSE;
    }

    if(redis_sock->prefix != NULL && redis_sock->prefix_len>0) {
        redis_key_prefix(redis_sock, &key, &key_len);
        RETVAL_STRINGL(key, key_len);
        efree(key);
    } else {
        RETURN_STRINGL(key, key_len);
    }
}

void redis_serialize_handler(INTERNAL_FUNCTION_PARAMETERS,
                             RedisSock *redis_sock)
{
    zval *z_val;
    char *val;
    strlen_t val_len;

    if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "z", &z_val)==FAILURE) {
        RETURN_FALSE;
    }

    int val_free = redis_serialize(redis_sock, z_val, &val, &val_len TSRMLS_CC);

    RETVAL_STRINGL(val, val_len);
    if(val_free) efree(val);
}

void redis_unserialize_handler(INTERNAL_FUNCTION_PARAMETERS,
                               RedisSock *redis_sock, zend_class_entry *ex)
{
    char *value;
    strlen_t value_len;

    // Parse our arguments
    if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &value, &value_len)
                                    == FAILURE)
    {
        RETURN_FALSE;
    }

    // We only need to attempt unserialization if we have a serializer running
    if (redis_sock->serializer == REDIS_SERIALIZER_NONE) {
        // Just return the value that was passed to us
        RETURN_STRINGL(value, value_len);
    }
    zval zv, *z_ret = &zv;
    if (!redis_unserialize(redis_sock, value, value_len, z_ret TSRMLS_CC)) {
        // Badly formed input, throw an execption
        zend_throw_exception(ex, "Invalid serialized data, or unserialization error", 0 TSRMLS_CC);
        RETURN_FALSE;
    }
    RETURN_ZVAL(z_ret, 1, 0);
}

/* vim: set tabstop=4 softtabstop=4 expandtab shiftwidth=4: */
