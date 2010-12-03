#include "common.h"
#include "php_network.h"
#include <sys/types.h>
#include <netinet/tcp.h>  /* TCP_NODELAY */
#include <sys/socket.h>

PHPAPI int redis_check_eof(RedisSock *redis_sock TSRMLS_DC)
{

    int eof = php_stream_eof(redis_sock->stream);
    int count = 0;
    while(eof) {
	if(count++ == 10) { /* too many failures */
	    if(redis_sock->stream) { /* close stream if still here */
                php_stream_close(redis_sock->stream);
                redis_sock->stream = NULL;
	    }
            zend_throw_exception(redis_exception_ce, "Connection lost", 0 TSRMLS_CC);
	    return -1;
	}
	if(redis_sock->stream) { /* close existing stream before reconnecting */
            php_stream_close(redis_sock->stream);
            redis_sock->stream = NULL;
	}
        redis_sock_connect(redis_sock TSRMLS_CC); /* reconnect */
        if(redis_sock->stream) { /*  check for EOF again. */
            eof = php_stream_eof(redis_sock->stream);
        }
    }
    return 0;
}

PHPAPI zval *redis_sock_read_multibulk_reply_zval(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock) {
    char inbuf[1024], *response;
    int response_len;

    if(-1 == redis_check_eof(redis_sock TSRMLS_CC)) {
        return NULL;
    }

    php_stream_gets(redis_sock->stream, inbuf, 1024);

    if(inbuf[0] != '*') {
        return NULL;
    }
    int numElems = atoi(inbuf+1);

    zval *z_tab;
    MAKE_STD_ZVAL(z_tab);
    array_init(z_tab);

    redis_sock_read_multibulk_reply_loop(INTERNAL_FUNCTION_PARAM_PASSTHRU,
                    redis_sock, z_tab, numElems);
	return z_tab;
}

/**
 * redis_sock_read_bulk_reply
 */
PHPAPI char *redis_sock_read_bulk_reply(RedisSock *redis_sock, int bytes TSRMLS_DC)
{
    int offset = 0;
    size_t got;

    char * reply;

    if(-1 == redis_check_eof(redis_sock TSRMLS_CC)) {
        return NULL;
    }

    if (bytes == -1) {
        return NULL;
    } else {
        reply = emalloc(bytes+1);

        while(offset < bytes) {
            got = php_stream_read(redis_sock->stream, reply + offset, bytes-offset);
            offset += got;
        }
        char c;
        int i;
        for(i = 0; i < 2; i++) {
            php_stream_read(redis_sock->stream, &c, 1);
        }
    }

    reply[bytes] = 0;
    return reply;
}

/**
 * redis_sock_read
 */
PHPAPI char *redis_sock_read(RedisSock *redis_sock, int *buf_len TSRMLS_DC)
{

    char inbuf[1024];
    char *resp = NULL;

    if(-1 == redis_check_eof(redis_sock TSRMLS_CC)) {
        return NULL;
    }
    php_stream_gets(redis_sock->stream, inbuf, 1024);

    switch(inbuf[0]) {

        case '-':
            return NULL;

        case '$':
            *buf_len = atoi(inbuf + 1);
            resp = redis_sock_read_bulk_reply(redis_sock, *buf_len TSRMLS_CC);
            return resp;

        case '+':
        case ':':
	    // Single Line Reply
            /* :123\r\n */
            *buf_len = strlen(inbuf) - 2;
            if(*buf_len >= 2) {
                resp = emalloc(1+*buf_len);
                memcpy(resp, inbuf, *buf_len);
                resp[*buf_len] = 0;
                return resp;
            }

        default:
			zend_throw_exception_ex(
				redis_exception_ce,
				0 TSRMLS_CC,
				"protocol error, got '%c' as reply type byte\n",
				inbuf[0]
			);
    }

    return NULL;
}

