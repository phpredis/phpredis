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
  | Author: Michael Grunder <michael.grunder@gmail.com>                  |
  | Maintainer: Nicolas Favre-Felix <n.favre-felix@owlient.eu>           |
  +----------------------------------------------------------------------+
*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "common.h"
#include "php_redis.h"
#include "ext/standard/info.h"
#include "crc16.h"
#include "redis_cluster.h"
#include "redis_commands.h"
#include <ext/spl/spl_exceptions.h>
#include <zend_exceptions.h>
#include "library.h"
#include <php_variables.h>
#include <SAPI.h>

zend_class_entry *redis_cluster_ce;

/* Exception handler */
zend_class_entry *redis_cluster_exception_ce;

extern RedisCmdCtx redis_empty_ctx;

#if PHP_VERSION_ID < 80000
#include "redis_cluster_legacy_arginfo.h"
#else
#include "zend_attributes.h"
#include "redis_cluster_arginfo.h"
#endif

static void
cluster_enqueue_response(redisCluster *c, short slot, cluster_cb cb, RedisCmdCtx ctx)
{
    clusterFoldItem *item;

    item = emalloc(sizeof(clusterFoldItem));
    item->callback = cb;
    item->slot = slot;
    item->ctx = ctx;
    item->next = NULL;
    item->flags = c->flags->flags;

    if (UNEXPECTED(c->multi_head == NULL)) {
        c->multi_head = item;
        c->multi_curr = item;
    } else {
        c->multi_curr->next = item;
        c->multi_curr = item;
    }
}

static void cluster_free_queue(redisCluster *c) {
    clusterFoldItem *item = c->multi_head, *tmp;

    while (item) {
        tmp = item->next;
        redis_cmd_ctx_free(item->ctx);
        efree(item);
        item = tmp;
    }

    c->multi_head = NULL;
    c->multi_curr = NULL;
}

static void cluster_reset_multi(redisCluster *c) {
    redisClusterNode *node;
    ZEND_HASH_FOREACH_PTR(c->nodes, node) {
        if (node == NULL)
            continue;
        node->sock->watching = 0;
        node->sock->mode = ATOMIC;
    } ZEND_HASH_FOREACH_END();

    c->flags->watching = 0;
    c->flags->mode = ATOMIC;
}

void
cluster_process_cmd(INTERNAL_FUNCTION_PARAMETERS, redisCluster *c,
                    redis_cmd_cb cmd_cb, cluster_cb resp_cb, int readonly)
{
    RedisCmdCtx ctx;
    RedisCmd *cmd;
    short slot;

    c->readonly = readonly && CLUSTER_IS_ATOMIC(c);

    cmd = cmd_cb(INTERNAL_FUNCTION_PARAM_PASSTHRU, c->flags);
    if (cmd == NULL) {
        RETURN_FALSE;
    }

    ctx = redis_cmd_pop_ctx(cmd);
    slot = cmd->slot;

    if (cluster_send_rcmd(c, cmd) < 0 || c->err != NULL) {
        redis_cmd_ctx_free(ctx);
        redis_cmd_free(cmd);
        RETURN_FALSE;
    }

    redis_cmd_free(cmd);

    if (c->flags->mode == MULTI) {
        cluster_enqueue_response(c, slot, resp_cb, ctx);
        RETURN_ZVAL(getThis(), 1, 0);
    }

    resp_cb(INTERNAL_FUNCTION_PARAM_PASSTHRU, c, ctx);
    redis_cmd_ctx_free(ctx);
}

void
cluster_process_kw_cmd(INTERNAL_FUNCTION_PARAMETERS, redisCluster *c,
                       const char *kw, redis_kw_cmd_cb cmd_cb, cluster_cb resp_cb,
                       int readonly)
{
    RedisCmdCtx ctx;
    RedisCmd *cmd;
    short slot;

    c->readonly = readonly && CLUSTER_IS_ATOMIC(c);

    cmd = cmd_cb(INTERNAL_FUNCTION_PARAM_PASSTHRU, c->flags, (char*)kw);
    if (cmd == NULL) {
        RETURN_FALSE;
    }

    ctx = redis_cmd_pop_ctx(cmd);
    slot = cmd->slot;

    if (cluster_send_rcmd(c, cmd) < 0 || c->err != NULL)
    {
        redis_cmd_ctx_free(ctx);
        redis_cmd_free(cmd);
        RETURN_FALSE;
    }

    redis_cmd_free(cmd);

    if (c->flags->mode == MULTI) {
        cluster_enqueue_response(c, slot, resp_cb, ctx);
        RETURN_ZVAL(getThis(), 1, 0);
    }

    resp_cb(INTERNAL_FUNCTION_PARAM_PASSTHRU, c, ctx);
    redis_cmd_ctx_free(ctx);
}

PHP_MINIT_FUNCTION(redis_cluster)
{
    redis_cluster_ce = register_class_RedisCluster();
    redis_cluster_ce->create_object = create_cluster_context;

    redis_cluster_exception_ce = register_class_RedisClusterException(spl_ce_RuntimeException);

    return SUCCESS;
}

/* Handlers for RedisCluster */
zend_object_handlers RedisCluster_handlers;

/* Our context seeds will be a hash table with RedisSock* pointers */
static void ht_free_seed(zval *data) {
    RedisSock *redis_sock = *(RedisSock**)data;
    if (redis_sock) redis_free_socket(redis_sock);
}

/* Free redisClusterNode objects we've stored */
static void ht_free_node(zval *data) {
    redisClusterNode *node = *(redisClusterNode**)data;
    cluster_free_node(node);
}

/* Create redisCluster context */
zend_object * create_cluster_context(zend_class_entry *class_type) {
    redisCluster *cluster;

    // Allocate our actual struct
    cluster = ecalloc(1, sizeof(redisCluster) + zend_object_properties_size(class_type));

    // We're not currently subscribed anywhere
    cluster->subscribed_slot = -1;

    // Allocate our RedisSock we'll use to store prefix/serialization flags
    cluster->flags = ecalloc(1, sizeof(RedisSock));

    // Allocate our hash table for seeds
    ALLOC_HASHTABLE(cluster->seeds);
    zend_hash_init(cluster->seeds, 0, NULL, ht_free_seed, 0);

    // Allocate our hash table for connected Redis objects
    ALLOC_HASHTABLE(cluster->nodes);
    zend_hash_init(cluster->nodes, 0, NULL, ht_free_node, 0);

    // Initialize it
    zend_object_std_init(&cluster->std, class_type);

    object_properties_init(&cluster->std, class_type);
    memcpy(&RedisCluster_handlers, zend_get_std_object_handlers(), sizeof(RedisCluster_handlers));
    RedisCluster_handlers.offset = XtOffsetOf(redisCluster, std);
    RedisCluster_handlers.free_obj = free_cluster_context;
    RedisCluster_handlers.clone_obj = NULL;

    cluster->std.handlers = &RedisCluster_handlers;

    return &cluster->std;
}

/* Free redisCluster context */
void free_cluster_context(zend_object *object) {
    redisCluster *cluster = PHPREDIS_GET_OBJECT(redisCluster, object);

    cluster_free(cluster, 0);
    zend_object_std_dtor(&cluster->std);
}

/* Take user provided seeds and return unique and valid ones */
/* Attempt to connect to a Redis cluster provided seeds and timeout options */
static void redis_cluster_init(redisCluster *c, HashTable *ht_seeds, double timeout,
                               double read_timeout, int persistent, zend_string *user,
                               zend_string *pass, zval *context)
{
    zend_string *hash = NULL, **seeds;
    redisCachedCluster *cc;
    uint32_t nseeds;
    char *err;

    /* Validate our arguments and get a sanitized seed array */
    seeds = cluster_validate_args(timeout, read_timeout, ht_seeds, &nseeds, &err);
    if (seeds == NULL) {
        CLUSTER_THROW_EXCEPTION(err, 0);
        return;
    }

    if (user && ZSTR_LEN(user))
        c->flags->user = zend_string_copy(user);
    if (pass && ZSTR_LEN(pass))
        c->flags->pass = zend_string_copy(pass);
    if (context)
        redis_sock_set_context_zval(c->flags, context);

    c->flags->type = REDIS_SOCK_CLUSTER;
    c->flags->timeout = timeout;
    c->flags->read_timeout = read_timeout;
    c->flags->persistent = persistent;
    c->waitms = (long)(1000 * (timeout + read_timeout));

    /* Attempt to load slots from cache if caching is enabled */
    if (CLUSTER_CACHING_ENABLED()) {
        /* Exit early if we can load from cache */
        hash = cluster_hash_seeds(seeds, nseeds);
        if ((cc = cluster_cache_load(hash))) {
            cluster_init_cache(c, cc);
            goto cleanup;
        }
    }

    /* Initialize seeds and attempt to map keyspace */
    cluster_init_seeds(c, seeds, nseeds);
    if (cluster_map_keyspace(c) == SUCCESS && hash)
        cluster_cache_store(hash, c->nodes);

cleanup:
    if (hash) zend_string_release(hash);
    free_seed_array(seeds, nseeds);
}


/* Attempt to load a named cluster configured in php.ini */
void redis_cluster_load(redisCluster *c, char *name, int name_len) {
    zval z_seeds, z_tmp, *z_value;
    zend_string *user = NULL, *pass = NULL;
    double timeout = 0, read_timeout = 0;
    int persistent = 0;
    char *iptr;
    HashTable *ht_seeds = NULL;

    /* Seeds */
    array_init(&z_seeds);
    if ((iptr = INI_STR("redis.clusters.seeds")) != NULL) {
        sapi_module.treat_data(PARSE_STRING, estrdup(iptr), &z_seeds);
    }
    if ((z_value = zend_hash_str_find(Z_ARRVAL(z_seeds), name, name_len)) != NULL) {
        ht_seeds = Z_ARRVAL_P(z_value);
    } else {
        zval_ptr_dtor_nogc(&z_seeds);
        CLUSTER_THROW_EXCEPTION("Couldn't find seeds for cluster", 0);
        return;
    }

    /* Connection timeout */
    if ((iptr = INI_STR("redis.clusters.timeout")) != NULL) {
        array_init(&z_tmp);
        sapi_module.treat_data(PARSE_STRING, estrdup(iptr), &z_tmp);
        redis_conf_double(Z_ARRVAL(z_tmp), name, name_len, &timeout);
        zval_ptr_dtor_nogc(&z_tmp);
    }

    /* Read timeout */
    if ((iptr = INI_STR("redis.clusters.read_timeout")) != NULL) {
        array_init(&z_tmp);
        sapi_module.treat_data(PARSE_STRING, estrdup(iptr), &z_tmp);
        redis_conf_double(Z_ARRVAL(z_tmp), name, name_len, &read_timeout);
        zval_ptr_dtor_nogc(&z_tmp);
    }

    /* Persistent connections */
    if ((iptr = INI_STR("redis.clusters.persistent")) != NULL) {
        array_init(&z_tmp);
        sapi_module.treat_data(PARSE_STRING, estrdup(iptr), &z_tmp);
        redis_conf_bool(Z_ARRVAL(z_tmp), name, name_len, &persistent);
        zval_ptr_dtor_nogc(&z_tmp);
    }

    if ((iptr = INI_STR("redis.clusters.auth"))) {
        array_init(&z_tmp);
        sapi_module.treat_data(PARSE_STRING, estrdup(iptr), &z_tmp);
        redis_conf_auth(Z_ARRVAL(z_tmp), name, name_len, &user, &pass);
        zval_ptr_dtor_nogc(&z_tmp);
    }

    /* Attempt to create/connect to the cluster */
    redis_cluster_init(c, ht_seeds, timeout, read_timeout, persistent, user, pass, NULL);

    /* Clean up */
    zval_ptr_dtor_nogc(&z_seeds);
    if (user) zend_string_release(user);
    if (pass) zend_string_release(pass);
}

/*
 * PHP Methods
 */

/* Create a RedisCluster Object */
PHP_METHOD(RedisCluster, __construct) {
    zval *z_seeds = NULL, *z_auth = NULL, *context = NULL;
    zend_string *user = NULL, *pass = NULL;
    double timeout = 0.0, read_timeout = 0.0;
    size_t name_len;
    zend_bool persistent = 0;
    redisCluster *c = GET_CONTEXT();
    char *name;

    ZEND_PARSE_PARAMETERS_START(1, 7)
        Z_PARAM_STRING_OR_NULL(name, name_len)
        Z_PARAM_OPTIONAL
        Z_PARAM_ARRAY_OR_NULL(z_seeds)
        Z_PARAM_DOUBLE(timeout)
        Z_PARAM_DOUBLE(read_timeout)
        Z_PARAM_BOOL(persistent)
        Z_PARAM_ZVAL(z_auth)
        Z_PARAM_ARRAY_OR_NULL(context)
    ZEND_PARSE_PARAMETERS_END_EX(RETURN_FALSE);

    /* If we've got a string try to load from INI */
    if (ZEND_NUM_ARGS() < 2 || z_seeds == NULL) {
        if (name_len == 0) { // Require a name
            CLUSTER_THROW_EXCEPTION("You must specify a name or pass seeds!", 0);
        }
        redis_cluster_load(c, name, name_len);
        return;
    }

    /* The normal case, loading from arguments */
    redis_extract_auth_info(z_auth, &user, &pass);
    redis_cluster_init(c, Z_ARRVAL_P(z_seeds), timeout, read_timeout,
                       persistent, user, pass, context);

    if (user) zend_string_release(user);
    if (pass) zend_string_release(pass);
}

/*
 * RedisCluster method implementation
 */

/* {{{ proto bool RedisCluster::close() */
PHP_METHOD(RedisCluster, close) {
    cluster_disconnect(GET_CONTEXT(), 1);
    RETURN_TRUE;
}

/* {{{ proto string RedisCluster::get(string key) */
PHP_METHOD(RedisCluster, get) {
    CLUSTER_PROCESS_KW_CMD("GET", redis_key_cmd, cluster_bulk_resp, 1);
}
/* }}} */

/* {{{ proto string RedisCluster::getdel(string key) */
PHP_METHOD(RedisCluster, getdel) {
    CLUSTER_PROCESS_KW_CMD("GETDEL", redis_key_cmd, cluster_bulk_resp, 1);
}
/* }}} */

/* {{{ proto array|false RedisCluster::getWithMeta(string key) */
PHP_METHOD(RedisCluster, getWithMeta) {
    CLUSTER_PROCESS_KW_CMD("GET", redis_key_cmd, cluster_bulk_withmeta_resp, 1);
}
/* }}} */


/* {{{ proto bool RedisCluster::set(string key, string value) */
PHP_METHOD(RedisCluster, set) {
    CLUSTER_PROCESS_CMD(set, cluster_set_resp, 0);
}
/* }}} */

static void cluster_multi_ctx_dtor(void *ptr)
{
    clusterMultiCtx *mctx = ptr;

    if (mctx->last) {
        efree(mctx->z_multi);
    }
    efree(mctx);
}

/* Generic handler for MGET/MSET/MSETNX */
static int
distcmd_resp_handler(INTERNAL_FUNCTION_PARAMETERS, redisCluster *c, short slot,
                     clusterMultiCmd *mc, zval *z_ret, int last, cluster_cb cb)
{
    clusterMultiCtx *mctx;
    RedisCmdCtx ctx = {0};

    // Finalize multi command
    cluster_multi_fini(mc);

    // Spin up multi context
    mctx = emalloc(sizeof(clusterMultiCtx));
    mctx->z_multi = z_ret;
    mctx->count   = mc->argc;
    mctx->last    = last;

    // Attempt to send the command
    if (cluster_send_rcmd_ex(c, slot, mc->cmd) < 0 || c->err != NULL)
    {
        efree(mctx);
        return -1;
    }

    ctx.ptr = mctx;
    ctx.dtor = cluster_multi_ctx_dtor;

    if (CLUSTER_IS_ATOMIC(c)) {
        // Process response now
        cb(INTERNAL_FUNCTION_PARAM_PASSTHRU, c, ctx);
        redis_cmd_ctx_free(ctx);
    } else {
        cluster_enqueue_response(c, slot, cb, ctx);
    }

    // Clear out our command but retain allocated memory
    CLUSTER_MULTI_CLEAR(mc);

    return 0;
}

/* Container struct for a key/value pair pulled from an array */
typedef struct clusterKeyValHT {
    char kbuf[22];

    char  *key;
    size_t key_len;
    int key_free;
    short slot;

    char *val;
    size_t val_len;
    int val_free;
} clusterKeyValHT;

