#ifndef REDIS_COMMANDS_H
#define REDIS_COMMANDS_H

#include "common.h"
#include "library.h"
#include "cluster_library.h"

/* Pick a random slot, any slot (for stuff like publish/subscribe) */
#define CMD_RAND_SLOT(slot) \
    if(slot) *slot = rand() % REDIS_CLUSTER_MOD

/* Macro for setting the slot if we've been asked to */
#define CMD_SET_SLOT(slot,key,key_len) \
    if (slot) *slot = cluster_hash_key(key,key_len);

/* Simple container so we can push subscribe context out */
typedef struct {
    zend_fcall_info fci;
    zend_fcall_info_cache fci_cache;
} subscribeCallback;

typedef struct subscribeContext {
    char *kw;
    int argc;
    subscribeCallback cb;
} subscribeContext;

/* Construct a raw command */
int redis_build_raw_cmd(zval *z_args, int argc, char **cmd, int *cmd_len);

/* Construct a script command */
smart_string *redis_build_script_cmd(smart_string *cmd, int argc, zval *z_args);

/* Redis command generics.  Many commands share common prototypes meaning that
 * we can write one function to handle all of them.  For example, there are
 * many COMMAND key value commands, or COMMAND key commands. */

int redis_replicaof_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock,
                        char *kw, char **cmd, int *cmd_len, short *slot,
                        void **ctx);

int redis_empty_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock,
    char *kw, char **cmd, int *cmd_len, short *slot, void **ctx);

int redis_opt_str_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock, char *kw,
    char **cmd, int *cmd_len, short *slot, void **ctx);

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

int redis_long_long_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock,
    char *kw, char **cmd, int *cmd_len, short *slot, void **ctx);

int redis_key_long_long_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock,
    char *kw, char **cmd, int *cmd_len, short *slot, void **ctx);

int redis_key_str_str_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock,
    char *kw, char **cmd, int *cmd_len, short *slot, void **ctx);

int redis_key_dbl_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock,
    char *kw, char **cmd, int *cmd_len, short *slot, void **ctx);

int redis_key_varval_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock,
    char *kw, char **cmd, int *cmd_len, short *slot, void **ctx);

int redis_key_val_arr_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock,
    char *kw, char **cmd, int *cmd_len, short *slot, void **ctx);

int redis_key_str_arr_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock,
    char *kw, char **cmd, int *cmd_len, short *slot, void **ctx);

int redis_pop_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock,
    char *kw, char **cmd, int *cmd_len, short *slot, void **ctx);

int redis_blocking_pop_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock,
    char *kw, char **cmd, int *cmd_len, short *slot, void **ctx);

/* Construct SCAN and similar commands, as well as check iterator  */
int redis_scan_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock,
    REDIS_SCAN_TYPE type, char **cmd, int *cmd_len);

/* ZRANGE, ZREVRANGE, ZRANGEBYSCORE, and ZREVRANGEBYSCORE callback type */
typedef int (*zrange_cb)(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock,
                         char *,char**,int*,int*,short*,void**);

int redis_zrange_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock,
    char *kw, char **cmd, int *cmd_len, short *slot, void **ctx);

int redis_config_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock,
    char **cmd, int *cmd_len, short *slot, void **ctx);

int redis_zrandmember_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock,
    char **cmd, int *cmd_len, short *slot, void **ctx);

int redis_zdiff_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock,
    char **cmd, int *cmd_len, short *slot, void **ctx);

int redis_zinterunion_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock,
    char *kw, char **cmd, int *cmd_len, short *slot, void **ctx);

int redis_zdiffstore_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock,
    char **cmd, int *cmd_len, short *slot, void **ctx);

int redis_zinterunionstore_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock,
    char *kw, char **cmd, int *cmd_len, short *slot, void **ctx);

int redis_intercard_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock,
                        char *kw, char **cmd, int *cmd_len, short *slot,
                        void **ctx);

int redis_slowlog_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock,
                      char **cmd, int *cmd_len, short *slot, void **ctx);

int redis_lcs_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock,
                  char **cmd, int *cmd_len, short *slot, void **ctx);

