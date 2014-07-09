#include "php_redis.h"
#include "common.h"
#include "library.h"
#include "redis_commands.h"
#include "cluster_library.h"
#include "crc16.h"
#include <zend_exceptions.h>

extern zend_class_entry *redis_cluster_exception_ce;

/* Some debug methods that will go away when we're through with them */

static void cluster_dump_nodes(redisCluster *c) {
    redisClusterNode **pp, *p;
    
    for(zend_hash_internal_pointer_reset(c->nodes);
        zend_hash_has_more_elements(c->nodes)==SUCCESS;
        zend_hash_move_forward(c->nodes))
    {
        zend_hash_get_current_data(c->nodes, (void**)&pp);
        p = *pp;

        const char *slave = (p->slave) ? "slave" : "master";
        php_printf("%d %s %d %d", p->sock->port, slave,p->sock->prefix_len,
            p->slot);
        
        php_printf("\n");
    }
}

/* Direct handling of variant replies, in a hiredis like way.  These methods
 * are used for non userland facing commands, as well as passed through from 
 * them when the reply is just variant (e.g. eval) */

/* Debug function to dump a clusterReply structure recursively */
static void dump_reply(clusterReply *reply, int indent) { 
    smart_str buf = {0};
    int i;

    switch(reply->type) {
        case TYPE_ERR:
            smart_str_appendl(&buf, "(error) ", sizeof("(error) ")-1);
            smart_str_appendl(&buf, reply->str, reply->len);
            break;
        case TYPE_LINE:
            smart_str_appendl(&buf, reply->str, reply->len);
            break;
        case TYPE_INT:
            smart_str_appendl(&buf, "(integer) ", sizeof("(integer) ")-1);
            smart_str_append_long(&buf, reply->integer);
            break;
        case TYPE_BULK:
            smart_str_appendl(&buf,"\"", 1);
            smart_str_appendl(&buf, reply->str, reply->len);
            smart_str_appendl(&buf, "\"", 1);
            break;
        case TYPE_MULTIBULK:
            if(reply->elements == (size_t)-1) {
                smart_str_appendl(&buf, "(nil)", sizeof("(nil)")-1);
            } else {
                for(i=0;i<reply->elements;i++) {
                    dump_reply(reply->element[i], indent+2);
                }
            }
            break;
        default:
            break;
    }

    if(buf.len > 0) {
        for(i=0;i<indent;i++) {
            php_printf(" ");
        }

        smart_str_0(&buf);
        php_printf("%s", buf.c);
        php_printf("\n");

        efree(buf.c);
    }
}

/* Recursively free our reply object.  If free_data is non-zero we'll also free
 * the payload data (strings) themselves.  If not, we just free the structs */
void cluster_free_reply(clusterReply *reply, int free_data) {
    int i;

    switch(reply->type) {
        case TYPE_ERR:
        case TYPE_LINE:
        case TYPE_BULK:
            if(free_data) 
                efree(reply->str);
            break;
        case TYPE_MULTIBULK:
            for(i=0;i<reply->elements && reply->element[i]; i++) {
                cluster_free_reply(reply->element[i], free_data);
            }
            efree(reply->element);
            break;
        default:
            break;
    }
    efree(reply);    
}

static void
cluster_multibulk_resp_recursive(RedisSock *sock, size_t elements, 
                                 clusterReply **element, int *err TSRMLS_DC)
{
    size_t idx = 0;
    clusterReply *r;
    int len;
    char buf[1024];

    while(elements-- > 0) {
        element[idx] = ecalloc(1, sizeof(clusterReply));
        r = element[idx];
        
        // Bomb out, flag error condition on a communication failure
        if(redis_read_reply_type(sock, &r->type, &len TSRMLS_CC)<0) {
            *err = 1; 
            return;
        }
       
        r->len = len;

        switch(r->type) {
            case TYPE_ERR:
            case TYPE_LINE:
                if(redis_sock_gets(sock,buf,sizeof(buf),&r->len TSRMLS_CC)<0) {
                    *err = 1;
                    return;
                }
                r->str = estrndup(buf,r->len);
                break;
            case TYPE_INT:
                r->integer = len;
                break;
            case TYPE_BULK:
                r->str = redis_sock_read_bulk_reply(sock,r->len TSRMLS_CC);
                if(!r->str) {
                    *err = 1;
                    return;
                }
                break;
            case TYPE_MULTIBULK:
                r->element = ecalloc(r->len,r->len*sizeof(clusterReply*));
                r->elements = r->len;
                cluster_multibulk_resp_recursive(sock, r->elements, r->element, 
                    err TSRMLS_CC);
                if(*err) return;
                break;
            default:
                *err = 1;
                return;
        }

        idx++;
    }
}

/* Read the response from a cluster */
clusterReply *cluster_read_resp(redisCluster *c TSRMLS_DC) {
    return cluster_read_sock_resp(SLOT_SOCK(c,c->reply_slot), c->reply_type,
        c->reply_len TSRMLS_CC);
}

/* Read any sort of response from the socket, having already issued the 
 * command and consumed the reply type and meta info (length) */
clusterReply*
cluster_read_sock_resp(RedisSock *redis_sock, REDIS_REPLY_TYPE type, 
                       size_t len TSRMLS_DC) 
{
    clusterReply *r = ecalloc(1, sizeof(clusterReply));
    r->type = type;

    // Error flag in case we go recursive
    int err = 0;

    switch(r->type) {
        case TYPE_INT:
            r->integer = len;
            break;
        case TYPE_BULK:
            r->len = len;
            r->str = redis_sock_read_bulk_reply(redis_sock, len TSRMLS_CC);
            if(r->len != -1 && !r->str) {
                cluster_free_reply(r, 1);
                return NULL;
            }
            break;
        case TYPE_MULTIBULK:
            r->elements = len;
            if(len != (size_t)-1) {
                r->element = ecalloc(len, sizeof(clusterReply*)*len);
                cluster_multibulk_resp_recursive(redis_sock, len, r->element, 
                    &err TSRMLS_CC);
            }
            break;
        default:
            cluster_free_reply(r,1);
            return NULL;
    }

    // Free/return null on communication error
    if(err) {
        cluster_free_reply(r,1);
        return NULL;
    }

    // Success, return the reply
    return r;
}

/* Cluster key distribution helpers.  For a small handlful of commands, we want 
 * to distribute them across 1-N nodes.  These methods provide simple containers 
 * for the purposes of splitting keys/values in this way */

/* Free cluster distribution list inside a HashTable */
static void cluster_dist_free_ht(void *p) {
    clusterDistList *dl = *(clusterDistList**)p;
    int i;

    for(i=0; i < dl->len; i++) {
        if(dl->entry[i].key_free) 
            efree(dl->entry[i].key);
        if(dl->entry[i].val_free) 
            efree(dl->entry[i].val);
    }

    efree(dl->entry);
    efree(dl);
}

/* Spin up a HashTable that will contain distribution lists */
HashTable *cluster_dist_create() {
    HashTable *ret;
    
    ALLOC_HASHTABLE(ret);
    zend_hash_init(ret, 0, NULL, cluster_dist_free_ht, 0);

    return ret;
}

/* Free distribution list */
void cluster_dist_free(HashTable *ht) {
    zend_hash_destroy(ht);
    efree(ht);
}

