#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include "common.h"
#include "php_network.h"
#include <sys/types.h>
#ifndef _MSC_VER
#include <netinet/tcp.h>  /* TCP_NODELAY */
#include <sys/socket.h>
#endif
#include <ext/standard/php_smart_str.h>
#include <ext/standard/php_var.h>
#ifdef HAVE_REDIS_IGBINARY
#include "igbinary/igbinary.h"
#endif
#include <zend_exceptions.h>
#include "php_redis.h"
#include "library.h"
#include <ext/standard/php_math.h>

#define UNSERIALIZE_ONLY_VALUES 0
#define UNSERIALIZE_ALL 1

extern zend_class_entry *redis_ce;
extern zend_class_entry *redis_exception_ce;
extern zend_class_entry *spl_ce_RuntimeException;

PHPAPI void redis_stream_close(RedisSock *redis_sock TSRMLS_DC) {
	if (!redis_sock->persistent) {
		php_stream_close(redis_sock->stream);
	} else {
		php_stream_pclose(redis_sock->stream);
	}
}

PHPAPI int redis_check_eof(RedisSock *redis_sock TSRMLS_DC)
{
    int eof;
    int count = 0;

	if (!redis_sock->stream) {
		return -1;
	}

	eof = php_stream_eof(redis_sock->stream);
    for (; eof; count++) {
        if((MULTI == redis_sock->mode) || redis_sock->watching || count == 10) { /* too many failures */
	    if(redis_sock->stream) { /* close stream if still here */
                redis_stream_close(redis_sock TSRMLS_CC);
                redis_sock->stream = NULL;
				redis_sock->mode   = ATOMIC;
                redis_sock->status = REDIS_SOCK_STATUS_FAILED;
                redis_sock->watching = 0;
	    }
            zend_throw_exception(redis_exception_ce, "Connection lost", 0 TSRMLS_CC);
	    return -1;
	}
	if(redis_sock->stream) { /* close existing stream before reconnecting */
            redis_stream_close(redis_sock TSRMLS_CC);
            redis_sock->stream = NULL;
			redis_sock->mode   = ATOMIC;
            redis_sock->watching = 0;
	}
    // Wait for a while before trying to reconnect
    if (redis_sock->retry_interval) {
    	// Random factor to avoid having several (or many) concurrent connections trying to reconnect at the same time
   		long retry_interval = (count ? redis_sock->retry_interval : (random() % redis_sock->retry_interval));
    	usleep(retry_interval);
    }
        redis_sock_connect(redis_sock TSRMLS_CC); /* reconnect */
        if(redis_sock->stream) { /*  check for EOF again. */
            eof = php_stream_eof(redis_sock->stream);
        }
    }

    // Reselect the DB.
    if (count && redis_sock->dbNumber) {
        char *cmd, *response;
        int cmd_len, response_len;

        cmd_len = redis_cmd_format_static(&cmd, "SELECT", "d", redis_sock->dbNumber);

        if (redis_sock_write(redis_sock, cmd, cmd_len TSRMLS_CC) < 0) {
            efree(cmd);
            return -1;
        }
        efree(cmd);

        if ((response = redis_sock_read(redis_sock, &response_len TSRMLS_CC)) == NULL) {
            return -1;
        }

        if (strncmp(response, "+OK", 3)) {
            efree(response);
            return -1;
        }
        efree(response);
    }

    return 0;
}

