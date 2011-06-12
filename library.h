void add_constant_long(zend_class_entry *ce, char *name, int value);
int integer_length(int i);
int redis_cmd_format(char **ret, char *format, ...);
int redis_cmd_format_static(char **ret, char *keyword, char *format, ...);

PHPAPI char * redis_sock_read(RedisSock *redis_sock, int *buf_len TSRMLS_DC);

PHPAPI void redis_1_response(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock, zval *z_tab, void *ctx);
PHPAPI void redis_long_response(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock, zval* z_tab, void *ctx);
PHPAPI void redis_boolean_response(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock, zval *z_tab, void *ctx);
PHPAPI void redis_bulk_double_response(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock, zval *z_tab, void *ctx);
PHPAPI void redis_string_response(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock, zval *z_tab, void *ctx);
PHPAPI void redis_ping_response(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock, zval *z_tab, void *ctx);
PHPAPI void redis_info_response(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock, zval *z_tab, void *ctx);
PHPAPI void redis_type_response(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock, zval *z_tab, void *ctx);
PHPAPI RedisSock* redis_sock_create(char *host, int host_len, unsigned short port, double timeout, int persistent, char *persistent_id);
PHPAPI int redis_sock_connect(RedisSock *redis_sock TSRMLS_DC);
PHPAPI int redis_sock_server_open(RedisSock *redis_sock, int force_connect TSRMLS_DC);
PHPAPI int redis_sock_disconnect(RedisSock *redis_sock TSRMLS_DC);
PHPAPI zval *redis_sock_read_multibulk_reply_zval(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock);
PHPAPI char *redis_sock_read_bulk_reply(RedisSock *redis_sock, int bytes TSRMLS_DC);
PHPAPI int redis_sock_read_multibulk_reply(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock, zval *_z_tab, void *ctx);
PHPAPI int redis_sock_read_multibulk_reply_raw(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock, zval *z_tab, void *ctx);
PHPAPI int redis_sock_read_multibulk_reply_loop(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock, zval *z_tab, int numElems, int unwrap_key);
PHPAPI int redis_sock_read_multibulk_reply_zipped(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock, zval *z_tab, void *ctx);
PHPAPI int redis_sock_read_multibulk_reply_zipped_strings(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock, zval *z_tab, void *ctx);
PHPAPI int redis_sock_read_multibulk_reply_assoc(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock, zval *z_tab, void *ctx);
PHPAPI int redis_sock_write(RedisSock *redis_sock, char *cmd, size_t sz TSRMLS_DC);
PHPAPI void redis_stream_close(RedisSock *redis_sock TSRMLS_DC);
PHPAPI int redis_check_eof(RedisSock *redis_sock TSRMLS_DC);
//PHPAPI int redis_sock_get(zval *id, RedisSock **redis_sock TSRMLS_DC);
PHPAPI void redis_free_socket(RedisSock *redis_sock);

PHPAPI int
redis_serialize(RedisSock *redis_sock, zval *z, char **val, int *val_len TSRMLS_DC);
PHPAPI int
redis_key_prefix(RedisSock *redis_sock, char **key, int *key_len TSRMLS_DC);

PHPAPI int
redis_unserialize(RedisSock *redis_sock, const char *val, int val_len, zval **return_value TSRMLS_DC);
