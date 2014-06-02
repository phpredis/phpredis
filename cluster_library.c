#include "php_redis.h"
#include "common.h"
#include "library.h"
#include "cluster_library.h"
#include "crc16.h"
#include <zend_exceptions.h>

extern zend_class_entry *redis_cluster_exception_ce;

/* Some debug methods that will go away when we're through with them */

static void cluster_dump_nodes(redisCluster *c) {
    redisClusterNode **pp, *p;
    int i;
    for(zend_hash_internal_pointer_reset(c->nodes);
        zend_hash_has_more_elements(c->nodes)==SUCCESS;
        zend_hash_move_forward(c->nodes))
    {
        zend_hash_get_current_data(c->nodes, (void**)&pp);
        p = *pp;

        const char *slave = (p->slave) ? "slave" : "master";
        php_printf("%d %s %d ", p->sock->port, slave,p->sock->prefix_len);
        for(i=0;i<p->slots_size;i++) {
            php_printf("[%d-%d] ",p->slots[i].start,p->slots[i].end);
        }
        php_printf("\n");
    }
}

/* Set our last error string encountered */
static void cluster_set_err(redisCluster *c, char *err, int err_len)
{
    if(err && err_len>0) {
        if(c->err == NULL) {
            c->err = emalloc(err_len+1);
        } else if(err_len > c->err_len) {
            c->err = erealloc(c->err, err_len + 1);
        }
        memcpy(c->err,err,err_len);
        c->err[err_len]='\0';
        c->err_len = err_len;
    } else {
        if(c->err) efree(c->err);
        c->err = NULL;
        c->err_len = 0;
    }
}

