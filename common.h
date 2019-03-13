#include "php.h"
#include "php_ini.h"

#ifndef REDIS_COMMON_H
#define REDIS_COMMON_H

#define PHPREDIS_NOTUSED(v) ((void)v)

#include <ext/standard/php_var.h>
#include <ext/standard/php_math.h>
#if (PHP_MAJOR_VERSION < 7)
#include <ext/standard/php_smart_str.h>
typedef smart_str smart_string;
#define smart_string_0(x) smart_str_0(x)
#define smart_string_appendc(dest, c) smart_str_appendc(dest, c)
#define smart_string_append_long(dest, val) smart_str_append_long(dest, val)
#define smart_string_appendl(dest, src, len) smart_str_appendl(dest, src, len)

typedef struct {
    short gc;
    size_t len;
    char *val;
} zend_string;

#define REDIS_MAKE_STD_ZVAL(zv) MAKE_STD_ZVAL(zv)
#define REDIS_FREE_ZVAL(zv) (efree(zv))

#define ZSTR_VAL(s) (s)->val
#define ZSTR_LEN(s) (s)->len

static zend_always_inline zend_string *
zend_string_alloc(size_t len, int persistent)
{
    zend_string *zstr = emalloc(sizeof(*zstr) + len + 1);

    ZSTR_VAL(zstr) = (char *)zstr + sizeof(*zstr);
    ZSTR_LEN(zstr) = len;
    zstr->gc = 0x01;
    return zstr;
}

static zend_always_inline zend_string *
zend_string_init(const char *str, size_t len, int persistent)
{
    zend_string *zstr = zend_string_alloc(len, persistent);

    memcpy(ZSTR_VAL(zstr), str, len);
    ZSTR_VAL(zstr)[len] = '\0';
    return zstr;
}

static zend_always_inline zend_string *
zend_string_realloc(zend_string *s, size_t len, int persistent)
{
    zend_string *zstr;

    if (!s->gc) {
        zstr = zend_string_init(ZSTR_VAL(s), len, 0);
    } else if (s->gc & 0x10) {
        ZSTR_VAL(s) = erealloc(ZSTR_VAL(s), len + 1);
        ZSTR_LEN(s) = len;
        zstr = s;
    } else {
        zstr = erealloc(s, sizeof(*zstr) + len + 1);
        ZSTR_VAL(zstr) = (char *)zstr + sizeof(*zstr);
        ZSTR_LEN(zstr) = len;
    }
    return zstr;
}

#define zend_string_copy(s) zend_string_init(ZSTR_VAL(s), ZSTR_LEN(s), 0)

#define zend_string_equal_val(s1, s2) !memcmp(ZSTR_VAL(s1), ZSTR_VAL(s2), ZSTR_LEN(s1))
#define zend_string_equal_content(s1, s2) (ZSTR_LEN(s1) == ZSTR_LEN(s2) && zend_string_equal_val(s1, s2))
#define zend_string_equals(s1, s2) (s1 == s2 || zend_string_equal_content(s1, s2))

#define zend_string_release(s) do { \
    if ((s) && (s)->gc) { \
        if ((s)->gc & 0x10 && ZSTR_VAL(s)) efree(ZSTR_VAL(s)); \
        if ((s)->gc & 0x01) efree((s)); \
    } \
} while (0)

