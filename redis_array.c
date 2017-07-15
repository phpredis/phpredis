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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "common.h"
#include "ext/standard/info.h"
#include "php_ini.h"
#include "php_redis.h"
#include <zend_exceptions.h>

#include "library.h"
#include "redis_array.h"
#include "redis_array_impl.h"

/* Simple macro to detect failure in a RedisArray call */
#if (PHP_MAJOR_VERSION < 7)
#define RA_CALL_FAILED(rv, cmd) ( \
    (Z_TYPE_P(rv) == IS_BOOL && !Z_LVAL_P(rv)) || \
    (Z_TYPE_P(rv) == IS_ARRAY && zend_hash_num_elements(Z_ARRVAL_P(rv)) == 0) || \
    (Z_TYPE_P(rv) == IS_LONG && Z_LVAL_P(rv) == 0 && !strcasecmp(cmd, "TYPE")) \
)
#else
#define RA_CALL_FAILED(rv, cmd) ( \
    (Z_TYPE_P(rv) == IS_FALSE) || \
    (Z_TYPE_P(rv) == IS_ARRAY && zend_hash_num_elements(Z_ARRVAL_P(rv)) == 0) || \
    (Z_TYPE_P(rv) == IS_LONG && Z_LVAL_P(rv) == 0 && !strcasecmp(cmd, "TYPE")) \
)
#endif

extern zend_class_entry *redis_ce;
zend_class_entry *redis_array_ce;

ZEND_BEGIN_ARG_INFO_EX(__redis_array_call_args, 0, 0, 2)
	ZEND_ARG_INFO(0, function_name)
	ZEND_ARG_INFO(0, arguments)
ZEND_END_ARG_INFO()

zend_function_entry redis_array_functions[] = {
     PHP_ME(RedisArray, __construct, NULL, ZEND_ACC_PUBLIC)
     PHP_ME(RedisArray, __call, __redis_array_call_args, ZEND_ACC_PUBLIC)

     PHP_ME(RedisArray, _hosts, NULL, ZEND_ACC_PUBLIC)
     PHP_ME(RedisArray, _target, NULL, ZEND_ACC_PUBLIC)
     PHP_ME(RedisArray, _instance, NULL, ZEND_ACC_PUBLIC)
     PHP_ME(RedisArray, _function, NULL, ZEND_ACC_PUBLIC)
     PHP_ME(RedisArray, _distributor, NULL, ZEND_ACC_PUBLIC)
     PHP_ME(RedisArray, _rehash, NULL, ZEND_ACC_PUBLIC)

     /* special implementation for a few functions */
     PHP_ME(RedisArray, select, NULL, ZEND_ACC_PUBLIC)
     PHP_ME(RedisArray, info, NULL, ZEND_ACC_PUBLIC)
     PHP_ME(RedisArray, ping, NULL, ZEND_ACC_PUBLIC)
     PHP_ME(RedisArray, flushdb, NULL, ZEND_ACC_PUBLIC)
     PHP_ME(RedisArray, flushall, NULL, ZEND_ACC_PUBLIC)
     PHP_ME(RedisArray, mget, NULL, ZEND_ACC_PUBLIC)
     PHP_ME(RedisArray, mset, NULL, ZEND_ACC_PUBLIC)
     PHP_ME(RedisArray, del, NULL, ZEND_ACC_PUBLIC)
     PHP_ME(RedisArray, getOption, NULL, ZEND_ACC_PUBLIC)
     PHP_ME(RedisArray, setOption, NULL, ZEND_ACC_PUBLIC)
     PHP_ME(RedisArray, keys, NULL, ZEND_ACC_PUBLIC)
     PHP_ME(RedisArray, save, NULL, ZEND_ACC_PUBLIC)
     PHP_ME(RedisArray, bgsave, NULL, ZEND_ACC_PUBLIC)

	 /* Multi/Exec */
     PHP_ME(RedisArray, multi, NULL, ZEND_ACC_PUBLIC)
     PHP_ME(RedisArray, exec, NULL, ZEND_ACC_PUBLIC)
     PHP_ME(RedisArray, discard, NULL, ZEND_ACC_PUBLIC)
     PHP_ME(RedisArray, unwatch, NULL, ZEND_ACC_PUBLIC)

     /* Aliases */
     PHP_MALIAS(RedisArray, delete, del, NULL, ZEND_ACC_PUBLIC)
     PHP_MALIAS(RedisArray, getMultiple, mget, NULL, ZEND_ACC_PUBLIC)
     PHP_FE_END
};

static void
redis_array_free(RedisArray *ra)
{
    int i;

    /* Redis objects */
    for(i=0;i<ra->count;i++) {
        zval_dtor(&ra->redis[i]);
        efree(ra->hosts[i]);
    }
    efree(ra->redis);
    efree(ra->hosts);

    /* delete hash function */
    zval_dtor(&ra->z_fun);

    /* Distributor */
    zval_dtor(&ra->z_dist);

    /* Delete pur commands */
    zval_dtor(&ra->z_pure_cmds);

    /* Free structure itself */
    efree(ra);
}

#if (PHP_MAJOR_VERSION < 7)
typedef struct {
    zend_object std;
    RedisArray *ra;
} redis_array_object;

void
free_redis_array_object(void *object TSRMLS_DC)
{
    redis_array_object *obj = (redis_array_object *)object;

    zend_object_std_dtor(&obj->std TSRMLS_CC);
    if (obj->ra) {
        if (obj->ra->prev) redis_array_free(obj->ra->prev);
        redis_array_free(obj->ra);
    }
    efree(obj);
}

