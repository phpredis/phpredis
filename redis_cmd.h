#ifndef REDIS_CMD_H
#define REDIS_CMD_H

#include "php.h"
#include <zend_smart_string.h>

#include "common.h"

typedef struct RedisCmd {
    RedisSock *redis_sock;
    short slot;
    smart_string s;
    RedisCmdCtx ctx;
    uint32_t argc;
    char *head;
} RedisCmd;

#define redis_cmd_create_literal(redis_sock, kw) \
    redis_cmd_create(redis_sock, kw, sizeof(kw) - 1)

#define redis_cmd_cat_literal(cmd, lit) \
    redis_cmd_cat_str(cmd, lit, sizeof(lit) - 1)

#define redis_cmd_cat_literal_if(cmd, opt, lit) do { \
    if (opt) \
        redis_cmd_cat_str(cmd, lit, sizeof(lit) - 1); \
} while (0)

#define redis_cmd_cat_zstr_if(cmd, opt, zstr) do { \
    if (opt) \
        redis_cmd_cat_zstr(cmd, zstr); \
} while (0)

RedisCmd *redis_cmd_create(RedisSock *redis_sock, const char *kw, size_t kwlen);
void redis_cmd_reset(RedisCmd *cmd, const char *kw, size_t kwlen);
void redis_cmd_free_ex(RedisCmd *cmd, int free_ctx);
void redis_cmd_free_ctx(RedisCmd *cmd);

static inline RedisCmdCtx redis_cmd_pop_ctx(RedisCmd *cmd) {
    RedisCmdCtx ctx = cmd->ctx;

    memset(&cmd->ctx, 0, sizeof(RedisCmdCtx));

    return ctx;
}

static inline void redis_cmd_free(RedisCmd *cmd) {
    redis_cmd_free_ex(cmd, 0);
}

static inline RedisCmd *
redis_cmd_create_zstr(RedisSock *redis_sock, zend_string *kw) {
    return redis_cmd_create(redis_sock, ZSTR_VAL(kw), ZSTR_LEN(kw));
}

void redis_cmd_cat_zstr(RedisCmd *cmd, zend_string *str);
void redis_cmd_cat_str(RedisCmd *cmd, const char *str, size_t len);
void redis_cmd_cat_long(RedisCmd *cmd, zend_long lval);
void redis_cmd_cat_ulong(RedisCmd *cmd, zend_ulong ulval);
void redis_cmd_cat_u64(RedisCmd *cmd, uint64_t u64);
void redis_cmd_cat_double(RedisCmd *cmd, double dval);

zend_bool redis_cmd_cat_key_zval(RedisCmd *cmd, zval *zv);
zend_bool redis_cmd_cat_key_zstr(RedisCmd *cmd, zend_string *key);
zend_bool redis_cmd_cat_key_long(RedisCmd *cmd, zend_long key);
zend_bool redis_cmd_cat_key_str(RedisCmd *cmd, const char *key, size_t len);
zend_bool redis_cmd_cat_key_arrkey(RedisCmd *cmd, zend_string *str, zend_ulong idx);

void redis_cmd_cat_zval(RedisCmd *cmd, zval *zv);
void redis_cmd_cat_zval_zstr(RedisCmd *cmd, zval *zv);

void redis_cmd_cat_arrkey(RedisCmd *cmd, zend_string *str, zend_ulong idx);

void redis_cmd_set_ctx_ex(RedisCmd *cmd, void *ptr, RedisCmdCtxDtor *dtor);

static zend_always_inline void redis_cmd_set_ctx(RedisCmd *cmd, void *ptr) {
    redis_cmd_set_ctx_ex(cmd, ptr, NULL);
}

static zend_always_inline void
redis_cmd_set_ctx_u64(RedisCmd *cmd, uint64_t u64) {
    redis_cmd_set_ctx(cmd, (void*)(uintptr_t)u64);
}

void redis_cmd_randslot(RedisCmd *cmd);

static inline void redis_cmd_ctx_efree(void *ptr) {
    if (ptr) efree(ptr);
}

RedisCmd *redis_cmd_fmt(RedisSock *redis_sock, const char *kw, const char *fmt, ...);
RedisCmd *redis_cmd_fmt_ex(RedisSock *redis_sock, const char *kw, size_t kw_len,
                           const char *fmt, ...);

const char *redis_cmd_str(RedisCmd *cmd);
size_t redis_cmd_len(RedisCmd *cmd);

/* Specialized thin wrapper around smart_str for the times we want to construct
 * one buffer with 1-N commands */
void resp_str_cat_str(smart_str *s, const char *str, zend_ulong len);
void resp_str_cat_cmd(smart_str *s, zend_ulong argc, const char *kw, size_t len);
void resp_str_cat_zstr(smart_str *s, zend_string *str);

#define resp_str_cat_cmd_literal(s_, kw_, argc_) \
    resp_str_cat_cmd(s_, argc_, kw_, sizeof(kw_) - 1)

#endif // REDIS_CMD_H
