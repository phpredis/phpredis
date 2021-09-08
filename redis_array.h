#ifndef REDIS_ARRAY_H
#define REDIS_ARRAY_H

#if (defined(_MSC_VER) && _MSC_VER <= 1920)
#include "win32/php_stdint.h"
#else
#include <stdint.h>
#endif
#include "common.h"

typedef struct {
    uint32_t value;
    int index;
} ContinuumPoint;

typedef struct {
    size_t nb_points;
    ContinuumPoint *points;
} Continuum;

typedef struct RedisArray_ {
    int count;
    zend_string **hosts;    /* array of host:port strings */
    zval *redis;            /* array of Redis instances */
    zval *z_multi_exec;     /* Redis instance to be used in multi-exec */
    zend_bool index;        /* use per-node index */
    zend_bool auto_rehash;  /* migrate keys on read operations */
    zend_bool pconnect;     /* should we use pconnect */
    zval z_fun;             /* key extractor, callable */
    zval z_dist;            /* key distributor, callable */
    zend_string *algorithm; /* key hashing algorithm name */
    HashTable *pure_cmds;   /* hash table */
    double connect_timeout; /* socket connect timeout */
    double read_timeout;    /* socket read timeout */
    Continuum *continuum;
    struct RedisArray_ *prev;
} RedisArray;

extern const zend_function_entry *redis_array_get_methods(void);
zend_object *create_redis_array_object(zend_class_entry *ce);
void free_redis_array_object(zend_object *object);

#endif
