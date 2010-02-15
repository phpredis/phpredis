/* -*- Mode: C; tab-width: 4 -*- */
/*
  +----------------------------------------------------------------------+
  | PHP Version 5                                                        |
  +----------------------------------------------------------------------+
  | Copyright (c) 1997-2009 The PHP Group                                |
  +----------------------------------------------------------------------+
  | This source file is subject to version 3.01 of the PHP license,      |
  | that is bundled with this package in the file LICENSE, and is        |
  | available through the world-wide-web at the following url:           |
  | http://www.php.net/license/3_01.txt                                  |
  | If you did not receive a copy of the PHP license and are unable to   |
  | obtain it through the world-wide-web, please send a note to          |
  | license@php.net so we can mail you a copy immediately.               |
  +----------------------------------------------------------------------+
  | Original author: Alfonso Jimenez <yo@alfonsojimenez.com>             |
  | Maintainer: Nicolas Favre-Felix <n.favre-felix@owlient.eu>           |
  | Maintainer: Nasreddine Bouafif <n.bouafif@owlient.eu>                |
  +----------------------------------------------------------------------+
*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "php.h"
#include "php_ini.h"
#include "ext/standard/info.h"
#include "php_redis.h"
#include <zend_exceptions.h>

static int le_redis_sock;
static zend_class_entry *redis_ce;
static zend_class_entry *redis_exception_ce;
static zend_class_entry *spl_ce_RuntimeException = NULL;

ZEND_DECLARE_MODULE_GLOBALS(redis)

static zend_function_entry redis_functions[] = {
     PHP_ME(Redis, __construct, NULL, ZEND_ACC_PUBLIC)
     PHP_ME(Redis, connect, NULL, ZEND_ACC_PUBLIC)
     PHP_ME(Redis, close, NULL, ZEND_ACC_PUBLIC)
     PHP_ME(Redis, ping, NULL, ZEND_ACC_PUBLIC)
     PHP_ME(Redis, get, NULL, ZEND_ACC_PUBLIC)
     PHP_ME(Redis, set, NULL, ZEND_ACC_PUBLIC)
     PHP_ME(Redis, setnx, NULL, ZEND_ACC_PUBLIC)
     PHP_ME(Redis, getSet, NULL, ZEND_ACC_PUBLIC)
     PHP_ME(Redis, randomKey, NULL, ZEND_ACC_PUBLIC)
     PHP_ME(Redis, renameKey, NULL, ZEND_ACC_PUBLIC)
     PHP_ME(Redis, renameNx, NULL, ZEND_ACC_PUBLIC)
     PHP_ME(Redis, add, NULL, ZEND_ACC_PUBLIC)
     PHP_ME(Redis, getMultiple, NULL, ZEND_ACC_PUBLIC)
     PHP_ME(Redis, exists, NULL, ZEND_ACC_PUBLIC)
     PHP_ME(Redis, delete, NULL, ZEND_ACC_PUBLIC)
     PHP_ME(Redis, incr, NULL, ZEND_ACC_PUBLIC)
     PHP_ME(Redis, decr, NULL, ZEND_ACC_PUBLIC)
     PHP_ME(Redis, type, NULL, ZEND_ACC_PUBLIC)
     PHP_ME(Redis, getKeys, NULL, ZEND_ACC_PUBLIC)
     PHP_ME(Redis, sortAsc, NULL, ZEND_ACC_PUBLIC)
     PHP_ME(Redis, sortAscAlpha, NULL, ZEND_ACC_PUBLIC)
     PHP_ME(Redis, sortDesc, NULL, ZEND_ACC_PUBLIC)
     PHP_ME(Redis, sortDescAlpha, NULL, ZEND_ACC_PUBLIC)
     PHP_ME(Redis, lPush, NULL, ZEND_ACC_PUBLIC)
     PHP_ME(Redis, rPush, NULL, ZEND_ACC_PUBLIC)
     PHP_ME(Redis, lPop, NULL, ZEND_ACC_PUBLIC)
     PHP_ME(Redis, rPop, NULL, ZEND_ACC_PUBLIC)
     PHP_ME(Redis, lSize, NULL, ZEND_ACC_PUBLIC)
     PHP_ME(Redis, lRemove, NULL, ZEND_ACC_PUBLIC)
     PHP_ME(Redis, listTrim, NULL, ZEND_ACC_PUBLIC)
     PHP_ME(Redis, lGet, NULL, ZEND_ACC_PUBLIC)
     PHP_ME(Redis, lGetRange, NULL, ZEND_ACC_PUBLIC)
     PHP_ME(Redis, lSet, NULL, ZEND_ACC_PUBLIC)
     PHP_ME(Redis, sAdd, NULL, ZEND_ACC_PUBLIC)
     PHP_ME(Redis, sSize, NULL, ZEND_ACC_PUBLIC)
     PHP_ME(Redis, sRemove, NULL, ZEND_ACC_PUBLIC)
     PHP_ME(Redis, sMove, NULL, ZEND_ACC_PUBLIC)
     PHP_ME(Redis, sPop, NULL, ZEND_ACC_PUBLIC)
     PHP_ME(Redis, sContains, NULL, ZEND_ACC_PUBLIC)
     PHP_ME(Redis, sGetMembers, NULL, ZEND_ACC_PUBLIC)
     PHP_ME(Redis, sInter, NULL, ZEND_ACC_PUBLIC)
     PHP_ME(Redis, sInterStore, NULL, ZEND_ACC_PUBLIC)
     PHP_ME(Redis, sUnion, NULL, ZEND_ACC_PUBLIC)
     PHP_ME(Redis, sUnionStore, NULL, ZEND_ACC_PUBLIC)
     PHP_ME(Redis, sDiff, NULL, ZEND_ACC_PUBLIC)
     PHP_ME(Redis, sDiffStore, NULL, ZEND_ACC_PUBLIC)
     PHP_ME(Redis, setTimeout, NULL, ZEND_ACC_PUBLIC)
     PHP_ME(Redis, save, NULL, ZEND_ACC_PUBLIC)
     PHP_ME(Redis, bgSave, NULL, ZEND_ACC_PUBLIC)
     PHP_ME(Redis, lastSave, NULL, ZEND_ACC_PUBLIC)
     PHP_ME(Redis, flushDB, NULL, ZEND_ACC_PUBLIC)
     PHP_ME(Redis, flushAll, NULL, ZEND_ACC_PUBLIC)
     PHP_ME(Redis, dbSize, NULL, ZEND_ACC_PUBLIC)
     PHP_ME(Redis, auth, NULL, ZEND_ACC_PUBLIC)
     PHP_ME(Redis, ttl, NULL, ZEND_ACC_PUBLIC)
     PHP_ME(Redis, info, NULL, ZEND_ACC_PUBLIC)
     PHP_ME(Redis, select, NULL, ZEND_ACC_PUBLIC)
     PHP_ME(Redis, move, NULL, ZEND_ACC_PUBLIC)

     /* 1.1 */
     PHP_ME(Redis, mset, NULL, ZEND_ACC_PUBLIC)
     PHP_ME(Redis, rpoplpush, NULL, ZEND_ACC_PUBLIC)
     PHP_ME(Redis, zAdd, NULL, ZEND_ACC_PUBLIC)
     PHP_ME(Redis, zRemove, NULL, ZEND_ACC_PUBLIC)
     PHP_ME(Redis, zRange, NULL, ZEND_ACC_PUBLIC)
     PHP_ME(Redis, zReverseRange, NULL, ZEND_ACC_PUBLIC)
     PHP_ME(Redis, zRangeByScore, NULL, ZEND_ACC_PUBLIC)
     PHP_ME(Redis, zDeleteRangeByScore, NULL, ZEND_ACC_PUBLIC)
     PHP_ME(Redis, zCard, NULL, ZEND_ACC_PUBLIC)
     PHP_ME(Redis, zScore, NULL, ZEND_ACC_PUBLIC)
    /* PHP_ME(Redis, zIncrBy, NULL, ZEND_ACC_PUBLIC) */

     /* aliases */
     PHP_MALIAS(Redis, open, connect, NULL, ZEND_ACC_PUBLIC)
     PHP_MALIAS(Redis, lLen, lSize, NULL, ZEND_ACC_PUBLIC)
     PHP_MALIAS(Redis, sMembers, sGetMembers, NULL, ZEND_ACC_PUBLIC)
     PHP_MALIAS(Redis, mget, getMultiple, NULL, ZEND_ACC_PUBLIC)
     PHP_MALIAS(Redis, expire, setTimeout, NULL, ZEND_ACC_PUBLIC)

     PHP_MALIAS(Redis, zDelete, zRemove, NULL, ZEND_ACC_PUBLIC)
     PHP_MALIAS(Redis, zSize, zCard, NULL, ZEND_ACC_PUBLIC)
     {NULL, NULL, NULL}
};

zend_module_entry redis_module_entry = {
#if ZEND_MODULE_API_NO >= 20010901
     STANDARD_MODULE_HEADER,
#endif
     "redis",
     NULL,
     PHP_MINIT(redis),
     PHP_MSHUTDOWN(redis),
     PHP_RINIT(redis),
     PHP_RSHUTDOWN(redis),
     PHP_MINFO(redis),
#if ZEND_MODULE_API_NO >= 20010901
     PHP_REDIS_VERSION,
#endif
     STANDARD_MODULE_PROPERTIES
};

#ifdef COMPILE_DL_REDIS
ZEND_GET_MODULE(redis)
#endif

void
add_constant_long(zend_class_entry *ce, char *name, int value) {

	zval *constval;
	constval = pemalloc(sizeof(zval), 1);
	INIT_PZVAL(constval);
	ZVAL_LONG(constval, value);
	zend_hash_add(&ce->constants_table, name, 1 + strlen(name),
		(void*)&constval, sizeof(zval*), NULL);
}

/**
 * This command behave somehow like printf, except that strings need 2 arguments:
 * Their data and their size (strlen).
 * Supported formats are: %d, %i, %s
 */
