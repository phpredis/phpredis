#ifndef REDIS_ARRAY_IMPL_H
#define REDIS_ARRAY_IMPL_H

#if (defined(_MSC_VER) && _MSC_VER <= 1920)
#include <win32/php_stdint.h>
#else
#include <stdint.h>
#endif

#include "redis_array.h"

RedisArray *ra_load_array(const char *name);

RedisArray *ra_make_array(HashTable *hosts, zval *z_fun, zval *z_dist,
                          HashTable *hosts_prev, zend_bool b_index,
                          zend_bool b_pconnect, long retry_interval,
                          zend_bool b_lazy_connect, double connect_timeout,
                          double read_timeout, zend_bool consistent,
                          zend_string *algorithm, zend_string *auth,
                          zend_string *pass);

zval *ra_find_node_by_name(RedisArray *ra, zend_string *host);
zval *ra_find_node(RedisArray *ra, const char *key, int key_len, int *out_pos);
void ra_init_function_table(RedisArray *ra);

void ra_move_key(const char *key, int key_len, zval *z_from, zval *z_to);
void ra_index_multi(zval *z_redis, long multi_value);

void ra_index_key(const char *key, int key_len, zval *z_redis);
void ra_index_keys(zval *z_pairs, zval *z_redis);
void ra_index_del(zval *z_keys, zval *z_redis);
void ra_index_exec(zval *z_redis, zval *return_value, int keep_all);
void ra_index_discard(zval *z_redis, zval *return_value);
void ra_index_unwatch(zval *z_redis, zval *return_value);
zend_bool ra_is_write_cmd(RedisArray *ra, const char *cmd, int cmd_len);

void ra_rehash(RedisArray *ra, zend_fcall_info *z_cb, zend_fcall_info_cache *z_cb_cache);

#endif
