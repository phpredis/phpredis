#ifndef _PHPREDIS_CLUSTER_LIBRARY_H
#define _PHPREDIS_CLUSTER_LIBRARY_H

#include "common.h"

#ifdef ZTS
#include "TSRM.h"
#endif

/* Redis cluster hash slots and N-1 which we'll use to find it */
#define REDIS_CLUSTER_SLOTS 16384
#define REDIS_CLUSTER_MOD   (REDIS_CLUSTER_SLOTS-1)

/* Minimum valid CLUSTER NODES line element count
   and the minimum we expect if there are slots */
#define CLUSTER_MIN_NODE_LINE     8
#define CLUSTER_MIN_SLOTS_COUNT   9

/* Length of a cluster name */
#define CLUSTER_NAME_LEN 40

/* RedisCluster class constants */
#define CLUSTER_OPT_DISTRIBUTE    5

/* Maintain order of execution vs. efficiency of delivery */
#define CLUSTER_DIST_OOE          0
#define CLUSTER_DIST_SPEED        1

/* The parts for our cluster nodes command */
#define CLUSTER_NODES_HASH        0
#define CLUSTER_NODES_HOST_PORT   1
#define CLUSTER_NODES_TYPE        2
#define CLUSTER_NODES_MASTER_HASH 3
#define CLUSTER_NODES_PING        4
#define CLUSTER_NODES_PONG        5
#define CLUSTER_NODES_EPOCH       6
#define CLUSTER_NODES_CONNECTED   7
#define CLUSTER_SLOTS             8

/* Complete representation for MULTI and EXEC in RESP */
#define RESP_MULTI_CMD   "*1\r\n$5\r\nMULTI\r\n"
#define RESP_EXEC_CMD    "*1\r\n$4\r\nEXEC\r\n"
#define RESP_DISCARD_CMD "*1\r\n$7\r\nDISCARD\r\n"

/* MOVED/ASK comparison macros */
#define IS_MOVED(p) (p[0]=='M' && p[1]=='O' && p[2]=='V' && p[3]=='E' && \
                     p[4]=='D' && p[5]==' ')
#define IS_ASK(p)   (p[0]=='A' && p[1]=='S' && p[3]=='K' && p[4]==' ')

/* MOVED/ASK lengths */
#define MOVED_LEN (sizeof("MOVED ")-1)
#define ASK_LEN   (sizeof("ASK ")-1)

/* Initial allocation size for key distribution container */
#define CLUSTER_KEYDIST_ALLOC 8

/* Slot/RedisSock/RedisSock->stream macros */
#define SLOT(c,s) (c->master[s])
#define SLOT_SOCK(c,s) (SLOT(c,s)->sock)
#define SLOT_STREAM(c,s) (SLOT_SOCK(c,s)->stream)

/* Compare redirection slot information with what we have */
#define CLUSTER_REDIR_CMP(c) \
    (SLOT_SOCK(c,c->redir_slot)->port != c->redir_port || \
    strlen(SLOT_SOCK(c,c->redir_slot)->host) != c->redir_host_len || \
    memcmp(SLOT_SOCK(c,c->redir_slot)->host,c->redir_host,c->redir_host_len))

/* Lazy connect logic */
#define CLUSTER_LAZY_CONNECT(s) \
    if(s->lazy_connect) { \
        s->lazy_connect = 0; \
        redis_sock_server_open(s, 1 TSRMLS_CC); \
    }

/* Clear out our "last error" */
#define CLUSTER_CLEAR_ERROR(c) \
    if(c->err) { \
        efree(c->err); \
        c->err = NULL; \
        c->err_len = 0; \
    }

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
        add_next_index_bool(c->multi_resp, 0); \
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
        add_next_index_bool(c->multi_resp, b); \
    }

/* Helper to respond with a double or add it to our MULTI response */
#define CLUSTER_RETURN_DOUBLE(c, d) \
    if(CLUSTER_IS_ATOMIC(c)) { \
        RETURN_DOUBLE(d); \
    } else { \
        add_next_index_double(c->multi_resp, d); \
    }

