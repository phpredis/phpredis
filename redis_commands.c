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

#include "redis_commands.h"

/* Generic commands based on method signature and what kind of things we're
 * processing.  Lots of Redis commands take something like key, value, or
 * key, value long.  Each unique signature like this is written only once */

/* A command that takes no arguments */
int redis_empty_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock,
                    char *kw, char **cmd, int *cmd_len, short *slot,
                    void **ctx)
{
    *cmd_len = redis_cmd_format_static(cmd, kw, "");
    return SUCCESS;
}

/* Generic command where we just take a string and do nothing to it*/
int redis_str_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock, char *kw,
                  char **cmd, int *cmd_len, short *slot, void **ctx)
{
    char *arg;
    int arg_len;

    // Parse args
    if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &arg, &arg_len)
                             ==FAILURE)
    {
        return FAILURE;
    }

    // Build the command without molesting the string
    *cmd_len = redis_cmd_format_static(cmd, kw, "s", arg, arg_len);

    return SUCCESS;
}

/* Key, long, zval (serialized) */
int redis_key_long_val_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock,
                           char *kw, char **cmd, int *cmd_len, short *slot,
                           void **ctx)
{
    char *key = NULL, *val=NULL;
    int key_len, val_len, val_free, key_free;
    long expire;
    zval *z_val;

    if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "slz", &key, &key_len,
                             &expire, &z_val)==FAILURE)
    {
        return FAILURE;
    }

    // Serialize value, prefix key
    val_free = redis_serialize(redis_sock, z_val, &val, &val_len TSRMLS_CC);
    key_free = redis_key_prefix(redis_sock, &key, &key_len);

    // Construct our command
    *cmd_len = redis_cmd_format_static(cmd, kw, "sls", key, key_len, expire,
                                      val, val_len);

    // Set the slot if directed
    CMD_SET_SLOT(slot,key,key_len);

    if(val_free) STR_FREE(val);
    if(key_free) efree(key);

    return SUCCESS;
}

/* Generic key, long, string (unserialized) */
int redis_key_long_str_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock,
                           char *kw, char **cmd, int *cmd_len, short *slot,
                           void **ctx)
{
    char *key, *val;
    int key_len, val_len, key_free;
    long lval;

    if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "sls", &key, &key_len,
                             &lval, &val, &val_len)==FAILURE)
    {
        return FAILURE;
    }

    // Prefix our key if requested
    key_free = redis_key_prefix(redis_sock, &key, &key_len);
    
    // Construct command
    *cmd_len = redis_cmd_format_static(cmd, kw, "sds", key, key_len, (int)lval,
        val, val_len);

    // Set slot
    CMD_SET_SLOT(slot,key,key_len);

    // Free our key if we prefixed
    if(key_free) efree(key);

    return SUCCESS;
}

/* Generic command construction when we just take a key and value */
int redis_kv_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock,
                 char *kw, char **cmd, int *cmd_len, short *slot, 
                 void **ctx)
{
    char *key, *val;
    int key_len, val_len, key_free, val_free;
    zval *z_val;

    if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "sz", &key, &key_len,
                             &z_val)==FAILURE)
    {
        return FAILURE;
    }

    val_free = redis_serialize(redis_sock, z_val, &val, &val_len TSRMLS_CC);
    key_free = redis_key_prefix(redis_sock, &key, &key_len);

    // Construct our command
    *cmd_len = redis_cmd_format_static(cmd, kw, "ss", key, key_len, val,
                                       val_len);

    // Set our slot if directed
    CMD_SET_SLOT(slot,key,key_len);

    if(val_free) STR_FREE(val);
    if(key_free) efree(key);

    return SUCCESS;
}

/* Generic command that takes a key and an unserialized value */
int redis_key_str_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock,
                      char *kw, char **cmd, int *cmd_len, short *slot, 
                      void **ctx)
{
    char *key, *val;
    int key_len, val_len, key_free;

    if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "ss", &key, &key_len,
                             &val, &val_len)==FAILURE)
    {
        return FAILURE;
    }

    // Prefix key
    key_free = redis_key_prefix(redis_sock, &key, &key_len);

    // Construct command
    *cmd_len = redis_cmd_format_static(cmd, kw, "ss", key, key_len, val,
                                       val_len);

    // Set slot if directed
    CMD_SET_SLOT(slot,key,key_len);

    return SUCCESS;
}

/* Key, string, string without serialization (ZCOUNT, ZREMRANGEBYSCORE) */
int redis_key_str_str_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock,
                          char *kw, char **cmd, int *cmd_len, short *slot, 
                          void **ctx)
{
    char *key, *val1, *val2;
    int key_len, val1_len, val2_len, key_free;

    if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "sss", &key, &key_len,
                             &val1, &val1_len, &val2, &val2_len)==FAILURE)
    {
        return FAILURE;
    }

    // Prefix key
    key_free = redis_key_prefix(redis_sock, &key, &key_len);

    // Construct command
    *cmd_len = redis_cmd_format_static(cmd, kw, "sss", key, key_len, val1,
        val1_len, val2, val2_len);

    // Set slot
    CMD_SET_SLOT(slot,key,key_len);

    // Free key if prefixed
    if(key_free) efree(key);

    // Success!
    return SUCCESS;
}