int redis_mpop_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock, char *kw,
                   char **cmd, int *cmd_len, short *slot, void **ctx);

int redis_restore_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock,
                      char **cmd, int *cmd_len, short *slot, void **ctx);

int redis_pubsub_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock,
    char **cmd, int *cmd_len, short *slot, void **ctx);

int redis_subscribe_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock,
    char *kw, char **cmd, int *cmd_len, short *slot, void **ctx);

int redis_unsubscribe_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock,
    char *kw, char **cmd, int *cmd_len, short *slot, void **ctx);

int redis_zrangebylex_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock,
    char *kw, char **cmd, int *cmd_len, short *slot, void **ctx);

int redis_gen_zlex_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock,
    char *kw, char **cmd, int *cmd_len, short *slot, void **ctx);

int redis_eval_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock,
    char *kw, char **cmd, int *cmd_len, short *slot, void **ctx);

int redis_failover_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock,
    char **cmd, int *cmd_len, short *slot, void **ctx);

int redis_flush_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock,
    char *kw, char **cmd, int *cmd_len, short *slot, void **ctx);

int redis_xrange_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock,
    char *kw, char **cmd, int *cmd_len, short *slot, void **ctx);

int redis_georadius_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock,
    char *kw, char **cmd, int *cmd_len, short *slot, void **ctx);

int redis_georadiusbymember_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock,
    char *kw, char **cmd, int *cmd_len, short *slot, void **ctx);

int redis_geosearch_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock,
    char **cmd, int *cmd_len, short *slot, void **ctx);

int redis_geosearchstore_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock,
    char **cmd, int *cmd_len, short *slot, void **ctx);

/* Commands which need a unique construction mechanism.  This is either because
 * they don't share a signature with any other command, or because there is
 * specific processing we do (e.g. verifying subarguments) that make them
 * unique */

int redis_acl_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock,
    char **cmd, int *cmd_len, short *slot, void **ctx);

int redis_set_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock,
    char **cmd, int *cmd_len, short *slot, void **ctx);

int redis_getex_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock,
    char **cmd, int *cmd_len, short *slot, void **ctx);

int redis_brpoplpush_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock,
    char **cmd, int *cmd_len, short *slot, void **ctx);

int redis_incr_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock,
    char **cmd, int *cmd_len, short *slot, void **ctx);

int redis_decr_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock,
    char **cmd, int *cmd_len, short *slot, void **ctx);

int redis_hincrby_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock,
    char **cmd, int *cmd_len, short *slot, void **ctx);

int redis_hincrbyfloat_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock,
    char **cmd, int *cmd_len, short *slot, void **ctx);

int redis_hmget_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock,
    char **cmd, int *cmd_len, short *slot, void **ctx);

int redis_hmset_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock,
    char **cmd, int *cmd_len, short *slot, void **ctx);

int redis_hstrlen_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock,
    char **cmd, int *cmd_len, short *slot, void **ctx);

int redis_bitop_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock,
    char **cmd, int *cmd_len, short *slot, void **ctx);

int redis_bitcount_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock,
    char **cmd, int *cmd_len, short *slot, void **ctx);

int redis_bitpos_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock,
    char **cmd, int *cmd_len, short *slot, void **ctx);

int redis_pfcount_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock,
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

int redis_lpos_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock,
    char **cmd, int *cmd_len, short *slot, void **ctx);

int redis_smove_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock,
    char **cmd, int *cmd_len, short *slot, void **ctx);

int redis_hrandfield_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock,
    char **cmd, int *cmd_len, short *slot, void **ctx);

int redis_hset_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock,
    char **cmd, int *cmd_len, short *slot, void **ctx);

int redis_hsetnx_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock,
    char **cmd, int *cmd_len, short *slot, void **ctx);

int redis_srandmember_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock,
    char **cmd, int *cmd_len, short *slot, void **ctx, short *have_count);

int redis_select_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock,
                     char **cmd, int *cmd_len, short *slot, void **ctx);

int redis_zincrby_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock,
    char **cmd, int *cmd_len, short *slot, void **ctx);

int redis_hdel_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock,
    char **cmd, int *cmd_len, short *slot, void **ctx);

