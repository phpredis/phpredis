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
    char            *name;
    char            *master_name;

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
} redisCluster;

/* Hash a key to it's slot, using the Redis Cluster hash algorithm */
unsigned short cluster_hash_key(const char *key, int len);

PHPAPI int cluster_init_seeds(redisCluster *cluster, HashTable *ht_seeds);
PHPAPI int cluster_map_keyspace(redisCluster *cluster TSRMLS_DC);
PHPAPI void cluster_free_node(redisClusterNode *node);

PHPAPI char **cluster_sock_read_multibulk_reply(RedisSock *redis_sock, int *len TSRMLS_DC);

PHPAPI int cluster_node_add_slave(redisCluster *cluster, redisClusterNode *master, clusterNodeInfo *slave TSRMLS_DC);

#endif