/* Generic command that takes two keys */
int redis_key_key_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock,
                      char *kw, char **cmd, int *cmd_len, short *slot, 
                      void **ctx)
{
    char *key1, *key2;
    int key1_len, key2_len;
    int key1_free, key2_free;

    if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "ss", &key1, &key1_len,
                             &key2, &key2_len)==FAILURE)
    {
        return FAILURE;
    }

    // Prefix both keys
    key1_free = redis_key_prefix(redis_sock, &key1, &key1_len);
    key2_free = redis_key_prefix(redis_sock, &key2, &key2_len);

    // If a slot is requested, we can test that they hash the same
    if(slot) {
        // Slots where these keys resolve
        short slot1 = cluster_hash_key(key1, key1_len);
        short slot2 = cluster_hash_key(key2, key2_len);
        
        // Check if Redis would give us a CROSSLOT error
        if(slot1 != slot2) {
            php_error_docref(NULL TSRMLS_CC, E_WARNING,
                "Keys don't hash to the same slot");
            if(key1_free) efree(key1);
            if(key2_free) efree(key2);
            return FAILURE;
        }

        // They're both the same
        *slot = slot1;
    }

    // Construct our command
    *cmd_len = redis_cmd_format_static(cmd, kw, "ss", key1, key1_len, key2,
                                       key2_len);

    return SUCCESS;
}

/* Generic command construction where we take a key and a long */
int redis_key_long_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock,
                       char *kw, char **cmd, int *cmd_len, short *slot, 
                       void **ctx)
{
    char *key;
    int key_len, key_free;
    long lval;

    if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "sl", &key, &key_len,
                             &lval)==FAILURE)
    {
        return FAILURE;
    }

    // Prefix key
    key_free = redis_key_prefix(redis_sock, &key, &key_len);

    // Disallow zero length keys (for now)
    if(key_len == 0) {
        if(key_free) efree(key);
        return FAILURE;
    }

    // Construct our command
    *cmd_len = redis_cmd_format_static(cmd, kw, "sl", key, key_len, lval);

    // Set slot if directed
    CMD_SET_SLOT(slot, key, key_len);

    // Success!
    return SUCCESS;
}

/* key, long, long */
int redis_key_long_long_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock,
                            char *kw, char **cmd, int *cmd_len, short *slot, 
                            void **ctx)
{
    char *key;
    int key_len, key_free;
    long val1, val2;

    if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "sll", &key, &key_len,
                             &val1, &val2)==FAILURE)
    {
        return FAILURE;
    }

    // Prefix our key
    key_free = redis_key_prefix(redis_sock, &key, &key_len);

    // Construct command
    *cmd_len = redis_cmd_format_static(cmd, kw, "sll", key, key_len, val1,
                                       val2);

    // Set slot
    CMD_SET_SLOT(slot,key,key_len);

    if(key_free) efree(key);

    return SUCCESS;
}

/* Generic command where we take a single key */
int redis_key_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock,
                  char *kw, char **cmd, int *cmd_len, short *slot, 
                  void **ctx)
{
    char *key;
    int key_len, key_free;

    if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &key, &key_len)
                             ==FAILURE)
    {
        return FAILURE;
    }

    // Prefix our key
    key_free = redis_key_prefix(redis_sock, &key, &key_len);

    // Construct our command
    *cmd_len = redis_cmd_format_static(cmd, kw, "s", key, key_len);

    // Set slot if directed
    CMD_SET_SLOT(slot,key,key_len);

    if(key_free) efree(key);

    return SUCCESS;
}

/* Generic command where we take a key and a double */
int redis_key_dbl_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock,
                      char *kw, char **cmd, int *cmd_len, short *slot, 
                      void **ctx)
{
    char *key;
    int key_len, key_free;
    double val;

    if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "sd", &key, &key_len,
                             &val)==FAILURE)
    {
        return FAILURE;
    }

    // Prefix our key
    key_free = redis_key_prefix(redis_sock, &key, &key_len);

    // Construct our command
    *cmd_len = redis_cmd_format_static(cmd, kw, "sf", key, key_len, val);

    // Set slot if directed
    CMD_SET_SLOT(slot,key,key_len);

    if(key_free) efree(key);

    return SUCCESS;
}

/* ZRANGE/ZREVRANGE */
int redis_zrange_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock,
                     char *kw, char **cmd, int *cmd_len, int *withscores,
                     short *slot, void **ctx)
{
    char *key;
    int key_len, key_free;
    long start, end;
    zend_bool ws;

    if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "sll|b", &key, &key_len,
                             &start, &end, &ws)==FAILURE)
    {
        return FAILURE;
    }

    key_free = redis_key_prefix(redis_sock, &key, &key_len);
    if(ws) {
        *cmd_len = redis_cmd_format_static(cmd, kw, "sdds", key, key_len, start,
            end, "WITHSCORES", sizeof("WITHSCORES")-1);
    } else {
        *cmd_len = redis_cmd_format_static(cmd, kw, "sdd", key, key_len, start,
            end);
    }

    CMD_SET_SLOT(slot, key, key_len);

    // Free key, push out WITHSCORES option
    if(key_free) efree(key);
    *withscores = ws;

    return SUCCESS;
}