static int
redis_cmd_format(char **ret, char *format, ...) {

    char *p, *s;
    va_list ap;

    int total = 0, sz, ret_sz;
    int i, ci;
    unsigned int u;


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


/**
 * redis_sock_create
 */
PHPAPI RedisSock* redis_sock_create(char *host, int host_len, unsigned short port,
                                                                       long timeout)
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

    tv.tv_sec  = redis_sock->timeout;
    tv.tv_usec = 0;

    host_len = spprintf(&host, 0, "%s:%d", redis_sock->host, redis_sock->port);

    if(tv.tv_sec != 0) {
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
    int res = 0;

    if (redis_sock->stream != NULL) {
        redis_sock_write(redis_sock, "QUIT", sizeof("QUIT") - 1);

        redis_sock->status = REDIS_SOCK_STATUS_DISCONNECTED;
        php_stream_close(redis_sock->stream);
        redis_sock->stream = NULL;

        res = 1;
    }

    return res;
}

/**
 * redis_sock_read
 */
PHPAPI char *redis_sock_read(RedisSock *redis_sock, int *buf_len TSRMLS_DC)
{

    char inbuf[1024];
    char *resp = NULL;

    redis_check_eof(redis_sock TSRMLS_CC);
    php_stream_gets(redis_sock->stream, inbuf, 1024);

    switch(inbuf[0]) {

        case '-':
            return NULL;

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
            } else {
                printf("protocol error \n");
                return NULL;
            }

        case '$':
            *buf_len = atoi(inbuf + 1);
            resp = redis_sock_read_bulk_reply(redis_sock, *buf_len);
            return resp;

        default:
            printf("protocol error, got '%c' as reply type byte\n", inbuf[0]);
    }

    return NULL;
}

/**
 * redis_sock_read_bulk_reply
 */
PHPAPI char *redis_sock_read_bulk_reply(RedisSock *redis_sock, int bytes)
{

    int offset = 0;
    size_t got;

    char * reply;

    redis_check_eof(redis_sock TSRMLS_CC);

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
 * redis_sock_read_multibulk_reply
 */
PHPAPI int redis_sock_read_multibulk_reply(INTERNAL_FUNCTION_PARAMETERS,
                                      RedisSock *redis_sock TSRMLS_DC)
{
    char inbuf[1024], *response;
    int response_len;

    redis_check_eof(redis_sock TSRMLS_CC);
    php_stream_gets(redis_sock->stream, inbuf, 1024);

    if(inbuf[0] != '*') {
        return -1;
    }
    int numElems = atoi(inbuf+1);

    array_init(return_value);

    while(numElems > 0) {
        int response_len;
        response = redis_sock_read(redis_sock, &response_len TSRMLS_CC);

        if(response != NULL) {
            add_next_index_stringl(return_value, response, response_len, 0);
        } else {
            add_next_index_bool(return_value, 0);
        }
        numElems --;
    }
    return 0;
}

/**
 * redis_sock_write
 */
PHPAPI int redis_sock_write(RedisSock *redis_sock, char *cmd, size_t sz)
{
    redis_check_eof(redis_sock TSRMLS_CC);
    return php_stream_write(redis_sock->stream, cmd, sz);

    return 0;
}


PHPAPI void redis_check_eof(RedisSock *redis_sock TSRMLS_DC) {
    int eof = php_stream_eof(redis_sock->stream);
    while(eof) {
        redis_sock->stream = NULL;
        redis_sock_connect(redis_sock TSRMLS_CC);
        eof = php_stream_eof(redis_sock->stream);
    }
}

/**
 * redis_sock_get
 */
PHPAPI int redis_sock_get(zval *id, RedisSock **redis_sock TSRMLS_DC)
{

    zval **socket;
    int resource_type;

    if (Z_TYPE_P(id) != IS_OBJECT || zend_hash_find(Z_OBJPROP_P(id), "socket",
                                  sizeof("socket"), (void **) &socket) == FAILURE) {
        return -1;
    }

    *redis_sock = (RedisSock *) zend_list_find(Z_LVAL_PP(socket), &resource_type);

    if (!*redis_sock || resource_type != le_redis_sock) {
            return -1;
    }

    return Z_LVAL_PP(socket);
}

/**
 * redis_free_socket
 */
PHPAPI void redis_free_socket(RedisSock *redis_sock)
{
    efree(redis_sock->host);
    efree(redis_sock);
}

PHPAPI zend_class_entry *redis_get_exception_base(int root TSRMLS_DC)
{
#if HAVE_SPL
        if (!root) {
                if (!spl_ce_RuntimeException) {
                        zend_class_entry **pce;
        
                        if (zend_hash_find(CG(class_table), "runtimeexception",
                                                           sizeof("RuntimeException"), (void **) &pce) == SUCCESS) {
                                spl_ce_RuntimeException = *pce;
                                return *pce;
                        }
                } else {
                        return spl_ce_RuntimeException;
                }
        }
#endif
#if (PHP_MAJOR_VERSION == 5) && (PHP_MINOR_VERSION < 2)
        return zend_exception_get_default();
#else
        return zend_exception_get_default(TSRMLS_C);
#endif
}

/**
 * redis_destructor_redis_sock
 */
static void redis_destructor_redis_sock(zend_rsrc_list_entry * rsrc TSRMLS_DC)
{
    RedisSock *redis_sock = (RedisSock *) rsrc->ptr;
    redis_sock_disconnect(redis_sock TSRMLS_CC);
    redis_free_socket(redis_sock);
}

/**
 * PHP_MINIT_FUNCTION
 */
PHP_MINIT_FUNCTION(redis)
{
    zend_class_entry redis_class_entry;
    INIT_CLASS_ENTRY(redis_class_entry, "Redis", redis_functions);
    redis_ce = zend_register_internal_class(&redis_class_entry TSRMLS_CC);

    zend_class_entry redis_exception_class_entry;
    INIT_CLASS_ENTRY(redis_exception_class_entry, "RedisException", NULL);
    redis_exception_ce = zend_register_internal_class_ex(
        &redis_exception_class_entry,
        redis_get_exception_base(0 TSRMLS_CC),
        NULL TSRMLS_CC
    );

    le_redis_sock = zend_register_list_destructors_ex(
        redis_destructor_redis_sock,
        NULL,
        redis_sock_name, module_number
    );

	add_constant_long(redis_ce, "REDIS_NOT_FOUND", REDIS_NOT_FOUND);
	add_constant_long(redis_ce, "REDIS_STRING", REDIS_STRING);
	add_constant_long(redis_ce, "REDIS_SET", REDIS_SET);
	add_constant_long(redis_ce, "REDIS_LIST", REDIS_LIST);

    return SUCCESS;
}

/**
 * PHP_MSHUTDOWN_FUNCTION
 */
PHP_MSHUTDOWN_FUNCTION(redis)
{
    return SUCCESS;
}

/**
 * PHP_RINIT_FUNCTION
 */
PHP_RINIT_FUNCTION(redis)
{
    return SUCCESS;
}

/**
 * PHP_RSHUTDOWN_FUNCTION
 */
PHP_RSHUTDOWN_FUNCTION(redis)
{
    return SUCCESS;
}

/**
 * PHP_MINFO_FUNCTION
 */
PHP_MINFO_FUNCTION(redis)
{
    php_info_print_table_start();
    php_info_print_table_header(2, "Redis Support", "enabled");
    php_info_print_table_row(2, "Version", PHP_REDIS_VERSION);
    php_info_print_table_end();
}

/* {{{ proto Redis Redis::__construct()
    Public constructor */
PHP_METHOD(Redis, __construct)
{
    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "") == FAILURE) {
        RETURN_FALSE;
    }
}
/* }}} */

/* {{{ proto boolean Redis::connect(string host, int port [, int timeout])
 */
PHP_METHOD(Redis, connect)
{
    zval *object;
    int host_len, id;
    char *host = NULL;
    long port;

    struct timeval timeout = {0L, 0L};
    RedisSock *redis_sock  = NULL;

    if (zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), "Osl|l",
                                     &object, redis_ce, &host, &host_len, &port,
                                     &timeout.tv_sec) == FAILURE) {
       RETURN_FALSE;
    }

    if (timeout.tv_sec < 0L || timeout.tv_sec > INT_MAX) {
        zend_throw_exception(redis_exception_ce, "Invalid timeout", 0 TSRMLS_CC);
        RETURN_FALSE;
    }

    redis_sock = redis_sock_create(host, host_len, port, timeout.tv_sec);

    if (redis_sock_server_open(redis_sock, 1 TSRMLS_CC) < 0) {
        redis_free_socket(redis_sock);
        zend_throw_exception_ex(
            redis_exception_ce,
            0 TSRMLS_CC,
            "Can't connect to %s:%d",
            host,
            port
        );
        RETURN_FALSE;
    }

    id = zend_list_insert(redis_sock, le_redis_sock);
    add_property_resource(object, "socket", id);

    RETURN_TRUE;
}
/* }}} */

/* {{{ proto boolean Redis::close()
 */
PHP_METHOD(Redis, close)
{
    zval *object;
    RedisSock *redis_sock = NULL;

    if (zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), "O",
        &object, redis_ce) == FAILURE) {
        RETURN_FALSE;
    }

    if (redis_sock_get(object, &redis_sock TSRMLS_CC) < 0) {
        RETURN_FALSE;
    }

    if (redis_sock_disconnect(redis_sock TSRMLS_CC)) {
        RETURN_TRUE;
    }

    RETURN_FALSE;
}
/* }}} */

PHPAPI void redis_boolean_response(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock TSRMLS_DC) {
    char *response;
    int response_len;
    char ret;

    if ((response = redis_sock_read(redis_sock, &response_len TSRMLS_CC)) == NULL) {
        RETURN_FALSE;
    }
    ret = response[0];
    efree(response);

    if (ret == '+') {
        RETURN_TRUE;
    } else {
        RETURN_FALSE;
    }
}

PHPAPI void redis_long_response(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock TSRMLS_DC) {
    char *response;
    int response_len;

    if ((response = redis_sock_read(redis_sock, &response_len TSRMLS_CC)) == NULL) {
        RETURN_FALSE;
    }

    if(response[0] == ':') {
        long ret = atol(response + 1);
        efree(response);
        RETURN_LONG(ret);
    } else {
        efree(response);
        RETURN_FALSE;
    }
}

PHPAPI void redis_bulk_double_response(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock TSRMLS_DC) {

    char *response;
    int response_len;    

    if ((response = redis_sock_read(redis_sock, &response_len TSRMLS_CC)) == NULL) {
        RETURN_FALSE;
    }

	long ret = atof(response);
	efree(response);
	RETURN_LONG(ret);
}
    
PHPAPI void redis_1_response(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock TSRMLS_DC) {

    char *response;
    int response_len;
    char ret;

    if ((response = redis_sock_read(redis_sock, &response_len TSRMLS_CC)) == NULL) {
        RETURN_FALSE;
    }

    ret = response[1];
    efree(response);

    if (ret == '1') {
        RETURN_TRUE;
    } else {
        RETURN_FALSE;
    }
}

/* {{{ proto boolean Redis::set(string key, string value)
 */
