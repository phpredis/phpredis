#include "redis_array_impl.h"
#include "php_redis.h"
#include "library.h"

#define PHPREDIS_INDEX_NAME	"__phpredis_array_index__"

extern int le_redis_sock;
extern zend_class_entry *redis_ce;

RedisArray*
ra_load_hosts(RedisArray *ra, HashTable *hosts)
{
	int i, host_len, id;
	int count = zend_hash_num_elements(hosts);
	char *host, *p;
	short port;
	zval **zpData, z_cons, *z_args, z_ret;
	RedisSock *redis_sock  = NULL;

	/* function calls on the Redis object */
	ZVAL_STRING(&z_cons, "__construct", 0);

	/* init connections */
	for(i = 0; i < count; ++i) {
		if(FAILURE == zend_hash_quick_find(hosts, NULL, 0, i, (void**)&zpData)) {
			efree(ra);
			return NULL;
		}

		ra->hosts[i] = estrdup(Z_STRVAL_PP(zpData));

		/* default values */
		host = Z_STRVAL_PP(zpData);
		host_len = Z_STRLEN_PP(zpData);
		port = 6379;

		if((p = strchr(host, ':'))) { /* found port */
			host_len = p - host;
			port = (short)atoi(p+1);
		}

		/* create Redis object */
		MAKE_STD_ZVAL(ra->redis[i]);
		object_init_ex(ra->redis[i], redis_ce);
		INIT_PZVAL(ra->redis[i]);
		call_user_function(&redis_ce->function_table, &ra->redis[i], &z_cons, &z_ret, 0, NULL TSRMLS_CC);

		/* create socket */
		redis_sock = redis_sock_create(host, host_len, port, 0, 0, NULL); /* TODO: persistence? */

		/* connect */
		redis_sock_server_open(redis_sock, 1 TSRMLS_CC);

		/* attach */
		id = zend_list_insert(redis_sock, le_redis_sock);
		add_property_resource(ra->redis[i], "socket", id);
	}

	return ra;
}

/* List pure functions */
void ra_init_function_table(RedisArray *ra) {

	MAKE_STD_ZVAL(ra->z_pure_cmds);
	array_init(ra->z_pure_cmds);

	add_assoc_bool(ra->z_pure_cmds, "HGET", 1);
	add_assoc_bool(ra->z_pure_cmds, "HGETALL", 1);
	add_assoc_bool(ra->z_pure_cmds, "HKEYS", 1);
	add_assoc_bool(ra->z_pure_cmds, "HLEN", 1);
	add_assoc_bool(ra->z_pure_cmds, "SRANDMEMBER", 1);
	add_assoc_bool(ra->z_pure_cmds, "HMGET", 1);
	add_assoc_bool(ra->z_pure_cmds, "STRLEN", 1);
	add_assoc_bool(ra->z_pure_cmds, "SUNION", 1);
	add_assoc_bool(ra->z_pure_cmds, "HVALS", 1);
	add_assoc_bool(ra->z_pure_cmds, "TYPE", 1);
	add_assoc_bool(ra->z_pure_cmds, "LINDEX", 1);
	add_assoc_bool(ra->z_pure_cmds, "SCARD", 1);
	add_assoc_bool(ra->z_pure_cmds, "LLEN", 1);
	add_assoc_bool(ra->z_pure_cmds, "SDIFF", 1);
	add_assoc_bool(ra->z_pure_cmds, "ZCARD", 1);
	add_assoc_bool(ra->z_pure_cmds, "ZCOUNT", 1);
	add_assoc_bool(ra->z_pure_cmds, "LRANGE", 1);
	add_assoc_bool(ra->z_pure_cmds, "ZRANGE", 1);
	add_assoc_bool(ra->z_pure_cmds, "ZRANK", 1);
	add_assoc_bool(ra->z_pure_cmds, "GET", 1);
	add_assoc_bool(ra->z_pure_cmds, "GETBIT", 1);
	add_assoc_bool(ra->z_pure_cmds, "SINTER", 1);
	add_assoc_bool(ra->z_pure_cmds, "GETRANGE", 1);
	add_assoc_bool(ra->z_pure_cmds, "ZREVRANGE", 1);
	add_assoc_bool(ra->z_pure_cmds, "SISMEMBER", 1);
	add_assoc_bool(ra->z_pure_cmds, "ZREVRANGEBYSCORE", 1);
	add_assoc_bool(ra->z_pure_cmds, "ZREVRANK", 1);
	add_assoc_bool(ra->z_pure_cmds, "HEXISTS", 1);
	add_assoc_bool(ra->z_pure_cmds, "ZSCORE", 1);
	add_assoc_bool(ra->z_pure_cmds, "HGET", 1);
	add_assoc_bool(ra->z_pure_cmds, "OBJECT", 1);
	add_assoc_bool(ra->z_pure_cmds, "SMEMBERS", 1);
}