void add_constant_long(zend_class_entry *ce, char *name, int value) {

    zval *constval;
    constval = pemalloc(sizeof(zval), 1);
    INIT_PZVAL(constval);
    ZVAL_LONG(constval, value);
    zend_hash_add(&ce->constants_table, name, 1 + strlen(name),
        (void*)&constval, sizeof(zval*), NULL);
}

int
integer_length(int i) {
    int sz = 0;
    int ci = abs(i);
    while (ci>0) {
            ci = (ci/10);
            sz += 1;
    }
    if(i == 0) { /* log 0 doesn't make sense. */
            sz = 1;
    } else if(i < 0) { /* allow for neg sign as well. */
            sz++;
    }
    return sz;
}

int
double_length(double d, int *has_F) {
        char *s;
        int ret = spprintf(&s, 0, "%F", d);
	*has_F = 1;
	if(ret == 2 && strncmp(s, "%F", 2) == 0) { /* we don't have the 'F' format, get back to 'f' */
		efree(s);
		ret = spprintf(&s, 0, "%f", d);
		*has_F = 0;
	}
        efree(s);
        return ret;
}


int
redis_cmd_format_static(char **ret, char *keyword, char *format, ...) {

    char *p, *s;
    va_list ap;

    int total = 0, sz, ret_sz;
    int i, ci;
    unsigned int u;
    double dbl;
    char *double_str;
    int double_len;

    int stage; /* 0: count & alloc. 1: copy. */
    int elements = strlen(format);
    int keyword_len = strlen(keyword);
    int header_sz = 1 + integer_length(1 + elements) + 2	/* star + elements + CRLF */
            + 1 + integer_length(keyword_len) + 2		/* dollar + command length + CRLF */
            + keyword_len + 2;					/* command + CRLF */

    for(stage = 0; stage < 2; ++stage) {
        va_start(ap, format);
	if(stage == 0) {
	    total = 0;
	} else {
	    total = header_sz;
	}
        for(p = format; *p; ) {
            switch(*p) {
                case 's':
                    s = va_arg(ap, char*);
                    sz = va_arg(ap, int);
                    if(stage == 1) {
                        memcpy((*ret) + total, "$", 1);		/* dollar */
			total++;

			sprintf((*ret) + total, "%d", sz);	/* size */
			total += integer_length(sz);

			memcpy((*ret) + total, _NL, 2);		/* CRLF */
			total += 2;

                        memcpy((*ret) + total, s, sz);		/* string */
			total += sz;

			memcpy((*ret) + total, _NL, 2);		/* CRLF */
			total += 2;
                    } else {
                        total += 1 + integer_length(sz) + 2 + sz + 2;
		    }
                    break;

                case 'F':
                case 'f':
                    /* use spprintf here */
                    dbl = va_arg(ap, double);
		    int has_F;
                    sz = double_length(dbl, &has_F);
		    if(stage == 1) {
		        memcpy((*ret) + total, "$", 1); 	/* dollar */
			total++;

			sprintf((*ret) + total, "%d", sz);	/* size */
			total += integer_length(sz);

			memcpy((*ret) + total, _NL, 2);		/* CRLF */
			total += 2;

			if(has_F) {
				sprintf((*ret) + total, "%F", dbl);	/* float */
			} else {
				sprintf((*ret) + total, "%f", dbl);	/* float */
			}
			total += sz;

			memcpy((*ret) + total, _NL, 2);		/* CRLF */
			total += 2;
		    } else {
                        total += 1 + integer_length(sz) + 2 + sz + 2;
		    }
                    break;

                case 'i':
                case 'd':
                    i = va_arg(ap, int);
                    /* compute display size of integer value */
                    sz = integer_length(i);
		    if(stage == 1) {
		        memcpy((*ret) + total, "$", 1); 	/* dollar */
			total++;

			sprintf((*ret) + total, "%d", sz);	/* size */
			total += integer_length(sz);

			memcpy((*ret) + total, _NL, 2);		/* CRLF */
			total += 2;

			sprintf((*ret) + total, "%d", i);	/* int */
			total += sz;

			memcpy((*ret) + total, _NL, 2);		/* CRLF */
			total += 2;
		    } else {
                        total += 1 + integer_length(sz) + 2 + sz + 2;
		    }
                    break;
            }
            p++;
        }
        if(stage == 0) {
            ret_sz = total + header_sz;
            (*ret) = emalloc(ret_sz+1);
	    sprintf(*ret, "*%d" _NL "$%d" _NL "%s" _NL, elements + 1, keyword_len, keyword);
        } else {
            (*ret)[ret_sz] = 0;
	//    printf("cmd(%d)=[%s]\n", ret_sz, *ret);
            return ret_sz;
        }
    }

}

