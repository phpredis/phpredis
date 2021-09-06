/* This is a generated file, edit the .stub.php file instead.
 * Stub hash: 15e5928c22404f33c5790ad9a8c7beed4d485b86 */

ZEND_BEGIN_ARG_INFO_EX(arginfo_class_Redis___construct, 0, 0, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_Redis__compress, 0, 1, IS_STRING, 0)
	ZEND_ARG_TYPE_INFO(0, value, IS_STRING, 0)
ZEND_END_ARG_INFO()

#define arginfo_class_Redis___destruct arginfo_class_Redis___construct

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_Redis__pack, 0, 1, IS_STRING, 0)
	ZEND_ARG_TYPE_INFO(0, value, IS_MIXED, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_Redis__prefix, 0, 1, IS_STRING, 0)
	ZEND_ARG_TYPE_INFO(0, key, IS_STRING, 0)
ZEND_END_ARG_INFO()

#define arginfo_class_Redis__serialize arginfo_class_Redis__pack

#define arginfo_class_Redis__uncompress arginfo_class_Redis__compress

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_Redis__unpack, 0, 1, IS_MIXED, 0)
	ZEND_ARG_TYPE_INFO(0, value, IS_STRING, 0)
ZEND_END_ARG_INFO()

#define arginfo_class_Redis__unserialize arginfo_class_Redis__unpack

ZEND_BEGIN_ARG_INFO_EX(arginfo_class_Redis_acl, 0, 0, 1)
	ZEND_ARG_TYPE_INFO(0, subcmd, IS_STRING, 0)
	ZEND_ARG_VARIADIC_INFO(0, args)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_class_Redis_append, 0, 0, 2)
	ZEND_ARG_TYPE_INFO(0, key, IS_STRING, 0)
	ZEND_ARG_TYPE_INFO(0, value, IS_MIXED, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_Redis_auth, 0, 1, _IS_BOOL, 0)
	ZEND_ARG_TYPE_INFO(0, credentials, IS_MIXED, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_Redis_bgSave, 0, 0, _IS_BOOL, 0)
ZEND_END_ARG_INFO()

#define arginfo_class_Redis_bgrewriteaof arginfo_class_Redis_bgSave

ZEND_BEGIN_ARG_INFO_EX(arginfo_class_Redis_bitcount, 0, 0, 1)
	ZEND_ARG_TYPE_INFO(0, key, IS_STRING, 0)
	ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, start, IS_LONG, 0, "0")
	ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, end, IS_LONG, 0, "-1")
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_Redis_bitop, 0, 3, IS_LONG, 0)
	ZEND_ARG_TYPE_INFO(0, operation, IS_STRING, 0)
	ZEND_ARG_TYPE_INFO(0, deskey, IS_STRING, 0)
	ZEND_ARG_TYPE_INFO(0, srckey, IS_STRING, 0)
	ZEND_ARG_VARIADIC_INFO(0, otherkeys)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_class_Redis_bitpos, 0, 0, 2)
	ZEND_ARG_TYPE_INFO(0, key, IS_STRING, 0)
	ZEND_ARG_TYPE_INFO(0, bit, IS_LONG, 0)
	ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, start, IS_LONG, 0, "0")
	ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, end, IS_LONG, 0, "-1")
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_Redis_blPop, 0, 2, IS_ARRAY, 0)
	ZEND_ARG_TYPE_MASK(0, key, MAY_BE_STRING|MAY_BE_ARRAY, NULL)
	ZEND_ARG_TYPE_MASK(0, timeout_or_key, MAY_BE_STRING|MAY_BE_LONG, NULL)
	ZEND_ARG_VARIADIC_TYPE_INFO(0, extra_args, IS_MIXED, 0)
ZEND_END_ARG_INFO()

#define arginfo_class_Redis_brPop arginfo_class_Redis_blPop

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_Redis_brpoplpush, 0, 3, IS_STRING, 0)
	ZEND_ARG_TYPE_INFO(0, src, IS_STRING, 0)
	ZEND_ARG_TYPE_INFO(0, dst, IS_STRING, 0)
	ZEND_ARG_TYPE_INFO(0, timeout, IS_LONG, 0)
ZEND_END_ARG_INFO()

#define arginfo_class_Redis_bzPopMax arginfo_class_Redis_blPop

#define arginfo_class_Redis_bzPopMin arginfo_class_Redis_blPop

#define arginfo_class_Redis_clearLastError arginfo_class_Redis_bgSave

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_Redis_client, 0, 1, IS_MIXED, 0)
	ZEND_ARG_TYPE_INFO(0, opt, IS_STRING, 0)
	ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, arg, IS_STRING, 0, "null")
ZEND_END_ARG_INFO()

#define arginfo_class_Redis_close arginfo_class_Redis_bgSave

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_Redis_command, 0, 2, IS_MIXED, 0)
	ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, opt, IS_STRING, 0, "null")
	ZEND_ARG_TYPE_MASK(0, arg, MAY_BE_STRING|MAY_BE_ARRAY, NULL)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_Redis_config, 0, 2, IS_MIXED, 0)
	ZEND_ARG_TYPE_INFO(0, operation, IS_STRING, 0)
	ZEND_ARG_TYPE_INFO(0, key, IS_STRING, 0)
	ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, value, IS_MIXED, 0, "null")
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_Redis_connect, 0, 1, _IS_BOOL, 0)
	ZEND_ARG_TYPE_INFO(0, host, IS_STRING, 0)
	ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, port, IS_LONG, 0, "26379")
	ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, timeout, IS_DOUBLE, 0, "0")
	ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, persistent_id, IS_STRING, 0, "null")
	ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, retry_interval, IS_LONG, 0, "0")
	ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, read_timeout, IS_DOUBLE, 0, "0")
	ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, context, IS_ARRAY, 0, "null")
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_Redis_copy, 0, 2, _IS_BOOL, 0)
	ZEND_ARG_TYPE_INFO(0, src, IS_STRING, 0)
	ZEND_ARG_TYPE_INFO(0, dst, IS_STRING, 0)
	ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, options, IS_ARRAY, 0, "null")
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_Redis_dbSize, 0, 0, IS_LONG, 0)
ZEND_END_ARG_INFO()