PHP_METHOD(Redis, set)
{
    zval *object;
    RedisSock *redis_sock;
    char *key = NULL, *val = NULL, *cmd;
    int key_len, val_len, cmd_len;

    if (zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), "Oss",
                                     &object, redis_ce, &key, &key_len,
                                     &val, &val_len) == FAILURE) {
        RETURN_FALSE;
    }

    if (redis_sock_get(object, &redis_sock TSRMLS_CC) < 0) {
        RETURN_FALSE;
    }

    cmd_len = redis_cmd_format(&cmd, "SET %s %d\r\n%s\r\n", key, key_len, val_len, val, val_len);

    if (redis_sock_write(redis_sock, cmd, cmd_len) < 0) {
        efree(cmd);
        RETURN_FALSE;
    }
    efree(cmd);
    redis_boolean_response(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock TSRMLS_CC);
}

/* {{{ proto boolean Redis::setnx(string key, string value)
 */
PHP_METHOD(Redis, setnx)
{

    zval *object;
    RedisSock *redis_sock;
    char *key = NULL, *val = NULL, *cmd;
    int key_len, val_len, cmd_len;

    if (zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), "Oss",
                                     &object, redis_ce, &key, &key_len,
                                     &val, &val_len) == FAILURE) {
        RETURN_FALSE;
    }

    if (redis_sock_get(object, &redis_sock TSRMLS_CC) < 0) {
        RETURN_FALSE;
    }

    cmd_len = redis_cmd_format(&cmd, "SETNX %s %d\r\n%s\r\n", key, key_len, val_len, val, val_len);

    if (redis_sock_write(redis_sock, cmd, cmd_len) < 0) {
        efree(cmd);
        RETURN_FALSE;
    }
    efree(cmd);
    redis_1_response(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock TSRMLS_CC);
}
/* }}} */
/* {{{ proto string Redis::getSet(string key, string value)
 */
PHP_METHOD(Redis, getSet)
{

    zval *object;
    RedisSock *redis_sock;
    char *key = NULL, *val = NULL, *cmd, *response;
    int key_len, val_len, cmd_len, response_len;

    if (zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), "Oss",
                                     &object, redis_ce, &key, &key_len,
                                     &val, &val_len) == FAILURE) {
        RETURN_FALSE;
    }

    if (redis_sock_get(object, &redis_sock TSRMLS_CC) < 0) {
        RETURN_FALSE;
    }

    cmd_len = redis_cmd_format(&cmd, "GETSET %s %d\r\n%s\r\n", key, key_len, val_len, val, val_len);

    if (redis_sock_write(redis_sock, cmd, cmd_len) < 0) {
        efree(cmd);
        RETURN_FALSE;
    }
    efree(cmd);
    if ((response = redis_sock_read(redis_sock, &response_len TSRMLS_CC)) == NULL) {
        RETURN_FALSE;
    }
    RETURN_STRINGL(response, response_len, 0);
}
/* }}} */

/* {{{ proto string Redis::randomKey()
 */
PHP_METHOD(Redis, randomKey)
{

    zval *object;
    RedisSock *redis_sock;
    char *cmd, *response, *ret;
    int cmd_len, response_len;

    if (zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), "O",
                                     &object, redis_ce) == FAILURE) {
        RETURN_FALSE;
    }

    if (redis_sock_get(object, &redis_sock TSRMLS_CC) < 0) {
        RETURN_FALSE;
    }

    cmd_len = redis_cmd_format(&cmd, "RANDOMKEY\r\n");

    if (redis_sock_write(redis_sock, cmd, cmd_len) < 0) {
        efree(cmd);
        RETURN_FALSE;
    }
    efree(cmd);
    if ((response = redis_sock_read(redis_sock, &response_len TSRMLS_CC)) == NULL) {
        RETURN_FALSE;
    }

    if(response[0] == '+') {
        ret = estrndup(response+1, response_len-1);
        efree(response);
        RETURN_STRINGL(ret, response_len-1, 0);  // need to remove the '+'
    } else {
        efree(response);
        RETURN_FALSE;
    }

}
/* }}} */

/* {{{ proto string Redis::renameKey(string key_src, string key_dst)
 */
PHP_METHOD(Redis, renameKey)
{

    zval *object;
    RedisSock *redis_sock;
    char *cmd, *src, *dst;
    int cmd_len, src_len, dst_len;

    if (zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), "Oss",
                                     &object, redis_ce,
                                     &src, &src_len,
                                     &dst, &dst_len) == FAILURE) {
        RETURN_FALSE;
    }

    if (redis_sock_get(object, &redis_sock TSRMLS_CC) < 0) {
        RETURN_FALSE;
    }

    cmd_len = redis_cmd_format(&cmd, "RENAME %s %s\r\n",
                               src, src_len,
                               dst, dst_len);

    if (redis_sock_write(redis_sock, cmd, cmd_len) < 0) {
        efree(cmd);
        RETURN_FALSE;
    }
    efree(cmd);
    redis_boolean_response(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock TSRMLS_CC);
}
/* }}} */

/* {{{ proto string Redis::renameNx(string key_src, string key_dst)
 */
PHP_METHOD(Redis, renameNx)
{

    zval *object;
    RedisSock *redis_sock;
    char *cmd, *src, *dst;
    int cmd_len, src_len, dst_len;

    if (zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), "Oss",
                                     &object, redis_ce,
                                     &src, &src_len,
                                     &dst, &dst_len) == FAILURE) {
        RETURN_FALSE;
    }

    if (redis_sock_get(object, &redis_sock TSRMLS_CC) < 0) {
        RETURN_FALSE;
    }

    cmd_len = redis_cmd_format(&cmd, "RENAMENX %s %s\r\n",
                               src, src_len,
                               dst, dst_len);

    if (redis_sock_write(redis_sock, cmd, cmd_len) < 0) {
        efree(cmd);
        RETURN_FALSE;
    }
    efree(cmd);
    redis_boolean_response(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock TSRMLS_CC);
}
/* }}} */

/* {{{ proto string Redis::get(string key)
 */
PHP_METHOD(Redis, get)
{
    zval *object;
    RedisSock *redis_sock;
    char *key = NULL, *cmd, *response;
    int key_len, cmd_len, response_len;

    if (zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), "Os",
                                     &object, redis_ce,
                                     &key, &key_len) == FAILURE) {
        RETURN_FALSE;
    }

    if (redis_sock_get(object, &redis_sock TSRMLS_CC) < 0) {
        RETURN_FALSE;
    }

    cmd_len = spprintf(&cmd, 0, "GET %s\r\n", key);

    if (redis_sock_write(redis_sock, cmd, cmd_len) < 0) {
        efree(cmd);
        RETURN_FALSE;
    }
    efree(cmd);

    if ((response = redis_sock_read(redis_sock, &response_len TSRMLS_CC)) == NULL) {
        RETURN_FALSE;
    }

    RETURN_STRINGL(response, response_len, 0);
}
/* }}} */

/* {{{ proto boolean Redis::add(string key, string value)
 */
PHP_METHOD(Redis, add)
{
    zval *object;
    RedisSock *redis_sock;
    char *key = NULL, *val = NULL, *cmd;
    int key_len, val_len, cmd_len;

    if (zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), "Oss",
                                     &object, redis_ce, &key, &key_len,
                                     &val, &val_len) == FAILURE) {
        RETURN_FALSE;
    }

    if (redis_sock_get(object, &redis_sock TSRMLS_CC) < 0) {
        RETURN_FALSE;
    }

    cmd_len = redis_cmd_format(&cmd, "SETNX %s %d\r\n%s\r\n",
                    key, key_len,
                    val_len,
                    val, val_len);

    if (redis_sock_write(redis_sock, cmd, cmd_len) < 0) {
        efree(cmd);
        RETURN_FALSE;
    }
    efree(cmd);
    redis_1_response(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock TSRMLS_CC);
}
/* }}} */

/* {{{ proto string Redis::ping()
 */
PHP_METHOD(Redis, ping)
{
    zval *object;
    RedisSock *redis_sock;
    char *cmd, *response;
    int cmd_len, response_len;

    if (zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), "O",
                                     &object, redis_ce) == FAILURE) {
        RETURN_FALSE;
    }

    if (redis_sock_get(object, &redis_sock TSRMLS_CC) < 0) {
        RETURN_FALSE;
    }

    if (redis_sock->stream) {
       char cmd[] = "PING\r\n";
       char buf[8];

       redis_sock_write(redis_sock, cmd, sizeof(cmd)-1);

       if((response = redis_sock_read(redis_sock, &response_len TSRMLS_CC)) == NULL) {
           RETURN_FALSE;
       }

       RETURN_STRINGL(response, response_len, 0);
    } else {
       php_error_docref(NULL TSRMLS_CC, E_ERROR, "The object is not connected");
       RETURN_FALSE;
    }
}
/* }}} */


PHPAPI void redis_atomic_increment(INTERNAL_FUNCTION_PARAMETERS, char *keyword TSRMLS_DC) {

    zval *object;
    RedisSock *redis_sock;
    char *key = NULL, *cmd;
    int key_len, cmd_len;
    long val = 1;

    if (zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), "Os|l",
                                     &object, redis_ce,
                                     &key, &key_len, &val) == FAILURE) {
        RETURN_FALSE;
    }

    if (redis_sock_get(object, &redis_sock TSRMLS_CC) < 0) {
        RETURN_FALSE;
    }
    if (val <= 1) {
        cmd_len = spprintf(&cmd, 0, "%s %s\r\n", keyword, key);
    } else {
        cmd_len = spprintf(&cmd, 0, "%sBY %s %d\r\n", keyword, key, (int)val);
    }

    if (redis_sock_write(redis_sock, cmd, cmd_len) < 0) {
        efree(cmd);
        RETURN_FALSE;
    }
    efree(cmd);
    redis_long_response(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock TSRMLS_CC);
}

/* {{{ proto boolean Redis::incr(string key [,int value])
 */
PHP_METHOD(Redis, incr)
{
    redis_atomic_increment(INTERNAL_FUNCTION_PARAM_PASSTHRU, "INCR" TSRMLS_CC);
}
/* }}} */

/* {{{ proto boolean Redis::decr(string key [,int value])
 */
PHP_METHOD(Redis, decr)
{
    redis_atomic_increment(INTERNAL_FUNCTION_PARAM_PASSTHRU, "DECR" TSRMLS_CC);
}
/* }}} */

/* {{{ proto array Redis::getMultiple(array keys)
 */
