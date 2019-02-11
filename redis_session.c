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

/* Session lock LUA as well as its SHA1 hash */
#define LOCK_RELEASE_LUA_STR "if redis.call(\"get\",KEYS[1]) == ARGV[1] then return redis.call(\"del\",KEYS[1]) else return 0 end"
#define LOCK_RELEASE_LUA_LEN (sizeof(LOCK_RELEASE_LUA_STR) - 1)
#define LOCK_RELEASE_SHA_STR "b70c2384248f88e6b75b9f89241a180f856ad852"
#define LOCK_RELEASE_SHA_LEN (sizeof(LOCK_RELEASE_SHA_STR) - 1)

/* Check if a response is the Redis +OK status response */
#define IS_REDIS_OK(r, len) (r != NULL && len == 3 && !memcmp(r, "+OK", 3))
#define NEGATIVE_LOCK_RESPONSE 1

ps_module ps_mod_redis = {
#if (PHP_MAJOR_VERSION < 7)
    PS_MOD_SID(redis)
#else
    PS_MOD_UPDATE_TIMESTAMP(redis)
#endif
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
    zend_string *prefix;
    zend_string *auth;
    struct redis_pool_member_ *next;

} redis_pool_member;

typedef struct {

    int totalWeight;
    int count;

    redis_pool_member *head;
    redis_session_lock_status lock_status;

} redis_pool;

PHP_REDIS_API void
redis_pool_add(redis_pool *pool, RedisSock *redis_sock, int weight,
                int database, zend_string *prefix, zend_string *auth TSRMLS_DC) {

    redis_pool_member *rpm = ecalloc(1, sizeof(redis_pool_member));
    rpm->redis_sock = redis_sock;
    rpm->weight = weight;
    rpm->database = database;

    rpm->prefix = prefix;
    rpm->auth = auth;

    rpm->next = pool->head;
    pool->head = rpm;

    pool->totalWeight += weight;
}