/* Helper to pull a key/value pair from a HashTable */
static int get_key_val_ht(redisCluster *c, HashTable *ht, HashPosition *ptr,
                          clusterKeyValHT *kv)
{
    zval *z_val;
    zend_ulong idx;

    // Grab the key, convert it to a string using provided kbuf buffer if it's
    // a LONG style key
    zend_string *zkey;
    switch (zend_hash_get_current_key_ex(ht, &zkey, &idx, ptr)) {
        case HASH_KEY_IS_STRING:
            kv->key_len = ZSTR_LEN(zkey);
            kv->key = ZSTR_VAL(zkey);
            break;
        case HASH_KEY_IS_LONG:
            kv->key_len = snprintf(kv->kbuf,sizeof(kv->kbuf),"%ld",(long)idx);
            kv->key     = kv->kbuf;
            break;
        default:
            CLUSTER_THROW_EXCEPTION("Internal Zend HashTable error", 0);
            return -1;
    }

    // Prefix our key if we need to, set the slot
    kv->key_free = redis_key_prefix(c->flags, &(kv->key), &(kv->key_len));
    kv->slot     = cluster_hash_key(kv->key, kv->key_len);

    // Now grab our value
    if ((z_val = zend_hash_get_current_data_ex(ht, ptr)) == NULL) {
        CLUSTER_THROW_EXCEPTION("Internal Zend HashTable error", 0);
        return -1;
    }

    // Serialize our value if required
    kv->val_free = redis_pack(c->flags,z_val,&(kv->val),&(kv->val_len));

    // Success
    return 0;
}

/* Helper to pull, prefix, and hash a key from a HashTable value */
static int get_key_ht(redisCluster *c, HashTable *ht, HashPosition *ptr,
                      clusterKeyValHT *kv)
{
    zval *z_key;

    if ((z_key = zend_hash_get_current_data_ex(ht, ptr)) == NULL) {
        // Shouldn't happen, but check anyway
        CLUSTER_THROW_EXCEPTION("Internal Zend HashTable error", 0);
        return -1;
    }

    // Always want to work with strings
    convert_to_string(z_key);

    kv->key = Z_STRVAL_P(z_key);
    kv->key_len = Z_STRLEN_P(z_key);
    kv->key_free = redis_key_prefix(c->flags, &(kv->key), &(kv->key_len));

    // Hash our key
    kv->slot = cluster_hash_key(kv->key, kv->key_len);

    // Success
    return 0;
}

/* Turn variable arguments into a HashTable for processing */
static HashTable *method_args_to_ht(zval *z_args, int argc) {
    HashTable *ht_ret;
    int i;

    /* Allocate our hash table */
    ALLOC_HASHTABLE(ht_ret);
    zend_hash_init(ht_ret, argc, NULL, NULL, 0);

    /* Populate our return hash table with our arguments */
    for (i = 0; i < argc; i++) {
        zend_hash_next_index_insert(ht_ret, &z_args[i]);
    }

    /* Return our hash table */
    return ht_ret;
}

/* Convenience handler for commands that take multiple keys such as
 * MGET, DEL, and UNLINK */
static int cluster_mkey_cmd(INTERNAL_FUNCTION_PARAMETERS, char *kw, int kw_len,
                            zval *z_ret, cluster_cb cb)
{
    redisCluster *c = GET_CONTEXT();
    clusterMultiCmd mc = {0};
    clusterKeyValHT kv;
    zval *z_args;
    HashTable *ht_arr;
    HashPosition ptr;
    int i = 1, argc = ZEND_NUM_ARGS(), ht_free = 0;
    short slot;

    /* If we don't have any arguments we're invalid */
    if (!argc) return -1;

    /* Extract our arguments into an array */
    z_args = ecalloc(argc, sizeof(zval));
    if (zend_get_parameters_array(ht, argc, z_args) == FAILURE) {
        efree(z_args);
        return -1;
    }

    /* Determine if we're working with a single array or variadic args */
    if (argc == 1 && Z_TYPE(z_args[0]) == IS_ARRAY) {
        ht_arr = Z_ARRVAL(z_args[0]);
        argc = zend_hash_num_elements(ht_arr);
        if (!argc) {
            efree(z_args);
            return -1;
        }
    } else {
        ht_arr = method_args_to_ht(z_args, argc);
        ht_free = 1;
    }

    /* MGET is readonly, DEL is not */
    c->readonly = kw_len == 4 && CLUSTER_IS_ATOMIC(c);

    // Initialize our "multi" command handler with command/len
    CLUSTER_MULTI_INIT(mc, kw, kw_len);

    // Process the first key outside of our loop, so we don't have to check if
    // it's the first iteration every time, needlessly
    zend_hash_internal_pointer_reset_ex(ht_arr, &ptr);
    if (get_key_ht(c, ht_arr, &ptr, &kv) < 0) {
        efree(z_args);
        return -1;
    }

    // Process our key and add it to the command
    cluster_multi_add(&mc, kv.key, kv.key_len);

    // Free key if we prefixed
    if (kv.key_free) efree(kv.key);

    // Move to the next key
    zend_hash_move_forward_ex(ht_arr, &ptr);

    // Iterate over keys 2...N
    slot = kv.slot;
    while (zend_hash_has_more_elements_ex(ht_arr, &ptr) ==SUCCESS) {
        if (get_key_ht(c, ht_arr, &ptr, &kv) < 0) {
            cluster_multi_free(&mc);
            if (ht_free) {
                zend_hash_destroy(ht_arr);
                efree(ht_arr);
            }
            efree(z_args);
            return -1;
        }

        // If the slots have changed, kick off the keys we've aggregated
        if (slot != kv.slot) {
            // Process this batch of MGET keys
            if (distcmd_resp_handler(INTERNAL_FUNCTION_PARAM_PASSTHRU, c, slot,
                                    &mc, z_ret, i == argc, cb) < 0)
            {
                cluster_multi_free(&mc);
                if (ht_free) {
                    zend_hash_destroy(ht_arr);
                    efree(ht_arr);
                }
                efree(z_args);
                return -1;
            }
        }

        // Add this key to the command
        cluster_multi_add(&mc, kv.key, kv.key_len);

        // Free key if we prefixed
        if (kv.key_free) efree(kv.key);

        // Update the last slot we encountered, and the key we're on
        slot = kv.slot;
        i++;

        zend_hash_move_forward_ex(ht_arr, &ptr);
    }
    efree(z_args);

    // If we've got straggler(s) process them
    if (mc.argc > 0) {
        if (distcmd_resp_handler(INTERNAL_FUNCTION_PARAM_PASSTHRU, c, slot,
                                &mc, z_ret, 1, cb) < 0)
        {
            cluster_multi_free(&mc);
            if (ht_free) {
                zend_hash_destroy(ht_arr);
                efree(ht_arr);
            }
            return -1;
        }
    }

    // Free our command
    cluster_multi_free(&mc);

    /* Clean up our hash table if we constructed it from variadic args */
    if (ht_free) {
        zend_hash_destroy(ht_arr);
        efree(ht_arr);
    }

    /* Return our object if we're in MULTI mode */
    if (!CLUSTER_IS_ATOMIC(c))
        RETVAL_ZVAL(getThis(), 1, 0);

    // Success
    return 0;
}

/* Handler for both MSET and MSETNX */
static int cluster_mset_cmd(INTERNAL_FUNCTION_PARAMETERS, char *kw, int kw_len,
                            zval *z_ret, cluster_cb cb)
{
    redisCluster *c = GET_CONTEXT();
    clusterKeyValHT kv;
    clusterMultiCmd mc = {0};
    zval *z_arr;
    HashTable *ht_arr;
    HashPosition ptr;
    int i = 1, argc;
    short slot;

    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_ARRAY(z_arr)
    ZEND_PARSE_PARAMETERS_END_EX(return FAILURE);

    // No reason to send zero args
    ht_arr = Z_ARRVAL_P(z_arr);
    if ((argc = zend_hash_num_elements(ht_arr)) == 0) {
        return -1;
    }

    /* This is a write command */
    c->readonly = 0;

    // Set up our multi command handler
    CLUSTER_MULTI_INIT(mc, kw, kw_len);

    // Process the first key/value pair outside of our loop
    zend_hash_internal_pointer_reset_ex(ht_arr, &ptr);
    if (get_key_val_ht(c, ht_arr, &ptr, &kv) ==-1) return -1;
    zend_hash_move_forward_ex(ht_arr, &ptr);

    // Add this to our multi cmd, set slot, free key if we prefixed
    cluster_multi_add(&mc, kv.key, kv.key_len);
    cluster_multi_add(&mc, kv.val, kv.val_len);
    if (kv.key_free) efree(kv.key);
    if (kv.val_free) efree(kv.val);

    // While we've got more keys to set
    slot = kv.slot;
    while (zend_hash_has_more_elements_ex(ht_arr, &ptr) ==SUCCESS) {
        // Pull the next key/value pair
        if (get_key_val_ht(c, ht_arr, &ptr, &kv) ==-1) {
            return -1;
        }

        // If the slots have changed, process responses
        if (slot != kv.slot) {
            if (distcmd_resp_handler(INTERNAL_FUNCTION_PARAM_PASSTHRU, c,
                                    slot, &mc, z_ret, i == argc, cb) < 0)
            {
                cluster_multi_free(&mc);
                return -1;
            }
        }

        // Add this key and value to our command
        cluster_multi_add(&mc, kv.key, kv.key_len);
        cluster_multi_add(&mc, kv.val, kv.val_len);

        // Free our key and value if we need to
        if (kv.key_free) efree(kv.key);
        if (kv.val_free) efree(kv.val);

        // Update our slot, increment position
        slot = kv.slot;
        i++;

        // Move on
        zend_hash_move_forward_ex(ht_arr, &ptr);
    }

    // If we've got stragglers, process them too
    if (mc.argc > 0) {
        if (distcmd_resp_handler(INTERNAL_FUNCTION_PARAM_PASSTHRU, c, slot, &mc,
                                z_ret, 1, cb) < 0)
        {
            cluster_multi_free(&mc);
            return -1;
        }
    }

    // Free our command
    cluster_multi_free(&mc);

    /* Return our object if we're in MULTI mode */
    if (!CLUSTER_IS_ATOMIC(c))
        RETVAL_ZVAL(getThis(), 1, 0);

    // Success
    return 0;
}

/* Generic passthru for DEL and UNLINK which act identically */
static void cluster_generic_delete(INTERNAL_FUNCTION_PARAMETERS,
                                   char *kw, int kw_len)
{
    zval *z_ret = emalloc(sizeof(*z_ret));

    // Initialize a LONG value to zero for our return
    ZVAL_LONG(z_ret, 0);

    // Parse args, process
    if (cluster_mkey_cmd(INTERNAL_FUNCTION_PARAM_PASSTHRU, kw, kw_len, z_ret,
                        cluster_del_resp) < 0)
    {
        efree(z_ret);
        RETURN_FALSE;
    }
}

/* {{{ proto array RedisCluster::del(string key1, string key2, ... keyN) */
PHP_METHOD(RedisCluster, del) {
    cluster_generic_delete(INTERNAL_FUNCTION_PARAM_PASSTHRU, ZEND_STRL("DEL"));
}

PHP_METHOD(RedisCluster, delex) {
    CLUSTER_PROCESS_CMD(delex, cluster_long_resp, 0);
}

PHP_METHOD(RedisCluster, delifeq) {
    CLUSTER_PROCESS_KW_CMD("DELIFEQ", redis_kv_cmd, cluster_long_resp, 0);
}

/* {{{ proto array RedisCluster::unlink(string key1, string key2, ... keyN) */
PHP_METHOD(RedisCluster, unlink) {
    cluster_generic_delete(INTERNAL_FUNCTION_PARAM_PASSTHRU, "UNLINK", sizeof("UNLINK") - 1);
}

PHP_METHOD(RedisCluster, msetex) {
    CLUSTER_PROCESS_CMD(msetex, cluster_long_resp, 0);
}

/* {{{ proto array RedisCluster::mget(array keys) */
PHP_METHOD(RedisCluster, mget) {
    zval *z_ret = emalloc(sizeof(*z_ret));

    array_init(z_ret);

    // Parse args, process
    if (cluster_mkey_cmd(INTERNAL_FUNCTION_PARAM_PASSTHRU, ZEND_STRL("MGET"),
                         z_ret, cluster_mbulk_mget_resp) < 0)
    {
        zval_ptr_dtor_nogc(z_ret);
        efree(z_ret);
        RETURN_FALSE;
    }
}

/* {{{ proto bool RedisCluster::mset(array keyvalues) */
PHP_METHOD(RedisCluster, mset) {
    zval *z_ret = emalloc(sizeof(*z_ret));

    ZVAL_TRUE(z_ret);

    // Parse args and process.  If we get a failure, free zval and return FALSE.
    if (cluster_mset_cmd(INTERNAL_FUNCTION_PARAM_PASSTHRU, ZEND_STRL("MSET"),
                         z_ret, cluster_mset_resp) == -1)
    {
        efree(z_ret);
        RETURN_FALSE;
    }
}

/* {{{ proto array RedisCluster::msetnx(array keyvalues) */
PHP_METHOD(RedisCluster, msetnx) {
    zval *z_ret = emalloc(sizeof(*z_ret));

    array_init(z_ret);

    // Parse args and process.  If we get a failure, free mem and return FALSE
    if (cluster_mset_cmd(INTERNAL_FUNCTION_PARAM_PASSTHRU, ZEND_STRL("MSETNX"),
                         z_ret, cluster_msetnx_resp) ==-1)
    {
        zval_ptr_dtor_nogc(z_ret);
        efree(z_ret);
        RETURN_FALSE;
    }
}
/* }}} */

PHP_METHOD(RedisCluster, getex) {
    CLUSTER_PROCESS_CMD(getex, cluster_bulk_resp, 0);
}

/* {{{ proto bool RedisCluster::setex(string key, string value, int expiry) */
PHP_METHOD(RedisCluster, setex) {
    CLUSTER_PROCESS_KW_CMD("SETEX", redis_key_long_val_cmd, cluster_bool_resp, 0);
}
/* }}} */

/* {{{ proto bool RedisCluster::psetex(string key, string value, int expiry) */
PHP_METHOD(RedisCluster, psetex) {
    CLUSTER_PROCESS_KW_CMD("PSETEX", redis_key_long_val_cmd, cluster_bool_resp, 0);
}
/* }}} */

/* {{{ proto bool RedisCluster::setnx(string key, string value) */
PHP_METHOD(RedisCluster, setnx) {
    CLUSTER_PROCESS_KW_CMD("SETNX", redis_kv_cmd, cluster_1_resp, 0);
}
/* }}} */

/* {{{ proto string RedisCluster::getSet(string key, string value) */
PHP_METHOD(RedisCluster, getset) {
    CLUSTER_PROCESS_KW_CMD("GETSET", redis_kv_cmd, cluster_bulk_resp, 0);
}
/* }}} */

/* {{{ proto int RedisCluster::exists(string $key, string ...$more_keys) */
PHP_METHOD(RedisCluster, exists) {
    CLUSTER_PROCESS_KW_CMD("EXISTS", redis_varkey_cmd, cluster_long_resp, 1);
}
/* }}} */

/* {{{ proto int RedisCluster::exists(string $key, string ...$more_keys) */
PHP_METHOD(RedisCluster, touch) {
    CLUSTER_PROCESS_KW_CMD("TOUCH", redis_varkey_cmd, cluster_long_resp, 0);
}

static zend_always_inline int
cluster_send_slot_cmd(redisCluster *c, short slot, RedisCmd *cmd,
                      REDIS_REPLY_TYPE type)
{
    const char *str;
    size_t len;

    cmd->slot = slot;

    str = redis_cmd_str(cmd);
    len = redis_cmd_len(cmd);

    return cluster_send_slot(c, slot, str, len, type);
}

/* }}} */
/* {{{ proto array Redis::keys(string pattern) */
PHP_METHOD(RedisCluster, keys) {
    redisCluster *c = GET_CONTEXT();
    redisClusterNode *node;
    clusterReply *resp;
    zend_string *pat;
    RedisCmd *cmd;
    int i;

    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_STR(pat)
    ZEND_PARSE_PARAMETERS_END_EX(RETURN_FALSE);

    /* Prefix and then build our command */
    cmd = redis_cmd_create_literal(c->flags, "KEYS");

    redis_cmd_cat_key_zstr(cmd, pat);

    array_init(return_value);

    /* Treat as readonly */
    c->readonly = CLUSTER_IS_ATOMIC(c);

    /* Iterate over our known nodes */
    ZEND_HASH_FOREACH_PTR(c->nodes, node) {
        if (node == NULL) continue;
        if (cluster_send_slot_cmd(c, node->slot, cmd, TYPE_MULTIBULK) < 0) {
            php_error_docref(0, E_ERROR, "Can't send KEYS to %s:%d",
                ZSTR_VAL(node->sock->host), node->sock->port);
            zval_ptr_dtor_nogc(return_value);
            redis_cmd_free(cmd);
            RETURN_FALSE;
        }

        /* Ensure we can get a response */
        resp = cluster_read_resp(c, 0);
        if (!resp) {
            php_error_docref(0, E_WARNING,
                "Can't read response from %s:%d", ZSTR_VAL(node->sock->host),
                node->sock->port);
            continue;
        }

        /* Iterate keys, adding to our big array */
        for(i = 0; i < resp->elements; i++) {
            /* Skip non bulk responses, they should all be bulk */
            if (resp->element[i]->type != TYPE_BULK) {
                continue;
            }

            add_next_index_stringl(return_value, resp->element[i]->str,
                resp->element[i]->len);
        }

        /* Free response, don't free data */
        cluster_free_reply(resp, 1);
    } ZEND_HASH_FOREACH_END();

    redis_cmd_free(cmd);
}
/* }}} */

