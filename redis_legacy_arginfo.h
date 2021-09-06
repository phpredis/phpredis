/* This is a generated file, edit the .stub.php file instead.
 * Stub hash: 19c4026366635e6429dd7a7c17fbe8cdfe6b00e0 */

ZEND_BEGIN_ARG_INFO_EX(arginfo_class_Redis___construct, 0, 0, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_class_Redis__compress, 0, 0, 1)
	ZEND_ARG_INFO(0, value)
ZEND_END_ARG_INFO()

#define arginfo_class_Redis___destruct arginfo_class_Redis___construct

#define arginfo_class_Redis__pack arginfo_class_Redis__compress

ZEND_BEGIN_ARG_INFO_EX(arginfo_class_Redis__prefix, 0, 0, 1)
	ZEND_ARG_INFO(0, key)
ZEND_END_ARG_INFO()

#define arginfo_class_Redis__serialize arginfo_class_Redis__compress

#define arginfo_class_Redis__uncompress arginfo_class_Redis__compress

#define arginfo_class_Redis__unpack arginfo_class_Redis__compress

#define arginfo_class_Redis__unserialize arginfo_class_Redis__compress

ZEND_BEGIN_ARG_INFO_EX(arginfo_class_Redis_acl, 0, 0, 1)
	ZEND_ARG_INFO(0, subcmd)
	ZEND_ARG_VARIADIC_INFO(0, args)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_class_Redis_append, 0, 0, 2)
	ZEND_ARG_INFO(0, key)
	ZEND_ARG_INFO(0, value)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_class_Redis_auth, 0, 0, 1)
	ZEND_ARG_INFO(0, credentials)
ZEND_END_ARG_INFO()

#define arginfo_class_Redis_bgSave arginfo_class_Redis___construct

#define arginfo_class_Redis_bgrewriteaof arginfo_class_Redis___construct

ZEND_BEGIN_ARG_INFO_EX(arginfo_class_Redis_bitcount, 0, 0, 1)
	ZEND_ARG_INFO(0, key)
	ZEND_ARG_INFO(0, start)
	ZEND_ARG_INFO(0, end)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_class_Redis_bitop, 0, 0, 3)
	ZEND_ARG_INFO(0, operation)
	ZEND_ARG_INFO(0, deskey)
	ZEND_ARG_INFO(0, srckey)
	ZEND_ARG_VARIADIC_INFO(0, otherkeys)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_class_Redis_bitpos, 0, 0, 2)
	ZEND_ARG_INFO(0, key)
	ZEND_ARG_INFO(0, bit)
	ZEND_ARG_INFO(0, start)
	ZEND_ARG_INFO(0, end)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_class_Redis_blPop, 0, 0, 2)
	ZEND_ARG_INFO(0, key)
	ZEND_ARG_INFO(0, timeout_or_key)
	ZEND_ARG_VARIADIC_INFO(0, extra_args)
ZEND_END_ARG_INFO()

#define arginfo_class_Redis_brPop arginfo_class_Redis_blPop

ZEND_BEGIN_ARG_INFO_EX(arginfo_class_Redis_brpoplpush, 0, 0, 3)
	ZEND_ARG_INFO(0, src)
	ZEND_ARG_INFO(0, dst)
	ZEND_ARG_INFO(0, timeout)
ZEND_END_ARG_INFO()

#define arginfo_class_Redis_bzPopMax arginfo_class_Redis_blPop

#define arginfo_class_Redis_bzPopMin arginfo_class_Redis_blPop

#define arginfo_class_Redis_clearLastError arginfo_class_Redis___construct

ZEND_BEGIN_ARG_INFO_EX(arginfo_class_Redis_client, 0, 0, 1)
	ZEND_ARG_INFO(0, opt)
	ZEND_ARG_INFO(0, arg)
ZEND_END_ARG_INFO()

#define arginfo_class_Redis_close arginfo_class_Redis___construct