#define arginfo_class_Redis_debug arginfo_class_Redis__prefix

ZEND_BEGIN_ARG_INFO_EX(arginfo_class_Redis_decr, 0, 0, 1)
	ZEND_ARG_TYPE_INFO(0, key, IS_STRING, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_class_Redis_decrBy, 0, 0, 2)
	ZEND_ARG_TYPE_INFO(0, key, IS_STRING, 0)
	ZEND_ARG_TYPE_INFO(0, value, IS_LONG, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_class_Redis_del, 0, 0, 1)
	ZEND_ARG_TYPE_MASK(0, key, MAY_BE_ARRAY|MAY_BE_STRING, NULL)
	ZEND_ARG_VARIADIC_INFO(0, otherkeys)
ZEND_END_ARG_INFO()

#define arginfo_class_Redis_delete arginfo_class_Redis_del

#define arginfo_class_Redis_discard arginfo_class_Redis_bgSave

#define arginfo_class_Redis_dump arginfo_class_Redis__prefix

ZEND_BEGIN_ARG_INFO_EX(arginfo_class_Redis_echo, 0, 0, 1)
	ZEND_ARG_TYPE_INFO(0, str, IS_STRING, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_Redis_eval, 0, 1, IS_MIXED, 0)
	ZEND_ARG_TYPE_INFO(0, script, IS_STRING, 0)
	ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, keys, IS_ARRAY, 0, "null")
	ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, num_keys, IS_LONG, 0, "0")
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_Redis_evalsha, 0, 1, IS_MIXED, 0)
	ZEND_ARG_TYPE_INFO(0, sha1, IS_STRING, 0)
	ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, keys, IS_ARRAY, 0, "null")
	ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, num_keys, IS_LONG, 0, "0")
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_Redis_exec, 0, 0, IS_ARRAY, 0)
ZEND_END_ARG_INFO()

#define arginfo_class_Redis_exists arginfo_class_Redis_decr

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_Redis_expire, 0, 2, _IS_BOOL, 0)
	ZEND_ARG_TYPE_INFO(0, key, IS_STRING, 0)
	ZEND_ARG_TYPE_INFO(0, timeout, IS_LONG, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_Redis_expireAt, 0, 2, _IS_BOOL, 0)
	ZEND_ARG_TYPE_INFO(0, key, IS_STRING, 0)
	ZEND_ARG_TYPE_INFO(0, timestamp, IS_LONG, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_Redis_flushAll, 0, 0, _IS_BOOL, 0)
	ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, async, _IS_BOOL, 0, "false")
ZEND_END_ARG_INFO()

#define arginfo_class_Redis_flushDB arginfo_class_Redis_flushAll

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_Redis_geoadd, 0, 4, IS_LONG, 0)
	ZEND_ARG_TYPE_INFO(0, key, IS_STRING, 0)
	ZEND_ARG_TYPE_INFO(0, lng, IS_DOUBLE, 0)
	ZEND_ARG_TYPE_INFO(0, lat, IS_DOUBLE, 0)
	ZEND_ARG_TYPE_INFO(0, member, IS_STRING, 0)
	ZEND_ARG_VARIADIC_TYPE_INFO(0, other_triples, IS_MIXED, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_Redis_geodist, 0, 3, IS_ARRAY, 0)
	ZEND_ARG_TYPE_INFO(0, key, IS_STRING, 0)
	ZEND_ARG_TYPE_INFO(0, src, IS_STRING, 0)
	ZEND_ARG_TYPE_INFO(0, dst, IS_STRING, 0)
	ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, unit, IS_STRING, 1, "null")
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_Redis_geohash, 0, 2, IS_ARRAY, 0)
	ZEND_ARG_TYPE_INFO(0, key, IS_STRING, 0)
	ZEND_ARG_TYPE_INFO(0, member, IS_STRING, 0)
	ZEND_ARG_VARIADIC_TYPE_INFO(0, other_members, IS_STRING, 0)
ZEND_END_ARG_INFO()

#define arginfo_class_Redis_geopos arginfo_class_Redis_geohash

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_Redis_georadius, 0, 5, IS_ARRAY, 0)
	ZEND_ARG_TYPE_INFO(0, key, IS_STRING, 0)
	ZEND_ARG_TYPE_INFO(0, lng, IS_DOUBLE, 0)
	ZEND_ARG_TYPE_INFO(0, lat, IS_DOUBLE, 0)
	ZEND_ARG_TYPE_INFO(0, radius, IS_DOUBLE, 0)
	ZEND_ARG_TYPE_INFO(0, unit, IS_STRING, 0)
	ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, options, IS_ARRAY, 0, "[]")
ZEND_END_ARG_INFO()

#define arginfo_class_Redis_georadius_ro arginfo_class_Redis_georadius

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_Redis_georadiusbymember, 0, 4, IS_ARRAY, 0)
	ZEND_ARG_TYPE_INFO(0, key, IS_STRING, 0)
	ZEND_ARG_TYPE_INFO(0, member, IS_STRING, 0)
	ZEND_ARG_TYPE_INFO(0, radius, IS_DOUBLE, 0)
	ZEND_ARG_TYPE_INFO(0, unit, IS_STRING, 0)
	ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, options, IS_ARRAY, 0, "[]")
ZEND_END_ARG_INFO()

#define arginfo_class_Redis_georadiusbymember_ro arginfo_class_Redis_georadiusbymember

#define arginfo_class_Redis_get arginfo_class_Redis_decr

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_Redis_getAuth, 0, 0, IS_MIXED, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_class_Redis_getBit, 0, 0, 2)
	ZEND_ARG_TYPE_INFO(0, key, IS_STRING, 0)
	ZEND_ARG_TYPE_INFO(0, idx, IS_LONG, 0)
ZEND_END_ARG_INFO()

