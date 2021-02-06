/* This is a generated file, edit the .stub.php file instead.
 * Stub hash: b00398a68b9846d7266b8232d11de787fc4bae0c */

ZEND_BEGIN_ARG_INFO_EX(arginfo_class_RedisCluster___construct, 0, 0, 1)
	ZEND_ARG_INFO(0, name)
	ZEND_ARG_INFO(0, seeds)
	ZEND_ARG_INFO(0, timeout)
	ZEND_ARG_INFO(0, read_timeout)
	ZEND_ARG_INFO(0, persistant)
	ZEND_ARG_INFO(0, auth)
	ZEND_ARG_INFO(0, context)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_class_RedisCluster__masters, 0, 0, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_class_RedisCluster__prefix, 0, 0, 1)
	ZEND_ARG_INFO(0, key)
ZEND_END_ARG_INFO()

#define arginfo_class_RedisCluster__redir arginfo_class_RedisCluster__masters

ZEND_BEGIN_ARG_INFO_EX(arginfo_class_RedisCluster__serialize, 0, 0, 1)
	ZEND_ARG_INFO(0, value)
ZEND_END_ARG_INFO()

#define arginfo_class_RedisCluster__unserialize arginfo_class_RedisCluster__serialize

ZEND_BEGIN_ARG_INFO_EX(arginfo_class_RedisCluster_acl, 0, 0, 2)
	ZEND_ARG_INFO(0, key_or_address)
	ZEND_ARG_INFO(0, subcmd)
	ZEND_ARG_VARIADIC_INFO(0, args)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_class_RedisCluster_append, 0, 0, 2)
	ZEND_ARG_INFO(0, key)
	ZEND_ARG_INFO(0, value)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_class_RedisCluster_bgrewriteaof, 0, 0, 1)
	ZEND_ARG_INFO(0, key_or_address)
ZEND_END_ARG_INFO()

#define arginfo_class_RedisCluster_bgsave arginfo_class_RedisCluster_bgrewriteaof

ZEND_BEGIN_ARG_INFO_EX(arginfo_class_RedisCluster_bitcount, 0, 0, 1)
	ZEND_ARG_INFO(0, key)
	ZEND_ARG_INFO(0, start)
	ZEND_ARG_INFO(0, end)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_class_RedisCluster_bitop, 0, 0, 3)
	ZEND_ARG_INFO(0, operation)
	ZEND_ARG_INFO(0, deskey)
	ZEND_ARG_INFO(0, srckey)
	ZEND_ARG_VARIADIC_INFO(0, otherkeys)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_class_RedisCluster_bitpos, 0, 0, 2)
	ZEND_ARG_INFO(0, key)
	ZEND_ARG_INFO(0, bit)
	ZEND_ARG_INFO(0, start)
	ZEND_ARG_INFO(0, end)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_class_RedisCluster_blpop, 0, 0, 2)
	ZEND_ARG_INFO(0, key)
	ZEND_ARG_INFO(0, timeout_or_key)
	ZEND_ARG_VARIADIC_INFO(0, extra_args)
ZEND_END_ARG_INFO()

#define arginfo_class_RedisCluster_brpop arginfo_class_RedisCluster_blpop

ZEND_BEGIN_ARG_INFO_EX(arginfo_class_RedisCluster_brpoplpush, 0, 0, 3)
	ZEND_ARG_INFO(0, srckey)
	ZEND_ARG_INFO(0, deskey)
	ZEND_ARG_INFO(0, timeout)
ZEND_END_ARG_INFO()

#define arginfo_class_RedisCluster_bzpopmax arginfo_class_RedisCluster_blpop

#define arginfo_class_RedisCluster_bzpopmin arginfo_class_RedisCluster_blpop

#define arginfo_class_RedisCluster_clearlasterror arginfo_class_RedisCluster__masters

ZEND_BEGIN_ARG_INFO_EX(arginfo_class_RedisCluster_client, 0, 0, 3)
	ZEND_ARG_INFO(0, node)
	ZEND_ARG_INFO(0, subcommand)
	ZEND_ARG_INFO(0, arg)
ZEND_END_ARG_INFO()

#define arginfo_class_RedisCluster_close arginfo_class_RedisCluster__masters

ZEND_BEGIN_ARG_INFO_EX(arginfo_class_RedisCluster_cluster, 0, 0, 2)
	ZEND_ARG_INFO(0, node)
	ZEND_ARG_INFO(0, command)
	ZEND_ARG_VARIADIC_INFO(0, extra_args)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_class_RedisCluster_command, 0, 0, 0)
	ZEND_ARG_VARIADIC_INFO(0, extra_args)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_class_RedisCluster_config, 0, 0, 2)
	ZEND_ARG_INFO(0, node)
	ZEND_ARG_INFO(0, subcommand)
	ZEND_ARG_VARIADIC_INFO(0, extra_args)
ZEND_END_ARG_INFO()

#define arginfo_class_RedisCluster_dbsize arginfo_class_RedisCluster_bgrewriteaof

#define arginfo_class_RedisCluster_decr arginfo_class_RedisCluster__prefix

#define arginfo_class_RedisCluster_decrby arginfo_class_RedisCluster_append

ZEND_BEGIN_ARG_INFO_EX(arginfo_class_RedisCluster_del, 0, 0, 1)
	ZEND_ARG_INFO(0, key)
	ZEND_ARG_VARIADIC_INFO(0, other_keys)
ZEND_END_ARG_INFO()

#define arginfo_class_RedisCluster_discard arginfo_class_RedisCluster__masters

#define arginfo_class_RedisCluster_dump arginfo_class_RedisCluster__prefix


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
