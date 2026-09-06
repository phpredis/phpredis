#include "php.h"
#include "common.h"
#include "redis_cmd.h"
#include "library.h"
#include "cluster_library.h"
#include "Zend/zend_smart_str.h"

// Max possible:     "*4294967295\r\n"
#define REDIS_MB_HDR "PHPREDISPHP\r\n"

#define REDIS_MB_HDR_LEN (sizeof(REDIS_MB_HDR) - 1)

typedef struct RespHeader {
    char *str;
    size_t len;
} RespHeader;

static const RespHeader bulk_header[] = {
    {ZEND_STRL("$0\r\n")},  {ZEND_STRL("$1\r\n")},  {ZEND_STRL("$2\r\n")},
    {ZEND_STRL("$3\r\n")},  {ZEND_STRL("$4\r\n")},  {ZEND_STRL("$5\r\n")},
    {ZEND_STRL("$6\r\n")},  {ZEND_STRL("$7\r\n")},  {ZEND_STRL("$8\r\n")},
    {ZEND_STRL("$9\r\n")},  {ZEND_STRL("$10\r\n")}, {ZEND_STRL("$11\r\n")},
    {ZEND_STRL("$12\r\n")}, {ZEND_STRL("$13\r\n")}, {ZEND_STRL("$14\r\n")},
    {ZEND_STRL("$15\r\n")}, {ZEND_STRL("$16\r\n")}, {ZEND_STRL("$17\r\n")},
    {ZEND_STRL("$18\r\n")}, {ZEND_STRL("$19\r\n")}, {ZEND_STRL("$20\r\n")},
    {ZEND_STRL("$21\r\n")}, {ZEND_STRL("$22\r\n")}, {ZEND_STRL("$23\r\n")},
    {ZEND_STRL("$24\r\n")}, {ZEND_STRL("$25\r\n")}, {ZEND_STRL("$26\r\n")},
    {ZEND_STRL("$27\r\n")}, {ZEND_STRL("$28\r\n")}, {ZEND_STRL("$29\r\n")},
    {ZEND_STRL("$30\r\n")}, {ZEND_STRL("$31\r\n")}, {ZEND_STRL("$32\r\n")},
};

static zend_always_inline const RespHeader *
bulk_header_get(size_t n)
{
    return &bulk_header[n];
}

/* zend_print_ulong_to_buf will null terminate the string which we do not want */
static inline char *print_u64_to_buf(char *buf, uint64_t n) {
    do {
        *--buf = (char)(n % 10) + '0';
        n /= 10;
    } while (n > 0);

    return buf;
}

/* zend_print_ulong_to_buf will null terminate the string which we do not want */
static inline char *print_u32_to_buf(char *buf, uint32_t n) {
    return print_u64_to_buf(buf, n);
}

static RedisCmd *redis_cmd_alloc(RedisSock *redis_sock) {
    RedisCmd *cmd;

    cmd = ecalloc(1, sizeof(*cmd));
    cmd->redis_sock = redis_sock;
    cmd->slot = -1;

    return cmd;
}

RedisCmd *
redis_cmd_create(RedisSock *redis_sock, const char *kw, size_t kwlen) {
    RedisCmd *cmd;

    cmd = redis_cmd_alloc(redis_sock);

    smart_string_appendl_ex(&cmd->s, ZEND_STRL(REDIS_MB_HDR), 0);

    redis_cmd_cat_str(cmd, kw, kwlen);

    return cmd;
}

void redis_cmd_reset(RedisCmd *cmd, const char *kw, size_t kwlen) {
    redis_cmd_free_ctx(cmd);
    smart_string_reset(&cmd->s);

    cmd->argc = 0;
    cmd->head = NULL;
    cmd->slot = -1;
    cmd->slot_is_random = 0;

    smart_string_appendl_ex(&cmd->s, ZEND_STRL(REDIS_MB_HDR), 0);
    redis_cmd_cat_str(cmd, kw, kwlen);
}

void redis_cmd_free_ctx(RedisCmd *cmd) {
    redis_cmd_ctx_free(cmd->ctx);
    memset(&cmd->ctx, 0, sizeof(RedisCmdCtx));
}

void redis_cmd_free_ex(RedisCmd *cmd, int free_ctx) {
    if (cmd == NULL)
        return;

    smart_string_free(&cmd->s);

    if (free_ctx) {
        redis_cmd_free_ctx(cmd);
    }

    efree(cmd);
}