/**
 * This command behave somehow like printf, except that strings need 2 arguments:
 * Their data and their size (strlen).
 * Supported formats are: %d, %i, %s
 */
//static  /!\ problem with static commands !!
int
redis_cmd_format(char **ret, char *format, ...) {

    char *p, *s;
    va_list ap;

    int total = 0, sz, ret_sz;
    int i, ci;
    unsigned int u;
    double dbl;
    char *double_str;
    int double_len;

    int stage; /* 0: count & alloc. 1: copy. */

    for(stage = 0; stage < 2; ++stage) {
        va_start(ap, format);
        total = 0;
        for(p = format; *p; ) {

            if(*p == '%') {
                switch(*(p+1)) {
                    case 's':
                        s = va_arg(ap, char*);
                        sz = va_arg(ap, int);
                        if(stage == 1) {
                            memcpy((*ret) + total, s, sz);
                        }
                        total += sz;
                        break;

                    case 'F':
                    case 'f':
                        /* use spprintf here */
                        dbl = va_arg(ap, double);
                        double_len = spprintf(&double_str, 0, "%F", dbl);
                        if(stage == 1) {
                            memcpy((*ret) + total, double_str, double_len);
                        }
                        total += double_len;
                        efree(double_str);
                        break;

                    case 'i':
                    case 'd':
                        i = va_arg(ap, int);
                        /* compute display size of integer value */
                        sz = 0;
                        ci = abs(i);
                        while (ci>0) {
                                ci = (ci/10);
                                sz += 1;
                        }
                        if(i == 0) { /* log 0 doesn't make sense. */
                                sz = 1;
                        } else if(i < 0) { /* allow for neg sign as well. */
                                sz++;
                        }
                        if(stage == 1) {
                            sprintf((*ret) + total, "%d", i);
                        }
                        total += sz;
                        break;
                }
                p++;
            } else {
                if(stage == 1) {
                    (*ret)[total] = *p;
                }
                total++;
            }

            p++;
        }
        if(stage == 0) {
            ret_sz = total;
            (*ret) = emalloc(ret_sz+1);
        } else {
            (*ret)[ret_sz] = 0;
            return ret_sz;
        }
    }
}

PHPAPI void redis_bulk_double_response(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock, zval *z_tab, void *ctx) {

    char *response;
    int response_len;

    zval *object = getThis();
    if ((response = redis_sock_read(redis_sock, &response_len TSRMLS_CC)) == NULL) {
        RETURN_FALSE;
    }

    double ret = atof(response);
    efree(response);
    IF_MULTI_OR_PIPELINE() {
	add_next_index_double(z_tab, ret);
    } else {
    	RETURN_DOUBLE(ret);
    }
}

