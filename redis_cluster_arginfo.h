/* This is a generated file, edit the .stub.php file instead.
 * Stub hash: aecb51252f52073c36fbe147be0aacddd649db07 */

ZEND_BEGIN_ARG_INFO_EX(arginfo_class_RedisCluster___construct, 0, 0, 1)
	ZEND_ARG_TYPE_INFO(0, name, IS_STRING, 1)
	ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, seeds, IS_ARRAY, 0, "NULL")
	ZEND_ARG_TYPE_MASK(0, timeout, MAY_BE_LONG|MAY_BE_DOUBLE, "0")
	ZEND_ARG_TYPE_MASK(0, read_timeout, MAY_BE_LONG|MAY_BE_DOUBLE, "0")
	ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, persistant, _IS_BOOL, 0, "false")
	ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, auth, IS_MIXED, 0, "NULL")
	ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, context, IS_ARRAY, 0, "NULL")
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_RedisCluster__compress, 0, 1, IS_STRING, 0)
	ZEND_ARG_TYPE_INFO(0, value, IS_STRING, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_RedisCluster__masters, 0, 0, IS_ARRAY, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_RedisCluster__pack, 0, 1, IS_STRING, 0)
	ZEND_ARG_TYPE_INFO(0, value, IS_MIXED, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_MASK_EX(arginfo_class_RedisCluster__prefix, 0, 1, MAY_BE_BOOL|MAY_BE_STRING)
	ZEND_ARG_TYPE_INFO(0, key, IS_STRING, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_RedisCluster__redir, 0, 0, IS_STRING, 1)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_MASK_EX(arginfo_class_RedisCluster__serialize, 0, 1, MAY_BE_BOOL|MAY_BE_STRING)
	ZEND_ARG_TYPE_INFO(0, value, IS_MIXED, 0)
ZEND_END_ARG_INFO()

#define arginfo_class_RedisCluster__uncompress arginfo_class_RedisCluster__compress

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_RedisCluster__unpack, 0, 1, IS_MIXED, 0)
	ZEND_ARG_TYPE_INFO(0, value, IS_STRING, 0)
ZEND_END_ARG_INFO()

#define arginfo_class_RedisCluster__unserialize arginfo_class_RedisCluster__unpack

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

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_RedisCluster_decrbyfloat, 0, 2, IS_DOUBLE, 0)
	ZEND_ARG_TYPE_INFO(0, key, IS_STRING, 0)
	ZEND_ARG_TYPE_INFO(0, value, IS_DOUBLE, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_RedisCluster_del, 0, 1, IS_ARRAY, 0)
	ZEND_ARG_TYPE_INFO(0, key, IS_STRING, 0)
	ZEND_ARG_VARIADIC_TYPE_INFO(0, other_keys, IS_STRING, 0)
ZEND_END_ARG_INFO()

#define arginfo_class_RedisCluster_discard arginfo_class_RedisCluster_clearlasterror

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_RedisCluster_dump, 0, 1, IS_STRING, 0)
	ZEND_ARG_TYPE_INFO(0, key, IS_STRING, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_RedisCluster_echo, 0, 2, IS_STRING, 0)
	ZEND_ARG_TYPE_MASK(0, node, MAY_BE_STRING|MAY_BE_ARRAY, NULL)
	ZEND_ARG_TYPE_INFO(0, msg, IS_STRING, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_RedisCluster_eval, 0, 1, IS_MIXED, 0)
	ZEND_ARG_TYPE_INFO(0, script, IS_STRING, 0)
	ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, args, IS_ARRAY, 0, "[]")
	ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, num_keys, IS_LONG, 0, "0")
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_RedisCluster_evalsha, 0, 1, IS_MIXED, 0)
	ZEND_ARG_TYPE_INFO(0, script_sha, IS_STRING, 0)
	ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, args, IS_ARRAY, 0, "[]")
	ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, num_keys, IS_LONG, 0, "0")
ZEND_END_ARG_INFO()

#define arginfo_class_RedisCluster_exec arginfo_class_RedisCluster__masters

#define arginfo_class_RedisCluster_exists arginfo_class_RedisCluster_decr

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_RedisCluster_expire, 0, 2, _IS_BOOL, 0)
	ZEND_ARG_TYPE_INFO(0, key, IS_STRING, 0)
	ZEND_ARG_TYPE_INFO(0, timeout, IS_LONG, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_RedisCluster_expireat, 0, 2, _IS_BOOL, 0)
	ZEND_ARG_TYPE_INFO(0, key, IS_STRING, 0)
	ZEND_ARG_TYPE_INFO(0, timestamp, IS_LONG, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_RedisCluster_flushall, 0, 1, _IS_BOOL, 0)
	ZEND_ARG_TYPE_MASK(0, node, MAY_BE_STRING|MAY_BE_ARRAY, NULL)
	ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, async, _IS_BOOL, 0, "false")
ZEND_END_ARG_INFO()

#define arginfo_class_RedisCluster_flushdb arginfo_class_RedisCluster_flushall

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_RedisCluster_geoadd, 0, 4, IS_LONG, 0)
	ZEND_ARG_TYPE_INFO(0, key, IS_STRING, 0)
	ZEND_ARG_TYPE_INFO(0, lng, IS_DOUBLE, 0)
	ZEND_ARG_TYPE_INFO(0, lat, IS_DOUBLE, 0)
	ZEND_ARG_TYPE_INFO(0, member, IS_STRING, 0)
	ZEND_ARG_VARIADIC_TYPE_INFO(0, other_triples, IS_MIXED, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_RedisCluster_geodist, 0, 3, IS_ARRAY, 0)
	ZEND_ARG_TYPE_INFO(0, key, IS_STRING, 0)
	ZEND_ARG_TYPE_INFO(0, src, IS_STRING, 0)
	ZEND_ARG_TYPE_INFO(0, dest, IS_STRING, 0)
	ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, unit, IS_STRING, 1, "null")
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_RedisCluster_geohash, 0, 2, IS_ARRAY, 0)
	ZEND_ARG_TYPE_INFO(0, key, IS_STRING, 0)
	ZEND_ARG_TYPE_INFO(0, member, IS_STRING, 0)
	ZEND_ARG_VARIADIC_TYPE_INFO(0, other_members, IS_STRING, 0)
ZEND_END_ARG_INFO()

#define arginfo_class_RedisCluster_geopos arginfo_class_RedisCluster_geohash

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_RedisCluster_georadius, 0, 5, IS_ARRAY, 0)
	ZEND_ARG_TYPE_INFO(0, key, IS_STRING, 0)
	ZEND_ARG_TYPE_INFO(0, lng, IS_DOUBLE, 0)
	ZEND_ARG_TYPE_INFO(0, lat, IS_DOUBLE, 0)
	ZEND_ARG_TYPE_INFO(0, radius, IS_DOUBLE, 0)
	ZEND_ARG_TYPE_INFO(0, unit, IS_STRING, 0)
	ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, options, IS_ARRAY, 0, "[]")
ZEND_END_ARG_INFO()

#define arginfo_class_RedisCluster_georadius_ro arginfo_class_RedisCluster_georadius

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_RedisCluster_georadiusbymember, 0, 4, IS_ARRAY, 0)
	ZEND_ARG_TYPE_INFO(0, key, IS_STRING, 0)
	ZEND_ARG_TYPE_INFO(0, member, IS_STRING, 0)
	ZEND_ARG_TYPE_INFO(0, radius, IS_DOUBLE, 0)
	ZEND_ARG_TYPE_INFO(0, unit, IS_STRING, 0)
	ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, options, IS_ARRAY, 0, "[]")
ZEND_END_ARG_INFO()

#define arginfo_class_RedisCluster_georadiusbymember_ro arginfo_class_RedisCluster_georadiusbymember

#define arginfo_class_RedisCluster_get arginfo_class_RedisCluster_dump

#define arginfo_class_RedisCluster_getbit arginfo_class_RedisCluster_decrby