PHPAPI zval *redis_sock_read_multibulk_reply_zval(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock) {
    char inbuf[1024];
	int numElems;
    zval *z_tab;

    if(-1 == redis_check_eof(redis_sock TSRMLS_CC)) {
        return NULL;
    }

    if(php_stream_gets(redis_sock->stream, inbuf, 1024) == NULL) {
		redis_stream_close(redis_sock TSRMLS_CC);
        redis_sock->stream = NULL;
        redis_sock->status = REDIS_SOCK_STATUS_FAILED;
        redis_sock->mode = ATOMIC;
        redis_sock->watching = 0;
        zend_throw_exception(redis_exception_ce, "read error on connection", 0 TSRMLS_CC);
        return NULL;
    }

    if(inbuf[0] != '*') {
        return NULL;
    }
    numElems = atoi(inbuf+1);

    MAKE_STD_ZVAL(z_tab);
    array_init(z_tab);

    redis_sock_read_multibulk_reply_loop(INTERNAL_FUNCTION_PARAM_PASSTHRU,
                    redis_sock, z_tab, numElems, 1, UNSERIALIZE_ALL);
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
        char c;
        int i;
        
		reply = emalloc(bytes+1);

        while(offset < bytes) {
            got = php_stream_read(redis_sock->stream, reply + offset, bytes-offset);
            if (got <= 0) {
                /* Error or EOF */
				zend_throw_exception(redis_exception_ce, "socket error on read socket", 0 TSRMLS_CC);
                break;
            }
            offset += got;
        }
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
    size_t err_len;

    if(-1 == redis_check_eof(redis_sock TSRMLS_CC)) {
        return NULL;
    }

    if(php_stream_gets(redis_sock->stream, inbuf, 1024) == NULL) {
		redis_stream_close(redis_sock TSRMLS_CC);
        redis_sock->stream = NULL;
        redis_sock->status = REDIS_SOCK_STATUS_FAILED;
        redis_sock->mode = ATOMIC;
        redis_sock->watching = 0;
        zend_throw_exception(redis_exception_ce, "read error on connection", 0 TSRMLS_CC);
        return NULL;
    }

    switch(inbuf[0]) {
        case '-':
			err_len = strlen(inbuf+1) - 2;
			redis_sock_set_err(redis_sock, inbuf+1, err_len);
			/* stale data */
			if(memcmp(inbuf + 1, "-ERR SYNC ", 10) == 0) {
				zend_throw_exception(redis_exception_ce, "SYNC with master in progress", 0 TSRMLS_CC);
			}
            return NULL;

        case '$':
            *buf_len = atoi(inbuf + 1);
            resp = redis_sock_read_bulk_reply(redis_sock, *buf_len TSRMLS_CC);
            return resp;

        case '*':
            /* For null multi-bulk replies (like timeouts from brpoplpush): */
            if(memcmp(inbuf + 1, "-1", 2) == 0) {
                return NULL;
            }
            /* fall through */

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
	while (ci > 0) {
		ci /= 10;
		sz++;
	}
	if (i == 0) { /* log 0 doesn't make sense. */
		sz = 1;
	} else if (i < 0) { /* allow for neg sign as well. */
		sz++;
	}
	return sz;
}

int
redis_cmd_format_header(char **ret, char *keyword, int arg_count) {
	// Our return buffer
	smart_str buf = {0};

	// Keyword length
	int l = strlen(keyword);

	smart_str_appendc(&buf, '*');
	smart_str_append_long(&buf, arg_count + 1);
	smart_str_appendl(&buf, _NL, sizeof(_NL) -1);
	smart_str_appendc(&buf, '$');
	smart_str_append_long(&buf, l);
	smart_str_appendl(&buf, _NL, sizeof(_NL) -1);
	smart_str_appendl(&buf, keyword, l);
	smart_str_appendl(&buf, _NL, sizeof(_NL) - 1);

	// Set our return pointer
	*ret = buf.c;

	// Return the length
	return buf.len;
}

int
redis_cmd_format_static(char **ret, char *keyword, char *format, ...) {

    char *p = format;
    va_list ap;
    smart_str buf = {0};
    int l = strlen(keyword);
	char *dbl_str;
	int dbl_len;

	va_start(ap, format);

	/* add header */
	smart_str_appendc(&buf, '*');
	smart_str_append_long(&buf, strlen(format) + 1);
	smart_str_appendl(&buf, _NL, sizeof(_NL) - 1);
	smart_str_appendc(&buf, '$');
	smart_str_append_long(&buf, l);
	smart_str_appendl(&buf, _NL, sizeof(_NL) - 1);
	smart_str_appendl(&buf, keyword, l);
	smart_str_appendl(&buf, _NL, sizeof(_NL) - 1);

	while (*p) {
		smart_str_appendc(&buf, '$');

		switch(*p) {
			case 's': {
					char *val = va_arg(ap, char*);
					int val_len = va_arg(ap, int);
					smart_str_append_long(&buf, val_len);
					smart_str_appendl(&buf, _NL, sizeof(_NL) - 1);
					smart_str_appendl(&buf, val, val_len);
				}
				break;

			case 'f':
			case 'F': {
				double d = va_arg(ap, double);
				REDIS_DOUBLE_TO_STRING(dbl_str, dbl_len, d)
				smart_str_append_long(&buf, dbl_len);
				smart_str_appendl(&buf, _NL, sizeof(_NL) - 1);
				smart_str_appendl(&buf, dbl_str, dbl_len);
				efree(dbl_str);
			}
				break;

			case 'i':
			case 'd': {
				int i = va_arg(ap, int);
				char tmp[32];
				int tmp_len = snprintf(tmp, sizeof(tmp), "%d", i);
				smart_str_append_long(&buf, tmp_len);
				smart_str_appendl(&buf, _NL, sizeof(_NL) - 1);
				smart_str_appendl(&buf, tmp, tmp_len);
			}
				break;
			case 'l':
			case 'L': {
				long l = va_arg(ap, long);
				char tmp[32];
				int tmp_len = snprintf(tmp, sizeof(tmp), "%ld", l);
				smart_str_append_long(&buf, tmp_len);
				smart_str_appendl(&buf, _NL, sizeof(_NL) -1);
				smart_str_appendl(&buf, tmp, tmp_len);
			}
				break;
		}
		p++;
		smart_str_appendl(&buf, _NL, sizeof(_NL) - 1);
	}
	smart_str_0(&buf);

	*ret = buf.c;

	return buf.len;
}

/**
 * This command behave somehow like printf, except that strings need 2 arguments:
 * Their data and their size (strlen).
 * Supported formats are: %d, %i, %s, %l
 */
int
redis_cmd_format(char **ret, char *format, ...) {

	smart_str buf = {0};
	va_list ap;
	char *p = format;
	char *dbl_str;
	int dbl_len;

	va_start(ap, format);

	while (*p) {
		if (*p == '%') {
			switch (*(++p)) {
				case 's': {
					char *tmp = va_arg(ap, char*);
					int tmp_len = va_arg(ap, int);
					smart_str_appendl(&buf, tmp, tmp_len);
				}
					break;

				case 'F':
				case 'f': {
					double d = va_arg(ap, double);
					REDIS_DOUBLE_TO_STRING(dbl_str, dbl_len, d)
					smart_str_append_long(&buf, dbl_len);
					smart_str_appendl(&buf, _NL, sizeof(_NL) - 1);
					smart_str_appendl(&buf, dbl_str, dbl_len);
					efree(dbl_str);
				}
					break;

				case 'd':
				case 'i': {
					int i = va_arg(ap, int);
					char tmp[32];
					int tmp_len = snprintf(tmp, sizeof(tmp), "%d", i);
					smart_str_appendl(&buf, tmp, tmp_len);
				}
					break;
			}
		} else {
			smart_str_appendc(&buf, *p);
		}

		p++;
	}

	smart_str_0(&buf);

	*ret = buf.c;

	return buf.len;
}

/*
 * Append a command sequence to a Redis command
 */
int redis_cmd_append_str(char **cmd, int cmd_len, char *append, int append_len) {
	// Smart string buffer
	smart_str buf = {0};

	// Append the current command to our smart_str
	smart_str_appendl(&buf, *cmd, cmd_len);

	// Append our new command sequence
	smart_str_appendc(&buf, '$');
	smart_str_append_long(&buf, append_len);
	smart_str_appendl(&buf, _NL, sizeof(_NL) -1);
	smart_str_appendl(&buf, append, append_len);
	smart_str_appendl(&buf, _NL, sizeof(_NL) -1);

	// Free our old command
	efree(*cmd);

	// Set our return pointer
	*cmd = buf.c;

	// Return new command length
	return buf.len;
}

/*
 * Given a smart string, number of arguments, a keyword, and the length of the keyword
 * initialize our smart string with the proper Redis header for the command to follow
 */
int redis_cmd_init_sstr(smart_str *str, int num_args, char *keyword, int keyword_len) {
    smart_str_appendc(str, '*');
    smart_str_append_long(str, num_args + 1);
    smart_str_appendl(str, _NL, sizeof(_NL) -1);
    smart_str_appendc(str, '$');
    smart_str_append_long(str, keyword_len);
    smart_str_appendl(str, _NL, sizeof(_NL) - 1);
    smart_str_appendl(str, keyword, keyword_len);
    smart_str_appendl(str, _NL, sizeof(_NL) - 1);
    return str->len;
}

/*
 * Append a command sequence to a smart_str
 */
int redis_cmd_append_sstr(smart_str *str, char *append, int append_len) {
    smart_str_appendc(str, '$');
    smart_str_append_long(str, append_len);
    smart_str_appendl(str, _NL, sizeof(_NL) - 1);
    smart_str_appendl(str, append, append_len);
    smart_str_appendl(str, _NL, sizeof(_NL) - 1);

    // Return our new length
    return str->len;
}

/*
 * Append an integer to a smart string command
 */
int redis_cmd_append_sstr_int(smart_str *str, int append) {
    char int_buf[32];
    int int_len = snprintf(int_buf, sizeof(int_buf), "%d", append);
    return redis_cmd_append_sstr(str, int_buf, int_len);
}

/*
 * Append a long to a smart string command
 */
int redis_cmd_append_sstr_long(smart_str *str, long append) {
    char long_buf[32];
    int long_len = snprintf(long_buf, sizeof(long_buf), "%ld", append);
    return redis_cmd_append_sstr(str, long_buf, long_len);
}

/*
 * Append a double to a smart string command
 */
int redis_cmd_append_sstr_dbl(smart_str *str, double value) {
    char *dbl_str;
    int dbl_len;

    /// Convert to double
    REDIS_DOUBLE_TO_STRING(dbl_str, dbl_len, value);

    // Append the string
    int retval = redis_cmd_append_sstr(str, dbl_str, dbl_len);

    // Free our double string
    efree(dbl_str);

    // Return new length
    return retval;
}

/*
 * Append an integer command to a Redis command
 */
int redis_cmd_append_int(char **cmd, int cmd_len, int append) {
	char int_buf[32];

	// Conver to an int, capture length
	int int_len = snprintf(int_buf, sizeof(int_buf), "%d", append);

	// Return the new length
	return redis_cmd_append_str(cmd, cmd_len, int_buf, int_len);
}

PHPAPI void redis_bulk_double_response(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock, zval *z_tab, void *ctx) {

    char *response;
    int response_len;
	double ret;

    if ((response = redis_sock_read(redis_sock, &response_len TSRMLS_CC)) == NULL) {
		IF_MULTI_OR_PIPELINE() {
			add_next_index_bool(z_tab, 0);
		} else {
			RETURN_FALSE;
		}
		return;
    }

    ret = atof(response);
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
    long l;

    if ((response = redis_sock_read(redis_sock, &response_len TSRMLS_CC)) == NULL) {
		IF_MULTI_OR_PIPELINE() {
			add_next_index_bool(z_tab, 0);
		} else {
			RETURN_FALSE;
		}
    }

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
	char *pos, *cur;
	char *key, *value, *p;
	int is_numeric;
    zval *z_multi_result;

    if ((response = redis_sock_read(redis_sock, &response_len TSRMLS_CC)) == NULL) {
        RETURN_FALSE;
    }

    MAKE_STD_ZVAL(z_multi_result);
    array_init(z_multi_result); /* pre-allocate array for multi's results. */
    /* response :: [response_line]
     * response_line :: key ':' value CRLF
     */

    cur = response;
    while(1) {

		/* skip comments and empty lines */
		if(*cur == '#' || *cur == '\r') {
			if(!(cur = strchr(cur, '\n')))
				break;
			cur++;
			continue;
		}

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
        RETVAL_ZVAL(z_multi_result, 0, 1);
    }
}

/*
 * Specialized handling of the CLIENT LIST output so it comes out in a simple way for PHP userland code
 * to handle.
 */
PHPAPI void redis_client_list_reply(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock, zval *z_tab) {
    char *resp;
    int resp_len;
    zval *z_result, *z_sub_result;
    
    // Make sure we can read a response from Redis
    if((resp = redis_sock_read(redis_sock, &resp_len TSRMLS_CC)) == NULL) {
        RETURN_FALSE;
    }

    // Allocate memory for our response
    MAKE_STD_ZVAL(z_result);
    array_init(z_result);

    // Allocate memory for one user (there should be at least one, namely us!)
    ALLOC_INIT_ZVAL(z_sub_result);
    array_init(z_sub_result);

    // Pointers for parsing
    char *p = resp, *lpos = resp, *kpos = NULL, *vpos = NULL, *p2, *key, *value;

    // Key length, done flag
    int klen, done = 0, is_numeric;

    // While we've got more to parse
    while(!done) {
        // What character are we on
        switch(*p) {
            /* We're done */
            case '\0':
                done = 1;
                break;
            /* \n, ' ' mean we can pull a k/v pair */
            case '\n':
            case ' ':
                // Grab our value
                vpos = lpos;

                // There is some communication error or Redis bug if we don't
                // have a key and value, but check anyway.
                if(kpos && vpos) {
                    // Allocate, copy in our key
                    key = emalloc(klen + 1);
                    strncpy(key, kpos, klen);
                    key[klen] = 0;

                    // Allocate, copy in our value
                    value = emalloc(p-lpos+1);
                    strncpy(value,lpos,p-lpos+1);
                    value[p-lpos]=0;

                    // Treat numbers as numbers, strings as strings
                    is_numeric = 1;
                    for(p2 = value; *p; ++p) {
                        if(*p < '0' || *p > '9') {
                            is_numeric = 0;
                            break;
                        }
                    }

                    // Add as a long or string, depending
                    if(is_numeric == 1) {
                        add_assoc_long(z_sub_result, key, atol(value));
                        efree(value);
                    } else {
                        add_assoc_string(z_sub_result, key, value, 0);
                    }

                    // If we hit a '\n', then we can add this user to our list
                    if(*p == '\n') {
                        // Add our user
                        add_next_index_zval(z_result, z_sub_result);

                        // If we have another user, make another one
                        if(*(p+1) != '\0') {
                            ALLOC_INIT_ZVAL(z_sub_result);
                            array_init(z_sub_result);
                        }
                    }
                    
                    // Free our key
                    efree(key);
                } else {
                    // Something is wrong
                    efree(resp);
                    RETURN_FALSE;
                }

                // Move forward
                lpos = p + 1;

                break;
            /* We can pull the key and null terminate at our sep */
            case '=':
                // Key, key length
                kpos = lpos;
                klen = p - lpos;

                // Move forward
                lpos = p + 1;

                break;
        }

        // Increment
        p++;
    }

    // Free our respoonse
    efree(resp);

    IF_MULTI_OR_PIPELINE() { 
        add_next_index_zval(z_tab, z_result);
    } else {
        RETVAL_ZVAL(z_result, 0, 1);
    }
}

PHPAPI void redis_boolean_response_impl(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock, zval *z_tab, void *ctx, SuccessCallback success_callback) {

    char *response;
    int response_len;
    char ret;

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
            if (success_callback != NULL) {
                success_callback(redis_sock);
            }
            add_next_index_bool(z_tab, 1);
        } else {
            add_next_index_bool(z_tab, 0);
        }
    } else {
		if (ret == '+') {
            if (success_callback != NULL) {
                success_callback(redis_sock);
            }
			RETURN_TRUE;
		} else {
			RETURN_FALSE;
		}
	}
}

