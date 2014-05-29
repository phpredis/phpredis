#ifndef _PHPREDIS_CLUSTER_LIBRARY_H
#define _PHPREDIS_CLUSTER_LIBRARY_H

#include "common.h"

#ifdef ZTS
#include "TSRM.h"
#endif

/* Redis cluster hash slots and N-1 which we'll use to find it */
#define REDIS_CLUSTER_SLOTS 16384
#define REDIS_CLUSTER_MOD   (REDIS_CLUSTER_SLOTS-1)

/* Nodes we expect for slave or master */
#define CLUSTER_NODES_MASTER_ELE 9
#define CLUSTER_NODES_SLAVE_ELE 8

/* Length of a cluster name */
#define CLUSTER_NAME_LEN 40

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

/* Cluster redirection enum */
typedef enum CLUSTER_REDIR_TYPE {
    REDIR_NONE,
    REDIR_MOVED,
    REDIR_ASK
} CLUSTER_REDIR_TYPE;

/* MOVED/ASK comparison macros */
#define IS_MOVED(p) (p[0]=='M' && p[1]=='O' && p[2]=='V' && p[3]=='E' && \
                     p[5]=='D' && p[6]==' ')
#define IS_ASK(p)   (p[0]=='A' && p[1]=='S' && p[3]=='K' && p[4]==' ')

/* MOVED/ASK lengths */
#define MOVED_LEN (sizeof("MOVED ")-1)
#define ASK_LEN   (sizeof("ASK ")-1)

/* Slot/RedisSock/RedisSock->stream macros */
#define SLOT(c,s) (c->master[s])
#define SLOT_SOCK(c,s) (SLOT(c,s)->sock)
#define SLOT_STREAM(c,s) (SLOT_SOCK(c,s)->stream)

/* Compare redirection slot information with what we have */
#define CLUSTER_REDIR_CMP(c) \
    (SLOT_SOCK(c,c->redir_slot)->port != c->redir_port || \
    strlen(SLOT_SOCK(c,c->redir_slot)->host) != c->redir_host_len || \
    memcmp(SLOT_SOCK(c,c->redir_slot)->host,c->redir_host,c->redir_host_len))

/* Send a request to our cluster, and process it's response */
#define CLUSTER_PROCESS_REQUEST(cluster, slot, cmd, cmd_len, resp_cb) \
    if(cluster_send_command(cluster,slot,cmd,cmd_len TSRMLS_CC)<0 || \
       resp_cb(cluster, INTERNAL_FUNCTION_PARAM_PASSTHRU)<0) \
    { \
        RETVAL_FALSE; \
    } \
    efree(cmd); \`


/* Specific destructor to free a cluster object */
// void redis_destructor_redis_cluster(zend_rsrc_list_entry *rsrc TSRMLS_DC);

/* Bits related to CLUSTER NODES output */
typedef struct clusterNodeInfo {
    char *name, *master_name;

    short seed;

    char *host;
    int host_len;

    unsigned short port;

    unsigned short start_slot;
    unsigned short end_slot;
} clusterNodeInfo;

/* A Redis Cluster master node */
typedef struct redisClusterNode {
    /* Our cluster ID and master ID */
    char *name;
    char *master_name;

    /* Our Redis socket in question */
    RedisSock *sock;

    /* Our start and end slots that we serve */
    unsigned short start_slot;
    unsigned short end_slot;    

    /* A HashTable containing any slaves */
    HashTable *slaves;
} redisClusterNode;

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

    /* The last ERROR we encountered */
    char *err;
    int err_len;

    /* The slot where we should read replies */
    short reply_slot;

    /* The last reply length we got, which we'll use to parse and
     * format our replies to the client. */
    int reply_len;

    /* Last MOVED or ASK redirection response information */
    CLUSTER_REDIR_TYPE redir_type;
    char               redir_host[255];
    int                redir_host_len;
    unsigned short     redir_slot;
    unsigned short     redir_port;
} redisCluster;

/* Hash a key to it's slot, using the Redis Cluster hash algorithm */
unsigned short cluster_hash_key_zval(zval *key);
unsigned short cluster_hash_key(const char *key, int len);

/* Send a command to where we think the key(s) should live and redirect when 
 * needed */
PHPAPI short cluster_send_command(redisCluster *cluster, short slot, 
                                const char *cmd, int cmd_len TSRMLS_DC);

PHPAPI int cluster_init_seeds(redisCluster *cluster, HashTable *ht_seeds);
PHPAPI int cluster_map_keyspace(redisCluster *cluster TSRMLS_DC);
PHPAPI void cluster_free_node(redisClusterNode *node);

PHPAPI char **cluster_sock_read_multibulk_reply(RedisSock *redis_sock, 
                                                int *len TSRMLS_DC);

PHPAPI int cluster_node_add_slave(redisCluster *cluster, 
                                  redisClusterNode *master, 
                                  clusterNodeInfo *slave TSRMLS_DC);

/* Response handlers */
PHPAPI int cluster_bulk_response(

#endif
