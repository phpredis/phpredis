#include "php.h"
#include "php_ini.h"

#ifndef REDIS_COMMON_H
#define REDIS_COMMON_H

#include "php_compat.h"

#define PHPREDIS_CTX_PTR ((char *)0xDEADC0DE)
#define PHPREDIS_NOTUSED(v) ((void)v)

#include "zend_llist.h"
#include <ext/standard/php_var.h>
#include <ext/standard/php_math.h>
#include <zend_smart_str.h>
#include <zend_smart_string.h>

#define PHPREDIS_GET_OBJECT(class_entry, o) (class_entry *)((char *)o - offsetof(class_entry, std))
#define PHPREDIS_ZVAL_GET_OBJECT(class_entry, z) PHPREDIS_GET_OBJECT(class_entry, Z_OBJ_P(z))

/* We'll fallthrough if we want to */
#ifndef __has_attribute
#define __has_attribute(x) 0
#endif
#if __has_attribute(__fallthrough__)
#define REDIS_FALLTHROUGH __attribute__((__fallthrough__))
#else
#define REDIS_FALLTHROUGH do { } while (0)
#endif

#if defined(_WIN32) || defined(_WIN64)
# define PHPREDIS_LITTLE_ENDIAN
#elif defined(__BYTE_ORDER__) && (__BYTE_ORDER__ == __ORDER_BIG_ENDIAN__)
# define PHPREDIS_BIG_ENDIAN
#elif defined(__BYTE_ORDER__) && (__BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__)
# define PHPREDIS_LITTLE_ENDIAN
#else
# error "Unknown endianness"
#endif

#if defined(_MSC_VER) && !defined(__clang__)
#  define REDIS_MSVC 1
#endif

#ifndef REDIS_MSVC
#  ifndef REDIS_NODISCARD
#    if defined(__has_c_attribute)
#      if __has_c_attribute(nodiscard)
#        define REDIS_NODISCARD [[nodiscard]]
#      endif
#    endif
#  endif
#  ifndef REDIS_NODISCARD
#    if defined(__has_attribute)
#      if __has_attribute(warn_unused_result)
#        define REDIS_NODISCARD __attribute__((warn_unused_result))
#      endif
#    elif defined(__GNUC__) || defined(__clang__)
#      define REDIS_NODISCARD __attribute__((warn_unused_result))
#    endif
#  endif
#endif /* ifndef REDIS_MSVC */

#ifndef REDIS_NODISCARD
#  define REDIS_NODISCARD
#endif

#include "backoff.h"