PHPAPI void redis_type_response(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock, zval *z_tab, void *ctx) {
    char *response;
    int response_len;
    char ret;

    zval *object = getThis();

    if ((response = redis_sock_read(redis_sock, &response_len TSRMLS_CC)) == NULL) {
        RETURN_FALSE;
    }

    long l;
    if (strncmp(response, "+string", 7) == 0) {
	l = REDIS_STRING;
    } else if (strncmp(response, "+set", 4) == 0){
	l = REDIS_SET;
    } else if (strncmp(response, "+list", 5) == 0){
	l = REDIS_LIST;
    } else if (strncmp(response, "+zset", 5) == 0){
	l = REDIS_ZSET;
    } else if (strncmp(response, "+hash", 5) == 0){
	l = REDIS_HASH;
    } else {
	l = REDIS_NOT_FOUND;
    }

    efree(response);
    IF_MULTI_OR_PIPELINE() {
	add_next_index_long(z_tab, l);
    } else {
    	RETURN_LONG(l);
    }
}

PHPAPI void redis_info_response(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock, zval *z_tab, void *ctx) {
    char *response;
    int response_len;
    char ret;

    zval *object = getThis();

    if ((response = redis_sock_read(redis_sock, &response_len TSRMLS_CC)) == NULL) {
        RETURN_FALSE;
    }

    zval *z_multi_result;
    MAKE_STD_ZVAL(z_multi_result);
    array_init(z_multi_result); /* pre-allocate array for multi's results. */
    /* response :: [response_line]
     * response_line :: key ':' value CRLF
     */

    char *pos, *cur = response;
    while(1) {
	char *key, *value, *p;
	int is_numeric;
        /* key */
        pos = strchr(cur, ':');
        if(pos == NULL) {
            break;
        }
        key = emalloc(pos - cur + 1);
        memcpy(key, cur, pos-cur);
        key[pos-cur] = 0;

        /* value */
        cur = pos + 1;
        pos = strchr(cur, '\r');
        if(pos == NULL) {
            break;
        }
        value = emalloc(pos - cur + 1);
        memcpy(value, cur, pos-cur);
        value[pos-cur] = 0;
        pos += 2; /* \r, \n */
        cur = pos;

        is_numeric = 1;
        for(p = value; *p; ++p) {
            if(*p < '0' || *p > '9') {
                is_numeric = 0;
                break;
            }
        }

        if(is_numeric == 1) {
            add_assoc_long(z_multi_result, key, atol(value));
            efree(value);
        } else {
            add_assoc_string(z_multi_result, key, value, 0);
        }
        efree(key);
    }
    efree(response);

    IF_MULTI_OR_PIPELINE() {
        add_next_index_zval(z_tab, z_multi_result);
    } else {
	    *return_value = *z_multi_result;
	    zval_copy_ctor(return_value);
	    efree(z_multi_result);
    }
}

PHPAPI void redis_boolean_response(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock, zval *z_tab, void *ctx) {

    char *response;
    int response_len;
    char ret;

    zval *object = getThis();
    if ((response = redis_sock_read(redis_sock, &response_len TSRMLS_CC)) == NULL) {
	IF_MULTI_OR_PIPELINE() {
            add_next_index_bool(z_tab, 0);
	    return;
	}
        RETURN_FALSE;
    }
    ret = response[0];
    efree(response);

    IF_MULTI_OR_PIPELINE() {
        if (ret == '+') {
            add_next_index_bool(z_tab, 1);
        } else {
            add_next_index_bool(z_tab, 0);
        }
    }

    if (ret == '+') {
        RETURN_TRUE;
    } else {
        RETURN_FALSE;
    }
}

PHPAPI void redis_long_response(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock, zval * z_tab, void *ctx) {

    char *response;
    int response_len;
    zval *object = getThis();

    if ((response = redis_sock_read(redis_sock, &response_len TSRMLS_CC)) == NULL) {
	IF_MULTI_OR_PIPELINE() {
            add_next_index_bool(z_tab, 0);
	    return;
	}
        RETURN_FALSE;
    }

    if(response[0] == ':') {
        long ret = atol(response + 1);
        IF_MULTI_OR_PIPELINE() {
            add_next_index_long(z_tab, ret);
        }
        efree(response);
        RETURN_LONG(ret);
    } else {
        IF_MULTI_OR_PIPELINE() {
          add_next_index_null(z_tab);
        }
        efree(response);
        RETURN_FALSE;
    }
}

