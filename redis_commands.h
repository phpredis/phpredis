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

int redis_key_long_val_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock,
    char *kw, char **cmd, int *cmd_len, short *slot);

int redis_kv_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock,
    char *kw, char **cmd, int *cmd_len, short *slot);

int redis_key_str_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock,
    char *kw, char **cmd, int *cmd_len, short *slot);     

int redis_key_key_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock,
    char *kw, char **cmd, int *cmd_len, short *slot);

int redis_key_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock,
    char *kw, char **cmd, int *cmd_len, short *slot);

int redis_key_long_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock,
    char *kw, char **cmd, int *cmd_len, short *slot);

int redis_key_long_long_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock,
    char *kw, char **cmd, int *cmd_len, short *slot);

int redis_key_dbl_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock,
    char *kw, char **cmd, int *cmd_len, short *slot);

int redis_ss_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock,
    char *kw, char **cmd, int *cmd_len, short *slot);

/* Commands which need a unique construction mechanism.  This is either because
 * they don't share a signature with any other command, or because there is 
 * specific processing we do (e.g. verifying subarguments) that make them
 * unique */

int redis_set_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock,
    char **cmd, int *cmd_len, short *slot);

int redis_brpoplpush_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock,
    char **cmd, int *cmd_len, short *slot);

int redis_hincrby_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock,
    char **cmd, int *cmd_len, short *slot);

int redis_hincrbyfloat_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock,
    char **cmd, int *cmd_len, short *slot);

#endif

/* vim: set tabstop=4 softtabstops=4 noexpandtab shiftwidth=4: */