PHP_METHOD(Redis, getMultiple)
{
    zval *object, *array, **data;
    HashTable *arr_hash;
    HashPosition pointer;
    RedisSock *redis_sock;
    char *cmd = "", *old_cmd = NULL;
    int cmd_len, response_len, array_count;

    if (zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), "Oa",
                                     &object, redis_ce, &array) == FAILURE) {
        RETURN_FALSE;
    }

    if (redis_sock_get(object, &redis_sock TSRMLS_CC) < 0) {
        RETURN_FALSE;
    }

    arr_hash    = Z_ARRVAL_P(array);
    array_count = zend_hash_num_elements(arr_hash);

    if (array_count == 0) {
        RETURN_FALSE;
    }

    for (zend_hash_internal_pointer_reset_ex(arr_hash, &pointer);
         zend_hash_get_current_data_ex(arr_hash, (void**) &data,
                                       &pointer) == SUCCESS;
         zend_hash_move_forward_ex(arr_hash, &pointer)) {

        if (Z_TYPE_PP(data) == IS_STRING) {
            char *old_cmd = NULL;
            if(*cmd) {
                old_cmd = cmd;
            }
            cmd_len = spprintf(&cmd, 0, "%s %s", cmd, Z_STRVAL_PP(data));
            if(old_cmd) {
                efree(old_cmd);
            }
        }
    }

    old_cmd = cmd;
    cmd_len = spprintf(&cmd, 0, "MGET%s\r\n", cmd);
    efree(old_cmd);
    if (redis_sock_write(redis_sock, cmd, cmd_len) < 0) {
        efree(cmd);
        RETURN_FALSE;
    }
    efree(cmd);

    if (redis_sock_read_multibulk_reply(INTERNAL_FUNCTION_PARAM_PASSTHRU,
                                        redis_sock TSRMLS_CC) < 0) {
        RETURN_FALSE;
    }
}
/* }}} */

/* {{{ proto boolean Redis::exists(string key)
 */
PHP_METHOD(Redis, exists)
{
    zval *object;
    RedisSock *redis_sock;
    char *key = NULL, *cmd;
    int key_len, cmd_len;

    if (zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), "Os",
                                     &object, redis_ce,
                                     &key, &key_len) == FAILURE) {
        RETURN_FALSE;
    }

    if (redis_sock_get(object, &redis_sock TSRMLS_CC) < 0) {
        RETURN_FALSE;
    }

    cmd_len = spprintf(&cmd, 0, "EXISTS %s\r\n", key);

    if (redis_sock_write(redis_sock, cmd, cmd_len) < 0) {
        efree(cmd);
        RETURN_FALSE;
    }
    efree(cmd);
    redis_1_response(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock TSRMLS_CC);
}
/* }}} */

/* {{{ proto boolean Redis::delete(string key)
 */
PHP_METHOD(Redis, delete)
{
    RedisSock *redis_sock;

    generic_multiple_args_cmd(INTERNAL_FUNCTION_PARAM_PASSTHRU,
                    "DEL", sizeof("DEL") - 1,
                    1, &redis_sock TSRMLS_CC);
    redis_long_response(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock TSRMLS_CC);
}
/* }}} */

/* {{{ proto array Redis::getKeys(string pattern)
 */
PHP_METHOD(Redis, getKeys)
{
    zval *object;
    RedisSock *redis_sock;
    char *pattern = NULL, *cmd, *response;
    int pattern_len, cmd_len, response_len;

    if (zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), "Os",
                                     &object, redis_ce,
                                     &pattern, &pattern_len) == FAILURE) {
        RETURN_NULL();
    }

    if (redis_sock_get(object, &redis_sock TSRMLS_CC) < 0) {
        RETURN_FALSE;
    }

    cmd_len = spprintf(&cmd, 0, "KEYS %s\r\n", pattern);

    if (redis_sock_write(redis_sock, cmd, cmd_len) < 0) {
        efree(cmd);
        RETURN_FALSE;
    }
    efree(cmd);

    if ((response = redis_sock_read(redis_sock, &response_len TSRMLS_CC)) == NULL) {
        RETURN_FALSE;
    }

    /* prepare for php_explode */
    array_init(return_value);

    if(response_len) { /* empty array in case of an empty string */
        zval *delimiter; /* delimiter */
        MAKE_STD_ZVAL(delimiter);
        ZVAL_STRING(delimiter, " ", 1);

        zval *keys; /* keys */
        MAKE_STD_ZVAL(keys);
        ZVAL_STRING(keys, response, 1);
        php_explode(delimiter, keys, return_value, -1);

        /* free memory */
        zval_dtor(keys);
        efree(keys);
        zval_dtor(delimiter);
        efree(delimiter);
    }

    efree(response);
}
/* }}} */

/* {{{ proto int Redis::type(string key)
 */
PHP_METHOD(Redis, type)
{
    zval *object;
    RedisSock *redis_sock;
    char *key = NULL, *cmd, *response;
    int key_len, cmd_len, response_len;

    if (zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), "Os",
                                     &object, redis_ce,
                                     &key, &key_len) == FAILURE) {
        RETURN_NULL();
    }

    if (redis_sock_get(object, &redis_sock TSRMLS_CC) < 0) {
        RETURN_FALSE;
    }

    cmd_len = spprintf(&cmd, 0, "TYPE %s\r\n", key);

    if (redis_sock_write(redis_sock, cmd, cmd_len) < 0) {
        efree(cmd);
        RETURN_FALSE;
    }
    efree(cmd);

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
    } else {
       l = REDIS_NOT_FOUND;
    }
    efree(response);
    RETURN_LONG(l);
}
/* }}} */

/* {{{ proto boolean Redis::lPush(string key , string value)
 */
PHP_METHOD(Redis, lPush)
{
    zval *object;
    RedisSock *redis_sock;
    char *cmd, *key, *val;
    int cmd_len, key_len, val_len;

    if (zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), "Oss",
                                     &object, redis_ce,
                                     &key, &key_len, &val, &val_len) == FAILURE) {
        RETURN_NULL();
    }

    if (redis_sock_get(object, &redis_sock TSRMLS_CC) < 0) {
        RETURN_FALSE;
    }

    cmd_len = redis_cmd_format(&cmd, "LPUSH %s %d\r\n%s\r\n",
                               key, key_len,
                               val_len,
                               val, val_len);

    if (redis_sock_write(redis_sock, cmd, cmd_len) < 0) {
        efree(cmd);
        RETURN_FALSE;
    }
    efree(cmd);
    redis_boolean_response(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock TSRMLS_CC);
}

/* {{{ proto boolean Redis::rPush(string key , string value)
 */
PHP_METHOD(Redis, rPush)
{
    zval *object;
    RedisSock *redis_sock;
    char *cmd, *key, *val;
    int cmd_len, key_len, val_len;

    if (zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), "Oss",
                                     &object, redis_ce,
                                     &key, &key_len, &val, &val_len) == FAILURE) {
        RETURN_NULL();
    }

    if (redis_sock_get(object, &redis_sock TSRMLS_CC) < 0) {
        RETURN_FALSE;
    }

    cmd_len = redis_cmd_format(&cmd, "RPUSH %s %d\r\n%s\r\n",
                               key, key_len,
                               val_len,
                               val, val_len);

    if (redis_sock_write(redis_sock, cmd, cmd_len) < 0) {
        efree(cmd);
        RETURN_FALSE;
    }
    efree(cmd);
    redis_boolean_response(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock TSRMLS_CC);
}
/* }}} */

/* {{{ proto string Redis::lPOP(string key)
 */
PHP_METHOD(Redis, lPop)
{
    zval *object;
    RedisSock *redis_sock;
    char *key = NULL, *cmd, *response;
    int key_len, cmd_len, response_len;
    long type = 0;

    if (zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), "Os",
                                     &object, redis_ce,
                                     &key, &key_len) == FAILURE) {
        RETURN_FALSE;
    }

    if (redis_sock_get(object, &redis_sock TSRMLS_CC) < 0) {
        RETURN_FALSE;
    }

    cmd_len = spprintf(&cmd, 0, "LPOP %s\r\n", key);

    if (redis_sock_write(redis_sock, cmd, cmd_len) < 0) {
        efree(cmd);
        RETURN_FALSE;
    }
    efree(cmd);

    if ((response = redis_sock_read(redis_sock, &response_len TSRMLS_CC)) == NULL) {
        RETURN_FALSE;
    }

    RETURN_STRINGL(response, response_len, 0);
}

/* {{{ proto string Redis::rPOP(string key)
 */
PHP_METHOD(Redis, rPop)
{
    zval *object;
    RedisSock *redis_sock;
    char *key = NULL, *cmd, *response;
    int key_len, cmd_len, response_len;
    long type = 0;

    if (zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), "Os",
                                     &object, redis_ce,
                                     &key, &key_len) == FAILURE) {
        RETURN_FALSE;
    }

    if (redis_sock_get(object, &redis_sock TSRMLS_CC) < 0) {
        RETURN_FALSE;
    }

    cmd_len = spprintf(&cmd, 0, "RPOP %s\r\n", key);

    if (redis_sock_write(redis_sock, cmd, cmd_len) < 0) {
        efree(cmd);
        RETURN_FALSE;
    }
    efree(cmd);

    if ((response = redis_sock_read(redis_sock, &response_len TSRMLS_CC)) == NULL) {
        RETURN_FALSE;
    }

    RETURN_STRINGL(response, response_len, 0);
}
/* }}} */

/* {{{ proto int Redis::lSize(string key)
 */
PHP_METHOD(Redis, lSize)
{
    zval *object;
    RedisSock *redis_sock;
    char *key = NULL, *cmd;
    int key_len, cmd_len;

    if (zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), "Os",
                                     &object, redis_ce,
                                     &key, &key_len) == FAILURE) {
        RETURN_FALSE;
    }

    if (redis_sock_get(object, &redis_sock TSRMLS_CC) < 0) {
        RETURN_FALSE;
    }

    cmd_len = spprintf(&cmd, 0, "LLEN %s\r\n", key);

    if (redis_sock_write(redis_sock, cmd, cmd_len) < 0) {
        efree(cmd);
        RETURN_FALSE;
    }
    efree(cmd);
    redis_long_response(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock TSRMLS_CC);
}
/* }}} */

/* {{{ proto boolean Redis::lRemove(string list, string value, int count = 0)
 */
PHP_METHOD(Redis, lRemove)
{
    zval *object;
    RedisSock *redis_sock;
    char *cmd;
    int cmd_len, key_len, val_len;
    char *key, *val;
    long count = 0;

    if (zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), "Oss|l",
                                     &object, redis_ce,
                                     &key, &key_len, &val, &val_len, &count) == FAILURE) {
        RETURN_NULL();
    }

    if (redis_sock_get(object, &redis_sock TSRMLS_CC) < 0) {
        RETURN_FALSE;
    }

    cmd_len = redis_cmd_format(&cmd, "LREM %s %d %d\r\n%s\r\n",
                       key, key_len,
                       count, 
                       val_len,
                       val, val_len);

    if (redis_sock_write(redis_sock, cmd, cmd_len) < 0) {
        efree(cmd);
        RETURN_FALSE;
    }
    efree(cmd);
    redis_long_response(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock TSRMLS_CC);
}
/* }}} */

