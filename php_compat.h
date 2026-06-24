#ifndef PHP_REDIS_COMPAT_H
#define PHP_REDIS_COMPAT_H

#include "php.h"
#include <ext/hash/php_hash.h>

#if PHP_VERSION_ID < 80400
#include <ext/standard/php_random.h>
#include <ext/standard/php_rand.h>
#else
#include <ext/random/php_random.h>
#endif

#if PHP_VERSION_ID < 80000

static zend_always_inline zend_string *
zend_string_concat2(const char *str1, size_t len1, const char *str2,
                    size_t len2)
{
    size_t len = len1 + len2;
    zend_string *res = zend_string_alloc(len, 0);

    memcpy(ZSTR_VAL(res), str1, len1);
    memcpy(ZSTR_VAL(res) + len1, str2, len2);
    ZSTR_VAL(res)[len] = '\0';

    return res;
}


static zend_always_inline const php_hash_ops *
redis_hash_fetch_ops(zend_string *zstr)
{
    return php_hash_fetch_ops(ZSTR_VAL(zstr), ZSTR_LEN(zstr));
}

#ifndef RETURN_THROWS
#define RETURN_THROWS() RETURN_FALSE
#endif

#ifndef ZVAL_STRINGL_FAST
#define ZVAL_STRINGL_FAST(z, s, l) ZVAL_STRINGL(z, s, l)
#endif

#ifndef RETVAL_STRINGL_FAST
#define RETVAL_STRINGL_FAST(s, l) RETVAL_STRINGL(s, l)
#endif

#ifndef Z_PARAM_ARRAY_OR_NULL
#define Z_PARAM_ARRAY_OR_NULL(dest) \
        Z_PARAM_ARRAY_EX(dest, 1, 0)
#endif

#ifndef Z_PARAM_ARRAY_HT_OR_NULL
#define Z_PARAM_ARRAY_HT_OR_NULL(dest) \
        Z_PARAM_ARRAY_HT_EX(dest, 1, 0)
#endif

#ifndef Z_PARAM_STRING_OR_NULL
#define Z_PARAM_STRING_OR_NULL(dest, dest_len) \
        Z_PARAM_STRING_EX(dest, dest_len, 1, 0)
#endif

#ifndef Z_PARAM_STR_OR_NULL
#define Z_PARAM_STR_OR_NULL(dest) \
        Z_PARAM_STR_EX(dest, 1, 0)
#endif

#ifndef Z_PARAM_LONG_OR_NULL
#define Z_PARAM_LONG_OR_NULL(dest, is_null) \
        Z_PARAM_LONG_EX(dest, is_null, 1, 0)
#endif

#ifndef Z_PARAM_ZVAL_OR_NULL
#define Z_PARAM_ZVAL_OR_NULL(dest) \
        Z_PARAM_ZVAL_EX(dest, 1, 0)
#endif

#ifndef Z_PARAM_BOOL_OR_NULL
#define Z_PARAM_BOOL_OR_NULL(dest, is_null) \
        Z_PARAM_BOOL_EX(dest, is_null, 1, 0)
#endif
#else
static zend_always_inline const php_hash_ops *
redis_hash_fetch_ops(zend_string *zstr)
{
    return php_hash_fetch_ops(zstr);
}
#endif

#if PHP_VERSION_ID < 80200
static zend_always_inline zend_bool
zend_string_starts_with_cstr(const zend_string *str, const char *prefix,
                             size_t prefix_length)
{
    return ZSTR_LEN(str) >= prefix_length &&
        !memcmp(ZSTR_VAL(str), prefix, prefix_length);
}

static zend_always_inline HashTable *
zend_array_to_list(HashTable *arr)
{
    zval zret = {0}, *zv;

    array_init_size(&zret, zend_hash_num_elements(arr));

    ZEND_HASH_FOREACH_VAL(arr, zv) {
        Z_TRY_ADDREF_P(zv);
        add_next_index_zval(&zret, zv);
    } ZEND_HASH_FOREACH_END();

    return Z_ARRVAL(zret);
}
#endif

#endif // PHP_REDIS_COMPAT_H