/* {{{ proto int RedisCluster::type(string key) */
PHP_METHOD(RedisCluster, type) {
    CLUSTER_PROCESS_KW_CMD("TYPE", redis_key_cmd, cluster_type_resp, 1);
}
/* }}} */

/* {{{ proto string RedisCluster::pop(string key, [int count = 0]) */
PHP_METHOD(RedisCluster, lpop) {
    CLUSTER_PROCESS_KW_CMD("LPOP", redis_pop_cmd, cluster_pop_resp, 0);
}
/* }}} */

PHP_METHOD(RedisCluster, lpos) {
    CLUSTER_PROCESS_CMD(lpos, cluster_lpos_resp, 1);
}

/* {{{ proto string RedisCluster::rpop(string key, [int count = 0]) */
PHP_METHOD(RedisCluster, rpop) {
    CLUSTER_PROCESS_KW_CMD("RPOP", redis_pop_cmd, cluster_pop_resp, 0);
}
/* }}} */

/* {{{ proto bool RedisCluster::lset(string key, long index, string val) */
PHP_METHOD(RedisCluster, lset) {
    CLUSTER_PROCESS_KW_CMD("LSET", redis_key_long_val_cmd, cluster_bool_resp, 0);
}
/* }}} */

/* {{{ proto string RedisCluster::spop(string key) */
PHP_METHOD(RedisCluster, spop) {
    if (ZEND_NUM_ARGS() == 1) {
        CLUSTER_PROCESS_KW_CMD("SPOP", redis_key_cmd, cluster_bulk_resp, 0);
    } else if (ZEND_NUM_ARGS() == 2) {
        CLUSTER_PROCESS_KW_CMD("SPOP", redis_key_long_cmd, cluster_mbulk_resp, 0);
    } else {
        zend_wrong_param_count();
    }
}
/* }}} */

/* {{{ proto string|array RedisCluster::srandmember(string key, [long count]) */
PHP_METHOD(RedisCluster, srandmember) {
    CLUSTER_PROCESS_KW_CMD("SRANDMEMBER", redis_randmember_cmd,
                           cluster_randmember_resp, 1);
}

/* {{{ proto string RedisCluster::strlen(string key) */
PHP_METHOD(RedisCluster, strlen) {
    CLUSTER_PROCESS_KW_CMD("STRLEN", redis_key_cmd, cluster_long_resp, 1);
}

/* {{{ proto long RedisCluster::lpush(string key, string val1, ... valN) */
PHP_METHOD(RedisCluster, lpush) {
    CLUSTER_PROCESS_KW_CMD("LPUSH", redis_key_varval_cmd, cluster_long_resp, 0);
}
/* }}} */

/* {{{ proto long RedisCluster::rpush(string key, string val1, ... valN) */
PHP_METHOD(RedisCluster, rpush) {
    CLUSTER_PROCESS_KW_CMD("RPUSH", redis_key_varval_cmd, cluster_long_resp, 0);
}
/* }}} */

/* {{{ proto array RedisCluster::blpop(string key1, ... keyN, long timeout) */
PHP_METHOD(RedisCluster, blpop) {
    CLUSTER_PROCESS_KW_CMD("BLPOP", redis_blocking_pop_cmd, cluster_mbulk_resp, 0);
}
/* }}} */

/* {{{ proto array RedisCluster::brpop(string key1, ... keyN, long timeout */
PHP_METHOD(RedisCluster, brpop) {
    CLUSTER_PROCESS_KW_CMD("BRPOP", redis_blocking_pop_cmd, cluster_mbulk_resp, 0);
}
/* }}} */

/* {{{ proto long RedisCluster::rpushx(string key, mixed value) */
PHP_METHOD(RedisCluster, rpushx) {
    CLUSTER_PROCESS_KW_CMD("RPUSHX", redis_kv_cmd, cluster_long_resp, 0);
}
/* }}} */

/* {{{ proto long RedisCluster::lpushx(string key, mixed value) */
PHP_METHOD(RedisCluster, lpushx) {
    CLUSTER_PROCESS_KW_CMD("LPUSHX", redis_kv_cmd, cluster_long_resp, 0);
}
/* }}} */

/* {{{ proto long RedisCluster::linsert(string k,string pos,mix pvt,mix val) */
PHP_METHOD(RedisCluster, linsert) {
    CLUSTER_PROCESS_CMD(linsert, cluster_long_resp, 0);
}
/* }}} */

/* {{{ proto string RedisCluster::lindex(string key, long index) */
PHP_METHOD(RedisCluster, lindex) {
    CLUSTER_PROCESS_KW_CMD("LINDEX", redis_key_long_cmd, cluster_bulk_resp, 0);
}
/* }}} */

/* {{{ proto long RedisCluster::lrem(string key, long count, string val) */
PHP_METHOD(RedisCluster, lrem) {
    CLUSTER_PROCESS_CMD(lrem, cluster_long_resp, 0);
}
/* }}} */

/* {{{ proto string RedisCluster::rpoplpush(string key, string key) */
PHP_METHOD(RedisCluster, rpoplpush) {
    CLUSTER_PROCESS_KW_CMD("RPOPLPUSH", redis_key_key_cmd, cluster_bulk_resp, 0);
}
/* }}} */

/* {{{ proto string RedisCluster::brpoplpush(string key, string key, long tm) */
PHP_METHOD(RedisCluster, brpoplpush) {
    CLUSTER_PROCESS_CMD(brpoplpush, cluster_bulk_resp, 0);
}
/* }}} */

PHP_METHOD(RedisCluster, lmove) {
    CLUSTER_PROCESS_KW_CMD("LMOVE", redis_lmove_cmd, cluster_bulk_resp, 0);
}

PHP_METHOD(RedisCluster, blmove) {
    CLUSTER_PROCESS_KW_CMD("BLMOVE", redis_lmove_cmd, cluster_bulk_resp, 0);
}

/* {{{ proto long RedisCluster::llen(string key)  */
PHP_METHOD(RedisCluster, llen) {
    CLUSTER_PROCESS_KW_CMD("LLEN", redis_key_cmd, cluster_long_resp, 1);
}
/* }}} */

/* {{{ proto long RedisCluster::scard(string key) */
PHP_METHOD(RedisCluster, scard) {
    CLUSTER_PROCESS_KW_CMD("SCARD", redis_key_cmd, cluster_long_resp, 1);
}
/* }}} */

/* {{{ proto array RedisCluster::smembers(string key) */
PHP_METHOD(RedisCluster, smembers) {
    CLUSTER_PROCESS_KW_CMD("SMEMBERS", redis_key_cmd, cluster_mbulk_resp, 1);
}
/* }}} */

/* {{{ proto long RedisCluster::sismember(string key) */
PHP_METHOD(RedisCluster, sismember) {
    CLUSTER_PROCESS_KW_CMD("SISMEMBER", redis_kv_cmd, cluster_1_resp, 1);
}
/* }}} */

/* {{{ proto array RedisCluster::smismember(string key, string member0, ...memberN) */
PHP_METHOD(RedisCluster, smismember) {
    CLUSTER_PROCESS_KW_CMD("SMISMEMBER", redis_key_varval_cmd, cluster_variant_resp, 1);
}
/* }}} */

/* {{{ proto long RedisCluster::sadd(string key, string val1 [, ...]) */
PHP_METHOD(RedisCluster, sadd) {
    CLUSTER_PROCESS_KW_CMD("SADD", redis_key_varval_cmd, cluster_long_resp, 0);
}
/* }}} */

/* {{{ proto long RedisCluster::saddarray(string key, array values) */
PHP_METHOD(RedisCluster, saddarray) {
    CLUSTER_PROCESS_KW_CMD("SADD", redis_key_val_arr_cmd, cluster_long_resp, 0);
}
/* }}} */

/* {{{ proto long RedisCluster::srem(string key, string val1 [, ...]) */
PHP_METHOD(RedisCluster, srem) {
    CLUSTER_PROCESS_KW_CMD("SREM", redis_key_varval_cmd, cluster_long_resp, 0);
}
/* }}} */

/* {{{ proto array RedisCluster::sunion(string key1, ... keyN) */
PHP_METHOD(RedisCluster, sunion) {
    CLUSTER_PROCESS_KW_CMD("SUNION", redis_varkey_cmd, cluster_mbulk_resp, 0);
}
/* }}} */

/* {{{ proto long RedisCluster::sunionstore(string dst, string k1, ... kN) */
PHP_METHOD(RedisCluster, sunionstore) {
    CLUSTER_PROCESS_KW_CMD("SUNIONSTORE", redis_varkey_cmd, cluster_long_resp, 0);
}
/* }}} */

/* {{{ ptoto array RedisCluster::sinter(string k1, ... kN) */
PHP_METHOD(RedisCluster, sinter) {
    CLUSTER_PROCESS_KW_CMD("SINTER", redis_varkey_cmd, cluster_mbulk_resp, 0);
}

/* {{{ proto RedisCluster::sintercard(array $keys, int $count = -1) */
PHP_METHOD(RedisCluster, sintercard) {
    CLUSTER_PROCESS_KW_CMD("SINTERCARD", redis_intercard_cmd, cluster_long_resp, 0);
}
/* }}} */

/* }}} */

/* {{{ ptoto long RedisCluster::sinterstore(string dst, string k1, ... kN) */
PHP_METHOD(RedisCluster, sinterstore) {
    CLUSTER_PROCESS_KW_CMD("SINTERSTORE", redis_varkey_cmd, cluster_long_resp, 0);
}
/* }}} */

/* {{{ proto array RedisCluster::sdiff(string k1, ... kN) */
PHP_METHOD(RedisCluster, sdiff) {
    CLUSTER_PROCESS_KW_CMD("SDIFF", redis_varkey_cmd, cluster_mbulk_resp, 1);
}
/* }}} */

/* {{{ proto long RedisCluster::sdiffstore(string dst, string k1, ... kN) */
PHP_METHOD(RedisCluster, sdiffstore) {
    CLUSTER_PROCESS_KW_CMD("SDIFFSTORE", redis_varkey_cmd, cluster_long_resp, 0);
}
/* }}} */

/* {{{ proto bool RedisCluster::smove(string src, string dst, string mem) */
PHP_METHOD(RedisCluster, smove) {
    CLUSTER_PROCESS_CMD(smove, cluster_1_resp, 0);
}
/* }}} */

/* {{{ proto bool RedisCluster::persist(string key) */
PHP_METHOD(RedisCluster, persist) {
    CLUSTER_PROCESS_KW_CMD("PERSIST", redis_key_cmd, cluster_1_resp, 0);
}
/* }}} */

/* {{{ proto long RedisCluster::ttl(string key) */
PHP_METHOD(RedisCluster, ttl) {
    CLUSTER_PROCESS_KW_CMD("TTL", redis_key_cmd, cluster_long_resp, 1);
}
/* }}} */

/* {{{ proto long RedisCluster::pttl(string key) */
PHP_METHOD(RedisCluster, pttl) {
    CLUSTER_PROCESS_KW_CMD("PTTL", redis_key_cmd, cluster_long_resp, 1);
}
/* }}} */

/* {{{ proto long RedisCluster::zcard(string key) */
PHP_METHOD(RedisCluster, zcard) {
    CLUSTER_PROCESS_KW_CMD("ZCARD", redis_key_cmd, cluster_long_resp, 1);
}
/* }}} */

/* {{{ proto double RedisCluster::zscore(string key) */
PHP_METHOD(RedisCluster, zscore) {
    CLUSTER_PROCESS_KW_CMD("ZSCORE", redis_kv_cmd, cluster_dbl_resp, 1);
}
/* }}} */

PHP_METHOD(RedisCluster, zmscore) {
    CLUSTER_PROCESS_KW_CMD("ZMSCORE", redis_key_varval_cmd, cluster_mbulk_dbl_resp, 1);
}

/* {{{ proto long RedisCluster::zadd(string key,double score,string mem, ...) */
PHP_METHOD(RedisCluster, zadd) {
    CLUSTER_PROCESS_CMD(zadd, cluster_zadd_resp, 0);
}
/* }}} */

/* {{{ proto double RedisCluster::zincrby(string key, double by, string mem) */
PHP_METHOD(RedisCluster, zincrby) {
    CLUSTER_PROCESS_CMD(zincrby, cluster_dbl_resp, 0);
}
/* }}} */

/* {{{ proto RedisCluster::zremrangebyscore(string k, string s, string e) */
PHP_METHOD(RedisCluster, zremrangebyscore) {
    CLUSTER_PROCESS_KW_CMD("ZREMRANGEBYSCORE", redis_key_str_str_cmd,
        cluster_long_resp, 0);
}
/* }}} */

/* {{{ proto RedisCluster::zcount(string key, string s, string e) */
PHP_METHOD(RedisCluster, zcount) {
    CLUSTER_PROCESS_KW_CMD("ZCOUNT", redis_key_str_str_cmd, cluster_long_resp, 1);
}
/* }}} */

/* {{{ proto long RedisCluster::zrank(string key, mixed member) */
PHP_METHOD(RedisCluster, zrank) {
    CLUSTER_PROCESS_KW_CMD("ZRANK", redis_kv_cmd, cluster_long_resp, 1);
}
/* }}} */

/* {{{ proto long RedisCluster::zrevrank(string key, mixed member) */
PHP_METHOD(RedisCluster, zrevrank) {
    CLUSTER_PROCESS_KW_CMD("ZREVRANK", redis_kv_cmd, cluster_long_resp, 1);
}
/* }}} */

/* {{{ proto long RedisCluster::hlen(string key) */
PHP_METHOD(RedisCluster, hlen) {
    CLUSTER_PROCESS_KW_CMD("HLEN", redis_key_cmd, cluster_long_resp, 1);
}
/* }}} */

/* {{{ proto array RedisCluster::hkeys(string key) */
PHP_METHOD(RedisCluster, hkeys) {
    CLUSTER_PROCESS_KW_CMD("HKEYS", redis_key_cmd, cluster_mbulk_raw_resp, 1);
}
/* }}} */

/* {{{ proto array RedisCluster::hvals(string key) */
PHP_METHOD(RedisCluster, hvals) {
    CLUSTER_PROCESS_KW_CMD("HVALS", redis_key_cmd, cluster_mbulk_resp, 1);
}
/* }}} */

/* {{{ proto string RedisCluster::hget(string key, string mem) */
PHP_METHOD(RedisCluster, hget) {
    CLUSTER_PROCESS_KW_CMD("HGET", redis_key_str_cmd, cluster_bulk_resp, 1);
}
/* }}} */

/* {{{ proto string RedisCluster::hgetWithMeta(string key, string mem) */
PHP_METHOD(RedisCluster, hgetWithMeta) {
    CLUSTER_PROCESS_KW_CMD("HGET", redis_key_str_cmd, cluster_bulk_withmeta_resp, 1);
}
/* }}} */

/* {{{ proto bool RedisCluster::hset(string key, string mem, string val) */
PHP_METHOD(RedisCluster, hset) {
    CLUSTER_PROCESS_CMD(hset, cluster_long_resp, 0);
}
/* }}} */

/* {{{ proto bool RedisCluster::hsetnx(string key, string mem, string val) */
PHP_METHOD(RedisCluster, hsetnx) {
    CLUSTER_PROCESS_CMD(hsetnx, cluster_1_resp, 0);
}
/* }}} */

/* {{{ proto array RedisCluster::hgetall(string key) */
PHP_METHOD(RedisCluster, hgetall) {
    CLUSTER_PROCESS_KW_CMD("HGETALL", redis_key_cmd,
        cluster_mbulk_zipstr_resp, 1);
}
/* }}} */

/* {{{ proto bool RedisCluster::hexists(string key, string member) */
PHP_METHOD(RedisCluster, hexists) {
    CLUSTER_PROCESS_KW_CMD("HEXISTS", redis_key_str_cmd, cluster_1_resp, 1);
}
/* }}} */

/* {{{ proto long RedisCluster::hincr(string key, string mem, long val) */
PHP_METHOD(RedisCluster, hincrby) {
    CLUSTER_PROCESS_CMD(hincrby, cluster_long_resp, 0);
}
/* }}} */

