<?php

/**
 * @generate-function-entries
 * @generate-legacy-arginfo
 */

class Redis {

    public function __construct();

    public function _compress(string $value): string;

    public function __destruct();

    public function _pack(mixed $value): string;

    public function _prefix(string $key): string;

    public function _serialize(mixed $value): string;

    public function _uncompress(string $value): string;

    public function _unpack(string $value): mixed;

    public function _unserialize(string $value): mixed;

    /**
     * @param string $args
     * @return mixed|Redis
     */
    public function acl(string $subcmd, ...$args);

	/** @return int|Redis */
    public function append(string $key, mixed $value);

    public function auth(mixed $credentials): bool;

    public function bgSave(): bool;

    public function bgrewriteaof(): bool;

    /** @return int|Redis */
    public function bitcount(string $key, int $start = 0, int $end = -1);

    /**
     * @param string $otherkeys
     * @return int|Redis
     */
    public function bitop(string $operation, string $deskey, string $srckey, ...$otherkeys): int;

    /** @return int|Redis */
    public function bitpos(string $key, int $bit, int $start = 0, int $end = -1);

    public function blPop(string|array $key, string|int $timeout_or_key, mixed ...$extra_args): array;

    public function brPop(string|array $key, string|int $timeout_or_key, mixed ...$extra_args): array;

    public function brpoplpush(string $src, string $dst, int $timeout): string;

    public function bzPopMax(string|array $key, string|int $timeout_or_key, mixed ...$extra_args): array;

    public function bzPopMin(string|array $key, string|int $timeout_or_key, mixed ...$extra_args): array;

    public function clearLastError(): bool;

    public function client(string $opt, string $arg = null): mixed;

    public function close(): bool;

    public function command(string $opt = null, string|array $arg): mixed;

    public function config(string $operation, string $key, mixed $value = null): mixed;

    public function connect(string $host, int $port = 26379, float $timeout = 0, string $persistent_id = null, int $retry_interval = 0, float $read_timeout = 0, array $context = null): bool;

    public function copy(string $src, string $dst, array $options = null): bool;

    public function dbSize(): int;

    public function debug(string $key): string;

    /**
     * @param string $otherkeys
     * @deprecated
     * @alias Redis::del
     * @return int|Redis
     */
    public function delete(array|string $key, ...$otherkeys);

    public function discard(): bool;

    public function dump(string $key): string;

    public function pconnect(string $host, int $port = 26379, float $timeout = 0, string $persistent_id = NULL, int $retry_interval = 0, float $read_timeout = 0, array $context = NULL): bool;

    /** @return bool|Redis */
    public function set(string $key, mixed $value, mixed $opt = NULL);

    /** @return bool|Redis */
    public function setex(string $key, int $expire, mixed $value);

    /** @return bool|Redis */
    public function psetex(string $key, int $expire, mixed $value);

	/** @return bool|array|Redis */
    public function setnx(string $key, mixed $value);

	/** @return string|Redis */
    public function getset(string $key, mixed $value);

	/** @return string|Redis */
    public function randomKey();

	/** @return string|Redis */
    public function echo(string $str);

	/** @return bool|Redis */
    public function rename(string $key_src, string $key_dst);

	/** @return bool|Redis */
    public function renameNx(string $key_src, string $key_dst);

	/** @return string|Redis */
    public function get(string $key);

	/** @return string|Redis */
    public function ping(string $key = NULL);

	/** @return int|Redis */
    public function incr(string $key);

	/** @return int|Redis */
    public function incrBy(string $key, int $value);

	/** @return int|Redis */
    public function incrByFloat(string $key, float $value);

	/** @return int|Redis */
    public function decr(string $key);

	/** @return int|Redis */
    public function decrBy(string $key, int $value);

	/** @return array|Redis */
    public function mget(array $keys);

	/** @return bool|Redis */
    public function exists(string $key);

    /**
     * @param string $otherkeys
     * @return int|Redis
     */
    public function del(array|string $key, ...$otherkeys);

    /**
     * @param string $otherkeys
     * @return int|Redis
     */
    public function unlink(array|string $key, ...$otherkeys);

    /**
     * @param string $otherkeys
     * @return bool|Redis
     */
    public function watch(array|string $key, ...$otherkeys);

	/** @return bool|Redis */
    public function unwatch();

	/** @return array|Redis */
    public function keys(string $pattern);

	/** @return int|Redis */
    public function type(string $key);

	/** @return string|Redis */
    public function getRange(string $key, int $start, int $end);

