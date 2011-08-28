#ifndef REDIS_ARRAY_IMPL_H
#define REDIS_ARRAY_IMPL_H

#include <stdint.h>
#include "common.h"
#include "redis_array.h"

RedisArray* ra_load_hosts(RedisArray *ra, HashTable *hosts);
RedisArray *ra_load_array(const char *name);
RedisArray *ra_make_array(HashTable *hosts, zval *z_fun, HashTable *hosts_prev, zend_bool b_index);
zval *ra_find_node(RedisArray *ra, const char *key, int key_len, int *out_pos);
void ra_init_function_table(RedisArray *ra);

char * ra_find_key(RedisArray *ra, zval *z_args, const char *cmd, int *key_len);
void ra_index_multi(zval *z_redis);

void ra_index_key(const char *key, int key_len, zval *z_redis TSRMLS_DC);
void ra_index_exec(zval *z_redis, zval *return_value);
zend_bool ra_is_write_cmd(RedisArray *ra, const char *cmd, int cmd_len);

void ra_rehash(RedisArray *ra, zval *z_cb);

#endif