zend_object_value
create_redis_array_object(zend_class_entry *ce TSRMLS_DC)
{
    zend_object_value retval;
    redis_array_object *obj = ecalloc(1, sizeof(redis_array_object));
    memset(obj, 0, sizeof(redis_array_object));

    zend_object_std_init(&obj->std, ce TSRMLS_CC);

#if PHP_VERSION_ID < 50399
    zval *tmp;
    zend_hash_copy(obj->std.properties, &ce->default_properties,
        (copy_ctor_func_t)zval_add_ref, (void *)&tmp, sizeof(zval *));
#endif

    retval.handle = zend_objects_store_put(obj,
        (zend_objects_store_dtor_t)zend_objects_destroy_object,
        (zend_objects_free_object_storage_t)free_redis_array_object,
        NULL TSRMLS_CC);
    retval.handlers = zend_get_std_object_handlers();

    return retval;
}
#else
typedef struct {
    RedisArray *ra;
    zend_object std;
} redis_array_object;

zend_object_handlers redis_array_object_handlers;

void
free_redis_array_object(zend_object *object)
{
    redis_array_object *obj = (redis_array_object *)((char *)(object) - XtOffsetOf(redis_array_object, std));

    if (obj->ra) {
        if (obj->ra->prev) redis_array_free(obj->ra->prev);
        redis_array_free(obj->ra);
    }
    zend_object_std_dtor(&obj->std TSRMLS_CC);
}

zend_object *
create_redis_array_object(zend_class_entry *ce TSRMLS_DC)
{
    redis_array_object *obj = ecalloc(1, sizeof(redis_array_object) + zend_object_properties_size(ce));

    obj->ra = NULL;

    zend_object_std_init(&obj->std, ce TSRMLS_CC);
    object_properties_init(&obj->std, ce);

    memcpy(&redis_array_object_handlers, zend_get_std_object_handlers(), sizeof(redis_array_object_handlers));
    redis_array_object_handlers.offset = XtOffsetOf(redis_array_object, std);
    redis_array_object_handlers.free_obj = free_redis_array_object;
    obj->std.handlers = &redis_array_object_handlers;

    return &obj->std;
}
#endif

/**
 * redis_array_get
 */
PHP_REDIS_API RedisArray *
redis_array_get(zval *id TSRMLS_DC)
{
    redis_array_object *obj;

    if (Z_TYPE_P(id) == IS_OBJECT) {
#if (PHP_MAJOR_VERSION < 7)
        obj = (redis_array_object *)zend_objects_get_address(id TSRMLS_CC);
#else
        obj = (redis_array_object *)((char *)Z_OBJ_P(id) - XtOffsetOf(redis_array_object, std));
#endif
        if (obj->ra) {
            return obj->ra;
        }
    }
    return NULL;
}

/* {{{ proto RedisArray RedisArray::__construct()
    Public constructor */
PHP_METHOD(RedisArray, __construct)
{
	zval *z0, z_fun, z_dist, *zpData, *z_opts = NULL;
	RedisArray *ra = NULL;
	zend_bool b_index = 0, b_autorehash = 0, b_pconnect = 0;
	HashTable *hPrev = NULL, *hOpts = NULL;
	long l_retry_interval = 0;
  	zend_bool b_lazy_connect = 0;
	double d_connect_timeout = 0, read_timeout = 0.0;
    redis_array_object *obj;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "z|a", &z0, &z_opts) == FAILURE) {
		RETURN_FALSE;
	}

    ZVAL_NULL(&z_fun);
    ZVAL_NULL(&z_dist);
	/* extract options */
	if(z_opts) {
		hOpts = Z_ARRVAL_P(z_opts);

		/* extract previous ring. */
        if ((zpData = zend_hash_str_find(hOpts, "previous", sizeof("previous") - 1)) != NULL && Z_TYPE_P(zpData) == IS_ARRAY
            && zend_hash_num_elements(Z_ARRVAL_P(zpData)) != 0
        ) {
			/* consider previous array as non-existent if empty. */
            hPrev = Z_ARRVAL_P(zpData);
		}

		/* extract function name. */
        if ((zpData = zend_hash_str_find(hOpts, "function", sizeof("function") - 1)) != NULL) {
            ZVAL_ZVAL(&z_fun, zpData, 1, 0);
		}

		/* extract function name. */
        if ((zpData = zend_hash_str_find(hOpts, "distributor", sizeof("distributor") - 1)) != NULL) {
            ZVAL_ZVAL(&z_dist, zpData, 1, 0);
		}

		/* extract index option. */
        if ((zpData = zend_hash_str_find(hOpts, "index", sizeof("index") - 1)) != NULL) {
            b_index = zval_is_true(zpData);
		}

		/* extract autorehash option. */
        if ((zpData = zend_hash_str_find(hOpts, "autorehash", sizeof("autorehash") - 1)) != NULL) {
            b_autorehash = zval_is_true(zpData);
		}

		/* pconnect */
        if ((zpData = zend_hash_str_find(hOpts, "pconnect", sizeof("pconnect") - 1)) != NULL) {
            b_pconnect = zval_is_true(zpData);
		}

		/* extract retry_interval option. */
        if ((zpData = zend_hash_str_find(hOpts, "retry_interval", sizeof("retry_interval") - 1)) != NULL) {
            if (Z_TYPE_P(zpData) == IS_LONG) {
                l_retry_interval = Z_LVAL_P(zpData);
            } else if (Z_TYPE_P(zpData) == IS_STRING) {
                l_retry_interval = atol(Z_STRVAL_P(zpData));
            }
        }

		/* extract lazy connect option. */
        if ((zpData = zend_hash_str_find(hOpts, "lazy_connect", sizeof("lazy_connect") - 1)) != NULL) {
            b_lazy_connect = zval_is_true(zpData);
		}
		
		/* extract connect_timeout option */		
        if ((zpData = zend_hash_str_find(hOpts, "connect_timeout", sizeof("connect_timeout") - 1)) != NULL) {
            if (Z_TYPE_P(zpData) == IS_DOUBLE) {
                d_connect_timeout = Z_DVAL_P(zpData);
            } else if (Z_TYPE_P(zpData) == IS_LONG) {
                d_connect_timeout = Z_LVAL_P(zpData);
            } else if (Z_TYPE_P(zpData) == IS_STRING) {
                d_connect_timeout = atof(Z_STRVAL_P(zpData));
            }
        }

        /* extract read_timeout option */
        if ((zpData = zend_hash_str_find(hOpts, "read_timeout", sizeof("read_timeout") - 1)) != NULL) {
            if (Z_TYPE_P(zpData) == IS_DOUBLE) {
                read_timeout = Z_DVAL_P(zpData);
            } else if (Z_TYPE_P(zpData) == IS_LONG) {
                read_timeout = Z_LVAL_P(zpData);
            } else if (Z_TYPE_P(zpData) == IS_STRING) {
                read_timeout = atof(Z_STRVAL_P(zpData));
            }
        }
	}

	/* extract either name of list of hosts from z0 */
	switch(Z_TYPE_P(z0)) {
		case IS_STRING:
			ra = ra_load_array(Z_STRVAL_P(z0) TSRMLS_CC);
			break;

		case IS_ARRAY:
			ra = ra_make_array(Z_ARRVAL_P(z0), &z_fun, &z_dist, hPrev, b_index, b_pconnect, l_retry_interval, b_lazy_connect, d_connect_timeout, read_timeout TSRMLS_CC);
			break;

		default:
			WRONG_PARAM_COUNT;
	}
    zval_dtor(&z_dist);
    zval_dtor(&z_fun);

	if(ra) {
		ra->auto_rehash = b_autorehash;
		ra->connect_timeout = d_connect_timeout;
		if(ra->prev) ra->prev->auto_rehash = b_autorehash;
#if (PHP_MAJOR_VERSION < 7)
        obj = (redis_array_object *)zend_objects_get_address(getThis() TSRMLS_CC);
#else
        obj = (redis_array_object *)((char *)Z_OBJ_P(getThis()) - XtOffsetOf(redis_array_object, std));
#endif
        obj->ra = ra;
	}
}

