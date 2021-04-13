#ifndef _PHPREDIS_CLUSTER_LIBRARY_H
#define _PHPREDIS_CLUSTER_LIBRARY_H

#include "common.h"

#ifdef ZTS
#include "TSRM.h"
#endif

/* Redis cluster hash slots and N-1 which we'll use to find it */
#define REDIS_CLUSTER_SLOTS 16384
#define REDIS_CLUSTER_MOD   (REDIS_CLUSTER_SLOTS-1)

/* Complete representation for various commands in RESP */
#define RESP_MULTI_CMD         "*1\r\n$5\r\nMULTI\r\n"
#define RESP_EXEC_CMD          "*1\r\n$4\r\nEXEC\r\n"
#define RESP_DISCARD_CMD       "*1\r\n$7\r\nDISCARD\r\n"
#define RESP_UNWATCH_CMD       "*1\r\n$7\r\nUNWATCH\r\n"
#define RESP_CLUSTER_SLOTS_CMD "*2\r\n$7\r\nCLUSTER\r\n$5\r\nSLOTS\r\n"
#define RESP_ASKING_CMD        "*1\r\n$6\r\nASKING\r\n"
#define RESP_READONLY_CMD      "*1\r\n$8\r\nREADONLY\r\n"
#define RESP_READWRITE_CMD     "*1\r\n$9\r\nREADWRITE\r\n"

#define RESP_READONLY_CMD_LEN (sizeof(RESP_READONLY_CMD)-1)

/* MOVED/ASK comparison macros */
#define IS_MOVED(p) (p[0]=='M' && p[1]=='O' && p[2]=='V' && p[3]=='E' && \
                     p[4]=='D' && p[5]==' ')
#define IS_ASK(p)   (p[0]=='A' && p[1]=='S' && p[2]=='K' && p[3]==' ')

/* MOVED/ASK lengths */
#define MOVED_LEN (sizeof("MOVED ")-1)
#define ASK_LEN   (sizeof("ASK ")-1)

/* Initial allocation size for key distribution container */
#define CLUSTER_KEYDIST_ALLOC 8

/* Macros to access nodes, sockets, and streams for a given slot */
#define SLOT(c,s) (c->master[s])
#define SLOT_SOCK(c,s) (SLOT(c,s)->sock)
#define SLOT_STREAM(c,s) (SLOT_SOCK(c,s)->stream)
#define SLOT_SLAVES(c,s) (c->master[s]->slaves)

/* Macros to access socket and stream for the node we're communicating with */
#define CMD_SOCK(c) (c->cmd_sock)
#define CMD_STREAM(c) (c->cmd_sock->stream)

/* Compare redirection slot information with what we have */
#define CLUSTER_REDIR_CMP(c) \
    (SLOT_SOCK(c,c->redir_slot)->port != c->redir_port || \
    ZSTR_LEN(SLOT_SOCK(c,c->redir_slot)->host) != c->redir_host_len || \
    memcmp(ZSTR_VAL(SLOT_SOCK(c,c->redir_slot)->host),c->redir_host,c->redir_host_len))

/* Clear out our "last error" */
#define CLUSTER_CLEAR_ERROR(c) do { \
    if (c->err) { \
        zend_string_release(c->err); \
        c->err = NULL; \
    } \
    c->clusterdown = 0; \
} while (0)

/* Protected sending of data down the wire to a RedisSock->stream */
#define CLUSTER_SEND_PAYLOAD(sock, buf, len) \
    (sock && !redis_sock_server_open(sock) && sock->stream && !redis_check_eof(sock, 1 ) && \
     php_stream_write(sock->stream, buf, len)==len)

/* Macro to read our reply type character */
#define CLUSTER_VALIDATE_REPLY_TYPE(sock, type) \
    (redis_check_eof(sock, 1) == 0 && \
     (php_stream_getc(sock->stream) == type))

/* Reset our last single line reply buffer and length */
#define CLUSTER_CLEAR_REPLY(c) \
    *c->line_reply = '\0'; c->reply_len = 0;

/* Helper to determine if we're in MULTI mode */
#define CLUSTER_IS_ATOMIC(c) (c->flags->mode != MULTI)

/* Helper that either returns false or adds false in multi mode */
#define CLUSTER_RETURN_FALSE(c) \
    if(CLUSTER_IS_ATOMIC(c)) { \
        RETURN_FALSE; \
    } else { \
        add_next_index_bool(&c->multi_resp, 0); \
        return; \
    }

