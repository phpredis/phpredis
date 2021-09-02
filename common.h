#include "php.h"
#include "php_ini.h"

#ifndef REDIS_COMMON_H
#define REDIS_COMMON_H

#define PHPREDIS_CTX_PTR ((void *)0xDEADC0DE)
#define PHPREDIS_NOTUSED(v) ((void)v)

#include "zend_llist.h"
#include <ext/standard/php_var.h>
#include <ext/standard/php_math.h>
#include <zend_smart_str.h>
#include <ext/standard/php_smart_string.h>

#define PHPREDIS_GET_OBJECT(class_entry, o) (class_entry *)((char *)o - XtOffsetOf(class_entry, std))
#define PHPREDIS_ZVAL_GET_OBJECT(class_entry, z) PHPREDIS_GET_OBJECT(class_entry, Z_OBJ_P(z))

/* NULL check so Eclipse doesn't go crazy */
#ifndef NULL
#define NULL   ((void *) 0)
#endif

#include "backoff.h"

typedef enum {
    REDIS_SOCK_STATUS_FAILED = -1,
    REDIS_SOCK_STATUS_DISCONNECTED,
    REDIS_SOCK_STATUS_CONNECTED,
    REDIS_SOCK_STATUS_READY
} redis_sock_status;

#define _NL "\r\n"

/* properties */
#define REDIS_NOT_FOUND 0
#define REDIS_STRING    1
#define REDIS_SET       2
#define REDIS_LIST      3
#define REDIS_ZSET      4
#define REDIS_HASH      5
#define REDIS_STREAM    6

#ifdef PHP_WIN32
#define PHP_REDIS_API __declspec(dllexport)
#define phpredis_atoi64(p) _atoi64((p))
#else
#define PHP_REDIS_API
#define phpredis_atoi64(p) atoll((p))
#endif

/* reply types */
typedef enum _REDIS_REPLY_TYPE {
    TYPE_EOF       = -1,
    TYPE_LINE      = '+',
    TYPE_INT       = ':',
    TYPE_ERR       = '-',
    TYPE_BULK      = '$',
    TYPE_MULTIBULK = '*'
} REDIS_REPLY_TYPE;

/* SCAN variants */
typedef enum _REDIS_SCAN_TYPE {
    TYPE_SCAN,
    TYPE_SSCAN,
    TYPE_HSCAN,
    TYPE_ZSCAN
} REDIS_SCAN_TYPE;

/* PUBSUB subcommands */
typedef enum _PUBSUB_TYPE {
    PUBSUB_CHANNELS,
    PUBSUB_NUMSUB,
    PUBSUB_NUMPAT
} PUBSUB_TYPE;

/* options */
#define REDIS_OPT_SERIALIZER         1
#define REDIS_OPT_PREFIX             2
#define REDIS_OPT_READ_TIMEOUT       3
#define REDIS_OPT_SCAN               4
#define REDIS_OPT_FAILOVER           5
#define REDIS_OPT_TCP_KEEPALIVE      6
#define REDIS_OPT_COMPRESSION        7
#define REDIS_OPT_REPLY_LITERAL      8
#define REDIS_OPT_COMPRESSION_LEVEL  9
#define REDIS_OPT_NULL_MBULK_AS_NULL 10
#define REDIS_OPT_MAX_RETRIES        11
#define REDIS_OPT_BACKOFF_ALGORITHM  12
#define REDIS_OPT_BACKOFF_BASE       13
#define REDIS_OPT_BACKOFF_CAP        14

/* cluster options */
#define REDIS_FAILOVER_NONE              0
#define REDIS_FAILOVER_ERROR             1
#define REDIS_FAILOVER_DISTRIBUTE        2
#define REDIS_FAILOVER_DISTRIBUTE_SLAVES 3
/* serializers */
typedef enum {
    REDIS_SERIALIZER_NONE,
    REDIS_SERIALIZER_PHP,
    REDIS_SERIALIZER_IGBINARY,
    REDIS_SERIALIZER_MSGPACK,
    REDIS_SERIALIZER_JSON
} redis_serializer;
/* compression */
#define REDIS_COMPRESSION_NONE 0
#define REDIS_COMPRESSION_LZF  1
#define REDIS_COMPRESSION_ZSTD 2
#define REDIS_COMPRESSION_LZ4  3

/* SCAN options */
#define REDIS_SCAN_NORETRY 0
#define REDIS_SCAN_RETRY   1
#define REDIS_SCAN_PREFIX  2
#define REDIS_SCAN_NOPREFIX 3