PHPAPI int redis_sock_read_multibulk_reply_zipped_with_flag(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock, zval *z_tab, int flag) {

	/*
	int ret = redis_sock_read_multibulk_reply(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock, z_tab TSRMLS_CC);
	array_zip_values_and_scores(return_value, 0);
	*/

    char inbuf[1024], *response;
    int response_len;

    if(-1 == redis_check_eof(redis_sock TSRMLS_CC)) {
        return -1;
    }
    php_stream_gets(redis_sock->stream, inbuf, 1024);

    if(inbuf[0] != '*') {
        return -1;
    }
    int numElems = atoi(inbuf+1);
    zval *z_multi_result;
    MAKE_STD_ZVAL(z_multi_result);
    array_init(z_multi_result); /* pre-allocate array for multi's results. */

    redis_sock_read_multibulk_reply_loop(INTERNAL_FUNCTION_PARAM_PASSTHRU,
                    redis_sock, z_multi_result, numElems);

    array_zip_values_and_scores(z_multi_result, 0);

    zval *object = getThis();
    IF_MULTI_OR_PIPELINE() {
        add_next_index_zval(z_tab, z_multi_result);
    } else {
	    *return_value = *z_multi_result;
	    zval_copy_ctor(return_value);
	    zval_dtor(z_multi_result);
	    efree(z_multi_result);
    }

    return 0;
}

PHPAPI int redis_sock_read_multibulk_reply_zipped(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock, zval *z_tab, void *ctx) {

	return redis_sock_read_multibulk_reply_zipped_with_flag(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock, z_tab, 1);
}

PHPAPI int redis_sock_read_multibulk_reply_zipped_strings(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock, zval *z_tab, void *ctx) {
	return redis_sock_read_multibulk_reply_zipped_with_flag(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock, z_tab, 0);
}

PHPAPI void redis_1_response(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock, zval *z_tab, void *ctx) {

	char *response;
	int response_len;
	char ret;

	zval *object = getThis();
	if ((response = redis_sock_read(redis_sock, &response_len TSRMLS_CC)) == NULL) {
		IF_MULTI_OR_PIPELINE() {
			add_next_index_bool(z_tab, 0);
			return;
		}
		RETURN_FALSE;
	}
	ret = response[1];
	efree(response);

	IF_MULTI_OR_PIPELINE() {
		if(ret == '1') {
			add_next_index_bool(z_tab, 1);
		} else {
			add_next_index_bool(z_tab, 0);
		}
	}

	if (ret == '1') {
		RETURN_TRUE;
    } else {
        RETURN_FALSE;
    }
}

PHPAPI void redis_string_response(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock, zval *z_tab, void *ctx) {

    char *response;
    int response_len;
    char ret;

    zval *object = getThis();

    if ((response = redis_sock_read(redis_sock, &response_len TSRMLS_CC)) == NULL) {
        IF_MULTI_OR_PIPELINE() {
            add_next_index_bool(z_tab, 0);
	    return;
        }
        RETURN_FALSE;
    }
    IF_MULTI_OR_PIPELINE() {
        add_next_index_stringl(z_tab, response, response_len, 0);
    }

    RETURN_STRINGL(response, response_len, 0);
}


/**
 * redis_sock_create
 */
PHPAPI RedisSock* redis_sock_create(char *host, int host_len, unsigned short port,
                                                                       double timeout)
{
    RedisSock *redis_sock;

    redis_sock         = emalloc(sizeof *redis_sock);
    redis_sock->host   = emalloc(host_len + 1);
    redis_sock->stream = NULL;
    redis_sock->status = REDIS_SOCK_STATUS_DISCONNECTED;

    memcpy(redis_sock->host, host, host_len);
    redis_sock->host[host_len] = '\0';

    redis_sock->port    = port;
    redis_sock->timeout = timeout;

    redis_sock->mode = ATOMIC;
    redis_sock->head = NULL;
    redis_sock->current = NULL;
    redis_sock->pipeline_head = NULL;
    redis_sock->pipeline_current = NULL;

    return redis_sock;
}

