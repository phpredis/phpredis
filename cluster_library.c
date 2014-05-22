#include "php_redis.h"
#include "common.h"
#include "library.h"
#include "cluster_library.h"
#include "crc16.h"
#include <zend_exceptions.h>

extern zend_class_entry *redis_cluster_exception_ce;

static void free_cluster_info(clusterNodeInfo *info) {
    if(info->name) {
        efree(info->name);
    }
    if(info->master_name) {
        efree(info->master_name);
    }
    if(info->host) {
        efree(info->host);
    }
    efree(info);
}

/* Destructor callback for a hash table containing inner hash tables */
static void free_inner_ht(void *data) {
    if(*(HashTable**)data) {
        zend_hash_destroy(*(HashTable**)data);
        efree(*(HashTable**)data);
    }
}

/* Destructor for HashTable containing clusterNodeInfo */
static void free_inner_info(void *data) {
    clusterNodeInfo *info = *(clusterNodeInfo**)data;
    if(info) free_cluster_info(info);
}

/* Get the hash slot for a given key */
unsigned short cluster_hash_key(const char *key, int len) {
    int s, e;

    // Find first occurrence of {, if any
    for(s=0;s<len;s++) {
        if(key[s]=='{') break;
    }

    // There is no '{', hash everything
    return crc16(key, len) & REDIS_CLUSTER_MOD;

    // Found it, look for a tailing '}'
    for(e=s+1;e<len;e++) {
        if(key[e]=='}') break;
    }

    // Hash the whole key if we don't find a tailing } or if {} is empty
    if(e == len || e == s+1) return crc16(key, len) & REDIS_CLUSTER_MOD;

    // Hash just the bit betweeen { and }
    return crc16(key+s+1,e-s-1) & REDIS_CLUSTER_MOD;
}

/* Hash a key from a ZVAL */
unsigned short cluster_hash_key_zval(zval *z_key) {
    const char *kptr;
    char buf[255];
    int klen;
    
    switch(Z_TYPE_P(z_key)) {
        case IS_STRING:
            kptr = Z_STRVAL_P(z_key);
            klen = Z_STRLEN_P(z_key);
            break;
        case IS_LONG:
            klen = snprintf(buf,sizeof(buf),"%ld",Z_LVAL_P(z_key));
            kptr = (const char *)buf;
            break;
        case IS_DOUBLE:
            klen = snprintf(buf,sizeof(buf),"%f",Z_DVAL_P(z_key));
            kptr = (const char *)buf;
            break;
        case IS_ARRAY:
            kptr = "Array";
            klen = sizeof("Array")-1;
            break;
        case IS_OBJECT:
            kptr = "Object";
            klen = sizeof("Object")-1;
            break;
    }

    // Hash the string representation
    return cluster_hash_key(kptr, klen);
}

static char **split_str_by_delim(char *str, char *delim, int *len) {
    char **array, *tok, *tok_buf;
    int size=16;

    *len = 0;

    // Initial storage
    array = emalloc(size * sizeof(char*));

    tok = php_strtok_r(str, delim, &tok_buf);

    while(tok) {
        if(size == *len) {
            size *= 2;
            array = erealloc(array, size * sizeof(char*));
        }

        array[*len] = tok;
        (*len)++;

        tok = php_strtok_r(NULL, delim, &tok_buf);
    }

    return array;
}

