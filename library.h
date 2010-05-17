void add_constant_long(zend_class_entry *ce, char *name, int value);
int integer_length(int i);
int double_length(double d);
int redis_cmd_format(char **ret, char *format, ...);

PHPAPI char * redis_sock_read(RedisSock *redis_sock, int *buf_len TSRMLS_DC);

PHPAPI void redis_1_response(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock, zval *z_tab TSRMLS_DC);
PHPAPI void redis_long_response(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock, zval* z_tab TSRMLS_DC);
PHPAPI void redis_boolean_response(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock, zval *z_tab TSRMLS_DC);
PHPAPI void redis_bulk_double_response(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock, zval *z_tab TSRMLS_DC);
PHPAPI void redis_string_response(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock, zval *z_tab TSRMLS_DC);
PHPAPI RedisSock* redis_sock_create(char *host, int host_len, unsigned short port, long timeout);
PHPAPI int redis_sock_connect(RedisSock *redis_sock TSRMLS_DC);
PHPAPI int redis_sock_server_open(RedisSock *redis_sock, int force_connect TSRMLS_DC);
PHPAPI int redis_sock_disconnect(RedisSock *redis_sock TSRMLS_DC);
PHPAPI char *redis_sock_read_bulk_reply(RedisSock *redis_sock, int bytes);
PHPAPI int redis_sock_read_multibulk_reply(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock, zval *_z_tab TSRMLS_DC);
PHPAPI int redis_sock_read_multibulk_reply_loop(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock, zval *z_tab, int numElems TSRMLS_DC);
PHPAPI int redis_sock_read_multibulk_reply_zipped(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock, zval *z_tab TSRMLS_DC);
PHPAPI int redis_sock_read_multibulk_reply_zipped_strings(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock, zval *z_tab TSRMLS_DC);
PHPAPI int redis_sock_write(RedisSock *redis_sock, char *cmd, size_t sz);
PHPAPI void redis_check_eof(RedisSock *redis_sock TSRMLS_DC);
//PHPAPI int redis_sock_get(zval *id, RedisSock **redis_sock TSRMLS_DC);
PHPAPI void redis_free_socket(RedisSock *redis_sock);