static void
ra_forward_call(INTERNAL_FUNCTION_PARAMETERS, RedisArray *ra, const char *cmd, int cmd_len, zval *z_args, zval *z_new_target) {

	zval z_fun, *redis_inst, *z_callargs, *zp_tmp;
	char *key = NULL; /* set to avoid "unused-but-set-variable" */
	int i, key_len = 0, argc;
	HashTable *h_args;
	zend_bool b_write_cmd = 0;

	h_args = Z_ARRVAL_P(z_args);
	argc = zend_hash_num_elements(h_args);

	if(ra->z_multi_exec) {
		redis_inst = ra->z_multi_exec; /* we already have the instance */
	} else {
		/* extract key and hash it. */
        if ((zp_tmp = zend_hash_index_find(h_args, 0)) == NULL || Z_TYPE_P(zp_tmp) != IS_STRING) {
			php_error_docref(NULL TSRMLS_CC, E_ERROR, "Could not find key");
			RETURN_FALSE;
        }
        key = Z_STRVAL_P(zp_tmp);
        key_len = Z_STRLEN_P(zp_tmp);

		/* find node */
		redis_inst = ra_find_node(ra, key, key_len, NULL TSRMLS_CC);
		if(!redis_inst) {
			php_error_docref(NULL TSRMLS_CC, E_ERROR, "Could not find any redis servers for this key.");
			RETURN_FALSE;
		}
	}

	/* pass call through */
	ZVAL_STRINGL(&z_fun, cmd, cmd_len); /* method name */
	z_callargs = ecalloc(argc + 1, sizeof(zval));

	/* copy args to array */
    i = 0;
    ZEND_HASH_FOREACH_VAL(h_args, zp_tmp) {
        z_callargs[i] = *zp_tmp;
        i++;
    } ZEND_HASH_FOREACH_END();

	/* multi/exec */
	if(ra->z_multi_exec) {
        call_user_function(&redis_ce->function_table, ra->z_multi_exec, &z_fun, return_value, argc, z_callargs);
        zval_dtor(return_value);
        zval_dtor(&z_fun);
		efree(z_callargs);
		RETURN_ZVAL(getThis(), 1, 0);
	}

    /* check if write cmd */
    b_write_cmd = ra_is_write_cmd(ra, cmd, cmd_len);

	/* CALL! */
	if(ra->index && b_write_cmd) {
        /* add MULTI + SADD */
        ra_index_multi(redis_inst, MULTI TSRMLS_CC);
		/* call using discarded temp value and extract exec results after. */
        call_user_function(&redis_ce->function_table, redis_inst, &z_fun, return_value, argc, z_callargs);
        zval_dtor(return_value);

		/* add keys to index. */
		ra_index_key(key, key_len, redis_inst TSRMLS_CC);

		/* call EXEC */
		ra_index_exec(redis_inst, return_value, 0 TSRMLS_CC);
	} else { /* call directly through. */
		call_user_function(&redis_ce->function_table, redis_inst, &z_fun, return_value, argc, z_callargs);

        if (!b_write_cmd) {
            /* check if we have an error. */
            if (ra->prev && RA_CALL_FAILED(return_value, cmd)) { /* there was an error reading, try with prev ring. */
                /* Free previous return value */
                zval_dtor(return_value);

                /* ERROR, FALLBACK TO PREVIOUS RING and forward a reference to the first redis instance we were looking at. */
                ra_forward_call(INTERNAL_FUNCTION_PARAM_PASSTHRU, ra->prev, cmd, cmd_len, z_args, z_new_target ? z_new_target : redis_inst);
            }

            /* Autorehash if the key was found on the previous node if this is a read command and auto rehashing is on */
            if (ra->auto_rehash && z_new_target && !RA_CALL_FAILED(return_value, cmd)) { /* move key from old ring to new ring */
                ra_move_key(key, key_len, redis_inst, z_new_target TSRMLS_CC);
            }
        }
	}

	/* cleanup */
    zval_dtor(&z_fun);
	efree(z_callargs);
}

