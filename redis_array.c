#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "common.h"
#include "ext/standard/info.h"
#include "php_ini.h"
#include "php_redis.h"
#include "redis_array.h"
#include <zend_exceptions.h>

#include "library.h"
#include "redis_array.h"

extern zend_class_entry *redis_ce;

ZEND_BEGIN_ARG_INFO_EX(__redis_array_call_args, 0, 0, 2)
	ZEND_ARG_INFO(0, function_name)
	ZEND_ARG_INFO(0, arguments)
ZEND_END_ARG_INFO()

zend_function_entry redis_array_functions[] = {
     PHP_ME(RedisArray, __construct, NULL, ZEND_ACC_PUBLIC)
     PHP_ME(RedisArray, __call, __redis_array_call_args, ZEND_ACC_PUBLIC)

     PHP_ME(RedisArray, _hosts, NULL, ZEND_ACC_PUBLIC)
     PHP_ME(RedisArray, _target, NULL, ZEND_ACC_PUBLIC)

     /* special implementation for a few functions */
     PHP_ME(RedisArray, info, NULL, ZEND_ACC_PUBLIC)
     PHP_ME(RedisArray, mget, NULL, ZEND_ACC_PUBLIC)
     {NULL, NULL, NULL}
};

int le_redis_array;
extern int le_redis_sock;
void redis_destructor_redis_array(zend_rsrc_list_entry * rsrc TSRMLS_DC)
{
	/* TODO */
	/*
	   RedisSock *redis_sock = (RedisSock *) rsrc->ptr;
	   redis_sock_disconnect(redis_sock TSRMLS_CC);
	   redis_free_socket(redis_sock);
	 */
}

/**
 * redis_array_get
 */
PHPAPI int redis_array_get(zval *id, RedisArray **ra TSRMLS_DC)
{

    zval **socket;
    int resource_type;

    if (Z_TYPE_P(id) != IS_OBJECT || zend_hash_find(Z_OBJPROP_P(id), "socket",
                                  sizeof("socket"), (void **) &socket) == FAILURE) {
        return -1;
    }

    *ra = (RedisArray *) zend_list_find(Z_LVAL_PP(socket), &resource_type);

    if (!*ra || resource_type != le_redis_array) {
            return -1;
    }

    return Z_LVAL_PP(socket);
}

RedisArray *
ra_make_array(HashTable *hosts, zval *z_fun) {

	int i, host_len, id;
	int count = zend_hash_num_elements(hosts);
	char *host, *p;
	short port;
	zval **zpData, z_cons, *z_args, z_ret;
	RedisSock *redis_sock  = NULL;

	/* create object */
	RedisArray *ra = emalloc(sizeof(RedisArray));
	ra->hosts = emalloc(count * sizeof(char*));
	ra->redis = emalloc(count * sizeof(zval*));
	ra->count = count;
	ra->z_fun = NULL;

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

char *
ra_extract_key(RedisArray *ra, const char *key, int key_len, int *out_len) {

	char *start, *end;
	*out_len = key_len;

	if(ra->z_fun)
		return ra_call_extractor(ra, key, key_len, out_len);

	/* look for '{' */
	start = strchr(key, '{');
	if(!start) return estrndup(key, key_len);

	/* look for '}' */
	end = strchr(start+1, '}');
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

uint32_t crc32(const char *s, size_t sz) {

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
	zval *z0, *z_fun = NULL;
	char *name = NULL;
	int id;
	RedisArray *ra = NULL;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "z|z", &z0, &z_fun) == FAILURE) {
		RETURN_FALSE;
	}

	if(!z_fun) { /* either an array name or a list of hosts */
		switch(Z_TYPE_P(z0)) {
			case IS_STRING:
				name = Z_STRVAL_P(z0);
				break;

			case IS_ARRAY:
				ra = ra_make_array(Z_ARRVAL_P(z0), NULL);
				break;

			default:
				WRONG_PARAM_COUNT;
				break;
		}
	} else {
		if(Z_TYPE_P(z0) != IS_ARRAY) {
			WRONG_PARAM_COUNT;
		}
		// printf("ARRAY OF HOSTS, fun=%s\n", fun);
		ra = ra_make_array(Z_ARRVAL_P(z0), z_fun);
	}

	if(ra) {
		id = zend_list_insert(ra, le_redis_array);
		add_property_resource(getThis(), "socket", id);
	}
}

