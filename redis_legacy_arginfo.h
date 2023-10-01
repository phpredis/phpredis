/* This is a generated file, edit the .stub.php file instead.
 * Stub hash: f98761a9bf8bfd22f34609b4d7c0c26f69248668 */

ZEND_BEGIN_ARG_INFO_EX(arginfo_class_Redis___construct, 0, 0, 0)
	ZEND_ARG_INFO(0, options)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_class_Redis___destruct, 0, 0, 0)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_class_Redis__compress, 0, 0, 1)
	ZEND_ARG_INFO(0, value)
ZEND_END_ARG_INFO()

#define arginfo_class_Redis__uncompress arginfo_class_Redis__compress

ZEND_BEGIN_ARG_INFO_EX(arginfo_class_Redis__prefix, 0, 0, 1)
	ZEND_ARG_INFO(0, key)
ZEND_END_ARG_INFO()

#define arginfo_class_Redis__serialize arginfo_class_Redis__compress

#define arginfo_class_Redis__unserialize arginfo_class_Redis__compress

#define arginfo_class_Redis__pack arginfo_class_Redis__compress

#define arginfo_class_Redis__unpack arginfo_class_Redis__compress

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

#define arginfo_class_Redis_bgSave arginfo_class_Redis___destruct

#define arginfo_class_Redis_bgrewriteaof arginfo_class_Redis___destruct

ZEND_BEGIN_ARG_INFO_EX(arginfo_class_Redis_bitcount, 0, 0, 1)
	ZEND_ARG_INFO(0, key)
	ZEND_ARG_INFO(0, start)
	ZEND_ARG_INFO(0, end)
	ZEND_ARG_INFO(0, bybit)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_class_Redis_bitop, 0, 0, 3)
	ZEND_ARG_INFO(0, operation)
	ZEND_ARG_INFO(0, deskey)
	ZEND_ARG_INFO(0, srckey)
	ZEND_ARG_VARIADIC_INFO(0, other_keys)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_class_Redis_bitpos, 0, 0, 2)
	ZEND_ARG_INFO(0, key)
	ZEND_ARG_INFO(0, bit)
	ZEND_ARG_INFO(0, start)
	ZEND_ARG_INFO(0, end)
	ZEND_ARG_INFO(0, bybit)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_class_Redis_blPop, 0, 0, 2)
	ZEND_ARG_INFO(0, key_or_keys)
	ZEND_ARG_INFO(0, timeout_or_key)
	ZEND_ARG_VARIADIC_INFO(0, extra_args)
ZEND_END_ARG_INFO()

#define arginfo_class_Redis_brPop arginfo_class_Redis_blPop

ZEND_BEGIN_ARG_INFO_EX(arginfo_class_Redis_brpoplpush, 0, 0, 3)
	ZEND_ARG_INFO(0, src)
	ZEND_ARG_INFO(0, dst)
	ZEND_ARG_INFO(0, timeout)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_class_Redis_bzPopMax, 0, 0, 2)
	ZEND_ARG_INFO(0, key)
	ZEND_ARG_INFO(0, timeout_or_key)
	ZEND_ARG_VARIADIC_INFO(0, extra_args)
ZEND_END_ARG_INFO()

#define arginfo_class_Redis_bzPopMin arginfo_class_Redis_bzPopMax

ZEND_BEGIN_ARG_INFO_EX(arginfo_class_Redis_bzmpop, 0, 0, 3)
	ZEND_ARG_INFO(0, timeout)
	ZEND_ARG_INFO(0, keys)
	ZEND_ARG_INFO(0, from)
	ZEND_ARG_INFO(0, count)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_class_Redis_zmpop, 0, 0, 2)
	ZEND_ARG_INFO(0, keys)
	ZEND_ARG_INFO(0, from)
	ZEND_ARG_INFO(0, count)
ZEND_END_ARG_INFO()

#define arginfo_class_Redis_blmpop arginfo_class_Redis_bzmpop

#define arginfo_class_Redis_lmpop arginfo_class_Redis_zmpop

#define arginfo_class_Redis_clearLastError arginfo_class_Redis___destruct

ZEND_BEGIN_ARG_INFO_EX(arginfo_class_Redis_client, 0, 0, 1)
	ZEND_ARG_INFO(0, opt)
	ZEND_ARG_VARIADIC_INFO(0, args)
ZEND_END_ARG_INFO()

#define arginfo_class_Redis_close arginfo_class_Redis___destruct

ZEND_BEGIN_ARG_INFO_EX(arginfo_class_Redis_command, 0, 0, 0)
	ZEND_ARG_INFO(0, opt)
	ZEND_ARG_VARIADIC_INFO(0, args)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_class_Redis_config, 0, 0, 1)
	ZEND_ARG_INFO(0, operation)
	ZEND_ARG_INFO(0, key_or_settings)
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

#define arginfo_class_Redis_dbSize arginfo_class_Redis___destruct

#define arginfo_class_Redis_debug arginfo_class_Redis__prefix

ZEND_BEGIN_ARG_INFO_EX(arginfo_class_Redis_decr, 0, 0, 1)
	ZEND_ARG_INFO(0, key)
	ZEND_ARG_INFO(0, by)
ZEND_END_ARG_INFO()

#define arginfo_class_Redis_decrBy arginfo_class_Redis_append

ZEND_BEGIN_ARG_INFO_EX(arginfo_class_Redis_del, 0, 0, 1)
	ZEND_ARG_INFO(0, key)
	ZEND_ARG_VARIADIC_INFO(0, other_keys)
ZEND_END_ARG_INFO()

#define arginfo_class_Redis_delete arginfo_class_Redis_del

#define arginfo_class_Redis_discard arginfo_class_Redis___destruct

#define arginfo_class_Redis_dump arginfo_class_Redis__prefix

ZEND_BEGIN_ARG_INFO_EX(arginfo_class_Redis_echo, 0, 0, 1)
	ZEND_ARG_INFO(0, str)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_class_Redis_eval, 0, 0, 1)
	ZEND_ARG_INFO(0, script)
	ZEND_ARG_INFO(0, args)
	ZEND_ARG_INFO(0, num_keys)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_class_Redis_eval_ro, 0, 0, 1)
	ZEND_ARG_INFO(0, script_sha)
	ZEND_ARG_INFO(0, args)
	ZEND_ARG_INFO(0, num_keys)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_class_Redis_evalsha, 0, 0, 1)
	ZEND_ARG_INFO(0, sha1)
	ZEND_ARG_INFO(0, args)
	ZEND_ARG_INFO(0, num_keys)
ZEND_END_ARG_INFO()

#define arginfo_class_Redis_evalsha_ro arginfo_class_Redis_evalsha

#define arginfo_class_Redis_exec arginfo_class_Redis___destruct

#define arginfo_class_Redis_exists arginfo_class_Redis_del

ZEND_BEGIN_ARG_INFO_EX(arginfo_class_Redis_expire, 0, 0, 2)
	ZEND_ARG_INFO(0, key)
	ZEND_ARG_INFO(0, timeout)
	ZEND_ARG_INFO(0, mode)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_class_Redis_expireAt, 0, 0, 2)
	ZEND_ARG_INFO(0, key)
	ZEND_ARG_INFO(0, timestamp)
	ZEND_ARG_INFO(0, mode)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_class_Redis_failover, 0, 0, 0)
	ZEND_ARG_INFO(0, to)
	ZEND_ARG_INFO(0, abort)
	ZEND_ARG_INFO(0, timeout)
ZEND_END_ARG_INFO()

#define arginfo_class_Redis_expiretime arginfo_class_Redis__prefix

#define arginfo_class_Redis_pexpiretime arginfo_class_Redis__prefix

ZEND_BEGIN_ARG_INFO_EX(arginfo_class_Redis_fcall, 0, 0, 1)
	ZEND_ARG_INFO(0, fn)
	ZEND_ARG_INFO(0, keys)
	ZEND_ARG_INFO(0, args)
ZEND_END_ARG_INFO()

#define arginfo_class_Redis_fcall_ro arginfo_class_Redis_fcall

ZEND_BEGIN_ARG_INFO_EX(arginfo_class_Redis_flushAll, 0, 0, 0)
	ZEND_ARG_INFO(0, sync)
ZEND_END_ARG_INFO()

#define arginfo_class_Redis_flushDB arginfo_class_Redis_flushAll

ZEND_BEGIN_ARG_INFO_EX(arginfo_class_Redis_function, 0, 0, 1)
	ZEND_ARG_INFO(0, operation)
	ZEND_ARG_VARIADIC_INFO(0, args)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_class_Redis_geoadd, 0, 0, 4)
	ZEND_ARG_INFO(0, key)
	ZEND_ARG_INFO(0, lng)
	ZEND_ARG_INFO(0, lat)
	ZEND_ARG_INFO(0, member)
	ZEND_ARG_VARIADIC_INFO(0, other_triples_and_options)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_class_Redis_geodist, 0, 0, 3)
	ZEND_ARG_INFO(0, key)
	ZEND_ARG_INFO(0, src)
	ZEND_ARG_INFO(0, dst)
	ZEND_ARG_INFO(0, unit)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_class_Redis_geohash, 0, 0, 2)
	ZEND_ARG_INFO(0, key)
	ZEND_ARG_INFO(0, member)
	ZEND_ARG_VARIADIC_INFO(0, other_members)
ZEND_END_ARG_INFO()

#define arginfo_class_Redis_geopos arginfo_class_Redis_geohash

ZEND_BEGIN_ARG_INFO_EX(arginfo_class_Redis_georadius, 0, 0, 5)
	ZEND_ARG_INFO(0, key)
	ZEND_ARG_INFO(0, lng)
	ZEND_ARG_INFO(0, lat)
	ZEND_ARG_INFO(0, radius)
	ZEND_ARG_INFO(0, unit)
	ZEND_ARG_INFO(0, options)
ZEND_END_ARG_INFO()

#define arginfo_class_Redis_georadius_ro arginfo_class_Redis_georadius

ZEND_BEGIN_ARG_INFO_EX(arginfo_class_Redis_georadiusbymember, 0, 0, 4)
	ZEND_ARG_INFO(0, key)
	ZEND_ARG_INFO(0, member)
	ZEND_ARG_INFO(0, radius)
	ZEND_ARG_INFO(0, unit)
	ZEND_ARG_INFO(0, options)
ZEND_END_ARG_INFO()

#define arginfo_class_Redis_georadiusbymember_ro arginfo_class_Redis_georadiusbymember

ZEND_BEGIN_ARG_INFO_EX(arginfo_class_Redis_geosearch, 0, 0, 4)
	ZEND_ARG_INFO(0, key)
	ZEND_ARG_INFO(0, position)
	ZEND_ARG_INFO(0, shape)
	ZEND_ARG_INFO(0, unit)
	ZEND_ARG_INFO(0, options)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_class_Redis_geosearchstore, 0, 0, 5)
	ZEND_ARG_INFO(0, dst)
	ZEND_ARG_INFO(0, src)
	ZEND_ARG_INFO(0, position)
	ZEND_ARG_INFO(0, shape)
	ZEND_ARG_INFO(0, unit)
	ZEND_ARG_INFO(0, options)
ZEND_END_ARG_INFO()

#define arginfo_class_Redis_get arginfo_class_Redis__prefix

#define arginfo_class_Redis_getAuth arginfo_class_Redis___destruct

ZEND_BEGIN_ARG_INFO_EX(arginfo_class_Redis_getBit, 0, 0, 2)
	ZEND_ARG_INFO(0, key)
	ZEND_ARG_INFO(0, idx)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_class_Redis_getEx, 0, 0, 1)
	ZEND_ARG_INFO(0, key)
	ZEND_ARG_INFO(0, options)
ZEND_END_ARG_INFO()

#define arginfo_class_Redis_getDBNum arginfo_class_Redis___destruct

#define arginfo_class_Redis_getDel arginfo_class_Redis__prefix

#define arginfo_class_Redis_getHost arginfo_class_Redis___destruct

#define arginfo_class_Redis_getLastError arginfo_class_Redis___destruct

#define arginfo_class_Redis_getMode arginfo_class_Redis___destruct

ZEND_BEGIN_ARG_INFO_EX(arginfo_class_Redis_getOption, 0, 0, 1)
	ZEND_ARG_INFO(0, option)
ZEND_END_ARG_INFO()

#define arginfo_class_Redis_getPersistentID arginfo_class_Redis___destruct

#define arginfo_class_Redis_getPort arginfo_class_Redis___destruct

ZEND_BEGIN_ARG_INFO_EX(arginfo_class_Redis_getRange, 0, 0, 3)
	ZEND_ARG_INFO(0, key)
	ZEND_ARG_INFO(0, start)
	ZEND_ARG_INFO(0, end)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_class_Redis_lcs, 0, 0, 2)
	ZEND_ARG_INFO(0, key1)
	ZEND_ARG_INFO(0, key2)
	ZEND_ARG_INFO(0, options)