#define arginfo_class_RedisCluster_getlasterror arginfo_class_RedisCluster__redir

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_RedisCluster_getmode, 0, 0, IS_LONG, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_RedisCluster_getoption, 0, 1, IS_MIXED, 0)
	ZEND_ARG_TYPE_INFO(0, option, IS_LONG, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_RedisCluster_getrange, 0, 3, IS_STRING, 0)
	ZEND_ARG_TYPE_INFO(0, key, IS_STRING, 0)
	ZEND_ARG_TYPE_INFO(0, start, IS_LONG, 0)
	ZEND_ARG_TYPE_INFO(0, end, IS_LONG, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_RedisCluster_getset, 0, 2, IS_STRING, 0)
	ZEND_ARG_TYPE_INFO(0, key, IS_STRING, 0)
	ZEND_ARG_TYPE_INFO(0, value, IS_MIXED, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_RedisCluster_hdel, 0, 2, IS_LONG, 0)
	ZEND_ARG_TYPE_INFO(0, key, IS_STRING, 0)
	ZEND_ARG_TYPE_INFO(0, member, IS_STRING, 0)
	ZEND_ARG_VARIADIC_TYPE_INFO(0, other_members, IS_STRING, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_RedisCluster_hexists, 0, 2, _IS_BOOL, 0)
	ZEND_ARG_TYPE_INFO(0, key, IS_STRING, 0)
	ZEND_ARG_TYPE_INFO(0, member, IS_STRING, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_RedisCluster_hget, 0, 2, IS_STRING, 0)
	ZEND_ARG_TYPE_INFO(0, key, IS_STRING, 0)
	ZEND_ARG_TYPE_INFO(0, member, IS_STRING, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_RedisCluster_hgetall, 0, 1, IS_ARRAY, 0)
	ZEND_ARG_TYPE_INFO(0, key, IS_STRING, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_RedisCluster_hincrby, 0, 3, IS_LONG, 0)
	ZEND_ARG_TYPE_INFO(0, key, IS_STRING, 0)
	ZEND_ARG_TYPE_INFO(0, member, IS_STRING, 0)
	ZEND_ARG_TYPE_INFO(0, value, IS_LONG, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_RedisCluster_hincrbyfloat, 0, 3, IS_DOUBLE, 0)
	ZEND_ARG_TYPE_INFO(0, key, IS_STRING, 0)
	ZEND_ARG_TYPE_INFO(0, member, IS_STRING, 0)
	ZEND_ARG_TYPE_INFO(0, value, IS_DOUBLE, 0)
ZEND_END_ARG_INFO()

#define arginfo_class_RedisCluster_hkeys arginfo_class_RedisCluster_hgetall

#define arginfo_class_RedisCluster_hlen arginfo_class_RedisCluster_decr

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_RedisCluster_hmget, 0, 2, IS_ARRAY, 0)
	ZEND_ARG_TYPE_INFO(0, key, IS_STRING, 0)
	ZEND_ARG_TYPE_INFO(0, members, IS_ARRAY, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_RedisCluster_hmset, 0, 2, _IS_BOOL, 0)
	ZEND_ARG_TYPE_INFO(0, key, IS_STRING, 0)
	ZEND_ARG_TYPE_INFO(0, key_values, IS_ARRAY, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_MASK_EX(arginfo_class_RedisCluster_hscan, 0, 2, MAY_BE_ARRAY|MAY_BE_BOOL)
	ZEND_ARG_TYPE_INFO(0, key, IS_STRING, 0)
	ZEND_ARG_TYPE_INFO(1, iterator, IS_LONG, 0)
	ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, pattern, IS_STRING, 1, "null")
	ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, count, IS_LONG, 0, "0")
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_RedisCluster_hset, 0, 3, IS_LONG, 0)
	ZEND_ARG_TYPE_INFO(0, key, IS_STRING, 0)
	ZEND_ARG_TYPE_INFO(0, member, IS_STRING, 0)
	ZEND_ARG_TYPE_INFO(0, value, IS_MIXED, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_RedisCluster_hsetnx, 0, 3, _IS_BOOL, 0)
	ZEND_ARG_TYPE_INFO(0, key, IS_STRING, 0)
	ZEND_ARG_TYPE_INFO(0, member, IS_STRING, 0)
	ZEND_ARG_TYPE_INFO(0, value, IS_MIXED, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_RedisCluster_hstrlen, 0, 2, IS_LONG, 0)
	ZEND_ARG_TYPE_INFO(0, key, IS_STRING, 0)
	ZEND_ARG_TYPE_INFO(0, field, IS_STRING, 0)
ZEND_END_ARG_INFO()

#define arginfo_class_RedisCluster_hvals arginfo_class_RedisCluster_hgetall

#define arginfo_class_RedisCluster_incr arginfo_class_RedisCluster_decr

#define arginfo_class_RedisCluster_incrby arginfo_class_RedisCluster_decrby

#define arginfo_class_RedisCluster_incrbyfloat arginfo_class_RedisCluster_decrbyfloat

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_RedisCluster_info, 0, 1, IS_ARRAY, 0)
	ZEND_ARG_TYPE_MASK(0, node, MAY_BE_STRING|MAY_BE_ARRAY, NULL)
	ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, section, IS_STRING, 1, "null")
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_RedisCluster_keys, 0, 1, IS_ARRAY, 0)
	ZEND_ARG_TYPE_INFO(0, pattern, IS_STRING, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_RedisCluster_lastsave, 0, 1, IS_LONG, 0)
	ZEND_ARG_TYPE_MASK(0, node, MAY_BE_STRING|MAY_BE_ARRAY, NULL)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_MASK_EX(arginfo_class_RedisCluster_lget, 0, 2, MAY_BE_STRING|MAY_BE_BOOL)
	ZEND_ARG_TYPE_INFO(0, key, IS_STRING, 0)
	ZEND_ARG_TYPE_INFO(0, index, IS_LONG, 0)
ZEND_END_ARG_INFO()

