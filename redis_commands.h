#ifndef REDIS_COMMANDS_H
#define REDIS_COMMANDS_H

#include "common.h"
#include "library.h"
#include "cluster_library.h"

/* Macro for setting the slot if we've been asked to */
#define CMD_SET_SLOT(slot,key,key_len) \
    if(slot) *slot = cluster_hash_key(key,key_len);

/* Redis command generics.  Many commands share common prototypes meaning that
 * we can write one function to handle all of them.  For example, there are
 * many COMMAND key value commands, or COMMAND key commands. */

int redis_empty_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock,
    char *kw, char **cmd, int *cmd_len, short *slot, void **ctx);

int redis_str_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock,
    char *kw, char **cmd, int *cmd_len, short *slot, void **ctx);

int redis_key_long_val_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock,
    char *kw, char **cmd, int *cmd_len, short *slot, void **ctx);

int redis_key_long_str_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock,
    char *kw, char **cmd, int *cmd_len, short *slot, void **ctx);

int redis_kv_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock,
    char *kw, char **cmd, int *cmd_len, short *slot, void **ctx);

int redis_key_str_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock,
    char *kw, char **cmd, int *cmd_len, short *slot, void **ctx);     

int redis_key_key_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock,
    char *kw, char **cmd, int *cmd_len, short *slot, void **ctx);

int redis_key_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock,
    char *kw, char **cmd, int *cmd_len, short *slot, void **ctx);

int redis_key_long_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock,
    char *kw, char **cmd, int *cmd_len, short *slot, void **ctx);

int redis_key_long_long_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock,
    char *kw, char **cmd, int *cmd_len, short *slot, void **ctx);

int redis_key_str_str_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock,
    char *kw, char **cmd, int *cmd_len, short *slot, void **ctx);

int redis_key_dbl_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock,
    char *kw, char **cmd, int *cmd_len, short *slot, void **ctx);

int redis_zrange_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock,
    char *kw, char **cmd, int *cmd_len, zend_bool *withscores, short *slot, 
    void **ctx);

/* Commands which need a unique construction mechanism.  This is either because
 * they don't share a signature with any other command, or because there is 
 * specific processing we do (e.g. verifying subarguments) that make them
 * unique */

int redis_set_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock,
    char **cmd, int *cmd_len, short *slot, void **ctx);

int redis_brpoplpush_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock,
    char **cmd, int *cmd_len, short *slot, void **ctx);

int redis_hincrby_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock,
    char **cmd, int *cmd_len, short *slot, void **ctx);

int redis_hincrbyfloat_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock,
    char **cmd, int *cmd_len, short *slot, void **ctx);

int redis_hmget_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock,
    char **cmd, int *cmd_len, short *slot, void **ctx);

int redis_hmset_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock,
    char **cmd, int *cmd_len, short *slot, void **ctx);

int redis_bitop_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock, 
    char **cmd, int *cmd_len, short *slot, void **ctx);

int redis_bitcount_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock,
    char **cmd, int *cmd_len, short *slot, void **ctx);

int redis_bitpos_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock,
    char **cmd, int *cmd_len, short *slot, void **ctx);

int redis_pfadd_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock,
    char **cmd, int *cmd_len, short *slot, void **ctx);

int redis_pfmerge_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock,
    char **cmd, int *cmd_len, short *slot, void **ctx);

int redis_auth_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock,
    char **cmd, int *cmd_len, short *slot, void **ctx);

int redis_setbit_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock,
    char **cmd, int *cmd_len, short *slot, void **ctx);

int redis_linsert_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock,
    char **cmd, int *cmd_len, short *slot, void **ctx);

int redis_lrem_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock,
    char **cmd, int *cmd_len, short *slot, void **ctx);

int redis_smove_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock,
    char **cmd, int *cmd_len, short *slot, void **ctx);

#endif

/* vim: set tabstop=4 softtabstops=4 noexpandtab shiftwidth=4: */
