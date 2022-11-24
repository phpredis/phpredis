#include "php_redis.h"
#include "common.h"
#include "library.h"
#include "redis_commands.h"
#include "cluster_library.h"
#include "crc16.h"
#include <zend_exceptions.h>

extern zend_class_entry *redis_cluster_exception_ce;
int le_cluster_slot_cache;

/* Debugging methods/
static void cluster_dump_nodes(redisCluster *c) {
    redisClusterNode *p;

    ZEND_HASH_FOREACH_PTR(c->nodes, p) {
        if (p == NULL) {
            continue;
        }

        const char *slave = (p->slave) ? "slave" : "master";
        php_printf("%d %s %d %d", p->sock->port, slave,p->sock->prefix_len,
            p->slot);

        php_printf("\n");
    } ZEND_HASH_FOREACH_END();
}

static void cluster_log(char *fmt, ...)
{
    va_list args;
    char buffer[1024];

    va_start(args, fmt);
    vsnprintf(buffer,sizeof(buffer),fmt,args);
    va_end(args);

    fprintf(stderr, "%s\n", buffer);
}

// Debug function to dump a clusterReply structure recursively
static void dump_reply(clusterReply *reply, int indent) {
    smart_string buf = {0};
    int i;

    switch(reply->type) {
        case TYPE_ERR:
            smart_string_appendl(&buf, "(error) ", sizeof("(error) ")-1);
            smart_string_appendl(&buf, reply->str, reply->len);
            break;
        case TYPE_LINE:
            smart_string_appendl(&buf, reply->str, reply->len);
            break;
        case TYPE_INT:
            smart_string_appendl(&buf, "(integer) ", sizeof("(integer) ")-1);
            smart_string_append_long(&buf, reply->integer);
            break;
        case TYPE_BULK:
            smart_string_appendl(&buf,"\"", 1);
            smart_string_appendl(&buf, reply->str, reply->len);
            smart_string_appendl(&buf, "\"", 1);
            break;
        case TYPE_MULTIBULK:
            if (reply->elements < 0) {
                smart_string_appendl(&buf, "(nil)", sizeof("(nil)")-1);
            } else {
                for (i = 0; i < reply->elements; i++) {
                    dump_reply(reply->element[i], indent+2);
                }
            }
            break;
        default:
            break;
    }

    if (buf.len > 0) {
        for (i = 0; i < indent; i++) {
            php_printf(" ");
        }

        smart_string_0(&buf);
        php_printf("%s", buf.c);
        php_printf("\n");

        efree(buf.c);
    }
}
*/


/* Recursively free our reply object.  If free_data is non-zero we'll also free
 * the payload data (strings) themselves.  If not, we just free the structs */
void cluster_free_reply(clusterReply *reply, int free_data) {
    long long i;

    switch(reply->type) {
        case TYPE_ERR:
        case TYPE_LINE:
        case TYPE_BULK:
            if (free_data && reply->str)
                efree(reply->str);
            break;
        case TYPE_MULTIBULK:
            if (reply->element) {
                if (reply->elements > 0) {
                    for (i = 0; i < reply->elements && reply->element[i]; i++) {
                        cluster_free_reply(reply->element[i], free_data);
                    }
                }
                efree(reply->element);
            }
            break;
        default:
            break;
    }
    efree(reply);
}

static int
cluster_multibulk_resp_recursive(RedisSock *sock, size_t elements,
                                 clusterReply **element, int status_strings)
{
    int i;
    size_t sz;
    clusterReply *r;
    long len;
    char buf[1024];

    for (i = 0; i < elements; i++) {
        r = element[i] = ecalloc(1, sizeof(clusterReply));

        // Bomb out, flag error condition on a communication failure
        if (redis_read_reply_type(sock, &r->type, &len) < 0) {
            return FAILURE;
        }

        /* Set our reply len */
        r->len = len;

        switch(r->type) {
            case TYPE_ERR:
            case TYPE_LINE:
                if (redis_sock_gets(sock,buf,sizeof(buf),&sz) < 0) {
                    return FAILURE;
                }
                r->len = (long long)sz;
                if (status_strings) r->str = estrndup(buf, r->len);
                break;
            case TYPE_INT:
                r->integer = len;
                break;
            case TYPE_BULK:
                if (r->len >= 0) {
                    r->str = redis_sock_read_bulk_reply(sock,r->len);
                    if (!r->str) {
                        return FAILURE;
                    }
                }
                break;
            case TYPE_MULTIBULK:
                r->elements = r->len;
                if (r->elements > 0) {
                    r->element = ecalloc(r->len, sizeof(*r->element));
                    if (cluster_multibulk_resp_recursive(sock, r->elements, r->element, status_strings) < 0) {
                        return FAILURE;
                    }
                }
                break;
            default:
                return FAILURE;
        }
    }
    return SUCCESS;
}

/* Return the socket for a slot and slave index */
static RedisSock *cluster_slot_sock(redisCluster *c, unsigned short slot,
                                    zend_ulong slaveidx)
{
    redisClusterNode *node;

    /* Return the master if we're not looking for a slave */
    if (slaveidx == 0) {
        return SLOT_SOCK(c, slot);
    }

    /* Abort if we can't find this slave */
    if (!SLOT_SLAVES(c, slot) ||
        (node = zend_hash_index_find_ptr(SLOT_SLAVES(c,slot), slaveidx)) == NULL
    ) {
        return NULL;
    }

    /* Success, return the slave */
    return node->sock;
}

/* Read the response from a cluster */
clusterReply *cluster_read_resp(redisCluster *c, int status_strings) {
    return cluster_read_sock_resp(c->cmd_sock, c->reply_type,
                                  status_strings ? c->line_reply : NULL,
                                  c->reply_len);
}

/* Read any sort of response from the socket, having already issued the
 * command and consumed the reply type and meta info (length) */
clusterReply*
cluster_read_sock_resp(RedisSock *redis_sock, REDIS_REPLY_TYPE type,
                       char *line_reply, long long len)
{
    clusterReply *r;

    r = ecalloc(1, sizeof(clusterReply));
    r->type = type;

    switch(r->type) {
        case TYPE_INT:
            r->integer = len;
            break;
        case TYPE_LINE:
            if (line_reply) {
                r->str = estrndup(line_reply, len);
                r->len = len;
            }
            REDIS_FALLTHROUGH;
        case TYPE_ERR:
            return r;
        case TYPE_BULK:
            r->len = len;
            r->str = redis_sock_read_bulk_reply(redis_sock, len);
            if (r->len != -1 && !r->str) {
                cluster_free_reply(r, 1);
                return NULL;
            }
            break;
        case TYPE_MULTIBULK:
            r->elements = len;
            if (r->elements > 0) {
                r->element = ecalloc(len, sizeof(clusterReply*));
                if (cluster_multibulk_resp_recursive(redis_sock, len, r->element, line_reply != NULL) < 0) {
                    cluster_free_reply(r, 1);
                    return NULL;
                }
            }
            break;
        default:
            cluster_free_reply(r, 1);
            return NULL;
    }

    // Success, return the reply
    return r;
}

/*
 * Helpers to send various 'control type commands to a specific node, e.g.
 * MULTI, ASKING, READONLY, READWRITE, etc
 */

/* Send a command to the specific socket and validate reply type */
static int cluster_send_direct(RedisSock *redis_sock, char *cmd, int cmd_len,
                               REDIS_REPLY_TYPE type)
{
    char buf[1024];

    /* Connect to the socket if we aren't yet and send our command, validate the reply type, and consume the first line */
    if (!CLUSTER_SEND_PAYLOAD(redis_sock,cmd,cmd_len) ||
        !CLUSTER_VALIDATE_REPLY_TYPE(redis_sock, type) ||
        !redis_sock_gets_raw(redis_sock, buf, sizeof(buf))) return -1;

    /* Success! */
    return 0;
}

static int cluster_send_asking(RedisSock *redis_sock) {
    return cluster_send_direct(redis_sock, ZEND_STRL(RESP_ASKING_CMD), TYPE_LINE);
}

/* Send READONLY to a specific RedisSock unless it's already flagged as being
 * in READONLY mode.  If we can send the command, we flag the socket as being
 * in that mode. */
static int cluster_send_readonly(RedisSock *redis_sock) {
    int ret;

    /* We don't have to do anything if we're already in readonly mode */
    if (redis_sock->readonly) return 0;

    /* Return success if we can send it */
    ret = cluster_send_direct(redis_sock, ZEND_STRL(RESP_READONLY_CMD), TYPE_LINE);

    /* Flag this socket as READONLY if our command worked */
    redis_sock->readonly = !ret;

    /* Return the result of our send */
    return ret;
}

/* Send MULTI to a specific ReidsSock */
static int cluster_send_multi(redisCluster *c, short slot) {
    if (cluster_send_direct(SLOT_SOCK(c,slot), ZEND_STRL(RESP_MULTI_CMD), TYPE_LINE) == 0) {
        c->flags->txBytes += sizeof(RESP_MULTI_CMD) - 1;
        c->cmd_sock->mode = MULTI;
        return 0;
    }
    return -1;
}

/* Send EXEC to a given slot.  We can use the normal command processing mechanism
 * here because we know we'll only have sent MULTI to the master nodes.  We can't
 * failover inside a transaction, as we don't know if the transaction will only
 * be readonly commands, or contain write commands as well */
PHP_REDIS_API int cluster_send_exec(redisCluster *c, short slot) {
    int retval;

    /* Send exec */
    retval = cluster_send_slot(c, slot, ZEND_STRL(RESP_EXEC_CMD), TYPE_MULTIBULK);

    /* We'll either get a length corresponding to the number of commands sent to
     * this node, or -1 in the case of EXECABORT or WATCH failure. */
    c->multi_len[slot] = c->reply_len > 0 ? 1 : -1;

    /* Return our retval */
    return retval;
}

PHP_REDIS_API int cluster_send_discard(redisCluster *c, short slot) {
    if (cluster_send_direct(SLOT_SOCK(c,slot), ZEND_STRL(RESP_DISCARD_CMD), TYPE_LINE))
    {
        return 0;
    }
    return -1;
}

/*
 * Cluster key distribution helpers.  For a small handlful of commands, we want
 * to distribute them across 1-N nodes.  These methods provide simple containers
 * for the purposes of splitting keys/values in this way
 * */

/* Free cluster distribution list inside a HashTable */
static void cluster_dist_free_ht(zval *p) {
    clusterDistList *dl = *(clusterDistList**)p;
    int i;

    for (i = 0; i < dl->len; i++) {
        if (dl->entry[i].key_free)
            efree(dl->entry[i].key);
        if (dl->entry[i].val_free)
            efree(dl->entry[i].val);
    }

    efree(dl->entry);
    efree(dl);
}