/* {{{ proto double RedisCluster::hincrbyfloat(string k, string m, double v) */
PHP_METHOD(RedisCluster, hincrbyfloat) {
    CLUSTER_PROCESS_CMD(hincrbyfloat, cluster_dbl_resp, 0);
}
/* }}} */

/* {{{ proto bool RedisCluster::hmset(string key, array key_vals) */
PHP_METHOD(RedisCluster, hmset) {
    CLUSTER_PROCESS_CMD(hmset, cluster_bool_resp, 0);
}
/* }}} */

PHP_METHOD(RedisCluster, hexpire) {
    CLUSTER_PROCESS_KW_CMD("HEXPIRE",
                           redis_hexpire_cmd, cluster_variant_resp, 0);
}

PHP_METHOD(RedisCluster, hpexpire) {
    CLUSTER_PROCESS_KW_CMD("HPEXPIRE",
                           redis_hexpire_cmd, cluster_variant_resp, 0);
}

PHP_METHOD(RedisCluster, hexpireat) {
    CLUSTER_PROCESS_KW_CMD("HEXPIREAT",
                           redis_hexpire_cmd, cluster_variant_resp, 0);
}

PHP_METHOD(RedisCluster, hpexpireat) {
    CLUSTER_PROCESS_KW_CMD("HPEXPIREAT",
                           redis_hexpire_cmd, cluster_variant_resp, 0);
}

PHP_METHOD(RedisCluster, httl) {
    CLUSTER_PROCESS_KW_CMD("HTTL", redis_httl_cmd, cluster_variant_resp, 1);
}

PHP_METHOD(RedisCluster, hpttl) {
    CLUSTER_PROCESS_KW_CMD("HPTTL", redis_httl_cmd, cluster_variant_resp, 1);
}


PHP_METHOD(RedisCluster, hexpiretime) {
    CLUSTER_PROCESS_KW_CMD("HEXPIRETIME", redis_httl_cmd,
                           cluster_variant_resp, 1);
}

PHP_METHOD(RedisCluster, hpexpiretime) {
    CLUSTER_PROCESS_KW_CMD("HPEXPIRETIME", redis_httl_cmd,
                           cluster_variant_resp, 1);
}

PHP_METHOD(RedisCluster, hpersist) {
    CLUSTER_PROCESS_KW_CMD("HPERSIST", redis_httl_cmd, cluster_variant_resp, 0);
}

/* {{{ proto bool RedisCluster::hrandfield(string key, [array $options]) */
PHP_METHOD(RedisCluster, hrandfield) {
    CLUSTER_PROCESS_CMD(hrandfield, cluster_hrandfield_resp, 1);
}
/* }}} */

/* {{{ proto long RedisCluster::hdel(string key, string mem1, ... memN) */
PHP_METHOD(RedisCluster, hdel) {
    CLUSTER_PROCESS_CMD(hdel, cluster_long_resp, 0);
}
/* }}} */

/* {{{ proto array RedisCluster::hmget(string key, array members) */
PHP_METHOD(RedisCluster, hmget) {
    CLUSTER_PROCESS_CMD(hmget, cluster_mbulk_assoc_resp, 1);
}
/* }}} */

PHP_METHOD(RedisCluster, hgetex) {
    CLUSTER_PROCESS_CMD(hgetex, cluster_mbulk_assoc_resp, 0);
}

PHP_METHOD(RedisCluster, hsetex) {
    CLUSTER_PROCESS_CMD(hsetex, cluster_long_resp, 0);
}

PHP_METHOD(RedisCluster, hgetdel) {
    CLUSTER_PROCESS_CMD(hgetdel, cluster_mbulk_assoc_resp, 0);
}

/* {{{ proto array RedisCluster::hstrlen(string key, string field) */
PHP_METHOD(RedisCluster, hstrlen) {
    CLUSTER_PROCESS_CMD(hstrlen, cluster_long_resp, 1);
}
/* }}} */


/* {{{ proto string RedisCluster::dump(string key) */
PHP_METHOD(RedisCluster, dump) {
    CLUSTER_PROCESS_KW_CMD("DUMP", redis_key_cmd, cluster_bulk_raw_resp, 1);
}

/* {{{ proto long RedisCluster::incr(string key) */
PHP_METHOD(RedisCluster, incr) {
    CLUSTER_PROCESS_CMD(incr, cluster_long_resp, 0);
}
/* }}} */

/* {{{ proto long RedisCluster::incrby(string key, long byval) */
PHP_METHOD(RedisCluster, incrby) {
    CLUSTER_PROCESS_KW_CMD("INCRBY", redis_key_long_cmd, cluster_long_resp, 0);
}
/* }}} */

/* {{{ proto long RedisCluster::decr(string key) */
PHP_METHOD(RedisCluster, decr) {
    CLUSTER_PROCESS_CMD(decr, cluster_long_resp, 0);
}
/* }}} */

/* {{{ proto long RedisCluster::decrby(string key, long byval) */
PHP_METHOD(RedisCluster, decrby) {
    CLUSTER_PROCESS_KW_CMD("DECRBY", redis_key_long_cmd, cluster_long_resp, 0);
}
/* }}} */

/* {{{ proto double RedisCluster::incrbyfloat(string key, double val) */
PHP_METHOD(RedisCluster, incrbyfloat) {
    CLUSTER_PROCESS_KW_CMD("INCRBYFLOAT", redis_key_dbl_cmd,
        cluster_dbl_resp, 0);
}
/* }}} */

/* {{{ proto double RedisCluster::decrbyfloat(string key, double val) */
PHP_METHOD(RedisCluster, decrbyfloat) {
    CLUSTER_PROCESS_KW_CMD("DECRBYFLOAT", redis_key_dbl_cmd,
        cluster_dbl_resp, 0);
}
/* }}} */

/* {{{ proto bool RedisCluster::expire(string key, long sec) */
PHP_METHOD(RedisCluster, expire) {
    CLUSTER_PROCESS_KW_CMD("EXPIRE", redis_expire_cmd, cluster_1_resp, 0);
}
/* }}} */

/* {{{ proto bool RedisCluster::expireat(string key, long ts) */
PHP_METHOD(RedisCluster, expireat) {
    CLUSTER_PROCESS_KW_CMD("EXPIREAT", redis_expire_cmd, cluster_1_resp, 0);
}

/* {{{ proto bool RedisCluster::pexpire(string key, long ms) */
PHP_METHOD(RedisCluster, pexpire) {
    CLUSTER_PROCESS_KW_CMD("PEXPIRE", redis_expire_cmd, cluster_1_resp, 0);
}
/* }}} */

/* {{{ proto bool RedisCluster::pexpireat(string key, long ts) */
PHP_METHOD(RedisCluster, pexpireat) {
    CLUSTER_PROCESS_KW_CMD("PEXPIREAT", redis_expire_cmd, cluster_1_resp, 0);
}
/* }}} */

/* {{{ Redis::expiretime(string $key): int */
PHP_METHOD(RedisCluster, expiretime) {
    CLUSTER_PROCESS_KW_CMD("EXPIRETIME", redis_key_cmd, cluster_long_resp, 1);
}

/* {{{ Redis::pexpiretime(string $key): int */
PHP_METHOD(RedisCluster, pexpiretime) {
    CLUSTER_PROCESS_KW_CMD("PEXPIRETIME", redis_key_cmd, cluster_long_resp, 1);
}

/* {{{ proto long RedisCluster::append(string key, string val) */
PHP_METHOD(RedisCluster, append) {
    CLUSTER_PROCESS_KW_CMD("APPEND", redis_kv_cmd, cluster_long_resp, 0);
}
/* }}} */

/* {{{ proto long RedisCluster::getbit(string key, long val) */
PHP_METHOD(RedisCluster, getbit) {
    CLUSTER_PROCESS_KW_CMD("GETBIT", redis_key_long_cmd, cluster_long_resp, 1);
}
/* }}} */

PHP_METHOD(RedisCluster, expiremember) {
    CLUSTER_PROCESS_CMD(expiremember, cluster_long_resp, 0);
}

PHP_METHOD(RedisCluster, expirememberat) {
    CLUSTER_PROCESS_CMD(expiremember, cluster_long_resp, 0);
}

/* {{{ proto long RedisCluster::setbit(string key, long offset, bool onoff) */
PHP_METHOD(RedisCluster, setbit) {
    CLUSTER_PROCESS_CMD(setbit, cluster_long_resp, 0);
}

/* {{{ proto long RedisCluster::bitop(string op,string key,[string key2,...]) */
PHP_METHOD(RedisCluster, bitop)
{
    CLUSTER_PROCESS_CMD(bitop, cluster_long_resp, 0);
}
/* }}} */

/* {{{ proto long RedisCluster::bitcount(string key, [int start, int end]) */
PHP_METHOD(RedisCluster, bitcount) {
    CLUSTER_PROCESS_CMD(bitcount, cluster_long_resp, 1);
}
/* }}} */

/* {{{ proto long RedisCluster::bitpos(string key, int bit, [int s, int end]) */
PHP_METHOD(RedisCluster, bitpos) {
    CLUSTER_PROCESS_CMD(bitpos, cluster_long_resp, 1);
}
/* }}} */

/* {{{ proto string Redis::lget(string key, long index) */
PHP_METHOD(RedisCluster, lget) {
    CLUSTER_PROCESS_KW_CMD("LINDEX", redis_key_long_cmd, cluster_bulk_resp, 1);
}
/* }}} */

/* {{{ proto string RedisCluster::getrange(string key, long start, long end) */ PHP_METHOD(RedisCluster, getrange) {
    CLUSTER_PROCESS_KW_CMD("GETRANGE", redis_key_long_long_cmd,
        cluster_bulk_resp, 1);
}
/* }}} */

/* {{{ prot RedisCluster::lcs(string $key1, string $key2, ?array $options = NULL): mixed; */
PHP_METHOD(RedisCluster, lcs) {
    CLUSTER_PROCESS_CMD(lcs, cluster_variant_resp, 1);
}

/* {{{ proto Redis|array|false Redis::lmpop(array $keys, string $from, int $count = 1) */
PHP_METHOD(RedisCluster, lmpop) {
    CLUSTER_PROCESS_KW_CMD("LMPOP", redis_mpop_cmd, cluster_mpop_resp, 0);
}
/* }}} */

/* {{{ proto Redis|array|false Redis::blmpop(double $timeout, array $keys, string $from, int $count = 1) */
PHP_METHOD(RedisCluster, blmpop) {
    CLUSTER_PROCESS_KW_CMD("BLMPOP", redis_mpop_cmd, cluster_mpop_resp, 0);
}
/* }}} */

/* {{{ proto Redis|array|false Redis::zmpop(array $keys, string $from, int $count = 1) */
PHP_METHOD(RedisCluster, zmpop) {
    CLUSTER_PROCESS_KW_CMD("ZMPOP", redis_mpop_cmd, cluster_mpop_resp, 0);
}
/* }}} */

/* {{{ proto Redis|array|false Redis::bzmpop(double $timeout, array $keys, string $from, int $count = 1) */
PHP_METHOD(RedisCluster, bzmpop) {
    CLUSTER_PROCESS_KW_CMD("BZMPOP", redis_mpop_cmd, cluster_mpop_resp, 0);
}
/* }}} */

/* {{{ proto string RedisCluster::ltrim(string key, long start, long end) */
PHP_METHOD(RedisCluster, ltrim) {
    CLUSTER_PROCESS_KW_CMD("LTRIM", redis_key_long_long_cmd, cluster_bool_resp, 0);
}
/* }}} */

/* {{{ proto array RedisCluster::lrange(string key, long start, long end) */
PHP_METHOD(RedisCluster, lrange) {
    CLUSTER_PROCESS_KW_CMD("LRANGE", redis_key_long_long_cmd,
        cluster_mbulk_resp, 1);
}
/* }}} */

/* {{{ proto long RedisCluster::zremrangebyrank(string k, long s, long e) */
PHP_METHOD(RedisCluster, zremrangebyrank) {
    CLUSTER_PROCESS_KW_CMD("ZREMRANGEBYRANK", redis_key_long_long_cmd,
        cluster_long_resp, 0);
}
/* }}} */

/* {{{ proto long RedisCluster::publish(string key, string msg) */
PHP_METHOD(RedisCluster, publish) {
    CLUSTER_PROCESS_KW_CMD("PUBLISH", redis_key_str_cmd, cluster_long_resp, 0);
}
/* }}} */

/* {{{ proto bool RedisCluster::rename(string key1, string key2) */
PHP_METHOD(RedisCluster, rename) {
    CLUSTER_PROCESS_KW_CMD("RENAME", redis_key_key_cmd, cluster_bool_resp, 0);
}
/* }}} */

/* {{{ proto bool RedisCluster::renamenx(string key1, string key2) */
PHP_METHOD(RedisCluster, renamenx) {
    CLUSTER_PROCESS_KW_CMD("RENAMENX", redis_key_key_cmd, cluster_1_resp, 0);
}
/* }}} */

/* {{{ proto long RedisCluster::pfcount(string key) */
PHP_METHOD(RedisCluster, pfcount) {
    CLUSTER_PROCESS_CMD(pfcount, cluster_long_resp, 1);
}
/* }}} */

/* {{{ proto bool RedisCluster::pfadd(string key, array vals) */
PHP_METHOD(RedisCluster, pfadd) {
    CLUSTER_PROCESS_CMD(pfadd, cluster_1_resp, 0);
}
/* }}} */

/* {{{ proto bool RedisCluster::pfmerge(string key, array keys) */
PHP_METHOD(RedisCluster, pfmerge) {
    CLUSTER_PROCESS_CMD(pfmerge, cluster_bool_resp, 0);
}
/* }}} */

/* {{{ proto boolean RedisCluster::restore(string key, long ttl, string val) */
PHP_METHOD(RedisCluster, restore) {
    CLUSTER_PROCESS_CMD(restore, cluster_bool_resp, 0);
}
/* }}} */

/* {{{ proto long RedisCluster::setrange(string key, long offset, string val) */
PHP_METHOD(RedisCluster, setrange) {
    CLUSTER_PROCESS_KW_CMD("SETRANGE", redis_key_long_str_cmd,
        cluster_long_resp, 0);
}
/* }}} */

/* {{{ proto
 *     array RedisCluster::zrange(string k, long s, long e, bool score = 0) */
PHP_METHOD(RedisCluster, zrange) {
    CLUSTER_PROCESS_KW_CMD("ZRANGE", redis_zrange_cmd, cluster_zrange_resp, 1);
}
/* }}} */

/* {{{ proto
 *     array RedisCluster::zrange(string $dstkey, string $srckey, long s, long e, array|bool $options = false) */
PHP_METHOD(RedisCluster, zrangestore) {
    CLUSTER_PROCESS_KW_CMD("ZRANGESTORE", redis_zrange_cmd, cluster_long_resp, 0);
}

/* }}} */
/* {{{ proto
 *     array RedisCluster::zrevrange(string k,long s,long e,bool scores = 0) */
PHP_METHOD(RedisCluster, zrevrange) {
    CLUSTER_PROCESS_KW_CMD("ZREVRANGE", redis_zrange_cmd, cluster_zrange_resp, 1);
}
/* }}} */

/* {{{ proto array
 *     RedisCluster::zrangebyscore(string k, long s, long e, array opts) */
PHP_METHOD(RedisCluster, zrangebyscore) {
    CLUSTER_PROCESS_KW_CMD("ZRANGEBYSCORE", redis_zrange_cmd, cluster_zrange_resp, 1);
}
/* }}} */

/* {{{ proto RedisCluster::zunionstore(string dst, array keys, [array weights,
 *                                     string agg]) */
PHP_METHOD(RedisCluster, zunionstore) {
    CLUSTER_PROCESS_KW_CMD("ZUNIONSTORE", redis_zinterunionstore_cmd, cluster_long_resp, 0);
}
/* }}} */

PHP_METHOD(RedisCluster, zdiff) {
    CLUSTER_PROCESS_CMD(zdiff, cluster_zdiff_resp, 1);
}

PHP_METHOD(RedisCluster, zdiffstore) {
    CLUSTER_PROCESS_CMD(zdiffstore, cluster_long_resp, 0);
}

PHP_METHOD(RedisCluster, zinter) {
    CLUSTER_PROCESS_KW_CMD("ZUNION", redis_zinterunion_cmd, cluster_zdiff_resp, 1);
}

PHP_METHOD(RedisCluster, zunion) {
    CLUSTER_PROCESS_KW_CMD("ZINTER", redis_zinterunion_cmd, cluster_zdiff_resp, 1);
}

/* {{{ proto array RedisCluster::zrandmember(string key, array options) */
PHP_METHOD(RedisCluster, zrandmember) {
    CLUSTER_PROCESS_CMD(zrandmember, cluster_zrandmember_resp, 1);
}