/* Free a cluster info structure */
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
    if(info->slots) {
        efree(info->slots);
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

    // Switch based on ZVAL type
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
    char **array, *p, *p2;
    int count, i;

    // First split by space
    array = split_str_by_delim(line, " ", &count);

    // Sanity check on the number of elements we see
    if(count < CLUSTER_MIN_NODE_LINE) {
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
    
    // Set the host/port
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
    
    // Flag this a slave node if it's a slave
    info->slave = strstr(array[CLUSTER_NODES_TYPE], "slave")!=NULL;

    // If we've got a master hash slot, set it
    if(memcmp(array[CLUSTER_NODES_MASTER_HASH], "-", sizeof("-"))!=0) {
        if(strlen(array[CLUSTER_NODES_MASTER_HASH])!=CLUSTER_NAME_LEN) {
            efree(array);
            return -1;
        }
        info->master_name = estrndup(array[CLUSTER_NODES_MASTER_HASH],
            CLUSTER_NAME_LEN);
    } else if(info->slave) {
        // Slaves should always have a master hash
        efree(array);
        return -1;
    }
    
    // See if the node serves slots
    if(count >= CLUSTER_MIN_SLOTS_COUNT) {
        // Allocate for enough ranges
        info->slots_size = count - CLUSTER_MIN_SLOTS_COUNT + 1;
        info->slots = ecalloc(info->slots_size, sizeof(clusterSlotRange));

        // Now iterate over each range
        for(i=0;i<info->slots_size;i++) {
            p = array[i+CLUSTER_MIN_SLOTS_COUNT-1];

            // If we don't see -, this node only serves one slot
            if((p2 = strchr(p,'-'))==NULL) {
                info->slots[i].start = atoi(p);
                info->slots[i].end   = atoi(p);
            } else {
                *p2++ = '\0';

                // Set this range
                info->slots[i].start = atoi(p);
                info->slots[i].end = atoi(p2);
            }
        }
    } else {
        info->slots_size = 0;
        info->slots = NULL;
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
    if(redis_read_reply_type(redis_sock, &type, len TSRMLS_CC)<0 ||
       type!=TYPE_BULK || (reply = redis_sock_read_bulk_reply(redis_sock,
       *len TSRMLS_CC))==NULL)
    {
        efree(cmd);
        return NULL;
    }

    // Split by \n
    lines = split_str_by_delim(reply, "\n", &count);

    // Allocate storage for nodes
    nodes = ecalloc(count, sizeof(clusterNodeInfo*));

    for(i=0;i<count;i++) {
        nodes[i] = ecalloc(1, sizeof(clusterNodeInfo));
        if(cluster_parse_node_line(redis_sock, lines[i], nodes[i])<0) {
            efree(reply);
            for(j=0;j<i;j++) {
                free_cluster_info(nodes[j]);
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
    
    // Pull in our contiguous slot ranges
    node->slots = info->slots;
    node->slots_size = info->slots_size;
    
    // Set slave flag
    node->slave = info->slave;

    // Create and attach our socket
    node->sock = redis_sock_create(info->host, info->host_len, info->port,
        cluster->timeout, 0, NULL, 0, 1);

    // Return our node
    return node;
}

/* Helper to create a cluster node struct from each bit of info */
static redisClusterNode*
cluster_node_create_ex(redisCluster *c, const char *name,
                       const char *master_name, const char *host,
                       int host_len, unsigned short port,
                       unsigned short start_slot,
                       unsigned short end_slot)
{
    clusterNodeInfo info = {0};

    info.name = (char*)name;
    info.master_name = (char*)master_name;
    info.host = (char*)host;
    info.host_len = host_len;
    info.port = port;

    info.slots = ecalloc(1, sizeof(clusterSlotRange));
    info.slots_size = 1;

    info.slots[0].start = start_slot;
    info.slots[0].end = end_slot;

    return cluster_node_create(c, &info);
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

/* Get a RedisSock object from the host and port where we have been
 * directed from an ASK response.  We'll first see if we have
 * connected to this node already, and return that.  If not, we
 * create it and add it to our nodes.
 */
static RedisSock *cluster_get_asking_sock(redisCluster *c TSRMLS_DC) {
    redisClusterNode **ppNode;
    clusterNodeInfo pInfo = {0};
    char key[1024];
    int key_len;

    // It'll be hashed as host:port in our nodes HashTable
    key_len = snprintf(key, sizeof(key), "%s:%u", c->redir_host,
        c->redir_port);

    // See if we've already attached to it
    if(zend_hash_find(c->nodes, key, key_len+1, (void**)&ppNode)==SUCCESS)
    {
        return (*ppNode)->sock;
    }

    // We have yet to encounter this host:port so create
    pInfo.name = NULL;
    pInfo.master_name = NULL;
    pInfo.host = c->redir_host;
    pInfo.host_len = strlen(c->redir_host);
    pInfo.port = c->redir_port;
    
    // At this point, we only know one slot it might serve
    pInfo.slots = ecalloc(1, sizeof(clusterSlotRange));
    pInfo.slots_size = 1;
    pInfo.slots[0].start = c->redir_slot;
    pInfo.slots[0].end = c->redir_slot;

    // Create a redisClusterNode
    *ppNode = cluster_node_create(c, &pInfo);

    // Now add it to the nodes we have
    zend_hash_update(c->nodes, key, key_len+1, (void*)ppNode,
        sizeof(redisClusterNode*), NULL);

    // Return the RedisSock
    return (*ppNode)->sock;
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

    // Inherit the start - end slots from our master
    slave->slots = ecalloc(master->slots_size, sizeof(clusterSlotRange));
    slave->slots_size = master->slots_size;
    memcpy(slave->slots, master->slots, 
           master->slots_size * sizeof(clusterSlotRange));

    // Flag this node as a slave
    slave->slave = 1;

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
    int i, j, key_len;
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

        // Iterate each contiguous slot range, and point to the node
        for(i=0;i<node->slots_size;i++) {
           for(j=node->slots[i].start;j<=node->slots[i].end;j++) {
              cluster->master[j] = node;
           }
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
            (unsigned short)atoi(psep+1),cluster->timeout,0,NULL,0,0);

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
       if(!node[i]->slave) {
            zend_hash_update(masters, node[i]->name, CLUSTER_NAME_LEN+1,
                (void*)&node[i], sizeof(clusterNodeInfo*), NULL);
        } else {
            HashTable *ht_inner;

            // Determine if we've already got at least one slave for this
            // master node.
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

/* Helper to find if we've got a host:port mapped in our cluster nodes. */
static redisClusterNode *cluster_find_node(redisCluster *c, const char *host,
                                           unsigned short port)
{
    redisClusterNode **ret = NULL;
    int key_len;
    char key[1024];

    key_len = snprintf(key,sizeof(key),"%s:%d", host, port);

    if(zend_hash_find(c->nodes, key, key_len+1, (void**)&ret)==SUCCESS) {
        return *ret;
    }

    // Not found
    return NULL;
}

/* Parse the MOVED OR ASK redirection payload when we get such a response
 * and apply this information to our cluster.  If we encounter a parse error
 * nothing in the cluster will be modified, and -1 is returned. */
static int cluster_set_redirection(redisCluster* c, char *msg, int moved)
{
    char *host, *port;

    // Move past MOVED or ASK
    if(moved) {
        msg += MOVED_LEN;
    } else {
        msg += ASK_LEN;
    }

    // We need a slot seperator
    if(!(host = strchr(msg, ' '))) return -1;
    *host++ = '\0';

    // We need a : that seperates host from port
    if(!(port = strchr(host,':'))) return -1;
    *port++ = '\0';

    // Success, apply it
    c->redir_type = moved ? REDIR_MOVED : REDIR_ASK;
    strncpy(c->redir_host, host, sizeof(c->redir_host));
    c->redir_host_len = port - host - 1;
    c->redir_slot = (unsigned short)atoi(msg);
    c->redir_port = (unsigned short)atoi(port);

    return 0;
}

/* Once we write a command to a node in our cluster, this function will check
 * the reply type and extract information from those that will specify a length
 * bit.  If we encounter an error condition, we'll check for MOVED or ASK
 * redirection, parsing out slot host and port so the caller can take
 * appropriate action.
 *
 * In the case of a non MOVED/ASK error, we wlll set our cluster error
 * condition so GetLastError can be queried by the client.
 *
 * This function will return -1 on a critical error (e.g. parse/communication
 * error, 0 if no redirection was encountered, and 1 if the data was moved. */
static int cluster_check_response(redisCluster *c, unsigned short slot,
                                  REDIS_REPLY_TYPE *reply_type TSRMLS_DC)
{
    // Clear out any prior error state and our last line response
    CLUSTER_CLEAR_ERROR(c);
    CLUSTER_CLEAR_REPLY(c);    

    if(-1 == redis_check_eof(SLOT_SOCK(c,slot)) ||
       EOF == (*reply_type = php_stream_getc(SLOT_STREAM(c,slot)))) 
    {
        return -1;
    }
    
    // In the event of an ERROR, check if it's a MOVED/ASK error
    if(*reply_type == TYPE_ERR) {
        char inbuf[1024];
        int moved;

        // Attempt to read the error
        if(php_stream_gets(SLOT_STREAM(c,slot), inbuf, sizeof(inbuf))<0) {
            return -1;
        }

        // Check for MOVED or ASK redirection
        if((moved = IS_MOVED(inbuf)) || IS_ASK(inbuf)) {
            // Set our redirection information
            if(cluster_set_redirection(c,inbuf,moved)<0) {
                return -1;
            }

            // Data moved
            return 1;
        } else {
            // Capture the error string Redis returned
            cluster_set_err(c, inbuf+1, strlen(inbuf+1)-2);
            return 0;
        }
    }

    // Fetch the first line of our response from Redis.
    if(redis_sock_gets(SLOT_SOCK(c,slot),c->line_reply,sizeof(c->line_reply), 
                       &c->reply_len)<0)
    {
        return -1;
    }

    // For replies that will give us a numberic length, convert it
    if(*reply_type != TYPE_LINE) { 
        c->reply_len = atoi(c->line_reply);
    }

    // Clear out any previous error, and return that the data is here
    CLUSTER_CLEAR_ERROR(c);
    return 0;
}

/* Attempt to write to a cluster node.  If the node is NULL (e.g.
 * it's been umapped, we keep falling back until we run out of nodes
 * to try */
static int cluster_sock_write(redisCluster *c, unsigned short slot,
                              const char *cmd, size_t sz TSRMLS_DC)
{
    RedisSock *redis_sock;
    redisClusterNode **seed_node;

    // If we're in an ASK redirection state, attempt a connection to that
    // host and port.  Otherwise, try on the requested slot.
    if(c->redir_type != REDIR_ASK) {
        redis_sock = SLOT_SOCK(c,slot);
    } else {
        redis_sock = cluster_get_asking_sock(c);
    }

    // If the lazy_connect flag is still set, we've not actually
    // connected to this node, so do that now.
    CLUSTER_LAZY_CONNECT(redis_sock);

    // First attempt to write it to the slot that's been requested
    if(redis_sock && redis_sock->stream && 
       !redis_check_eof(redis_sock TSRMLS_CC) &&
       php_stream_write(redis_sock->stream, cmd, sz)==sz)
    {
        // We were able to write it
        return slot;
    }

    // Fall back by attempting the request against every connected node
    for(zend_hash_internal_pointer_reset(c->nodes);
        zend_hash_has_more_elements(c->nodes)==SUCCESS;
        zend_hash_move_forward(c->nodes))
    {
        zend_hash_get_current_data(c->nodes, (void**)&seed_node);

        // TODO:  Allow for failure/redirection queries to be sent
        //        to slave nodes, but for now, stick with masters.
        if((*seed_node)->slave) continue;

        CLUSTER_LAZY_CONNECT((*seed_node)->sock);

        // Attempt to write our request to this node
        if(!redis_check_eof((*seed_node)->sock TSRMLS_CC) &&
           php_stream_write((*seed_node)->sock->stream, cmd, sz)==sz)
        {
            // Just return the first slot we think this node handles
            return (*seed_node)->slots[0].start;
        }
    }

    // We were unable to write to any node in our cluster
    return -1;
}

/* Provided a redisCluster object, the slot where we thought data was and
 * the slot where data was moved, update our node mapping */
static void cluster_update_slot(redisCluster *c, short orig_slot TSRMLS_CC) {
    redisClusterNode *node;

    // Invalidate original slot
    c->master[orig_slot] = NULL;

    // Do we already have the new slot mapped
    if(c->master[c->redir_slot]) {
        // No need to do anything if it's the same node
        if(!CLUSTER_REDIR_CMP(c)) {
            return;
        }

        // Check to see if we have this new node mapped
        node = cluster_find_node(c, c->redir_host, c->redir_port);

        if(node) {
            // Just point to this slot
            c->master[c->redir_slot] = node;
        } else {
            // Create our node
            node = cluster_node_create_ex(c, NULL, NULL, c->redir_host,
                c->redir_host_len, c->redir_port, c->redir_slot,
                c->redir_slot);

            // Now point our slot at the node
            c->master[c->redir_slot] = node;
        }
    } else {
        // Check to see if the ip and port are mapped
        node = cluster_find_node(c, c->redir_host, c->redir_port);
        if(!node) {
            node = cluster_node_create_ex(c, NULL, NULL, c->redir_host,
                c->redir_host_len, c->redir_port, c->redir_slot, c->redir_slot);
        }

        // Map the slot to this node
        c->master[c->redir_slot] = node;
    }

    // Make sure we unflag this node as a slave, as Redis Cluster will only
    // ever direct us to master nodes.
    node->slave = 0;
}

/* Send a command to given slot in our cluster.  If we get a MOVED
 * or ASK error we attempt to send the command to the node as
 * directed. */
PHPAPI short cluster_send_command(redisCluster *c, short slot,
                                  const char *cmd, int cmd_len TSRMLS_DC)
{
    int resp, rslot = slot;

    // Issue commands until we find the right node or fail
    do {
        // Attempt to send the command to the slot requested
        if((slot = cluster_sock_write(c, slot, cmd, cmd_len TSRMLS_CC))==-1)
        {
            // We have no choice but to throw an exception.  We
            // can't communicate with any node at all.
            zend_throw_exception(redis_cluster_exception_ce,
                "Can't communicate with any node in the cluster",
                0 TSRMLS_CC);
            return -1;
        }

        // Check the response from the slot we ended up querying.
        resp = cluster_check_response(c, slot, &c->reply_type TSRMLS_CC);

        // If we're getting an error condition, impose a slight delay before
        // we try again (e.g. server went down, election in process).  If the
        // data has been moved, update node configuration, and if ASK has been
        // encountered, we'll just try again at that slot.
        if(resp == -1) {
            sleep(1);
        } else if(resp == 1) {
            // In case of a MOVED redirection, update our node mapping
            if(c->redir_type == REDIR_MOVED) {
                cluster_update_slot(c, rslot);
            }
            slot = c->redir_slot;
        }
    } while(resp != 0);

    // Inform the cluster where to read the rest of our response,
    // and clear out redirection flag.
    c->reply_slot = slot;
    c->redir_type = REDIR_NONE;

    // Success, return the slot where data exists.
    return 0;
}

/* RedisCluster response handlers.  These methods all have the same prototype
 * and set the proper return value for the calling cluster method.  These
 * methods will never be called in the case of a communication error when
 * we try to send the request to the Cluster *or* if a non MOVED or ASK
 * error is encountered, in which case our response processing macro will
 * short circuit and RETURN_FALSE, as the error will have already been
 * consumed.
 */

/* Unmodified BULK response handler */
PHPAPI void cluster_bulk_resp(INTERNAL_FUNCTION_PARAMETERS, redisCluster *c)
{
    char *resp;

    // Make sure we can read the response
    if(c->reply_type != TYPE_BULK ||
       (resp = redis_sock_read_bulk_reply(SLOT_SOCK(c,c->reply_slot),
                                          c->reply_len TSRMLS_CC))==NULL)
    {
        RETURN_FALSE;
    }

    // Return the string if we can unserialize it
    if(redis_unserialize(c->flags, resp, c->reply_len, &return_value)==0) {
        RETURN_STRINGL(resp,c->reply_len,0);
    } else {
        efree(resp);
    }
}

/* Bulk response where we expect a double */
PHPAPI void cluster_dbl_resp(INTERNAL_FUNCTION_PARAMETERS, redisCluster *c)
{
    char *resp;
    double dbl;

    // Make sure we can read the response
    if(c->reply_type != TYPE_BULK ||
       (resp = redis_sock_read_bulk_reply(SLOT_SOCK(c,c->reply_slot),
                                          c->reply_len TSRMLS_CC))==NULL)
    {
        RETURN_FALSE;
    }

    // Convert to double, free response
    dbl = atof(resp);
    efree(resp);

    RETURN_DOUBLE(dbl);
}

/* A boolean response.  If we get here, we've consumed the '+' reply
 * type and will now just verify we can read the OK */
PHPAPI void cluster_bool_resp(INTERNAL_FUNCTION_PARAMETERS, redisCluster *c)
{
    // Check that we have +OK
    if(c->reply_type != TYPE_LINE || c->reply_len != 2 || 
       c->line_reply[0] != 'O' || c->line_reply[1] != 'K') 
    {
        RETURN_FALSE;
    }

    RETURN_TRUE;
}

/* 1 or 0 response, for things like SETNX */
PHPAPI void cluster_1_resp(INTERNAL_FUNCTION_PARAMETERS, redisCluster *c)
{
    // Validate our reply type, and check for a zero
    if(c->reply_type != TYPE_INT || c->reply_len == 0) {
        RETURN_FALSE;
    }

    RETURN_TRUE;
}

/* Generic integer response */
PHPAPI void cluster_long_resp(INTERNAL_FUNCTION_PARAMETERS, redisCluster *c)
{
    if(c->reply_type != TYPE_INT) {
        RETURN_FALSE;
    }
    RETURN_TRUE;
}

/* TYPE response handler */
PHPAPI void cluster_type_resp(INTERNAL_FUNCTION_PARAMETERS, redisCluster *c)
{
    // Make sure we got the right kind of response
    if(c->reply_type != TYPE_LINE) {
        RETURN_FALSE;
    }

    // Switch on the type
    if(strncmp(c->line_reply, "+string", 7)==0) {
        RETURN_LONG(REDIS_STRING);
    } else if(strncmp(c->line_reply, "+set", 4)==0) {
        RETURN_LONG(REDIS_SET);
    } else if(strncmp(c->line_reply, "+list", 5)==0) {
        RETURN_LONG(REDIS_LIST);
    } else if(strncmp(c->line_reply, "+hash", 5)==0) {
        RETURN_LONG(REDIS_HASH);
    } else {
        RETURN_LONG(REDIS_NOT_FOUND);
    }
}

/* Generic MULTI BULK response processor */
PHPAPI void cluster_gen_mbulk_resp(INTERNAL_FUNCTION_PARAMETERS, 
                                   redisCluster *c, mbulk_cb cb)
{
    zval *z_result;

    // Verify our reply type byte is correct
    if(c->reply_type != TYPE_MULTIBULK) {
        RETURN_FALSE;
    }

    // Allocate array
    MAKE_STD_ZVAL(z_result);
    array_init(z_result);

    // Call our specified callback
    if(cb(SLOT_SOCK(c,c->reply_slot), z_result, c->reply_len TSRMLS_CC)
                     ==FAILURE)
    {
        zval_dtor(z_result);
        FREE_ZVAL(z_result);
        RETURN_FALSE;
    }
    
    // Success, make this array our return value
    *return_value = *z_result;
    efree(z_result);
}

/* Raw MULTI BULK reply */
PHPAPI void 
cluster_mbulk_resp_raw(INTERNAL_FUNCTION_PARAMETERS, redisCluster *c) {
    cluster_gen_mbulk_resp(INTERNAL_FUNCTION_PARAM_PASSTHRU,
        c, mbulk_resp_loop_raw);
}

/* Unserialize all the things */
PHPAPI void
cluster_mbulk_resp(INTERNAL_FUNCTION_PARAMETERS, redisCluster *c) {
    cluster_gen_mbulk_resp(INTERNAL_FUNCTION_PARAM_PASSTHRU,
        c, mbulk_resp_loop);
}

/* 
 * Various MULTI BULK reply callback functions 
 */

/* MULTI BULK response where we don't touch the values (e.g. KEYS) */
int mbulk_resp_loop_raw(RedisSock *redis_sock, zval *z_result, 
                        size_t count TSRMLS_DC) 
{
    char *line;
    int line_len;

    // Iterate over the number we have
    while(count--) {
        // Read the line, which should never come back null
        line = redis_sock_read(redis_sock, &line_len TSRMLS_CC);
        if(line == NULL) return FAILURE;

        // Add to our result array
        add_next_index_stringl(z_result, line, line_len, 0);
    }

    // Success!
    return SUCCESS;
}

/* MULTI BULK response where we unserialize everything */
int mbulk_resp_loop(RedisSock *redis_sock, zval *z_result, size_t count 
                    TSRMLS_CC)
{
    char *line;
    int line_len;
    zval *z;

    // Iterate over the lines we have to process
    while(count--) {
        // Read the line
        line = redis_sock_read(redis_sock, &line_len TSRMLS_CC);
        if(line == NULL) return FAILURE;

        if(redis_unserialize(redis_sock, line, line_len, &z TSRMLS_CC)==1) {
            add_next_index_zval(z_result, z);
            efree(line);
        } else {
            add_next_index_stringl(z_result, line, line_len, 0);
        }
    }

    return SUCCESS;
}

/* vim: set tabstop=4 softtabstops=4 noexpandtab shiftwidth=4: */