ZEND_BEGIN_ARG_INFO_EX(arginfo_class_Redis_command, 0, 0, 2)
	ZEND_ARG_INFO(0, opt)
	ZEND_ARG_INFO(0, arg)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_class_Redis_config, 0, 0, 2)
	ZEND_ARG_INFO(0, operation)
	ZEND_ARG_INFO(0, key)
	ZEND_ARG_INFO(0, value)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_class_Redis_connect, 0, 0, 1)
	ZEND_ARG_INFO(0, host)
	ZEND_ARG_INFO(0, port)
	ZEND_ARG_INFO(0, timeout)
	ZEND_ARG_INFO(0, persistent_id)
	ZEND_ARG_INFO(0, retry_interval)
	ZEND_ARG_INFO(0, read_timeout)
	ZEND_ARG_INFO(0, context)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_class_Redis_copy, 0, 0, 2)
	ZEND_ARG_INFO(0, src)
	ZEND_ARG_INFO(0, dst)
	ZEND_ARG_INFO(0, options)
ZEND_END_ARG_INFO()

#define arginfo_class_Redis_dbSize arginfo_class_Redis___construct

#define arginfo_class_Redis_debug arginfo_class_Redis__prefix

#define arginfo_class_Redis_decr arginfo_class_Redis__prefix

#define arginfo_class_Redis_decrBy arginfo_class_Redis_append

ZEND_BEGIN_ARG_INFO_EX(arginfo_class_Redis_del, 0, 0, 1)
	ZEND_ARG_INFO(0, key)
	ZEND_ARG_VARIADIC_INFO(0, otherkeys)
ZEND_END_ARG_INFO()

#define arginfo_class_Redis_delete arginfo_class_Redis_del

#define arginfo_class_Redis_discard arginfo_class_Redis___construct

#define arginfo_class_Redis_dump arginfo_class_Redis__prefix

ZEND_BEGIN_ARG_INFO_EX(arginfo_class_Redis_eval, 0, 0, 1)
	ZEND_ARG_INFO(0, script)
	ZEND_ARG_INFO(0, keys)
	ZEND_ARG_INFO(0, num_keys)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_class_Redis_evalsha, 0, 0, 1)
	ZEND_ARG_INFO(0, sha1)
	ZEND_ARG_INFO(0, keys)
	ZEND_ARG_INFO(0, num_keys)
ZEND_END_ARG_INFO()

#define arginfo_class_Redis_exec arginfo_class_Redis___construct

#define arginfo_class_Redis_exists arginfo_class_Redis__prefix

ZEND_BEGIN_ARG_INFO_EX(arginfo_class_Redis_expire, 0, 0, 2)
	ZEND_ARG_INFO(0, key)
	ZEND_ARG_INFO(0, timeout)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_class_Redis_expireAt, 0, 0, 2)
	ZEND_ARG_INFO(0, key)
	ZEND_ARG_INFO(0, timestamp)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_class_Redis_flushAll, 0, 0, 0)
	ZEND_ARG_INFO(0, async)
ZEND_END_ARG_INFO()

#define arginfo_class_Redis_flushDB arginfo_class_Redis_flushAll

#define arginfo_class_Redis_pconnect arginfo_class_Redis_connect

ZEND_BEGIN_ARG_INFO_EX(arginfo_class_Redis_set, 0, 0, 2)
	ZEND_ARG_INFO(0, key)
	ZEND_ARG_INFO(0, value)
	ZEND_ARG_INFO(0, opt)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_class_Redis_setex, 0, 0, 3)
	ZEND_ARG_INFO(0, key)
	ZEND_ARG_INFO(0, expire)
	ZEND_ARG_INFO(0, value)
ZEND_END_ARG_INFO()

#define arginfo_class_Redis_psetex arginfo_class_Redis_setex

#define arginfo_class_Redis_setnx arginfo_class_Redis_append

#define arginfo_class_Redis_getset arginfo_class_Redis_append

#define arginfo_class_Redis_randomKey arginfo_class_Redis___construct

ZEND_BEGIN_ARG_INFO_EX(arginfo_class_Redis_echo, 0, 0, 1)
	ZEND_ARG_INFO(0, str)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_class_Redis_rename, 0, 0, 2)
	ZEND_ARG_INFO(0, key_src)
	ZEND_ARG_INFO(0, key_dst)
ZEND_END_ARG_INFO()

#define arginfo_class_Redis_renameNx arginfo_class_Redis_rename

#define arginfo_class_Redis_get arginfo_class_Redis__prefix

ZEND_BEGIN_ARG_INFO_EX(arginfo_class_Redis_ping, 0, 0, 0)
	ZEND_ARG_INFO(0, key)