/* Helper to either return a bool value or add it to MULTI response */
#define CLUSTER_RETURN_BOOL(c, b) \
    if(CLUSTER_IS_ATOMIC(c)) { \
        RETURN_BOOL(b); \
    } else { \
        add_next_index_bool(&c->multi_resp, b); \
    }

/* Helper to respond with a double or add it to our MULTI response */
#define CLUSTER_RETURN_DOUBLE(c, d) \
    if(CLUSTER_IS_ATOMIC(c)) { \
        RETURN_DOUBLE(d); \
    } else { \
        add_next_index_double(&c->multi_resp, d); \
    }

/* Helper to return a string value */
#define CLUSTER_RETURN_STRING(c, str, len) \
    if(CLUSTER_IS_ATOMIC(c)) { \
        RETVAL_STRINGL(str, len); \
    } else { \
        add_next_index_stringl(&c->multi_resp, str, len); \
    } \

/* Return a LONG value */
#define CLUSTER_RETURN_LONG(c, val) \
    if(CLUSTER_IS_ATOMIC(c)) { \
        RETURN_LONG(val); \
    } else { \
        add_next_index_long(&c->multi_resp, val); \
    }

/* Macro to clear out a clusterMultiCmd structure */
#define CLUSTER_MULTI_CLEAR(mc) \
    mc->cmd.len  = 0; \
    mc->args.len = 0; \
    mc->argc     = 0; \

/* Initialzie a clusterMultiCmd with a keyword and length */
#define CLUSTER_MULTI_INIT(mc, keyword, keyword_len) \
    mc.kw     = keyword; \
    mc.kw_len = keyword_len; \

#define CLUSTER_CACHING_ENABLED() (INI_INT("redis.clusters.cache_slots") == 1)

/* Cluster redirection enum */
typedef enum CLUSTER_REDIR_TYPE {
    REDIR_NONE,
    REDIR_MOVED,
    REDIR_ASK
} CLUSTER_REDIR_TYPE;

/* MULTI BULK response callback typedef */
typedef int  (*mbulk_cb)(RedisSock*,zval*,long long, void*);

/* A list of covered slot ranges */
typedef struct redisSlotRange {
    unsigned short low;
    unsigned short high;
} redisSlotRange;

/* Simple host/port information for our cache */
typedef struct redisCachedHost {
    zend_string *addr;
    unsigned short port;
} redisCachedHost;

/* Storage for a cached master node */
typedef struct redisCachedMaster {
    redisCachedHost host;

    redisSlotRange *slot;   /* Slots and count */
    size_t slots;

    redisCachedHost *slave; /* Slaves and their count */
    size_t slaves;
} redisCachedMaster;

typedef struct redisCachedCluster {
    // int rsrc_id;               /* Zend resource ID */
    zend_string *hash;         /* What we're cached by */
    redisCachedMaster *master; /* Array of masters */
    size_t count;              /* Number of masters */
} redisCachedCluster;

/* A Redis Cluster master node */
typedef struct redisClusterNode {
    RedisSock *sock;      /* Our Redis socket in question */
    short slot;           /* One slot we believe this node serves */
    zend_llist slots;     /* List of all slots we believe this node serves */
    unsigned short slave; /* Are we a slave */
    HashTable *slaves;    /* Hash table of slaves */
} redisClusterNode;

/* Forward declarations */
typedef struct clusterFoldItem clusterFoldItem;

/* RedisCluster implementation structure */
typedef struct redisCluster {

    /* One RedisSock struct for serialization and prefix information */
    RedisSock *flags;

    /* How long in milliseconds should we wait when being bounced around */
    long waitms;

    /* Are we flagged as being in readonly mode, meaning we could fall back to
     * a given master's slave */
    short readonly;

    /* RedisCluster failover options (never, on error, to load balance) */
    short failover;

    /* Hash table of seed host/ports */
    HashTable *seeds;

    /* RedisCluster masters, by direct slot */
    redisClusterNode *master[REDIS_CLUSTER_SLOTS];

    /* All RedisCluster objects we've created/are connected to */
    HashTable *nodes;

    /* Transaction handling linked list, and where we are as we EXEC */
    clusterFoldItem *multi_head;
    clusterFoldItem *multi_curr;

    /* When we issue EXEC to nodes, we need to keep track of how many replies
     * we have, as this can fail for various reasons (EXECABORT, watch, etc.) */
    char multi_len[REDIS_CLUSTER_SLOTS];

    /* Variable to store MULTI response */
    zval multi_resp;

    /* Flag for when we get a CLUSTERDOWN error */
    short clusterdown;

    /* Key to our persistent list cache and number of redirections we've
     * received since construction */
    zend_string *cache_key;
    uint64_t redirections;

    /* The last ERROR we encountered */
    zend_string *err;

    /* The slot our command is operating on, as well as it's socket */
    unsigned short cmd_slot;
    RedisSock *cmd_sock;

    /* The slot where we're subscribed */
    short subscribed_slot;

    /* The first line of our last reply, not including our reply type byte
     * or the trailing \r\n */
    char line_reply[1024];

    /* The last reply type and length or integer response we got */
    REDIS_REPLY_TYPE reply_type;
    long long reply_len;

    /* Last MOVED or ASK redirection response information */
    CLUSTER_REDIR_TYPE redir_type;
    char               redir_host[255];
    int                redir_host_len;
    unsigned short     redir_slot;
    unsigned short     redir_port;

    /* Zend object handler */
    zend_object std;
} redisCluster;