PHPAPI void redis_boolean_response(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock, zval *z_tab, void *ctx) {
    redis_boolean_response_impl(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock, z_tab, ctx, NULL);
}

PHPAPI void redis_long_response(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock, zval * z_tab, void *ctx) {

    char *response;
    int response_len;

    if ((response = redis_sock_read(redis_sock, &response_len TSRMLS_CC)) == NULL) {
		IF_MULTI_OR_PIPELINE() {
			add_next_index_bool(z_tab, 0);
			return;
		} else {
			RETURN_FALSE;
		}
    }

    if(response[0] == ':') {
        long long ret = atoll(response + 1);
        IF_MULTI_OR_PIPELINE() {
			if(ret > LONG_MAX) { /* overflow */
				add_next_index_stringl(z_tab, response+1, response_len-1, 1);
			} else {
				efree(response);
				add_next_index_long(z_tab, (long)ret);
			}
        } else {
			if(ret > LONG_MAX) { /* overflow */
				RETURN_STRINGL(response+1, response_len-1, 1);
			} else {
				efree(response);
				RETURN_LONG((long)ret);
			}
		}
    } else {
        efree(response);
        IF_MULTI_OR_PIPELINE() {
          add_next_index_null(z_tab);
        } else {
			RETURN_FALSE;
		}
    }
}



