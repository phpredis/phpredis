/* -*- Mode: C; tab-width: 4 -*- */
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
  | Original author: Alfonso Jimenez <yo@alfonsojimenez.com>             |
  | Maintainer: Nicolas Favre-Felix <n.favre-felix@owlient.eu>           |
  | Maintainer: Nasreddine Bouafif <n.bouafif@owlient.eu>                |
  | Maintainer: Michael Grunder <michael.grunder@gmail.com>              |
  +----------------------------------------------------------------------+
*/

#include "common.h"

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef PHP_SESSION
#include "ext/standard/info.h"
#include "php_redis.h"
#include "redis_session.h"
#include <zend_exceptions.h>

#include "library.h"
#include "cluster_library.h"

#include "php.h"
#include "php_ini.h"
#include "php_variables.h"
#include "SAPI.h"
#include "ext/standard/url.h"

ps_module ps_mod_redis = {
    PS_MOD(redis)
};
ps_module ps_mod_redis_cluster = {
    PS_MOD(rediscluster)
};

typedef struct redis_pool_member_ {

    RedisSock *redis_sock;
    int weight;
    int database;

    char *prefix;
    size_t prefix_len;

    char *auth;
    size_t auth_len;

    struct redis_pool_member_ *next;

} redis_pool_member;

typedef struct {

    int totalWeight;
    int count;

    redis_pool_member *head;

} redis_pool;

PHP_REDIS_API redis_pool*
redis_pool_new(TSRMLS_D) {
    return ecalloc(1, sizeof(redis_pool));
}

PHP_REDIS_API void
redis_pool_add(redis_pool *pool, RedisSock *redis_sock, int weight,
                int database, char *prefix, char *auth TSRMLS_DC) {

    redis_pool_member *rpm = ecalloc(1, sizeof(redis_pool_member));
    rpm->redis_sock = redis_sock;
    rpm->weight = weight;
    rpm->database = database;

    rpm->prefix = prefix;
    rpm->prefix_len = (prefix?strlen(prefix):0);

    rpm->auth = auth;
    rpm->auth_len = (auth?strlen(auth):0);

    rpm->next = pool->head;
    pool->head = rpm;

    pool->totalWeight += weight;
}

PHP_REDIS_API void
redis_pool_free(redis_pool *pool TSRMLS_DC) {

    redis_pool_member *rpm, *next;
    rpm = pool->head;
    while(rpm) {
        next = rpm->next;
        redis_sock_disconnect(rpm->redis_sock TSRMLS_CC);
        redis_free_socket(rpm->redis_sock);
        if(rpm->prefix) efree(rpm->prefix);
        if(rpm->auth) efree(rpm->auth);
        efree(rpm);
        rpm = next;
    }
    efree(pool);
}

static void
redis_pool_member_auth(redis_pool_member *rpm TSRMLS_DC) {
    RedisSock *redis_sock = rpm->redis_sock;
    char *response, *cmd;
    int response_len, cmd_len;

    /* Short circuit if we don't have a password */
    if(!rpm->auth || !rpm->auth_len) {
        return;
    }

    cmd_len = REDIS_SPPRINTF(&cmd, "AUTH", "s", rpm->auth, rpm->auth_len);
    if(redis_sock_write(redis_sock, cmd, cmd_len TSRMLS_CC) >= 0) {
        if ((response = redis_sock_read(redis_sock, &response_len TSRMLS_CC))) {
            efree(response);
        }
    }
    efree(cmd);
}

static void
redis_pool_member_select(redis_pool_member *rpm TSRMLS_DC) {
    RedisSock *redis_sock = rpm->redis_sock;
    char *response, *cmd;
    int response_len, cmd_len;

    cmd_len = REDIS_SPPRINTF(&cmd, "SELECT", "d", rpm->database);
    if(redis_sock_write(redis_sock, cmd, cmd_len TSRMLS_CC) >= 0) {
        if ((response = redis_sock_read(redis_sock, &response_len TSRMLS_CC))) {
            efree(response);
        }
    }
    efree(cmd);
}

