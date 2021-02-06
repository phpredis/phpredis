/* This is a generated file, edit the .stub.php file instead.
 * Stub hash: b00398a68b9846d7266b8232d11de787fc4bae0c */

ZEND_BEGIN_ARG_INFO_EX(arginfo_class_RedisCluster___construct, 0, 0, 1)
	ZEND_ARG_TYPE_INFO(0, name, IS_STRING, 1)
	ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, seeds, IS_ARRAY, 0, "NULL")
	ZEND_ARG_TYPE_MASK(0, timeout, MAY_BE_LONG|MAY_BE_DOUBLE, "0")
	ZEND_ARG_TYPE_MASK(0, read_timeout, MAY_BE_LONG|MAY_BE_DOUBLE, "0")
	ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, persistant, _IS_BOOL, 0, "false")
	ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, auth, IS_MIXED, 0, "NULL")
	ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, context, IS_ARRAY, 0, "NULL")
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_RedisCluster__masters, 0, 0, IS_ARRAY, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_MASK_EX(arginfo_class_RedisCluster__prefix, 0, 1, MAY_BE_BOOL|MAY_BE_STRING)
	ZEND_ARG_TYPE_INFO(0, key, IS_STRING, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_RedisCluster__redir, 0, 0, IS_STRING, 1)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_MASK_EX(arginfo_class_RedisCluster__serialize, 0, 1, MAY_BE_BOOL|MAY_BE_STRING)
	ZEND_ARG_TYPE_INFO(0, value, IS_MIXED, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_RedisCluster__unserialize, 0, 1, IS_MIXED, 0)
	ZEND_ARG_TYPE_INFO(0, value, IS_STRING, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_RedisCluster_acl, 0, 2, IS_MIXED, 0)
	ZEND_ARG_TYPE_MASK(0, key_or_address, MAY_BE_STRING|MAY_BE_ARRAY, NULL)
	ZEND_ARG_TYPE_INFO(0, subcmd, IS_STRING, 0)
	ZEND_ARG_VARIADIC_TYPE_INFO(0, args, IS_STRING, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_MASK_EX(arginfo_class_RedisCluster_append, 0, 2, MAY_BE_BOOL|MAY_BE_LONG)
	ZEND_ARG_TYPE_INFO(0, key, IS_STRING, 0)
	ZEND_ARG_TYPE_INFO(0, value, IS_MIXED, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_RedisCluster_bgrewriteaof, 0, 1, _IS_BOOL, 0)
	ZEND_ARG_TYPE_MASK(0, key_or_address, MAY_BE_STRING|MAY_BE_ARRAY, NULL)
ZEND_END_ARG_INFO()

#define arginfo_class_RedisCluster_bgsave arginfo_class_RedisCluster_bgrewriteaof

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_MASK_EX(arginfo_class_RedisCluster_bitcount, 0, 1, MAY_BE_BOOL|MAY_BE_LONG)
	ZEND_ARG_TYPE_INFO(0, key, IS_STRING, 0)
	ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, start, IS_LONG, 0, "0")
	ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, end, IS_LONG, 0, "-1")
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_MASK_EX(arginfo_class_RedisCluster_bitop, 0, 3, MAY_BE_BOOL|MAY_BE_LONG)
	ZEND_ARG_TYPE_INFO(0, operation, IS_STRING, 0)
	ZEND_ARG_TYPE_INFO(0, deskey, IS_STRING, 0)
	ZEND_ARG_TYPE_INFO(0, srckey, IS_STRING, 0)
	ZEND_ARG_VARIADIC_TYPE_INFO(0, otherkeys, IS_STRING, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_MASK_EX(arginfo_class_RedisCluster_bitpos, 0, 2, MAY_BE_BOOL|MAY_BE_LONG)
	ZEND_ARG_TYPE_INFO(0, key, IS_STRING, 0)
	ZEND_ARG_TYPE_INFO(0, bit, IS_LONG, 0)
	ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, start, IS_LONG, 0, "NULL")
	ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, end, IS_LONG, 0, "NULL")
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_RedisCluster_blpop, 0, 2, IS_ARRAY, 0)
	ZEND_ARG_TYPE_MASK(0, key, MAY_BE_STRING|MAY_BE_ARRAY, NULL)
	ZEND_ARG_TYPE_MASK(0, timeout_or_key, MAY_BE_STRING|MAY_BE_LONG, NULL)
	ZEND_ARG_VARIADIC_TYPE_INFO(0, extra_args, IS_MIXED, 0)
ZEND_END_ARG_INFO()