/* RedisCluster response processing callback */
typedef void (*cluster_cb)(INTERNAL_FUNCTION_PARAMETERS, redisCluster*, void*);

/* Context for processing transactions */
struct clusterFoldItem {
    /* Response processing callback */
    cluster_cb callback;

    /* The actual socket where we send this request */
    unsigned short slot;

    /* Any context we need to send to our callback */
    void *ctx;

    /* Next item in our list */
    struct clusterFoldItem *next;
};

/* Key and value container, with info if they need freeing */
typedef struct clusterKeyVal {
    char *key, *val;
    int  key_len,  val_len;
    int  key_free, val_free;
} clusterKeyVal;

/* Container to hold keys (and possibly values) for when we need to distribute
 * commands across more than 1 node (e.g. WATCH, MGET, MSET, etc) */
typedef struct clusterDistList {
    clusterKeyVal *entry;
    size_t len, size;
} clusterDistList;

/* Context for things like MGET/MSET/MSETNX.  When executing in MULTI mode,
 * we'll want to re-integrate into one running array, except for the last
 * command execution, in which we'll want to return the value (or add it) */
typedef struct clusterMultiCtx {
    /* Our running array */
    zval *z_multi;

    /* How many keys did we request for this bit */
    int count;

    /* Is this the last entry */
    short last;
} clusterMultiCtx;

/* Container for things like MGET, MSET, and MSETNX, which split the command
 * into a header and payload while aggregating to a specific slot. */
typedef struct clusterMultiCmd {
    /* Keyword and keyword length */
    char *kw;
    int  kw_len;

    /* Arguments in our payload */
    int argc;

    /* The full command, built into cmd, and args as we aggregate */
    smart_string cmd;
    smart_string args;
} clusterMultiCmd;

/* Hiredis like structure for processing any sort of reply Redis Cluster might
 * give us, including N level deep nested multi-bulk replies.  Unlike hiredis
 * we don't encode errors, here as that's handled in the cluster structure. */
typedef struct clusterReply {
    REDIS_REPLY_TYPE type;         /* Our reply type */
    size_t integer;                /* Integer reply */
    long long len;                 /* Length of our string */
    char *str;                     /* String reply */
    long long elements;            /* Count of array elements */
    struct clusterReply **element; /* Array elements */
} clusterReply;

/* Direct variant response handler */
clusterReply *cluster_read_resp(redisCluster *c, int status_strings);
clusterReply *cluster_read_sock_resp(RedisSock *redis_sock,
    REDIS_REPLY_TYPE type, char *line_reply, long long reply_len);
void cluster_free_reply(clusterReply *reply, int free_data);

/* Cluster distribution helpers for WATCH */
HashTable *cluster_dist_create();
void cluster_dist_free(HashTable *ht);
int cluster_dist_add_key(redisCluster *c, HashTable *ht, char *key,
    size_t key_len, clusterKeyVal **kv);
void cluster_dist_add_val(redisCluster *c, clusterKeyVal *kv, zval *val
   );

/* Aggregation for multi commands like MGET, MSET, and MSETNX */
void cluster_multi_init(clusterMultiCmd *mc, char *kw, int kw_len);
void cluster_multi_free(clusterMultiCmd *mc);
void cluster_multi_add(clusterMultiCmd *mc, char *data, int data_len);
void cluster_multi_fini(clusterMultiCmd *mc);

/* Hash a key to it's slot, using the Redis Cluster hash algorithm */
unsigned short cluster_hash_key_zval(zval *key);
unsigned short cluster_hash_key(const char *key, int len);

/* Validate and sanitize cluster construction args */
zend_string** cluster_validate_args(double timeout, double read_timeout, 
    HashTable *seeds, uint32_t *nseeds, char **errstr);

void free_seed_array(zend_string **seeds, uint32_t nseeds);

