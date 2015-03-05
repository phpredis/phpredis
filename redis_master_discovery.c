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
  | Author: Axel Etcheverry <axel@etcheverry.biz>                        |
  +----------------------------------------------------------------------+
*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "common.h"
#include "ext/standard/info.h"
#include "php_ini.h"
#include "php_redis.h"
#include "redis_sentinel.h"
#include "redis_master_discovery.h"
#include <zend_exceptions.h>
#include "library.h"

extern int le_redis_sock;
extern zend_class_entry *redis_ce;
extern zend_class_entry *redis_exception_ce;
extern zend_class_entry *redis_sentinel_ce;

zend_class_entry *redis_master_discovery_ce;

ZEND_BEGIN_ARG_INFO(arginfo_redis_master_discovery_construct, 0)
ZEND_END_ARG_INFO()

/*ZEND_BEGIN_ARG_INFO(arginfo_redis_master_discovery_destruct, 0)
ZEND_END_ARG_INFO()*/

ZEND_BEGIN_ARG_INFO(arginfo_redis_master_discovery_add_sentinel, 0)
ZEND_ARG_INFO(0, sentinel)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO(arginfo_redis_master_discovery_get_sentinels, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO(arginfo_redis_master_discovery_get_master_addr, 0)
ZEND_END_ARG_INFO()

zend_function_entry redis_master_discovery_functions[] = {
    PHP_ME(RedisMasterDiscovery, __construct,   arginfo_redis_master_discovery_construct, ZEND_ACC_CTOR | ZEND_ACC_PUBLIC)
    //PHP_ME(RedisMasterDiscovery, __destruct,    arginfo_redis_master_discovery_destruct, ZEND_ACC_DTOR | ZEND_ACC_PUBLIC)
    PHP_ME(RedisMasterDiscovery, addSentinel,   arginfo_redis_master_discovery_add_sentinel, ZEND_ACC_PUBLIC)
    PHP_ME(RedisMasterDiscovery, getSentinels,  arginfo_redis_master_discovery_get_sentinels, ZEND_ACC_PUBLIC)
    PHP_ME(RedisMasterDiscovery, getMasterAddr, arginfo_redis_master_discovery_get_master_addr, ZEND_ACC_PUBLIC)
    PHP_FE_END
};

/* {{{ proto RedisMasterDiscovery RedisMasterDiscovery::__construct()
    Public constructor */
PHP_METHOD(RedisMasterDiscovery, __construct)
{
    zval *sentinels;

    if (zend_parse_parameters_none() == FAILURE) {
        return;
    }

    MAKE_STD_ZVAL(sentinels);
    array_init(sentinels);

    zend_update_property(
        redis_master_discovery_ce,
        getThis(),
        ZEND_STRS("sentinels")-1,
        sentinels TSRMLS_CC
    );
    zval_ptr_dtor(&sentinels);
}
/* }}} */

/* {{{ proto RedisMasterDiscovery RedisMasterDiscovery::__destruct()
    Public Destructor
 */
/*PHP_METHOD(RedisMasterDiscovery, __destruct)
{

}*/
/* }}} */

/* {{{ proto void RedisMasterDiscovery::addSentinel(RedisSentinel sentinel)
 */
PHP_METHOD(RedisMasterDiscovery, addSentinel)
{
    zval *object;
    zval *sentinel;
    zval *sentinels;

    if (zend_parse_method_parameters(
        ZEND_NUM_ARGS() TSRMLS_CC,
        getThis(),
        "OO",
        &object,
        redis_master_discovery_ce,
        &sentinel,
        redis_sentinel_ce
    ) == FAILURE) {
        RETURN_FALSE;
    }

    sentinels = zend_read_property(
        redis_master_discovery_ce,
        getThis(),
        ZEND_STRS("sentinels")-1,
        0 TSRMLS_CC
    );

    if (zend_hash_next_index_insert(Z_ARRVAL_P(sentinels), &sentinel, sizeof(sentinel), NULL) == SUCCESS) {
        Z_ADDREF_P(sentinel);
    }
}
/* }}} */


/* {{{ proto array RedisMasterDiscovery::getSentinels()
 */
PHP_METHOD(RedisMasterDiscovery, getSentinels)
{
    zval *sentinels;

    if (zend_parse_parameters_none() == FAILURE) {
        return;
    }

    sentinels = zend_read_property(
        redis_master_discovery_ce,
        getThis(),
        ZEND_STRS("sentinels")-1,
        0 TSRMLS_CC
    );

    RETURN_ZVAL(sentinels, 1, 0);
}
/* }}} */


/* {{{ proto array RedisMasterDiscovery::getMasterAddr()
 */
PHP_METHOD(RedisMasterDiscovery, getMasterAddr)
{
    zval *sentinels;

    if (zend_parse_parameters_none() == FAILURE) {
        return;
    }

    sentinels = zend_read_property(
        redis_master_discovery_ce,
        getThis(),
        ZEND_STRS("sentinels")-1,
        0 TSRMLS_CC
    );

    if (zend_hash_num_elements(Z_ARRVAL_P(sentinels)) <= 0) {
        zend_throw_exception(redis_exception_ce, "You need to add sentinel nodes before attempting to fetch a master", 0 TSRMLS_CC);
        RETURN_FALSE;
    }

    zval **sentinel;

    zval *fun_connect;
    MAKE_STD_ZVAL(fun_connect);
    ZVAL_STRING(fun_connect, "connect", 1);

    zval *available;
    MAKE_STD_ZVAL(available);

    for (zend_hash_internal_pointer_reset(Z_ARRVAL_P(sentinels));
         zend_hash_get_current_data(Z_ARRVAL_P(sentinels), (void **) &sentinel) == SUCCESS;
         zend_hash_move_forward(Z_ARRVAL_P(sentinels))
    ) {

        if (call_user_function(
            &redis_sentinel_ce->function_table,
            (zval**)sentinel,
            fun_connect,
            available,
            0,
            NULL TSRMLS_CC
        ) == SUCCESS) {

            if (Z_TYPE_P(available) == IS_BOOL && Z_BVAL_P(available)) {

                zval *fun_get_master_addr;
                MAKE_STD_ZVAL(fun_get_master_addr);
                ZVAL_STRING(fun_get_master_addr, "getMasterAddr", 1);

                zval *master;
                MAKE_STD_ZVAL(master);

                if (call_user_function(
                    &redis_sentinel_ce->function_table,
                    sentinel,
                    fun_get_master_addr,
                    master,
                    0,
                    NULL TSRMLS_CC
                ) == SUCCESS) {

                    if (Z_TYPE_P(master) == IS_ARRAY) {
                        zval_ptr_dtor(&fun_connect);
                        zval_ptr_dtor(&fun_get_master_addr);
                        zval_ptr_dtor(&available);
                        RETURN_ZVAL(master, 1, 1);
                    }
                }

                zval_ptr_dtor(&fun_get_master_addr);
                zval_ptr_dtor(&master);
            }
        }
    }

    zval_ptr_dtor(&available);
    zval_ptr_dtor(&fun_connect);

    zend_throw_exception(redis_exception_ce, "All sentinels are unreachable", 0 TSRMLS_CC);
    RETURN_FALSE;
}
/* }}} */