static void redis_cmd_finalize(RedisCmd *cmd) {
    char *eptr;

    if (cmd->head != NULL)
        return;

    if (cmd->redis_sock != NULL && cmd->redis_sock->type == REDIS_SOCK_CLUSTER) {
        ZEND_ASSERT(cmd->slot >= 0 && cmd->slot < REDIS_CLUSTER_SLOTS);

        if (UNEXPECTED(cmd->slot < 0 || cmd->slot >= REDIS_CLUSTER_SLOTS)) {
            zend_error_noreturn(E_ERROR, "Invalid Redis Cluster slot %d", cmd->slot);
        }
    }

    /* Set the end pointer right before the trailing \r\n and print argc */
    eptr = cmd->s.c + REDIS_MB_HDR_LEN - 2;
    cmd->head = print_u32_to_buf(eptr, cmd->argc);

    /* Prepend the multibulk reply-type */
    *--cmd->head = '*';

    /* Null-terminate the string */
    smart_string_0(&cmd->s);
}

void redis_cmd_cat_str(RedisCmd *cmd, const char *str, size_t len) {
    const RespHeader *hdr;

    if (cmd->argc == UINT32_MAX) {
        zend_error_noreturn(E_ERROR, "Too many arguments");
    }

    if (EXPECTED(len < sizeof(bulk_header) / sizeof(RespHeader))) {
        hdr = bulk_header_get(len);
        smart_string_appendl_ex(&cmd->s, hdr->str, hdr->len, 0);
    } else {
        smart_string_appendc(&cmd->s, '$');
        smart_string_append_unsigned(&cmd->s, len);
        smart_string_appendl_ex(&cmd->s, ZEND_STRL("\r\n"), 0);
    }

    smart_string_appendl(&cmd->s, str, len);
    smart_string_appendl_ex(&cmd->s, ZEND_STRL("\r\n"), 0);

    cmd->argc++;
}

void redis_cmd_cat_arrkey(RedisCmd *cmd, zend_string *str, zend_ulong idx) {
    if (str) {
        redis_cmd_cat_zstr(cmd, str);
    } else {
        redis_cmd_cat_ulong(cmd, idx);
    }
}

static inline zend_bool
update_slot(RedisCmd *cmd, const char *key, size_t keylen) {
    short slot;

    if (cmd->redis_sock == NULL || cmd->redis_sock->type != REDIS_SOCK_CLUSTER)
        return 1;

    slot = cluster_hash_key(key, keylen);

    if (cmd->slot == -1) {
        cmd->slot = slot;
        return 1;
    }

    return cmd->slot == slot ? 1 : 0;
}

zend_bool redis_cmd_cat_key_str(RedisCmd *cmd, const char *key, size_t len) {
    char *aux = (char*)key;
    int keyfree;

    if (UNEXPECTED(cmd->redis_sock == NULL)) {
        redis_cmd_cat_str(cmd, key, len);
        return 1;
    }

    keyfree = redis_key_prefix(cmd->redis_sock, &aux, &len);

    if (!update_slot(cmd, aux, len)) {
        if (keyfree) efree(aux);
        return 0;
    }

    redis_cmd_cat_str(cmd, aux, len);
    if (keyfree) efree(aux);

    return 1;
}

zend_bool redis_cmd_cat_key_zval(RedisCmd *cmd, zval *zv) {
    zend_string *zstr, *tmp;
    int ret;

    zstr = zval_get_tmp_string(zv, &tmp);
    ret = redis_cmd_cat_key_zstr(cmd, zstr);
    zend_tmp_string_release(tmp);

    return ret;
}

zend_bool redis_cmd_cat_key_zstr(RedisCmd *cmd, zend_string *key) {
    if (UNEXPECTED(cmd->redis_sock == NULL)) {
        redis_cmd_cat_zstr(cmd, key);
        return 1;
    }

    key = redis_key_prefix_zstr(cmd->redis_sock, key);

    if (!update_slot(cmd, ZSTR_VAL(key), ZSTR_LEN(key))) {
        zend_string_release(key);
        return 0;
    }

    redis_cmd_cat_zstr(cmd, key);
    zend_string_release(key);

    return 1;
}

zend_bool redis_cmd_cat_key_long(RedisCmd *cmd, zend_long lval) {
    char buf[21], *key;
    int keyfree;
    size_t len;

    key = zend_print_long_to_buf(buf + sizeof(buf) - 1, lval);
    len = buf + sizeof(buf) - 1 - key;

    if (UNEXPECTED(cmd->redis_sock == NULL)) {
        redis_cmd_cat_str(cmd, key, len);
        return 1;
    }

    keyfree = redis_key_prefix(cmd->redis_sock, &key, &len);

    if (!update_slot(cmd, key, len)) {
        if (keyfree) efree(key);
        return 0;
    }

    redis_cmd_cat_str(cmd, key, len);
    if (keyfree) efree(key);

    return 1;
}

