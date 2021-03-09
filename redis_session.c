/* -*- Mode: C; tab-width: 4 -*- */
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

#define REDIS_SESSION_PREFIX "PHPREDIS_SESSION:"
#define CLUSTER_SESSION_PREFIX "PHPREDIS_CLUSTER_SESSION:"

/* Session lock LUA as well as its SHA1 hash */
#define LOCK_RELEASE_LUA_STR "if redis.call(\"get\",KEYS[1]) == ARGV[1] then return redis.call(\"del\",KEYS[1]) else return 0 end"
#define LOCK_RELEASE_LUA_LEN (sizeof(LOCK_RELEASE_LUA_STR) - 1)
#define LOCK_RELEASE_SHA_STR "b70c2384248f88e6b75b9f89241a180f856ad852"
#define LOCK_RELEASE_SHA_LEN (sizeof(LOCK_RELEASE_SHA_STR) - 1)

/* Check if a response is the Redis +OK status response */
#define IS_REDIS_OK(r, len) (r != NULL && len == 3 && !memcmp(r, "+OK", 3))
#define NEGATIVE_LOCK_RESPONSE 1

#define CLUSTER_DEFAULT_PREFIX() \
    zend_string_init(CLUSTER_SESSION_PREFIX, sizeof(CLUSTER_SESSION_PREFIX) - 1, 0)

ps_module ps_mod_redis = {
    PS_MOD_UPDATE_TIMESTAMP(redis)
};

ps_module ps_mod_redis_cluster = {
    PS_MOD(rediscluster)
};

typedef struct {
    zend_bool is_locked;
    zend_string *session_key;
    zend_string *lock_key;
    zend_string *lock_secret;
} redis_session_lock_status;

typedef struct redis_pool_member_ {

    RedisSock *redis_sock;
    int weight;
    int database;
    struct redis_pool_member_ *next;

} redis_pool_member;

typedef struct {

    int totalWeight;
    int count;

    redis_pool_member *head;
    redis_session_lock_status lock_status;

} redis_pool;

// static char *session_conf_string(HashTable *ht, const char *key, size_t keylen) {
// }

PHP_REDIS_API void
redis_pool_add(redis_pool *pool, RedisSock *redis_sock, int weight, int database)
{
    redis_pool_member *rpm = ecalloc(1, sizeof(redis_pool_member));
    rpm->redis_sock = redis_sock;
    rpm->weight = weight;
    rpm->database = database;

    rpm->next = pool->head;
    pool->head = rpm;

    pool->totalWeight += weight;
}

PHP_REDIS_API void
redis_pool_free(redis_pool *pool) {

    redis_pool_member *rpm, *next;
    rpm = pool->head;
    while (rpm) {
        next = rpm->next;
        redis_sock_disconnect(rpm->redis_sock, 0);
        redis_free_socket(rpm->redis_sock);
        efree(rpm);
        rpm = next;
    }

    /* Cleanup after our lock */
    if (pool->lock_status.session_key) zend_string_release(pool->lock_status.session_key);
    if (pool->lock_status.lock_secret) zend_string_release(pool->lock_status.lock_secret);
    if (pool->lock_status.lock_key) zend_string_release(pool->lock_status.lock_key);

    /* Cleanup pool itself */
    efree(pool);
}

/* Retreive session.gc_maxlifetime from php.ini protecting against an integer overflow */
static int session_gc_maxlifetime() {
    zend_long value = INI_INT("session.gc_maxlifetime");
    if (value > INT_MAX) {
        php_error_docref(NULL, E_NOTICE, "session.gc_maxlifetime overflows INT_MAX, truncating.");
        return INT_MAX;
    } else if (value <= 0) {
        php_error_docref(NULL, E_NOTICE, "session.gc_maxlifetime is <= 0, defaulting to 1440 seconds");
        return 1440;
    }

    return value;
}

/* Send a command to Redis.  Returns byte count written to socket (-1 on failure) */
static int redis_simple_cmd(RedisSock *redis_sock, char *cmd, int cmdlen,
                              char **reply, int *replylen)
{
    *reply = NULL;
    int len_written = redis_sock_write(redis_sock, cmd, cmdlen);

    if (len_written >= 0) {
        *reply = redis_sock_read(redis_sock, replylen);
    }

    return len_written;
}