PHP_METHOD(RedisArray, __call)
{
	zval *object, *z_args, **zp_tmp;

	char *cmd, *key;
	int cmd_len, key_len;
	int key_pos, i;
	RedisArray *ra;
	zval *redis_inst;
	zval z_fun, **z_callargs;
	HashPosition pointer;
	HashTable *h_args;
	int argc;

	if (zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), "Osa",
				&object, redis_array_ce, &cmd, &cmd_len, &z_args) == FAILURE) {
		RETURN_FALSE;
	}

	if (redis_array_get(object, &ra TSRMLS_CC) < 0) {
		RETURN_FALSE;
	}

	h_args = Z_ARRVAL_P(z_args);
	argc = zend_hash_num_elements(h_args);

	/* get key and hash it. */
	key_pos = 0; /* TODO: change this depending on the command */

	if(	zend_hash_num_elements(Z_ARRVAL_P(z_args)) == 0
		|| zend_hash_quick_find(Z_ARRVAL_P(z_args), NULL, 0, key_pos, (void**)&zp_tmp) == FAILURE
		|| Z_TYPE_PP(zp_tmp) != IS_STRING) {

		php_error_docref(NULL TSRMLS_CC, E_ERROR, "Could not find key");
		RETURN_FALSE;
	}

	key = Z_STRVAL_PP(zp_tmp);
	key_len = Z_STRLEN_PP(zp_tmp);

	/* find node */
	redis_inst = ra_find_node(ra, key, key_len, NULL);
	if(!redis_inst) {
		php_error_docref(NULL TSRMLS_CC, E_ERROR, "Could not find any redis servers for this key.");
		RETURN_FALSE;
	}

	/* pass call through */
	ZVAL_STRING(&z_fun, cmd, 0);	/* method name */
	z_callargs = emalloc(argc * sizeof(zval*));
	/* copy args to */
	for (i = 0, zend_hash_internal_pointer_reset_ex(h_args, &pointer);
			zend_hash_get_current_data_ex(h_args, (void**) &zp_tmp,
				&pointer) == SUCCESS;
			++i, zend_hash_move_forward_ex(h_args, &pointer)) {

		z_callargs[i] = *zp_tmp;
	}

	/* CALL! */
	call_user_function(&redis_ce->function_table, &redis_inst,
			&z_fun, return_value, argc, z_callargs TSRMLS_CC);
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
		add_next_index_string(return_value, ra->hosts[i], 1);
	}
}

PHP_METHOD(RedisArray, _target)
{
	zval *object;
	RedisArray *ra;
	char *key;
	int key_len, i;
	zval *redis_inst;

	if (zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), "Os",
				&object, redis_array_ce, &key, &key_len) == FAILURE) {
		RETURN_FALSE;
	}

	if (redis_array_get(object, &ra TSRMLS_CC) < 0) {
		RETURN_FALSE;
	}

	redis_inst = ra_find_node(ra, key, key_len, &i);
	if(redis_inst) {
		ZVAL_STRING(return_value, ra->hosts[i], 1);
	} else {
		RETURN_NULL();
	}
}

PHP_METHOD(RedisArray, info)
{
	zval *object, z_fun, *z_tmp;
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
	ZVAL_STRING(&z_fun, "INFO", 0);

	array_init(return_value);
	for(i = 0; i < ra->count; ++i) {

		MAKE_STD_ZVAL(z_tmp);

		/* Call each node in turn */
		call_user_function(&redis_ce->function_table, &ra->redis[i],
				&z_fun, z_tmp, 0, NULL TSRMLS_CC);

		add_assoc_zval(return_value, ra->hosts[i], z_tmp);
	}
}