ZEND_END_ARG_INFO()

#define arginfo_class_Redis_getReadTimeout arginfo_class_Redis___destruct

#define arginfo_class_Redis_getset arginfo_class_Redis_append

#define arginfo_class_Redis_getTimeout arginfo_class_Redis___destruct

#define arginfo_class_Redis_getTransferredBytes arginfo_class_Redis___destruct

#define arginfo_class_Redis_clearTransferredBytes arginfo_class_Redis___destruct

ZEND_BEGIN_ARG_INFO_EX(arginfo_class_Redis_hDel, 0, 0, 2)
	ZEND_ARG_INFO(0, key)
	ZEND_ARG_INFO(0, field)
	ZEND_ARG_VARIADIC_INFO(0, other_fields)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_class_Redis_hExists, 0, 0, 2)
	ZEND_ARG_INFO(0, key)
	ZEND_ARG_INFO(0, field)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_class_Redis_hGet, 0, 0, 2)
	ZEND_ARG_INFO(0, key)
	ZEND_ARG_INFO(0, member)
ZEND_END_ARG_INFO()

#define arginfo_class_Redis_hGetAll arginfo_class_Redis__prefix

ZEND_BEGIN_ARG_INFO_EX(arginfo_class_Redis_hIncrBy, 0, 0, 3)
	ZEND_ARG_INFO(0, key)
	ZEND_ARG_INFO(0, field)
	ZEND_ARG_INFO(0, value)
ZEND_END_ARG_INFO()

#define arginfo_class_Redis_hIncrByFloat arginfo_class_Redis_hIncrBy

#define arginfo_class_Redis_hKeys arginfo_class_Redis__prefix

#define arginfo_class_Redis_hLen arginfo_class_Redis__prefix

ZEND_BEGIN_ARG_INFO_EX(arginfo_class_Redis_hMget, 0, 0, 2)
	ZEND_ARG_INFO(0, key)
	ZEND_ARG_INFO(0, fields)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_class_Redis_hMset, 0, 0, 2)
	ZEND_ARG_INFO(0, key)
	ZEND_ARG_INFO(0, fieldvals)
ZEND_END_ARG_INFO()

#define arginfo_class_Redis_hRandField arginfo_class_Redis_getEx

ZEND_BEGIN_ARG_INFO_EX(arginfo_class_Redis_hSet, 0, 0, 3)
	ZEND_ARG_INFO(0, key)
	ZEND_ARG_INFO(0, member)
	ZEND_ARG_INFO(0, value)
ZEND_END_ARG_INFO()

#define arginfo_class_Redis_hSetNx arginfo_class_Redis_hIncrBy

#define arginfo_class_Redis_hStrLen arginfo_class_Redis_hExists

#define arginfo_class_Redis_hVals arginfo_class_Redis__prefix

ZEND_BEGIN_ARG_INFO_EX(arginfo_class_Redis_hscan, 0, 0, 2)
	ZEND_ARG_INFO(0, key)
	ZEND_ARG_INFO(1, iterator)
	ZEND_ARG_INFO(0, pattern)
	ZEND_ARG_INFO(0, count)
ZEND_END_ARG_INFO()

#define arginfo_class_Redis_incr arginfo_class_Redis_decr

#define arginfo_class_Redis_incrBy arginfo_class_Redis_append

#define arginfo_class_Redis_incrByFloat arginfo_class_Redis_append

ZEND_BEGIN_ARG_INFO_EX(arginfo_class_Redis_info, 0, 0, 0)
	ZEND_ARG_VARIADIC_INFO(0, sections)
ZEND_END_ARG_INFO()

#define arginfo_class_Redis_isConnected arginfo_class_Redis___destruct

ZEND_BEGIN_ARG_INFO_EX(arginfo_class_Redis_keys, 0, 0, 1)
	ZEND_ARG_INFO(0, pattern)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_class_Redis_lInsert, 0, 0, 4)
	ZEND_ARG_INFO(0, key)
	ZEND_ARG_INFO(0, pos)
	ZEND_ARG_INFO(0, pivot)
	ZEND_ARG_INFO(0, value)
ZEND_END_ARG_INFO()

#define arginfo_class_Redis_lLen arginfo_class_Redis__prefix

ZEND_BEGIN_ARG_INFO_EX(arginfo_class_Redis_lMove, 0, 0, 4)
	ZEND_ARG_INFO(0, src)
	ZEND_ARG_INFO(0, dst)
	ZEND_ARG_INFO(0, wherefrom)
	ZEND_ARG_INFO(0, whereto)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_class_Redis_blmove, 0, 0, 5)
	ZEND_ARG_INFO(0, src)
	ZEND_ARG_INFO(0, dst)
	ZEND_ARG_INFO(0, wherefrom)
	ZEND_ARG_INFO(0, whereto)
	ZEND_ARG_INFO(0, timeout)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_class_Redis_lPop, 0, 0, 1)
	ZEND_ARG_INFO(0, key)
	ZEND_ARG_INFO(0, count)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_class_Redis_lPos, 0, 0, 2)
	ZEND_ARG_INFO(0, key)
	ZEND_ARG_INFO(0, value)
	ZEND_ARG_INFO(0, options)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_class_Redis_lPush, 0, 0, 1)
	ZEND_ARG_INFO(0, key)
	ZEND_ARG_VARIADIC_INFO(0, elements)
ZEND_END_ARG_INFO()

#define arginfo_class_Redis_rPush arginfo_class_Redis_lPush

#define arginfo_class_Redis_lPushx arginfo_class_Redis_append

#define arginfo_class_Redis_rPushx arginfo_class_Redis_append

ZEND_BEGIN_ARG_INFO_EX(arginfo_class_Redis_lSet, 0, 0, 3)
	ZEND_ARG_INFO(0, key)
	ZEND_ARG_INFO(0, index)
	ZEND_ARG_INFO(0, value)
ZEND_END_ARG_INFO()

#define arginfo_class_Redis_lastSave arginfo_class_Redis___destruct

ZEND_BEGIN_ARG_INFO_EX(arginfo_class_Redis_lindex, 0, 0, 2)
	ZEND_ARG_INFO(0, key)
	ZEND_ARG_INFO(0, index)
ZEND_END_ARG_INFO()

#define arginfo_class_Redis_lrange arginfo_class_Redis_getRange

ZEND_BEGIN_ARG_INFO_EX(arginfo_class_Redis_lrem, 0, 0, 2)
	ZEND_ARG_INFO(0, key)
	ZEND_ARG_INFO(0, value)
	ZEND_ARG_INFO(0, count)
ZEND_END_ARG_INFO()

#define arginfo_class_Redis_ltrim arginfo_class_Redis_getRange

ZEND_BEGIN_ARG_INFO_EX(arginfo_class_Redis_mget, 0, 0, 1)
	ZEND_ARG_INFO(0, keys)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_class_Redis_migrate, 0, 0, 5)
	ZEND_ARG_INFO(0, host)
	ZEND_ARG_INFO(0, port)
	ZEND_ARG_INFO(0, key)
	ZEND_ARG_INFO(0, dstdb)
	ZEND_ARG_INFO(0, timeout)
	ZEND_ARG_INFO(0, copy)
	ZEND_ARG_INFO(0, replace)
	ZEND_ARG_INFO(0, credentials)
ZEND_END_ARG_INFO()

#define arginfo_class_Redis_move arginfo_class_Redis_lindex

ZEND_BEGIN_ARG_INFO_EX(arginfo_class_Redis_mset, 0, 0, 1)
	ZEND_ARG_INFO(0, key_values)
ZEND_END_ARG_INFO()

#define arginfo_class_Redis_msetnx arginfo_class_Redis_mset

ZEND_BEGIN_ARG_INFO_EX(arginfo_class_Redis_multi, 0, 0, 0)
	ZEND_ARG_INFO(0, value)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_class_Redis_object, 0, 0, 2)
	ZEND_ARG_INFO(0, subcommand)
	ZEND_ARG_INFO(0, key)
ZEND_END_ARG_INFO()

#define arginfo_class_Redis_open arginfo_class_Redis_connect

#define arginfo_class_Redis_pconnect arginfo_class_Redis_connect

#define arginfo_class_Redis_persist arginfo_class_Redis__prefix

#define arginfo_class_Redis_pexpire arginfo_class_Redis_expire

#define arginfo_class_Redis_pexpireAt arginfo_class_Redis_expireAt

ZEND_BEGIN_ARG_INFO_EX(arginfo_class_Redis_pfadd, 0, 0, 2)
	ZEND_ARG_INFO(0, key)
	ZEND_ARG_INFO(0, elements)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_class_Redis_pfcount, 0, 0, 1)
	ZEND_ARG_INFO(0, key_or_keys)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_class_Redis_pfmerge, 0, 0, 2)
	ZEND_ARG_INFO(0, dst)
	ZEND_ARG_INFO(0, srckeys)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_class_Redis_ping, 0, 0, 0)
	ZEND_ARG_INFO(0, message)
ZEND_END_ARG_INFO()

#define arginfo_class_Redis_pipeline arginfo_class_Redis___destruct

#define arginfo_class_Redis_popen arginfo_class_Redis_connect

ZEND_BEGIN_ARG_INFO_EX(arginfo_class_Redis_psetex, 0, 0, 3)
	ZEND_ARG_INFO(0, key)
	ZEND_ARG_INFO(0, expire)
	ZEND_ARG_INFO(0, value)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_class_Redis_psubscribe, 0, 0, 2)
	ZEND_ARG_INFO(0, patterns)
	ZEND_ARG_INFO(0, cb)
ZEND_END_ARG_INFO()

#define arginfo_class_Redis_pttl arginfo_class_Redis__prefix

ZEND_BEGIN_ARG_INFO_EX(arginfo_class_Redis_publish, 0, 0, 2)
	ZEND_ARG_INFO(0, channel)
	ZEND_ARG_INFO(0, message)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_class_Redis_pubsub, 0, 0, 1)
	ZEND_ARG_INFO(0, command)
	ZEND_ARG_INFO(0, arg)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_class_Redis_punsubscribe, 0, 0, 1)
	ZEND_ARG_INFO(0, patterns)
ZEND_END_ARG_INFO()

#define arginfo_class_Redis_rPop arginfo_class_Redis_lPop

#define arginfo_class_Redis_randomKey arginfo_class_Redis___destruct

ZEND_BEGIN_ARG_INFO_EX(arginfo_class_Redis_rawcommand, 0, 0, 1)
	ZEND_ARG_INFO(0, command)
	ZEND_ARG_VARIADIC_INFO(0, args)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_class_Redis_rename, 0, 0, 2)
	ZEND_ARG_INFO(0, old_name)
	ZEND_ARG_INFO(0, new_name)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_class_Redis_renameNx, 0, 0, 2)
	ZEND_ARG_INFO(0, key_src)
	ZEND_ARG_INFO(0, key_dst)
ZEND_END_ARG_INFO()

#define arginfo_class_Redis_reset arginfo_class_Redis___destruct

ZEND_BEGIN_ARG_INFO_EX(arginfo_class_Redis_restore, 0, 0, 3)
	ZEND_ARG_INFO(0, key)
	ZEND_ARG_INFO(0, ttl)
	ZEND_ARG_INFO(0, value)
	ZEND_ARG_INFO(0, options)
ZEND_END_ARG_INFO()

#define arginfo_class_Redis_role arginfo_class_Redis___destruct

ZEND_BEGIN_ARG_INFO_EX(arginfo_class_Redis_rpoplpush, 0, 0, 2)
	ZEND_ARG_INFO(0, srckey)
	ZEND_ARG_INFO(0, dstkey)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_class_Redis_sAdd, 0, 0, 2)
	ZEND_ARG_INFO(0, key)
	ZEND_ARG_INFO(0, value)
	ZEND_ARG_VARIADIC_INFO(0, other_values)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_class_Redis_sAddArray, 0, 0, 2)
	ZEND_ARG_INFO(0, key)
	ZEND_ARG_INFO(0, values)
ZEND_END_ARG_INFO()

#define arginfo_class_Redis_sDiff arginfo_class_Redis_del

ZEND_BEGIN_ARG_INFO_EX(arginfo_class_Redis_sDiffStore, 0, 0, 2)
	ZEND_ARG_INFO(0, dst)
	ZEND_ARG_INFO(0, key)
	ZEND_ARG_VARIADIC_INFO(0, other_keys)
ZEND_END_ARG_INFO()

#define arginfo_class_Redis_sInter arginfo_class_Redis_del

ZEND_BEGIN_ARG_INFO_EX(arginfo_class_Redis_sintercard, 0, 0, 1)
	ZEND_ARG_INFO(0, keys)
	ZEND_ARG_INFO(0, limit)