#define arginfo_class_RedisCluster_lindex arginfo_class_RedisCluster_lget

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_RedisCluster_linsert, 0, 4, IS_LONG, 0)
	ZEND_ARG_TYPE_INFO(0, key, IS_STRING, 0)
	ZEND_ARG_TYPE_INFO(0, pos, IS_STRING, 0)
	ZEND_ARG_TYPE_INFO(0, pivot, IS_MIXED, 0)
	ZEND_ARG_TYPE_INFO(0, value, IS_MIXED, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_MASK_EX(arginfo_class_RedisCluster_llen, 0, 1, MAY_BE_LONG|MAY_BE_BOOL)
	ZEND_ARG_TYPE_INFO(0, key, IS_STRING, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_MASK_EX(arginfo_class_RedisCluster_lpop, 0, 1, MAY_BE_STRING|MAY_BE_BOOL)
	ZEND_ARG_TYPE_INFO(0, key, IS_STRING, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_MASK_EX(arginfo_class_RedisCluster_lpush, 0, 2, MAY_BE_LONG|MAY_BE_BOOL)
	ZEND_ARG_TYPE_INFO(0, key, IS_STRING, 0)
	ZEND_ARG_TYPE_INFO(0, value, IS_MIXED, 0)
	ZEND_ARG_VARIADIC_TYPE_INFO(0, other_values, IS_MIXED, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_MASK_EX(arginfo_class_RedisCluster_lpushx, 0, 2, MAY_BE_LONG|MAY_BE_BOOL)
	ZEND_ARG_TYPE_INFO(0, key, IS_STRING, 0)
	ZEND_ARG_TYPE_INFO(0, value, IS_MIXED, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_RedisCluster_lrange, 0, 3, IS_ARRAY, 0)
	ZEND_ARG_TYPE_INFO(0, key, IS_STRING, 0)
	ZEND_ARG_TYPE_INFO(0, start, IS_LONG, 0)
	ZEND_ARG_TYPE_INFO(0, end, IS_LONG, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_MASK_EX(arginfo_class_RedisCluster_lrem, 0, 3, MAY_BE_LONG|MAY_BE_BOOL)
	ZEND_ARG_TYPE_INFO(0, key, IS_STRING, 0)
	ZEND_ARG_TYPE_INFO(0, count, IS_LONG, 0)
	ZEND_ARG_TYPE_INFO(0, value, IS_STRING, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_RedisCluster_lset, 0, 3, _IS_BOOL, 0)
	ZEND_ARG_TYPE_INFO(0, key, IS_STRING, 0)
	ZEND_ARG_TYPE_INFO(0, index, IS_LONG, 0)
	ZEND_ARG_TYPE_INFO(0, value, IS_STRING, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_RedisCluster_ltrim, 0, 3, _IS_BOOL, 0)
	ZEND_ARG_TYPE_INFO(0, key, IS_STRING, 0)
	ZEND_ARG_TYPE_INFO(0, start, IS_LONG, 0)
	ZEND_ARG_TYPE_INFO(0, end, IS_LONG, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_RedisCluster_mget, 0, 1, IS_ARRAY, 0)
	ZEND_ARG_TYPE_INFO(0, keys, IS_ARRAY, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_RedisCluster_mset, 0, 1, _IS_BOOL, 0)
	ZEND_ARG_TYPE_INFO(0, key_values, IS_ARRAY, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_RedisCluster_msetnx, 0, 1, IS_LONG, 0)
	ZEND_ARG_TYPE_INFO(0, key_values, IS_ARRAY, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_OBJ_TYPE_MASK_EX(arginfo_class_RedisCluster_multi, 0, 0, RedisCluster, MAY_BE_BOOL)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_MASK_EX(arginfo_class_RedisCluster_object, 0, 2, MAY_BE_LONG|MAY_BE_STRING)
	ZEND_ARG_TYPE_INFO(0, subcommand, IS_STRING, 0)
	ZEND_ARG_TYPE_INFO(0, key, IS_STRING, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_RedisCluster_persist, 0, 1, _IS_BOOL, 0)
	ZEND_ARG_TYPE_INFO(0, key, IS_STRING, 0)
ZEND_END_ARG_INFO()

#define arginfo_class_RedisCluster_pexpire arginfo_class_RedisCluster_expire

#define arginfo_class_RedisCluster_pexpireat arginfo_class_RedisCluster_expireat

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_RedisCluster_pfadd, 0, 2, _IS_BOOL, 0)
	ZEND_ARG_TYPE_INFO(0, key, IS_STRING, 0)
	ZEND_ARG_TYPE_INFO(0, elements, IS_ARRAY, 0)
ZEND_END_ARG_INFO()

#define arginfo_class_RedisCluster_pfcount arginfo_class_RedisCluster_decr

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_RedisCluster_pfmerge, 0, 2, _IS_BOOL, 0)
	ZEND_ARG_TYPE_INFO(0, key, IS_STRING, 0)
	ZEND_ARG_TYPE_INFO(0, keys, IS_ARRAY, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_RedisCluster_ping, 0, 2, IS_MIXED, 0)
	ZEND_ARG_TYPE_MASK(0, key_or_address, MAY_BE_STRING|MAY_BE_ARRAY, NULL)
	ZEND_ARG_TYPE_INFO(0, message, IS_STRING, 1)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_RedisCluster_psetex, 0, 3, _IS_BOOL, 0)
	ZEND_ARG_TYPE_INFO(0, key, IS_STRING, 0)
	ZEND_ARG_TYPE_INFO(0, timeout, IS_LONG, 0)
	ZEND_ARG_TYPE_INFO(0, value, IS_STRING, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_RedisCluster_psubscribe, 0, 2, IS_VOID, 0)
	ZEND_ARG_TYPE_INFO(0, patterns, IS_ARRAY, 0)
	ZEND_ARG_TYPE_INFO(0, callback, IS_CALLABLE, 0)
ZEND_END_ARG_INFO()

#define arginfo_class_RedisCluster_pttl arginfo_class_RedisCluster_decr

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_RedisCluster_publish, 0, 2, _IS_BOOL, 0)
	ZEND_ARG_TYPE_INFO(0, channel, IS_STRING, 0)
	ZEND_ARG_TYPE_INFO(0, message, IS_STRING, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_RedisCluster_pubsub, 0, 1, IS_MIXED, 0)
	ZEND_ARG_TYPE_MASK(0, key_or_address, MAY_BE_STRING|MAY_BE_ARRAY, NULL)
	ZEND_ARG_VARIADIC_TYPE_INFO(0, values, IS_STRING, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_MASK_EX(arginfo_class_RedisCluster_punsubscribe, 0, 1, MAY_BE_BOOL|MAY_BE_ARRAY)
	ZEND_ARG_TYPE_INFO(0, pattern, IS_STRING, 0)
	ZEND_ARG_VARIADIC_TYPE_INFO(0, other_patterns, IS_STRING, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_MASK_EX(arginfo_class_RedisCluster_randomkey, 0, 1, MAY_BE_BOOL|MAY_BE_STRING)
	ZEND_ARG_TYPE_MASK(0, key_or_address, MAY_BE_STRING|MAY_BE_ARRAY, NULL)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_RedisCluster_rawcommand, 0, 2, IS_MIXED, 0)
	ZEND_ARG_TYPE_MASK(0, key_or_address, MAY_BE_STRING|MAY_BE_ARRAY, NULL)
	ZEND_ARG_TYPE_INFO(0, command, IS_STRING, 0)
	ZEND_ARG_VARIADIC_TYPE_INFO(0, args, IS_MIXED, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_RedisCluster_rename, 0, 2, _IS_BOOL, 0)
	ZEND_ARG_TYPE_INFO(0, key, IS_STRING, 0)
	ZEND_ARG_TYPE_INFO(0, newkey, IS_STRING, 0)
ZEND_END_ARG_INFO()

#define arginfo_class_RedisCluster_renamenx arginfo_class_RedisCluster_rename

#define arginfo_class_RedisCluster_restore arginfo_class_RedisCluster_psetex

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_RedisCluster_role, 0, 1, IS_MIXED, 0)
	ZEND_ARG_TYPE_MASK(0, key_or_address, MAY_BE_STRING|MAY_BE_ARRAY, NULL)
ZEND_END_ARG_INFO()

#define arginfo_class_RedisCluster_rpop arginfo_class_RedisCluster__prefix

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_MASK_EX(arginfo_class_RedisCluster_rpoplpush, 0, 2, MAY_BE_BOOL|MAY_BE_STRING)
	ZEND_ARG_TYPE_INFO(0, src, IS_STRING, 0)
	ZEND_ARG_TYPE_INFO(0, dst, IS_STRING, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_MASK_EX(arginfo_class_RedisCluster_rpush, 0, 2, MAY_BE_BOOL|MAY_BE_LONG)
	ZEND_ARG_TYPE_INFO(0, key, IS_STRING, 0)
	ZEND_ARG_TYPE_INFO(0, value, IS_STRING, 0)
	ZEND_ARG_VARIADIC_TYPE_INFO(0, other_values, IS_STRING, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_MASK_EX(arginfo_class_RedisCluster_rpushx, 0, 2, MAY_BE_BOOL|MAY_BE_LONG)
	ZEND_ARG_TYPE_INFO(0, key, IS_STRING, 0)
	ZEND_ARG_TYPE_INFO(0, value, IS_STRING, 0)
ZEND_END_ARG_INFO()

#define arginfo_class_RedisCluster_sadd arginfo_class_RedisCluster_rpush

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_MASK_EX(arginfo_class_RedisCluster_saddarray, 0, 2, MAY_BE_BOOL|MAY_BE_LONG)
	ZEND_ARG_TYPE_INFO(0, key, IS_STRING, 0)
	ZEND_ARG_TYPE_INFO(0, values, IS_ARRAY, 0)
ZEND_END_ARG_INFO()

#define arginfo_class_RedisCluster_save arginfo_class_RedisCluster_bgrewriteaof

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_MASK_EX(arginfo_class_RedisCluster_scan, 0, 2, MAY_BE_BOOL|MAY_BE_ARRAY)
	ZEND_ARG_TYPE_INFO(1, iterator, IS_LONG, 0)
	ZEND_ARG_TYPE_INFO(0, node, IS_MIXED, 0)
	ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, pattern, IS_STRING, 1, "null")
	ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, count, IS_LONG, 0, "0")
ZEND_END_ARG_INFO()

#define arginfo_class_RedisCluster_scard arginfo_class_RedisCluster_decr

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_RedisCluster_script, 0, 1, IS_MIXED, 0)
	ZEND_ARG_TYPE_MASK(0, key_or_address, MAY_BE_STRING|MAY_BE_ARRAY, NULL)
	ZEND_ARG_VARIADIC_TYPE_INFO(0, args, IS_MIXED, 0)
ZEND_END_ARG_INFO()

#define arginfo_class_RedisCluster_sdiff arginfo_class_RedisCluster_del

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_RedisCluster_sdiffstore, 0, 2, IS_LONG, 0)
	ZEND_ARG_TYPE_INFO(0, dst, IS_STRING, 0)
	ZEND_ARG_TYPE_INFO(0, key, IS_STRING, 0)
	ZEND_ARG_VARIADIC_TYPE_INFO(0, other_keys, IS_STRING, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_RedisCluster_set, 0, 2, _IS_BOOL, 0)
	ZEND_ARG_TYPE_INFO(0, key, IS_STRING, 0)
	ZEND_ARG_TYPE_INFO(0, value, IS_STRING, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_RedisCluster_setbit, 0, 3, _IS_BOOL, 0)
	ZEND_ARG_TYPE_INFO(0, key, IS_STRING, 0)
	ZEND_ARG_TYPE_INFO(0, offset, IS_LONG, 0)
	ZEND_ARG_TYPE_INFO(0, onoff, _IS_BOOL, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_RedisCluster_setex, 0, 3, _IS_BOOL, 0)
	ZEND_ARG_TYPE_INFO(0, key, IS_STRING, 0)
	ZEND_ARG_TYPE_INFO(0, value, IS_STRING, 0)
	ZEND_ARG_TYPE_INFO(0, timeout, IS_LONG, 0)
