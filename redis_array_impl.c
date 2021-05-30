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
#include "redis_array_impl.h"
#include "php_redis.h"
#include "library.h"

#include "php_variables.h"
#include "SAPI.h"
#include "ext/standard/url.h"
#include "ext/standard/crc32.h"
#include "ext/standard/md5.h"

#include "ext/hash/php_hash.h"

#define PHPREDIS_INDEX_NAME "__phpredis_array_index__"

extern zend_class_entry *redis_ce;

static RedisArray *
ra_load_hosts(RedisArray *ra, HashTable *hosts, zend_string *user,
              zend_string *pass, long retry_interval, zend_bool b_lazy_connect)
{
    int i = 0, host_len;
    char *host, *p;
    short port;
    zval *zpData;
    redis_object *redis;

    /* init connections */
    ZEND_HASH_FOREACH_VAL(hosts, zpData) {
        if (Z_TYPE_P(zpData) != IS_STRING) {
            return NULL;
        }

        /* default values */
        host = Z_STRVAL_P(zpData);
        host_len = Z_STRLEN_P(zpData);
        ra->hosts[i] = zend_string_init(host, host_len, 0);
        port = 6379;

        if((p = strrchr(host, ':'))) { /* found port */
            host_len = p - host;
            port = (short)atoi(p+1);
        } else if(strchr(host,'/') != NULL) { /* unix socket */
            port = -1;
        }

        /* create Redis object */
        object_init_ex(&ra->redis[i], redis_ce);
        redis = PHPREDIS_ZVAL_GET_OBJECT(redis_object, &ra->redis[i]);

        /* create socket */
        redis->sock = redis_sock_create(host, host_len, port, ra->connect_timeout,
                                        ra->read_timeout, ra->pconnect, NULL,
                                        retry_interval);

        redis_sock_set_auth(redis->sock, user, pass);

        if (!b_lazy_connect) {
            if (redis_sock_server_open(redis->sock) < 0) {
                ra->count = ++i;
                return NULL;
            }
        }

        ra->count = ++i;
    } ZEND_HASH_FOREACH_END();

    return ra;
}

