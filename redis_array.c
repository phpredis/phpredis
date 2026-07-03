/*
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
#include "library.h"
#include "redis_array.h"
#include "redis_array_impl.h"

#include <ext/standard/info.h>
#include <zend_exceptions.h>

/* Simple helper to detect failure in a RedisArray call */
static zend_always_inline zend_bool
ra_call_failed(zval *rv, const char *cmd)
{
    return Z_TYPE_P(rv) == IS_FALSE ||
           (Z_TYPE_P(rv) == IS_ARRAY && zend_hash_num_elements(Z_ARRVAL_P(rv)) == 0) ||
           (Z_TYPE_P(rv) == IS_LONG && Z_LVAL_P(rv) == 0 && !strcasecmp(cmd, "TYPE"));
}

extern zend_class_entry *redis_ce;
zend_class_entry *redis_array_ce;

zend_object_handlers redis_array_object_handlers;

typedef struct {
    RedisArray *ra;
    zend_object std;
} redis_array_object;

#if PHP_VERSION_ID < 80000
#include "redis_array_legacy_arginfo.h"
#else
#include "zend_attributes.h"
#include "redis_array_arginfo.h"
#endif

static void redis_array_free(RedisArray *ra) {
    int i;

    /* continuum */
    if (ra->continuum) {
        efree(ra->continuum->points);
        efree(ra->continuum);
    }

    /* Redis objects */
    for(i = 0; i< ra->count; i++) {
        zval_ptr_dtor_nogc(&ra->redis[i]);
        zend_string_release(ra->hosts[i]);
    }
    efree(ra->redis);
    efree(ra->hosts);

    /* delete hash function */
    zval_ptr_dtor_nogc(&ra->z_fun);

    /* Distributor */
    zval_ptr_dtor_nogc(&ra->z_dist);

    /* Hashing algorithm */
    if (ra->algorithm) zend_string_release(ra->algorithm);

    /* Delete pur commands */
    zend_hash_destroy(ra->pure_cmds);
    FREE_HASHTABLE(ra->pure_cmds);

    /* Free structure itself */
    efree(ra);
}


void
free_redis_array_object(zend_object *object)
{
    redis_array_object *obj = PHPREDIS_GET_OBJECT(redis_array_object, object);

    if (obj->ra) {
        if (obj->ra->prev) redis_array_free(obj->ra->prev);
        redis_array_free(obj->ra);
    }
    zend_object_std_dtor(&obj->std);
}

static void
redis_array_init_object_handlers(void)
{
    memcpy(&redis_array_object_handlers, zend_get_std_object_handlers(),
           sizeof(redis_array_object_handlers));
    redis_array_object_handlers.offset = offsetof(redis_array_object, std);
    redis_array_object_handlers.free_obj = free_redis_array_object;
    redis_array_object_handlers.clone_obj = NULL;
}

zend_object *
create_redis_array_object(zend_class_entry *ce)
{
    redis_array_object *obj = ecalloc(1, sizeof(redis_array_object) + zend_object_properties_size(ce));

    obj->ra = NULL;

    zend_object_std_init(&obj->std, ce);
    object_properties_init(&obj->std, ce);

    obj->std.handlers = &redis_array_object_handlers;

    return &obj->std;
}


PHP_MINIT_FUNCTION(redis_array)
{
    /* RedisArray class */
    redis_array_ce = register_class_RedisArray();
    redis_array_ce->create_object = create_redis_array_object;

    /* RedisArray object handler initialization */
    redis_array_init_object_handlers();

    return SUCCESS;
}

/**
 * redis_array_get
 */
PHP_REDIS_API RedisArray *
redis_array_get(zval *id)
{
    redis_array_object *obj;

    if (Z_TYPE_P(id) == IS_OBJECT) {
        obj = PHPREDIS_ZVAL_GET_OBJECT(redis_array_object, id);
        return obj->ra;
    }
    return NULL;
}

/* {{{ proto RedisArray RedisArray::__construct()
    Public constructor */