PHP_METHOD(RedisArray, __call)
{
	zval *object;
	RedisArray *ra;
	zval *z_args;

	char *cmd;
	strlen_t cmd_len;

	if (zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), "Osa",
				&object, redis_array_ce, &cmd, &cmd_len, &z_args) == FAILURE) {
		RETURN_FALSE;
	}

	if ((ra = redis_array_get(object TSRMLS_CC)) == NULL) {
		RETURN_FALSE;
	}

	ra_forward_call(INTERNAL_FUNCTION_PARAM_PASSTHRU, ra, cmd, cmd_len, z_args, NULL);
}

PHP_METHOD(RedisArray, _hosts)
{
	zval *object;
	int i;
	RedisArray *ra;

	if (zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), "O",
				&object, redis_array_ce) == FAILURE) {
		RETURN_FALSE;
	}

	if ((ra = redis_array_get(object TSRMLS_CC)) == NULL) {
		RETURN_FALSE;
	}

	array_init(return_value);
	for(i = 0; i < ra->count; ++i) {
		add_next_index_string(return_value, ra->hosts[i]);
	}
}

PHP_METHOD(RedisArray, _target)
{
	zval *object;
	RedisArray *ra;
	char *key;
	strlen_t key_len;
	zval *redis_inst;
    int i;

	if (zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), "Os",
				&object, redis_array_ce, &key, &key_len) == FAILURE) {
		RETURN_FALSE;
	}

	if ((ra = redis_array_get(object TSRMLS_CC)) == NULL) {
		RETURN_FALSE;
	}

	redis_inst = ra_find_node(ra, key, key_len, &i TSRMLS_CC);
	if(redis_inst) {
        RETURN_STRING(ra->hosts[i]);
	} else {
		RETURN_NULL();
	}
}

PHP_METHOD(RedisArray, _instance)
{
	zval *object;
	RedisArray *ra;
	char *target;
	strlen_t target_len;
	zval *z_redis;

	if (zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), "Os",
				&object, redis_array_ce, &target, &target_len) == FAILURE) {
		RETURN_FALSE;
	}

	if ((ra = redis_array_get(object TSRMLS_CC)) == NULL) {
		RETURN_FALSE;
	}

	z_redis = ra_find_node_by_name(ra, target, target_len TSRMLS_CC);
	if(z_redis) {
		RETURN_ZVAL(z_redis, 1, 0);
	} else {
		RETURN_NULL();
	}
}

PHP_METHOD(RedisArray, _function)
{
	zval *object, *z_fun;
	RedisArray *ra;

	if (zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), "O",
				&object, redis_array_ce) == FAILURE) {
		RETURN_FALSE;
	}

	if ((ra = redis_array_get(object TSRMLS_CC)) == NULL) {
		RETURN_FALSE;
	}

    z_fun = &ra->z_fun;
    RETURN_ZVAL(z_fun, 1, 0);
}

PHP_METHOD(RedisArray, _distributor)
{
	zval *object, *z_dist;
	RedisArray *ra;

	if (zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), "O",
				&object, redis_array_ce) == FAILURE) {
		RETURN_FALSE;
	}

	if ((ra = redis_array_get(object TSRMLS_CC)) == NULL) {
		RETURN_FALSE;
	}

    z_dist = &ra->z_dist;
    RETURN_ZVAL(z_dist, 1, 0);
}

PHP_METHOD(RedisArray, _rehash)
{
	zval *object;
	RedisArray *ra;
	zend_fcall_info z_cb = {0};
	zend_fcall_info_cache z_cb_cache = {0};

	if (zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), "O|f",
				&object, redis_array_ce, &z_cb, &z_cb_cache) == FAILURE) {
		RETURN_FALSE;
	}

	if ((ra = redis_array_get(object TSRMLS_CC)) == NULL) {
		RETURN_FALSE;
	}

	if (ZEND_NUM_ARGS() == 0) {
		ra_rehash(ra, NULL, NULL TSRMLS_CC);
	} else {
		ra_rehash(ra, &z_cb, &z_cb_cache TSRMLS_CC);
	}
}

static void multihost_distribute(INTERNAL_FUNCTION_PARAMETERS, const char *method_name)
{
	zval *object, z_fun;
	int i;
	RedisArray *ra;

	if (zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), "O",
				&object, redis_array_ce) == FAILURE) {
		RETURN_FALSE;
	}

	if ((ra = redis_array_get(object TSRMLS_CC)) == NULL) {
		RETURN_FALSE;
	}

	/* prepare call */
	ZVAL_STRING(&z_fun, method_name);

	array_init(return_value);
	for(i = 0; i < ra->count; ++i) {
        zval zv, *z_tmp = &zv;
#if (PHP_MAJOR_VERSION < 7)
		MAKE_STD_ZVAL(z_tmp);
#endif

		/* Call each node in turn */
        call_user_function(&redis_ce->function_table, &ra->redis[i], &z_fun, z_tmp, 0, NULL);

		add_assoc_zval(return_value, ra->hosts[i], z_tmp);
	}
    zval_dtor(&z_fun);
}

PHP_METHOD(RedisArray, info)
{
	multihost_distribute(INTERNAL_FUNCTION_PARAM_PASSTHRU, "INFO");
}

PHP_METHOD(RedisArray, ping)
{
	multihost_distribute(INTERNAL_FUNCTION_PARAM_PASSTHRU, "PING");
}

PHP_METHOD(RedisArray, flushdb)
{
	multihost_distribute(INTERNAL_FUNCTION_PARAM_PASSTHRU, "FLUSHDB");
}

PHP_METHOD(RedisArray, flushall)
{
	multihost_distribute(INTERNAL_FUNCTION_PARAM_PASSTHRU, "FLUSHALL");
}

PHP_METHOD(RedisArray, save) 
{
    multihost_distribute(INTERNAL_FUNCTION_PARAM_PASSTHRU, "SAVE");
}