ZEND_END_ARG_INFO()

#define arginfo_class_Redis_sInterStore arginfo_class_Redis_del

#define arginfo_class_Redis_sMembers arginfo_class_Redis__prefix

#define arginfo_class_Redis_sMisMember arginfo_class_Redis_geohash

ZEND_BEGIN_ARG_INFO_EX(arginfo_class_Redis_sMove, 0, 0, 3)
	ZEND_ARG_INFO(0, src)
	ZEND_ARG_INFO(0, dst)
	ZEND_ARG_INFO(0, value)
ZEND_END_ARG_INFO()

#define arginfo_class_Redis_sPop arginfo_class_Redis_lPop

#define arginfo_class_Redis_sRandMember arginfo_class_Redis_lPop

#define arginfo_class_Redis_sUnion arginfo_class_Redis_del

#define arginfo_class_Redis_sUnionStore arginfo_class_Redis_sDiffStore

#define arginfo_class_Redis_save arginfo_class_Redis___destruct

ZEND_BEGIN_ARG_INFO_EX(arginfo_class_Redis_scan, 0, 0, 1)
	ZEND_ARG_INFO(1, iterator)
	ZEND_ARG_INFO(0, pattern)
	ZEND_ARG_INFO(0, count)
	ZEND_ARG_INFO(0, type)
ZEND_END_ARG_INFO()

#define arginfo_class_Redis_scard arginfo_class_Redis__prefix

#define arginfo_class_Redis_script arginfo_class_Redis_rawcommand

ZEND_BEGIN_ARG_INFO_EX(arginfo_class_Redis_select, 0, 0, 1)
	ZEND_ARG_INFO(0, db)
ZEND_END_ARG_INFO()

#define arginfo_class_Redis_set arginfo_class_Redis_lPos

ZEND_BEGIN_ARG_INFO_EX(arginfo_class_Redis_setBit, 0, 0, 3)
	ZEND_ARG_INFO(0, key)
	ZEND_ARG_INFO(0, idx)
	ZEND_ARG_INFO(0, value)
ZEND_END_ARG_INFO()

#define arginfo_class_Redis_setRange arginfo_class_Redis_lSet

ZEND_BEGIN_ARG_INFO_EX(arginfo_class_Redis_setOption, 0, 0, 2)
	ZEND_ARG_INFO(0, option)
	ZEND_ARG_INFO(0, value)
ZEND_END_ARG_INFO()

#define arginfo_class_Redis_setex arginfo_class_Redis_psetex

#define arginfo_class_Redis_setnx arginfo_class_Redis_append

#define arginfo_class_Redis_sismember arginfo_class_Redis_append

ZEND_BEGIN_ARG_INFO_EX(arginfo_class_Redis_slaveof, 0, 0, 0)
	ZEND_ARG_INFO(0, host)
	ZEND_ARG_INFO(0, port)
ZEND_END_ARG_INFO()

#define arginfo_class_Redis_replicaof arginfo_class_Redis_slaveof

ZEND_BEGIN_ARG_INFO_EX(arginfo_class_Redis_touch, 0, 0, 1)
	ZEND_ARG_INFO(0, key_or_array)
	ZEND_ARG_VARIADIC_INFO(0, more_keys)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_class_Redis_slowlog, 0, 0, 1)
	ZEND_ARG_INFO(0, operation)
	ZEND_ARG_INFO(0, length)
ZEND_END_ARG_INFO()

#define arginfo_class_Redis_sort arginfo_class_Redis_getEx

#define arginfo_class_Redis_sort_ro arginfo_class_Redis_getEx

ZEND_BEGIN_ARG_INFO_EX(arginfo_class_Redis_sortAsc, 0, 0, 1)
	ZEND_ARG_INFO(0, key)
	ZEND_ARG_INFO(0, pattern)
	ZEND_ARG_INFO(0, get)
	ZEND_ARG_INFO(0, offset)
	ZEND_ARG_INFO(0, count)
	ZEND_ARG_INFO(0, store)
ZEND_END_ARG_INFO()

#define arginfo_class_Redis_sortAscAlpha arginfo_class_Redis_sortAsc

#define arginfo_class_Redis_sortDesc arginfo_class_Redis_sortAsc

#define arginfo_class_Redis_sortDescAlpha arginfo_class_Redis_sortAsc

#define arginfo_class_Redis_srem arginfo_class_Redis_sAdd

#define arginfo_class_Redis_sscan arginfo_class_Redis_hscan

ZEND_BEGIN_ARG_INFO_EX(arginfo_class_Redis_ssubscribe, 0, 0, 2)
	ZEND_ARG_INFO(0, channels)
	ZEND_ARG_INFO(0, cb)
ZEND_END_ARG_INFO()

#define arginfo_class_Redis_strlen arginfo_class_Redis__prefix

#define arginfo_class_Redis_subscribe arginfo_class_Redis_ssubscribe

ZEND_BEGIN_ARG_INFO_EX(arginfo_class_Redis_sunsubscribe, 0, 0, 1)
	ZEND_ARG_INFO(0, channels)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_class_Redis_swapdb, 0, 0, 2)
	ZEND_ARG_INFO(0, src)
	ZEND_ARG_INFO(0, dst)
ZEND_END_ARG_INFO()

#define arginfo_class_Redis_time arginfo_class_Redis___destruct

#define arginfo_class_Redis_ttl arginfo_class_Redis__prefix

#define arginfo_class_Redis_type arginfo_class_Redis__prefix

#define arginfo_class_Redis_unlink arginfo_class_Redis_del

#define arginfo_class_Redis_unsubscribe arginfo_class_Redis_sunsubscribe

#define arginfo_class_Redis_unwatch arginfo_class_Redis___destruct

#define arginfo_class_Redis_watch arginfo_class_Redis_del

ZEND_BEGIN_ARG_INFO_EX(arginfo_class_Redis_wait, 0, 0, 2)
	ZEND_ARG_INFO(0, numreplicas)
	ZEND_ARG_INFO(0, timeout)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_class_Redis_xack, 0, 0, 3)
	ZEND_ARG_INFO(0, key)
	ZEND_ARG_INFO(0, group)
	ZEND_ARG_INFO(0, ids)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_class_Redis_xadd, 0, 0, 3)
	ZEND_ARG_INFO(0, key)
	ZEND_ARG_INFO(0, id)
	ZEND_ARG_INFO(0, values)
	ZEND_ARG_INFO(0, maxlen)
	ZEND_ARG_INFO(0, approx)
	ZEND_ARG_INFO(0, nomkstream)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_class_Redis_xautoclaim, 0, 0, 5)
	ZEND_ARG_INFO(0, key)
	ZEND_ARG_INFO(0, group)
	ZEND_ARG_INFO(0, consumer)
	ZEND_ARG_INFO(0, min_idle)
	ZEND_ARG_INFO(0, start)
	ZEND_ARG_INFO(0, count)
	ZEND_ARG_INFO(0, justid)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_class_Redis_xclaim, 0, 0, 6)
	ZEND_ARG_INFO(0, key)
	ZEND_ARG_INFO(0, group)
	ZEND_ARG_INFO(0, consumer)
	ZEND_ARG_INFO(0, min_idle)
	ZEND_ARG_INFO(0, ids)
	ZEND_ARG_INFO(0, options)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_class_Redis_xdel, 0, 0, 2)
	ZEND_ARG_INFO(0, key)
	ZEND_ARG_INFO(0, ids)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_class_Redis_xgroup, 0, 0, 1)
	ZEND_ARG_INFO(0, operation)
	ZEND_ARG_INFO(0, key)
	ZEND_ARG_INFO(0, group)
	ZEND_ARG_INFO(0, id_or_consumer)
	ZEND_ARG_INFO(0, mkstream)
	ZEND_ARG_INFO(0, entries_read)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_class_Redis_xinfo, 0, 0, 1)
	ZEND_ARG_INFO(0, operation)
	ZEND_ARG_INFO(0, arg1)
	ZEND_ARG_INFO(0, arg2)
	ZEND_ARG_INFO(0, count)
ZEND_END_ARG_INFO()

#define arginfo_class_Redis_xlen arginfo_class_Redis__prefix

ZEND_BEGIN_ARG_INFO_EX(arginfo_class_Redis_xpending, 0, 0, 2)
	ZEND_ARG_INFO(0, key)
	ZEND_ARG_INFO(0, group)
	ZEND_ARG_INFO(0, start)
	ZEND_ARG_INFO(0, end)
	ZEND_ARG_INFO(0, count)
	ZEND_ARG_INFO(0, consumer)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_class_Redis_xrange, 0, 0, 3)
	ZEND_ARG_INFO(0, key)
	ZEND_ARG_INFO(0, start)
	ZEND_ARG_INFO(0, end)
	ZEND_ARG_INFO(0, count)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_class_Redis_xread, 0, 0, 1)
	ZEND_ARG_INFO(0, streams)
	ZEND_ARG_INFO(0, count)
	ZEND_ARG_INFO(0, block)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_class_Redis_xreadgroup, 0, 0, 3)
	ZEND_ARG_INFO(0, group)
	ZEND_ARG_INFO(0, consumer)
	ZEND_ARG_INFO(0, streams)
	ZEND_ARG_INFO(0, count)
	ZEND_ARG_INFO(0, block)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_class_Redis_xrevrange, 0, 0, 3)
	ZEND_ARG_INFO(0, key)
	ZEND_ARG_INFO(0, end)
	ZEND_ARG_INFO(0, start)
	ZEND_ARG_INFO(0, count)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_class_Redis_xtrim, 0, 0, 2)
	ZEND_ARG_INFO(0, key)
	ZEND_ARG_INFO(0, threshold)
	ZEND_ARG_INFO(0, approx)
	ZEND_ARG_INFO(0, minid)
	ZEND_ARG_INFO(0, limit)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_class_Redis_zAdd, 0, 0, 2)
	ZEND_ARG_INFO(0, key)
	ZEND_ARG_INFO(0, score_or_options)
	ZEND_ARG_VARIADIC_INFO(0, more_scores_and_mems)
ZEND_END_ARG_INFO()

#define arginfo_class_Redis_zCard arginfo_class_Redis__prefix

#define arginfo_class_Redis_zCount arginfo_class_Redis_getRange

ZEND_BEGIN_ARG_INFO_EX(arginfo_class_Redis_zIncrBy, 0, 0, 3)
	ZEND_ARG_INFO(0, key)
	ZEND_ARG_INFO(0, value)
	ZEND_ARG_INFO(0, member)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_class_Redis_zLexCount, 0, 0, 3)
	ZEND_ARG_INFO(0, key)
	ZEND_ARG_INFO(0, min)
	ZEND_ARG_INFO(0, max)
ZEND_END_ARG_INFO()

#define arginfo_class_Redis_zMscore arginfo_class_Redis_geohash

#define arginfo_class_Redis_zPopMax arginfo_class_Redis_lPop

#define arginfo_class_Redis_zPopMin arginfo_class_Redis_lPop

ZEND_BEGIN_ARG_INFO_EX(arginfo_class_Redis_zRange, 0, 0, 3)
	ZEND_ARG_INFO(0, key)
	ZEND_ARG_INFO(0, start)
	ZEND_ARG_INFO(0, end)
	ZEND_ARG_INFO(0, options)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_class_Redis_zRangeByLex, 0, 0, 3)
	ZEND_ARG_INFO(0, key)
	ZEND_ARG_INFO(0, min)
	ZEND_ARG_INFO(0, max)
	ZEND_ARG_INFO(0, offset)
	ZEND_ARG_INFO(0, count)
ZEND_END_ARG_INFO()

#define arginfo_class_Redis_zRangeByScore arginfo_class_Redis_zRange

ZEND_BEGIN_ARG_INFO_EX(arginfo_class_Redis_zrangestore, 0, 0, 4)
	ZEND_ARG_INFO(0, dstkey)
	ZEND_ARG_INFO(0, srckey)
	ZEND_ARG_INFO(0, start)
	ZEND_ARG_INFO(0, end)
	ZEND_ARG_INFO(0, options)
ZEND_END_ARG_INFO()

#define arginfo_class_Redis_zRandMember arginfo_class_Redis_getEx

#define arginfo_class_Redis_zRank arginfo_class_Redis_hGet

#define arginfo_class_Redis_zRem arginfo_class_Redis_geohash

#define arginfo_class_Redis_zRemRangeByLex arginfo_class_Redis_zLexCount

#define arginfo_class_Redis_zRemRangeByRank arginfo_class_Redis_getRange

#define arginfo_class_Redis_zRemRangeByScore arginfo_class_Redis_getRange