/* }}} */
/* {{{ proto RedisCluster::zinterstore(string dst, array keys, [array weights,
 *                                     string agg]) */
PHP_METHOD(RedisCluster, zinterstore) {
    CLUSTER_PROCESS_KW_CMD("ZINTERSTORE", redis_zinterunionstore_cmd, cluster_long_resp, 0);
}
/* }}} */

/* {{{ proto RedisCluster::zintercard(array $keys, int $count = -1) */
PHP_METHOD(RedisCluster, zintercard) {
    CLUSTER_PROCESS_KW_CMD("ZINTERCARD", redis_intercard_cmd, cluster_long_resp, 0);
}
/* }}} */

/* {{{ proto RedisCluster::zrem(string key, string val1, ... valN) */
PHP_METHOD(RedisCluster, zrem) {
    CLUSTER_PROCESS_KW_CMD("ZREM", redis_key_varval_cmd, cluster_long_resp, 0);
}
/* }}} */

/* {{{ proto array
 *     RedisCluster::zrevrangebyscore(string k, long s, long e, array opts) */
PHP_METHOD(RedisCluster, zrevrangebyscore) {
    CLUSTER_PROCESS_KW_CMD("ZREVRANGEBYSCORE", redis_zrange_cmd, cluster_zrange_resp, 1);
}
/* }}} */

/* {{{ proto array RedisCluster::zrangebylex(string key, string min, string max,
 *                                           [offset, count]) */
PHP_METHOD(RedisCluster, zrangebylex) {
    CLUSTER_PROCESS_KW_CMD("ZRANGEBYLEX", redis_zrangebylex_cmd,
        cluster_mbulk_resp, 1);
}
/* }}} */

/* {{{ proto array RedisCluster::zrevrangebylex(string key, string min,
 *                                              string min, [long off, long limit) */
PHP_METHOD(RedisCluster, zrevrangebylex) {
    CLUSTER_PROCESS_KW_CMD("ZREVRANGEBYLEX", redis_zrangebylex_cmd,
        cluster_mbulk_resp, 1);
}
/* }}} */

/* {{{ proto long RedisCluster::zlexcount(string key, string min, string max) */
PHP_METHOD(RedisCluster, zlexcount) {
    CLUSTER_PROCESS_KW_CMD("ZLEXCOUNT", redis_gen_zlex_cmd, cluster_long_resp, 1);
}
/* }}} */

/* {{{ proto long RedisCluster::zremrangebylex(string key, string min, string max) */
PHP_METHOD(RedisCluster, zremrangebylex) {
    CLUSTER_PROCESS_KW_CMD("ZREMRANGEBYLEX", redis_gen_zlex_cmd,
        cluster_long_resp, 0);
}
/* }}} */

/* {{{ proto array RedisCluster::zpopmax(string key) */
PHP_METHOD(RedisCluster, zpopmax) {
    if (ZEND_NUM_ARGS() == 1) {
        CLUSTER_PROCESS_KW_CMD("ZPOPMAX", redis_key_cmd, cluster_mbulk_zipdbl_resp, 0);
    } else if (ZEND_NUM_ARGS() == 2) {
        CLUSTER_PROCESS_KW_CMD("ZPOPMAX", redis_key_long_cmd, cluster_mbulk_zipdbl_resp, 0);
    } else {
        zend_wrong_param_count();
    }
}
/* }}} */

/* {{{ proto array RedisCluster::zpopmin(string key) */
PHP_METHOD(RedisCluster, zpopmin) {
    if (ZEND_NUM_ARGS() == 1) {
        CLUSTER_PROCESS_KW_CMD("ZPOPMIN", redis_key_cmd, cluster_mbulk_zipdbl_resp, 0);
    } else if (ZEND_NUM_ARGS() == 2) {
        CLUSTER_PROCESS_KW_CMD("ZPOPMIN", redis_key_long_cmd, cluster_mbulk_zipdbl_resp, 0);
    } else {
        zend_wrong_param_count();
    }
}
/* }}} */

/* {{{ proto array RedisCluster::bzPopMin(Array keys [, timeout]) }}} */
PHP_METHOD(RedisCluster, bzpopmax) {
    CLUSTER_PROCESS_KW_CMD("BZPOPMAX", redis_blocking_pop_cmd, cluster_mbulk_resp, 0);
}

/* {{{ proto array RedisCluster::bzPopMax(Array keys [, timeout]) }}} */
PHP_METHOD(RedisCluster, bzpopmin) {
    CLUSTER_PROCESS_KW_CMD("BZPOPMIN", redis_blocking_pop_cmd, cluster_mbulk_resp, 0);
}

/* {{{ proto RedisCluster::sort(string key, array options) */
PHP_METHOD(RedisCluster, sort) {
    CLUSTER_PROCESS_KW_CMD("SORT", redis_sort_cmd, cluster_variant_resp, 0);
}

/* {{{ proto RedisCluster::sort_ro(string key, array options) */
PHP_METHOD(RedisCluster, sort_ro) {
    CLUSTER_PROCESS_KW_CMD("SORT_RO", redis_sort_cmd, cluster_variant_resp, 1);
}

/* {{{ proto RedisCluster::object(string subcmd, string key) */
PHP_METHOD(RedisCluster, object) {
    CLUSTER_PROCESS_CMD(object, cluster_object_resp, 1);
}

/* {{{ proto null RedisCluster::subscribe(array chans, callable cb) */
PHP_METHOD(RedisCluster, subscribe) {
    CLUSTER_PROCESS_KW_CMD("SUBSCRIBE", redis_subscribe_cmd, cluster_sub_resp, 0);
}
/* }}} */

/* {{{ proto null RedisCluster::psubscribe(array pats, callable cb) */
PHP_METHOD(RedisCluster, psubscribe) {
    CLUSTER_PROCESS_KW_CMD("PSUBSCRIBE", redis_subscribe_cmd, cluster_sub_resp, 0);
}
/* }}} */

static void generic_unsub_cmd(INTERNAL_FUNCTION_PARAMETERS, redisCluster *c,
                              char *kw)
{
    RedisCmdCtx ctx;
    RedisCmd *cmd;

    // There is not reason to unsubscribe outside of a subscribe loop
    if (c->subscribed_slot == -1) {
        php_error_docref(0, E_WARNING,
            "You can't unsubscribe outside of a subscribe loop");
        RETURN_FALSE;
    }

    // Call directly because we're going to set the slot manually
    cmd = redis_unsubscribe_cmd(INTERNAL_FUNCTION_PARAM_PASSTHRU, c->flags, kw);
    if (cmd == NULL) {
        RETURN_FALSE;
    }

    ctx = redis_cmd_pop_ctx(cmd);

    // This has to operate on our subscribe slot
    if (cluster_send_slot_cmd(c, c->subscribed_slot, cmd,  TYPE_MULTIBULK)
                              == FAILURE)
    {
        CLUSTER_THROW_EXCEPTION("Failed to UNSUBSCRIBE within our subscribe loop!", 0);
        redis_cmd_ctx_free(ctx);
        redis_cmd_free(cmd);
        RETURN_FALSE;
    }

    // Now process response from the slot we're subscribed on
    cluster_unsub_resp(INTERNAL_FUNCTION_PARAM_PASSTHRU, c, ctx);

    // Cleanup our command
    redis_cmd_ctx_free(ctx);
    redis_cmd_free(cmd);
}

/* {{{ proto array RedisCluster::unsubscribe(array chans) */
PHP_METHOD(RedisCluster, unsubscribe) {
    generic_unsub_cmd(INTERNAL_FUNCTION_PARAM_PASSTHRU, GET_CONTEXT(),
        "UNSUBSCRIBE");
}
/* }}} */

/* {{{ proto array RedisCluster::punsubscribe(array pats) */
PHP_METHOD(RedisCluster, punsubscribe) {
    generic_unsub_cmd(INTERNAL_FUNCTION_PARAM_PASSTHRU, GET_CONTEXT(),
        "PUNSUBSCRIBE");
}
/* }}} */

/* {{{ proto mixed RedisCluster::eval(string script, [array args, int numkeys) */
PHP_METHOD(RedisCluster, eval) {
    CLUSTER_PROCESS_KW_CMD("EVAL", redis_eval_cmd, cluster_variant_raw_resp, 0);
}
/* }}} */

/* {{{ proto mixed RedisCluster::eval_ro(string script, [array args, int numkeys) */
PHP_METHOD(RedisCluster, eval_ro) {
    CLUSTER_PROCESS_KW_CMD("EVAL_RO", redis_eval_cmd, cluster_variant_raw_resp, 1);
}
/* }}} */

/* {{{ proto mixed RedisCluster::evalsha(string sha, [array args, int numkeys]) */
PHP_METHOD(RedisCluster, evalsha) {
    CLUSTER_PROCESS_KW_CMD("EVALSHA", redis_eval_cmd, cluster_variant_raw_resp, 0);
}
/* }}} */

/* {{{ proto mixed RedisCluster::evalsha_ro(string sha, [array args, int numkeys]) */
PHP_METHOD(RedisCluster, evalsha_ro) {
    CLUSTER_PROCESS_KW_CMD("EVALSHA_RO", redis_eval_cmd, cluster_variant_raw_resp, 1);
}

/* }}} */
/* Commands that do not interact with Redis, but just report stuff about
 * various options, etc */

/* {{{ proto string RedisCluster::getmode() */
PHP_METHOD(RedisCluster, getmode) {
    redisCluster *c = GET_CONTEXT();
    RETURN_LONG(c->flags->mode);
}
/* }}} */

/* {{{ proto string RedisCluster::getlasterror() */
PHP_METHOD(RedisCluster, getlasterror) {
    redisCluster *c = GET_CONTEXT();

    if (c->err) {
        RETURN_STRINGL(ZSTR_VAL(c->err), ZSTR_LEN(c->err));
    }
    RETURN_NULL();
}
/* }}} */

/* {{{ proto bool RedisCluster::clearlasterror() */
PHP_METHOD(RedisCluster, clearlasterror) {
    redisCluster *c = GET_CONTEXT();

    if (c->err) {
        zend_string_release(c->err);
        c->err = NULL;
    }

    RETURN_TRUE;
}

static void redisSumNodeBytes(redisClusterNode *node, zend_long *tx, zend_long *rx) {
    struct redisClusterNode *slave;

    *tx += node->sock->txBytes;
    *rx += node->sock->rxBytes;

    if (node->slaves) {
        ZEND_HASH_FOREACH_PTR(node->slaves, slave) {
            *tx += slave->sock->txBytes;
            *rx += slave->sock->rxBytes;
        } ZEND_HASH_FOREACH_END();
    }
}

static void redisClearNodeBytes(redisClusterNode *node) {
    struct redisClusterNode *slave;

    node->sock->txBytes = 0;
    node->sock->rxBytes = 0;

    if (node->slaves) {
        ZEND_HASH_FOREACH_PTR(node->slaves, slave) {
            slave->sock->txBytes = 0;
            slave->sock->rxBytes = 0;
        } ZEND_HASH_FOREACH_END();
    }
}

PHP_METHOD(RedisCluster, gettransferredbytes) {
    redisCluster *c = GET_CONTEXT();
    zend_long rx = 0, tx = 0;
    redisClusterNode *node;

    ZEND_HASH_FOREACH_PTR(c->nodes, node) {
        redisSumNodeBytes(node, &tx, &rx);
    } ZEND_HASH_FOREACH_END();

    array_init_size(return_value, 2);
    add_next_index_long(return_value, tx);
    add_next_index_long(return_value, rx);
}
/* }}} */

PHP_METHOD(RedisCluster, cleartransferredbytes) {
    redisCluster *c = GET_CONTEXT();
    redisClusterNode *node;

    ZEND_HASH_FOREACH_PTR(c->nodes, node) {
        redisClearNodeBytes(node);
    } ZEND_HASH_FOREACH_END();
}

/* {{{ proto long RedisCluster::getOption(long option */
PHP_METHOD(RedisCluster, getoption) {
    redisCluster *c = GET_CONTEXT();
    redis_getoption_handler(INTERNAL_FUNCTION_PARAM_PASSTHRU, c->flags, c);
}
/* }}} */

/* {{{ proto bool RedisCluster::setOption(long option, mixed value) */
PHP_METHOD(RedisCluster, setoption) {
    redisCluster *c = GET_CONTEXT();
    redis_setoption_handler(INTERNAL_FUNCTION_PARAM_PASSTHRU, c->flags, c);
}
/* }}} */

/* {{{ proto string RedisCluster::_prefix(string key) */
PHP_METHOD(RedisCluster, _prefix) {
    redisCluster *c = GET_CONTEXT();
    redis_prefix_handler(INTERNAL_FUNCTION_PARAM_PASSTHRU, c->flags);
}
/* }}} */

/* {{{ proto string RedisCluster::_serialize(mixed val) */
PHP_METHOD(RedisCluster, _serialize) {
    redisCluster *c = GET_CONTEXT();
    redis_serialize_handler(INTERNAL_FUNCTION_PARAM_PASSTHRU, c->flags);
}
/* }}} */

/* {{{ proto mixed RedisCluster::_unserialize(string val) */
PHP_METHOD(RedisCluster, _unserialize) {
    redisCluster *c = GET_CONTEXT();
    redis_unserialize_handler(INTERNAL_FUNCTION_PARAM_PASSTHRU,
        c->flags, redis_cluster_exception_ce);
}
/* }}} */

PHP_METHOD(RedisCluster, _compress) {
    redisCluster *c = GET_CONTEXT();
    redis_compress_handler(INTERNAL_FUNCTION_PARAM_PASSTHRU, c->flags);
}

PHP_METHOD(RedisCluster, _uncompress) {
    redisCluster *c = GET_CONTEXT();
    redis_uncompress_handler(INTERNAL_FUNCTION_PARAM_PASSTHRU, c->flags,
                             redis_cluster_exception_ce);
}

PHP_METHOD(RedisCluster, _pack) {
    redisCluster *c = GET_CONTEXT();
    redis_pack_handler(INTERNAL_FUNCTION_PARAM_PASSTHRU, c->flags);
}

PHP_METHOD(RedisCluster, _digest) {
    redisCluster *c = GET_CONTEXT();

    redis_digest_handler(INTERNAL_FUNCTION_PARAM_PASSTHRU, c->flags,
                         redis_cluster_exception_ce);
}

PHP_METHOD(RedisCluster, _unpack) {
    redisCluster *c = GET_CONTEXT();
    redis_unpack_handler(INTERNAL_FUNCTION_PARAM_PASSTHRU, c->flags);
}

/* {{{ proto array RedisCluster::_masters() */
PHP_METHOD(RedisCluster, _masters) {
    redisCluster *c = GET_CONTEXT();
    redisClusterNode *node;

    array_init(return_value);

    ZEND_HASH_FOREACH_PTR(c->nodes, node) {
        if (node == NULL) break;

        zval z_sub;

        array_init(&z_sub);

        add_next_index_stringl(&z_sub, ZSTR_VAL(node->sock->host), ZSTR_LEN(node->sock->host));
        add_next_index_long(&z_sub, node->sock->port);
        add_next_index_zval(return_value, &z_sub);
    } ZEND_HASH_FOREACH_END();
}

PHP_METHOD(RedisCluster, _redir) {
    redisCluster *c = GET_CONTEXT();
    smart_str s = {0};

    if (*c->redir_host && c->redir_host_len) {
        smart_str_append_printf(&s, "%s:%d", c->redir_host, c->redir_port);
        smart_str_0(&s);
        RETURN_STR(s.s);
    } else {
        RETURN_NULL();
    }
}

/*
 * Transaction handling
 */

/* {{{ proto bool RedisCluster::multi() */
PHP_METHOD(RedisCluster, multi) {
    redisCluster *c = GET_CONTEXT();
    zend_long value = MULTI;

    ZEND_PARSE_PARAMETERS_START(0, 1)
        Z_PARAM_OPTIONAL
        Z_PARAM_LONG(value)
    ZEND_PARSE_PARAMETERS_END();

    if (value != MULTI) {
        php_error_docref(NULL, E_WARNING, "RedisCluster does not support PIPELINING");
    }

    if (c->flags->mode == MULTI) {
        php_error_docref(NULL, E_WARNING,
            "RedisCluster is already in MULTI mode, ignoring");
        RETURN_FALSE;
    }

    /* Flag that we're in MULTI mode */
    c->flags->mode = MULTI;

    c->flags->txBytes = 0;
    c->flags->rxBytes = 0;

    /* Return our object so we can chain MULTI calls */
    RETVAL_ZVAL(getThis(), 1, 0);
}