ZEND_END_ARG_INFO()

#define arginfo_class_RedisCluster_setnx arginfo_class_RedisCluster_setex

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_RedisCluster_setoption, 0, 2, _IS_BOOL, 0)
	ZEND_ARG_TYPE_INFO(0, option, IS_LONG, 0)
	ZEND_ARG_TYPE_INFO(0, value, IS_MIXED, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_RedisCluster_setrange, 0, 3, IS_LONG, 0)
	ZEND_ARG_TYPE_INFO(0, key, IS_STRING, 0)
	ZEND_ARG_TYPE_INFO(0, offset, IS_LONG, 0)
	ZEND_ARG_TYPE_INFO(0, value, IS_STRING, 0)
ZEND_END_ARG_INFO()

#define arginfo_class_RedisCluster_sinter arginfo_class_RedisCluster_del

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_RedisCluster_sinterstore, 0, 2, _IS_BOOL, 0)
	ZEND_ARG_TYPE_INFO(0, dst, IS_STRING, 0)
	ZEND_ARG_TYPE_INFO(0, key, IS_STRING, 0)
	ZEND_ARG_VARIADIC_TYPE_INFO(0, other_keys, IS_STRING, 0)
ZEND_END_ARG_INFO()

#define arginfo_class_RedisCluster_sismember arginfo_class_RedisCluster_decr

#define arginfo_class_RedisCluster_slowlog arginfo_class_RedisCluster_script

#define arginfo_class_RedisCluster_smembers arginfo_class_RedisCluster_hgetall

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_RedisCluster_smove, 0, 3, _IS_BOOL, 0)
	ZEND_ARG_TYPE_INFO(0, src, IS_STRING, 0)
	ZEND_ARG_TYPE_INFO(0, dst, IS_STRING, 0)
	ZEND_ARG_TYPE_INFO(0, member, IS_STRING, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_MASK_EX(arginfo_class_RedisCluster_sort, 0, 2, MAY_BE_BOOL|MAY_BE_LONG|MAY_BE_STRING)
	ZEND_ARG_TYPE_INFO(0, key, IS_STRING, 0)
	ZEND_ARG_TYPE_INFO(0, options, IS_ARRAY, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_MASK_EX(arginfo_class_RedisCluster_spop, 0, 1, MAY_BE_STRING|MAY_BE_ARRAY)
	ZEND_ARG_TYPE_INFO(0, key, IS_STRING, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_MASK_EX(arginfo_class_RedisCluster_srandmember, 0, 1, MAY_BE_STRING|MAY_BE_ARRAY)
	ZEND_ARG_TYPE_INFO(0, key, IS_STRING, 0)
	ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, count, IS_LONG, 0, "0")
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_RedisCluster_srem, 0, 2, IS_LONG, 0)
	ZEND_ARG_TYPE_INFO(0, key, IS_STRING, 0)
	ZEND_ARG_TYPE_INFO(0, value, IS_STRING, 0)
	ZEND_ARG_VARIADIC_TYPE_INFO(0, other_values, IS_STRING, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_MASK_EX(arginfo_class_RedisCluster_sscan, 0, 3, MAY_BE_BOOL|MAY_BE_ARRAY)
	ZEND_ARG_TYPE_INFO(0, key, IS_STRING, 0)
	ZEND_ARG_TYPE_INFO(1, iterator, IS_LONG, 0)
	ZEND_ARG_TYPE_INFO(0, node, IS_MIXED, 0)
	ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, pattern, IS_STRING, 1, "null")
	ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, count, IS_LONG, 0, "0")
ZEND_END_ARG_INFO()

#define arginfo_class_RedisCluster_strlen arginfo_class_RedisCluster_decr

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_RedisCluster_subscribe, 0, 2, IS_VOID, 0)
	ZEND_ARG_TYPE_INFO(0, channels, IS_ARRAY, 0)
	ZEND_ARG_TYPE_INFO(0, cb, IS_CALLABLE, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_MASK_EX(arginfo_class_RedisCluster_sunion, 0, 1, MAY_BE_BOOL|MAY_BE_ARRAY)
	ZEND_ARG_TYPE_INFO(0, key, IS_STRING, 0)
	ZEND_ARG_VARIADIC_TYPE_INFO(0, other_keys, IS_STRING, 0)
ZEND_END_ARG_INFO()

#define arginfo_class_RedisCluster_sunionstore arginfo_class_RedisCluster_sdiffstore

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_MASK_EX(arginfo_class_RedisCluster_time, 0, 1, MAY_BE_BOOL|MAY_BE_ARRAY)
	ZEND_ARG_TYPE_MASK(0, key_or_address, MAY_BE_STRING|MAY_BE_ARRAY, NULL)
ZEND_END_ARG_INFO()

#define arginfo_class_RedisCluster_ttl arginfo_class_RedisCluster_decr

#define arginfo_class_RedisCluster_type arginfo_class_RedisCluster_decr

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_MASK_EX(arginfo_class_RedisCluster_unsubscribe, 0, 1, MAY_BE_BOOL|MAY_BE_ARRAY)
	ZEND_ARG_TYPE_INFO(0, channels, IS_ARRAY, 0)
ZEND_END_ARG_INFO()

#define arginfo_class_RedisCluster_unlink arginfo_class_RedisCluster_del

#define arginfo_class_RedisCluster_unwatch arginfo_class_RedisCluster_clearlasterror

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_RedisCluster_watch, 0, 1, _IS_BOOL, 0)
	ZEND_ARG_TYPE_INFO(0, key, IS_STRING, 0)
	ZEND_ARG_VARIADIC_TYPE_INFO(0, other_keys, IS_STRING, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_RedisCluster_xack, 0, 3, IS_LONG, 0)
	ZEND_ARG_TYPE_INFO(0, key, IS_STRING, 0)
	ZEND_ARG_TYPE_INFO(0, group, IS_STRING, 0)
	ZEND_ARG_TYPE_INFO(0, ids, IS_ARRAY, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_RedisCluster_xadd, 0, 3, IS_STRING, 0)
	ZEND_ARG_TYPE_INFO(0, key, IS_STRING, 0)
	ZEND_ARG_TYPE_INFO(0, id, IS_STRING, 0)
	ZEND_ARG_TYPE_INFO(0, values, IS_ARRAY, 0)
	ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, maxlen, IS_LONG, 0, "0")
	ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, approx, _IS_BOOL, 0, "false")
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_MASK_EX(arginfo_class_RedisCluster_xclaim, 0, 6, MAY_BE_STRING|MAY_BE_ARRAY)
	ZEND_ARG_TYPE_INFO(0, key, IS_STRING, 0)
	ZEND_ARG_TYPE_INFO(0, group, IS_STRING, 0)
	ZEND_ARG_TYPE_INFO(0, consumer, IS_STRING, 0)
	ZEND_ARG_TYPE_INFO(0, min_iddle, IS_LONG, 0)
	ZEND_ARG_TYPE_INFO(0, ids, IS_ARRAY, 0)
	ZEND_ARG_TYPE_INFO(0, options, IS_ARRAY, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_RedisCluster_xdel, 0, 2, IS_LONG, 0)
	ZEND_ARG_TYPE_INFO(0, key, IS_STRING, 0)
	ZEND_ARG_TYPE_INFO(0, ids, IS_ARRAY, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_RedisCluster_xgroup, 0, 1, IS_MIXED, 0)
	ZEND_ARG_TYPE_INFO(0, operation, IS_STRING, 0)
	ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, key, IS_STRING, 0, "null")
	ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, arg1, IS_STRING, 0, "null")
	ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, arg2, IS_STRING, 0, "null")
	ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, arg3, _IS_BOOL, 0, "false")
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_RedisCluster_xinfo, 0, 1, IS_MIXED, 0)
	ZEND_ARG_TYPE_INFO(0, operation, IS_STRING, 0)
	ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, arg1, IS_STRING, 0, "null")
	ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, arg2, IS_STRING, 0, "null")
ZEND_END_ARG_INFO()