PHP_METHOD(RedisArray, bgsave)
{
    multihost_distribute(INTERNAL_FUNCTION_PARAM_PASSTHRU, "BGSAVE");
}


PHP_METHOD(RedisArray, keys)
{
	zval *object, z_args[1], z_fun;
	RedisArray *ra;
	char *pattern;
	strlen_t pattern_len;
    int i;

	/* Make sure the prototype is correct */
	if(zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), "Os",
								    &object, redis_array_ce, &pattern, &pattern_len) == FAILURE)
	{
		RETURN_FALSE;
	}

	/* Make sure we can grab our RedisArray object */
    if ((ra = redis_array_get(object TSRMLS_CC)) == NULL) {
		RETURN_FALSE;
	}

	/* Set up our function call (KEYS) */
	ZVAL_STRINGL(&z_fun, "KEYS", 4);

	/* We will be passing with one string argument (the pattern) */
	ZVAL_STRINGL(z_args, pattern, pattern_len);

	/* Init our array return */
	array_init(return_value);

	/* Iterate our RedisArray nodes */
	for(i=0; i<ra->count; ++i) {
        zval zv, *z_tmp = &zv;
#if (PHP_MAJOR_VERSION < 7)
		/* Return for this node */
		MAKE_STD_ZVAL(z_tmp);
#endif

		/* Call KEYS on each node */
        call_user_function(&redis_ce->function_table, &ra->redis[i], &z_fun, z_tmp, 1, z_args);

		/* Add the result for this host */
		add_assoc_zval(return_value, ra->hosts[i], z_tmp);
	}
    zval_dtor(&z_args[0]);
    zval_dtor(&z_fun);
}

PHP_METHOD(RedisArray, getOption)
{
	zval *object, z_fun, z_args[1];
	int i;
	RedisArray *ra;
	zend_long opt;

	if (zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), "Ol",
				&object, redis_array_ce, &opt) == FAILURE) {
		RETURN_FALSE;
	}

	if ((ra = redis_array_get(object TSRMLS_CC)) == NULL) {
		RETURN_FALSE;
	}

	/* prepare call */
	ZVAL_STRINGL(&z_fun, "getOption", 9);

	/* copy arg */
	ZVAL_LONG(&z_args[0], opt);

	array_init(return_value);
	for(i = 0; i < ra->count; ++i) {
        zval zv, *z_tmp = &zv;
#if (PHP_MAJOR_VERSION < 7)
		MAKE_STD_ZVAL(z_tmp);
#endif

		/* Call each node in turn */
        call_user_function(&redis_ce->function_table, &ra->redis[i], &z_fun, z_tmp, 1, z_args);

		add_assoc_zval(return_value, ra->hosts[i], z_tmp);
	}
    zval_dtor(&z_fun);
}

PHP_METHOD(RedisArray, setOption)
{
	zval *object, z_fun, z_args[2];
	int i;
	RedisArray *ra;
	zend_long opt;
	char *val_str;
	strlen_t val_len;

	if (zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), "Ols",
				&object, redis_array_ce, &opt, &val_str, &val_len) == FAILURE) {
		RETURN_FALSE;
	}

	if ((ra = redis_array_get(object TSRMLS_CC)) == NULL) {
		RETURN_FALSE;
	}

	/* prepare call */
	ZVAL_STRINGL(&z_fun, "setOption", 9);

	/* copy args */
	ZVAL_LONG(&z_args[0], opt);
	ZVAL_STRINGL(&z_args[1], val_str, val_len);

	array_init(return_value);
	for(i = 0; i < ra->count; ++i) {
        zval zv, *z_tmp = &zv;
#if (PHP_MAJOR_VERSION < 7)
		MAKE_STD_ZVAL(z_tmp);
#endif

		/* Call each node in turn */
        call_user_function(&redis_ce->function_table, &ra->redis[i], &z_fun, z_tmp, 2, z_args);

		add_assoc_zval(return_value, ra->hosts[i], z_tmp);
	}
    zval_dtor(&z_args[1]);
    zval_dtor(&z_fun);
}

PHP_METHOD(RedisArray, select)
{
    zval *object, z_fun, z_args[1];
	int i;
	RedisArray *ra;
	zend_long opt;

	if (zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), "Ol",
				&object, redis_array_ce, &opt) == FAILURE) {
		RETURN_FALSE;
	}

	if ((ra = redis_array_get(object TSRMLS_CC)) == NULL) {
		RETURN_FALSE;
	}

	/* prepare call */
	ZVAL_STRINGL(&z_fun, "select", 6);

	/* copy args */
    ZVAL_LONG(&z_args[0], opt);

	array_init(return_value);
	for(i = 0; i < ra->count; ++i) {
        zval zv, *z_tmp = &zv;
#if (PHP_MAJOR_VERSION < 7)
		MAKE_STD_ZVAL(z_tmp);
#endif

		/* Call each node in turn */
        call_user_function(&redis_ce->function_table, &ra->redis[i], &z_fun, z_tmp, 1, z_args);

		add_assoc_zval(return_value, ra->hosts[i], z_tmp);
	}
    zval_dtor(&z_fun);
}
#if (PHP_MAJOR_VERSION < 7)
#define HANDLE_MULTI_EXEC(ra, cmd) do { \
    if (ra && ra->z_multi_exec) { \
		int i, num_varargs;\
		zval ***varargs = NULL;\
		zval z_arg_array;\
		if (zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), "O*",\
								&object, redis_array_ce, &varargs, &num_varargs) == FAILURE) {\
				RETURN_FALSE;\
		}\
		/* copy all args into a zval hash table */\
		array_init(&z_arg_array);\
		for(i = 0; i < num_varargs; ++i) {\
			zval *z_tmp;\
			MAKE_STD_ZVAL(z_tmp);\
			*z_tmp = **varargs[i];\
			zval_copy_ctor(z_tmp);\
			INIT_PZVAL(z_tmp);\
			add_next_index_zval(&z_arg_array, z_tmp);\
		}\
		/* call */\
		ra_forward_call(INTERNAL_FUNCTION_PARAM_PASSTHRU, ra, cmd, sizeof(cmd)-1, &z_arg_array, NULL);\
		zval_dtor(&z_arg_array);\
		if(varargs) {\
			efree(varargs);\
		}\
		return;\
	}\
}while(0)
#else
#define HANDLE_MULTI_EXEC(ra, cmd) do { \
    if (ra && ra->z_multi_exec) { \
        int i, num_varargs; \
        zval *varargs = NULL, z_arg_array; \
        if (zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), "O*", \
                                            &object, redis_array_ce, &varargs, &num_varargs) == FAILURE) { \
            RETURN_FALSE;\
        } \
        /* copy all args into a zval hash table */\
        array_init(&z_arg_array); \
        for (i = 0; i < num_varargs; i++) { \
            zval z_tmp; \
            ZVAL_ZVAL(&z_tmp, &varargs[i], 1, 0); \
            add_next_index_zval(&z_arg_array, &z_tmp); \
        } \
        /* call */\
        ra_forward_call(INTERNAL_FUNCTION_PARAM_PASSTHRU, ra, cmd, sizeof(cmd) - 1, &z_arg_array, NULL); \
        zval_dtor(&z_arg_array); \
        return; \
    } \
} while(0)
#endif