PHP_METHOD(RedisArray, __construct)
{
    zval *z0, z_fun, z_dist, *zpData, *z_opts = NULL;
    RedisArray *ra = NULL;
    zend_bool b_index = 0, b_autorehash = 0, b_pconnect = 0, consistent = 0;
    HashTable *hPrev = NULL, *hOpts = NULL;
    zend_long l_retry_interval = 0;
      zend_bool b_lazy_connect = 0;
    double d_connect_timeout = 0, read_timeout = 0.0;
    zend_string *algorithm = NULL, *user = NULL, *pass = NULL;
    redis_array_object *obj;

    ZEND_PARSE_PARAMETERS_START(1, 2)
        Z_PARAM_ZVAL(z0)
        Z_PARAM_OPTIONAL
        Z_PARAM_ARRAY(z_opts)
    ZEND_PARSE_PARAMETERS_END_EX(RETURN_FALSE);

    /* Bail if z0 isn't a string or an array.
     * Note:  WRONG_PARAM_COUNT seems wrong but this is what we have been doing
     *        for ages so we can't really change it until the next major version.
     */
    if (Z_TYPE_P(z0) != IS_ARRAY && Z_TYPE_P(z0) != IS_STRING) {
#if PHP_VERSION_ID < 80000
        WRONG_PARAM_COUNT;
#else
        zend_argument_type_error(1, "must be of type string|array, %s given", zend_zval_type_name(z0));
        RETURN_THROWS();
#endif
    }

    /* If it's a string we want to load the array from ini information */
    if (Z_TYPE_P(z0) == IS_STRING) {
        ra = ra_load_array(Z_STRVAL_P(z0));
        goto finish;
    }

    ZVAL_NULL(&z_fun);
    ZVAL_NULL(&z_dist);

    /* extract options */
    if(z_opts) {
        hOpts = Z_ARRVAL_P(z_opts);

        /* extract previous ring. */
        zpData = REDIS_HASH_STR_FIND_STATIC(hOpts, "previous");
        if (zpData && Z_TYPE_P(zpData) == IS_ARRAY && zend_hash_num_elements(Z_ARRVAL_P(zpData)) > 0) {
            hPrev = Z_ARRVAL_P(zpData);
        }


        REDIS_CONF_AUTH_STATIC(hOpts, "auth", &user, &pass);
        REDIS_CONF_ZVAL_STATIC(hOpts, "function", &z_fun, 1, 0);
        REDIS_CONF_ZVAL_STATIC(hOpts, "distributor", &z_dist, 1, 0);
        REDIS_CONF_STRING_STATIC(hOpts, "algorithm", &algorithm);
        REDIS_CONF_ZEND_BOOL_STATIC(hOpts, "index", &b_index);
        REDIS_CONF_ZEND_BOOL_STATIC(hOpts, "autorehash", &b_autorehash);
        REDIS_CONF_ZEND_BOOL_STATIC(hOpts, "pconnect", &b_pconnect);
        REDIS_CONF_LONG_STATIC(hOpts, "retry_interval", &l_retry_interval);
        REDIS_CONF_ZEND_BOOL_STATIC(hOpts, "lazy_connect", &b_lazy_connect);
        REDIS_CONF_ZEND_BOOL_STATIC(hOpts, "consistent", &consistent);
        REDIS_CONF_DOUBLE_STATIC(hOpts, "connect_timeout", &d_connect_timeout);
        REDIS_CONF_DOUBLE_STATIC(hOpts, "read_timeout", &read_timeout);
    }

    ra = ra_make_array(Z_ARRVAL_P(z0), &z_fun, &z_dist, hPrev, b_index,
                       b_pconnect, l_retry_interval, b_lazy_connect,
                       d_connect_timeout, read_timeout, consistent,
                       algorithm, user, pass);

    if (algorithm) zend_string_release(algorithm);
    if (user) zend_string_release(user);
    if (pass) zend_string_release(pass);
    zval_ptr_dtor_nogc(&z_dist);
    zval_ptr_dtor_nogc(&z_fun);

finish:

    if(ra) {
        ra->auto_rehash = b_autorehash;
        ra->connect_timeout = d_connect_timeout;
        if(ra->prev) ra->prev->auto_rehash = b_autorehash;
        obj = PHPREDIS_ZVAL_GET_OBJECT(redis_array_object, getThis());
        obj->ra = ra;
    }
}

static void
ra_forward_call(INTERNAL_FUNCTION_PARAMETERS, RedisArray *ra, const char *cmd,
                int cmd_len, zval *z_args, zval *z_new_target)
{

    zval z_fun, *redis_inst, *z_callargs, *zp_tmp;
    char *key = NULL; /* set to avoid "unused-but-set-variable" */
    int i, key_len = 0, argc;
    HashTable *h_args;
    zend_bool b_write_cmd = 0;

    h_args = Z_ARRVAL_P(z_args);
    if ((argc = zend_hash_num_elements(h_args)) == 0) {
        RETURN_FALSE;
    }

    if(ra->z_multi_exec) {
        redis_inst = ra->z_multi_exec; /* we already have the instance */
    } else {
        /* extract key and hash it. */
        if ((zp_tmp = zend_hash_index_find(h_args, 0)) == NULL || Z_TYPE_P(zp_tmp) != IS_STRING) {
            php_error_docref(NULL, E_ERROR, "Could not find key");
            RETURN_FALSE;
        }
        key = Z_STRVAL_P(zp_tmp);
        key_len = Z_STRLEN_P(zp_tmp);

        /* find node */
        redis_inst = ra_find_node(ra, key, key_len, NULL);
        if(!redis_inst) {
            php_error_docref(NULL, E_ERROR, "Could not find any redis servers for this key.");
            RETURN_FALSE;
        }
    }

    /* pass call through */
    ZVAL_STRINGL(&z_fun, cmd, cmd_len); /* method name */
    z_callargs = ecalloc(argc, sizeof(*z_callargs));

    /* copy args to array */
    i = 0;
    ZEND_HASH_FOREACH_VAL(h_args, zp_tmp) {
        ZVAL_ZVAL(&z_callargs[i], zp_tmp, 1, 0);
        i++;
    } ZEND_HASH_FOREACH_END();

    /* multi/exec */
    if(ra->z_multi_exec) {
        call_user_function(&redis_ce->function_table, ra->z_multi_exec, &z_fun, return_value, argc, z_callargs);
        zval_ptr_dtor_nogc(return_value);
        zval_ptr_dtor_nogc(&z_fun);
        for (i = 0; i < argc; ++i) {
            zval_ptr_dtor_nogc(&z_callargs[i]);
        }
        efree(z_callargs);
        RETURN_ZVAL(getThis(), 1, 0);
    }

    /* check if write cmd */
    b_write_cmd = ra_is_write_cmd(ra, cmd, cmd_len);

    /* CALL! */
    if(ra->index && b_write_cmd) {
        /* add MULTI + SADD */
        ra_index_multi(redis_inst, MULTI);
        /* call using discarded temp value and extract exec results after. */
        call_user_function(&redis_ce->function_table, redis_inst, &z_fun, return_value, argc, z_callargs);
        zval_ptr_dtor_nogc(return_value);

        /* add keys to index. */
        ra_index_key(key, key_len, redis_inst);

        /* call EXEC */
        ra_index_exec(redis_inst, return_value, 0);
    } else { /* call directly through. */
        call_user_function(&redis_ce->function_table, redis_inst, &z_fun, return_value, argc, z_callargs);

        if (!b_write_cmd) {
            /* check if we have an error. */
            if (ra->prev && ra_call_failed(return_value, cmd)) { /* there was an error reading, try with prev ring. */
                /* Free previous return value */
                zval_ptr_dtor_nogc(return_value);

                /* ERROR, FALLBACK TO PREVIOUS RING and forward a reference to the first redis instance we were looking at. */
                ra_forward_call(INTERNAL_FUNCTION_PARAM_PASSTHRU, ra->prev, cmd, cmd_len, z_args, z_new_target ? z_new_target : redis_inst);
            }

            /* Autorehash if the key was found on the previous node if this is a read command and auto rehashing is on */
            if (ra->auto_rehash && z_new_target && !ra_call_failed(return_value, cmd)) { /* move key from old ring to new ring */
                ra_move_key(key, key_len, redis_inst, z_new_target);
            }
        }
    }

    /* cleanup */
    zval_ptr_dtor_nogc(&z_fun);
    for (i = 0; i < argc; ++i) {
        zval_ptr_dtor_nogc(&z_callargs[i]);
    }
    efree(z_callargs);
}

