#include "php.h"
#include "php_ini.h"

#ifndef REDIS_COMMON_H
#define REDIS_COMMON_H

#define redis_sock_name "Redis Socket Buffer"
#define REDIS_SOCK_STATUS_FAILED 0
#define REDIS_SOCK_STATUS_DISCONNECTED 1
#define REDIS_SOCK_STATUS_UNKNOWN 2
#define REDIS_SOCK_STATUS_CONNECTED 3

#define redis_multi_access_type_name "Redis Multi type access"

#define _NL "\r\n"

/* properties */
#define REDIS_NOT_FOUND 0
#define REDIS_STRING 1
#define REDIS_SET 2
#define REDIS_LIST 3
#define REDIS_ZSET 4
#define REDIS_HASH 5

/* options */
#define REDIS_OPT_SERIALIZER		1
#define REDIS_OPT_PREFIX		2

/* serializers */
#define REDIS_SERIALIZER_NONE		0
#define REDIS_SERIALIZER_PHP 		1
#define REDIS_SERIALIZER_IGBINARY 	2

#define IF_MULTI() if(redis_sock->mode == MULTI)
#define IF_MULTI_OR_ATOMIC() if(redis_sock->mode == MULTI || redis_sock->mode == ATOMIC)\

#define IF_MULTI_OR_PIPELINE() if(redis_sock->mode == MULTI || redis_sock->mode == PIPELINE)
#define IF_PIPELINE() if(redis_sock->mode == PIPELINE)
#define IF_NOT_MULTI() if(redis_sock->mode != MULTI)
#define IF_ATOMIC() if(redis_sock->mode == ATOMIC)
#define ELSE_IF_MULTI() else if(redis_sock->mode == MULTI) { \
	if(redis_response_enqueued(redis_sock TSRMLS_CC) == 1) {\
		RETURN_ZVAL(getThis(), 1, 0);\
	} else {\
		RETURN_FALSE;\
	}				 \
}

#define ELSE_IF_PIPELINE() else IF_PIPELINE() {	\
	RETURN_ZVAL(getThis(), 1, 0);\
}


#define MULTI_RESPONSE(callback) IF_MULTI_OR_PIPELINE() { \
	fold_item *f1, *current; \
	f1 = malloc(sizeof(fold_item)); \
	f1->fun = (void *)callback; \
	f1->next = NULL; \
	current = redis_sock->current;\
	if(current) current->next = f1; \
	redis_sock->current = f1; \
  }

#define PIPELINE_ENQUEUE_COMMAND(cmd, cmd_len) request_item *tmp; \
	struct request_item *current_request;\
	tmp = malloc(sizeof(request_item));\
	tmp->request_str = calloc(cmd_len, 1);\
	memcpy(tmp->request_str, cmd, cmd_len);\
	tmp->request_size = cmd_len;\
	tmp->next = NULL;\
	current_request = redis_sock->pipeline_current; \
	if(current_request) {\
		current_request->next = tmp;\
	} \
	redis_sock->pipeline_current = tmp; \
	if(NULL == redis_sock->pipeline_head) { \
		redis_sock->pipeline_head = redis_sock->pipeline_current;\
	}

#define SOCKET_WRITE_COMMAND(redis_sock, cmd, cmd_len) if(redis_sock_write(redis_sock, cmd, cmd_len TSRMLS_CC) < 0) { \
	efree(cmd); \
    RETURN_FALSE; \
}

#define REDIS_SAVE_CALLBACK(callback, closure_context) IF_MULTI_OR_PIPELINE() { \
	fold_item *f1, *current; \
	f1 = malloc(sizeof(fold_item)); \
	f1->fun = (void *)callback; \
	f1->ctx = closure_context; \
	f1->next = NULL; \
	current = redis_sock->current;\
	if(current) current->next = f1; \
	redis_sock->current = f1; \
	if(NULL == redis_sock->head) { \
		redis_sock->head = redis_sock->current;\
	}\
}

#define REDIS_ELSE_IF_MULTI(function, closure_context) \
else if(redis_sock->mode == MULTI) { \
	if(redis_response_enqueued(redis_sock TSRMLS_CC) == 1) {\
		REDIS_SAVE_CALLBACK(function, closure_context); \
		RETURN_ZVAL(getThis(), 1, 0);\
	} else {\
		RETURN_FALSE;\
	}\
}

#define REDIS_ELSE_IF_PIPELINE(function, closure_context) else IF_PIPELINE() {	\
	REDIS_SAVE_CALLBACK(function, closure_context); \
	RETURN_ZVAL(getThis(), 1, 0);\
}

#define REDIS_PROCESS_REQUEST(redis_sock, cmd, cmd_len) 	\
	IF_MULTI_OR_ATOMIC() { \
		SOCKET_WRITE_COMMAND(redis_sock, cmd, cmd_len); \
		efree(cmd); \
	}\
	IF_PIPELINE() { \
		PIPELINE_ENQUEUE_COMMAND(cmd, cmd_len); \
		efree(cmd); \
	}

#define REDIS_PROCESS_RESPONSE_CLOSURE(function, closure_context) \
	REDIS_ELSE_IF_MULTI(function, closure_context) \
	REDIS_ELSE_IF_PIPELINE(function, closure_context);

#define REDIS_PROCESS_RESPONSE(function) REDIS_PROCESS_RESPONSE_CLOSURE(function, NULL)

typedef enum {ATOMIC, MULTI, PIPELINE} redis_mode;

typedef struct fold_item {
	zval * (*fun)(INTERNAL_FUNCTION_PARAMETERS, void *, ...);
	void *ctx;
	struct fold_item *next;
} fold_item;

typedef struct request_item {
	char *request_str; 
	int request_size; /* size_t */
	struct request_item *next;
} request_item;

/* {{{ struct RedisSock */
typedef struct {
    php_stream     *stream;
    char           *host;
    short          port;
    double         timeout;
    int            failed;
    int            status;
    int            persistent;
    int            watching;
    char           *persistent_id;

    int            serializer;

    char           *prefix;
    int            prefix_len;

    redis_mode     mode;
    fold_item      *head;
    fold_item      *current;

    request_item   *pipeline_head;
    request_item   *pipeline_current;
} RedisSock;
/* }}} */

void
free_reply_callbacks(zval *z_this, RedisSock *redis_sock);

#endif