/* Create a clusterDistList object */
static clusterDistList *cluster_dl_create() {
    clusterDistList *dl;
   
    dl        = emalloc(sizeof(clusterDistList));
    dl->entry = emalloc(CLUSTER_KEYDIST_ALLOC * sizeof(clusterKeyVal));
    dl->size  = CLUSTER_KEYDIST_ALLOC;
    dl->len   = 0;

    return dl;
}

/* Add a key to a dist list, returning the keval entry */
static clusterKeyVal *cluster_dl_add_key(clusterDistList *dl, char *key, 
                                         int key_len, int key_free) 
{
    // Reallocate if required
    if(dl->len==dl->size) {
        dl->entry = erealloc(dl->entry, sizeof(clusterKeyVal) * dl->size * 2);
        dl->size *= 2;
    }

    // Set key info
    dl->entry[dl->len].key = key;
    dl->entry[dl->len].key_len = key_len;
    dl->entry[dl->len].key_free = key_free;
    
    // NULL out any values
    dl->entry[dl->len].val = NULL;
    dl->entry[dl->len].val_len = 0;
    dl->entry[dl->len].val_free = 0;

    return &(dl->entry[dl->len++]);
}

/* Add a key, returning a pointer to the entry where passed for easy adding
 * of values to match this key */
int cluster_dist_add_key(redisCluster *c, HashTable *ht, char *key, 
                          int key_len, clusterKeyVal **kv)
{
    int key_free;
    short slot;
    clusterDistList **ppdl, *dl;
    clusterKeyVal *retptr;

    // Prefix our key and hash it
    key_free = redis_key_prefix(c->flags, &key, &key_len);
    slot     = cluster_hash_key(key, key_len);

    // We can't do this if we don't fully understand the keyspace
    if(c->master[slot] == NULL) {
        if(key_free) efree(key);
        return FAILURE;
    }

    // Look for this slot
    if(zend_hash_index_find(ht, (ulong)slot, (void**)&ppdl)==FAILURE) {
        dl = cluster_dl_create();
        zend_hash_index_update(ht, (ulong)slot, (void**)&dl,
            sizeof(clusterDistList*), NULL);
    } else {
        dl = *ppdl;
    }

    // Now actually add this key
    retptr = cluster_dl_add_key(dl, key, key_len, key_free);

    // Push our return pointer if requested
    if(kv) *kv = retptr;

    return SUCCESS;
}

/* Provided a clusterKeyVal, add a value */
void cluster_dist_add_val(redisCluster *c, clusterKeyVal *kv, zval *z_val 
                         TSRMLS_CC)
{
    char *val;
    int val_len, val_free;

    // Serialize our value
    val_free = redis_serialize(c->flags, z_val, &val, &val_len TSRMLS_CC);

    // Attach it to the provied keyval entry
    kv->val = val;
    kv->val_len = val_len;
    kv->val_free = val_free;
}

/* Free allocated memory for a clusterMultiCmd */
void cluster_multi_free(clusterMultiCmd *mc) {
    efree(mc->cmd.c);
    efree(mc->args.c);
}

/* Add an argument to a clusterMultiCmd */
void cluster_multi_add(clusterMultiCmd *mc, char *data, int data_len) {
    mc->argc++;
    redis_cmd_append_sstr(&(mc->args), data, data_len);
}