#define arginfo_class_RedisCluster_brpop arginfo_class_RedisCluster_blpop

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_RedisCluster_brpoplpush, 0, 3, IS_MIXED, 0)
	ZEND_ARG_TYPE_INFO(0, srckey, IS_STRING, 0)
	ZEND_ARG_TYPE_INFO(0, deskey, IS_STRING, 0)
	ZEND_ARG_TYPE_INFO(0, timeout, IS_LONG, 0)
ZEND_END_ARG_INFO()

#define arginfo_class_RedisCluster_bzpopmax arginfo_class_RedisCluster_blpop

#define arginfo_class_RedisCluster_bzpopmin arginfo_class_RedisCluster_blpop

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_RedisCluster_clearlasterror, 0, 0, _IS_BOOL, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_MASK_EX(arginfo_class_RedisCluster_client, 0, 3, MAY_BE_ARRAY|MAY_BE_STRING|MAY_BE_BOOL)
	ZEND_ARG_TYPE_MASK(0, node, MAY_BE_STRING|MAY_BE_ARRAY, NULL)
	ZEND_ARG_TYPE_INFO(0, subcommand, IS_STRING, 0)
	ZEND_ARG_TYPE_INFO(0, arg, IS_STRING, 1)
ZEND_END_ARG_INFO()

#define arginfo_class_RedisCluster_close arginfo_class_RedisCluster_clearlasterror

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_RedisCluster_cluster, 0, 2, IS_MIXED, 0)
	ZEND_ARG_TYPE_MASK(0, node, MAY_BE_STRING|MAY_BE_ARRAY, NULL)
	ZEND_ARG_TYPE_INFO(0, command, IS_STRING, 0)
	ZEND_ARG_VARIADIC_TYPE_INFO(0, extra_args, IS_MIXED, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_RedisCluster_command, 0, 0, IS_MIXED, 0)
	ZEND_ARG_VARIADIC_TYPE_INFO(0, extra_args, IS_MIXED, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_RedisCluster_config, 0, 2, IS_MIXED, 0)
	ZEND_ARG_TYPE_MASK(0, node, MAY_BE_STRING|MAY_BE_ARRAY, NULL)
	ZEND_ARG_TYPE_INFO(0, subcommand, IS_STRING, 0)
	ZEND_ARG_VARIADIC_TYPE_INFO(0, extra_args, IS_MIXED, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_RedisCluster_dbsize, 0, 1, IS_LONG, 0)
	ZEND_ARG_TYPE_MASK(0, key_or_address, MAY_BE_STRING|MAY_BE_ARRAY, NULL)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_RedisCluster_decr, 0, 1, IS_LONG, 0)
	ZEND_ARG_TYPE_INFO(0, key, IS_STRING, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_RedisCluster_decrby, 0, 2, IS_LONG, 0)
	ZEND_ARG_TYPE_INFO(0, key, IS_STRING, 0)
	ZEND_ARG_TYPE_INFO(0, value, IS_LONG, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_RedisCluster_del, 0, 1, IS_ARRAY, 0)
	ZEND_ARG_TYPE_INFO(0, key, IS_STRING, 0)
	ZEND_ARG_VARIADIC_TYPE_INFO(0, other_keys, IS_STRING, 0)
ZEND_END_ARG_INFO()

#define arginfo_class_RedisCluster_discard arginfo_class_RedisCluster_clearlasterror

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_RedisCluster_dump, 0, 1, IS_STRING, 0)
	ZEND_ARG_TYPE_INFO(0, key, IS_STRING, 0)
ZEND_END_ARG_INFO()


ZEND_METHOD(RedisCluster, __construct);
ZEND_METHOD(RedisCluster, _masters);
ZEND_METHOD(RedisCluster, _prefix);
ZEND_METHOD(RedisCluster, _redir);
ZEND_METHOD(RedisCluster, _serialize);
ZEND_METHOD(RedisCluster, _unserialize);
ZEND_METHOD(RedisCluster, acl);
ZEND_METHOD(RedisCluster, append);
ZEND_METHOD(RedisCluster, bgrewriteaof);
ZEND_METHOD(RedisCluster, bgsave);
ZEND_METHOD(RedisCluster, bitcount);
ZEND_METHOD(RedisCluster, bitop);
ZEND_METHOD(RedisCluster, bitpos);
ZEND_METHOD(RedisCluster, blpop);
ZEND_METHOD(RedisCluster, brpop);
ZEND_METHOD(RedisCluster, brpoplpush);
ZEND_METHOD(RedisCluster, bzpopmax);
ZEND_METHOD(RedisCluster, bzpopmin);
ZEND_METHOD(RedisCluster, clearlasterror);
ZEND_METHOD(RedisCluster, client);
ZEND_METHOD(RedisCluster, close);
ZEND_METHOD(RedisCluster, cluster);
ZEND_METHOD(RedisCluster, command);
ZEND_METHOD(RedisCluster, config);
ZEND_METHOD(RedisCluster, dbsize);
ZEND_METHOD(RedisCluster, decr);
ZEND_METHOD(RedisCluster, decrby);
ZEND_METHOD(RedisCluster, del);
ZEND_METHOD(RedisCluster, discard);
ZEND_METHOD(RedisCluster, dump);