static void
redis_pool_member_select(redis_pool_member *rpm) {
    RedisSock *redis_sock = rpm->redis_sock;
    char *response, *cmd;
    int response_len, cmd_len;

    cmd_len = REDIS_SPPRINTF(&cmd, "SELECT", "d", rpm->database);
    if (redis_sock_write(redis_sock, cmd, cmd_len) >= 0) {
        if ((response = redis_sock_read(redis_sock, &response_len))) {
            efree(response);
        }
    }
    efree(cmd);
}

PHP_REDIS_API redis_pool_member *
redis_pool_get_sock(redis_pool *pool, const char *key) {

    unsigned int pos, i;
    memcpy(&pos, key, sizeof(pos));
    pos %= pool->totalWeight;

    redis_pool_member *rpm = pool->head;

    for(i = 0; i < pool->totalWeight;) {
        if (pos >= i && pos < i + rpm->weight) {
            if (redis_sock_server_open(rpm->redis_sock) == 0) {
                if (rpm->database >= 0) { /* default is -1 which leaves the choice to redis. */
                    redis_pool_member_select(rpm);
                }

                return rpm;
            }
        }
        i += rpm->weight;
        rpm = rpm->next;
    }

    return NULL;
}

/* Helper to set our session lock key */
static int set_session_lock_key(RedisSock *redis_sock, char *cmd, int cmd_len
                               )
{
    char *reply;
    int sent_len, reply_len;

    sent_len = redis_simple_cmd(redis_sock, cmd, cmd_len, &reply, &reply_len);
    if (reply) {
        if (IS_REDIS_OK(reply, reply_len)) {
            efree(reply);
            return SUCCESS;
        }

        efree(reply);
    }

    /* Return FAILURE in case of network problems */
    return sent_len >= 0 ? NEGATIVE_LOCK_RESPONSE : FAILURE;
}

static int lock_acquire(RedisSock *redis_sock, redis_session_lock_status *lock_status
                       )
{
    char *cmd, hostname[HOST_NAME_MAX] = {0}, suffix[] = "_LOCK";
    int cmd_len, lock_wait_time, retries, i, set_lock_key_result, expiry;

    /* Short circuit if we are already locked or not using session locks */
    if (lock_status->is_locked || !INI_INT("redis.session.locking_enabled"))
        return SUCCESS;

    /* How long to wait between attempts to acquire lock */
    lock_wait_time = INI_INT("redis.session.lock_wait_time");
    if (lock_wait_time == 0) {
        lock_wait_time = 20000;
    }

    /* Maximum number of times to retry (-1 means infinite) */
    retries = INI_INT("redis.session.lock_retries");
    if (retries == 0) {
        retries = 100;
    }

    /* How long should the lock live (in seconds) */
    expiry = INI_INT("redis.session.lock_expire");
    if (expiry == 0) {
        expiry = INI_INT("max_execution_time");
    }

    /* Generate our qualified lock key */
    if (lock_status->lock_key) zend_string_release(lock_status->lock_key);
    lock_status->lock_key = zend_string_alloc(ZSTR_LEN(lock_status->session_key) + sizeof(suffix) - 1, 0);
    memcpy(ZSTR_VAL(lock_status->lock_key), ZSTR_VAL(lock_status->session_key), ZSTR_LEN(lock_status->session_key));
    memcpy(ZSTR_VAL(lock_status->lock_key) + ZSTR_LEN(lock_status->session_key), suffix, sizeof(suffix) - 1);

    /* Calculate lock secret */
    gethostname(hostname, HOST_NAME_MAX);
    if (lock_status->lock_secret) zend_string_release(lock_status->lock_secret);
    lock_status->lock_secret = strpprintf(0, "%s|%ld", hostname, (long)getpid());

    if (expiry > 0) {
        cmd_len = REDIS_SPPRINTF(&cmd, "SET", "SSssd", lock_status->lock_key,
                                 lock_status->lock_secret, "NX", 2, "PX", 2,
                                 expiry * 1000);
    } else {
        cmd_len = REDIS_SPPRINTF(&cmd, "SET", "SSs", lock_status->lock_key,
                                 lock_status->lock_secret, "NX", 2);
    }

    /* Attempt to get our lock */
    for (i = 0; retries == -1 || i <= retries; i++) {
        set_lock_key_result = set_session_lock_key(redis_sock, cmd, cmd_len);

        if (set_lock_key_result == SUCCESS) {
            lock_status->is_locked = 1;
            break;
        } else if (set_lock_key_result == FAILURE) {
            /* In case of network problems, break the loop and report to userland */
            lock_status->is_locked = 0;
            break;
        }

        /* Sleep unless we're done making attempts */
        if (retries == -1 || i < retries) {
            usleep(lock_wait_time);
        }
    }

    /* Cleanup SET command */
    efree(cmd);

    /* Success if we're locked */
    return lock_status->is_locked ? SUCCESS : FAILURE;
}