/* Finalize a clusterMutliCmd by constructing the whole thing */
void cluster_multi_fini(clusterMultiCmd *mc) {
    mc->cmd.len = 0;
    redis_cmd_init_sstr(&(mc->cmd), mc->argc, mc->kw, mc->kw_len);
    smart_str_appendl(&(mc->cmd), mc->args.c, mc->args.len);
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

/* Destructor for slaves */
static void ht_free_slave(void *data) {
    if(*(redisClusterNode**)data) {
        cluster_free_node(*(redisClusterNode**)data);
    }
}

/* Get the hash slot for a given key */
unsigned short cluster_hash_key(const char *key, int len) {
    int s, e;

    // Find first occurrence of {, if any
    for(s=0;s<len;s++) {
        if(key[s]=='{') break;
    }
    
    // There is no '{', hash everything
    if(s == len) return crc16(key, len) & REDIS_CLUSTER_MOD;

    // Found it, look for a tailing '}'
    for(e=s+1;e<len;e++) {
        if(key[e]=='}') break;
    }

    // Hash the whole key if we don't find a tailing } or if {} is empty
    if(e == len || e == s+1) return crc16(key, len) & REDIS_CLUSTER_MOD;

    // Hash just the bit betweeen { and }
    return crc16((char*)key+s+1,e-s-1) & REDIS_CLUSTER_MOD;
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

/*static int
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
        // Null terminate at the : character
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
*/

/* Execute a CLUSTER SLOTS command against the seed socket, and return the
 * reply or NULL on failure. */
clusterReply* cluster_get_slots(RedisSock *redis_sock TSRMLS_DC)
{
    clusterReply *r;
    REDIS_REPLY_TYPE type;
    int len;

    // Send the command to the socket and consume reply type
    if(redis_sock_write(redis_sock, RESP_CLUSTER_SLOTS_CMD, 
                        sizeof(RESP_CLUSTER_SLOTS_CMD)-1 TSRMLS_CC)<0 ||
                        redis_read_reply_type(redis_sock, &type, &len)<0)
    {
        return NULL;
    }

    // Consume the rest of our response
    if((r = cluster_read_sock_resp(redis_sock, type, len))==NULL ||
       r->type != TYPE_MULTIBULK || r->elements < 3)
    {
        if(r) cluster_free_reply(r, 1);
        return NULL;
    }

    // Return our reply
    return r;
}

/* Create a cluster node */
static redisClusterNode*
cluster_node_create(redisCluster *c, char *host, size_t host_len, 
                    unsigned short port, unsigned short slot, short slave)
{
    redisClusterNode *node = emalloc(sizeof(redisClusterNode));

    // It lives in at least this slot, flag slave status
    node->slot   = slot;
    node->slave  = slave;
    node->slaves = NULL;

    // Attach socket
    node->sock = redis_sock_create(host, host_len, port, c->timeout, 
        0, NULL, 0, 1); 

    return node;
}

/* Attach a slave to a master */
PHPAPI int 
cluster_node_add_slave(redisClusterNode *master, redisClusterNode *slave)
{
    // Allocate our slaves hash table if we haven't yet
    if(!master->slaves) {
        ALLOC_HASHTABLE(master->slaves);
        zend_hash_init(master->slaves, 0, NULL, ht_free_slave, 0);
    }

    return zend_hash_next_index_insert(master->slaves, (void*)&slave,
        sizeof(redisClusterNode*), NULL)!=SUCCESS;
}

/* Sanity check/validation for CLUSTER SLOTS command */
#define VALIDATE_SLOTS_OUTER(r) \
    (r->elements>=3 && r2->element[0]->type == TYPE_INT && \
     r->element[1]->type==TYPE_INT)
#define VALIDATE_SLOTS_INNER(r) \
    (r->type == TYPE_MULTIBULK && r->elements>=2 && \
     r->element[0]->type == TYPE_BULK && r->element[1]->type==TYPE_INT)

/* Use the output of CLUSTER SLOTS to map our nodes */
static int cluster_map_slots(redisCluster *c, clusterReply *r) {
    int i,j, hlen, klen;
    short low, high;
    clusterReply *r2, *r3;
    redisClusterNode **ppnode, *master, *slave;
    unsigned short port;
    char *host, key[1024];

    for(i=0;i<r->elements;i++) {
        // Inner response
        r2 = r->element[i];
        
        // Validate outer and master slot
        if(!VALIDATE_SLOTS_OUTER(r2) || !VALIDATE_SLOTS_INNER(r2->element[2])) {
            return -1;
        }
        
        // Master
        r3 = r2->element[2];

        // Grab our slot range, as well as master host/port
        low  = (unsigned short)r2->element[0]->integer;
        high = (unsigned short)r2->element[1]->integer;
        host = r3->element[0]->str;
        hlen = r3->element[0]->len;
        port = (unsigned short)r3->element[1]->integer;

        // If the node is new, create and add to nodes.  Otherwise use it.
        klen = snprintf(key,sizeof(key),"%s:%ld",host,port);
        if(zend_hash_find(c->nodes,key,klen+1,(void**)&ppnode)==FAILURE) {
            master = cluster_node_create(c, host, hlen, port, low, 0);
            zend_hash_update(c->nodes, key, klen+1, (void*)&master,
                sizeof(redisClusterNode*), NULL);
        } else {
            master = *ppnode;
        }
            
        // Attach slaves
        for(j=3;j<r2->elements;j++) {
            r3 = r2->element[j];
            if(!VALIDATE_SLOTS_INNER(r3)) {
                return -1;
            }
           
            // Attach this node to our slave 
            slave = cluster_node_create(c, r3->element[0]->str, 
                (int)r3->element[0]->len, 
                (unsigned short)r3->element[1]->integer, low, 1);
            cluster_node_add_slave(master, slave);
        }
        
        // Attach this node to each slot in the range
        for(j=low;j<=high;j++) {
            c->master[j]=master;
        }
    }

    // Success
    return 0;
}

/* Free a redisClusterNode structure */
PHPAPI void cluster_free_node(redisClusterNode *node) {
    if(node->slaves) {
        zend_hash_destroy(node->slaves);
        efree(node->slaves);
    }
    redis_free_socket(node->sock);
    efree(node);
}

/* When we're in an ASK redirection state, Redis Cluster wants us to send
 * the command only after starting our query with ASKING, or it'll just
 * bounce us back and forth until the slots have migrated */
static int cluster_send_asking(RedisSock *redis_sock TSRMLS_DC) 
{
    REDIS_REPLY_TYPE reply_type;
    char buf[255];

    // Make sure we can send the request
    if(redis_check_eof(redis_sock TSRMLS_DC) ||
       php_stream_write(redis_sock->stream, RESP_ASKING_CMD, 
                        sizeof(RESP_ASKING_CMD)-1) != sizeof(RESP_ASKING_CMD)-1)
    {
        return -1;
    }

    // Read our reply type
    if((redis_check_eof(redis_sock TSRMLS_CC) == - 1) ||
       (reply_type = php_stream_getc(redis_sock->stream TSRMLS_DC) 
                                     != TYPE_LINE))
    {
        return -1;
    }

    // Consume the rest of our response
    if(php_stream_gets(redis_sock->stream, buf, sizeof(buf)<0)) {
        return -1;
    }

    // Success
    return 0;
}

/* Get a RedisSock object from the host and port where we have been
 * directed from an ASK response.  We'll first see if we have
 * connected to this node already, and return that.  If not, we
 * create it and add it to our nodes.
 */
static RedisSock *cluster_get_asking_sock(redisCluster *c TSRMLS_DC) {
    redisClusterNode **ppNode;
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

    // Create a redisClusterNode
    *ppNode = cluster_node_create(c, c->redir_host, c->redir_host_len,
        c->redir_port, c->redir_slot, 0);

    // Now add it to the nodes we have
    zend_hash_update(c->nodes, key, key_len+1, (void*)ppNode,
        sizeof(redisClusterNode*), NULL);

    // Return the RedisSock
    return (*ppNode)->sock;
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
cluster_map_keyspace(redisCluster *c TSRMLS_DC) {
    RedisSock **seed;
    clusterReply *slots;
    int mapped=0;

    // Iterate over seeds until we can get slots
    for(zend_hash_internal_pointer_reset(c->seeds);
        !mapped && zend_hash_has_more_elements(c->seeds) == SUCCESS;
        zend_hash_move_forward(c->seeds))
    {
        // Grab the redis_sock for this seed
        zend_hash_get_current_data(c->seeds, (void**)&seed);

        // Attempt to connect to this seed node
        if(redis_sock_connect(*seed TSRMLS_CC)!=0) {
            continue;
        }

        // Parse out cluster nodes.  Flag mapped if we are valid
        slots = cluster_get_slots(*seed TSRMLS_CC);
        if(slots) mapped = !cluster_map_slots(c, slots);

        // Bin anything mapped, if we failed somewhere
        if(!mapped && slots) {
            memset(c->master, 0, sizeof(redisClusterNode*)*REDIS_CLUSTER_SLOTS);
        }
    }

    // Clean up slots reply if we got one
    if(slots) cluster_free_reply(slots, 1);

    // Throw an exception if we couldn't map
    if(!mapped) {
        zend_throw_exception(redis_cluster_exception_ce,
            "Couldn't map cluster keyspace using any provided seed", 0
            TSRMLS_CC);
        return -1;
    }

    return 0;
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
    size_t sz;

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
            cluster_set_err(c, inbuf, strlen(inbuf)-1);
            return 0;
        }
    }

    // Fetch the first line of our response from Redis.
    if(redis_sock_gets(SLOT_SOCK(c,slot),c->line_reply,sizeof(c->line_reply), 
                       &sz)<0)
    {
        return -1;
    }

    // For replies that will give us a numberic length, convert it
    if(*reply_type != TYPE_LINE) { 
        c->reply_len = atoi(c->line_reply);
    } else {
        c->reply_len = (long long)sz;
    }

    // Clear out any previous error, and return that the data is here
    CLUSTER_CLEAR_ERROR(c);
    return 0;
}

/* Disconnect from each node we're connected to */
PHPAPI void cluster_disconnect(redisCluster *c TSRMLS_DC) {
    redisClusterNode **node;

    for(zend_hash_internal_pointer_reset(c->nodes);
        zend_hash_get_current_data(c->nodes, (void**)&node)==SUCCESS;
        zend_hash_move_forward(c->nodes))
    {
        redis_sock_disconnect((*node)->sock TSRMLS_CC);
        (*node)->sock->lazy_connect = 1;
    }
}

/* Attempt to write to a cluster node.  If the node is NULL (e.g. it's been 
 * umapped, we keep falling back until we run out of nodes to try */
static int cluster_sock_write(redisCluster *c, unsigned short slot,
                              const char *cmd, size_t sz, int
                              direct TSRMLS_DC)
{
    RedisSock *redis_sock;
    redisClusterNode **seed_node;

    // If we're not in ASK redirection, use the slot requested, otherwise
    // send our ASKING command and use the asking slot.
    if(c->redir_type != REDIR_ASK) {
        redis_sock = SLOT_SOCK(c,slot);
    } else {
        redis_sock = cluster_get_asking_sock(c);
    
        // Redis Cluster wants this command preceded by the "ASKING" command
        if(cluster_send_asking(redis_sock TSRMLS_CC)<0) {
            return -1;
        }
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

    // Don't fall back if direct communication with this slot is required.
    if(direct) return -1;

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
            return (*seed_node)->slot;
        }
    }

    // We were unable to write to any node in our cluster
    return -1;
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

/* Provided a redisCluster object, the slot where we thought data was and
 * the slot where data was moved, update our node mapping */
static void cluster_update_slot(redisCluster *c TSRMLS_CC) {
    redisClusterNode *node;

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
            node = cluster_node_create(c, c->redir_host, c->redir_host_len,
                c->redir_port, c->redir_slot, 0);

            // Now point our slot at the node
            c->master[c->redir_slot] = node;
        }
    } else {
        // Check to see if the ip and port are mapped
        node = cluster_find_node(c, c->redir_host, c->redir_port);
        if(!node) {
            node = cluster_node_create(c, c->redir_host, c->redir_host_len,
                c->redir_port, c->redir_slot, 0);
        }

        // Map the slot to this node
        c->master[c->redir_slot] = node;
    }

    // Update slot inside of node, so it can be found for command sending
    node->slot = c->redir_slot;

    // Make sure we unflag this node as a slave, as Redis Cluster will only
    // ever direct us to master nodes.
    node->slave = 0;
}

