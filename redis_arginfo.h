/* This is a generated file, edit the .stub.php file instead.
 * Stub hash: be75361e8e76c8a25455d7c40a36735b56a9e8a0 */

ZEND_BEGIN_ARG_INFO_EX(arginfo_class_Redis___construct, 0, 0, 0)
ZEND_END_ARG_INFO()

#define arginfo_class_Redis___destruct arginfo_class_Redis___construct

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_Redis_connect, 0, 1, _IS_BOOL, 0)
	ZEND_ARG_TYPE_INFO(0, host, IS_STRING, 0)
	ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, port, IS_LONG, 0, "26379")
	ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, timeout, IS_DOUBLE, 0, "0")
	ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, persistent_id, IS_STRING, 0, "NULL")
	ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, retry_interval, IS_LONG, 0, "0")
	ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, read_timeout, IS_DOUBLE, 0, "0")
	ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, context, IS_ARRAY, 0, "NULL")
ZEND_END_ARG_INFO()

#define arginfo_class_Redis_pconnect arginfo_class_Redis_connect

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_Redis_bitop, 0, 3, IS_LONG, 0)
	ZEND_ARG_TYPE_INFO(0, operation, IS_STRING, 0)
	ZEND_ARG_TYPE_INFO(0, deskey, IS_STRING, 0)
	ZEND_ARG_TYPE_INFO(0, srckey, IS_STRING, 0)
	ZEND_ARG_VARIADIC_INFO(0, otherkeys)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_class_Redis_bitcount, 0, 0, 1)
	ZEND_ARG_TYPE_INFO(0, key, IS_STRING, 0)
	ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, start, IS_LONG, 0, "0")
	ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, end, IS_LONG, 0, "-1")
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_class_Redis_bitpos, 0, 0, 2)
	ZEND_ARG_TYPE_INFO(0, key, IS_STRING, 0)
	ZEND_ARG_TYPE_INFO(0, bit, IS_LONG, 0)
	ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, start, IS_LONG, 0, "0")
	ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, end, IS_LONG, 0, "-1")
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_Redis_close, 0, 0, _IS_BOOL, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_class_Redis_set, 0, 0, 2)
	ZEND_ARG_TYPE_INFO(0, key, IS_STRING, 0)
	ZEND_ARG_TYPE_INFO(0, value, IS_MIXED, 0)
	ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, opt, IS_MIXED, 0, "NULL")
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_class_Redis_setex, 0, 0, 3)
	ZEND_ARG_TYPE_INFO(0, key, IS_STRING, 0)
	ZEND_ARG_TYPE_INFO(0, expire, IS_LONG, 0)
	ZEND_ARG_TYPE_INFO(0, value, IS_MIXED, 0)
ZEND_END_ARG_INFO()

#define arginfo_class_Redis_psetex arginfo_class_Redis_setex

ZEND_BEGIN_ARG_INFO_EX(arginfo_class_Redis_setnx, 0, 0, 2)
	ZEND_ARG_TYPE_INFO(0, key, IS_STRING, 0)
	ZEND_ARG_TYPE_INFO(0, value, IS_MIXED, 0)
ZEND_END_ARG_INFO()

#define arginfo_class_Redis_getset arginfo_class_Redis_setnx

#define arginfo_class_Redis_randomKey arginfo_class_Redis___construct

ZEND_BEGIN_ARG_INFO_EX(arginfo_class_Redis_echo, 0, 0, 1)
	ZEND_ARG_TYPE_INFO(0, str, IS_STRING, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_class_Redis_rename, 0, 0, 2)
	ZEND_ARG_TYPE_INFO(0, key_src, IS_STRING, 0)
	ZEND_ARG_TYPE_INFO(0, key_dst, IS_STRING, 0)
ZEND_END_ARG_INFO()

#define arginfo_class_Redis_renameNx arginfo_class_Redis_rename

ZEND_BEGIN_ARG_INFO_EX(arginfo_class_Redis_get, 0, 0, 1)
	ZEND_ARG_TYPE_INFO(0, key, IS_STRING, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_class_Redis_ping, 0, 0, 0)
	ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, key, IS_STRING, 0, "NULL")
ZEND_END_ARG_INFO()

#define arginfo_class_Redis_incr arginfo_class_Redis_get

ZEND_BEGIN_ARG_INFO_EX(arginfo_class_Redis_incrBy, 0, 0, 2)
	ZEND_ARG_TYPE_INFO(0, key, IS_STRING, 0)
	ZEND_ARG_TYPE_INFO(0, value, IS_LONG, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_class_Redis_incrByFloat, 0, 0, 2)
	ZEND_ARG_TYPE_INFO(0, key, IS_STRING, 0)
	ZEND_ARG_TYPE_INFO(0, value, IS_DOUBLE, 0)
ZEND_END_ARG_INFO()

#define arginfo_class_Redis_decr arginfo_class_Redis_get

#define arginfo_class_Redis_decrBy arginfo_class_Redis_incrBy

ZEND_BEGIN_ARG_INFO_EX(arginfo_class_Redis_mget, 0, 0, 1)
	ZEND_ARG_TYPE_INFO(0, keys, IS_ARRAY, 0)
ZEND_END_ARG_INFO()

#define arginfo_class_Redis_exists arginfo_class_Redis_get