RedisArray *
ra_make_array(HashTable *hosts, zval *z_fun, HashTable *hosts_prev, zend_bool b_index) {

	int count = zend_hash_num_elements(hosts);

	/* create object */
	RedisArray *ra = emalloc(sizeof(RedisArray));
	ra->hosts = emalloc(count * sizeof(char*));
	ra->redis = emalloc(count * sizeof(zval*));
	ra->count = count;
	ra->z_fun = NULL;
	ra->index = b_index;

	/* init array data structures */
	ra_init_function_table(ra);

	if(NULL == ra_load_hosts(ra, hosts)) {
		return NULL;
	}
	ra->prev = hosts_prev ? ra_make_array(hosts_prev, z_fun, NULL, b_index) : NULL;

	/* copy function if provided */
	if(z_fun) {
		MAKE_STD_ZVAL(ra->z_fun);
		*ra->z_fun = *z_fun;
		zval_copy_ctor(ra->z_fun);
	}

	return ra;
}


/* call userland key extraction function */
char *
ra_call_extractor(RedisArray *ra, const char *key, int key_len, int *out_len) {

	char *error = NULL, *out;
	zval z_ret;
	zval *z_argv0;

	/* check that we can call the extractor function */
	if(!zend_is_callable_ex(ra->z_fun, NULL, 0, NULL, NULL, NULL, &error TSRMLS_CC)) {
		php_error_docref(NULL TSRMLS_CC, E_ERROR, "Could not call extractor function");
		return NULL;
	}
	convert_to_string(ra->z_fun);

	/* call extraction function */
	MAKE_STD_ZVAL(z_argv0);
	ZVAL_STRINGL(z_argv0, key, key_len, 0);
	call_user_function(EG(function_table), NULL, ra->z_fun, &z_ret, 1, &z_argv0 TSRMLS_CC);
	efree(z_argv0);

	if(Z_TYPE(z_ret) != IS_STRING) {
		zval_dtor(&z_ret);
		return NULL;
	}

	*out_len = Z_STRLEN(z_ret);
	out = estrndup(Z_STRVAL(z_ret), Z_STRLEN(z_ret));

	zval_dtor(&z_ret);
	return out;
}

static char *
ra_extract_key(RedisArray *ra, const char *key, int key_len, int *out_len) {

	char *start, *end;
	*out_len = key_len;

	if(ra->z_fun)
		return ra_call_extractor(ra, key, key_len, out_len);

	/* look for '{' */
	start = strchr(key, '{');
	if(!start) return estrndup(key, key_len);

	/* look for '}' */
	end = strchr(start + 1, '}');
	if(!end) return estrndup(key, key_len);

	/* found substring */
	*out_len = end - start - 1;
	return estrndup(start + 1, *out_len);
}

zval *
ra_find_node(RedisArray *ra, const char *key, int key_len, int *out_pos) {

	uint32_t hash;
	char *out;
	int pos, out_len;

	/* extract relevant part of the key */
	out = ra_extract_key(ra, key, key_len, &out_len);
	if(!out)
		return NULL;

	/* hash */
	hash = crc32(out, out_len);
	efree(out);

	/* get position on ring */
	pos = (int)((((uint64_t)hash) * ra->count) / 0xffffffff);
	if(out_pos) *out_pos = pos;

	return ra->redis[pos];
}


char *
ra_find_key(RedisArray *ra, zval *z_args, const char *cmd, int *key_len) {

	zval **zp_tmp;
	int key_pos = 0; /* TODO: change this depending on the command */

	if(	zend_hash_num_elements(Z_ARRVAL_P(z_args)) == 0
		|| zend_hash_quick_find(Z_ARRVAL_P(z_args), NULL, 0, key_pos, (void**)&zp_tmp) == FAILURE
		|| Z_TYPE_PP(zp_tmp) != IS_STRING) {

		return NULL;
	}

	*key_len = Z_STRLEN_PP(zp_tmp);
	return Z_STRVAL_PP(zp_tmp);
}

void
ra_index_multi(RedisArray *ra, zval *z_redis) {

	zval z_fun_multi, z_ret;

	/* run MULTI */
	ZVAL_STRING(&z_fun_multi, "MULTI", 0);
	call_user_function(&redis_ce->function_table, &z_redis, &z_fun_multi, &z_ret, 0, NULL TSRMLS_CC);
	zval_dtor(&z_ret);
}

