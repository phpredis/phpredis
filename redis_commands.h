#ifndef REDIS_COMMANDS_H
#define REDIS_COMMANDS_H

#include "common.h"
#include "library.h"
#include "cluster_library.h"
#include "redis_cmd.h"

/* Simple container so we can push subscribe context out */
typedef struct {
    zend_fcall_info fci;
    zend_fcall_info_cache fci_cache;
} subscribeCallback;

typedef struct subscribeContext {
    const char *kw;
    int argc;
    subscribeCallback cb;
} subscribeContext;

/* Construct a raw command */
RedisCmd *redis_build_raw_cmd(zval *z_args, int argc);

/* Construct a script command */
RedisCmd *redis_build_script_cmd(int argc, zval *z_args);

typedef RedisCmd * redis_cmd_cb(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock);

typedef RedisCmd * redis_kw_cmd_cb(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock,
                                   const char *kw, size_t kw_len);

/* Process a command but with a specific command building function
 * and keyword which is passed to us*/
void redis_process_kw_cmd(INTERNAL_FUNCTION_PARAMETERS, const char *kw, size_t kw_len,
    redis_kw_cmd_cb cmd_cb, FailableResultCallback resp_cb);

#define REDIS_PROCESS_KW_CMD(kw, cmdfunc, resp_func) \
    redis_process_kw_cmd(INTERNAL_FUNCTION_PARAM_PASSTHRU, kw, sizeof(kw) - 1, \
                         cmdfunc, resp_func)

/* Redis command builders */

RedisCmd *redis_replicaof_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock, const char *kw, size_t kw_len);
RedisCmd *redis_empty_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock, const char *kw, size_t kw_len);
RedisCmd *redis_opt_str_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock, const char *kw, size_t kw_len);
RedisCmd *redis_str_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock, const char *kw, size_t kw_len);
RedisCmd *redis_key_long_val_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock, const char *kw, size_t kw_len);
RedisCmd *redis_key_long_str_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock, const char *kw, size_t kw_len);
RedisCmd *redis_kv_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock, const char *kw, size_t kw_len);
RedisCmd *redis_key_str_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock, const char *kw, size_t kw_len);
RedisCmd *redis_key_key_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock, const char *kw, size_t kw_len);
RedisCmd *redis_key_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock, const char *kw, size_t kw_len);
RedisCmd *redis_key_long_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock, const char *kw, size_t kw_len);
RedisCmd *redis_long_long_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock, const char *kw, size_t kw_len);
RedisCmd *redis_key_long_long_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock, const char *kw, size_t kw_len);
RedisCmd *redis_key_str_str_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock, const char *kw, size_t kw_len);
RedisCmd *redis_key_dbl_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock, const char *kw, size_t kw_len);
RedisCmd *redis_key_varval_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock, const char *kw, size_t kw_len);
RedisCmd *redis_key_val_arr_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock, const char *kw, size_t kw_len);
RedisCmd *redis_key_str_arr_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock, const char *kw, size_t kw_len);
RedisCmd *redis_pop_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock, const char *kw, size_t kw_len);
RedisCmd *redis_blocking_pop_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock, const char *kw, size_t kw_len);
RedisCmd *redis_zrange_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock, const char *kw, size_t kw_len);
RedisCmd *redis_config_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock);
RedisCmd *redis_function_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock);
RedisCmd *redis_zrandmember_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock);
RedisCmd *redis_zdiff_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock);
RedisCmd *redis_zinterunion_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock, const char *kw, size_t kw_len);
RedisCmd *redis_zdiffstore_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock);
RedisCmd *redis_zinterunionstore_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock, const char *kw, size_t kw_len);
RedisCmd *redis_intercard_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock, const char *kw, size_t kw_len);
RedisCmd *redis_slowlog_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock);
RedisCmd *redis_lcs_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock);
RedisCmd *redis_mpop_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock, const char *kw, size_t kw_len);
RedisCmd *redis_restore_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock);
RedisCmd *redis_pubsub_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock);
RedisCmd *redis_subscribe_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock, const char *kw, size_t kw_len);
RedisCmd *redis_unsubscribe_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock, const char *kw, size_t kw_len);
RedisCmd *redis_zrangebylex_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock, const char *kw, size_t kw_len);
RedisCmd *redis_gen_zlex_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock, const char *kw, size_t kw_len);
RedisCmd *redis_eval_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock, const char *kw, size_t kw_len);
RedisCmd *redis_fcall_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock, const char *kw, size_t kw_len);
RedisCmd *redis_failover_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock);
RedisCmd *redis_flush_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock, const char *kw, size_t kw_len);
RedisCmd *redis_xrange_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock, const char *kw, size_t kw_len);
RedisCmd *redis_vrange_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock, const char *kw, size_t kw_len);
RedisCmd *redis_georadius_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock, const char *kw, size_t kw_len);
RedisCmd *redis_georadiusbymember_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock, const char *kw, size_t kw_len);
RedisCmd *redis_geosearch_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock);
RedisCmd *redis_geosearchstore_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock);
RedisCmd *redis_waitaof_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock);
RedisCmd *redis_info_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock);
RedisCmd *redis_script_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock);
RedisCmd *redis_acl_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock);
RedisCmd *redis_set_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock);
RedisCmd *redis_getex_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock);
RedisCmd *redis_brpoplpush_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock);
RedisCmd *redis_incr_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock);
RedisCmd *redis_decr_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock);
RedisCmd *redis_delex_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock);
RedisCmd *redis_hincrby_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock);
RedisCmd *redis_hincrbyfloat_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock);
RedisCmd *redis_hmget_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock);
RedisCmd *redis_hgetex_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock);
RedisCmd *redis_hsetex_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock);
RedisCmd *redis_hgetdel_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock);
RedisCmd *redis_mget_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock);
RedisCmd *redis_hmset_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock);
RedisCmd *redis_hstrlen_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock);
RedisCmd *redis_bitop_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock);
RedisCmd *redis_bitcount_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock);
RedisCmd *redis_bitpos_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock);
RedisCmd *redis_pfcount_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock);
RedisCmd *redis_pfadd_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock);
RedisCmd *redis_pfmerge_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock);
RedisCmd *redis_auth_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock);
RedisCmd *redis_setbit_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock);
RedisCmd *redis_linsert_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock);
RedisCmd *redis_lrem_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock);
RedisCmd *redis_lpos_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock);
RedisCmd *redis_smove_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock);
RedisCmd *redis_sunioncard_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock);
RedisCmd *redis_sdiffcard_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock);
RedisCmd *redis_hrandfield_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock);
RedisCmd *redis_hset_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock);
RedisCmd *redis_hsetnx_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock);
RedisCmd *redis_select_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock);
RedisCmd *redis_zincrby_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock);
RedisCmd *redis_hdel_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock);
RedisCmd *redis_zadd_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock);
RedisCmd *redis_object_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock);
RedisCmd *redis_client_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock);
RedisCmd *redis_command_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock);
RedisCmd *redis_copy_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock);
RedisCmd *redis_fmt_scan_cmd(RedisSock *redis_sock, REDIS_SCAN_TYPE type,
    char *key, int key_len, uint64_t it, char *pat, int pat_len, long count);
