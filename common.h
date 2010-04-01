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
#define IF_PIPELINE() if(get_flag(object) == REDIS_PIPELINE)
#define IF_NOT_MULTI() if(get_flag(object) != REDIS_MULTI)
#define ELSE_IF_MULTI() else {\
	if(redis_response_enqueued(redis_sock TSRMLS_CC) == 1) {\
		RETURN_ZVAL(getThis(), 1, 0);\
	} else {\
		RETURN_FALSE;\
	}\
}\

#define MULTI_RESPONSE(string, callback) 	IF_MULTI() { \
	fold_item *f1 = malloc(sizeof(fold_item)); \
	f1->function_name = strdup(string); \
	f1->fun = (void *)callback; \
	f1->next = NULL; \
	current->next = f1; \
	current = f1; \
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