#define arginfo_class_Redis_getDBNum arginfo_class_Redis_dbSize

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_Redis_getHost, 0, 0, IS_STRING, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_Redis_getLastError, 0, 0, IS_STRING, 1)
ZEND_END_ARG_INFO()

#define arginfo_class_Redis_getMode arginfo_class_Redis_dbSize

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_Redis_getOption, 0, 1, IS_MIXED, 0)
	ZEND_ARG_TYPE_INFO(0, option, IS_LONG, 0)
ZEND_END_ARG_INFO()

#define arginfo_class_Redis_getPersistentID arginfo_class_Redis_getLastError

#define arginfo_class_Redis_getPort arginfo_class_Redis_dbSize

ZEND_BEGIN_ARG_INFO_EX(arginfo_class_Redis_getRange, 0, 0, 3)
	ZEND_ARG_TYPE_INFO(0, key, IS_STRING, 0)
	ZEND_ARG_TYPE_INFO(0, start, IS_LONG, 0)
	ZEND_ARG_TYPE_INFO(0, end, IS_LONG, 0)
ZEND_END_ARG_INFO()

#define arginfo_class_Redis_getReadTimeout arginfo_class_Redis_dbSize

#define arginfo_class_Redis_getset arginfo_class_Redis_append

#define arginfo_class_Redis_getTimeout arginfo_class_Redis_dbSize

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_Redis_hDel, 0, 2, IS_LONG, 0)
	ZEND_ARG_TYPE_INFO(0, key, IS_STRING, 0)
	ZEND_ARG_TYPE_INFO(0, member, IS_STRING, 0)
	ZEND_ARG_VARIADIC_TYPE_INFO(0, other_members, IS_STRING, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_Redis_hExists, 0, 2, _IS_BOOL, 0)
	ZEND_ARG_TYPE_INFO(0, key, IS_STRING, 0)
	ZEND_ARG_TYPE_INFO(0, member, IS_STRING, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_Redis_hGet, 0, 2, IS_STRING, 0)
	ZEND_ARG_TYPE_INFO(0, key, IS_STRING, 0)
	ZEND_ARG_TYPE_INFO(0, member, IS_STRING, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_Redis_hGetAll, 0, 1, IS_ARRAY, 0)
	ZEND_ARG_TYPE_INFO(0, key, IS_STRING, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_Redis_hIncrBy, 0, 3, IS_LONG, 0)
	ZEND_ARG_TYPE_INFO(0, key, IS_STRING, 0)
	ZEND_ARG_TYPE_INFO(0, member, IS_STRING, 0)
	ZEND_ARG_TYPE_INFO(0, value, IS_LONG, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_Redis_hIncrByFloat, 0, 3, IS_DOUBLE, 0)
	ZEND_ARG_TYPE_INFO(0, key, IS_STRING, 0)
	ZEND_ARG_TYPE_INFO(0, member, IS_STRING, 0)
	ZEND_ARG_TYPE_INFO(0, value, IS_DOUBLE, 0)
ZEND_END_ARG_INFO()

#define arginfo_class_Redis_hKeys arginfo_class_Redis_hGetAll

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_Redis_hLen, 0, 1, IS_LONG, 0)
	ZEND_ARG_TYPE_INFO(0, key, IS_STRING, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_Redis_hMget, 0, 2, IS_ARRAY, 0)
	ZEND_ARG_TYPE_INFO(0, key, IS_STRING, 0)
	ZEND_ARG_TYPE_INFO(0, keys, IS_ARRAY, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_Redis_hMset, 0, 2, _IS_BOOL, 0)
	ZEND_ARG_TYPE_INFO(0, key, IS_STRING, 0)
	ZEND_ARG_TYPE_INFO(0, keyvals, IS_ARRAY, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_Redis_hSet, 0, 3, IS_LONG, 0)
	ZEND_ARG_TYPE_INFO(0, key, IS_STRING, 0)
	ZEND_ARG_TYPE_INFO(0, member, IS_STRING, 0)
	ZEND_ARG_TYPE_INFO(0, value, IS_STRING, 0)
ZEND_END_ARG_INFO()

#define arginfo_class_Redis_hSetNx arginfo_class_Redis_hSet

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_Redis_hStrLen, 0, 2, IS_LONG, 0)
	ZEND_ARG_TYPE_INFO(0, key, IS_STRING, 0)
	ZEND_ARG_TYPE_INFO(0, member, IS_STRING, 0)
ZEND_END_ARG_INFO()

#define arginfo_class_Redis_hVals arginfo_class_Redis_hGetAll

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_MASK_EX(arginfo_class_Redis_hscan, 0, 2, MAY_BE_BOOL|MAY_BE_ARRAY)
	ZEND_ARG_TYPE_INFO(0, key, IS_STRING, 0)
	ZEND_ARG_TYPE_INFO(0, iterator, IS_LONG, 0)
	ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, pattern, IS_STRING, 1, "null")
	ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, count, IS_LONG, 0, "0")
ZEND_END_ARG_INFO()

#define arginfo_class_Redis_incr arginfo_class_Redis_decr

#define arginfo_class_Redis_incrBy arginfo_class_Redis_decrBy

ZEND_BEGIN_ARG_INFO_EX(arginfo_class_Redis_incrByFloat, 0, 0, 2)
	ZEND_ARG_TYPE_INFO(0, key, IS_STRING, 0)
	ZEND_ARG_TYPE_INFO(0, value, IS_DOUBLE, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_Redis_info, 0, 0, IS_ARRAY, 0)
	ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, opt, IS_STRING, 0, "null")
ZEND_END_ARG_INFO()

#define arginfo_class_Redis_isConnected arginfo_class_Redis_bgSave

ZEND_BEGIN_ARG_INFO_EX(arginfo_class_Redis_keys, 0, 0, 1)
	ZEND_ARG_TYPE_INFO(0, pattern, IS_STRING, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_class_Redis_lInsert, 0, 0, 4)
	ZEND_ARG_TYPE_INFO(0, key, IS_STRING, 0)
	ZEND_ARG_TYPE_INFO(0, pos, IS_STRING, 0)
	ZEND_ARG_TYPE_INFO(0, pivot, IS_MIXED, 0)
	ZEND_ARG_TYPE_INFO(0, value, IS_MIXED, 0)