PHPAPI int redis_sock_read_multibulk_reply_zipped_with_flag(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock, zval *z_tab, int flag) {

	/*
	int ret = redis_sock_read_multibulk_reply(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock, z_tab TSRMLS_CC);
	array_zip_values_and_scores(return_value, 0);
	*/

    char inbuf[1024];
	int numElems;
    zval *z_multi_result;

    if(-1 == redis_check_eof(redis_sock TSRMLS_CC)) {
        return -1;
    }
    if(php_stream_gets(redis_sock->stream, inbuf, 1024) == NULL) {
		redis_stream_close(redis_sock TSRMLS_CC);
        redis_sock->stream = NULL;
        redis_sock->status = REDIS_SOCK_STATUS_FAILED;
        redis_sock->mode = ATOMIC;
        redis_sock->watching = 0;
        zend_throw_exception(redis_exception_ce, "read error on connection", 0 TSRMLS_CC);
        return -1;
    }

    if(inbuf[0] != '*') {
        IF_MULTI_OR_PIPELINE() {
            add_next_index_bool(z_tab, 0);
        } else {
            RETVAL_FALSE;
        }
        return -1;
    }
    numElems = atoi(inbuf+1);
    MAKE_STD_ZVAL(z_multi_result);
    array_init(z_multi_result); /* pre-allocate array for multi's results. */

    redis_sock_read_multibulk_reply_loop(INTERNAL_FUNCTION_PARAM_PASSTHRU,
                    redis_sock, z_multi_result, numElems, 1, flag ? UNSERIALIZE_ALL : UNSERIALIZE_ONLY_VALUES);

    array_zip_values_and_scores(redis_sock, z_multi_result, 0 TSRMLS_CC);

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

	if ((response = redis_sock_read(redis_sock, &response_len TSRMLS_CC)) == NULL) {
		IF_MULTI_OR_PIPELINE() {
			add_next_index_bool(z_tab, 0);
			return;
		} else {
			RETURN_FALSE;
		}
	}
	ret = response[1];
	efree(response);

	IF_MULTI_OR_PIPELINE() {
		if(ret == '1') {
			add_next_index_bool(z_tab, 1);
		} else {
			add_next_index_bool(z_tab, 0);
		}
	} else {
		if (ret == '1') {
			RETURN_TRUE;
		} else {
			RETURN_FALSE;
		}
	}
}

PHPAPI void redis_string_response(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock, zval *z_tab, void *ctx) {

    char *response;
    int response_len;

    if ((response = redis_sock_read(redis_sock, &response_len TSRMLS_CC)) == NULL) {
        IF_MULTI_OR_PIPELINE() {
            add_next_index_bool(z_tab, 0);
	    return;
        }
        RETURN_FALSE;
    }
    IF_MULTI_OR_PIPELINE() {
		zval *z = NULL;
		if(redis_unserialize(redis_sock, response, response_len, &z TSRMLS_CC) == 1) {
			efree(response);
			add_next_index_zval(z_tab, z);
		} else {
			add_next_index_stringl(z_tab, response, response_len, 0);
		}
    } else {
		if(redis_unserialize(redis_sock, response, response_len, &return_value TSRMLS_CC) == 0) {
		    RETURN_STRINGL(response, response_len, 0);
		} else {
			efree(response);
		}
	}
}

/* like string response, but never unserialized. */
PHPAPI void redis_ping_response(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock, zval *z_tab, void *ctx) {

    char *response;
    int response_len;

    if ((response = redis_sock_read(redis_sock, &response_len TSRMLS_CC)) == NULL) {
        IF_MULTI_OR_PIPELINE() {
            add_next_index_bool(z_tab, 0);
	    return;
        }
        RETURN_FALSE;
    }
    IF_MULTI_OR_PIPELINE() {
		add_next_index_stringl(z_tab, response, response_len, 0);
    } else {
		RETURN_STRINGL(response, response_len, 0);
	}
}


/**
 * redis_sock_create
 */