PHP_METHOD(RedisArray, __call)
{
    RedisArray *ra;
    zval *z_args;

    char *cmd;
    size_t cmd_len;

    ZEND_PARSE_PARAMETERS_START(2, 2)
        Z_PARAM_STRING(cmd, cmd_len)
        Z_PARAM_ARRAY(z_args)
    ZEND_PARSE_PARAMETERS_END_EX(RETURN_FALSE);

    if ((ra = redis_array_get(getThis())) == NULL) {
        RETURN_FALSE;
    }

    ra_forward_call(INTERNAL_FUNCTION_PARAM_PASSTHRU, ra, cmd, cmd_len, z_args, NULL);
}

PHP_METHOD(RedisArray, _hosts)
{
    int i;
    RedisArray *ra;

    if ((ra = redis_array_get(getThis())) == NULL) {
        RETURN_FALSE;
    }

    array_init(return_value);
    for(i = 0; i < ra->count; ++i) {
        add_next_index_stringl(return_value, ZSTR_VAL(ra->hosts[i]), ZSTR_LEN(ra->hosts[i]));
    }
}

PHP_METHOD(RedisArray, _target)
{
    RedisArray *ra;
    char *key;
    size_t key_len;
    zval *redis_inst;
    int i;

    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_STRING(key, key_len)
    ZEND_PARSE_PARAMETERS_END_EX(RETURN_FALSE);

    if ((ra = redis_array_get(getThis())) == NULL) {
        RETURN_FALSE;
    }

    redis_inst = ra_find_node(ra, key, key_len, &i);
    if(redis_inst) {
        RETURN_STRINGL(ZSTR_VAL(ra->hosts[i]), ZSTR_LEN(ra->hosts[i]));
    } else {
        RETURN_NULL();
    }
}

PHP_METHOD(RedisArray, _instance)
{
    RedisArray *ra;
    zend_string *host;
    zval *z_redis;

    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_STR(host)
    ZEND_PARSE_PARAMETERS_END_EX(RETURN_FALSE);

    if ((ra = redis_array_get(getThis())) == NULL) {
        RETURN_FALSE;
    }

    if ((z_redis = ra_find_node_by_name(ra, host)) == NULL) {
        RETURN_NULL();
    }
    RETURN_ZVAL(z_redis, 1, 0);
}

PHP_METHOD(RedisArray, _function)
{
    RedisArray *ra;

    if ((ra = redis_array_get(getThis())) == NULL) {
        RETURN_FALSE;
    }

    RETURN_ZVAL(&ra->z_fun, 1, 0);
}

PHP_METHOD(RedisArray, _distributor)
{
    RedisArray *ra;

    if ((ra = redis_array_get(getThis())) == NULL) {
        RETURN_FALSE;
    }

    RETURN_ZVAL(&ra->z_dist, 1, 0);
}

PHP_METHOD(RedisArray, _rehash)
{
    RedisArray *ra;
    zend_fcall_info z_cb = {0};
    zend_fcall_info_cache z_cb_cache = {0};

    ZEND_PARSE_PARAMETERS_START(0, 1)
        Z_PARAM_OPTIONAL
        Z_PARAM_FUNC(z_cb, z_cb_cache)
    ZEND_PARSE_PARAMETERS_END_EX(RETURN_FALSE);

    if ((ra = redis_array_get(getThis())) == NULL) {
        RETURN_FALSE;
    }

    if (ZEND_NUM_ARGS() == 0) {
        ra_rehash(ra, NULL, NULL);
    } else {
        ra_rehash(ra, &z_cb, &z_cb_cache);
    }
}