/* MGET will distribute the call to several nodes and regroup the values. */
PHP_METHOD(RedisArray, mget)
{
	zval *object, *z_keys, z_argarray, *data, z_ret, *z_cur, z_tmp_array, *z_tmp;
	int i, j, n;
	RedisArray *ra;
	int *pos, argc, *argc_each;
	HashTable *h_keys;
	zval **argv;

    if ((ra = redis_array_get(getThis() TSRMLS_CC)) == NULL) {
        RETURN_FALSE;
    }

	/* Multi/exec support */
    HANDLE_MULTI_EXEC(ra, "MGET");

	if (zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), "Oa",
				&object, redis_array_ce, &z_keys) == FAILURE) {
		RETURN_FALSE;
	}


	/* init data structures */
	h_keys = Z_ARRVAL_P(z_keys);
	argc = zend_hash_num_elements(h_keys);
	argv = emalloc(argc * sizeof(zval*));
	pos = emalloc(argc * sizeof(int));

	argc_each = emalloc(ra->count * sizeof(int));
	memset(argc_each, 0, ra->count * sizeof(int));

	/* associate each key to a redis node */
    i = 0;
    ZEND_HASH_FOREACH_VAL(h_keys, data) {
	    /* If we need to represent a long key as a string */
	    unsigned int key_len;
	    char kbuf[40], *key_lookup;

	    /* phpredis proper can only use string or long keys, so restrict to that here */
	    if (Z_TYPE_P(data) != IS_STRING && Z_TYPE_P(data) != IS_LONG) {
	        php_error_docref(NULL TSRMLS_CC, E_ERROR, "MGET: all keys must be strings or longs");
	        efree(argv);
	        efree(pos);
	        efree(argc_each);
	        RETURN_FALSE;
	    }

	    /* Convert to a string for hash lookup if it isn't one */
	    if (Z_TYPE_P(data) == IS_STRING) {
	        key_len = Z_STRLEN_P(data);
            key_lookup = Z_STRVAL_P(data);
	    } else {
	        key_len = snprintf(kbuf, sizeof(kbuf), "%ld", Z_LVAL_P(data));
	        key_lookup = (char*)kbuf;
	    }

		/* Find our node */
        if (ra_find_node(ra, key_lookup, key_len, &pos[i] TSRMLS_CC) == NULL) {
            /* TODO: handle */
        }

		argc_each[pos[i]]++;	/* count number of keys per node */
		argv[i++] = data;
	} ZEND_HASH_FOREACH_END();

	array_init(&z_tmp_array);
	/* calls */
	for(n = 0; n < ra->count; ++n) { /* for each node */
	    /* We don't even need to make a call to this node if no keys go there */
	    if(!argc_each[n]) continue;

	    /* copy args for MGET call on node. */
		array_init(&z_argarray);

		for(i = 0; i < argc; ++i) {
			if(pos[i] != n) continue;

#if (PHP_MAJOR_VERSION < 7)
			MAKE_STD_ZVAL(z_tmp);
#else
            zval zv;
            z_tmp = &zv;
#endif
            ZVAL_ZVAL(z_tmp, argv[i], 1, 0);
			add_next_index_zval(&z_argarray, z_tmp);
		}

        zval z_fun;
        /* prepare call */
        ZVAL_STRINGL(&z_fun, "MGET", 4);
		/* call MGET on the node */
        call_user_function(&redis_ce->function_table, &ra->redis[n], &z_fun, &z_ret, 1, &z_argarray);
        zval_dtor(&z_fun);

		/* cleanup args array */
		zval_dtor(&z_argarray);

        /* Error out if we didn't get a proper response */
        if (Z_TYPE(z_ret) != IS_ARRAY) {
            /* cleanup */
            zval_dtor(&z_ret);
            zval_dtor(&z_tmp_array);
	        efree(argv);
            efree(pos);
            efree(argc_each);

            /* failure */
            RETURN_FALSE;
        }

		for(i = 0, j = 0; i < argc; ++i) {
		    if(pos[i] != n) continue;

			z_cur = zend_hash_index_find(Z_ARRVAL(z_ret), j++);

#if (PHP_MAJOR_VERSION < 7)
			MAKE_STD_ZVAL(z_tmp);
#else
            zval zv;
            z_tmp = &zv;
#endif
            ZVAL_ZVAL(z_tmp, z_cur, 1, 0);
			add_index_zval(&z_tmp_array, i, z_tmp);
		}
		zval_dtor(&z_ret);
	}

	array_init(return_value);
	/* copy temp array in the right order to return_value */
	for(i = 0; i < argc; ++i) {
		z_cur = zend_hash_index_find(Z_ARRVAL(z_tmp_array), i);

#if (PHP_MAJOR_VERSION < 7)
		MAKE_STD_ZVAL(z_tmp);
#else
        zval zv;
        z_tmp = &zv;
#endif
        ZVAL_ZVAL(z_tmp, z_cur, 1, 0);
		add_next_index_zval(return_value, z_tmp);
	}

	/* cleanup */
	zval_dtor(&z_tmp_array);
	efree(argv);
	efree(pos);
	efree(argc_each);
}