PHPAPI RedisSock* redis_sock_create(char *host, int host_len, unsigned short port,
                                    double timeout, int persistent, char *persistent_id,
                                    long retry_interval,
                                    zend_bool lazy_connect)
{
    RedisSock *redis_sock;

    redis_sock         = ecalloc(1, sizeof(RedisSock));
    redis_sock->host   = estrndup(host, host_len);
    redis_sock->stream = NULL;
    redis_sock->status = REDIS_SOCK_STATUS_DISCONNECTED;
    redis_sock->watching = 0;
    redis_sock->dbNumber = 0;
    redis_sock->retry_interval = retry_interval * 1000;
    redis_sock->persistent = persistent;
    redis_sock->lazy_connect = lazy_connect;

    if(persistent_id) {
		size_t persistent_id_len = strlen(persistent_id);
        redis_sock->persistent_id = ecalloc(persistent_id_len + 1, 1);
        memcpy(redis_sock->persistent_id, persistent_id, persistent_id_len);
    } else {
        redis_sock->persistent_id = NULL;
    }

    memcpy(redis_sock->host, host, host_len);
    redis_sock->host[host_len] = '\0';

    redis_sock->port    = port;
    redis_sock->timeout = timeout;
    redis_sock->read_timeout = timeout;

    redis_sock->serializer = REDIS_SERIALIZER_NONE;
    redis_sock->mode = ATOMIC;
    redis_sock->head = NULL;
    redis_sock->current = NULL;
    redis_sock->pipeline_head = NULL;
    redis_sock->pipeline_current = NULL;

    redis_sock->err = NULL;
    redis_sock->err_len = 0;

    return redis_sock;
}

/**
 * redis_sock_connect
 */