/* {{{ proto boolean Redis::listTrim(string key , int start , int end)
 */
PHP_METHOD(Redis, listTrim)
{
    zval *object;
    RedisSock *redis_sock;
    char *key = NULL, *val = NULL, *cmd;
    int key_len, cmd_len;
    long start, end;

    if (zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), "Osll",
                                     &object, redis_ce, &key, &key_len,
                                     &start, &end) == FAILURE) {
        RETURN_FALSE;
    }

    if (redis_sock_get(object, &redis_sock TSRMLS_CC) < 0) {
        RETURN_FALSE;
    }

    cmd_len = spprintf(&cmd, 0, "LTRIM %s %d %d\r\n", key, (int)start, (int)end);

    if (redis_sock_write(redis_sock, cmd, cmd_len) < 0) {
        efree(cmd);
        RETURN_FALSE;
    }
    efree(cmd);
    redis_boolean_response(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock TSRMLS_CC);
}
/* }}} */

/* {{{ proto string Redis::lGet(string key , int index)
 */
PHP_METHOD(Redis, lGet)
{
    zval *object;
    RedisSock *redis_sock;
    char *key = NULL, *cmd, *response;
    int key_len,cmd_len, response_len;
    long index;

    if (zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), "Osl",
                                     &object, redis_ce,
                                     &key, &key_len, &index) == FAILURE) {
        RETURN_NULL();
    }

    if (redis_sock_get(object, &redis_sock TSRMLS_CC) < 0) {
        RETURN_FALSE;
    }

    cmd_len = spprintf(&cmd, 0, "LINDEX %s %d\r\n", key, (int)index);

    if (redis_sock_write(redis_sock, cmd, cmd_len) < 0) {
        efree(cmd);
        RETURN_FALSE;
    }
    efree(cmd);

    if ((response = redis_sock_read(redis_sock, &response_len TSRMLS_CC)) == NULL) {
        RETURN_FALSE;
    }

    RETURN_STRINGL(response, response_len, 0);
}
/* }}} */

/* {{{ proto array Redis::lGetRange(string key, int start , int end)
 */
PHP_METHOD(Redis, lGetRange)
{
    zval *object;
    RedisSock *redis_sock;
    char *key = NULL, *cmd;
    int key_len, cmd_len, response_len;
    long start, end;

    if (zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), "Osll",
                                     &object, redis_ce,
                                     &key, &key_len, &start, &end) == FAILURE) {
        RETURN_FALSE;
    }

    if (redis_sock_get(object, &redis_sock TSRMLS_CC) < 0) {
        RETURN_FALSE;
    }

    cmd_len = spprintf(&cmd, 0, "LRANGE %s %d %d\r\n\r\n", key, (int)start, (int)end);

    if (redis_sock_write(redis_sock, cmd, cmd_len) < 0) {
        efree(cmd);
        RETURN_FALSE;
    }
    efree(cmd);

    if (redis_sock_read_multibulk_reply(INTERNAL_FUNCTION_PARAM_PASSTHRU,
                                        redis_sock TSRMLS_CC) < 0) {
        RETURN_FALSE;
    }
}
/* }}} */

/* {{{ proto boolean Redis::sAdd(string key , string value)
 */
PHP_METHOD(Redis, sAdd)
{
    zval *object;
    RedisSock *redis_sock;
    char *key = NULL, *val = NULL, *cmd;
    int key_len, val_len, cmd_len;

    if (zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), "Oss",
                                     &object, redis_ce, &key, &key_len,
                                     &val, &val_len) == FAILURE) {
        RETURN_NULL();
    }

    if (redis_sock_get(object, &redis_sock TSRMLS_CC) < 0) {
        RETURN_FALSE;
    }

    cmd_len = redis_cmd_format(&cmd, "SADD %s %d\r\n%s\r\n",
                    key, key_len,
                    val_len, 
                    val, val_len);

    if (redis_sock_write(redis_sock, cmd, cmd_len) < 0) {
        efree(cmd);
        RETURN_FALSE;
    }
    efree(cmd);
    redis_1_response(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock TSRMLS_CC);
}
/* }}} */

/* {{{ proto int Redis::sSize(string key)
 */
PHP_METHOD(Redis, sSize)
{
    zval *object;
    RedisSock *redis_sock;
    char *key = NULL, *cmd;
    int key_len, cmd_len, response_len;

    if (zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), "Os",
                                     &object, redis_ce,
                                     &key, &key_len) == FAILURE) {
        RETURN_FALSE;
    }

    if (redis_sock_get(object, &redis_sock TSRMLS_CC) < 0) {
        RETURN_FALSE;
    }

    cmd_len = spprintf(&cmd, 0, "SCARD %s\r\n", key);

    if (redis_sock_write(redis_sock, cmd, cmd_len) < 0) {
        efree(cmd);
        RETURN_FALSE;
    }
    efree(cmd);
    redis_long_response(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock TSRMLS_CC);
}
/* }}} */

/* {{{ proto boolean Redis::sRemove(string set, string value)
 */
PHP_METHOD(Redis, sRemove)
{
    zval *object;
    RedisSock *redis_sock;
    char *key = NULL, *val = NULL, *cmd;
    int key_len, val_len, cmd_len;

    if (zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), "Oss",
                                     &object, redis_ce,
                                     &key, &key_len, &val, &val_len) == FAILURE) {
        RETURN_FALSE;
    }

    if (redis_sock_get(object, &redis_sock TSRMLS_CC) < 0) {
        RETURN_FALSE;
    }

    cmd_len = redis_cmd_format(&cmd, "SREM %s %d\r\n%s\r\n",
                               key, key_len,
                               val_len, 
                               val, val_len);

    if (redis_sock_write(redis_sock, cmd, cmd_len) < 0) {
        efree(cmd);
        RETURN_FALSE;
    }
    efree(cmd);
    redis_1_response(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock TSRMLS_CC);
}
/* }}} */
/* {{{ proto boolean Redis::sMove(string set_src, string set_dst, string value)
 */
PHP_METHOD(Redis, sMove)
{
    zval *object;
    RedisSock *redis_sock;
    char *src = NULL, *dst = NULL, *val = NULL, *cmd;
    int src_len, dst_len, val_len, cmd_len;

    if (zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), "Osss",
                                     &object, redis_ce,
                                     &src, &src_len,
                                     &dst, &dst_len,
                                     &val, &val_len) == FAILURE) {
        RETURN_FALSE;
    }

    if (redis_sock_get(object, &redis_sock TSRMLS_CC) < 0) {
        RETURN_FALSE;
    }

    cmd_len = redis_cmd_format(&cmd, "SMOVE %s %s %d\r\n%s\r\n",
                               src, src_len,
                               dst, dst_len,
                               val_len,
                               val, val_len);

    if (redis_sock_write(redis_sock, cmd, cmd_len) < 0) {
        efree(cmd);
        RETURN_FALSE;
    }
    efree(cmd);
    redis_1_response(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock TSRMLS_CC);
}
/* }}} */

/* }}} */
/* {{{ proto string Redis::sPop(string key)
 */
PHP_METHOD(Redis, sPop)
{
    zval *object;
    RedisSock *redis_sock;
    char *key = NULL, *cmd, *response;
    int key_len, cmd_len, response_len;

    if (zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), "Os",
                                     &object, redis_ce,
                                     &key, &key_len) == FAILURE) {
        RETURN_FALSE;
    }

    if (redis_sock_get(object, &redis_sock TSRMLS_CC) < 0) {
        RETURN_FALSE;
    }

    cmd_len = redis_cmd_format(&cmd, "SPOP %s\r\n", key, key_len);

    if (redis_sock_write(redis_sock, cmd, cmd_len) < 0) {
        efree(cmd);
        RETURN_FALSE;
    }
    efree(cmd);

    if ((response = redis_sock_read(redis_sock, &response_len TSRMLS_CC)) == NULL) {
        RETURN_FALSE;
    }
    RETURN_STRINGL(response, response_len, 0);
}
/* }}} */

/* {{{ proto boolean Redis::sContains(string set, string value)
 */
PHP_METHOD(Redis, sContains)
{
    zval *object;
    RedisSock *redis_sock;
    char *key = NULL, *val = NULL, *cmd;
    int key_len, val_len, cmd_len;

    if (zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), "Oss",
                                     &object, redis_ce,
                                     &key, &key_len, &val, &val_len) == FAILURE) {
        return;
    }

    if (redis_sock_get(object, &redis_sock TSRMLS_CC) < 0) {
        RETURN_FALSE;
    }

    cmd_len = redis_cmd_format(&cmd, "SISMEMBER %s %d\r\n%s\r\n",
                               key, key_len,
                               val_len, 
                               val, val_len);

    if (redis_sock_write(redis_sock, cmd, cmd_len) < 0) {
        efree(cmd);
        RETURN_FALSE;
    }
    efree(cmd);
    redis_1_response(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock TSRMLS_CC);
}
/* }}} */

/* {{{ proto array Redis::sGetMembers(string set)
 */
PHP_METHOD(Redis, sGetMembers)
{
    zval *object;
    RedisSock *redis_sock;
    char *key = NULL, *cmd;
    int key_len, cmd_len, response_len;

    if (zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), "Os",
                                     &object, redis_ce,
                                     &key, &key_len) == FAILURE) {
        RETURN_FALSE;
    }

    if (redis_sock_get(object, &redis_sock TSRMLS_CC) < 0) {
        RETURN_FALSE;
    }

    cmd_len = spprintf(&cmd, 0, "SMEMBERS %s\r\n", key);

    if (redis_sock_write(redis_sock, cmd, cmd_len) < 0) {
        efree(cmd);
        RETURN_FALSE;
    }
    efree(cmd);

    if (redis_sock_read_multibulk_reply(INTERNAL_FUNCTION_PARAM_PASSTHRU,
                                        redis_sock TSRMLS_CC) < 0) {
        RETURN_FALSE;
    }
}
/* }}} */