/* Generate a unique hash string from seeds array */
zend_string *cluster_hash_seeds(zend_string **seeds, uint32_t nseeds);

/* Get the current time in milliseconds */
long long mstime(void);

PHP_REDIS_API short cluster_send_command(redisCluster *c, short slot, const char *cmd,
    int cmd_len);

PHP_REDIS_API void cluster_disconnect(redisCluster *c, int force);

PHP_REDIS_API int cluster_send_exec(redisCluster *c, short slot);
PHP_REDIS_API int cluster_send_discard(redisCluster *c, short slot);
PHP_REDIS_API int cluster_abort_exec(redisCluster *c);
PHP_REDIS_API int cluster_reset_multi(redisCluster *c);

PHP_REDIS_API short cluster_find_slot(redisCluster *c, const char *host,
    unsigned short port);
PHP_REDIS_API int cluster_send_slot(redisCluster *c, short slot, char *cmd,
    int cmd_len, REDIS_REPLY_TYPE rtype);

PHP_REDIS_API redisCluster *cluster_create(double timeout, double read_timeout,
    int failover, int persistent);
PHP_REDIS_API void cluster_free(redisCluster *c, int free_ctx);
PHP_REDIS_API void cluster_init_seeds(redisCluster *c, zend_string **seeds, uint32_t nseeds);
PHP_REDIS_API int cluster_map_keyspace(redisCluster *c);
PHP_REDIS_API void cluster_free_node(redisClusterNode *node);

/* Functions for interacting with cached slots maps */
PHP_REDIS_API redisCachedCluster *cluster_cache_create(zend_string *hash, HashTable *nodes);
PHP_REDIS_API void cluster_cache_free(redisCachedCluster *rcc);
PHP_REDIS_API void cluster_init_cache(redisCluster *c, redisCachedCluster *rcc);

/* Functions to facilitate cluster slot caching */

PHP_REDIS_API char **cluster_sock_read_multibulk_reply(RedisSock *redis_sock, int *len);

PHP_REDIS_API int cluster_cache_store(zend_string *hash, HashTable *nodes);
PHP_REDIS_API redisCachedCluster *cluster_cache_load(zend_string *hash);

/*
 * Redis Cluster response handlers.  Our response handlers generally take the
 * following form:
 *      PHP_REDIS_API void handler(INTERNAL_FUNCTION_PARAMETERS, redisCluster *c,
 *          void *ctx)
 *
 * Reply handlers are responsible for setting the PHP return value (either to
 * something valid, or FALSE in the case of some failures).
 */

PHP_REDIS_API void cluster_bool_resp(INTERNAL_FUNCTION_PARAMETERS, redisCluster *c,
    void *ctx);
PHP_REDIS_API void cluster_ping_resp(INTERNAL_FUNCTION_PARAMETERS, redisCluster *c,
    void *ctx);
PHP_REDIS_API void cluster_set_resp(INTERNAL_FUNCTION_PARAMETERS, redisCluster *c,
    void *ctx);
PHP_REDIS_API void cluster_single_line_resp(INTERNAL_FUNCTION_PARAMETERS, redisCluster *c,
    void *ctx);
PHP_REDIS_API void cluster_bulk_resp(INTERNAL_FUNCTION_PARAMETERS, redisCluster *c,
    void *ctx);
PHP_REDIS_API void cluster_bulk_raw_resp(INTERNAL_FUNCTION_PARAMETERS, redisCluster *c,
    void *ctx);
PHP_REDIS_API void cluster_dbl_resp(INTERNAL_FUNCTION_PARAMETERS, redisCluster *c,
    void *ctx);
PHP_REDIS_API void cluster_1_resp(INTERNAL_FUNCTION_PARAMETERS, redisCluster *c,
    void *ctx);
PHP_REDIS_API void cluster_long_resp(INTERNAL_FUNCTION_PARAMETERS, redisCluster *c,
    void *ctx);
PHP_REDIS_API void cluster_type_resp(INTERNAL_FUNCTION_PARAMETERS, redisCluster *c,
    void *ctx);
PHP_REDIS_API void cluster_sub_resp(INTERNAL_FUNCTION_PARAMETERS, redisCluster *c,
    void *ctx);
PHP_REDIS_API void cluster_unsub_resp(INTERNAL_FUNCTION_PARAMETERS, redisCluster *c,
    void *ctx);

PHP_REDIS_API void cluster_variant_resp(INTERNAL_FUNCTION_PARAMETERS,
    redisCluster *c, void *ctx);

PHP_REDIS_API void cluster_variant_raw_resp(INTERNAL_FUNCTION_PARAMETERS,
    redisCluster *c, void *ctx);