/**
 * redis_sock_connect
 */
PHPAPI int redis_sock_connect(RedisSock *redis_sock TSRMLS_DC)
{
    struct timeval tv, *tv_ptr = NULL;
    char *host = NULL, *hash_key = NULL, *errstr = NULL;
    int host_len, err = 0;

    if (redis_sock->stream != NULL) {
        redis_sock_disconnect(redis_sock TSRMLS_CC);
    }

    tv.tv_sec  = (time_t)redis_sock->timeout;
    tv.tv_usec = (int)((redis_sock->timeout - tv.tv_sec) * 1000000);

    host_len = spprintf(&host, 0, "%s:%d", redis_sock->host, redis_sock->port);

    if(tv.tv_sec != 0 || tv.tv_usec != 0) {
        tv_ptr = &tv;
    }
    redis_sock->stream = php_stream_xport_create(host, host_len, ENFORCE_SAFE_MODE,
                                                 STREAM_XPORT_CLIENT
                                                 | STREAM_XPORT_CONNECT,
                                                 hash_key, tv_ptr, NULL, &errstr, &err
                                                );

    efree(host);

    if (!redis_sock->stream) {
        efree(errstr);
        return -1;
    }

    /* set TCP_NODELAY */
    php_netstream_data_t *sock = (php_netstream_data_t*)redis_sock->stream->abstract;
    int tcp_flag = 1;
    int result = setsockopt(sock->socket, IPPROTO_TCP, TCP_NODELAY, (char *) &tcp_flag, sizeof(int));

    php_stream_auto_cleanup(redis_sock->stream);

    if(tv.tv_sec != 0) {
        php_stream_set_option(redis_sock->stream, PHP_STREAM_OPTION_READ_TIMEOUT,
                              0, &tv);
    }
    php_stream_set_option(redis_sock->stream,
                          PHP_STREAM_OPTION_WRITE_BUFFER,
                          PHP_STREAM_BUFFER_NONE, NULL);

    redis_sock->status = REDIS_SOCK_STATUS_CONNECTED;

    return 0;
}

/**
 * redis_sock_server_open
 */
PHPAPI int redis_sock_server_open(RedisSock *redis_sock, int force_connect TSRMLS_DC)
{
    int res = -1;

    switch (redis_sock->status) {
        case REDIS_SOCK_STATUS_DISCONNECTED:
            return redis_sock_connect(redis_sock TSRMLS_CC);
        case REDIS_SOCK_STATUS_CONNECTED:
            res = 0;
        break;
        case REDIS_SOCK_STATUS_UNKNOWN:
            if (force_connect > 0 && redis_sock_connect(redis_sock TSRMLS_CC) < 0) {
                res = -1;
            } else {
                res = 0;

                redis_sock->status = REDIS_SOCK_STATUS_CONNECTED;
            }
        break;
    }

    return res;
}

/**
 * redis_sock_disconnect
 */
PHPAPI int redis_sock_disconnect(RedisSock *redis_sock TSRMLS_DC)
{
    if (redis_sock == NULL) {
	    return 1;
    }

    if (redis_sock->stream != NULL) {
        redis_sock_write(redis_sock, "QUIT", sizeof("QUIT") - 1 TSRMLS_CC);

        redis_sock->status = REDIS_SOCK_STATUS_DISCONNECTED;
        if(redis_sock->stream) { /* still valid after the write? */
		php_stream_close(redis_sock->stream);
	}
        redis_sock->stream = NULL;

        return 1;
    }

    return 0;
}

/**
 * redis_sock_read_multibulk_reply
 */
