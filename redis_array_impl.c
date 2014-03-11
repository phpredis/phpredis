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
  | Author: Nicolas Favre-Felix <n.favre-felix@owlient.eu>               |
  | Maintainer: Michael Grunder <michael.grunder@gmail.com>              |
  +----------------------------------------------------------------------+
*/
#include "redis_array_impl.h"
#include "php_redis.h"
#include "library.h"

#include "php_variables.h"
#include "SAPI.h"
#include "ext/standard/url.h"

#define PHPREDIS_INDEX_NAME	"__phpredis_array_index__"

extern int le_redis_sock;
extern zend_class_entry *redis_ce;

RedisArray*
ra_load_hosts(RedisArray *ra, HashTable *hosts, long retry_interval, zend_bool b_lazy_connect TSRMLS_DC)
{
	int i, host_len, id;
	int count = zend_hash_num_elements(hosts);
	char *host, *p;
	short port;
	zval **zpData, z_cons, z_ret;
	RedisSock *redis_sock  = NULL;

	/* function calls on the Redis object */
	ZVAL_STRING(&z_cons, "__construct", 0);

	/* init connections */
	for(i = 0; i < count; ++i) {
		if(FAILURE == zend_hash_quick_find(hosts, NULL, 0, i, (void**)&zpData) ||
           Z_TYPE_PP(zpData) != IS_STRING) 
        {
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
		} else if(strchr(host,'/') != NULL) { /* unix socket */
            port = -1;
        }

		/* create Redis object */
		MAKE_STD_ZVAL(ra->redis[i]);
		object_init_ex(ra->redis[i], redis_ce);
		INIT_PZVAL(ra->redis[i]);
		call_user_function(&redis_ce->function_table, &ra->redis[i], &z_cons, &z_ret, 0, NULL TSRMLS_CC);

		/* create socket */
		redis_sock = redis_sock_create(host, host_len, port, 0, ra->pconnect, NULL, retry_interval, b_lazy_connect);

	    if (!b_lazy_connect)
    	{
			/* connect */
			redis_sock_server_open(redis_sock, 1 TSRMLS_CC);
		}

		/* attach */
#if PHP_VERSION_ID >= 50400
		id = zend_list_insert(redis_sock, le_redis_sock TSRMLS_CC);
#else
		id = zend_list_insert(redis_sock, le_redis_sock);
#endif
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
	add_assoc_bool(ra->z_pure_cmds, "EXISTS", 1);
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

static int
ra_find_name(const char *name) {

	const char *ini_names, *p, *next;
	/* php_printf("Loading redis array with name=[%s]\n", name); */

	ini_names = INI_STR("redis.arrays.names");
	for(p = ini_names; p;) {
		next = strchr(p, ',');
		if(next) {
			if(strncmp(p, name, next - p) == 0) {
				return 1;
			}
		} else {
			if(strcmp(p, name) == 0) {
				return 1;
			}
			break;
		}
		p = next + 1;
	}

	return 0;
}

/* laod array from INI settings */
RedisArray *ra_load_array(const char *name TSRMLS_DC) {

	zval *z_params_hosts, **z_hosts;
	zval *z_params_prev, **z_prev;
	zval *z_params_funs, **z_data_pp, *z_fun = NULL, *z_dist = NULL;
	zval *z_params_index;
	zval *z_params_autorehash;
	zval *z_params_retry_interval;
	zval *z_params_pconnect;
	zval *z_params_lazy_connect;
	RedisArray *ra = NULL;

	zend_bool b_index = 0, b_autorehash = 0, b_pconnect = 0;
	long l_retry_interval = 0;
	zend_bool b_lazy_connect = 0;
	HashTable *hHosts = NULL, *hPrev = NULL;

	/* find entry */
	if(!ra_find_name(name))
		return ra;

	/* find hosts */
	MAKE_STD_ZVAL(z_params_hosts);
	array_init(z_params_hosts);
	sapi_module.treat_data(PARSE_STRING, estrdup(INI_STR("redis.arrays.hosts")), z_params_hosts TSRMLS_CC);
	if (zend_hash_find(Z_ARRVAL_P(z_params_hosts), name, strlen(name) + 1, (void **) &z_hosts) != FAILURE) {
		hHosts = Z_ARRVAL_PP(z_hosts);
	}

	/* find previous hosts */
	MAKE_STD_ZVAL(z_params_prev);
	array_init(z_params_prev);
	sapi_module.treat_data(PARSE_STRING, estrdup(INI_STR("redis.arrays.previous")), z_params_prev TSRMLS_CC);
	if (zend_hash_find(Z_ARRVAL_P(z_params_prev), name, strlen(name) + 1, (void **) &z_prev) != FAILURE) {
		hPrev = Z_ARRVAL_PP(z_prev);
	}

	/* find function */
	MAKE_STD_ZVAL(z_params_funs);
	array_init(z_params_funs);
	sapi_module.treat_data(PARSE_STRING, estrdup(INI_STR("redis.arrays.functions")), z_params_funs TSRMLS_CC);
	if (zend_hash_find(Z_ARRVAL_P(z_params_funs), name, strlen(name) + 1, (void **) &z_data_pp) != FAILURE) {
		MAKE_STD_ZVAL(z_fun);
		*z_fun = **z_data_pp;
		zval_copy_ctor(z_fun);
	}

	/* find distributor */
	MAKE_STD_ZVAL(z_params_funs);
	array_init(z_params_funs);
	sapi_module.treat_data(PARSE_STRING, estrdup(INI_STR("redis.arrays.distributor")), z_params_funs TSRMLS_CC);
	if (zend_hash_find(Z_ARRVAL_P(z_params_funs), name, strlen(name) + 1, (void **) &z_data_pp) != FAILURE) {
		MAKE_STD_ZVAL(z_dist);
		*z_dist = **z_data_pp;
		zval_copy_ctor(z_dist);
	}

	/* find index option */
	MAKE_STD_ZVAL(z_params_index);
	array_init(z_params_index);
	sapi_module.treat_data(PARSE_STRING, estrdup(INI_STR("redis.arrays.index")), z_params_index TSRMLS_CC);
	if (zend_hash_find(Z_ARRVAL_P(z_params_index), name, strlen(name) + 1, (void **) &z_data_pp) != FAILURE) {
		if(Z_TYPE_PP(z_data_pp) == IS_STRING && strncmp(Z_STRVAL_PP(z_data_pp), "1", 1) == 0) {
			b_index = 1;
		}
	}

	/* find autorehash option */
	MAKE_STD_ZVAL(z_params_autorehash);
	array_init(z_params_autorehash);
	sapi_module.treat_data(PARSE_STRING, estrdup(INI_STR("redis.arrays.autorehash")), z_params_autorehash TSRMLS_CC);
	if (zend_hash_find(Z_ARRVAL_P(z_params_autorehash), name, strlen(name) + 1, (void **) &z_data_pp) != FAILURE) {
		if(Z_TYPE_PP(z_data_pp) == IS_STRING && strncmp(Z_STRVAL_PP(z_data_pp), "1", 1) == 0) {
			b_autorehash = 1;
		}
	}

	/* find retry interval option */
	MAKE_STD_ZVAL(z_params_retry_interval);
	array_init(z_params_retry_interval);
	sapi_module.treat_data(PARSE_STRING, estrdup(INI_STR("redis.arrays.retryinterval")), z_params_retry_interval TSRMLS_CC);
	if (zend_hash_find(Z_ARRVAL_P(z_params_retry_interval), name, strlen(name) + 1, (void **) &z_data_pp) != FAILURE) {
		if (Z_TYPE_PP(z_data_pp) == IS_LONG || Z_TYPE_PP(z_data_pp) == IS_STRING) {
			if (Z_TYPE_PP(z_data_pp) == IS_LONG) {
				l_retry_interval = Z_LVAL_PP(z_data_pp);
			}
			else {
				l_retry_interval = atol(Z_STRVAL_PP(z_data_pp));
			}
		}
	}

	/* find pconnect option */
    MAKE_STD_ZVAL(z_params_pconnect);
    array_init(z_params_pconnect);
    sapi_module.treat_data(PARSE_STRING, estrdup(INI_STR("redis.arrays.pconnect")), z_params_pconnect TSRMLS_CC);
    if (zend_hash_find(Z_ARRVAL_P(z_params_pconnect), name, strlen(name) + 1, (void**) &z_data_pp) != FAILURE) {
        if(Z_TYPE_PP(z_data_pp) == IS_STRING && strncmp(Z_STRVAL_PP(z_data_pp), "1", 1) == 0) {
            b_pconnect = 1;
        }
    }
	/* find retry interval option */
	MAKE_STD_ZVAL(z_params_lazy_connect);
	array_init(z_params_lazy_connect);
	sapi_module.treat_data(PARSE_STRING, estrdup(INI_STR("redis.arrays.lazyconnect")), z_params_lazy_connect TSRMLS_CC);
	if (zend_hash_find(Z_ARRVAL_P(z_params_lazy_connect), name, strlen(name) + 1, (void **) &z_data_pp) != FAILURE) {
		if(Z_TYPE_PP(z_data_pp) == IS_STRING && strncmp(Z_STRVAL_PP(z_data_pp), "1", 1) == 0) {
			b_lazy_connect = 1;
		}
	}

	/* create RedisArray object */
	ra = ra_make_array(hHosts, z_fun, z_dist, hPrev, b_index, b_pconnect, l_retry_interval, b_lazy_connect TSRMLS_CC);
	ra->auto_rehash = b_autorehash;
	if(ra->prev) ra->prev->auto_rehash = b_autorehash;

	/* cleanup */
	zval_dtor(z_params_hosts);
	efree(z_params_hosts);
	zval_dtor(z_params_prev);
	efree(z_params_prev);
	zval_dtor(z_params_funs);
	efree(z_params_funs);
	zval_dtor(z_params_index);
	efree(z_params_index);
	zval_dtor(z_params_autorehash);
	efree(z_params_autorehash);
	zval_dtor(z_params_retry_interval);
	efree(z_params_retry_interval);
	zval_dtor(z_params_pconnect);
	efree(z_params_pconnect);
	zval_dtor(z_params_lazy_connect);
	efree(z_params_lazy_connect);

	return ra;
}

RedisArray *
ra_make_array(HashTable *hosts, zval *z_fun, zval *z_dist, HashTable *hosts_prev, zend_bool b_index, zend_bool b_pconnect, long retry_interval, zend_bool b_lazy_connect TSRMLS_DC) {

	int count = zend_hash_num_elements(hosts);

	/* create object */
	RedisArray *ra = emalloc(sizeof(RedisArray));
	ra->hosts = emalloc(count * sizeof(char*));
	ra->redis = emalloc(count * sizeof(zval*));
	ra->count = count;
	ra->z_fun = NULL;
	ra->z_dist = NULL;
	ra->z_multi_exec = NULL;
	ra->index = b_index;
	ra->auto_rehash = 0;
	ra->pconnect = b_pconnect;

	/* init array data structures */
	ra_init_function_table(ra);

	if(NULL == ra_load_hosts(ra, hosts, retry_interval, b_lazy_connect TSRMLS_CC)) {
		return NULL;
	}
	ra->prev = hosts_prev ? ra_make_array(hosts_prev, z_fun, z_dist, NULL, b_index, b_pconnect, retry_interval, b_lazy_connect TSRMLS_CC) : NULL;

	/* copy function if provided */
	if(z_fun) {
		MAKE_STD_ZVAL(ra->z_fun);
		*ra->z_fun = *z_fun;
		zval_copy_ctor(ra->z_fun);
	}

	/* copy distributor if provided */
	if(z_dist) {
		MAKE_STD_ZVAL(ra->z_dist);
		*ra->z_dist = *z_dist;
		zval_copy_ctor(ra->z_dist);
	}

	return ra;
}


/* call userland key extraction function */
char *
ra_call_extractor(RedisArray *ra, const char *key, int key_len, int *out_len TSRMLS_DC) {

	char *out;
	zval z_ret;
	zval *z_argv0;

	/* check that we can call the extractor function */
	if(!zend_is_callable_ex(ra->z_fun, NULL, 0, NULL, NULL, NULL, NULL TSRMLS_CC)) {
		php_error_docref(NULL TSRMLS_CC, E_ERROR, "Could not call extractor function");
		return NULL;
	}
	//convert_to_string(ra->z_fun);

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
	out = emalloc(*out_len + 1);
	out[*out_len] = 0;
	memcpy(out, Z_STRVAL(z_ret), *out_len);

	zval_dtor(&z_ret);
	return out;
}

static char *
ra_extract_key(RedisArray *ra, const char *key, int key_len, int *out_len TSRMLS_DC) {

	char *start, *end, *out;
	*out_len = key_len;

	if(ra->z_fun)
		return ra_call_extractor(ra, key, key_len, out_len TSRMLS_CC);

	/* look for '{' */
	start = strchr(key, '{');
	if(!start) return estrndup(key, key_len);

	/* look for '}' */
	end = strchr(start + 1, '}');
	if(!end) return estrndup(key, key_len);

	/* found substring */
	*out_len = end - start - 1;
	out = emalloc(*out_len + 1);
	out[*out_len] = 0;
	memcpy(out, start+1, *out_len);

	return out;
}

/* call userland key distributor function */
zend_bool
ra_call_distributor(RedisArray *ra, const char *key, int key_len, int *pos TSRMLS_DC) {

	zval z_ret;
	zval *z_argv0;

	/* check that we can call the extractor function */
	if(!zend_is_callable_ex(ra->z_dist, NULL, 0, NULL, NULL, NULL, NULL TSRMLS_CC)) {
		php_error_docref(NULL TSRMLS_CC, E_ERROR, "Could not call distributor function");
		return 0;
	}
	//convert_to_string(ra->z_fun);

	/* call extraction function */
	MAKE_STD_ZVAL(z_argv0);
	ZVAL_STRINGL(z_argv0, key, key_len, 0);
	call_user_function(EG(function_table), NULL, ra->z_dist, &z_ret, 1, &z_argv0 TSRMLS_CC);
	efree(z_argv0);

	if(Z_TYPE(z_ret) != IS_LONG) {
		zval_dtor(&z_ret);
		return 0;
	}

	*pos = Z_LVAL(z_ret);
	zval_dtor(&z_ret);
	return 1;
}

zval *
ra_find_node(RedisArray *ra, const char *key, int key_len, int *out_pos TSRMLS_DC) {

        uint32_t hash;
        char *out;
        int pos, out_len;

        /* extract relevant part of the key */
        out = ra_extract_key(ra, key, key_len, &out_len TSRMLS_CC);
        if(!out)
                return NULL;

        if(ra->z_dist) {
                if (!ra_call_distributor(ra, key, key_len, &pos TSRMLS_CC)) {
                        return NULL;
                }
        }
        else {
                /* hash */
                hash = rcrc32(out, out_len);
                efree(out);
        
                /* get position on ring */
                uint64_t h64 = hash;
                h64 *= ra->count;
                h64 /= 0xffffffff;
                pos = (int)h64;
        }
        if(out_pos) *out_pos = pos;

        return ra->redis[pos];
}

zval *
ra_find_node_by_name(RedisArray *ra, const char *host, int host_len TSRMLS_DC) {

	int i;
	for(i = 0; i < ra->count; ++i) {
		if(strncmp(ra->hosts[i], host, host_len) == 0) {
			return ra->redis[i];
		}
	}
	return NULL;
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
ra_index_multi(zval *z_redis, long multi_value TSRMLS_DC) {

	zval z_fun_multi, z_ret;
	zval *z_args[1];

	/* run MULTI */
	ZVAL_STRING(&z_fun_multi, "MULTI", 0);
	MAKE_STD_ZVAL(z_args[0]);
	ZVAL_LONG(z_args[0], multi_value);
	call_user_function(&redis_ce->function_table, &z_redis, &z_fun_multi, &z_ret, 1, z_args TSRMLS_CC);
	efree(z_args[0]);
	//zval_dtor(&z_ret);
}

static void
ra_index_change_keys(const char *cmd, zval *z_keys, zval *z_redis TSRMLS_DC) {

	int i, argc;
	zval z_fun, z_ret, **z_args;

	/* alloc */
	argc = 1 + zend_hash_num_elements(Z_ARRVAL_P(z_keys));
	z_args = emalloc(argc * sizeof(zval*));

	/* prepare first parameters */
	ZVAL_STRING(&z_fun, cmd, 0);
	MAKE_STD_ZVAL(z_args[0]);
	ZVAL_STRING(z_args[0], PHPREDIS_INDEX_NAME, 0);

	/* prepare keys */
	for(i = 0; i < argc - 1; ++i) {
		zval **zpp;
		zend_hash_quick_find(Z_ARRVAL_P(z_keys), NULL, 0, i, (void**)&zpp);
		z_args[i+1] = *zpp;
	}

	/* run cmd */
	call_user_function(&redis_ce->function_table, &z_redis, &z_fun, &z_ret, argc, z_args TSRMLS_CC);

	/* don't dtor z_ret, since we're returning z_redis */
	efree(z_args[0]); 	/* free index name zval */
	efree(z_args);		/* free container */
}

void
ra_index_del(zval *z_keys, zval *z_redis TSRMLS_DC) {
	ra_index_change_keys("SREM", z_keys, z_redis TSRMLS_CC);
}

void
ra_index_keys(zval *z_pairs, zval *z_redis TSRMLS_DC) {

	/* Initialize key array */
	zval *z_keys, **z_entry_pp;
	MAKE_STD_ZVAL(z_keys);
#if PHP_VERSION_ID > 50300
	array_init_size(z_keys, zend_hash_num_elements(Z_ARRVAL_P(z_pairs)));
#else
	array_init(z_keys);
#endif
	HashPosition pos;

	/* Go through input array and add values to the key array */
	zend_hash_internal_pointer_reset_ex(Z_ARRVAL_P(z_pairs), &pos);
	while (zend_hash_get_current_data_ex(Z_ARRVAL_P(z_pairs), (void **)&z_entry_pp, &pos) == SUCCESS) {
			char *key;
			unsigned int key_len;
			unsigned long num_key;
			zval *z_new;
			MAKE_STD_ZVAL(z_new);

			switch (zend_hash_get_current_key_ex(Z_ARRVAL_P(z_pairs), &key, &key_len, &num_key, 1, &pos)) {
				case HASH_KEY_IS_STRING:
					ZVAL_STRINGL(z_new, key, (int)key_len - 1, 0);
					zend_hash_next_index_insert(Z_ARRVAL_P(z_keys), &z_new, sizeof(zval *), NULL);
					break;

				case HASH_KEY_IS_LONG:
					Z_TYPE_P(z_new) = IS_LONG;
					Z_LVAL_P(z_new) = (long)num_key;
					zend_hash_next_index_insert(Z_ARRVAL_P(z_keys), &z_new, sizeof(zval *), NULL);
					break;
			}
			zend_hash_move_forward_ex(Z_ARRVAL_P(z_pairs), &pos);
	}

	/* add keys to index */
	ra_index_change_keys("SADD", z_keys, z_redis TSRMLS_CC);

	/* cleanup */
	zval_dtor(z_keys);
	efree(z_keys);
}

void
ra_index_key(const char *key, int key_len, zval *z_redis TSRMLS_DC) {

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
    zval_dtor(z_args[1]);
	efree(z_args[1]);
}

void
ra_index_exec(zval *z_redis, zval *return_value, int keep_all TSRMLS_DC) {

	zval z_fun_exec, z_ret, **zp_tmp;

	/* run EXEC */
	ZVAL_STRING(&z_fun_exec, "EXEC", 0);
	call_user_function(&redis_ce->function_table, &z_redis, &z_fun_exec, &z_ret, 0, NULL TSRMLS_CC);


	/* extract first element of exec array and put into return_value. */
	if(Z_TYPE(z_ret) == IS_ARRAY) {
		if(return_value) {
				if(keep_all) {
						*return_value = z_ret;
						zval_copy_ctor(return_value);
				} else if(zend_hash_quick_find(Z_ARRVAL(z_ret), NULL, 0, 0, (void**)&zp_tmp) != FAILURE) {
						*return_value = **zp_tmp;
						zval_copy_ctor(return_value);
				}
		}
		zval_dtor(&z_ret);
	}

	//zval *zptr = &z_ret;
	//php_var_dump(&zptr, 0 TSRMLS_CC);
}

void
ra_index_discard(zval *z_redis, zval *return_value TSRMLS_DC) {

	zval z_fun_discard, z_ret;

	/* run DISCARD */
	ZVAL_STRING(&z_fun_discard, "DISCARD", 0);
	call_user_function(&redis_ce->function_table, &z_redis, &z_fun_discard, &z_ret, 0, NULL TSRMLS_CC);

	zval_dtor(&z_ret);
}

void
ra_index_unwatch(zval *z_redis, zval *return_value TSRMLS_DC) {

	zval z_fun_unwatch, z_ret;

	/* run UNWATCH */
	ZVAL_STRING(&z_fun_unwatch, "UNWATCH", 0);
	call_user_function(&redis_ce->function_table, &z_redis, &z_fun_unwatch, &z_ret, 0, NULL TSRMLS_CC);

	zval_dtor(&z_ret);
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
ra_rehash_scan(zval *z_redis, char ***keys, int **key_lens, const char *cmd, const char *arg TSRMLS_DC) {

	long count, i;
	zval z_fun_smembers, z_ret, *z_arg, **z_data_pp;
	HashTable *h_keys;
	HashPosition pointer;
	char *key;
	int key_len;

	/* arg */
	MAKE_STD_ZVAL(z_arg);
	ZVAL_STRING(z_arg, arg, 0);

	/* run SMEMBERS */
	ZVAL_STRING(&z_fun_smembers, cmd, 0);
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

static long
ra_rehash_scan_index(zval *z_redis, char ***keys, int **key_lens TSRMLS_DC) {
	return ra_rehash_scan(z_redis, keys, key_lens, "SMEMBERS", PHPREDIS_INDEX_NAME TSRMLS_CC);
}

/* list keys using KEYS command */
static long
ra_rehash_scan_keys(zval *z_redis, char ***keys, int **key_lens TSRMLS_DC) {
	return ra_rehash_scan(z_redis, keys, key_lens, "KEYS", "*" TSRMLS_CC);
}

/* run TYPE to find the type */
static zend_bool
ra_get_key_type(zval *z_redis, const char *key, int key_len, zval *z_from, long *res TSRMLS_DC) {

	int i;
	zval z_fun_type, z_ret, *z_arg;
	zval **z_data;
	long success = 1;

	MAKE_STD_ZVAL(z_arg);
	/* Pipelined */
	ra_index_multi(z_from, PIPELINE TSRMLS_CC);

	/* prepare args */
	ZVAL_STRINGL(&z_fun_type, "TYPE", 4, 0);
	ZVAL_STRINGL(z_arg, key, key_len, 0);
	/* run TYPE */
	call_user_function(&redis_ce->function_table, &z_redis, &z_fun_type, &z_ret, 1, &z_arg TSRMLS_CC);

	ZVAL_STRINGL(&z_fun_type, "TTL", 3, 0);
	ZVAL_STRINGL(z_arg, key, key_len, 0);
	/* run TYPE */
	call_user_function(&redis_ce->function_table, &z_redis, &z_fun_type, &z_ret, 1, &z_arg TSRMLS_CC);

	/* cleanup */
	efree(z_arg);

	/* Get the result from the pipeline. */
	ra_index_exec(z_from, &z_ret, 1 TSRMLS_CC);
	if(Z_TYPE(z_ret) == IS_ARRAY) {
        HashTable *retHash = Z_ARRVAL(z_ret);
		for(i = 0, zend_hash_internal_pointer_reset(retHash);
				zend_hash_has_more_elements(retHash) == SUCCESS;
				zend_hash_move_forward(retHash)) {

			if(zend_hash_get_current_data(retHash, (void**)&z_data) == FAILURE) {
				success = 0;
				break;
			}	
			if(Z_TYPE_PP(z_data) != IS_LONG) {
				success = 0;
				break;
			}
			/* Get the result - Might change in the future to handle doubles as well */
			res[i] = Z_LVAL_PP(z_data);
			i++;
		}
	}
	zval_dtor(&z_ret);
	return success;
}

/* delete key from source server index during rehashing */
static void
ra_remove_from_index(zval *z_redis, const char *key, int key_len TSRMLS_DC) {

	zval z_fun_srem, z_ret, *z_args[2];

	/* run SREM on source index */
	ZVAL_STRINGL(&z_fun_srem, "SREM", 4, 0);
	MAKE_STD_ZVAL(z_args[0]);
	ZVAL_STRING(z_args[0], PHPREDIS_INDEX_NAME, 0);
	MAKE_STD_ZVAL(z_args[1]);
	ZVAL_STRINGL(z_args[1], key, key_len, 0);

	call_user_function(&redis_ce->function_table, &z_redis, &z_fun_srem, &z_ret, 2, z_args TSRMLS_CC);

	/* cleanup */
	efree(z_args[0]);
	efree(z_args[1]);
}


/* delete key from source server during rehashing */
static zend_bool
ra_del_key(const char *key, int key_len, zval *z_from TSRMLS_DC) {

	zval z_fun_del, z_ret, *z_args;

	/* in a transaction */
	ra_index_multi(z_from, MULTI TSRMLS_CC);

	/* run DEL on source */
	MAKE_STD_ZVAL(z_args);
	ZVAL_STRINGL(&z_fun_del, "DEL", 3, 0);
	ZVAL_STRINGL(z_args, key, key_len, 0);
	call_user_function(&redis_ce->function_table, &z_from, &z_fun_del, &z_ret, 1, &z_args TSRMLS_CC);
	efree(z_args);

	/* remove key from index */
	ra_remove_from_index(z_from, key, key_len TSRMLS_CC);

	/* close transaction */
	ra_index_exec(z_from, NULL, 0 TSRMLS_CC);

	return 1;
}

static zend_bool
ra_expire_key(const char *key, int key_len, zval *z_to, long ttl TSRMLS_DC) {

	zval z_fun_expire, z_ret, *z_args[2];

	if (ttl > 0)
	{
		/* run EXPIRE on target */
		MAKE_STD_ZVAL(z_args[0]);
		MAKE_STD_ZVAL(z_args[1]);
		ZVAL_STRINGL(&z_fun_expire, "EXPIRE", 6, 0);
		ZVAL_STRINGL(z_args[0], key, key_len, 0);
		ZVAL_LONG(z_args[1], ttl);
		call_user_function(&redis_ce->function_table, &z_to, &z_fun_expire, &z_ret, 2, z_args TSRMLS_CC);
		/* cleanup */
		efree(z_args[0]);
		efree(z_args[1]);
	}

	return 1;
}

static zend_bool
ra_move_zset(const char *key, int key_len, zval *z_from, zval *z_to, long ttl TSRMLS_DC) {

	zval z_fun_zrange, z_fun_zadd, z_ret, *z_args[4], **z_zadd_args, **z_score_pp;
	int count;
	HashTable *h_zset_vals;
	char *val;
	unsigned int val_len;
	int i;
	unsigned long idx;
	
	/* run ZRANGE key 0 -1 WITHSCORES on source */
	ZVAL_STRINGL(&z_fun_zrange, "ZRANGE", 6, 0);
	for(i = 0; i < 4; ++i) {
		MAKE_STD_ZVAL(z_args[i]);
	}
	ZVAL_STRINGL(z_args[0], key, key_len, 0);
	ZVAL_STRINGL(z_args[1], "0", 1, 0);
	ZVAL_STRINGL(z_args[2], "-1", 2, 0);
	ZVAL_BOOL(z_args[3], 1);
	call_user_function(&redis_ce->function_table, &z_from, &z_fun_zrange, &z_ret, 4, z_args TSRMLS_CC);

	/* cleanup zrange args */
	for(i = 0; i < 4; ++i) {
		efree(z_args[i]); /* FIXME */
	}

	if(Z_TYPE(z_ret) != IS_ARRAY) { /* key not found or replaced */
		/* TODO: report? */
		return 0;
	}

	/* we now have an array of value → score pairs in z_ret. */
	h_zset_vals = Z_ARRVAL(z_ret);

	/* allocate argument array for ZADD */
	count = zend_hash_num_elements(h_zset_vals);
	z_zadd_args = emalloc((1 + 2*count) * sizeof(zval*));

	for(i = 1, zend_hash_internal_pointer_reset(h_zset_vals);
			zend_hash_has_more_elements(h_zset_vals) == SUCCESS;
			zend_hash_move_forward(h_zset_vals)) {

		if(zend_hash_get_current_data(h_zset_vals, (void**)&z_score_pp) == FAILURE) {
			continue;
		}

		/* add score */
		convert_to_double(*z_score_pp);
		MAKE_STD_ZVAL(z_zadd_args[i]);
		ZVAL_DOUBLE(z_zadd_args[i], Z_DVAL_PP(z_score_pp));

		/* add value */
		MAKE_STD_ZVAL(z_zadd_args[i+1]);
		switch (zend_hash_get_current_key_ex(h_zset_vals, &val, &val_len, &idx, 0, NULL)) {
			case HASH_KEY_IS_STRING:
				ZVAL_STRINGL(z_zadd_args[i+1], val, (int)val_len-1, 0); /* we have to remove 1 because it is an array key. */
				break;
			case HASH_KEY_IS_LONG:
				ZVAL_LONG(z_zadd_args[i+1], (long)idx);
				break;
			default:
				return -1; // Todo: log error
				break;
		}
		i += 2;
	}

	/* run ZADD on target */
	ZVAL_STRINGL(&z_fun_zadd, "ZADD", 4, 0);
	MAKE_STD_ZVAL(z_zadd_args[0]);
	ZVAL_STRINGL(z_zadd_args[0], key, key_len, 0);
	call_user_function(&redis_ce->function_table, &z_to, &z_fun_zadd, &z_ret, 1 + 2 * count, z_zadd_args TSRMLS_CC);

	/* Expire if needed */
	ra_expire_key(key, key_len, z_to, ttl TSRMLS_CC);

	/* cleanup */
	for(i = 0; i < 1 + 2 * count; ++i) {
		efree(z_zadd_args[i]);
	}

	return 1;
}

static zend_bool
ra_move_string(const char *key, int key_len, zval *z_from, zval *z_to, long ttl TSRMLS_DC) {

	zval z_fun_get, z_fun_set, z_ret, *z_args[3];

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
	if (ttl > 0) {
		MAKE_STD_ZVAL(z_args[2]);
		ZVAL_STRINGL(&z_fun_set, "SETEX", 5, 0);
		ZVAL_STRINGL(z_args[0], key, key_len, 0);
		ZVAL_LONG(z_args[1], ttl);
		ZVAL_STRINGL(z_args[2], Z_STRVAL(z_ret), Z_STRLEN(z_ret), 1); /* copy z_ret to arg 1 */
		zval_dtor(&z_ret); /* free memory from our previous call */
		call_user_function(&redis_ce->function_table, &z_to, &z_fun_set, &z_ret, 3, z_args TSRMLS_CC);
		/* cleanup */
		efree(z_args[1]);
		zval_dtor(z_args[2]);
		efree(z_args[2]);
	}
	else {
		ZVAL_STRINGL(&z_fun_set, "SET", 3, 0);
		ZVAL_STRINGL(z_args[0], key, key_len, 0);
		ZVAL_STRINGL(z_args[1], Z_STRVAL(z_ret), Z_STRLEN(z_ret), 1); /* copy z_ret to arg 1 */
		zval_dtor(&z_ret); /* free memory from our previous return value */
		call_user_function(&redis_ce->function_table, &z_to, &z_fun_set, &z_ret, 2, z_args TSRMLS_CC);
		/* cleanup */
		zval_dtor(z_args[1]);
		efree(z_args[1]);
	}

	/* cleanup */
	efree(z_args[0]);

	return 1;
}

static zend_bool
ra_move_hash(const char *key, int key_len, zval *z_from, zval *z_to, long ttl TSRMLS_DC) {

	zval z_fun_hgetall, z_fun_hmset, z_ret, *z_args[2];

	/* run HGETALL on source */
	MAKE_STD_ZVAL(z_args[0]);
	ZVAL_STRINGL(&z_fun_hgetall, "HGETALL", 7, 0);
	ZVAL_STRINGL(z_args[0], key, key_len, 0);
	call_user_function(&redis_ce->function_table, &z_from, &z_fun_hgetall, &z_ret, 1, z_args TSRMLS_CC);

	if(Z_TYPE(z_ret) != IS_ARRAY) { /* key not found or replaced */
		/* TODO: report? */
		efree(z_args[0]);
		return 0;
	}

	/* run HMSET on target */
	ZVAL_STRINGL(&z_fun_hmset, "HMSET", 5, 0);
	ZVAL_STRINGL(z_args[0], key, key_len, 0);
	z_args[1] = &z_ret; /* copy z_ret to arg 1 */
	call_user_function(&redis_ce->function_table, &z_to, &z_fun_hmset, &z_ret, 2, z_args TSRMLS_CC);

	/* Expire if needed */
	ra_expire_key(key, key_len, z_to, ttl TSRMLS_CC);

	/* cleanup */
	efree(z_args[0]);

	return 1;
}

static zend_bool
ra_move_collection(const char *key, int key_len, zval *z_from, zval *z_to,
		int list_count, const char **cmd_list,
		int add_count, const char **cmd_add, long ttl TSRMLS_DC) {

	zval z_fun_retrieve, z_fun_sadd, z_ret, **z_retrieve_args, **z_sadd_args, **z_data_pp;
	int count, i;
	HashTable *h_set_vals;

	/* run retrieval command on source */
	z_retrieve_args = emalloc((1+list_count) * sizeof(zval*));
	ZVAL_STRING(&z_fun_retrieve, cmd_list[0], 0);	/* set the command */

	/* set the key */
	MAKE_STD_ZVAL(z_retrieve_args[0]);
	ZVAL_STRINGL(z_retrieve_args[0], key, key_len, 0);

	/* possibly add some other args if they were provided. */
	for(i = 1; i < list_count; ++i) {
		MAKE_STD_ZVAL(z_retrieve_args[i]);
		ZVAL_STRING(z_retrieve_args[i], cmd_list[i], 0);
	}

	call_user_function(&redis_ce->function_table, &z_from, &z_fun_retrieve, &z_ret, list_count, z_retrieve_args TSRMLS_CC);

	/* cleanup */
	for(i = 0; i < list_count; ++i) {
		efree(z_retrieve_args[i]);
	}
	efree(z_retrieve_args);

	if(Z_TYPE(z_ret) != IS_ARRAY) { /* key not found or replaced */
		/* TODO: report? */
		return 0;
	}

	/* run SADD/RPUSH on target */
	h_set_vals = Z_ARRVAL(z_ret);
	count = zend_hash_num_elements(h_set_vals);
	z_sadd_args = emalloc((1 + count) * sizeof(zval*));
	ZVAL_STRING(&z_fun_sadd, cmd_add[0], 0);
	MAKE_STD_ZVAL(z_sadd_args[0]);	/* add key */
	ZVAL_STRINGL(z_sadd_args[0], key, key_len, 0);

	for(i = 0, zend_hash_internal_pointer_reset(h_set_vals);
			zend_hash_has_more_elements(h_set_vals) == SUCCESS;
			zend_hash_move_forward(h_set_vals), i++) {

		if(zend_hash_get_current_data(h_set_vals, (void**)&z_data_pp) == FAILURE) {
			continue;
		}

		/* add set elements */
		MAKE_STD_ZVAL(z_sadd_args[i+1]);
		*(z_sadd_args[i+1]) = **z_data_pp;
		zval_copy_ctor(z_sadd_args[i+1]);
	}
	call_user_function(&redis_ce->function_table, &z_to, &z_fun_sadd, &z_ret, count+1, z_sadd_args TSRMLS_CC);

	/* Expire if needed */
	ra_expire_key(key, key_len, z_to, ttl TSRMLS_CC);

	/* cleanup */
	efree(z_sadd_args[0]); /* no dtor at [0] */

	for(i = 0; i < count; ++i) {
		zval_dtor(z_sadd_args[i + 1]);
		efree(z_sadd_args[i + 1]);
	}
	efree(z_sadd_args);

	return 1;
}

static zend_bool
ra_move_set(const char *key, int key_len, zval *z_from, zval *z_to, long ttl TSRMLS_DC) {

	const char *cmd_list[] = {"SMEMBERS"};
	const char *cmd_add[] = {"SADD"};
	return ra_move_collection(key, key_len, z_from, z_to, 1, cmd_list, 1, cmd_add, ttl TSRMLS_CC);
}

static zend_bool
ra_move_list(const char *key, int key_len, zval *z_from, zval *z_to, long ttl TSRMLS_DC) {

	const char *cmd_list[] = {"LRANGE", "0", "-1"};
	const char *cmd_add[] = {"RPUSH"};
	return ra_move_collection(key, key_len, z_from, z_to, 3, cmd_list, 1, cmd_add, ttl TSRMLS_CC);
}

void
ra_move_key(const char *key, int key_len, zval *z_from, zval *z_to TSRMLS_DC) {

	long res[2], type, ttl;
	zend_bool success = 0;
	if (ra_get_key_type(z_from, key, key_len, z_from, res TSRMLS_CC)) {
		type = res[0];
		ttl = res[1];
		/* open transaction on target server */
		ra_index_multi(z_to, MULTI TSRMLS_CC);
		switch(type) {
			case REDIS_STRING:
				success = ra_move_string(key, key_len, z_from, z_to, ttl TSRMLS_CC);
				break;
	
			case REDIS_SET:
				success = ra_move_set(key, key_len, z_from, z_to, ttl TSRMLS_CC);
				break;
	
			case REDIS_LIST:
				success = ra_move_list(key, key_len, z_from, z_to, ttl TSRMLS_CC);
				break;
	
			case REDIS_ZSET:
				success = ra_move_zset(key, key_len, z_from, z_to, ttl TSRMLS_CC);
				break;
	
			case REDIS_HASH:
				success = ra_move_hash(key, key_len, z_from, z_to, ttl TSRMLS_CC);
				break;
	
			default:
				/* TODO: report? */
				break;
		}
	}

	if(success) {
		ra_del_key(key, key_len, z_from TSRMLS_CC);
		ra_index_key(key, key_len, z_to TSRMLS_CC);
	}

	/* close transaction */
	ra_index_exec(z_to, NULL, 0 TSRMLS_CC);
}

/* callback with the current progress, with hostname and count */
static void zval_rehash_callback(zend_fcall_info *z_cb, zend_fcall_info_cache *z_cb_cache,
	const char *hostname, long count TSRMLS_DC) {

	zval *z_ret = NULL, **z_args[2];
	zval *z_host, *z_count;

	z_cb->retval_ptr_ptr = &z_ret;
	z_cb->params = (struct _zval_struct ***)&z_args;
	z_cb->param_count = 2;
	z_cb->no_separation = 0;

	/* run cb(hostname, count) */
	MAKE_STD_ZVAL(z_host);
	ZVAL_STRING(z_host, hostname, 0);
	z_args[0] = &z_host;
	MAKE_STD_ZVAL(z_count);
	ZVAL_LONG(z_count, count);
	z_args[1] = &z_count;

	zend_call_function(z_cb, z_cb_cache TSRMLS_CC);

	/* cleanup */
	efree(z_host);
	efree(z_count);
	if(z_ret)
		efree(z_ret);
}

static void
ra_rehash_server(RedisArray *ra, zval *z_redis, const char *hostname, zend_bool b_index,
		zend_fcall_info *z_cb, zend_fcall_info_cache *z_cb_cache TSRMLS_DC) {

	char **keys;
	int *key_lens;
	long count, i;
	int target_pos;
	zval *z_target;

	/* list all keys */
	if(b_index) {
		count = ra_rehash_scan_index(z_redis, &keys, &key_lens TSRMLS_CC);
	} else {
		count = ra_rehash_scan_keys(z_redis, &keys, &key_lens TSRMLS_CC);
	}

	/* callback */
	if(z_cb && z_cb_cache) {
		zval_rehash_callback(z_cb, z_cb_cache, hostname, count TSRMLS_CC);
	}

	/* for each key, redistribute */
	for(i = 0; i < count; ++i) {

		/* check that we're not moving to the same node. */
		z_target = ra_find_node(ra, keys[i], key_lens[i], &target_pos TSRMLS_CC);

		if(strcmp(hostname, ra->hosts[target_pos])) { /* different host */
			/* php_printf("move [%s] from [%s] to [%s]\n", keys[i], hostname, ra->hosts[target_pos]); */
			ra_move_key(keys[i], key_lens[i], z_redis, z_target TSRMLS_CC);
		}
	}

	/* cleanup */
	for(i = 0; i < count; ++i) {
		efree(keys[i]);
	}
	efree(keys);
	efree(key_lens);
}

void
ra_rehash(RedisArray *ra, zend_fcall_info *z_cb, zend_fcall_info_cache *z_cb_cache TSRMLS_DC) {

	int i;

	/* redistribute the data, server by server. */
	if(!ra->prev)
		return;	/* TODO: compare the two rings for equality */

	for(i = 0; i < ra->prev->count; ++i) {
		ra_rehash_server(ra, ra->prev->redis[i], ra->prev->hosts[i], ra->index, z_cb, z_cb_cache TSRMLS_CC);
	}
}