#define ZEND_HASH_FOREACH_KEY_VAL(ht, _h, _key, _val) do { \
    HashPosition _hpos; \
    for (zend_hash_internal_pointer_reset_ex(ht, &_hpos); \
         zend_hash_has_more_elements_ex(ht, &_hpos) == SUCCESS; \
         zend_hash_move_forward_ex(ht, &_hpos) \
    ) { \
        zend_string _zstr = {0}; \
        char *_str_index; uint _str_length; ulong _num_index; \
        _h = 0; _key = NULL; _val = zend_hash_get_current_data_ex(ht, &_hpos); \
        switch (zend_hash_get_current_key_ex(ht, &_str_index, &_str_length, &_num_index, 0, &_hpos)) { \
            case HASH_KEY_IS_STRING: \
                _zstr.len = _str_length - 1; \
                _zstr.val = _str_index; \
                _key = &_zstr; \
                break; \
            case HASH_KEY_IS_LONG: \
                _h = _num_index; \
                break; \
            default: \
                /* noop */ break; \
        }

#define ZEND_HASH_FOREACH_VAL(ht, _val) do { \
    HashPosition _hpos; \
    for (zend_hash_internal_pointer_reset_ex(ht, &_hpos); \
         zend_hash_has_more_elements_ex(ht, &_hpos) == SUCCESS; \
         zend_hash_move_forward_ex(ht, &_hpos) \
    ) { \
         _val = zend_hash_get_current_data_ex(ht, &_hpos); \

#define ZEND_HASH_FOREACH_PTR(ht, _ptr) do { \
    HashPosition _hpos; \
    for (zend_hash_internal_pointer_reset_ex(ht, &_hpos); \
         zend_hash_has_more_elements_ex(ht, &_hpos) == SUCCESS; \
         zend_hash_move_forward_ex(ht, &_hpos) \
    ) { \
         _ptr = zend_hash_get_current_data_ptr_ex(ht, &_hpos); \

#define ZEND_HASH_FOREACH_END() \
    } \
} while(0)

#undef zend_hash_get_current_key
#define zend_hash_get_current_key(ht, str_index, num_index) \
    zend_hash_get_current_key_ex(ht, str_index, NULL, num_index, 0, NULL)

#define zend_hash_str_exists(ht, str, len) zend_hash_exists(ht, str, len + 1)

static zend_always_inline zval *
zend_hash_str_find(const HashTable *ht, const char *key, size_t len)
{
    zval **zv;

    if (zend_hash_find(ht, key, len + 1, (void **)&zv) == SUCCESS) {
        return *zv;
    }
    return NULL;
}

#define zend_hash_find_ptr(ht, s) zend_hash_str_find_ptr(ht, ZSTR_VAL(s), ZSTR_LEN(s))

static zend_always_inline void *
zend_hash_str_find_ptr(const HashTable *ht, const char *str, size_t len)
{
    void **ptr;

    if (zend_hash_find(ht, str, len + 1, (void **)&ptr) == SUCCESS) {
        return *ptr;
    }
    return NULL;
}

#define zend_hash_str_update_ptr(ht, str, len, pData) zend_hash_str_update_mem(ht, str, len, pData, sizeof(void *))

static zend_always_inline void *
zend_hash_str_update_mem(HashTable *ht, const char *str, size_t len, void *pData, size_t size)
{
    if (zend_hash_update(ht, str, len + 1, (void *)&pData, size, NULL) == SUCCESS) {
        return pData;
    }
    return NULL;
}

static zend_always_inline void *
zend_hash_index_update_ptr(HashTable *ht, zend_ulong h, void *pData)
{
    if (zend_hash_index_update(ht, h, (void **)&pData, sizeof(void *), NULL) == SUCCESS) {
        return pData;
    }
    return NULL;
}

#undef zend_hash_get_current_data
static zend_always_inline zval *
zend_hash_get_current_data(HashTable *ht)
{
    zval **zv;

    if (zend_hash_get_current_data_ex(ht, (void **)&zv, NULL) == SUCCESS) {
        return *zv;
    }
    return NULL;
}

static zend_always_inline void *
zend_hash_get_current_data_ptr_ex(HashTable *ht, HashPosition *pos)
{
    void **ptr;

    if (zend_hash_get_current_data_ex(ht, (void **)&ptr, pos) == SUCCESS) {
        return *ptr;
    }
    return NULL;
}
#define zend_hash_get_current_data_ptr(ht) zend_hash_get_current_data_ptr_ex(ht, NULL)

static int (*_zend_hash_index_find)(const HashTable *, ulong, void **) = &zend_hash_index_find;
#define zend_hash_index_find(ht, h) inline_zend_hash_index_find(ht, h)

static zend_always_inline zval *
inline_zend_hash_index_find(const HashTable *ht, zend_ulong h)
{
    zval **zv;
    if (_zend_hash_index_find(ht, h, (void **)&zv) == SUCCESS) {
        return *zv;
    }
    return NULL;
}

static zend_always_inline void *
zend_hash_index_find_ptr(const HashTable *ht, zend_ulong h)
{
    void **ptr;

    if (_zend_hash_index_find(ht, h, (void **)&ptr) == SUCCESS) {
        return *ptr;
    }
    return NULL;
}

static int (*_zend_hash_get_current_data_ex)(HashTable *, void **, HashPosition *) = &zend_hash_get_current_data_ex;
#define zend_hash_get_current_data_ex(ht, pos) inline_zend_hash_get_current_data_ex(ht, pos)
static zend_always_inline zval *
inline_zend_hash_get_current_data_ex(HashTable *ht, HashPosition *pos)
{
    zval **zv;
    if (_zend_hash_get_current_data_ex(ht, (void **)&zv, pos) == SUCCESS) {
        return *zv;
    }
    return NULL;
}

#undef zend_hash_next_index_insert
#define zend_hash_next_index_insert(ht, pData) \
    _zend_hash_next_index_insert(ht, pData ZEND_FILE_LINE_CC)
static zend_always_inline zval *
_zend_hash_next_index_insert(HashTable *ht, zval *pData ZEND_FILE_LINE_DC)
{
    if (_zend_hash_index_update_or_next_insert(ht, 0, &pData, sizeof(pData),
            NULL, HASH_NEXT_INSERT ZEND_FILE_LINE_CC) == SUCCESS
    ) {
        return pData;
    }
    return NULL;
}

#undef zend_get_parameters_array
#define zend_get_parameters_array(ht, param_count, argument_array) \
    inline_zend_get_parameters_array(ht, param_count, argument_array TSRMLS_CC)

static zend_always_inline int
inline_zend_get_parameters_array(int ht, int param_count, zval *argument_array TSRMLS_DC)
{
    int i, ret = FAILURE;
    zval **zv = ecalloc(param_count, sizeof(zval *));

    if (_zend_get_parameters_array(ht, param_count, zv TSRMLS_CC) == SUCCESS) {
        for (i = 0; i < param_count; i++) {
            argument_array[i] = *zv[i];
        }
        ret = SUCCESS;
    }
    efree(zv);
    return ret;
}

typedef zend_rsrc_list_entry zend_resource;

extern int (*_add_next_index_string)(zval *, const char *, int);
#define add_next_index_string(arg, str) _add_next_index_string(arg, str, 1);
extern int (*_add_next_index_stringl)(zval *, const char *, uint, int);
#define add_next_index_stringl(arg, str, length) _add_next_index_stringl(arg, str, length, 1);

#undef ZVAL_STRING
#define ZVAL_STRING(z, s) do { \
    const char *_s = (s); \
    ZVAL_STRINGL(z, _s, strlen(_s)); \
} while (0)
#undef RETVAL_STRING
#define RETVAL_STRING(s) ZVAL_STRING(return_value, s)
#undef RETURN_STRING
#define RETURN_STRING(s) { RETVAL_STRING(s); return; }