/* Simple function to split a string into up to N parts */
static int
cluster_parse_node_line(RedisSock *sock, char *line, clusterNodeInfo *info) {
    char **array, *p;
    int count;

    // First split by space
    array = split_str_by_delim(line, " ", &count);

    if(count != CLUSTER_NODES_MASTER_ELE && count != CLUSTER_NODES_SLAVE_ELE) {
        efree(array);
        return -1;
    }
    
    // Sanity check on our cluster ID value
    if(strlen(array[CLUSTER_NODES_HASH])!=CLUSTER_NAME_LEN) {
        efree(array);
        return -1;
    }

    // Our cluster ID
    info->name = estrndup(array[CLUSTER_NODES_HASH], CLUSTER_NAME_LEN);

    // Set the host/port bits
    if(memcmp(array[CLUSTER_NODES_HOST_PORT], ":0", sizeof(":0"))==0) {
        info->seed = 1;
        info->host_len = strlen(sock->host);
        info->host = estrndup(sock->host, info->host_len);
        info->port = sock->port;
    } else if((p = strchr(array[CLUSTER_NODES_HOST_PORT], ':'))!=NULL) {
        /* Null terminate at the : character */
        *p = '\0';

        info->seed = 0;
        info->host_len = p - array[CLUSTER_NODES_HOST_PORT];
        info->host = estrndup(array[CLUSTER_NODES_HOST_PORT], info->host_len);
        info->port = atoi(p+1);
    } else {
        efree(array);
        return -1;
    }
    
    // If we've got a master hash slot, set it
    if(memcmp(array[CLUSTER_NODES_MASTER_HASH], "-", sizeof("-"))!=0) {
        if(strlen(array[CLUSTER_NODES_MASTER_HASH])!=CLUSTER_NAME_LEN) {
            efree(array);
            return -1;
        }
        info->master_name = estrndup(array[CLUSTER_NODES_MASTER_HASH], CLUSTER_NAME_LEN);
    } else {
        info->master_name = NULL;
    }

    // If this is a master, parse slots
    if(count == CLUSTER_NODES_MASTER_ELE) {
        // This must be in the form <start>-<end>
        if((p = strchr(array[CLUSTER_SLOTS], '-'))==NULL) {
            efree(array);
            return -1;
        }

        // Null terminate a the - 
        *p = '\0';

        // Pull out start and end slot
        info->start_slot = atoi(array[CLUSTER_SLOTS]);
        info->end_slot   = atoi(p+1);
    } else {
        info->start_slot = 0;
        info->end_slot   = 0;
    }

    // Free our array
    efree(array);

    // Success!
    return 0;
}

/* 
 * Execute a CLUSTER NODES command against the given seed and return an array
 * of nodes that came back, along with setting a pointer as to how many there
 * are.
 */
static clusterNodeInfo 
**cluster_get_nodes(redisCluster *cluster, int *len, RedisSock *redis_sock
                    TSRMLS_DC)
{
    clusterNodeInfo **nodes;
    REDIS_REPLY_TYPE type;
    char *cmd, *reply, **lines;
    int cmd_len, i, j, count;

    // Create our CLUSTER NODES command
    cmd_len = redis_cmd_format_static(&cmd, "CLUSTER", "s", "NODES",
                                      sizeof("NODES")-1);

    // Send the command directly to this socket
    if(redis_sock_write(redis_sock, cmd, cmd_len TSRMLS_CC)<0) {
        efree(cmd);
        return NULL;
    }

    // Make sure we've got a bulk reply and we can read it
    if(redis_read_reply_type(redis_sock, &type, len TSRMLS_CC)<0 || type!=TYPE_BULK ||
       (reply = redis_sock_read_bulk_reply(redis_sock, *len TSRMLS_CC))==NULL)
    {
        efree(cmd);
        return NULL;
    }

    // Split by \n
    lines = split_str_by_delim(reply, "\n", &count);

    // Allocate storage for nodes
    nodes = ecalloc(count, sizeof(clusterNodeInfo*));

    for(i=0;i<count;i++) {
        nodes[i] = emalloc(sizeof(clusterNodeInfo));
        if(cluster_parse_node_line(redis_sock, lines[i], nodes[i])<0) {
            efree(reply);
            for(j=0;j<i;j++) {
                free_cluster_info(nodes[i]);
            }
            efree(nodes);
            return NULL;
        }
    }

    efree(cmd);
    efree(lines);
    efree(reply);

    // Success, return our nodes
    *len = count;
    return nodes;
}

/* Create a cluster node struct */
static redisClusterNode* 
cluster_node_create(redisCluster *cluster, clusterNodeInfo *info) {
    redisClusterNode *node;

    node = ecalloc(1, sizeof(redisClusterNode));
    
    // Set top level cluster info
    node->name = estrndup(info->name, CLUSTER_NAME_LEN);
    if(info->master_name) {
        node->master_name = estrndup(info->master_name, CLUSTER_NAME_LEN);
    }
    node->start_slot = info->start_slot;
    node->end_slot = info->end_slot;

    /* Attach our RedisSock */
    /* TODO: Lazy connect as an option? */
    node->sock = redis_sock_create(info->host, info->host_len, info->port,
        cluster->timeout, 0, NULL, 0, 1);

    // Return our node
    return node;
}