#define arginfo_class_RedisCluster_xlen arginfo_class_RedisCluster_decr

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_RedisCluster_xpending, 0, 2, IS_STRING, 0)
	ZEND_ARG_TYPE_INFO(0, key, IS_STRING, 0)
	ZEND_ARG_TYPE_INFO(0, group, IS_STRING, 0)
	ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, start, IS_STRING, 0, "null")
	ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, end, IS_STRING, 0, "null")
	ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, count, IS_LONG, 0, "-1")
	ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, consumer, IS_STRING, 0, "null")
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_MASK_EX(arginfo_class_RedisCluster_xrange, 0, 3, MAY_BE_BOOL|MAY_BE_ARRAY)
	ZEND_ARG_TYPE_INFO(0, key, IS_STRING, 0)
	ZEND_ARG_TYPE_INFO(0, start, IS_STRING, 0)
	ZEND_ARG_TYPE_INFO(0, end, IS_STRING, 0)
	ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, count, IS_LONG, 0, "-1")
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_MASK_EX(arginfo_class_RedisCluster_xread, 0, 1, MAY_BE_BOOL|MAY_BE_ARRAY)
	ZEND_ARG_TYPE_INFO(0, streams, IS_ARRAY, 0)
	ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, count, IS_LONG, 0, "-1")
	ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, block, IS_LONG, 0, "-1")
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_MASK_EX(arginfo_class_RedisCluster_xreadgroup, 0, 3, MAY_BE_BOOL|MAY_BE_ARRAY)
	ZEND_ARG_TYPE_INFO(0, group, IS_STRING, 0)
	ZEND_ARG_TYPE_INFO(0, consumer, IS_STRING, 0)
	ZEND_ARG_TYPE_INFO(0, streams, IS_ARRAY, 0)
	ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, count, IS_LONG, 0, "1")
	ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, block, IS_LONG, 0, "1")
ZEND_END_ARG_INFO()

#define arginfo_class_RedisCluster_xrevrange arginfo_class_RedisCluster_xrange

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_RedisCluster_xtrim, 0, 2, IS_LONG, 0)
	ZEND_ARG_TYPE_INFO(0, key, IS_STRING, 0)
	ZEND_ARG_TYPE_INFO(0, maxlen, IS_LONG, 0)
	ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, approx, _IS_BOOL, 0, "false")
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_RedisCluster_zadd, 0, 3, IS_LONG, 0)
	ZEND_ARG_TYPE_INFO(0, key, IS_STRING, 0)
	ZEND_ARG_TYPE_INFO(0, score, IS_DOUBLE, 0)
	ZEND_ARG_TYPE_INFO(0, member, IS_STRING, 0)
	ZEND_ARG_VARIADIC_TYPE_INFO(0, extra_args, IS_MIXED, 0)
ZEND_END_ARG_INFO()

#define arginfo_class_RedisCluster_zcard arginfo_class_RedisCluster_decr

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_RedisCluster_zcount, 0, 3, IS_LONG, 0)
	ZEND_ARG_TYPE_INFO(0, key, IS_STRING, 0)
	ZEND_ARG_TYPE_INFO(0, start, IS_STRING, 0)
	ZEND_ARG_TYPE_INFO(0, end, IS_STRING, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_RedisCluster_zincrby, 0, 3, IS_DOUBLE, 0)
	ZEND_ARG_TYPE_INFO(0, key, IS_STRING, 0)
	ZEND_ARG_TYPE_INFO(0, value, IS_DOUBLE, 0)
	ZEND_ARG_TYPE_INFO(0, member, IS_STRING, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_RedisCluster_zinterstore, 0, 2, IS_LONG, 0)
	ZEND_ARG_TYPE_INFO(0, key, IS_STRING, 0)
	ZEND_ARG_TYPE_INFO(0, keys, IS_ARRAY, 0)
	ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, weights, IS_ARRAY, 0, "null")
	ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, aggregate, IS_STRING, 0, "null")
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_RedisCluster_zlexcount, 0, 3, IS_LONG, 0)
	ZEND_ARG_TYPE_INFO(0, key, IS_STRING, 0)
	ZEND_ARG_TYPE_INFO(0, min, IS_STRING, 0)
	ZEND_ARG_TYPE_INFO(0, max, IS_STRING, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_MASK_EX(arginfo_class_RedisCluster_zpopmax, 0, 1, MAY_BE_BOOL|MAY_BE_ARRAY)
	ZEND_ARG_TYPE_INFO(0, key, IS_STRING, 0)
	ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, value, IS_LONG, 0, "null")
ZEND_END_ARG_INFO()

#define arginfo_class_RedisCluster_zpopmin arginfo_class_RedisCluster_zpopmax

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_MASK_EX(arginfo_class_RedisCluster_zrange, 0, 3, MAY_BE_BOOL|MAY_BE_ARRAY)
	ZEND_ARG_TYPE_INFO(0, key, IS_STRING, 0)
	ZEND_ARG_TYPE_INFO(0, start, IS_LONG, 0)
	ZEND_ARG_TYPE_INFO(0, end, IS_LONG, 0)
	ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, options, IS_ARRAY, 0, "null")
ZEND_END_ARG_INFO()

#define arginfo_class_RedisCluster_zrangebylex arginfo_class_RedisCluster_zrange

#define arginfo_class_RedisCluster_zrangebyscore arginfo_class_RedisCluster_zrange

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_RedisCluster_zrank, 0, 2, IS_LONG, 0)
	ZEND_ARG_TYPE_INFO(0, key, IS_STRING, 0)
	ZEND_ARG_TYPE_INFO(0, member, IS_MIXED, 0)
ZEND_END_ARG_INFO()

#define arginfo_class_RedisCluster_zrem arginfo_class_RedisCluster_srem

#define arginfo_class_RedisCluster_zremrangebylex arginfo_class_RedisCluster_zlexcount

#define arginfo_class_RedisCluster_zremrangebyrank arginfo_class_RedisCluster_zlexcount

#define arginfo_class_RedisCluster_zremrangebyscore arginfo_class_RedisCluster_zlexcount

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_MASK_EX(arginfo_class_RedisCluster_zrevrange, 0, 3, MAY_BE_BOOL|MAY_BE_ARRAY)
	ZEND_ARG_TYPE_INFO(0, key, IS_STRING, 0)
	ZEND_ARG_TYPE_INFO(0, min, IS_STRING, 0)
	ZEND_ARG_TYPE_INFO(0, max, IS_STRING, 0)
	ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, options, IS_ARRAY, 0, "null")
ZEND_END_ARG_INFO()

#define arginfo_class_RedisCluster_zrevrangebylex arginfo_class_RedisCluster_zrevrange

#define arginfo_class_RedisCluster_zrevrangebyscore arginfo_class_RedisCluster_zrevrange

#define arginfo_class_RedisCluster_zrevrank arginfo_class_RedisCluster_zrank

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_MASK_EX(arginfo_class_RedisCluster_zscan, 0, 2, MAY_BE_BOOL|MAY_BE_ARRAY)
	ZEND_ARG_TYPE_INFO(0, key, IS_STRING, 0)
	ZEND_ARG_TYPE_INFO(1, iterator, IS_LONG, 0)
	ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, pattern, IS_STRING, 1, "null")
	ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, count, IS_LONG, 0, "0")
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_RedisCluster_zscore, 0, 1, IS_DOUBLE, 0)
	ZEND_ARG_TYPE_INFO(0, key, IS_STRING, 0)
ZEND_END_ARG_INFO()

#define arginfo_class_RedisCluster_zunionstore arginfo_class_RedisCluster_zinterstore