static const zend_function_entry class_RedisCluster_methods[] = {
	ZEND_ME(RedisCluster, __construct, arginfo_class_RedisCluster___construct, ZEND_ACC_PUBLIC)
	ZEND_ME(RedisCluster, _masters, arginfo_class_RedisCluster__masters, ZEND_ACC_PUBLIC)
	ZEND_ME(RedisCluster, _prefix, arginfo_class_RedisCluster__prefix, ZEND_ACC_PUBLIC)
	ZEND_ME(RedisCluster, _redir, arginfo_class_RedisCluster__redir, ZEND_ACC_PUBLIC)
	ZEND_ME(RedisCluster, _serialize, arginfo_class_RedisCluster__serialize, ZEND_ACC_PUBLIC)
	ZEND_ME(RedisCluster, _unserialize, arginfo_class_RedisCluster__unserialize, ZEND_ACC_PUBLIC)
	ZEND_ME(RedisCluster, acl, arginfo_class_RedisCluster_acl, ZEND_ACC_PUBLIC)
	ZEND_ME(RedisCluster, append, arginfo_class_RedisCluster_append, ZEND_ACC_PUBLIC)
	ZEND_ME(RedisCluster, bgrewriteaof, arginfo_class_RedisCluster_bgrewriteaof, ZEND_ACC_PUBLIC)
	ZEND_ME(RedisCluster, bgsave, arginfo_class_RedisCluster_bgsave, ZEND_ACC_PUBLIC)
	ZEND_ME(RedisCluster, bitcount, arginfo_class_RedisCluster_bitcount, ZEND_ACC_PUBLIC)
	ZEND_ME(RedisCluster, bitop, arginfo_class_RedisCluster_bitop, ZEND_ACC_PUBLIC)
	ZEND_ME(RedisCluster, bitpos, arginfo_class_RedisCluster_bitpos, ZEND_ACC_PUBLIC)
	ZEND_ME(RedisCluster, blpop, arginfo_class_RedisCluster_blpop, ZEND_ACC_PUBLIC)
	ZEND_ME(RedisCluster, brpop, arginfo_class_RedisCluster_brpop, ZEND_ACC_PUBLIC)
	ZEND_ME(RedisCluster, brpoplpush, arginfo_class_RedisCluster_brpoplpush, ZEND_ACC_PUBLIC)
	ZEND_ME(RedisCluster, bzpopmax, arginfo_class_RedisCluster_bzpopmax, ZEND_ACC_PUBLIC)
	ZEND_ME(RedisCluster, bzpopmin, arginfo_class_RedisCluster_bzpopmin, ZEND_ACC_PUBLIC)
	ZEND_ME(RedisCluster, clearlasterror, arginfo_class_RedisCluster_clearlasterror, ZEND_ACC_PUBLIC)
	ZEND_ME(RedisCluster, client, arginfo_class_RedisCluster_client, ZEND_ACC_PUBLIC)
	ZEND_ME(RedisCluster, close, arginfo_class_RedisCluster_close, ZEND_ACC_PUBLIC)
	ZEND_ME(RedisCluster, cluster, arginfo_class_RedisCluster_cluster, ZEND_ACC_PUBLIC)
	ZEND_ME(RedisCluster, command, arginfo_class_RedisCluster_command, ZEND_ACC_PUBLIC)
	ZEND_ME(RedisCluster, config, arginfo_class_RedisCluster_config, ZEND_ACC_PUBLIC)
	ZEND_ME(RedisCluster, dbsize, arginfo_class_RedisCluster_dbsize, ZEND_ACC_PUBLIC)
	ZEND_ME(RedisCluster, decr, arginfo_class_RedisCluster_decr, ZEND_ACC_PUBLIC)
	ZEND_ME(RedisCluster, decrby, arginfo_class_RedisCluster_decrby, ZEND_ACC_PUBLIC)
	ZEND_ME(RedisCluster, del, arginfo_class_RedisCluster_del, ZEND_ACC_PUBLIC)
	ZEND_ME(RedisCluster, discard, arginfo_class_RedisCluster_discard, ZEND_ACC_PUBLIC)
	ZEND_ME(RedisCluster, dump, arginfo_class_RedisCluster_dump, ZEND_ACC_PUBLIC)
	ZEND_FE_END
};