PHP_METHOD(RedisArray, _continuum)
{
    int i;
    zval z_ret;
    RedisArray *ra;

    if ((ra = redis_array_get(getThis())) == NULL) {
        RETURN_FALSE;
    }

    array_init(return_value);
    if (ra->continuum) {
        for (i = 0; i < ra->continuum->nb_points; ++i) {
            array_init(&z_ret);
            add_assoc_long(&z_ret, "index", ra->continuum->points[i].index);
            add_assoc_long(&z_ret, "value", ra->continuum->points[i].value);
            add_next_index_zval(return_value, &z_ret);
        }
    }
}


static void
multihost_distribute_call(RedisArray *ra, zval *return_value, zval *z_fun, int argc, zval *argv)
{
    zval z_tmp;
    int i;

    /* Init our array return */
    array_init(return_value);

    /* Iterate our RedisArray nodes */
    for (i = 0; i < ra->count; ++i) {
        /* Call each node in turn */
        call_user_function(&redis_array_ce->function_table, &ra->redis[i], z_fun, &z_tmp, argc, argv);

        /* Add the result for this host */
        add_assoc_zval_ex(return_value, ZSTR_VAL(ra->hosts[i]), ZSTR_LEN(ra->hosts[i]), &z_tmp);
    }
}

static void
multihost_distribute(INTERNAL_FUNCTION_PARAMETERS, const char *method_name)
{
    zval z_fun;
    RedisArray *ra;

    if ((ra = redis_array_get(getThis())) == NULL) {
        RETURN_FALSE;
    }

    /* prepare call */
    ZVAL_STRING(&z_fun, method_name);

    multihost_distribute_call(ra, return_value, &z_fun, 0, NULL);

    zval_ptr_dtor_nogc(&z_fun);
}

static void
multihost_distribute_flush(INTERNAL_FUNCTION_PARAMETERS, const char *method_name)
{
    zval z_fun, z_args[1];
    zend_bool async = 0;
    RedisArray *ra;

    ZEND_PARSE_PARAMETERS_START(0, 1)
        Z_PARAM_OPTIONAL
        Z_PARAM_BOOL(async)
    ZEND_PARSE_PARAMETERS_END_EX(RETURN_FALSE);

    if ((ra = redis_array_get(getThis())) == NULL) {
        RETURN_FALSE;
    }

    /* prepare call */
    ZVAL_STRING(&z_fun, method_name);
    ZVAL_BOOL(&z_args[0], async);

    multihost_distribute_call(ra, return_value, &z_fun, 1, z_args);

    zval_ptr_dtor_nogc(&z_fun);
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
    multihost_distribute_flush(INTERNAL_FUNCTION_PARAM_PASSTHRU, "FLUSHDB");
}

PHP_METHOD(RedisArray, flushall)
{
    multihost_distribute_flush(INTERNAL_FUNCTION_PARAM_PASSTHRU, "FLUSHALL");
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
    zval z_fun, z_args[1];
    RedisArray *ra;
    char *pattern;
    size_t pattern_len;

    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_STRING(pattern, pattern_len)
    ZEND_PARSE_PARAMETERS_END_EX(RETURN_FALSE);

    /* Make sure we can grab our RedisArray object */
    if ((ra = redis_array_get(getThis())) == NULL) {
        RETURN_FALSE;
    }

    /* Set up our function call (KEYS) */
    ZVAL_STRINGL(&z_fun, "KEYS", 4);

    /* We will be passing with one string argument (the pattern) */
    ZVAL_STRINGL(z_args, pattern, pattern_len);

    multihost_distribute_call(ra, return_value, &z_fun, 1, z_args);

    zval_ptr_dtor_nogc(&z_args[0]);
    zval_ptr_dtor_nogc(&z_fun);
}

PHP_METHOD(RedisArray, getOption)
{
    zval z_fun, z_args[1];
    RedisArray *ra;
    zend_long opt;

    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_LONG(opt)
    ZEND_PARSE_PARAMETERS_END_EX(RETURN_FALSE);

    if ((ra = redis_array_get(getThis())) == NULL) {
        RETURN_FALSE;
    }

    /* prepare call */
    ZVAL_STRINGL(&z_fun, "getOption", 9);

    /* copy arg */
    ZVAL_LONG(&z_args[0], opt);

    multihost_distribute_call(ra, return_value, &z_fun, 1, z_args);

    zval_ptr_dtor_nogc(&z_fun);
}

PHP_METHOD(RedisArray, setOption)
{
    zval z_fun, z_args[2];
    RedisArray *ra;
    zend_long opt;
    char *val_str;
    size_t val_len;

    ZEND_PARSE_PARAMETERS_START(2, 2)
        Z_PARAM_LONG(opt)
        Z_PARAM_STRING(val_str, val_len)
    ZEND_PARSE_PARAMETERS_END_EX(RETURN_FALSE);

    if ((ra = redis_array_get(getThis())) == NULL) {
        RETURN_FALSE;
    }

    /* prepare call */
    ZVAL_STRINGL(&z_fun, "setOption", 9);

    /* copy args */
    ZVAL_LONG(&z_args[0], opt);
    ZVAL_STRINGL(&z_args[1], val_str, val_len);

    multihost_distribute_call(ra, return_value, &z_fun, 2, z_args);

    zval_ptr_dtor_nogc(&z_args[1]);
    zval_ptr_dtor_nogc(&z_fun);
}