ZEND_END_ARG_INFO()

#define arginfo_class_Redis_incr arginfo_class_Redis__prefix

#define arginfo_class_Redis_incrBy arginfo_class_Redis_append

#define arginfo_class_Redis_incrByFloat arginfo_class_Redis_append

ZEND_BEGIN_ARG_INFO_EX(arginfo_class_Redis_info, 0, 0, 0)
	ZEND_ARG_INFO(0, opt)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_class_Redis_mget, 0, 0, 1)
	ZEND_ARG_INFO(0, keys)
ZEND_END_ARG_INFO()

#define arginfo_class_Redis_unlink arginfo_class_Redis_del

#define arginfo_class_Redis_watch arginfo_class_Redis_del

#define arginfo_class_Redis_unwatch arginfo_class_Redis___construct

ZEND_BEGIN_ARG_INFO_EX(arginfo_class_Redis_keys, 0, 0, 1)
	ZEND_ARG_INFO(0, pattern)
ZEND_END_ARG_INFO()

#define arginfo_class_Redis_type arginfo_class_Redis__prefix

ZEND_BEGIN_ARG_INFO_EX(arginfo_class_Redis_getRange, 0, 0, 3)
	ZEND_ARG_INFO(0, key)
	ZEND_ARG_INFO(0, start)
	ZEND_ARG_INFO(0, end)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_class_Redis_setRange, 0, 0, 3)
	ZEND_ARG_INFO(0, key)
	ZEND_ARG_INFO(0, start)
	ZEND_ARG_INFO(0, value)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_class_Redis_getBit, 0, 0, 2)
	ZEND_ARG_INFO(0, key)
	ZEND_ARG_INFO(0, idx)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_class_Redis_setBit, 0, 0, 3)
	ZEND_ARG_INFO(0, key)
	ZEND_ARG_INFO(0, idx)
	ZEND_ARG_INFO(0, value)
ZEND_END_ARG_INFO()

#define arginfo_class_Redis_strlen arginfo_class_Redis__prefix

ZEND_BEGIN_ARG_INFO_EX(arginfo_class_Redis_lPush, 0, 0, 1)
	ZEND_ARG_INFO(0, key)
	ZEND_ARG_VARIADIC_INFO(0, elements)
ZEND_END_ARG_INFO()

#define arginfo_class_Redis_rPush arginfo_class_Redis_lPush

ZEND_BEGIN_ARG_INFO_EX(arginfo_class_Redis_lInsert, 0, 0, 4)
	ZEND_ARG_INFO(0, key)
	ZEND_ARG_INFO(0, pos)
	ZEND_ARG_INFO(0, pivot)
	ZEND_ARG_INFO(0, value)
ZEND_END_ARG_INFO()

#define arginfo_class_Redis_lPushx arginfo_class_Redis_append

#define arginfo_class_Redis_rPushx arginfo_class_Redis_append

#define arginfo_class_Redis_lPop arginfo_class_Redis__prefix

#define arginfo_class_Redis_rPop arginfo_class_Redis__prefix

#define arginfo_class_Redis_open arginfo_class_Redis_connect

#define arginfo_class_Redis_popen arginfo_class_Redis_connect