/* MSET will distribute the call to several nodes and regroup the values. */
PHP_METHOD(RedisArray, mset)
{
    zval *object, *z_keys, z_argarray, *data, z_ret, **argv;
    int i = 0, n;
    RedisArray *ra;
    int *pos, argc, *argc_each;
    HashTable *h_keys;
    char *key, **keys, kbuf[40];
    int key_len, *key_lens;
    zend_string *zkey;
    ulong idx;

    if ((ra = redis_array_get(getThis() TSRMLS_CC)) == NULL) {
        RETURN_FALSE;
    }

    /* Multi/exec support */
    HANDLE_MULTI_EXEC(ra, "MSET");

    if (zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), "Oa",
                                     &object, redis_array_ce, &z_keys) == FAILURE)
    {
        RETURN_FALSE;
    }

    /* init data structures */
    h_keys = Z_ARRVAL_P(z_keys);
    argc = zend_hash_num_elements(h_keys);
    argv = emalloc(argc * sizeof(zval*));
    pos = emalloc(argc * sizeof(int));
    keys = emalloc(argc * sizeof(char*));
    key_lens = emalloc(argc * sizeof(int));

    argc_each = emalloc(ra->count * sizeof(int));
    memset(argc_each, 0, ra->count * sizeof(int));

    /* associate each key to a redis node */
    ZEND_HASH_FOREACH_KEY_VAL(h_keys, idx, zkey, data) {
        /* If the key isn't a string, make a string representation of it */
        if (zkey) {
            key_len = zkey->len;
            key = zkey->val;
        } else {
            key_len = snprintf(kbuf, sizeof(kbuf), "%lu", idx);
            key = kbuf;
        }

        if (ra_find_node(ra, key, (int)key_len, &pos[i] TSRMLS_CC) == NULL) {
            // TODO: handle
        }

        argc_each[pos[i]]++;	/* count number of keys per node */
        keys[i] = estrndup(key, key_len);
        key_lens[i] = (int)key_len;
        argv[i] = data;
        i++;
    } ZEND_HASH_FOREACH_END();


    /* calls */
    for (n = 0; n < ra->count; ++n) { /* for each node */
        /* We don't even need to make a call to this node if no keys go there */
        if(!argc_each[n]) continue;

        int found = 0;

        /* copy args */
        array_init(&z_argarray);
        for(i = 0; i < argc; ++i) {
            if(pos[i] != n) continue;

            zval zv, *z_tmp = &zv;
#if (PHP_MAJOR_VERSION < 7)
            MAKE_STD_ZVAL(z_tmp);
#endif
            ZVAL_ZVAL(z_tmp, argv[i], 1, 0);
            add_assoc_zval_ex(&z_argarray, keys[i], key_lens[i], z_tmp);
            found++;
        }

        if(!found) {
            zval_dtor(&z_argarray);
            continue; /* don't run empty MSETs */
        }

        if(ra->index) { /* add MULTI */
            ra_index_multi(&ra->redis[n], MULTI TSRMLS_CC);
        }

        zval z_fun;

        /* prepare call */
        ZVAL_STRINGL(&z_fun, "MSET", 4);

        /* call */
        call_user_function(&redis_ce->function_table, &ra->redis[n], &z_fun, &z_ret, 1, &z_argarray);
        zval_dtor(&z_fun);
        zval_dtor(&z_ret);

        if(ra->index) {
            ra_index_keys(&z_argarray, &ra->redis[n] TSRMLS_CC); /* use SADD to add keys to node index */
            ra_index_exec(&ra->redis[n], NULL, 0 TSRMLS_CC); /* run EXEC */
        }

        zval_dtor(&z_argarray);
    }

    /* Free any keys that we needed to allocate memory for, because they weren't strings */
    for(i = 0; i < argc; i++) {
        efree(keys[i]);
    }

    /* cleanup */
    efree(keys);
    efree(key_lens);
    efree(argv);
    efree(pos);
    efree(argc_each);

    RETURN_TRUE;
}