PHP_REDIS_API redis_pool_member *
redis_pool_get_sock(redis_pool *pool, const char *key TSRMLS_DC) {

    unsigned int pos, i;
    memcpy(&pos, key, sizeof(pos));
    pos %= pool->totalWeight;

    redis_pool_member *rpm = pool->head;

    for(i = 0; i < pool->totalWeight;) {
        if(pos >= i && pos < i + rpm->weight) {
            int needs_auth = 0;
            if(rpm->auth && rpm->auth_len && rpm->redis_sock->status != REDIS_SOCK_STATUS_CONNECTED) {
                    needs_auth = 1;
            }
            redis_sock_server_open(rpm->redis_sock TSRMLS_CC);
            if(needs_auth) {
                redis_pool_member_auth(rpm TSRMLS_CC);
            }
            if(rpm->database >= 0) { /* default is -1 which leaves the choice to redis. */
                redis_pool_member_select(rpm TSRMLS_CC);
            }

            return rpm;
        }
        i += rpm->weight;
        rpm = rpm->next;
    }

    return NULL;
}

/* {{{ PS_OPEN_FUNC
 */
PS_OPEN_FUNC(redis)
{
    php_url *url;
    zval params, *param;
    int i, j, path_len;

    redis_pool *pool = redis_pool_new(TSRMLS_C);

    for (i=0,j=0,path_len=strlen(save_path); i<path_len; i=j+1) {
        /* find beginning of url */
        while (i<path_len && (isspace(save_path[i]) || save_path[i] == ','))
            i++;

        /* find end of url */
        j = i;
        while (j<path_len && !isspace(save_path[j]) && save_path[j] != ',')
             j++;

        if (i < j) {
            int weight = 1;
            double timeout = 86400.0, read_timeout = 0.0;
            int persistent = 0;
            int database = -1;
            char *prefix = NULL, *auth = NULL, *persistent_id = NULL;
            long retry_interval = 0;

            /* translate unix: into file: */
            if (!strncmp(save_path+i, "unix:", sizeof("unix:")-1)) {
                int len = j-i;
                char *path = estrndup(save_path+i, len);
                memcpy(path, "file:", sizeof("file:")-1);
                url = php_url_parse_ex(path, len);
                efree(path);
            } else {
                url = php_url_parse_ex(save_path+i, j-i);
            }

            if (!url) {
                char *path = estrndup(save_path+i, j-i);
                php_error_docref(NULL TSRMLS_CC, E_WARNING,
                    "Failed to parse session.save_path (error at offset %d, url was '%s')", i, path);
                efree(path);

                redis_pool_free(pool TSRMLS_CC);
                PS_SET_MOD_DATA(NULL);
                return FAILURE;
            }

            /* parse parameters */
            if (url->query != NULL) {
                array_init(&params);

                sapi_module.treat_data(PARSE_STRING, estrdup(url->query), &params TSRMLS_CC);

                if ((param = zend_hash_str_find(Z_ARRVAL(params), "weight", sizeof("weight") - 1)) != NULL) {
                    weight = zval_get_long(param);
                }
                if ((param = zend_hash_str_find(Z_ARRVAL(params), "timeout", sizeof("timeout") - 1)) != NULL) {
                    timeout = atof(Z_STRVAL_P(param));
                }
                if ((param = zend_hash_str_find(Z_ARRVAL(params), "read_timeout", sizeof("read_timeout") - 1)) != NULL) {
                    read_timeout = atof(Z_STRVAL_P(param));
                }
                if ((param = zend_hash_str_find(Z_ARRVAL(params), "persistent", sizeof("persistent") - 1)) != NULL) {
                    persistent = (atol(Z_STRVAL_P(param)) == 1 ? 1 : 0);
                }
                if ((param = zend_hash_str_find(Z_ARRVAL(params), "persistent_id", sizeof("persistent_id") - 1)) != NULL) {
                    persistent_id = estrndup(Z_STRVAL_P(param), Z_STRLEN_P(param));
                }
                if ((param = zend_hash_str_find(Z_ARRVAL(params), "prefix", sizeof("prefix") - 1)) != NULL) {
                    prefix = estrndup(Z_STRVAL_P(param), Z_STRLEN_P(param));
                }
                if ((param = zend_hash_str_find(Z_ARRVAL(params), "auth", sizeof("auth") - 1)) != NULL) {
                    auth = estrndup(Z_STRVAL_P(param), Z_STRLEN_P(param));
                }
                if ((param = zend_hash_str_find(Z_ARRVAL(params), "database", sizeof("database") - 1)) != NULL) {
                    database = zval_get_long(param);
                }
                if ((param = zend_hash_str_find(Z_ARRVAL(params), "retry_interval", sizeof("retry_interval") - 1)) != NULL) {
                    retry_interval = zval_get_long(param);
                }

                zval_dtor(&params);
            }

            if ((url->path == NULL && url->host == NULL) || weight <= 0 || timeout <= 0) {
                php_url_free(url);
                if (persistent_id) efree(persistent_id);
                if (prefix) efree(prefix);
                if (auth) efree(auth);
                redis_pool_free(pool TSRMLS_CC);
                PS_SET_MOD_DATA(NULL);
                return FAILURE;
            }

            RedisSock *redis_sock;
            if(url->host) {
                redis_sock = redis_sock_create(url->host, strlen(url->host), url->port, timeout, read_timeout, persistent, persistent_id, retry_interval, 0);
            } else { /* unix */
                redis_sock = redis_sock_create(url->path, strlen(url->path), 0, timeout, read_timeout, persistent, persistent_id, retry_interval, 0);
            }
            redis_pool_add(pool, redis_sock, weight, database, prefix, auth TSRMLS_CC);

            php_url_free(url);
        }
    }

    if (pool->head) {
        PS_SET_MOD_DATA(pool);
        return SUCCESS;
    }

    return FAILURE;
}
/* }}} */