/* Free a redisClusterNode structure */
PHPAPI void cluster_free_node(redisClusterNode *node) {
    efree(node->name);
    if(node->master_name) {
        efree(node->master_name);
    }
    if(node->slaves) {
        zend_hash_destroy(node->slaves);
        efree(node->slaves);
    }
    redis_free_socket(node->sock);
    efree(node);
}

/* Free an array of clusterNodeInfo structs */
void cluster_free_info_array(clusterNodeInfo **array, int count) {
    int i;

    // Free each info structure we've got
    for(i=0;i<count;i++) {
        free_cluster_info(array[i]);
    }

    // Free the array itself
    efree(array);
}

/* Attach a slave to a cluster node */
int
cluster_node_add_slave(redisCluster *cluster, redisClusterNode *master,
                       clusterNodeInfo *slave TSRMLS_DC)
{
    redisClusterNode *slave_node;
    char key[1024];
    int key_len;

    // Allocate our hash table if needed
    if(!master->slaves) {
        ALLOC_HASHTABLE(master->slaves);
        zend_hash_init(master->slaves, 0, NULL, NULL, 0);
    }

    // Create our slave node
    slave_node = cluster_node_create(cluster, slave);
    
    // Attach it to our slave
    if(zend_hash_next_index_insert(master->slaves, (void*)&slave_node,
        sizeof(redisClusterNode*), NULL)==FAILURE) 
    {
        return -1;
    }

    // Index by host:port
    key_len = snprintf(key, sizeof(key), "%s:%u", slave_node->sock->host,
        slave_node->sock->port);

    // Add this to our overall table of nodes
    zend_hash_update(cluster->nodes, key, key_len+1,
        (void*)&slave_node, sizeof(redisClusterNode*),NULL);

    // Success
    return 0;
}

/* Given masters and slaves from node discovery, set up our nodes */
static int 
cluster_set_node_info(redisCluster *cluster, HashTable *masters, 
                      HashTable *slaves TSRMLS_DC)
{
    clusterNodeInfo **info, **slave;
    HashTable **master_slaves;
    redisClusterNode *node;
    char *name, key[1024];
    int i, key_len;
    uint name_len;
    ulong idx;
    
    // Iterate over our master nodes
    for(zend_hash_internal_pointer_reset(masters);
        zend_hash_has_more_elements(masters)==SUCCESS;
        zend_hash_move_forward(masters))
    {
        // Get our node name (stored in the key) as well as information.
        zend_hash_get_current_key_ex(masters, &name, &name_len, &idx, 0, NULL);
        zend_hash_get_current_data(masters, (void**)&info);

        // Create our master node
        node = cluster_node_create(cluster, *info);

        // If we find slaves for this master, add them to the node
        if(zend_hash_find(slaves, (*info)->name, CLUSTER_NAME_LEN+1,
                          (void**)&master_slaves)==SUCCESS)
        {
            // Iterate through the slaves for this node
            for(zend_hash_internal_pointer_reset(*master_slaves);
                zend_hash_has_more_elements(*master_slaves)==SUCCESS;
                zend_hash_move_forward(*master_slaves))
            {
                zend_hash_get_current_data(*master_slaves, (void**)&slave);

                if(cluster_node_add_slave(cluster, node, *slave TSRMLS_CC)!=0) {
                    zend_throw_exception(redis_cluster_exception_ce,
                        "Can't add slave node to master instance", 0 TSRMLS_CC);
                    return -1;
                }
            }
        }

        // Point appropriate slots to this node
        for(i=node->start_slot;i<=node->end_slot;i++) {
            cluster->master[i] = node;
        }

        // Create a host:port key for this node
        key_len = snprintf(key, sizeof(key), "%s:%u", node->sock->host,
            node->sock->port);

        zend_hash_update(cluster->nodes, key, key_len+1,
            (void*)&node, sizeof(redisClusterNode*),NULL);
    }

    // Success
    return 0;
}