zend_bool redis_cmd_cat_key_arrkey(RedisCmd *cmd, zend_string *str, zend_ulong idx) {
    char buf[21], *key;
    int keyfree;
    size_t len;

    if (str == NULL) {
        key = zend_print_ulong_to_buf(buf + sizeof(buf) - 1, idx);
        len = buf + sizeof(buf) - 1 - key;
    } else {
        key = ZSTR_VAL(str);
        len = ZSTR_LEN(str);
    }

    if (UNEXPECTED(cmd->redis_sock == NULL)) {
        redis_cmd_cat_str(cmd, key, len);
        return 1;
    }

    keyfree = redis_key_prefix(cmd->redis_sock, &key, &len);
    if (!update_slot(cmd, key, len)) {
        if (keyfree) efree(key);
        return 0;
    }

    redis_cmd_cat_str(cmd, key, len);
    if (keyfree) efree(key);

    return 1;
}

void redis_cmd_cat_zstr(RedisCmd *cmd, zend_string *str) {
    redis_cmd_cat_str(cmd, ZSTR_VAL(str), ZSTR_LEN(str));
}

void redis_cmd_cat_long(RedisCmd *cmd, zend_long lval) {
    char buf[21], *res;
    int len;

    res = zend_print_long_to_buf(buf + sizeof(buf) - 1, lval);
    len = buf + sizeof(buf) - 1 - res;

    redis_cmd_cat_str(cmd, res, len);
}

void redis_cmd_cat_ulong(RedisCmd *cmd, zend_ulong lval) {
    char buf[21], *res;
    int len;

    res = zend_print_ulong_to_buf(buf + sizeof(buf) - 1, lval);
    len = buf + sizeof(buf) - 1 - res;

    redis_cmd_cat_str(cmd, res, len);
}

void redis_cmd_cat_u64(RedisCmd *cmd, uint64_t u64) {
    char buf[21], *res;
    int len;

    res = print_u64_to_buf(buf + sizeof(buf) - 1, u64);
    len = buf + sizeof(buf) - 1 - res;

    redis_cmd_cat_str(cmd, res, len);
}

void redis_cmd_cat_double(RedisCmd *cmd, double dval) {
#if PHP_VERSION_ID >= 80200
    char buf[ZEND_DOUBLE_MAX_LENGTH], *nul;

    zend_gcvt(dval, 17, '.', 'e', buf);

    nul = memchr(buf, 0, sizeof(buf));
    ZEND_ASSERT(nul != NULL);

    redis_cmd_cat_str(cmd, buf, nul - buf);

#else
    char buf[64], *p;
    int len;

    len = snprintf(buf, sizeof(buf), "%.17g", dval);

    /* Legacy behavior locale fixup */
    if ((p = strchr(buf, ',')) != NULL)
        *p = '.';

    redis_cmd_cat_str(cmd, buf, len);
#endif
}

void redis_cmd_cat_zval_zstr(RedisCmd *cmd, zval *zv) {
    zend_string *zstr, *tmp;

    switch (Z_TYPE_P(zv)) {
        case IS_FALSE:
            redis_cmd_cat_str(cmd, ZEND_STRL("0"));
            return;
        case IS_TRUE:
            redis_cmd_cat_str(cmd, ZEND_STRL("1"));
            return;
        case IS_LONG:
            redis_cmd_cat_long(cmd, Z_LVAL_P(zv));
            return;
        case IS_DOUBLE:
            redis_cmd_cat_double(cmd, Z_DVAL_P(zv));
            return;
        default:
            zstr = zval_get_tmp_string(zv, &tmp);
            redis_cmd_cat_zstr(cmd, zstr);
            zend_tmp_string_release(tmp);
    }
}

void redis_cmd_cat_zval(RedisCmd *cmd, zval *zv) {
    int valfree;
    size_t len;
    char *str;

    if (UNEXPECTED(cmd->redis_sock == NULL)) {
        redis_cmd_cat_zval_zstr(cmd, zv);
        return;
    }

    valfree = redis_pack(cmd->redis_sock, zv, &str, &len);
    redis_cmd_cat_str(cmd, str, len);

    if (valfree)
        efree(str);
}