/* {{{ proto bool RedisCluster::watch() */
PHP_METHOD(RedisCluster, watch) {
    redisCluster *c = GET_CONTEXT();
    clusterDistList *dl;
    HashTable *ht_dist;
    zend_string *zstr;
    zend_ulong slot;
    RedisCmd *cmd;
    zval *argv;
    int argc;

    // Disallow in MULTI mode
    if (c->flags->mode == MULTI) {
        php_error_docref(NULL, E_WARNING,
            "WATCH command not allowed in MULTI mode");
        RETURN_FALSE;
    }

    // Don't need to process zero arguments
    if (!ZEND_NUM_ARGS())
        RETURN_FALSE;

    // Create our distribution HashTable
    ht_dist = cluster_dist_create();

    ZEND_PARSE_PARAMETERS_START(1, -1)
        Z_PARAM_VARIADIC('+', argv, argc)
    ZEND_PARSE_PARAMETERS_END();

    // Loop through arguments, prefixing if needed
    for(int i = 0 ; i < argc; i++) {
        // We'll need the key as a string
        zstr = zval_get_string(&argv[i]);

        // Add this key to our distribution handler
        if (cluster_dist_add_key(c, ht_dist, ZSTR_VAL(zstr), ZSTR_LEN(zstr),
                                 NULL) == FAILURE)
        {
            CLUSTER_THROW_EXCEPTION(
                "Can't issue WATCH command as the keyspace isn't fully mapped", 0);
            zend_string_release(zstr);
            RETURN_FALSE;
        }

        zend_string_release(zstr);
    }

    // Iterate over each node we'll be sending commands to
    ZEND_HASH_FOREACH_NUM_KEY_PTR(ht_dist, slot, dl) {
        cmd = redis_cmd_create_literal(NULL, "WATCH");
        for (int i = 0; i < dl->len; i++) {
            redis_cmd_cat_str(cmd, dl->entry[i].key, dl->entry[i].key_len);
        }

        // If we get a failure from this, we have to abort
        if (cluster_send_rcmd_ex(c, slot, cmd) < 0)
        {
            redis_cmd_free(cmd);
            RETURN_FALSE;
        }

        SLOT_SOCK(c, slot)->watching = 1;

        redis_cmd_free(cmd);
    } ZEND_HASH_FOREACH_END();

    cluster_dist_free(ht_dist);

    RETURN_TRUE;
}

/* {{{ proto bool RedisCluster::unwatch() */
PHP_METHOD(RedisCluster, unwatch) {
    redisCluster *c = GET_CONTEXT();
    short slot;

    // Send UNWATCH to nodes that need it
    for(slot = 0; slot < REDIS_CLUSTER_SLOTS; slot++) {
        if (c->master[slot] && SLOT_SOCK(c,slot)->watching) {
            if (cluster_send_slot(c, slot, ZEND_STRL(RESP_UNWATCH_CMD),
                                  TYPE_LINE) == -1)
            {
                CLUSTER_RETURN_BOOL(c, 0);
            }

            // No longer watching
            SLOT_SOCK(c,slot)->watching = 0;
        }
    }

    CLUSTER_RETURN_BOOL(c, 1);
}

/* {{{ proto array RedisCluster::exec() */
PHP_METHOD(RedisCluster, exec) {
    redisCluster *c = GET_CONTEXT();
    clusterFoldItem *fi;

    // Verify we are in fact in multi mode
    if (CLUSTER_IS_ATOMIC(c)) {
        php_error_docref(NULL, E_WARNING, "RedisCluster is not in MULTI mode");
        RETURN_FALSE;
    }

    // First pass, send EXEC and abort on failure
    fi = c->multi_head;
    while (fi) {
        if (SLOT_SOCK(c, fi->slot)->mode == MULTI) {
            if ( cluster_send_exec(c, fi->slot) < 0) {
                cluster_abort_exec(c);
                CLUSTER_THROW_EXCEPTION("Error processing EXEC across the cluster", 0);

                // Free our queue, reset MULTI state
                cluster_free_queue(c);
                cluster_reset_multi(c);

                RETURN_FALSE;
            }
            SLOT_SOCK(c, fi->slot)->mode     = ATOMIC;
            SLOT_SOCK(c, fi->slot)->watching = 0;
        }
        fi = fi->next;
    }

    // MULTI multi-bulk response handler
    cluster_multi_mbulk_resp(INTERNAL_FUNCTION_PARAM_PASSTHRU, c, redis_empty_ctx);

    // Free our callback queue, any enqueued distributed command context items
    // and reset our MULTI state.
    cluster_free_queue(c);
    cluster_reset_multi(c);
}

/* {{{ proto bool RedisCluster::discard() */
PHP_METHOD(RedisCluster, discard) {
    redisCluster *c = GET_CONTEXT();

    if (CLUSTER_IS_ATOMIC(c)) {
        php_error_docref(NULL, E_WARNING, "Cluster is not in MULTI mode");
        RETURN_FALSE;
    }

    if (cluster_abort_exec(c) < 0) {
        cluster_reset_multi(c);
    }

    cluster_free_queue(c);

    RETURN_TRUE;
}

/* Get a slot either by key (string) or host/port array */
static short
cluster_cmd_get_slot(redisCluster *c, zval *z_arg)
{
    size_t key_len;
    int key_free;
    zval *z_host, *z_port;
    short slot;
    char *key;
    zend_string *zstr;

    /* If it's a string, treat it as a key.  Otherwise, look for a two
     * element array */
    if (Z_TYPE_P(z_arg) ==IS_STRING || Z_TYPE_P(z_arg) ==IS_LONG ||
       Z_TYPE_P(z_arg) ==IS_DOUBLE)
    {
        /* Allow for any scalar here */
        zstr = zval_get_string(z_arg);
        key = ZSTR_VAL(zstr);
        key_len = ZSTR_LEN(zstr);

        /* Hash it */
        key_free = redis_key_prefix(c->flags, &key, &key_len);
        slot = cluster_hash_key(key, key_len);
        zend_string_release(zstr);
        if (key_free) efree(key);
    } else if (Z_TYPE_P(z_arg) == IS_ARRAY &&
        (z_host = zend_hash_index_find(Z_ARRVAL_P(z_arg), 0)) != NULL &&
        (z_port = zend_hash_index_find(Z_ARRVAL_P(z_arg), 1)) != NULL &&
        Z_TYPE_P(z_host) == IS_STRING && Z_TYPE_P(z_port) == IS_LONG
    ) {
        /* Attempt to find this specific node by host:port */
        slot = cluster_find_slot(c,(const char *)Z_STRVAL_P(z_host),
            (unsigned short)Z_LVAL_P(z_port));

        /* Inform the caller if they've passed bad data */
        if (slot < 0) {
            php_error_docref(0, E_WARNING, "Unknown node %s:" ZEND_LONG_FMT,
                Z_STRVAL_P(z_host), Z_LVAL_P(z_port));
        }
    } else {
        php_error_docref(0, E_WARNING,
            "Directed commands must be passed a key or [host,port] array");
        return -1;
    }

    return slot;
}

/* Generic handler for things we want directed at a given node, like SAVE,
 * BGSAVE, FLUSHDB, FLUSHALL, etc */
static void cluster_empty_node_cmd(INTERNAL_FUNCTION_PARAMETERS, char *kw,
                                   size_t kwlen, REDIS_REPLY_TYPE reply_type,
                                   cluster_cb cb)
{
    redisCluster *c = GET_CONTEXT();
    RedisCmd *cmd;
    zval *z_arg;
    short slot;

    ZEND_PARSE_PARAMETERS_START(1, 1)
        Z_PARAM_ZVAL(z_arg)
    ZEND_PARSE_PARAMETERS_END_EX(RETURN_FALSE);

    // One argument means find the node (treated like a key), and two means
    // send the command to a specific host and port
    slot = cluster_cmd_get_slot(c, z_arg);
    if (slot < 0) {
        RETURN_FALSE;
    }

    // Construct our command
    cmd = redis_cmd_create(NULL, kw, kwlen);

    // Kick off our command
    if (cluster_send_slot_cmd(c, slot, cmd, reply_type) < 0) {
        CLUSTER_THROW_EXCEPTION("Unable to send command at a specific node", 0);
        redis_cmd_free(cmd);
        RETURN_FALSE;
    }

    // Our response callback
    cb(INTERNAL_FUNCTION_PARAM_PASSTHRU, c, redis_empty_ctx);

    // Free our command
    redis_cmd_free(cmd);
}

static void
cluster_flush_cmd(INTERNAL_FUNCTION_PARAMETERS, char *kw,
                  REDIS_REPLY_TYPE reply_type, cluster_cb cb)
{
    redisCluster *c = GET_CONTEXT();
    RedisCmd *cmd;
    zval *z_arg;
    zend_bool async = 0;
    short slot;

    ZEND_PARSE_PARAMETERS_START(1, 2)
        Z_PARAM_ZVAL(z_arg)
        Z_PARAM_OPTIONAL
        Z_PARAM_BOOL(async)
    ZEND_PARSE_PARAMETERS_END_EX(RETURN_FALSE);

    // One argument means find the node (treated like a key), and two means
    // send the command to a specific host and port
    slot = cluster_cmd_get_slot(c, z_arg);
    if (slot < 0) {
        RETURN_FALSE;
    }

    cmd = redis_cmd_create(NULL, kw, strlen(kw));

    redis_cmd_cat_literal_if(cmd, async, "ASYNC");

    // Kick off our command
    if (cluster_send_slot_cmd(c, slot, cmd, reply_type) < 0) {
        CLUSTER_THROW_EXCEPTION("Unable to send command at a specific node", 0);
        redis_cmd_free(cmd);
        RETURN_FALSE;
    }

    // Our response callback
    cb(INTERNAL_FUNCTION_PARAM_PASSTHRU, c, redis_empty_ctx);

    // Free our command
    redis_cmd_free(cmd);
}

/* Generic routine for handling various commands which need to be directed at
 * a node, but have complex syntax.  We simply parse out the arguments and send
 * the command as constructed by the caller */
static void cluster_raw_cmd(INTERNAL_FUNCTION_PARAMETERS, char *kw, int kw_len)
{
    redisCluster *c = GET_CONTEXT();
    RedisCmd *cmd;
    int i, argc;
    zval *argv;
    short slot;

    /* Commands using this pass-through don't need to be enabled in MULTI mode */
    if (!CLUSTER_IS_ATOMIC(c)) {
        php_error_docref(0, E_WARNING,
            "Command can't be issued in MULTI mode");
        RETURN_FALSE;
    }

    /* We at least need the key or [host,port] argument */
    if (ZEND_NUM_ARGS() < 1) {
        php_error_docref(0, E_WARNING,
            "Command requires at least an argument to direct to a node");
        RETURN_FALSE;
    }

    ZEND_PARSE_PARAMETERS_START(1, -1)
        Z_PARAM_VARIADIC('*', argv, argc)
    ZEND_PARSE_PARAMETERS_END();

    /* First argument needs to be the "where" */
    if ((slot = cluster_cmd_get_slot(c, &argv[0])) < 0) {
        RETURN_FALSE;
    }

    cmd = redis_cmd_create(NULL, kw, kw_len);

    /* Iterate, appending args */
    for(i = 1; i < argc; i++) {
        redis_cmd_cat_zval_zstr(cmd, &argv[i]);
    }

    /* Send it off */
    if (cluster_send_slot_cmd(c, slot, cmd, TYPE_EOF) < 0) {
        CLUSTER_THROW_EXCEPTION("Couldn't send command to node", 0);
        redis_cmd_free(cmd);
        RETURN_FALSE;
    }

    /* Read the response variant */
    cluster_variant_resp(INTERNAL_FUNCTION_PARAM_PASSTHRU, c, redis_empty_ctx);

    redis_cmd_free(cmd);
}

/* Generic method for HSCAN, SSCAN, and ZSCAN */
static void cluster_kscan_cmd(INTERNAL_FUNCTION_PARAMETERS,
                              REDIS_SCAN_TYPE type)
{
    redisCluster *c = GET_CONTEXT();
    char *pat = NULL, *key = NULL;
    size_t key_len = 0, pat_len = 0, pat_free = 0;
    RedisCmd *cmd;
    zval *z_it;
    HashTable *hash;
    long num_ele;
    zend_long count = 0;
    zend_bool completed;
    uint64_t cursor;

    // Can't be in MULTI mode
    if (!CLUSTER_IS_ATOMIC(c)) {
        CLUSTER_THROW_EXCEPTION("SCAN type commands can't be called in MULTI mode!", 0);
        RETURN_FALSE;
    }

    ZEND_PARSE_PARAMETERS_START(2, 4)
        Z_PARAM_STRING(key, key_len)
        Z_PARAM_ZVAL_EX(z_it, 0, 1)
        Z_PARAM_OPTIONAL
        Z_PARAM_STRING_OR_NULL(pat, pat_len)
        Z_PARAM_LONG(count)
    ZEND_PARSE_PARAMETERS_END_EX(RETURN_FALSE);

    /* Treat as readonly */
    c->readonly = 1;

    /* Get our scan cursor and return early if we're done */
    cursor = redisGetScanCursor(z_it, &completed);
    if (completed)
        RETURN_FALSE;

    if (c->flags->scan & REDIS_SCAN_PREFIX) {
        pat_free = redis_key_prefix(c->flags, &pat, &pat_len);
    }

    // If SCAN_RETRY is set, loop until we get a zero iterator or until
    // we get non-zero elements.  Otherwise we just send the command once.
    do {
        /* Free our return value if we're back in the loop */
        if (Z_TYPE_P(return_value) == IS_ARRAY) {
            zval_ptr_dtor_nogc(return_value);
            ZVAL_NULL(return_value);
        }

        // Create command
        cmd = redis_fmt_scan_cmd(c->flags, type, key, key_len, cursor, pat,
                                 pat_len, count);
        if (cmd == NULL) {
            CLUSTER_THROW_EXCEPTION("Couldn't construct SCAN command", 0);
            if (pat_free) efree(pat);
            RETURN_FALSE;
        }

        // Send it off
        if (cluster_send_rcmd(c, cmd) == FAILURE)
        {
            CLUSTER_THROW_EXCEPTION("Couldn't send SCAN command", 0);
            if (pat_free) efree(pat);
            redis_cmd_free(cmd);
            RETURN_FALSE;
        }

        // Read response
        if (cluster_scan_resp(INTERNAL_FUNCTION_PARAM_PASSTHRU, c, type,
                              &cursor) == FAILURE)
        {
            CLUSTER_THROW_EXCEPTION("Couldn't read SCAN response", 0);
            if (pat_free) efree(pat);
            redis_cmd_free(cmd);
            RETURN_FALSE;
        }

        // Count the elements we got back
        hash = Z_ARRVAL_P(return_value);
        num_ele = zend_hash_num_elements(hash);

        // Free our command
        redis_cmd_free(cmd);
    } while (c->flags->scan & REDIS_SCAN_RETRY && cursor != 0 && num_ele == 0);

    // Free our pattern
    if (pat_free) efree(pat);

    // Update iterator reference
    redisSetScanCursor(z_it, cursor);
}

static int redis_acl_op_readonly(zend_string *op) {
    /* Only return read-only for operations we know to be */
    if (ZSTR_STRICMP_STATIC(op, "LIST") ||
        ZSTR_STRICMP_STATIC(op, "USERS") ||
        ZSTR_STRICMP_STATIC(op, "GETUSER") ||
        ZSTR_STRICMP_STATIC(op, "CAT") ||
        ZSTR_STRICMP_STATIC(op, "GENPASS") ||
        ZSTR_STRICMP_STATIC(op, "WHOAMI") ||
        ZSTR_STRICMP_STATIC(op, "LOG")) return 1;

    return 0;
}

PHP_METHOD(RedisCluster, acl) {
    redisCluster *c = GET_CONTEXT();
    zval *argv, *znode;
    zend_string *op;
    cluster_cb cb;
    RedisCmd *cmd;
    int argc, i;

    ZEND_PARSE_PARAMETERS_START(2, -1)
        Z_PARAM_ZVAL(znode)
        Z_PARAM_STR(op)
        Z_PARAM_OPTIONAL
        Z_PARAM_VARIADIC('+', argv, argc)
    ZEND_PARSE_PARAMETERS_END();

    cmd = redis_cmd_create_literal(c->flags, "ACL");

    if ((cmd->slot = cluster_cmd_get_slot(c, znode)) < 0) {
        redis_cmd_free(cmd);
        RETURN_FALSE;
    }

    redis_cmd_cat_zstr(cmd, op);

    /* We have specialized handlers for GETUSER and LOG, whereas every other ACL
     * command can be handled generically */
    if (zend_string_equals_literal_ci(op, "GETUSER")) {
        cb = cluster_acl_getuser_resp;
    } else if (zend_string_equals_literal_ci(op, "LOG")) {
        cb = cluster_acl_log_resp;
    } else {
        cb = cluster_variant_resp;
    }

    /* Process remaining args */
    for (i = 0; i < argc; i++) {
        redis_cmd_cat_zval_zstr(cmd, &argv[i]);
    }

    /* Can we use replicas? */
    c->readonly = redis_acl_op_readonly(op) && CLUSTER_IS_ATOMIC(c);

    /* Kick off our command */
    if (cluster_send_slot_cmd(c, cmd->slot, cmd, TYPE_EOF) < 0) {
        redis_cmd_free(cmd);
        CLUSTER_THROW_EXCEPTION("Unabler to send ACL command", 0);
        RETURN_FALSE;
    }

    if (CLUSTER_IS_ATOMIC(c)) {
        cb(INTERNAL_FUNCTION_PARAM_PASSTHRU, c, redis_empty_ctx);
    } else {
        cluster_enqueue_response(c, cmd->slot, cb, redis_empty_ctx);
    }

    redis_cmd_free(cmd);
}