/* ZRANGEBYSCORE/ZREVRANGEBYSCORE */
int redis_zrangebyscore_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock,
                            char *kw, char **cmd, int *cmd_len, int *withscores,
                            short *slot, void **ctx)
{
    char *key;
    int key_len, key_free;
    char *start, *end;
    int start_len, end_len;
    int has_limit=0;
    long limit_low, limit_high;
    zval *z_opt=NULL, **z_ele;
    HashTable *ht_opt;

    if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "sss|a", &key, &key_len,
                             &start, &start_len, &end, &end_len, &z_opt)
                             ==FAILURE)
    {
        return FAILURE;
    }

    // Check for an options array
    if(z_opt && Z_TYPE_P(z_opt)==IS_ARRAY) {
        ht_opt = Z_ARRVAL_P(z_opt);

        // Check for WITHSCORES
        *withscores = (zend_hash_find(ht_opt,"withscores",sizeof("withscores"),
                       (void**)&z_ele)==SUCCESS && Z_TYPE_PP(z_ele)==IS_BOOL &&
                       Z_BVAL_PP(z_ele)==1);
        
        // LIMIT
        if(zend_hash_find(ht_opt,"limit",sizeof("limit"),(void**)&z_ele)
                          ==SUCCESS)
        {
            HashTable *ht_limit = Z_ARRVAL_PP(z_ele);
            zval **z_off, **z_cnt;
            if(zend_hash_index_find(ht_limit,0,(void**)&z_off)==SUCCESS &&
               zend_hash_index_find(ht_limit,1,(void**)&z_cnt)==SUCCESS &&
               Z_TYPE_PP(z_off)==IS_LONG && Z_TYPE_PP(z_cnt)==IS_LONG)
            {
                has_limit  = 1;
                limit_low  = Z_LVAL_PP(z_off);
                limit_high = Z_LVAL_PP(z_cnt);
            } 
        }
    } 
    
    // Prefix our key, set slot
    key_free = redis_key_prefix(redis_sock, &key, &key_len);
    CMD_SET_SLOT(slot,key,key_len);
    
    // Construct our command
    if(*withscores) {
        if(has_limit) {
            *cmd_len = redis_cmd_format_static(cmd, kw, "ssssdds", key, key_len,
                start, start_len, end, end_len, "LIMIT", 5, limit_low,
                limit_high, "WITHSCORES", 10);
        } else {
            *cmd_len = redis_cmd_format_static(cmd, kw, "ssss", key, key_len,
                start, start_len, end, end_len, "WITHSCORES", 10);
        }
    } else {
        if(has_limit) {
            *cmd_len = redis_cmd_format_static(cmd, kw, "ssssdd", key, key_len,
                start, start_len, end, end_len, "LIMIT", 5, limit_low, 
                limit_high);
        } else {
            *cmd_len = redis_cmd_format_static(cmd, kw, "sss", key, key_len,
                start, start_len, end, end_len);
        }
    }

    // Free our key if we prefixed
    if(key_free) efree(key);

    return SUCCESS;
}

/* Commands with specific signatures or that need unique functions because they
 * have specific processing (argument validation, etc) that make them unique */

/* SET */
int redis_set_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock,
                  char **cmd, int *cmd_len, short *slot, void **ctx)
{
    zval *z_value, *z_opts=NULL;
    char *key = NULL, *val = NULL, *exp_type = NULL, *set_type = NULL;
    int key_len, val_len, key_free, val_free;
    long expire = -1;
     
    // Make sure the function is being called correctly
    if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "sz|z", &key, &key_len,
                             &z_value, &z_opts)==FAILURE)
    {
        return FAILURE;
    }

    // Serialize and key prefix if required
    val_free = redis_serialize(redis_sock, z_value, &val, &val_len TSRMLS_CC);
    key_free = redis_key_prefix(redis_sock, &key, &key_len);

    // Check for an options array
    if(z_opts && Z_TYPE_P(z_opts) == IS_ARRAY) {
        HashTable *kt = Z_ARRVAL_P(z_opts);
        int type;
        unsigned int ht_key_len;
        unsigned long idx;
        char *k;
        zval **v;

        /* Iterate our option array */
        for(zend_hash_internal_pointer_reset(kt);
            zend_hash_has_more_elements(kt) == SUCCESS;
            zend_hash_move_forward(kt))
        {
            // Grab key and value
            type = zend_hash_get_current_key_ex(kt, &k, &ht_key_len, &idx, 0,
                                                NULL);
            zend_hash_get_current_data(kt, (void**)&v);

            if(type == HASH_KEY_IS_STRING && (Z_TYPE_PP(v) == IS_LONG) &&
               (Z_LVAL_PP(v) > 0) && IS_EX_PX_ARG(k))
            {
                exp_type = k;
                expire = Z_LVAL_PP(v);
            } else if(Z_TYPE_PP(v) == IS_STRING &&
                      IS_NX_XX_ARG(Z_STRVAL_PP(v)))
            {
                set_type = Z_STRVAL_PP(v);
            }
        }
    } else if(z_opts && Z_TYPE_P(z_opts) == IS_LONG) {
        expire = Z_LVAL_P(z_opts);
    }

    /* Now let's construct the command we want */
    if(exp_type && set_type) {
        /* SET <key> <value> NX|XX PX|EX <timeout> */
        *cmd_len = redis_cmd_format_static(cmd, "SET", "ssssl", key, key_len,
                                           val, val_len, set_type, 2, exp_type,
                                           2, expire);
    } else if(exp_type) {
        /* SET <key> <value> PX|EX <timeout> */
        *cmd_len = redis_cmd_format_static(cmd, "SET", "sssl", key, key_len,
                                           val, val_len, exp_type, 2, expire);
    } else if(set_type) {
        /* SET <key> <value> NX|XX */
        *cmd_len = redis_cmd_format_static(cmd, "SET", "sss", key, key_len, val,
                                           val_len, set_type, 2);
    } else if(expire > 0) {
        /* Backward compatible SETEX redirection */
        *cmd_len = redis_cmd_format_static(cmd, "SETEX", "sls", key, key_len,
                                           expire, val, val_len);
    } else {
        /* SET <key> <value> */
        *cmd_len = redis_cmd_format_static(cmd, "SET", "ss", key, key_len, val,
                                           val_len);
    }

    // If we've been passed a slot pointer, return the key's slot
    CMD_SET_SLOT(slot,key,key_len);

    if(key_free) efree(key);
    if(val_free) efree(val);

    return SUCCESS;
}