ZEND_BEGIN_ARG_INFO_EX(arginfo_class_Redis_del, 0, 0, 1)
	ZEND_ARG_TYPE_MASK(0, key, MAY_BE_ARRAY|MAY_BE_STRING, NULL)
	ZEND_ARG_VARIADIC_INFO(0, otherkeys)
ZEND_END_ARG_INFO()

#define arginfo_class_Redis_unlink arginfo_class_Redis_del

#define arginfo_class_Redis_watch arginfo_class_Redis_del

#define arginfo_class_Redis_unwatch arginfo_class_Redis___construct

ZEND_BEGIN_ARG_INFO_EX(arginfo_class_Redis_keys, 0, 0, 1)
	ZEND_ARG_TYPE_INFO(0, pattern, IS_STRING, 0)
ZEND_END_ARG_INFO()

#define arginfo_class_Redis_keys arginfo_class_Redis_get

ZEND_BEGIN_ARG_INFO_EX(arginfo_class_Redis_acl, 0, 0, 1)
	ZEND_ARG_TYPE_INFO(0, subcmd, IS_STRING, 0)
	ZEND_ARG_VARIADIC_INFO(0, args)
ZEND_END_ARG_INFO()

#define arginfo_class_Redis_delete arginfo_class_Redis_del

#define arginfo_class_Redis_open arginfo_class_Redis_connect

#define arginfo_class_Redis_popen arginfo_class_Redis_connect


ZEND_METHOD(Redis, __construct);
ZEND_METHOD(Redis, __destruct);
ZEND_METHOD(Redis, connect);
ZEND_METHOD(Redis, pconnect);
ZEND_METHOD(Redis, bitop);
ZEND_METHOD(Redis, bitcount);
ZEND_METHOD(Redis, bitpos);
ZEND_METHOD(Redis, close);
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
ZEND_METHOD(Redis, decr);
ZEND_METHOD(Redis, decrBy);
ZEND_METHOD(Redis, mget);
ZEND_METHOD(Redis, exists);
ZEND_METHOD(Redis, del);
ZEND_METHOD(Redis, unlink);
ZEND_METHOD(Redis, watch);
ZEND_METHOD(Redis, unwatch);
ZEND_METHOD(Redis, keys);
ZEND_METHOD(Redis, acl);


static const zend_function_entry class_Redis_methods[] = {
	ZEND_ME(Redis, __construct, arginfo_class_Redis___construct, ZEND_ACC_PUBLIC)
	ZEND_ME(Redis, __destruct, arginfo_class_Redis___destruct, ZEND_ACC_PUBLIC)
	ZEND_ME(Redis, connect, arginfo_class_Redis_connect, ZEND_ACC_PUBLIC)
	ZEND_ME(Redis, pconnect, arginfo_class_Redis_pconnect, ZEND_ACC_PUBLIC)
	ZEND_ME(Redis, bitop, arginfo_class_Redis_bitop, ZEND_ACC_PUBLIC)
	ZEND_ME(Redis, bitcount, arginfo_class_Redis_bitcount, ZEND_ACC_PUBLIC)
	ZEND_ME(Redis, bitpos, arginfo_class_Redis_bitpos, ZEND_ACC_PUBLIC)
	ZEND_ME(Redis, close, arginfo_class_Redis_close, ZEND_ACC_PUBLIC)
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
	ZEND_ME(Redis, decr, arginfo_class_Redis_decr, ZEND_ACC_PUBLIC)
	ZEND_ME(Redis, decrBy, arginfo_class_Redis_decrBy, ZEND_ACC_PUBLIC)
	ZEND_ME(Redis, mget, arginfo_class_Redis_mget, ZEND_ACC_PUBLIC)
	ZEND_ME(Redis, exists, arginfo_class_Redis_exists, ZEND_ACC_PUBLIC)
	ZEND_ME(Redis, del, arginfo_class_Redis_del, ZEND_ACC_PUBLIC)
	ZEND_ME(Redis, unlink, arginfo_class_Redis_unlink, ZEND_ACC_PUBLIC)
	ZEND_ME(Redis, watch, arginfo_class_Redis_watch, ZEND_ACC_PUBLIC)
	ZEND_ME(Redis, unwatch, arginfo_class_Redis_unwatch, ZEND_ACC_PUBLIC)
	ZEND_ME(Redis, keys, arginfo_class_Redis_keys, ZEND_ACC_PUBLIC)
	ZEND_ME(Redis, keys, arginfo_class_Redis_keys, ZEND_ACC_PUBLIC)
	ZEND_ME(Redis, acl, arginfo_class_Redis_acl, ZEND_ACC_PUBLIC)
	ZEND_MALIAS(Redis, delete, del, arginfo_class_Redis_delete, ZEND_ACC_PUBLIC|ZEND_ACC_DEPRECATED)
	ZEND_MALIAS(Redis, open, connect, arginfo_class_Redis_open, ZEND_ACC_PUBLIC|ZEND_ACC_DEPRECATED)
	ZEND_MALIAS(Redis, popen, pconnect, arginfo_class_Redis_popen, ZEND_ACC_PUBLIC|ZEND_ACC_DEPRECATED)
	ZEND_FE_END
};