ZEND_BEGIN_ARG_INFO_EX(arginfo_class_Redis_zRevRange, 0, 0, 3)
	ZEND_ARG_INFO(0, key)
	ZEND_ARG_INFO(0, start)
	ZEND_ARG_INFO(0, end)
	ZEND_ARG_INFO(0, scores)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_class_Redis_zRevRangeByLex, 0, 0, 3)
	ZEND_ARG_INFO(0, key)
	ZEND_ARG_INFO(0, max)
	ZEND_ARG_INFO(0, min)
	ZEND_ARG_INFO(0, offset)
	ZEND_ARG_INFO(0, count)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_class_Redis_zRevRangeByScore, 0, 0, 3)
	ZEND_ARG_INFO(0, key)
	ZEND_ARG_INFO(0, max)
	ZEND_ARG_INFO(0, min)
	ZEND_ARG_INFO(0, options)
ZEND_END_ARG_INFO()

#define arginfo_class_Redis_zRevRank arginfo_class_Redis_hGet

#define arginfo_class_Redis_zScore arginfo_class_Redis_hGet

ZEND_BEGIN_ARG_INFO_EX(arginfo_class_Redis_zdiff, 0, 0, 1)
	ZEND_ARG_INFO(0, keys)
	ZEND_ARG_INFO(0, options)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_class_Redis_zdiffstore, 0, 0, 2)
	ZEND_ARG_INFO(0, dst)
	ZEND_ARG_INFO(0, keys)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_class_Redis_zinter, 0, 0, 1)
	ZEND_ARG_INFO(0, keys)
	ZEND_ARG_INFO(0, weights)
	ZEND_ARG_INFO(0, options)
ZEND_END_ARG_INFO()

#define arginfo_class_Redis_zintercard arginfo_class_Redis_sintercard

ZEND_BEGIN_ARG_INFO_EX(arginfo_class_Redis_zinterstore, 0, 0, 2)
	ZEND_ARG_INFO(0, dst)
	ZEND_ARG_INFO(0, keys)
	ZEND_ARG_INFO(0, weights)
	ZEND_ARG_INFO(0, aggregate)
ZEND_END_ARG_INFO()

#define arginfo_class_Redis_zscan arginfo_class_Redis_hscan

#define arginfo_class_Redis_zunion arginfo_class_Redis_zinter

#define arginfo_class_Redis_zunionstore arginfo_class_Redis_zinterstore


ZEND_METHOD(Redis, __construct);
ZEND_METHOD(Redis, __destruct);
ZEND_METHOD(Redis, _compress);
ZEND_METHOD(Redis, _uncompress);
ZEND_METHOD(Redis, _prefix);
ZEND_METHOD(Redis, _serialize);
ZEND_METHOD(Redis, _unserialize);
ZEND_METHOD(Redis, _pack);
ZEND_METHOD(Redis, _unpack);
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
ZEND_METHOD(Redis, bzmpop);
ZEND_METHOD(Redis, zmpop);
ZEND_METHOD(Redis, blmpop);
ZEND_METHOD(Redis, lmpop);
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
ZEND_METHOD(Redis, eval_ro);
ZEND_METHOD(Redis, evalsha);
ZEND_METHOD(Redis, evalsha_ro);
ZEND_METHOD(Redis, exec);
ZEND_METHOD(Redis, exists);
ZEND_METHOD(Redis, expire);
ZEND_METHOD(Redis, expireAt);
ZEND_METHOD(Redis, failover);
ZEND_METHOD(Redis, expiretime);
ZEND_METHOD(Redis, pexpiretime);
ZEND_METHOD(Redis, fcall);
ZEND_METHOD(Redis, fcall_ro);
ZEND_METHOD(Redis, flushAll);
ZEND_METHOD(Redis, flushDB);
ZEND_METHOD(Redis, function);
ZEND_METHOD(Redis, geoadd);
ZEND_METHOD(Redis, geodist);
ZEND_METHOD(Redis, geohash);
ZEND_METHOD(Redis, geopos);
ZEND_METHOD(Redis, georadius);
ZEND_METHOD(Redis, georadius_ro);
ZEND_METHOD(Redis, georadiusbymember);
ZEND_METHOD(Redis, georadiusbymember_ro);
ZEND_METHOD(Redis, geosearch);
ZEND_METHOD(Redis, geosearchstore);
ZEND_METHOD(Redis, get);
ZEND_METHOD(Redis, getAuth);
ZEND_METHOD(Redis, getBit);
ZEND_METHOD(Redis, getEx);
ZEND_METHOD(Redis, getDBNum);
ZEND_METHOD(Redis, getDel);
ZEND_METHOD(Redis, getHost);
ZEND_METHOD(Redis, getLastError);
ZEND_METHOD(Redis, getMode);
ZEND_METHOD(Redis, getOption);
ZEND_METHOD(Redis, getPersistentID);
ZEND_METHOD(Redis, getPort);
ZEND_METHOD(Redis, getRange);
ZEND_METHOD(Redis, lcs);
ZEND_METHOD(Redis, getReadTimeout);
ZEND_METHOD(Redis, getset);
ZEND_METHOD(Redis, getTimeout);
ZEND_METHOD(Redis, getTransferredBytes);
ZEND_METHOD(Redis, clearTransferredBytes);
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
ZEND_METHOD(Redis, hRandField);
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
ZEND_METHOD(Redis, blmove);
ZEND_METHOD(Redis, lPop);
ZEND_METHOD(Redis, lPos);
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
ZEND_METHOD(Redis, reset);
ZEND_METHOD(Redis, restore);
ZEND_METHOD(Redis, role);
ZEND_METHOD(Redis, rpoplpush);
ZEND_METHOD(Redis, sAdd);
ZEND_METHOD(Redis, sAddArray);
ZEND_METHOD(Redis, sDiff);
ZEND_METHOD(Redis, sDiffStore);
ZEND_METHOD(Redis, sInter);
ZEND_METHOD(Redis, sintercard);
ZEND_METHOD(Redis, sInterStore);
ZEND_METHOD(Redis, sMembers);
ZEND_METHOD(Redis, sMisMember);
ZEND_METHOD(Redis, sMove);
ZEND_METHOD(Redis, sPop);
ZEND_METHOD(Redis, sRandMember);
ZEND_METHOD(Redis, sUnion);
ZEND_METHOD(Redis, sUnionStore);
ZEND_METHOD(Redis, save);
ZEND_METHOD(Redis, scan);
ZEND_METHOD(Redis, scard);
ZEND_METHOD(Redis, script);
ZEND_METHOD(Redis, select);
ZEND_METHOD(Redis, set);
ZEND_METHOD(Redis, setBit);
ZEND_METHOD(Redis, setRange);
ZEND_METHOD(Redis, setOption);
ZEND_METHOD(Redis, setex);
ZEND_METHOD(Redis, setnx);
ZEND_METHOD(Redis, sismember);
ZEND_METHOD(Redis, slaveof);
ZEND_METHOD(Redis, replicaof);
ZEND_METHOD(Redis, touch);
ZEND_METHOD(Redis, slowlog);
ZEND_METHOD(Redis, sort);
ZEND_METHOD(Redis, sort_ro);
ZEND_METHOD(Redis, sortAsc);
ZEND_METHOD(Redis, sortAscAlpha);
ZEND_METHOD(Redis, sortDesc);
ZEND_METHOD(Redis, sortDescAlpha);
ZEND_METHOD(Redis, srem);
ZEND_METHOD(Redis, sscan);
ZEND_METHOD(Redis, ssubscribe);
ZEND_METHOD(Redis, strlen);
ZEND_METHOD(Redis, subscribe);
ZEND_METHOD(Redis, sunsubscribe);
ZEND_METHOD(Redis, swapdb);
ZEND_METHOD(Redis, time);
ZEND_METHOD(Redis, ttl);
ZEND_METHOD(Redis, type);
ZEND_METHOD(Redis, unlink);
ZEND_METHOD(Redis, unsubscribe);
ZEND_METHOD(Redis, unwatch);
ZEND_METHOD(Redis, watch);
ZEND_METHOD(Redis, wait);
ZEND_METHOD(Redis, xack);
ZEND_METHOD(Redis, xadd);
ZEND_METHOD(Redis, xautoclaim);
ZEND_METHOD(Redis, xclaim);
ZEND_METHOD(Redis, xdel);
ZEND_METHOD(Redis, xgroup);
ZEND_METHOD(Redis, xinfo);
ZEND_METHOD(Redis, xlen);
ZEND_METHOD(Redis, xpending);
ZEND_METHOD(Redis, xrange);
ZEND_METHOD(Redis, xread);
ZEND_METHOD(Redis, xreadgroup);
ZEND_METHOD(Redis, xrevrange);
ZEND_METHOD(Redis, xtrim);
ZEND_METHOD(Redis, zAdd);
ZEND_METHOD(Redis, zCard);
ZEND_METHOD(Redis, zCount);
ZEND_METHOD(Redis, zIncrBy);
ZEND_METHOD(Redis, zLexCount);
ZEND_METHOD(Redis, zMscore);
ZEND_METHOD(Redis, zPopMax);
ZEND_METHOD(Redis, zPopMin);
ZEND_METHOD(Redis, zRange);
ZEND_METHOD(Redis, zRangeByLex);
ZEND_METHOD(Redis, zRangeByScore);
ZEND_METHOD(Redis, zrangestore);
ZEND_METHOD(Redis, zRandMember);
ZEND_METHOD(Redis, zRank);
ZEND_METHOD(Redis, zRem);
ZEND_METHOD(Redis, zRemRangeByLex);
ZEND_METHOD(Redis, zRemRangeByRank);
ZEND_METHOD(Redis, zRemRangeByScore);
ZEND_METHOD(Redis, zRevRange);
ZEND_METHOD(Redis, zRevRangeByLex);
ZEND_METHOD(Redis, zRevRangeByScore);
ZEND_METHOD(Redis, zRevRank);
ZEND_METHOD(Redis, zScore);
ZEND_METHOD(Redis, zdiff);
ZEND_METHOD(Redis, zdiffstore);
ZEND_METHOD(Redis, zinter);
ZEND_METHOD(Redis, zintercard);
ZEND_METHOD(Redis, zinterstore);
ZEND_METHOD(Redis, zscan);
ZEND_METHOD(Redis, zunion);
ZEND_METHOD(Redis, zunionstore);