/* A printf like method to construct a Redis RESP command.  It has been extended
 * to take a few different format specifiers that are convenient to phpredis.
 *
 * s - C string followed by length as a
 * S - Pointer to a zend_string
 * k - Same as 's' but the value will be prefixed if phpredis is set up do do
 *     that and the working slot will be set if it has been passed.
 * K - Same as 'S' but the value will be prefixed if phpredis is set up do do
 * v - A z_val which will be serialized if phpredis is configured to serialize.
 * f - A double value
 * F - Alias to 'f'
 * i - An integer
 * d - Alias to 'i'
 * l - A long
 * L - Alias to 'l'
 */
static RedisCmd *
redis_cmd_vfmt(RedisSock *redis_sock, const char *kw, size_t kw_len,
               const char *fmt, va_list ap)
{
    RedisCmd *ret;
    size_t len;
    char *str;

    ret = redis_cmd_create(redis_sock, kw, kw_len);

    while (*fmt) {
        switch (*fmt) {
            case 's':
                str = va_arg(ap, char*);
                len = va_arg(ap, size_t);
                redis_cmd_cat_str(ret, str, len);
                break;
            case 'S':
                redis_cmd_cat_zstr(ret, va_arg(ap, zend_string*));
                break;
            case 'k':
                str = va_arg(ap, char*);
                len = va_arg(ap, size_t);
                redis_cmd_cat_key_str(ret, str, len);
                break;
            case 'K':
                redis_cmd_cat_key_zstr(ret, va_arg(ap, zend_string*));
                break;
            case 'v':
                redis_cmd_cat_zval(ret, va_arg(ap, zval*));
                break;
            case 'f':
            case 'F':
                redis_cmd_cat_double(ret, va_arg(ap, double));
                break;
            case 'i':
            case 'd':
                redis_cmd_cat_long(ret, va_arg(ap, int));
                break;
            case 'l':
            case 'L':
                redis_cmd_cat_long(ret, va_arg(ap, zend_long));
                break;
        }

        fmt++;
    }

    return ret;
}

RedisCmd *redis_cmd_fmt(RedisSock *redis_sock, const char *kw, const char *fmt, ...) {
    RedisCmd *ret;
    va_list ap;

    va_start(ap, fmt);
    ret = redis_cmd_vfmt(redis_sock, kw, strlen(kw), fmt, ap);
    va_end(ap);

    return ret;
}

RedisCmd *
redis_cmd_fmt_ex(RedisSock *redis_sock, const char *kw, size_t kw_len,
                 const char *fmt, ...)
{
    RedisCmd *ret;
    va_list ap;

    va_start(ap, fmt);
    ret = redis_cmd_vfmt(redis_sock, kw, kw_len, fmt, ap);
    va_end(ap);

    return ret;
}

void redis_cmd_set_ctx_ex(RedisCmd *cmd, void *ctx, RedisCmdCtxDtor *dtor) {
    cmd->ctx.ptr = ctx;
    cmd->ctx.dtor = dtor;
}

const char *redis_cmd_str(RedisCmd *cmd) {
    redis_cmd_finalize(cmd);

    return cmd->head;
}

size_t redis_cmd_len(RedisCmd *cmd) {
    redis_cmd_finalize(cmd);

    return cmd->s.len - (cmd->head - cmd->s.c);
}

void redis_cmd_randslot(RedisCmd *cmd) {
    if (cmd->redis_sock && cmd->redis_sock->type == REDIS_SOCK_CLUSTER) {
        cmd->slot = rand() % REDIS_CLUSTER_MOD;
        cmd->slot_is_random = 1;
    }
}

void resp_str_cat_str(smart_str *s, const char *str, zend_ulong len) {
    smart_str_appendc(s, '$');
    smart_str_append_unsigned(s, len);
    smart_str_appendl_ex(s, ZEND_STRL("\r\n"), 0);
    smart_str_appendl(s, str, len);
    smart_str_appendl_ex(s, ZEND_STRL("\r\n"), 0);
}

void resp_str_cat_cmd(smart_str *s, zend_ulong argc, const char *kw, size_t len) {
    smart_str_appendc(s, '*');
    smart_str_append_unsigned(s, 1 + argc);
    smart_str_appendl_ex(s, ZEND_STRL("\r\n"), 0);
    resp_str_cat_str(s, kw, len);
}

void resp_str_cat_zstr(smart_str *s, zend_string *str) {
    resp_str_cat_str(s, ZSTR_VAL(str), ZSTR_LEN(str));
}
