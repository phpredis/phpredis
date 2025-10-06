#include "php.h"
#include "php_ini.h"

#ifndef REDIS_COMMON_H
#define REDIS_COMMON_H

#define PHPREDIS_CTX_PTR ((char *)0xDEADC0DE)
#define PHPREDIS_NOTUSED(v) ((void)v)

#include "zend_llist.h"
#include <ext/standard/php_var.h>
#include <ext/standard/php_math.h>
#include <zend_smart_str.h>
#include <zend_smart_string.h>

#define PHPREDIS_GET_OBJECT(class_entry, o) (class_entry *)((char *)o - XtOffsetOf(class_entry, std))
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

/* NULL check so Eclipse doesn't go crazy */
#ifndef NULL
#define NULL   ((void *) 0)
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
#  if defined(__has_c_attribute)
#    if __has_c_attribute(nodiscard)
#      define REDIS_NODISCARD [[nodiscard]]
#    endif
#  endif
#  ifndef REDIS_NODISCARD
#    if defined(__has_attribute)
#      if __has_attribute(warn_unused_result)
#        define REDIS_NODISCARD __attribute__((warn_unused_result))
#      endif
#    elif defined(__GNUC__) || defined(__clang__)
#      define REDIS_NODISCARD __attribute__((warn_unused_result))
#    else
#      define REDIS_NODISCARD
#    endif
#  endif
#else
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

#if PHP_VERSION_ID < 80000
#define Z_PARAM_ARRAY_HT_OR_NULL(dest) \
        Z_PARAM_ARRAY_HT_EX(dest, 1, 0)
#define Z_PARAM_STR_OR_NULL(dest) \
        Z_PARAM_STR_EX(dest, 1, 0)
#define Z_PARAM_ZVAL_OR_NULL(dest) \
	Z_PARAM_ZVAL_EX(dest, 1, 0)
#define Z_PARAM_BOOL_OR_NULL(dest, is_null) \
	Z_PARAM_BOOL_EX(dest, is_null, 1, 0)
#endif

#if PHPREDIS_DEBUG_LOGGING == 1
#define redisDbgFmt(fmt, ...) \
    php_printf("%s:%d:%s(): " fmt "\n", __FILE__, __LINE__, __func__, __VA_ARGS__)
#define redisDbgStr(str) phpredisDebugFmt("%s", str)
#else
#define redisDbgFmt(fmt, ...) ((void)0)
#define redisDbgStr(str) ((void)0)
#endif

#define IS_ATOMIC(redis_sock) (redis_sock->mode == ATOMIC)
#define IS_MULTI(redis_sock) (redis_sock->mode & MULTI)
#define IS_PIPELINE(redis_sock) (redis_sock->mode & PIPELINE)

/* Case sensitive compare against compile-time static string */
#define REDIS_STRCMP_STATIC(s, len, sstr) \
    (len == sizeof(sstr) - 1 && !strncmp(s, sstr, len))

/* Case insensitive compare against compile-time static string */
#define REDIS_STRICMP_STATIC(s, len, sstr) \
    (len == sizeof(sstr) - 1 && !strncasecmp(s, sstr, len))

/* On some versions of glibc strncmp is a macro. This wrapper allows us to
   use it in combination with ZEND_STRL in those cases. */
static inline int redis_strncmp(const char *s1, const char *s2, size_t n) {
    return strncmp(s1, s2, n);
}

/* Test if a zval is a string and (case insensitive) matches a static string */
#define ZVAL_STRICMP_STATIC(zv, sstr) \
    REDIS_STRICMP_STATIC(Z_STRVAL_P(zv), Z_STRLEN_P(zv), sstr)

/* Case insensitive compare of a zend_string to a static string */
#define ZSTR_STRICMP_STATIC(zs, sstr) \
    REDIS_STRICMP_STATIC(ZSTR_VAL(zs), ZSTR_LEN(zs), sstr)

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
    zend_bool           sentinel;
    size_t              txBytes;
    size_t              rxBytes;
    uint8_t             flags;
} RedisSock;
/* }}} */

/* Redis response handler function callback prototype */
typedef void (*ResultCallback)(INTERNAL_FUNCTION_PARAMETERS,
    RedisSock *redis_sock, zval *z_tab, void *ctx);
typedef int (*FailableResultCallback)(INTERNAL_FUNCTION_PARAMETERS,
    RedisSock*, zval*, void*);

typedef struct fold_item {
    FailableResultCallback fun;
    uint8_t flags;
    void *ctx;
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