#undef ZVAL_STRINGL
#define ZVAL_STRINGL(z, s, l) do { \
    const char *__s = (s); int __l = l; \
    zval *__z = (z); \
    Z_STRLEN_P(__z) = __l; \
    Z_STRVAL_P(__z) = estrndup(__s, __l); \
    Z_TYPE_P(__z) = IS_STRING; \
} while (0)
#undef RETVAL_STRINGL
#define RETVAL_STRINGL(s, l) ZVAL_STRINGL(return_value, s, l)
#undef RETURN_STRINGL
#define RETURN_STRINGL(s, l) { RETVAL_STRINGL(s, l); return; }

static int (*_call_user_function)(HashTable *, zval **, zval *, zval *, zend_uint, zval *[] TSRMLS_DC) = &call_user_function;
#define call_user_function(function_table, object, function_name, retval_ptr, param_count, params) \
    inline_call_user_function(function_table, object, function_name, retval_ptr, param_count, params TSRMLS_CC)

static zend_always_inline int
inline_call_user_function(HashTable *function_table, zval *object, zval *function_name, zval *retval_ptr, zend_uint param_count, zval params[] TSRMLS_DC)
{
    int i, ret;
    zval **_params = NULL;
    if (!params) param_count = 0;
    if (param_count > 0) {
        _params = ecalloc(param_count, sizeof(zval *));
        for (i = 0; i < param_count; ++i) {
            zval *zv = &params[i];
            MAKE_STD_ZVAL(_params[i]);
            ZVAL_ZVAL(_params[i], zv, 1, 0);
        }
    }
    ret = _call_user_function(function_table, &object, function_name, retval_ptr, param_count, _params TSRMLS_CC);
    if (_params) {
        for (i = 0; i < param_count; ++i) {
            zval_ptr_dtor(&_params[i]);
        }
        efree(_params);
    }
    return ret;
}

#undef add_assoc_bool
#define add_assoc_bool(__arg, __key, __b) add_assoc_bool_ex(__arg, __key, strlen(__key), __b)
extern int (*_add_assoc_bool_ex)(zval *, const char *, uint, int);
#define add_assoc_bool_ex(_arg, _key, _key_len, _b) _add_assoc_bool_ex(_arg, _key, _key_len + 1, _b)

#undef add_assoc_long
#define add_assoc_long(__arg, __key, __n) add_assoc_long_ex(__arg, __key, strlen(__key), __n)
extern int (*_add_assoc_long_ex)(zval *, const char *, uint, long);
#define add_assoc_long_ex(_arg, _key, _key_len, _n) _add_assoc_long_ex(_arg, _key, _key_len + 1, _n)

#undef add_assoc_double
#define add_assoc_double(__arg, __key, __d) add_assoc_double_ex(__arg, __key, strlen(__key), __d)
extern int (*_add_assoc_double_ex)(zval *, const char *, uint, double);
#define add_assoc_double_ex(_arg, _key, _key_len, _d) _add_assoc_double_ex(_arg, _key, _key_len + 1, _d)

#undef add_assoc_string
#define add_assoc_string(__arg, __key, __str) add_assoc_string_ex(__arg, __key, strlen(__key), __str)
extern int (*_add_assoc_string_ex)(zval *, const char *, uint, char *, int);
#define add_assoc_string_ex(_arg, _key, _key_len, _str) _add_assoc_string_ex(_arg, _key, _key_len + 1, _str, 1)