/* List pure functions */
void
ra_init_function_table(RedisArray *ra)
{
    ALLOC_HASHTABLE(ra->pure_cmds);
    zend_hash_init(ra->pure_cmds, 0, NULL, NULL, 0);

    zend_hash_str_update_ptr(ra->pure_cmds, "EXISTS", sizeof("EXISTS") - 1, NULL);
    zend_hash_str_update_ptr(ra->pure_cmds, "GET", sizeof("GET") - 1, NULL);
    zend_hash_str_update_ptr(ra->pure_cmds, "GETBIT", sizeof("GETBIT") - 1, NULL);
    zend_hash_str_update_ptr(ra->pure_cmds, "GETRANGE", sizeof("GETRANGE") - 1, NULL);
    zend_hash_str_update_ptr(ra->pure_cmds, "HEXISTS", sizeof("HEXISTS") - 1, NULL);
    zend_hash_str_update_ptr(ra->pure_cmds, "HGET", sizeof("HGET") - 1, NULL);
    zend_hash_str_update_ptr(ra->pure_cmds, "HGETALL", sizeof("HGETALL") - 1, NULL);
    zend_hash_str_update_ptr(ra->pure_cmds, "HKEYS", sizeof("HKEYS") - 1, NULL);
    zend_hash_str_update_ptr(ra->pure_cmds, "HLEN", sizeof("HLEN") - 1, NULL);
    zend_hash_str_update_ptr(ra->pure_cmds, "HMGET", sizeof("HMGET") - 1, NULL);
    zend_hash_str_update_ptr(ra->pure_cmds, "HVALS", sizeof("HVALS") - 1, NULL);
    zend_hash_str_update_ptr(ra->pure_cmds, "LINDEX", sizeof("LINDEX") - 1, NULL);
    zend_hash_str_update_ptr(ra->pure_cmds, "LLEN", sizeof("LLEN") - 1, NULL);
    zend_hash_str_update_ptr(ra->pure_cmds, "LRANGE", sizeof("LRANGE") - 1, NULL);
    zend_hash_str_update_ptr(ra->pure_cmds, "OBJECT", sizeof("OBJECT") - 1, NULL);
    zend_hash_str_update_ptr(ra->pure_cmds, "SCARD", sizeof("SCARD") - 1, NULL);
    zend_hash_str_update_ptr(ra->pure_cmds, "SDIFF", sizeof("SDIFF") - 1, NULL);
    zend_hash_str_update_ptr(ra->pure_cmds, "SINTER", sizeof("SINTER") - 1, NULL);
    zend_hash_str_update_ptr(ra->pure_cmds, "SISMEMBER", sizeof("SISMEMBER") - 1, NULL);
    zend_hash_str_update_ptr(ra->pure_cmds, "SMEMBERS", sizeof("SMEMBERS") - 1, NULL);
    zend_hash_str_update_ptr(ra->pure_cmds, "SRANDMEMBER", sizeof("SRANDMEMBER") - 1, NULL);
    zend_hash_str_update_ptr(ra->pure_cmds, "STRLEN", sizeof("STRLEN") - 1, NULL);
    zend_hash_str_update_ptr(ra->pure_cmds, "SUNION", sizeof("SUNION") - 1, NULL);
    zend_hash_str_update_ptr(ra->pure_cmds, "TYPE", sizeof("TYPE") - 1, NULL);
    zend_hash_str_update_ptr(ra->pure_cmds, "ZCARD", sizeof("ZCARD") - 1, NULL);
    zend_hash_str_update_ptr(ra->pure_cmds, "ZCOUNT", sizeof("ZCOUNT") - 1, NULL);
    zend_hash_str_update_ptr(ra->pure_cmds, "ZRANGE", sizeof("ZRANGE") - 1, NULL);
    zend_hash_str_update_ptr(ra->pure_cmds, "ZRANK", sizeof("ZRANK") - 1, NULL);
    zend_hash_str_update_ptr(ra->pure_cmds, "ZREVRANGE", sizeof("ZREVRANGE") - 1, NULL);
    zend_hash_str_update_ptr(ra->pure_cmds, "ZREVRANGEBYSCORE", sizeof("ZREVRANGEBYSCORE") - 1, NULL);
    zend_hash_str_update_ptr(ra->pure_cmds, "ZREVRANK", sizeof("ZREVRANK") - 1, NULL);
    zend_hash_str_update_ptr(ra->pure_cmds, "ZSCORE", sizeof("ZSCORE") - 1, NULL);
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

/* load array from INI settings */
RedisArray *ra_load_array(const char *name) {
    zval *z_data, z_tmp, z_fun, z_dist;
    zval z_params_hosts;
    zval z_params_prev;
    RedisArray *ra = NULL;

    zend_string *algorithm = NULL, *user = NULL, *pass = NULL;
    zend_bool b_index = 0, b_autorehash = 0, b_pconnect = 0, consistent = 0;
    zend_long l_retry_interval = 0;
    zend_bool b_lazy_connect = 0;
    double d_connect_timeout = 0, read_timeout = 0.0;
    HashTable *hHosts = NULL, *hPrev = NULL;
    size_t name_len = strlen(name);
    char *iptr;

    /* find entry */
    if(!ra_find_name(name))
        return ra;

    ZVAL_NULL(&z_fun);
    ZVAL_NULL(&z_dist);

    /* find hosts */
    array_init(&z_params_hosts);
    if ((iptr = INI_STR("redis.arrays.hosts")) != NULL) {
        sapi_module.treat_data(PARSE_STRING, estrdup(iptr), &z_params_hosts);
        if ((z_data = zend_hash_str_find(Z_ARRVAL(z_params_hosts), name, name_len)) != NULL) {
            hHosts = Z_ARRVAL_P(z_data);
        }
    }

    /* find previous hosts */
    array_init(&z_params_prev);
    if ((iptr = INI_STR("redis.arrays.previous")) != NULL) {
        sapi_module.treat_data(PARSE_STRING, estrdup(iptr), &z_params_prev);
        if ((z_data = zend_hash_str_find(Z_ARRVAL(z_params_prev), name, name_len)) != NULL) {
            if (Z_TYPE_P(z_data) == IS_ARRAY) {
                hPrev = Z_ARRVAL_P(z_data);
            }
        }
    }

    /* find function */
    if ((iptr = INI_STR("redis.arrays.functions")) != NULL) {
        array_init(&z_tmp);
        sapi_module.treat_data(PARSE_STRING, estrdup(iptr), &z_tmp);
        redis_conf_zval(Z_ARRVAL(z_tmp), name, name_len, &z_fun, 1, 0);
        zval_dtor(&z_tmp);
    }

    /* find distributor */
    if ((iptr = INI_STR("redis.arrays.distributor")) != NULL) {
        array_init(&z_tmp);
        sapi_module.treat_data(PARSE_STRING, estrdup(iptr), &z_tmp);
        redis_conf_zval(Z_ARRVAL(z_tmp), name, name_len, &z_dist, 1, 0);
        zval_dtor(&z_tmp);
    }

    /* find hash algorithm */
    if ((iptr = INI_STR("redis.arrays.algorithm")) != NULL) {
        array_init(&z_tmp);
        sapi_module.treat_data(PARSE_STRING, estrdup(iptr), &z_tmp);
        redis_conf_string(Z_ARRVAL(z_tmp), name, name_len, &algorithm);
        zval_dtor(&z_tmp);
    }

    /* find index option */
    if ((iptr = INI_STR("redis.arrays.index")) != NULL) {
        array_init(&z_tmp);
        sapi_module.treat_data(PARSE_STRING, estrdup(iptr), &z_tmp);
        redis_conf_zend_bool(Z_ARRVAL(z_tmp), name, name_len, &b_index);
        zval_dtor(&z_tmp);
    }

    /* find autorehash option */
    if ((iptr = INI_STR("redis.arrays.autorehash")) != NULL) {
        array_init(&z_tmp);
        sapi_module.treat_data(PARSE_STRING, estrdup(iptr), &z_tmp);
        redis_conf_zend_bool(Z_ARRVAL(z_tmp), name, name_len, &b_autorehash);
        zval_dtor(&z_tmp);
    }

    /* find retry interval option */
    if ((iptr = INI_STR("redis.arrays.retryinterval")) != NULL) {
        array_init(&z_tmp);
        sapi_module.treat_data(PARSE_STRING, estrdup(iptr), &z_tmp);
        redis_conf_long(Z_ARRVAL(z_tmp), name, name_len, &l_retry_interval);
        zval_dtor(&z_tmp);
    }

    /* find pconnect option */
    if ((iptr = INI_STR("redis.arrays.pconnect")) != NULL) {
        array_init(&z_tmp);
        sapi_module.treat_data(PARSE_STRING, estrdup(iptr), &z_tmp);
        redis_conf_zend_bool(Z_ARRVAL(z_tmp), name, name_len, &b_pconnect);
        zval_dtor(&z_tmp);
    }

    /* find lazy connect option */
    if ((iptr = INI_STR("redis.arrays.lazyconnect")) != NULL) {
        array_init(&z_tmp);
        sapi_module.treat_data(PARSE_STRING, estrdup(iptr), &z_tmp);
        redis_conf_zend_bool(Z_ARRVAL(z_tmp), name, name_len, &b_lazy_connect);
        zval_dtor(&z_tmp);
    }

    /* find connect timeout option */
    if ((iptr = INI_STR("redis.arrays.connecttimeout")) != NULL) {
        array_init(&z_tmp);
        sapi_module.treat_data(PARSE_STRING, estrdup(iptr), &z_tmp);
        redis_conf_double(Z_ARRVAL(z_tmp), name, name_len, &d_connect_timeout);
        zval_dtor(&z_tmp);
    }

    /* find read timeout option */
    if ((iptr = INI_STR("redis.arrays.readtimeout")) != NULL) {
        array_init(&z_tmp);
        sapi_module.treat_data(PARSE_STRING, estrdup(iptr), &z_tmp);
        redis_conf_double(Z_ARRVAL(z_tmp), name, name_len, &read_timeout);
        zval_dtor(&z_tmp);
    }

    /* find consistent option */
    if ((iptr = INI_STR("redis.arrays.consistent")) != NULL) {
        array_init(&z_tmp);
        sapi_module.treat_data(PARSE_STRING, estrdup(iptr), &z_tmp);
        if ((z_data = zend_hash_str_find(Z_ARRVAL(z_tmp), name, name_len)) != NULL) {
            consistent = Z_TYPE_P(z_data) == IS_STRING && strncmp(Z_STRVAL_P(z_data), "1", 1) == 0;
        }
        zval_dtor(&z_tmp);
    }

    /* find auth option */
    if ((iptr = INI_STR("redis.arrays.auth")) != NULL) {
        array_init(&z_tmp);
        sapi_module.treat_data(PARSE_STRING, estrdup(iptr), &z_tmp);
        redis_conf_auth(Z_ARRVAL(z_tmp), name, name_len, &user, &pass);
        zval_dtor(&z_tmp);
    }

    /* create RedisArray object */
    ra = ra_make_array(hHosts, &z_fun, &z_dist, hPrev, b_index, b_pconnect, l_retry_interval,
                       b_lazy_connect, d_connect_timeout, read_timeout, consistent, algorithm,
                       user, pass);
    if (ra) {
        ra->auto_rehash = b_autorehash;
        if(ra->prev) ra->prev->auto_rehash = b_autorehash;
    }

    if (algorithm) zend_string_release(algorithm);
    if (user) zend_string_release(user);
    if (pass) zend_string_release(pass);

    zval_dtor(&z_params_hosts);
    zval_dtor(&z_params_prev);
    zval_dtor(&z_dist);
    zval_dtor(&z_fun);

    return ra;
}

static int
ra_points_cmp(const void *v1, const void *v2)
{
    const ContinuumPoint *p1 = v1, *p2 = v2;

    return p1->value < p2->value ? - 1 : p1->value > p2->value;
}

static Continuum *
ra_make_continuum(zend_string **hosts, int nb_hosts)
{
    int i, j, k, len, idx = 0;
    char host[HOST_NAME_MAX];
    unsigned char digest[16];
    PHP_MD5_CTX ctx;
    Continuum *c;

    c = ecalloc(1, sizeof(*c));
    c->nb_points = nb_hosts * 160; /* 40 hashes, 4 numbers per hash = 160 points per server */
    c->points = ecalloc(c->nb_points, sizeof(*c->points));

    for (i = 0; i < nb_hosts; ++i) {
        for (j = 0; j < 40; ++j) {
            len = snprintf(host, sizeof(host), "%.*s-%u", (int)ZSTR_LEN(hosts[i]), ZSTR_VAL(hosts[i]), j);
            PHP_MD5Init(&ctx);
            PHP_MD5Update(&ctx, host, len);
            PHP_MD5Final(digest, &ctx);
            for (k = 0; k < 4; ++k) {
                c->points[idx].index = i;
                c->points[idx++].value = (digest[3 + k * 4] << 24)
                    | (digest[2 + k * 4] << 16)
                    | (digest[1 + k * 4] << 8)
                    | (digest[k * 4]);
            }
        }
    }
    qsort(c->points, c->nb_points, sizeof(*c->points), ra_points_cmp);
    return c;
}

RedisArray *
ra_make_array(HashTable *hosts, zval *z_fun, zval *z_dist, HashTable *hosts_prev,
              zend_bool b_index, zend_bool b_pconnect, long retry_interval,
              zend_bool b_lazy_connect, double connect_timeout, double read_timeout,
              zend_bool consistent, zend_string *algorithm, zend_string *user,
              zend_string *pass)
{
    int i, count;
    RedisArray *ra;

    if (!hosts || (count = zend_hash_num_elements(hosts)) == 0) return NULL;

    /* create object */
    ra = emalloc(sizeof(RedisArray));
    ra->hosts = ecalloc(count, sizeof(*ra->hosts));
    ra->redis = ecalloc(count, sizeof(*ra->redis));
    ra->count = 0;
    ra->z_multi_exec = NULL;
    ra->index = b_index;
    ra->auto_rehash = 0;
    ra->pconnect = b_pconnect;
    ra->connect_timeout = connect_timeout;
    ra->read_timeout = read_timeout;
    ra->continuum = NULL;
    ra->algorithm = NULL;

    if (ra_load_hosts(ra, hosts, user, pass, retry_interval, b_lazy_connect) == NULL || !ra->count) {
        for (i = 0; i < ra->count; ++i) {
            zval_dtor(&ra->redis[i]);
            zend_string_release(ra->hosts[i]);
        }
        efree(ra->redis);
        efree(ra->hosts);
        efree(ra);
        return NULL;
    }

    ra->prev = hosts_prev ? ra_make_array(hosts_prev, z_fun, z_dist, NULL, b_index, b_pconnect, retry_interval, b_lazy_connect, connect_timeout, read_timeout, consistent, algorithm, user, pass) : NULL;

    /* init array data structures */
    ra_init_function_table(ra);

    /* Set hash function and distribtor if provided */
    ZVAL_ZVAL(&ra->z_fun, z_fun, 1, 0);
    ZVAL_ZVAL(&ra->z_dist, z_dist, 1, 0);
    if (algorithm) ra->algorithm = zend_string_copy(algorithm);

    /* init continuum */
    if (consistent) {
        ra->continuum = ra_make_continuum(ra->hosts, ra->count);
    }

    return ra;
}


/* call userland key extraction function */
zend_string *
ra_call_extractor(RedisArray *ra, const char *key, int key_len)
{
    zend_string *out = NULL;
    zval z_ret, z_argv;

    /* check that we can call the extractor function */
    if (!zend_is_callable_ex(&ra->z_fun, NULL, 0, NULL, NULL, NULL)) {
        php_error_docref(NULL, E_ERROR, "Could not call extractor function");
        return NULL;
    }

    ZVAL_NULL(&z_ret);
    /* call extraction function */
    ZVAL_STRINGL(&z_argv, key, key_len);
    call_user_function(EG(function_table), NULL, &ra->z_fun, &z_ret, 1, &z_argv);

    if (Z_TYPE(z_ret) == IS_STRING) {
        out = zval_get_string(&z_ret);
    }

    zval_dtor(&z_argv);
    zval_dtor(&z_ret);
    return out;
}

static zend_string *
ra_extract_key(RedisArray *ra, const char *key, int key_len)
{
    char *start, *end;

    if (Z_TYPE(ra->z_fun) != IS_NULL) {
        return ra_call_extractor(ra, key, key_len);
    } else if ((start = strchr(key, '{')) == NULL || (end = strchr(start + 1, '}')) == NULL) {
        return zend_string_init(key, key_len, 0);
    }
    /* found substring */
    return zend_string_init(start + 1, end - start - 1, 0);
}

/* call userland key distributor function */
int
ra_call_distributor(RedisArray *ra, const char *key, int key_len)
{
    int ret;
    zval z_ret, z_argv;

    /* check that we can call the extractor function */
    if (!zend_is_callable_ex(&ra->z_dist, NULL, 0, NULL, NULL, NULL)) {
        php_error_docref(NULL, E_ERROR, "Could not call distributor function");
        return -1;
    }

    ZVAL_NULL(&z_ret);
    /* call extraction function */
    ZVAL_STRINGL(&z_argv, key, key_len);
    call_user_function(EG(function_table), NULL, &ra->z_dist, &z_ret, 1, &z_argv);

    ret = (Z_TYPE(z_ret) == IS_LONG) ? Z_LVAL(z_ret) : -1;

    zval_dtor(&z_argv);
    zval_dtor(&z_ret);
    return ret;
}

zval *
ra_find_node(RedisArray *ra, const char *key, int key_len, int *out_pos)
{
    int pos;
    zend_string *out;

    /* extract relevant part of the key */
    if ((out = ra_extract_key(ra, key, key_len)) == NULL) {
        return NULL;
    }

    if (Z_TYPE(ra->z_dist) == IS_NULL) {
        int i;
        unsigned long ret = 0xffffffff;
        const php_hash_ops *ops;

        /* hash */
        if (ra->algorithm && (ops = redis_hash_fetch_ops(ra->algorithm))) {
            void *ctx = emalloc(ops->context_size);
            unsigned char *digest = emalloc(ops->digest_size);

#if PHP_VERSION_ID >= 80100
            ops->hash_init(ctx,NULL);
#else
            ops->hash_init(ctx);
#endif
            ops->hash_update(ctx, (const unsigned char *)ZSTR_VAL(out), ZSTR_LEN(out));
            ops->hash_final(digest, ctx);

            memcpy(&ret, digest, MIN(sizeof(ret), ops->digest_size));
            ret %= 0xffffffff;

            efree(digest);
            efree(ctx);
        } else {
            for (i = 0; i < ZSTR_LEN(out); ++i) {
                CRC32(ret, ZSTR_VAL(out)[i]);
            }
        }

        /* get position on ring */
        if (ra->continuum) {
            int left = 0, right = ra->continuum->nb_points;
            while (left < right) {
                i = (int)((left + right) / 2);
                if (ra->continuum->points[i].value < ret) {
                    left = i + 1;
                } else {
                    right = i;
                }
            }
            if (right == ra->continuum->nb_points) {
                right = 0;
            }
            pos = ra->continuum->points[right].index;
        } else {
            pos = (int)((ret ^ 0xffffffff) * ra->count / 0xffffffff);
        }
    } else {
        pos = ra_call_distributor(ra, key, key_len);
        if (pos < 0 || pos >= ra->count) {
            zend_string_release(out);
            return NULL;
        }
    }
    zend_string_release(out);

    if(out_pos) *out_pos = pos;

    return &ra->redis[pos];
}

zval *
ra_find_node_by_name(RedisArray *ra, zend_string *host) {

    int i;
    for(i = 0; i < ra->count; ++i) {
        if (zend_string_equals(host, ra->hosts[i])) {
            return &ra->redis[i];
        }
    }
    return NULL;
}

void
ra_index_multi(zval *z_redis, long multi_value) {

    zval z_fun_multi, z_ret;
    zval z_args[1];

    /* run MULTI */
    ZVAL_STRINGL(&z_fun_multi, "MULTI", 5);
    ZVAL_LONG(&z_args[0], multi_value);
    call_user_function(&redis_ce->function_table, z_redis, &z_fun_multi, &z_ret, 1, z_args);
    zval_dtor(&z_fun_multi);
    zval_dtor(&z_ret);
}

static void
ra_index_change_keys(const char *cmd, zval *z_keys, zval *z_redis) {

    int i, argc;
    zval z_fun, z_ret, *z_args;

    /* alloc */
    argc = 1 + zend_hash_num_elements(Z_ARRVAL_P(z_keys));
    z_args = ecalloc(argc, sizeof(zval));

    /* prepare first parameters */
    ZVAL_STRING(&z_fun, cmd);
    ZVAL_STRINGL(&z_args[0], PHPREDIS_INDEX_NAME, sizeof(PHPREDIS_INDEX_NAME) - 1);

    /* prepare keys */
    for(i = 0; i < argc - 1; ++i) {
        zval *zv = zend_hash_index_find(Z_ARRVAL_P(z_keys), i);
        if (zv == NULL) {
            ZVAL_NULL(&z_args[i+1]);
        } else {
            z_args[i+1] = *zv;
        }
    }

    /* run cmd */
    call_user_function(&redis_ce->function_table, z_redis, &z_fun, &z_ret, argc, z_args);

    zval_dtor(&z_args[0]);
    zval_dtor(&z_fun);
    zval_dtor(&z_ret);
    efree(z_args);      /* free container */
}

void
ra_index_del(zval *z_keys, zval *z_redis) {
    ra_index_change_keys("SREM", z_keys, z_redis);
}

void
ra_index_keys(zval *z_pairs, zval *z_redis) {

    zval z_keys, *z_val;
    zend_string *zkey;
    zend_ulong idx;

    /* Initialize key array */
    array_init_size(&z_keys, zend_hash_num_elements(Z_ARRVAL_P(z_pairs)));

    /* Go through input array and add values to the key array */
    ZEND_HASH_FOREACH_KEY_VAL(Z_ARRVAL_P(z_pairs), idx, zkey, z_val) {
        zval z_new;

        PHPREDIS_NOTUSED(z_val);

        if (zkey) {
            ZVAL_STRINGL(&z_new, ZSTR_VAL(zkey), ZSTR_LEN(zkey));
        } else {
            ZVAL_LONG(&z_new, idx);
        }
        zend_hash_next_index_insert(Z_ARRVAL(z_keys), &z_new);
    } ZEND_HASH_FOREACH_END();

    /* add keys to index */
    ra_index_change_keys("SADD", &z_keys, z_redis);

    /* cleanup */
    zval_dtor(&z_keys);
}

void
ra_index_key(const char *key, int key_len, zval *z_redis) {

    zval z_fun_sadd, z_ret, z_args[2];

    /* prepare args */
    ZVAL_STRINGL(&z_fun_sadd, "SADD", 4);

    ZVAL_STRINGL(&z_args[0], PHPREDIS_INDEX_NAME, sizeof(PHPREDIS_INDEX_NAME) - 1);
    ZVAL_STRINGL(&z_args[1], key, key_len);

    /* run SADD */
    call_user_function(&redis_ce->function_table, z_redis, &z_fun_sadd, &z_ret, 2, z_args);
    zval_dtor(&z_fun_sadd);
    zval_dtor(&z_args[1]);
    zval_dtor(&z_args[0]);
    zval_dtor(&z_ret);
}

void
ra_index_exec(zval *z_redis, zval *return_value, int keep_all) {

    zval z_fun_exec, z_ret, *zp_tmp;

    /* run EXEC */
    ZVAL_STRINGL(&z_fun_exec, "EXEC", 4);
    call_user_function(&redis_ce->function_table, z_redis, &z_fun_exec, &z_ret, 0, NULL);
    zval_dtor(&z_fun_exec);

    /* extract first element of exec array and put into return_value. */
    if(Z_TYPE(z_ret) == IS_ARRAY) {
        if(return_value) {
                if(keep_all) {
                    zp_tmp = &z_ret;
                    RETVAL_ZVAL(zp_tmp, 1, 0);
                } else if ((zp_tmp = zend_hash_index_find(Z_ARRVAL(z_ret), 0)) != NULL) {
                    RETVAL_ZVAL(zp_tmp, 1, 0);
                }
        }
    }
    zval_dtor(&z_ret);

    /* zval *zptr = &z_ret; */
    /* php_var_dump(&zptr, 0); */
}

void
ra_index_discard(zval *z_redis, zval *return_value) {

    zval z_fun_discard, z_ret;

    /* run DISCARD */
    ZVAL_STRINGL(&z_fun_discard, "DISCARD", 7);
    call_user_function(&redis_ce->function_table, z_redis, &z_fun_discard, &z_ret, 0, NULL);

    zval_dtor(&z_fun_discard);
    zval_dtor(&z_ret);
}

void
ra_index_unwatch(zval *z_redis, zval *return_value) {

    zval z_fun_unwatch, z_ret;

    /* run UNWATCH */
    ZVAL_STRINGL(&z_fun_unwatch, "UNWATCH", 7);
    call_user_function(&redis_ce->function_table, z_redis, &z_fun_unwatch, &z_ret, 0, NULL);

    zval_dtor(&z_fun_unwatch);
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

    ret = zend_hash_str_exists(ra->pure_cmds, cmd_up, cmd_len);

    efree(cmd_up);
    return !ret;
}

/* run TYPE to find the type */
static zend_bool
ra_get_key_type(zval *z_redis, const char *key, int key_len, zval *z_from, long *res) {

    int i = 0;
    zval z_fun, z_ret, z_arg, *z_data;
    long success = 1;

    /* Pipelined */
    ra_index_multi(z_from, PIPELINE);

    /* prepare args */
    ZVAL_STRINGL(&z_arg, key, key_len);

    /* run TYPE */
    ZVAL_NULL(&z_ret);
    ZVAL_STRINGL(&z_fun, "TYPE", 4);
    call_user_function(&redis_ce->function_table, z_redis, &z_fun, &z_ret, 1, &z_arg);
    zval_dtor(&z_fun);
    zval_dtor(&z_ret);

    /* run TYPE */
    ZVAL_NULL(&z_ret);
    ZVAL_STRINGL(&z_fun, "TTL", 3);
    call_user_function(&redis_ce->function_table, z_redis, &z_fun, &z_ret, 1, &z_arg);
    zval_dtor(&z_fun);
    zval_dtor(&z_ret);

    /* Get the result from the pipeline. */
    ra_index_exec(z_from, &z_ret, 1);
    if (Z_TYPE(z_ret) == IS_ARRAY) {
        ZEND_HASH_FOREACH_VAL(Z_ARRVAL(z_ret), z_data) {
            if (z_data == NULL || Z_TYPE_P(z_data) != IS_LONG) {
                success = 0;
                break;
            }
            /* Get the result - Might change in the future to handle doubles as well */
            res[i++] = Z_LVAL_P(z_data);
        } ZEND_HASH_FOREACH_END();
    }
    zval_dtor(&z_arg);
    zval_dtor(&z_ret);
    return success;
}

/* delete key from source server index during rehashing */
static void
ra_remove_from_index(zval *z_redis, const char *key, int key_len) {

    zval z_fun_srem, z_ret, z_args[2];

    /* run SREM on source index */
    ZVAL_STRINGL(&z_fun_srem, "SREM", 4);
    ZVAL_STRINGL(&z_args[0], PHPREDIS_INDEX_NAME, sizeof(PHPREDIS_INDEX_NAME) - 1);
    ZVAL_STRINGL(&z_args[1], key, key_len);

    call_user_function(&redis_ce->function_table, z_redis, &z_fun_srem, &z_ret, 2, z_args);

    /* cleanup */
    zval_dtor(&z_fun_srem);
    zval_dtor(&z_args[1]);
    zval_dtor(&z_args[0]);
    zval_dtor(&z_ret);
}


/* delete key from source server during rehashing */
static zend_bool
ra_del_key(const char *key, int key_len, zval *z_from) {

    zval z_fun_del, z_ret, z_args[1];

    /* in a transaction */
    ra_index_multi(z_from, MULTI);

    /* run DEL on source */
    ZVAL_STRINGL(&z_fun_del, "DEL", 3);
    ZVAL_STRINGL(&z_args[0], key, key_len);
    call_user_function(&redis_ce->function_table, z_from, &z_fun_del, &z_ret, 1, z_args);
    zval_dtor(&z_fun_del);
    zval_dtor(&z_args[0]);
    zval_dtor(&z_ret);

    /* remove key from index */
    ra_remove_from_index(z_from, key, key_len);

    /* close transaction */
    ra_index_exec(z_from, NULL, 0);

    return 1;
}

static zend_bool
ra_expire_key(const char *key, int key_len, zval *z_to, long ttl) {

    zval z_fun_expire, z_ret, z_args[2];

    if (ttl > 0)
    {
        /* run EXPIRE on target */
        ZVAL_STRINGL(&z_fun_expire, "EXPIRE", 6);
        ZVAL_STRINGL(&z_args[0], key, key_len);
        ZVAL_LONG(&z_args[1], ttl);
        call_user_function(&redis_ce->function_table, z_to, &z_fun_expire, &z_ret, 2, z_args);
        zval_dtor(&z_fun_expire);
        zval_dtor(&z_args[0]);
        zval_dtor(&z_ret);
    }

    return 1;
}

static zend_bool
ra_move_zset(const char *key, int key_len, zval *z_from, zval *z_to, long ttl) {

    zval z_fun_zrange, z_fun_zadd, z_ret, z_ret_dest, z_args[4], *z_zadd_args, *z_score_p;
    int i, count;
    HashTable *h_zset_vals;
    zend_string *zkey;
    zend_ulong idx;

    /* run ZRANGE key 0 -1 WITHSCORES on source */
    ZVAL_STRINGL(&z_fun_zrange, "ZRANGE", 6);
    ZVAL_STRINGL(&z_args[0], key, key_len);
    ZVAL_STRINGL(&z_args[1], "0", 1);
    ZVAL_STRINGL(&z_args[2], "-1", 2);
    ZVAL_BOOL(&z_args[3], 1);
    call_user_function(&redis_ce->function_table, z_from, &z_fun_zrange, &z_ret, 4, z_args);
    zval_dtor(&z_fun_zrange);
    zval_dtor(&z_args[2]);
    zval_dtor(&z_args[1]);
    zval_dtor(&z_args[0]);


    if(Z_TYPE(z_ret) != IS_ARRAY) { /* key not found or replaced */
        /* TODO: report? */
        zval_dtor(&z_ret);
        return 0;
    }

    /* we now have an array of value → score pairs in z_ret. */
    h_zset_vals = Z_ARRVAL(z_ret);

    /* allocate argument array for ZADD */
    count = zend_hash_num_elements(h_zset_vals);
    z_zadd_args = ecalloc((1 + 2*count), sizeof(zval));

    ZVAL_STRINGL(&z_zadd_args[0], key, key_len);

    i = 1;
    ZEND_HASH_FOREACH_KEY_VAL(h_zset_vals, idx, zkey, z_score_p) {
        /* add score */
        ZVAL_DOUBLE(&z_zadd_args[i], Z_DVAL_P(z_score_p));

        /* add value */
        if (zkey) {
            ZVAL_STRINGL(&z_zadd_args[i+1], ZSTR_VAL(zkey), ZSTR_LEN(zkey));
        } else {
            ZVAL_LONG(&z_zadd_args[i+1], (long)idx);
        }
        i += 2;
    } ZEND_HASH_FOREACH_END();

    /* run ZADD on target */
    ZVAL_STRINGL(&z_fun_zadd, "ZADD", 4);
    call_user_function(&redis_ce->function_table, z_to, &z_fun_zadd, &z_ret_dest, 1 + 2 * count, z_zadd_args);

    /* Expire if needed */
    ra_expire_key(key, key_len, z_to, ttl);

    /* cleanup */
    zval_dtor(&z_fun_zadd);
    zval_dtor(&z_ret_dest);
    zval_dtor(&z_ret);

    /* Free the array itself */
    for (i = 0; i < 1 + 2 * count; i++) {
        zval_dtor(&z_zadd_args[i]);
    }
    efree(z_zadd_args);

    return 1;
}

static zend_bool
ra_move_string(const char *key, int key_len, zval *z_from, zval *z_to, long ttl) {

    zval z_fun_get, z_fun_set, z_ret, z_args[3];

    /* run GET on source */
    ZVAL_STRINGL(&z_fun_get, "GET", 3);
    ZVAL_STRINGL(&z_args[0], key, key_len);
    call_user_function(&redis_ce->function_table, z_from, &z_fun_get, &z_ret, 1, z_args);
    zval_dtor(&z_fun_get);

    if(Z_TYPE(z_ret) != IS_STRING) { /* key not found or replaced */
        /* TODO: report? */
        zval_dtor(&z_args[0]);
        zval_dtor(&z_ret);
        return 0;
    }

    /* run SET on target */
    if (ttl > 0) {
        ZVAL_STRINGL(&z_fun_set, "SETEX", 5);
        ZVAL_LONG(&z_args[1], ttl);
        ZVAL_STRINGL(&z_args[2], Z_STRVAL(z_ret), Z_STRLEN(z_ret)); /* copy z_ret to arg 1 */
        zval_dtor(&z_ret); /* free memory from our previous call */
        call_user_function(&redis_ce->function_table, z_to, &z_fun_set, &z_ret, 3, z_args);
        /* cleanup */
        zval_dtor(&z_args[2]);
    } else {
        ZVAL_STRINGL(&z_fun_set, "SET", 3);
        ZVAL_STRINGL(&z_args[1], Z_STRVAL(z_ret), Z_STRLEN(z_ret)); /* copy z_ret to arg 1 */
        zval_dtor(&z_ret); /* free memory from our previous return value */
        call_user_function(&redis_ce->function_table, z_to, &z_fun_set, &z_ret, 2, z_args);
        /* cleanup */
        zval_dtor(&z_args[1]);
    }
    zval_dtor(&z_fun_set);
    zval_dtor(&z_args[0]);
    zval_dtor(&z_ret);

    return 1;
}

static zend_bool
ra_move_hash(const char *key, int key_len, zval *z_from, zval *z_to, long ttl) {
    zval z_fun_hgetall, z_fun_hmset, z_ret_dest, z_args[2];

    /* run HGETALL on source */
    ZVAL_STRINGL(&z_args[0], key, key_len);
    ZVAL_STRINGL(&z_fun_hgetall, "HGETALL", 7);
    call_user_function(&redis_ce->function_table, z_from, &z_fun_hgetall, &z_args[1], 1, z_args);
    zval_dtor(&z_fun_hgetall);

    if (Z_TYPE(z_args[1]) != IS_ARRAY) { /* key not found or replaced */
        /* TODO: report? */
        zval_dtor(&z_args[1]);
        zval_dtor(&z_args[0]);
        return 0;
    }

    /* run HMSET on target */
    ZVAL_STRINGL(&z_fun_hmset, "HMSET", 5);
    call_user_function(&redis_ce->function_table, z_to, &z_fun_hmset, &z_ret_dest, 2, z_args);
    zval_dtor(&z_fun_hmset);
    zval_dtor(&z_ret_dest);

    /* Expire if needed */
    ra_expire_key(key, key_len, z_to, ttl);

    /* cleanup */
    zval_dtor(&z_args[1]);
    zval_dtor(&z_args[0]);

    return 1;
}

static zend_bool
ra_move_collection(const char *key, int key_len, zval *z_from, zval *z_to,
        int list_count, const char **cmd_list,
        int add_count, const char **cmd_add, long ttl) {

    zval z_fun_retrieve, z_fun_sadd, z_ret, *z_retrieve_args, *z_sadd_args, *z_data_p;
    int count, i;
    HashTable *h_set_vals;

    /* run retrieval command on source */
    ZVAL_STRING(&z_fun_retrieve, cmd_list[0]);  /* set the command */

    z_retrieve_args = ecalloc(list_count, sizeof(zval));
    /* set the key */
    ZVAL_STRINGL(&z_retrieve_args[0], key, key_len);

    /* possibly add some other args if they were provided. */
    for(i = 1; i < list_count; ++i) {
        ZVAL_STRING(&z_retrieve_args[i], cmd_list[i]);
    }

    call_user_function(&redis_ce->function_table, z_from, &z_fun_retrieve, &z_ret, list_count, z_retrieve_args);

    /* cleanup */
    zval_dtor(&z_fun_retrieve);
    for(i = 0; i < list_count; ++i) {
        zval_dtor(&z_retrieve_args[i]);
    }
    efree(z_retrieve_args);

    if(Z_TYPE(z_ret) != IS_ARRAY) { /* key not found or replaced */
        /* TODO: report? */
        zval_dtor(&z_ret);
        return 0;
    }

    /* run SADD/RPUSH on target */
    h_set_vals = Z_ARRVAL(z_ret);
    count = 1 + zend_hash_num_elements(h_set_vals);
    ZVAL_STRING(&z_fun_sadd, cmd_add[0]);
    z_sadd_args = ecalloc(count, sizeof(zval));
    ZVAL_STRINGL(&z_sadd_args[0], key, key_len);

    i = 1;
    ZEND_HASH_FOREACH_VAL(h_set_vals, z_data_p) {
        /* add set elements */
        ZVAL_ZVAL(&z_sadd_args[i], z_data_p, 1, 0);
        i++;
    } ZEND_HASH_FOREACH_END();

    /* Clean up our input return value */
    zval_dtor(&z_ret);

    call_user_function(&redis_ce->function_table, z_to, &z_fun_sadd, &z_ret, count, z_sadd_args);

    /* cleanup */
    zval_dtor(&z_fun_sadd);
    for (i = 0; i < count; i++) {
        zval_dtor(&z_sadd_args[i]);
    }
    efree(z_sadd_args);

    /* Clean up our output return value */
    zval_dtor(&z_ret);

    /* Expire if needed */
    ra_expire_key(key, key_len, z_to, ttl);

    return 1;
}

static zend_bool
ra_move_set(const char *key, int key_len, zval *z_from, zval *z_to, long ttl) {

    const char *cmd_list[] = {"SMEMBERS"};
    const char *cmd_add[] = {"SADD"};
    return ra_move_collection(key, key_len, z_from, z_to, 1, cmd_list, 1, cmd_add, ttl);
}

static zend_bool
ra_move_list(const char *key, int key_len, zval *z_from, zval *z_to, long ttl) {

    const char *cmd_list[] = {"LRANGE", "0", "-1"};
    const char *cmd_add[] = {"RPUSH"};
    return ra_move_collection(key, key_len, z_from, z_to, 3, cmd_list, 1, cmd_add, ttl);
}

void
ra_move_key(const char *key, int key_len, zval *z_from, zval *z_to) {

    long res[2] = {0}, type, ttl;
    zend_bool success = 0;
    if (ra_get_key_type(z_from, key, key_len, z_from, res)) {
        type = res[0];
        ttl = res[1];
        /* open transaction on target server */
        ra_index_multi(z_to, MULTI);
        switch(type) {
            case REDIS_STRING:
                success = ra_move_string(key, key_len, z_from, z_to, ttl);
                break;

            case REDIS_SET:
                success = ra_move_set(key, key_len, z_from, z_to, ttl);
                break;

            case REDIS_LIST:
                success = ra_move_list(key, key_len, z_from, z_to, ttl);
                break;

            case REDIS_ZSET:
                success = ra_move_zset(key, key_len, z_from, z_to, ttl);
                break;

            case REDIS_HASH:
                success = ra_move_hash(key, key_len, z_from, z_to, ttl);
                break;

            default:
                /* TODO: report? */
                break;
        }
    }

    if(success) {
        ra_del_key(key, key_len, z_from);
        ra_index_key(key, key_len, z_to);
    }

    /* close transaction */
    ra_index_exec(z_to, NULL, 0);
}

/* callback with the current progress, with hostname and count */
static void
zval_rehash_callback(zend_fcall_info *z_cb, zend_fcall_info_cache *z_cb_cache,
    zend_string *hostname, long count) {

    zval zv, *z_ret = &zv;

    ZVAL_NULL(z_ret);

    zval z_args[2];

    ZVAL_STRINGL(&z_args[0], ZSTR_VAL(hostname), ZSTR_LEN(hostname));
    ZVAL_LONG(&z_args[1], count);

    z_cb->params = z_args;
    z_cb->retval = z_ret;

    z_cb->param_count = 2;

    /* run cb(hostname, count) */
    zend_call_function(z_cb, z_cb_cache);

    /* cleanup */
    zval_dtor(&z_args[0]);
    zval_dtor(z_ret);
}

static void
ra_rehash_server(RedisArray *ra, zval *z_redis, zend_string *hostname, zend_bool b_index,
        zend_fcall_info *z_cb, zend_fcall_info_cache *z_cb_cache) {

    HashTable *h_keys;
    long count = 0;
    zval z_fun, z_ret, z_argv, *z_ele;

    /* list all keys */
    if (b_index) {
        ZVAL_STRING(&z_fun, "SMEMBERS");
        ZVAL_STRING(&z_argv, PHPREDIS_INDEX_NAME);
    } else {
        ZVAL_STRING(&z_fun, "KEYS");
        ZVAL_STRING(&z_argv, "*");
    }
    ZVAL_NULL(&z_ret);
    call_user_function(&redis_ce->function_table, z_redis, &z_fun, &z_ret, 1, &z_argv);
    zval_dtor(&z_argv);
    zval_dtor(&z_fun);

    if (Z_TYPE(z_ret) == IS_ARRAY) {
        h_keys = Z_ARRVAL(z_ret);
        count = zend_hash_num_elements(h_keys);
    }

    if (!count) {
        zval_dtor(&z_ret);
        return;
    }

    /* callback */
    if(z_cb && z_cb_cache) {
        zval_rehash_callback(z_cb, z_cb_cache, hostname, count);
    }

    /* for each key, redistribute */
    ZEND_HASH_FOREACH_VAL(h_keys, z_ele) {
        int pos = 0;
        /* check that we're not moving to the same node. */
        zval *z_target = ra_find_node(ra, Z_STRVAL_P(z_ele), Z_STRLEN_P(z_ele), &pos);

        if (z_target && !zend_string_equals(hostname, ra->hosts[pos])) { /* different host */
            ra_move_key(Z_STRVAL_P(z_ele), Z_STRLEN_P(z_ele), z_redis, z_target);
        }

    } ZEND_HASH_FOREACH_END();

    /* cleanup */
    zval_dtor(&z_ret);
}

void
ra_rehash(RedisArray *ra, zend_fcall_info *z_cb, zend_fcall_info_cache *z_cb_cache) {
    int i;

    /* redistribute the data, server by server. */
    if(!ra->prev)
        return; /* TODO: compare the two rings for equality */

    for(i = 0; i < ra->prev->count; ++i) {
        ra_rehash_server(ra, &ra->prev->redis[i], ra->prev->hosts[i], ra->index, z_cb, z_cb_cache);
    }
}