/* Send EXEC to a specific slot */
PHPAPI int cluster_send_exec(redisCluster *c, short slot TSRMLS_DC) {
    // We have to be able to write this to the slot requested
    if(cluster_sock_write(c, slot, RESP_EXEC_CMD, sizeof(RESP_EXEC_CMD)-1, 1
                          TSRMLS_CC)==-1)
    {
        return -1;
    }

    // We have to get a proper response from the slot to continue
    if(cluster_check_response(c, slot, &c->reply_type TSRMLS_CC)!=0 ||
       c->reply_type != TYPE_MULTIBULK)
    {
        return -1;
    }

    // Return the number of multi-bulk replies
    return c->reply_len;
}

/* Send DISCARD to a specific slot */
PHPAPI int cluster_send_discard(redisCluster *c, short slot TSRMLS_DC) {
    if(cluster_sock_write(c, slot, RESP_DISCARD_CMD, sizeof(RESP_DISCARD_CMD)-1,
                          1 TSRMLS_CC)==-1)
    {
        return -1;
    }

    if(cluster_check_response(c, slot, &c->reply_type TSRMLS_CC)!=0 ||
       c->reply_type != TYPE_LINE)
    {
        return -1;
    }

    return 0;
}


/* Abort any transaction in process, by sending DISCARD to any nodes that
 * have active transactions in progress.  If we can't send DISCARD, we need
 * to disconnect as it would leave us in an undefined state. */
PHPAPI int cluster_abort_exec(redisCluster *c TSRMLS_DC) {
    clusterFoldItem *fi = c->multi_head;
    
    // Loop through our fold items
    while(fi) {
        if(SLOT_SOCK(c,fi->slot)->mode == MULTI) {
            if(cluster_send_discard(c, fi->slot TSRMLS_CC)<0) {
                cluster_disconnect(c TSRMLS_CC);
                return -1;
            }
            SLOT_SOCK(c,fi->slot)->mode = ATOMIC;
            SLOT_SOCK(c,fi->slot)->watching = 0;
        }
        fi = fi->next;
    }
   
    // Update our overall cluster state
    c->flags->mode = ATOMIC;

    // Success
    return 0;
}

/* Send MULTI to a given slot and consume the response.  If we can't send the
 * command OR we get an error in our response, we have to fail. */
static int cluster_send_multi(redisCluster *c, short slot TSRMLS_DC) {
    // We have to be able to communicate with the node we want
    if(cluster_sock_write(c, slot, RESP_MULTI_CMD, sizeof(RESP_MULTI_CMD)-1, 1
                          TSRMLS_CC)==-1)
    {
        return -1;
    }

    // We have to get a proper response
    if(cluster_check_response(c, slot, &c->reply_type TSRMLS_CC)!=0 ||
       c->reply_type != TYPE_LINE)
    {
        return -1;
    }

    // Success
    return 0;
}

/* Iterate through our slots, looking for the host/port in question.  This 
 * should perform well enough as in almost all situations, a few or a few
 * dozen servers will map all the slots */
PHPAPI short cluster_find_slot(redisCluster *c, const char *host,
                               unsigned short port)
{
    int i;

    for(i=0;i<REDIS_CLUSTER_SLOTS;i++) {
        if(c->master[i] && c->master[i]->sock && 
           c->master[i]->sock->port == port &&
           !strcasecmp(c->master[i]->sock->host, host))
        {
            return i;
        }
    }

    // We didn't find it
    return -1;
}

/* Send a command to a specific slot */
PHPAPI int cluster_send_slot(redisCluster *c, short slot, char *cmd, 
                             int cmd_len, REDIS_REPLY_TYPE rtype TSRMLS_DC)
{
    // Try only this node
    if(cluster_sock_write(c, slot, cmd, cmd_len, 1 TSRMLS_CC)==-1) {
        return -1;
    }

    // Check our response and verify the type unless passed in as TYPE_EOF
    if(cluster_check_response(c, slot, &c->reply_type TSRMLS_CC)!=0 ||
       (rtype != TYPE_EOF && rtype != c->reply_type))
    {
        return -1;
    }

    return 0;
}

/* Send a command to given slot in our cluster.  If we get a MOVED or ASK error 
 * we attempt to send the command to the node as directed. */