/* {{{ proto RedisCluster::scan(string master, long it [, string pat, long cnt]) */
PHP_METHOD(RedisCluster, scan) {
    redisCluster *c = GET_CONTEXT();
    char *pat = NULL;
    size_t pat_len = 0;
    short slot;
    RedisCmd *cmd;
    zval *zcursor, *z_node;
    long num_ele, pat_free = 0;
    zend_long count = 0;
    zend_bool completed;
    uint64_t cursor;

    /* Treat as read-only */
    c->readonly = CLUSTER_IS_ATOMIC(c);

    /* Can't be in MULTI mode */
    if (!CLUSTER_IS_ATOMIC(c)) {
        CLUSTER_THROW_EXCEPTION("SCAN type commands can't be called in MULTI mode", 0);
        RETURN_FALSE;
    }

    ZEND_PARSE_PARAMETERS_START(2, 4)
        Z_PARAM_ZVAL_EX(zcursor, 0, 1)
        Z_PARAM_ZVAL(z_node)
        Z_PARAM_OPTIONAL
        Z_PARAM_STRING_OR_NULL(pat, pat_len)
        Z_PARAM_LONG(count)
    ZEND_PARSE_PARAMETERS_END_EX(RETURN_FALSE);

    /* Get the scan cursor and return early if we're done */
    cursor = redisGetScanCursor(zcursor, &completed);
    if (completed)
        RETURN_FALSE;

    if (c->flags->scan & REDIS_SCAN_PREFIX) {
        pat_free = redis_key_prefix(c->flags, &pat, &pat_len);
    }

    /* With SCAN_RETRY on, loop until we get some keys, otherwise just return
     * what Redis does, as it does */
    do {
        /* Free our return value if we're back in the loop */
        if (Z_TYPE_P(return_value) == IS_ARRAY) {
            zval_ptr_dtor_nogc(return_value);
            ZVAL_NULL(return_value);
        }

        /* Construct our command */
        cmd = redis_fmt_scan_cmd(NULL, TYPE_SCAN, NULL, 0, cursor, pat,
                                 pat_len, count);

        if ((slot = cluster_cmd_get_slot(c, z_node)) < 0) {
           redis_cmd_free(cmd);
           RETURN_FALSE;
        }

        // Send it to the node in question
        if (cluster_send_rcmd_ex(c, slot, cmd) < 0)
        {
            CLUSTER_THROW_EXCEPTION("Couldn't send SCAN to node", 0);
            redis_cmd_free(cmd);
            RETURN_FALSE;
        }

        if (cluster_scan_resp(INTERNAL_FUNCTION_PARAM_PASSTHRU, c, TYPE_SCAN,
                              &cursor) == FAILURE || Z_TYPE_P(return_value) != IS_ARRAY)
        {
            CLUSTER_THROW_EXCEPTION("Couldn't process SCAN response from node", 0);
            redis_cmd_free(cmd);
            RETURN_FALSE;
        }

        redis_cmd_free(cmd);

        num_ele = zend_hash_num_elements(Z_ARRVAL_P(return_value));
    } while (c->flags->scan & REDIS_SCAN_RETRY && cursor != 0 && num_ele == 0);

    if (pat_free) efree(pat);

    redisSetScanCursor(zcursor, cursor);
}
/* }}} */

/* {{{ proto RedisCluster::sscan(string key, long it [string pat, long cnt]) */
PHP_METHOD(RedisCluster, sscan) {
    cluster_kscan_cmd(INTERNAL_FUNCTION_PARAM_PASSTHRU, TYPE_SSCAN);
}
/* }}} */

/* {{{ proto RedisCluster::zscan(string key, long it [string pat, long cnt]) */
PHP_METHOD(RedisCluster, zscan) {
    cluster_kscan_cmd(INTERNAL_FUNCTION_PARAM_PASSTHRU, TYPE_ZSCAN);
}
/* }}} */

/* {{{ proto RedisCluster::hscan(string key, long it [string pat, long cnt]) */
PHP_METHOD(RedisCluster, hscan) {
    cluster_kscan_cmd(INTERNAL_FUNCTION_PARAM_PASSTHRU, TYPE_HSCAN);
}
/* }}} */

/* {{{ proto RedisCluster::save(string key)
 *     proto RedisCluster::save(array host_port) */
PHP_METHOD(RedisCluster, save) {
    cluster_empty_node_cmd(INTERNAL_FUNCTION_PARAM_PASSTHRU, ZEND_STRL("SAVE"),
                           TYPE_LINE, cluster_bool_resp);
}
/* }}} */

/* {{{ proto RedisCluster::bgsave(string key)
 *     proto RedisCluster::bgsave(array host_port) */
PHP_METHOD(RedisCluster, bgsave) {
    cluster_empty_node_cmd(INTERNAL_FUNCTION_PARAM_PASSTHRU,
                           ZEND_STRL("BGSAVE"), TYPE_LINE, cluster_bool_resp);
}
/* }}} */

/* {{{ proto RedisCluster::flushdb(string key, [bool async])
 *     proto RedisCluster::flushdb(array host_port, [bool async]) */
PHP_METHOD(RedisCluster, flushdb) {
    cluster_flush_cmd(INTERNAL_FUNCTION_PARAM_PASSTHRU, "FLUSHDB",
        TYPE_LINE, cluster_bool_resp);
}
/* }}} */

/* {{{ proto RedisCluster::flushall(string key, [bool async])
 *     proto RedisCluster::flushall(array host_port, [bool async]) */
PHP_METHOD(RedisCluster, flushall) {
    cluster_flush_cmd(INTERNAL_FUNCTION_PARAM_PASSTHRU, "FLUSHALL",
        TYPE_LINE, cluster_bool_resp);
}
/* }}} */

/* {{{ proto RedisCluster::dbsize(string key)
 *     proto RedisCluster::dbsize(array host_port) */
PHP_METHOD(RedisCluster, dbsize) {
    cluster_empty_node_cmd(INTERNAL_FUNCTION_PARAM_PASSTHRU,
                           ZEND_STRL("DBSIZE"), TYPE_INT, cluster_long_resp);
}
/* }}} */

/* {{{ proto RedisCluster::bgrewriteaof(string key)
 *     proto RedisCluster::bgrewriteaof(array host_port) */
PHP_METHOD(RedisCluster, bgrewriteaof) {
    cluster_empty_node_cmd(INTERNAL_FUNCTION_PARAM_PASSTHRU,
                           ZEND_STRL("BGREWRITEAOF"), TYPE_LINE,
                           cluster_bool_resp);
}
/* }}} */

/* {{{ proto RedisCluster::lastsave(string key)
 *     proto RedisCluster::lastsave(array $host_port) */
PHP_METHOD(RedisCluster, lastsave) {
    cluster_empty_node_cmd(INTERNAL_FUNCTION_PARAM_PASSTHRU,
                           ZEND_STRL("LASTSAVE"),  TYPE_INT,
                           cluster_long_resp);
}
/* }}} */

/* {{{ proto array RedisCluster::info(string key, [string $arg])
 *     proto array RedisCluster::info(array host_port, [string $arg]) */
PHP_METHOD(RedisCluster, info) {
    redisCluster *c = GET_CONTEXT();
    zval *node = NULL, *args = NULL;
    REDIS_REPLY_TYPE rtype;
    RedisCmd *cmd;
    int i, argc;
    short slot;

    ZEND_PARSE_PARAMETERS_START(1, -1)
        Z_PARAM_ZVAL(node)
        Z_PARAM_OPTIONAL
        Z_PARAM_VARIADIC('*', args, argc)
    ZEND_PARSE_PARAMETERS_END();

    if ((slot = cluster_cmd_get_slot(c, node)) < 0)
        RETURN_FALSE;

    cmd = redis_cmd_create_literal(c->flags, "INFO");

    /* Direct this command at the master */
    c->readonly = 0;

    for (i = 0; i < argc; i++) {
        redis_cmd_cat_zval_zstr(cmd, &args[i]);
    }

    rtype = CLUSTER_IS_ATOMIC(c) ? TYPE_BULK : TYPE_LINE;
    if (cluster_send_slot_cmd(c, slot, cmd, rtype) < 0) {
        CLUSTER_THROW_EXCEPTION("Unable to send INFO command to specific node", 0);
        redis_cmd_free(cmd);
        RETURN_FALSE;
    }

    if (CLUSTER_IS_ATOMIC(c)) {
        cluster_info_resp(INTERNAL_FUNCTION_PARAM_PASSTHRU, c, redis_empty_ctx);
    } else {
        cluster_enqueue_response(c, slot, cluster_info_resp, redis_empty_ctx);
    }

    redis_cmd_free(cmd);
}
/* }}} */

/* {{{ proto array RedisCluster::client('list')
 *     proto bool RedisCluster::client('kill', $ipport)
 *     proto bool RedisCluster::client('setname', $name)
 *     proto string RedisCluster::client('getname')
 */
PHP_METHOD(RedisCluster, client) {
    redisCluster *c = GET_CONTEXT();
    zend_string *op, *arg = NULL;
    REDIS_REPLY_TYPE rtype;
    cluster_cb cb;
    RedisCmd *cmd;
    zval *z_node;
    short slot;

    ZEND_PARSE_PARAMETERS_START(2, 3)
        Z_PARAM_ZVAL(z_node)
        Z_PARAM_STR(op)
        Z_PARAM_OPTIONAL
        Z_PARAM_STR(arg)
    ZEND_PARSE_PARAMETERS_END_EX(RETURN_FALSE);

    /* Make sure we can properly resolve the slot */
    slot = cluster_cmd_get_slot(c, z_node);
    if (slot < 0) RETURN_FALSE;

    /* Our return type and reply callback is different for all subcommands */
    if (zend_string_equals_literal_ci(op, "LIST")) {
        rtype = CLUSTER_IS_ATOMIC(c) ? TYPE_BULK : TYPE_LINE;
        cb = cluster_client_list_resp;
    } else if (zend_string_equals_literal_ci(op, "KILL") ||
               zend_string_equals_literal_ci(op, "SETNAME"))
    {
        rtype = TYPE_LINE;
        cb = cluster_bool_resp;
    } else if (zend_string_equals_literal_ci(op, "GETNAME")) {
        rtype = CLUSTER_IS_ATOMIC(c) ? TYPE_BULK : TYPE_LINE;
        cb = cluster_bulk_resp;
    } else {
        php_error_docref(NULL, E_WARNING,
            "Invalid CLIENT subcommand (LIST, KILL, GETNAME, and SETNAME are valid");
        RETURN_FALSE;
    }

    cmd = redis_cmd_create_literal(c->flags, "CLIENT");

    redis_cmd_cat_zstr(cmd, op);
    if (arg)
        redis_cmd_cat_zstr(cmd, arg);

    /* Attempt to write our command */
    if (cluster_send_slot_cmd(c, slot, cmd, rtype) < 0) {
        CLUSTER_THROW_EXCEPTION("Unable to send CLIENT command to specific node", 0);
        redis_cmd_free(cmd);
        RETURN_FALSE;
    }

    /* Now enqueue or process response */
    if (CLUSTER_IS_ATOMIC(c)) {
        cb(INTERNAL_FUNCTION_PARAM_PASSTHRU, c, redis_empty_ctx);
    } else {
        cluster_enqueue_response(c, slot, cb, redis_empty_ctx);
    }

    redis_cmd_free(cmd);
}

/* {{{ proto mixed RedisCluster::cluster(variant) */
PHP_METHOD(RedisCluster, cluster) {
    cluster_raw_cmd(INTERNAL_FUNCTION_PARAM_PASSTHRU, ZEND_STRL("CLUSTER"));
}
/* }}} */

/* }}} */

/* {{{ proto mixed RedisCluster::config(string key, ...)
 *     proto mixed RedisCluster::config(array host_port, ...) */
PHP_METHOD(RedisCluster, config) {
    cluster_raw_cmd(INTERNAL_FUNCTION_PARAM_PASSTHRU, ZEND_STRL("CONFIG"));
}
/* }}} */

/* {{{ proto mixed RedisCluster::pubsub(string key, ...)
 *     proto mixed RedisCluster::pubsub(array host_port, ...) */
PHP_METHOD(RedisCluster, pubsub) {
    cluster_raw_cmd(INTERNAL_FUNCTION_PARAM_PASSTHRU, ZEND_STRL("PUBSUB"));
}
/* }}} */

/* {{{ proto mixed RedisCluster::script(string key, ...)
 *     proto mixed RedisCluster::script(array host_port, ...) */
PHP_METHOD(RedisCluster, script) {
    redisCluster *c = GET_CONTEXT();
    RedisCmd *cmd;
    zval *z_args;
    short slot;
    int argc = ZEND_NUM_ARGS();

    /* Commands using this pass-through don't need to be enabled in MULTI mode */
    if (!CLUSTER_IS_ATOMIC(c)) {
        php_error_docref(0, E_WARNING,
            "Command can't be issued in MULTI mode");
        RETURN_FALSE;
    }

    /* We at least need the key or [host,port] argument */
    if (argc < 2) {
        php_error_docref(0, E_WARNING,
            "Command requires at least an argument to direct to a node");
        RETURN_FALSE;
    }

    /* Allocate an array to process arguments */
    z_args = ecalloc(argc, sizeof(zval));

    /* Grab args */
    if (zend_get_parameters_array(ht, argc, z_args) == FAILURE ||
        (slot = cluster_cmd_get_slot(c, &z_args[0])) < 0 ||
        (cmd = redis_build_script_cmd(argc - 1, &z_args[1])) == NULL
    ) {
        efree(z_args);
        RETURN_FALSE;
    }

    /* Send it off */
    if (cluster_send_slot_cmd(c, slot, cmd, TYPE_EOF) < 0) {
        CLUSTER_THROW_EXCEPTION("Couldn't send command to node", 0);
        redis_cmd_free(cmd);
        efree(z_args);
        RETURN_FALSE;
    }

    /* Read the response variant */
    cluster_variant_resp(INTERNAL_FUNCTION_PARAM_PASSTHRU, c, redis_empty_ctx);

    redis_cmd_free(cmd);
    efree(z_args);
}
/* }}} */

/* {{{ proto mixed RedisCluster::slowlog(string key, ...)
 *     proto mixed RedisCluster::slowlog(array host_port, ...) */
PHP_METHOD(RedisCluster, slowlog) {
    cluster_raw_cmd(INTERNAL_FUNCTION_PARAM_PASSTHRU, ZEND_STRL("SLOWLOG"));
}
/* }}} */

/* {{{ proto int RedisCluster::geoadd(string key, float long float lat string mem, ...) */
PHP_METHOD(RedisCluster, geoadd) {
    CLUSTER_PROCESS_CMD(geoadd, cluster_long_resp, 0);
}

/* {{{ proto array RedisCluster::geohash(string key, string mem1, [string mem2...]) */
PHP_METHOD(RedisCluster, geohash) {
    CLUSTER_PROCESS_KW_CMD("GEOHASH", redis_key_varval_cmd, cluster_mbulk_raw_resp, 1);
}

/* {{{ proto array RedisCluster::geopos(string key, string mem1, [string mem2...]) */
PHP_METHOD(RedisCluster, geopos) {
    CLUSTER_PROCESS_KW_CMD("GEOPOS", redis_key_varval_cmd, cluster_variant_resp, 1);
}

/* {{{ proto array RedisCluster::geodist(string key, string mem1, string mem2 [string unit]) */
PHP_METHOD(RedisCluster, geodist) {
    CLUSTER_PROCESS_CMD(geodist, cluster_dbl_resp, 1);
}

/* {{{ proto array RedisCluster::georadius() }}} */
PHP_METHOD(RedisCluster, georadius) {
    CLUSTER_PROCESS_KW_CMD("GEORADIUS", redis_georadius_cmd, cluster_variant_resp, 1);
}

/* {{{ proto array RedisCluster::georadius() }}} */
PHP_METHOD(RedisCluster, georadius_ro) {
    CLUSTER_PROCESS_KW_CMD("GEORADIUS_RO", redis_georadius_cmd, cluster_variant_resp, 1);
}