#define IS_LOCK_SECRET(reply, len, secret) (len == ZSTR_LEN(secret) && !strncmp(reply, ZSTR_VAL(secret), len))
static int write_allowed(RedisSock *redis_sock, redis_session_lock_status *lock_status)
{
    if (!INI_INT("redis.session.locking_enabled")) {
        return 1;
    }
    /* If locked and redis.session.lock_expire is not set => TTL=max_execution_time
       Therefore it is guaranteed that the current process is still holding the lock */

    if (lock_status->is_locked && INI_INT("redis.session.lock_expire") != 0) {
        char *cmd, *reply = NULL;
        int replylen, cmdlen;
        /* Command to get our lock key value and compare secrets */
        cmdlen = REDIS_SPPRINTF(&cmd, "GET", "S", lock_status->lock_key);

        /* Attempt to refresh the lock */
        redis_simple_cmd(redis_sock, cmd, cmdlen, &reply, &replylen);
        /* Cleanup */
        efree(cmd);

        if (reply == NULL) {
            lock_status->is_locked = 0;
        } else {
            lock_status->is_locked = IS_LOCK_SECRET(reply, replylen, lock_status->lock_secret);
            efree(reply);
        }

        /* Issue a warning if we're not locked.  We don't attempt to refresh the lock
         * if we aren't flagged as locked, so if we're not flagged here something
         * failed */
        if (!lock_status->is_locked) {
            php_error_docref(NULL, E_WARNING, "Session lock expired");
        }
    }

    return lock_status->is_locked;
}

/* Release any session lock we hold and cleanup allocated lock data.  This function
 * first attempts to use EVALSHA and then falls back to EVAL if EVALSHA fails.  This
 * will cause Redis to cache the script, so subsequent calls should then succeed
 * using EVALSHA. */
static void lock_release(RedisSock *redis_sock, redis_session_lock_status *lock_status)
{
    char *cmd, *reply;
    int i, cmdlen, replylen;

    /* Keywords, command, and length fallbacks */
    const char *kwd[] = {"EVALSHA", "EVAL"};
    const char *lua[] = {LOCK_RELEASE_SHA_STR, LOCK_RELEASE_LUA_STR};
    int len[] = {LOCK_RELEASE_SHA_LEN, LOCK_RELEASE_LUA_LEN};

    /* We first want to try EVALSHA and then fall back to EVAL */
    for (i = 0; lock_status->is_locked && i < sizeof(kwd)/sizeof(*kwd); i++) {
        /* Construct our command */
        cmdlen = REDIS_SPPRINTF(&cmd, (char*)kwd[i], "sdSS", lua[i], len[i], 1,
            lock_status->lock_key, lock_status->lock_secret);

        /* Send it off */
        redis_simple_cmd(redis_sock, cmd, cmdlen, &reply, &replylen);

        /* Release lock and cleanup reply if we got one */
        if (reply != NULL) {
            lock_status->is_locked = 0;
            efree(reply);
        }

        /* Cleanup command */
        efree(cmd);
    }

    /* Something has failed if we are still locked */
    if (lock_status->is_locked) {
        php_error_docref(NULL, E_WARNING, "Failed to release session lock");
    }
}

#if PHP_VERSION_ID < 70300
#define REDIS_URL_STR(umem) umem
#else
#define REDIS_URL_STR(umem) ZSTR_VAL(umem)
#endif

/* {{{ PS_OPEN_FUNC
 */
