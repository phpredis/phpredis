#ifndef REDIS_ARRAY_IMPL_H
#define REDIS_ARRAY_IMPL_H

#include <stdint.h>
#include "common.h"
#include "redis_array.h"

RedisArray *ra_load_hosts(RedisArray *ra, HashTable *hosts, long retry_interval, zend_bool b_lazy_connect TSRMLS_DC);
RedisArray *ra_load_array(const char *name TSRMLS_DC);
RedisArray *ra_make_array(HashTable *hosts, zval *z_fun, zval *z_dist, HashTable *hosts_prev, zend_bool b_index, zend_bool b_pconnect, long retry_interval, zend_bool b_lazy_connect TSRMLS_DC);
zval *ra_find_node_by_name(RedisArray *ra, const char *host, int host_len TSRMLS_DC);
zval *ra_find_node(RedisArray *ra, const char *key, int key_len, int *out_pos TSRMLS_DC);
void ra_init_function_table(RedisArray *ra);

void ra_move_key(const char *key, int key_len, zval *z_from, zval *z_to TSRMLS_DC);
char * ra_find_key(RedisArray *ra, zval *z_args, const char *cmd, int *key_len);
void ra_index_multi(zval *z_redis, long multi_value TSRMLS_DC);

void ra_index_key(const char *key, int key_len, zval *z_redis TSRMLS_DC);
void ra_index_keys(zval *z_pairs, zval *z_redis TSRMLS_DC);
void ra_index_del(zval *z_keys, zval *z_redis TSRMLS_DC);
void ra_index_exec(zval *z_redis, zval *return_value, int keep_all TSRMLS_DC);
void ra_index_discard(zval *z_redis, zval *return_value TSRMLS_DC);
void ra_index_unwatch(zval *z_redis, zval *return_value TSRMLS_DC);
zend_bool ra_is_write_cmd(RedisArray *ra, const char *cmd, int cmd_len);

void ra_rehash(RedisArray *ra, zend_fcall_info *z_cb, zend_fcall_info_cache *z_cb_cache TSRMLS_DC);

#endif
