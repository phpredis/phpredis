void add_constant_long(zend_class_entry *ce, char *name, int value);
int integer_length(int i);
int redis_cmd_format(char **ret, char *format, ...);
int redis_cmd_format_static(char **ret, char *keyword, char *format, ...);
int redis_cmd_format_header(char **ret, char *keyword, int arg_count);
int redis_cmd_append_str(char **cmd, int cmd_len, char *append, int append_len);
int redis_cmd_init_sstr(smart_str *str, int num_args, char *keyword, int keyword_len);
int redis_cmd_append_sstr(smart_str *str, char *append, int append_len);
int redis_cmd_append_sstr_int(smart_str *str, int append);
int redis_cmd_append_sstr_long(smart_str *str, long append);
int redis_cmd_append_int(char **cmd, int cmd_len, int append);
int redis_cmd_append_sstr_dbl(smart_str *str, double value);

PHPAPI char * redis_sock_read(RedisSock *redis_sock, int *buf_len TSRMLS_DC);

PHPAPI void redis_1_response(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock, zval *z_tab, void *ctx);
PHPAPI void redis_long_response(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock, zval* z_tab, void *ctx);
typedef void (*SuccessCallback)(RedisSock *redis_sock);
PHPAPI void redis_boolean_response_impl(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock, zval *z_tab, void *ctx, SuccessCallback success_callback);
PHPAPI void redis_boolean_response(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock, zval *z_tab, void *ctx);
PHPAPI void redis_bulk_double_response(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock, zval *z_tab, void *ctx);
PHPAPI void redis_string_response(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock, zval *z_tab, void *ctx);
PHPAPI void redis_ping_response(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock, zval *z_tab, void *ctx);
PHPAPI void redis_info_response(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock, zval *z_tab, void *ctx);
PHPAPI void redis_type_response(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock, zval *z_tab, void *ctx);
PHPAPI RedisSock* redis_sock_create(char *host, int host_len, unsigned short port, double timeout, int persistent, char *persistent_id, long retry_interval, zend_bool lazy_connect);
PHPAPI int redis_sock_connect(RedisSock *redis_sock TSRMLS_DC);
PHPAPI int redis_sock_server_open(RedisSock *redis_sock, int force_connect TSRMLS_DC);
PHPAPI int redis_sock_disconnect(RedisSock *redis_sock TSRMLS_DC);
PHPAPI zval *redis_sock_read_multibulk_reply_zval(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock);
PHPAPI char *redis_sock_read_bulk_reply(RedisSock *redis_sock, int bytes TSRMLS_DC);
PHPAPI int redis_sock_read_multibulk_reply(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock, zval *_z_tab, void *ctx);
PHPAPI int redis_sock_read_multibulk_reply_raw(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock, zval *z_tab, void *ctx);
PHPAPI int redis_sock_read_multibulk_reply_loop(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock, zval *z_tab, int numElems, int unwrap_key, int unserialize_even_only);
PHPAPI int redis_sock_read_multibulk_reply_zipped(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock, zval *z_tab, void *ctx);
PHPAPI int redis_sock_read_multibulk_reply_zipped_strings(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock, zval *z_tab, void *ctx);
PHPAPI int redis_sock_read_multibulk_reply_assoc(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock, zval *z_tab, void *ctx);
PHPAPI int redis_sock_write(RedisSock *redis_sock, char *cmd, size_t sz TSRMLS_DC);
PHPAPI void redis_stream_close(RedisSock *redis_sock TSRMLS_DC);
PHPAPI int redis_check_eof(RedisSock *redis_sock TSRMLS_DC);
//PHPAPI int redis_sock_get(zval *id, RedisSock **redis_sock TSRMLS_DC);
PHPAPI void redis_free_socket(RedisSock *redis_sock);
PHPAPI void redis_send_discard(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock);
PHPAPI int redis_sock_set_err(RedisSock *redis_sock, const char *msg, int msg_len);

PHPAPI int
redis_serialize(RedisSock *redis_sock, zval *z, char **val, int *val_len TSRMLS_DC);
PHPAPI int
redis_key_prefix(RedisSock *redis_sock, char **key, int *key_len TSRMLS_DC);

PHPAPI int
redis_unserialize(RedisSock *redis_sock, const char *val, int val_len, zval **return_value TSRMLS_DC);


/*
* Variant Read methods, mostly to implement eval
*/

PHPAPI int redis_read_reply_type(RedisSock *redis_sock, REDIS_REPLY_TYPE *reply_type, int *reply_info TSRMLS_DC);
PHPAPI int redis_read_variant_line(RedisSock *redis_sock, REDIS_REPLY_TYPE reply_type, zval **z_ret TSRMLS_DC);
PHPAPI int redis_read_variant_bulk(RedisSock *redis_sock, int size, zval **z_ret TSRMLS_DC);
PHPAPI int redis_read_multibulk_recursive(RedisSock *redis_sock, int elements, zval **z_ret TSRMLS_DC);
PHPAPI int redis_read_variant_reply(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock, zval *z_tab);

PHPAPI void redis_client_list_reply(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock, zval *z_tab);

#if ZEND_MODULE_API_NO >= 20100000
#define REDIS_DOUBLE_TO_STRING(dbl_str, dbl_len, dbl) \
	char dbl_decsep; \
	dbl_decsep = '.'; \
    dbl_str = _php_math_number_format_ex(dbl, 16, &dbl_decsep, 1, NULL, 0); \
	dbl_len = strlen(dbl_str);
#else
#define REDIS_DOUBLE_TO_STRING(dbl_str, dbl_len, dbl) \
	dbl_str = _php_math_number_format(dbl, 16, '.', '\x00'); \
	dbl_len = strlen(dbl_str);
#endif