/* {{{ PS_CLOSE_FUNC
 */
PS_CLOSE_FUNC(redis)
{
    redis_pool *pool = PS_GET_MOD_DATA();

    if(pool){
        redis_pool_free(pool TSRMLS_CC);
        PS_SET_MOD_DATA(NULL);
    }
    return SUCCESS;
}
/* }}} */

static char *
redis_session_key(redis_pool_member *rpm, const char *key, int key_len, int *session_len) {

    char *session;
    char default_prefix[] = "PHPREDIS_SESSION:";
    char *prefix = default_prefix;
    size_t prefix_len = sizeof(default_prefix)-1;

    if(rpm->prefix) {
        prefix = rpm->prefix;
        prefix_len = rpm->prefix_len;
    }

    /* build session key */
    *session_len = key_len + prefix_len;
    session = emalloc(*session_len);
    memcpy(session, prefix, prefix_len);
    memcpy(session + prefix_len, key, key_len);

    return session;
}

/* {{{ PS_READ_FUNC
 */
PS_READ_FUNC(redis)
{
    char *resp, *cmd;
    int resp_len, cmd_len;

    redis_pool *pool = PS_GET_MOD_DATA();
#if (PHP_MAJOR_VERSION < 7)
    redis_pool_member *rpm = redis_pool_get_sock(pool, key TSRMLS_CC);
#else
    redis_pool_member *rpm = redis_pool_get_sock(pool, key->val TSRMLS_CC);
#endif
    RedisSock *redis_sock = rpm?rpm->redis_sock:NULL;
    if(!rpm || !redis_sock){
        return FAILURE;
    }

    /* send GET command */
#if (PHP_MAJOR_VERSION < 7)
    resp = redis_session_key(rpm, key, strlen(key), &resp_len);
#else
    resp = redis_session_key(rpm, key->val, key->len, &resp_len);
#endif
    cmd_len = REDIS_SPPRINTF(&cmd, "GET", "s", resp, resp_len);

    efree(resp);
    if(redis_sock_write(redis_sock, cmd, cmd_len TSRMLS_CC) < 0) {
        efree(cmd);
        return FAILURE;
    }
    efree(cmd);

    /* Read response from Redis.  If we get a NULL response from redis_sock_read
     * this can indicate an error, OR a "NULL bulk" reply (empty session data)
     * in which case we can reply with success. */
    if ((resp = redis_sock_read(redis_sock, &resp_len TSRMLS_CC)) == NULL && resp_len != -1) {
        return FAILURE;
    }
#if (PHP_MAJOR_VERSION < 7)
    if (resp_len < 0) {
        *val = STR_EMPTY_ALLOC();
        *vallen = 0;
    } else {
        *val = resp;
        *vallen = resp_len;
    }
#else
    if (resp_len < 0) {
        *val = ZSTR_EMPTY_ALLOC();
    } else {
        *val = zend_string_init(resp, resp_len, 0);
    }
    efree(resp);
#endif

    return SUCCESS;
}
/* }}} */