ZEND_METHOD(RedisCluster, __construct);
ZEND_METHOD(RedisCluster, _compress);
ZEND_METHOD(RedisCluster, _masters);
ZEND_METHOD(RedisCluster, _pack);
ZEND_METHOD(RedisCluster, _prefix);
ZEND_METHOD(RedisCluster, _redir);
ZEND_METHOD(RedisCluster, _serialize);
ZEND_METHOD(RedisCluster, _uncompress);
ZEND_METHOD(RedisCluster, _unpack);
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
ZEND_METHOD(RedisCluster, decrbyfloat);
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
ZEND_METHOD(RedisCluster, hdel);
ZEND_METHOD(RedisCluster, hexists);
ZEND_METHOD(RedisCluster, hget);
ZEND_METHOD(RedisCluster, hgetall);
ZEND_METHOD(RedisCluster, hincrby);
ZEND_METHOD(RedisCluster, hincrbyfloat);
ZEND_METHOD(RedisCluster, hkeys);
ZEND_METHOD(RedisCluster, hlen);
ZEND_METHOD(RedisCluster, hmget);
ZEND_METHOD(RedisCluster, hmset);
ZEND_METHOD(RedisCluster, hscan);
ZEND_METHOD(RedisCluster, hset);
ZEND_METHOD(RedisCluster, hsetnx);
ZEND_METHOD(RedisCluster, hstrlen);
ZEND_METHOD(RedisCluster, hvals);
ZEND_METHOD(RedisCluster, incr);
ZEND_METHOD(RedisCluster, incrby);
ZEND_METHOD(RedisCluster, incrbyfloat);
ZEND_METHOD(RedisCluster, info);
ZEND_METHOD(RedisCluster, keys);
ZEND_METHOD(RedisCluster, lastsave);
ZEND_METHOD(RedisCluster, lget);
ZEND_METHOD(RedisCluster, lindex);
ZEND_METHOD(RedisCluster, linsert);
ZEND_METHOD(RedisCluster, llen);
ZEND_METHOD(RedisCluster, lpop);
ZEND_METHOD(RedisCluster, lpush);
ZEND_METHOD(RedisCluster, lpushx);
ZEND_METHOD(RedisCluster, lrange);
ZEND_METHOD(RedisCluster, lrem);
ZEND_METHOD(RedisCluster, lset);
ZEND_METHOD(RedisCluster, ltrim);
ZEND_METHOD(RedisCluster, mget);
ZEND_METHOD(RedisCluster, mset);
ZEND_METHOD(RedisCluster, msetnx);
ZEND_METHOD(RedisCluster, multi);
ZEND_METHOD(RedisCluster, object);
ZEND_METHOD(RedisCluster, persist);
ZEND_METHOD(RedisCluster, pexpire);
ZEND_METHOD(RedisCluster, pexpireat);
ZEND_METHOD(RedisCluster, pfadd);
ZEND_METHOD(RedisCluster, pfcount);
ZEND_METHOD(RedisCluster, pfmerge);
ZEND_METHOD(RedisCluster, ping);
ZEND_METHOD(RedisCluster, psetex);
ZEND_METHOD(RedisCluster, psubscribe);
ZEND_METHOD(RedisCluster, pttl);
ZEND_METHOD(RedisCluster, publish);
ZEND_METHOD(RedisCluster, pubsub);
ZEND_METHOD(RedisCluster, punsubscribe);
ZEND_METHOD(RedisCluster, randomkey);
ZEND_METHOD(RedisCluster, rawcommand);
ZEND_METHOD(RedisCluster, rename);
ZEND_METHOD(RedisCluster, renamenx);
ZEND_METHOD(RedisCluster, restore);
ZEND_METHOD(RedisCluster, role);
ZEND_METHOD(RedisCluster, rpop);
ZEND_METHOD(RedisCluster, rpoplpush);
ZEND_METHOD(RedisCluster, rpush);
ZEND_METHOD(RedisCluster, rpushx);
ZEND_METHOD(RedisCluster, sadd);
ZEND_METHOD(RedisCluster, saddarray);
ZEND_METHOD(RedisCluster, save);
ZEND_METHOD(RedisCluster, scan);
ZEND_METHOD(RedisCluster, scard);
ZEND_METHOD(RedisCluster, script);
ZEND_METHOD(RedisCluster, sdiff);
ZEND_METHOD(RedisCluster, sdiffstore);
ZEND_METHOD(RedisCluster, set);
ZEND_METHOD(RedisCluster, setbit);
ZEND_METHOD(RedisCluster, setex);
ZEND_METHOD(RedisCluster, setnx);
ZEND_METHOD(RedisCluster, setoption);
ZEND_METHOD(RedisCluster, setrange);
ZEND_METHOD(RedisCluster, sinter);
ZEND_METHOD(RedisCluster, sinterstore);
ZEND_METHOD(RedisCluster, sismember);
ZEND_METHOD(RedisCluster, slowlog);
ZEND_METHOD(RedisCluster, smembers);
ZEND_METHOD(RedisCluster, smove);
ZEND_METHOD(RedisCluster, sort);
ZEND_METHOD(RedisCluster, spop);
ZEND_METHOD(RedisCluster, srandmember);
ZEND_METHOD(RedisCluster, srem);
ZEND_METHOD(RedisCluster, sscan);
ZEND_METHOD(RedisCluster, strlen);
ZEND_METHOD(RedisCluster, subscribe);
ZEND_METHOD(RedisCluster, sunion);
ZEND_METHOD(RedisCluster, sunionstore);
ZEND_METHOD(RedisCluster, time);
ZEND_METHOD(RedisCluster, ttl);
ZEND_METHOD(RedisCluster, type);
ZEND_METHOD(RedisCluster, unsubscribe);
ZEND_METHOD(RedisCluster, unlink);
ZEND_METHOD(RedisCluster, unwatch);
ZEND_METHOD(RedisCluster, watch);
ZEND_METHOD(RedisCluster, xack);
ZEND_METHOD(RedisCluster, xadd);
ZEND_METHOD(RedisCluster, xclaim);
ZEND_METHOD(RedisCluster, xdel);
ZEND_METHOD(RedisCluster, xgroup);
ZEND_METHOD(RedisCluster, xinfo);
ZEND_METHOD(RedisCluster, xlen);
ZEND_METHOD(RedisCluster, xpending);
ZEND_METHOD(RedisCluster, xrange);
ZEND_METHOD(RedisCluster, xread);
ZEND_METHOD(RedisCluster, xreadgroup);
ZEND_METHOD(RedisCluster, xrevrange);
ZEND_METHOD(RedisCluster, xtrim);
ZEND_METHOD(RedisCluster, zadd);
ZEND_METHOD(RedisCluster, zcard);
ZEND_METHOD(RedisCluster, zcount);
ZEND_METHOD(RedisCluster, zincrby);
ZEND_METHOD(RedisCluster, zinterstore);
ZEND_METHOD(RedisCluster, zlexcount);
ZEND_METHOD(RedisCluster, zpopmax);
ZEND_METHOD(RedisCluster, zpopmin);
ZEND_METHOD(RedisCluster, zrange);
ZEND_METHOD(RedisCluster, zrangebylex);
ZEND_METHOD(RedisCluster, zrangebyscore);
ZEND_METHOD(RedisCluster, zrank);
ZEND_METHOD(RedisCluster, zrem);
ZEND_METHOD(RedisCluster, zremrangebylex);
ZEND_METHOD(RedisCluster, zremrangebyrank);
ZEND_METHOD(RedisCluster, zremrangebyscore);
ZEND_METHOD(RedisCluster, zrevrange);
ZEND_METHOD(RedisCluster, zrevrangebylex);
ZEND_METHOD(RedisCluster, zrevrangebyscore);
ZEND_METHOD(RedisCluster, zrevrank);
ZEND_METHOD(RedisCluster, zscan);
ZEND_METHOD(RedisCluster, zscore);
ZEND_METHOD(RedisCluster, zunionstore);


