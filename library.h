#ifndef REDIS_LIBRARY_H
#define REDIS_LIBRARY_H

#define REDIS_CMD_APPEND_SSTR_STATIC(sstr, str) \
    redis_cmd_append_sstr(sstr, str, sizeof(str)-1);

#define REDIS_CMD_INIT_SSTR_STATIC(sstr, argc, keyword) \
    redis_cmd_init_sstr(sstr, argc, keyword, sizeof(keyword)-1);

int integer_length(int i);
int redis_cmd_format(char **ret, char *format, ...);
int redis_cmd_format_static(char **ret, char *keyword, char *format, ...);
int redis_cmd_format_header(char **ret, char *keyword, int arg_count);
int redis_cmd_append_str(char **cmd, int cmd_len, char *append, int append_len);
int redis_cmd_init_sstr(smart_string *str, int num_args, char *keyword, int keyword_len);
int redis_cmd_append_sstr(smart_string *str, char *append, int append_len);
int redis_cmd_append_sstr_int(smart_string *str, int append);
int redis_cmd_append_sstr_long(smart_string *str, long append);
int redis_cmd_append_int(char **cmd, int cmd_len, int append);
int redis_cmd_append_sstr_dbl(smart_string *str, double value);

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
PHP_REDIS_API RedisSock* redis_sock_create(char *host, int host_len, unsigned short port, double timeout, int persistent, char *persistent_id, long retry_interval, zend_bool lazy_connect);
PHP_REDIS_API int redis_sock_connect(RedisSock *redis_sock TSRMLS_DC);
PHP_REDIS_API int redis_sock_server_open(RedisSock *redis_sock, int force_connect TSRMLS_DC);
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
PHP_REDIS_API int redis_sock_get(zval *id, RedisSock **redis_sock TSRMLS_DC, int nothrow);
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

#if (PHP_MAJOR_VERSION < 7)
#if ZEND_MODULE_API_NO >= 20100000
#define REDIS_DOUBLE_TO_STRING(dbl_str, dbl) do { \
    char dbl_decsep = '.'; \
    dbl_str = emalloc(sizeof(zend_string)); \
    dbl_str->val = _php_math_number_format_ex(dbl, 16, &dbl_decsep, 1, NULL, 0); \
    dbl_str->len = strlen(dbl_str->val); \
    dbl_str->gc = 0x11; \
} while (0);
#else
#define REDIS_DOUBLE_TO_STRING(dbl_str, dbl) do { \
    dbl_str = emalloc(sizeof(zend_string)); \
    dbl_str->val = _php_math_number_format(dbl, 16, '.', '\x00'); \
    dbl_str->len = strlen(dbl_str->val); \
    dbl_str->gc = 0x11; \
} while (0)
#endif
#else
#define REDIS_DOUBLE_TO_STRING(dbl_str, dbl) do { \
    char dbl_decsep = '.'; \
    dbl_str = _php_math_number_format_ex(dbl, 16, &dbl_decsep, 1, NULL, 0); \
} while (0);
#endif

#endif