/* BRPOPLPUSH */
int redis_brpoplpush_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock,
                         char **cmd, int *cmd_len, short *slot, void **ctx)
{
    char *key1, *key2;
    int key1_len, key2_len;
    int key1_free, key2_free;
    short slot1, slot2;
    long timeout;

    if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "ssl", &key1, &key1_len,
                             &key2, &key2_len, &timeout)==FAILURE)
    {
        return FAILURE;
    }

    // Key prefixing
    key1_free = redis_key_prefix(redis_sock, &key1, &key1_len);
    key2_free = redis_key_prefix(redis_sock, &key2, &key2_len);

    // In cluster mode, verify the slots match
    if(slot) {
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
        *cmd_len = redis_cmd_format_static(cmd, "RPOPLPUSH", "ss", key1, 
           key1_len, key2, key2_len);
    } else {
        *cmd_len = redis_cmd_format_static(cmd, "BRPOPLPUSH", "ssd", key1, 
           key1_len, key2, key2_len, timeout);
    }

    return SUCCESS;
}

/* HINCRBY */
int redis_hincrby_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock,
                      char **cmd, int *cmd_len, short *slot, void **ctx)
{
    char *key, *mem;
    int key_len, mem_len, key_free;
    long byval;

    if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "ssl", &key, &key_len,
                             &mem, &mem_len, &byval)==FAILURE)
    {
        return FAILURE;
    }

    // Prefix our key if necissary 
    key_free = redis_key_prefix(redis_sock, &key, &key_len);
    
    // Construct command
    *cmd_len = redis_cmd_format_static(cmd, "HINCRBY", "ssd", key, key_len, mem,
                                       mem_len, byval);
    // Set slot
    CMD_SET_SLOT(slot,key,key_len);
    
    // Success
    return SUCCESS;
}

/* HINCRBYFLOAT */
int redis_hincrbyfloat_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock,
                           char **cmd, int *cmd_len, short *slot, void **ctx)
{
    char *key, *mem;
    int key_len, mem_len, key_free;
    double byval;

    if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "ssd", &key, &key_len,
                             &mem, &mem_len, &byval)==FAILURE)
    {
        return FAILURE;
    }

    // Prefix key
    key_free = redis_key_prefix(redis_sock, &key, &key_len);

    // Construct command
    *cmd_len = redis_cmd_format_static(cmd, "HINCRBYFLOAT", "ssf", key, key_len,
        mem, mem_len, byval);

    // Set slot
    CMD_SET_SLOT(slot,key,key_len);

    // Success
    return SUCCESS;
}

/* HMGET */
int redis_hmget_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock,
                    char **cmd, int *cmd_len, short *slot, void **ctx)
{
    char *key;
    zval *z_arr, **z_mems, **z_mem;
    int i, count, valid=0, key_len, key_free;
    HashTable *ht_arr;
    HashPosition ptr;
    smart_str cmdstr = {0};

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

    // Prefix our key
    key_free = redis_key_prefix(redis_sock, &key, &key_len);

    // Allocate memory for the max members we'll grab
    z_mems = ecalloc(count, sizeof(zval*));

    // Iterate over our member array
    for(zend_hash_internal_pointer_reset_ex(ht_arr, &ptr);
        zend_hash_get_current_data_ex(ht_arr, (void**)&z_mem, &ptr)==SUCCESS;
        zend_hash_move_forward_ex(ht_arr, &ptr))
    {
        // We can only handle string or long values here
        if(Z_TYPE_PP(z_mem)==IS_STRING || Z_TYPE_PP(z_mem)==IS_LONG) {
            // Copy into our member array
            MAKE_STD_ZVAL(z_mems[valid]);
            *z_mems[valid] = **z_mem;
            zval_copy_ctor(z_mems[valid]);
            convert_to_string(z_mems[valid]);

            // Increment the member count to actually send
            valid++;
        }
    }

    // If nothing was valid, fail
    if(valid == 0) {
        if(key_free) efree(key);
        efree(z_mems);
        return FAILURE;
    }

    // Start command construction
    redis_cmd_init_sstr(&cmdstr, valid+1, "HMGET", sizeof("HMGET")-1);
    redis_cmd_append_sstr(&cmdstr, key, key_len);

    // Iterate over members, appending as arguments
    for(i=0;i<valid;i++) {
        redis_cmd_append_sstr(&cmdstr, Z_STRVAL_P(z_mems[i]), 
            Z_STRLEN_P(z_mems[i]));
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
    int key_len, key_free, count, ktype;
    unsigned long idx;
    zval *z_arr;
    HashTable *ht_vals;
    HashPosition pos;
    smart_str cmdstr = {0};

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
    for(zend_hash_internal_pointer_reset_ex(ht_vals, &pos);
        zend_hash_has_more_elements_ex(ht_vals, &pos)==SUCCESS;
        zend_hash_move_forward_ex(ht_vals, &pos))
    {
        char *val, kbuf[40];
        int val_len, val_free;
        unsigned int key_len;
        zval **z_val;

        // Grab our key, and value for this element in our input
        ktype = zend_hash_get_current_key_ex(ht_vals, &key, 
            &key_len, &idx, 0, &pos);
        zend_hash_get_current_data_ex(ht_vals, (void**)&z_val, &pos);

        // If the hash key is an integer, convert it to a string
        if(ktype != HASH_KEY_IS_STRING) {
            key_len = snprintf(kbuf, sizeof(kbuf), "%ld", (long)idx);
            key = (char*)kbuf;
        } else {
            // Length returned includes the \0
            key_len--;
        }

        // Serialize value (if directed)
        val_free = redis_serialize(redis_sock, *z_val, &val, &val_len 
            TSRMLS_CC);

        // Append the key and value to our command
        redis_cmd_append_sstr(&cmdstr, key, key_len);
        redis_cmd_append_sstr(&cmdstr, val, val_len);
    }

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

/* BITPOS */
int redis_bitpos_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock,
                     char **cmd, int *cmd_len, short *slot, void **ctx)
{
    char *key;
    int argc, key_len, key_free;
    long bit, start, end;

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
    
    // Prefix key
    key_free = redis_key_prefix(redis_sock, &key, &key_len);

    // Construct command based on arg count
    if(argc == 2) {
        *cmd_len = redis_cmd_format_static(cmd, "BITPOS", "sd", key, key_len, 
            bit);
    } else if(argc == 3) {
        *cmd_len = redis_cmd_format_static(cmd, "BITPOS", "sdd", key, key_len, 
            bit, start);
    } else {
        *cmd_len = redis_cmd_format_static(cmd, "BITPOS", "sddd", key, key_len, 
            bit, start, end);
    }

    // Set our slot
    CMD_SET_SLOT(slot, key, key_len);

    return SUCCESS;
}

/* BITOP */
int redis_bitop_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock,
                    char **cmd, int *cmd_len, short *slot, void **ctx)
{
    zval **z_args;
    char *key;
    int key_len, i, key_free, argc = ZEND_NUM_ARGS();
    smart_str cmdstr = {0};
    short kslot;

    // Allocate space for args, parse them as an array
    z_args = emalloc(argc * sizeof(zval*));
    if(zend_get_parameters_array(ht, argc, z_args)==FAILURE ||
       argc < 3 || Z_TYPE_P(z_args[0]) != IS_STRING)
    {
        efree(z_args);
        return FAILURE;
    }

    // If we were passed a slot pointer, init to a sentinel value
    if(slot) *slot = -1;

    // Initialize command construction, add our operation argument
    redis_cmd_init_sstr(&cmdstr, argc, "BITOP", sizeof("BITOP")-1);
    redis_cmd_append_sstr(&cmdstr, Z_STRVAL_P(z_args[0]), 
        Z_STRLEN_P(z_args[0]));

    // Now iterate over our keys argument
    for(i=1;i<argc;i++) {
        // Make sure we've got a string
        convert_to_string(z_args[i]);
        
        // Grab this key and length
        key = Z_STRVAL_P(z_args[i]);
        key_len = Z_STRLEN_P(z_args[i]);

        // Prefix key, append
        key_free = redis_key_prefix(redis_sock, &key, &key_len);
        redis_cmd_append_sstr(&cmdstr, key, key_len);
  
        // Verify slot if this is a Cluster request
        if(slot) {
            kslot = cluster_hash_key(key, key_len);
            if(*slot == -1 || kslot != *slot) {
                php_error_docref(NULL TSRMLS_CC, E_WARNING,
                    "Warning, not all keys hash to the same slot!");
                if(key_free) efree(key);
                return FAILURE;
            }
            *slot = kslot;
        }
  
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
    int key_len, key_free;
    long start = 0, end = -1;

    if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s|ll", &key, &key_len,
                             &start, &end)==FAILURE)
    {
        return FAILURE;
    }

    // Prefix key, construct command
    key_free = redis_key_prefix(redis_sock, &key, &key_len);
    *cmd_len = redis_cmd_format_static(cmd, "BITCOUNT", "sdd", key, key_len,
        (int)start, (int)end);
    
    // Set our slot
    CMD_SET_SLOT(slot,key,key_len);

    // Fre key if we prefixed it
    if(key_free) efree(key);

    return SUCCESS;
}