/* Spin up a HashTable that will contain distribution lists */
HashTable *cluster_dist_create(void) {
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
static clusterDistList *cluster_dl_create(void) {
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
    if (dl->len == dl->size) {
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
                          size_t key_len, clusterKeyVal **kv)
{
    int key_free;
    short slot;
    clusterDistList *dl;
    clusterKeyVal *retptr;

    // Prefix our key and hash it
    key_free = redis_key_prefix(c->flags, &key, &key_len);
    slot = cluster_hash_key(key, key_len);

    // We can't do this if we don't fully understand the keyspace
    if (c->master[slot] == NULL) {
        if (key_free) efree(key);
        return FAILURE;
    }

    // Look for this slot
    if ((dl = zend_hash_index_find_ptr(ht, (zend_ulong)slot)) == NULL) {
        dl = cluster_dl_create();
        zend_hash_index_update_ptr(ht, (zend_ulong)slot, dl);
    }

    // Now actually add this key
    retptr = cluster_dl_add_key(dl, key, key_len, key_free);

    // Push our return pointer if requested
    if (kv) *kv = retptr;

    return SUCCESS;
}

/* Provided a clusterKeyVal, add a value */
void cluster_dist_add_val(redisCluster *c, clusterKeyVal *kv, zval *z_val
                        )
{
    char *val;
    size_t val_len;
    int val_free;

    // Serialize our value
    val_free = redis_pack(c->flags, z_val, &val, &val_len);

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
    smart_string_appendl(&(mc->cmd), mc->args.c, mc->args.len);
}

/* Set our last error string encountered */
static void
cluster_set_err(redisCluster *c, char *err, int err_len)
{
    // Free our last error
    if (c->err != NULL) {
        zend_string_release(c->err);
        c->err = NULL;
    }
    if (err != NULL && err_len > 0) {
        c->err = zend_string_init(err, err_len, 0);
        if (err_len >= sizeof("CLUSTERDOWN") - 1 &&
            !memcmp(err, "CLUSTERDOWN", sizeof("CLUSTERDOWN") - 1)
        ) {
            c->clusterdown = 1;
        }
    }
}

/* Destructor for slaves */
static void ht_free_slave(zval *data) {
    if (*(redisClusterNode**)data) {
        cluster_free_node(*(redisClusterNode**)data);
    }
}

/* Get the hash slot for a given key */
unsigned short cluster_hash_key(const char *key, int len) {
    int s, e;

    // Find first occurrence of {, if any
    for (s = 0; s < len; s++) {
        if (key[s]=='{') break;
    }

    // There is no '{', hash everything
    if (s == len) return crc16(key, len) & REDIS_CLUSTER_MOD;

    // Found it, look for a tailing '}'
    for (e =s + 1; e < len; e++) {
        if (key[e] == '}') break;
    }

    // Hash the whole key if we don't find a tailing } or if {} is empty
    if (e == len || e == s+1) return crc16(key, len) & REDIS_CLUSTER_MOD;

    // Hash just the bit between { and }
    return crc16((char*)key+s+1,e-s-1) & REDIS_CLUSTER_MOD;
}

unsigned short cluster_hash_key_zstr(zend_string *key) {
    return cluster_hash_key(ZSTR_VAL(key), ZSTR_LEN(key));
}

/* Grab the current time in milliseconds */
long long mstime(void) {
    struct timeval tv;
    long long mst;

    gettimeofday(&tv, NULL);
    mst = ((long long)tv.tv_sec)*1000;
    mst += tv.tv_usec/1000;

    return mst;
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
            klen = snprintf(buf,sizeof(buf),ZEND_LONG_FMT,Z_LVAL_P(z_key));
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
        default:
            kptr = "";
            klen = 0;
    }

    // Hash the string representation
    return cluster_hash_key(kptr, klen);
}

/* Fisher-Yates shuffle for integer array */
static void fyshuffle(int *array, size_t len) {
    int temp, n = len;
    size_t r;

    /* Randomize */
    while (n > 1) {
        r = ((int)((double)n-- * (rand() / (RAND_MAX+1.0))));
        temp = array[n];
        array[n] = array[r];
        array[r] = temp;
    };
}

/* Execute a CLUSTER SLOTS command against the seed socket, and return the
 * reply or NULL on failure. */
clusterReply* cluster_get_slots(RedisSock *redis_sock)
{
    clusterReply *r;
    REDIS_REPLY_TYPE type;
    long len;

    // Send the command to the socket and consume reply type
    if (redis_sock_write(redis_sock, RESP_CLUSTER_SLOTS_CMD,
                        sizeof(RESP_CLUSTER_SLOTS_CMD)-1) < 0 ||
                        redis_read_reply_type(redis_sock, &type, &len) < 0)
    {
        return NULL;
    }

    // Consume the rest of our response
    if ((r = cluster_read_sock_resp(redis_sock, type, NULL, len)) == NULL ||
       r->type != TYPE_MULTIBULK || r->elements < 1)
    {
        if (r) cluster_free_reply(r, 1);
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

    /* Initialize our list of slot ranges */
    zend_llist_init(&node->slots, sizeof(redisSlotRange), NULL, 0);

    // Attach socket
    node->sock = redis_sock_create(host, host_len, port,
                                   c->flags->timeout, c->flags->read_timeout,
                                   c->flags->persistent, NULL, 0);

    /* Stream context */
    node->sock->stream_ctx = c->flags->stream_ctx;

    redis_sock_set_auth(node->sock, c->flags->user, c->flags->pass);

    return node;
}

/* Attach a slave to a master */
PHP_REDIS_API int
cluster_node_add_slave(redisClusterNode *master, redisClusterNode *slave)
{
    zend_ulong index;

    // Allocate our slaves hash table if we haven't yet
    if (!master->slaves) {
        ALLOC_HASHTABLE(master->slaves);
        zend_hash_init(master->slaves, 0, NULL, ht_free_slave, 0);
        index = 1;
    } else {
        index = master->slaves->nNextFreeElement;
    }

    return zend_hash_index_update_ptr(master->slaves, index, slave) != NULL;
}

/* Sanity check/validation for CLUSTER SLOTS command */
#define VALIDATE_SLOTS_OUTER(r) \
    (r->elements >= 3 && r2->element[0]->type == TYPE_INT && \
     r->element[1]->type == TYPE_INT)
#define VALIDATE_SLOTS_INNER(r) \
    (r->type == TYPE_MULTIBULK && r->elements >= 2 && \
     r->element[0]->type == TYPE_BULK && r->element[1]->type == TYPE_INT)

/* Use the output of CLUSTER SLOTS to map our nodes */
static int cluster_map_slots(redisCluster *c, clusterReply *r) {
    redisClusterNode *pnode, *master, *slave;
    redisSlotRange range;
    int i,j, hlen, klen;
    short low, high;
    clusterReply *r2, *r3;
    unsigned short port;
    char *host, key[1024];
    zend_hash_clean(c->nodes);
    for (i = 0; i < r->elements; i++) {
        // Inner response
        r2 = r->element[i];

        // Validate outer and master slot
        if (!VALIDATE_SLOTS_OUTER(r2) || !VALIDATE_SLOTS_INNER(r2->element[2])) {
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
        klen = snprintf(key, sizeof(key), "%s:%d", host, port);
        if ((pnode = zend_hash_str_find_ptr(c->nodes, key, klen)) == NULL) {
            master = cluster_node_create(c, host, hlen, port, low, 0);
            zend_hash_str_update_ptr(c->nodes, key, klen, master);

            // Attach slaves first time we encounter a given master in order to avoid regitering the slaves multiple times
            for (j = 3; j< r2->elements; j++) {
                r3 = r2->element[j];
                if (!VALIDATE_SLOTS_INNER(r3)) {
                    return -1;
                }

                // Skip slaves where the host is ""
                if (r3->element[0]->len == 0) continue;

                // Attach this node to our slave
                slave = cluster_node_create(c, r3->element[0]->str,
                    (int)r3->element[0]->len,
                    (unsigned short)r3->element[1]->integer, low, 1);
                cluster_node_add_slave(master, slave);
            }
        } else {
            master = pnode;
        }

        // Attach this node to each slot in the range
        for (j = low; j<= high; j++) {
            c->master[j] = master;
        }

        /* Append to our list of slot ranges */
        range.low = low; range.high = high;
        zend_llist_add_element(&master->slots, &range);
    }

    // Success
    return 0;
}

/* Free a redisClusterNode structure */
PHP_REDIS_API void cluster_free_node(redisClusterNode *node) {
    if (node->slaves) {
        zend_hash_destroy(node->slaves);
        efree(node->slaves);
    }

    zend_llist_destroy(&node->slots);
    redis_free_socket(node->sock);

    efree(node);
}

/* Get or create a redisClusterNode that corresponds to the asking redirection */
static redisClusterNode *cluster_get_asking_node(redisCluster *c) {
    redisClusterNode *pNode;
    char key[1024];
    int key_len;

    /* Hashed by host:port */
    key_len = snprintf(key, sizeof(key), "%s:%u", c->redir_host, c->redir_port);

    /* See if we've already attached to it */
    if ((pNode = zend_hash_str_find_ptr(c->nodes, key, key_len)) != NULL) {
        return pNode;
    }

    /* This host:port is unknown to us, so add it */
    pNode = cluster_node_create(c, c->redir_host, c->redir_host_len,
        c->redir_port, c->redir_slot, 0);

    /* Return the node */
   return pNode;
}

/* Get or create a node at the host:port we were asked to check, and return the
 * redis_sock for it. */
static RedisSock *cluster_get_asking_sock(redisCluster *c) {
    return cluster_get_asking_node(c)->sock;
}

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

/* zend_llist of slot ranges -> persistent array */
static redisSlotRange *slot_range_list_clone(zend_llist *src, size_t *count) {
    redisSlotRange *dst, *range;
    size_t i = 0;

    *count = zend_llist_count(src);
    dst = pemalloc(*count * sizeof(*dst), 1);

    range = zend_llist_get_first(src);
    while (range) {
        memcpy(&dst[i++], range, sizeof(*range));
        range = zend_llist_get_next(src);
     }

    return dst;
}

/* Construct a redisCluster object */
PHP_REDIS_API redisCluster *cluster_create(double timeout, double read_timeout,
                                           int failover, int persistent)
{
    redisCluster *c;

    /* Actual our actual cluster structure */
    c = ecalloc(1, sizeof(redisCluster));

    /* Initialize flags and settings */
    c->flags = ecalloc(1, sizeof(RedisSock));
    c->flags->timeout = timeout;
    c->flags->read_timeout = read_timeout;
    c->flags->persistent = persistent;
    c->subscribed_slot = -1;
    c->clusterdown = 0;
    c->failover = failover;
    c->err = NULL;

    /* Set up our waitms based on timeout */
    c->waitms  = (long)(1000 * timeout);

    /* Allocate our seeds hash table */
    ALLOC_HASHTABLE(c->seeds);
    zend_hash_init(c->seeds, 0, NULL, ht_free_seed, 0);

    /* Allocate our nodes HashTable */
    ALLOC_HASHTABLE(c->nodes);
    zend_hash_init(c->nodes, 0, NULL, ht_free_node, 0);

    return c;
}

PHP_REDIS_API void
cluster_free(redisCluster *c, int free_ctx)
{
    /* Disconnect from each node we're connected to */
    cluster_disconnect(c, 0);

    /* Free any allocated prefix */
    if (c->flags->prefix) zend_string_release(c->flags->prefix);

    redis_sock_free_auth(c->flags);
    efree(c->flags);

    /* Call hash table destructors */
    zend_hash_destroy(c->seeds);
    zend_hash_destroy(c->nodes);

    /* Free hash tables themselves */
    efree(c->seeds);
    efree(c->nodes);

    /* Free any error we've got */
    if (c->err) zend_string_release(c->err);

    if (c->cache_key) {
        /* Invalidate persistent cache if the cluster has changed */
        if (c->redirections) {
            zend_hash_del(&EG(persistent_list), c->cache_key);
        }

        /* Release our hold on the cache key */
        zend_string_release(c->cache_key);
    }

    /* Free structure itself */
    if (free_ctx) efree(c);
}

/* Create a cluster slot cache structure */
PHP_REDIS_API
redisCachedCluster *cluster_cache_create(zend_string *hash, HashTable *nodes) {
    redisCachedCluster *cc;
    redisCachedMaster *cm;
    redisClusterNode *node, *slave;

    cc = pecalloc(1, sizeof(*cc), 1);
    cc->hash = zend_string_dup(hash, 1);

    /* Copy nodes */
    cc->master = pecalloc(zend_hash_num_elements(nodes), sizeof(*cc->master), 1);
    ZEND_HASH_FOREACH_PTR(nodes, node) {
        /* Skip slaves */
        if (node->slave) continue;

        cm = &cc->master[cc->count];

        /* Duplicate host/port and clone slot ranges */
        cm->host.addr = zend_string_dup(node->sock->host, 1);
        cm->host.port = node->sock->port;

        /* Copy over slot ranges */
        cm->slot = slot_range_list_clone(&node->slots, &cm->slots);

        /* Attach any slave nodes we have. */
        if (node->slaves) {
            /* Allocate memory for slaves */
            cm->slave = pecalloc(zend_hash_num_elements(node->slaves), sizeof(*cm->slave), 1);

            /* Copy host/port information for each slave */
            ZEND_HASH_FOREACH_PTR(node->slaves, slave) {
                cm->slave[cm->slaves].addr = zend_string_dup(slave->sock->host, 1);
                cm->slave[cm->slaves].port = slave->sock->port;
                cm->slaves++;
            } ZEND_HASH_FOREACH_END();
        }

        cc->count++;
    } ZEND_HASH_FOREACH_END();

    return cc;
}

static void cluster_free_cached_master(redisCachedMaster *cm) {
    size_t i;

    /* Free each slave entry */
    for (i = 0; i < cm->slaves; i++) {
        zend_string_release(cm->slave[i].addr);
    }

    /* Free other elements */
    zend_string_release(cm->host.addr);
    pefree(cm->slave, 1);
    pefree(cm->slot, 1);
}

static redisClusterNode*
cached_master_clone(redisCluster *c, redisCachedMaster *cm) {
    redisClusterNode *node;
    size_t i;

    node = cluster_node_create(c, ZSTR_VAL(cm->host.addr), ZSTR_LEN(cm->host.addr),
                               cm->host.port, cm->slot[0].low, 0);

    /* Now copy in our slot ranges */
    for (i = 0; i < cm->slots; i++) {
        zend_llist_add_element(&node->slots, &cm->slot[i]);
    }

    return node;
}

/* Destroy a persistent cached cluster */
PHP_REDIS_API void cluster_cache_free(redisCachedCluster *rcc) {
    size_t i;

    /* Free masters */
    for (i = 0; i < rcc->count; i++) {
        cluster_free_cached_master(&rcc->master[i]);
    }

    zend_string_release(rcc->hash);
    pefree(rcc->master, 1);
    pefree(rcc, 1);
}

/* Initialize cluster from cached slots */
PHP_REDIS_API
void cluster_init_cache(redisCluster *c, redisCachedCluster *cc) {
    RedisSock *sock;
    redisClusterNode *mnode, *slave;
    redisCachedMaster *cm;
    char key[HOST_NAME_MAX];
    size_t keylen, i, j, s;
    int *map;

    /* Randomize seeds */
    map = emalloc(sizeof(*map) * cc->count);
    for (i = 0; i < cc->count; i++) map[i] = i;
    fyshuffle(map, cc->count);

    /* Duplicate the hash key so we can invalidate when redirected */
    c->cache_key = zend_string_copy(cc->hash);

    /* Iterate over masters */
    for (i = 0; i < cc->count; i++) {
        /* Grab the next master */
        cm = &cc->master[map[i]];

        /* Hash our host and port */
        keylen = snprintf(key, sizeof(key), "%s:%u", ZSTR_VAL(cm->host.addr), cm->host.port);

        /* Create socket */
        sock = redis_sock_create(ZSTR_VAL(cm->host.addr), ZSTR_LEN(cm->host.addr), cm->host.port,
                                 c->flags->timeout, c->flags->read_timeout, c->flags->persistent,
                                 NULL, 0);

        /* Stream context */
        sock->stream_ctx = c->flags->stream_ctx;

        /* Add to seed nodes */
        zend_hash_str_update_ptr(c->seeds, key, keylen, sock);

        /* Create master node */
        mnode = cached_master_clone(c, cm);

        /* Add our master */
        zend_hash_str_update_ptr(c->nodes, key, keylen, mnode);

        /* Attach any slaves */
        for (s = 0; s < cm->slaves; s++) {
            zend_string *host = cm->slave[s].addr;
            slave = cluster_node_create(c, ZSTR_VAL(host), ZSTR_LEN(host), cm->slave[s].port, 0, 1);
            cluster_node_add_slave(mnode, slave);
        }

        /* Hook up direct slot access */
        for (j = 0; j < cm->slots; j++) {
            for (s = cm->slot[j].low; s <= cm->slot[j].high; s++) {
                c->master[s] = mnode;
            }
        }
    }

    efree(map);
}

/* Initialize seeds.  By the time we get here we've already validated our
 * seeds array and know we have a non-empty array of strings all in
 * host:port format. */
PHP_REDIS_API void
cluster_init_seeds(redisCluster *c, zend_string **seeds, uint32_t nseeds)
{
    RedisSock *sock;
    char *seed, *sep, key[1024];
    int key_len, i, *map;

    /* Get a randomized order to hit our seeds */
    map = ecalloc(nseeds, sizeof(*map));
    for (i = 0; i < nseeds; i++) map[i] = i;
    fyshuffle(map, nseeds);

    for (i = 0; i < nseeds; i++) {
        seed = ZSTR_VAL(seeds[map[i]]);

        sep = strrchr(seed, ':');
        ZEND_ASSERT(sep != NULL);

        // Allocate a structure for this seed
        sock = redis_sock_create(seed, sep - seed, atoi(sep + 1),
                                 c->flags->timeout, c->flags->read_timeout,
                                 c->flags->persistent, NULL, 0);

        /* Stream context */
        sock->stream_ctx = c->flags->stream_ctx;

        /* Credentials */
        redis_sock_set_auth(sock, c->flags->user, c->flags->pass);

        // Index this seed by host/port
        key_len = snprintf(key, sizeof(key), "%s:%u", ZSTR_VAL(sock->host),
            sock->port);

        // Add to our seed HashTable
        zend_hash_str_update_ptr(c->seeds, key, key_len, sock);
    }

    efree(map);
}

/* Initial mapping of our cluster keyspace */
PHP_REDIS_API int cluster_map_keyspace(redisCluster *c) {
    RedisSock *seed;
    clusterReply *slots = NULL;
    int mapped = 0;

    // Iterate over seeds until we can get slots
    ZEND_HASH_FOREACH_PTR(c->seeds, seed) {
        // Attempt to connect to this seed node
        if (seed == NULL || redis_sock_server_open(seed) != SUCCESS) {
            continue;
        }

        // Parse out cluster nodes.  Flag mapped if we are valid
        slots = cluster_get_slots(seed);
        if (slots) {
            mapped = !cluster_map_slots(c, slots);
            // Bin anything mapped, if we failed somewhere
            if (!mapped) {
                memset(c->master, 0, sizeof(redisClusterNode*)*REDIS_CLUSTER_SLOTS);
            }
        }
        redis_sock_disconnect(seed, 0);
        if (mapped) break;
    } ZEND_HASH_FOREACH_END();

    // Clean up slots reply if we got one
    if (slots) cluster_free_reply(slots, 1);

    // Throw an exception if we couldn't map
    if (!mapped) {
        CLUSTER_THROW_EXCEPTION("Couldn't map cluster keyspace using any provided seed", 0);
        return FAILURE;
    }

    return SUCCESS;
}

/* Parse the MOVED OR ASK redirection payload when we get such a response
 * and apply this information to our cluster.  If we encounter a parse error
 * nothing in the cluster will be modified, and -1 is returned. */
static int cluster_set_redirection(redisCluster* c, char *msg, int moved)
{
    char *host, *port;

    /* The Redis Cluster specification suggests clients do not update
     * their slot mapping for an ASK redirection, only for MOVED */
    if (moved) c->redirections++;

    /* Move past "MOVED" or "ASK */
    msg += moved ? MOVED_LEN : ASK_LEN;

    /* Make sure we can find host */
    if ((host = strchr(msg, ' ')) == NULL) return -1;
    *host++ = '\0';

    /* Find port, searching right to left in case of IPv6 */
    if ((port = strrchr(host, ':')) == NULL) return -1;
    *port++ = '\0';

    // Success, apply it
    c->redir_type = moved ? REDIR_MOVED : REDIR_ASK;
    strncpy(c->redir_host, host, sizeof(c->redir_host) - 1);
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
static int cluster_check_response(redisCluster *c, REDIS_REPLY_TYPE *reply_type)
{
    size_t sz;

    // Clear out any prior error state and our last line response
    CLUSTER_CLEAR_ERROR(c);
    CLUSTER_CLEAR_REPLY(c);

    if (-1 == redis_check_eof(c->cmd_sock, 1, 1) ||
       EOF == (*reply_type = redis_sock_getc(c->cmd_sock)))
    {
        return -1;
    }

    // In the event of an ERROR, check if it's a MOVED/ASK error
    if (*reply_type == TYPE_ERR) {
        char inbuf[4096];
        size_t nbytes;
        int moved;

        // Attempt to read the error
        if (!redis_sock_get_line(c->cmd_sock, inbuf, sizeof(inbuf), &nbytes)) {
            return -1;
        }

        // Check for MOVED or ASK redirection
        if ((moved = IS_MOVED(inbuf)) || IS_ASK(inbuf)) {
            /* Make sure we can parse the redirection host and port */
            return !cluster_set_redirection(c, inbuf, moved) ? 1 : -1;
        }
        // Capture the error string Redis returned
        cluster_set_err(c, inbuf, strlen(inbuf)-2);
        return 0;
    }

    // Fetch the first line of our response from Redis.
    if (redis_sock_gets(c->cmd_sock,c->line_reply,sizeof(c->line_reply),
                       &sz) < 0)
    {
        return -1;
    }

    // For replies that will give us a numeric length, convert it
    if (*reply_type != TYPE_LINE) {
        c->reply_len = strtol(c->line_reply, NULL, 10);
    } else {
        c->reply_len = (long long)sz;
    }

    // Clear out any previous error, and return that the data is here
    CLUSTER_CLEAR_ERROR(c);
    return 0;
}

/* Disconnect from each node we're connected to */
PHP_REDIS_API void cluster_disconnect(redisCluster *c, int force) {
    redisClusterNode *node, *slave;

    ZEND_HASH_FOREACH_PTR(c->nodes, node) {
        if (node == NULL) continue;

        /* Disconnect from the master */
        redis_sock_disconnect(node->sock, force);

        /* We also want to disconnect any slave connections so they will be pooled
         * in the event we are using persistent connections and connection pooling. */
        if (node->slaves) {
            ZEND_HASH_FOREACH_PTR(node->slaves, slave) {
                redis_sock_disconnect(slave->sock, force);
            } ZEND_HASH_FOREACH_END();
        }
    } ZEND_HASH_FOREACH_END();
}

/* This method attempts to write our command at random to the master and any
 * attached slaves, until we either successufly do so, or fail. */
static int cluster_dist_write(redisCluster *c, const char *cmd, size_t sz,
                              int nomaster)
{
    int i, count = 1, *nodes;
    RedisSock *redis_sock;

    /* Determine our overall node count */
    if (c->master[c->cmd_slot]->slaves) {
        count += zend_hash_num_elements(c->master[c->cmd_slot]->slaves);
    }

    /* Allocate memory for master + slaves or just slaves */
    nodes = emalloc(sizeof(int)*count);

    /* Populate our array with the master and each of it's slaves, then
     * randomize them, so we will pick from the master or some slave.  */
    for (i = 0; i < count; i++) nodes[i] = i;
    fyshuffle(nodes, count);

    /* Iterate through our nodes until we find one we can write to or fail */
    for (i = 0; i < count; i++) {
        /* Skip if this is the master node and we don't want to query that */
        if (nomaster && nodes[i] == 0)
           continue;

        /* Get the slave for this index */
        redis_sock = cluster_slot_sock(c, c->cmd_slot, nodes[i]);
        if (!redis_sock) continue;

        /* If we're not on the master, attempt to send the READONLY command to
         * this slave, and skip it if that fails */
        if (nodes[i] == 0 || cluster_send_readonly(redis_sock) == 0) {
            /* Attempt to send the command */
            if (CLUSTER_SEND_PAYLOAD(redis_sock, cmd, sz)) {
                c->cmd_sock = redis_sock;
                efree(nodes);
                return 0;
            }
        }
    }

    /* Clean up our shuffled array */
    efree(nodes);

    /* Couldn't send to the master or any slave */
    return -1;
}

/* Attempt to write our command to the current c->cmd_sock socket.  For write
 * commands, we attempt to query the master for this slot, and in the event of
 * a failure, try to query every remaining node for a redirection.
 *
 * If we're issuing a readonly command, we use one of three strategies, depending
 * on our redisCluster->failover setting.
 *
 * REDIS_FAILOVER_NONE:
 *   The command is treated just like a write command, and will only be executed
 *   against the known master for this slot.
 * REDIS_FAILOVER_ERROR:
 *   If we're unable to communicate with this slot's master, we attempt the query
 *   against any slaves (at random) that this master has.
 * REDIS_FAILOVER_DISTRIBUTE:
 *   We pick at random from the master and any slaves it has.  This option will
 *   load balance between masters and slaves
 * REDIS_FAILOVER_DISTRIBUTE_SLAVES:
 *   We pick at random from slave nodes of a given master.  This option is
 *   used to load balance read queries against N slaves.
 *
 * Once we are able to find a node we can write to, we check for MOVED or
 * ASKING redirection, such that the keyspace can be updated.
*/
static int cluster_sock_write(redisCluster *c, const char *cmd, size_t sz,
                              int direct)
{
    redisClusterNode *seed_node;
    RedisSock *redis_sock;
    int failover, nomaster;

    /* First try the socket requested */
    redis_sock = c->cmd_sock;

    /* Readonly is irrelevant if we're not configured to failover */
    failover = c->readonly && c->failover != REDIS_FAILOVER_NONE ?
        c->failover : REDIS_FAILOVER_NONE;

    /* If in ASK redirection, get/create the node for that host:port, otherwise
     * just use the command socket. */
    if (c->redir_type == REDIR_ASK) {
        if (cluster_send_asking(c->cmd_sock) < 0) {
            return -1;
        }
    }

    /* Attempt to send our command payload to the cluster.  If we're not set up
     * to failover, just try the master.  If we're configured to failover on
     * error, try the master and then fall back to any slaves.  When we're set
     * up to distribute the commands, try to write to any node on this slot
     * at random. */
    if (failover == REDIS_FAILOVER_NONE) {
        /* Success if we can send our payload to the master */
        if (CLUSTER_SEND_PAYLOAD(redis_sock, cmd, sz)) return 0;
    } else if (failover == REDIS_FAILOVER_ERROR) {
        /* Try the master, then fall back to any slaves we may have */
        if (CLUSTER_SEND_PAYLOAD(redis_sock, cmd, sz) ||
           !cluster_dist_write(c, cmd, sz, 1)) return 0;
    } else {
        /* Include or exclude master node depending on failover option and
         * attempt to make our write */
        nomaster = failover == REDIS_FAILOVER_DISTRIBUTE_SLAVES;
        if (!cluster_dist_write(c, cmd, sz, nomaster)) {
            /* We were able to write to a master or slave at random */
            return 0;
        }
    }

    /* Don't fall back if direct communication with this slot is required. */
    if (direct) return -1;

    /* Fall back by attempting the request against every known node */
    ZEND_HASH_FOREACH_PTR(c->nodes, seed_node) {
        /* Skip this node if it's the one that failed, or if it's a slave */
        if (seed_node == NULL || seed_node->sock == redis_sock || seed_node->slave) continue;

        /* Connect to this node if we haven't already and attempt to write our request to this node */
        if (CLUSTER_SEND_PAYLOAD(seed_node->sock, cmd, sz)) {
            c->cmd_slot = seed_node->slot;
            c->cmd_sock = seed_node->sock;
            return 0;
        }
    } ZEND_HASH_FOREACH_END();

    /* We were unable to write to any node in our cluster */
    return -1;
}

/* Helper to find if we've got a host:port mapped in our cluster nodes. */
static redisClusterNode *cluster_find_node(redisCluster *c, const char *host,
                                           unsigned short port)
{
    int key_len;
    char key[1024];

    key_len = snprintf(key,sizeof(key),"%s:%d", host, port);

    return zend_hash_str_find_ptr(c->nodes, key, key_len);
}

/* Provided a redisCluster object, the slot where we thought data was and
 * the slot where data was moved, update our node mapping */
static void cluster_update_slot(redisCluster *c) {
    redisClusterNode *node;
    char key[1024];
    size_t klen;

    /* Do we already have the new slot mapped */
    if (c->master[c->redir_slot]) {
        /* No need to do anything if it's the same node */
        if (!CLUSTER_REDIR_CMP(c, SLOT_SOCK(c,c->redir_slot))) {
            return;
        }

        /* Check to see if we have this new node mapped */
        node = cluster_find_node(c, c->redir_host, c->redir_port);

        if (node) {
            /* Just point to this slot */
            c->master[c->redir_slot] = node;
        } else {
            /* If the redirected node is a replica of the previous slot owner, a failover has taken place.
            We must then remap the cluster's keyspace in order to update the cluster's topology. */
            redisClusterNode *prev_master = SLOT(c,c->redir_slot);
            redisClusterNode *slave;
            ZEND_HASH_FOREACH_PTR(prev_master->slaves, slave) {
                if (slave == NULL) {
                    continue;
                }
                if (!CLUSTER_REDIR_CMP(c, slave->sock)) {
                    // Detected a failover, the redirected node was a replica
                    // Remap the cluster's keyspace
                    cluster_map_keyspace(c);
                    return;
                }
            } ZEND_HASH_FOREACH_END();

            /* Create our node */
            node = cluster_node_create(c, c->redir_host, c->redir_host_len,
                c->redir_port, c->redir_slot, 0);

            /* Our node is new, so keep track of it for cleanup */
            klen = snprintf(key, sizeof(key), "%s:%d", c->redir_host, c->redir_port);
            zend_hash_str_update_ptr(c->nodes, key, klen, node);

            /* Now point our slot at the node */
            c->master[c->redir_slot] = node;
        }
    } else {
        /* Check to see if the ip and port are mapped */
        node = cluster_find_node(c, c->redir_host, c->redir_port);
        if (!node) {
            node = cluster_node_create(c, c->redir_host, c->redir_host_len,
                c->redir_port, c->redir_slot, 0);
        }

        /* Map the slot to this node */
        c->master[c->redir_slot] = node;
    }

    /* Update slot inside of node, so it can be found for command sending */
    node->slot = c->redir_slot;

    /* Make sure we unflag this node as a slave, as Redis Cluster will only ever
     * direct us to master nodes. */
    node->slave = 0;
}

/* Abort any transaction in process, by sending DISCARD to any nodes that
 * have active transactions in progress.  If we can't send DISCARD, we need
 * to disconnect as it would leave us in an undefined state. */
PHP_REDIS_API int cluster_abort_exec(redisCluster *c) {
    clusterFoldItem *fi = c->multi_head;

    /* Loop through our fold items */
    while (fi) {
        if (SLOT_SOCK(c,fi->slot)->mode == MULTI) {
            if (cluster_send_discard(c, fi->slot) < 0) {
                cluster_disconnect(c, 0);
                return -1;
            }
            SLOT_SOCK(c,fi->slot)->mode = ATOMIC;
            SLOT_SOCK(c,fi->slot)->watching = 0;
        }
        fi = fi->next;
    }

    /* Update our overall cluster state */
    c->flags->mode = ATOMIC;

    /* Success */
    return 0;
}

/* Iterate through our slots, looking for the host/port in question.  This
 * should perform well enough as in almost all situations, a few or a few
 * dozen servers will map all the slots */
PHP_REDIS_API short cluster_find_slot(redisCluster *c, const char *host,
                               unsigned short port)
{
    int i;

    for (i = 0; i < REDIS_CLUSTER_SLOTS; i++) {
        if (c->master[i] && c->master[i]->sock &&
           c->master[i]->sock->port == port &&
           !strcasecmp(ZSTR_VAL(c->master[i]->sock->host), host))
        {
            return i;
        }
    }

    // We didn't find it
    return -1;
}

/* Send a command to a specific slot */
PHP_REDIS_API int cluster_send_slot(redisCluster *c, short slot, char *cmd,
                             int cmd_len, REDIS_REPLY_TYPE rtype)
{
    /* Point our cluster to this slot and it's socket */
    c->cmd_slot = slot;
    c->cmd_sock = SLOT_SOCK(c, slot);

    /* Enable multi mode on this slot if we've been directed to but haven't
     * send it to this node yet */
    if (c->flags->mode == MULTI && c->cmd_sock->mode != MULTI) {
        if (cluster_send_multi(c, slot) == -1) {
            CLUSTER_THROW_EXCEPTION("Unable to enter MULTI mode on requested slot", 0);
            return -1;
        }
    }

    /* Try the slot */
    if (cluster_sock_write(c, cmd, cmd_len, 1) == -1) {
        return -1;
    }
    c->flags->txBytes += cmd_len;

    /* Check our response */
    if (cluster_check_response(c, &c->reply_type) != 0 ||
       (rtype != TYPE_EOF && rtype != c->reply_type)) return -1;

    /* Success */
    return 0;
}

/* Send a command to given slot in our cluster.  If we get a MOVED or ASK error
 * we attempt to send the command to the node as directed. */
PHP_REDIS_API short cluster_send_command(redisCluster *c, short slot, const char *cmd,
                                         int cmd_len)
{
    int resp, timedout = 0;
    long msstart;

    if (!SLOT(c, slot)) {
        zend_throw_exception_ex(redis_cluster_exception_ce, 0,
            "The slot %d is not covered by any node in this cluster", slot);
        return -1;
    }
    /* Set the slot we're operating against as well as it's socket.  These can
     * change during our request loop if we have a master failure and are
     * configured to fall back to slave nodes, or if we have to fall back to
     * a different slot due to no nodes serving this slot being reachable. */
    c->cmd_slot = slot;
    c->cmd_sock = SLOT_SOCK(c, slot);

    /* Get the current time in milliseconds to handle any timeout */
    msstart = mstime();

    /* Our main cluster request/reply loop.  This loop runs until we're able to
     * get a valid reply from a node, hit our "request" timeout, or encounter a
     * CLUSTERDOWN state from Redis Cluster. */
    do {
        /* Send MULTI to the socket if we're in MULTI mode but haven't yet */
        if (c->flags->mode == MULTI && c->cmd_sock->mode != MULTI) {
            /* We have to fail if we can't send MULTI to the node */
            if (cluster_send_multi(c, slot) == -1) {
                CLUSTER_THROW_EXCEPTION("Unable to enter MULTI mode on requested slot", 0);
                return -1;
            }
        }

        /* Attempt to deliver our command to the node, and that failing, to any
         * node until we find one that is available. */
        if (cluster_sock_write(c, cmd, cmd_len, 0) == -1) {
            /* We have to abort, as no nodes are reachable */
            CLUSTER_THROW_EXCEPTION("Can't communicate with any node in the cluster", 0);
            return -1;
        }

        /* Check response and short-circuit on success or communication error */
        resp = cluster_check_response(c, &c->reply_type);
        if (resp <= 0) {
            break;
        }

        /* Handle MOVED or ASKING redirection */
        if (resp == 1) {
           /* Abort if we're in a transaction as it will be invalid */
           if (c->flags->mode == MULTI) {
               CLUSTER_THROW_EXCEPTION("Can't process MULTI sequence when cluster is resharding", 0);
               return -1;
           }

           if (c->redir_type == REDIR_MOVED) {
               /* For MOVED redirection we want to update our cached mapping */
               cluster_update_slot(c);
               c->cmd_sock = SLOT_SOCK(c, slot);
           } else if (c->redir_type == REDIR_ASK) {
               /* For ASK redirection we want to redirect but not update slot mapping */
               c->cmd_sock = cluster_get_asking_sock(c);
           }
        }

        /* See if we've timed out in the command loop */
        timedout = c->waitms ? mstime() - msstart >= c->waitms : 0;
    } while (!c->clusterdown && !timedout);

    // If we've detected the cluster is down, throw an exception
    if (c->clusterdown) {
        CLUSTER_THROW_EXCEPTION("The Redis Cluster is down (CLUSTERDOWN)", 0);
        return -1;
    } else if (timedout || resp == -1) {
        // Make sure the socket is reconnected, it such that it is in a clean state
        redis_sock_disconnect(c->cmd_sock, 1);

        if (timedout) {
            CLUSTER_THROW_EXCEPTION("Timed out attempting to find data in the correct node!", 0);
        } else {
            CLUSTER_THROW_EXCEPTION("Error processing response from Redis node!", 0);
        }

        return -1;
    }

    /* Clear redirection flag */
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
 * consumed. */

/* RAW bulk response handler */
PHP_REDIS_API void cluster_bulk_raw_resp(INTERNAL_FUNCTION_PARAMETERS,
                                  redisCluster *c, void *ctx)
{
    char *resp;

    // Make sure we can read the response
    if (c->reply_type != TYPE_BULK ||
       (resp = redis_sock_read_bulk_reply(c->cmd_sock, c->reply_len)) == NULL)
    {
        if (c->flags->mode != MULTI) {
            RETURN_FALSE;
        } else {
            add_next_index_bool(&c->multi_resp, 0);
            return;
        }
    }

    // Return our response raw
    CLUSTER_RETURN_STRING(c, resp, c->reply_len);
    efree(resp);
}

PHP_REDIS_API void
cluster_single_line_resp(INTERNAL_FUNCTION_PARAMETERS, redisCluster *c, void *ctx)
{
    char *p;

    /* Cluster already has the reply so abort if this isn't a LINE response *or* if for
     * some freaky reason we don't detect a null terminator */
    if (c->reply_type != TYPE_LINE || !(p = memchr(c->line_reply,'\0',sizeof(c->line_reply)))) {
        CLUSTER_RETURN_FALSE(c);
    }

    if (CLUSTER_IS_ATOMIC(c)) {
        CLUSTER_RETURN_STRING(c, c->line_reply, p - c->line_reply);
    } else {
        add_next_index_stringl(&c->multi_resp, c->line_reply, p - c->line_reply);
    }
}

/* BULK response handler */
PHP_REDIS_API void cluster_bulk_resp(INTERNAL_FUNCTION_PARAMETERS, redisCluster *c,
                              void *ctx)
{
    char *resp;

    // Make sure we can read the response
    if (c->reply_type != TYPE_BULK ||
       (resp = redis_sock_read_bulk_reply(c->cmd_sock, c->reply_len)) == NULL)
    {
        CLUSTER_RETURN_FALSE(c);
    }

    if (CLUSTER_IS_ATOMIC(c)) {
        if (!redis_unpack(c->flags, resp, c->reply_len, return_value)) {
            CLUSTER_RETURN_STRING(c, resp, c->reply_len);
        }
    } else {
        zval z_unpacked;
        if (redis_unpack(c->flags, resp, c->reply_len, &z_unpacked)) {
            add_next_index_zval(&c->multi_resp, &z_unpacked);
        } else {
            add_next_index_stringl(&c->multi_resp, resp, c->reply_len);
        }
    }
    efree(resp);
}

/* Bulk response where we expect a double */
PHP_REDIS_API void cluster_dbl_resp(INTERNAL_FUNCTION_PARAMETERS, redisCluster *c,
                             void *ctx)
{
    char *resp;
    double dbl;

    // Make sure we can read the response
    if (c->reply_type != TYPE_BULK ||
       (resp = redis_sock_read_bulk_reply(c->cmd_sock, c->reply_len)) == NULL)
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
PHP_REDIS_API void cluster_bool_resp(INTERNAL_FUNCTION_PARAMETERS, redisCluster *c,
                              void *ctx)
{
    // Check that we have +OK
    if (c->reply_type != TYPE_LINE || c->reply_len != 2 ||
       c->line_reply[0] != 'O' || c->line_reply[1] != 'K')
    {
        CLUSTER_RETURN_FALSE(c);
    }

    CLUSTER_RETURN_BOOL(c, 1);
}

/* Boolean response, specialized for PING */
PHP_REDIS_API void cluster_ping_resp(INTERNAL_FUNCTION_PARAMETERS, redisCluster *c,
                              void *ctx)
{
    if (c->reply_type != TYPE_LINE || c->reply_len != 4 ||
       memcmp(c->line_reply,"PONG",sizeof("PONG")-1))
    {
        CLUSTER_RETURN_FALSE(c);
    }

    CLUSTER_RETURN_BOOL(c, 1);
}

PHP_REDIS_API void
cluster_pop_resp(INTERNAL_FUNCTION_PARAMETERS, redisCluster *c, void *ctx)
{
    if (ctx == NULL) {
        cluster_bulk_resp(INTERNAL_FUNCTION_PARAM_PASSTHRU, c, NULL);
    } else if (ctx == PHPREDIS_CTX_PTR) {
        cluster_mbulk_raw_resp(INTERNAL_FUNCTION_PARAM_PASSTHRU, c, NULL);
    } else {
        ZEND_ASSERT(!"memory corruption?");
    }
}

PHP_REDIS_API void
cluster_set_resp(INTERNAL_FUNCTION_PARAMETERS, redisCluster *c, void *ctx)
{
    if (ctx == NULL) {
        cluster_bool_resp(INTERNAL_FUNCTION_PARAM_PASSTHRU, c, NULL);
    } else {
        cluster_bulk_resp(INTERNAL_FUNCTION_PARAM_PASSTHRU, c, NULL);
    }
}

/* 1 or 0 response, for things like SETNX */
PHP_REDIS_API void cluster_1_resp(INTERNAL_FUNCTION_PARAMETERS, redisCluster *c,
                           void *ctx)
{
    // Validate our reply type, and check for a zero
    if (c->reply_type != TYPE_INT || c->reply_len == 0) {
        CLUSTER_RETURN_FALSE(c);
    }

    CLUSTER_RETURN_BOOL(c, 1);
}

/* Generic integer response */
PHP_REDIS_API void cluster_long_resp(INTERNAL_FUNCTION_PARAMETERS, redisCluster *c,
                              void *ctx)
{
    if (c->reply_type != TYPE_INT) {
        CLUSTER_RETURN_FALSE(c);
    }
    CLUSTER_RETURN_LONG(c, c->reply_len);
}

/* TYPE response handler */
PHP_REDIS_API void cluster_type_resp(INTERNAL_FUNCTION_PARAMETERS, redisCluster *c,
                              void *ctx)
{
    // Make sure we got the right kind of response
    if (c->reply_type != TYPE_LINE) {
        CLUSTER_RETURN_FALSE(c);
    }

    // Switch on the type
    if (strncmp (c->line_reply, "string", 6) == 0) {
        CLUSTER_RETURN_LONG(c, REDIS_STRING);
    } else if (strncmp(c->line_reply, "set", 3) == 0) {
        CLUSTER_RETURN_LONG(c, REDIS_SET);
    } else if (strncmp(c->line_reply, "list", 4) == 0) {
        CLUSTER_RETURN_LONG(c, REDIS_LIST);
    } else if (strncmp(c->line_reply, "hash", 4) == 0) {
        CLUSTER_RETURN_LONG(c, REDIS_HASH);
    } else if (strncmp(c->line_reply, "zset", 4) == 0) {
        CLUSTER_RETURN_LONG(c, REDIS_ZSET);
    } else if (strncmp(c->line_reply, "stream", 6) == 0) {
        CLUSTER_RETURN_LONG(c, REDIS_STREAM);
    } else {
        CLUSTER_RETURN_LONG(c, REDIS_NOT_FOUND);
    }
}

/* SUBSCRIBE/PSCUBSCRIBE handler */
PHP_REDIS_API void cluster_sub_resp(INTERNAL_FUNCTION_PARAMETERS, redisCluster *c,
                             void *ctx)
{
    subscribeContext *sctx = (subscribeContext*)ctx;
    zval z_tab, *z_tmp;
    int pull = 0;


    // Consume each MULTI BULK response (one per channel/pattern)
    while (sctx->argc--) {
        if (!cluster_zval_mbulk_resp(INTERNAL_FUNCTION_PARAM_PASSTHRU, c,
            pull, mbulk_resp_loop_raw, &z_tab)
        ) {
            efree(sctx);
            RETURN_FALSE;
        }

        if ((z_tmp = zend_hash_index_find(Z_ARRVAL(z_tab), 0)) == NULL ||
            strcasecmp(Z_STRVAL_P(z_tmp), sctx->kw) != 0
        ) {
            zval_dtor(&z_tab);
            efree(sctx);
            RETURN_FALSE;
        }

        zval_dtor(&z_tab);
        pull = 1;
    }

    // Set up our callback pointers
    zval z_ret, z_args[4];
    sctx->cb.fci.retval = &z_ret;
    sctx->cb.fci.params = z_args;

    /* We're in a subscribe loop */
    c->subscribed_slot = c->cmd_slot;

    /* Multibulk response, {[pattern], type, channel, payload} */
    while (1) {
        /* Arguments */
        zval *z_type, *z_chan, *z_pat = NULL, *z_data;
        int tab_idx = 1, is_pmsg;

        // Get the next subscribe response
        if (!cluster_zval_mbulk_resp(INTERNAL_FUNCTION_PARAM_PASSTHRU, c, 1, mbulk_resp_loop, &z_tab) ||
            (z_type = zend_hash_index_find(Z_ARRVAL(z_tab), 0)) == NULL
        ) {
            break;
        }

        // Make sure we have a message or pmessage
        if (!strncmp(Z_STRVAL_P(z_type), "message", 7) ||
            !strncmp(Z_STRVAL_P(z_type), "pmessage", 8)
        ) {
            is_pmsg = *Z_STRVAL_P(z_type) == 'p';
        } else {
            zval_dtor(&z_tab);
            continue;
        }

        if (is_pmsg && (z_pat = zend_hash_index_find(Z_ARRVAL(z_tab), tab_idx++)) == NULL) {
            break;
        }

        // Extract channel and data
        if ((z_chan = zend_hash_index_find(Z_ARRVAL(z_tab), tab_idx++)) == NULL ||
           (z_data = zend_hash_index_find(Z_ARRVAL(z_tab), tab_idx++)) == NULL
        ) {
            break;
        }

        // Always pass our object through
        z_args[0] = *getThis();

        // Set up calbacks depending on type
        if (is_pmsg) {
            z_args[1] = *z_pat;
            z_args[2] = *z_chan;
            z_args[3] = *z_data;
        } else {
            z_args[1] = *z_chan;
            z_args[2] = *z_data;
        }

        // Set arg count
        sctx->cb.fci.param_count = tab_idx;

        // Execute our callback
        if (zend_call_function(&sctx->cb.fci, &sctx->cb.fci_cache) != SUCCESS) {
            break;
        }

        // If we have a return value, free it
        zval_ptr_dtor(&z_ret);

        zval_dtor(&z_tab);
    }

    // We're no longer subscribing, due to an error
    c->subscribed_slot = -1;

    // Cleanup
    zval_dtor(&z_tab);
    efree(sctx);

    // Failure
    RETURN_FALSE;
}

/* UNSUBSCRIBE/PUNSUBSCRIBE */
PHP_REDIS_API void cluster_unsub_resp(INTERNAL_FUNCTION_PARAMETERS,
                               redisCluster *c, void *ctx)
{
    subscribeContext *sctx = (subscribeContext*)ctx;
    zval z_tab, *z_chan, *z_flag;
    int pull = 0, argc = sctx->argc;

    efree(sctx);
    array_init(return_value);

    // Consume each response
    while (argc--) {
        // Fail if we didn't get an array or can't find index 1
        if (!cluster_zval_mbulk_resp(INTERNAL_FUNCTION_PARAM_PASSTHRU, c, pull, mbulk_resp_loop_raw, &z_tab) ||
            (z_chan = zend_hash_index_find(Z_ARRVAL(z_tab), 1)) == NULL
        ) {
            zval_dtor(&z_tab);
            zval_dtor(return_value);
            RETURN_FALSE;
        }

        // Find the flag for this channel/pattern
        if ((z_flag = zend_hash_index_find(Z_ARRVAL(z_tab), 2)) == NULL ||
            Z_STRLEN_P(z_flag) != 2
        ) {
            zval_dtor(&z_tab);
            zval_dtor(return_value);
            RETURN_FALSE;
        }

        // Redis will give us either :1 or :0 here
        char *flag = Z_STRVAL_P(z_flag);

        // Add result
        add_assoc_bool(return_value, Z_STRVAL_P(z_chan), flag[1] == '1');

        zval_dtor(&z_tab);
        pull = 1;
    }
}

/* Recursive MULTI BULK -> PHP style response handling */
static void cluster_mbulk_variant_resp(clusterReply *r, int null_mbulk_as_null,
                                       zval *z_ret)
{
    zval z_sub_ele;
    long long i;

    switch(r->type) {
        case TYPE_INT:
            add_next_index_long(z_ret, r->integer);
            break;
        case TYPE_LINE:
            if (r->str) {
                add_next_index_stringl(z_ret, r->str, r->len);
            } else {
                add_next_index_bool(z_ret, 1);
            }
            break;
        case TYPE_BULK:
            if (r->len > -1) {
                add_next_index_stringl(z_ret, r->str, r->len);
            } else {
                add_next_index_null(z_ret);
            }
            break;
        case TYPE_MULTIBULK:
            if (r->elements < 0 && null_mbulk_as_null) {
                add_next_index_null(z_ret);
            } else {
                array_init(&z_sub_ele);
                for (i = 0; i < r->elements; i++) {
                    cluster_mbulk_variant_resp(r->element[i], null_mbulk_as_null, &z_sub_ele);
                }
                add_next_index_zval(z_ret, &z_sub_ele);
            }
            break;
        default:
            add_next_index_bool(z_ret, 0);
            break;
    }
}

/* Variant response handling, for things like EVAL and various other responses
 * where we just map the replies from Redis type values to PHP ones directly. */
static void
cluster_variant_resp_generic(INTERNAL_FUNCTION_PARAMETERS, redisCluster *c,
                             int status_strings, void *ctx)
{
    clusterReply *r;
    zval zv, *z_arr = &zv;
    long long i;

    // Make sure we can read it
    if ((r = cluster_read_resp(c, status_strings)) == NULL) {
        CLUSTER_RETURN_FALSE(c);
    }

    // Handle ATOMIC vs. MULTI mode in a separate switch
    if (CLUSTER_IS_ATOMIC(c)) {
        switch(r->type) {
            case TYPE_INT:
                RETVAL_LONG(r->integer);
                break;
            case TYPE_ERR:
                RETVAL_FALSE;
                break;
            case TYPE_LINE:
                if (status_strings) {
                    RETVAL_STRINGL(r->str, r->len);
                } else {
                    RETVAL_TRUE;
                }
                break;
            case TYPE_BULK:
                if (r->len < 0) {
                    RETVAL_NULL();
                } else {
                    RETVAL_STRINGL(r->str, r->len);
                }
                break;
            case TYPE_MULTIBULK:
                if (r->elements < 0 && c->flags->null_mbulk_as_null) {
                    RETVAL_NULL();
                } else {
                    array_init(z_arr);
                    for (i = 0; i < r->elements; i++) {
                        cluster_mbulk_variant_resp(r->element[i], c->flags->null_mbulk_as_null, z_arr);
                    }
                    RETVAL_ZVAL(z_arr, 0, 0);
                }
                break;
            default:
                RETVAL_FALSE;
                break;
        }
    } else {
        switch(r->type) {
            case TYPE_INT:
                add_next_index_long(&c->multi_resp, r->integer);
                break;
            case TYPE_ERR:
                add_next_index_bool(&c->multi_resp, 0);
                break;
            case TYPE_LINE:
                if (status_strings) {
                    add_next_index_stringl(&c->multi_resp, r->str, r->len);
                } else {
                    add_next_index_bool(&c->multi_resp, 1);
                }
                break;
            case TYPE_BULK:
                if (r->len < 0) {
                    add_next_index_null(&c->multi_resp);
                } else {
                    add_next_index_stringl(&c->multi_resp, r->str, r->len);
                }
                break;
            case TYPE_MULTIBULK:
                if (r->elements < 0 && c->flags->null_mbulk_as_null) {
                    add_next_index_null(&c->multi_resp);
                } else {
                    cluster_mbulk_variant_resp(r, c->flags->null_mbulk_as_null, &c->multi_resp);
                }
                break;
            default:
                add_next_index_bool(&c->multi_resp, 0);
                break;
        }
    }

    // Free our response structs, but not allocated data itself
    cluster_free_reply(r, 1);
}

PHP_REDIS_API void
cluster_zrange_resp(INTERNAL_FUNCTION_PARAMETERS, redisCluster *c, void *ctx) {
    cluster_cb cb;

    ZEND_ASSERT(ctx == NULL || ctx == PHPREDIS_CTX_PTR);

    cb = ctx ? cluster_mbulk_zipdbl_resp : cluster_mbulk_resp;

    cb(INTERNAL_FUNCTION_PARAM_PASSTHRU, c, ctx);
}

PHP_REDIS_API void cluster_variant_resp(INTERNAL_FUNCTION_PARAMETERS, redisCluster *c,
                                        void *ctx)
{
    cluster_variant_resp_generic(INTERNAL_FUNCTION_PARAM_PASSTHRU, c, 0, ctx);
}

PHP_REDIS_API void cluster_variant_raw_resp(INTERNAL_FUNCTION_PARAMETERS, redisCluster *c,
                                            void *ctx)
{
    cluster_variant_resp_generic(INTERNAL_FUNCTION_PARAM_PASSTHRU, c,
                                 c->flags->reply_literal, ctx);
}

PHP_REDIS_API void cluster_variant_resp_strings(INTERNAL_FUNCTION_PARAMETERS, redisCluster *c,
                                        void *ctx)
{
    cluster_variant_resp_generic(INTERNAL_FUNCTION_PARAM_PASSTHRU, c, 1, ctx);
}

/* Generic MULTI BULK response processor */
PHP_REDIS_API void cluster_gen_mbulk_resp(INTERNAL_FUNCTION_PARAMETERS,
                                   redisCluster *c, mbulk_cb cb, void *ctx)
{
    zval z_result;

    /* Abort if the reply isn't MULTIBULK or has an invalid length */
    if (c->reply_type != TYPE_MULTIBULK || c->reply_len < -1) {
        CLUSTER_RETURN_FALSE(c);
    }

    if (c->reply_len == -1 && c->flags->null_mbulk_as_null) {
        ZVAL_NULL(&z_result);
    } else {
        array_init(&z_result);

        if (c->reply_len > 0) {
            /* Push serialization settings from the cluster into our socket */
            c->cmd_sock->serializer = c->flags->serializer;
            c->cmd_sock->compression = c->flags->compression;

            /* Call our specified callback */
            if (cb(c->cmd_sock, &z_result, c->reply_len, ctx) == FAILURE) {
                zval_dtor(&z_result);
                CLUSTER_RETURN_FALSE(c);
            }
        }
    }

    // Success, make this array our return value
    if (CLUSTER_IS_ATOMIC(c)) {
        RETVAL_ZVAL(&z_result, 0, 1);
    } else {
        add_next_index_zval(&c->multi_resp, &z_result);
    }
}

/* HSCAN, SSCAN, ZSCAN */
PHP_REDIS_API int cluster_scan_resp(INTERNAL_FUNCTION_PARAMETERS, redisCluster *c,
                              REDIS_SCAN_TYPE type, long *it)
{
    char *pit;

    // We always want to see a MULTIBULK response with two elements
    if (c->reply_type != TYPE_MULTIBULK || c->reply_len != 2)
    {
        return FAILURE;
    }

    // Read the BULK size
    if (cluster_check_response(c, &c->reply_type) ||
        c->reply_type != TYPE_BULK)
    {
        return FAILURE;
    }

    // Read the iterator
    if ((pit = redis_sock_read_bulk_reply(c->cmd_sock,c->reply_len)) == NULL)
    {
        return FAILURE;
    }

    // Push the new iterator value to our caller
    *it = atol(pit);
    efree(pit);

    // We'll need another MULTIBULK response for the payload
    if (cluster_check_response(c, &c->reply_type) < 0)
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

/* INFO response */
PHP_REDIS_API void cluster_info_resp(INTERNAL_FUNCTION_PARAMETERS, redisCluster *c,
                              void *ctx)
{
    zval z_result;
    char *info;

    // Read our bulk response
    if ((info = redis_sock_read_bulk_reply(c->cmd_sock, c->reply_len)) == NULL)
    {
        CLUSTER_RETURN_FALSE(c);
    }

    /* Parse response, free memory */
    redis_parse_info_response(info, &z_result);
    efree(info);

    // Return our array
    if (CLUSTER_IS_ATOMIC(c)) {
        RETVAL_ZVAL(&z_result, 0, 1);
    } else {
        add_next_index_zval(&c->multi_resp, &z_result);
    }
}

/* CLIENT LIST response */
PHP_REDIS_API void cluster_client_list_resp(INTERNAL_FUNCTION_PARAMETERS, redisCluster *c,
                                     void *ctx)
{
    char *info;
    zval z_result;

    /* Read the bulk response */
    info = redis_sock_read_bulk_reply(c->cmd_sock, c->reply_len);
    if (info == NULL) {
        CLUSTER_RETURN_FALSE(c);
    }

    /* Parse it and free the bulk string */
    redis_parse_client_list_response(info, &z_result);
    efree(info);

    if (CLUSTER_IS_ATOMIC(c)) {
        RETVAL_ZVAL(&z_result, 0, 1);
    } else {
        add_next_index_zval(&c->multi_resp, &z_result);
    }
}

/* XRANGE */
PHP_REDIS_API void
cluster_xrange_resp(INTERNAL_FUNCTION_PARAMETERS, redisCluster *c, void *ctx) {
    zval z_messages;

    array_init(&z_messages);

    c->cmd_sock->serializer = c->flags->serializer;
    c->cmd_sock->compression = c->flags->compression;

    if (redis_read_stream_messages(c->cmd_sock, c->reply_len, &z_messages) < 0) {
        zval_dtor(&z_messages);
        CLUSTER_RETURN_FALSE(c);
    }

    if (CLUSTER_IS_ATOMIC(c)) {
        RETVAL_ZVAL(&z_messages, 0, 1);
    } else {
        add_next_index_zval(&c->multi_resp, &z_messages);
    }
}

/* XREAD */
PHP_REDIS_API void
cluster_xread_resp(INTERNAL_FUNCTION_PARAMETERS, redisCluster *c, void *ctx) {
    zval z_streams;

    c->cmd_sock->serializer = c->flags->serializer;
    c->cmd_sock->compression = c->flags->compression;

    if (c->reply_len == -1 && c->flags->null_mbulk_as_null) {
        ZVAL_NULL(&z_streams);
    } else {
        array_init(&z_streams);
        if (redis_read_stream_messages_multi(c->cmd_sock, c->reply_len, &z_streams) < 0) {
            zval_dtor(&z_streams);
            CLUSTER_RETURN_FALSE(c);
        }
    }

    if (CLUSTER_IS_ATOMIC(c)) {
        RETVAL_ZVAL(&z_streams, 0, 1);
    } else {
        add_next_index_zval(&c->multi_resp, &z_streams);
    }
}

/* XCLAIM */
PHP_REDIS_API void
cluster_xclaim_resp(INTERNAL_FUNCTION_PARAMETERS, redisCluster *c, void *ctx) {
    zval z_msg;

    array_init(&z_msg);

    ZEND_ASSERT(ctx == NULL || ctx == PHPREDIS_CTX_PTR);

    if (redis_read_xclaim_reply(c->cmd_sock, c->reply_len, ctx == PHPREDIS_CTX_PTR, &z_msg) < 0) {
        zval_dtor(&z_msg);
        CLUSTER_RETURN_FALSE(c);
    }

    if (CLUSTER_IS_ATOMIC(c)) {
        RETVAL_ZVAL(&z_msg, 0, 1);
    } else {
        add_next_index_zval(&c->multi_resp, &z_msg);
    }

}

/* XINFO */
PHP_REDIS_API void
cluster_xinfo_resp(INTERNAL_FUNCTION_PARAMETERS, redisCluster *c, void *ctx)
{
    zval z_ret;

    array_init(&z_ret);
    if (redis_read_xinfo_response(c->cmd_sock, &z_ret, c->reply_len) != SUCCESS) {
        zval_dtor(&z_ret);
        CLUSTER_RETURN_FALSE(c);
    }

    if (CLUSTER_IS_ATOMIC(c)) {
        RETURN_ZVAL(&z_ret, 0, 1);
    }
    add_next_index_zval(&c->multi_resp, &z_ret);
}

/* LMPOP, ZMPOP, BLMPOP, BZMPOP */
PHP_REDIS_API void
cluster_mpop_resp(INTERNAL_FUNCTION_PARAMETERS, redisCluster *c, void *ctx)
{
    zval z_ret;

    c->cmd_sock->null_mbulk_as_null = c->flags->null_mbulk_as_null;
    if (redis_read_mpop_response(c->cmd_sock, &z_ret, c->reply_len, ctx) == FAILURE) {
        CLUSTER_RETURN_FALSE(c);
    }

    if (CLUSTER_IS_ATOMIC(c)) {
        RETURN_ZVAL(&z_ret, 0, 0);
    }
    add_next_index_zval(&c->multi_resp, &z_ret);
}

static void
cluster_acl_custom_resp(INTERNAL_FUNCTION_PARAMETERS, redisCluster *c, void *ctx,
                        int (*cb)(RedisSock*, zval*, long))
{
    zval z_ret;

    array_init(&z_ret);
    if (cb(c->cmd_sock, &z_ret, c->reply_len) != SUCCESS) {
        zval_dtor(&z_ret);
        CLUSTER_RETURN_FALSE(c);
    }

    if (CLUSTER_IS_ATOMIC(c)) {
        RETURN_ZVAL(&z_ret, 0, 1);
    }
    add_next_index_zval(&c->multi_resp, &z_ret);
}

PHP_REDIS_API void
cluster_acl_getuser_resp(INTERNAL_FUNCTION_PARAMETERS, redisCluster *c, void *ctx) {
    cluster_acl_custom_resp(INTERNAL_FUNCTION_PARAM_PASSTHRU, c, ctx, redis_read_acl_getuser_reply);
}

PHP_REDIS_API void
cluster_acl_log_resp(INTERNAL_FUNCTION_PARAMETERS, redisCluster *c, void *ctx) {
    cluster_acl_custom_resp(INTERNAL_FUNCTION_PARAM_PASSTHRU, c, ctx, redis_read_acl_log_reply);
}

/* MULTI BULK response loop where we might pull the next one */
PHP_REDIS_API zval *cluster_zval_mbulk_resp(INTERNAL_FUNCTION_PARAMETERS,
                                     redisCluster *c, int pull, mbulk_cb cb, zval *z_ret)
{
    ZVAL_NULL(z_ret);

    // Pull our next response if directed
    if (pull) {
        if (cluster_check_response(c, &c->reply_type) < 0)
        {
            return NULL;
        }
    }

    // Validate reply type and length
    if (c->reply_type != TYPE_MULTIBULK || c->reply_len == -1) {
        return NULL;
    }

    array_init(z_ret);

    // Call our callback
    if (cb(c->cmd_sock, z_ret, c->reply_len, NULL) == FAILURE) {
        zval_dtor(z_ret);
        return NULL;
    }

    return z_ret;
}

/* MULTI MULTI BULK reply (for EXEC) */
PHP_REDIS_API void cluster_multi_mbulk_resp(INTERNAL_FUNCTION_PARAMETERS,
                                     redisCluster *c, void *ctx)
{
    zval *multi_resp = &c->multi_resp;
    array_init(multi_resp);

    clusterFoldItem *fi = c->multi_head;
    while (fi) {
        /* Make sure our transaction didn't fail here */
        if (c->multi_len[fi->slot] > -1) {
            /* Set the slot where we should look for responses.  We don't allow
             * failover inside a transaction, so it will be the master we have
             * mapped. */
            c->cmd_slot = fi->slot;
            c->cmd_sock = SLOT_SOCK(c, fi->slot);

            if (cluster_check_response(c, &c->reply_type) < 0) {
                zval_dtor(multi_resp);
                RETURN_FALSE;
            }

            fi->callback(INTERNAL_FUNCTION_PARAM_PASSTHRU, c, fi->ctx);
        } else {
            /* Just add false */
            add_next_index_bool(multi_resp, 0);
        }
        fi = fi->next;
    }

    // Set our return array
    zval_dtor(return_value);
    RETVAL_ZVAL(multi_resp, 0, 1);
}

/* Generic handler for MGET */
PHP_REDIS_API void cluster_mbulk_mget_resp(INTERNAL_FUNCTION_PARAMETERS,
                                    redisCluster *c, void *ctx)
{
    clusterMultiCtx *mctx = (clusterMultiCtx*)ctx;

    /* Protect against an invalid response type, -1 response length, and failure
     * to consume the responses. */
    c->cmd_sock->serializer = c->flags->serializer;
    c->cmd_sock->compression = c->flags->compression;
    short fail = c->reply_type != TYPE_MULTIBULK || c->reply_len == -1 ||
        mbulk_resp_loop(c->cmd_sock, mctx->z_multi, c->reply_len, NULL) == FAILURE;

    // If we had a failure, pad results with FALSE to indicate failure.  Non
    // existent keys (e.g. for MGET will come back as NULL)
    if (fail) {
        while (mctx->count--) {
            add_next_index_bool(mctx->z_multi, 0);
        }
    }

    // If this is the tail of our multi command, we can set our returns
    if (mctx->last) {
        if (CLUSTER_IS_ATOMIC(c)) {
            RETVAL_ZVAL(mctx->z_multi, 0, 1);
        } else {
            add_next_index_zval(&c->multi_resp, mctx->z_multi);
        }

        efree(mctx->z_multi);
    }

    // Clean up this context item
    efree(mctx);
}

/* Handler for MSETNX */
PHP_REDIS_API void cluster_msetnx_resp(INTERNAL_FUNCTION_PARAMETERS, redisCluster *c,
                                void *ctx)
{
    clusterMultiCtx *mctx = (clusterMultiCtx*)ctx;
    int real_argc = mctx->count/2;

    // Protect against an invalid response type
    if (c->reply_type != TYPE_INT) {
        php_error_docref(0, E_WARNING,
            "Invalid response type for MSETNX");
        while (real_argc--) {
            add_next_index_bool(mctx->z_multi, 0);
        }
        return;
    }

    // Response will be 1/0 per key, so the client can match them up
    while (real_argc--) {
        add_next_index_long(mctx->z_multi, c->reply_len);
    }

    // Set return value if it's our last response
    if (mctx->last) {
        if (CLUSTER_IS_ATOMIC(c)) {
            RETVAL_ZVAL(mctx->z_multi, 0, 0);
        } else {
            add_next_index_zval(&c->multi_resp, mctx->z_multi);
        }
        efree(mctx->z_multi);
    }

    // Free multi context
    efree(mctx);
}

/* Handler for DEL */
PHP_REDIS_API void cluster_del_resp(INTERNAL_FUNCTION_PARAMETERS, redisCluster *c,
                             void *ctx)
{
    clusterMultiCtx *mctx = (clusterMultiCtx*)ctx;

    // If we get an invalid reply, inform the client
    if (c->reply_type != TYPE_INT) {
        php_error_docref(0, E_WARNING,
            "Invalid reply type returned for DEL command");
        efree(mctx);
        return;
    }

    // Increment by the number of keys deleted
    Z_LVAL_P(mctx->z_multi) += c->reply_len;

    if (mctx->last) {
        if (CLUSTER_IS_ATOMIC(c)) {
            ZVAL_LONG(return_value, Z_LVAL_P(mctx->z_multi));
        } else {
            add_next_index_long(&c->multi_resp, Z_LVAL_P(mctx->z_multi));
        }
        efree(mctx->z_multi);
    }

    efree(ctx);
}

/* Handler for MSET */
PHP_REDIS_API void cluster_mset_resp(INTERNAL_FUNCTION_PARAMETERS, redisCluster *c,
                              void *ctx)
{
    clusterMultiCtx *mctx = (clusterMultiCtx*)ctx;

    // If we get an invalid reply type something very wrong has happened,
    // and we have to abort.
    if (c->reply_type != TYPE_LINE) {
        php_error_docref(0, E_ERROR,
            "Invalid reply type returned for MSET command");
        zval_dtor(mctx->z_multi);
        efree(mctx->z_multi);
        efree(mctx);
        RETURN_FALSE;
    }

    // Set our return if it's the last call
    if (mctx->last) {
        if (CLUSTER_IS_ATOMIC(c)) {
            ZVAL_BOOL(return_value, zval_is_true(mctx->z_multi));
        } else {
            add_next_index_bool(&c->multi_resp, zval_is_true(mctx->z_multi));
        }
        efree(mctx->z_multi);
    }

    efree(mctx);
}

/* Raw MULTI BULK reply */
PHP_REDIS_API void
cluster_mbulk_raw_resp(INTERNAL_FUNCTION_PARAMETERS, redisCluster *c, void *ctx)
{
    cluster_gen_mbulk_resp(INTERNAL_FUNCTION_PARAM_PASSTHRU, c,
        mbulk_resp_loop_raw, NULL);
}

/* Unserialize all the things */
PHP_REDIS_API void
cluster_mbulk_resp(INTERNAL_FUNCTION_PARAMETERS, redisCluster *c, void *ctx)
{
    cluster_gen_mbulk_resp(INTERNAL_FUNCTION_PARAM_PASSTHRU, c,
        mbulk_resp_loop, NULL);
}

/* For handling responses where we get key, value, key, value that
 * we will turn into key => value, key => value. */
PHP_REDIS_API void
cluster_mbulk_zipstr_resp(INTERNAL_FUNCTION_PARAMETERS, redisCluster *c,
                          void *ctx)
{
    cluster_gen_mbulk_resp(INTERNAL_FUNCTION_PARAM_PASSTHRU, c,
        mbulk_resp_loop_zipstr, NULL);
}

/* Handling key,value to key=>value where the values are doubles */
PHP_REDIS_API void
cluster_mbulk_zipdbl_resp(INTERNAL_FUNCTION_PARAMETERS, redisCluster *c,
                          void *ctx)
{
    cluster_gen_mbulk_resp(INTERNAL_FUNCTION_PARAM_PASSTHRU, c,
        mbulk_resp_loop_zipdbl, NULL);
}

/* Associate multi bulk response (for HMGET really) */
PHP_REDIS_API void
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
                        long long count, void *ctx)
{
    char *line;
    int line_len;

    // Iterate over the number we have
    while (count--) {
        // Read the line, which should never come back null
        line = redis_sock_read(redis_sock, &line_len);
        if (line == NULL) return FAILURE;

        // Add to our result array
        add_next_index_stringl(z_result, line, line_len);
        efree(line);
    }

    // Success!
    return SUCCESS;
}

/* MULTI BULK response where we unserialize everything */
int mbulk_resp_loop(RedisSock *redis_sock, zval *z_result,
                    long long count, void *ctx)
{
    char *line;
    int line_len;

    /* Iterate over the lines we have to process */
    while (count--) {
        /* Read our line */
        line = redis_sock_read(redis_sock, &line_len);

        if (line != NULL) {
            zval z_unpacked;
            if (redis_unpack(redis_sock, line, line_len, &z_unpacked)) {
                add_next_index_zval(z_result, &z_unpacked);
            } else {
                add_next_index_stringl(z_result, line, line_len);
            }
            efree(line);
        } else {
            add_next_index_bool(z_result, 0);
        }
    }

    return SUCCESS;
}

/* MULTI BULK response where we turn key1,value1 into key1=>value1 */
int mbulk_resp_loop_zipstr(RedisSock *redis_sock, zval *z_result,
                           long long count, void *ctx)
{
    char *line, *key = NULL;
    int line_len, key_len = 0;
    long long idx = 0;

    // Our count will need to be divisible by 2
    if (count % 2 != 0) {
        return -1;
    }

    // Iterate through our elements
    while (count--) {
        // Grab our line, bomb out on failure
        line = redis_sock_read(redis_sock, &line_len);
        if (!line) return -1;

        if (idx++ % 2 == 0) {
            // Save our key and length
            key = line;
            key_len = line_len;
        } else {
            /* Attempt unpacking */
            zval z_unpacked;
            if (redis_unpack(redis_sock, line, line_len, &z_unpacked)) {
                add_assoc_zval(z_result, key, &z_unpacked);
            } else {
                add_assoc_stringl_ex(z_result, key, key_len, line, line_len);
            }

            efree(line);
            efree(key);
        }
    }

    return SUCCESS;
}

/* MULTI BULK loop processor where we expect key,score key, score */
int mbulk_resp_loop_zipdbl(RedisSock *redis_sock, zval *z_result,
                           long long count, void *ctx)
{
    char *line, *key = NULL;
    int line_len, key_len = 0;
    long long idx = 0;

    // Our context will need to be divisible by 2
    if (count %2 != 0) {
        return -1;
    }

    // While we have elements
    while (count--) {
        line = redis_sock_read(redis_sock, &line_len);
        if (line != NULL) {
            if (idx++ % 2 == 0) {
                key = line;
                key_len = line_len;
            } else {
                zval zv, *z = &zv;
                if (redis_unpack(redis_sock,key,key_len, z)) {
                    zend_string *zstr = zval_get_string(z);
                    add_assoc_double_ex(z_result, ZSTR_VAL(zstr), ZSTR_LEN(zstr), atof(line));
                    zend_string_release(zstr);
                    zval_dtor(z);
                } else {
                    add_assoc_double_ex(z_result, key, key_len, atof(line));
                }

                /* Free our key and line */
                efree(key);
                efree(line);
            }
        }
    }

    return SUCCESS;
}

/* MULTI BULK where we're passed the keys, and we attach vals */
int mbulk_resp_loop_assoc(RedisSock *redis_sock, zval *z_result,
                          long long count, void *ctx)
{
    char *line;
    int line_len, i;
    zval *z_keys = ctx;

    // Loop while we've got replies
    for (i = 0; i < count; ++i) {
        zend_string *zstr = zval_get_string(&z_keys[i]);
        line = redis_sock_read(redis_sock, &line_len);

        if (line != NULL) {
            zval z_unpacked;
            if (redis_unpack(redis_sock, line, line_len, &z_unpacked)) {
                add_assoc_zval_ex(z_result, ZSTR_VAL(zstr), ZSTR_LEN(zstr), &z_unpacked);
            } else {
                add_assoc_stringl_ex(z_result, ZSTR_VAL(zstr), ZSTR_LEN(zstr), line, line_len);
            }
            efree(line);
        } else {
            add_assoc_bool_ex(z_result, ZSTR_VAL(zstr), ZSTR_LEN(zstr), 0);
        }

        // Clean up key context
        zend_string_release(zstr);
        zval_dtor(&z_keys[i]);
    }

    // Clean up our keys overall
    efree(z_keys);

    // Success!
    return SUCCESS;
}

/* Free an array of zend_string seeds */
void free_seed_array(zend_string **seeds, uint32_t nseeds) {
    int i;

    if (seeds == NULL)
        return;

    for (i = 0; i < nseeds; i++)
        zend_string_release(seeds[i]);

    efree(seeds);
}

static zend_string **get_valid_seeds(HashTable *input, uint32_t *nseeds) {
    HashTable *valid;
    uint32_t count, idx = 0;
    zval *z_seed;
    zend_string *zkey, **seeds = NULL;

    /* Short circuit if we don't have any sees */
    count = zend_hash_num_elements(input);
    if (count == 0)
        return NULL;

    ALLOC_HASHTABLE(valid);
    zend_hash_init(valid, count, NULL, NULL, 0);

    ZEND_HASH_FOREACH_VAL(input, z_seed) {
        ZVAL_DEREF(z_seed);

        if (Z_TYPE_P(z_seed) != IS_STRING) {
            php_error_docref(NULL, E_WARNING, "Skipping non-string entry in seeds array");
            continue;
        } else if (strrchr(Z_STRVAL_P(z_seed), ':') == NULL) {
            php_error_docref(NULL, E_WARNING,
                "Seed '%s' not in host:port format, ignoring", Z_STRVAL_P(z_seed));
            continue;
        }

        /* Add as a key to avoid duplicates */
        zend_hash_str_update_ptr(valid, Z_STRVAL_P(z_seed), Z_STRLEN_P(z_seed), NULL);
    } ZEND_HASH_FOREACH_END();

    /* We need at least one valid seed */
    count = zend_hash_num_elements(valid);
    if (count == 0)
        goto cleanup;

    /* Populate our return array */
    seeds = ecalloc(count, sizeof(*seeds));
    ZEND_HASH_FOREACH_STR_KEY(valid, zkey) {
        seeds[idx++] = zend_string_copy(zkey);
    } ZEND_HASH_FOREACH_END();

    *nseeds = idx;

cleanup:
    zend_hash_destroy(valid);
    FREE_HASHTABLE(valid);

    return seeds;
}

/* Validate cluster construction arguments and return a sanitized and validated
 * array of seeds */
zend_string**
cluster_validate_args(double timeout, double read_timeout, HashTable *seeds,
                      uint32_t *nseeds, char **errstr)
{
    zend_string **retval;

    if (timeout > INT_MAX) {
        if (errstr) *errstr = "Invalid timeout";
        return NULL;
    }

    if (read_timeout > INT_MAX) {
        if (errstr) *errstr = "Invalid read timeout";
        return NULL;
    }

    retval = get_valid_seeds(seeds, nseeds);
    if (retval == NULL && errstr)
        *errstr = "No valid seeds detected";

    return retval;
}

/* Helper function to compare to host:port seeds */
static int cluster_cmp_seeds(const void *a, const void *b) {
    zend_string *za = *(zend_string **)a;
    zend_string *zb = *(zend_string **)b;
    return strcmp(ZSTR_VAL(za), ZSTR_VAL(zb));
}

static void cluster_swap_seeds(void *a, void *b) {
    zend_string **za, **zb, *tmp;

    za = a;
    zb = b;

    tmp = *za;
    *za = *zb;
    *zb = tmp;
}

/* Turn an array of cluster seeds into a string we can cache.  If we get here we know
 * we have at least one entry and that every entry is a string in the form host:port */
#define SLOT_CACHE_PREFIX "phpredis_slots:"
zend_string *cluster_hash_seeds(zend_string **seeds, uint32_t count) {
    smart_str hash = {0};
    size_t i;

    /* Sort our seeds so any any array with identical seeds hashes to the same key
     * regardless of what order the user gives them to us in. */
    zend_sort(seeds, count, sizeof(*seeds), cluster_cmp_seeds, cluster_swap_seeds);

    /* Global phpredis hash prefix */
    smart_str_appendl(&hash, SLOT_CACHE_PREFIX, sizeof(SLOT_CACHE_PREFIX) - 1);

    /* Construct our actual hash */
    for (i = 0; i < count; i++) {
        smart_str_appendc(&hash, '[');
        smart_str_append_ex(&hash, seeds[i], 0);
        smart_str_appendc(&hash, ']');
    }

    /* Null terminate */
    smart_str_0(&hash);

    /* Return the internal zend_string */
    return hash.s;
}

PHP_REDIS_API redisCachedCluster *cluster_cache_load(zend_string *hash) {
    zend_resource *le;

    /* Look for cached slot information */
    le = zend_hash_find_ptr(&EG(persistent_list), hash);

    if (le != NULL) {
        /* Sanity check on our list type */
        if (le->type == le_cluster_slot_cache) {
            /* Success, return the cached entry */
            return le->ptr;
        }
        php_error_docref(0, E_WARNING, "Invalid slot cache resource");
    }

    /* Not found */
    return NULL;
}

/* Cache a cluster's slot information in persistent_list if it's enabled */
PHP_REDIS_API void cluster_cache_store(zend_string *hash, HashTable *nodes) {
    redisCachedCluster *cc = cluster_cache_create(hash, nodes);

    redis_register_persistent_resource(cc->hash, cc, le_cluster_slot_cache);
}


/* vim: set tabstop=4 softtabstop=4 expandtab shiftwidth=4: */