/* BACKOFF_ALGORITHM options */
#define REDIS_BACKOFF_ALGORITHMS                    7
#define REDIS_BACKOFF_ALGORITHM_DEFAULT             0
#define REDIS_BACKOFF_ALGORITHM_DECORRELATED_JITTER 1
#define REDIS_BACKOFF_ALGORITHM_FULL_JITTER         2
#define REDIS_BACKOFF_ALGORITHM_EQUAL_JITTER        3
#define REDIS_BACKOFF_ALGORITHM_EXPONENTIAL         4
#define REDIS_BACKOFF_ALGORITHM_UNIFORM             5
#define REDIS_BACKOFF_ALGORITHM_CONSTANT            6

/* GETBIT/SETBIT offset range limits */
#define BITOP_MIN_OFFSET 0
#define BITOP_MAX_OFFSET 4294967295U

/* Transaction modes */
#define ATOMIC   0
#define MULTI    1
#define PIPELINE 2

#define IS_ATOMIC(redis_sock) (redis_sock->mode == ATOMIC)
#define IS_MULTI(redis_sock) (redis_sock->mode & MULTI)
#define IS_PIPELINE(redis_sock) (redis_sock->mode & PIPELINE)

#define PIPELINE_ENQUEUE_COMMAND(cmd, cmd_len) do { \
    if (redis_sock->pipeline_cmd == NULL) { \
        redis_sock->pipeline_cmd = zend_string_init(cmd, cmd_len, 0); \
    } else { \
        size_t pipeline_len = ZSTR_LEN(redis_sock->pipeline_cmd); \
        redis_sock->pipeline_cmd = zend_string_realloc(redis_sock->pipeline_cmd, pipeline_len + cmd_len, 0); \
        memcpy(&ZSTR_VAL(redis_sock->pipeline_cmd)[pipeline_len], cmd, cmd_len); \
    } \
} while (0)

#define SOCKET_WRITE_COMMAND(redis_sock, cmd, cmd_len) \
    if(redis_sock_write(redis_sock, cmd, cmd_len) < 0) { \
    efree(cmd); \
    RETURN_FALSE; \
}

#define REDIS_SAVE_CALLBACK(callback, closure_context) do { \
    fold_item *fi = malloc(sizeof(fold_item)); \
    fi->fun = callback; \
    fi->ctx = closure_context; \
    fi->next = NULL; \
    if (redis_sock->current) { \
        redis_sock->current->next = fi; \
    } \
    redis_sock->current = fi; \
    if (NULL == redis_sock->head) { \
        redis_sock->head = redis_sock->current; \
    } \
} while (0)

#define REDIS_PROCESS_REQUEST(redis_sock, cmd, cmd_len) \
    if (IS_PIPELINE(redis_sock)) { \
        PIPELINE_ENQUEUE_COMMAND(cmd, cmd_len); \
    } else { \
        SOCKET_WRITE_COMMAND(redis_sock, cmd, cmd_len); \
    } \
    efree(cmd);

#define REDIS_PROCESS_RESPONSE_CLOSURE(function, closure_context) \
    if (!IS_PIPELINE(redis_sock)) { \
        if (redis_response_enqueued(redis_sock) != SUCCESS) { \
            RETURN_FALSE; \
        } \
    } \
    REDIS_SAVE_CALLBACK(function, closure_context); \
    RETURN_ZVAL(getThis(), 1, 0); \

#define REDIS_PROCESS_RESPONSE(function) else { \
    REDIS_PROCESS_RESPONSE_CLOSURE(function, NULL) \
}

/* Clear redirection info */
#define REDIS_MOVED_CLEAR(redis_sock) \
    redis_sock->redir_slot = 0; \
    redis_sock->redir_port = 0; \
    redis_sock->redir_type = MOVED_NONE; \

/* Process a command assuming our command where our command building
 * function is redis_<cmdname>_cmd */
#define REDIS_PROCESS_CMD(cmdname, resp_func) \
    RedisSock *redis_sock; char *cmd; int cmd_len; void *ctx=NULL; \
    if ((redis_sock = redis_sock_get(getThis(), 0)) == NULL || \
       redis_##cmdname##_cmd(INTERNAL_FUNCTION_PARAM_PASSTHRU,redis_sock, \
                             &cmd, &cmd_len, NULL, &ctx)==FAILURE) { \
            RETURN_FALSE; \
    } \
    REDIS_PROCESS_REQUEST(redis_sock, cmd, cmd_len); \
    if (IS_ATOMIC(redis_sock)) { \
        resp_func(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock, NULL, ctx); \
    } else { \
        REDIS_PROCESS_RESPONSE_CLOSURE(resp_func, ctx) \
    }

/* Process a command but with a specific command building function
 * and keyword which is passed to us*/