void
ra_index_key(RedisArray *ra, zval *z_redis, const char *key, int key_len TSRMLS_DC) {

	int i;
	zval z_fun_sadd, z_ret, *z_args[2];
	MAKE_STD_ZVAL(z_args[0]);
	MAKE_STD_ZVAL(z_args[1]);

	/* prepare args */
	ZVAL_STRINGL(&z_fun_sadd, "SADD", 4, 0);

	ZVAL_STRING(z_args[0], PHPREDIS_INDEX_NAME, 0);
	ZVAL_STRINGL(z_args[1], key, key_len, 1);


	/* run SADD */
	call_user_function(&redis_ce->function_table, &z_redis, &z_fun_sadd, &z_ret, 2, z_args TSRMLS_CC);

	/* don't dtor z_ret, since we're returning z_redis */

	efree(z_args[0]);
	efree(z_args[1]);
}

void
ra_index_exec(RedisArray *ra, zval *z_redis, zval *return_value) {

	zval z_fun_exec, z_ret, **zp_tmp;

	/* run EXEC */
	ZVAL_STRING(&z_fun_exec, "EXEC", 0);
	call_user_function(&redis_ce->function_table, &z_redis, &z_fun_exec, &z_ret, 0, NULL TSRMLS_CC);

	/* extract first element of exec array and put into return_value. */
	if(Z_TYPE(z_ret) == IS_ARRAY) {
		if(zend_hash_quick_find(Z_ARRVAL(z_ret), NULL, 0, 0, (void**)&zp_tmp) != FAILURE) {
			*return_value = **zp_tmp;
			zval_copy_ctor(return_value);
		}
		zval_dtor(&z_ret);
	}
}

zend_bool
ra_is_write_cmd(RedisArray *ra, const char *cmd, int cmd_len) {

	zend_bool ret;
	int i;
	char *cmd_up = emalloc(1 + cmd_len);
	/* convert to uppercase */
	for(i = 0; i < cmd_len; ++i)
		cmd_up[i] = toupper(cmd[i]);
	cmd_up[cmd_len] = 0;

	ret = zend_hash_exists(Z_ARRVAL_P(ra->z_pure_cmds), cmd_up, cmd_len+1);

	efree(cmd_up);
	return !ret;
}

/* list keys from array index */
static long
ra_rehash_scan_index(zval *z_redis, char ***keys, int **key_lens) {

	long count, i;
	zval z_fun_smembers, z_ret, *z_arg, **z_data_pp;
	HashTable *h_keys;
	HashPosition pointer;
	char *key;
	int key_len;

	/* arg */
	MAKE_STD_ZVAL(z_arg);
	ZVAL_STRING(z_arg, PHPREDIS_INDEX_NAME, 0);

	/* run SMEMBERS */
	ZVAL_STRING(&z_fun_smembers, "SMEMBERS", 0);
	call_user_function(&redis_ce->function_table, &z_redis, &z_fun_smembers, &z_ret, 1, &z_arg TSRMLS_CC);
	efree(z_arg);
	if(Z_TYPE(z_ret) != IS_ARRAY) { /* failure */
		return -1;	/* TODO: log error. */
	}
	h_keys = Z_ARRVAL(z_ret);

	/* allocate key array */
	count = zend_hash_num_elements(h_keys);
	*keys = emalloc(count * sizeof(char*));
	*key_lens = emalloc(count * sizeof(int));

	for (i = 0, zend_hash_internal_pointer_reset_ex(h_keys, &pointer);
			zend_hash_get_current_data_ex(h_keys, (void**) &z_data_pp, &pointer) == SUCCESS;
			zend_hash_move_forward_ex(h_keys, &pointer), ++i) {

		key = Z_STRVAL_PP(z_data_pp);
		key_len = Z_STRLEN_PP(z_data_pp);

		/* copy key and length */
		(*keys)[i] = emalloc(1 + key_len);
		memcpy((*keys)[i], key, key_len);
		(*key_lens)[i] = key_len;
		(*keys)[i][key_len] = 0; /* null-terminate string */
	}

	/* cleanup */
	zval_dtor(&z_ret);

	return count;
}

/* list keys using KEYS command */
static long
ra_rehash_scan_keys(zval *z_redis, char ***keys, int **key_lens) {

	/* TODO */
	return 0;
}

/* run TYPE to find the type */
static long
ra_get_key_type(zval *z_redis, const char *key, int key_len) {

	int i;
	zval z_fun_type, z_ret, *z_arg;
	MAKE_STD_ZVAL(z_arg);

	/* prepare args */
	ZVAL_STRINGL(&z_fun_type, "TYPE", 4, 0);
	ZVAL_STRINGL(z_arg, key, key_len, 0);

	/* run TYPE */
	call_user_function(&redis_ce->function_table, &z_redis, &z_fun_type, &z_ret, 1, &z_arg TSRMLS_CC);

	/* cleanup */
	efree(z_arg);

	return Z_LVAL(z_ret);
}