ZEND_METHOD(Redis, __construct);
ZEND_METHOD(Redis, _compress);
ZEND_METHOD(Redis, __destruct);
ZEND_METHOD(Redis, _pack);
ZEND_METHOD(Redis, _prefix);
ZEND_METHOD(Redis, _serialize);
ZEND_METHOD(Redis, _uncompress);
ZEND_METHOD(Redis, _unpack);
ZEND_METHOD(Redis, _unserialize);
ZEND_METHOD(Redis, acl);
ZEND_METHOD(Redis, append);
ZEND_METHOD(Redis, auth);
ZEND_METHOD(Redis, bgSave);
ZEND_METHOD(Redis, bgrewriteaof);
ZEND_METHOD(Redis, bitcount);
ZEND_METHOD(Redis, bitop);
ZEND_METHOD(Redis, bitpos);
ZEND_METHOD(Redis, blPop);
ZEND_METHOD(Redis, brPop);
ZEND_METHOD(Redis, brpoplpush);
ZEND_METHOD(Redis, bzPopMax);
ZEND_METHOD(Redis, bzPopMin);
ZEND_METHOD(Redis, clearLastError);
ZEND_METHOD(Redis, client);
ZEND_METHOD(Redis, close);
ZEND_METHOD(Redis, command);
ZEND_METHOD(Redis, config);
ZEND_METHOD(Redis, connect);
ZEND_METHOD(Redis, copy);
ZEND_METHOD(Redis, dbSize);
ZEND_METHOD(Redis, debug);
ZEND_METHOD(Redis, decr);
ZEND_METHOD(Redis, decrBy);
ZEND_METHOD(Redis, del);
ZEND_METHOD(Redis, discard);
ZEND_METHOD(Redis, dump);
ZEND_METHOD(Redis, eval);
ZEND_METHOD(Redis, evalsha);
ZEND_METHOD(Redis, exec);
ZEND_METHOD(Redis, exists);
ZEND_METHOD(Redis, expire);
ZEND_METHOD(Redis, expireAt);
ZEND_METHOD(Redis, flushAll);
ZEND_METHOD(Redis, flushDB);
ZEND_METHOD(Redis, pconnect);
ZEND_METHOD(Redis, set);
ZEND_METHOD(Redis, setex);
ZEND_METHOD(Redis, psetex);
ZEND_METHOD(Redis, setnx);
ZEND_METHOD(Redis, getset);
ZEND_METHOD(Redis, randomKey);
ZEND_METHOD(Redis, echo);
ZEND_METHOD(Redis, rename);
ZEND_METHOD(Redis, renameNx);
ZEND_METHOD(Redis, get);
ZEND_METHOD(Redis, ping);
ZEND_METHOD(Redis, incr);
ZEND_METHOD(Redis, incrBy);
ZEND_METHOD(Redis, incrByFloat);
ZEND_METHOD(Redis, info);
ZEND_METHOD(Redis, mget);
ZEND_METHOD(Redis, unlink);
ZEND_METHOD(Redis, watch);
ZEND_METHOD(Redis, unwatch);
ZEND_METHOD(Redis, keys);
ZEND_METHOD(Redis, type);
ZEND_METHOD(Redis, getRange);
ZEND_METHOD(Redis, setRange);
ZEND_METHOD(Redis, getBit);
ZEND_METHOD(Redis, setBit);
ZEND_METHOD(Redis, strlen);
ZEND_METHOD(Redis, lPush);
ZEND_METHOD(Redis, rPush);
ZEND_METHOD(Redis, lInsert);
ZEND_METHOD(Redis, lPushx);
ZEND_METHOD(Redis, rPushx);
ZEND_METHOD(Redis, lPop);
ZEND_METHOD(Redis, rPop);