ZEND_END_ARG_INFO()

#define arginfo_class_Redis_lLen arginfo_class_Redis_hLen

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_Redis_lMove, 0, 4, IS_STRING, 0)
	ZEND_ARG_TYPE_INFO(0, src, IS_STRING, 0)
	ZEND_ARG_TYPE_INFO(0, dst, IS_STRING, 0)
	ZEND_ARG_TYPE_INFO(0, wherefrom, IS_STRING, 0)
	ZEND_ARG_TYPE_INFO(0, whereto, IS_STRING, 0)
ZEND_END_ARG_INFO()

#define arginfo_class_Redis_lPop arginfo_class_Redis_decr

ZEND_BEGIN_ARG_INFO_EX(arginfo_class_Redis_lPush, 0, 0, 1)
	ZEND_ARG_TYPE_INFO(0, key, IS_STRING, 0)
	ZEND_ARG_VARIADIC_INFO(0, elements)
ZEND_END_ARG_INFO()

#define arginfo_class_Redis_rPush arginfo_class_Redis_lPush

#define arginfo_class_Redis_lPushx arginfo_class_Redis_append

#define arginfo_class_Redis_rPushx arginfo_class_Redis_append

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_Redis_lSet, 0, 3, _IS_BOOL, 0)
	ZEND_ARG_TYPE_INFO(0, key, IS_STRING, 0)
	ZEND_ARG_TYPE_INFO(0, index, IS_LONG, 0)
	ZEND_ARG_TYPE_INFO(0, value, IS_STRING, 0)
ZEND_END_ARG_INFO()

#define arginfo_class_Redis_lastSave arginfo_class_Redis_dbSize

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_Redis_lindex, 0, 2, IS_STRING, 0)
	ZEND_ARG_TYPE_INFO(0, key, IS_STRING, 0)
	ZEND_ARG_TYPE_INFO(0, index, IS_LONG, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_Redis_lrange, 0, 3, IS_ARRAY, 0)
	ZEND_ARG_TYPE_INFO(0, key, IS_STRING, 0)
	ZEND_ARG_TYPE_INFO(0, start, IS_LONG, 0)
	ZEND_ARG_TYPE_INFO(0, end, IS_LONG, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_Redis_lrem, 0, 2, _IS_BOOL, 0)
	ZEND_ARG_TYPE_INFO(0, key, IS_STRING, 0)
	ZEND_ARG_TYPE_INFO(0, value, IS_STRING, 0)
	ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, count, IS_LONG, 0, "0")
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_Redis_ltrim, 0, 3, _IS_BOOL, 0)
	ZEND_ARG_TYPE_INFO(0, key, IS_STRING, 0)
	ZEND_ARG_TYPE_INFO(0, start, IS_LONG, 0)
	ZEND_ARG_TYPE_INFO(0, end, IS_LONG, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_class_Redis_mget, 0, 0, 1)
	ZEND_ARG_TYPE_INFO(0, keys, IS_ARRAY, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_Redis_migrate, 0, 5, _IS_BOOL, 0)
	ZEND_ARG_TYPE_INFO(0, host, IS_STRING, 0)
	ZEND_ARG_TYPE_INFO(0, port, IS_LONG, 0)
	ZEND_ARG_TYPE_INFO(0, key, IS_STRING, 0)
	ZEND_ARG_TYPE_INFO(0, dst, IS_STRING, 0)
	ZEND_ARG_TYPE_INFO(0, timeout, IS_LONG, 0)
	ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, copy, _IS_BOOL, 0, "false")
	ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, replace, _IS_BOOL, 0, "false")
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_Redis_move, 0, 2, _IS_BOOL, 0)
	ZEND_ARG_TYPE_INFO(0, key, IS_STRING, 0)
	ZEND_ARG_TYPE_INFO(0, index, IS_LONG, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_Redis_mset, 0, 1, _IS_BOOL, 0)
	ZEND_ARG_TYPE_INFO(0, key_values, IS_ARRAY, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_Redis_msetnx, 0, 1, IS_LONG, 0)
	ZEND_ARG_TYPE_INFO(0, key_values, IS_ARRAY, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_OBJ_TYPE_MASK_EX(arginfo_class_Redis_multi, 0, 0, Redis, MAY_BE_BOOL)
	ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, value, IS_LONG, 0, "Redis::MULTI")
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_MASK_EX(arginfo_class_Redis_object, 0, 1, MAY_BE_LONG|MAY_BE_STRING)
	ZEND_ARG_TYPE_INFO(0, key, IS_STRING, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_Redis_open, 0, 1, _IS_BOOL, 0)
	ZEND_ARG_TYPE_INFO(0, host, IS_STRING, 0)
	ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, port, IS_LONG, 0, "26379")
	ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, timeout, IS_DOUBLE, 0, "0")
	ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, persistent_id, IS_STRING, 0, "NULL")
	ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, retry_interval, IS_LONG, 0, "0")
	ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, read_timeout, IS_DOUBLE, 0, "0")
	ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, context, IS_ARRAY, 0, "NULL")
ZEND_END_ARG_INFO()

#define arginfo_class_Redis_pconnect arginfo_class_Redis_open

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_Redis_persist, 0, 1, _IS_BOOL, 0)
	ZEND_ARG_TYPE_INFO(0, key, IS_STRING, 0)
ZEND_END_ARG_INFO()

#define arginfo_class_Redis_pexpire arginfo_class_Redis_expire

#define arginfo_class_Redis_pexpireAt arginfo_class_Redis_expireAt

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_Redis_pfadd, 0, 2, IS_LONG, 0)
	ZEND_ARG_TYPE_INFO(0, key, IS_STRING, 0)
	ZEND_ARG_TYPE_INFO(0, elements, IS_ARRAY, 0)
ZEND_END_ARG_INFO()

