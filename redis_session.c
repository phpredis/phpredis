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
#include "redis_cmd.h"

#include <ext/hash/php_hash.h>

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
#define LOCK_DEL_LUA_STR "if redis.call(\"get\",KEYS[1]) == ARGV[1] then return redis.call(\"del\",KEYS[1]) else return 0 end"
#define LOCK_DEL_SHA_STR "b70c2384248f88e6b75b9f89241a180f856ad852"

/* Atomic acquire-and-read Lua script for the cluster handler.
 *   KEYS[1] - session data key
 *   KEYS[2] - lock key, hash tagged to same slot
 *   ARGV[1] - lock secret
 *   ARGV[2] - lock TTL in milliseconds
 *   ARGV[3] - session TTL in seconds (a positive value issues GETEX EX <ttl> to support early_refresh)
 *
 * Returns {locked ? 1 : 0, data}; reads data unconditionally to honour lock_failure_readonly. */
#define LOCK_RW_LUA_STR "local d;if tonumber(ARGV[3])>0 then d=redis.call('GETEX',KEYS[1],'EX',ARGV[3]) else d=redis.call('GET',KEYS[1]) end;local l;if tonumber(ARGV[2])>0 then l=redis.call('SET',KEYS[2],ARGV[1],'NX','PX',ARGV[2]) else l=redis.call('SET',KEYS[2],ARGV[1],'NX') end;return {l and 1 or 0,d or ''}"
#define LOCK_RW_SHA_STR "87c7c94521579faf92d4f7cae910d3546923c8d8"

typedef struct evalCmd {
    char *kw;
    char *str;
    size_t len;
} evalCmd;

static evalCmd lua_cmd[2] = {
    {"EVALSHA", ZEND_STRL(LOCK_DEL_SHA_STR)},
    {"EVAL", ZEND_STRL(LOCK_DEL_LUA_STR)}
};

static evalCmd lua_rw_cmd[2] = {
    {"EVALSHA", ZEND_STRL(LOCK_RW_SHA_STR)},
    {"EVAL", ZEND_STRL(LOCK_RW_LUA_STR)}
};

typedef enum lockDelCmd {
    LOCK_DEL_EVAL,
    LOCK_DEL_DELEX,
    LOCK_DEL_DELIFEQ,
} lockDelCmd;

typedef enum releaseResult {
    DEL_SUCCESS,
    DEL_FAILURE,
    DEL_NO_CMD,
} delResult;


static inline zend_bool is_redis_ok(const char *str, size_t len) {
    return len == 3 && !memcmp(str, "+OK", 3);
}

#define NEGATIVE_LOCK_RESPONSE 1

#define CLUSTER_DEFAULT_PREFIX() \
    zend_string_init(CLUSTER_SESSION_PREFIX, sizeof(CLUSTER_SESSION_PREFIX) - 1, 0)

ps_module ps_mod_redis = {
    PS_MOD_UPDATE_TIMESTAMP(redis)
};

ps_module ps_mod_redis_cluster = {
    PS_MOD_UPDATE_TIMESTAMP(rediscluster)
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
    struct redis_pool_member_ *next;

} redis_pool_member;

typedef struct {
    int totalWeight;
    int count;

    redis_pool_member *head;
    redis_session_lock_status lock_status;
    zend_bool session_key_missing;
    int buster;
} redis_pool;

static void
redis_pool_add(redis_pool *pool, RedisSock *redis_sock, int weight)
{
    redis_pool_member *rpm = ecalloc(1, sizeof(*rpm));
    rpm->redis_sock = redis_sock;
    rpm->weight = weight;

    rpm->next = pool->head;
    pool->head = rpm;

    pool->totalWeight += weight;
}