extern int (*_add_assoc_stringl_ex)(zval *, const char *, uint, char *, uint, int);
#define add_assoc_stringl_ex(_arg, _key, _key_len, _str, _length) _add_assoc_stringl_ex(_arg, _key, _key_len + 1, _str, _length, 1)

#undef add_assoc_zval
#define add_assoc_zval(__arg, __key, __value) add_assoc_zval_ex(__arg, __key, strlen(__key), __value)
extern int (*_add_assoc_zval_ex)(zval *, const char *, uint, zval *);
#define add_assoc_zval_ex(_arg, _key, _key_len, _value) _add_assoc_zval_ex(_arg, _key, _key_len + 1, _value);

typedef long zend_long;
static zend_always_inline zend_long
zval_get_long(zval *op)
{
    switch (Z_TYPE_P(op)) {
        case IS_BOOL:
        case IS_LONG:
            return Z_LVAL_P(op);
        case IS_DOUBLE:
            return zend_dval_to_lval(Z_DVAL_P(op));
        case IS_STRING:
            {
                double dval;
                zend_long lval;
                zend_uchar type = is_numeric_string(Z_STRVAL_P(op), Z_STRLEN_P(op), &lval, &dval, 0);
                if (type == IS_LONG) {
                    return lval;
                } else if (type == IS_DOUBLE) {
                    return zend_dval_to_lval(dval);
                }
            }
            break;
        EMPTY_SWITCH_DEFAULT_CASE()
    }
    return 0;
}

static zend_always_inline double
zval_get_double(zval *op)
{
    switch (Z_TYPE_P(op)) {
        case IS_BOOL:
        case IS_LONG:
            return (double)Z_LVAL_P(op);
        case IS_DOUBLE:
            return Z_DVAL_P(op);
        case IS_STRING:
            return zend_strtod(Z_STRVAL_P(op), NULL);
        EMPTY_SWITCH_DEFAULT_CASE()
    }
    return 0.0;
}

static zend_always_inline zend_string *
zval_get_string(zval *op)
{
    zend_string *zstr = ecalloc(1, sizeof(zend_string));

    zstr->gc = 0;
    ZSTR_VAL(zstr) = "";
    ZSTR_LEN(zstr) = 0;
    switch (Z_TYPE_P(op)) {
        case IS_STRING:
            ZSTR_VAL(zstr) = Z_STRVAL_P(op);
            ZSTR_LEN(zstr) = Z_STRLEN_P(op);
            break;
        case IS_BOOL:
            if (Z_LVAL_P(op)) {
                ZSTR_VAL(zstr) = "1";
                ZSTR_LEN(zstr) = 1;
            }
            break;
        case IS_LONG: {
            zstr->gc = 0x10;
            ZSTR_LEN(zstr) = spprintf(&ZSTR_VAL(zstr), 0, "%ld", Z_LVAL_P(op));
            break;
        }
        case IS_DOUBLE: {
            zstr->gc = 0x10;
            ZSTR_LEN(zstr) = spprintf(&ZSTR_VAL(zstr), 0, "%.16g", Z_DVAL_P(op));
            break;
        }
        EMPTY_SWITCH_DEFAULT_CASE()
    }
    zstr->gc |= 0x01;
    return zstr;
}

extern void (*_php_var_serialize)(smart_str *, zval **, php_serialize_data_t * TSRMLS_DC);
#define php_var_serialize(buf, struc, data) _php_var_serialize(buf, &struc, data TSRMLS_CC)
extern int (*_php_var_unserialize)(zval **, const unsigned char **, const unsigned char *, php_unserialize_data_t * TSRMLS_DC);
#define php_var_unserialize(rval, p, max, var_hash) _php_var_unserialize(&rval, p, max, var_hash TSRMLS_CC)
typedef int strlen_t;

#define PHPREDIS_ZVAL_IS_STRICT_FALSE(z) (Z_TYPE_P(z) == IS_BOOL && !Z_BVAL_P(z))

/* If ZEND_MOD_END isn't defined, use legacy version */
#ifndef ZEND_MOD_END
#define ZEND_MOD_END { NULL, NULL, NULL }
#endif

/* PHP_FE_END exists since 5.3.7 */
#ifndef PHP_FE_END
#define PHP_FE_END { NULL, NULL, NULL }
#endif

/* References don't need any actions */
#define ZVAL_DEREF(v) PHPREDIS_NOTUSED(v)

#define PHPREDIS_GET_OBJECT(class_entry, z) (class_entry *)zend_objects_get_address(z TSRMLS_CC)

#else
#include <zend_smart_str.h>
#include <ext/standard/php_smart_string.h>
typedef size_t strlen_t;
#define PHPREDIS_ZVAL_IS_STRICT_FALSE(z) (Z_TYPE_P(z) == IS_FALSE)
#define PHPREDIS_GET_OBJECT(class_entry, z) (class_entry *)((char *)Z_OBJ_P(z) - XtOffsetOf(class_entry, std))