PHP_METHOD(RedisArray, select)
{
    zval z_fun, z_args[1];
    RedisArray *ra;
    zend_long opt;

    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_LONG(opt)
    ZEND_PARSE_PARAMETERS_END_EX(RETURN_FALSE);

    if ((ra = redis_array_get(getThis())) == NULL) {
        RETURN_FALSE;
    }

    /* prepare call */
    ZVAL_STRINGL(&z_fun, "select", 6);

    /* copy args */
    ZVAL_LONG(&z_args[0], opt);

    multihost_distribute_call(ra, return_value, &z_fun, 1, z_args);

    zval_ptr_dtor_nogc(&z_fun);
}

#define HANDLE_MULTI_EXEC(ra, cmd, cmdlen) do { \
    if (ra && ra->z_multi_exec) { \
        int i, num_varargs; \
        zval *varargs = NULL, z_arg_array; \
        ZEND_PARSE_PARAMETERS_START(0, -1) \
            Z_PARAM_VARIADIC('*', varargs, num_varargs) \
        ZEND_PARSE_PARAMETERS_END_EX(RETURN_FALSE); \
        /* copy all args into a zval hash table */\
        array_init(&z_arg_array); \
        for (i = 0; i < num_varargs; i++) { \
            zval z_tmp; \
            ZVAL_ZVAL(&z_tmp, &varargs[i], 1, 0); \
            add_next_index_zval(&z_arg_array, &z_tmp); \
        } \
        /* call */\
        ra_forward_call(INTERNAL_FUNCTION_PARAM_PASSTHRU, ra, cmd, cmdlen, &z_arg_array, NULL); \
        zval_ptr_dtor_nogc(&z_arg_array); \
        return; \
    } \
} while(0)

/* MGET will distribute the call to several nodes and regroup the values. */
PHP_METHOD(RedisArray, mget)
{
    zval *z_keys, *data, z_ret, *z_cur, z_tmp_array, z_fun, z_arg, **argv;
    int i, j, n, *pos, argc, *argc_each;
    HashTable *h_keys;
    RedisArray *ra;

    if ((ra = redis_array_get(getThis())) == NULL) {
        RETURN_FALSE;
    }

    /* Multi/exec support */
    HANDLE_MULTI_EXEC(ra, "MGET", sizeof("MGET") - 1);

    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_ARRAY(z_keys)
    ZEND_PARSE_PARAMETERS_END_EX(RETURN_FALSE);

    /* init data structures */
    h_keys = Z_ARRVAL_P(z_keys);
    if ((argc = zend_hash_num_elements(h_keys)) == 0) {
        RETURN_FALSE;
    }
    argv = ecalloc(argc, sizeof(*argv));
    pos = ecalloc(argc, sizeof(*pos));

    argc_each = ecalloc(ra->count, sizeof(*argc_each));

    /* associate each key to a redis node */
    i = 0;
    ZEND_HASH_FOREACH_VAL(h_keys, data) {
        /* If we need to represent a long key as a string */
        unsigned int key_len;
        char kbuf[40], *key_lookup;

        /* Handle the possibility that we're a reference */
        ZVAL_DEREF(data);

        /* Convert to a string for hash lookup if it isn't one */
        if (Z_TYPE_P(data) == IS_STRING) {
            key_len = Z_STRLEN_P(data);
            key_lookup = Z_STRVAL_P(data);
        } else if (Z_TYPE_P(data) == IS_LONG) {
            key_len = snprintf(kbuf, sizeof(kbuf), ZEND_LONG_FMT, Z_LVAL_P(data));
            key_lookup = (char*)kbuf;
        } else {
            /* phpredis proper can only use string or long keys, so restrict to that here */
            php_error_docref(NULL, E_ERROR, "MGET: all keys must be strings or longs");
            RETVAL_FALSE;
            goto cleanup;
        }

        /* Find our node */
        if (ra_find_node(ra, key_lookup, key_len, &pos[i]) == NULL) {
            RETVAL_FALSE;
            goto cleanup;
        }

        argc_each[pos[i]]++;    /* count number of keys per node */
        argv[i++] = data;
    } ZEND_HASH_FOREACH_END();

    /* prepare call */
    array_init(&z_tmp_array);
    ZVAL_STRINGL(&z_fun, "MGET", sizeof("MGET") - 1);

    /* calls */
    for(n = 0; n < ra->count; ++n) { /* for each node */
        /* We don't even need to make a call to this node if no keys go there */
        if(!argc_each[n]) continue;

        /* copy args for MGET call on node. */
        array_init(&z_arg);

        for(i = 0; i < argc; ++i) {
            if (pos[i] == n) {
                ZVAL_ZVAL(&z_ret, argv[i], 1, 0);
                add_next_index_zval(&z_arg, &z_ret);
            }
        }

        /* call MGET on the node */
        call_user_function(&redis_ce->function_table, &ra->redis[n], &z_fun, &z_ret, 1, &z_arg);

        /* cleanup args array */
        zval_ptr_dtor_nogc(&z_arg);

        /* Error out if we didn't get a proper response */
        if (Z_TYPE(z_ret) != IS_ARRAY) {
            /* cleanup */
            zval_ptr_dtor_nogc(&z_ret);
            zval_ptr_dtor_nogc(&z_tmp_array);
            RETVAL_FALSE;
            goto cleanup;
        }

        for(i = 0, j = 0; i < argc; ++i) {
            if (pos[i] != n || (z_cur = zend_hash_index_find(Z_ARRVAL(z_ret), j++)) == NULL) continue;

            ZVAL_ZVAL(&z_arg, z_cur, 1, 0);
            add_index_zval(&z_tmp_array, i, &z_arg);
        }
        zval_ptr_dtor_nogc(&z_ret);
    }

    zval_ptr_dtor_nogc(&z_fun);

    array_init(return_value);
    /* copy temp array in the right order to return_value */
    for(i = 0; i < argc; ++i) {
        if ((z_cur = zend_hash_index_find(Z_ARRVAL(z_tmp_array), i)) == NULL) continue;

        ZVAL_ZVAL(&z_arg, z_cur, 1, 0);
        add_next_index_zval(return_value, &z_arg);
    }

    /* cleanup */
    zval_ptr_dtor_nogc(&z_tmp_array);
cleanup:
    efree(argv);
    efree(pos);
    efree(argc_each);
}