#define arginfo_class_Redis_pfcount arginfo_class_Redis_hLen

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_Redis_pfmerge, 0, 2, _IS_BOOL, 0)
	ZEND_ARG_TYPE_INFO(0, dst, IS_STRING, 0)
	ZEND_ARG_TYPE_INFO(0, keys, IS_ARRAY, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_class_Redis_ping, 0, 0, 0)
	ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, key, IS_STRING, 0, "NULL")
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_OBJ_TYPE_MASK_EX(arginfo_class_Redis_pipeline, 0, 0, Redis, MAY_BE_BOOL)
ZEND_END_ARG_INFO()

#define arginfo_class_Redis_popen arginfo_class_Redis_open

ZEND_BEGIN_ARG_INFO_EX(arginfo_class_Redis_psetex, 0, 0, 3)
	ZEND_ARG_TYPE_INFO(0, key, IS_STRING, 0)
	ZEND_ARG_TYPE_INFO(0, expire, IS_LONG, 0)
	ZEND_ARG_TYPE_INFO(0, value, IS_MIXED, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_Redis_psubscribe, 0, 1, IS_VOID, 0)
	ZEND_ARG_TYPE_INFO(0, patterns, IS_ARRAY, 0)
ZEND_END_ARG_INFO()

#define arginfo_class_Redis_pttl arginfo_class_Redis_hLen

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_Redis_publish, 0, 2, IS_LONG, 0)
	ZEND_ARG_TYPE_INFO(0, channel, IS_STRING, 0)
	ZEND_ARG_TYPE_INFO(0, message, IS_STRING, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_Redis_pubsub, 0, 1, IS_MIXED, 0)
	ZEND_ARG_TYPE_INFO(0, command, IS_STRING, 0)
	ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, arg, IS_MIXED, 0, "null")
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_Redis_punsubscribe, 0, 1, IS_ARRAY, 0)
	ZEND_ARG_TYPE_INFO(0, patterns, IS_ARRAY, 0)
ZEND_END_ARG_INFO()

#define arginfo_class_Redis_rPop arginfo_class_Redis_decr

#define arginfo_class_Redis_randomKey arginfo_class_Redis___construct

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_Redis_rawcommand, 0, 1, IS_MIXED, 0)
	ZEND_ARG_TYPE_INFO(0, command, IS_STRING, 0)
	ZEND_ARG_VARIADIC_TYPE_INFO(0, args, IS_MIXED, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_class_Redis_rename, 0, 0, 2)
	ZEND_ARG_TYPE_INFO(0, key_src, IS_STRING, 0)
	ZEND_ARG_TYPE_INFO(0, key_dst, IS_STRING, 0)
ZEND_END_ARG_INFO()

#define arginfo_class_Redis_renameNx arginfo_class_Redis_rename

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_Redis_restore, 0, 3, _IS_BOOL, 0)
	ZEND_ARG_TYPE_INFO(0, key, IS_STRING, 0)
	ZEND_ARG_TYPE_INFO(0, timeout, IS_LONG, 0)
	ZEND_ARG_TYPE_INFO(0, value, IS_STRING, 0)
ZEND_END_ARG_INFO()

#define arginfo_class_Redis_role arginfo_class_Redis_getAuth

ZEND_BEGIN_ARG_WITH_RETURN_TYPE_INFO_EX(arginfo_class_Redis_rpoplpush, 0, 2, IS_STRING, 0)
	ZEND_ARG_TYPE_INFO(0, src, IS_STRING, 0)
	ZEND_ARG_TYPE_INFO(0, dst, IS_STRING, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_class_Redis_set, 0, 0, 2)
	ZEND_ARG_TYPE_INFO(0, key, IS_STRING, 0)
	ZEND_ARG_TYPE_INFO(0, value, IS_MIXED, 0)
	ZEND_ARG_TYPE_INFO_WITH_DEFAULT_VALUE(0, opt, IS_MIXED, 0, "NULL")
ZEND_END_ARG_INFO()

#define arginfo_class_Redis_setex arginfo_class_Redis_psetex

#define arginfo_class_Redis_setnx arginfo_class_Redis_append

#define arginfo_class_Redis_unlink arginfo_class_Redis_del

#define arginfo_class_Redis_watch arginfo_class_Redis_del

#define arginfo_class_Redis_unwatch arginfo_class_Redis___construct

#define arginfo_class_Redis_type arginfo_class_Redis_decr

ZEND_BEGIN_ARG_INFO_EX(arginfo_class_Redis_setRange, 0, 0, 3)
	ZEND_ARG_TYPE_INFO(0, key, IS_STRING, 0)
	ZEND_ARG_TYPE_INFO(0, start, IS_LONG, 0)
	ZEND_ARG_TYPE_INFO(0, value, IS_STRING, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_class_Redis_setBit, 0, 0, 3)
	ZEND_ARG_TYPE_INFO(0, key, IS_STRING, 0)
	ZEND_ARG_TYPE_INFO(0, idx, IS_LONG, 0)
	ZEND_ARG_TYPE_INFO(0, value, _IS_BOOL, 0)
ZEND_END_ARG_INFO()