#define REDIS_MAKE_STD_ZVAL(zv) do {} while(0)
#define REDIS_FREE_ZVAL(zv) do {} while(0)
#endif

/* NULL check so Eclipse doesn't go crazy */
#ifndef NULL
#define NULL   ((void *) 0)
#endif

#define REDIS_SOCK_STATUS_FAILED       0
#define REDIS_SOCK_STATUS_DISCONNECTED 1
#define REDIS_SOCK_STATUS_CONNECTED    2

#define _NL "\r\n"

/* properties */
#define REDIS_NOT_FOUND 0
#define REDIS_STRING    1
#define REDIS_SET       2
#define REDIS_LIST      3
#define REDIS_ZSET      4
#define REDIS_HASH      5

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

/* cluster options */
#define REDIS_FAILOVER_NONE              0
#define REDIS_FAILOVER_ERROR             1
#define REDIS_FAILOVER_DISTRIBUTE        2
#define REDIS_FAILOVER_DISTRIBUTE_SLAVES 3
/* serializers */
#define REDIS_SERIALIZER_NONE        0
#define REDIS_SERIALIZER_PHP         1
#define REDIS_SERIALIZER_IGBINARY    2
/* compression */
#define REDIS_COMPRESSION_NONE 0
#define REDIS_COMPRESSION_LZF  1

/* SCAN options */
#define REDIS_SCAN_NORETRY 0
#define REDIS_SCAN_RETRY 1

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
    if(redis_sock_write(redis_sock, cmd, cmd_len TSRMLS_CC) < 0) { \
    efree(cmd); \
    RETURN_FALSE; \
}

#define REDIS_SAVE_CALLBACK(callback, closure_context) do { \
    fold_item *fi = malloc(sizeof(fold_item)); \
    fi->fun = (void *)callback; \
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
        if (redis_response_enqueued(redis_sock TSRMLS_CC) != SUCCESS) { \
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
    if ((redis_sock = redis_sock_get(getThis() TSRMLS_CC, 0)) == NULL || \
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
    if ((redis_sock = redis_sock_get(getThis() TSRMLS_CC, 0)) == NULL || \
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

/* Extended SET argument detection */
#define IS_EX_ARG(a) \
    ((a[0]=='e' || a[0]=='E') && (a[1]=='x' || a[1]=='X') && a[2]=='\0')
#define IS_PX_ARG(a) \
    ((a[0]=='p' || a[0]=='P') && (a[1]=='x' || a[1]=='X') && a[2]=='\0')
#define IS_NX_ARG(a) \
    ((a[0]=='n' || a[0]=='N') && (a[1]=='x' || a[1]=='X') && a[2]=='\0')
#define IS_XX_ARG(a) \
    ((a[0]=='x' || a[0]=='X') && (a[1]=='x' || a[1]=='X') && a[2]=='\0')

#define IS_EX_PX_ARG(a) (IS_EX_ARG(a) || IS_PX_ARG(a))
#define IS_NX_XX_ARG(a) (IS_NX_ARG(a) || IS_XX_ARG(a))

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

typedef struct fold_item {
    zval * (*fun)(INTERNAL_FUNCTION_PARAMETERS, void *, ...);
    void *ctx;
    struct fold_item *next;
} fold_item;

/* {{{ struct RedisSock */
typedef struct {
    php_stream     *stream;
    zend_string    *host;
    short          port;
    zend_string    *auth;
    double         timeout;
    double         read_timeout;
    long           retry_interval;
    int            failed;
    int            status;
    int            persistent;
    int            watching;
    zend_string    *persistent_id;

    int            serializer;
    int            compression;
    long           dbNumber;

    zend_string    *prefix;

    short          mode;
    fold_item      *head;
    fold_item      *current;

    zend_string    *pipeline_cmd;

    zend_string    *err;

    int            scan;

    int            readonly;
    int            tcp_keepalive;
} RedisSock;
/* }}} */

typedef struct {
    zend_llist list;
    int nb_active;
} ConnectionPool;

#if (PHP_MAJOR_VERSION < 7)
typedef struct {
    zend_object std;
    RedisSock *sock;
} redis_object;
#else
typedef struct {
    RedisSock *sock;
    zend_object std;
} redis_object;
#endif

/** Argument info for any function expecting 0 args */
ZEND_BEGIN_ARG_INFO_EX(arginfo_void, 0, 0, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_key, 0, 0, 1)
    ZEND_ARG_INFO(0, key)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_value, 0, 0, 1)
    ZEND_ARG_INFO(0, value)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_key_value, 0, 0, 2)
    ZEND_ARG_INFO(0, key)
    ZEND_ARG_INFO(0, value)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_key_expire_value, 0, 0, 3)
    ZEND_ARG_INFO(0, key)
    ZEND_ARG_INFO(0, expire)
    ZEND_ARG_INFO(0, value)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_key_newkey, 0, 0, 2)
    ZEND_ARG_INFO(0, key)
    ZEND_ARG_INFO(0, newkey)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_pairs, 0, 0, 1)
    ZEND_ARG_ARRAY_INFO(0, pairs, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_nkeys, 0, 0, 1)
    ZEND_ARG_INFO(0, key)