/* MSET will distribute the call to several nodes and regroup the values. */
PHP_METHOD(RedisArray, mset)
{
    zval *z_keys, z_argarray, *data, z_fun, z_ret, **argv;
    int i = 0, n, *pos, argc, *argc_each, key_len;
    RedisArray *ra;
    HashTable *h_keys;
    char *key, kbuf[40];
    zend_string **keys, *zkey;
    zend_ulong idx;

    if ((ra = redis_array_get(getThis())) == NULL) {
        RETURN_FALSE;
    }

    /* Multi/exec support */
    HANDLE_MULTI_EXEC(ra, "MSET", sizeof("MSET") - 1);

    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_ARRAY(z_keys)
    ZEND_PARSE_PARAMETERS_END_EX(RETURN_FALSE);

    /* init data structures */
    h_keys = Z_ARRVAL_P(z_keys);
    if ((argc = zend_hash_num_elements(h_keys)) == 0) {
        RETURN_FALSE;
    }
    argv = ecalloc(argc, sizeof(*argv));
    pos = ecalloc(argc, sizeof(*pos));
    keys = ecalloc(argc, sizeof(*keys));

    argc_each = ecalloc(ra->count, sizeof(*argc_each));

    /* associate each key to a redis node */
    ZEND_HASH_FOREACH_KEY_VAL(h_keys, idx, zkey, data) {
        /* If the key isn't a string, make a string representation of it */
        if (zkey) {
            key_len = ZSTR_LEN(zkey);
            key = ZSTR_VAL(zkey);
        } else {
            key_len = snprintf(kbuf, sizeof(kbuf), ZEND_ULONG_FMT, idx);
            key = kbuf;
        }

        if (ra_find_node(ra, key, (int)key_len, &pos[i]) == NULL) {
            for (n = 0; n < i; ++n) {
                zend_string_release(keys[n]);
            }
            efree(keys);
            efree(argv);
            efree(pos);
            efree(argc_each);
            RETURN_FALSE;
        }

        argc_each[pos[i]]++;    /* count number of keys per node */
        keys[i] = zkey ? zend_string_copy(zkey) : zend_string_init(key, key_len, 0);
        argv[i] = data;
        i++;
    } ZEND_HASH_FOREACH_END();


    /* prepare call */
    ZVAL_STRINGL(&z_fun, "MSET", sizeof("MSET") - 1);

    /* calls */
    for (n = 0; n < ra->count; ++n) { /* for each node */
        /* We don't even need to make a call to this node if no keys go there */
        if(!argc_each[n]) continue;

        int found = 0;

        /* copy args */
        array_init(&z_argarray);
        for(i = 0; i < argc; ++i) {
            if(pos[i] != n) continue;

            if (argv[i] == NULL) {
                ZVAL_NULL(&z_ret);
            } else {
                ZVAL_ZVAL(&z_ret, argv[i], 1, 0);
            }
            add_assoc_zval_ex(&z_argarray, ZSTR_VAL(keys[i]), ZSTR_LEN(keys[i]), &z_ret);
            found++;
        }

        if(!found) {
            zval_ptr_dtor_nogc(&z_argarray);
            continue; /* don't run empty MSETs */
        }

        if(ra->index) { /* add MULTI */
            ra_index_multi(&ra->redis[n], MULTI);
            call_user_function(&redis_ce->function_table, &ra->redis[n], &z_fun, &z_ret, 1, &z_argarray);
            ra_index_keys(&z_argarray, &ra->redis[n]); /* use SADD to add keys to node index */
            ra_index_exec(&ra->redis[n], NULL, 0); /* run EXEC */
        } else {
            call_user_function(&redis_ce->function_table, &ra->redis[n], &z_fun, &z_ret, 1, &z_argarray);
        }

        zval_ptr_dtor_nogc(&z_argarray);
        zval_ptr_dtor_nogc(&z_ret);
    }

    zval_ptr_dtor_nogc(&z_fun);

    /* Free any keys that we needed to allocate memory for, because they weren't strings */
    for(i = 0; i < argc; i++) {
        zend_string_release(keys[i]);
    }

    /* cleanup */
    efree(keys);
    efree(argv);
    efree(pos);
    efree(argc_each);

    RETURN_TRUE;
}