/* Helper to return a string value */
#define CLUSTER_RETURN_STRING(c, str, len) \
    if(CLUSTER_IS_ATOMIC(c)) { \
        RETURN_STRINGL(str, len, 0); \
    } else { \
        add_next_index_stringl(c->multi_resp, str, len, 0); \
    } \

/* Return a LONG value */
#define CLUSTER_RETURN_LONG(c, val) \
    if(CLUSTER_IS_ATOMIC(c)) { \
        RETURN_LONG(val); \
    } else { \
        add_next_index_long(c->multi_resp, val); \
    }

/* Cluster redirection enum */
typedef enum CLUSTER_REDIR_TYPE {
    REDIR_NONE,
    REDIR_MOVED,
    REDIR_ASK
} CLUSTER_REDIR_TYPE;

/* MULTI BULK response callback typedef */
typedef int  (*mbulk_cb)(RedisSock*,zval*,long long, void* TSRMLS_DC);

/* Specific destructor to free a cluster object */
// void redis_destructor_redis_cluster(zend_rsrc_list_entry *rsrc TSRMLS_DC);

/* Slot range structure */
typedef struct clusterSlotRange {
    unsigned short start, end;
} clusterSlotRange;

/* Bits related to CLUSTER NODES output */
typedef struct clusterNodeInfo {
    char *name, *master_name;

    short seed;

    char *host;
    int host_len;

    unsigned short port, slave;

    clusterSlotRange *slots;
    size_t slots_size;
} clusterNodeInfo;

/* A Redis Cluster master node */
typedef struct redisClusterNode {
    /* Our cluster ID and master ID */
    char *name;
    char *master_name;

    /* Our Redis socket in question */
    RedisSock *sock;

    /* Contiguous slots we serve */
    clusterSlotRange *slots;
    size_t slots_size;

    /* Is this a slave node */
    unsigned short slave;

    /* A HashTable containing any slaves */
    HashTable *slaves;
} redisClusterNode;

/* Forward decl */
typedef struct clusterFoldItem clusterFoldItem;

/* RedisCluster implementation structure */
typedef struct redisCluster {
    /* Object reference for Zend */
    zend_object std;

    /* Timeout and read timeout */
    double timeout;
    double read_timeout;

    /* Hash table of seed host/ports */
    HashTable *seeds;

    /* RedisCluster masters, by direct slot */
    redisClusterNode *master[REDIS_CLUSTER_SLOTS];

    /* All RedisCluster objects we've created/are connected to */
    HashTable *nodes;

    /* Transaction handling linked list, and where we are as we EXEC */
    clusterFoldItem *multi_head;
    clusterFoldItem *multi_curr;

    /* Variable to store MULTI response */
    zval *multi_resp;

    /* How many failures have we had in a row */
    int failures;

    /* The last ERROR we encountered */
    char *err;
    int err_len;

    /* The slot where we should read replies */
    short reply_slot;

    /* One RedisSock* struct for serialization and prefix information */
    RedisSock *flags;

    /* Cluster distribution mode (speed, vs. maintaining order of execution) */
    short dist_mode;

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
} redisCluster;

/* RedisCluster response processing callback */
typedef void (*cluster_cb)(INTERNAL_FUNCTION_PARAMETERS, redisCluster*, void*);

/* Context for processing transactions */
struct clusterFoldItem {
    /* Response processing callback */
    cluster_cb callback;