#if PHP_VERSION_ID >= 50600
    ZEND_ARG_VARIADIC_INFO(0, other_keys)
#else
    ZEND_ARG_INFO(0, ...)
#endif
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_dst_nkeys, 0, 0, 2)
    ZEND_ARG_INFO(0, dst)
    ZEND_ARG_INFO(0, key)
#if PHP_VERSION_ID >= 50600
    ZEND_ARG_VARIADIC_INFO(0, other_keys)
#else
    ZEND_ARG_INFO(0, ...)
#endif
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_key_min_max, 0, 0, 3)
    ZEND_ARG_INFO(0, key)
    ZEND_ARG_INFO(0, min)
    ZEND_ARG_INFO(0, max)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_key_member, 0, 0, 2)
    ZEND_ARG_INFO(0, key)
    ZEND_ARG_INFO(0, member)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_key_member_value, 0, 0, 3)
    ZEND_ARG_INFO(0, key)
    ZEND_ARG_INFO(0, member)
    ZEND_ARG_INFO(0, value)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_key_members, 0, 0, 2)
    ZEND_ARG_INFO(0, key)
    ZEND_ARG_INFO(0, member)
#if PHP_VERSION_ID >= 50600
    ZEND_ARG_VARIADIC_INFO(0, other_members)
#else
    ZEND_ARG_INFO(0, ...)
#endif
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_key_timestamp, 0, 0, 2)
    ZEND_ARG_INFO(0, key)
    ZEND_ARG_INFO(0, timestamp)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_key_offset, 0, 0, 2)
    ZEND_ARG_INFO(0, key)
    ZEND_ARG_INFO(0, offset)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_key_offset_value, 0, 0, 3)
    ZEND_ARG_INFO(0, key)
    ZEND_ARG_INFO(0, offset)
    ZEND_ARG_INFO(0, value)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_swapdb, 0, 0, 2)
    ZEND_ARG_INFO(0, srcdb)
    ZEND_ARG_INFO(0, dstdb)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_key_start_end, 0, 0, 3)
    ZEND_ARG_INFO(0, key)
    ZEND_ARG_INFO(0, start)
    ZEND_ARG_INFO(0, end)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_echo, 0, 0, 1)
    ZEND_ARG_INFO(0, msg)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_expire, 0, 0, 2)
    ZEND_ARG_INFO(0, key)
    ZEND_ARG_INFO(0, timeout)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_set, 0, 0, 2)
    ZEND_ARG_INFO(0, key)
    ZEND_ARG_INFO(0, value)
    ZEND_ARG_INFO(0, opts)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_lset, 0, 0, 3)
    ZEND_ARG_INFO(0, key)
    ZEND_ARG_INFO(0, index)
    ZEND_ARG_INFO(0, value)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_blrpop, 0, 0, 2)
    ZEND_ARG_INFO(0, key)
    ZEND_ARG_INFO(0, timeout_or_key)
// Can't have variadic keys before timeout.
#if PHP_VERSION_ID >= 50600
    ZEND_ARG_VARIADIC_INFO(0, extra_args)
#else
    ZEND_ARG_INFO(0, ...)
#endif
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_linsert, 0, 0, 4)
    ZEND_ARG_INFO(0, key)
    ZEND_ARG_INFO(0, position)
    ZEND_ARG_INFO(0, pivot)
    ZEND_ARG_INFO(0, value)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_lindex, 0, 0, 2)
    ZEND_ARG_INFO(0, key)
    ZEND_ARG_INFO(0, index)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_brpoplpush, 0, 0, 3)
    ZEND_ARG_INFO(0, src)
    ZEND_ARG_INFO(0, dst)
    ZEND_ARG_INFO(0, timeout)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_rpoplpush, 0, 0, 2)
    ZEND_ARG_INFO(0, src)
    ZEND_ARG_INFO(0, dst)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_sadd_array, 0, 0, 2)
    ZEND_ARG_INFO(0, key)
    ZEND_ARG_ARRAY_INFO(0, options, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_srand_member, 0, 0, 1)
    ZEND_ARG_INFO(0, key)
    ZEND_ARG_INFO(0, count)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_zadd, 0, 0, 3)
    ZEND_ARG_INFO(0, key)
    ZEND_ARG_INFO(0, score)
    ZEND_ARG_INFO(0, value)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_zincrby, 0, 0, 3)
    ZEND_ARG_INFO(0, key)
    ZEND_ARG_INFO(0, value)
    ZEND_ARG_INFO(0, member)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_hmget, 0, 0, 2)
    ZEND_ARG_INFO(0, key)
    ZEND_ARG_ARRAY_INFO(0, keys, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_hmset, 0, 0, 2)
    ZEND_ARG_INFO(0, key)
    ZEND_ARG_ARRAY_INFO(0, pairs, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_bitop, 0, 0, 3)
    ZEND_ARG_INFO(0, operation)
    ZEND_ARG_INFO(0, ret_key)
    ZEND_ARG_INFO(0, key)