static zend_bool
ra_del_key(const char *key, int key_len, zval *z_from) {

	zval z_fun_del, z_ret, *z_args[2];

	/* run GET on source */
	MAKE_STD_ZVAL(z_args[0]);
	ZVAL_STRINGL(&z_fun_del, "DEL", 3, 0);
	ZVAL_STRINGL(z_args[0], key, key_len, 0);
	call_user_function(&redis_ce->function_table, &z_from, &z_fun_del, &z_ret, 1, z_args TSRMLS_CC);
}

static zend_bool
ra_move_string(const char *key, int key_len, zval *z_from, zval *z_to) {

	zval z_fun_get, z_fun_set, z_ret, *z_args[2];

	/* run GET on source */
	MAKE_STD_ZVAL(z_args[0]);
	ZVAL_STRINGL(&z_fun_get, "GET", 3, 0);
	ZVAL_STRINGL(z_args[0], key, key_len, 0);
	call_user_function(&redis_ce->function_table, &z_from, &z_fun_get, &z_ret, 1, z_args TSRMLS_CC);

	if(Z_TYPE(z_ret) != IS_STRING) { /* key not found or replaced */
		/* TODO: report? */
		efree(z_args[0]);
		return 0;
	}

	/* run SET on target */
	MAKE_STD_ZVAL(z_args[1]);
	ZVAL_STRINGL(&z_fun_set, "SET", 3, 0);
	ZVAL_STRINGL(z_args[0], key, key_len, 0);
	ZVAL_STRINGL(z_args[1], Z_STRVAL(z_ret), Z_STRLEN(z_ret), 1); // copy z_ret to arg 1
	call_user_function(&redis_ce->function_table, &z_to, &z_fun_set, &z_ret, 2, z_args TSRMLS_CC);

	/* cleanup */
	efree(z_args[0]);
	zval_dtor(z_args[1]);
	efree(z_args[1]);

	return 1;
}

static void
ra_move_key(const char *key, int key_len, zval *z_from, zval *z_to) {

	long type = ra_get_key_type(z_from, key, key_len);
	zend_bool success = 0;

	switch(type) {
		case REDIS_STRING:
			success = ra_move_string(key, key_len, z_from, z_to);
			break;

		case REDIS_SET:
			success = ra_move_set(key, key_len, z_from, z_to);
			break;

		case REDIS_LIST:
			success = ra_move_list(key, key_len, z_from, z_to);
			break;

		case REDIS_ZSET:
			success = ra_move_zset(key, key_len, z_from, z_to);
			break;

		case REDIS_HASH:
			success = ra_move_hash(key, key_len, z_from, z_to);
			break;

		default:
			/* TODO: report? */
			break;
	}

	if(success) {
		ra_del_key(key, key_len, z_from);
	}
}

static void
ra_rehash_server(RedisArray *ra, zval *z_redis, const char *hostname, zend_bool b_index) {

	char **keys;
	int *key_lens;
	long count, i;
	int target_pos;
	zval *z_target;

	/* list all keys */
	if(b_index) {
		count = ra_rehash_scan_index(z_redis, &keys, &key_lens);
	} else {
		count = ra_rehash_scan_keys(z_redis, &keys, &key_lens);
	}

	php_printf("%s has %ld keys to redistribute.\n", hostname, count);
	/* for each key, redistribute */

	for(i = 0; i < count; ++i) {

		/* TODO: check that we're not moving to the same node. */
		z_target = ra_find_node(ra, keys[i], key_lens[i], &target_pos);

		if(strcmp(hostname, ra->hosts[target_pos])) { /* different host */

			/* php_printf("move [%s] from [%s] to [%s]\n", keys[i], hostname, ra->hosts[target_pos]); */

			ra_move_key(keys[i], key_lens[i], z_redis, z_target);
		} else {
			// php_printf("key [%s] stays on [%s]\n", keys[i], hostname);
		}

		efree(keys[i]);
	}

	// cleanup
	efree(keys);
	efree(key_lens);
}

void
ra_rehash(RedisArray *ra) {

	int i;

	/* redistribute the data, server by server. */
	if(!ra->prev)
		return;	/* TODO: compare the two rings for equality */

	for(i = 0; i < ra->prev->count; ++i) {
		ra_rehash_server(ra, ra->prev->redis[i], ra->prev->hosts[i], ra->index);
	}
}