#define arginfo_class_Redis_strlen arginfo_class_Redis_decr


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
ZEND_METHOD(Redis, echo);
ZEND_METHOD(Redis, eval);
ZEND_METHOD(Redis, evalsha);
ZEND_METHOD(Redis, exec);
ZEND_METHOD(Redis, exists);
ZEND_METHOD(Redis, expire);
ZEND_METHOD(Redis, expireAt);
ZEND_METHOD(Redis, flushAll);
ZEND_METHOD(Redis, flushDB);
ZEND_METHOD(Redis, geoadd);
ZEND_METHOD(Redis, geodist);
ZEND_METHOD(Redis, geohash);
ZEND_METHOD(Redis, geopos);
ZEND_METHOD(Redis, georadius);
ZEND_METHOD(Redis, georadius_ro);
ZEND_METHOD(Redis, georadiusbymember);
ZEND_METHOD(Redis, georadiusbymember_ro);
ZEND_METHOD(Redis, get);
ZEND_METHOD(Redis, getAuth);
ZEND_METHOD(Redis, getBit);
ZEND_METHOD(Redis, getDBNum);
ZEND_METHOD(Redis, getHost);
ZEND_METHOD(Redis, getLastError);
ZEND_METHOD(Redis, getMode);
ZEND_METHOD(Redis, getOption);
ZEND_METHOD(Redis, getPersistentID);
ZEND_METHOD(Redis, getPort);
ZEND_METHOD(Redis, getRange);
ZEND_METHOD(Redis, getReadTimeout);
ZEND_METHOD(Redis, getset);
ZEND_METHOD(Redis, getTimeout);
ZEND_METHOD(Redis, hDel);
ZEND_METHOD(Redis, hExists);
ZEND_METHOD(Redis, hGet);
ZEND_METHOD(Redis, hGetAll);
ZEND_METHOD(Redis, hIncrBy);
ZEND_METHOD(Redis, hIncrByFloat);
ZEND_METHOD(Redis, hKeys);
ZEND_METHOD(Redis, hLen);
ZEND_METHOD(Redis, hMget);
ZEND_METHOD(Redis, hMset);
ZEND_METHOD(Redis, hSet);
ZEND_METHOD(Redis, hSetNx);
ZEND_METHOD(Redis, hStrLen);
ZEND_METHOD(Redis, hVals);
ZEND_METHOD(Redis, hscan);
ZEND_METHOD(Redis, incr);
ZEND_METHOD(Redis, incrBy);
ZEND_METHOD(Redis, incrByFloat);
ZEND_METHOD(Redis, info);
ZEND_METHOD(Redis, isConnected);
ZEND_METHOD(Redis, keys);
ZEND_METHOD(Redis, lInsert);
ZEND_METHOD(Redis, lLen);
ZEND_METHOD(Redis, lMove);
ZEND_METHOD(Redis, lPop);
ZEND_METHOD(Redis, lPush);
ZEND_METHOD(Redis, rPush);
ZEND_METHOD(Redis, lPushx);
ZEND_METHOD(Redis, rPushx);
ZEND_METHOD(Redis, lSet);
ZEND_METHOD(Redis, lastSave);
ZEND_METHOD(Redis, lindex);
ZEND_METHOD(Redis, lrange);
ZEND_METHOD(Redis, lrem);
ZEND_METHOD(Redis, ltrim);
ZEND_METHOD(Redis, mget);
ZEND_METHOD(Redis, migrate);
ZEND_METHOD(Redis, move);
ZEND_METHOD(Redis, mset);
ZEND_METHOD(Redis, msetnx);
ZEND_METHOD(Redis, multi);
ZEND_METHOD(Redis, object);
ZEND_METHOD(Redis, pconnect);
ZEND_METHOD(Redis, persist);
ZEND_METHOD(Redis, pexpire);
ZEND_METHOD(Redis, pexpireAt);
ZEND_METHOD(Redis, pfadd);
ZEND_METHOD(Redis, pfcount);
ZEND_METHOD(Redis, pfmerge);
ZEND_METHOD(Redis, ping);
ZEND_METHOD(Redis, pipeline);
ZEND_METHOD(Redis, psetex);
ZEND_METHOD(Redis, psubscribe);
ZEND_METHOD(Redis, pttl);
ZEND_METHOD(Redis, publish);
ZEND_METHOD(Redis, pubsub);
ZEND_METHOD(Redis, punsubscribe);
ZEND_METHOD(Redis, rPop);
ZEND_METHOD(Redis, randomKey);
ZEND_METHOD(Redis, rawcommand);
ZEND_METHOD(Redis, rename);
ZEND_METHOD(Redis, renameNx);
ZEND_METHOD(Redis, restore);
ZEND_METHOD(Redis, role);
ZEND_METHOD(Redis, rpoplpush);
ZEND_METHOD(Redis, set);
ZEND_METHOD(Redis, setex);
ZEND_METHOD(Redis, setnx);
ZEND_METHOD(Redis, unlink);
ZEND_METHOD(Redis, watch);
ZEND_METHOD(Redis, unwatch);
ZEND_METHOD(Redis, type);
ZEND_METHOD(Redis, setRange);
ZEND_METHOD(Redis, setBit);
ZEND_METHOD(Redis, strlen);


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
	ZEND_ME(Redis, echo, arginfo_class_Redis_echo, ZEND_ACC_PUBLIC)
	ZEND_ME(Redis, eval, arginfo_class_Redis_eval, ZEND_ACC_PUBLIC)
	ZEND_ME(Redis, evalsha, arginfo_class_Redis_evalsha, ZEND_ACC_PUBLIC)
	ZEND_ME(Redis, exec, arginfo_class_Redis_exec, ZEND_ACC_PUBLIC)
	ZEND_ME(Redis, exists, arginfo_class_Redis_exists, ZEND_ACC_PUBLIC)
	ZEND_ME(Redis, expire, arginfo_class_Redis_expire, ZEND_ACC_PUBLIC)
	ZEND_ME(Redis, expireAt, arginfo_class_Redis_expireAt, ZEND_ACC_PUBLIC)
	ZEND_ME(Redis, flushAll, arginfo_class_Redis_flushAll, ZEND_ACC_PUBLIC)
	ZEND_ME(Redis, flushDB, arginfo_class_Redis_flushDB, ZEND_ACC_PUBLIC)
	ZEND_ME(Redis, geoadd, arginfo_class_Redis_geoadd, ZEND_ACC_PUBLIC)
	ZEND_ME(Redis, geodist, arginfo_class_Redis_geodist, ZEND_ACC_PUBLIC)
	ZEND_ME(Redis, geohash, arginfo_class_Redis_geohash, ZEND_ACC_PUBLIC)
	ZEND_ME(Redis, geopos, arginfo_class_Redis_geopos, ZEND_ACC_PUBLIC)
	ZEND_ME(Redis, georadius, arginfo_class_Redis_georadius, ZEND_ACC_PUBLIC)
	ZEND_ME(Redis, georadius_ro, arginfo_class_Redis_georadius_ro, ZEND_ACC_PUBLIC)
	ZEND_ME(Redis, georadiusbymember, arginfo_class_Redis_georadiusbymember, ZEND_ACC_PUBLIC)
	ZEND_ME(Redis, georadiusbymember_ro, arginfo_class_Redis_georadiusbymember_ro, ZEND_ACC_PUBLIC)
	ZEND_ME(Redis, get, arginfo_class_Redis_get, ZEND_ACC_PUBLIC)
	ZEND_ME(Redis, getAuth, arginfo_class_Redis_getAuth, ZEND_ACC_PUBLIC)
	ZEND_ME(Redis, getBit, arginfo_class_Redis_getBit, ZEND_ACC_PUBLIC)
	ZEND_ME(Redis, getDBNum, arginfo_class_Redis_getDBNum, ZEND_ACC_PUBLIC)
	ZEND_ME(Redis, getHost, arginfo_class_Redis_getHost, ZEND_ACC_PUBLIC)
	ZEND_ME(Redis, getLastError, arginfo_class_Redis_getLastError, ZEND_ACC_PUBLIC)
	ZEND_ME(Redis, getMode, arginfo_class_Redis_getMode, ZEND_ACC_PUBLIC)
	ZEND_ME(Redis, getOption, arginfo_class_Redis_getOption, ZEND_ACC_PUBLIC)
	ZEND_ME(Redis, getPersistentID, arginfo_class_Redis_getPersistentID, ZEND_ACC_PUBLIC)
	ZEND_ME(Redis, getPort, arginfo_class_Redis_getPort, ZEND_ACC_PUBLIC)
	ZEND_ME(Redis, getRange, arginfo_class_Redis_getRange, ZEND_ACC_PUBLIC)
	ZEND_ME(Redis, getReadTimeout, arginfo_class_Redis_getReadTimeout, ZEND_ACC_PUBLIC)
	ZEND_ME(Redis, getset, arginfo_class_Redis_getset, ZEND_ACC_PUBLIC)
	ZEND_ME(Redis, getTimeout, arginfo_class_Redis_getTimeout, ZEND_ACC_PUBLIC)
	ZEND_ME(Redis, hDel, arginfo_class_Redis_hDel, ZEND_ACC_PUBLIC)
	ZEND_ME(Redis, hExists, arginfo_class_Redis_hExists, ZEND_ACC_PUBLIC)
	ZEND_ME(Redis, hGet, arginfo_class_Redis_hGet, ZEND_ACC_PUBLIC)
	ZEND_ME(Redis, hGetAll, arginfo_class_Redis_hGetAll, ZEND_ACC_PUBLIC)
	ZEND_ME(Redis, hIncrBy, arginfo_class_Redis_hIncrBy, ZEND_ACC_PUBLIC)
	ZEND_ME(Redis, hIncrByFloat, arginfo_class_Redis_hIncrByFloat, ZEND_ACC_PUBLIC)
	ZEND_ME(Redis, hKeys, arginfo_class_Redis_hKeys, ZEND_ACC_PUBLIC)
	ZEND_ME(Redis, hLen, arginfo_class_Redis_hLen, ZEND_ACC_PUBLIC)
	ZEND_ME(Redis, hMget, arginfo_class_Redis_hMget, ZEND_ACC_PUBLIC)
	ZEND_ME(Redis, hMset, arginfo_class_Redis_hMset, ZEND_ACC_PUBLIC)
	ZEND_ME(Redis, hSet, arginfo_class_Redis_hSet, ZEND_ACC_PUBLIC)
	ZEND_ME(Redis, hSetNx, arginfo_class_Redis_hSetNx, ZEND_ACC_PUBLIC)
	ZEND_ME(Redis, hStrLen, arginfo_class_Redis_hStrLen, ZEND_ACC_PUBLIC)
	ZEND_ME(Redis, hVals, arginfo_class_Redis_hVals, ZEND_ACC_PUBLIC)
	ZEND_ME(Redis, hscan, arginfo_class_Redis_hscan, ZEND_ACC_PUBLIC)
	ZEND_ME(Redis, incr, arginfo_class_Redis_incr, ZEND_ACC_PUBLIC)
	ZEND_ME(Redis, incrBy, arginfo_class_Redis_incrBy, ZEND_ACC_PUBLIC)
	ZEND_ME(Redis, incrByFloat, arginfo_class_Redis_incrByFloat, ZEND_ACC_PUBLIC)
	ZEND_ME(Redis, info, arginfo_class_Redis_info, ZEND_ACC_PUBLIC)
	ZEND_ME(Redis, isConnected, arginfo_class_Redis_isConnected, ZEND_ACC_PUBLIC)
	ZEND_ME(Redis, keys, arginfo_class_Redis_keys, ZEND_ACC_PUBLIC)
	ZEND_ME(Redis, lInsert, arginfo_class_Redis_lInsert, ZEND_ACC_PUBLIC)
	ZEND_ME(Redis, lLen, arginfo_class_Redis_lLen, ZEND_ACC_PUBLIC)
	ZEND_ME(Redis, lMove, arginfo_class_Redis_lMove, ZEND_ACC_PUBLIC)
	ZEND_ME(Redis, lPop, arginfo_class_Redis_lPop, ZEND_ACC_PUBLIC)
	ZEND_ME(Redis, lPush, arginfo_class_Redis_lPush, ZEND_ACC_PUBLIC)
	ZEND_ME(Redis, rPush, arginfo_class_Redis_rPush, ZEND_ACC_PUBLIC)
	ZEND_ME(Redis, lPushx, arginfo_class_Redis_lPushx, ZEND_ACC_PUBLIC)
	ZEND_ME(Redis, rPushx, arginfo_class_Redis_rPushx, ZEND_ACC_PUBLIC)
	ZEND_ME(Redis, lSet, arginfo_class_Redis_lSet, ZEND_ACC_PUBLIC)
	ZEND_ME(Redis, lastSave, arginfo_class_Redis_lastSave, ZEND_ACC_PUBLIC)
	ZEND_ME(Redis, lindex, arginfo_class_Redis_lindex, ZEND_ACC_PUBLIC)
	ZEND_ME(Redis, lrange, arginfo_class_Redis_lrange, ZEND_ACC_PUBLIC)
	ZEND_ME(Redis, lrem, arginfo_class_Redis_lrem, ZEND_ACC_PUBLIC)
	ZEND_ME(Redis, ltrim, arginfo_class_Redis_ltrim, ZEND_ACC_PUBLIC)
	ZEND_ME(Redis, mget, arginfo_class_Redis_mget, ZEND_ACC_PUBLIC)
	ZEND_ME(Redis, migrate, arginfo_class_Redis_migrate, ZEND_ACC_PUBLIC)
	ZEND_ME(Redis, move, arginfo_class_Redis_move, ZEND_ACC_PUBLIC)
	ZEND_ME(Redis, mset, arginfo_class_Redis_mset, ZEND_ACC_PUBLIC)
	ZEND_ME(Redis, msetnx, arginfo_class_Redis_msetnx, ZEND_ACC_PUBLIC)
	ZEND_ME(Redis, multi, arginfo_class_Redis_multi, ZEND_ACC_PUBLIC)
	ZEND_ME(Redis, object, arginfo_class_Redis_object, ZEND_ACC_PUBLIC)
	ZEND_MALIAS(Redis, open, connect, arginfo_class_Redis_open, ZEND_ACC_PUBLIC|ZEND_ACC_DEPRECATED)
	ZEND_ME(Redis, pconnect, arginfo_class_Redis_pconnect, ZEND_ACC_PUBLIC)
	ZEND_ME(Redis, persist, arginfo_class_Redis_persist, ZEND_ACC_PUBLIC)
	ZEND_ME(Redis, pexpire, arginfo_class_Redis_pexpire, ZEND_ACC_PUBLIC)
	ZEND_ME(Redis, pexpireAt, arginfo_class_Redis_pexpireAt, ZEND_ACC_PUBLIC)
	ZEND_ME(Redis, pfadd, arginfo_class_Redis_pfadd, ZEND_ACC_PUBLIC)
	ZEND_ME(Redis, pfcount, arginfo_class_Redis_pfcount, ZEND_ACC_PUBLIC)
	ZEND_ME(Redis, pfmerge, arginfo_class_Redis_pfmerge, ZEND_ACC_PUBLIC)
	ZEND_ME(Redis, ping, arginfo_class_Redis_ping, ZEND_ACC_PUBLIC)
	ZEND_ME(Redis, pipeline, arginfo_class_Redis_pipeline, ZEND_ACC_PUBLIC)
	ZEND_MALIAS(Redis, popen, pconnect, arginfo_class_Redis_popen, ZEND_ACC_PUBLIC|ZEND_ACC_DEPRECATED)
	ZEND_ME(Redis, psetex, arginfo_class_Redis_psetex, ZEND_ACC_PUBLIC)
	ZEND_ME(Redis, psubscribe, arginfo_class_Redis_psubscribe, ZEND_ACC_PUBLIC)
	ZEND_ME(Redis, pttl, arginfo_class_Redis_pttl, ZEND_ACC_PUBLIC)
	ZEND_ME(Redis, publish, arginfo_class_Redis_publish, ZEND_ACC_PUBLIC)
	ZEND_ME(Redis, pubsub, arginfo_class_Redis_pubsub, ZEND_ACC_PUBLIC)
	ZEND_ME(Redis, punsubscribe, arginfo_class_Redis_punsubscribe, ZEND_ACC_PUBLIC)
	ZEND_ME(Redis, rPop, arginfo_class_Redis_rPop, ZEND_ACC_PUBLIC)
	ZEND_ME(Redis, randomKey, arginfo_class_Redis_randomKey, ZEND_ACC_PUBLIC)
	ZEND_ME(Redis, rawcommand, arginfo_class_Redis_rawcommand, ZEND_ACC_PUBLIC)
	ZEND_ME(Redis, rename, arginfo_class_Redis_rename, ZEND_ACC_PUBLIC)
	ZEND_ME(Redis, renameNx, arginfo_class_Redis_renameNx, ZEND_ACC_PUBLIC)
	ZEND_ME(Redis, restore, arginfo_class_Redis_restore, ZEND_ACC_PUBLIC)
	ZEND_ME(Redis, role, arginfo_class_Redis_role, ZEND_ACC_PUBLIC)
	ZEND_ME(Redis, rpoplpush, arginfo_class_Redis_rpoplpush, ZEND_ACC_PUBLIC)
	ZEND_ME(Redis, set, arginfo_class_Redis_set, ZEND_ACC_PUBLIC)
	ZEND_ME(Redis, setex, arginfo_class_Redis_setex, ZEND_ACC_PUBLIC)
	ZEND_ME(Redis, setnx, arginfo_class_Redis_setnx, ZEND_ACC_PUBLIC)
	ZEND_ME(Redis, unlink, arginfo_class_Redis_unlink, ZEND_ACC_PUBLIC)
	ZEND_ME(Redis, watch, arginfo_class_Redis_watch, ZEND_ACC_PUBLIC)
	ZEND_ME(Redis, unwatch, arginfo_class_Redis_unwatch, ZEND_ACC_PUBLIC)
	ZEND_ME(Redis, type, arginfo_class_Redis_type, ZEND_ACC_PUBLIC)
	ZEND_ME(Redis, setRange, arginfo_class_Redis_setRange, ZEND_ACC_PUBLIC)
	ZEND_ME(Redis, setBit, arginfo_class_Redis_setBit, ZEND_ACC_PUBLIC)
	ZEND_ME(Redis, strlen, arginfo_class_Redis_strlen, ZEND_ACC_PUBLIC)
	ZEND_FE_END
};