PHP_REDIS_API void cluster_variant_resp_strings(INTERNAL_FUNCTION_PARAMETERS,
    redisCluster *c, void *ctx);

/* MULTI BULK response functions */
PHP_REDIS_API void cluster_gen_mbulk_resp(INTERNAL_FUNCTION_PARAMETERS,
    redisCluster *c, mbulk_cb func, void *ctx);
PHP_REDIS_API void cluster_mbulk_raw_resp(INTERNAL_FUNCTION_PARAMETERS,
    redisCluster *c, void *ctx);
PHP_REDIS_API void cluster_mbulk_resp(INTERNAL_FUNCTION_PARAMETERS,
    redisCluster *c, void *ctx);
PHP_REDIS_API void cluster_mbulk_zipstr_resp(INTERNAL_FUNCTION_PARAMETERS,
    redisCluster *c, void *ctx);
PHP_REDIS_API void cluster_mbulk_zipdbl_resp(INTERNAL_FUNCTION_PARAMETERS,
    redisCluster *c, void *ctx);
PHP_REDIS_API void cluster_mbulk_assoc_resp(INTERNAL_FUNCTION_PARAMETERS,
    redisCluster *c, void *ctx);
PHP_REDIS_API void cluster_multi_mbulk_resp(INTERNAL_FUNCTION_PARAMETERS,
    redisCluster *c, void *ctx);
PHP_REDIS_API zval *cluster_zval_mbulk_resp(INTERNAL_FUNCTION_PARAMETERS,
    redisCluster *c, int pull, mbulk_cb cb, zval *z_ret);

/* Handlers for things like DEL/MGET/MSET/MSETNX */
PHP_REDIS_API void cluster_del_resp(INTERNAL_FUNCTION_PARAMETERS,
    redisCluster *c, void *ctx);
PHP_REDIS_API void cluster_mbulk_mget_resp(INTERNAL_FUNCTION_PARAMETERS,
    redisCluster *c, void *ctx);
PHP_REDIS_API void cluster_mset_resp(INTERNAL_FUNCTION_PARAMETERS,
    redisCluster *c, void *ctx);
PHP_REDIS_API void cluster_msetnx_resp(INTERNAL_FUNCTION_PARAMETERS,
    redisCluster *c, void *ctx);

/* Response handler for ZSCAN, SSCAN, and HSCAN */
PHP_REDIS_API int cluster_scan_resp(INTERNAL_FUNCTION_PARAMETERS,
    redisCluster *c, REDIS_SCAN_TYPE type, long *it);

/* INFO response handler */
PHP_REDIS_API void cluster_info_resp(INTERNAL_FUNCTION_PARAMETERS,
    redisCluster *c, void *ctx);

/* CLIENT LIST response handler */
PHP_REDIS_API void cluster_client_list_resp(INTERNAL_FUNCTION_PARAMETERS,
    redisCluster *c, void *ctx);

/* Custom STREAM handlers */
PHP_REDIS_API void cluster_xread_resp(INTERNAL_FUNCTION_PARAMETERS,
    redisCluster *c, void *ctx);
PHP_REDIS_API void cluster_xrange_resp(INTERNAL_FUNCTION_PARAMETERS,
    redisCluster *c, void *ctx);
PHP_REDIS_API void cluster_xclaim_resp(INTERNAL_FUNCTION_PARAMETERS,
    redisCluster *c, void *ctx);
PHP_REDIS_API void cluster_xinfo_resp(INTERNAL_FUNCTION_PARAMETERS,
    redisCluster *c, void *ctx);

/* Custom ACL handlers */
PHP_REDIS_API void cluster_acl_getuser_resp(INTERNAL_FUNCTION_PARAMETERS, redisCluster *c, void *ctx);
PHP_REDIS_API void cluster_acl_log_resp(INTERNAL_FUNCTION_PARAMETERS, redisCluster *c, void *ctx);

/* MULTI BULK processing callbacks */
int mbulk_resp_loop(RedisSock *redis_sock, zval *z_result,
    long long count, void *ctx);
int mbulk_resp_loop_raw(RedisSock *redis_sock, zval *z_result,
    long long count, void *ctx);
int mbulk_resp_loop_zipstr(RedisSock *redis_sock, zval *z_result,
    long long count, void *ctx);
int mbulk_resp_loop_zipdbl(RedisSock *redis_sock, zval *z_result,
    long long count, void *ctx);
int mbulk_resp_loop_assoc(RedisSock *redis_sock, zval *z_result,
    long long count, void *ctx);

#endif

/* vim: set tabstop=4 softtabstop=4 expandtab shiftwidth=4: */