PHPAPI int redis_sock_read_multibulk_reply(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock, zval *z_tab, void *ctx)
{
    char inbuf[1024], *response;
    int response_len;

    if(-1 == redis_check_eof(redis_sock TSRMLS_CC)) {
        return -1;
    }
    php_stream_gets(redis_sock->stream, inbuf, 1024);

    if(inbuf[0] != '*') {
        return -1;
    }
    int numElems = atoi(inbuf+1);
    zval *z_multi_result;
    MAKE_STD_ZVAL(z_multi_result);
    array_init(z_multi_result); /* pre-allocate array for multi's results. */

    redis_sock_read_multibulk_reply_loop(INTERNAL_FUNCTION_PARAM_PASSTHRU,
                    redis_sock, z_multi_result, numElems);

    zval *object = getThis();
    IF_MULTI_OR_PIPELINE() {
        add_next_index_zval(z_tab, z_multi_result);
    }

    *return_value = *z_multi_result;
    //zval_copy_ctor(return_value);
    // efree(z_multi_result);
    return 0;
}

PHPAPI int
redis_sock_read_multibulk_reply_loop(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock,
                                     zval *z_tab, int numElems)
{
    char *response;
    int response_len;

    while(numElems > 0) {
        int response_len;
        response = redis_sock_read(redis_sock, &response_len TSRMLS_CC);
        if(response != NULL) {
            add_next_index_stringl(z_tab, response, response_len, 0);
        } else {
            add_next_index_bool(z_tab, 0);
        }
        numElems --;
    }
    return 0;
}

/**
 * redis_sock_read_multibulk_reply_assoc
 */
PHPAPI int redis_sock_read_multibulk_reply_assoc(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock, zval *z_tab, void *ctx)
{
    char inbuf[1024], *response;
    int response_len;

    zval **z_keys = ctx;

    if(-1 == redis_check_eof(redis_sock TSRMLS_CC)) {
        return -1;
    }
    php_stream_gets(redis_sock->stream, inbuf, 1024);

    if(inbuf[0] != '*') {
        return -1;
    }
    int i, numElems = atoi(inbuf+1);
    zval *z_multi_result;
    MAKE_STD_ZVAL(z_multi_result);
    array_init(z_multi_result); /* pre-allocate array for multi's results. */

    for(i = 0; i < numElems; ++i) {
        response = redis_sock_read(redis_sock, &response_len TSRMLS_CC);
        if(response != NULL) {
            add_assoc_stringl_ex(z_multi_result, Z_STRVAL_P(z_keys[i]), 1+Z_STRLEN_P(z_keys[i]), response, response_len, 1);
        } else {
            add_assoc_bool_ex(z_multi_result, Z_STRVAL_P(z_keys[i]), 1+Z_STRLEN_P(z_keys[i]), 0);
        }
	zval_dtor(z_keys[i]);
	efree(z_keys[i]);
    }
    efree(z_keys);

    zval *object = getThis();
    IF_MULTI_OR_PIPELINE() {
        add_next_index_zval(z_tab, z_multi_result);
    }

    *return_value = *z_multi_result;
    //zval_copy_ctor(return_value);
    // efree(z_multi_result);
    return 0;
}

/**
 * redis_sock_write
 */
PHPAPI int redis_sock_write(RedisSock *redis_sock, char *cmd, size_t sz TSRMLS_DC)
{
	if(redis_sock && redis_sock->status == REDIS_SOCK_STATUS_DISCONNECTED) {
		zend_throw_exception(redis_exception_ce, "Connection closed", 0 TSRMLS_CC);
		return -1;
	}
    if(-1 == redis_check_eof(redis_sock TSRMLS_CC)) {
        return -1;
    }
    return php_stream_write(redis_sock->stream, cmd, sz);
}

/**
 * redis_free_socket
 */
PHPAPI void redis_free_socket(RedisSock *redis_sock)
{
    efree(redis_sock->host);
    efree(redis_sock);
}