typedef enum {
    REDIS_SOCK_STATUS_FAILED = -1,
    REDIS_SOCK_STATUS_DISCONNECTED,
    REDIS_SOCK_STATUS_CONNECTED,
    REDIS_SOCK_STATUS_AUTHENTICATED,
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
#define REDIS_VECTORSET 7

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

#define REDIS_SUBSCRIBE_IDX  0
#define REDIS_PSUBSCRIBE_IDX 1
#define REDIS_SSUBSCRIBE_IDX 2
#define REDIS_SUBS_BUCKETS   3

/* options */
#define REDIS_OPT_SERIALIZER          1
#define REDIS_OPT_PREFIX              2
#define REDIS_OPT_READ_TIMEOUT        3
#define REDIS_OPT_SCAN                4
#define REDIS_OPT_FAILOVER            5
#define REDIS_OPT_TCP_KEEPALIVE       6
#define REDIS_OPT_COMPRESSION         7
#define REDIS_OPT_REPLY_LITERAL       8
#define REDIS_OPT_COMPRESSION_LEVEL   9
#define REDIS_OPT_NULL_MBULK_AS_NULL  10
#define REDIS_OPT_MAX_RETRIES         11
#define REDIS_OPT_BACKOFF_ALGORITHM   12
#define REDIS_OPT_BACKOFF_BASE        13
#define REDIS_OPT_BACKOFF_CAP         14
#define REDIS_OPT_PACK_IGNORE_NUMBERS 15

/* cluster options */
#define REDIS_FAILOVER_NONE              0
#define REDIS_FAILOVER_ERROR             1
#define REDIS_FAILOVER_DISTRIBUTE        2
#define REDIS_FAILOVER_DISTRIBUTE_SLAVES 3
#define REDIS_FAILOVER_PREFER_REPLICA    4
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

#define PHPREDIS_DEBUG_LOGGING 0

#if PHPREDIS_DEBUG_LOGGING == 1
#define redisDbgFmt(fmt, ...) \
    php_printf("%s:%d:%s(): " fmt "\n", __FILE__, __LINE__, __func__, __VA_ARGS__)
#define redisDbgStr(str) redisDbgFmt("%s", str)
#else
#define redisDbgFmt(fmt, ...) ((void)0)
#define redisDbgStr(str) ((void)0)
#endif

/* On some versions of glibc strncmp and strncasecmp can be a macro a macro.
 * This wrapper allows us to use it in combination with ZEND_STRL in those
 * cases. */
static zend_always_inline int redis_strncmp(const char *s1, const char *s2, size_t n) {
    return strncmp(s1, s2, n);
}
static zend_always_inline int redis_strncasecmp(const char *s1, const char *s2, size_t n) {
    return strncasecmp(s1, s2, n);
}

static zend_always_inline zend_bool
redis_str_eq(const char *s, size_t len, const char *cmp, size_t cmp_len)
{
    return len == cmp_len && redis_strncmp(s, cmp, len) == 0;
}

static zend_always_inline zend_bool
redis_str_ieq(const char *s, size_t len, const char *cmp, size_t cmp_len)
{
    return len == cmp_len && redis_strncasecmp(s, cmp, len) == 0;
}

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

/* Complete representation for various commands in RESP */
#define RESP_MULTI_CMD         "*1\r\n$5\r\nMULTI\r\n"
#define RESP_EXEC_CMD          "*1\r\n$4\r\nEXEC\r\n"
#define RESP_DISCARD_CMD       "*1\r\n$7\r\nDISCARD\r\n"

typedef struct RedisHello {
    zend_string *server;
    zend_string *version;
} RedisHello;

typedef enum RedisSockType {
    REDIS_SOCK_STANDALONE,
    REDIS_SOCK_ARRAY,
    REDIS_SOCK_CLUSTER,
    REDIS_SOCK_SENTINEL,
    REDIS_SOCK_SESSION
} RedisSockType;

/* {{{ struct RedisSock */
typedef struct {
    RedisSockType       type;
    php_stream          *stream;
    zend_string         *host;
    int                 port;
    zend_string         *user;
    zend_string         *pass;
    HashTable           *context;
    double              timeout;
    double              read_timeout;
    long                retry_interval;
    int                 max_retries;
    struct RedisBackoff backoff;
    redis_sock_status   status;
    zend_bool           persistent;
    zend_bool           watching;
    zend_string         *persistent_id;
    HashTable           *subs[REDIS_SUBS_BUCKETS];
    redis_serializer    serializer;
    zend_bool           pack_ignore_numbers;
    int                 compression;
    int                 compression_level;
    long                dbNumber;
    zend_string         *prefix;
    struct RedisHello   hello;
    short               mode;
    struct fold_item    *reply_callback;
    size_t              reply_callback_count;
    size_t              reply_callback_capacity;
    smart_string        pipeline_cmd;

    zend_string         *err;
    int                 scan;

    zend_bool           readonly;
    zend_bool           reply_literal;
    zend_bool           null_mbulk_as_null;
    zend_bool           tcp_keepalive;
    size_t              txBytes;
    size_t              rxBytes;
    uint8_t             flags;
} RedisSock;
/* }}} */

static zend_always_inline zend_bool redis_sock_is_atomic(const RedisSock *redis_sock) {
    return redis_sock->mode == ATOMIC;
}

static zend_always_inline zend_bool redis_sock_is_multi(const RedisSock *redis_sock) {
    return (redis_sock->mode & MULTI) != 0;
}

static zend_always_inline zend_bool redis_sock_is_pipeline(const RedisSock *redis_sock) {
    return (redis_sock->mode & PIPELINE) != 0;
}

typedef void (RedisCmdCtxDtor)(void *ptr);
typedef struct RedisCmdCtx {
    void *ptr;
    RedisCmdCtxDtor *dtor;
} RedisCmdCtx;

/* Redis response handler function callback prototype */
typedef void (*ResultCallback)(INTERNAL_FUNCTION_PARAMETERS,
    RedisSock*, zval*, RedisCmdCtx);
typedef int (*FailableResultCallback)(INTERNAL_FUNCTION_PARAMETERS,
    RedisSock*, zval*, RedisCmdCtx);

static zend_always_inline void redis_cmd_ctx_free(RedisCmdCtx ctx) {
    if (ctx.dtor)
        ctx.dtor(ctx.ptr);
}

typedef struct fold_item {
    FailableResultCallback fun;
    RedisCmdCtx ctx;
    uint8_t flags;
} fold_item;

typedef struct {
    zend_llist list;
    int nb_active;
} ConnectionPool;

typedef struct {
    RedisSock *sock;
    zend_object std;
} redis_object;

#endif