/* DEL will distribute the call to several nodes and regroup the values. */
PHP_METHOD(RedisArray, del)
{
	zval *object, z_keys, z_fun, *data, z_ret, *z_tmp, *z_args;
	int i, n;
	RedisArray *ra;
	int *pos, argc = ZEND_NUM_ARGS(), *argc_each;
	HashTable *h_keys;
	zval **argv;
	long total = 0;
	int free_zkeys = 0;

    if ((ra = redis_array_get(getThis() TSRMLS_CC)) == NULL) {
        RETURN_FALSE;
    }
	/* Multi/exec support */
    HANDLE_MULTI_EXEC(ra, "DEL");

	/* get all args in z_args */
	z_args = emalloc(argc * sizeof(zval));
	if (zend_get_parameters_array(ht, argc, z_args) == FAILURE) {
		efree(z_args);
		RETURN_FALSE;
	}

	/* if single array arg, point z_keys to it. */
	if (argc == 1 && Z_TYPE(z_args[0]) == IS_ARRAY) {
		z_keys = z_args[0];
	} else {
		/* copy all elements to z_keys */
		array_init(&z_keys);
		for (i = 0; i < argc; ++i) {
            zval *z_arg = &z_args[i];
#if (PHP_MAJOR_VERSION < 7)
			MAKE_STD_ZVAL(z_tmp);
#else
            zval zv;
            z_tmp = &zv;
#endif
            ZVAL_ZVAL(z_tmp, z_arg, 1, 0);
			/* add copy to z_keys */
			add_next_index_zval(&z_keys, z_tmp);
		}
		free_zkeys = 1;
	}

	/* init data structures */
	h_keys = Z_ARRVAL(z_keys);
	argc = zend_hash_num_elements(h_keys);
	argv = emalloc(argc * sizeof(zval*));
	pos = emalloc(argc * sizeof(int));

	argc_each = emalloc(ra->count * sizeof(int));
	memset(argc_each, 0, ra->count * sizeof(int));

	/* associate each key to a redis node */
    i = 0;
    ZEND_HASH_FOREACH_VAL(h_keys, data) {
		if (Z_TYPE_P(data) != IS_STRING) {
			php_error_docref(NULL TSRMLS_CC, E_ERROR, "DEL: all keys must be string.");
            if (free_zkeys) zval_dtor(&z_keys);
            efree(z_args);
            efree(argv);
			efree(pos);
			efree(argc_each);
			RETURN_FALSE;
		}

        if (ra_find_node(ra, Z_STRVAL_P(data), Z_STRLEN_P(data), &pos[i] TSRMLS_CC) == NULL) {
            // TODO: handle
        }
		argc_each[pos[i]]++;	/* count number of keys per node */
		argv[i++] = data;
	} ZEND_HASH_FOREACH_END();

	/* prepare call */
	ZVAL_STRINGL(&z_fun, "DEL", 3);

	/* calls */
	for(n = 0; n < ra->count; ++n) { /* for each node */
	    /* We don't even need to make a call to this node if no keys go there */
	    if(!argc_each[n]) continue;

		int found = 0;
        zval z_argarray;

		/* copy args */
		array_init(&z_argarray);
		for(i = 0; i < argc; ++i) {
			if(pos[i] != n) continue;

#if (PHP_MAJOR_VERSION < 7)
			MAKE_STD_ZVAL(z_tmp);
#else
            zval zv;
            z_tmp = &zv;
#endif
            ZVAL_ZVAL(z_tmp, argv[i], 1, 0);
			add_next_index_zval(&z_argarray, z_tmp);
			found++;
		}

		if(!found) {	/* don't run empty DELs */
			zval_dtor(&z_argarray);
			continue;
		}

		if(ra->index) { /* add MULTI */
			ra_index_multi(&ra->redis[n], MULTI TSRMLS_CC);
		}

		/* call */
        call_user_function(&redis_ce->function_table, &ra->redis[n], &z_fun, &z_ret, 1, &z_argarray);

		if(ra->index) {
            zval_dtor(&z_ret);
			ra_index_del(&z_argarray, &ra->redis[n] TSRMLS_CC); /* use SREM to remove keys from node index */
			ra_index_exec(&ra->redis[n], &z_ret, 0 TSRMLS_CC); /* run EXEC */
		}
		total += Z_LVAL(z_ret);	/* increment total */

		zval_dtor(&z_argarray);
		zval_dtor(&z_ret);
	}

	/* cleanup */
	zval_dtor(&z_fun);
	efree(argv);
	efree(pos);
	efree(argc_each);

	if(free_zkeys) {
		zval_dtor(&z_keys);
	}

	efree(z_args);
	RETURN_LONG(total);
}

PHP_METHOD(RedisArray, multi)
{
	zval *object;
	RedisArray *ra;
	zval *z_redis;
	char *host;
	strlen_t host_len;
	zend_long multi_value = MULTI;

	if (zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), "Os|l",
				&object, redis_array_ce, &host, &host_len, &multi_value) == FAILURE) {
		RETURN_FALSE;
	}

	if ((ra = redis_array_get(object TSRMLS_CC)) == NULL) {
		RETURN_FALSE;
	}

	/* find node */
	z_redis = ra_find_node_by_name(ra, host, host_len TSRMLS_CC);
	if(!z_redis) {
		RETURN_FALSE;
	}

	if(multi_value != MULTI && multi_value != PIPELINE) {
		RETURN_FALSE;
	}

	/* save multi object */
	ra->z_multi_exec = z_redis;

	/* switch redis instance to multi/exec mode. */
	ra_index_multi(z_redis, multi_value TSRMLS_CC);

	/* return this. */
	RETURN_ZVAL(object, 1, 0);
}

PHP_METHOD(RedisArray, exec)
{
	zval *object;
	RedisArray *ra;

	if (zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), "O",
				&object, redis_array_ce) == FAILURE) {
		RETURN_FALSE;
	}

	if ((ra = redis_array_get(object TSRMLS_CC)) == NULL || !ra->z_multi_exec) {
		RETURN_FALSE;
	}

	/* switch redis instance out of multi/exec mode. */
	ra_index_exec(ra->z_multi_exec, return_value, 1 TSRMLS_CC);

	/* remove multi object */
	ra->z_multi_exec = NULL;
}

PHP_METHOD(RedisArray, discard)
{
	zval *object;
	RedisArray *ra;

	if (zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), "O",
				&object, redis_array_ce) == FAILURE) {
		RETURN_FALSE;
	}

	if ((ra = redis_array_get(object TSRMLS_CC)) == NULL || !ra->z_multi_exec) {
		RETURN_FALSE;
	}

	/* switch redis instance out of multi/exec mode. */
	ra_index_discard(ra->z_multi_exec, return_value TSRMLS_CC);

	/* remove multi object */
	ra->z_multi_exec = NULL;
}

PHP_METHOD(RedisArray, unwatch)
{
	zval *object;
	RedisArray *ra;

	if (zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), "O",
				&object, redis_array_ce) == FAILURE) {
		RETURN_FALSE;
	}

	if ((ra = redis_array_get(object TSRMLS_CC)) == NULL || !ra->z_multi_exec) {
		RETURN_FALSE;
	}

	/* unwatch keys, stay in multi/exec mode. */
	ra_index_unwatch(ra->z_multi_exec, return_value TSRMLS_CC);
}