static const zend_function_entry class_Redis_methods[] = {
	ZEND_ME(Redis, __construct, arginfo_class_Redis___construct, ZEND_ACC_PUBLIC)
	ZEND_ME(Redis, _compress, arginfo_class_Redis__compress, ZEND_ACC_PUBLIC)
	ZEND_ME(Redis, __destruct, arginfo_class_Redis___destruct, ZEND_ACC_PUBLIC)
	ZEND_ME(Redis, _pack, arginfo_class_Redis__pack, ZEND_ACC_PUBLIC)
	ZEND_ME(Redis, _prefix, arginfo_class_Redis__prefix, ZEND_ACC_PUBLIC)
	ZEND_ME(Redis, _serialize, arginfo_class_Redis__serialize, ZEND_ACC_PUBLIC)
	ZEND_ME(Redis, _uncompress, arginfo_class_Redis__uncompress, ZEND_ACC_PUBLIC)
	ZEND_ME(Redis, _unpack, arginfo_class_Redis__unpack, ZEND_ACC_PUBLIC)
	ZEND_ME(Redis, _unserialize, arginfo_class_Redis__unserialize, ZEND_ACC_PUBLIC)
	ZEND_ME(Redis, acl, arginfo_class_Redis_acl, ZEND_ACC_PUBLIC)
	ZEND_ME(Redis, append, arginfo_class_Redis_append, ZEND_ACC_PUBLIC)
	ZEND_ME(Redis, auth, arginfo_class_Redis_auth, ZEND_ACC_PUBLIC)
	ZEND_ME(Redis, bgSave, arginfo_class_Redis_bgSave, ZEND_ACC_PUBLIC)
	ZEND_ME(Redis, bgrewriteaof, arginfo_class_Redis_bgrewriteaof, ZEND_ACC_PUBLIC)
	ZEND_ME(Redis, bitcount, arginfo_class_Redis_bitcount, ZEND_ACC_PUBLIC)
	ZEND_ME(Redis, bitop, arginfo_class_Redis_bitop, ZEND_ACC_PUBLIC)
	ZEND_ME(Redis, bitpos, arginfo_class_Redis_bitpos, ZEND_ACC_PUBLIC)
	ZEND_ME(Redis, blPop, arginfo_class_Redis_blPop, ZEND_ACC_PUBLIC)
	ZEND_ME(Redis, brPop, arginfo_class_Redis_brPop, ZEND_ACC_PUBLIC)
	ZEND_ME(Redis, brpoplpush, arginfo_class_Redis_brpoplpush, ZEND_ACC_PUBLIC)
	ZEND_ME(Redis, bzPopMax, arginfo_class_Redis_bzPopMax, ZEND_ACC_PUBLIC)
	ZEND_ME(Redis, bzPopMin, arginfo_class_Redis_bzPopMin, ZEND_ACC_PUBLIC)
	ZEND_ME(Redis, clearLastError, arginfo_class_Redis_clearLastError, ZEND_ACC_PUBLIC)
	ZEND_ME(Redis, client, arginfo_class_Redis_client, ZEND_ACC_PUBLIC)
	ZEND_ME(Redis, close, arginfo_class_Redis_close, ZEND_ACC_PUBLIC)
	ZEND_ME(Redis, command, arginfo_class_Redis_command, ZEND_ACC_PUBLIC)
	ZEND_ME(Redis, config, arginfo_class_Redis_config, ZEND_ACC_PUBLIC)
	ZEND_ME(Redis, connect, arginfo_class_Redis_connect, ZEND_ACC_PUBLIC)
	ZEND_ME(Redis, copy, arginfo_class_Redis_copy, ZEND_ACC_PUBLIC)
	ZEND_ME(Redis, dbSize, arginfo_class_Redis_dbSize, ZEND_ACC_PUBLIC)
	ZEND_ME(Redis, debug, arginfo_class_Redis_debug, ZEND_ACC_PUBLIC)
	ZEND_ME(Redis, decr, arginfo_class_Redis_decr, ZEND_ACC_PUBLIC)
	ZEND_ME(Redis, decrBy, arginfo_class_Redis_decrBy, ZEND_ACC_PUBLIC)
	ZEND_ME(Redis, del, arginfo_class_Redis_del, ZEND_ACC_PUBLIC)
	ZEND_MALIAS(Redis, delete, del, arginfo_class_Redis_delete, ZEND_ACC_PUBLIC|ZEND_ACC_DEPRECATED)
	ZEND_ME(Redis, discard, arginfo_class_Redis_discard, ZEND_ACC_PUBLIC)
	ZEND_ME(Redis, dump, arginfo_class_Redis_dump, ZEND_ACC_PUBLIC)
	ZEND_ME(Redis, eval, arginfo_class_Redis_eval, ZEND_ACC_PUBLIC)
	ZEND_ME(Redis, evalsha, arginfo_class_Redis_evalsha, ZEND_ACC_PUBLIC)
	ZEND_ME(Redis, exec, arginfo_class_Redis_exec, ZEND_ACC_PUBLIC)
	ZEND_ME(Redis, exists, arginfo_class_Redis_exists, ZEND_ACC_PUBLIC)
	ZEND_ME(Redis, expire, arginfo_class_Redis_expire, ZEND_ACC_PUBLIC)
	ZEND_ME(Redis, expireAt, arginfo_class_Redis_expireAt, ZEND_ACC_PUBLIC)
	ZEND_ME(Redis, flushAll, arginfo_class_Redis_flushAll, ZEND_ACC_PUBLIC)
	ZEND_ME(Redis, flushDB, arginfo_class_Redis_flushDB, ZEND_ACC_PUBLIC)
	ZEND_ME(Redis, pconnect, arginfo_class_Redis_pconnect, ZEND_ACC_PUBLIC)
	ZEND_ME(Redis, set, arginfo_class_Redis_set, ZEND_ACC_PUBLIC)
	ZEND_ME(Redis, setex, arginfo_class_Redis_setex, ZEND_ACC_PUBLIC)
	ZEND_ME(Redis, psetex, arginfo_class_Redis_psetex, ZEND_ACC_PUBLIC)
	ZEND_ME(Redis, setnx, arginfo_class_Redis_setnx, ZEND_ACC_PUBLIC)
	ZEND_ME(Redis, getset, arginfo_class_Redis_getset, ZEND_ACC_PUBLIC)
	ZEND_ME(Redis, randomKey, arginfo_class_Redis_randomKey, ZEND_ACC_PUBLIC)
	ZEND_ME(Redis, echo, arginfo_class_Redis_echo, ZEND_ACC_PUBLIC)
	ZEND_ME(Redis, rename, arginfo_class_Redis_rename, ZEND_ACC_PUBLIC)
	ZEND_ME(Redis, renameNx, arginfo_class_Redis_renameNx, ZEND_ACC_PUBLIC)
	ZEND_ME(Redis, get, arginfo_class_Redis_get, ZEND_ACC_PUBLIC)
	ZEND_ME(Redis, ping, arginfo_class_Redis_ping, ZEND_ACC_PUBLIC)
	ZEND_ME(Redis, incr, arginfo_class_Redis_incr, ZEND_ACC_PUBLIC)
	ZEND_ME(Redis, incrBy, arginfo_class_Redis_incrBy, ZEND_ACC_PUBLIC)
	ZEND_ME(Redis, incrByFloat, arginfo_class_Redis_incrByFloat, ZEND_ACC_PUBLIC)
	ZEND_ME(Redis, info, arginfo_class_Redis_info, ZEND_ACC_PUBLIC)
	ZEND_ME(Redis, mget, arginfo_class_Redis_mget, ZEND_ACC_PUBLIC)
	ZEND_ME(Redis, unlink, arginfo_class_Redis_unlink, ZEND_ACC_PUBLIC)
	ZEND_ME(Redis, watch, arginfo_class_Redis_watch, ZEND_ACC_PUBLIC)
	ZEND_ME(Redis, unwatch, arginfo_class_Redis_unwatch, ZEND_ACC_PUBLIC)
	ZEND_ME(Redis, keys, arginfo_class_Redis_keys, ZEND_ACC_PUBLIC)
	ZEND_ME(Redis, type, arginfo_class_Redis_type, ZEND_ACC_PUBLIC)
	ZEND_ME(Redis, getRange, arginfo_class_Redis_getRange, ZEND_ACC_PUBLIC)
	ZEND_ME(Redis, setRange, arginfo_class_Redis_setRange, ZEND_ACC_PUBLIC)
	ZEND_ME(Redis, getBit, arginfo_class_Redis_getBit, ZEND_ACC_PUBLIC)
	ZEND_ME(Redis, setBit, arginfo_class_Redis_setBit, ZEND_ACC_PUBLIC)
	ZEND_ME(Redis, strlen, arginfo_class_Redis_strlen, ZEND_ACC_PUBLIC)
	ZEND_ME(Redis, lPush, arginfo_class_Redis_lPush, ZEND_ACC_PUBLIC)
	ZEND_ME(Redis, rPush, arginfo_class_Redis_rPush, ZEND_ACC_PUBLIC)
	ZEND_ME(Redis, lInsert, arginfo_class_Redis_lInsert, ZEND_ACC_PUBLIC)
	ZEND_ME(Redis, lPushx, arginfo_class_Redis_lPushx, ZEND_ACC_PUBLIC)
	ZEND_ME(Redis, rPushx, arginfo_class_Redis_rPushx, ZEND_ACC_PUBLIC)
	ZEND_ME(Redis, lPop, arginfo_class_Redis_lPop, ZEND_ACC_PUBLIC)
	ZEND_ME(Redis, rPop, arginfo_class_Redis_rPop, ZEND_ACC_PUBLIC)
	ZEND_MALIAS(Redis, open, connect, arginfo_class_Redis_open, ZEND_ACC_PUBLIC|ZEND_ACC_DEPRECATED)
	ZEND_MALIAS(Redis, popen, pconnect, arginfo_class_Redis_popen, ZEND_ACC_PUBLIC|ZEND_ACC_DEPRECATED)
	ZEND_FE_END
};