    /* The slot where this response was sent */
    short slot;

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

/* Cluster distribution helpers */
HashTable *cluster_dist_create();
void cluster_dist_free(HashTable *ht);
int cluster_dist_add_key(redisCluster *c, HashTable *ht, char *key, 
    int key_len, clusterKeyVal **kv);
void cluster_dist_add_val(redisCluster *c, clusterKeyVal *kv, zval *val 
    TSRMLS_CC);

/* Hash a key to it's slot, using the Redis Cluster hash algorithm */
unsigned short cluster_hash_key_zval(zval *key);
unsigned short cluster_hash_key(const char *key, int len);

PHPAPI short cluster_send_command(redisCluster *c, short slot, const char *cmd, 
    int cmd_len TSRMLS_DC);

PHPAPI void cluster_disconnect(redisCluster *c TSRMLS_DC);

PHPAPI int cluster_send_exec(redisCluster *c, short slot TSRMLS_DC);
PHPAPI int cluster_send_discard(redisCluster *c, short slot TSRMLS_DC);
PHPAPI int cluster_abort_exec(redisCluster *c TSRMLS_DC);
PHPAPI int cluster_reset_multi(redisCluster *c);

PHPAPI int cluster_send_direct(redisCluster *c, short slot, char *cmd, 
    int cmd_len, REDIS_REPLY_TYPE rtype TSRMLS_DC);

PHPAPI int cluster_init_seeds(redisCluster *c, HashTable *ht_seeds);
PHPAPI int cluster_map_keyspace(redisCluster *c TSRMLS_DC);
PHPAPI void cluster_free_node(redisClusterNode *node);

PHPAPI char **cluster_sock_read_multibulk_reply(RedisSock *redis_sock,
    int *len TSRMLS_DC);

PHPAPI int cluster_node_add_slave(redisCluster *c, redisClusterNode *master, 
    clusterNodeInfo *slave TSRMLS_DC);

/*
 * Redis Cluster response handlers.  All of our response handlers take the
 * following form:
 *      PHPAPI void handler(INTERNAL_FUNCTION_PARAMETERS, redisCluster *c)
 *
 * Reply handlers are responsible for setting the PHP return value (either to
 * something valid, or FALSE in the case of some failures).
 */

PHPAPI void cluster_bool_resp(INTERNAL_FUNCTION_PARAMETERS, redisCluster *c, 
    void *ctx);
PHPAPI void cluster_bulk_resp(INTERNAL_FUNCTION_PARAMETERS, redisCluster *c, 
    void *ctx);
PHPAPI void cluster_bulk_raw_resp(INTERNAL_FUNCTION_PARAMETERS, redisCluster *c, 
    void *ctx);
PHPAPI void cluster_dbl_resp(INTERNAL_FUNCTION_PARAMETERS, redisCluster *c, 
    void *ctx);
PHPAPI void cluster_1_resp(INTERNAL_FUNCTION_PARAMETERS, redisCluster *c, 
    void *ctx);
PHPAPI void cluster_long_resp(INTERNAL_FUNCTION_PARAMETERS, redisCluster *c, 
    void *ctx);
PHPAPI void cluster_type_resp(INTERNAL_FUNCTION_PARAMETERS, redisCluster *c, 
    void *ctx);

/* MULTI BULK response functions */
PHPAPI void cluster_gen_mbulk_resp(INTERNAL_FUNCTION_PARAMETERS, 
    redisCluster *c, mbulk_cb func, void *ctx);
PHPAPI void cluster_mbulk_raw_resp(INTERNAL_FUNCTION_PARAMETERS, 
    redisCluster *c, void *ctx);
PHPAPI void cluster_mbulk_resp(INTERNAL_FUNCTION_PARAMETERS, 
    redisCluster *c, void *ctx);
PHPAPI void cluster_mbulk_zipstr_resp(INTERNAL_FUNCTION_PARAMETERS,
    redisCluster *c, void *ctx);
PHPAPI void cluster_mbulk_zipdbl_resp(INTERNAL_FUNCTION_PARAMETERS,
    redisCluster *c, void *ctx);
PHPAPI void cluster_mbulk_assoc_resp(INTERNAL_FUNCTION_PARAMETERS, 
    redisCluster *c, void *ctx);
PHPAPI void cluster_multi_mbulk_resp(INTERNAL_FUNCTION_PARAMETERS,
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

/* vim: set tabstop=4 softtabstops=4 noexpandtab shiftwidth=4: */