/* Generic handler for DEL or UNLINK which behave identically to phpredis */
static void
ra_generic_del(INTERNAL_FUNCTION_PARAMETERS, char *kw, int kw_len)
{
    zval z_keys, z_fun, *data, z_ret, *z_args, **argv;
    int i, n, *pos, argc = ZEND_NUM_ARGS(), *argc_each, free_zkeys = 0;
    HashTable *h_keys;
    RedisArray *ra;
    long total = 0;

    if ((ra = redis_array_get(getThis())) == NULL) {
        RETURN_FALSE;
    }

    /* Multi/exec support */
    HANDLE_MULTI_EXEC(ra, kw, kw_len);

    /* get all args in z_args */
    z_args = ecalloc(argc, sizeof(*z_args));
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
            ZVAL_ZVAL(&z_ret, &z_args[i], 1, 0);
            add_next_index_zval(&z_keys, &z_ret);
        }
        free_zkeys = 1;
    }

    /* init data structures */
    h_keys = Z_ARRVAL(z_keys);
    if ((argc = zend_hash_num_elements(h_keys)) == 0) {
        if (free_zkeys) zval_ptr_dtor_nogc(&z_keys);
        efree(z_args);
        RETURN_FALSE;
    }
    argv = ecalloc(argc, sizeof(*argv));
    pos = ecalloc(argc, sizeof(*pos));

    argc_each = ecalloc(ra->count, sizeof(*argc_each));

    /* associate each key to a redis node */
    i = 0;
    ZEND_HASH_FOREACH_VAL(h_keys, data) {
        if (Z_TYPE_P(data) != IS_STRING) {
            php_error_docref(NULL, E_ERROR, "DEL: all keys must be string.");
            RETVAL_FALSE;
            goto cleanup;
        }

        if (ra_find_node(ra, Z_STRVAL_P(data), Z_STRLEN_P(data), &pos[i]) == NULL) {
            RETVAL_FALSE;
            goto cleanup;
        }
        argc_each[pos[i]]++;    /* count number of keys per node */
        argv[i++] = data;
    } ZEND_HASH_FOREACH_END();

    /* prepare call */
    ZVAL_STRINGL(&z_fun, kw, kw_len);

    /* calls */
    for(n = 0; n < ra->count; ++n) { /* for each node */
        /* We don't even need to make a call to this node if no keys go there */
        if(!argc_each[n]) continue;

        int found = 0;
        zval z_argarray;

        /* copy args */
        array_init(&z_argarray);
        for(i = 0; i < argc; ++i) {
            if (pos[i] == n) {
                ZVAL_ZVAL(&z_ret, argv[i], 1, 0);
                add_next_index_zval(&z_argarray, &z_ret);
                found++;
            }
        }

        if(!found) {    /* don't run empty DEL or UNLINK commands */
            zval_ptr_dtor_nogc(&z_argarray);
            continue;
        }

        if(ra->index) { /* add MULTI */
            ra_index_multi(&ra->redis[n], MULTI);
            call_user_function(&redis_ce->function_table, &ra->redis[n], &z_fun, &z_ret, 1, &z_argarray);
            ra_index_del(&z_argarray, &ra->redis[n]); /* use SREM to remove keys from node index */
            ra_index_exec(&ra->redis[n], &z_ret, 0); /* run EXEC */
        } else {
            call_user_function(&redis_ce->function_table, &ra->redis[n], &z_fun, &z_ret, 1, &z_argarray);
        }
        total += Z_LVAL(z_ret);    /* increment total */

        zval_ptr_dtor_nogc(&z_argarray);
        zval_ptr_dtor_nogc(&z_ret);
    }

    zval_ptr_dtor_nogc(&z_fun);

    RETVAL_LONG(total);

cleanup:
    efree(argv);
    efree(pos);
    efree(argc_each);

    if(free_zkeys) {
        zval_ptr_dtor_nogc(&z_keys);
    }
    efree(z_args);
}

/* DEL will distribute the call to several nodes and regroup the values. */
PHP_METHOD(RedisArray, del)
{
    ra_generic_del(INTERNAL_FUNCTION_PARAM_PASSTHRU, "DEL", sizeof("DEL")-1);
}

PHP_METHOD(RedisArray, unlink) {
    ra_generic_del(INTERNAL_FUNCTION_PARAM_PASSTHRU, "UNLINK", sizeof("UNLINK") - 1);
}

