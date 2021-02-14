/* This is a generated file, edit the .stub.php file instead.
 * Stub hash: 5b130a06b4290b7ebec9b20d3973f726cc3fe7af */

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

ZEND_BEGIN_ARG_INFO_EX(arginfo_class_RedisCluster_echo, 0, 0, 2)
	ZEND_ARG_INFO(0, node)
	ZEND_ARG_INFO(0, msg)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_class_RedisCluster_eval, 0, 0, 1)
	ZEND_ARG_INFO(0, script)
	ZEND_ARG_INFO(0, args)
	ZEND_ARG_INFO(0, num_keys)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_class_RedisCluster_evalsha, 0, 0, 1)
	ZEND_ARG_INFO(0, script_sha)
	ZEND_ARG_INFO(0, args)
	ZEND_ARG_INFO(0, num_keys)
ZEND_END_ARG_INFO()

#define arginfo_class_RedisCluster_exec arginfo_class_RedisCluster__masters

#define arginfo_class_RedisCluster_exists arginfo_class_RedisCluster__prefix

ZEND_BEGIN_ARG_INFO_EX(arginfo_class_RedisCluster_expire, 0, 0, 2)
	ZEND_ARG_INFO(0, key)
	ZEND_ARG_INFO(0, timeout)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_class_RedisCluster_expireat, 0, 0, 2)
	ZEND_ARG_INFO(0, key)
	ZEND_ARG_INFO(0, timestamp)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_class_RedisCluster_flushall, 0, 0, 1)
	ZEND_ARG_INFO(0, node)
	ZEND_ARG_INFO(0, async)
ZEND_END_ARG_INFO()

#define arginfo_class_RedisCluster_flushdb arginfo_class_RedisCluster_flushall

ZEND_BEGIN_ARG_INFO_EX(arginfo_class_RedisCluster_geoadd, 0, 0, 4)
	ZEND_ARG_INFO(0, key)
	ZEND_ARG_INFO(0, lng)
	ZEND_ARG_INFO(0, lat)
	ZEND_ARG_INFO(0, member)
	ZEND_ARG_VARIADIC_INFO(0, other_triples)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_class_RedisCluster_geodist, 0, 0, 3)
	ZEND_ARG_INFO(0, key)
	ZEND_ARG_INFO(0, src)
	ZEND_ARG_INFO(0, dest)
	ZEND_ARG_INFO(0, unit)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_class_RedisCluster_geohash, 0, 0, 2)
	ZEND_ARG_INFO(0, key)
	ZEND_ARG_INFO(0, member)
	ZEND_ARG_VARIADIC_INFO(0, other_members)
ZEND_END_ARG_INFO()

#define arginfo_class_RedisCluster_geopos arginfo_class_RedisCluster_geohash

ZEND_BEGIN_ARG_INFO_EX(arginfo_class_RedisCluster_georadius, 0, 0, 5)
	ZEND_ARG_INFO(0, key)
	ZEND_ARG_INFO(0, lng)
	ZEND_ARG_INFO(0, lat)
	ZEND_ARG_INFO(0, radius)
	ZEND_ARG_INFO(0, unit)
	ZEND_ARG_INFO(0, options)
ZEND_END_ARG_INFO()

#define arginfo_class_RedisCluster_georadius_ro arginfo_class_RedisCluster_georadius

ZEND_BEGIN_ARG_INFO_EX(arginfo_class_RedisCluster_georadiusbymember, 0, 0, 4)
	ZEND_ARG_INFO(0, key)
	ZEND_ARG_INFO(0, member)
	ZEND_ARG_INFO(0, radius)
	ZEND_ARG_INFO(0, unit)
	ZEND_ARG_INFO(0, options)
ZEND_END_ARG_INFO()

#define arginfo_class_RedisCluster_georadiusbymember_ro arginfo_class_RedisCluster_georadiusbymember

#define arginfo_class_RedisCluster_get arginfo_class_RedisCluster__prefix

#define arginfo_class_RedisCluster_getbit arginfo_class_RedisCluster_append

#define arginfo_class_RedisCluster_getlasterror arginfo_class_RedisCluster__masters

#define arginfo_class_RedisCluster_getmode arginfo_class_RedisCluster__masters

ZEND_BEGIN_ARG_INFO_EX(arginfo_class_RedisCluster_getoption, 0, 0, 1)
	ZEND_ARG_INFO(0, option)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_class_RedisCluster_getrange, 0, 0, 3)
	ZEND_ARG_INFO(0, key)
	ZEND_ARG_INFO(0, start)
	ZEND_ARG_INFO(0, end)
ZEND_END_ARG_INFO()