/* {{{ PS_WRITE_FUNC
 */
PS_WRITE_FUNC(redis)
{
    char *cmd, *response, *session;
    int cmd_len, response_len, session_len;

    redis_pool *pool = PS_GET_MOD_DATA();
#if (PHP_MAJOR_VERSION < 7)
    redis_pool_member *rpm = redis_pool_get_sock(pool, key TSRMLS_CC);
#else
    redis_pool_member *rpm = redis_pool_get_sock(pool, key->val TSRMLS_CC);
#endif
    RedisSock *redis_sock = rpm?rpm->redis_sock:NULL;
    if(!rpm || !redis_sock){
        return FAILURE;
    }

    /* send SET command */
#if (PHP_MAJOR_VERSION < 7)
    session = redis_session_key(rpm, key, strlen(key), &session_len);
    cmd_len = REDIS_SPPRINTF(&cmd, "SETEX", "sds", session, session_len,
                             INI_INT("session.gc_maxlifetime"), val, vallen);
#else
    session = redis_session_key(rpm, key->val, key->len, &session_len);
    cmd_len = REDIS_SPPRINTF(&cmd, "SETEX", "sds", session, session_len,
                             INI_INT("session.gc_maxlifetime"), val->val,
                             val->len);
#endif
    efree(session);
    if(redis_sock_write(redis_sock, cmd, cmd_len TSRMLS_CC) < 0) {
        efree(cmd);
        return FAILURE;
    }
    efree(cmd);

    /* read response */
    if ((response = redis_sock_read(redis_sock, &response_len TSRMLS_CC)) == NULL) {
        return FAILURE;
    }

    if(response_len == 3 && strncmp(response, "+OK", 3) == 0) {
        efree(response);
        return SUCCESS;
    } else {
        efree(response);
        return FAILURE;
    }
}
/* }}} */

/* {{{ PS_DESTROY_FUNC
 */
