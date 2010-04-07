#include "php.h"
#include "php_ini.h"

#define redis_sock_name "Redis Socket Buffer"
#define REDIS_SOCK_STATUS_FAILED 0
#define REDIS_SOCK_STATUS_DISCONNECTED 1
#define REDIS_SOCK_STATUS_UNKNOWN 2
#define REDIS_SOCK_STATUS_CONNECTED 3

#define redis_multi_access_type_name "Redis Multi type access"

/* properties */
#define REDIS_NOT_FOUND 0
#define REDIS_STRING 1
#define REDIS_SET 2
#define REDIS_LIST 3

#define REDIS_ATOMIC 0
#define REDIS_MULTI 1
#define REDIS_PIPELINE 2

#define IF_MULTI() if(get_flag(object) == REDIS_MULTI)
#define IF_MULTI_OR_ATOMIC() if(get_flag(object) == REDIS_MULTI || get_flag(object) == REDIS_ATOMIC)
#define IF_MULTI_OR_PIPELINE() if(get_flag(object) == REDIS_MULTI || get_flag(object) == REDIS_PIPELINE)
#define IF_PIPELINE() if(get_flag(object) == REDIS_PIPELINE)
#define IF_NOT_MULTI() if(get_flag(object) != REDIS_MULTI)
#define IF_ATOMIC() if(get_flag(object) == REDIS_ATOMIC)
#define ELSE_IF_MULTI() else if(get_flag(object) == REDIS_MULTI) {\
	if(redis_response_enqueued(redis_sock TSRMLS_CC) == 1) {\
		RETURN_ZVAL(getThis(), 1, 0);\
	} else {\
		RETURN_FALSE;\
	}\
}\

#define ELSE_IF_PIPELINE() else IF_PIPELINE() {	\
	RETURN_ZVAL(getThis(), 1, 0);\
}\

#define MULTI_RESPONSE(string, callback) IF_MULTI_OR_PIPELINE() { \
	fold_item *f1 = malloc(sizeof(fold_item)); \
	f1->function_name = strdup(string); \
	f1->fun = (void *)callback; \
	f1->next = NULL; \
	current->next = f1; \
	current = f1; \
}\

#define PIPELINE_ENQUEUE_COMMAND(string) request_item *tmp;\		
	tmp = malloc(sizeof(request_item));\
	tmp->function_name = strdup(string);\
	tmp->request_str = strdup(cmd);\
	tmp->request_size = cmd_len;\
	tmp->next = NULL;\
	current_request->next = tmp;\
	current_request = tmp;\

#define SOCKET_WRITE_COMMAND() if(redis_sock_write(redis_sock, cmd, cmd_len) < 0) { \
	efree(cmd); \
    RETURN_FALSE; \
}\

/* {{{ struct RedisSock */
typedef struct RedisSock_ {
    php_stream     *stream;
    char           *host;
    unsigned short port;
    long           timeout;
    int            failed;
    int            status;
} RedisSock;
/* }}} */

typedef struct fold_item {
	char *function_name;
	zval * (*fun)(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock, ...);
	struct fold_item *next;
} fold_item;
fold_item *head, *current;

typedef struct request_item {
	char *function_name;
	char *request_str; 
	int request_size; /* size_t */
	struct request_item *next;
} request_item;
request_item *head_request, *current_request;