/* {{{ proto array RedisCluster::georadiusbymember() }}} */
PHP_METHOD(RedisCluster, georadiusbymember) {
    CLUSTER_PROCESS_KW_CMD("GEORADIUSBYMEMBER", redis_georadiusbymember_cmd, cluster_variant_resp, 1);
}

/* {{{ proto array RedisCluster::georadiusbymember() }}} */
PHP_METHOD(RedisCluster, georadiusbymember_ro) {
    CLUSTER_PROCESS_KW_CMD("GEORADIUSBYMEMBER_RO", redis_georadiusbymember_cmd, cluster_variant_resp, 1);
}

PHP_METHOD(RedisCluster, geosearch) {
    CLUSTER_PROCESS_CMD(geosearch, cluster_geosearch_resp, 1);
}

PHP_METHOD(RedisCluster, geosearchstore) {
    CLUSTER_PROCESS_CMD(geosearchstore, cluster_long_resp, 0);
}


/* {{{ proto array RedisCluster::role(string key)
 *     proto array RedisCluster::role(array host_port) */
PHP_METHOD(RedisCluster, role) {
    cluster_empty_node_cmd(INTERNAL_FUNCTION_PARAM_PASSTHRU, ZEND_STRL("ROLE"),
        TYPE_MULTIBULK, cluster_variant_resp);
}

/* {{{ proto array RedisCluster::time(string key)
 *     proto array RedisCluster::time(array host_port) */
PHP_METHOD(RedisCluster, time) {
    cluster_empty_node_cmd(INTERNAL_FUNCTION_PARAM_PASSTHRU, ZEND_STRL("TIME"),
        TYPE_MULTIBULK, cluster_variant_resp);
}
/* }}} */

/* {{{ proto string RedisCluster::randomkey(string key)
 *     proto string RedisCluster::randomkey(array host_port) */
PHP_METHOD(RedisCluster, randomkey) {
    cluster_empty_node_cmd(INTERNAL_FUNCTION_PARAM_PASSTHRU,
                           ZEND_STRL("RANDOMKEY"), TYPE_BULK,
                           cluster_bulk_resp);
}
/* }}} */

static
void cluster_gen_wait_cmd(INTERNAL_FUNCTION_PARAMETERS, const char *kw,
                          size_t kwlen, zend_bool has_local, int reply_type)
{
    zend_long numreplicas, timeout, numlocal = 0;
    redisCluster *c = GET_CONTEXT();
    RedisCmd *cmd;
    zval *node;
    int argc;

    argc = 3 + !!has_local;

    ZEND_PARSE_PARAMETERS_START(argc, argc)
        Z_PARAM_ZVAL(node)
        if (has_local) {
            Z_PARAM_LONG(numlocal)
        }
        Z_PARAM_LONG(numreplicas)
        Z_PARAM_LONG(timeout)
    ZEND_PARSE_PARAMETERS_END();

    if (numreplicas < 0 || timeout < 0 || numlocal < 0) {
        php_error_docref(NULL, E_WARNING, "No arguments can be negative");
        RETURN_FALSE;
    }

    cmd = redis_cmd_create(c->flags, kw, kwlen);

    cmd->slot = cluster_cmd_get_slot(c, node);
    if (cmd->slot < 0) {
        RETURN_FALSE;
    }

    if (has_local) {
        redis_cmd_cat_long(cmd, numlocal);
    }

    redis_cmd_cat_long(cmd, numreplicas);
    redis_cmd_cat_long(cmd, timeout);

    c->readonly = 0;

    if (cluster_send_slot_cmd(c, cmd->slot, cmd, reply_type) < 0) {
        CLUSTER_THROW_EXCEPTION("Unable to send command at the specified node", 0);
        redis_cmd_free(cmd);
        RETURN_FALSE;
    }

    if (CLUSTER_IS_ATOMIC(c)) {
        cluster_variant_resp(INTERNAL_FUNCTION_PARAM_PASSTHRU, c, redis_empty_ctx);
    } else {
        cluster_enqueue_response(c, cmd->slot, cluster_variant_resp,
                                 redis_empty_ctx);
    }

    redis_cmd_free(cmd);
}

PHP_METHOD(RedisCluster, wait) {
    cluster_gen_wait_cmd(INTERNAL_FUNCTION_PARAM_PASSTHRU, ZEND_STRL("WAIT"), 0,
                         TYPE_INT);
}

PHP_METHOD(RedisCluster, waitaof) {
    cluster_gen_wait_cmd(INTERNAL_FUNCTION_PARAM_PASSTHRU, ZEND_STRL("WAITAOF"), 1,
                         TYPE_MULTIBULK);
}

/* {{{ proto bool RedisCluster::ping(string key| string msg)
 *     proto bool RedisCluster::ping(array host_port| string msg) */
PHP_METHOD(RedisCluster, ping) {
    redisCluster *c = GET_CONTEXT();
    REDIS_REPLY_TYPE rtype;
    zend_string *arg = NULL;
    zval *z_node;
    RedisCmd *cmd;
    short slot;

    ZEND_PARSE_PARAMETERS_START(1, 2)
        Z_PARAM_ZVAL(z_node)
        Z_PARAM_OPTIONAL
        Z_PARAM_STR_OR_NULL(arg)
    ZEND_PARSE_PARAMETERS_END_EX(RETURN_FALSE);

    /* Treat this as a readonly command */
    c->readonly = CLUSTER_IS_ATOMIC(c);

    /* Grab slot either by key or host/port */
    slot = cluster_cmd_get_slot(c, z_node);
    if (slot < 0) {
        RETURN_FALSE;
    }

    cmd = redis_cmd_create_literal(c->flags, "PING");
    if (arg)
        redis_cmd_cat_zstr(cmd, arg);

    /* Send it off */
    rtype = CLUSTER_IS_ATOMIC(c) && arg != NULL ? TYPE_BULK : TYPE_LINE;
    if (cluster_send_slot_cmd(c, slot, cmd, rtype) < 0) {
        CLUSTER_THROW_EXCEPTION(
            "Unable to send command at the specified node", 0);
        redis_cmd_free(cmd);
        RETURN_FALSE;
    }

    /* We're done with our command */
    redis_cmd_free(cmd);

    /* Process response */
    if (CLUSTER_IS_ATOMIC(c)) {
        if (arg != NULL) {
            cluster_bulk_resp(INTERNAL_FUNCTION_PARAM_PASSTHRU, c,
                              redis_empty_ctx);
        } else {
            /* If we're atomic and didn't send an argument then we have already
             * processed the reply (which must have been successful. */
            RETURN_TRUE;
        }
    } else {
        if (arg != NULL) {
            cluster_enqueue_response(c, slot, cluster_bulk_resp,
                                     redis_empty_ctx);
        } else {
            cluster_enqueue_response(c, slot, cluster_variant_resp,
                                     redis_empty_ctx);
        }

        RETURN_ZVAL(getThis(), 1, 0);
    }
}
/* }}} */

PHP_METHOD(RedisCluster, vadd) {
    CLUSTER_PROCESS_CMD(vadd, cluster_long_resp, 0);
}

PHP_METHOD(RedisCluster, vsim) {
    CLUSTER_PROCESS_CMD(vsim, cluster_zrange_resp, 1);
}

PHP_METHOD(RedisCluster, vcard) {
    CLUSTER_PROCESS_KW_CMD("VCARD", redis_key_cmd, cluster_long_resp, 1);
}

PHP_METHOD(RedisCluster, vdim) {
    CLUSTER_PROCESS_KW_CMD("VDIM", redis_key_cmd, cluster_long_resp, 1);
}

PHP_METHOD(RedisCluster, vinfo) {
    CLUSTER_PROCESS_KW_CMD("VINFO", redis_key_cmd, cluster_vinfo_resp, 1);
}

PHP_METHOD(RedisCluster, vismember) {
    CLUSTER_PROCESS_KW_CMD("VISMEMBER", redis_kv_cmd, cluster_1_resp, 1);
}

PHP_METHOD(RedisCluster, vemb) {
    CLUSTER_PROCESS_CMD(vemb, cluster_vemb_resp, 1);
}

PHP_METHOD(RedisCluster, vrandmember) {
    CLUSTER_PROCESS_KW_CMD("VRANDMEMBER", redis_randmember_cmd,
                           cluster_randmember_resp, 1);
}

PHP_METHOD(RedisCluster, vrange) {
    CLUSTER_PROCESS_KW_CMD("VRANGE", redis_vrange_cmd, cluster_mbulk_resp, 1);
}

PHP_METHOD(RedisCluster, vrem) {
    CLUSTER_PROCESS_KW_CMD("VREM", redis_kv_cmd, cluster_long_resp, 0);
}

PHP_METHOD(RedisCluster, vlinks) {
    CLUSTER_PROCESS_CMD(vlinks, cluster_vlinks_resp, 1);
}

PHP_METHOD(RedisCluster, vgetattr) {
    CLUSTER_PROCESS_CMD(vgetattr, cluster_vgetattr_resp, 1);
}

PHP_METHOD(RedisCluster, vsetattr) {
    CLUSTER_PROCESS_CMD(vsetattr, cluster_long_resp, 0);
}

PHP_METHOD(RedisCluster, gcra) {
    CLUSTER_PROCESS_CMD(gcra, cluster_variant_resp, 0);
}

/* {{{ proto long RedisCluster::xack(string key, string group, array ids) }}} */
PHP_METHOD(RedisCluster, xack) {
    CLUSTER_PROCESS_CMD(xack, cluster_long_resp, 0);
}

/* {{{ proto string RedisCluster::xadd(string key, string id, array field_values) }}} */
PHP_METHOD(RedisCluster, xadd) {
    CLUSTER_PROCESS_CMD(xadd, cluster_bulk_raw_resp, 0);
}

/* {{{ proto array RedisCluster::xclaim(string key, string group, string consumer,
 *                                      long min_idle_time, array ids, array options) */
PHP_METHOD(RedisCluster, xclaim) {
    CLUSTER_PROCESS_CMD(xclaim, cluster_xclaim_resp, 0);
}

PHP_METHOD(RedisCluster, xautoclaim) {
    CLUSTER_PROCESS_CMD(xautoclaim, cluster_xclaim_resp, 0);
}

PHP_METHOD(RedisCluster, xdel) {
    CLUSTER_PROCESS_KW_CMD("XDEL", redis_key_str_arr_cmd, cluster_long_resp, 0);
}

PHP_METHOD(RedisCluster, xdelex) {
    CLUSTER_PROCESS_CMD(xdelex, cluster_variant_resp, 0);
}

/* {{{ proto variant RedisCluster::xgroup(string op, [string key, string arg1, string arg2]) }}} */
PHP_METHOD(RedisCluster, xgroup) {
    CLUSTER_PROCESS_CMD(xgroup, cluster_variant_resp, 0);
}

/* {{{ proto variant RedisCluster::xinfo(string op, [string arg1, string arg2]); */
PHP_METHOD(RedisCluster, xinfo) {
    CLUSTER_PROCESS_CMD(xinfo, cluster_xinfo_resp, 0);
}

/* {{{ proto string RedisCluster::xlen(string key) }}} */
PHP_METHOD(RedisCluster, xlen) {
    CLUSTER_PROCESS_KW_CMD("XLEN", redis_key_cmd, cluster_long_resp, 1);
}

PHP_METHOD(RedisCluster, xpending) {
    CLUSTER_PROCESS_CMD(xpending, cluster_variant_resp_strings, 1);
}

PHP_METHOD(RedisCluster, xrange) {
    CLUSTER_PROCESS_KW_CMD("XRANGE", redis_xrange_cmd, cluster_xrange_resp, 1);
}

PHP_METHOD(RedisCluster, xrevrange) {
    CLUSTER_PROCESS_KW_CMD("XREVRANGE", redis_xrange_cmd, cluster_xrange_resp, 1);
}

PHP_METHOD(RedisCluster, xread) {
    CLUSTER_PROCESS_CMD(xread, cluster_xread_resp, 1);
}

PHP_METHOD(RedisCluster, xreadgroup) {
    CLUSTER_PROCESS_CMD(xreadgroup, cluster_xread_resp, 0);
}

PHP_METHOD(RedisCluster, xtrim) {
    CLUSTER_PROCESS_CMD(xtrim, cluster_long_resp, 0);
}



/* {{{ proto string RedisCluster::echo(string key, string msg)
 *     proto string RedisCluster::echo(array host_port, string msg) */
PHP_METHOD(RedisCluster, echo) {
    redisCluster *c = GET_CONTEXT();
    REDIS_REPLY_TYPE rtype;
    zend_string *msg;
    RedisCmd *cmd;
    zval *z_arg;
    short slot;

    ZEND_PARSE_PARAMETERS_START(2, 2)
        Z_PARAM_ZVAL(z_arg)
        Z_PARAM_STR(msg)
    ZEND_PARSE_PARAMETERS_END_EX(RETURN_FALSE);

    /* Treat this as a readonly command */
    c->readonly = CLUSTER_IS_ATOMIC(c);

    /* Grab slot either by key or host/port */
    slot = cluster_cmd_get_slot(c, z_arg);
    if (slot < 0) {
        RETURN_FALSE;
    }

    /* Construct our command */
    cmd = redis_cmd_fmt(NULL, "ECHO", "S", msg);

    /* Send it off */
    rtype = CLUSTER_IS_ATOMIC(c) ? TYPE_BULK : TYPE_LINE;
    if (cluster_send_slot_cmd(c, slot, cmd, rtype) < 0) {
        CLUSTER_THROW_EXCEPTION(
            "Unable to send command at the specified node", 0);
        redis_cmd_free(cmd);
        RETURN_FALSE;
    }

    /* Process bulk response */
    if (CLUSTER_IS_ATOMIC(c)) {
        cluster_bulk_resp(INTERNAL_FUNCTION_PARAM_PASSTHRU, c, redis_empty_ctx);
    } else {
        cluster_enqueue_response(c, slot, cluster_bulk_resp, redis_empty_ctx);
    }

    redis_cmd_free(cmd);
}
/* }}} */

/* {{{ proto mixed RedisCluster::rawcommand(string $key, string $cmd, [ $argv1 .. $argvN])
 *     proto mixed RedisCluster::rawcommand(array $host_port, string $cmd, [ $argv1 .. $argvN]) */
PHP_METHOD(RedisCluster, rawcommand) {
    REDIS_REPLY_TYPE rtype;
    int argc = ZEND_NUM_ARGS();
    redisCluster *c = GET_CONTEXT();
    RedisCmd *cmd;
    zval *z_args;

    /* Sanity check on our arguments */
    if (argc < 2) {
        php_error_docref(NULL, E_WARNING,
            "You must pass at least node information as well as at least a command.");
        RETURN_FALSE;
    }
    z_args = emalloc(argc * sizeof(zval));
    if (zend_get_parameters_array(ht, argc, z_args) == FAILURE) {
        php_error_docref(NULL, E_WARNING,
            "Internal PHP error parsing method parameters.");
        efree(z_args);
        RETURN_FALSE;
    } else if ((cmd = redis_build_raw_cmd(&z_args[1], argc-1)) == NULL ||
               (cmd->slot = cluster_cmd_get_slot(c, &z_args[0])) < 0)
    {
        redis_cmd_free(cmd);
        efree(z_args);
        RETURN_FALSE;
    }

    /* Free argument array */
    efree(z_args);

    /* Direct the command */
    rtype = CLUSTER_IS_ATOMIC(c) ? TYPE_EOF : TYPE_LINE;
    if (cluster_send_slot_cmd(c, cmd->slot, cmd, rtype) < 0) {
        CLUSTER_THROW_EXCEPTION("Unable to send command to the specified node", 0);
        redis_cmd_free(cmd);
        RETURN_FALSE;
    }

    /* Process variant response */
    if (CLUSTER_IS_ATOMIC(c)) {
        cluster_variant_raw_resp(INTERNAL_FUNCTION_PARAM_PASSTHRU, c,
                                 redis_empty_ctx);
    } else {
        cluster_enqueue_response(c, cmd->slot, cluster_variant_raw_resp,
                                 redis_empty_ctx);
    }

    redis_cmd_free(cmd);
}
/* }}} */

/* {{{ proto array RedisCluster::command()
 *     proto array RedisCluster::command('INFO', string cmd)
 *     proto array RedisCluster::command('GETKEYS', array cmd_args) */
PHP_METHOD(RedisCluster, command) {
    CLUSTER_PROCESS_CMD(command, cluster_variant_resp_strings, 0);
}

PHP_METHOD(RedisCluster, copy) {
    CLUSTER_PROCESS_CMD(copy, cluster_1_resp, 0);
}

PHP_METHOD(RedisCluster, digest) {
    CLUSTER_PROCESS_KW_CMD("DIGEST", redis_key_cmd, cluster_bulk_raw_resp, 1);
}

/* vim: set tabstop=4 softtabstop=4 expandtab shiftwidth=4: */