static const zend_function_entry class_RedisCluster_methods[] = {
	ZEND_ME(RedisCluster, __construct, arginfo_class_RedisCluster___construct, ZEND_ACC_PUBLIC)
	ZEND_ME(RedisCluster, _compress, arginfo_class_RedisCluster__compress, ZEND_ACC_PUBLIC)
	ZEND_ME(RedisCluster, _masters, arginfo_class_RedisCluster__masters, ZEND_ACC_PUBLIC)
	ZEND_ME(RedisCluster, _pack, arginfo_class_RedisCluster__pack, ZEND_ACC_PUBLIC)
	ZEND_ME(RedisCluster, _prefix, arginfo_class_RedisCluster__prefix, ZEND_ACC_PUBLIC)
	ZEND_ME(RedisCluster, _redir, arginfo_class_RedisCluster__redir, ZEND_ACC_PUBLIC)
	ZEND_ME(RedisCluster, _serialize, arginfo_class_RedisCluster__serialize, ZEND_ACC_PUBLIC)
	ZEND_ME(RedisCluster, _uncompress, arginfo_class_RedisCluster__uncompress, ZEND_ACC_PUBLIC)
	ZEND_ME(RedisCluster, _unpack, arginfo_class_RedisCluster__unpack, ZEND_ACC_PUBLIC)
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
	ZEND_ME(RedisCluster, decrbyfloat, arginfo_class_RedisCluster_decrbyfloat, ZEND_ACC_PUBLIC)
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
	ZEND_ME(RedisCluster, hdel, arginfo_class_RedisCluster_hdel, ZEND_ACC_PUBLIC)
	ZEND_ME(RedisCluster, hexists, arginfo_class_RedisCluster_hexists, ZEND_ACC_PUBLIC)
	ZEND_ME(RedisCluster, hget, arginfo_class_RedisCluster_hget, ZEND_ACC_PUBLIC)
	ZEND_ME(RedisCluster, hgetall, arginfo_class_RedisCluster_hgetall, ZEND_ACC_PUBLIC)
	ZEND_ME(RedisCluster, hincrby, arginfo_class_RedisCluster_hincrby, ZEND_ACC_PUBLIC)
	ZEND_ME(RedisCluster, hincrbyfloat, arginfo_class_RedisCluster_hincrbyfloat, ZEND_ACC_PUBLIC)
	ZEND_ME(RedisCluster, hkeys, arginfo_class_RedisCluster_hkeys, ZEND_ACC_PUBLIC)
	ZEND_ME(RedisCluster, hlen, arginfo_class_RedisCluster_hlen, ZEND_ACC_PUBLIC)
	ZEND_ME(RedisCluster, hmget, arginfo_class_RedisCluster_hmget, ZEND_ACC_PUBLIC)
	ZEND_ME(RedisCluster, hmset, arginfo_class_RedisCluster_hmset, ZEND_ACC_PUBLIC)
	ZEND_ME(RedisCluster, hscan, arginfo_class_RedisCluster_hscan, ZEND_ACC_PUBLIC)
	ZEND_ME(RedisCluster, hset, arginfo_class_RedisCluster_hset, ZEND_ACC_PUBLIC)
	ZEND_ME(RedisCluster, hsetnx, arginfo_class_RedisCluster_hsetnx, ZEND_ACC_PUBLIC)
	ZEND_ME(RedisCluster, hstrlen, arginfo_class_RedisCluster_hstrlen, ZEND_ACC_PUBLIC)
	ZEND_ME(RedisCluster, hvals, arginfo_class_RedisCluster_hvals, ZEND_ACC_PUBLIC)
	ZEND_ME(RedisCluster, incr, arginfo_class_RedisCluster_incr, ZEND_ACC_PUBLIC)
	ZEND_ME(RedisCluster, incrby, arginfo_class_RedisCluster_incrby, ZEND_ACC_PUBLIC)
	ZEND_ME(RedisCluster, incrbyfloat, arginfo_class_RedisCluster_incrbyfloat, ZEND_ACC_PUBLIC)
	ZEND_ME(RedisCluster, info, arginfo_class_RedisCluster_info, ZEND_ACC_PUBLIC)
	ZEND_ME(RedisCluster, keys, arginfo_class_RedisCluster_keys, ZEND_ACC_PUBLIC)
	ZEND_ME(RedisCluster, lastsave, arginfo_class_RedisCluster_lastsave, ZEND_ACC_PUBLIC)
	ZEND_ME(RedisCluster, lget, arginfo_class_RedisCluster_lget, ZEND_ACC_PUBLIC)
	ZEND_ME(RedisCluster, lindex, arginfo_class_RedisCluster_lindex, ZEND_ACC_PUBLIC)
	ZEND_ME(RedisCluster, linsert, arginfo_class_RedisCluster_linsert, ZEND_ACC_PUBLIC)
	ZEND_ME(RedisCluster, llen, arginfo_class_RedisCluster_llen, ZEND_ACC_PUBLIC)
	ZEND_ME(RedisCluster, lpop, arginfo_class_RedisCluster_lpop, ZEND_ACC_PUBLIC)
	ZEND_ME(RedisCluster, lpush, arginfo_class_RedisCluster_lpush, ZEND_ACC_PUBLIC)
	ZEND_ME(RedisCluster, lpushx, arginfo_class_RedisCluster_lpushx, ZEND_ACC_PUBLIC)
	ZEND_ME(RedisCluster, lrange, arginfo_class_RedisCluster_lrange, ZEND_ACC_PUBLIC)
	ZEND_ME(RedisCluster, lrem, arginfo_class_RedisCluster_lrem, ZEND_ACC_PUBLIC)
	ZEND_ME(RedisCluster, lset, arginfo_class_RedisCluster_lset, ZEND_ACC_PUBLIC)
	ZEND_ME(RedisCluster, ltrim, arginfo_class_RedisCluster_ltrim, ZEND_ACC_PUBLIC)
	ZEND_ME(RedisCluster, mget, arginfo_class_RedisCluster_mget, ZEND_ACC_PUBLIC)
	ZEND_ME(RedisCluster, mset, arginfo_class_RedisCluster_mset, ZEND_ACC_PUBLIC)
	ZEND_ME(RedisCluster, msetnx, arginfo_class_RedisCluster_msetnx, ZEND_ACC_PUBLIC)
	ZEND_ME(RedisCluster, multi, arginfo_class_RedisCluster_multi, ZEND_ACC_PUBLIC)
	ZEND_ME(RedisCluster, object, arginfo_class_RedisCluster_object, ZEND_ACC_PUBLIC)
	ZEND_ME(RedisCluster, persist, arginfo_class_RedisCluster_persist, ZEND_ACC_PUBLIC)
	ZEND_ME(RedisCluster, pexpire, arginfo_class_RedisCluster_pexpire, ZEND_ACC_PUBLIC)
	ZEND_ME(RedisCluster, pexpireat, arginfo_class_RedisCluster_pexpireat, ZEND_ACC_PUBLIC)
	ZEND_ME(RedisCluster, pfadd, arginfo_class_RedisCluster_pfadd, ZEND_ACC_PUBLIC)
	ZEND_ME(RedisCluster, pfcount, arginfo_class_RedisCluster_pfcount, ZEND_ACC_PUBLIC)
	ZEND_ME(RedisCluster, pfmerge, arginfo_class_RedisCluster_pfmerge, ZEND_ACC_PUBLIC)
	ZEND_ME(RedisCluster, ping, arginfo_class_RedisCluster_ping, ZEND_ACC_PUBLIC)
	ZEND_ME(RedisCluster, psetex, arginfo_class_RedisCluster_psetex, ZEND_ACC_PUBLIC)
	ZEND_ME(RedisCluster, psubscribe, arginfo_class_RedisCluster_psubscribe, ZEND_ACC_PUBLIC)
	ZEND_ME(RedisCluster, pttl, arginfo_class_RedisCluster_pttl, ZEND_ACC_PUBLIC)
	ZEND_ME(RedisCluster, publish, arginfo_class_RedisCluster_publish, ZEND_ACC_PUBLIC)
	ZEND_ME(RedisCluster, pubsub, arginfo_class_RedisCluster_pubsub, ZEND_ACC_PUBLIC)
	ZEND_ME(RedisCluster, punsubscribe, arginfo_class_RedisCluster_punsubscribe, ZEND_ACC_PUBLIC)
	ZEND_ME(RedisCluster, randomkey, arginfo_class_RedisCluster_randomkey, ZEND_ACC_PUBLIC)
	ZEND_ME(RedisCluster, rawcommand, arginfo_class_RedisCluster_rawcommand, ZEND_ACC_PUBLIC)
	ZEND_ME(RedisCluster, rename, arginfo_class_RedisCluster_rename, ZEND_ACC_PUBLIC)
	ZEND_ME(RedisCluster, renamenx, arginfo_class_RedisCluster_renamenx, ZEND_ACC_PUBLIC)
	ZEND_ME(RedisCluster, restore, arginfo_class_RedisCluster_restore, ZEND_ACC_PUBLIC)
	ZEND_ME(RedisCluster, role, arginfo_class_RedisCluster_role, ZEND_ACC_PUBLIC)
	ZEND_ME(RedisCluster, rpop, arginfo_class_RedisCluster_rpop, ZEND_ACC_PUBLIC)
	ZEND_ME(RedisCluster, rpoplpush, arginfo_class_RedisCluster_rpoplpush, ZEND_ACC_PUBLIC)
	ZEND_ME(RedisCluster, rpush, arginfo_class_RedisCluster_rpush, ZEND_ACC_PUBLIC)
	ZEND_ME(RedisCluster, rpushx, arginfo_class_RedisCluster_rpushx, ZEND_ACC_PUBLIC)
	ZEND_ME(RedisCluster, sadd, arginfo_class_RedisCluster_sadd, ZEND_ACC_PUBLIC)
	ZEND_ME(RedisCluster, saddarray, arginfo_class_RedisCluster_saddarray, ZEND_ACC_PUBLIC)
	ZEND_ME(RedisCluster, save, arginfo_class_RedisCluster_save, ZEND_ACC_PUBLIC)
	ZEND_ME(RedisCluster, scan, arginfo_class_RedisCluster_scan, ZEND_ACC_PUBLIC)
	ZEND_ME(RedisCluster, scard, arginfo_class_RedisCluster_scard, ZEND_ACC_PUBLIC)
	ZEND_ME(RedisCluster, script, arginfo_class_RedisCluster_script, ZEND_ACC_PUBLIC)
	ZEND_ME(RedisCluster, sdiff, arginfo_class_RedisCluster_sdiff, ZEND_ACC_PUBLIC)
	ZEND_ME(RedisCluster, sdiffstore, arginfo_class_RedisCluster_sdiffstore, ZEND_ACC_PUBLIC)
	ZEND_ME(RedisCluster, set, arginfo_class_RedisCluster_set, ZEND_ACC_PUBLIC)
	ZEND_ME(RedisCluster, setbit, arginfo_class_RedisCluster_setbit, ZEND_ACC_PUBLIC)
	ZEND_ME(RedisCluster, setex, arginfo_class_RedisCluster_setex, ZEND_ACC_PUBLIC)
	ZEND_ME(RedisCluster, setnx, arginfo_class_RedisCluster_setnx, ZEND_ACC_PUBLIC)
	ZEND_ME(RedisCluster, setoption, arginfo_class_RedisCluster_setoption, ZEND_ACC_PUBLIC)
	ZEND_ME(RedisCluster, setrange, arginfo_class_RedisCluster_setrange, ZEND_ACC_PUBLIC)
	ZEND_ME(RedisCluster, sinter, arginfo_class_RedisCluster_sinter, ZEND_ACC_PUBLIC)
	ZEND_ME(RedisCluster, sinterstore, arginfo_class_RedisCluster_sinterstore, ZEND_ACC_PUBLIC)
	ZEND_ME(RedisCluster, sismember, arginfo_class_RedisCluster_sismember, ZEND_ACC_PUBLIC)
	ZEND_ME(RedisCluster, slowlog, arginfo_class_RedisCluster_slowlog, ZEND_ACC_PUBLIC)
	ZEND_ME(RedisCluster, smembers, arginfo_class_RedisCluster_smembers, ZEND_ACC_PUBLIC)
	ZEND_ME(RedisCluster, smove, arginfo_class_RedisCluster_smove, ZEND_ACC_PUBLIC)
	ZEND_ME(RedisCluster, sort, arginfo_class_RedisCluster_sort, ZEND_ACC_PUBLIC)
	ZEND_ME(RedisCluster, spop, arginfo_class_RedisCluster_spop, ZEND_ACC_PUBLIC)
	ZEND_ME(RedisCluster, srandmember, arginfo_class_RedisCluster_srandmember, ZEND_ACC_PUBLIC)
	ZEND_ME(RedisCluster, srem, arginfo_class_RedisCluster_srem, ZEND_ACC_PUBLIC)
	ZEND_ME(RedisCluster, sscan, arginfo_class_RedisCluster_sscan, ZEND_ACC_PUBLIC)
	ZEND_ME(RedisCluster, strlen, arginfo_class_RedisCluster_strlen, ZEND_ACC_PUBLIC)
	ZEND_ME(RedisCluster, subscribe, arginfo_class_RedisCluster_subscribe, ZEND_ACC_PUBLIC)
	ZEND_ME(RedisCluster, sunion, arginfo_class_RedisCluster_sunion, ZEND_ACC_PUBLIC)
	ZEND_ME(RedisCluster, sunionstore, arginfo_class_RedisCluster_sunionstore, ZEND_ACC_PUBLIC)
	ZEND_ME(RedisCluster, time, arginfo_class_RedisCluster_time, ZEND_ACC_PUBLIC)
	ZEND_ME(RedisCluster, ttl, arginfo_class_RedisCluster_ttl, ZEND_ACC_PUBLIC)
	ZEND_ME(RedisCluster, type, arginfo_class_RedisCluster_type, ZEND_ACC_PUBLIC)
	ZEND_ME(RedisCluster, unsubscribe, arginfo_class_RedisCluster_unsubscribe, ZEND_ACC_PUBLIC)
	ZEND_ME(RedisCluster, unlink, arginfo_class_RedisCluster_unlink, ZEND_ACC_PUBLIC)
	ZEND_ME(RedisCluster, unwatch, arginfo_class_RedisCluster_unwatch, ZEND_ACC_PUBLIC)
	ZEND_ME(RedisCluster, watch, arginfo_class_RedisCluster_watch, ZEND_ACC_PUBLIC)
	ZEND_ME(RedisCluster, xack, arginfo_class_RedisCluster_xack, ZEND_ACC_PUBLIC)
	ZEND_ME(RedisCluster, xadd, arginfo_class_RedisCluster_xadd, ZEND_ACC_PUBLIC)
	ZEND_ME(RedisCluster, xclaim, arginfo_class_RedisCluster_xclaim, ZEND_ACC_PUBLIC)
	ZEND_ME(RedisCluster, xdel, arginfo_class_RedisCluster_xdel, ZEND_ACC_PUBLIC)
	ZEND_ME(RedisCluster, xgroup, arginfo_class_RedisCluster_xgroup, ZEND_ACC_PUBLIC)
	ZEND_ME(RedisCluster, xinfo, arginfo_class_RedisCluster_xinfo, ZEND_ACC_PUBLIC)
	ZEND_ME(RedisCluster, xlen, arginfo_class_RedisCluster_xlen, ZEND_ACC_PUBLIC)
	ZEND_ME(RedisCluster, xpending, arginfo_class_RedisCluster_xpending, ZEND_ACC_PUBLIC)
	ZEND_ME(RedisCluster, xrange, arginfo_class_RedisCluster_xrange, ZEND_ACC_PUBLIC)
	ZEND_ME(RedisCluster, xread, arginfo_class_RedisCluster_xread, ZEND_ACC_PUBLIC)
	ZEND_ME(RedisCluster, xreadgroup, arginfo_class_RedisCluster_xreadgroup, ZEND_ACC_PUBLIC)
	ZEND_ME(RedisCluster, xrevrange, arginfo_class_RedisCluster_xrevrange, ZEND_ACC_PUBLIC)
	ZEND_ME(RedisCluster, xtrim, arginfo_class_RedisCluster_xtrim, ZEND_ACC_PUBLIC)
	ZEND_ME(RedisCluster, zadd, arginfo_class_RedisCluster_zadd, ZEND_ACC_PUBLIC)
	ZEND_ME(RedisCluster, zcard, arginfo_class_RedisCluster_zcard, ZEND_ACC_PUBLIC)
	ZEND_ME(RedisCluster, zcount, arginfo_class_RedisCluster_zcount, ZEND_ACC_PUBLIC)
	ZEND_ME(RedisCluster, zincrby, arginfo_class_RedisCluster_zincrby, ZEND_ACC_PUBLIC)
	ZEND_ME(RedisCluster, zinterstore, arginfo_class_RedisCluster_zinterstore, ZEND_ACC_PUBLIC)
	ZEND_ME(RedisCluster, zlexcount, arginfo_class_RedisCluster_zlexcount, ZEND_ACC_PUBLIC)
	ZEND_ME(RedisCluster, zpopmax, arginfo_class_RedisCluster_zpopmax, ZEND_ACC_PUBLIC)
	ZEND_ME(RedisCluster, zpopmin, arginfo_class_RedisCluster_zpopmin, ZEND_ACC_PUBLIC)
	ZEND_ME(RedisCluster, zrange, arginfo_class_RedisCluster_zrange, ZEND_ACC_PUBLIC)
	ZEND_ME(RedisCluster, zrangebylex, arginfo_class_RedisCluster_zrangebylex, ZEND_ACC_PUBLIC)
	ZEND_ME(RedisCluster, zrangebyscore, arginfo_class_RedisCluster_zrangebyscore, ZEND_ACC_PUBLIC)
	ZEND_ME(RedisCluster, zrank, arginfo_class_RedisCluster_zrank, ZEND_ACC_PUBLIC)
	ZEND_ME(RedisCluster, zrem, arginfo_class_RedisCluster_zrem, ZEND_ACC_PUBLIC)
	ZEND_ME(RedisCluster, zremrangebylex, arginfo_class_RedisCluster_zremrangebylex, ZEND_ACC_PUBLIC)
	ZEND_ME(RedisCluster, zremrangebyrank, arginfo_class_RedisCluster_zremrangebyrank, ZEND_ACC_PUBLIC)
	ZEND_ME(RedisCluster, zremrangebyscore, arginfo_class_RedisCluster_zremrangebyscore, ZEND_ACC_PUBLIC)
	ZEND_ME(RedisCluster, zrevrange, arginfo_class_RedisCluster_zrevrange, ZEND_ACC_PUBLIC)
	ZEND_ME(RedisCluster, zrevrangebylex, arginfo_class_RedisCluster_zrevrangebylex, ZEND_ACC_PUBLIC)
	ZEND_ME(RedisCluster, zrevrangebyscore, arginfo_class_RedisCluster_zrevrangebyscore, ZEND_ACC_PUBLIC)
	ZEND_ME(RedisCluster, zrevrank, arginfo_class_RedisCluster_zrevrank, ZEND_ACC_PUBLIC)
	ZEND_ME(RedisCluster, zscan, arginfo_class_RedisCluster_zscan, ZEND_ACC_PUBLIC)
	ZEND_ME(RedisCluster, zscore, arginfo_class_RedisCluster_zscore, ZEND_ACC_PUBLIC)
	ZEND_ME(RedisCluster, zunionstore, arginfo_class_RedisCluster_zunionstore, ZEND_ACC_PUBLIC)
	ZEND_FE_END
};