PHP_REDIS_API void
redis_pool_free(redis_pool *pool TSRMLS_DC) {

    redis_pool_member *rpm, *next;
    rpm = pool->head;
    while (rpm) {
        next = rpm->next;
        redis_sock_disconnect(rpm->redis_sock, 0 TSRMLS_CC);
        redis_free_socket(rpm->redis_sock);
        if (rpm->prefix) zend_string_release(rpm->prefix);
        if (rpm->auth) zend_string_release(rpm->auth);
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

/* Send a command to Redis.  Returns byte count written to socket (-1 on failure) */
static int redis_simple_cmd(RedisSock *redis_sock, char *cmd, int cmdlen,
                              char **reply, int *replylen TSRMLS_DC)
{
    *reply = NULL;
    int len_written = redis_sock_write(redis_sock, cmd, cmdlen TSRMLS_CC);

    if (len_written >= 0) {
        *reply = redis_sock_read(redis_sock, replylen TSRMLS_CC);
    }

    return len_written;
}

static void
redis_pool_member_auth(redis_pool_member *rpm TSRMLS_DC) {
    RedisSock *redis_sock = rpm->redis_sock;
    char *response, *cmd;
    int response_len, cmd_len;

    /* Short circuit if we don't have a password */
    if (!rpm->auth) {
        return;
    }

    cmd_len = REDIS_SPPRINTF(&cmd, "AUTH", "S", rpm->auth);
    if (redis_sock_write(redis_sock, cmd, cmd_len TSRMLS_CC) >= 0) {
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
    if (redis_sock_write(redis_sock, cmd, cmd_len TSRMLS_CC) >= 0) {
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
        if (pos >= i && pos < i + rpm->weight) {
            int needs_auth = 0;
            if (rpm->auth && rpm->redis_sock->status != REDIS_SOCK_STATUS_CONNECTED) {
                    needs_auth = 1;
            }
            if (redis_sock_server_open(rpm->redis_sock TSRMLS_CC) == 0) {
                if (needs_auth) {
                    redis_pool_member_auth(rpm TSRMLS_CC);
                }
                if (rpm->database >= 0) { /* default is -1 which leaves the choice to redis. */
                    redis_pool_member_select(rpm TSRMLS_CC);
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
                                TSRMLS_DC)
{
    char *reply;
    int sent_len, reply_len;

    sent_len = redis_simple_cmd(redis_sock, cmd, cmd_len, &reply, &reply_len TSRMLS_CC);
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
                        TSRMLS_DC)
{
    char *cmd, hostname[HOST_NAME_MAX] = {0}, suffix[] = "_LOCK", pid[32];
    int cmd_len, lock_wait_time, retries, i, set_lock_key_result, expiry;

    /* Short circuit if we are already locked or not using session locks */
    if (lock_status->is_locked || !INI_INT("redis.session.locking_enabled"))
        return SUCCESS;

    /* How long to wait between attempts to acquire lock */
    lock_wait_time = INI_INT("redis.session.lock_wait_time");
    if (lock_wait_time == 0) {
        lock_wait_time = 2000;
    }

    /* Maximum number of times to retry (-1 means infinite) */
    retries = INI_INT("redis.session.lock_retries");
    if (retries == 0) {
        retries = 10;
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
    size_t hostname_len = strlen(hostname);
    size_t pid_len = snprintf(pid, sizeof(pid), "|%ld", (long)getpid());
    if (lock_status->lock_secret) zend_string_release(lock_status->lock_secret);
    lock_status->lock_secret = zend_string_alloc(hostname_len + pid_len, 0);
    memcpy(ZSTR_VAL(lock_status->lock_secret), hostname, hostname_len);
    memcpy(ZSTR_VAL(lock_status->lock_secret) + hostname_len, pid, pid_len);

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
        set_lock_key_result = set_session_lock_key(redis_sock, cmd, cmd_len TSRMLS_CC);

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
static void refresh_lock_status(RedisSock *redis_sock, redis_session_lock_status *lock_status TSRMLS_DC)
{
    char *cmd, *reply = NULL;
    int replylen, cmdlen;

    /* Return early if we're not locked */
    if (!lock_status->is_locked)
        return;

    /* If redis.session.lock_expire is not set => TTL=max_execution_time
       Therefore it is guaranteed that the current process is still holding
       the lock */
    if (lock_status->is_locked && INI_INT("redis.session.lock_expire") == 0)
        return;

    /* Command to get our lock key value and compare secrets */
    cmdlen = REDIS_SPPRINTF(&cmd, "GET", "S", lock_status->lock_key);

    /* Attempt to refresh the lock */
    redis_simple_cmd(redis_sock, cmd, cmdlen, &reply, &replylen TSRMLS_CC);
    if (reply != NULL) {
        lock_status->is_locked = IS_LOCK_SECRET(reply, replylen, lock_status->lock_secret);
        efree(reply);
    } else {
        lock_status->is_locked = 0;
    }

    /* Issue a warning if we're not locked.  We don't attempt to refresh the lock
     * if we aren't flagged as locked, so if we're not flagged here something
     * failed */
    if (!lock_status->is_locked) {
        php_error_docref(NULL TSRMLS_CC, E_WARNING, "Failed to refresh session lock");
    }

    /* Cleanup */
    efree(cmd);
}

static int write_allowed(RedisSock *redis_sock, redis_session_lock_status *lock_status TSRMLS_DC)
{
    if (!INI_INT("redis.session.locking_enabled"))
        return 1;

    refresh_lock_status(redis_sock, lock_status TSRMLS_CC);

    return lock_status->is_locked;
}

/* Release any session lock we hold and cleanup allocated lock data.  This function
 * first attempts to use EVALSHA and then falls back to EVAL if EVALSHA fails.  This
 * will cause Redis to cache the script, so subsequent calls should then succeed
 * using EVALSHA. */
static void lock_release(RedisSock *redis_sock, redis_session_lock_status *lock_status TSRMLS_DC)
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
        redis_simple_cmd(redis_sock, cmd, cmdlen, &reply, &replylen TSRMLS_CC);

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
        php_error_docref(NULL TSRMLS_CC, E_WARNING, "Failed to release session lock");
    }
}

/* {{{ PS_OPEN_FUNC
 */
PS_OPEN_FUNC(redis)
{
    php_url *url;
    zval params, *param;
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
            int persistent = 0;
            int database = -1;
            char *persistent_id = NULL;
            long retry_interval = 0;
            zend_string *prefix = NULL, *auth = NULL;

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

#if (PHP_VERSION_ID < 70300)
                sapi_module.treat_data(PARSE_STRING, estrdup(url->query), &params TSRMLS_CC);
#else
                sapi_module.treat_data(PARSE_STRING, estrndup(ZSTR_VAL(url->query), ZSTR_LEN(url->query)), &params TSRMLS_CC);
#endif

                if ((param = zend_hash_str_find(Z_ARRVAL(params), "weight", sizeof("weight") - 1)) != NULL) {
                    weight = zval_get_long(param);
                }
                if ((param = zend_hash_str_find(Z_ARRVAL(params), "timeout", sizeof("timeout") - 1)) != NULL) {
                    timeout = zval_get_double(param);
                }
                if ((param = zend_hash_str_find(Z_ARRVAL(params), "read_timeout", sizeof("read_timeout") - 1)) != NULL) {
                    read_timeout = zval_get_double(param);
                }
                if ((param = zend_hash_str_find(Z_ARRVAL(params), "persistent", sizeof("persistent") - 1)) != NULL) {
                    persistent = (atol(Z_STRVAL_P(param)) == 1 ? 1 : 0);
                }
                if ((param = zend_hash_str_find(Z_ARRVAL(params), "persistent_id", sizeof("persistent_id") - 1)) != NULL) {
                    persistent_id = estrndup(Z_STRVAL_P(param), Z_STRLEN_P(param));
                }
                if ((param = zend_hash_str_find(Z_ARRVAL(params), "prefix", sizeof("prefix") - 1)) != NULL) {
                    prefix = zend_string_init(Z_STRVAL_P(param), Z_STRLEN_P(param), 0);
                }
                if ((param = zend_hash_str_find(Z_ARRVAL(params), "auth", sizeof("auth") - 1)) != NULL) {
                    auth = zend_string_init(Z_STRVAL_P(param), Z_STRLEN_P(param), 0);
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
                if (prefix) zend_string_release(prefix);
                if (auth) zend_string_release(auth);
                redis_pool_free(pool TSRMLS_CC);
                PS_SET_MOD_DATA(NULL);
                return FAILURE;
            }

            RedisSock *redis_sock;
            if (url->host) {
#if (PHP_VERSION_ID < 70300)
                redis_sock = redis_sock_create(url->host, strlen(url->host), url->port, timeout, read_timeout, persistent, persistent_id, retry_interval);
#else
                redis_sock = redis_sock_create(ZSTR_VAL(url->host), ZSTR_LEN(url->host), url->port, timeout, read_timeout, persistent, persistent_id, retry_interval);
#endif
            } else { /* unix */
#if (PHP_VERSION_ID < 70300)
                redis_sock = redis_sock_create(url->path, strlen(url->path), 0, timeout, read_timeout, persistent, persistent_id, retry_interval);
#else
                redis_sock = redis_sock_create(ZSTR_VAL(url->path), ZSTR_LEN(url->path), 0, timeout, read_timeout, persistent, persistent_id, retry_interval);
#endif
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

    if (pool) {
        if (pool->lock_status.session_key) {
            redis_pool_member *rpm = redis_pool_get_sock(pool, ZSTR_VAL(pool->lock_status.session_key) TSRMLS_CC);

            RedisSock *redis_sock = rpm ? rpm->redis_sock : NULL;
            if (redis_sock) {
                lock_release(redis_sock, &pool->lock_status TSRMLS_CC);
            }
        }

        redis_pool_free(pool TSRMLS_CC);
        PS_SET_MOD_DATA(NULL);
    }

    return SUCCESS;
}
/* }}} */

static zend_string *
redis_session_key(redis_pool_member *rpm, const char *key, int key_len)
{
    zend_string *session;
    char default_prefix[] = "PHPREDIS_SESSION:";
    char *prefix = default_prefix;
    size_t prefix_len = sizeof(default_prefix)-1;

    if (rpm->prefix) {
        prefix = ZSTR_VAL(rpm->prefix);
        prefix_len = ZSTR_LEN(rpm->prefix);
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
#if (PHP_MAJOR_VERSION < 7)
        return php_session_create_id(NULL, newlen TSRMLS_CC);
#else
        return php_session_create_id(NULL TSRMLS_CC);
#endif
    }

    while (retries-- > 0) {
#if (PHP_MAJOR_VERSION < 7)
        char* sid = php_session_create_id((void **) &pool, newlen TSRMLS_CC);
        redis_pool_member *rpm = redis_pool_get_sock(pool, sid TSRMLS_CC);
#else
        zend_string* sid = php_session_create_id((void **) &pool TSRMLS_CC);
        redis_pool_member *rpm = redis_pool_get_sock(pool, ZSTR_VAL(sid) TSRMLS_CC);
#endif
        RedisSock *redis_sock = rpm?rpm->redis_sock:NULL;

        if (!rpm || !redis_sock) {
            php_error_docref(NULL TSRMLS_CC, E_NOTICE,
                "Redis not available while creating session_id");

#if (PHP_MAJOR_VERSION < 7)
            efree(sid);
            return php_session_create_id(NULL, newlen TSRMLS_CC);
#else
            zend_string_release(sid);
            return php_session_create_id(NULL TSRMLS_CC);
#endif
        }

        if (pool->lock_status.session_key) zend_string_release(pool->lock_status.session_key);
#if (PHP_MAJOR_VERSION < 7)
        pool->lock_status.session_key = redis_session_key(rpm, sid, strlen(sid));
#else
        pool->lock_status.session_key = redis_session_key(rpm, ZSTR_VAL(sid), ZSTR_LEN(sid));
#endif
        if (lock_acquire(redis_sock, &pool->lock_status TSRMLS_CC) == SUCCESS) {
            return sid;
        }

        zend_string_release(pool->lock_status.session_key);
#if (PHP_MAJOR_VERSION < 7)
        efree(sid);
#else
        zend_string_release(sid);
#endif
        sid = NULL;
    }

    php_error_docref(NULL TSRMLS_CC, E_NOTICE,
        "Acquiring session lock failed while creating session_id");

    return NULL;
}
/* }}} */

#if (PHP_MAJOR_VERSION >= 7)
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
    redis_pool_member *rpm = redis_pool_get_sock(pool, skey TSRMLS_CC);
    RedisSock *redis_sock = rpm ? rpm->redis_sock : NULL;
    if (!redis_sock) {
        return FAILURE;
    }

    /* send EXISTS command */
    zend_string *session = redis_session_key(rpm, skey, skeylen);
    cmd_len = REDIS_SPPRINTF(&cmd, "EXISTS", "S", session);
    zend_string_release(session);
    if (redis_sock_write(redis_sock, cmd, cmd_len TSRMLS_CC) < 0) {
        efree(cmd);
        return FAILURE;
    }
    efree(cmd);

    /* read response */
    if ((response = redis_sock_read(redis_sock, &response_len TSRMLS_CC)) == NULL) {
        return FAILURE;
    }

    if (response_len == 2 && response[0] == ':' && response[1] == '1') {
        efree(response);
        return SUCCESS;
    } else {
        efree(response);
        return FAILURE;
    }
}
/* }}} */
#endif

#if (PHP_MAJOR_VERSION >= 7)
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
    redis_pool_member *rpm = redis_pool_get_sock(pool, skey TSRMLS_CC);
    RedisSock *redis_sock = rpm ? rpm->redis_sock : NULL;
    if (!redis_sock) {
        return FAILURE;
    }

    /* send EXPIRE command */
    zend_string *session = redis_session_key(rpm, skey, skeylen);
    cmd_len = REDIS_SPPRINTF(&cmd, "EXPIRE", "Sd", session, INI_INT("session.gc_maxlifetime"));
    zend_string_release(session);

    if (redis_sock_write(redis_sock, cmd, cmd_len TSRMLS_CC) < 0) {
        efree(cmd);
        return FAILURE;
    }
    efree(cmd);

    /* read response */
    if ((response = redis_sock_read(redis_sock, &response_len TSRMLS_CC)) == NULL) {
        return FAILURE;
    }

    if (response_len == 2 && response[0] == ':') {
        efree(response);
        return SUCCESS;
    } else {
        efree(response);
        return FAILURE;
    }
}
/* }}} */
#endif

/* {{{ PS_READ_FUNC
 */
PS_READ_FUNC(redis)
{
    char *resp, *cmd;
    int resp_len, cmd_len;
#if (PHP_MAJOR_VERSION < 7)
    const char *skey = key;
    size_t skeylen = strlen(key);
#else
    const char *skey = ZSTR_VAL(key);
    size_t skeylen = ZSTR_LEN(key);
#endif

    if (!skeylen) return FAILURE;

    redis_pool *pool = PS_GET_MOD_DATA();
    redis_pool_member *rpm = redis_pool_get_sock(pool, skey TSRMLS_CC);
    RedisSock *redis_sock = rpm?rpm->redis_sock:NULL;
    if (!rpm || !redis_sock){
        return FAILURE;
    }

    /* send GET command */
    if (pool->lock_status.session_key) zend_string_release(pool->lock_status.session_key);
    pool->lock_status.session_key = redis_session_key(rpm, skey, skeylen);
    cmd_len = REDIS_SPPRINTF(&cmd, "GET", "S", pool->lock_status.session_key);

    if (lock_acquire(redis_sock, &pool->lock_status TSRMLS_CC) != SUCCESS) {
        php_error_docref(NULL TSRMLS_CC, E_NOTICE,
            "Acquire of session lock was not successful");
    }

    if (redis_sock_write(redis_sock, cmd, cmd_len TSRMLS_CC) < 0) {
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
    char *cmd, *response;
    int cmd_len, response_len;
#if (PHP_MAJOR_VERSION < 7)
    const char *skey = key, *sval = val;
    size_t skeylen = strlen(key), svallen = vallen;
#else
    const char *skey = ZSTR_VAL(key), *sval = ZSTR_VAL(val);
    size_t skeylen = ZSTR_LEN(key), svallen = ZSTR_LEN(val);
#endif

    if (!skeylen) return FAILURE;

    redis_pool *pool = PS_GET_MOD_DATA();
    redis_pool_member *rpm = redis_pool_get_sock(pool, skey TSRMLS_CC);
    RedisSock *redis_sock = rpm ? rpm->redis_sock : NULL;
    if (!redis_sock) {
        return FAILURE;
    }

    /* send SET command */
    zend_string *session = redis_session_key(rpm, skey, skeylen);
#if (PHP_MAJOR_VERSION < 7)
    /* We need to check for PHP5 if the session key changes (a bug with session_regenerate_id() is causing a missing PS_CREATE_SID call)*/
    if (!zend_string_equals(pool->lock_status.session_key, session)) {
        zend_string_release(pool->lock_status.session_key);
        pool->lock_status.session_key = zend_string_init(ZSTR_VAL(session), ZSTR_LEN(session), 0);
        if (lock_acquire(redis_sock, &pool->lock_status TSRMLS_CC) != SUCCESS) {
            zend_string_release(pool->lock_status.session_key);
            zend_string_release(session);
            return FAILURE;
        }
    }
#endif
    cmd_len = REDIS_SPPRINTF(&cmd, "SETEX", "Sds", session, INI_INT("session.gc_maxlifetime"), sval, svallen);
    zend_string_release(session);

    if (!write_allowed(redis_sock, &pool->lock_status TSRMLS_CC) || redis_sock_write(redis_sock, cmd, cmd_len TSRMLS_CC) < 0) {
        efree(cmd);
        return FAILURE;
    }
    efree(cmd);

    /* read response */
    if ((response = redis_sock_read(redis_sock, &response_len TSRMLS_CC)) == NULL) {
        return FAILURE;
    }

    if (IS_REDIS_OK(response, response_len)) {
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
    char *cmd, *response;
    int cmd_len, response_len;
#if (PHP_MAJOR_VERSION < 7)
    const char *skey = key;
    size_t skeylen = strlen(key);
#else
    const char *skey = ZSTR_VAL(key);
    size_t skeylen = ZSTR_LEN(key);
#endif

    redis_pool *pool = PS_GET_MOD_DATA();
    redis_pool_member *rpm = redis_pool_get_sock(pool, skey TSRMLS_CC);
    RedisSock *redis_sock = rpm ? rpm->redis_sock : NULL;
    if (!redis_sock) {
        return FAILURE;
    }

    /* Release lock */
    if (redis_sock) {
        lock_release(redis_sock, &pool->lock_status TSRMLS_CC);
    }

    /* send DEL command */
    zend_string *session = redis_session_key(rpm, skey, skeylen);
    cmd_len = REDIS_SPPRINTF(&cmd, "DEL", "S", session);
    zend_string_release(session);
    if (redis_sock_write(redis_sock, cmd, cmd_len TSRMLS_CC) < 0) {
        efree(cmd);
        return FAILURE;
    }
    efree(cmd);

    /* read response */
    if ((response = redis_sock_read(redis_sock, &response_len TSRMLS_CC)) == NULL) {
        return FAILURE;
    }

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

    *skeylen = keylen + ZSTR_LEN(c->flags->prefix);
    skey = emalloc(*skeylen);
    memcpy(skey, ZSTR_VAL(c->flags->prefix), ZSTR_LEN(c->flags->prefix));
    memcpy(skey + ZSTR_LEN(c->flags->prefix), key, keylen);

    *slot = cluster_hash_key(skey, *skeylen);

    return skey;
}

PS_OPEN_FUNC(rediscluster) {
    redisCluster *c;
    zval z_conf, *z_val;
    HashTable *ht_conf, *ht_seeds;
    double timeout = 0, read_timeout = 0;
    int retval, persistent = 0, failover = REDIS_FAILOVER_NONE;
    strlen_t prefix_len, auth_len = 0;
    char *prefix, *auth = NULL;

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
        Z_TYPE_P(z_val) == IS_STRING && Z_STRLEN_P(z_val) > 0
    ) {
        if (!strcasecmp(Z_STRVAL_P(z_val), "error")) {
            failover = REDIS_FAILOVER_ERROR;
        } else if (!strcasecmp(Z_STRVAL_P(z_val), "distribute")) {
            failover = REDIS_FAILOVER_DISTRIBUTE;
        }
    }

    /* Look for a specific auth setting */
    if ((z_val = zend_hash_str_find(ht_conf, "auth", sizeof("auth") - 1)) != NULL &&
        Z_TYPE_P(z_val) == IS_STRING && Z_STRLEN_P(z_val) > 0
    ) {
        auth = Z_STRVAL_P(z_val);
        auth_len = Z_STRLEN_P(z_val);
    }

    c = cluster_create(timeout, read_timeout, failover, persistent);
    if (auth && auth_len > 0) {
        c->auth = zend_string_init(auth, auth_len, 0);
    }
    if (!cluster_init_seeds(c, ht_seeds) && !cluster_map_keyspace(c TSRMLS_CC)) {
        /* Set up our prefix */
        c->flags->prefix = zend_string_init(prefix, prefix_len, 0);

        PS_SET_MOD_DATA(c);
        retval = SUCCESS;
    } else {
        cluster_free(c, 1 TSRMLS_CC);
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
    int cmdlen, skeylen, free_flag;
    short slot;

    /* Set up our command and slot information */
#if (PHP_MAJOR_VERSION < 7)
    skey = cluster_session_key(c, key, strlen(key), &skeylen, &slot);
#else
    skey = cluster_session_key(c, ZSTR_VAL(key), ZSTR_LEN(key), &skeylen, &slot);
#endif
    cmdlen = redis_spprintf(NULL, NULL TSRMLS_CC, &cmd, "GET", "s", skey, skeylen);
    efree(skey);

    /* Attempt to kick off our command */
    c->readonly = 1;
    if (cluster_send_command(c,slot,cmd,cmdlen TSRMLS_CC) < 0 || c->err) {
        efree(cmd);
        return FAILURE;
    }

    /* Clean up command */
    efree(cmd);

    /* Attempt to read reply */
    reply = cluster_read_resp(c, 0 TSRMLS_CC);
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

    free_flag = 0;
#else
    if (reply->str == NULL) {
        *val = ZSTR_EMPTY_ALLOC();
    } else {
        *val = zend_string_init(reply->str, reply->len, 0);
    }

    free_flag = 1;
#endif

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
#if (PHP_MAJOR_VERSION < 7)
    skey = cluster_session_key(c, key, strlen(key), &skeylen, &slot);
    cmdlen = redis_spprintf(NULL, NULL TSRMLS_CC, &cmd, "SETEX", "sds", skey,
                            skeylen, INI_INT("session.gc_maxlifetime"), val,
                            vallen);
#else
    skey = cluster_session_key(c, ZSTR_VAL(key), ZSTR_LEN(key), &skeylen, &slot);
    cmdlen = redis_spprintf(NULL, NULL TSRMLS_CC, &cmd, "SETEX", "sds", skey,
                            skeylen, INI_INT("session.gc_maxlifetime"),
                            ZSTR_VAL(val), ZSTR_LEN(val));
#endif
    efree(skey);

    /* Attempt to send command */
    c->readonly = 0;
    if (cluster_send_command(c,slot,cmd,cmdlen TSRMLS_CC) < 0 || c->err) {
        efree(cmd);
        return FAILURE;
    }

    /* Clean up our command */
    efree(cmd);

    /* Attempt to read reply */
    reply = cluster_read_resp(c, 0 TSRMLS_CC);
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
    skey = cluster_session_key(c, ZSTR_VAL(key), ZSTR_LEN(key), &skeylen, &slot);
#endif
    cmdlen = redis_spprintf(NULL, NULL TSRMLS_CC, &cmd, "DEL", "s", skey, skeylen);
    efree(skey);

    /* Attempt to send command */
    if (cluster_send_command(c,slot,cmd,cmdlen TSRMLS_CC) < 0 || c->err) {
        efree(cmd);
        return FAILURE;
    }

    /* Clean up our command */
    efree(cmd);

    /* Attempt to read reply */
    reply = cluster_read_resp(c, 0 TSRMLS_CC);
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
        cluster_free(c, 1 TSRMLS_CC);
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
