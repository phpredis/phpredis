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

#include "common.h"

#ifndef PHP_REDIS_H
#define PHP_REDIS_H

PHP_METHOD(Redis, __construct);
PHP_METHOD(Redis, __destruct);
PHP_METHOD(Redis, connect);
PHP_METHOD(Redis, pconnect);
PHP_METHOD(Redis, close);
PHP_METHOD(Redis, ping);
PHP_METHOD(Redis, echo);
PHP_METHOD(Redis, get);
PHP_METHOD(Redis, set);
PHP_METHOD(Redis, setex);
PHP_METHOD(Redis, psetex);
PHP_METHOD(Redis, setnx);
PHP_METHOD(Redis, getSet);
PHP_METHOD(Redis, randomKey);
PHP_METHOD(Redis, renameKey);
PHP_METHOD(Redis, renameNx);
PHP_METHOD(Redis, getMultiple);
PHP_METHOD(Redis, exists);
PHP_METHOD(Redis, delete);
PHP_METHOD(Redis, incr);
PHP_METHOD(Redis, incrBy);
PHP_METHOD(Redis, incrByFloat);
PHP_METHOD(Redis, decr);
PHP_METHOD(Redis, decrBy);
PHP_METHOD(Redis, type);
PHP_METHOD(Redis, append);
PHP_METHOD(Redis, getRange);
PHP_METHOD(Redis, setRange);
PHP_METHOD(Redis, getBit);
PHP_METHOD(Redis, setBit);
PHP_METHOD(Redis, strlen);
PHP_METHOD(Redis, getKeys);
PHP_METHOD(Redis, sort);
PHP_METHOD(Redis, sortAsc);
PHP_METHOD(Redis, sortAscAlpha);
PHP_METHOD(Redis, sortDesc);
PHP_METHOD(Redis, sortDescAlpha);
PHP_METHOD(Redis, lPush);
PHP_METHOD(Redis, lPushx);
PHP_METHOD(Redis, rPush);
PHP_METHOD(Redis, rPushx);
PHP_METHOD(Redis, lPop);
PHP_METHOD(Redis, rPop);
PHP_METHOD(Redis, blPop);
PHP_METHOD(Redis, brPop);
PHP_METHOD(Redis, lSize);
PHP_METHOD(Redis, lRemove);
PHP_METHOD(Redis, listTrim);
PHP_METHOD(Redis, lGet);
PHP_METHOD(Redis, lGetRange);
PHP_METHOD(Redis, lSet);
PHP_METHOD(Redis, lInsert);
PHP_METHOD(Redis, sAdd);
PHP_METHOD(Redis, sSize);
PHP_METHOD(Redis, sRemove);
PHP_METHOD(Redis, sMove);
PHP_METHOD(Redis, sPop);
PHP_METHOD(Redis, sRandMember);
PHP_METHOD(Redis, sContains);
PHP_METHOD(Redis, sMembers);
PHP_METHOD(Redis, sInter);
PHP_METHOD(Redis, sInterStore);
PHP_METHOD(Redis, sUnion);
PHP_METHOD(Redis, sUnionStore);
PHP_METHOD(Redis, sDiff);
PHP_METHOD(Redis, sDiffStore);
PHP_METHOD(Redis, setTimeout);
PHP_METHOD(Redis, pexpire);
PHP_METHOD(Redis, save);
PHP_METHOD(Redis, bgSave);
PHP_METHOD(Redis, lastSave);
PHP_METHOD(Redis, flushDB);
PHP_METHOD(Redis, flushAll);
PHP_METHOD(Redis, dbSize);
PHP_METHOD(Redis, auth);
PHP_METHOD(Redis, ttl);
PHP_METHOD(Redis, pttl);
PHP_METHOD(Redis, persist);
PHP_METHOD(Redis, info);
PHP_METHOD(Redis, resetStat);
PHP_METHOD(Redis, select);
PHP_METHOD(Redis, move);
PHP_METHOD(Redis, zAdd);
PHP_METHOD(Redis, zDelete);
PHP_METHOD(Redis, zRange);
PHP_METHOD(Redis, zReverseRange);
PHP_METHOD(Redis, zRangeByScore);
PHP_METHOD(Redis, zRevRangeByScore);
PHP_METHOD(Redis, zCount);
PHP_METHOD(Redis, zDeleteRangeByScore);
PHP_METHOD(Redis, zDeleteRangeByRank);
PHP_METHOD(Redis, zCard);
PHP_METHOD(Redis, zScore);
PHP_METHOD(Redis, zRank);
PHP_METHOD(Redis, zRevRank);
PHP_METHOD(Redis, zIncrBy);
PHP_METHOD(Redis, zInter);
PHP_METHOD(Redis, zUnion);
PHP_METHOD(Redis, expireAt);
PHP_METHOD(Redis, pexpireAt);
PHP_METHOD(Redis, bgrewriteaof);
PHP_METHOD(Redis, slaveof);
PHP_METHOD(Redis, object);
PHP_METHOD(Redis, bitop);
PHP_METHOD(Redis, bitcount);

PHP_METHOD(Redis, eval);
PHP_METHOD(Redis, evalsha);
PHP_METHOD(Redis, script);
PHP_METHOD(Redis, dump);
PHP_METHOD(Redis, restore);
PHP_METHOD(Redis, migrate);

PHP_METHOD(Redis, time);