/* PFADD and PFMERGE are the same except that in one case we serialize,
 * and in the other case we key prefix */
static int redis_gen_pf_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock,
                            char *kw, int kw_len, int is_keys, char **cmd, 
                            int *cmd_len, short *slot)
{
    zval *z_arr, **z_ele;
    HashTable *ht_arr;
    HashPosition pos;
    smart_str cmdstr = {0};
    char *mem, *key;
    int key_len, key_free;
    int mem_len, mem_free, argc=1;

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
    for(zend_hash_internal_pointer_reset_ex(ht_arr, &pos);
        zend_hash_get_current_data_ex(ht_arr, (void**)&z_ele, &pos)==SUCCESS;
        zend_hash_move_forward_ex(ht_arr, &pos))
    {
        zval *z_tmp = NULL;

        // Prefix keys, serialize values
        if(is_keys) {
            if(Z_TYPE_PP(z_ele)!=IS_STRING) {
                MAKE_STD_ZVAL(z_tmp);
                *z_tmp = **z_ele;
                convert_to_string(z_tmp);
                z_ele = &z_tmp;
            }
            mem = Z_STRVAL_PP(z_ele);
            mem_len = Z_STRLEN_PP(z_ele);
            
            // Key prefix
            mem_free = redis_key_prefix(redis_sock, &mem, &mem_len);

            // Verify slot
            if(slot && *slot != cluster_hash_key(mem, mem_len)) {
                php_error_docref(0 TSRMLS_CC, E_WARNING,
                    "All keys must hash to the same slot!");
                if(key_free) efree(key);
                if(z_tmp) {
                    zval_dtor(z_tmp);
                    efree(z_tmp);
                }
                return FAILURE;
            }
        } else {
            mem_free = redis_serialize(redis_sock, *z_ele, &mem, &mem_len 
                TSRMLS_CC);
            
            if(!mem_free) {
                if(Z_TYPE_PP(z_ele)!=IS_STRING) {
                    MAKE_STD_ZVAL(z_tmp);
                    *z_tmp = **z_ele;
                    convert_to_string(z_tmp);
                    z_ele = &z_tmp;
                }
                mem = Z_STRVAL_PP(z_ele);
                mem_len = Z_STRLEN_PP(z_ele);
            }
        }

        // Append our key or member
        redis_cmd_append_sstr(&cmdstr, mem, mem_len);

        // Clean up our temp val if it was used
        if(z_tmp) {
            zval_dtor(z_tmp);
            efree(z_tmp);
            z_tmp = NULL;
        }

        // Clean up prefixed or serialized data
        if(mem_free) {
            if(!is_keys) {
                STR_FREE(mem);
            } else {
                efree(mem);
            }
        }
    }

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

/* AUTH -- we need to update the password stored in RedisSock */
int redis_auth_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock,
                   char **cmd, int *cmd_len, short *slot, void **ctx)
{
    char *pw;
    int pw_len;

    if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &pw, &pw_len)
                             ==FAILURE)
    {
        return FAILURE;
    }

    // Construct our AUTH command
    *cmd_len = redis_cmd_format_static(cmd, "AUTH", "s", pw, pw_len);

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
    int key_len, key_free;
    long offset;
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

    key_free = redis_key_prefix(redis_sock, &key, &key_len);
    *cmd_len = redis_cmd_format_static(cmd, "SETBIT", "sdd", key, key_len,
        (int)offset, (int)val);

    CMD_SET_SLOT(slot, key, key_len);

    if(key_free) efree(key);

    return SUCCESS;
}

