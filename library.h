#ifndef REDIS_LIBRARY_H
#define REDIS_LIBRARY_H

/* Non cluster command helper */
#define REDIS_SPPRINTF(ret, kw, fmt, ...) \
    redis_spprintf(redis_sock, NULL TSRMLS_CC, ret, kw, fmt, ##__VA_ARGS__)

#define REDIS_CMD_APPEND_SSTR_STATIC(sstr, str) \
    redis_cmd_append_sstr(sstr, str, sizeof(str)-1);

#define REDIS_CMD_APPEND_SSTR_OPT_STATIC(sstr, opt, str) \
    if (opt) REDIS_CMD_APPEND_SSTR_STATIC(sstr, str);

#define REDIS_CMD_INIT_SSTR_STATIC(sstr, argc, keyword) \
    redis_cmd_init_sstr(sstr, argc, keyword, sizeof(keyword)-1);

int redis_cmd_init_sstr(smart_string *str, int num_args, char *keyword, int keyword_len);
int redis_cmd_append_sstr(smart_string *str, char *append, int append_len);
int redis_cmd_append_sstr_int(smart_string *str, int append);
int redis_cmd_append_sstr_long(smart_string *str, long append);
int redis_cmd_append_sstr_dbl(smart_string *str, double value);
int redis_cmd_append_sstr_zval(smart_string *str, zval *z, RedisSock *redis_sock TSRMLS_DC);
int redis_cmd_append_sstr_key(smart_string *str, char *key, strlen_t len, RedisSock *redis_sock, short *slot);

PHP_REDIS_API int redis_spprintf(RedisSock *redis_sock, short *slot TSRMLS_DC, char **ret, char *kw, char *fmt, ...);

PHP_REDIS_API char * redis_sock_read(RedisSock *redis_sock, int *buf_len TSRMLS_DC);
PHP_REDIS_API int redis_sock_gets(RedisSock *redis_sock, char *buf, int buf_size, size_t* line_len TSRMLS_DC);
PHP_REDIS_API void redis_1_response(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock, zval *z_tab, void *ctx);
PHP_REDIS_API void redis_long_response(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock, zval* z_tab, void *ctx);
typedef void (*SuccessCallback)(RedisSock *redis_sock);
PHP_REDIS_API void redis_boolean_response_impl(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock, zval *z_tab, void *ctx, SuccessCallback success_callback);
PHP_REDIS_API void redis_boolean_response(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock, zval *z_tab, void *ctx);
PHP_REDIS_API void redis_bulk_double_response(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock, zval *z_tab, void *ctx);
PHP_REDIS_API void redis_string_response(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock, zval *z_tab, void *ctx);
PHP_REDIS_API void redis_ping_response(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock, zval *z_tab, void *ctx);
PHP_REDIS_API void redis_info_response(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock, zval *z_tab, void *ctx);
PHP_REDIS_API void redis_parse_info_response(char *response, zval *z_ret);
PHP_REDIS_API void redis_parse_client_list_response(char *response, zval *z_ret);
PHP_REDIS_API void redis_type_response(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock, zval *z_tab, void *ctx);
PHP_REDIS_API RedisSock* redis_sock_create(char *host, int host_len, unsigned short port, double timeout, double read_timeout, int persistent, char *persistent_id, long retry_interval, zend_bool lazy_connect);
PHP_REDIS_API int redis_sock_connect(RedisSock *redis_sock TSRMLS_DC);
PHP_REDIS_API int redis_sock_server_open(RedisSock *redis_sock TSRMLS_DC);
PHP_REDIS_API int redis_sock_disconnect(RedisSock *redis_sock TSRMLS_DC);
PHP_REDIS_API zval *redis_sock_read_multibulk_reply_zval(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock, zval *z_tab);
PHP_REDIS_API char *redis_sock_read_bulk_reply(RedisSock *redis_sock, int bytes TSRMLS_DC);
PHP_REDIS_API int redis_sock_read_multibulk_reply(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock, zval *_z_tab, void *ctx);
PHP_REDIS_API void redis_mbulk_reply_loop(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock, zval *z_tab, int count, int unserialize);

PHP_REDIS_API int redis_mbulk_reply_raw(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock, zval *z_tab, void *ctx);
PHP_REDIS_API int redis_mbulk_reply_zipped_raw(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock, zval *z_tab, void *ctx);
PHP_REDIS_API int redis_mbulk_reply_zipped_vals(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock, zval *z_tab, void *ctx);
PHP_REDIS_API int redis_mbulk_reply_zipped_keys_int(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock, zval *z_tab, void *ctx);
PHP_REDIS_API int redis_mbulk_reply_zipped_keys_dbl(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock, zval *z_tab, void *ctx);
PHP_REDIS_API int redis_mbulk_reply_assoc(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock, zval *z_tab, void *ctx);

PHP_REDIS_API int redis_sock_read_scan_reply(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock, REDIS_SCAN_TYPE type, zend_long *iter);

PHP_REDIS_API int redis_subscribe_response(INTERNAL_FUNCTION_PARAMETERS, 
    RedisSock *redis_sock, zval *z_tab, void *ctx);
PHP_REDIS_API int redis_unsubscribe_response(INTERNAL_FUNCTION_PARAMETERS,
    RedisSock *redis_sock, zval *z_tab, void *ctx);

PHP_REDIS_API int redis_sock_write(RedisSock *redis_sock, char *cmd, size_t sz TSRMLS_DC);
PHP_REDIS_API void redis_stream_close(RedisSock *redis_sock TSRMLS_DC);
PHP_REDIS_API int redis_check_eof(RedisSock *redis_sock, int no_throw TSRMLS_DC);
PHP_REDIS_API RedisSock *redis_sock_get(zval *id TSRMLS_DC, int nothrow);
PHP_REDIS_API void redis_free_socket(RedisSock *redis_sock);
PHP_REDIS_API void redis_send_discard(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock);
PHP_REDIS_API void redis_sock_set_err(RedisSock *redis_sock, const char *msg, int msg_len);

PHP_REDIS_API int
redis_serialize(RedisSock *redis_sock, zval *z, char **val, strlen_t *val_len TSRMLS_DC);
PHP_REDIS_API int
redis_key_prefix(RedisSock *redis_sock, char **key, strlen_t *key_len);

PHP_REDIS_API int
redis_unserialize(RedisSock *redis_sock, const char *val, int val_len, zval *z_ret TSRMLS_DC);

/*
* Variant Read methods, mostly to implement eval
*/

PHP_REDIS_API int redis_read_reply_type(RedisSock *redis_sock, REDIS_REPLY_TYPE *reply_type, long *reply_info TSRMLS_DC);
PHP_REDIS_API int redis_read_variant_line(RedisSock *redis_sock, REDIS_REPLY_TYPE reply_type, zval *z_ret TSRMLS_DC);
PHP_REDIS_API int redis_read_variant_bulk(RedisSock *redis_sock, int size, zval *z_ret TSRMLS_DC);
PHP_REDIS_API int redis_read_multibulk_recursive(RedisSock *redis_sock, int elements, zval *z_ret TSRMLS_DC);
PHP_REDIS_API int redis_read_variant_reply(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock, zval *z_tab, void *ctx);

PHP_REDIS_API void redis_client_list_reply(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock, zval *z_tab);

#endif