PHPAPI int generic_multiple_args_cmd(INTERNAL_FUNCTION_PARAMETERS, char *keyword, int keyword_len,
                int min_argc, RedisSock **redis_sock TSRMLS_DC)
{
    zval *object, **z_args;
    char **keys, *cmd;
    int cmd_len, *keys_len;
    int i, argc = ZEND_NUM_ARGS();

    if(argc < min_argc) {
        WRONG_PARAM_COUNT;
        RETURN_FALSE;
    }

    z_args = emalloc(argc * sizeof(zval*));
    if(zend_get_parameters_array(ht, argc, z_args) == FAILURE) {
        efree(z_args);
        RETURN_FALSE;
    }

    /* prepare an array for the keys, and one for their lengths */
    keys = emalloc(argc * sizeof(char*));
    keys_len = emalloc(argc * sizeof(int));

    cmd_len = keyword_len; /* start computing the command length */

    for(i = 0; i < argc; ++i) { /* store each key */
        keys[i] = Z_STRVAL_P(z_args[i]);
        keys_len[i] = Z_STRLEN_P(z_args[i]);
        cmd_len += keys_len[i] + 1; /* +1 for the preceding space. */
    }

    /* get redis socket */
    if (redis_sock_get(getThis(), redis_sock TSRMLS_CC) < 0) {
        efree(keys);
        efree(keys_len);
        efree(z_args);
        RETURN_FALSE;
    }

    cmd_len += sizeof("\r\n") - 1;
    cmd = emalloc(cmd_len);

    memcpy(cmd, keyword, keyword_len);
    int pos = keyword_len;
    /* copy each key to its destination */
    for(i = 0; i < argc; ++i) {
        cmd[pos] = ' ';
        memcpy(cmd + pos + 1, keys[i], keys_len[i]);
        pos += 1+keys_len[i];
    }
    /* add the final new line. */
    memcpy(cmd + pos, "\r\n", 2);
    efree(keys);
    efree(keys_len);
    efree(z_args);


    if (redis_sock_write(*redis_sock, cmd, cmd_len) < 0) {
        efree(cmd);
        RETURN_FALSE;
    }
    efree(cmd);
}

/* {{{ proto array Redis::sInter(string key0, ... string keyN)
 */
PHP_METHOD(Redis, sInter) {

    int response_len;
    RedisSock *redis_sock;

    generic_multiple_args_cmd(INTERNAL_FUNCTION_PARAM_PASSTHRU,
                    "SINTER", sizeof("SINTER") - 1,
                    0, &redis_sock TSRMLS_CC);

    /* read multibulk reply */
    if (redis_sock_read_multibulk_reply(INTERNAL_FUNCTION_PARAM_PASSTHRU,
                                        redis_sock TSRMLS_CC) < 0) {
        RETURN_FALSE;
    }
}
/* }}} */

/* {{{ proto array Redis::sInterStore(string destination, string key0, ... string keyN)
 */
PHP_METHOD(Redis, sInterStore) {

    RedisSock *redis_sock;

    generic_multiple_args_cmd(INTERNAL_FUNCTION_PARAM_PASSTHRU,
                    "SINTERSTORE", sizeof("SINTERSTORE") - 1,
                    1, &redis_sock TSRMLS_CC);
    redis_long_response(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock TSRMLS_CC);
}
/* }}} */

/* {{{ proto array Redis::sUnion(string key0, ... string keyN)
 */
PHP_METHOD(Redis, sUnion) {

    int response_len;
    RedisSock *redis_sock;

    generic_multiple_args_cmd(INTERNAL_FUNCTION_PARAM_PASSTHRU,
                    "SUNION", sizeof("SUNION") - 1,
                    0, &redis_sock TSRMLS_CC);

    /* read multibulk reply */
    if (redis_sock_read_multibulk_reply(INTERNAL_FUNCTION_PARAM_PASSTHRU,
                                        redis_sock TSRMLS_CC) < 0) {
        RETURN_FALSE;
    }
}
/* }}} */
/* {{{ proto array Redis::sUnionStore(string destination, string key0, ... string keyN)
 */
PHP_METHOD(Redis, sUnionStore) {

    RedisSock *redis_sock;

    generic_multiple_args_cmd(INTERNAL_FUNCTION_PARAM_PASSTHRU,
                    "SUNIONSTORE", sizeof("SUNIONSTORE") - 1,
                    1, &redis_sock TSRMLS_CC);
    redis_long_response(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock TSRMLS_CC);
}
/* }}} */

/* {{{ proto array Redis::sDiff(string key0, ... string keyN)
 */
PHP_METHOD(Redis, sDiff) {

    int response_len;
    RedisSock *redis_sock;

    generic_multiple_args_cmd(INTERNAL_FUNCTION_PARAM_PASSTHRU,
                    "SDiff", sizeof("SDiff") - 1,
                    0, &redis_sock TSRMLS_CC);

    /* read multibulk reply */
    if (redis_sock_read_multibulk_reply(INTERNAL_FUNCTION_PARAM_PASSTHRU,
                                        redis_sock TSRMLS_CC) < 0) {
        RETURN_FALSE;
    }
}
/* }}} */

/* {{{ proto array Redis::sDiffStore(string destination, string key0, ... string keyN)
 */
PHP_METHOD(Redis, sDiffStore) {

    RedisSock *redis_sock;

    generic_multiple_args_cmd(INTERNAL_FUNCTION_PARAM_PASSTHRU,
                    "SDIFFSTORE", sizeof("SDIFFSTORE") - 1,
                    1, &redis_sock TSRMLS_CC);
    redis_long_response(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock TSRMLS_CC);
}
/* }}} */


PHPAPI void generic_sort_cmd(INTERNAL_FUNCTION_PARAMETERS, char *sort, int use_alpha TSRMLS_DC) {

    zval *object;
    RedisSock *redis_sock;
    char *key = NULL, *pattern = NULL, *get = NULL, *cmd;
    int key_len, pattern_len = -1, get_len = -1, cmd_len, response_len;
    long start = -1, end = -1;

    long use_pound = 0;

    char *by_cmd = "";
    char *by_arg = "";

    char *get_cmd = "";
    char *get_arg = "";
    char *get_pound = "";

    char *limit = "";

    char *alpha = "";

    if (zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), "Os|ssllb",
                                     &object, redis_ce,
                                     &key, &key_len, &pattern, &pattern_len,
                                     &get, &get_len, &start, &end, &use_pound) == FAILURE) {
        RETURN_FALSE;
    }

    if (redis_sock_get(object, &redis_sock TSRMLS_CC) < 0) {
        RETURN_FALSE;
    }
    if(key_len == 0) {
        RETURN_FALSE;
    }

    if(start >= 0 && end >= start) {
        redis_cmd_format(&limit, "LIMIT %d %d", start, end);
    }

    char format[] = "SORT "
            "%s " /* key, mandatory */
            "%s " /* BY, optional */
            "%s " /* argument for BY, optional */
            "%s " /* GET, optional */
            "%s " /* argument for GET, optional */
            "%s " /* "#" argument for GET, optional */
            "%s " /* LIMIT, optional. full string in this format: "LIMIT 42, 100" */
            "%s " /* ALPHA, optional */
            "%s " /* ASC or DESC, optional */
            "\r\n";

    if(pattern && pattern_len) {
        by_cmd = "BY";
        by_arg = pattern;
    }
    if(get && get_len) {
        get_cmd = "GET";
        get_arg = get;
        get_pound = use_pound ? "#" : "";
    }
    if(start >= 0 && end >= start) {
        spprintf(&limit, 0, "LIMIT %ld %ld", start, end);
    }
    if(use_alpha) {
        alpha = "ALPHA";
    }

    cmd_len = spprintf(&cmd, 0, format, key, by_cmd, by_arg, get_cmd, get_arg, get_pound, limit, alpha, sort);

    if(*limit) {
        efree(limit);
    }

    if (redis_sock_write(redis_sock, cmd, cmd_len) < 0) {
        efree(cmd);
        RETURN_FALSE;
    }
    efree(cmd);

    if (redis_sock_read_multibulk_reply(INTERNAL_FUNCTION_PARAM_PASSTHRU,
                                        redis_sock TSRMLS_CC) < 0) {
        RETURN_FALSE;
    }
}

/* {{{ proto array Redis::sortAsc(string key, string pattern, string get, int start, int end, bool getList])
 */
PHP_METHOD(Redis, sortAsc)
{
    generic_sort_cmd(INTERNAL_FUNCTION_PARAM_PASSTHRU, "ASC", 0 TSRMLS_CC);
}
/* }}} */

/* {{{ proto array Redis::sortAscAlpha(string key, string pattern, string get, int start, int end, bool getList])
 */
PHP_METHOD(Redis, sortAscAlpha)
{
    generic_sort_cmd(INTERNAL_FUNCTION_PARAM_PASSTHRU, "ASC", 1 TSRMLS_CC);
}
/* }}} */

/* {{{ proto array Redis::sortDesc(string key, string pattern, string get, int start, int end, bool getList])
 */
PHP_METHOD(Redis, sortDesc)
{
    generic_sort_cmd(INTERNAL_FUNCTION_PARAM_PASSTHRU, "DESC", 0 TSRMLS_CC);
}
/* }}} */

/* {{{ proto array Redis::sortDescAlpha(string key, string pattern, string get, int start, int end, bool getList])
 */
PHP_METHOD(Redis, sortDescAlpha)
{
    generic_sort_cmd(INTERNAL_FUNCTION_PARAM_PASSTHRU, "DESC", 1 TSRMLS_CC);
}
/* }}} */

/* {{{ proto array Redis::setTimeout(string key, int timeout)
 */
PHP_METHOD(Redis, setTimeout) {

    zval *object;
    RedisSock *redis_sock;
    char *key = NULL, *cmd;
    int key_len, cmd_len;
    long timeout;

    if (zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), "Osl",
                                     &object, redis_ce, &key, &key_len,
                                     &timeout) == FAILURE) {
        RETURN_FALSE;
    }

    if (redis_sock_get(object, &redis_sock TSRMLS_CC) < 0) {
        RETURN_FALSE;
    }

    cmd_len = spprintf(&cmd, 0, "EXPIRE %s %d\r\n", key, (int)timeout);

    if (redis_sock_write(redis_sock, cmd, cmd_len) < 0) {
        efree(cmd);
        RETURN_FALSE;
    }
    efree(cmd);
    redis_1_response(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock TSRMLS_CC);
}
/* }}} */

/* {{{ proto array Redis::lSet(string key, int index, string value)
 */
PHP_METHOD(Redis, lSet) {

    zval *object;
    RedisSock *redis_sock;

    char *cmd;
    int cmd_len, key_len, val_len;
    long index;
    char *key, *val;

    if (zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), "Osls",
                                     &object, redis_ce, &key, &key_len, &index, &val, &val_len) == FAILURE) {
        RETURN_FALSE;
    }

    if (redis_sock_get(object, &redis_sock TSRMLS_CC) < 0) {
        RETURN_FALSE;
    }

    cmd_len = redis_cmd_format(&cmd, "LSET %s %d %d\r\n%s\r\n",
                               key, key_len,
                               (int)index,
                               val_len,
                               val, val_len);

    if (redis_sock_write(redis_sock, cmd, cmd_len) < 0) {
        efree(cmd);
        RETURN_FALSE;
    }
    efree(cmd);
    redis_boolean_response(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock TSRMLS_CC);
}
/* }}} */