/* LINSERT */
int redis_linsert_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock,
                      char **cmd, int *cmd_len, short *slot, void **ctx)
{
    char *key, *pivot, *pos, *val;
    int key_len, pivot_len, pos_len, val_len;
    int key_free, pivot_free, val_free;
    zval *z_val, *z_pivot;

    if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "sszz", &key, &key_len,
                             &pos, &pos_len, &z_pivot, &z_val)==FAILURE)
    {
        return FAILURE;
    }

    // Validate position
    if(strncasecmp(pos, "after", 5)!=0 && strncasecmp(pos, "before", 6)==0) {
        php_error_docref(NULL TSRMLS_CC, E_WARNING,
            "Position must be either 'BEFORE' or 'AFTER'");
        return FAILURE;
    }

    // Prefix key, serialize value and position
    key_free = redis_key_prefix(redis_sock, &key, &key_len);
    val_free = redis_serialize(redis_sock, z_val, &val, &val_len TSRMLS_CC);
    pivot_free = redis_serialize(redis_sock, z_pivot, &pivot, &pivot_len 
        TSRMLS_CC);

    // Construct command
    *cmd_len = redis_cmd_format_static(cmd, "LINSERT", "ssss", key, key_len,
        pos, pos_len, pivot, pivot_len, val, val_len);

    // Set slot
    CMD_SET_SLOT(slot, key, key_len);

    // Clean up
    if(val_free) STR_FREE(val);
    if(key_free) efree(key);
    if(pivot_free) STR_FREE(pivot);

    // Success
    return SUCCESS;
}

/* LREM */
int redis_lrem_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock, 
                   char **cmd, int *cmd_len, short *slot, void **ctx)
{
    char *key, *val;
    int key_len, val_len, key_free, val_free;
    long count = 0;
    zval *z_val;

    if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "sz|l", &key, &key_len,
                             &z_val, &count)==FAILURE)
    {
        return FAILURE;
    }

    // Prefix key, serialize value
    key_free = redis_key_prefix(redis_sock, &key, &key_len);
    val_free = redis_serialize(redis_sock, z_val, &val, &val_len TSRMLS_CC);

    // Construct command
    *cmd_len = redis_cmd_format_static(cmd, "LREM", "sds", key, key_len, count,
        val, val_len);

    // Set slot
    CMD_SET_SLOT(slot, key, key_len);

    // Cleanup
    if(val_free) STR_FREE(val);
    if(key_free) efree(key);

    // Success!
    return SUCCESS;
}

int redis_smove_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock,
                    char **cmd, int *cmd_len, short *slot, void **ctx)
{
    char *src, *dst, *val;
    int src_len, dst_len, val_len;
    int val_free, src_free, dst_free;
    zval *z_val;

    if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "ssz", &src, &src_len,
                             &dst, &dst_len, &z_val)==FAILURE)
    {
        return FAILURE;
    }

    val_free = redis_serialize(redis_sock, z_val, &val, &val_len TSRMLS_CC);
    src_free = redis_key_prefix(redis_sock, &src, &src_len);
    dst_free = redis_key_prefix(redis_sock, &dst, &dst_len);

    // Protect against a CROSSSLOT error
    if(slot) {
        short slot1 = cluster_hash_key(src, src_len);
        short slot2 = cluster_hash_key(dst, dst_len);
        if(slot1 != slot2) {
            php_error_docref(0 TSRMLS_CC, E_WARNING,
                "Source and destination keys don't hash to the same slot!");
            if(val_free) STR_FREE(val);
            if(src_free) efree(src);
            if(dst_free) efree(dst);
            return FAILURE;
        }
        *slot = slot1;
    }

    // Construct command
    *cmd_len = redis_cmd_format_static(cmd, "SMOVE", "sss", src, src_len, dst,
        dst_len, val, val_len);

    // Cleanup
    if(val_free) STR_FREE(val);
    if(src_free) efree(src);
    if(dst_free) efree(dst);

    // Succcess!
    return SUCCESS;
}