#if PHP_VERSION_ID >= 50600
    ZEND_ARG_VARIADIC_INFO(0, other_keys)
#else
    ZEND_ARG_INFO(0, ...)
#endif
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_bitpos, 0, 0, 2)
    ZEND_ARG_INFO(0, key)
    ZEND_ARG_INFO(0, bit)
    ZEND_ARG_INFO(0, start)
    ZEND_ARG_INFO(0, end)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_ltrim, 0, 0, 3)
    ZEND_ARG_INFO(0, key)
    ZEND_ARG_INFO(0, start)
    ZEND_ARG_INFO(0, stop)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_publish, 0, 0, 2)
    ZEND_ARG_INFO(0, channel)
    ZEND_ARG_INFO(0, message)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_pfadd, 0, 0, 2)
    ZEND_ARG_INFO(0, key)
    ZEND_ARG_ARRAY_INFO(0, elements, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_pfmerge, 0, 0, 2)
    ZEND_ARG_INFO(0, dstkey)
    ZEND_ARG_ARRAY_INFO(0, keys, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_restore, 0, 0, 3)
    ZEND_ARG_INFO(0, ttl)
    ZEND_ARG_INFO(0, key)
    ZEND_ARG_INFO(0, value)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_smove, 0, 0, 3)
    ZEND_ARG_INFO(0, src)
    ZEND_ARG_INFO(0, dst)
    ZEND_ARG_INFO(0, value)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_zrange, 0, 0, 3)
    ZEND_ARG_INFO(0, key)
    ZEND_ARG_INFO(0, start)
    ZEND_ARG_INFO(0, end)
    ZEND_ARG_INFO(0, scores)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_zrangebyscore, 0, 0, 3)
    ZEND_ARG_INFO(0, key)
    ZEND_ARG_INFO(0, start)
    ZEND_ARG_INFO(0, end)
    ZEND_ARG_ARRAY_INFO(0, options, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_zrangebylex, 0, 0, 3)
    ZEND_ARG_INFO(0, key)
    ZEND_ARG_INFO(0, min)
    ZEND_ARG_INFO(0, max)
    ZEND_ARG_INFO(0, offset)
    ZEND_ARG_INFO(0, limit)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_zstore, 0, 0, 2)
    ZEND_ARG_INFO(0, key)
    ZEND_ARG_ARRAY_INFO(0, keys, 0)
    ZEND_ARG_ARRAY_INFO(0, weights, 1)
    ZEND_ARG_INFO(0, aggregate)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_sort, 0, 0, 1)
    ZEND_ARG_INFO(0, key)
    ZEND_ARG_ARRAY_INFO(0, options, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_object, 0, 0, 2)
    ZEND_ARG_INFO(0, field)
    ZEND_ARG_INFO(0, key)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_subscribe, 0, 0, 2)
    ZEND_ARG_ARRAY_INFO(0, channels, 0)
    ZEND_ARG_INFO(0, callback)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_psubscribe, 0, 0, 2)
    ZEND_ARG_ARRAY_INFO(0, patterns, 0)
    ZEND_ARG_INFO(0, callback)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_unsubscribe, 0, 0, 1)
    ZEND_ARG_INFO(0, channel)
#if PHP_VERSION_ID >= 50600
    ZEND_ARG_VARIADIC_INFO(0, other_channels)
#else
    ZEND_ARG_INFO(0, ...)
#endif
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_punsubscribe, 0, 0, 1)
    ZEND_ARG_INFO(0, pattern)
#if PHP_VERSION_ID >= 50600
    ZEND_ARG_VARIADIC_INFO(0, other_patterns)
#else
    ZEND_ARG_INFO(0, ...)
#endif
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_eval, 0, 0, 1)
    ZEND_ARG_INFO(0, script)
    ZEND_ARG_INFO(0, args)
    ZEND_ARG_INFO(0, num_keys)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_evalsha, 0, 0, 1)
    ZEND_ARG_INFO(0, script_sha)
    ZEND_ARG_INFO(0, args)
    ZEND_ARG_INFO(0, num_keys)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_getoption, 0, 0, 1)
    ZEND_ARG_INFO(0, option)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_setoption, 0, 0, 2)
    ZEND_ARG_INFO(0, option)
    ZEND_ARG_INFO(0, value)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_watch, 0, 0, 1)
    ZEND_ARG_INFO(0, key)
#if PHP_VERSION_ID >= 50600
    ZEND_ARG_VARIADIC_INFO(0, other_keys)
#else
    ZEND_ARG_INFO(0, ...)