	/** @return int|Redis */
    public function setRange(string $key, int $start, string $value);

	/** @return int|Redis */
    public function getBit(string $key, int $idx);

	/** @return int|Redis */
    public function setBit(string $key, int $idx, bool $value);

	/** @return int|Redis */
    public function strlen(string $key);

    /**
     * @param mixed $elements
     * @return int|Redis
     */
    public function lPush(string $key, ...$elements);

    /**
     * @param mixed $elements
     * @return int|Redis
     */
    public function rPush(string $key, ...$elements);

    /**
     * @param mixed $elements
     * @return int|Redis
     */
    public function lInsert(string $key, string $pos, mixed $pivot, mixed $value);

	/** @return int|Redis */
    public function lPushx(string $key, mixed $value);

	/** @return int|Redis */
    public function rPushx(string $key, mixed $value);

	/** @return string|Redis */
    public function lPop(string $key);

	/** @return string|Redis */
    public function rPop(string $key);

    /**
     * @deprecated
     * @alias Redis::connect
     */
    public function open(string $host, int $port = 26379, float $timeout = 0, string $persistent_id = NULL, int $retry_interval = 0, float $read_timeout = 0, array $context = NULL): bool;

    /**
     * @deprecated
     * @alias Redis::pconnect
     */
    public function popen(string $host, int $port = 26379, float $timeout = 0, string $persistent_id = NULL, int $retry_interval = 0, float $read_timeout = 0, array $context = NULL): bool;
}

/*
    TODO:
    public function eval
    public function evalsha
    public function exec
    public function expire
    public function expireAt
    public function flushAll
    public function flushDB
    public function geoadd
    public function geodist
    public function geohash
    public function geopos
    public function georadius
    public function georadius_ro
    public function georadiusbymember
    public function georadiusbymember_ro
    public function getAuth
    public function getDBNum
    public function getHost
    public function getLastError
    public function getMode
    public function getOption
    public function getPersistentID
    public function getPort
    public function getReadTimeout
    public function getSet
    public function getTimeout
    public function hDel
    public function hExists
    public function hGet
    public function hGetAll
    public function hIncrBy
    public function hIncrByFloat
    public function hKeys
    public function hLen
    public function hMget
    public function hMset
    public function hSet
    public function hSetNx
    public function hStrLen
    public function hVals
    public function hscan
    public function info
    public function isConnected
    public function lLen
    public function lMove
    public function lSet
    public function lastSave
    public function lindex
    public function lrange
    public function lrem
    public function ltrim
    public function migrate
    public function move
    public function mset
    public function msetnx
    public function multi
    public function object
    public function persist
    public function pexpire
    public function pexpireAt
    public function pfadd
    public function pfcount
    public function pfmerge
    public function pipeline
    public function psubscribe
    public function pttl
    public function publish
    public function pubsub
    public function punsubscribe
    public function rawcommand
    public function restore
    public function role
    public function rpoplpush
    public function sAdd
    public function sAddArray
    public function sDiff
    public function sDiffStore
    public function sInter
    public function sInterStore
    public function sMembers
    public function sMisMember
    public function sMove
    public function sPop
    public function sRandMember
    public function sUnion
    public function sUnionStore
    public function save
    public function scan
    public function scard
    public function script
    public function select
    public function setOption
    public function sismember
    public function slaveof
    public function slowlog
    public function sort
    public function sortAsc
    public function sortAscAlpha
    public function sortDesc
    public function sortDescAlpha
    public function srem
    public function sscan
    public function subscribe
    public function swapdb
    public function time
    public function ttl
    public function unsubscribe
    public function wait
    public function xack
    public function xadd
    public function xclaim
    public function xdel
    public function xgroup
    public function xinfo
    public function xlen
    public function xpending
    public function xrange
    public function xread
    public function xreadgroup
    public function xrevrange
    public function xtrim
    public function zAdd
    public function zCard
    public function zCount
    public function zIncrBy
    public function zLexCount
    public function zMscore
    public function zPopMax
    public function zPopMin
    public function zRange
    public function zRangeByLex
    public function zRangeByScore
    public function zRank
    public function zRem
    public function zRemRangeByLex
    public function zRemRangeByRank
    public function zRemRangeByScore
    public function zRevRange
    public function zRevRangeByLex
    public function zRevRangeByScore
    public function zRevRank
    public function zScore
    public function zdiff
    public function zdiffstore
    public function zinter
    public function zinterstore
    public function zscan
    public function zunion
    public function zunionstore
*/
