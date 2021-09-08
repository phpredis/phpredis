/* This is a generated file, edit the .stub.php file instead.
 * Stub hash: e7f3cbb6cba7b52d3cc2d8b2f311dcb37c93ea5b */

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_RedisArray___call, 0, 2, IS_MIXED, 0)
	ZEND_ARG_TYPE_INFO(0, function_name, IS_STRING, 0)
	ZEND_ARG_TYPE_INFO(0, arguments, IS_ARRAY, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_class_RedisArray___construct, 0, 0, 1)
	ZEND_ARG_TYPE_MASK(0, name_or_hosts, MAY_BE_STRING|MAY_BE_ARRAY, NULL)
	ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, options, IS_ARRAY, 0, "NULL")
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_MASK_EX(arginfo_class_RedisArray__continuum, 0, 0, MAY_BE_BOOL|MAY_BE_ARRAY)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_MASK_EX(arginfo_class_RedisArray__distributor, 0, 0, MAY_BE_BOOL|MAY_BE_CALLABLE)
ZEND_END_ARG_INFO()

#define arginfo_class_RedisArray__function arginfo_class_RedisArray__distributor

#define arginfo_class_RedisArray__hosts arginfo_class_RedisArray__continuum

ZEND_BEGIN_ARG_WITH_RETURN_OBJ_TYPE_MASK_EX(arginfo_class_RedisArray__instance, 0, 1, Redis, MAY_BE_BOOL|MAY_BE_NULL)
	ZEND_ARG_TYPE_INFO(0, host, IS_STRING, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_RedisArray__rehash, 0, 0, _IS_BOOL, 1)
	ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, fn, IS_CALLABLE, 0, "NULL")
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_MASK_EX(arginfo_class_RedisArray__target, 0, 1, MAY_BE_BOOL|MAY_BE_STRING|MAY_BE_NULL)
	ZEND_ARG_TYPE_INFO(0, key, IS_STRING, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_RedisArray_bgsave, 0, 0, IS_ARRAY, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_MASK_EX(arginfo_class_RedisArray_del, 0, 1, MAY_BE_BOOL|MAY_BE_LONG)
	ZEND_ARG_TYPE_MASK(0, key, MAY_BE_STRING|MAY_BE_ARRAY, NULL)
	ZEND_ARG_VARIADIC_TYPE_INFO(0, otherkeys, IS_STRING, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_RedisArray_discard, 0, 0, _IS_BOOL, 1)
ZEND_END_ARG_INFO()

#define arginfo_class_RedisArray_exec arginfo_class_RedisArray_discard

#define arginfo_class_RedisArray_flushall arginfo_class_RedisArray__continuum

#define arginfo_class_RedisArray_flushdb arginfo_class_RedisArray__continuum

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_MASK_EX(arginfo_class_RedisArray_getOption, 0, 1, MAY_BE_BOOL|MAY_BE_ARRAY)
	ZEND_ARG_TYPE_INFO(0, opt, IS_LONG, 0)
ZEND_END_ARG_INFO()

#define arginfo_class_RedisArray_info arginfo_class_RedisArray__continuum

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_MASK_EX(arginfo_class_RedisArray_keys, 0, 1, MAY_BE_BOOL|MAY_BE_ARRAY)
	ZEND_ARG_TYPE_INFO(0, pattern, IS_STRING, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_MASK_EX(arginfo_class_RedisArray_mget, 0, 1, MAY_BE_BOOL|MAY_BE_ARRAY)
	ZEND_ARG_TYPE_INFO(0, keys, IS_ARRAY, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_RedisArray_mset, 0, 1, _IS_BOOL, 0)
	ZEND_ARG_TYPE_INFO(0, pairs, IS_ARRAY, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_OBJ_TYPE_MASK_EX(arginfo_class_RedisArray_multi, 0, 1, RedisArray, MAY_BE_BOOL)
	ZEND_ARG_TYPE_INFO(0, host, IS_STRING, 0)
	ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, mode, IS_LONG, 0, "NULL")
ZEND_END_ARG_INFO()

#define arginfo_class_RedisArray_ping arginfo_class_RedisArray__continuum

#define arginfo_class_RedisArray_save arginfo_class_RedisArray__continuum

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_MASK_EX(arginfo_class_RedisArray_select, 0, 1, MAY_BE_BOOL|MAY_BE_ARRAY)
	ZEND_ARG_TYPE_INFO(0, index, IS_LONG, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_MASK_EX(arginfo_class_RedisArray_setOption, 0, 2, MAY_BE_BOOL|MAY_BE_ARRAY)
	ZEND_ARG_TYPE_INFO(0, opt, IS_LONG, 0)
	ZEND_ARG_TYPE_INFO(0, value, IS_STRING, 0)
ZEND_END_ARG_INFO()

#define arginfo_class_RedisArray_unlink arginfo_class_RedisArray_del

#define arginfo_class_RedisArray_unwatch arginfo_class_RedisArray_discard


ZEND_METHOD(RedisArray, __call);
ZEND_METHOD(RedisArray, __construct);
ZEND_METHOD(RedisArray, _continuum);
ZEND_METHOD(RedisArray, _distributor);
ZEND_METHOD(RedisArray, _function);
ZEND_METHOD(RedisArray, _hosts);
ZEND_METHOD(RedisArray, _instance);
ZEND_METHOD(RedisArray, _rehash);
ZEND_METHOD(RedisArray, _target);
ZEND_METHOD(RedisArray, bgsave);
ZEND_METHOD(RedisArray, del);
ZEND_METHOD(RedisArray, discard);
ZEND_METHOD(RedisArray, exec);
ZEND_METHOD(RedisArray, flushall);
ZEND_METHOD(RedisArray, flushdb);
ZEND_METHOD(RedisArray, getOption);
ZEND_METHOD(RedisArray, info);
ZEND_METHOD(RedisArray, keys);
ZEND_METHOD(RedisArray, mget);
ZEND_METHOD(RedisArray, mset);
ZEND_METHOD(RedisArray, multi);
ZEND_METHOD(RedisArray, ping);
ZEND_METHOD(RedisArray, save);
ZEND_METHOD(RedisArray, select);
ZEND_METHOD(RedisArray, setOption);
ZEND_METHOD(RedisArray, unlink);
ZEND_METHOD(RedisArray, unwatch);


static const zend_function_entry class_RedisArray_methods[] = {
	ZEND_ME(RedisArray, __call, arginfo_class_RedisArray___call, ZEND_ACC_PUBLIC)
	ZEND_ME(RedisArray, __construct, arginfo_class_RedisArray___construct, ZEND_ACC_PUBLIC)
	ZEND_ME(RedisArray, _continuum, arginfo_class_RedisArray__continuum, ZEND_ACC_PUBLIC)
	ZEND_ME(RedisArray, _distributor, arginfo_class_RedisArray__distributor, ZEND_ACC_PUBLIC)
	ZEND_ME(RedisArray, _function, arginfo_class_RedisArray__function, ZEND_ACC_PUBLIC)
	ZEND_ME(RedisArray, _hosts, arginfo_class_RedisArray__hosts, ZEND_ACC_PUBLIC)
	ZEND_ME(RedisArray, _instance, arginfo_class_RedisArray__instance, ZEND_ACC_PUBLIC)
	ZEND_ME(RedisArray, _rehash, arginfo_class_RedisArray__rehash, ZEND_ACC_PUBLIC)
	ZEND_ME(RedisArray, _target, arginfo_class_RedisArray__target, ZEND_ACC_PUBLIC)
	ZEND_ME(RedisArray, bgsave, arginfo_class_RedisArray_bgsave, ZEND_ACC_PUBLIC)
	ZEND_ME(RedisArray, del, arginfo_class_RedisArray_del, ZEND_ACC_PUBLIC)
	ZEND_ME(RedisArray, discard, arginfo_class_RedisArray_discard, ZEND_ACC_PUBLIC)
	ZEND_ME(RedisArray, exec, arginfo_class_RedisArray_exec, ZEND_ACC_PUBLIC)
	ZEND_ME(RedisArray, flushall, arginfo_class_RedisArray_flushall, ZEND_ACC_PUBLIC)
	ZEND_ME(RedisArray, flushdb, arginfo_class_RedisArray_flushdb, ZEND_ACC_PUBLIC)
	ZEND_ME(RedisArray, getOption, arginfo_class_RedisArray_getOption, ZEND_ACC_PUBLIC)
	ZEND_ME(RedisArray, info, arginfo_class_RedisArray_info, ZEND_ACC_PUBLIC)
	ZEND_ME(RedisArray, keys, arginfo_class_RedisArray_keys, ZEND_ACC_PUBLIC)
	ZEND_ME(RedisArray, mget, arginfo_class_RedisArray_mget, ZEND_ACC_PUBLIC)
	ZEND_ME(RedisArray, mset, arginfo_class_RedisArray_mset, ZEND_ACC_PUBLIC)
	ZEND_ME(RedisArray, multi, arginfo_class_RedisArray_multi, ZEND_ACC_PUBLIC)
	ZEND_ME(RedisArray, ping, arginfo_class_RedisArray_ping, ZEND_ACC_PUBLIC)
	ZEND_ME(RedisArray, save, arginfo_class_RedisArray_save, ZEND_ACC_PUBLIC)
	ZEND_ME(RedisArray, select, arginfo_class_RedisArray_select, ZEND_ACC_PUBLIC)
	ZEND_ME(RedisArray, setOption, arginfo_class_RedisArray_setOption, ZEND_ACC_PUBLIC)
	ZEND_ME(RedisArray, unlink, arginfo_class_RedisArray_unlink, ZEND_ACC_PUBLIC)
	ZEND_ME(RedisArray, unwatch, arginfo_class_RedisArray_unwatch, ZEND_ACC_PUBLIC)
	ZEND_FE_END
};
