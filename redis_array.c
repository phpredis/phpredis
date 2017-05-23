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

static void redis_array_free(RedisArray *ra) {
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

int le_redis_array;
void redis_destructor_redis_array(zend_resource * rsrc TSRMLS_DC)
{
    RedisArray *ra = (RedisArray*)rsrc->ptr;

    /* Free previous ring if it's set */
    if(ra->prev) redis_array_free(ra->prev);

    /* Free parent array */
    redis_array_free(ra);
}


/**
 * redis_array_get
 */
PHP_REDIS_API int redis_array_get(zval *id, RedisArray **ra TSRMLS_DC)
{

    zval *socket;

    if (Z_TYPE_P(id) != IS_OBJECT || (socket = zend_hash_str_find(Z_OBJPROP_P(id),
            "socket", sizeof("socket") - 1)) == NULL) {
        return -1;
    }

#if (PHP_MAJOR_VERSION < 7)
    int resource_type;
    *ra = (RedisArray *)zend_list_find(Z_LVAL_P(socket), &resource_type);
    if (!*ra || resource_type != le_redis_array) {
#else
    if (Z_RES_P(socket) == NULL ||
        !(*ra = (RedisArray *)Z_RES_P(socket)->ptr) ||
        Z_RES_P(socket)->type != le_redis_array
    ) {
#endif
        return -1;
    }

    return 0;
}

uint32_t rcrc32(const char *s, size_t sz) {

	static const uint32_t table[256] = {
		0x00000000,0x77073096,0xEE0E612C,0x990951BA,0x076DC419,0x706AF48F,0xE963A535,
		0x9E6495A3,0x0EDB8832,0x79DCB8A4,0xE0D5E91E,0x97D2D988,0x09B64C2B,0x7EB17CBD,
		0xE7B82D07,0x90BF1D91,0x1DB71064,0x6AB020F2,0xF3B97148,0x84BE41DE,0x1ADAD47D,
		0x6DDDE4EB,0xF4D4B551,0x83D385C7,0x136C9856,0x646BA8C0,0xFD62F97A,0x8A65C9EC,
		0x14015C4F,0x63066CD9,0xFA0F3D63,0x8D080DF5,0x3B6E20C8,0x4C69105E,0xD56041E4,
		0xA2677172,0x3C03E4D1,0x4B04D447,0xD20D85FD,0xA50AB56B,0x35B5A8FA,0x42B2986C,
		0xDBBBC9D6,0xACBCF940,0x32D86CE3,0x45DF5C75,0xDCD60DCF,0xABD13D59,0x26D930AC,
		0x51DE003A,0xC8D75180,0xBFD06116,0x21B4F4B5,0x56B3C423,0xCFBA9599,0xB8BDA50F,
		0x2802B89E,0x5F058808,0xC60CD9B2,0xB10BE924,0x2F6F7C87,0x58684C11,0xC1611DAB,
		0xB6662D3D,0x76DC4190,0x01DB7106,0x98D220BC,0xEFD5102A,0x71B18589,0x06B6B51F,
		0x9FBFE4A5,0xE8B8D433,0x7807C9A2,0x0F00F934,0x9609A88E,0xE10E9818,0x7F6A0DBB,
		0x086D3D2D,0x91646C97,0xE6635C01,0x6B6B51F4,0x1C6C6162,0x856530D8,0xF262004E,
		0x6C0695ED,0x1B01A57B,0x8208F4C1,0xF50FC457,0x65B0D9C6,0x12B7E950,0x8BBEB8EA,
		0xFCB9887C,0x62DD1DDF,0x15DA2D49,0x8CD37CF3,0xFBD44C65,0x4DB26158,0x3AB551CE,
		0xA3BC0074,0xD4BB30E2,0x4ADFA541,0x3DD895D7,0xA4D1C46D,0xD3D6F4FB,0x4369E96A,
		0x346ED9FC,0xAD678846,0xDA60B8D0,0x44042D73,0x33031DE5,0xAA0A4C5F,0xDD0D7CC9,
		0x5005713C,0x270241AA,0xBE0B1010,0xC90C2086,0x5768B525,0x206F85B3,0xB966D409,
		0xCE61E49F,0x5EDEF90E,0x29D9C998,0xB0D09822,0xC7D7A8B4,0x59B33D17,0x2EB40D81,
		0xB7BD5C3B,0xC0BA6CAD,0xEDB88320,0x9ABFB3B6,0x03B6E20C,0x74B1D29A,0xEAD54739,
		0x9DD277AF,0x04DB2615,0x73DC1683,0xE3630B12,0x94643B84,0x0D6D6A3E,0x7A6A5AA8,
		0xE40ECF0B,0x9309FF9D,0x0A00AE27,0x7D079EB1,0xF00F9344,0x8708A3D2,0x1E01F268,
		0x6906C2FE,0xF762575D,0x806567CB,0x196C3671,0x6E6B06E7,0xFED41B76,0x89D32BE0,
		0x10DA7A5A,0x67DD4ACC,0xF9B9DF6F,0x8EBEEFF9,0x17B7BE43,0x60B08ED5,0xD6D6A3E8,
		0xA1D1937E,0x38D8C2C4,0x4FDFF252,0xD1BB67F1,0xA6BC5767,0x3FB506DD,0x48B2364B,
		0xD80D2BDA,0xAF0A1B4C,0x36034AF6,0x41047A60,0xDF60EFC3,0xA867DF55,0x316E8EEF,
		0x4669BE79,0xCB61B38C,0xBC66831A,0x256FD2A0,0x5268E236,0xCC0C7795,0xBB0B4703,
		0x220216B9,0x5505262F,0xC5BA3BBE,0xB2BD0B28,0x2BB45A92,0x5CB36A04,0xC2D7FFA7,
		0xB5D0CF31,0x2CD99E8B,0x5BDEAE1D,0x9B64C2B0,0xEC63F226,0x756AA39C,0x026D930A,
		0x9C0906A9,0xEB0E363F,0x72076785,0x05005713,0x95BF4A82,0xE2B87A14,0x7BB12BAE,
		0x0CB61B38,0x92D28E9B,0xE5D5BE0D,0x7CDCEFB7,0x0BDBDF21,0x86D3D2D4,0xF1D4E242,
		0x68DDB3F8,0x1FDA836E,0x81BE16CD,0xF6B9265B,0x6FB077E1,0x18B74777,0x88085AE6,
		0xFF0F6A70,0x66063BCA,0x11010B5C,0x8F659EFF,0xF862AE69,0x616BFFD3,0x166CCF45,
		0xA00AE278,0xD70DD2EE,0x4E048354,0x3903B3C2,0xA7672661,0xD06016F7,0x4969474D,
		0x3E6E77DB,0xAED16A4A,0xD9D65ADC,0x40DF0B66,0x37D83BF0,0xA9BCAE53,0xDEBB9EC5,
		0x47B2CF7F,0x30B5FFE9,0xBDBDF21C,0xCABAC28A,0x53B39330,0x24B4A3A6,0xBAD03605,
		0xCDD70693,0x54DE5729,0x23D967BF,0xB3667A2E,0xC4614AB8,0x5D681B02,0x2A6F2B94,
		0xB40BBE37,0xC30C8EA1,0x5A05DF1B,0x2D02EF8D};

	unsigned long ret = 0xffffffff;
	size_t i;

	for (i = 0; i < sz; i++) {
		ret = (ret >> 8) ^ table[ (ret ^ ((unsigned char)s[i])) & 0xFF ];
	}
	return (ret ^ 0xFFFFFFFF);

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
	double d_connect_timeout = 0;

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
	}

	/* extract either name of list of hosts from z0 */
	switch(Z_TYPE_P(z0)) {
		case IS_STRING:
			ra = ra_load_array(Z_STRVAL_P(z0) TSRMLS_CC);
			break;

		case IS_ARRAY:
			ra = ra_make_array(Z_ARRVAL_P(z0), &z_fun, &z_dist, hPrev, b_index, b_pconnect, l_retry_interval, b_lazy_connect, d_connect_timeout TSRMLS_CC);
			break;

		default:
			WRONG_PARAM_COUNT;
			break;
	}
    zval_dtor(&z_dist);
    zval_dtor(&z_fun);

	if(ra) {
		ra->auto_rehash = b_autorehash;
		ra->connect_timeout = d_connect_timeout;
		if(ra->prev) ra->prev->auto_rehash = b_autorehash;
#if (PHP_MAJOR_VERSION < 7)
        int id;
#if PHP_VERSION_ID >= 50400
		id = zend_list_insert(ra, le_redis_array TSRMLS_CC);
#else
		id = zend_list_insert(ra, le_redis_array);
#endif
		add_property_resource(getThis(), "socket", id);
#else
        zval *id = zend_list_insert(ra, le_redis_array TSRMLS_CC);
        add_property_resource(getThis(), "socket", Z_RES_P(id));
#endif
	}
}

static void
ra_forward_call(INTERNAL_FUNCTION_PARAMETERS, RedisArray *ra, const char *cmd, int cmd_len, zval *z_args, zval *z_new_target) {

	zval z_tmp, z_fun, *redis_inst, *z_callargs, *zp_tmp;
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
		if(!(key = ra_find_key(ra, z_args, cmd, &key_len))) {
			php_error_docref(NULL TSRMLS_CC, E_ERROR, "Could not find key");
			RETURN_FALSE;
		}

		/* find node */
		redis_inst = ra_find_node(ra, key, key_len, NULL TSRMLS_CC);
		if(!redis_inst) {
			php_error_docref(NULL TSRMLS_CC, E_ERROR, "Could not find any redis servers for this key.");
			RETURN_FALSE;
		}
	}

	/* check if write cmd */
	b_write_cmd = ra_is_write_cmd(ra, cmd, cmd_len);

	if(ra->index && b_write_cmd && !ra->z_multi_exec) { /* add MULTI + SADD */
		ra_index_multi(redis_inst, MULTI TSRMLS_CC);
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
		call_user_function(&redis_ce->function_table, ra->z_multi_exec, &z_fun, &z_tmp, argc, z_callargs);
        zval_dtor(&z_fun);
        zval_dtor(&z_tmp);
		efree(z_callargs);
		RETURN_ZVAL(getThis(), 1, 0);
	}

	/* CALL! */
	if(ra->index && b_write_cmd) {
		/* call using discarded temp value and extract exec results after. */
		call_user_function(&redis_ce->function_table, redis_inst, &z_fun, &z_tmp, argc, z_callargs);
		zval_dtor(&z_tmp);

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

	if (redis_array_get(object, &ra TSRMLS_CC) < 0) {
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

	if (redis_array_get(object, &ra TSRMLS_CC) < 0) {
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

	if (redis_array_get(object, &ra TSRMLS_CC) < 0) {
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

	if (redis_array_get(object, &ra TSRMLS_CC) < 0) {
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

	if (redis_array_get(object, &ra TSRMLS_CC) < 0) {
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

	if (redis_array_get(object, &ra TSRMLS_CC) < 0) {
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

	if (redis_array_get(object, &ra TSRMLS_CC) < 0) {
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

	if (redis_array_get(object, &ra TSRMLS_CC) < 0) {
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
	if(redis_array_get(object, &ra TSRMLS_CC) < 0) {
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

	if (redis_array_get(object, &ra TSRMLS_CC) < 0) {
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

	if (redis_array_get(object, &ra TSRMLS_CC) < 0) {
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

	if (redis_array_get(object, &ra TSRMLS_CC) < 0) {
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

    if (redis_array_get(getThis(), &ra TSRMLS_CC) < 0) {
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
	int i, n;
	RedisArray *ra;
	int *pos, argc, *argc_each;
	HashTable *h_keys;
    char *key, **keys, kbuf[40];
    int key_len, *key_lens;
    zend_string *zkey;
    ulong idx;

    if (redis_array_get(getThis(), &ra TSRMLS_CC) < 0) {
        RETURN_FALSE;
    }

	/* Multi/exec support */
    HANDLE_MULTI_EXEC(ra, "MSET");

	if (zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), "Oa",
				&object, redis_array_ce, &z_keys) == FAILURE) {
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
    i = 0;
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
	for(n = 0; n < ra->count; ++n) { /* for each node */
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

		if(!found)
		{
			zval_dtor(&z_argarray);
			continue;				/* don't run empty MSETs */
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

    if (redis_array_get(getThis(), &ra TSRMLS_CC) < 0) {
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

	if (redis_array_get(object, &ra TSRMLS_CC) < 0) {
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

	if (redis_array_get(object, &ra TSRMLS_CC) < 0 || !ra->z_multi_exec) {
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

	if (redis_array_get(object, &ra TSRMLS_CC) < 0 || !ra->z_multi_exec) {
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

	if (redis_array_get(object, &ra TSRMLS_CC) < 0 || !ra->z_multi_exec) {
		RETURN_FALSE;
	}

	/* unwatch keys, stay in multi/exec mode. */
	ra_index_unwatch(ra->z_multi_exec, return_value TSRMLS_CC);
}