static const zend_function_entry class_Redis_methods[] = {
	ZEND_ME(Redis, __construct, arginfo_class_Redis___construct, ZEND_ACC_PUBLIC)
	ZEND_ME(Redis, __destruct, arginfo_class_Redis___destruct, ZEND_ACC_PUBLIC)
	ZEND_ME(Redis, _compress, arginfo_class_Redis__compress, ZEND_ACC_PUBLIC)
	ZEND_ME(Redis, _uncompress, arginfo_class_Redis__uncompress, ZEND_ACC_PUBLIC)
	ZEND_ME(Redis, _prefix, arginfo_class_Redis__prefix, ZEND_ACC_PUBLIC)
	ZEND_ME(Redis, _serialize, arginfo_class_Redis__serialize, ZEND_ACC_PUBLIC)
	ZEND_ME(Redis, _unserialize, arginfo_class_Redis__unserialize, ZEND_ACC_PUBLIC)
	ZEND_ME(Redis, _pack, arginfo_class_Redis__pack, ZEND_ACC_PUBLIC)
	ZEND_ME(Redis, _unpack, arginfo_class_Redis__unpack, ZEND_ACC_PUBLIC)
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
	ZEND_ME(Redis, bzmpop, arginfo_class_Redis_bzmpop, ZEND_ACC_PUBLIC)
	ZEND_ME(Redis, zmpop, arginfo_class_Redis_zmpop, ZEND_ACC_PUBLIC)
	ZEND_ME(Redis, blmpop, arginfo_class_Redis_blmpop, ZEND_ACC_PUBLIC)
	ZEND_ME(Redis, lmpop, arginfo_class_Redis_lmpop, ZEND_ACC_PUBLIC)
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
	ZEND_ME(Redis, eval_ro, arginfo_class_Redis_eval_ro, ZEND_ACC_PUBLIC)
	ZEND_ME(Redis, evalsha, arginfo_class_Redis_evalsha, ZEND_ACC_PUBLIC)
	ZEND_ME(Redis, evalsha_ro, arginfo_class_Redis_evalsha_ro, ZEND_ACC_PUBLIC)
	ZEND_ME(Redis, exec, arginfo_class_Redis_exec, ZEND_ACC_PUBLIC)
	ZEND_ME(Redis, exists, arginfo_class_Redis_exists, ZEND_ACC_PUBLIC)
	ZEND_ME(Redis, expire, arginfo_class_Redis_expire, ZEND_ACC_PUBLIC)
	ZEND_ME(Redis, expireAt, arginfo_class_Redis_expireAt, ZEND_ACC_PUBLIC)
	ZEND_ME(Redis, failover, arginfo_class_Redis_failover, ZEND_ACC_PUBLIC)
	ZEND_ME(Redis, expiretime, arginfo_class_Redis_expiretime, ZEND_ACC_PUBLIC)
	ZEND_ME(Redis, pexpiretime, arginfo_class_Redis_pexpiretime, ZEND_ACC_PUBLIC)
	ZEND_ME(Redis, fcall, arginfo_class_Redis_fcall, ZEND_ACC_PUBLIC)
	ZEND_ME(Redis, fcall_ro, arginfo_class_Redis_fcall_ro, ZEND_ACC_PUBLIC)
	ZEND_ME(Redis, flushAll, arginfo_class_Redis_flushAll, ZEND_ACC_PUBLIC)
	ZEND_ME(Redis, flushDB, arginfo_class_Redis_flushDB, ZEND_ACC_PUBLIC)
	ZEND_ME(Redis, function, arginfo_class_Redis_function, ZEND_ACC_PUBLIC)
	ZEND_ME(Redis, geoadd, arginfo_class_Redis_geoadd, ZEND_ACC_PUBLIC)
	ZEND_ME(Redis, geodist, arginfo_class_Redis_geodist, ZEND_ACC_PUBLIC)
	ZEND_ME(Redis, geohash, arginfo_class_Redis_geohash, ZEND_ACC_PUBLIC)
	ZEND_ME(Redis, geopos, arginfo_class_Redis_geopos, ZEND_ACC_PUBLIC)
	ZEND_ME(Redis, georadius, arginfo_class_Redis_georadius, ZEND_ACC_PUBLIC)
	ZEND_ME(Redis, georadius_ro, arginfo_class_Redis_georadius_ro, ZEND_ACC_PUBLIC)
	ZEND_ME(Redis, georadiusbymember, arginfo_class_Redis_georadiusbymember, ZEND_ACC_PUBLIC)
	ZEND_ME(Redis, georadiusbymember_ro, arginfo_class_Redis_georadiusbymember_ro, ZEND_ACC_PUBLIC)
	ZEND_ME(Redis, geosearch, arginfo_class_Redis_geosearch, ZEND_ACC_PUBLIC)
	ZEND_ME(Redis, geosearchstore, arginfo_class_Redis_geosearchstore, ZEND_ACC_PUBLIC)
	ZEND_ME(Redis, get, arginfo_class_Redis_get, ZEND_ACC_PUBLIC)
	ZEND_ME(Redis, getAuth, arginfo_class_Redis_getAuth, ZEND_ACC_PUBLIC)
	ZEND_ME(Redis, getBit, arginfo_class_Redis_getBit, ZEND_ACC_PUBLIC)
	ZEND_ME(Redis, getEx, arginfo_class_Redis_getEx, ZEND_ACC_PUBLIC)
	ZEND_ME(Redis, getDBNum, arginfo_class_Redis_getDBNum, ZEND_ACC_PUBLIC)
	ZEND_ME(Redis, getDel, arginfo_class_Redis_getDel, ZEND_ACC_PUBLIC)
	ZEND_ME(Redis, getHost, arginfo_class_Redis_getHost, ZEND_ACC_PUBLIC)
	ZEND_ME(Redis, getLastError, arginfo_class_Redis_getLastError, ZEND_ACC_PUBLIC)
	ZEND_ME(Redis, getMode, arginfo_class_Redis_getMode, ZEND_ACC_PUBLIC)
	ZEND_ME(Redis, getOption, arginfo_class_Redis_getOption, ZEND_ACC_PUBLIC)
	ZEND_ME(Redis, getPersistentID, arginfo_class_Redis_getPersistentID, ZEND_ACC_PUBLIC)
	ZEND_ME(Redis, getPort, arginfo_class_Redis_getPort, ZEND_ACC_PUBLIC)
	ZEND_ME(Redis, getRange, arginfo_class_Redis_getRange, ZEND_ACC_PUBLIC)
	ZEND_ME(Redis, lcs, arginfo_class_Redis_lcs, ZEND_ACC_PUBLIC)
	ZEND_ME(Redis, getReadTimeout, arginfo_class_Redis_getReadTimeout, ZEND_ACC_PUBLIC)
	ZEND_ME(Redis, getset, arginfo_class_Redis_getset, ZEND_ACC_PUBLIC)
	ZEND_ME(Redis, getTimeout, arginfo_class_Redis_getTimeout, ZEND_ACC_PUBLIC)
	ZEND_ME(Redis, getTransferredBytes, arginfo_class_Redis_getTransferredBytes, ZEND_ACC_PUBLIC)
	ZEND_ME(Redis, clearTransferredBytes, arginfo_class_Redis_clearTransferredBytes, ZEND_ACC_PUBLIC)
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
	ZEND_ME(Redis, hRandField, arginfo_class_Redis_hRandField, ZEND_ACC_PUBLIC)
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
	ZEND_ME(Redis, blmove, arginfo_class_Redis_blmove, ZEND_ACC_PUBLIC)
	ZEND_ME(Redis, lPop, arginfo_class_Redis_lPop, ZEND_ACC_PUBLIC)
	ZEND_ME(Redis, lPos, arginfo_class_Redis_lPos, ZEND_ACC_PUBLIC)
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
	ZEND_ME(Redis, reset, arginfo_class_Redis_reset, ZEND_ACC_PUBLIC)
	ZEND_ME(Redis, restore, arginfo_class_Redis_restore, ZEND_ACC_PUBLIC)
	ZEND_ME(Redis, role, arginfo_class_Redis_role, ZEND_ACC_PUBLIC)
	ZEND_ME(Redis, rpoplpush, arginfo_class_Redis_rpoplpush, ZEND_ACC_PUBLIC)
	ZEND_ME(Redis, sAdd, arginfo_class_Redis_sAdd, ZEND_ACC_PUBLIC)
	ZEND_ME(Redis, sAddArray, arginfo_class_Redis_sAddArray, ZEND_ACC_PUBLIC)
	ZEND_ME(Redis, sDiff, arginfo_class_Redis_sDiff, ZEND_ACC_PUBLIC)
	ZEND_ME(Redis, sDiffStore, arginfo_class_Redis_sDiffStore, ZEND_ACC_PUBLIC)
	ZEND_ME(Redis, sInter, arginfo_class_Redis_sInter, ZEND_ACC_PUBLIC)
	ZEND_ME(Redis, sintercard, arginfo_class_Redis_sintercard, ZEND_ACC_PUBLIC)
	ZEND_ME(Redis, sInterStore, arginfo_class_Redis_sInterStore, ZEND_ACC_PUBLIC)
	ZEND_ME(Redis, sMembers, arginfo_class_Redis_sMembers, ZEND_ACC_PUBLIC)
	ZEND_ME(Redis, sMisMember, arginfo_class_Redis_sMisMember, ZEND_ACC_PUBLIC)
	ZEND_ME(Redis, sMove, arginfo_class_Redis_sMove, ZEND_ACC_PUBLIC)
	ZEND_ME(Redis, sPop, arginfo_class_Redis_sPop, ZEND_ACC_PUBLIC)
	ZEND_ME(Redis, sRandMember, arginfo_class_Redis_sRandMember, ZEND_ACC_PUBLIC)
	ZEND_ME(Redis, sUnion, arginfo_class_Redis_sUnion, ZEND_ACC_PUBLIC)
	ZEND_ME(Redis, sUnionStore, arginfo_class_Redis_sUnionStore, ZEND_ACC_PUBLIC)
	ZEND_ME(Redis, save, arginfo_class_Redis_save, ZEND_ACC_PUBLIC)
	ZEND_ME(Redis, scan, arginfo_class_Redis_scan, ZEND_ACC_PUBLIC)
	ZEND_ME(Redis, scard, arginfo_class_Redis_scard, ZEND_ACC_PUBLIC)
	ZEND_ME(Redis, script, arginfo_class_Redis_script, ZEND_ACC_PUBLIC)
	ZEND_ME(Redis, select, arginfo_class_Redis_select, ZEND_ACC_PUBLIC)
	ZEND_ME(Redis, set, arginfo_class_Redis_set, ZEND_ACC_PUBLIC)
	ZEND_ME(Redis, setBit, arginfo_class_Redis_setBit, ZEND_ACC_PUBLIC)
	ZEND_ME(Redis, setRange, arginfo_class_Redis_setRange, ZEND_ACC_PUBLIC)
	ZEND_ME(Redis, setOption, arginfo_class_Redis_setOption, ZEND_ACC_PUBLIC)
	ZEND_ME(Redis, setex, arginfo_class_Redis_setex, ZEND_ACC_PUBLIC)
	ZEND_ME(Redis, setnx, arginfo_class_Redis_setnx, ZEND_ACC_PUBLIC)
	ZEND_ME(Redis, sismember, arginfo_class_Redis_sismember, ZEND_ACC_PUBLIC)
	ZEND_ME(Redis, slaveof, arginfo_class_Redis_slaveof, ZEND_ACC_PUBLIC|ZEND_ACC_DEPRECATED)
	ZEND_ME(Redis, replicaof, arginfo_class_Redis_replicaof, ZEND_ACC_PUBLIC)
	ZEND_ME(Redis, touch, arginfo_class_Redis_touch, ZEND_ACC_PUBLIC)
	ZEND_ME(Redis, slowlog, arginfo_class_Redis_slowlog, ZEND_ACC_PUBLIC)
	ZEND_ME(Redis, sort, arginfo_class_Redis_sort, ZEND_ACC_PUBLIC)
	ZEND_ME(Redis, sort_ro, arginfo_class_Redis_sort_ro, ZEND_ACC_PUBLIC)
	ZEND_ME(Redis, sortAsc, arginfo_class_Redis_sortAsc, ZEND_ACC_PUBLIC|ZEND_ACC_DEPRECATED)
	ZEND_ME(Redis, sortAscAlpha, arginfo_class_Redis_sortAscAlpha, ZEND_ACC_PUBLIC|ZEND_ACC_DEPRECATED)
	ZEND_ME(Redis, sortDesc, arginfo_class_Redis_sortDesc, ZEND_ACC_PUBLIC|ZEND_ACC_DEPRECATED)
	ZEND_ME(Redis, sortDescAlpha, arginfo_class_Redis_sortDescAlpha, ZEND_ACC_PUBLIC|ZEND_ACC_DEPRECATED)
	ZEND_ME(Redis, srem, arginfo_class_Redis_srem, ZEND_ACC_PUBLIC)
	ZEND_ME(Redis, sscan, arginfo_class_Redis_sscan, ZEND_ACC_PUBLIC)
	ZEND_ME(Redis, ssubscribe, arginfo_class_Redis_ssubscribe, ZEND_ACC_PUBLIC)
	ZEND_ME(Redis, strlen, arginfo_class_Redis_strlen, ZEND_ACC_PUBLIC)
	ZEND_ME(Redis, subscribe, arginfo_class_Redis_subscribe, ZEND_ACC_PUBLIC)
	ZEND_ME(Redis, sunsubscribe, arginfo_class_Redis_sunsubscribe, ZEND_ACC_PUBLIC)
	ZEND_ME(Redis, swapdb, arginfo_class_Redis_swapdb, ZEND_ACC_PUBLIC)
	ZEND_ME(Redis, time, arginfo_class_Redis_time, ZEND_ACC_PUBLIC)
	ZEND_ME(Redis, ttl, arginfo_class_Redis_ttl, ZEND_ACC_PUBLIC)
	ZEND_ME(Redis, type, arginfo_class_Redis_type, ZEND_ACC_PUBLIC)
	ZEND_ME(Redis, unlink, arginfo_class_Redis_unlink, ZEND_ACC_PUBLIC)
	ZEND_ME(Redis, unsubscribe, arginfo_class_Redis_unsubscribe, ZEND_ACC_PUBLIC)
	ZEND_ME(Redis, unwatch, arginfo_class_Redis_unwatch, ZEND_ACC_PUBLIC)
	ZEND_ME(Redis, watch, arginfo_class_Redis_watch, ZEND_ACC_PUBLIC)
	ZEND_ME(Redis, wait, arginfo_class_Redis_wait, ZEND_ACC_PUBLIC)
	ZEND_ME(Redis, xack, arginfo_class_Redis_xack, ZEND_ACC_PUBLIC)
	ZEND_ME(Redis, xadd, arginfo_class_Redis_xadd, ZEND_ACC_PUBLIC)
	ZEND_ME(Redis, xautoclaim, arginfo_class_Redis_xautoclaim, ZEND_ACC_PUBLIC)
	ZEND_ME(Redis, xclaim, arginfo_class_Redis_xclaim, ZEND_ACC_PUBLIC)
	ZEND_ME(Redis, xdel, arginfo_class_Redis_xdel, ZEND_ACC_PUBLIC)
	ZEND_ME(Redis, xgroup, arginfo_class_Redis_xgroup, ZEND_ACC_PUBLIC)
	ZEND_ME(Redis, xinfo, arginfo_class_Redis_xinfo, ZEND_ACC_PUBLIC)
	ZEND_ME(Redis, xlen, arginfo_class_Redis_xlen, ZEND_ACC_PUBLIC)
	ZEND_ME(Redis, xpending, arginfo_class_Redis_xpending, ZEND_ACC_PUBLIC)
	ZEND_ME(Redis, xrange, arginfo_class_Redis_xrange, ZEND_ACC_PUBLIC)
	ZEND_ME(Redis, xread, arginfo_class_Redis_xread, ZEND_ACC_PUBLIC)
	ZEND_ME(Redis, xreadgroup, arginfo_class_Redis_xreadgroup, ZEND_ACC_PUBLIC)
	ZEND_ME(Redis, xrevrange, arginfo_class_Redis_xrevrange, ZEND_ACC_PUBLIC)
	ZEND_ME(Redis, xtrim, arginfo_class_Redis_xtrim, ZEND_ACC_PUBLIC)
	ZEND_ME(Redis, zAdd, arginfo_class_Redis_zAdd, ZEND_ACC_PUBLIC)
	ZEND_ME(Redis, zCard, arginfo_class_Redis_zCard, ZEND_ACC_PUBLIC)
	ZEND_ME(Redis, zCount, arginfo_class_Redis_zCount, ZEND_ACC_PUBLIC)
	ZEND_ME(Redis, zIncrBy, arginfo_class_Redis_zIncrBy, ZEND_ACC_PUBLIC)
	ZEND_ME(Redis, zLexCount, arginfo_class_Redis_zLexCount, ZEND_ACC_PUBLIC)
	ZEND_ME(Redis, zMscore, arginfo_class_Redis_zMscore, ZEND_ACC_PUBLIC)
	ZEND_ME(Redis, zPopMax, arginfo_class_Redis_zPopMax, ZEND_ACC_PUBLIC)
	ZEND_ME(Redis, zPopMin, arginfo_class_Redis_zPopMin, ZEND_ACC_PUBLIC)
	ZEND_ME(Redis, zRange, arginfo_class_Redis_zRange, ZEND_ACC_PUBLIC)
	ZEND_ME(Redis, zRangeByLex, arginfo_class_Redis_zRangeByLex, ZEND_ACC_PUBLIC)
	ZEND_ME(Redis, zRangeByScore, arginfo_class_Redis_zRangeByScore, ZEND_ACC_PUBLIC)
	ZEND_ME(Redis, zrangestore, arginfo_class_Redis_zrangestore, ZEND_ACC_PUBLIC)
	ZEND_ME(Redis, zRandMember, arginfo_class_Redis_zRandMember, ZEND_ACC_PUBLIC)
	ZEND_ME(Redis, zRank, arginfo_class_Redis_zRank, ZEND_ACC_PUBLIC)
	ZEND_ME(Redis, zRem, arginfo_class_Redis_zRem, ZEND_ACC_PUBLIC)
	ZEND_ME(Redis, zRemRangeByLex, arginfo_class_Redis_zRemRangeByLex, ZEND_ACC_PUBLIC)
	ZEND_ME(Redis, zRemRangeByRank, arginfo_class_Redis_zRemRangeByRank, ZEND_ACC_PUBLIC)
	ZEND_ME(Redis, zRemRangeByScore, arginfo_class_Redis_zRemRangeByScore, ZEND_ACC_PUBLIC)
	ZEND_ME(Redis, zRevRange, arginfo_class_Redis_zRevRange, ZEND_ACC_PUBLIC)
	ZEND_ME(Redis, zRevRangeByLex, arginfo_class_Redis_zRevRangeByLex, ZEND_ACC_PUBLIC)
	ZEND_ME(Redis, zRevRangeByScore, arginfo_class_Redis_zRevRangeByScore, ZEND_ACC_PUBLIC)
	ZEND_ME(Redis, zRevRank, arginfo_class_Redis_zRevRank, ZEND_ACC_PUBLIC)
	ZEND_ME(Redis, zScore, arginfo_class_Redis_zScore, ZEND_ACC_PUBLIC)
	ZEND_ME(Redis, zdiff, arginfo_class_Redis_zdiff, ZEND_ACC_PUBLIC)
	ZEND_ME(Redis, zdiffstore, arginfo_class_Redis_zdiffstore, ZEND_ACC_PUBLIC)
	ZEND_ME(Redis, zinter, arginfo_class_Redis_zinter, ZEND_ACC_PUBLIC)
	ZEND_ME(Redis, zintercard, arginfo_class_Redis_zintercard, ZEND_ACC_PUBLIC)
	ZEND_ME(Redis, zinterstore, arginfo_class_Redis_zinterstore, ZEND_ACC_PUBLIC)
	ZEND_ME(Redis, zscan, arginfo_class_Redis_zscan, ZEND_ACC_PUBLIC)
	ZEND_ME(Redis, zunion, arginfo_class_Redis_zunion, ZEND_ACC_PUBLIC)
	ZEND_ME(Redis, zunionstore, arginfo_class_Redis_zunionstore, ZEND_ACC_PUBLIC)
	ZEND_FE_END
};