/* Initialize seeds */
PHPAPI int
cluster_init_seeds(redisCluster *cluster, HashTable *ht_seeds) {
    RedisSock *redis_sock;
    char *str, *psep, key[1024];
    int key_len;
    zval **z_seed;

    // Iterate our seeds array
    for(zend_hash_internal_pointer_reset(ht_seeds);
        zend_hash_has_more_elements(ht_seeds)==SUCCESS;
        zend_hash_move_forward(ht_seeds))
    {
        // Grab seed string
        zend_hash_get_current_data(ht_seeds, (void**)&z_seed);

        // Skip anything that isn't a string
        if(Z_TYPE_PP(z_seed)!=IS_STRING)
            continue;

        // Grab a copy of the string
        str = Z_STRVAL_PP(z_seed);

        // Must be in host:port form
        if(!(psep = strchr(str, ':')))
            continue;

        // Allocate a structure for this seed
        redis_sock = redis_sock_create(str, psep-str,
            (unsigned short)atoi(psep+1), cluster->timeout,0,NULL,0,0);

        // Index this seed by host/port
        key_len = snprintf(key, sizeof(key), "%s:%u", redis_sock->host,
            redis_sock->port);

        // Add to our seed HashTable
        zend_hash_update(cluster->seeds, key, key_len+1, (void*)&redis_sock,
            sizeof(RedisSock*),NULL);
    }

    // Success if at least one seed seems valid
    return zend_hash_num_elements(cluster->seeds) > 0 ? 0 : -1;
}

/* Initial mapping of our cluster keyspace */
PHPAPI int
cluster_map_keyspace(redisCluster *cluster TSRMLS_DC) {
    RedisSock **seed;
    clusterNodeInfo **node;

    HashTable *masters, *slaves, **sub_slaves;
    int count, valid=0, i;

    // Iterate over our seeds attempting to map keyspace
    for(zend_hash_internal_pointer_reset(cluster->seeds);
        zend_hash_has_more_elements(cluster->seeds) == SUCCESS;
        zend_hash_move_forward(cluster->seeds))
    {
        // Grab the redis_sock for this seed
        zend_hash_get_current_data(cluster->seeds, (void**)&seed);

        // Attempt to connect to this seed node
        if(redis_sock_connect(*seed TSRMLS_CC)!=0) {
            continue;
        }

        // Parse out cluster nodes.  Flag mapped if we are valid
        if((node = cluster_get_nodes(cluster, &count, *seed TSRMLS_CC))) {
            valid = 1;
            break;
        }        
    }

    // Throw an exception if we couldn't map
    if(!valid) {
        zend_throw_exception(redis_cluster_exception_ce,
            "Couldn't map cluster keyspace using any provided seed", 0 
            TSRMLS_CC);
        return -1;
    }

    // Hashes for masters and slaves
    ALLOC_HASHTABLE(masters);
    zend_hash_init(masters, 0, NULL, free_inner_info, 0);
    ALLOC_HASHTABLE(slaves);
    zend_hash_init(slaves, 0, NULL, free_inner_ht, 0);

    // Iterate nodes, splitting into master and slave groups
    for(i=0;i<count;i++) {
        if(node[i]->master_name == NULL) {
            zend_hash_update(masters, node[i]->name, CLUSTER_NAME_LEN+1, 
                (void*)&node[i], sizeof(clusterNodeInfo*), NULL);
        } else {
            HashTable *ht_inner;

            // Determine if we've already got at least one slave for this master
            if(zend_hash_find(slaves, node[i]->master_name, CLUSTER_NAME_LEN+1,
                              (void**)&sub_slaves)==FAILURE)
            {
                ALLOC_HASHTABLE(ht_inner);
                zend_hash_init(ht_inner, 0, NULL, free_inner_info, 0);

                zend_hash_update(slaves, node[i]->master_name,
                    CLUSTER_NAME_LEN+1, (void*)&ht_inner, sizeof(HashTable*),
                    NULL);
            } else {
                ht_inner = *sub_slaves;
            }

            // Add to this masters slaves.
            zend_hash_next_index_insert(ht_inner, (void*)&node[i],
                sizeof(clusterNodeInfo*), NULL);
        }
    }

    // Now that we have the key space mapped by master ID, we can set
    // socket information on them for communication.
    cluster_set_node_info(cluster, masters, slaves TSRMLS_CC);

    // Free our array of clusterNodeInfo* objects.  The HashTables will clean
    // up the clusterNodeInfo* pointers themselves.
    efree(node);

    // Destroy our hash tables
    zend_hash_destroy(masters);
    efree(masters);
    zend_hash_destroy(slaves);
    efree(slaves);

    return 0;
}

/* vim: set tabstop=4 softtabstops=4 noexpandtab shiftwidth=4: */