PHPAPI void generic_empty_cmd(INTERNAL_FUNCTION_PARAMETERS, char *cmd, int cmd_len TSRMLS_DC) {
    zval *object;
    RedisSock *redis_sock;
    char ret;

    if (zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), "O",
                                     &object, redis_ce) == FAILURE) {
        RETURN_FALSE;
    }

    if (redis_sock_get(object, &redis_sock TSRMLS_CC) < 0) {
        RETURN_FALSE;
    }

    if (redis_sock_write(redis_sock, cmd, cmd_len) < 0) {
        /* no efree here, cmd is an argument. */
        RETURN_FALSE;
    }
    redis_boolean_response(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock TSRMLS_CC);
}

/* {{{ proto string Redis::save()
 */
PHP_METHOD(Redis, save)
{
    generic_empty_cmd(INTERNAL_FUNCTION_PARAM_PASSTHRU, "SAVE\r\n", sizeof("SAVE\r\n")-1 TSRMLS_CC);

}
/* }}} */

/* {{{ proto string Redis::bgSave()
 */
PHP_METHOD(Redis, bgSave)
{
    generic_empty_cmd(INTERNAL_FUNCTION_PARAM_PASSTHRU, "BGSAVE\r\n", sizeof("BGSAVE\r\n")-1 TSRMLS_CC);

}
/* }}} */

PHPAPI void generic_empty_long_cmd(INTERNAL_FUNCTION_PARAMETERS, char *cmd, int cmd_len TSRMLS_DC) {

    zval *object;
    RedisSock *redis_sock;

    if (zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), "O",
                                     &object, redis_ce) == FAILURE) {
        RETURN_FALSE;
    }

    if (redis_sock_get(object, &redis_sock TSRMLS_CC) < 0) {
        RETURN_FALSE;
    }

    if (redis_sock_write(redis_sock, cmd, cmd_len) < 0) {
        /* no efree here, cmd is an argument. */
        RETURN_FALSE;
    }
    redis_long_response(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock TSRMLS_CC);
}

/* {{{ proto integer Redis::lastSave()
 */
PHP_METHOD(Redis, lastSave)
{
    generic_empty_long_cmd(INTERNAL_FUNCTION_PARAM_PASSTHRU, "LASTSAVE\r\n", sizeof("LASTSAVE\r\n") TSRMLS_CC);
}
/* }}} */


/* {{{ proto bool Redis::flushDB()
 */
PHP_METHOD(Redis, flushDB)
{
    generic_empty_cmd(INTERNAL_FUNCTION_PARAM_PASSTHRU, "FLUSHDB\r\n", sizeof("FLUSHDB\r\n")-1 TSRMLS_CC);
}
/* }}} */

/* {{{ proto bool Redis::flushAll()
 */
PHP_METHOD(Redis, flushAll)
{
    generic_empty_cmd(INTERNAL_FUNCTION_PARAM_PASSTHRU, "FLUSHALL\r\n", sizeof("FLUSHALL\r\n")-1 TSRMLS_CC);
}
/* }}} */

/* {{{ proto int Redis::dbSize()
 */
PHP_METHOD(Redis, dbSize)
{
    generic_empty_long_cmd(INTERNAL_FUNCTION_PARAM_PASSTHRU, "DBSIZE\r\n", sizeof("DBSIZE\r\n") TSRMLS_CC);
}
/* }}} */

/* {{{ proto bool Redis::auth(string passwd)
 */
PHP_METHOD(Redis, auth) {

    zval *object;
    RedisSock *redis_sock;

    char *cmd, *password;
    int cmd_len, password_len;

    if (zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), "Os",
                                     &object, redis_ce, &password, &password_len) == FAILURE) {
        RETURN_FALSE;
    }

    if (redis_sock_get(object, &redis_sock TSRMLS_CC) < 0) {
        RETURN_FALSE;
    }

    cmd_len = redis_cmd_format(&cmd, "AUTH %s\r\n", password, password_len);

    if (redis_sock_write(redis_sock, cmd, cmd_len) < 0) {
        efree(cmd);
        RETURN_FALSE;
    }
    efree(cmd);
    redis_boolean_response(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock TSRMLS_CC);
}
/* }}} */

/* {{{ proto long Redis::ttl(string key)
 */
PHP_METHOD(Redis, ttl) {

    zval *object;
    RedisSock *redis_sock;

    char *cmd, *key;
    int cmd_len, key_len;

    if (zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), "Os",
                                     &object, redis_ce, &key, &key_len) == FAILURE) {
        RETURN_FALSE;
    }

    if (redis_sock_get(object, &redis_sock TSRMLS_CC) < 0) {
        RETURN_FALSE;
    }

    cmd_len = redis_cmd_format(&cmd, "TTL %s\r\n", key, key_len);

    if (redis_sock_write(redis_sock, cmd, cmd_len) < 0) {
        efree(cmd);
        RETURN_FALSE;
    }
    efree(cmd);
    redis_long_response(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock TSRMLS_CC);
}
/* }}} */

/* {{{ proto array Redis::info()
 */
PHP_METHOD(Redis, info) {

    zval *object;
    RedisSock *redis_sock;

    char cmd[] = "INFO\r\n", *response, *key;
    int cmd_len = sizeof(cmd)-1, response_len;
    long ttl;
    char *cur, *pos, *value;
    int is_numeric;
    char *p;

    if (zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), "O",
                                     &object, redis_ce) == FAILURE) {
        RETURN_FALSE;
    }

    if (redis_sock_get(object, &redis_sock TSRMLS_CC) < 0) {
        RETURN_FALSE;
    }

    if (redis_sock_write(redis_sock, cmd, cmd_len) < 0) {
        /* no efree(cmd) here, it's on the stack. */
        RETURN_FALSE;
    }

    if ((response = redis_sock_read(redis_sock, &response_len TSRMLS_CC)) == NULL) {
        RETURN_FALSE;
    }

    array_init(return_value);
    /* response :: [response_line]
     * response_line :: key ':' value CRLF
     */

    cur = response;
    while(1) {
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
            add_assoc_long(return_value, key, atol(value));
            efree(value);
        } else {
            add_assoc_string(return_value, key, value, 0);
        }
        efree(key);
    }
}
/* }}} */

/* {{{ proto bool Redis::select(long dbNumber)
 */
PHP_METHOD(Redis, select) {

    zval *object;
    RedisSock *redis_sock;

    char *cmd;
    int cmd_len;
    long dbNumber;

    if (zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), "Ol",
                                     &object, redis_ce, &dbNumber) == FAILURE) {
        RETURN_FALSE;
    }

    if (redis_sock_get(object, &redis_sock TSRMLS_CC) < 0) {
        RETURN_FALSE;
    }

    cmd_len = redis_cmd_format(&cmd, "SELECT %d\r\n", dbNumber);

    if (redis_sock_write(redis_sock, cmd, cmd_len) < 0) {
        efree(cmd);
        RETURN_FALSE;
    }
    efree(cmd);
    redis_boolean_response(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock TSRMLS_CC);
}
/* }}} */

/* {{{ proto bool Redis::move(string key, long dbindex)
 */
PHP_METHOD(Redis, move) {

    zval *object;
    RedisSock *redis_sock;

    char *cmd, *key;
    int cmd_len, key_len;
    long dbNumber;

    if (zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), "Osl",
                                     &object, redis_ce, &key, &key_len, &dbNumber) == FAILURE) {
        RETURN_FALSE;
    }

    if (redis_sock_get(object, &redis_sock TSRMLS_CC) < 0) {
        RETURN_FALSE;
    }

    cmd_len = redis_cmd_format(&cmd, "MOVE %s %d\r\n",
                               key, key_len, dbNumber);

    if (redis_sock_write(redis_sock, cmd, cmd_len) < 0) {
        efree(cmd);
        RETURN_FALSE;
    }
    efree(cmd);
    redis_1_response(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock TSRMLS_CC);
}
/* }}} */

/* {{{ proto bool Redis::mset(array (key => value, ...))
 */
PHP_METHOD(Redis, mset) {
    zval *object;
    RedisSock *redis_sock;

    char *cmd;
    int cmd_len;
    zval *z_array;

    if (zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), "Oa",
                                     &object, redis_ce, &z_array) == FAILURE) {
        RETURN_FALSE;
    }

    if (redis_sock_get(object, &redis_sock TSRMLS_CC) < 0) {
        RETURN_FALSE;
    }

    if(zend_hash_num_elements(Z_ARRVAL_P(z_array)) == 0) {
        RETURN_FALSE;
    }

    cmd_len = redis_cmd_format(&cmd, "*%d\r\n", 1 + 2 * zend_hash_num_elements(Z_ARRVAL_P(z_array)));

    if (redis_sock_write(redis_sock, cmd, cmd_len) < 0) {
        efree(cmd);
        RETURN_FALSE;
    }
    efree(cmd);

    if (redis_sock_write(redis_sock, "$4\r\nMSET\r\n", 10) < 0) {
        RETURN_FALSE;
    }

    HashTable *keytable = Z_ARRVAL_P(z_array);
    for(zend_hash_internal_pointer_reset(keytable);
        zend_hash_has_more_elements(keytable) == SUCCESS;
        zend_hash_move_forward(keytable)) {

        char *key, *val;
        int key_len, val_len;
        unsigned long idx;
        int type;
        zval **z_value_pp;

        type = zend_hash_get_current_key_ex(keytable, &key, &key_len, &idx, 0, NULL);
        if(zend_hash_get_current_data(keytable, (void**)&z_value_pp) == FAILURE) {
            continue; 	/* this should never happen, according to the PHP people. */
        }

        if(type != HASH_KEY_IS_STRING) { /* ignore non-string keys */
            continue;
        }

        val = Z_STRVAL_PP(z_value_pp);
        val_len = Z_STRLEN_PP(z_value_pp);

        if(key_len > 0) {
            key_len--;
        }

        cmd_len = redis_cmd_format(&cmd, "$%d\r\n%s\r\n$%d\r\n%s\r\n",
                                   key_len, key, key_len,
                                   val_len, val, val_len);

        redis_sock_write(redis_sock, cmd, cmd_len);
        efree(cmd);
    }

    redis_boolean_response(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock TSRMLS_CC);
}
/* }}} */


