#ifndef REDIS_COMMANDS_H
#define REDIS_COMMANDS_H

#include "common.h"
#include "library.h"
#include "cluster_library.h"

/* Macro for setting the slot if we've been asked to */
#define CMD_SET_SLOT(slot,key,key_len) \
    if(slot) *slot = cluster_hash_key(key,key_len);

/* Redis command construction routines, which generally take the form:
 *      int function(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock,
 *                   char **cmd, int *cmd_len, short *slot);
 *     
 *      OR
 *
 *      int function(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock,
 *                   char *kw, char **cmd, int *cmd_len, short *slot);
 *
 * The functions will return SUCCESS on success, and FAILURE on failure.  In
 * the case of a failure, the cmd pointer will not have been updated, and
 * no memory wlll have been allocated (that wasn't freed).
 *
 * If the slot pointer is passed as non-null, it will be set to the Redis
 * Cluster hash slot where the key(s) belong. 
 */

int redis_get_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock,
                  char **cmd, int *cmd_len, short *slot);

int redis_set_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock,
                  char **cmd, int *cmd_len, short *slot);

int redis_gen_setex_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock,
                        char *kw, char **cmd, int *cmd_len, short *slot);
#endif

/* vim: set tabstop=4 softtabstops=4 noexpandtab shiftwidth=4: */