PS_DESTROY_FUNC(redis)
{
    char *cmd, *response, *session;
    int cmd_len, response_len, session_len;

    redis_pool *pool = PS_GET_MOD_DATA();
#if (PHP_MAJOR_VERSION < 7)
    redis_pool_member *rpm = redis_pool_get_sock(pool, key TSRMLS_CC);
#else
    redis_pool_member *rpm = redis_pool_get_sock(pool, key->val TSRMLS_CC);
#endif
    RedisSock *redis_sock = rpm?rpm->redis_sock:NULL;
    if(!rpm || !redis_sock){
        return FAILURE;
    }

    /* send DEL command */
#if (PHP_MAJOR_VERSION < 7)
    session = redis_session_key(rpm, key, strlen(key), &session_len);
#else
    session = redis_session_key(rpm, key->val, key->len, &session_len);
#endif
    cmd_len = REDIS_SPPRINTF(&cmd, "DEL", "s", session, session_len);
    efree(session);
    if(redis_sock_write(redis_sock, cmd, cmd_len TSRMLS_CC) < 0) {
        efree(cmd);
        return FAILURE;
    }
    efree(cmd);

    /* read response */
    if ((response = redis_sock_read(redis_sock, &response_len TSRMLS_CC)) == NULL) {
        return FAILURE;
    }

    if(response_len == 2 && response[0] == ':' && (response[1] == '0' || response[1] == '1')) {
        efree(response);
        return SUCCESS;
    } else {
        efree(response);
        return FAILURE;
    }
}
/* }}} */

/* {{{ PS_GC_FUNC
 */
PS_GC_FUNC(redis)
{
    return SUCCESS;
}
/* }}} */

/**
 * Redis Cluster session handler functions
 */

/* Helper to extract timeout values */
static void session_conf_timeout(HashTable *ht_conf, const char *key, int key_len,
                                 double *val)
{
    zval *z_val;

    if ((z_val = zend_hash_str_find(ht_conf, key, key_len - 1)) != NULL &&
        Z_TYPE_P(z_val) == IS_STRING
    ) {
        *val = atof(Z_STRVAL_P(z_val));
    }
}

/* Simple helper to retreive a boolean (0 or 1) value from a string stored in our
 * session.save_path variable.  This is so the user can use 0, 1, or 'true', 
 * 'false' */
static void session_conf_bool(HashTable *ht_conf, char *key, int keylen,
                              int *retval) {
    zval *z_val;
    char *str;
    int strlen;

    /* See if we have the option, and it's a string */   
    if ((z_val = zend_hash_str_find(ht_conf, key, keylen - 1)) != NULL &&
        Z_TYPE_P(z_val) == IS_STRING
    ) {
        str = Z_STRVAL_P(z_val);
        strlen = Z_STRLEN_P(z_val);

        /* true/yes/1 are treated as true.  Everything else is false */
        *retval = (strlen == 4 && !strncasecmp(str, "true", 4)) ||
                  (strlen == 3 && !strncasecmp(str, "yes", 3)) ||
                  (strlen == 1 && !strncasecmp(str, "1", 1));
    }
}

/* Prefix a session key */
static char *cluster_session_key(redisCluster *c, const char *key, int keylen, 
                                 int *skeylen, short *slot) {
    char *skey;

    *skeylen = keylen + c->flags->prefix_len;
    skey = emalloc(*skeylen);
    memcpy(skey, c->flags->prefix, c->flags->prefix_len);
    memcpy(skey + c->flags->prefix_len, key, keylen);
    
    *slot = cluster_hash_key(skey, *skeylen);

    return skey;
}