PHPAPI int redis_sock_connect(RedisSock *redis_sock TSRMLS_DC)
{
    struct timeval tv, read_tv, *tv_ptr = NULL;
    char *host = NULL, *persistent_id = NULL, *errstr = NULL;
    int host_len, err = 0;
	php_netstream_data_t *sock;
	int tcp_flag = 1;

    if (redis_sock->stream != NULL) {
        redis_sock_disconnect(redis_sock TSRMLS_CC);
    }

    tv.tv_sec  = (time_t)redis_sock->timeout;
    tv.tv_usec = (int)((redis_sock->timeout - tv.tv_sec) * 1000000);
    if(tv.tv_sec != 0 || tv.tv_usec != 0) {
	    tv_ptr = &tv;
    }

    read_tv.tv_sec  = (time_t)redis_sock->read_timeout;
    read_tv.tv_usec = (int)((redis_sock->read_timeout - read_tv.tv_sec) * 1000000);

    if(redis_sock->host[0] == '/' && redis_sock->port < 1) {
	    host_len = spprintf(&host, 0, "unix://%s", redis_sock->host);
    } else {
	    if(redis_sock->port == 0)
	    	redis_sock->port = 6379;
	    host_len = spprintf(&host, 0, "%s:%d", redis_sock->host, redis_sock->port);
    }

    if (redis_sock->persistent) {
      if (redis_sock->persistent_id) {
        spprintf(&persistent_id, 0, "phpredis:%s:%s", host, redis_sock->persistent_id);
      } else {
        spprintf(&persistent_id, 0, "phpredis:%s:%f", host, redis_sock->timeout);
      }
    }

    redis_sock->stream = php_stream_xport_create(host, host_len, ENFORCE_SAFE_MODE,
							 STREAM_XPORT_CLIENT
							 | STREAM_XPORT_CONNECT,
							 persistent_id, tv_ptr, NULL, &errstr, &err
							);

    if (persistent_id) {
      efree(persistent_id);
    }

    efree(host);

    if (!redis_sock->stream) {
        efree(errstr);
        return -1;
    }

    /* set TCP_NODELAY */
	sock = (php_netstream_data_t*)redis_sock->stream->abstract;
    setsockopt(sock->socket, IPPROTO_TCP, TCP_NODELAY, (char *) &tcp_flag, sizeof(int));

    php_stream_auto_cleanup(redis_sock->stream);

    if(tv.tv_sec != 0 || tv.tv_usec != 0) {
        php_stream_set_option(redis_sock->stream, PHP_STREAM_OPTION_READ_TIMEOUT,
                              0, &read_tv);
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

    redis_sock->dbNumber = 0;
    if (redis_sock->stream != NULL) {
			if (!redis_sock->persistent) {
				redis_sock_write(redis_sock, "QUIT" _NL, sizeof("QUIT" _NL) - 1 TSRMLS_CC);
			}

			redis_sock->status = REDIS_SOCK_STATUS_DISCONNECTED;
            redis_sock->watching = 0;
			if(redis_sock->stream && !redis_sock->persistent) { /* still valid after the write? */
				php_stream_close(redis_sock->stream);
			}
			redis_sock->stream = NULL;

			return 1;
    }

    return 0;
}

PHPAPI void redis_send_discard(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock)
{
    char *cmd;
	int response_len, cmd_len;
	char * response;

    cmd_len = redis_cmd_format_static(&cmd, "DISCARD", "");

    if (redis_sock_write(redis_sock, cmd, cmd_len TSRMLS_CC) < 0) {
        efree(cmd);
        RETURN_FALSE;
    }
    efree(cmd);

    if ((response = redis_sock_read(redis_sock, &response_len TSRMLS_CC)) == NULL) {
        RETURN_FALSE;
    }

	if(response_len == 3 && strncmp(response, "+OK", 3) == 0) {
		RETURN_TRUE;
	}
	RETURN_FALSE;
}

/**
 * redis_sock_set_err
 */
PHPAPI int redis_sock_set_err(RedisSock *redis_sock, const char *msg, int msg_len) {
	// Allocate/Reallocate our last error member
	if(msg != NULL && msg_len > 0) {
		if(redis_sock->err == NULL) {
			redis_sock->err = emalloc(msg_len + 1);
		} else if(msg_len > redis_sock->err_len) {
			redis_sock->err = erealloc(redis_sock->err, msg_len +1);
		}

		// Copy in our new error message, set new length, and null terminate
		memcpy(redis_sock->err, msg, msg_len);
		redis_sock->err[msg_len] = '\0';
		redis_sock->err_len = msg_len;
	} else {
		// Free our last error
		if(redis_sock->err != NULL) {
			efree(redis_sock->err);
		}

		// Set to null, with zero length
		redis_sock->err = NULL;
		redis_sock->err_len = 0;
	}

	// Success
	return 0;
}

/**
 * redis_sock_read_multibulk_reply
 */
PHPAPI int redis_sock_read_multibulk_reply(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock, zval *z_tab, void *ctx)
{
    char inbuf[1024];
	int numElems;
    zval *z_multi_result;

    if(-1 == redis_check_eof(redis_sock TSRMLS_CC)) {
        return -1;
    }
    if(php_stream_gets(redis_sock->stream, inbuf, 1024) == NULL) {
		redis_stream_close(redis_sock TSRMLS_CC);
        redis_sock->stream = NULL;
        redis_sock->status = REDIS_SOCK_STATUS_FAILED;
        redis_sock->mode = ATOMIC;
        redis_sock->watching = 0;
        zend_throw_exception(redis_exception_ce, "read error on connection", 0 TSRMLS_CC);
        return -1;
    }

    if(inbuf[0] != '*') {
        IF_MULTI_OR_PIPELINE() {
            add_next_index_bool(z_tab, 0);
        } else {
            RETVAL_FALSE;
        }
        return -1;
    }
    numElems = atoi(inbuf+1);
    MAKE_STD_ZVAL(z_multi_result);
    array_init(z_multi_result); /* pre-allocate array for multi's results. */

    redis_sock_read_multibulk_reply_loop(INTERNAL_FUNCTION_PARAM_PASSTHRU,
                    redis_sock, z_multi_result, numElems, 1, UNSERIALIZE_ALL);

    IF_MULTI_OR_PIPELINE() {
        add_next_index_zval(z_tab, z_multi_result);
    } else {
        *return_value = *z_multi_result;
        efree(z_multi_result);
    }
    //zval_copy_ctor(return_value);
    return 0;
}

/**
 * Like multibulk reply, but don't touch the values, they won't be compressed. (this is used by HKEYS).
 */
PHPAPI int redis_sock_read_multibulk_reply_raw(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock, zval *z_tab, void *ctx)
{
    char inbuf[1024];
	int numElems;
    zval *z_multi_result;

    if(-1 == redis_check_eof(redis_sock TSRMLS_CC)) {
        return -1;
    }
    if(php_stream_gets(redis_sock->stream, inbuf, 1024) == NULL) {
		redis_stream_close(redis_sock TSRMLS_CC);
        redis_sock->stream = NULL;
        redis_sock->status = REDIS_SOCK_STATUS_FAILED;
        redis_sock->mode = ATOMIC;
        redis_sock->watching = 0;
        zend_throw_exception(redis_exception_ce, "read error on connection", 0 TSRMLS_CC);
        return -1;
    }

    if(inbuf[0] != '*') {
        IF_MULTI_OR_PIPELINE() {
            add_next_index_bool(z_tab, 0);
        } else {
            RETVAL_FALSE;
        }
        return -1;
    }
    numElems = atoi(inbuf+1);
    MAKE_STD_ZVAL(z_multi_result);
    array_init(z_multi_result); /* pre-allocate array for multi's results. */

    redis_sock_read_multibulk_reply_loop(INTERNAL_FUNCTION_PARAM_PASSTHRU,
                    redis_sock, z_multi_result, numElems, 0, UNSERIALIZE_ALL);

    IF_MULTI_OR_PIPELINE() {
        add_next_index_zval(z_tab, z_multi_result);
    } else {
        *return_value = *z_multi_result;
        efree(z_multi_result);
    }
    //zval_copy_ctor(return_value);
    return 0;
}

PHPAPI int
redis_sock_read_multibulk_reply_loop(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock,
                                     zval *z_tab, int numElems, int unwrap_key, int unserialize_even_only)
{
    char *response;
    int response_len;

    while(numElems > 0) {
        response = redis_sock_read(redis_sock, &response_len TSRMLS_CC);
        if(response != NULL) {
		zval *z = NULL;
		int can_unserialize = unwrap_key;
		if(unserialize_even_only == UNSERIALIZE_ONLY_VALUES && numElems % 2 == 0)
			can_unserialize = 0;

		if(can_unserialize && redis_unserialize(redis_sock, response, response_len, &z TSRMLS_CC) == 1) {
			efree(response);
			add_next_index_zval(z_tab, z);
		} else {
			add_next_index_stringl(z_tab, response, response_len, 0);
		}
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
	int i, numElems;
	zval *z_multi_result;

    zval **z_keys = ctx;

    if(-1 == redis_check_eof(redis_sock TSRMLS_CC)) {
        return -1;
    }
    if(php_stream_gets(redis_sock->stream, inbuf, 1024) == NULL) {
		redis_stream_close(redis_sock TSRMLS_CC);
        redis_sock->stream = NULL;
        redis_sock->status = REDIS_SOCK_STATUS_FAILED;
        redis_sock->mode = ATOMIC;
        redis_sock->watching = 0;
        zend_throw_exception(redis_exception_ce, "read error on connection", 0 TSRMLS_CC);
        return -1;
    }

    if(inbuf[0] != '*') {
        IF_MULTI_OR_PIPELINE() {
            add_next_index_bool(z_tab, 0);
        } else {
            RETVAL_FALSE;
        }
        return -1;
    }
    numElems = atoi(inbuf+1);
    MAKE_STD_ZVAL(z_multi_result);
    array_init(z_multi_result); /* pre-allocate array for multi's results. */

    for(i = 0; i < numElems; ++i) {
        response = redis_sock_read(redis_sock, &response_len TSRMLS_CC);
        if(response != NULL) {
			zval *z = NULL;
			if(redis_unserialize(redis_sock, response, response_len, &z TSRMLS_CC) == 1) {
				efree(response);
				add_assoc_zval_ex(z_multi_result, Z_STRVAL_P(z_keys[i]), 1+Z_STRLEN_P(z_keys[i]), z);
			} else {
				add_assoc_stringl_ex(z_multi_result, Z_STRVAL_P(z_keys[i]), 1+Z_STRLEN_P(z_keys[i]), response, response_len, 0);
			}
		} else {
			add_assoc_bool_ex(z_multi_result, Z_STRVAL_P(z_keys[i]), 1+Z_STRLEN_P(z_keys[i]), 0);
		}
	zval_dtor(z_keys[i]);
	efree(z_keys[i]);
    }
    efree(z_keys);

    IF_MULTI_OR_PIPELINE() {
        add_next_index_zval(z_tab, z_multi_result);
    } else {
		*return_value = *z_multi_result;
		zval_copy_ctor(return_value);
		INIT_PZVAL(return_value);
		zval_dtor(z_multi_result);
		efree(z_multi_result);
	}
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
    if(redis_sock->prefix) {
		efree(redis_sock->prefix);
	}
    if(redis_sock->err) {
    	efree(redis_sock->err);
    }
    if(redis_sock->auth) {
        efree(redis_sock->auth);
    }
    if(redis_sock->persistent_id) {
        efree(redis_sock->persistent_id);
    }
    efree(redis_sock->host);
    efree(redis_sock);
}

PHPAPI int
redis_serialize(RedisSock *redis_sock, zval *z, char **val, int *val_len TSRMLS_DC) {
#if ZEND_MODULE_API_NO >= 20100000
	php_serialize_data_t ht;
#else
	HashTable ht;
#endif
	smart_str sstr = {0};
	zval *z_copy;
	size_t sz;
	uint8_t *val8;

	switch(redis_sock->serializer) {
		case REDIS_SERIALIZER_NONE:
			switch(Z_TYPE_P(z)) {

				case IS_STRING:
					*val = Z_STRVAL_P(z);
					*val_len = Z_STRLEN_P(z);
					return 0;

				case IS_OBJECT:
					MAKE_STD_ZVAL(z_copy);
					ZVAL_STRINGL(z_copy, "Object", 6, 1);
					break;

				case IS_ARRAY:
					MAKE_STD_ZVAL(z_copy);
					ZVAL_STRINGL(z_copy, "Array", 5, 1);
					break;

				default: /* copy */
					MAKE_STD_ZVAL(z_copy);
					*z_copy = *z;
					zval_copy_ctor(z_copy);
					break;
			}

			/* return string */
			convert_to_string(z_copy);
			*val = Z_STRVAL_P(z_copy);
			*val_len = Z_STRLEN_P(z_copy);
			efree(z_copy);
			return 1;

		case REDIS_SERIALIZER_PHP:

#if ZEND_MODULE_API_NO >= 20100000
			PHP_VAR_SERIALIZE_INIT(ht);
#else
			zend_hash_init(&ht, 10, NULL, NULL, 0);
#endif
			php_var_serialize(&sstr, &z, &ht TSRMLS_CC);
			*val = sstr.c;
			*val_len = (int)sstr.len;
#if ZEND_MODULE_API_NO >= 20100000
			PHP_VAR_SERIALIZE_DESTROY(ht);
#else
			zend_hash_destroy(&ht);
#endif

			return 1;

		case REDIS_SERIALIZER_IGBINARY:
#ifdef HAVE_REDIS_IGBINARY
			if(igbinary_serialize(&val8, (size_t *)&sz, z TSRMLS_CC) == 0) { /* ok */
				*val = (char*)val8;
				*val_len = (int)sz;
				return 1;
			}
#endif
			return 0;
	}
	return 0;
}

PHPAPI int
redis_unserialize(RedisSock *redis_sock, const char *val, int val_len, zval **return_value TSRMLS_DC) {

	php_unserialize_data_t var_hash;
	int ret, rv_free = 0;

	switch(redis_sock->serializer) {
		case REDIS_SERIALIZER_NONE:
			return 0;

		case REDIS_SERIALIZER_PHP:
			if(!*return_value) {
				MAKE_STD_ZVAL(*return_value);
				rv_free = 1;
			}
#if ZEND_MODULE_API_NO >= 20100000
			PHP_VAR_UNSERIALIZE_INIT(var_hash);
#else
			memset(&var_hash, 0, sizeof(var_hash));
#endif
			if(!php_var_unserialize(return_value, (const unsigned char**)&val,
					(const unsigned char*)val + val_len, &var_hash TSRMLS_CC)) {
				if(rv_free==1) efree(*return_value);
				ret = 0;
			} else {
				ret = 1;
			}
#if ZEND_MODULE_API_NO >= 20100000
			PHP_VAR_UNSERIALIZE_DESTROY(var_hash);
#else
			var_destroy(&var_hash);
#endif

			return ret;

		case REDIS_SERIALIZER_IGBINARY:
#ifdef HAVE_REDIS_IGBINARY
			if(!*return_value) {
				MAKE_STD_ZVAL(*return_value);
			}
			if(igbinary_unserialize((const uint8_t *)val, (size_t)val_len, return_value TSRMLS_CC) == 0) {
				return 1;
			}
			efree(*return_value);
#endif
			return 0;
			break;
	}
	return 0;
}

PHPAPI int
redis_key_prefix(RedisSock *redis_sock, char **key, int *key_len TSRMLS_DC) {
	int ret_len;
	char *ret;
	
	if(redis_sock->prefix == NULL || redis_sock->prefix_len == 0) {
		return 0;
	}

	ret_len = redis_sock->prefix_len + *key_len;
	ret = ecalloc(1 + ret_len, 1);
	memcpy(ret, redis_sock->prefix, redis_sock->prefix_len);
	memcpy(ret + redis_sock->prefix_len, *key, *key_len);

	*key = ret;
	*key_len = ret_len;
	return 1;
}

/*
 * Processing for variant reply types (think EVAL)
 */

PHPAPI int
redis_sock_gets(RedisSock *redis_sock, char *buf, int buf_size, size_t *line_size TSRMLS_DC) {
    // Handle EOF
	if(-1 == redis_check_eof(redis_sock TSRMLS_CC)) {
        return -1;
    }

	if(php_stream_get_line(redis_sock->stream, buf, buf_size, line_size) == NULL) {
		// Close, put our socket state into error
		redis_stream_close(redis_sock TSRMLS_CC);
		redis_sock->stream = NULL;
		redis_sock->status = REDIS_SOCK_STATUS_FAILED;
		redis_sock->mode = ATOMIC;
		redis_sock->watching = 0;

		// Throw a read error exception
		zend_throw_exception(redis_exception_ce, "read error on connection", 0 TSRMLS_CC);
	}

	// We don't need \r\n
	*line_size-=2;
	buf[*line_size]='\0';


	// Success!
	return 0;
}

PHPAPI int
redis_read_reply_type(RedisSock *redis_sock, REDIS_REPLY_TYPE *reply_type, int *reply_info TSRMLS_DC) {
	// Make sure we haven't lost the connection, even trying to reconnect
	if(-1 == redis_check_eof(redis_sock TSRMLS_CC)) {
		// Failure
		return -1;
	}

	// Attempt to read the reply-type byte
	if((*reply_type = php_stream_getc(redis_sock->stream)) == EOF) {
		zend_throw_exception(redis_exception_ce, "socket error on read socket", 0 TSRMLS_CC);
	}

	// If this is a BULK, MULTI BULK, or simply an INTEGER response, we can extract the value or size info here
	if(*reply_type == TYPE_INT || *reply_type == TYPE_BULK || *reply_type == TYPE_MULTIBULK) {
		// Buffer to hold size information
		char inbuf[255];

		// Read up to our newline
		if(php_stream_gets(redis_sock->stream, inbuf, sizeof(inbuf)) < 0) {
			return -1;
		}

		// Set our size response
		*reply_info = atoi(inbuf);
	}

	// Success!
	return 0;
}

/*
 * Read a single line response, having already consumed the reply-type byte
 */
PHPAPI int
redis_read_variant_line(RedisSock *redis_sock, REDIS_REPLY_TYPE reply_type, zval **z_ret TSRMLS_DC) {
	// Buffer to read our single line reply
	char inbuf[1024];
	size_t line_size;

	// Attempt to read our single line reply
	if(redis_sock_gets(redis_sock, inbuf, sizeof(inbuf), &line_size TSRMLS_CC) < 0) {
		return -1;
	}

	// If this is an error response, check if it is a SYNC error, and throw in that case
	if(reply_type == TYPE_ERR) {
		if(memcmp(inbuf, "ERR SYNC", 9) == 0) {
			zend_throw_exception(redis_exception_ce, "SYNC with master in progress", 0 TSRMLS_CC);
		}

		// Set our last error
		redis_sock_set_err(redis_sock, inbuf, line_size);

		// Set our response to FALSE
		ZVAL_FALSE(*z_ret);
	} else {
		// Set our response to TRUE
		ZVAL_TRUE(*z_ret);
	}

	return 0;
}

PHPAPI int
redis_read_variant_bulk(RedisSock *redis_sock, int size, zval **z_ret TSRMLS_DC) {
	// Attempt to read the bulk reply
	char *bulk_resp = redis_sock_read_bulk_reply(redis_sock, size TSRMLS_CC);

	// Set our reply to FALSE on failure, and the string on success
	if(bulk_resp == NULL) {
		ZVAL_FALSE(*z_ret);
		return -1;
	} else {
		ZVAL_STRINGL(*z_ret, bulk_resp, size, 0);
		return 0;
	}
}

PHPAPI int
redis_read_multibulk_recursive(RedisSock *redis_sock, int elements, zval **z_ret TSRMLS_DC) {
	int reply_info;
	REDIS_REPLY_TYPE reply_type;
	zval *z_subelem;

	// Iterate while we have elements
	while(elements > 0) {
		// Attempt to read our reply type
		if(redis_read_reply_type(redis_sock, &reply_type, &reply_info TSRMLS_CC) < 0) {
			zend_throw_exception_ex(redis_exception_ce, 0 TSRMLS_CC, "protocol error, couldn't parse MULTI-BULK response\n", reply_type);
			return -1;
		}

		// Switch on our reply-type byte
		switch(reply_type) {
			case TYPE_ERR:
			case TYPE_LINE:
				ALLOC_INIT_ZVAL(z_subelem);
				redis_read_variant_line(redis_sock, reply_type, &z_subelem TSRMLS_CC);
				add_next_index_zval(*z_ret, z_subelem);
				break;
			case TYPE_INT:
				// Add our long value
				add_next_index_long(*z_ret, reply_info);
				break;
			case TYPE_BULK:
				// Init a zval for our bulk response, read and add it
				ALLOC_INIT_ZVAL(z_subelem);
				redis_read_variant_bulk(redis_sock, reply_info, &z_subelem TSRMLS_CC);
				add_next_index_zval(*z_ret, z_subelem);
				break;
			case TYPE_MULTIBULK:
				// Construct an array for our sub element, and add it, and recurse
				ALLOC_INIT_ZVAL(z_subelem);
				array_init(z_subelem);
				add_next_index_zval(*z_ret, z_subelem);
				redis_read_multibulk_recursive(redis_sock, reply_info, &z_subelem TSRMLS_CC);
				break;
		}

		// Decrement our element counter
		elements--;
	}

	return 0;
}

PHPAPI int
redis_read_variant_reply(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock, zval *z_tab) {
	// Reply type, and reply size vars
	REDIS_REPLY_TYPE reply_type;
	int reply_info;
	//char *bulk_resp;
	zval *z_ret;

	// Attempt to read our header
	if(redis_read_reply_type(redis_sock, &reply_type, &reply_info TSRMLS_CC) < 0) {
		return -1;
	}

	// Our return ZVAL
	MAKE_STD_ZVAL(z_ret);

	// Switch based on our top level reply type
	switch(reply_type) {
		case TYPE_ERR:
		case TYPE_LINE:
			redis_read_variant_line(redis_sock, reply_type, &z_ret TSRMLS_CC);
			break;
		case TYPE_INT:
			ZVAL_LONG(z_ret, reply_info);
			break;
		case TYPE_BULK:
			redis_read_variant_bulk(redis_sock, reply_info, &z_ret TSRMLS_CC);
			break;
		case TYPE_MULTIBULK:
			// Initialize an array for our multi-bulk response
			array_init(z_ret);

			// If we've got more than zero elements, parse our multi bulk respoinse recursively
			if(reply_info > -1) {
				redis_read_multibulk_recursive(redis_sock, reply_info, &z_ret TSRMLS_CC);
			}
			break;
		default:
			// Protocol error
			zend_throw_exception_ex(redis_exception_ce, 0 TSRMLS_CC, "protocol error, got '%c' as reply-type byte\n", reply_type);
            return FAILURE;
	}

	IF_MULTI_OR_PIPELINE() {
		add_next_index_zval(z_tab, z_ret);
	} else {
		// Set our return value
		*return_value = *z_ret;
	    zval_copy_ctor(return_value);
	    zval_dtor(z_ret);
		efree(z_ret);
	}

	// Success
	return 0;
}

/* vim: set tabstop=4 softtabstop=4 noexpandtab shiftwidth=4: */