#endif
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_command, 0, 0, 0)
#if PHP_VERSION_ID >= 50600
    ZEND_ARG_VARIADIC_INFO(0, args)
#else
    ZEND_ARG_INFO(0, ...)
#endif
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_rawcommand, 0, 0, 1)
    ZEND_ARG_INFO(0, cmd)
#if PHP_VERSION_ID >= 50600
    ZEND_ARG_VARIADIC_INFO(0, args)
#else
    ZEND_ARG_INFO(0, ...)
#endif
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_geoadd, 0, 0, 4)
    ZEND_ARG_INFO(0, key)
    ZEND_ARG_INFO(0, lng)
    ZEND_ARG_INFO(0, lat)
    ZEND_ARG_INFO(0, member)
#if PHP_VERSION_ID >= 50600
    ZEND_ARG_VARIADIC_INFO(0, other_triples)
#else
    ZEND_ARG_INFO(0, ...)
#endif
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_geodist, 0, 0, 3)
    ZEND_ARG_INFO(0, key)
    ZEND_ARG_INFO(0, src)
    ZEND_ARG_INFO(0, dst)
    ZEND_ARG_INFO(0, unit)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_georadius, 0, 0, 5)
    ZEND_ARG_INFO(0, key)
    ZEND_ARG_INFO(0, lng)
    ZEND_ARG_INFO(0, lan)
    ZEND_ARG_INFO(0, radius)
    ZEND_ARG_INFO(0, unit)
    ZEND_ARG_ARRAY_INFO(0, opts, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_georadiusbymember, 0, 0, 4)
    ZEND_ARG_INFO(0, key)
    ZEND_ARG_INFO(0, member)
    ZEND_ARG_INFO(0, radius)
    ZEND_ARG_INFO(0, unit)
    ZEND_ARG_ARRAY_INFO(0, opts, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_xadd, 0, 0, 3)
    ZEND_ARG_INFO(0, str_key)
    ZEND_ARG_INFO(0, str_id)
    ZEND_ARG_ARRAY_INFO(0, arr_fields, 0)
    ZEND_ARG_INFO(0, i_maxlen)
    ZEND_ARG_INFO(0, boo_approximate)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_xpending, 0, 0, 2)
    ZEND_ARG_INFO(0, str_key)
    ZEND_ARG_INFO(0, str_group)
    ZEND_ARG_INFO(0, str_start)
    ZEND_ARG_INFO(0, str_end)
    ZEND_ARG_INFO(0, i_count)
    ZEND_ARG_INFO(0, str_consumer)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_xrange, 0, 0, 3)
    ZEND_ARG_INFO(0, str_key)
    ZEND_ARG_INFO(0, str_start)
    ZEND_ARG_INFO(0, str_end)
    ZEND_ARG_INFO(0, i_count)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_xread, 0, 0, 1)
    ZEND_ARG_ARRAY_INFO(0, arr_streams, 0)
    ZEND_ARG_INFO(0, i_count)
    ZEND_ARG_INFO(0, i_block)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_xreadgroup, 0, 0, 3)
    ZEND_ARG_INFO(0, str_group)
    ZEND_ARG_INFO(0, str_consumer)
    ZEND_ARG_ARRAY_INFO(0, arr_streams, 0)
    ZEND_ARG_INFO(0, i_count)
    ZEND_ARG_INFO(0, i_block)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_xack, 0, 0, 3)
    ZEND_ARG_INFO(0, str_key)
    ZEND_ARG_INFO(0, str_group)
    ZEND_ARG_ARRAY_INFO(0, arr_ids, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_xclaim, 0, 0, 5)
    ZEND_ARG_INFO(0, str_key)
    ZEND_ARG_INFO(0, str_group)
    ZEND_ARG_INFO(0, str_consumer)
    ZEND_ARG_INFO(0, i_min_idle)
    ZEND_ARG_ARRAY_INFO(0, arr_ids, 0)
    ZEND_ARG_ARRAY_INFO(0, arr_opts, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_xgroup, 0, 0, 1)
    ZEND_ARG_INFO(0, str_operation)
    ZEND_ARG_INFO(0, str_key)
    ZEND_ARG_INFO(0, str_arg1)
    ZEND_ARG_INFO(0, str_arg2)
    ZEND_ARG_INFO(0, str_arg3)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_xinfo, 0, 0, 1)
    ZEND_ARG_INFO(0, str_cmd)
    ZEND_ARG_INFO(0, str_key)
    ZEND_ARG_INFO(0, str_group)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_xtrim, 0, 0, 2)
    ZEND_ARG_INFO(0, str_key)
    ZEND_ARG_INFO(0, i_maxlen)
    ZEND_ARG_INFO(0, boo_approximate)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_xdel, 0, 0, 2)
    ZEND_ARG_INFO(0, str_key)
    ZEND_ARG_ARRAY_INFO(0, arr_ids, 0)
ZEND_END_ARG_INFO()

#endif
