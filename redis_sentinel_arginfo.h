/* This is a generated file, edit the .stub.php file instead.
 * Stub hash: 847c735dfbbb643366344acfe6e2c5e8b76d0520 */

ZEND_BEGIN_ARG_INFO_EX(arginfo_class_RedisSentinel___construct, 0, 0, 1)
	ZEND_ARG_TYPE_INFO(0, host, IS_STRING, 0)
	ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, port, IS_LONG, 0, "26379")
	ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, timeout, IS_DOUBLE, 0, "0")
	ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, persistent, IS_MIXED, 0, "null")
	ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, retry_interval, IS_LONG, 0, "0")
	ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, read_timeout, IS_DOUBLE, 0, "0")
	ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, auth, IS_MIXED, 0, "null")
	ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, context, IS_ARRAY, 0, "null")
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_class_RedisSentinel_ckquorum, 0, 0, 1)
	ZEND_ARG_TYPE_INFO(0, master, IS_STRING, 0)
ZEND_END_ARG_INFO()

#define arginfo_class_RedisSentinel_failover arginfo_class_RedisSentinel_ckquorum

ZEND_BEGIN_ARG_INFO_EX(arginfo_class_RedisSentinel_flushconfig, 0, 0, 0)
ZEND_END_ARG_INFO()

#define arginfo_class_RedisSentinel_getMasterAddrByName arginfo_class_RedisSentinel_ckquorum

#define arginfo_class_RedisSentinel_master arginfo_class_RedisSentinel_ckquorum

#define arginfo_class_RedisSentinel_masters arginfo_class_RedisSentinel_flushconfig

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_RedisSentinel_myid, 0, 0, IS_STRING, 0)
ZEND_END_ARG_INFO()

#define arginfo_class_RedisSentinel_ping arginfo_class_RedisSentinel_flushconfig

ZEND_BEGIN_ARG_INFO_EX(arginfo_class_RedisSentinel_reset, 0, 0, 1)
	ZEND_ARG_TYPE_INFO(0, pattern, IS_STRING, 0)
ZEND_END_ARG_INFO()

#define arginfo_class_RedisSentinel_sentinels arginfo_class_RedisSentinel_ckquorum

#define arginfo_class_RedisSentinel_slaves arginfo_class_RedisSentinel_ckquorum


ZEND_METHOD(RedisSentinel, __construct);
ZEND_METHOD(RedisSentinel, ckquorum);
ZEND_METHOD(RedisSentinel, failover);
ZEND_METHOD(RedisSentinel, flushconfig);
ZEND_METHOD(RedisSentinel, getMasterAddrByName);
ZEND_METHOD(RedisSentinel, master);
ZEND_METHOD(RedisSentinel, masters);
ZEND_METHOD(RedisSentinel, myid);
ZEND_METHOD(RedisSentinel, ping);
ZEND_METHOD(RedisSentinel, reset);
ZEND_METHOD(RedisSentinel, sentinels);
ZEND_METHOD(RedisSentinel, slaves);


static const zend_function_entry class_RedisSentinel_methods[] = {
	ZEND_ME(RedisSentinel, __construct, arginfo_class_RedisSentinel___construct, ZEND_ACC_PUBLIC)
	ZEND_ME(RedisSentinel, ckquorum, arginfo_class_RedisSentinel_ckquorum, ZEND_ACC_PUBLIC)
	ZEND_ME(RedisSentinel, failover, arginfo_class_RedisSentinel_failover, ZEND_ACC_PUBLIC)
	ZEND_ME(RedisSentinel, flushconfig, arginfo_class_RedisSentinel_flushconfig, ZEND_ACC_PUBLIC)
	ZEND_ME(RedisSentinel, getMasterAddrByName, arginfo_class_RedisSentinel_getMasterAddrByName, ZEND_ACC_PUBLIC)
	ZEND_ME(RedisSentinel, master, arginfo_class_RedisSentinel_master, ZEND_ACC_PUBLIC)
	ZEND_ME(RedisSentinel, masters, arginfo_class_RedisSentinel_masters, ZEND_ACC_PUBLIC)
	ZEND_ME(RedisSentinel, myid, arginfo_class_RedisSentinel_myid, ZEND_ACC_PUBLIC)
	ZEND_ME(RedisSentinel, ping, arginfo_class_RedisSentinel_ping, ZEND_ACC_PUBLIC)
	ZEND_ME(RedisSentinel, reset, arginfo_class_RedisSentinel_reset, ZEND_ACC_PUBLIC)
	ZEND_ME(RedisSentinel, sentinels, arginfo_class_RedisSentinel_sentinels, ZEND_ACC_PUBLIC)
	ZEND_ME(RedisSentinel, slaves, arginfo_class_RedisSentinel_slaves, ZEND_ACC_PUBLIC)
	ZEND_FE_END
};

static zend_class_entry *register_class_RedisSentinel(void)
{
	zend_class_entry ce, *class_entry;

	INIT_CLASS_ENTRY(ce, "RedisSentinel", class_RedisSentinel_methods);
	class_entry = zend_register_internal_class_ex(&ce, NULL);
#if (PHP_VERSION_ID >= 80200)


	zend_add_parameter_attribute(zend_hash_str_find_ptr(&class_entry->function_table, "__construct", sizeof("__construct") - 1), 6, ZSTR_KNOWN(ZEND_STR_SENSITIVEPARAMETER), 0);
#endif

	return class_entry;
}
