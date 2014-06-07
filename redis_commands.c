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
int
redis_key_long_val_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock,
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

/* vim: set tabstop=4 softtabstops=4 noexpandtab shiftwidth=4: */