PHP_METHOD(Redis, getLastError);
PHP_METHOD(Redis, clearLastError);
PHP_METHOD(Redis, _prefix);
PHP_METHOD(Redis, _unserialize);

PHP_METHOD(Redis, mset);
PHP_METHOD(Redis, msetnx);
PHP_METHOD(Redis, rpoplpush);
PHP_METHOD(Redis, brpoplpush);

PHP_METHOD(Redis, hGet);
PHP_METHOD(Redis, hSet);
PHP_METHOD(Redis, hSetNx);
PHP_METHOD(Redis, hDel);
PHP_METHOD(Redis, hLen);
PHP_METHOD(Redis, hKeys);
PHP_METHOD(Redis, hVals);
PHP_METHOD(Redis, hGetAll);
PHP_METHOD(Redis, hExists);
PHP_METHOD(Redis, hIncrBy);
PHP_METHOD(Redis, hIncrByFloat);
PHP_METHOD(Redis, hMset);
PHP_METHOD(Redis, hMget);

PHP_METHOD(Redis, multi);
PHP_METHOD(Redis, discard);
PHP_METHOD(Redis, exec);
PHP_METHOD(Redis, watch);
PHP_METHOD(Redis, unwatch);

PHP_METHOD(Redis, pipeline);

PHP_METHOD(Redis, publish);
PHP_METHOD(Redis, subscribe);
PHP_METHOD(Redis, psubscribe);
PHP_METHOD(Redis, unsubscribe);
PHP_METHOD(Redis, punsubscribe);

PHP_METHOD(Redis, getOption);
PHP_METHOD(Redis, setOption);

PHP_METHOD(Redis, config);
PHP_METHOD(Redis, slowlog);

PHP_METHOD(Redis, client);

PHP_METHOD(Redis, getHost);
PHP_METHOD(Redis, getPort);
PHP_METHOD(Redis, getDBNum);
PHP_METHOD(Redis, getTimeout);
PHP_METHOD(Redis, getReadTimeout);
PHP_METHOD(Redis, isConnected);
PHP_METHOD(Redis, getPersistentID);
PHP_METHOD(Redis, getAuth);

#ifdef PHP_WIN32
#define PHP_REDIS_API __declspec(dllexport)
#else
#define PHP_REDIS_API
#endif

#ifdef ZTS
#include "TSRM.h"
#endif

PHP_MINIT_FUNCTION(redis);
PHP_MSHUTDOWN_FUNCTION(redis);
PHP_RINIT_FUNCTION(redis);
PHP_RSHUTDOWN_FUNCTION(redis);
PHP_MINFO_FUNCTION(redis);

PHPAPI int redis_connect(INTERNAL_FUNCTION_PARAMETERS, int persistent);
PHPAPI void redis_atomic_increment(INTERNAL_FUNCTION_PARAMETERS, char *keyword, int count);
PHPAPI int generic_multiple_args_cmd(INTERNAL_FUNCTION_PARAMETERS, char *keyword, int keyword_len,
									 int min_argc, RedisSock **redis_sock, int has_timeout, int all_keys, int can_serialize);
PHPAPI void generic_sort_cmd(INTERNAL_FUNCTION_PARAMETERS, char *sort, int use_alpha);
typedef void (*ResultCallback)(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock, zval *z_tab, void *ctx);
PHPAPI void generic_empty_cmd_impl(INTERNAL_FUNCTION_PARAMETERS, char *cmd, int cmd_len, ResultCallback result_callback);
PHPAPI void generic_empty_cmd(INTERNAL_FUNCTION_PARAMETERS, char *cmd, int cmd_len, ...);
PHPAPI void generic_empty_long_cmd(INTERNAL_FUNCTION_PARAMETERS, char *cmd, int cmd_len, ...);

PHPAPI void generic_subscribe_cmd(INTERNAL_FUNCTION_PARAMETERS, char *sub_cmd);
PHPAPI void generic_unsubscribe_cmd(INTERNAL_FUNCTION_PARAMETERS, char *unsub_cmd);

PHPAPI void array_zip_values_and_scores(RedisSock *redis_sock, zval *z_tab, int use_atof TSRMLS_DC);
PHPAPI int redis_response_enqueued(RedisSock *redis_sock TSRMLS_DC);

PHPAPI int get_flag(zval *object TSRMLS_DC);
PHPAPI void set_flag(zval *object, int new_flag TSRMLS_DC);

PHPAPI int redis_sock_read_multibulk_multi_reply_loop(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock, zval *z_tab, int numElems);

/* pipeline */
PHPAPI request_item* get_pipeline_head(zval *object);
PHPAPI void set_pipeline_head(zval *object, request_item *head);
PHPAPI request_item* get_pipeline_current(zval *object);
PHPAPI void set_pipeline_current(zval *object, request_item *current);

#ifndef _MSC_VER
ZEND_BEGIN_MODULE_GLOBALS(redis)
ZEND_END_MODULE_GLOBALS(redis)
#endif

struct redis_queued_item {

	/* reading function */
	zval * (*fun)(INTERNAL_FUNCTION_PARAMETERS, RedisSock *redis_sock, ...);

	char *cmd; 
	int cmd_len;

	struct redis_queued_item *next;
};

extern zend_module_entry redis_module_entry;
#define redis_module_ptr &redis_module_entry

#define phpext_redis_ptr redis_module_ptr

#define PHP_REDIS_VERSION "2.2.4"

#endif

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