PS_OPEN_FUNC(rediscluster) {
    redisCluster *c;
    zval z_conf, *z_val;
    HashTable *ht_conf, *ht_seeds;
    double timeout = 0, read_timeout = 0;
    int persistent = 0;
    int retval, prefix_len, failover = REDIS_FAILOVER_NONE;
    char *prefix;

    /* Parse configuration for session handler */
    array_init(&z_conf);
    sapi_module.treat_data(PARSE_STRING, estrdup(save_path), &z_conf TSRMLS_CC);

    /* Sanity check that we're able to parse and have a seeds array */
    if (Z_TYPE(z_conf) != IS_ARRAY ||
        (z_val = zend_hash_str_find(Z_ARRVAL(z_conf), "seed", sizeof("seed") - 1)) == NULL ||
        Z_TYPE_P(z_val) != IS_ARRAY)
    {
        zval_dtor(&z_conf);
        return FAILURE;
    }

    /* Grab a copy of our config hash table and keep seeds array */
    ht_conf = Z_ARRVAL(z_conf);
    ht_seeds = Z_ARRVAL_P(z_val);

    /* Grab timeouts if they were specified */
    session_conf_timeout(ht_conf, "timeout", sizeof("timeout"), &timeout);
    session_conf_timeout(ht_conf, "read_timeout", sizeof("read_timeout"), &read_timeout);

    /* Grab persistent option */
    session_conf_bool(ht_conf, "persistent", sizeof("persistent"), &persistent);
    
    /* Sanity check on our timeouts */
    if (timeout < 0 || read_timeout < 0) {
        php_error_docref(NULL TSRMLS_CC, E_WARNING,
            "Can't set negative timeout values in session configuration");
        zval_dtor(&z_conf);
        return FAILURE;
    }

    /* Look for a specific prefix */
    if ((z_val = zend_hash_str_find(ht_conf, "prefix", sizeof("prefix") - 1)) != NULL &&
        Z_TYPE_P(z_val) == IS_STRING && Z_STRLEN_P(z_val) > 0
    ) {
        prefix = Z_STRVAL_P(z_val);
        prefix_len = Z_STRLEN_P(z_val);
    } else {
        prefix = "PHPREDIS_CLUSTER_SESSION:";
        prefix_len = sizeof("PHPREDIS_CLUSTER_SESSION:")-1;
    }

    /* Look for a specific failover setting */
    if ((z_val = zend_hash_str_find(ht_conf, "failover", sizeof("failover") - 1)) != NULL &&
        Z_TYPE_P(z_val) == IS_STRING
    ) {
        if (!strcasecmp(Z_STRVAL_P(z_val), "error")) {
            failover = REDIS_FAILOVER_ERROR;
        } else if (!strcasecmp(Z_STRVAL_P(z_val), "distribute")) {
            failover = REDIS_FAILOVER_DISTRIBUTE;
        }
    }

    c = cluster_create(timeout, read_timeout, failover, persistent);
    if (!cluster_init_seeds(c, ht_seeds) && !cluster_map_keyspace(c TSRMLS_CC)) {
        /* Set up our prefix */
        c->flags->prefix = estrndup(prefix, prefix_len);
        c->flags->prefix_len = prefix_len;

        PS_SET_MOD_DATA(c);
        retval = SUCCESS;
    } else {
        cluster_free(c);
        retval = FAILURE;
    }

    /* Cleanup */
    zval_dtor(&z_conf);
    
    return retval;
}

/* {{{ PS_READ_FUNC
 */
PS_READ_FUNC(rediscluster) {
    redisCluster *c = PS_GET_MOD_DATA();
    clusterReply *reply;
    char *cmd, *skey;
    int cmdlen, skeylen;
    short slot;

    /* Set up our command and slot information */
#if (PHP_MAJOR_VERSION < 7)
    skey = cluster_session_key(c, key, strlen(key), &skeylen, &slot);
#else
    skey = cluster_session_key(c, key->val, key->len, &skeylen, &slot);
#endif
    cmdlen = redis_spprintf(NULL, NULL TSRMLS_CC, &cmd, "GET", "s", skey, skeylen);
    efree(skey);

    /* Attempt to kick off our command */
    c->readonly = 1;
    if (cluster_send_command(c,slot,cmd,cmdlen TSRMLS_CC)<0 || c->err) {
        efree(cmd);
        return FAILURE;
    }

    /* Clean up command */
    efree(cmd);

    /* Attempt to read reply */
    reply = cluster_read_resp(c TSRMLS_CC);
    if (!reply || c->err) {
        if (reply) cluster_free_reply(reply, 1);
        return FAILURE;
    }

    /* Push reply value to caller */
#if (PHP_MAJOR_VERSION < 7)
    if (reply->str == NULL) {
        *val = STR_EMPTY_ALLOC();
        *vallen = 0;
    } else {
        *val = reply->str;
        *vallen = reply->len;
    }
#else
    if (reply->str == NULL) {
        *val = ZSTR_EMPTY_ALLOC();
    } else {
        *val = zend_string_init(reply->str, reply->len, 0);
    }
#endif

    /* Clean up */
    cluster_free_reply(reply, 0);

    /* Success! */
    return SUCCESS;
}