PS_OPEN_FUNC(redis)
{
    php_url *url;
    zval params;
    int i, j, path_len;

    redis_pool *pool = ecalloc(1, sizeof(*pool));

    for (i = 0, j = 0, path_len = strlen(save_path); i < path_len; i = j + 1) {
        /* find beginning of url */
        while ( i< path_len && (isspace(save_path[i]) || save_path[i] == ','))
            i++;

        /* find end of url */
        j = i;
        while (j<path_len && !isspace(save_path[j]) && save_path[j] != ',')
            j++;

        if (i < j) {
            int weight = 1;
            double timeout = 86400.0, read_timeout = 0.0;
            int persistent = 0, db = -1;
            zend_long retry_interval = 0;
            zend_string *persistent_id = NULL, *prefix = NULL;
            zend_string *user = NULL, *pass = NULL;

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
                php_error_docref(NULL, E_WARNING,
                    "Failed to parse session.save_path (error at offset %d, url was '%s')", i, path);
                efree(path);

                redis_pool_free(pool);
                PS_SET_MOD_DATA(NULL);
                return FAILURE;
            }

            /* parse parameters */
            if (url->query != NULL) {
                HashTable *ht;
                char *query;
                array_init(&params);

                if (url->fragment) {
                    spprintf(&query, 0, "%s#%s", REDIS_URL_STR(url->query), REDIS_URL_STR(url->fragment));
                } else {
                    query = estrdup(REDIS_URL_STR(url->query));
                }

                sapi_module.treat_data(PARSE_STRING, query, &params);
                ht = Z_ARRVAL(params);

                REDIS_CONF_INT_STATIC(ht, "weight", &weight);
                REDIS_CONF_BOOL_STATIC(ht, "persistent", &persistent);
                REDIS_CONF_INT_STATIC(ht, "database", &db);
                REDIS_CONF_DOUBLE_STATIC(ht, "timeout", &timeout);
                REDIS_CONF_DOUBLE_STATIC(ht, "read_timeout", &read_timeout);
                REDIS_CONF_LONG_STATIC(ht, "retry_interval", &retry_interval);
                REDIS_CONF_STRING_STATIC(ht, "persistent_id", &persistent_id);
                REDIS_CONF_STRING_STATIC(ht, "prefix", &prefix);
                REDIS_CONF_AUTH_STATIC(ht, "auth", &user, &pass);

                zval_dtor(&params);
            }

            if ((url->path == NULL && url->host == NULL) || weight <= 0 || timeout <= 0) {
                char *path = estrndup(save_path+i, j-i);
                php_error_docref(NULL, E_WARNING,
                    "Failed to parse session.save_path (error at offset %d, url was '%s')", i, path);
                efree(path);

                php_url_free(url);
                if (persistent_id) zend_string_release(persistent_id);
                if (prefix) zend_string_release(prefix);
                if (user) zend_string_release(user);
                if (pass) zend_string_release(pass);
                redis_pool_free(pool);
                PS_SET_MOD_DATA(NULL);

                return FAILURE;
            }

            RedisSock *redis_sock;
            char *addr, *scheme;
            size_t addrlen;
            int port, addr_free = 0;

            scheme = url->scheme ? REDIS_URL_STR(url->scheme) : "tcp";
            if (url->host) {
                port = url->port;
                addrlen = spprintf(&addr, 0, "%s://%s", scheme, REDIS_URL_STR(url->host));
                addr_free = 1;
            } else { /* unix */
                port = 0;
                addr = REDIS_URL_STR(url->path);
                addrlen = strlen(addr);
            }

            redis_sock = redis_sock_create(addr, addrlen, port, timeout, read_timeout,
                                           persistent, persistent_id ? ZSTR_VAL(persistent_id) : NULL,
                                           retry_interval);

            redis_pool_add(pool, redis_sock, weight, db);
            redis_sock->prefix = prefix;
            redis_sock_set_auth(redis_sock, user, pass);

            if (addr_free) efree(addr);
            if (persistent_id) zend_string_release(persistent_id);
            if (user) zend_string_release(user);
            if (pass) zend_string_release(pass);
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

    if (pool) {
        if (pool->lock_status.session_key) {
            redis_pool_member *rpm = redis_pool_get_sock(pool, ZSTR_VAL(pool->lock_status.session_key));

            RedisSock *redis_sock = rpm ? rpm->redis_sock : NULL;
            if (redis_sock) {
                lock_release(redis_sock, &pool->lock_status);
            }
        }

        redis_pool_free(pool);
        PS_SET_MOD_DATA(NULL);
    }

    return SUCCESS;
}
/* }}} */

static zend_string *
redis_session_key(RedisSock *redis_sock, const char *key, int key_len)
{
    zend_string *session;
    char default_prefix[] = REDIS_SESSION_PREFIX;
    char *prefix = default_prefix;
    size_t prefix_len = sizeof(default_prefix)-1;

    if (redis_sock->prefix) {
        prefix = ZSTR_VAL(redis_sock->prefix);
        prefix_len = ZSTR_LEN(redis_sock->prefix);
    }

    /* build session key */
    session = zend_string_alloc(key_len + prefix_len, 0);
    memcpy(ZSTR_VAL(session), prefix, prefix_len);
    memcpy(ZSTR_VAL(session) + prefix_len, key, key_len);

    return session;
}

/* {{{ PS_CREATE_SID_FUNC
 */
PS_CREATE_SID_FUNC(redis)
{
    int retries = 3;
    redis_pool *pool = PS_GET_MOD_DATA();

    if (!pool) {
        return php_session_create_id(NULL);
    }

    while (retries-- > 0) {
        zend_string* sid = php_session_create_id((void **) &pool);
        redis_pool_member *rpm = redis_pool_get_sock(pool, ZSTR_VAL(sid));

        RedisSock *redis_sock = rpm ? rpm->redis_sock : NULL;

        if (!redis_sock) {
            php_error_docref(NULL, E_NOTICE, "Redis connection not available");
            zend_string_release(sid);
            return php_session_create_id(NULL);
        }

        if (pool->lock_status.session_key) zend_string_release(pool->lock_status.session_key);
        pool->lock_status.session_key = redis_session_key(redis_sock, ZSTR_VAL(sid), ZSTR_LEN(sid));

        if (lock_acquire(redis_sock, &pool->lock_status) == SUCCESS) {
            return sid;
        }

        zend_string_release(pool->lock_status.session_key);
        zend_string_release(sid);

        sid = NULL;
    }

    php_error_docref(NULL, E_WARNING,
        "Acquiring session lock failed while creating session_id");

    return NULL;
}
/* }}} */

/* {{{ PS_VALIDATE_SID_FUNC
 */
PS_VALIDATE_SID_FUNC(redis)
{
    char *cmd, *response;
    int cmd_len, response_len;

    const char *skey = ZSTR_VAL(key);
    size_t skeylen = ZSTR_LEN(key);

    if (!skeylen) return FAILURE;

    redis_pool *pool = PS_GET_MOD_DATA();
    redis_pool_member *rpm = redis_pool_get_sock(pool, skey);
    RedisSock *redis_sock = rpm ? rpm->redis_sock : NULL;
    if (!redis_sock) {
        php_error_docref(NULL, E_WARNING, "Redis connection not available");
        return FAILURE;
    }

    /* send EXISTS command */
    zend_string *session = redis_session_key(redis_sock, skey, skeylen);
    cmd_len = REDIS_SPPRINTF(&cmd, "EXISTS", "S", session);
    zend_string_release(session);
    if (redis_sock_write(redis_sock, cmd, cmd_len) < 0 || (response = redis_sock_read(redis_sock, &response_len)) == NULL) {
        php_error_docref(NULL, E_WARNING, "Error communicating with Redis server");
        efree(cmd);
        return FAILURE;
    }

    efree(cmd);

    if (response_len == 2 && response[0] == ':' && response[1] == '1') {
        efree(response);
        return SUCCESS;
    } else {
        efree(response);
        return FAILURE;
    }
}
/* }}} */

/* {{{ PS_UPDATE_TIMESTAMP_FUNC
 */
PS_UPDATE_TIMESTAMP_FUNC(redis)
{
    char *cmd, *response;
    int cmd_len, response_len;

    const char *skey = ZSTR_VAL(key);
    size_t skeylen = ZSTR_LEN(key);

    if (!skeylen) return FAILURE;

    redis_pool *pool = PS_GET_MOD_DATA();
    redis_pool_member *rpm = redis_pool_get_sock(pool, skey);
    RedisSock *redis_sock = rpm ? rpm->redis_sock : NULL;
    if (!redis_sock) {
        php_error_docref(NULL, E_WARNING, "Redis connection not available");
        return FAILURE;
    }

    /* send EXPIRE command */
    zend_string *session = redis_session_key(redis_sock, skey, skeylen);
    cmd_len = REDIS_SPPRINTF(&cmd, "EXPIRE", "Sd", session, session_gc_maxlifetime());
    zend_string_release(session);

    if (redis_sock_write(redis_sock, cmd, cmd_len) < 0 || (response = redis_sock_read(redis_sock, &response_len)) == NULL) {
        php_error_docref(NULL, E_WARNING, "Error communicating with Redis server");
        efree(cmd);
        return FAILURE;
    }

    efree(cmd);

    if (response_len == 2 && response[0] == ':') {
        efree(response);
        return SUCCESS;
    } else {
        efree(response);
        return FAILURE;
    }
}
/* }}} */

/* {{{ PS_READ_FUNC
 */
PS_READ_FUNC(redis)
{
    char *resp, *cmd;
    int resp_len, cmd_len;
    const char *skey = ZSTR_VAL(key);
    size_t skeylen = ZSTR_LEN(key);

    if (!skeylen) return FAILURE;

    redis_pool *pool = PS_GET_MOD_DATA();
    redis_pool_member *rpm = redis_pool_get_sock(pool, skey);
    RedisSock *redis_sock = rpm ? rpm->redis_sock : NULL;
    if (!redis_sock) {
        php_error_docref(NULL, E_WARNING, "Redis connection not available");
        return FAILURE;
    }

    /* send GET command */
    if (pool->lock_status.session_key) zend_string_release(pool->lock_status.session_key);
    pool->lock_status.session_key = redis_session_key(redis_sock, skey, skeylen);
    cmd_len = REDIS_SPPRINTF(&cmd, "GET", "S", pool->lock_status.session_key);

    if (lock_acquire(redis_sock, &pool->lock_status) != SUCCESS) {
        php_error_docref(NULL, E_WARNING, "Failed to acquire session lock");
        efree(cmd);
        return FAILURE;
    }

    if (redis_sock_write(redis_sock, cmd, cmd_len) < 0) {
        php_error_docref(NULL, E_WARNING, "Error communicating with Redis server");
        efree(cmd);
        return FAILURE;
    }

    efree(cmd);

    /* Read response from Redis.  If we get a NULL response from redis_sock_read
     * this can indicate an error, OR a "NULL bulk" reply (empty session data)
     * in which case we can reply with success. */
    if ((resp = redis_sock_read(redis_sock, &resp_len)) == NULL && resp_len != -1) {
        php_error_docref(NULL, E_WARNING, "Error communicating with Redis server");
        return FAILURE;
    }

    if (resp_len < 0) {
        *val = ZSTR_EMPTY_ALLOC();
    } else {
        *val = zend_string_init(resp, resp_len, 0);
    }
    efree(resp);

    return SUCCESS;
}
/* }}} */

/* {{{ PS_WRITE_FUNC
 */
PS_WRITE_FUNC(redis)
{
    char *cmd, *response;
    int cmd_len, response_len;
    const char *skey = ZSTR_VAL(key), *sval = ZSTR_VAL(val);
    size_t skeylen = ZSTR_LEN(key), svallen = ZSTR_LEN(val);

    if (!skeylen) return FAILURE;

    redis_pool *pool = PS_GET_MOD_DATA();
    redis_pool_member *rpm = redis_pool_get_sock(pool, skey);
    RedisSock *redis_sock = rpm ? rpm->redis_sock : NULL;
    if (!redis_sock) {
        php_error_docref(NULL, E_WARNING, "Redis connection not available");
        return FAILURE;
    }

    /* send SET command */
    zend_string *session = redis_session_key(redis_sock, skey, skeylen);

    cmd_len = REDIS_SPPRINTF(&cmd, "SETEX", "Sds", session, session_gc_maxlifetime(), sval, svallen);
    zend_string_release(session);

    if (!write_allowed(redis_sock, &pool->lock_status)) {
        php_error_docref(NULL, E_WARNING, "Unable to write session: session lock not held");
        efree(cmd);
        return FAILURE;
    }

    if (redis_sock_write(redis_sock, cmd, cmd_len ) < 0 || (response = redis_sock_read(redis_sock, &response_len)) == NULL) {
        php_error_docref(NULL, E_WARNING, "Error communicating with Redis server");
        efree(cmd);
        return FAILURE;
    }

    efree(cmd);

    if (IS_REDIS_OK(response, response_len)) {
        efree(response);
        return SUCCESS;
    } else {
        php_error_docref(NULL, E_WARNING, "Error writing session data to Redis: %s", response);
        efree(response);
        return FAILURE;
    }
}
/* }}} */

/* {{{ PS_DESTROY_FUNC
 */
PS_DESTROY_FUNC(redis)
{
    char *cmd, *response;
    int cmd_len, response_len;
    const char *skey = ZSTR_VAL(key);
    size_t skeylen = ZSTR_LEN(key);

    redis_pool *pool = PS_GET_MOD_DATA();
    redis_pool_member *rpm = redis_pool_get_sock(pool, skey);
    RedisSock *redis_sock = rpm ? rpm->redis_sock : NULL;
    if (!redis_sock) {
        php_error_docref(NULL, E_WARNING, "Redis connection not available");
        return FAILURE;
    }

    /* Release lock */
    lock_release(redis_sock, &pool->lock_status);

    /* send DEL command */
    zend_string *session = redis_session_key(redis_sock, skey, skeylen);
    cmd_len = REDIS_SPPRINTF(&cmd, "DEL", "S", session);
    zend_string_release(session);
    if (redis_sock_write(redis_sock, cmd, cmd_len) < 0 || (response = redis_sock_read(redis_sock, &response_len)) == NULL) {
        php_error_docref(NULL, E_WARNING, "Error communicating with Redis server");
        efree(cmd);
        return FAILURE;
    }

    efree(cmd);

    if (response_len == 2 && response[0] == ':' && (response[1] == '0' || response[1] == '1')) {
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

/* Prefix a session key */
static char *cluster_session_key(redisCluster *c, const char *key, int keylen,
                                 int *skeylen, short *slot) {
    char *skey;

    *skeylen = keylen + ZSTR_LEN(c->flags->prefix);
    skey = emalloc(*skeylen);
    memcpy(skey, ZSTR_VAL(c->flags->prefix), ZSTR_LEN(c->flags->prefix));
    memcpy(skey + ZSTR_LEN(c->flags->prefix), key, keylen);

    *slot = cluster_hash_key(skey, *skeylen);

    return skey;
}

PS_OPEN_FUNC(rediscluster) {
    redisCluster *c;
    zval z_conf, *zv, *context;
    HashTable *ht_conf, *ht_seeds;
    double timeout = 0, read_timeout = 0;
    int persistent = 0, failover = REDIS_FAILOVER_NONE;
    zend_string *prefix = NULL, *user = NULL, *pass = NULL, *failstr = NULL;

    /* Parse configuration for session handler */
    array_init(&z_conf);
    sapi_module.treat_data(PARSE_STRING, estrdup(save_path), &z_conf);

    /* We need seeds */
    zv = REDIS_HASH_STR_FIND_TYPE_STATIC(Z_ARRVAL(z_conf), "seed", IS_ARRAY);
    if (zv == NULL) {
        zval_dtor(&z_conf);
        return FAILURE;
    }

    /* Grab a copy of our config hash table and keep seeds array */
    ht_conf = Z_ARRVAL(z_conf);
    ht_seeds = Z_ARRVAL_P(zv);

    /* Optional configuration settings */
    REDIS_CONF_DOUBLE_STATIC(ht_conf, "timeout", &timeout);
    REDIS_CONF_DOUBLE_STATIC(ht_conf, "read_timeout", &read_timeout);
    REDIS_CONF_BOOL_STATIC(ht_conf, "persistent", &persistent);

    /* Sanity check on our timeouts */
    if (timeout < 0 || read_timeout < 0) {
        php_error_docref(NULL, E_WARNING,
            "Can't set negative timeout values in session configuration");
        zval_dtor(&z_conf);
        return FAILURE;
    }

    REDIS_CONF_STRING_STATIC(ht_conf, "prefix", &prefix);
    REDIS_CONF_AUTH_STATIC(ht_conf, "auth", &user, &pass);
    REDIS_CONF_STRING_STATIC(ht_conf, "failover", &failstr);

    /* Need to massage failover string if we have it */
    if (failstr) {
        if (zend_string_equals_literal_ci(failstr, "error")) {
            failover = REDIS_FAILOVER_ERROR;
        } else if (zend_string_equals_literal_ci(failstr, "distribute")) {
            failover = REDIS_FAILOVER_DISTRIBUTE;
        }
    }

    redisCachedCluster *cc;
    zend_string **seeds, *hash = NULL;
    uint32_t nseeds;

    #define CLUSTER_SESSION_CLEANUP() \
        if (hash) zend_string_release(hash); \
        if (failstr) zend_string_release(failstr); \
        if (prefix) zend_string_release(prefix); \
        if (user) zend_string_release(user); \
        if (pass) zend_string_release(pass); \
        free_seed_array(seeds, nseeds); \
        zval_dtor(&z_conf); \

    /* Extract at least one valid seed or abort */
    seeds = cluster_validate_args(timeout, read_timeout, ht_seeds, &nseeds, NULL);
    if (seeds == NULL) {
        php_error_docref(NULL, E_WARNING, "No valid seeds detected");
        CLUSTER_SESSION_CLEANUP();
        return FAILURE;
    }

    c = cluster_create(timeout, read_timeout, failover, persistent);

    if (prefix) {
        c->flags->prefix = zend_string_copy(prefix);
    } else {
        c->flags->prefix = CLUSTER_DEFAULT_PREFIX();
    }

    redis_sock_set_auth(c->flags, user, pass);

    if ((context = REDIS_HASH_STR_FIND_TYPE_STATIC(ht_conf, "stream", IS_ARRAY)) != NULL) {
        redis_sock_set_stream_context(c->flags, context);
    }

    /* First attempt to load from cache */
    if (CLUSTER_CACHING_ENABLED()) {
        hash = cluster_hash_seeds(seeds, nseeds);
        if ((cc = cluster_cache_load(hash))) {
            cluster_init_cache(c, cc);
            goto success;
        }
    }

    /* Initialize seed array, and attempt to map keyspace */
    cluster_init_seeds(c, seeds, nseeds);
    if (cluster_map_keyspace(c) != SUCCESS)
        goto failure;

    /* Now cache our cluster if caching is enabled */
    if (hash)
        cluster_cache_store(hash, c->nodes);

success:
    CLUSTER_SESSION_CLEANUP();
    PS_SET_MOD_DATA(c);
    return SUCCESS;

failure:
    CLUSTER_SESSION_CLEANUP();
    cluster_free(c, 1);
    return FAILURE;
}

/* {{{ PS_READ_FUNC
 */
PS_READ_FUNC(rediscluster) {
    redisCluster *c = PS_GET_MOD_DATA();
    clusterReply *reply;
    char *cmd, *skey;
    int cmdlen, skeylen, free_flag;
    short slot;

    /* Set up our command and slot information */
    skey = cluster_session_key(c, ZSTR_VAL(key), ZSTR_LEN(key), &skeylen, &slot);

    cmdlen = redis_spprintf(NULL, NULL, &cmd, "GET", "s", skey, skeylen);
    efree(skey);

    /* Attempt to kick off our command */
    c->readonly = 1;
    if (cluster_send_command(c,slot,cmd,cmdlen) < 0 || c->err) {
        efree(cmd);
        return FAILURE;
    }

    /* Clean up command */
    efree(cmd);

    /* Attempt to read reply */
    reply = cluster_read_resp(c, 0);
    if (!reply || c->err) {
        if (reply) cluster_free_reply(reply, 1);
        return FAILURE;
    }

    /* Push reply value to caller */
    if (reply->str == NULL) {
        *val = ZSTR_EMPTY_ALLOC();
    } else {
        *val = zend_string_init(reply->str, reply->len, 0);
    }

    free_flag = 1;

    /* Clean up */
    cluster_free_reply(reply, free_flag);

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
    skey = cluster_session_key(c, ZSTR_VAL(key), ZSTR_LEN(key), &skeylen, &slot);
    cmdlen = redis_spprintf(NULL, NULL, &cmd, "SETEX", "sds", skey,
                            skeylen, session_gc_maxlifetime(),
                            ZSTR_VAL(val), ZSTR_LEN(val));
    efree(skey);

    /* Attempt to send command */
    c->readonly = 0;
    if (cluster_send_command(c,slot,cmd,cmdlen) < 0 || c->err) {
        efree(cmd);
        return FAILURE;
    }

    /* Clean up our command */
    efree(cmd);

    /* Attempt to read reply */
    reply = cluster_read_resp(c, 0);
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
    skey = cluster_session_key(c, ZSTR_VAL(key), ZSTR_LEN(key), &skeylen, &slot);

    cmdlen = redis_spprintf(NULL, NULL, &cmd, "DEL", "s", skey, skeylen);
    efree(skey);

    /* Attempt to send command */
    if (cluster_send_command(c,slot,cmd,cmdlen) < 0 || c->err) {
        efree(cmd);
        return FAILURE;
    }

    /* Clean up our command */
    efree(cmd);

    /* Attempt to read reply */
    reply = cluster_read_resp(c, 0);
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
        cluster_free(c, 1);
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