/* Generic command construction for HSET and HSETNX */
static int gen_hset_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock,
                        char *kw, char **cmd, int *cmd_len, short *slot)
{
    char *key, *mem, *val;
    int key_len, mem_len, val_len;
    int val_free, key_free;
    zval *z_val;

    if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "ssz", &key, &key_len,
                             &mem, &mem_len, &z_val)==FAILURE)
    {
        return FAILURE;
    }

    // Prefix/serialize
    val_free = redis_serialize(redis_sock, z_val, &val, &val_len TSRMLS_CC);
    key_free = redis_key_prefix(redis_sock, &key, &key_len);
    
    // Construct command
    *cmd_len = redis_cmd_format_static(cmd, kw, "sss", key, key_len, mem,
        mem_len, val, val_len);

    // Set slot
    CMD_SET_SLOT(slot,key,key_len);

    // Cleanup
    if(key_free) STR_FREE(val);
    if(val_free) efree(key);

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
    int key_len, key_free;
    long count;

    if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s|l", &key, &key_len,
                             &count)==FAILURE)
    {
        return FAILURE;
    }

    // Prefix key if requested
    key_free = redis_key_prefix(redis_sock, &key, &key_len);

    // Set our have count flag
    *have_count = ZEND_NUM_ARGS() == 2;

    // Two args means we have the optional COUNT
    if(*have_count) {
        *cmd_len = redis_cmd_format_static(cmd, "SRANDMEMBER", "sl", key, 
            key_len, count);
    } else {
        *cmd_len = redis_cmd_format_static(cmd, "SRANDMEMBER", "s", key,
            key_len);
    }

    // Set slot
    CMD_SET_SLOT(slot,key,key_len);

    // Cleanup
    if(key_free) efree(key);

    return SUCCESS;
}

/* ZINCRBY */
int redis_zincrby_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock,
                      char **cmd, int *cmd_len, short *slot, void **ctx)
{
    char *key, *mem;
    int key_len, mem_len;
    int key_free, mem_free;
    double incrby;
    zval *z_val;

    if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "sdz", &key, &key_len,
                             &incrby, &z_val)==FAILURE)
    {
        return FAILURE;
    }

    // Prefix key, serialize
    key_free = redis_key_prefix(redis_sock, &key, &key_len);
    mem_free = redis_serialize(redis_sock, z_val, &mem, &mem_len TSRMLS_CC);

    *cmd_len = redis_cmd_format_static(cmd, "ZINCRBY", "sfs", key, key_len,
        incrby, mem, mem_len);

    CMD_SET_SLOT(slot,key,key_len);

    // Cleanup
    if(key_free) efree(key);
    if(mem_free) STR_FREE(mem);

    return SUCCESS;
}

