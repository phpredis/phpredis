#ifndef REDIS_LIBRARY_H
#define REDIS_LIBRARY_H

void add_constant_long(zend_class_entry *ce, char *name, int value);
size_t integer_length(int i);
size_t redis_cmd_format(char **ret, char *format, ...);
size_t redis_cmd_format_static(char **ret, char *keyword, char *format, ...);
size_t redis_cmd_format_header(char **ret, char *keyword, int arg_count);
size_t redis_cmd_append_str(char **cmd, size_t cmd_len, char *append, size_t append_len);
size_t redis_cmd_init_sstr(smart_string *str, size_t num_args, char *keyword, size_t keyword_len);
size_t redis_cmd_append_sstr(smart_string *str, char *append, size_t append_len);
size_t redis_cmd_append_sstr_int(smart_string *str, int append);
size_t redis_cmd_append_sstr_long(smart_string *str, zend_long append);
size_t redis_cmd_append_int(char **cmd, size_t cmd_len, int append);
size_t redis_cmd_append_sstr_dbl(smart_string *str, double value);

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
PHP_REDIS_API void redis_parse_info_response(char *resp, zval *z_ret);
PHP_REDIS_API void redis_parse_client_list_response(char *resp, zval *z_result);
PHP_REDIS_API void redis_type_response(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock, zval *z_tab, void *ctx);
PHP_REDIS_API RedisSock* redis_sock_create(char *host, size_t host_len, unsigned short port, double timeout, int persistent, char *persistent_id, zend_long retry_interval, zend_bool lazy_connect);
PHP_REDIS_API int redis_sock_connect(RedisSock *redis_sock TSRMLS_DC);
PHP_REDIS_API int redis_sock_server_open(RedisSock *redis_sock, int force_connect TSRMLS_DC);
PHP_REDIS_API int redis_sock_disconnect(RedisSock *redis_sock TSRMLS_DC);
PHP_REDIS_API void redis_sock_read_multibulk_reply_zval(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock, zval *z_tab);
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
PHP_REDIS_API size_t redis_build_scan_cmd(char **cmd, REDIS_SCAN_TYPE type, char *key, size_t key_len, int iter, char *pattern, size_t pattern_len, int count);
PHP_REDIS_API size_t redis_build_script_exists_cmd(char **ret, zval *argv, int argc);
PHP_REDIS_API size_t redis_build_eval_cmd(RedisSock *redis_sock, char **ret, char *keyword, char *value, size_t val_len, zval *args, int keys_count TSRMLS_DC);

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
PHP_REDIS_API int redis_sock_set_err(RedisSock *redis_sock, const char *msg, int msg_len);

PHP_REDIS_API int
redis_serialize(RedisSock *redis_sock, zval *z, char **val, size_t *val_len TSRMLS_DC);
PHP_REDIS_API int
redis_key_prefix(RedisSock *redis_sock, char **key, size_t *key_len);

PHP_REDIS_API int
redis_unserialize(RedisSock *redis_sock, const char *val, size_t val_len, zval *return_value TSRMLS_DC);

PHP_REDIS_API void redis_free_socket(RedisSock *redis_sock);
PHP_REDIS_API void redis_send_discard(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock);
PHP_REDIS_API int redis_sock_set_err(RedisSock *redis_sock, const char *msg, int msg_len);


/*
* Variant Read methods, mostly to implement eval
*/

PHP_REDIS_API int redis_read_reply_type(RedisSock *redis_sock, REDIS_REPLY_TYPE *reply_type, long *reply_info TSRMLS_DC);
PHP_REDIS_API int redis_read_variant_line(RedisSock *redis_sock, REDIS_REPLY_TYPE reply_type, zval **z_ret TSRMLS_DC);
PHP_REDIS_API int redis_read_variant_bulk(RedisSock *redis_sock, int size, zval **z_ret TSRMLS_DC);
PHP_REDIS_API int redis_read_multibulk_recursive(RedisSock *redis_sock, int elements, zval **z_ret TSRMLS_DC);
PHP_REDIS_API int redis_read_variant_reply(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock, zval *z_tab, void *ctx);

PHP_REDIS_API void redis_client_list_reply(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock, zval *z_tab);

#define REDIS_DOUBLE_TO_STRING(dbl_str, dbl) do { \
	char dbl_decsep = '.'; \
	dbl_str = _php_math_number_format_ex(dbl, 16, &dbl_decsep, 1, NULL, 0); \
	} while (0);

#endif
