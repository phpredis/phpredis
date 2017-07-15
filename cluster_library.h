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
#define IS_ASK(p)   (p[0]=='A' && p[1]=='S' && p[3]=='K' && p[4]==' ')

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
    strlen(SLOT_SOCK(c,c->redir_slot)->host) != c->redir_host_len || \
    memcmp(SLOT_SOCK(c,c->redir_slot)->host,c->redir_host,c->redir_host_len))

/* Lazy connect logic */
#define CLUSTER_LAZY_CONNECT(s) \
    if(s->lazy_connect) { \
        s->lazy_connect = 0; \
        redis_sock_server_open(s TSRMLS_CC); \
    }

/* Clear out our "last error" */
#define CLUSTER_CLEAR_ERROR(c) \
    if(c->err) efree(c->err); \
    c->err = NULL; \
    c->err_len = 0; \
    c->clusterdown = 0;

/* Protected sending of data down the wire to a RedisSock->stream */
#define CLUSTER_SEND_PAYLOAD(sock, buf, len) \
    (sock && sock->stream && !redis_check_eof(sock, 1 TSRMLS_CC) && \
     php_stream_write(sock->stream, buf, len)==len)

/* Macro to read our reply type character */
#define CLUSTER_VALIDATE_REPLY_TYPE(sock, type) \
    (redis_check_eof(sock, 1 TSRMLS_CC) == 0 && \
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
        if(b==1) {\
            RETURN_TRUE; \
        } else {\
            RETURN_FALSE; \
        } \
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

/* Cluster redirection enum */
typedef enum CLUSTER_REDIR_TYPE {
    REDIR_NONE,
    REDIR_MOVED,
    REDIR_ASK
} CLUSTER_REDIR_TYPE;

/* MULTI BULK response callback typedef */
typedef int  (*mbulk_cb)(RedisSock*,zval*,long long, void* TSRMLS_DC);

/* Specific destructor to free a cluster object */
// void redis_destructor_redis_cluster(zend_resource *rsrc TSRMLS_DC);

/* A Redis Cluster master node */
typedef struct redisClusterNode {
    /* Our Redis socket in question */
    RedisSock *sock;

    /* A slot where one of these lives */
    short slot;

    /* Is this a slave node */
    unsigned short slave;

    /* A HashTable containing any slaves */
    HashTable *slaves;
} redisClusterNode;

/* Forward declarations */
typedef struct clusterFoldItem clusterFoldItem;

/* RedisCluster implementation structure */
typedef struct redisCluster {
#if (PHP_MAJOR_VERSION < 7)
    zend_object std;
#endif

    /* Timeout and read timeout (for normal operations) */
    double timeout;
    double read_timeout;

    /* Are we using persistent connections */
    int persistent;

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

    /* The last ERROR we encountered */
    char *err;
    int err_len;

    /* The slot our command is operating on, as well as it's socket */
    unsigned short cmd_slot;
    RedisSock *cmd_sock;

    /* The slot where we're subscribed */
    short subscribed_slot;

    /* One RedisSock struct for serialization and prefix information */
    RedisSock *flags;

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

#if (PHP_MAJOR_VERSION >= 7)
    /* Zend object handler */
    zend_object std;
#endif
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
    size_t elements;               /* Count of array elements */
    struct clusterReply **element; /* Array elements */
} clusterReply;

/* Direct variant response handler */
clusterReply *cluster_read_resp(redisCluster *c TSRMLS_DC);
clusterReply *cluster_read_sock_resp(RedisSock *redis_sock, 
    REDIS_REPLY_TYPE type, size_t reply_len TSRMLS_DC);
void cluster_free_reply(clusterReply *reply, int free_data);

/* Cluster distribution helpers for WATCH */
HashTable *cluster_dist_create();
void cluster_dist_free(HashTable *ht);
int cluster_dist_add_key(redisCluster *c, HashTable *ht, char *key, 
    strlen_t key_len, clusterKeyVal **kv);
void cluster_dist_add_val(redisCluster *c, clusterKeyVal *kv, zval *val 
    TSRMLS_DC);

/* Aggregation for multi commands like MGET, MSET, and MSETNX */
void cluster_multi_init(clusterMultiCmd *mc, char *kw, int kw_len);
void cluster_multi_free(clusterMultiCmd *mc);
void cluster_multi_add(clusterMultiCmd *mc, char *data, int data_len);
void cluster_multi_fini(clusterMultiCmd *mc);

/* Hash a key to it's slot, using the Redis Cluster hash algorithm */
unsigned short cluster_hash_key_zval(zval *key);
unsigned short cluster_hash_key(const char *key, int len);

/* Get the current time in miliseconds */
long long mstime(void);

PHP_REDIS_API short cluster_send_command(redisCluster *c, short slot, const char *cmd, 
    int cmd_len TSRMLS_DC);

PHP_REDIS_API void cluster_disconnect(redisCluster *c TSRMLS_DC);

PHP_REDIS_API int cluster_send_exec(redisCluster *c, short slot TSRMLS_DC);
PHP_REDIS_API int cluster_send_discard(redisCluster *c, short slot TSRMLS_DC);
PHP_REDIS_API int cluster_abort_exec(redisCluster *c TSRMLS_DC);
PHP_REDIS_API int cluster_reset_multi(redisCluster *c);

PHP_REDIS_API short cluster_find_slot(redisCluster *c, const char *host,
    unsigned short port);
PHP_REDIS_API int cluster_send_slot(redisCluster *c, short slot, char *cmd, 
    int cmd_len, REDIS_REPLY_TYPE rtype TSRMLS_DC);

PHP_REDIS_API redisCluster *cluster_create(double timeout, double read_timeout,
    int failover, int persistent);
PHP_REDIS_API void cluster_free(redisCluster *c);
PHP_REDIS_API int cluster_init_seeds(redisCluster *c, HashTable *ht_seeds);
PHP_REDIS_API int cluster_map_keyspace(redisCluster *c TSRMLS_DC);
PHP_REDIS_API void cluster_free_node(redisClusterNode *node);

PHP_REDIS_API char **cluster_sock_read_multibulk_reply(RedisSock *redis_sock,
    int *len TSRMLS_DC);

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

/* Generic/Variant handler for stuff like EVAL */
PHP_REDIS_API void cluster_variant_resp(INTERNAL_FUNCTION_PARAMETERS,
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

/* MULTI BULK processing callbacks */
int mbulk_resp_loop(RedisSock *redis_sock, zval *z_result, 
    long long count, void *ctx TSRMLS_DC);
int mbulk_resp_loop_raw(RedisSock *redis_sock, zval *z_result, 
    long long count, void *ctx TSRMLS_DC);
int mbulk_resp_loop_zipstr(RedisSock *redis_sock, zval *z_result,
    long long count, void *ctx TSRMLS_DC);
int mbulk_resp_loop_zipdbl(RedisSock *redis_sock, zval *z_result,
    long long count, void *ctx TSRMLS_DC);
int mbulk_resp_loop_assoc(RedisSock *redis_sock, zval *z_result,
    long long count, void *ctx TSRMLS_DC);

#endif

/* vim: set tabstop=4 softtabstop=4 expandtab shiftwidth=4: */