/* SORT */
int redis_sort_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock, 
                   int *using_store, char **cmd, int *cmd_len, short *slot, 
                   void **ctx)
{
    zval *z_opts=NULL, **z_ele, *z_argv;
    char *key;
    HashTable *ht_opts;
    smart_str cmdstr = {0};
    int key_len, key_free;

    if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s|a", &key, &key_len,
                             &z_opts)==FAILURE)
    {
        return FAILURE;
    }

    // Default that we're not using store
    *using_store = 0;

    // Handle key prefixing
    key_free = redis_key_prefix(redis_sock, &key, &key_len);

    // If we don't have an options array, the command is quite simple
    if(!z_opts) {
        // Construct command
        *cmd_len = redis_cmd_format_static(cmd, "SORT", "s", key, key_len);
        
        // Push out slot, store flag, and clean up
        *using_store = 0;
        CMD_SET_SLOT(slot,key,key_len);
        if(key_free) efree(key);

        return SUCCESS;
    }

    // Create our hash table to hold our sort arguments
    ALLOC_INIT_ZVAL(z_argv);
    array_init(z_argv);

    // SORT <key>
    add_next_index_stringl(z_argv, key, key_len, 0);

    // Set slot
    CMD_SET_SLOT(slot,key,key_len);

    // Grab the hash table
    ht_opts = Z_ARRVAL_P(z_opts);

    // Handle BY pattern
    if((zend_hash_find(ht_opts, "by", sizeof("by"), (void**)&z_ele)==SUCCESS ||
       zend_hash_find(ht_opts, "BY", sizeof("BY"), (void**)&z_ele)==SUCCESS) &&
       Z_TYPE_PP(z_ele)==IS_STRING)
    {
        // "BY" option is disabled in cluster
        if(slot) {
            php_error_docref(NULL TSRMLS_CC, E_WARNING,
                "SORT BY option is not allowed in Redis Cluster");
            if(key_free) efree(key);
            zval_dtor(z_argv);
            efree(z_argv);
            return FAILURE;
        }

        // ... BY <pattern>
        add_next_index_stringl(z_argv, "BY", sizeof("BY")-1, 1);
        add_next_index_stringl(z_argv,Z_STRVAL_PP(z_ele),Z_STRLEN_PP(z_ele),0);
    }

    // Handle ASC/DESC option
    if((zend_hash_find(ht_opts,"sort",sizeof("sort"),(void**)&z_ele)==SUCCESS ||
       zend_hash_find(ht_opts,"SORT",sizeof("SORT"),(void**)&z_ele)==SUCCESS) &&
       Z_TYPE_PP(z_ele)==IS_STRING)
    {
        // 'asc'|'desc'
        add_next_index_stringl(z_argv,Z_STRVAL_PP(z_ele),Z_STRLEN_PP(z_ele),0);
    }

    // STORE option
    if((zend_hash_find(ht_opts,"store",6,(void**)&z_ele)==SUCCESS ||
        zend_hash_find(ht_opts,"STORE",6,(void**)&z_ele)==SUCCESS) && 
        Z_TYPE_PP(z_ele)==IS_STRING)
    {
        // Slot verification
        int cross_slot = slot && *slot != cluster_hash_key(
            Z_STRVAL_PP(z_ele),Z_STRLEN_PP(z_ele));

        if(cross_slot) {
            php_error_docref(0 TSRMLS_CC, E_WARNING,
                "Error, SORT key and STORE key have different slots!");
            if(key_free) efree(key);
            zval_dtor(z_argv);
            efree(z_argv);
            return FAILURE;
        }

        // STORE <key>
        add_next_index_stringl(z_argv,"STORE",sizeof("STORE")-1, 1);
        add_next_index_stringl(z_argv,Z_STRVAL_PP(z_ele),Z_STRLEN_PP(z_ele),0);
        
        // We are using STORE
        *using_store = 1;
    }

    // GET option
    if((zend_hash_find(ht_opts,"get",4,(void**)&z_ele)==SUCCESS ||
        zend_hash_find(ht_opts,"GET",4,(void**)&z_ele)==SUCCESS) &&
        (Z_TYPE_PP(z_ele)==IS_STRING || Z_TYPE_PP(z_ele)==IS_ARRAY))
    {
        // Disabled in cluster
        if(slot) {
            php_error_docref(NULL TSRMLS_CC, E_WARNING,
                "GET option for SORT disabled in Redis Cluster");
            if(key_free) efree(key);
            zval_dtor(z_argv);
            efree(z_argv);
            return FAILURE;
        }

        // If it's a string just add it
        if(Z_TYPE_PP(z_ele)==IS_STRING) {
            add_next_index_stringl(z_argv,"GET",sizeof("GET")-1,0);
            add_next_index_stringl(z_argv,Z_STRVAL_PP(z_ele),
                Z_STRLEN_PP(z_ele), 0);
        } else {
            HashTable *ht_keys = Z_ARRVAL_PP(z_ele);
            int added=0;
            
            // Add our "GET" option
            add_next_index_stringl(z_argv,"GET",sizeof("GET")-1,0);

            for(zend_hash_internal_pointer_reset(ht_keys);
                zend_hash_has_more_elements(ht_keys)==SUCCESS;
                zend_hash_move_forward(ht_keys))
            {
                zval **z_key;

                // If we can't get the data, or it's not a string, skip
                if(zend_hash_get_current_data(ht_keys,(void**)&z_key)==FAILURE)
                    continue;
                if(Z_TYPE_PP(z_key)!=IS_STRING)
                    continue;
                
                // Add this key to our argv array
                add_next_index_stringl(z_argv, Z_STRVAL_PP(z_key), 
                    Z_STRLEN_PP(z_key), 0);
                added++;
            }

            // Make sure we were able to add at least one
            if(added==0) {
                php_error_docref(NULL TSRMLS_CC, E_WARNING,
                    "Array of GET values requested, but none are valid");
                if(key_free) efree(key);
                zval_dtor(z_argv);
                efree(z_argv);
                return FAILURE;
            }
        }
    }

    // ALPHA
    if((zend_hash_find(ht_opts,"alpha",6,(void**)&z_ele)==SUCCESS ||
        zend_hash_find(ht_opts,"ALPHA",6,(void**)&z_ele)==SUCCESS) &&
        Z_TYPE_PP(z_ele)==IS_BOOL && Z_BVAL_PP(z_ele)==1)
    {
        add_next_index_stringl(z_argv, "ALPHA", sizeof("ALPHA")-1,1);
    }

    // LIMIT <offset> <count>
    if((zend_hash_find(ht_opts,"limit",6,(void**)&z_ele)==SUCCESS ||
        zend_hash_find(ht_opts,"LIMIT",6,(void**)&z_ele)==SUCCESS) &&
        Z_TYPE_PP(z_ele)==IS_ARRAY)
    {
        HashTable *ht_off = Z_ARRVAL_PP(z_ele);
        zval **z_off, **z_cnt;

        if(zend_hash_index_find(ht_off, 0, (void**)&z_off)==SUCCESS &&
           zend_hash_index_find(ht_off, 1, (void**)&z_cnt)==SUCCESS)
        {
            if((Z_TYPE_PP(z_off)!=IS_STRING && Z_TYPE_PP(z_off)!=IS_LONG) ||
               (Z_TYPE_PP(z_cnt)!=IS_STRING && Z_TYPE_PP(z_cnt)!=IS_LONG))
            {
                php_error_docref(NULL TSRMLS_CC, E_WARNING,
                    "LIMIT options on SORT command must be longs or strings");
                if(key_free) efree(key);
                zval_dtor(z_argv);
                efree(z_argv);
                return FAILURE;
            }

            // Add LIMIT argument
            add_next_index_stringl(z_argv,"LIMIT",sizeof("LIMIT")-1,1);

            long low, high;
            if(Z_TYPE_PP(z_off)==IS_STRING) {
                low = atol(Z_STRVAL_PP(z_off));
            } else {
                low = Z_LVAL_PP(z_off);
            }
            if(Z_TYPE_PP(z_cnt)==IS_STRING) {
                high = atol(Z_STRVAL_PP(z_cnt));
            } else {
                high = Z_LVAL_PP(z_cnt);
            }

            // Add our two LIMIT arguments
            add_next_index_long(z_argv, low);
            add_next_index_long(z_argv, high);
        }
    }

    // Start constructing our command
    HashTable *ht_argv = Z_ARRVAL_P(z_argv);
    redis_cmd_init_sstr(&cmdstr, zend_hash_num_elements(ht_argv), "SORT",
        sizeof("SORT")-1);

    // Iterate through our arguments
    for(zend_hash_internal_pointer_reset(ht_argv);
        zend_hash_get_current_data(ht_argv, (void**)&z_ele)==SUCCESS;
        zend_hash_move_forward(ht_argv))
    {
        // Args are strings or longs
        if(Z_TYPE_PP(z_ele)==IS_STRING) {
            redis_cmd_append_sstr(&cmdstr,Z_STRVAL_PP(z_ele),
                Z_STRLEN_PP(z_ele));
        } else {
            redis_cmd_append_sstr_long(&cmdstr, Z_LVAL_PP(z_ele));
        }
    }

    // Free key if we prefixed, destroy argv array
    if(key_free) efree(key);
    zval_dtor(z_argv);
    efree(z_argv);

    // Push our length and command
    *cmd_len = cmdstr.len;
    *cmd     = cmdstr.c;

    // Success!
    return SUCCESS;
}

/* vim: set tabstop=4 softtabstops=4 noexpandtab shiftwidth=4: */