/* {{{ PS_WRITE_FUNC
 */
PS_WRITE_FUNC(rediscluster) {
    redisCluster *c = PS_GET_MOD_DATA();
    clusterReply *reply;
    char *cmd, *skey;
    int cmdlen, skeylen;
    short slot;

    /* Set up command and slot info */
#if (PHP_MAJOR_VERSION < 7)
    skey = cluster_session_key(c, key, strlen(key), &skeylen, &slot);
    cmdlen = redis_spprintf(NULL, NULL TSRMLS_CC, &cmd, "SETEX", "sds", skey,
                            skeylen, INI_INT("session.gc_maxlifetime"), val,
                            vallen);
#else
    skey = cluster_session_key(c, key->val, key->len, &skeylen, &slot);
    cmdlen = redis_spprintf(NULL, NULL TSRMLS_CC, &cmd, "SETEX", "sds", skey,
                            skeylen, INI_INT("session.gc_maxlifetime"),
                            val->val, val->len);
#endif
    efree(skey);

    /* Attempt to send command */
    c->readonly = 0;
    if (cluster_send_command(c,slot,cmd,cmdlen TSRMLS_CC)<0 || c->err) {
        efree(cmd);
        return FAILURE;
    }

    /* Clean up our command */
    efree(cmd);

    /* Attempt to read reply */
    reply = cluster_read_resp(c TSRMLS_CC);
    if (!reply || c->err) {
        if (reply) cluster_free_reply(reply, 1);
        return FAILURE;
    }

    /* Clean up*/
    cluster_free_reply(reply, 1);

    return SUCCESS;
}

/* {{{ PS_DESTROY_FUNC(rediscluster)
 */
PS_DESTROY_FUNC(rediscluster) {
    redisCluster *c = PS_GET_MOD_DATA();
    clusterReply *reply;
    char *cmd, *skey;
    int cmdlen, skeylen;
    short slot;

    /* Set up command and slot info */
#if (PHP_MAJOR_VERSION < 7)
    skey = cluster_session_key(c, key, strlen(key), &skeylen, &slot);
#else
    skey = cluster_session_key(c, key->val, key->len, &skeylen, &slot);
#endif
    cmdlen = redis_spprintf(NULL, NULL TSRMLS_CC, &cmd, "DEL", "s", skey, skeylen);
    efree(skey);

    /* Attempt to send command */
    if (cluster_send_command(c,slot,cmd,cmdlen TSRMLS_CC)<0 || c->err) {
        efree(cmd);
        return FAILURE;
    }

    /* Clean up our command */
    efree(cmd);

    /* Attempt to read reply */
    reply = cluster_read_resp(c TSRMLS_CC);
    if (!reply || c->err) {
        if (reply) cluster_free_reply(reply, 1);
        return FAILURE; 
    }

    /* Clean up our reply */
    cluster_free_reply(reply, 1);

    return SUCCESS;
}

/* {{{ PS_CLOSE_FUNC
 */
PS_CLOSE_FUNC(rediscluster)
{
    redisCluster *c = PS_GET_MOD_DATA();
    if (c) {
        cluster_free(c);
        PS_SET_MOD_DATA(NULL);
    }
    return SUCCESS;
}

/* {{{ PS_GC_FUNC
 */
PS_GC_FUNC(rediscluster) {
    return SUCCESS;
}

#endif

/* vim: set tabstop=4 expandtab: */