/* {{{ proto string Redis::rpoplpush(string srckey, string dstkey)
 */
PHP_METHOD(Redis, rpoplpush)
{
    zval *object;
    RedisSock *redis_sock;
    char *srckey = NULL, *dstkey = NULL, *cmd, *response;
    int srckey_len, dstkey_len, cmd_len, response_len;

    if (zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), "Oss",
                                     &object, redis_ce, &srckey, &srckey_len,
                                     &dstkey, &dstkey_len) == FAILURE) {
        RETURN_FALSE;
    }

    if (redis_sock_get(object, &redis_sock TSRMLS_CC) < 0) {
        RETURN_FALSE;
    }

    cmd_len = redis_cmd_format(&cmd, "RPOPLPUSH %s %d\r\n%s\r\n",
                               srckey, srckey_len,
                               dstkey_len,
                               dstkey, dstkey_len);

    if (redis_sock_write(redis_sock, cmd, cmd_len) < 0) {
        efree(cmd);
        RETURN_FALSE;
    }
    efree(cmd);
    if ((response = redis_sock_read(redis_sock, &response_len TSRMLS_CC)) == NULL) {
        RETURN_FALSE;
    }
    RETURN_STRINGL(response, response_len, 0);
}
/* }}} */
/* {{{ proto array Redis::zAdd(string key, int score, string value)
 */
PHP_METHOD(Redis, zAdd) {

    zval *object;
    RedisSock *redis_sock;

    char *cmd;
    int cmd_len, key_len, val_len;
    long score;
    char *key, *val;

    if (zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), "Osls",
                                     &object, redis_ce, &key, &key_len, &score, &val, &val_len) == FAILURE) {
        RETURN_FALSE;
    }

    if (redis_sock_get(object, &redis_sock TSRMLS_CC) < 0) {
        RETURN_FALSE;
    }

    cmd_len = redis_cmd_format(&cmd, "ZADD %s %d %d\r\n%s\r\n",
                               key, key_len,
                               (int)score,
                               val_len,
                               val, val_len);

    if (redis_sock_write(redis_sock, cmd, cmd_len) < 0) {
        efree(cmd);
        RETURN_FALSE;
    }
    efree(cmd);
    redis_long_response(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock TSRMLS_CC);
}
/* }}} */
/* {{{ proto array Redis::zRange(string key, int start , int end)
 */
PHP_METHOD(Redis, zRange)
{
    zval *object;
    RedisSock *redis_sock;
    char *key = NULL, *cmd;
    int key_len, cmd_len, response_len;
    long start, end;

    if (zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), "Osll",
                                     &object, redis_ce,
                                     &key, &key_len, &start, &end) == FAILURE) {
        RETURN_FALSE;
    }

    if (redis_sock_get(object, &redis_sock TSRMLS_CC) < 0) {
        RETURN_FALSE;
    }

    cmd_len = spprintf(&cmd, 0, "ZRANGE %s %d %d\r\n\r\n", key, (int)start, (int)end);

    if (redis_sock_write(redis_sock, cmd, cmd_len) < 0) {
        efree(cmd);
        RETURN_FALSE;
    }
    efree(cmd);

    if (redis_sock_read_multibulk_reply(INTERNAL_FUNCTION_PARAM_PASSTHRU,
                                        redis_sock TSRMLS_CC) < 0) {
        RETURN_FALSE;
    }
}
/* }}} */
/* {{{ proto boolean Redis::zDelete(string key, string member)
 */
PHP_METHOD(Redis, zRemove)
{
    zval *object;
    RedisSock *redis_sock;
    char *key = NULL, *member = NULL, *cmd;
    int key_len, member_len, cmd_len, count;

    if (zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), "Oss",
                                     &object, redis_ce, &key, &key_len,
                                     &member, &member_len) == FAILURE) {
        RETURN_FALSE;
    }

    if (redis_sock_get(object, &redis_sock TSRMLS_CC) < 0) {
        RETURN_FALSE;
    }

    cmd_len = redis_cmd_format(&cmd, "ZREM %s %d\r\n%s\r\n", key, key_len, member_len, member, member_len);

    if (redis_sock_write(redis_sock, cmd, cmd_len) < 0) {
        efree(cmd);
        RETURN_FALSE;
    }
    efree(cmd);
    redis_long_response(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock TSRMLS_CC);
}
/* }}} */
/* {{{ proto boolean Redis::zDeleteRangeByScore(string key, string member)
 */
PHP_METHOD(Redis, zDeleteRangeByScore)
{
    zval *object;
    RedisSock *redis_sock;
    char *key = NULL, *cmd;
    int key_len, cmd_len, response_len;
    long start, end;

    if (zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), "Osll",
                                     &object, redis_ce,
                                     &key, &key_len, &start, &end) == FAILURE) {
        RETURN_FALSE;
    }

    if (redis_sock_get(object, &redis_sock TSRMLS_CC) < 0) {
        RETURN_FALSE;
    }

    cmd_len = spprintf(&cmd, 0, "ZREMRANGEBYSCORE %s %d %d\r\n\r\n", key, (int)start, (int)end);

    if (redis_sock_write(redis_sock, cmd, cmd_len) < 0) {
        efree(cmd);
        RETURN_FALSE;
    }
    efree(cmd);
    redis_long_response(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock TSRMLS_CC);
}
/* }}} */
/* {{{ proto array Redis::zReverseRange(string key, int start , int end)
 */
PHP_METHOD(Redis, zReverseRange)
{
    zval *object;
    RedisSock *redis_sock;
    char *key = NULL, *cmd;
    int key_len, cmd_len, response_len;
    long start, end;

    if (zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), "Osll",
                                     &object, redis_ce,
                                     &key, &key_len, &start, &end) == FAILURE) {
        RETURN_FALSE;
    }

    if (redis_sock_get(object, &redis_sock TSRMLS_CC) < 0) {
        RETURN_FALSE;
    }

    cmd_len = spprintf(&cmd, 0, "ZREVRANGE %s %d %d\r\n\r\n", key, (int)start, (int)end);

    if (redis_sock_write(redis_sock, cmd, cmd_len) < 0) {
        efree(cmd);
        RETURN_FALSE;
    }
    efree(cmd);

    if (redis_sock_read_multibulk_reply(INTERNAL_FUNCTION_PARAM_PASSTHRU,
                                        redis_sock TSRMLS_CC) < 0) {
        RETURN_FALSE;
    }
}
/* }}} */
/* {{{ proto array Redis::zRangeByScore(string key, int start , int end)
 */
PHP_METHOD(Redis, zRangeByScore)
{
    zval *object;
    RedisSock *redis_sock;
    char *key = NULL, *cmd;
    int key_len, cmd_len, response_len;
    long start, end;

    if (zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), "Osll",
                                     &object, redis_ce,
                                     &key, &key_len, &start, &end) == FAILURE) {
        RETURN_FALSE;
    }

    if (redis_sock_get(object, &redis_sock TSRMLS_CC) < 0) {
        RETURN_FALSE;
    }

    cmd_len = spprintf(&cmd, 0, "ZRANGEBYSCORE %s %d %d\r\n\r\n", key, (int)start, (int)end);

    if (redis_sock_write(redis_sock, cmd, cmd_len) < 0) {
        efree(cmd);
        RETURN_FALSE;
    }
    efree(cmd);

    if (redis_sock_read_multibulk_reply(INTERNAL_FUNCTION_PARAM_PASSTHRU,
                                        redis_sock TSRMLS_CC) < 0) {
        RETURN_FALSE;
    }
}
/* }}} */

/* {{{ proto int Redis::zCard(string key)
 */
PHP_METHOD(Redis, zCard)
{
    zval *object;
    RedisSock *redis_sock;
    char *key = NULL, *cmd;
    int key_len, cmd_len, response_len;

    if (zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), "Os",
                                     &object, redis_ce,
                                     &key, &key_len) == FAILURE) {
        RETURN_FALSE;
    }

    if (redis_sock_get(object, &redis_sock TSRMLS_CC) < 0) {
        RETURN_FALSE;
    }

    cmd_len = spprintf(&cmd, 0, "ZCARD %s\r\n", key);

    if (redis_sock_write(redis_sock, cmd, cmd_len) < 0) {
        efree(cmd);
        RETURN_FALSE;
    }
    efree(cmd);
    redis_long_response(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock TSRMLS_CC);
}
/* }}} */
/* {{{ proto boolean Redis::zScore(string key, string member)
 */
PHP_METHOD(Redis, zScore)
{
    zval *object;
    RedisSock *redis_sock;
    char *key = NULL, *member = NULL, *cmd;
    int key_len, member_len, cmd_len, count;

    if (zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), "Oss",
                                     &object, redis_ce, &key, &key_len,
                                     &member, &member_len) == FAILURE) {
        RETURN_FALSE;
    }

    if (redis_sock_get(object, &redis_sock TSRMLS_CC) < 0) {
        RETURN_FALSE;
    }

    cmd_len = redis_cmd_format(&cmd, "ZSCORE %s %d\r\n%s\r\n", key, key_len, member_len, member, member_len);

    if (redis_sock_write(redis_sock, cmd, cmd_len) < 0) {
        efree(cmd);
        RETURN_FALSE;
    }
    efree(cmd);
    redis_bulk_double_response(INTERNAL_FUNCTION_PARAM_PASSTHRU, redis_sock TSRMLS_CC);
}
/* {{{ proto boolean Redis::zIncrBy(string key, int value, string member)
 */
/*
PHP_METHOD(Redis, zIncrBy)
{
    zval *object;
    RedisSock *redis_sock;
    char *key = NULL, *cmd, *member, *response;
    int key_len, member_len, cmd_len, response_len;
    long val = 1;

    if (zend_parse_method_parameters(ZEND_NUM_ARGS() TSRMLS_CC, getThis(), "Osls",
                                     &object, redis_ce,
                                     &key, &key_len, &val, &member, &member_len) == FAILURE) {
        RETURN_FALSE;
    }

    if (redis_sock_get(object, &redis_sock TSRMLS_CC) < 0) {
        RETURN_FALSE;
    }
    cmd_len = redis_cmd_format(&cmd, "ZINCRBY %s %d %d\r\n%s\r\n",
                    key, key_len, (int)val, member_len, member, member_len);

    if (redis_sock_write(redis_sock, cmd, cmd_len) < 0) {
        efree(cmd);
        RETURN_FALSE;
    }
    efree(cmd);
    if ((response = redis_sock_read(redis_sock, &response_len TSRMLS_CC)) == NULL) {
        RETURN_FALSE;
    }

    RETURN_LONG(atol(response));
}
*/
/* }}} */

/* vim: set tabstop=4 expandtab: */