/* MGET will distribute the call to several nodes and regroup the values. */
PHP_METHOD(RedisArray, mget)
{
	zval *object, *z_keys, z_fun, *z_argarray, **data, *z_ret, **z_cur, *z_tmp_array, *z_tmp;
	int i, j, n;
	RedisArray *ra;
	int *pos, argc, *argc_each;
	HashTable *h_keys;
	HashPosition pointer;
	zval **redis_instances, *redis_inst, **argv;

	if (zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), "Oa",
				&object, redis_array_ce, &z_keys) == FAILURE) {
		RETURN_FALSE;
	}

	if (redis_array_get(object, &ra TSRMLS_CC) < 0) {
		RETURN_FALSE;
	}

	/* prepare call */
	ZVAL_STRING(&z_fun, "MGET", 0);

	/* init data structures */
	h_keys = Z_ARRVAL_P(z_keys);
	argc = zend_hash_num_elements(h_keys);
	argv = emalloc(argc * sizeof(zval*));
	pos = emalloc(argc * sizeof(int));
	redis_instances = emalloc(argc * sizeof(zval*));
	memset(redis_instances, 0, argc * sizeof(zval*));

	argc_each = emalloc(ra->count * sizeof(int));
	memset(argc_each, 0, ra->count * sizeof(int));

	/* associate each key to a redis node */
	for (i = 0, zend_hash_internal_pointer_reset_ex(h_keys, &pointer);
			zend_hash_get_current_data_ex(h_keys, (void**) &data,
				&pointer) == SUCCESS;
			zend_hash_move_forward_ex(h_keys, &pointer), ++i) {

		if (Z_TYPE_PP(data) != IS_STRING) {
			php_error_docref(NULL TSRMLS_CC, E_ERROR, "MGET: all keys must be string.");
			efree(pos);
			RETURN_FALSE;
		}

		redis_instances[i] = ra_find_node(ra, Z_STRVAL_PP(data), Z_STRLEN_PP(data), &pos[i]);
		argc_each[pos[i]]++;	/* count number of keys per node */
		argv[i] = *data;
	}

	/* prepare return value */
	array_init(return_value);
	MAKE_STD_ZVAL(z_tmp_array);
	array_init(z_tmp_array);

	/* calls */
	for(n = 0; n < ra->count; ++n) { /* for each node */

		redis_inst = ra->redis[n];

		/* copy args */
		MAKE_STD_ZVAL(z_argarray);
		array_init(z_argarray);
		for(i = 0; i < argc; ++i) {
			if(pos[i] != n) continue;

			MAKE_STD_ZVAL(z_tmp);
			*z_tmp = *argv[i];
			zval_copy_ctor(z_tmp);

			add_next_index_zval(z_argarray, z_tmp);
		}

		/* call */
		MAKE_STD_ZVAL(z_ret);
		call_user_function(&redis_ce->function_table, &ra->redis[n],
				&z_fun, z_ret, 1, &z_argarray TSRMLS_CC);

		for(i = 0, j = 0; i < argc; ++i) {
			if(pos[i] != n) continue;

			zend_hash_quick_find(Z_ARRVAL_P(z_ret), NULL, 0, j, (void**)&z_cur);
			j++;

			MAKE_STD_ZVAL(z_tmp);
			*z_tmp = **z_cur;
			zval_copy_ctor(z_tmp);
			add_index_zval(z_tmp_array, i, z_tmp);
		}
		zval_dtor(z_ret);
		efree(z_ret);

		zval_dtor(z_argarray);
		efree(z_argarray);
	}

	/* copy temp array in the right order to return_value */
	for(i = 0; i < argc; ++i) {
		zend_hash_quick_find(Z_ARRVAL_P(z_tmp_array), NULL, 0, i, (void**)&z_cur);

		MAKE_STD_ZVAL(z_tmp);
		*z_tmp = **z_cur;
		zval_copy_ctor(z_tmp);
		add_next_index_zval(return_value, z_tmp);
	}

	/* cleanup */
	zval_dtor(z_tmp_array);
	efree(z_tmp_array);
	efree(argv);
	efree(pos);
	efree(redis_instances);
	efree(argc_each);
}