#define REDIS_PROCESS_KW_CMD(kw, cmdfunc, resp_func) \
    RedisSock *redis_sock; char *cmd; int cmd_len; void *ctx=NULL; \
    if ((redis_sock = redis_sock_get(getThis(), 0)) == NULL || \
       cmdfunc(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock, kw, &cmd, \
               &cmd_len, NULL, &ctx)==FAILURE) { \
            RETURN_FALSE; \
    } \
    REDIS_PROCESS_REQUEST(redis_sock, cmd, cmd_len); \
    if (IS_ATOMIC(redis_sock)) { \
        resp_func(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock, NULL, ctx); \
    } else { \
        REDIS_PROCESS_RESPONSE_CLOSURE(resp_func, ctx) \
    }

/* Case sensitive compare against compile-time static string */
#define REDIS_STRCMP_STATIC(s, len, sstr) \
    (len == sizeof(sstr) - 1 && !strncmp(s, sstr, len))

/* Case insensitive compare against compile-time static string */
#define REDIS_STRICMP_STATIC(s, len, sstr) \
    (len == sizeof(sstr) - 1 && !strncasecmp(s, sstr, len))

/* Test if a zval is a string and (case insensitive) matches a static string */
#define ZVAL_STRICMP_STATIC(zv, sstr) \
    REDIS_STRICMP_STATIC(Z_STRVAL_P(zv), Z_STRLEN_P(zv), sstr)

/* Case insensitive compare of a zend_string to a static string */
#define ZSTR_STRICMP_STATIC(zs, sstr) \
    REDIS_STRICMP_STATIC(ZSTR_VAL(zs), ZSTR_LEN(zs), sstr)

/* Extended SET argument detection */
#define ZSTR_IS_EX_ARG(zs) ZSTR_STRICMP_STATIC(zs, "EX")
#define ZSTR_IS_PX_ARG(zs) ZSTR_STRICMP_STATIC(zs, "PX")
#define ZSTR_IS_NX_ARG(zs) ZSTR_STRICMP_STATIC(zs, "NX")
#define ZSTR_IS_XX_ARG(zs) ZSTR_STRICMP_STATIC(zs, "XX")

#define ZVAL_IS_NX_XX_ARG(zv) (ZVAL_STRICMP_STATIC(zv, "NX") || ZVAL_STRICMP_STATIC(zv, "XX"))
#define ZSTR_IS_EX_PX_ARG(a) (ZSTR_IS_EX_ARG(a) || ZSTR_IS_PX_ARG(a))

/* Given a string and length, validate a zRangeByLex argument.  The semantics
 * here are that the argument must start with '(' or '[' or be just the char
 * '+' or '-' */
#define IS_LEX_ARG(s,l) \
    (l>0 && (*s=='(' || *s=='[' || (l==1 && (*s=='+' || *s=='-'))))

#define REDIS_ENABLE_MODE(redis_sock, m) (redis_sock->mode |= m)
#define REDIS_DISABLE_MODE(redis_sock, m) (redis_sock->mode &= ~m)

/* HOST_NAME_MAX doesn't exist everywhere */
#ifndef HOST_NAME_MAX
    #if defined(_POSIX_HOST_NAME_MAX)
        #define HOST_NAME_MAX _POSIX_HOST_NAME_MAX
    #elif defined(MAXHOSTNAMELEN)
        #define HOST_NAME_MAX MAXHOSTNAMELEN
    #else
        #define HOST_NAME_MAX 255
    #endif
#endif

/* {{{ struct RedisSock */
typedef struct {
    php_stream          *stream;
    php_stream_context  *stream_ctx;
    zend_string         *host;
    int                 port;
    zend_string         *user;
    zend_string         *pass;
    double              timeout;
    double              read_timeout;
    long                retry_interval;
    int                 max_retries;
    struct RedisBackoff backoff;
    redis_sock_status   status;
    int                 persistent;
    int                 watching;
    zend_string         *persistent_id;

    redis_serializer    serializer;
    int                 compression;
    int                 compression_level;
    long                dbNumber;

    zend_string         *prefix;

    short               mode;
    struct fold_item    *head;
    struct fold_item    *current;

    zend_string         *pipeline_cmd;

    zend_string         *err;

    int                 scan;

    int                 readonly;
    int                 reply_literal;
    int                 null_mbulk_as_null;
    int                 tcp_keepalive;
} RedisSock;
/* }}} */

/* Redis response handler function callback prototype */
typedef void (*ResultCallback)(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock, zval *z_tab, void *ctx);
typedef int (*FailableResultCallback)(INTERNAL_FUNCTION_PARAMETERS, RedisSock*, zval*, void*);

typedef struct fold_item {
    FailableResultCallback fun;
    void *ctx;
    struct fold_item *next;
} fold_item;

typedef struct {
    zend_llist list;
    int nb_active;
} ConnectionPool;

typedef struct {
    RedisSock *sock;
    zend_object std;
} redis_object;

extern const zend_function_entry *redis_get_methods(void);

#endif