static const zend_function_entry class_RedisException_methods[] = {
	ZEND_FE_END
};

static zend_class_entry *register_class_Redis(void)
{
	zend_class_entry ce, *class_entry;

	INIT_CLASS_ENTRY(ce, "Redis", class_Redis_methods);
	class_entry = zend_register_internal_class_ex(&ce, NULL);

	zval const_REDIS_NOT_FOUND_value;
	ZVAL_LONG(&const_REDIS_NOT_FOUND_value, REDIS_NOT_FOUND);
	zend_string *const_REDIS_NOT_FOUND_name = zend_string_init_interned("REDIS_NOT_FOUND", sizeof("REDIS_NOT_FOUND") - 1, 1);
	zend_declare_class_constant_ex(class_entry, const_REDIS_NOT_FOUND_name, &const_REDIS_NOT_FOUND_value, ZEND_ACC_PUBLIC, NULL);
	zend_string_release(const_REDIS_NOT_FOUND_name);

	zval const_REDIS_STRING_value;
	ZVAL_LONG(&const_REDIS_STRING_value, REDIS_STRING);
	zend_string *const_REDIS_STRING_name = zend_string_init_interned("REDIS_STRING", sizeof("REDIS_STRING") - 1, 1);
	zend_declare_class_constant_ex(class_entry, const_REDIS_STRING_name, &const_REDIS_STRING_value, ZEND_ACC_PUBLIC, NULL);
	zend_string_release(const_REDIS_STRING_name);

	zval const_REDIS_SET_value;
	ZVAL_LONG(&const_REDIS_SET_value, REDIS_SET);
	zend_string *const_REDIS_SET_name = zend_string_init_interned("REDIS_SET", sizeof("REDIS_SET") - 1, 1);
	zend_declare_class_constant_ex(class_entry, const_REDIS_SET_name, &const_REDIS_SET_value, ZEND_ACC_PUBLIC, NULL);
	zend_string_release(const_REDIS_SET_name);

	zval const_REDIS_LIST_value;
	ZVAL_LONG(&const_REDIS_LIST_value, REDIS_LIST);
	zend_string *const_REDIS_LIST_name = zend_string_init_interned("REDIS_LIST", sizeof("REDIS_LIST") - 1, 1);
	zend_declare_class_constant_ex(class_entry, const_REDIS_LIST_name, &const_REDIS_LIST_value, ZEND_ACC_PUBLIC, NULL);
	zend_string_release(const_REDIS_LIST_name);

	zval const_REDIS_ZSET_value;
	ZVAL_LONG(&const_REDIS_ZSET_value, REDIS_ZSET);
	zend_string *const_REDIS_ZSET_name = zend_string_init_interned("REDIS_ZSET", sizeof("REDIS_ZSET") - 1, 1);
	zend_declare_class_constant_ex(class_entry, const_REDIS_ZSET_name, &const_REDIS_ZSET_value, ZEND_ACC_PUBLIC, NULL);
	zend_string_release(const_REDIS_ZSET_name);

	zval const_REDIS_HASH_value;
	ZVAL_LONG(&const_REDIS_HASH_value, REDIS_HASH);
	zend_string *const_REDIS_HASH_name = zend_string_init_interned("REDIS_HASH", sizeof("REDIS_HASH") - 1, 1);
	zend_declare_class_constant_ex(class_entry, const_REDIS_HASH_name, &const_REDIS_HASH_value, ZEND_ACC_PUBLIC, NULL);
	zend_string_release(const_REDIS_HASH_name);

	zval const_REDIS_STREAM_value;
	ZVAL_LONG(&const_REDIS_STREAM_value, REDIS_STREAM);
	zend_string *const_REDIS_STREAM_name = zend_string_init_interned("REDIS_STREAM", sizeof("REDIS_STREAM") - 1, 1);
	zend_declare_class_constant_ex(class_entry, const_REDIS_STREAM_name, &const_REDIS_STREAM_value, ZEND_ACC_PUBLIC, NULL);
	zend_string_release(const_REDIS_STREAM_name);

	zval const_ATOMIC_value;
	ZVAL_LONG(&const_ATOMIC_value, ATOMIC);
	zend_string *const_ATOMIC_name = zend_string_init_interned("ATOMIC", sizeof("ATOMIC") - 1, 1);
	zend_declare_class_constant_ex(class_entry, const_ATOMIC_name, &const_ATOMIC_value, ZEND_ACC_PUBLIC, NULL);
	zend_string_release(const_ATOMIC_name);

	zval const_MULTI_value;
	ZVAL_LONG(&const_MULTI_value, MULTI);
	zend_string *const_MULTI_name = zend_string_init_interned("MULTI", sizeof("MULTI") - 1, 1);
	zend_declare_class_constant_ex(class_entry, const_MULTI_name, &const_MULTI_value, ZEND_ACC_PUBLIC, NULL);
	zend_string_release(const_MULTI_name);

	zval const_PIPELINE_value;
	ZVAL_LONG(&const_PIPELINE_value, PIPELINE);
	zend_string *const_PIPELINE_name = zend_string_init_interned("PIPELINE", sizeof("PIPELINE") - 1, 1);
	zend_declare_class_constant_ex(class_entry, const_PIPELINE_name, &const_PIPELINE_value, ZEND_ACC_PUBLIC, NULL);
	zend_string_release(const_PIPELINE_name);

	zval const_OPT_SERIALIZER_value;
	ZVAL_LONG(&const_OPT_SERIALIZER_value, REDIS_OPT_SERIALIZER);
	zend_string *const_OPT_SERIALIZER_name = zend_string_init_interned("OPT_SERIALIZER", sizeof("OPT_SERIALIZER") - 1, 1);
	zend_declare_class_constant_ex(class_entry, const_OPT_SERIALIZER_name, &const_OPT_SERIALIZER_value, ZEND_ACC_PUBLIC, NULL);
	zend_string_release(const_OPT_SERIALIZER_name);

	zval const_OPT_PREFIX_value;
	ZVAL_LONG(&const_OPT_PREFIX_value, REDIS_OPT_PREFIX);
	zend_string *const_OPT_PREFIX_name = zend_string_init_interned("OPT_PREFIX", sizeof("OPT_PREFIX") - 1, 1);
	zend_declare_class_constant_ex(class_entry, const_OPT_PREFIX_name, &const_OPT_PREFIX_value, ZEND_ACC_PUBLIC, NULL);
	zend_string_release(const_OPT_PREFIX_name);

	zval const_OPT_READ_TIMEOUT_value;
	ZVAL_LONG(&const_OPT_READ_TIMEOUT_value, REDIS_OPT_READ_TIMEOUT);
	zend_string *const_OPT_READ_TIMEOUT_name = zend_string_init_interned("OPT_READ_TIMEOUT", sizeof("OPT_READ_TIMEOUT") - 1, 1);
	zend_declare_class_constant_ex(class_entry, const_OPT_READ_TIMEOUT_name, &const_OPT_READ_TIMEOUT_value, ZEND_ACC_PUBLIC, NULL);
	zend_string_release(const_OPT_READ_TIMEOUT_name);

	zval const_OPT_TCP_KEEPALIVE_value;
	ZVAL_LONG(&const_OPT_TCP_KEEPALIVE_value, REDIS_OPT_TCP_KEEPALIVE);
	zend_string *const_OPT_TCP_KEEPALIVE_name = zend_string_init_interned("OPT_TCP_KEEPALIVE", sizeof("OPT_TCP_KEEPALIVE") - 1, 1);
	zend_declare_class_constant_ex(class_entry, const_OPT_TCP_KEEPALIVE_name, &const_OPT_TCP_KEEPALIVE_value, ZEND_ACC_PUBLIC, NULL);
	zend_string_release(const_OPT_TCP_KEEPALIVE_name);

	zval const_OPT_COMPRESSION_value;
	ZVAL_LONG(&const_OPT_COMPRESSION_value, REDIS_OPT_COMPRESSION);
	zend_string *const_OPT_COMPRESSION_name = zend_string_init_interned("OPT_COMPRESSION", sizeof("OPT_COMPRESSION") - 1, 1);
	zend_declare_class_constant_ex(class_entry, const_OPT_COMPRESSION_name, &const_OPT_COMPRESSION_value, ZEND_ACC_PUBLIC, NULL);
	zend_string_release(const_OPT_COMPRESSION_name);

	zval const_OPT_REPLY_LITERAL_value;
	ZVAL_LONG(&const_OPT_REPLY_LITERAL_value, REDIS_OPT_REPLY_LITERAL);
	zend_string *const_OPT_REPLY_LITERAL_name = zend_string_init_interned("OPT_REPLY_LITERAL", sizeof("OPT_REPLY_LITERAL") - 1, 1);
	zend_declare_class_constant_ex(class_entry, const_OPT_REPLY_LITERAL_name, &const_OPT_REPLY_LITERAL_value, ZEND_ACC_PUBLIC, NULL);
	zend_string_release(const_OPT_REPLY_LITERAL_name);

	zval const_OPT_COMPRESSION_LEVEL_value;
	ZVAL_LONG(&const_OPT_COMPRESSION_LEVEL_value, REDIS_OPT_COMPRESSION_LEVEL);
	zend_string *const_OPT_COMPRESSION_LEVEL_name = zend_string_init_interned("OPT_COMPRESSION_LEVEL", sizeof("OPT_COMPRESSION_LEVEL") - 1, 1);
	zend_declare_class_constant_ex(class_entry, const_OPT_COMPRESSION_LEVEL_name, &const_OPT_COMPRESSION_LEVEL_value, ZEND_ACC_PUBLIC, NULL);
	zend_string_release(const_OPT_COMPRESSION_LEVEL_name);

	zval const_OPT_NULL_MULTIBULK_AS_NULL_value;
	ZVAL_LONG(&const_OPT_NULL_MULTIBULK_AS_NULL_value, REDIS_OPT_NULL_MBULK_AS_NULL);
	zend_string *const_OPT_NULL_MULTIBULK_AS_NULL_name = zend_string_init_interned("OPT_NULL_MULTIBULK_AS_NULL", sizeof("OPT_NULL_MULTIBULK_AS_NULL") - 1, 1);
	zend_declare_class_constant_ex(class_entry, const_OPT_NULL_MULTIBULK_AS_NULL_name, &const_OPT_NULL_MULTIBULK_AS_NULL_value, ZEND_ACC_PUBLIC, NULL);
	zend_string_release(const_OPT_NULL_MULTIBULK_AS_NULL_name);

	zval const_SERIALIZER_NONE_value;
	ZVAL_LONG(&const_SERIALIZER_NONE_value, REDIS_SERIALIZER_NONE);
	zend_string *const_SERIALIZER_NONE_name = zend_string_init_interned("SERIALIZER_NONE", sizeof("SERIALIZER_NONE") - 1, 1);
	zend_declare_class_constant_ex(class_entry, const_SERIALIZER_NONE_name, &const_SERIALIZER_NONE_value, ZEND_ACC_PUBLIC, NULL);
	zend_string_release(const_SERIALIZER_NONE_name);

	zval const_SERIALIZER_PHP_value;
	ZVAL_LONG(&const_SERIALIZER_PHP_value, REDIS_SERIALIZER_PHP);
	zend_string *const_SERIALIZER_PHP_name = zend_string_init_interned("SERIALIZER_PHP", sizeof("SERIALIZER_PHP") - 1, 1);
	zend_declare_class_constant_ex(class_entry, const_SERIALIZER_PHP_name, &const_SERIALIZER_PHP_value, ZEND_ACC_PUBLIC, NULL);
	zend_string_release(const_SERIALIZER_PHP_name);
#if defined(HAVE_REDIS_IGBINARY)

	zval const_SERIALIZER_IGBINARY_value;
	ZVAL_LONG(&const_SERIALIZER_IGBINARY_value, REDIS_SERIALIZER_IGBINARY);
	zend_string *const_SERIALIZER_IGBINARY_name = zend_string_init_interned("SERIALIZER_IGBINARY", sizeof("SERIALIZER_IGBINARY") - 1, 1);
	zend_declare_class_constant_ex(class_entry, const_SERIALIZER_IGBINARY_name, &const_SERIALIZER_IGBINARY_value, ZEND_ACC_PUBLIC, NULL);
	zend_string_release(const_SERIALIZER_IGBINARY_name);
#endif
#if defined(HAVE_REDIS_MSGPACK)

	zval const_SERIALIZER_MSGPACK_value;
	ZVAL_LONG(&const_SERIALIZER_MSGPACK_value, REDIS_SERIALIZER_MSGPACK);
	zend_string *const_SERIALIZER_MSGPACK_name = zend_string_init_interned("SERIALIZER_MSGPACK", sizeof("SERIALIZER_MSGPACK") - 1, 1);
	zend_declare_class_constant_ex(class_entry, const_SERIALIZER_MSGPACK_name, &const_SERIALIZER_MSGPACK_value, ZEND_ACC_PUBLIC, NULL);
	zend_string_release(const_SERIALIZER_MSGPACK_name);
#endif

	zval const_SERIALIZER_JSON_value;
	ZVAL_LONG(&const_SERIALIZER_JSON_value, REDIS_SERIALIZER_JSON);
	zend_string *const_SERIALIZER_JSON_name = zend_string_init_interned("SERIALIZER_JSON", sizeof("SERIALIZER_JSON") - 1, 1);
	zend_declare_class_constant_ex(class_entry, const_SERIALIZER_JSON_name, &const_SERIALIZER_JSON_value, ZEND_ACC_PUBLIC, NULL);
	zend_string_release(const_SERIALIZER_JSON_name);

	zval const_COMPRESSION_NONE_value;
	ZVAL_LONG(&const_COMPRESSION_NONE_value, REDIS_COMPRESSION_NONE);
	zend_string *const_COMPRESSION_NONE_name = zend_string_init_interned("COMPRESSION_NONE", sizeof("COMPRESSION_NONE") - 1, 1);
	zend_declare_class_constant_ex(class_entry, const_COMPRESSION_NONE_name, &const_COMPRESSION_NONE_value, ZEND_ACC_PUBLIC, NULL);
	zend_string_release(const_COMPRESSION_NONE_name);
#if defined(HAVE_REDIS_LZF)

	zval const_COMPRESSION_LZF_value;
	ZVAL_LONG(&const_COMPRESSION_LZF_value, REDIS_COMPRESSION_LZF);
	zend_string *const_COMPRESSION_LZF_name = zend_string_init_interned("COMPRESSION_LZF", sizeof("COMPRESSION_LZF") - 1, 1);
	zend_declare_class_constant_ex(class_entry, const_COMPRESSION_LZF_name, &const_COMPRESSION_LZF_value, ZEND_ACC_PUBLIC, NULL);
	zend_string_release(const_COMPRESSION_LZF_name);
#endif
#if defined(HAVE_REDIS_ZSTD)

	zval const_COMPRESSION_ZSTD_value;
	ZVAL_LONG(&const_COMPRESSION_ZSTD_value, REDIS_COMPRESSION_ZSTD);
	zend_string *const_COMPRESSION_ZSTD_name = zend_string_init_interned("COMPRESSION_ZSTD", sizeof("COMPRESSION_ZSTD") - 1, 1);
	zend_declare_class_constant_ex(class_entry, const_COMPRESSION_ZSTD_name, &const_COMPRESSION_ZSTD_value, ZEND_ACC_PUBLIC, NULL);
	zend_string_release(const_COMPRESSION_ZSTD_name);
#endif
#if defined(HAVE_REDIS_ZSTD) && defined(ZSTD_CLEVEL_DEFAULT)

	zval const_COMPRESSION_ZSTD_DEFAULT_value;
	ZVAL_LONG(&const_COMPRESSION_ZSTD_DEFAULT_value, ZSTD_CLEVEL_DEFAULT);
	zend_string *const_COMPRESSION_ZSTD_DEFAULT_name = zend_string_init_interned("COMPRESSION_ZSTD_DEFAULT", sizeof("COMPRESSION_ZSTD_DEFAULT") - 1, 1);
	zend_declare_class_constant_ex(class_entry, const_COMPRESSION_ZSTD_DEFAULT_name, &const_COMPRESSION_ZSTD_DEFAULT_value, ZEND_ACC_PUBLIC, NULL);
	zend_string_release(const_COMPRESSION_ZSTD_DEFAULT_name);
#endif
#if defined(HAVE_REDIS_ZSTD) && !(defined(ZSTD_CLEVEL_DEFAULT))

	zval const_COMPRESSION_ZSTD_DEFAULT_value;
	ZVAL_LONG(&const_COMPRESSION_ZSTD_DEFAULT_value, 3);
	zend_string *const_COMPRESSION_ZSTD_DEFAULT_name = zend_string_init_interned("COMPRESSION_ZSTD_DEFAULT", sizeof("COMPRESSION_ZSTD_DEFAULT") - 1, 1);
	zend_declare_class_constant_ex(class_entry, const_COMPRESSION_ZSTD_DEFAULT_name, &const_COMPRESSION_ZSTD_DEFAULT_value, ZEND_ACC_PUBLIC, NULL);
	zend_string_release(const_COMPRESSION_ZSTD_DEFAULT_name);
#endif
#if defined(HAVE_REDIS_ZSTD) && defined(ZSTD_CLEVEL_MAX)

	zval const_COMPRESSION_ZSTD_MAX_value;
	ZVAL_LONG(&const_COMPRESSION_ZSTD_MAX_value, ZSTD_CLEVEL_MAX);
	zend_string *const_COMPRESSION_ZSTD_MAX_name = zend_string_init_interned("COMPRESSION_ZSTD_MAX", sizeof("COMPRESSION_ZSTD_MAX") - 1, 1);
	zend_declare_class_constant_ex(class_entry, const_COMPRESSION_ZSTD_MAX_name, &const_COMPRESSION_ZSTD_MAX_value, ZEND_ACC_PUBLIC, NULL);
	zend_string_release(const_COMPRESSION_ZSTD_MAX_name);
#endif
#if defined(HAVE_REDIS_ZSTD)

	zval const_COMPRESSION_ZSTD_MAX_value;
	ZVAL_LONG(&const_COMPRESSION_ZSTD_MAX_value, ZSTD_maxCLevel());
	zend_string *const_COMPRESSION_ZSTD_MAX_name = zend_string_init_interned("COMPRESSION_ZSTD_MAX", sizeof("COMPRESSION_ZSTD_MAX") - 1, 1);
	zend_declare_class_constant_ex(class_entry, const_COMPRESSION_ZSTD_MAX_name, &const_COMPRESSION_ZSTD_MAX_value, ZEND_ACC_PUBLIC, NULL);
	zend_string_release(const_COMPRESSION_ZSTD_MAX_name);
#endif
#if defined(HAVE_REDIS_LZ4)

	zval const_COMPRESSION_LZ4_value;
	ZVAL_LONG(&const_COMPRESSION_LZ4_value, REDIS_COMPRESSION_LZ4);
	zend_string *const_COMPRESSION_LZ4_name = zend_string_init_interned("COMPRESSION_LZ4", sizeof("COMPRESSION_LZ4") - 1, 1);
	zend_declare_class_constant_ex(class_entry, const_COMPRESSION_LZ4_name, &const_COMPRESSION_LZ4_value, ZEND_ACC_PUBLIC, NULL);
	zend_string_release(const_COMPRESSION_LZ4_name);
#endif

	zval const_OPT_SCAN_value;
	ZVAL_LONG(&const_OPT_SCAN_value, REDIS_OPT_SCAN);
	zend_string *const_OPT_SCAN_name = zend_string_init_interned("OPT_SCAN", sizeof("OPT_SCAN") - 1, 1);
	zend_declare_class_constant_ex(class_entry, const_OPT_SCAN_name, &const_OPT_SCAN_value, ZEND_ACC_PUBLIC, NULL);
	zend_string_release(const_OPT_SCAN_name);

	zval const_SCAN_RETRY_value;
	ZVAL_LONG(&const_SCAN_RETRY_value, REDIS_SCAN_RETRY);
	zend_string *const_SCAN_RETRY_name = zend_string_init_interned("SCAN_RETRY", sizeof("SCAN_RETRY") - 1, 1);
	zend_declare_class_constant_ex(class_entry, const_SCAN_RETRY_name, &const_SCAN_RETRY_value, ZEND_ACC_PUBLIC, NULL);
	zend_string_release(const_SCAN_RETRY_name);

	zval const_SCAN_NORETRY_value;
	ZVAL_LONG(&const_SCAN_NORETRY_value, REDIS_SCAN_NORETRY);
	zend_string *const_SCAN_NORETRY_name = zend_string_init_interned("SCAN_NORETRY", sizeof("SCAN_NORETRY") - 1, 1);
	zend_declare_class_constant_ex(class_entry, const_SCAN_NORETRY_name, &const_SCAN_NORETRY_value, ZEND_ACC_PUBLIC, NULL);
	zend_string_release(const_SCAN_NORETRY_name);

	zval const_SCAN_PREFIX_value;
	ZVAL_LONG(&const_SCAN_PREFIX_value, REDIS_SCAN_PREFIX);
	zend_string *const_SCAN_PREFIX_name = zend_string_init_interned("SCAN_PREFIX", sizeof("SCAN_PREFIX") - 1, 1);
	zend_declare_class_constant_ex(class_entry, const_SCAN_PREFIX_name, &const_SCAN_PREFIX_value, ZEND_ACC_PUBLIC, NULL);
	zend_string_release(const_SCAN_PREFIX_name);

	zval const_SCAN_NOPREFIX_value;
	ZVAL_LONG(&const_SCAN_NOPREFIX_value, REDIS_SCAN_NOPREFIX);
	zend_string *const_SCAN_NOPREFIX_name = zend_string_init_interned("SCAN_NOPREFIX", sizeof("SCAN_NOPREFIX") - 1, 1);
	zend_declare_class_constant_ex(class_entry, const_SCAN_NOPREFIX_name, &const_SCAN_NOPREFIX_value, ZEND_ACC_PUBLIC, NULL);
	zend_string_release(const_SCAN_NOPREFIX_name);

	zval const_BEFORE_value;
	zend_string *const_BEFORE_value_str = zend_string_init("before", strlen("before"), 1);
	ZVAL_STR(&const_BEFORE_value, const_BEFORE_value_str);
	zend_string *const_BEFORE_name = zend_string_init_interned("BEFORE", sizeof("BEFORE") - 1, 1);
	zend_declare_class_constant_ex(class_entry, const_BEFORE_name, &const_BEFORE_value, ZEND_ACC_PUBLIC, NULL);
	zend_string_release(const_BEFORE_name);

	zval const_AFTER_value;
	zend_string *const_AFTER_value_str = zend_string_init("after", strlen("after"), 1);
	ZVAL_STR(&const_AFTER_value, const_AFTER_value_str);
	zend_string *const_AFTER_name = zend_string_init_interned("AFTER", sizeof("AFTER") - 1, 1);
	zend_declare_class_constant_ex(class_entry, const_AFTER_name, &const_AFTER_value, ZEND_ACC_PUBLIC, NULL);
	zend_string_release(const_AFTER_name);

	zval const_LEFT_value;
	zend_string *const_LEFT_value_str = zend_string_init("left", strlen("left"), 1);
	ZVAL_STR(&const_LEFT_value, const_LEFT_value_str);
	zend_string *const_LEFT_name = zend_string_init_interned("LEFT", sizeof("LEFT") - 1, 1);
	zend_declare_class_constant_ex(class_entry, const_LEFT_name, &const_LEFT_value, ZEND_ACC_PUBLIC, NULL);
	zend_string_release(const_LEFT_name);

	zval const_RIGHT_value;
	zend_string *const_RIGHT_value_str = zend_string_init("right", strlen("right"), 1);
	ZVAL_STR(&const_RIGHT_value, const_RIGHT_value_str);
	zend_string *const_RIGHT_name = zend_string_init_interned("RIGHT", sizeof("RIGHT") - 1, 1);
	zend_declare_class_constant_ex(class_entry, const_RIGHT_name, &const_RIGHT_value, ZEND_ACC_PUBLIC, NULL);
	zend_string_release(const_RIGHT_name);

	zval const_OPT_MAX_RETRIES_value;
	ZVAL_LONG(&const_OPT_MAX_RETRIES_value, REDIS_OPT_MAX_RETRIES);
	zend_string *const_OPT_MAX_RETRIES_name = zend_string_init_interned("OPT_MAX_RETRIES", sizeof("OPT_MAX_RETRIES") - 1, 1);
	zend_declare_class_constant_ex(class_entry, const_OPT_MAX_RETRIES_name, &const_OPT_MAX_RETRIES_value, ZEND_ACC_PUBLIC, NULL);
	zend_string_release(const_OPT_MAX_RETRIES_name);

	zval const_OPT_BACKOFF_ALGORITHM_value;
	ZVAL_LONG(&const_OPT_BACKOFF_ALGORITHM_value, REDIS_OPT_BACKOFF_ALGORITHM);
	zend_string *const_OPT_BACKOFF_ALGORITHM_name = zend_string_init_interned("OPT_BACKOFF_ALGORITHM", sizeof("OPT_BACKOFF_ALGORITHM") - 1, 1);
	zend_declare_class_constant_ex(class_entry, const_OPT_BACKOFF_ALGORITHM_name, &const_OPT_BACKOFF_ALGORITHM_value, ZEND_ACC_PUBLIC, NULL);
	zend_string_release(const_OPT_BACKOFF_ALGORITHM_name);

	zval const_BACKOFF_ALGORITHM_DEFAULT_value;
	ZVAL_LONG(&const_BACKOFF_ALGORITHM_DEFAULT_value, REDIS_BACKOFF_ALGORITHM_DEFAULT);
	zend_string *const_BACKOFF_ALGORITHM_DEFAULT_name = zend_string_init_interned("BACKOFF_ALGORITHM_DEFAULT", sizeof("BACKOFF_ALGORITHM_DEFAULT") - 1, 1);
	zend_declare_class_constant_ex(class_entry, const_BACKOFF_ALGORITHM_DEFAULT_name, &const_BACKOFF_ALGORITHM_DEFAULT_value, ZEND_ACC_PUBLIC, NULL);
	zend_string_release(const_BACKOFF_ALGORITHM_DEFAULT_name);

	zval const_BACKOFF_ALGORITHM_CONSTANT_value;
	ZVAL_LONG(&const_BACKOFF_ALGORITHM_CONSTANT_value, REDIS_BACKOFF_ALGORITHM_CONSTANT);
	zend_string *const_BACKOFF_ALGORITHM_CONSTANT_name = zend_string_init_interned("BACKOFF_ALGORITHM_CONSTANT", sizeof("BACKOFF_ALGORITHM_CONSTANT") - 1, 1);
	zend_declare_class_constant_ex(class_entry, const_BACKOFF_ALGORITHM_CONSTANT_name, &const_BACKOFF_ALGORITHM_CONSTANT_value, ZEND_ACC_PUBLIC, NULL);
	zend_string_release(const_BACKOFF_ALGORITHM_CONSTANT_name);

	zval const_BACKOFF_ALGORITHM_UNIFORM_value;
	ZVAL_LONG(&const_BACKOFF_ALGORITHM_UNIFORM_value, REDIS_BACKOFF_ALGORITHM_UNIFORM);
	zend_string *const_BACKOFF_ALGORITHM_UNIFORM_name = zend_string_init_interned("BACKOFF_ALGORITHM_UNIFORM", sizeof("BACKOFF_ALGORITHM_UNIFORM") - 1, 1);
	zend_declare_class_constant_ex(class_entry, const_BACKOFF_ALGORITHM_UNIFORM_name, &const_BACKOFF_ALGORITHM_UNIFORM_value, ZEND_ACC_PUBLIC, NULL);
	zend_string_release(const_BACKOFF_ALGORITHM_UNIFORM_name);

	zval const_BACKOFF_ALGORITHM_EXPONENTIAL_value;
	ZVAL_LONG(&const_BACKOFF_ALGORITHM_EXPONENTIAL_value, REDIS_BACKOFF_ALGORITHM_EXPONENTIAL);
	zend_string *const_BACKOFF_ALGORITHM_EXPONENTIAL_name = zend_string_init_interned("BACKOFF_ALGORITHM_EXPONENTIAL", sizeof("BACKOFF_ALGORITHM_EXPONENTIAL") - 1, 1);
	zend_declare_class_constant_ex(class_entry, const_BACKOFF_ALGORITHM_EXPONENTIAL_name, &const_BACKOFF_ALGORITHM_EXPONENTIAL_value, ZEND_ACC_PUBLIC, NULL);
	zend_string_release(const_BACKOFF_ALGORITHM_EXPONENTIAL_name);

	zval const_BACKOFF_ALGORITHM_FULL_JITTER_value;
	ZVAL_LONG(&const_BACKOFF_ALGORITHM_FULL_JITTER_value, REDIS_BACKOFF_ALGORITHM_FULL_JITTER);
	zend_string *const_BACKOFF_ALGORITHM_FULL_JITTER_name = zend_string_init_interned("BACKOFF_ALGORITHM_FULL_JITTER", sizeof("BACKOFF_ALGORITHM_FULL_JITTER") - 1, 1);
	zend_declare_class_constant_ex(class_entry, const_BACKOFF_ALGORITHM_FULL_JITTER_name, &const_BACKOFF_ALGORITHM_FULL_JITTER_value, ZEND_ACC_PUBLIC, NULL);
	zend_string_release(const_BACKOFF_ALGORITHM_FULL_JITTER_name);

	zval const_BACKOFF_ALGORITHM_EQUAL_JITTER_value;
	ZVAL_LONG(&const_BACKOFF_ALGORITHM_EQUAL_JITTER_value, REDIS_BACKOFF_ALGORITHM_EQUAL_JITTER);
	zend_string *const_BACKOFF_ALGORITHM_EQUAL_JITTER_name = zend_string_init_interned("BACKOFF_ALGORITHM_EQUAL_JITTER", sizeof("BACKOFF_ALGORITHM_EQUAL_JITTER") - 1, 1);
	zend_declare_class_constant_ex(class_entry, const_BACKOFF_ALGORITHM_EQUAL_JITTER_name, &const_BACKOFF_ALGORITHM_EQUAL_JITTER_value, ZEND_ACC_PUBLIC, NULL);
	zend_string_release(const_BACKOFF_ALGORITHM_EQUAL_JITTER_name);

	zval const_BACKOFF_ALGORITHM_DECORRELATED_JITTER_value;
	ZVAL_LONG(&const_BACKOFF_ALGORITHM_DECORRELATED_JITTER_value, REDIS_BACKOFF_ALGORITHM_DECORRELATED_JITTER);
	zend_string *const_BACKOFF_ALGORITHM_DECORRELATED_JITTER_name = zend_string_init_interned("BACKOFF_ALGORITHM_DECORRELATED_JITTER", sizeof("BACKOFF_ALGORITHM_DECORRELATED_JITTER") - 1, 1);
	zend_declare_class_constant_ex(class_entry, const_BACKOFF_ALGORITHM_DECORRELATED_JITTER_name, &const_BACKOFF_ALGORITHM_DECORRELATED_JITTER_value, ZEND_ACC_PUBLIC, NULL);
	zend_string_release(const_BACKOFF_ALGORITHM_DECORRELATED_JITTER_name);

	zval const_OPT_BACKOFF_BASE_value;
	ZVAL_LONG(&const_OPT_BACKOFF_BASE_value, REDIS_OPT_BACKOFF_BASE);
	zend_string *const_OPT_BACKOFF_BASE_name = zend_string_init_interned("OPT_BACKOFF_BASE", sizeof("OPT_BACKOFF_BASE") - 1, 1);
	zend_declare_class_constant_ex(class_entry, const_OPT_BACKOFF_BASE_name, &const_OPT_BACKOFF_BASE_value, ZEND_ACC_PUBLIC, NULL);
	zend_string_release(const_OPT_BACKOFF_BASE_name);

	zval const_OPT_BACKOFF_CAP_value;
	ZVAL_LONG(&const_OPT_BACKOFF_CAP_value, REDIS_OPT_BACKOFF_CAP);
	zend_string *const_OPT_BACKOFF_CAP_name = zend_string_init_interned("OPT_BACKOFF_CAP", sizeof("OPT_BACKOFF_CAP") - 1, 1);
	zend_declare_class_constant_ex(class_entry, const_OPT_BACKOFF_CAP_name, &const_OPT_BACKOFF_CAP_value, ZEND_ACC_PUBLIC, NULL);
	zend_string_release(const_OPT_BACKOFF_CAP_name);

	return class_entry;
}

static zend_class_entry *register_class_RedisException(zend_class_entry *class_entry_RuntimeException)
{
	zend_class_entry ce, *class_entry;

	INIT_CLASS_ENTRY(ce, "RedisException", class_RedisException_methods);
	class_entry = zend_register_internal_class_ex(&ce, class_entry_RuntimeException);

	return class_entry;
}
