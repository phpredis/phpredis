#ifndef REDIS_LIBRARY_H
#define REDIS_LIBRARY_H

/* Non cluster command helper */
#define REDIS_SPPRINTF(ret, kw, fmt, ...) \
    redis_spprintf(redis_sock, NULL, ret, kw, fmt, ##__VA_ARGS__)

#define REDIS_CMD_APPEND_SSTR_STATIC(sstr, str) \
    redis_cmd_append_sstr(sstr, str, sizeof(str)-1);

#define REDIS_CMD_APPEND_SSTR_OPT_STATIC(sstr, opt, str) \
    if (opt) REDIS_CMD_APPEND_SSTR_STATIC(sstr, str);

#define REDIS_CMD_INIT_SSTR_STATIC(sstr, argc, keyword) \
    redis_cmd_init_sstr(sstr, argc, keyword, sizeof(keyword)-1);

#define REDIS_THROW_EXCEPTION(msg, code) \
    zend_throw_exception(redis_exception_ce, (msg), code)

#define CLUSTER_THROW_EXCEPTION(msg, code) \
    zend_throw_exception(redis_cluster_exception_ce, (msg), code)

#define redis_sock_write_sstr(redis_sock, sstr) \
    redis_sock_write(redis_sock, (sstr)->c, (sstr)->len)

#if PHP_VERSION_ID < 80000
    #define redis_hash_fetch_ops(zstr) php_hash_fetch_ops(ZSTR_VAL((zstr)), ZSTR_LEN((zstr)))
#else
    #define redis_hash_fetch_ops(zstr) php_hash_fetch_ops(zstr)
#endif

void redis_register_persistent_resource(zend_string *id, void *ptr, int le_id);

PHP_REDIS_API int redis_extract_auth_info(zval *ztest, zend_string **user, zend_string **pass);

int redis_cmd_init_sstr(smart_string *str, int num_args, char *keyword, int keyword_len);
int redis_cmd_append_sstr(smart_string *str, char *append, int append_len);
int redis_cmd_append_sstr_int(smart_string *str, int append);
int redis_cmd_append_sstr_long(smart_string *str, long append);
int redis_cmd_append_sstr_i64(smart_string *str, int64_t append);
int redis_cmd_append_sstr_dbl(smart_string *str, double value);
int redis_cmd_append_sstr_zstr(smart_string *str, zend_string *zstr);
int redis_cmd_append_sstr_zval(smart_string *str, zval *z, RedisSock *redis_sock);
int redis_cmd_append_sstr_key(smart_string *str, char *key, size_t len, RedisSock *redis_sock, short *slot);
int redis_cmd_append_sstr_arrkey(smart_string *cmd, zend_string *kstr, zend_ulong idx);

PHP_REDIS_API int redis_spprintf(RedisSock *redis_sock, short *slot, char **ret, char *kw, char *fmt, ...);
PHP_REDIS_API zend_string *redis_pool_spprintf(RedisSock *redis_sock, char *fmt, ...);

PHP_REDIS_API char *redis_sock_read(RedisSock *redis_sock, int *buf_len);
PHP_REDIS_API int redis_sock_gets(RedisSock *redis_sock, char *buf, int buf_size, size_t* line_len);
PHP_REDIS_API int redis_1_response(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock, zval *z_tab, void *ctx);
PHP_REDIS_API int redis_long_response(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock, zval* z_tab, void *ctx);
typedef void (*SuccessCallback)(RedisSock *redis_sock);
PHP_REDIS_API int redis_boolean_response_impl(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock, zval *z_tab, void *ctx, SuccessCallback success_callback);
PHP_REDIS_API int redis_boolean_response(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock, zval *z_tab, void *ctx);
PHP_REDIS_API int redis_bulk_double_response(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock, zval *z_tab, void *ctx);
PHP_REDIS_API int redis_string_response(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock, zval *z_tab, void *ctx);
PHP_REDIS_API void redis_single_line_reply(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock,
    zval *z_tab, void *ctx);
PHP_REDIS_API int redis_ping_response(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock, zval *z_tab, void *ctx);
PHP_REDIS_API int redis_info_response(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock, zval *z_tab, void *ctx);
PHP_REDIS_API void redis_parse_info_response(char *response, zval *z_ret);
PHP_REDIS_API void redis_parse_client_list_response(char *response, zval *z_ret);
PHP_REDIS_API int redis_type_response(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock, zval *z_tab, void *ctx);
PHP_REDIS_API RedisSock* redis_sock_create(char *host, int host_len, int port, double timeout, double read_timeout, int persistent, char *persistent_id, long retry_interval);
PHP_REDIS_API int redis_sock_connect(RedisSock *redis_sock);
PHP_REDIS_API int redis_sock_server_open(RedisSock *redis_sock);
PHP_REDIS_API int redis_sock_auth(RedisSock *redis_sock);
PHP_REDIS_API char *redis_sock_auth_cmd(RedisSock *redis_sock, int *cmdlen);
PHP_REDIS_API void redis_sock_set_auth(RedisSock *redis_sock, zend_string *user, zend_string *pass);
PHP_REDIS_API void redis_sock_set_auth_zval(RedisSock *redis_sock, zval *zv);
PHP_REDIS_API void redis_sock_copy_auth(RedisSock *dst, RedisSock *src);
PHP_REDIS_API void redis_sock_free_auth(RedisSock *redis_sock);
PHP_REDIS_API int redis_sock_disconnect(RedisSock *redis_sock, int force);
PHP_REDIS_API zval *redis_sock_read_multibulk_reply_zval(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock, zval *z_tab);
PHP_REDIS_API int redis_sock_read_single_line(RedisSock *redis_sock, char *buffer,
    size_t buflen, size_t *linelen, int set_err);
PHP_REDIS_API char *redis_sock_read_bulk_reply(RedisSock *redis_sock, int bytes);
PHP_REDIS_API int redis_sock_read_multibulk_reply(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock, zval *_z_tab, void *ctx);
PHP_REDIS_API void redis_mbulk_reply_loop(RedisSock *redis_sock, zval *z_tab, int count, int unserialize);


PHP_REDIS_API int redis_mbulk_reply_raw(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock, zval *z_tab, void *ctx);
PHP_REDIS_API int redis_mbulk_reply_zipped_raw(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock, zval *z_tab, void *ctx);
PHP_REDIS_API int redis_mbulk_reply_zipped_vals(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock, zval *z_tab, void *ctx);
PHP_REDIS_API int redis_mbulk_reply_zipped_keys_int(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock, zval *z_tab, void *ctx);
PHP_REDIS_API int redis_mbulk_reply_zipped_keys_dbl(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock, zval *z_tab, void *ctx);
PHP_REDIS_API int redis_mbulk_reply_assoc(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock, zval *z_tab, void *ctx);
PHP_REDIS_API int redis_mbulk_reply_double(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock, zval *z_tab, void *ctx);

PHP_REDIS_API int redis_sock_read_scan_reply(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock, REDIS_SCAN_TYPE type, zend_long *iter);


PHP_REDIS_API int redis_xrange_reply(INTERNAL_FUNCTION_PARAMETERS,
    RedisSock *redis_sock, zval *z_tab, void *ctx);
PHP_REDIS_API int redis_xread_reply(INTERNAL_FUNCTION_PARAMETERS,
    RedisSock *redis_sock, zval *z_tab, void *ctx);
PHP_REDIS_API int redis_xclaim_reply(INTERNAL_FUNCTION_PARAMETERS,
    RedisSock *redis_sock, zval *z_tab, void *ctx);
PHP_REDIS_API int redis_xinfo_reply(INTERNAL_FUNCTION_PARAMETERS,
    RedisSock *redis_sock, zval *z_tab, void *ctx);

PHP_REDIS_API int redis_subscribe_response(INTERNAL_FUNCTION_PARAMETERS,
    RedisSock *redis_sock, zval *z_tab, void *ctx);
PHP_REDIS_API int redis_unsubscribe_response(INTERNAL_FUNCTION_PARAMETERS,
    RedisSock *redis_sock, zval *z_tab, void *ctx);

PHP_REDIS_API int redis_sock_write(RedisSock *redis_sock, char *cmd, size_t sz);
PHP_REDIS_API int redis_check_eof(RedisSock *redis_sock, int no_throw);
PHP_REDIS_API RedisSock *redis_sock_get(zval *id, int nothrow);
PHP_REDIS_API void redis_free_socket(RedisSock *redis_sock);
PHP_REDIS_API void redis_sock_set_err(RedisSock *redis_sock, const char *msg, int msg_len);
PHP_REDIS_API int redis_sock_set_stream_context(RedisSock *redis_sock, zval *options);

PHP_REDIS_API int
redis_serialize(RedisSock *redis_sock, zval *z, char **val, size_t *val_len);
PHP_REDIS_API int
redis_key_prefix(RedisSock *redis_sock, char **key, size_t *key_len);

PHP_REDIS_API int
redis_unserialize(RedisSock *redis_sock, const char *val, int val_len, zval *z_ret);

PHP_REDIS_API int redis_pack(RedisSock *redis_sock, zval *z, char **val, size_t *val_len);
PHP_REDIS_API int redis_unpack(RedisSock *redis_sock, const char *val, int val_len, zval *z_ret);

PHP_REDIS_API int
redis_read_stream_messages(RedisSock *redis_sock, int count, zval *z_ret);
PHP_REDIS_API int
redis_read_stream_messages_multi(RedisSock *redis_sock, int count, zval *z_ret);
PHP_REDIS_API int
redis_read_xclaim_response(RedisSock *redis_sock, int count, zval *rv);
PHP_REDIS_API int
redis_read_xinfo_response(RedisSock *redis_sock, zval *z_ret, int elements);

/* Specialized ACL reply handlers */
PHP_REDIS_API int redis_read_acl_getuser_reply(RedisSock *redis_sock, zval *zret, long len);
PHP_REDIS_API int redis_acl_getuser_reply(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock, zval *z_tab, void *ctx);
PHP_REDIS_API int redis_read_acl_log_reply(RedisSock *redis_sock, zval *zret, long count);
PHP_REDIS_API int redis_acl_log_reply(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock, zval *z_tab, void *ctx);

/*
* Variant Read methods, mostly to implement eval
*/

PHP_REDIS_API int redis_read_reply_type(RedisSock *redis_sock, REDIS_REPLY_TYPE *reply_type, long *reply_info);
PHP_REDIS_API int redis_read_variant_bulk(RedisSock *redis_sock, int size, zval *z_ret);
PHP_REDIS_API int redis_read_multibulk_recursive(RedisSock *redis_sock, long long elements, int status_strings, zval *z_ret);
PHP_REDIS_API int redis_read_variant_reply(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock, zval *z_tab, void *ctx);
PHP_REDIS_API int redis_read_raw_variant_reply(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock, zval *z_tab, void *ctx);
PHP_REDIS_API int redis_read_variant_reply_strings(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock, zval *z_tab, void *ctx);
PHP_REDIS_API int redis_client_list_reply(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock, zval *z_tab, void *ctx);

PHP_REDIS_API int redis_zdiff_response(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock, zval *z_tab, void *ctx);
PHP_REDIS_API int redis_set_response(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock, zval *z_tab, void *ctx);

/* Helper methods to get configuration values from a HashTable. */

#define REDIS_HASH_STR_FIND_STATIC(ht, sstr) \
    zend_hash_str_find(ht, sstr, sizeof(sstr) - 1)
#define REDIS_HASH_STR_FIND_TYPE_STATIC(ht, sstr, type) \
    redis_hash_str_find_type(ht, sstr, sizeof(sstr) - 1, type)

#define REDIS_CONF_DOUBLE_STATIC(ht, sstr, dval) \
    redis_conf_double(ht, sstr, sizeof(sstr) - 1, dval)
#define REDIS_CONF_BOOL_STATIC(ht, sstr, rval) \
    redis_conf_bool(ht, sstr, sizeof(sstr) - 1, rval)
#define REDIS_CONF_ZEND_BOOL_STATIC(ht, sstr, bval) \
    redis_conf_zend_bool(ht, sstr, sizeof(sstr) - 1, bval)
#define REDIS_CONF_LONG_STATIC(ht, sstr, lval) \
    redis_conf_long(ht, sstr, sizeof(sstr) - 1, lval)
#define REDIS_CONF_INT_STATIC(ht, sstr, ival) \
    redis_conf_int(ht, sstr, sizeof(sstr) - 1, ival)
#define REDIS_CONF_STRING_STATIC(ht, sstr, sval) \
    redis_conf_string(ht, sstr, sizeof(sstr) - 1, sval)
#define REDIS_CONF_ZVAL_STATIC(ht, sstr, zret, copy, dtor) \
    redis_conf_zval(ht, sstr, sizeof(sstr) - 1, zret, copy, dtor)
#define REDIS_CONF_AUTH_STATIC(ht, sstr, user, pass) \
    redis_conf_auth(ht, sstr, sizeof(sstr) - 1, user, pass)

zval *redis_hash_str_find_type(HashTable *ht, const char *key, int keylen, int type);
void redis_conf_double(HashTable *ht, const char *key, int keylen, double *dval);
void redis_conf_bool(HashTable *ht, const char *key, int keylen, int *bval);
void redis_conf_zend_bool(HashTable *ht, const char *key, int keylen, zend_bool *bval);
void redis_conf_long(HashTable *ht, const char *key, int keylen, zend_long *lval);
void redis_conf_int(HashTable *ht, const char *key, int keylen, int *ival);
void redis_conf_string(HashTable *ht, const char *key, size_t keylen, zend_string **sval);
void redis_conf_zval(HashTable *ht, const char *key, size_t keylen, zval *zret, int copy, int dtor);
void redis_conf_auth(HashTable *ht, const char *key, size_t keylen, zend_string **user, zend_string **pass);

#endif