RedisCmd *redis_geoadd_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock);
RedisCmd *redis_geodist_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock);
RedisCmd *redis_migrate_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock);
RedisCmd *redis_vadd_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock);
RedisCmd *redis_vsim_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock);
RedisCmd *redis_vemb_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock);
RedisCmd *redis_vgetattr_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock);
RedisCmd *redis_vsetattr_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock);
RedisCmd *redis_gcra_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock);
RedisCmd *redis_vlinks_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock);
RedisCmd *redis_xadd_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock);
RedisCmd *redis_xdelex_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock);
RedisCmd *redis_xackdel_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock);
RedisCmd *redis_xautoclaim_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock);
RedisCmd *redis_xclaim_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock);
RedisCmd *redis_xpending_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock);
RedisCmd *redis_xack_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock);
RedisCmd *redis_xgroup_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock);
RedisCmd *redis_xinfo_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock);
RedisCmd *redis_xread_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock);
RedisCmd *redis_xreadgroup_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock);
RedisCmd *redis_xtrim_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock);
RedisCmd *redis_expiremember_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock);
RedisCmd *redis_expirememberat_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock);
RedisCmd *redis_randmember_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock, const char *kw, size_t kw_len);
RedisCmd *redis_hexpire_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock, const char *kw, size_t kw_len);
RedisCmd *redis_httl_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock, const char *kw, size_t kw_len);
RedisCmd *redis_lmove_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock, const char *kw, size_t kw_len);
RedisCmd *redis_lmovem_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock, const char *kw, size_t kw_len);
RedisCmd *redis_blmovem_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock, const char *kw, size_t kw_len);
RedisCmd *redis_expire_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock, const char *kw, size_t kw_len);
RedisCmd *redis_varkey_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock, const char *kw, size_t kw_len);
RedisCmd *redis_mset_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock, const char *kw, size_t kw_len);
RedisCmd *redis_msetex_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock);
RedisCmd *redis_sentinel_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock, const char *kw, size_t kw_len);
RedisCmd *redis_sentinel_str_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock, const char *kw, size_t kw_len);
RedisCmd *redis_sort_cmd(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock, const char *kw, size_t kw_len);

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
void redis_digest_handler(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock,
    zend_class_entry *exception_ce);

#endif

/* vim: set tabstop=4 softtabstop=4 expandtab shiftwidth=4: */