int redis_zadd_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock,
    char **cmd, int *cmd_len, short *slot, void **ctx);

int redis_object_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock,
    REDIS_REPLY_TYPE *rtype, char **cmd, int *cmd_len, short *slot,
    void **ctx);

int redis_client_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock,
    char **cmd, int *cmd_len, short *slot, void **ctx);

int redis_command_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock,
    char **cmd, int *cmd_len, short *slot, void **ctx);

int redis_copy_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock,
    char **cmd, int *cmd_len, short *slot, void **ctx);

int redis_fmt_scan_cmd(char **cmd, REDIS_SCAN_TYPE type, char *key, int key_len,
    long it, char *pat, int pat_len, long count);

int redis_geoadd_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock,
    char **cmd, int *cmd_len, short *slot, void **ctx);

int redis_geodist_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock,
    char **cmd, int *cmd_len, short *slot, void **ctx);

int redis_migrate_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock,
    char **cmd, int *cmd_len, short *slot, void **ctx);

int redis_xadd_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock,
    char **cmd, int *cmd_len, short *slot, void **ctx);

int redis_xautoclaim_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock,
    char **cmd, int *cmd_len, short *slot, void **ctx);

int redis_xclaim_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock,
    char **cmd, int *cmd_len, short *slot, void **ctx);

int redis_xpending_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock,
    char **cmd, int *cmd_len, short *slot, void **ctx);

int redis_xack_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock,
    char **cmd, int *cmd_len, short *slot, void **ctx);

int redis_xgroup_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock,
    char **cmd, int *cmd_len, short *slot, void **ctx);

int redis_xinfo_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock,
    char **cmd, int *cmd_len, short *slot, void **ctx);

int redis_xread_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock,
    char **cmd, int *cmd_len, short *slot, void **ctx);

int redis_xreadgroup_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock,
    char **cmd, int *cmd_len, short *slot, void **ctx);

int redis_xtrim_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock,
    char **cmd, int *cmd_len, short *slot, void **ctx);

int redis_lmove_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock,
    char *kw, char **cmd, int *cmd_len, short *slot, void **ctx);

int redis_expire_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock,
    char *kw, char **cmd, int *cmd_len, short *slot, void **ctx);

int redis_varkey_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock,
    char *kw, char **cmd, int *cmd_len, short *slot, void **ctx);

int redis_vararg_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock,
    char *kw, char **cmd, int *cmd_len, short *slot, void **ctx);

int redis_sentinel_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock,
    char *kw, char **cmd, int *cmd_len, short *slot, void **ctx);

int redis_sentinel_str_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock,
    char *kw, char **cmd, int *cmd_len, short *slot, void **ctx);

int redis_sort_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock,
    char *kw, char **cmd, int *cmd_len, short *slot, void **ctx);

/* Commands that don't communicate with Redis at all (such as getOption,
 * setOption, _prefix, _serialize, etc).  These can be handled in one place
 * with the method of grabbing our RedisSock* object in different ways
 * depending if this is a Redis object or a RedisCluster object. */

void redis_getoption_handler(INTERNAL_FUNCTION_PARAMETERS,
    RedisSock *redis_sock, redisCluster *c);
void redis_setoption_handler(INTERNAL_FUNCTION_PARAMETERS,
    RedisSock *redis_sock, redisCluster *c);
void redis_prefix_handler(INTERNAL_FUNCTION_PARAMETERS,
    RedisSock *redis_sock);
void redis_serialize_handler(INTERNAL_FUNCTION_PARAMETERS,
    RedisSock *redis_sock);
void redis_unserialize_handler(INTERNAL_FUNCTION_PARAMETERS,
    RedisSock *redis_sock, zend_class_entry *ex);
void redis_compress_handler(INTERNAL_FUNCTION_PARAMETERS,
    RedisSock *redis_sock);
void redis_uncompress_handler(INTERNAL_FUNCTION_PARAMETERS,
    RedisSock *redis_sock, zend_class_entry *ex);

void redis_pack_handler(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock);
void redis_unpack_handler(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock);

#endif

/* vim: set tabstop=4 softtabstop=4 expandtab shiftwidth=4: */