#define arginfo_class_RedisCluster_getset arginfo_class_RedisCluster_append


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
ZEND_METHOD(RedisCluster, echo);
ZEND_METHOD(RedisCluster, eval);
ZEND_METHOD(RedisCluster, evalsha);
ZEND_METHOD(RedisCluster, exec);
ZEND_METHOD(RedisCluster, exists);
ZEND_METHOD(RedisCluster, expire);
ZEND_METHOD(RedisCluster, expireat);
ZEND_METHOD(RedisCluster, flushall);
ZEND_METHOD(RedisCluster, flushdb);
ZEND_METHOD(RedisCluster, geoadd);
ZEND_METHOD(RedisCluster, geodist);
ZEND_METHOD(RedisCluster, geohash);
ZEND_METHOD(RedisCluster, geopos);
ZEND_METHOD(RedisCluster, georadius);
ZEND_METHOD(RedisCluster, georadius_ro);
ZEND_METHOD(RedisCluster, georadiusbymember);
ZEND_METHOD(RedisCluster, georadiusbymember_ro);
ZEND_METHOD(RedisCluster, get);
ZEND_METHOD(RedisCluster, getbit);
ZEND_METHOD(RedisCluster, getlasterror);
ZEND_METHOD(RedisCluster, getmode);
ZEND_METHOD(RedisCluster, getoption);
ZEND_METHOD(RedisCluster, getrange);
ZEND_METHOD(RedisCluster, getset);


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
	ZEND_ME(RedisCluster, echo, arginfo_class_RedisCluster_echo, ZEND_ACC_PUBLIC)
	ZEND_ME(RedisCluster, eval, arginfo_class_RedisCluster_eval, ZEND_ACC_PUBLIC)
	ZEND_ME(RedisCluster, evalsha, arginfo_class_RedisCluster_evalsha, ZEND_ACC_PUBLIC)
	ZEND_ME(RedisCluster, exec, arginfo_class_RedisCluster_exec, ZEND_ACC_PUBLIC)
	ZEND_ME(RedisCluster, exists, arginfo_class_RedisCluster_exists, ZEND_ACC_PUBLIC)
	ZEND_ME(RedisCluster, expire, arginfo_class_RedisCluster_expire, ZEND_ACC_PUBLIC)
	ZEND_ME(RedisCluster, expireat, arginfo_class_RedisCluster_expireat, ZEND_ACC_PUBLIC)
	ZEND_ME(RedisCluster, flushall, arginfo_class_RedisCluster_flushall, ZEND_ACC_PUBLIC)
	ZEND_ME(RedisCluster, flushdb, arginfo_class_RedisCluster_flushdb, ZEND_ACC_PUBLIC)
	ZEND_ME(RedisCluster, geoadd, arginfo_class_RedisCluster_geoadd, ZEND_ACC_PUBLIC)
	ZEND_ME(RedisCluster, geodist, arginfo_class_RedisCluster_geodist, ZEND_ACC_PUBLIC)
	ZEND_ME(RedisCluster, geohash, arginfo_class_RedisCluster_geohash, ZEND_ACC_PUBLIC)
	ZEND_ME(RedisCluster, geopos, arginfo_class_RedisCluster_geopos, ZEND_ACC_PUBLIC)
	ZEND_ME(RedisCluster, georadius, arginfo_class_RedisCluster_georadius, ZEND_ACC_PUBLIC)
	ZEND_ME(RedisCluster, georadius_ro, arginfo_class_RedisCluster_georadius_ro, ZEND_ACC_PUBLIC)
	ZEND_ME(RedisCluster, georadiusbymember, arginfo_class_RedisCluster_georadiusbymember, ZEND_ACC_PUBLIC)
	ZEND_ME(RedisCluster, georadiusbymember_ro, arginfo_class_RedisCluster_georadiusbymember_ro, ZEND_ACC_PUBLIC)
	ZEND_ME(RedisCluster, get, arginfo_class_RedisCluster_get, ZEND_ACC_PUBLIC)
	ZEND_ME(RedisCluster, getbit, arginfo_class_RedisCluster_getbit, ZEND_ACC_PUBLIC)
	ZEND_ME(RedisCluster, getlasterror, arginfo_class_RedisCluster_getlasterror, ZEND_ACC_PUBLIC)
	ZEND_ME(RedisCluster, getmode, arginfo_class_RedisCluster_getmode, ZEND_ACC_PUBLIC)
	ZEND_ME(RedisCluster, getoption, arginfo_class_RedisCluster_getoption, ZEND_ACC_PUBLIC)
	ZEND_ME(RedisCluster, getrange, arginfo_class_RedisCluster_getrange, ZEND_ACC_PUBLIC)
	ZEND_ME(RedisCluster, getset, arginfo_class_RedisCluster_getset, ZEND_ACC_PUBLIC)
	ZEND_FE_END
};