PHPAPI short cluster_send_command(redisCluster *c, short slot, const char *cmd, 
                                  int cmd_len TSRMLS_DC)
{
    int resp;

    // Issue commands until we find the right node or fail
    do {
        // Send MULTI to the node if we haven't yet.
        if(c->flags->mode == MULTI && SLOT_SOCK(c,slot)->mode != MULTI) {
            // We have to fail if we can't send MULTI to the node
            if(cluster_send_multi(c, slot TSRMLS_CC)==-1) {
                zend_throw_exception(redis_cluster_exception_ce,
                    "Unable to enter MULTI mode on required slot",
                    0 TSRMLS_CC);
                return -1;
            }

            // This node is now inside a transaction
            SLOT_SOCK(c,slot)->mode = MULTI;
        }

        // Attempt to send the command to the slot requested
        if((slot = cluster_sock_write(c, slot, cmd, cmd_len, 0 TSRMLS_CC))==-1)
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
            // If we get a MOVED response inside of a transaction, we have to
            // abort, because the transaction would be invalid.
            if(c->flags->mode == MULTI) {
                zend_throw_exception(redis_cluster_exception_ce,
                    "Can't process MULTI sequence when cluster is resharding",
                    0 TSRMLS_CC);
                return -1;
            }
            
            // In case of a MOVED redirection, update our node mapping
            if(c->redir_type == REDIR_MOVED) {
                cluster_update_slot(c);
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

/* RAW bulk response handler */
PHPAPI void cluster_bulk_raw_resp(INTERNAL_FUNCTION_PARAMETERS, 
                                  redisCluster *c, void *ctx)
{
    char *resp;

    // Make sure we can read the response
    if(c->reply_type != TYPE_BULK ||
       (resp = redis_sock_read_bulk_reply(SLOT_SOCK(c,c->reply_slot),
                                          c->reply_len TSRMLS_CC))==NULL)
    {
        if(c->flags->mode != MULTI) {
            RETURN_FALSE;
        } else {
            add_next_index_bool(c->multi_resp, 0);
        }
    }

    // Return our response raw
    CLUSTER_RETURN_STRING(c, resp, c->reply_len);
}

/* BULK response handler */
PHPAPI void cluster_bulk_resp(INTERNAL_FUNCTION_PARAMETERS, redisCluster *c,
                              void *ctx)
{
    char *resp;
    zval *z_ret;

    // Make sure we can read the response
    if(c->reply_type != TYPE_BULK ||
       (resp = redis_sock_read_bulk_reply(SLOT_SOCK(c,c->reply_slot),
                                          c->reply_len TSRMLS_CC))==NULL)
    {
        CLUSTER_RETURN_FALSE(c);
    }

    // Return the string if we can unserialize it
    if(redis_unserialize(c->flags, resp, c->reply_len, &z_ret)==0) {
        CLUSTER_RETURN_STRING(c, resp, c->reply_len);
    } else {
        if(CLUSTER_IS_ATOMIC(c)) {
            return_value = z_ret;
        } else {
            add_next_index_zval(c->multi_resp, z_ret);
        }
        efree(resp);
    }
}

/* Bulk response where we expect a double */
PHPAPI void cluster_dbl_resp(INTERNAL_FUNCTION_PARAMETERS, redisCluster *c,
                             void *ctx)
{
    char *resp;
    double dbl;

    // Make sure we can read the response
    if(c->reply_type != TYPE_BULK ||
       (resp = redis_sock_read_bulk_reply(SLOT_SOCK(c,c->reply_slot),
                                          c->reply_len TSRMLS_CC))==NULL)
    {
        CLUSTER_RETURN_FALSE(c);
    }

    // Convert to double, free response
    dbl = atof(resp);
    efree(resp);

    CLUSTER_RETURN_DOUBLE(c, dbl);
}

/* A boolean response.  If we get here, we've consumed the '+' reply
 * type and will now just verify we can read the OK */
PHPAPI void cluster_bool_resp(INTERNAL_FUNCTION_PARAMETERS, redisCluster *c,
                              void *ctx)
{
    // Check that we have +OK
    if(c->reply_type != TYPE_LINE || c->reply_len != 2 || 
       c->line_reply[0] != 'O' || c->line_reply[1] != 'K') 
    {
        CLUSTER_RETURN_FALSE(c);
    }

    CLUSTER_RETURN_BOOL(c, 1);
}

/* 1 or 0 response, for things like SETNX */
PHPAPI void cluster_1_resp(INTERNAL_FUNCTION_PARAMETERS, redisCluster *c,
                           void *ctx)
{
    // Validate our reply type, and check for a zero
    if(c->reply_type != TYPE_INT || c->reply_len == 0) {
        CLUSTER_RETURN_FALSE(c);
    }

    CLUSTER_RETURN_BOOL(c, 1);
}

/* Generic integer response */
PHPAPI void cluster_long_resp(INTERNAL_FUNCTION_PARAMETERS, redisCluster *c,
                              void *ctx)
{
    if(c->reply_type != TYPE_INT) {
        CLUSTER_RETURN_FALSE(c);
    }
    CLUSTER_RETURN_LONG(c, c->reply_len);
}

/* TYPE response handler */
PHPAPI void cluster_type_resp(INTERNAL_FUNCTION_PARAMETERS, redisCluster *c,
                              void *ctx)
{
    // Make sure we got the right kind of response
    if(c->reply_type != TYPE_LINE) {
        CLUSTER_RETURN_FALSE(c);
    }

    // Switch on the type
    if(strncmp(c->line_reply, "+string", 7)==0) {
        CLUSTER_RETURN_LONG(c, REDIS_STRING);
    } else if(strncmp(c->line_reply, "+set", 4)==0) {
        CLUSTER_RETURN_LONG(c, REDIS_SET);
    } else if(strncmp(c->line_reply, "+list", 5)==0) {
        CLUSTER_RETURN_LONG(c, REDIS_LIST); 
    } else if(strncmp(c->line_reply, "+hash", 5)==0) {
        CLUSTER_RETURN_LONG(c, REDIS_HASH);
    } else {
        CLUSTER_RETURN_LONG(c, REDIS_NOT_FOUND); 
    }
}

/* SUBSCRIBE/PSCUBSCRIBE handler */
PHPAPI void cluster_sub_resp(INTERNAL_FUNCTION_PARAMETERS, redisCluster *c,
                             void *ctx)
{
    subscribeContext *sctx = (subscribeContext*)ctx;
    zval *z_tab, **z_tmp, *z_ret, **z_args[4];
    int pull=0;

    // Consume each MULTI BULK response (one per channel/pattern)
    while(sctx->argc--) { 
        z_tab = cluster_zval_mbulk_resp(INTERNAL_FUNCTION_PARAM_PASSTHRU, c, 
            pull, mbulk_resp_loop_raw);
        
        if(!z_tab) {
            efree(sctx);
            RETURN_FALSE;
        }
       
        if(zend_hash_index_find(Z_ARRVAL_P(z_tab),0,(void**)&z_tmp)==FAILURE ||
           strcasecmp(Z_STRVAL_PP(z_tmp), sctx->kw) != 0)
        {
            zval_dtor(z_tab);
            FREE_ZVAL(z_tab);
            efree(sctx);
            RETURN_FALSE;
        }

        zval_dtor(z_tab);
        efree(z_tab);
        pull = 1;
    }

    // Set up our callback pointers
    sctx->cb.retval_ptr_ptr = &z_ret;
    sctx->cb.params = z_args;
    sctx->cb.no_separation = 0;

    /* We're in a subscribe loop */
    c->subscribed_slot = c->reply_slot;

    /* Multibulk response, {[pattern], type, channel, payload} */
    while(1) {
        /* Arguments */
        zval **z_type, **z_chan, **z_pat, **z_data;
        int tab_idx=1, is_pmsg;

        // Get the next subscribe response
        z_tab = cluster_zval_mbulk_resp(INTERNAL_FUNCTION_PARAM_PASSTHRU, c, 
            1, mbulk_resp_loop);
        
        if(!z_tab || zend_hash_index_find(Z_ARRVAL_P(z_tab), 0, (void**)&z_type)
                                          ==FAILURE) 
        {
            break;
        }

        // Make sure we have a message or pmessage
        if(!strncmp(Z_STRVAL_PP(z_type), "message", 7) ||
           !strncmp(Z_STRVAL_PP(z_type), "pmessage", 8))
        {
            is_pmsg = *Z_STRVAL_PP(z_type) == 'p';
        } else {
            zval_dtor(z_tab);
            efree(z_tab);
            continue;
        }

        if(is_pmsg && zend_hash_index_find(Z_ARRVAL_P(z_tab), tab_idx++, 
                                           (void**)&z_pat)==FAILURE)
        {
            break;
        }

        // Extract channel and data
        if(zend_hash_index_find(Z_ARRVAL_P(z_tab), tab_idx++, 
                                (void**)&z_chan)==FAILURE ||
           zend_hash_index_find(Z_ARRVAL_P(z_tab), tab_idx++,
                                (void**)&z_data)==FAILURE)
        {
            break;
        }

        // Always pass our object through
        z_args[0] = &getThis();

        // Set up calbacks depending on type
        if(is_pmsg) {
            z_args[1] = z_pat;
            z_args[2] = z_chan;
            z_args[3] = z_data;
        } else {
            z_args[1] = z_chan;
            z_args[2] = z_data;
        }

        // Set arg count
        sctx->cb.param_count = tab_idx;

        // Execute our callback
        if(zend_call_function(&(sctx->cb), &(sctx->cb_cache) TSRMLS_CC)!=
                              SUCCESS) 
        {
            break;
        }

        // If we have a return value, free it
        if(z_ret) zval_ptr_dtor(&z_ret);

        zval_dtor(z_tab);
        efree(z_tab);
    }
   
    // We're no longer subscribing, due to an error
    c->subscribed_slot = -1;

    // Cleanup
    efree(sctx);
    if(z_tab) {
        zval_dtor(z_tab);
        efree(z_tab);
    }

    // Failure
    RETURN_FALSE;
}

/* UNSUBSCRIBE/PUNSUBSCRIBE */
PHPAPI void cluster_unsub_resp(INTERNAL_FUNCTION_PARAMETERS, 
                               redisCluster *c, void *ctx)
{
    subscribeContext *sctx = (subscribeContext*)ctx;
    zval *z_tab, **z_chan, **z_flag;
    int pull = 0, argc = sctx->argc;

    efree(sctx);
    array_init(return_value);

    // Consume each response
    while(argc--) {
        z_tab = cluster_zval_mbulk_resp(INTERNAL_FUNCTION_PARAM_PASSTHRU,
            c, pull, mbulk_resp_loop_raw);
        
        // Fail if we didn't get an array or can't find index 1
        if(!z_tab || zend_hash_index_find(Z_ARRVAL_P(z_tab), 1, 
                                          (void**)&z_chan)==FAILURE) 
        {
            if(z_tab) {
                zval_dtor(z_tab);
                efree(z_tab);
            }
            zval_dtor(return_value);
            RETURN_FALSE;
        }

        // Find the flag for this channel/pattern
        if(zend_hash_index_find(Z_ARRVAL_P(z_tab), 2, (void**)&z_flag)
                                ==FAILURE || Z_STRLEN_PP(z_flag)!=2)
        {
            zval_dtor(z_tab);
            efree(z_tab);
            zval_dtor(return_value);
            RETURN_FALSE;
        }

        // Redis will give us either :1 or :0 here
        char *flag = Z_STRVAL_PP(z_flag);

        // Add result
        add_assoc_bool(return_value, Z_STRVAL_PP(z_chan), flag[1]=='1');

        zval_dtor(z_tab);
        efree(z_tab);
        pull = 1;
    }
}   

/* Recursive MULTI BULK -> PHP style response handling */
static void cluster_mbulk_variant_resp(clusterReply *r, zval *z_ret)
{
    zval *z_sub_ele;

    switch(r->type) {
        case TYPE_INT:
            add_next_index_long(z_ret, r->integer);
            break;
        case TYPE_LINE:
            add_next_index_bool(z_ret, 1);
            break;
        case TYPE_BULK:
            add_next_index_stringl(z_ret, r->str, r->len, 0);
            break;
        case TYPE_MULTIBULK:
            MAKE_STD_ZVAL(z_sub_ele);
            array_init(z_sub_ele);
            cluster_mbulk_variant_resp(r, z_sub_ele);
            add_next_index_zval(z_ret, z_sub_ele);
            break;
        default:
            add_next_index_bool(z_ret, 0);
            break;
    }
}

/* Variant response handling, for things like EVAL and various other responses
 * where we just map the replies from Redis type values to PHP ones directly. */
PHPAPI void cluster_variant_resp(INTERNAL_FUNCTION_PARAMETERS, redisCluster *c, 
                                 void *ctx)
{
    clusterReply *r;
    zval *z_arr;

    // Make sure we can read it
    if((r = cluster_read_resp(c TSRMLS_CC))==NULL) {
        CLUSTER_RETURN_FALSE(c);
    }

    // Handle ATOMIC vs. MULTI mode in a seperate switch
    if(CLUSTER_IS_ATOMIC(c)) {
        switch(r->type) {
            case TYPE_INT:
                RETVAL_LONG(r->integer);
                break;
            case TYPE_LINE:
                RETVAL_TRUE;
                break;
            case TYPE_BULK:
                RETVAL_STRINGL(r->str, r->len, 0);
                break;
            case TYPE_MULTIBULK:
                MAKE_STD_ZVAL(z_arr);
                array_init(z_arr);
                cluster_mbulk_variant_resp(r, z_arr);
       
                *return_value = *z_arr;
                efree(z_arr);
                break;
            default:
                RETVAL_FALSE;
                break;
        }
    } else {
        switch(r->type) {
            case TYPE_INT:
                add_next_index_long(c->multi_resp, r->integer);
                break;
            case TYPE_LINE:
                add_next_index_bool(c->multi_resp, 1);
                break;
            case TYPE_BULK:
                add_next_index_stringl(c->multi_resp, r->str, r->len, 0);
                break;
            case TYPE_MULTIBULK:
                MAKE_STD_ZVAL(z_arr);
                array_init(z_arr);
                cluster_mbulk_variant_resp(r, z_arr);
                add_next_index_zval(c->multi_resp, z_arr);
                break;
            default:
                add_next_index_bool(c->multi_resp, 0);
                break;
        }
    }

    // Free our response structs, but not allocated data itself
    cluster_free_reply(r, 0);
}

/* Generic MULTI BULK response processor */
PHPAPI void cluster_gen_mbulk_resp(INTERNAL_FUNCTION_PARAMETERS, 
                                   redisCluster *c, mbulk_cb cb, void *ctx)
{
    zval *z_result;

    // Verify our reply type byte is correct and that this isn't a NULL
    // (e.g. -1 count) multi bulk response.
    if(c->reply_type != TYPE_MULTIBULK || c->reply_len == -1) {
        CLUSTER_RETURN_FALSE(c);
    }

    // Allocate array
    MAKE_STD_ZVAL(z_result);
    array_init(z_result);

    // Call our specified callback
    if(cb(SLOT_SOCK(c,c->reply_slot), z_result, c->reply_len, ctx TSRMLS_CC)
                    ==FAILURE)
    {
        zval_dtor(z_result);
        FREE_ZVAL(z_result);
        CLUSTER_RETURN_FALSE(c);
    }
    
    // Success, make this array our return value
    if(CLUSTER_IS_ATOMIC(c)) {
        *return_value = *z_result;
        efree(z_result);
    } else {
        add_next_index_zval(c->multi_resp, z_result);
    }
}

/* HSCAN, SSCAN, ZSCAN */
PHPAPI int cluster_scan_resp(INTERNAL_FUNCTION_PARAMETERS, redisCluster *c, 
                              REDIS_SCAN_TYPE type, long *it)
{
    char *pit;

    // We always want to see a MULTIBULK response with two elements
    if(c->reply_type != TYPE_MULTIBULK || c->reply_len != 2)
    {
        return FAILURE;
    }

    // Read the BULK size
    if(cluster_check_response(c, c->reply_slot, &c->reply_type TSRMLS_CC),0 ||
       c->reply_type != TYPE_BULK)
    {
        return FAILURE;
    }

    // Read the iterator
    if((pit = redis_sock_read_bulk_reply(SLOT_SOCK(c,c->reply_slot), 
                                         c->reply_len TSRMLS_CC))==NULL) 
    {
        return FAILURE;
    }

    // Push the new iterator value to our caller
    *it = atol(pit);
    efree(pit);

    // We'll need another MULTIBULK response for the payload
    if(cluster_check_response(c, c->reply_slot, &c->reply_type TSRMLS_CC)<0)
    {
        return FAILURE;
    }

    // Use the proper response callback depending on scan type
    switch(type) {
        case TYPE_SCAN:
            cluster_mbulk_raw_resp(INTERNAL_FUNCTION_PARAM_PASSTHRU,c,NULL);
            break;
        case TYPE_SSCAN:
            cluster_mbulk_resp(INTERNAL_FUNCTION_PARAM_PASSTHRU,c,NULL);
            break;
        case TYPE_HSCAN:
            cluster_mbulk_zipstr_resp(INTERNAL_FUNCTION_PARAM_PASSTHRU,c,NULL);
            break;
        case TYPE_ZSCAN:
            cluster_mbulk_zipdbl_resp(INTERNAL_FUNCTION_PARAM_PASSTHRU,c,NULL);
            break;
        default:
            return FAILURE;
    }

    // Success
    return SUCCESS;
}

/* MULTI BULK response loop where we might pull the next one */
PHPAPI zval *cluster_zval_mbulk_resp(INTERNAL_FUNCTION_PARAMETERS,
                                     redisCluster *c, int pull, mbulk_cb cb)
{
    zval *z_result;

    // Pull our next response if directed
    if(pull) {
        if(cluster_check_response(c, c->reply_slot, &c->reply_type 
                                  TSRMLS_CC)<0)
        {
            return NULL;
        }
    }

    // Validate reply type and length
    if(c->reply_type != TYPE_MULTIBULK || c->reply_len == -1) {
        return NULL;
    }

    MAKE_STD_ZVAL(z_result);
    array_init(z_result);

    // Call our callback
    if(cb(SLOT_SOCK(c,c->reply_slot), z_result, c->reply_len, NULL)==FAILURE) {
        zval_dtor(z_result);
        FREE_ZVAL(z_result);
        return NULL;
    }

    return z_result;
}

/* MULTI MULTI BULK reply (for EXEC) */
PHPAPI void cluster_multi_mbulk_resp(INTERNAL_FUNCTION_PARAMETERS, 
                                     redisCluster *c, void *ctx)
{
    MAKE_STD_ZVAL(c->multi_resp);
    array_init(c->multi_resp);

    clusterFoldItem *fi = c->multi_head;
    while(fi) {
        if(cluster_check_response(c, fi->slot, &c->reply_type TSRMLS_CC)<0) {
            zval_dtor(c->multi_resp);
            efree(c->multi_resp);
            RETURN_FALSE;
        }

        c->reply_slot = fi->slot;
        fi->callback(INTERNAL_FUNCTION_PARAM_PASSTHRU, c, fi->ctx);
        fi = fi->next;
    }

    // Set our return array
    zval_dtor(return_value);
    *return_value = *c->multi_resp;
    efree(c->multi_resp);
}

/* Generic handler for MGET */
PHPAPI void cluster_mbulk_mget_resp(INTERNAL_FUNCTION_PARAMETERS, 
                                    redisCluster *c, void *ctx)
{
    clusterMultiCtx *mctx = (clusterMultiCtx*)ctx;

    // Protect against an invalid response type, -1 response length, and failure
    // to consume the responses.
    short fail = c->reply_type != TYPE_MULTIBULK || c->reply_len == -1 ||
                 mbulk_resp_loop(SLOT_SOCK(c,c->reply_slot), mctx->z_multi, 
                                 c->reply_len, NULL TSRMLS_CC)==FAILURE;

    // If we had a failure, pad results with FALSE to indicate failure.  Non
    // existant keys (e.g. for MGET will come back as NULL)
    if(fail) {
        php_error_docref(0 TSRMLS_CC, E_WARNING,
            "Invalid response from Redis for MGET command");
        while(mctx->count--) { 
            add_next_index_bool(mctx->z_multi, 0);
        }
    }

    // If this is the tail of our multi command, we can set our returns
    if(mctx->last) {
        if(CLUSTER_IS_ATOMIC(c)) {
            *return_value = *(mctx->z_multi);
            efree(mctx->z_multi);
        } else {
            add_next_index_zval(c->multi_resp, mctx->z_multi);
        }
    }

    // Clean up this context item
    efree(mctx);
}

/* Handler for MSETNX */
PHPAPI void cluster_msetnx_resp(INTERNAL_FUNCTION_PARAMETERS, redisCluster *c,
                                void *ctx)
{
    clusterMultiCtx *mctx = (clusterMultiCtx*)ctx;
    int real_argc = mctx->count/2;

    // Protect against an invalid response type
    if(c->reply_type != TYPE_INT) {
        php_error_docref(0 TSRMLS_CC, E_WARNING,
            "Invalid response type for MSETNX");
        while(real_argc--) {
            add_next_index_bool(mctx->z_multi, 0);
        }
        return;
    }

    // Response will be 1/0 per key, so the client can match them up
    while(real_argc--) {
        add_next_index_long(mctx->z_multi, c->reply_len);
    }

    // Set return value if it's our last response
    if(mctx->last) {
        if(CLUSTER_IS_ATOMIC(c)) {
            *return_value = *(mctx->z_multi);
            efree(mctx->z_multi);
        } else {
            add_next_index_zval(c->multi_resp, mctx->z_multi);
        }
    }

    // Free multi context
    efree(mctx);
}

/* Handler for DEL */
PHPAPI void cluster_del_resp(INTERNAL_FUNCTION_PARAMETERS, redisCluster *c,
                             void *ctx)
{
    clusterMultiCtx *mctx = (clusterMultiCtx*)ctx;

    // If we get an invalid reply, inform the client
    if(c->reply_type != TYPE_INT) {
        php_error_docref(0 TSRMLS_CC, E_WARNING,
            "Invalid reply type returned for DEL command");
        efree(mctx);
        return;
    }

    // Increment by the number of keys deleted
    Z_LVAL_P(mctx->z_multi) += c->reply_len;

    if(mctx->last) {
        if(CLUSTER_IS_ATOMIC(c)) {
            ZVAL_LONG(return_value, Z_LVAL_P(mctx->z_multi));
        } else {
            add_next_index_long(c->multi_resp, Z_LVAL_P(mctx->z_multi));
        }
        efree(mctx->z_multi);
    }

    efree(ctx);
}

/* Handler for MSET */
PHPAPI void cluster_mset_resp(INTERNAL_FUNCTION_PARAMETERS, redisCluster *c,
                              void *ctx)
{
    clusterMultiCtx *mctx = (clusterMultiCtx*)ctx;

    // If we get an invalid reply type something very wrong has happened, 
    // and we have to abort.
    if(c->reply_type != TYPE_LINE) {
        php_error_docref(0 TSRMLS_CC, E_ERROR,
            "Invalid reply type returned for MSET command");
        ZVAL_FALSE(return_value);
        efree(mctx->z_multi);
        efree(mctx);
        return;
    }

    // Set our return if it's the last call
    if(mctx->last) {
        if(CLUSTER_IS_ATOMIC(c)) {
            ZVAL_BOOL(return_value, Z_BVAL_P(mctx->z_multi));
        } else {
            add_next_index_bool(c->multi_resp, Z_BVAL_P(mctx->z_multi));
        }
        efree(mctx->z_multi);
    }

    efree(mctx);
}

/* Raw MULTI BULK reply */
PHPAPI void 
cluster_mbulk_raw_resp(INTERNAL_FUNCTION_PARAMETERS, redisCluster *c, void *ctx) 
{
    cluster_gen_mbulk_resp(INTERNAL_FUNCTION_PARAM_PASSTHRU, c, 
        mbulk_resp_loop_raw, NULL);
}

/* Unserialize all the things */
PHPAPI void
cluster_mbulk_resp(INTERNAL_FUNCTION_PARAMETERS, redisCluster *c, void *ctx) 
{
    cluster_gen_mbulk_resp(INTERNAL_FUNCTION_PARAM_PASSTHRU, c, 
        mbulk_resp_loop, NULL);
}

/* For handling responses where we get key, value, key, value that
 * we will turn into key => value, key => value. */
PHPAPI void
cluster_mbulk_zipstr_resp(INTERNAL_FUNCTION_PARAMETERS, redisCluster *c,
                          void *ctx) 
{
    cluster_gen_mbulk_resp(INTERNAL_FUNCTION_PARAM_PASSTHRU, c, 
        mbulk_resp_loop_zipstr, NULL);
}

/* Handling key,value to key=>value where the values are doubles */
PHPAPI void
cluster_mbulk_zipdbl_resp(INTERNAL_FUNCTION_PARAMETERS, redisCluster *c,
                          void *ctx) 
{
    cluster_gen_mbulk_resp(INTERNAL_FUNCTION_PARAM_PASSTHRU, c, 
        mbulk_resp_loop_zipdbl, NULL);
}

/* Associate multi bulk response (for HMGET really) */
PHPAPI void
cluster_mbulk_assoc_resp(INTERNAL_FUNCTION_PARAMETERS, redisCluster *c,
                         void *ctx)
{
    cluster_gen_mbulk_resp(INTERNAL_FUNCTION_PARAM_PASSTHRU, c,
        mbulk_resp_loop_assoc, ctx);
}

/* 
 * Various MULTI BULK reply callback functions 
 */

/* MULTI BULK response where we don't touch the values (e.g. KEYS) */
int mbulk_resp_loop_raw(RedisSock *redis_sock, zval *z_result, 
                        long long count, void *ctx TSRMLS_DC) 
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
int mbulk_resp_loop(RedisSock *redis_sock, zval *z_result, 
                    long long count, void *ctx TSRMLS_DC)
{
    char *line;
    int line_len;
    zval *z;

    // Iterate over the lines we have to process
    while(count--) {
        // Read the line
        line = redis_sock_read(redis_sock, &line_len TSRMLS_CC);
        if(line == NULL) return FAILURE;

        if(line_len > 0) {
            if(redis_unserialize(redis_sock, line, line_len, &z TSRMLS_CC)==1) {
                add_next_index_zval(z_result, z);
                efree(line);
            } else {
                add_next_index_stringl(z_result, line, line_len, 0);
            }
        } else {
            efree(line);
            add_next_index_null(z_result);
        }
    }

    return SUCCESS;
}

/* MULTI BULK response where we turn key1,value1 into key1=>value1 */
int mbulk_resp_loop_zipstr(RedisSock *redis_sock, zval *z_result, 
                           long long count, void *ctx TSRMLS_DC)
{
    char *line, *key;
    int line_len, key_len;
    long long idx=0;
    zval *z;

    // Our count wil need to be divisible by 2
    if(count % 2 != 0) {
        return -1;
    }

    // Iterate through our elements
    while(count--) {
        // Grab our line, bomb out on failure
        line = redis_sock_read(redis_sock, &line_len TSRMLS_CC);
        if(!line) return -1;

        if(idx++ % 2 == 0) {
            // Save our key and length
            key = line;
            key_len = line_len;
        } else {
            // Attempt unserialization, add value
            if(redis_unserialize(redis_sock, line, line_len, &z TSRMLS_CC)==1) {
                add_assoc_zval(z_result, key, z);
            } else {
                add_assoc_stringl_ex(z_result, key, 1+key_len, line, 
                    line_len, 0);
            }
            efree(key);
        }
    }

    return SUCCESS;
}

/* MULTI BULK loop processor where we expect key,score key, score */
int mbulk_resp_loop_zipdbl(RedisSock *redis_sock, zval *z_result,
                           long long count, void *ctx TSRMLS_DC)
{
    char *line, *key;
    int line_len, key_len;
    long long idx=0;

    // Our context will need to be divisible by 2
    if(count %2 != 0) {
        return -1;
    }

    // While we have elements
    while(count--) {
        line = redis_sock_read(redis_sock, &line_len TSRMLS_CC);
        if(!line) return -1;

        if(idx++ % 2 == 0) {
            key = line;
            key_len = line_len;
        } else {
            add_assoc_double_ex(z_result, key, 1+key_len, atof(line));
            efree(key);
            efree(line);
        }
    }

    return SUCCESS;
}

/* MULTI BULK where we're passed the keys, and we attach vals */
int mbulk_resp_loop_assoc(RedisSock *redis_sock, zval *z_result, 
                          long long count, void *ctx TSRMLS_DC)
{
    char *line;
    int line_len,i=0;
    zval **z_keys = ctx, *z;
    
    // Loop while we've got replies
    while(count--) {
        line = redis_sock_read(redis_sock, &line_len TSRMLS_CC);
        
        if(line != NULL) {
            if(redis_unserialize(redis_sock, line, line_len, &z TSRMLS_CC)==1) {
                efree(line);
                add_assoc_zval_ex(z_result,Z_STRVAL_P(z_keys[i]),
                    1+Z_STRLEN_P(z_keys[i]), z);
            } else {
                add_assoc_stringl_ex(z_result, Z_STRVAL_P(z_keys[i]),
                    1+Z_STRLEN_P(z_keys[i]), line, line_len, 0);
            }
        } else {
            add_assoc_bool_ex(z_result, Z_STRVAL_P(z_keys[i]),
                1+Z_STRLEN_P(z_keys[i]), 0);
        }

        // Clean up key context
        zval_dtor(z_keys[i]);
        efree(z_keys[i]);
        
        // Move to the next key
        i++;
    }

    // Clean up our keys overall
    efree(z_keys);

    // Success!
    return SUCCESS;
}
/* vim: set tabstop=4 softtabstops=4 noexpandtab shiftwidth=4: */