PHP_REDIS_API void
redis_pool_free(redis_pool *pool) {

    redis_pool_member *rpm, *next;

    if (pool == NULL)
        return;

    rpm = pool->head;
    while (rpm) {
        next = rpm->next;
        redis_sock_disconnect(rpm->redis_sock, 0, 1);
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

/* Retrieve session.gc_maxlifetime from php.ini protecting against an integer overflow */
static int session_gc_maxlifetime(void) {
    zend_long value = zend_ini_long_literal("session.gc_maxlifetime");
    if (value > INT_MAX) {
        php_error_docref(NULL, E_NOTICE, "session.gc_maxlifetime overflows INT_MAX, truncating.");
        return INT_MAX;
    } else if (value <= 0) {
        php_error_docref(NULL, E_NOTICE, "session.gc_maxlifetime is <= 0, defaulting to 1440 seconds");
        return 1440;
    }

    return value;
}

/* Retrieve redis.session.compression from php.ini */
static int session_compression_type(void) {
    const char *compression = zend_ini_string_literal("redis.session.compression");

    if(compression == NULL || *compression == '\0' ||
       redis_strncasecmp(compression, ZEND_STRL("none")) == 0)
    {
        return REDIS_COMPRESSION_NONE;
    }

#ifdef HAVE_REDIS_LZF
    if(redis_strncasecmp(compression, ZEND_STRL("lzf")) == 0) {
        return REDIS_COMPRESSION_LZF;
    }
#endif
#ifdef HAVE_REDIS_ZSTD
    if(redis_strncasecmp(compression, ZEND_STRL("zstd")) == 0) {
        return REDIS_COMPRESSION_ZSTD;
    }
#endif
#ifdef HAVE_REDIS_LZ4
    if(redis_strncasecmp(compression, ZEND_STRL("lz4")) == 0) {
        return REDIS_COMPRESSION_LZ4;
    }
#endif

    if (strcasecmp(compression, "lzf") ||
        strcasecmp(compression, "zstd") ||
        strcasecmp(compression, "lz4"))
    {
        php_error_docref(NULL, E_NOTICE,
            "redis.session.compression: '%s' compression is not available. "
            "Rebuild phpredis with %s support.", compression, compression);
    } else {
        php_error_docref(NULL, E_NOTICE,
            "redis.session.compression: '%s' is invalid", compression);
    }

    return REDIS_COMPRESSION_NONE;
}

/* Helper to compress session data */
static int
session_compress_data(RedisSock *redis_sock, char *data, size_t len,
                      char **compressed_data, size_t *compressed_len)
{
    if (redis_sock->compression) {
        if(redis_compress(redis_sock, compressed_data, compressed_len, data, len)) {
            return 1;
        }
    }

    *compressed_data = data;
    *compressed_len = len;

    return 0;
}

/* Helper to uncompress session data */
static int
session_uncompress_data(RedisSock *redis_sock, char *data, size_t len,
                                   char **decompressed_data, size_t *decompressed_len) {
    if (redis_sock->compression) {
        if(redis_uncompress(redis_sock, decompressed_data, decompressed_len, data, len)) {
            return 1;
        }
    }

    *decompressed_data = data;
    *decompressed_len = len;

    return 0;
}

/* Send a command to Redis.  Returns byte count written to socket (-1 on failure) */
static int redis_simple_cmd(RedisSock *redis_sock, const char *cmd, int cmdlen,
                            char **reply, int *replylen)
{
    *reply = NULL;
    int len_written = redis_sock_write(redis_sock, cmd, cmdlen);

    if (len_written >= 0) {
        *reply = redis_sock_read(redis_sock, replylen);
    }

    return len_written;
}

static inline int weighted_seed(zend_string *key) {
    /* In GCC the fast-path is one 32-bit load with a union */
    union { int pos; } u;

    if (EXPECTED(ZSTR_LEN(key) >= sizeof(u.pos))) {
        memcpy(&u.pos, ZSTR_VAL(key), sizeof(u.pos));
        return u.pos;
    }

    u.pos = 0;

    memcpy(&u.pos, ZSTR_VAL(key), ZSTR_LEN(key));

    return u.pos;
}

PHP_REDIS_API redis_pool_member *
redis_pool_get_sock(redis_pool *pool, zend_string *key) {
    redis_pool_member *rpm;
    int pos, i;

    /* In the next release, ensure pool->totalWeight > 0 */
    pos = weighted_seed(key) % (pool->totalWeight ? pool->totalWeight : 1);

    rpm = pool->head;

    for(i = 0; i < pool->totalWeight;) {
        if (pos >= i && pos < i + rpm->weight) {
            if (redis_sock_server_open(rpm->redis_sock) == 0) {
                return rpm;
            }
        }
        i += rpm->weight;
        rpm = rpm->next;
    }

    return NULL;
}

/* Helper to set our session lock key */
static int
set_session_lock_key(RedisSock *redis_sock, const char *cmd, int cmd_len)
{
    char *reply;
    int sent_len, reply_len;

    sent_len = redis_simple_cmd(redis_sock, cmd, cmd_len, &reply, &reply_len);
    if (reply) {
        if (is_redis_ok(reply, reply_len)) {
            efree(reply);
            return SUCCESS;
        }

        efree(reply);
    }

    /* Return FAILURE in case of network problems */
    return sent_len >= 0 ? NEGATIVE_LOCK_RESPONSE : FAILURE;
}

static void generate_lock_key(redis_session_lock_status *status) {
    static const char suffix[] = "_LOCK";

    if (status->lock_key)
        zend_string_release(status->lock_key);

    status->lock_key = zend_string_concat2(ZSTR_VAL(status->session_key),
                                           ZSTR_LEN(status->session_key),
                                           ZEND_STRL(suffix));
}

/* Select the lock-key form based on the full (prefixed) session key.
 * Mirrors Redis's keyHashSlot() (src/cluster.c) first-{tag} rule so
 * the derived lock key co-locates with the session key:
 *   EXISTING_TAG : non-empty first "{tag}" -> lock = <sk>_LOCK
 *   WRAP         : no braces at all        -> lock = {<sk>}_LOCK
 *   INVALID      : any other brace shape   -> reject
 *
 * Runs once per PS_READ; PHP's session-id generator never emits braces,
 * so INVALID is in practice reachable only via misconfigured
 * redis.session.prefix or explicit session_id() calls. */
typedef enum {
    LOCK_KEY_INVALID = 0,
    LOCK_KEY_EXISTING_TAG,
    LOCK_KEY_WRAP,
} cluster_lock_key_form;

static cluster_lock_key_form select_cluster_lock_key_form(const char *sk, size_t slen)
{
    int s = -1;
    size_t i;

    for (i = 0; i < slen; i++) {
        if (sk[i] == '{') {
            if (s < 0) s = (int)i;
        } else if (sk[i] == '}') {
            if (s >= 0) {
                return (i == (size_t)s + 1) ? LOCK_KEY_INVALID : LOCK_KEY_EXISTING_TAG;
            }
            if (s == -1) s = -2;
        }
    }
    return (s == -1) ? LOCK_KEY_WRAP : LOCK_KEY_INVALID;
}

/* Create a lock key that hashes to the SAME slot as the session key,
 * so the atomic acquire-and-read Lua script never trips CROSSSLOT.
 * Returns SUCCESS / FAILURE; on FAILURE the caller (PS_READ) emits a
 * diagnostic warning and fails the request */
static int generate_cluster_lock_key(redis_session_lock_status *status)
{
    const char *sk = ZSTR_VAL(status->session_key);
    size_t slen = ZSTR_LEN(status->session_key);
    size_t klen;
    cluster_lock_key_form form;
    zend_string *out;
    char *p;
    const char *prefix = "", *suffix = "";
    size_t prefix_len = 0, suffix_len = 0;

    /* Always (re)derive so lock_key stays in sync with the current session_key.
     * Caching was not worth the sid-change invalidation hazard. */
    if (status->lock_key) {
        zend_string_release(status->lock_key);
        status->lock_key = NULL;
    }

    form = select_cluster_lock_key_form(sk, slen);

    if (form == LOCK_KEY_EXISTING_TAG) {
        suffix = "_LOCK";
        suffix_len = sizeof("_LOCK") - 1;
    } else if (form == LOCK_KEY_WRAP) {
        prefix = "{";
        prefix_len = 1;
        suffix = "}_LOCK";
        suffix_len = sizeof("}_LOCK") - 1;
    } else {
        return FAILURE;
    }

    klen = prefix_len + slen + suffix_len;
    out = zend_string_alloc(klen, 0);
    p = ZSTR_VAL(out);
    memcpy(p, prefix, prefix_len);                     /* optional '{'          */
    memcpy(p + prefix_len, sk, slen);                  /* the session key bytes */
    memcpy(p + prefix_len + slen, suffix, suffix_len); /* "_LOCK" or "}_LOCK"   */
    p[klen] = '\0';                                    /* terminating NUL       */

    status->lock_key = out;
    return SUCCESS;
}

static void generate_lock_secret(redis_session_lock_status *status) {
    unsigned char buf[16];
    char hostname[HOST_NAME_MAX] = {0};

    if (status->lock_secret)
        zend_string_release(status->lock_secret);

    if (php_random_bytes_silent(buf, sizeof(buf)) == SUCCESS) {
        zend_string *s = zend_string_alloc(sizeof(buf) * 2, 0);
        php_hash_bin2hex(ZSTR_VAL(s), buf, sizeof(buf));
        ZSTR_VAL(s)[sizeof(buf) * 2] = '\0';
        status->lock_secret = s;
        return;
    }

    gethostname(hostname, HOST_NAME_MAX);
    status->lock_secret = strpprintf(0, "%s|%ld", hostname, (long)getpid());
}

static int
lock_acquire(RedisSock *redis_sock, redis_session_lock_status *lock_status) {
    zend_long wait_time, expiry, retries, attempt = 0;
    RedisCmd *cmd;
    int result;

    /* Short circuit if we are already locked or not using session locks */
    if (lock_status->is_locked || !zend_ini_long_literal("redis.session.locking_enabled"))
        return SUCCESS;

    /* How long to wait between attempts to acquire lock */
    wait_time = zend_ini_long_literal("redis.session.lock_wait_time");
    if (wait_time == 0) {
        wait_time = 20000;
    }

    /* Maximum number of times to retry (-1 means infinite) */
    retries = zend_ini_long_literal("redis.session.lock_retries");
    if (retries == 0) {
        retries = 100;
    }

    /* How long should the lock live (in seconds) */
    expiry = zend_ini_long_literal("redis.session.lock_expire");
    if (expiry == 0) {
        expiry = zend_ini_long_literal("max_execution_time");
    }

    generate_lock_key(lock_status);
    generate_lock_secret(lock_status);

    if (expiry > 0) {
        cmd = redis_cmd_fmt(redis_sock, "SET", "SSssd", lock_status->lock_key,
                             lock_status->lock_secret, ZEND_STRL("NX"), ZEND_STRL("PX"),
                             expiry * 1000);
    } else {
        cmd = redis_cmd_fmt(redis_sock, "SET", "SSs", lock_status->lock_key,
                             lock_status->lock_secret, ZEND_STRL("NX"));
    }

    /* Attempt to get our lock */
    for (;;) {
        result = set_session_lock_key(redis_sock, redis_cmd_str(cmd),
                                      redis_cmd_len(cmd));

        if (result == SUCCESS) {
            lock_status->is_locked = 1;
            break;
        } else if (result == FAILURE) {
            /* Network failure */
            break;
        }

        /* Lock is busy */

        if (retries >= 0 && attempt++ >= retries) {
            break;
        }

        usleep(wait_time);
    }

    redis_cmd_free(cmd);

    /* Success if we're locked */
    return lock_status->is_locked ? SUCCESS : FAILURE;
}

static zend_always_inline
zend_bool is_lock_secret(const char *str, size_t len, zend_string *secret) {
    return len == ZSTR_LEN(secret) && !redis_strncmp(str, ZSTR_VAL(secret), len);
}

static int
write_allowed(RedisSock *redis_sock, redis_session_lock_status *lock_status) {
    RedisCmd *cmd;

    if (!zend_ini_long_literal("redis.session.locking_enabled")) {
        return 1;
    }
    /* If locked and redis.session.lock_expire is not set => TTL=max_execution_time
       Therefore it is guaranteed that the current process is still holding the lock */

    if (lock_status->is_locked && zend_ini_long_literal("redis.session.lock_expire") != 0) {
        char *reply = NULL;
        int replylen;

        /* Command to get our lock key value and compare secrets */
        cmd = redis_cmd_fmt(redis_sock, "GET", "S", lock_status->lock_key);

        /* Attempt to refresh the lock */
        redis_simple_cmd(redis_sock, redis_cmd_str(cmd), redis_cmd_len(cmd),
                         &reply, &replylen);

        redis_cmd_free(cmd);

        if (reply == NULL) {
            lock_status->is_locked = 0;
        } else {
            lock_status->is_locked = is_lock_secret(reply, replylen,
                                                    lock_status->lock_secret);
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

static delResult
get_del_result(RedisSock *redis_sock, const char *reply, int len)
{
    zend_bool nocmd = 0;

    #define NOCMD_PFX "ERR unknown command"

    if (reply == NULL) {
        if (redis_sock->err) {
            php_error_docref(NULL, E_WARNING, "%s", ZSTR_VAL(redis_sock->err));
            nocmd = zend_string_starts_with_cstr(redis_sock->err,
                                                 ZEND_STRL(NOCMD_PFX));
            redis_sock_clear_err(redis_sock);
        }
        return nocmd ? DEL_NO_CMD : DEL_FAILURE;
    } else if (len == 4 && !redis_strncmp(reply, ZEND_STRL(":1"))) {
        return DEL_SUCCESS;
    } else {
        return DEL_FAILURE;
    }

    #undef NOCMD_PFX
}

static delResult
lock_release_delex(RedisSock *redis_sock, redis_session_lock_status *status) {
    delResult result;
    RedisCmd *cmd;
    char *reply;
    int len;

    cmd = redis_cmd_create_literal(redis_sock, "DELEX");

    redis_cmd_cat_zstr(cmd, status->lock_key);
    redis_cmd_cat_literal(cmd, "IFEQ");
    redis_cmd_cat_zstr(cmd, status->lock_secret);

    redis_simple_cmd(redis_sock, redis_cmd_str(cmd), redis_cmd_len(cmd), &reply,
                     &len);

    result = get_del_result(redis_sock, reply, len);

    if (reply) efree(reply);
    redis_cmd_free(cmd);

    return result;
}

static delResult
lock_release_delifeq(RedisSock *redis_sock, redis_session_lock_status *status) {
    delResult result;
    RedisCmd *cmd;
    char *reply;
    int len;

    cmd = redis_cmd_create_literal(redis_sock, "DELIFEQ");

    redis_cmd_cat_zstr(cmd, status->lock_key);
    redis_cmd_cat_zstr(cmd, status->lock_secret);

    redis_simple_cmd(redis_sock, redis_cmd_str(cmd), redis_cmd_len(cmd), &reply,
                     &len);

    result = get_del_result(redis_sock, reply, len);

    if (reply) efree(reply);
    redis_cmd_free(cmd);

    return result;
}

/* Release any session lock we hold and cleanup allocated lock data.  This
 * function first attempts to use EVALSHA and then falls back to EVAL if
 * EVALSHA fails.  This will cause Redis to cache the script, so subsequent
 * calls should then succeed using EVALSHA. */
static void
lock_release_lua(RedisSock *redis_sock, redis_session_lock_status *status) {
    int i, replylen;
    RedisCmd *cmd;
    char *reply;

    /* We first want to try EVALSHA and then fall back to EVAL */
    for (i = 0; status->is_locked && i < sizeof(lua_cmd)/sizeof(*lua_cmd); i++)
    {
        cmd = redis_cmd_fmt(redis_sock, lua_cmd[i].kw, "sdSS", lua_cmd[i].str,
                             lua_cmd[i].len, 1, status->lock_key,
                             status->lock_secret);

        /* Send it off */
        redis_simple_cmd(redis_sock, redis_cmd_str(cmd), redis_cmd_len(cmd),
                         &reply, &replylen);

        /* Release lock and cleanup reply if we got one */
        if (reply != NULL) {
            status->is_locked = 0;
            efree(reply);
        }

        /* Cleanup command */
        redis_cmd_free(cmd);
    }

    /* Something has failed if we are still locked */
    if (status->is_locked) {
        php_error_docref(NULL, E_WARNING, "Failed to release session lock");
    }
}

static lockDelCmd lock_release_cmd(void) {
    const char *cmd;

    cmd = zend_ini_string_literal("redis.session.lock_release_cmd");

    if (cmd == NULL) {
        return LOCK_DEL_EVAL;
    } else if (redis_strncasecmp(cmd, ZEND_STRL("DELEX")) == 0) {
        return LOCK_DEL_DELEX;
    } else if (redis_strncasecmp(cmd, ZEND_STRL("DELIFEQ")) == 0) {
        return LOCK_DEL_DELIFEQ;
    }

    return LOCK_DEL_EVAL;
}

static void
lock_release(RedisSock *redis_sock, redis_session_lock_status *status) {
    delResult res = DEL_NO_CMD;

    if (status->lock_key == NULL)
        return;

    switch (lock_release_cmd()) {
        case LOCK_DEL_DELEX:
            res = lock_release_delex(redis_sock, status);
            break;
        case LOCK_DEL_DELIFEQ:
            res = lock_release_delifeq(redis_sock, status);
            break;
        case LOCK_DEL_EVAL:
            break; /* fallthrough */
    }

    /* If res == DEL_NO_CMD LUA is selected or the new command didn't exist */
    if (res == DEL_NO_CMD)
        lock_release_lua(redis_sock, status);
}

/* {{{ PS_OPEN_FUNC */
PS_OPEN_FUNC(redis)
{
    php_url *url;
    zval params, context, *zv;
    int i, j, path_len;

#if PHP_VERSION_ID >= 80600
    const char *save_path_str = ZSTR_VAL(save_path);
    size_t save_path_len = ZSTR_LEN(save_path);
#else
    const char *save_path_str = save_path;
    size_t save_path_len = strlen(save_path);
#endif

    redis_pool *pool = ecalloc(1, sizeof(*pool));

    for (i = 0, j = 0, path_len = save_path_len; i < path_len; i = j + 1) {
        /* find beginning of url */
        while ( i< path_len && (isspace(save_path_str[i]) || save_path_str[i] == ','))
            i++;

        /* find end of url */
        j = i;
        while (j<path_len && !isspace(save_path_str[j]) && save_path_str[j] != ',')
            j++;

        if (i < j) {
            int weight = 1;
            double timeout = 86400.0, read_timeout = 0.0;
            int persistent = 0, db = -1;
            zend_long retry_interval = 0;
            zend_string *persistent_id = NULL, *prefix = NULL;
            zend_string *user = NULL, *pass = NULL;

            /* translate unix: into file: */
            if (!redis_strncmp(save_path_str+i, ZEND_STRL("unix:"))) {
                int len = j-i;
                char *path = estrndup(save_path_str+i, len);
                memcpy(path, "file:", sizeof("file:")-1);
                url = php_url_parse_ex(path, len);
                efree(path);
            } else {
                url = php_url_parse_ex(save_path_str+i, j-i);
            }

            if (!url) {
                char *path = estrndup(save_path_str+i, j-i);
                php_error_docref(NULL, E_WARNING,
                    "Failed to parse session.save_path (error at offset %d, url was '%s')", i, path);
                efree(path);

                goto fail;
            }

            ZVAL_NULL(&context);
            /* parse parameters */
            if (url->query != NULL) {
                HashTable *ht;
                char *query;
                array_init(&params);

                if (url->fragment) {
                    spprintf(&query, 0, "%s#%s", ZSTR_VAL(url->query), ZSTR_VAL(url->fragment));
                } else {
                    query = estrdup(ZSTR_VAL(url->query));
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

                if ((zv = REDIS_HASH_STR_FIND_TYPE_STATIC(ht, "stream", IS_ARRAY)) != NULL) {
                    ZVAL_ZVAL(&context, zv, 1, 0);
                }

                zval_ptr_dtor_nogc(&params);
            }

            if ((url->path == NULL && url->host == NULL) || weight <= 0 || timeout <= 0) {
                char *path = estrndup(save_path_str+i, j-i);
                php_error_docref(NULL, E_WARNING,
                    "Failed to parse session.save_path (error at offset %d, url was '%s')", i, path);
                efree(path);

                php_url_free(url);
                if (persistent_id) zend_string_release(persistent_id);
                if (prefix) zend_string_release(prefix);
                if (user) zend_string_release(user);
                if (pass) zend_string_release(pass);

                goto fail;
            }

            RedisSock *redis_sock;
            const char *persistent_id_str;
            char *addr, *scheme;
            size_t addrlen;
            int port, addr_free = 0;

            scheme = url->scheme ? ZSTR_VAL(url->scheme) : "tcp";
            if (url->host) {
                port = url->port;
                addrlen = spprintf(&addr, 0, "%s://%s", scheme, ZSTR_VAL(url->host));
                addr_free = 1;
            } else { /* unix */
                port = 0;
                addr = ZSTR_VAL(url->path);
                addrlen = strlen(addr);
            }

            persistent_id_str = persistent_id ? ZSTR_VAL(persistent_id) : NULL;
            redis_sock = redis_sock_create(REDIS_SOCK_SESSION, addr, addrlen,
                                           port, timeout, read_timeout,
                                           persistent, persistent_id_str,
                                           retry_interval);

            if (db >= 0) { /* default is -1 which leaves the choice to redis. */
                redis_sock->dbNumber = db;
            }

            redis_sock->compression = session_compression_type();
            redis_sock->compression_level = zend_ini_long_literal("redis.session.compression_level");

            redis_sock_set_context_zval(redis_sock, &context);

            redis_pool_add(pool, redis_sock, weight);
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
    } else {
        php_error_docref(NULL, E_WARNING,
            "Unable to extract any servers from session.save_path");
    }

fail:
    redis_pool_free(pool);
    PS_SET_MOD_DATA(NULL);
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
            redis_pool_member *rpm = redis_pool_get_sock(pool, pool->lock_status.session_key);

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
redis_session_key(RedisSock *redis_sock, zend_string *key) {
    if (redis_sock->prefix == NULL) {
        return zend_string_concat2(ZEND_STRL(REDIS_SESSION_PREFIX),
                                   ZSTR_VAL(key), ZSTR_LEN(key));
    }

    return zend_string_concat2(ZSTR_VAL(redis_sock->prefix),
                               ZSTR_LEN(redis_sock->prefix),
                               ZSTR_VAL(key), ZSTR_LEN(key));
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
        redis_pool_member *rpm = redis_pool_get_sock(pool, sid);

        RedisSock *redis_sock = rpm ? rpm->redis_sock : NULL;

        if (!redis_sock) {
            php_error_docref(NULL, E_NOTICE, "Redis connection not available");
            zend_string_release(sid);
            return php_session_create_id(NULL);
        }

        if (pool->lock_status.session_key)
            zend_string_release(pool->lock_status.session_key);
        pool->lock_status.session_key = redis_session_key(redis_sock, sid);

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
    int response_len;
    char *response;
    RedisCmd *cmd;

    if (ZSTR_LEN(key) < 1)
        return FAILURE;

    redis_pool *pool = PS_GET_MOD_DATA();
    redis_pool_member *rpm = redis_pool_get_sock(pool, key);
    RedisSock *redis_sock = rpm ? rpm->redis_sock : NULL;
    if (!redis_sock) {
        php_error_docref(NULL, E_WARNING, "Redis connection not available");
        return FAILURE;
    }

    /* send EXISTS command */
    zend_string *session = redis_session_key(redis_sock, key);
    cmd = redis_cmd_fmt(redis_sock, "EXISTS", "S", session);
    zend_string_release(session);

    if (redis_sock_write_cmd(redis_sock, cmd) < 0 ||
        (response = redis_sock_read(redis_sock, &response_len)) == NULL)
    {
        php_error_docref(NULL, E_WARNING, "Error communicating with Redis server");
        redis_cmd_free(cmd);
        return FAILURE;
    }

    redis_cmd_free(cmd);

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
    RedisCmd *cmd;
    int rlen, res;
    char *rstr;

    if (ZSTR_LEN(key) < 1)
        return FAILURE;

    redis_pool *pool = PS_GET_MOD_DATA();

    /* GETEX already refreshed an existing session during the read */
    if (zend_ini_long_literal("redis.session.early_refresh") &&
        !pool->session_key_missing
    ) {
        return SUCCESS;
    }

    redis_pool_member *rpm = redis_pool_get_sock(pool, key);
    RedisSock *redis_sock = rpm ? rpm->redis_sock : NULL;
    if (!redis_sock) {
        php_error_docref(NULL, E_WARNING, "Redis connection not available");
        return FAILURE;
    }

    /* send EXPIRE command */
    zend_string *session = redis_session_key(redis_sock, key);
    cmd = redis_cmd_fmt(redis_sock, "EXPIRE", "Sd", session, session_gc_maxlifetime());
    zend_string_release(session);

    if (redis_sock_write(redis_sock, redis_cmd_str(cmd), redis_cmd_len(cmd)) < 0 ||
        (rstr = redis_sock_read(redis_sock, &rlen)) == NULL)
    {
        php_error_docref(NULL, E_WARNING, "Error communicating with Redis server");
        redis_cmd_free(cmd);
        return FAILURE;
    }

    redis_cmd_free(cmd);

    /* Do the full check in the next major version */
    res = rlen == 2 && rstr[0] == ':';
    zend_bool missing = res && rstr[1] == '0';

    efree(rstr);

    if (missing) {
        return ps_write_redis(mod_data, key, val, maxlifetime);
    }

    return res ? SUCCESS : FAILURE;
}
/* }}} */

/* {{{ PS_READ_FUNC
 */
PS_READ_FUNC(redis)
{
    char *resp, *compressed_buf;
    int resp_len, compressed_free;
    size_t compressed_len;
    RedisCmd *cmd;

    if (!ZSTR_LEN(key))
        return FAILURE;

    redis_pool *pool = PS_GET_MOD_DATA();
    redis_pool_member *rpm = redis_pool_get_sock(pool, key);
    RedisSock *redis_sock = rpm ? rpm->redis_sock : NULL;
    if (!redis_sock) {
        php_error_docref(NULL, E_WARNING, "Redis connection not available");
        return FAILURE;
    }

    if (pool->lock_status.session_key)
        zend_string_release(pool->lock_status.session_key);

    pool->lock_status.session_key = redis_session_key(redis_sock, key);

    /* Update the session ttl if early refresh is enabled */
    if (zend_ini_long_literal("redis.session.early_refresh")) {
        cmd = redis_cmd_create_literal(redis_sock, "GETEX");
        redis_cmd_cat_zstr(cmd, pool->lock_status.session_key);
        redis_cmd_cat_literal(cmd, "EX");
        redis_cmd_cat_long(cmd, session_gc_maxlifetime());
    } else {
        cmd = redis_cmd_create_literal(redis_sock, "GET");
        redis_cmd_cat_zstr(cmd, pool->lock_status.session_key);
    }

    if (lock_acquire(redis_sock, &pool->lock_status) != SUCCESS) {
        if (zend_ini_long_literal("redis.session.lock_failure_readonly")) {
            // opt-in legacy behavior: readonly session
            php_error_docref(NULL, E_WARNING, "Failed to acquire session lock, session will be read only");
        } else {
            php_error_docref(NULL, E_WARNING, "Failed to acquire session lock");
            redis_cmd_free(cmd);
            return FAILURE;
        }
    }

    if (redis_sock_write_cmd(redis_sock, cmd) < 0) {
        php_error_docref(NULL, E_WARNING, "Error communicating with Redis server");
        redis_cmd_free(cmd);
        return FAILURE;
    }

    redis_cmd_free(cmd);

    /* Read response from Redis.  If we get a NULL response from redis_sock_read
     * this can indicate an error, OR a "NULL bulk" reply (empty session data)
     * in which case we can reply with success. */
    if ((resp = redis_sock_read(redis_sock, &resp_len)) == NULL && resp_len != -1) {
        php_error_docref(NULL, E_WARNING, "Error communicating with Redis server");
        return FAILURE;
    }

    pool->session_key_missing = resp_len < 0;

    if (pool->session_key_missing) {
        *val = ZSTR_EMPTY_ALLOC();
    } else {
        compressed_free = session_uncompress_data(redis_sock, resp, resp_len, &compressed_buf, &compressed_len);
        *val = zend_string_init(compressed_buf, compressed_len, 0);
        if (compressed_free) {
            efree(compressed_buf); // Free the buffer allocated by redis_uncompress
        }
    }

    efree(resp);

    return SUCCESS;
}
/* }}} */

/* {{{ PS_WRITE_FUNC
 */
PS_WRITE_FUNC(redis)
{
    char *response;
    int response_len, compressed_free;
    size_t svallen;
    RedisCmd *cmd;
    char *sval;

    if (ZSTR_LEN(key) < 1)
        return FAILURE;

    redis_pool *pool = PS_GET_MOD_DATA();
    redis_pool_member *rpm = redis_pool_get_sock(pool, key);
    RedisSock *redis_sock = rpm ? rpm->redis_sock : NULL;
    if (!redis_sock) {
        php_error_docref(NULL, E_WARNING, "Redis connection not available");
        return FAILURE;
    }

    /* send SET command */
    zend_string *session = redis_session_key(redis_sock, key);

    compressed_free = session_compress_data(redis_sock, ZSTR_VAL(val), ZSTR_LEN(val),
                                            &sval, &svallen);

    cmd = redis_cmd_fmt(redis_sock, "SETEX", "Sds", session,
                        session_gc_maxlifetime(), sval, svallen);
    zend_string_release(session);
    if (compressed_free) {
        efree(sval);
    }

    if (!write_allowed(redis_sock, &pool->lock_status)) {
        php_error_docref(NULL, E_WARNING, "Unable to write session: session lock not held");
        redis_cmd_free(cmd);
        return FAILURE;
    }

    if (redis_sock_write_cmd(redis_sock, cmd) < 0 ||
        (response = redis_sock_read(redis_sock, &response_len)) == NULL)
    {
        php_error_docref(NULL, E_WARNING, "Error communicating with Redis server");
        redis_cmd_free(cmd);
        return FAILURE;
    }

    redis_cmd_free(cmd);

    if (is_redis_ok(response, response_len)) {
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
    RedisCmd *cmd;
    int rlen, res;
    char *rstr;

    redis_pool *pool = PS_GET_MOD_DATA();
    redis_pool_member *rpm = redis_pool_get_sock(pool, key);
    RedisSock *redis_sock = rpm ? rpm->redis_sock : NULL;
    if (!redis_sock) {
        php_error_docref(NULL, E_WARNING, "Redis connection not available");
        return FAILURE;
    }

    /* Release lock */
    lock_release(redis_sock, &pool->lock_status);

    /* send DEL command */
    zend_string *session = redis_session_key(redis_sock, key);
    cmd = redis_cmd_fmt(redis_sock, "DEL", "S", session);
    zend_string_release(session);
    if (redis_sock_write_cmd(redis_sock, cmd) < 0 ||
        (rstr = redis_sock_read(redis_sock, &rlen)) == NULL)
    {
        php_error_docref(NULL, E_WARNING, "Error communicating with Redis server");
        redis_cmd_free(cmd);
        return FAILURE;
    }

    redis_cmd_free(cmd);

    res = rlen == 2 && (!memcmp(rstr, ":0", 2) || !memcmp(rstr, ":1", 2));

    efree(rstr);

    return res ? SUCCESS : FAILURE;
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

/* Bundle the cluster handle with the lock state so the lock_status
 * (mirroring how redis_pool carries lock_status for standalone). */
typedef struct {
    redisCluster *cluster;
    redis_session_lock_status lock_status;
} redis_cluster_session;

static redis_cluster_session *cluster_session_alloc(redisCluster *c) {
    redis_cluster_session *rcs = ecalloc(1, sizeof(*rcs));
    rcs->cluster = c;
    return rcs;
}

static void cluster_session_free(redis_cluster_session *rcs) {
    if (rcs == NULL) return;

    if (rcs->lock_status.session_key) zend_string_release(rcs->lock_status.session_key);
    if (rcs->lock_status.lock_secret) zend_string_release(rcs->lock_status.lock_secret);
    if (rcs->lock_status.lock_key)    zend_string_release(rcs->lock_status.lock_key);

    if (rcs->cluster) cluster_free(rcs->cluster, 1);
    efree(rcs);
}

/* Send a single command at the given slot. Returns SUCCESS (+OK = locked),
 * NEGATIVE_LOCK_RESPONSE (nil = busy), or FAILURE (transport error). */
static int cluster_set_session_lock_key(redisCluster *c, RedisCmd *cmd, short slot) {
    clusterReply *reply;
    int result;

    /* Must go to the slot master, not a replica */
    c->readonly = 0;
    if (cluster_send_rcmd_ex(c, slot, cmd) < 0 || c->err) {
        return FAILURE;
    }

    /* status_strings=1: +OK reply has len>0, nil bulk has len<=0. */
    reply = cluster_read_resp(c, 1);
    if (!reply || c->err) {
        if (reply) cluster_free_reply(reply, 1);
        return FAILURE;
    }

    result = (reply->len > 0) ? SUCCESS : NEGATIVE_LOCK_RESPONSE;
    cluster_free_reply(reply, 1);
    return result;
}

/* Cluster equivalent of lock_acquire(). Idempotent on lock_status: re-entry
 * is safe because generate_cluster_lock_key() and generate_lock_secret() are
 * deterministic on session_key and process identity respectively. */
static int cluster_lock_acquire(redisCluster *c, redis_session_lock_status *lock_status)
{
    zend_long wait_time, expiry, retries, attempt = 0;
    int result;
    RedisCmd *cmd;
    short slot;

    if (lock_status->is_locked || !zend_ini_long_literal("redis.session.locking_enabled"))
        return SUCCESS;

    wait_time = zend_ini_long_literal("redis.session.lock_wait_time");
    if (wait_time == 0) wait_time = 20000;

    retries = zend_ini_long_literal("redis.session.lock_retries");
    if (retries == 0) retries = 100;

    expiry = zend_ini_long_literal("redis.session.lock_expire");
    if (expiry == 0) expiry = zend_ini_long_literal("max_execution_time");

    /* Defensive re-derive in case we're ever called outside PS_READ */
    if (generate_cluster_lock_key(lock_status) != SUCCESS) {
        return FAILURE;
    }
    if (!lock_status->lock_secret) {
        generate_lock_secret(lock_status);
    }

    if (expiry > 0) {
        cmd = redis_cmd_fmt(NULL, "SET", "SSssd",
                            lock_status->lock_key, lock_status->lock_secret,
                            ZEND_STRL("NX"), ZEND_STRL("PX"), expiry * 1000);
    } else {
        cmd = redis_cmd_fmt(NULL, "SET", "SSs",
                            lock_status->lock_key, lock_status->lock_secret,
                            ZEND_STRL("NX"));
    }
    slot = cluster_hash_key_zstr(lock_status->lock_key);

    /* Attempt to get our lock */
    for (;;) {
        result = cluster_set_session_lock_key(c, cmd, slot);

        if (result == SUCCESS) {
            lock_status->is_locked = 1;
            break;
        } else if (result == FAILURE) {
            /* Network failure */
            break;
        }

        /* Lock is busy */

        if (retries >= 0 && attempt++ >= retries) {
            break;
        }

        usleep(wait_time);
    }

    /* Cleanup SET command */
    redis_cmd_free(cmd);

    /* Success if we're locked */
    return lock_status->is_locked ? SUCCESS : FAILURE;
}

/* Cluster equivalent of write_allowed(). GETs the lock key and verifies the
 * value still matches our secret; warns and clears is_locked if not. */
static int cluster_write_allowed(redisCluster *c, redis_session_lock_status *lock_status)
{
    if (!zend_ini_long_literal("redis.session.locking_enabled")) {
        return 1;
    }
    /* If locked and redis.session.lock_expire is not set => TTL=max_execution_time
       Therefore it is guaranteed that the current process is still holding the lock */

    if (lock_status->is_locked && zend_ini_long_literal("redis.session.lock_expire") != 0) {
        RedisCmd *cmd;
        short slot;
        clusterReply *reply = NULL;

        /* Command to get our lock key value and compare secrets */
        cmd = redis_cmd_fmt(NULL, "GET", "S", lock_status->lock_key);
        slot = cluster_hash_key_zstr(lock_status->lock_key);

        /* Must go to the slot master, not a replica */
        c->readonly = 0;

        /* Attempt to refresh the lock */
        if (cluster_send_rcmd_ex(c, slot, cmd) >= 0 && !c->err) {
            reply = cluster_read_resp(c, 0);
            if (c->err && reply) {
                cluster_free_reply(reply, 1);
                reply = NULL;
            }
        }
        /* Cleanup */
        redis_cmd_free(cmd);

        if (reply == NULL) {
            lock_status->is_locked = 0;
        } else {
            lock_status->is_locked = is_lock_secret(reply->str, reply->len, lock_status->lock_secret);
            cluster_free_reply(reply, 1);
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

static delResult
cluster_get_del_result(redisCluster *c, clusterReply *reply)
{
    zend_bool nocmd = 0;

    #define NOCMD_PFX "ERR unknown command"

    if (reply == NULL || c->err) {
        if (c->err) {
            php_error_docref(NULL, E_WARNING, "%s", ZSTR_VAL(c->err));
            nocmd = zend_string_starts_with_cstr(c->err, ZEND_STRL(NOCMD_PFX));
            zend_string_release(c->err);
            c->err = NULL;
        }
        return nocmd ? DEL_NO_CMD : DEL_FAILURE;
    } else if (reply->integer == 1) {
        return DEL_SUCCESS;
    } else {
        return DEL_FAILURE;
    }

    #undef NOCMD_PFX
}

/* Send a release command (DELEX or DELIFEQ) and parse the reply. */
static delResult cluster_send_release_cmd(redisCluster *c, short slot, RedisCmd *cmd)
{
    clusterReply *reply;
    delResult result;

    /* Scrub stale c->err so cluster_get_del_result classifies this call's
     * error (in particular "ERR unknown command" → DEL_NO_CMD so the
     * caller can fall back to LUA), not whatever the previous call left. */
    if (c->err) {
        zend_string_release(c->err);
        c->err = NULL;
    }

    c->readonly = 0;
    if (cluster_send_rcmd_ex(c, slot, cmd) < 0) {
        return DEL_FAILURE;
    }

    reply = cluster_read_resp(c, 0);
    result = cluster_get_del_result(c, reply);
    if (reply) cluster_free_reply(reply, 1);

    return result;
}

static delResult
cluster_lock_release_delex(redisCluster *c, redis_session_lock_status *status)
{
    RedisCmd *cmd;
    short slot;
    delResult result;

    cmd = redis_cmd_fmt(NULL, "DELEX", "SsS", status->lock_key,
                        ZEND_STRL("IFEQ"), status->lock_secret);
    slot = cluster_hash_key_zstr(status->lock_key);

    result = cluster_send_release_cmd(c, slot, cmd);

    redis_cmd_free(cmd);

    return result;
}

static delResult
cluster_lock_release_delifeq(redisCluster *c, redis_session_lock_status *status)
{
    RedisCmd *cmd;
    short slot;
    delResult result;

    cmd = redis_cmd_fmt(NULL, "DELIFEQ", "SS", status->lock_key,
                        status->lock_secret);
    slot = cluster_hash_key_zstr(status->lock_key);

    result = cluster_send_release_cmd(c, slot, cmd);

    redis_cmd_free(cmd);

    return result;
}

/* Release any session lock we hold and cleanup allocated lock data.  This
 * function first attempts to use EVALSHA and then falls back to EVAL if
 * EVALSHA fails.  This will cause Redis to cache the script, so subsequent
 * calls should then succeed using EVALSHA. */
static void
cluster_lock_release_lua(redisCluster *c, redis_session_lock_status *status)
{
    int i;
    RedisCmd *cmd;
    short slot;
    clusterReply *reply;

    slot = cluster_hash_key_zstr(status->lock_key);

    /* We first want to try EVALSHA and then fall back to EVAL */
    for (i = 0; status->is_locked && i < (int)(sizeof(lua_cmd) / sizeof(*lua_cmd)); i++) {
        cmd = redis_cmd_fmt(NULL, lua_cmd[i].kw, "sdSS",
                            lua_cmd[i].str, lua_cmd[i].len, 1,
                            status->lock_key, status->lock_secret);

        c->readonly = 0;
        if (cluster_send_rcmd_ex(c, slot, cmd) < 0) {
            redis_cmd_free(cmd);
            continue;
        }
        redis_cmd_free(cmd);

        reply = cluster_read_resp(c, 0);
        if (reply && !c->err) {
            status->is_locked = 0;
        }
        if (reply) cluster_free_reply(reply, 1);
    }

    /* Something has failed if we are still locked */
    if (status->is_locked) {
        php_error_docref(NULL, E_WARNING, "Failed to release session lock");
    }
}

static void
cluster_lock_release(redisCluster *c, redis_session_lock_status *status)
{
    delResult res = DEL_NO_CMD;

    if (status->lock_key == NULL)
        return;

    switch (lock_release_cmd()) {
        case LOCK_DEL_DELEX:
            res = cluster_lock_release_delex(c, status);
            break;
        case LOCK_DEL_DELIFEQ:
            res = cluster_lock_release_delifeq(c, status);
            break;
        case LOCK_DEL_EVAL:
            break; /* fallthrough */
    }

    /* If res == DEL_NO_CMD LUA is selected or the new command didn't exist */
    if (res == DEL_NO_CMD)
        cluster_lock_release_lua(c, status);
}

/* Plain session data read for PS_READ. Used when locking is disabled and
 * after a successful retry-acquire (the pre-lock read is discarded since
 * another writer may have mutated the data before we got the lock).
 *
 * Returns SUCCESS on transport success and writes the data into *out_data
 * (zero-length zend_string when the session key is missing). */
static int cluster_read_session_data(redisCluster *c,
                                     zend_string *session_key,
                                     zend_long session_ttl_seconds,
                                     zend_string **out_data)
{
    RedisCmd *cmd;
    short slot;
    clusterReply *reply;

    slot = cluster_hash_key_zstr(session_key);

    /* Must go to the slot master, not a replica */
    if (session_ttl_seconds > 0) {
        cmd = redis_cmd_fmt(NULL, "GETEX", "Ssd", session_key,
                            ZEND_STRL("EX"), session_ttl_seconds);
    } else {
        cmd = redis_cmd_fmt(NULL, "GET", "S", session_key);
    }
    c->readonly = 0;

    if (cluster_send_rcmd_ex(c, slot, cmd) < 0 || c->err) {
        redis_cmd_free(cmd);
        return FAILURE;
    }
    redis_cmd_free(cmd);

    reply = cluster_read_resp(c, 0);
    if (!reply || c->err) {
        if (reply) cluster_free_reply(reply, 1);
        return FAILURE;
    }

    if (reply->str == NULL) {
        *out_data = ZSTR_EMPTY_ALLOC();
    } else {
        *out_data = zend_string_init(reply->str, reply->len, 0);
    }

    cluster_free_reply(reply, 1);
    return SUCCESS;
}

/* Atomic acquire-and-read for PS_READ via EVAL/EVALSHA against
 * KEYS=[session_data, lock_key]. SUCCESS means the EVAL completed; the
 * caller inspects lock_status->is_locked and reads *out_data (zero-length
 * when the key is missing). Tries EVALSHA first, falls back to EVAL on
 * NOSCRIPT (which also warms the script cache for subsequent EVALSHA hits). */
static int cluster_lock_acquire_and_read(redisCluster *c,
                                         redis_session_lock_status *lock_status,
                                         zend_string *session_key,
                                         zend_long session_ttl_seconds,
                                         zend_string **out_data)
{
    zend_long expiry;
    zend_long lock_ms;
    RedisCmd *cmd;
    int i;
    short slot;
    clusterReply *reply;
    int got_response = 0;

    expiry = zend_ini_long_literal("redis.session.lock_expire");
    if (expiry == 0) expiry = zend_ini_long_literal("max_execution_time");
    lock_ms = expiry > 0 ? expiry * 1000 : 0;

    /* Caller must have generated the lock key (slot/co-location decided
     * up front). Lock secret is process-stable; (re)generate if missing. */
    if (!lock_status->lock_key) {
        return FAILURE;
    }
    if (!lock_status->lock_secret) {
        generate_lock_secret(lock_status);
    }

    /* Both keys hash to the same slot (caller-confirmed). */
    slot = cluster_hash_key_zstr(lock_status->lock_key);

    /* We first want to try EVALSHA and then fall back to EVAL */
    for (i = 0; i < (int)(sizeof(lua_rw_cmd) / sizeof(*lua_rw_cmd)); i++) {
        /* Clear prior NOSCRIPT error so the EVAL fallback can run */
        if (c->err) {
            zend_string_release(c->err);
            c->err = NULL;
        }

        /* EVAL[SHA] <script> 2 <session_key> <lock_key> <secret> <lock_ms> <session_ttl> */
        cmd = redis_cmd_fmt(NULL, lua_rw_cmd[i].kw, "sdSSSdd",
                            lua_rw_cmd[i].str, lua_rw_cmd[i].len,
                            2,
                            session_key, lock_status->lock_key,
                            lock_status->lock_secret,
                            lock_ms,
                            session_ttl_seconds);

        c->readonly = 0;
        if (cluster_send_rcmd_ex(c, slot, cmd) < 0) {
            redis_cmd_free(cmd);
            /* Transport failure — EVAL fallback can't help */
            return FAILURE;
        }
        redis_cmd_free(cmd);

        reply = cluster_read_resp(c, 0);
        if (reply && reply->type == TYPE_MULTIBULK && reply->elements >= 2) {
            clusterReply *r_locked = reply->element[0];
            clusterReply *r_data   = reply->element[1];
            zend_long locked_int = 0;

            /* Lua returns RESP integer per spec; accept bulk "1"/"0" too so
             * a RESP3 proxy can't force a wasted retry after we hold the lock */
            if (r_locked) {
                if (r_locked->type == TYPE_INT) {
                    locked_int = r_locked->integer;
                } else if (r_locked->type == TYPE_BULK && r_locked->str
                           && r_locked->len > 0)
                {
                    locked_int = strtol(r_locked->str, NULL, 10);
                }
            }

            if (locked_int == 1) {
                lock_status->is_locked = 1;
            }

            if (r_data && r_data->str) {
                *out_data = zend_string_init(r_data->str, r_data->len, 0);
            } else {
                *out_data = ZSTR_EMPTY_ALLOC();
            }

            cluster_free_reply(reply, 1);
            got_response = 1;
            break;
        }

        if (reply) cluster_free_reply(reply, 1);

        /* NOSCRIPT or unrecognised reply — let EVAL try next iteration */
    }

    /* Drop residual error so the caller's retry/write paths don't inherit it */
    if (c->err) {
        zend_string_release(c->err);
        c->err = NULL;
    }

    return got_response ? SUCCESS : FAILURE;
}

/* Prefix a session key */
static zend_string *
cluster_session_key(redisCluster *c, zend_string *key, short *slot) {
    if (ZSTR_LEN(c->flags->prefix) > ZSTR_MAX_LEN - ZSTR_LEN(key)) {
        zend_error_noreturn(E_ERROR,
            "Prefixing overflows the maximum allowed key length");
    }

    if (ZSTR_LEN(c->flags->prefix) > 0) {
        key = zend_string_concat2(ZSTR_VAL(c->flags->prefix),
                                  ZSTR_LEN(c->flags->prefix),
                                  ZSTR_VAL(key), ZSTR_LEN(key));
    } else {
        key = zend_string_copy(key);
    }

    *slot = cluster_hash_key_zstr(key);

    return key;
}

PS_OPEN_FUNC(rediscluster) {
    redisCluster *c;
    zval z_conf, *zv, *context;
    HashTable *ht_conf, *ht_seeds;
    double timeout = 0, read_timeout = 0;
    int persistent = 0, failover = REDIS_FAILOVER_NONE;
    zend_string *prefix = NULL, *user = NULL, *pass = NULL, *failstr = NULL;

#if PHP_VERSION_ID >= 80600
    const char *save_path_str = ZSTR_VAL(save_path);
#else
    const char *save_path_str = save_path;
#endif

    /* Parse configuration for session handler */
    array_init(&z_conf);
    sapi_module.treat_data(PARSE_STRING, estrdup(save_path_str), &z_conf);

    /* We need seeds */
    zv = REDIS_HASH_STR_FIND_TYPE_STATIC(Z_ARRVAL(z_conf), "seed", IS_ARRAY);
    if (zv == NULL) {
        zval_ptr_dtor_nogc(&z_conf);
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
        zval_ptr_dtor_nogc(&z_conf);
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
        zval_ptr_dtor_nogc(&z_conf); \

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

    c->flags->compression = session_compression_type();
    c->flags->compression_level = zend_ini_long_literal("redis.session.compression_level");

    redis_sock_set_auth(c->flags, user, pass);

    if ((context = REDIS_HASH_STR_FIND_TYPE_STATIC(ht_conf, "stream", IS_ARRAY)) != NULL) {
        redis_sock_set_context_zval(c->flags, context);
    }

    /* First attempt to load from cache */
    if (cluster_caching_enabled()) {
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
    PS_SET_MOD_DATA(cluster_session_alloc(c));
    return SUCCESS;

failure:
    CLUSTER_SESSION_CLEANUP();
    cluster_free(c, 1);
    return FAILURE;
}

/* {{{ PS_CREATE_SID_FUNC
 */
PS_CREATE_SID_FUNC(rediscluster)
{
    redis_cluster_session *rcs = PS_GET_MOD_DATA();
    redisCluster *c = rcs ? rcs->cluster : NULL;
    clusterReply *reply;
    zend_string *sid, *key;
    RedisCmd *cmd;
    int retries = 3;
    short slot;

    if (!c) {
        return php_session_create_id(NULL);
    }

    if (zend_ini_long_literal("session.use_strict_mode") == 0) {
        return php_session_create_id((void **) &c);
    }

    while (retries-- > 0) {
        sid = php_session_create_id((void **) &c);

        /* Create session key if it doesn't already exist */
        key = cluster_session_key(c, sid, &slot);
        cmd = redis_cmd_fmt(NULL, "SET", "Ssssd", key,
                            ZEND_STRL(""), ZEND_STRL("NX"), ZEND_STRL("EX"),
                            session_gc_maxlifetime());

        zend_string_release(key);

        /* Attempt to kick off our command */
        c->readonly = 0;
        if (cluster_send_rcmd_ex(c, slot, cmd) < 0 || c->err) {
            php_error_docref(NULL, E_NOTICE, "Redis connection not available");
            redis_cmd_free(cmd);
            zend_string_release(sid);
            return php_session_create_id(NULL);;
        }

        redis_cmd_free(cmd);

        /* Attempt to read reply */
        reply = cluster_read_resp(c, 1);

        if (!reply || c->err) {
            php_error_docref(NULL, E_NOTICE, "Unable to read redis response");
        } else if (reply->len > 0) {
            cluster_free_reply(reply, 1);
            break;
        } else {
            php_error_docref(NULL, E_NOTICE, "Redis sid collision on %s, retrying %d time(s)", sid->val, retries);
        }

        if (reply) {
            cluster_free_reply(reply, 1);
        }

        zend_string_release(sid);
        sid = NULL;
    }

    return sid;
}
/* }}} */

/* {{{ PS_VALIDATE_SID_FUNC
 */
PS_VALIDATE_SID_FUNC(rediscluster)
{
    redis_cluster_session *rcs = PS_GET_MOD_DATA();
    redisCluster *c = rcs ? rcs->cluster : NULL;
    clusterReply *reply;
    int res = FAILURE;
    RedisCmd *cmd;
    short slot;

    /* Check key is valid and whether it already exists */
    if (php_session_valid_key(ZSTR_VAL(key)) == FAILURE) {
        php_error_docref(NULL, E_NOTICE,
            "Invalid session key: %s", ZSTR_VAL(key));
        return FAILURE;
    }

    key = cluster_session_key(c, key, &slot);
    cmd = redis_cmd_fmt(NULL, "EXISTS", "S", key);

    zend_string_release(key);

    /* We send to master, to ensure consistency */
    c->readonly = 0;
    if (cluster_send_rcmd_ex(c, slot, cmd) < 0 || c->err) {
        php_error_docref(NULL, E_NOTICE, "Redis connection not available");
        redis_cmd_free(cmd);
        return FAILURE;
    }

    redis_cmd_free(cmd);

    /* Attempt to read reply */
    reply = cluster_read_resp(c, 0);

    if (!reply || c->err) {
        php_error_docref(NULL, E_NOTICE, "Unable to read redis response");
        res = FAILURE;
    } else if (reply->integer == 1) {
        res = SUCCESS;
    }

     /* Clean up */
    if (reply) {
        cluster_free_reply(reply, 1);
    }

    return res;
}
/* }}} */

/* {{{ PS_UPDATE_TIMESTAMP_FUNC
 */
PS_UPDATE_TIMESTAMP_FUNC(rediscluster) {
    redis_cluster_session *rcs = PS_GET_MOD_DATA();
    redisCluster *c = rcs ? rcs->cluster : NULL;
    clusterReply *reply;
    RedisCmd *cmd;
    short slot;

    /* GETEX already refreshed an existing session during the read */
    if (zend_ini_long_literal("redis.session.early_refresh") &&
        !c->session_key_missing
    ) {
        return SUCCESS;
    }

    /* Set up command and slot info */
    zend_string *session = cluster_session_key(c, key, &slot);
    cmd = redis_cmd_fmt(NULL, "EXPIRE", "Sd", session, session_gc_maxlifetime());

    zend_string_release(session);

    /* Attempt to send EXPIRE command */
    c->readonly = 0;
    if (cluster_send_rcmd_ex(c, slot, cmd) < 0 || c->err) {
        php_error_docref(NULL, E_NOTICE, "Redis unable to update session expiry");
        redis_cmd_free(cmd);
        return FAILURE;
    }

    /* Clean up our command */
    redis_cmd_free(cmd);

    /* Attempt to read reply */
    reply = cluster_read_resp(c, 0);
    if (!reply || c->err) {
        if (reply) cluster_free_reply(reply, 1);
        return FAILURE;
    }

    zend_bool missing = reply->type == TYPE_INT && reply->integer == 0;

    /* Clean up */
    cluster_free_reply(reply, 1);

    if (missing) {
        return ps_write_rediscluster(mod_data, key, val, maxlifetime);
    }

    return SUCCESS;
}
/* }}} */

/* {{{ PS_READ_FUNC
 *
 * Locking enabled: atomic SET-NX + GET/GETEX via Lua on the slot owner
 * (lock_key co-located with session_key via hash tag). On lock contention
 * we retry with plain SET NX and re-fetch under the lock on success.
 * Prefixes that can't produce a co-located lock key fail PS_READ with a
 * diagnostic rather than silently degrading.
 *
 * Locking disabled: plain GET (or GETEX with early_refresh). No Lua, no
 * lock keys, no prefix validation. */
PS_READ_FUNC(rediscluster) {
    redis_cluster_session *rcs = PS_GET_MOD_DATA();
    redisCluster *c = rcs ? rcs->cluster : NULL;
    clusterReply *reply;
    char *compressed_buf;
    RedisCmd *cmd;
    int compressed_free;
    size_t compressed_len;
    short slot;
    zend_string *raw_data = NULL;

    if (!c) return FAILURE;

    /* Build the prefixed session key once. The locking-enabled branch below
     * transfers ownership into lock_status.session_key; the disabled branch
     * uses it for the GET/GETEX command and releases it. */
    key = cluster_session_key(c, key, &slot);

    if (zend_ini_long_literal("redis.session.locking_enabled")) {
        zend_long ttl = zend_ini_long_literal("redis.session.early_refresh")
                          ? session_gc_maxlifetime() : 0;
        int rv;

        if (rcs->lock_status.session_key) {
            zend_string_release(rcs->lock_status.session_key);
        }
        rcs->lock_status.session_key = key; /* transfer ownership */

        /* Co-located lock key required by the atomic Lua; malformed brace
         * usage in the prefix is a hard failure. */
        if (generate_cluster_lock_key(&rcs->lock_status) != SUCCESS) {
            php_error_docref(NULL, E_WARNING,
                "Cluster session prefix must contain no braces or a non-empty {hash-tag}");
            return FAILURE;
        }

        rv = cluster_lock_acquire_and_read(c, &rcs->lock_status,
                                           rcs->lock_status.session_key,
                                           ttl, &raw_data);

        if (rv != SUCCESS) {
            return FAILURE;
        }

        if (!rcs->lock_status.is_locked) {
            /* Lua didn't get the lock — the read is potentially stale and
             * MUST NOT be reused on retry-success (lost-update race).
             *   1. Retry SUCCESS - re-fetch under the lock.
             *   2. Retry FAILURE + lock_failure_readonly=1 - keep stale
             *      read, warn, let PS_WRITE refuse via cluster_write_allowed.
             *   3. Retry FAILURE + lock_failure_readonly=0 - fail PS_READ. */
            int retry_rv = cluster_lock_acquire(c, &rcs->lock_status);

            if (retry_rv == SUCCESS) {
                if (raw_data) {
                    zend_string_release(raw_data);
                    raw_data = NULL;
                }
                if (cluster_read_session_data(c, rcs->lock_status.session_key,
                                              ttl, &raw_data) != SUCCESS)
                {
                    return FAILURE;
                }
            } else if (zend_ini_long_literal("redis.session.lock_failure_readonly")) {
                php_error_docref(NULL, E_WARNING,
                    "Failed to acquire session lock, session will be read only");
            } else {
                php_error_docref(NULL, E_WARNING, "Failed to acquire session lock");
                if (raw_data) zend_string_release(raw_data);
                return FAILURE;
            }
        }

        /* An empty read means the session key was absent; PS_UPDATE_TIMESTAMP
         * reads this to skip the redundant EXPIRE when early_refresh is on. */
        c->session_key_missing = raw_data == NULL || ZSTR_LEN(raw_data) == 0;

        /* Decompress and hand off */
        if (raw_data == NULL) {
            *val = ZSTR_EMPTY_ALLOC();
        } else if (ZSTR_LEN(raw_data) == 0) {
            *val = raw_data;            /* hand ownership to caller */
        } else {
            compressed_free = session_uncompress_data(c->flags,
                ZSTR_VAL(raw_data), ZSTR_LEN(raw_data),
                &compressed_buf, &compressed_len);
            *val = zend_string_init(compressed_buf, compressed_len, 0);
            if (compressed_free) {
                efree(compressed_buf);
            }
            zend_string_release(raw_data);
        }
        return SUCCESS;
    }

    /* Locking disabled: GETEX writes the TTL and goes to master, plain
     * GET is replica-OK via failover=distribute. Master-only reads
     * require enabling locking. */
    /* Update the session ttl if early refresh is enabled */
    if (zend_ini_long_literal("redis.session.early_refresh")) {
        cmd = redis_cmd_fmt(NULL, "GETEX", "Ssd", key, ZEND_STRL("EX"),
                            session_gc_maxlifetime());
        c->readonly = 0;
    } else {
        cmd = redis_cmd_fmt(NULL, "GET", "S", key);
        c->readonly = 1;
    }

    zend_string_release(key);

    /* Attempt to kick off our command */
    if (cluster_send_rcmd_ex(c, slot, cmd) < 0 || c->err) {
        redis_cmd_free(cmd);
        return FAILURE;
    }

    /* Clean up command */
    redis_cmd_free(cmd);

    /* Attempt to read reply */
    reply = cluster_read_resp(c, 0);
    if (!reply || c->err) {
        if (reply) cluster_free_reply(reply, 1);
        return FAILURE;
    }

    /* Push reply value to caller */
    c->session_key_missing = reply->len == -1;

    if (c->session_key_missing) {
        *val = ZSTR_EMPTY_ALLOC();
    } else {
        compressed_free = session_uncompress_data(c->flags, reply->str, reply->len, &compressed_buf, &compressed_len);
        *val = zend_string_init(compressed_buf, compressed_len, 0);
        if (compressed_free) {
            efree(compressed_buf); // Free the buffer allocated by redis_uncompress
        }
    }

    /* Clean up */
    cluster_free_reply(reply, 1);

    /* Success! */
    return SUCCESS;
}

/* {{{ PS_WRITE_FUNC
 */
PS_WRITE_FUNC(rediscluster) {
    redis_cluster_session *rcs = PS_GET_MOD_DATA();
    redisCluster *c = rcs ? rcs->cluster : NULL;
    clusterReply *reply;
    char *sval;
    RedisCmd *cmd;
    int compressed_free;
    size_t svallen;
    short slot;

    if (!c) return FAILURE;

    /* If locking is enabled but our lock is no longer held (expired or
     * stolen), refuse to write — same semantics as the standalone handler. */
    if (!cluster_write_allowed(c, &rcs->lock_status)) {
        return FAILURE;
    }

    compressed_free = session_compress_data(c->flags, ZSTR_VAL(val), ZSTR_LEN(val),
                                            &sval, &svallen);

    /* Set up command and slot info */
    key = cluster_session_key(c, key, &slot);
    cmd = redis_cmd_fmt(NULL, "SETEX", "Sds", key, session_gc_maxlifetime(),
                        sval, svallen);

    zend_string_release(key);
    if (compressed_free) {
        efree(sval);
    }

    /* Attempt to send command */
    c->readonly = 0;
    if (cluster_send_rcmd_ex(c, slot, cmd) < 0 || c->err) {
        redis_cmd_free(cmd);
        return FAILURE;
    }

    /* Clean up our command */
    redis_cmd_free(cmd);

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
    redis_cluster_session *rcs = PS_GET_MOD_DATA();
    redisCluster *c = rcs ? rcs->cluster : NULL;
    clusterReply *reply;
    RedisCmd *cmd;
    short slot;

    if (!c) return FAILURE;

    /* Set up command and slot info */
    key = cluster_session_key(c, key, &slot);

    cmd = redis_cmd_fmt(NULL, "DEL", "S", key);
    zend_string_release(key);

    /* Attempt to send command */
    if (cluster_send_rcmd_ex(c, slot, cmd) < 0 || c->err) {
        redis_cmd_free(cmd);
        return FAILURE;
    }

    /* Clean up our command */
    redis_cmd_free(cmd);

    /* Attempt to read reply */
    reply = cluster_read_resp(c, 0);
    if (!reply || c->err) {
        if (reply) cluster_free_reply(reply, 1);
        return FAILURE;
    }

    /* Clean up our reply */
    cluster_free_reply(reply, 1);

    /* Release the lock if we hold one — destroyed sessions don't need a
     * dangling lock. */
    cluster_lock_release(c, &rcs->lock_status);

    return SUCCESS;
}

/* {{{ PS_CLOSE_FUNC
 */
PS_CLOSE_FUNC(rediscluster)
{
    redis_cluster_session *rcs = PS_GET_MOD_DATA();
    if (rcs) {
        if (rcs->cluster) {
            cluster_lock_release(rcs->cluster, &rcs->lock_status);
        }
        cluster_session_free(rcs);
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