static void
ra_generic_scan_cmd(INTERNAL_FUNCTION_PARAMETERS, const char *kw, int kw_len)
{
    RedisArray *ra;
    zend_string *key, *pattern = NULL;
    zval *redis_inst, *z_cursor, z_fun, z_args[4];
    zend_long count = 0;

    ZEND_PARSE_PARAMETERS_START(2, 4)
        Z_PARAM_STR(key)
        Z_PARAM_ZVAL_EX(z_cursor, 0, 1)
        Z_PARAM_OPTIONAL
        Z_PARAM_STR_OR_NULL(pattern)
        Z_PARAM_LONG(count)
    ZEND_PARSE_PARAMETERS_END_EX(RETURN_FALSE);

    if ((ra = redis_array_get(getThis())) == NULL) {
        RETURN_FALSE;
    }

    if ((redis_inst = ra_find_node(ra, ZSTR_VAL(key), ZSTR_LEN(key), NULL)) == NULL) {
        php_error_docref(NULL, E_ERROR, "Could not find any redis servers for this key.");
        RETURN_FALSE;
    }

    ZVAL_STR(&z_args[0], key);
    ZVAL_NEW_REF(&z_args[1], z_cursor);
    if (pattern) ZVAL_STR(&z_args[2], pattern);
    ZVAL_LONG(&z_args[3], count);

    ZVAL_STRINGL(&z_fun, kw, kw_len);
    call_user_function(&redis_ce->function_table, redis_inst, &z_fun, return_value, ZEND_NUM_ARGS(), z_args);
    zval_ptr_dtor_nogc(&z_fun);

    ZVAL_ZVAL(z_cursor, &z_args[1], 0, 1);
}

PHP_METHOD(RedisArray, hscan)
{
    ra_generic_scan_cmd(INTERNAL_FUNCTION_PARAM_PASSTHRU, "HSCAN", sizeof("HSCAN") - 1);
}

PHP_METHOD(RedisArray, sscan)
{
    ra_generic_scan_cmd(INTERNAL_FUNCTION_PARAM_PASSTHRU, "SSCAN", sizeof("SSCAN") - 1);
}

PHP_METHOD(RedisArray, zscan)
{
    ra_generic_scan_cmd(INTERNAL_FUNCTION_PARAM_PASSTHRU, "ZSCAN", sizeof("ZSCAN") - 1);
}

PHP_METHOD(RedisArray, scan)
{
    RedisArray *ra;
    zend_string *host, *pattern = NULL;
    zval *redis_inst, *z_iter, z_fun, z_args[3];
    zend_long count = 0;

    ZEND_PARSE_PARAMETERS_START(2, 4)
        Z_PARAM_ZVAL_EX(z_iter, 0, 1)
        Z_PARAM_STR(host)
        Z_PARAM_OPTIONAL
        Z_PARAM_STR_OR_NULL(pattern)
        Z_PARAM_LONG(count)
    ZEND_PARSE_PARAMETERS_END_EX(RETURN_FALSE);

    if ((ra = redis_array_get(getThis())) == NULL) {
        RETURN_FALSE;
    }

    if ((redis_inst = ra_find_node_by_name(ra, host)) == NULL) {
        RETURN_FALSE;
    }

    ZVAL_NEW_REF(&z_args[0], z_iter);
    if (pattern) ZVAL_STR(&z_args[1], pattern);
    ZVAL_LONG(&z_args[2], count);

    ZVAL_STRING(&z_fun, "SCAN");
    call_user_function(&redis_ce->function_table, redis_inst, &z_fun, return_value, ZEND_NUM_ARGS() - 1, z_args);
    zval_ptr_dtor_nogc(&z_fun);

    ZVAL_ZVAL(z_iter, &z_args[0], 0, 1);
}

PHP_METHOD(RedisArray, multi)
{
    RedisArray *ra;
    zval *z_redis;
    zend_string *host;
    zend_long multi_value = MULTI;

    ZEND_PARSE_PARAMETERS_START(1, 2)
        Z_PARAM_STR(host)
        Z_PARAM_OPTIONAL
        Z_PARAM_LONG(multi_value)
    ZEND_PARSE_PARAMETERS_END_EX(RETURN_FALSE);

    if ((ra = redis_array_get(getThis())) == NULL) {
        RETURN_FALSE;
    }

    /* find node */
    if ((z_redis = ra_find_node_by_name(ra, host)) == NULL) {
        RETURN_FALSE;
    }

    if(multi_value != MULTI && multi_value != PIPELINE) {
        RETURN_FALSE;
    }

    /* save multi object */
    ra->z_multi_exec = z_redis;

    /* switch redis instance to multi/exec mode. */
    ra_index_multi(z_redis, multi_value);

    /* return this. */
    RETURN_ZVAL(getThis(), 1, 0);
}

PHP_METHOD(RedisArray, exec)
{
    RedisArray *ra;

    if ((ra = redis_array_get(getThis())) == NULL || !ra->z_multi_exec) {
        RETURN_FALSE;
    }

    /* switch redis instance out of multi/exec mode. */
    ra_index_exec(ra->z_multi_exec, return_value, 1);

    /* remove multi object */
    ra->z_multi_exec = NULL;
}

PHP_METHOD(RedisArray, discard)
{
    RedisArray *ra;

    if ((ra = redis_array_get(getThis())) == NULL || !ra->z_multi_exec) {
        RETURN_FALSE;
    }

    /* switch redis instance out of multi/exec mode. */
    ra_index_discard(ra->z_multi_exec, return_value);

    /* remove multi object */
    ra->z_multi_exec = NULL;
}

PHP_METHOD(RedisArray, unwatch)
{
    RedisArray *ra;

    if ((ra = redis_array_get(getThis())) == NULL || !ra->z_multi_exec) {
        RETURN_FALSE;
    }

    /* unwatch keys, stay in multi/exec mode. */
    ra_index_unwatch(ra->z_multi_exec, return_value);
}
