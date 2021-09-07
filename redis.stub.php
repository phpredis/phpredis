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
     * @return int|Redis
     */
    public function bitop(string $operation, string $deskey, string $srckey, string ...$other_keys): int;

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

	/** @return int|Redis */
    public function decr(string $key);

	/** @return int|Redis */
    public function decrBy(string $key, int $value);

    /**
     * @return int|Redis
     */
    public function del(array|string $key, string ...$other_keys);

    /**
     * @deprecated
     * @alias Redis::del
     * @return int|Redis
     */
    public function delete(array|string $key, string ...$other_keys);

    public function discard(): bool;

    public function dump(string $key): string;

	/** @return string|Redis */
    public function echo(string $str);

    public function eval(string $script, array $keys = null, int $num_keys = 0): mixed;

    public function evalsha(string $sha1, array $keys = null, int $num_keys = 0): mixed;

    public function exec(): array;

	/** @return bool|Redis */
    public function exists(string $key);

    public function expire(string $key, int $timeout): bool;

    public function expireAt(string $key, int $timestamp): bool;

    public function flushAll(bool $async = false): bool;

    public function flushDB(bool $async = false): bool;

    public function geoadd(string $key, float $lng, float $lat, string $member, mixed ...$other_triples): int;

    public function geodist(string $key, string $src, string $dst, ?string $unit = null): array;

    public function geohash(string $key, string $member, string ...$other_members): array;

    public function geopos(string $key, string $member, string ...$other_members): array;

    public function georadius(string $key, float $lng, float $lat, float $radius, string $unit, array $options = []): array;

    public function georadius_ro(string $key, float $lng, float $lat, float $radius, string $unit, array $options = []): array;

    public function georadiusbymember(string $key, string $member, float $radius, string $unit, array $options = []): array;

    public function georadiusbymember_ro(string $key, string $member, float $radius, string $unit, array $options = []): array;

	/** @return string|Redis */
    public function get(string $key);

    public function getAuth(): mixed;

	/** @return int|Redis */
    public function getBit(string $key, int $idx);

    public function getDBNum(): int;

    public function getHost(): string;

    public function getLastError(): ?string;

    public function getMode(): int;

    public function getOption(int $option): mixed;

    public function getPersistentID(): ?string;

    public function getPort(): int;

	/** @return string|Redis */
    public function getRange(string $key, int $start, int $end);

    public function getReadTimeout(): int;

	/** @return string|Redis */
    public function getset(string $key, mixed $value);

    public function getTimeout(): int;

    public function hDel(string $key, string $member, string ...$other_members): int;

    public function hExists(string $key, string $member): bool;

    public function hGet(string $key, string $member): string;

    public function hGetAll(string $key): array;

    public function hIncrBy(string $key, string $member, int $value): int;

    public function hIncrByFloat(string $key, string $member, float $value): float;

    public function hKeys(string $key): array;

    public function hLen(string $key): int;

    public function hMget(string $key, array $keys): array;

    public function hMset(string $key, array $keyvals): bool;

    public function hSet(string $key, string $member, string $value): int;

    public function hSetNx(string $key, string $member, string $value): int;

    public function hStrLen(string $key, string $member): int;

    public function hVals(string $key): array;

    public function hscan(string $key, int &$iterator, ?string $pattern = null, int $count = 0): bool|array;

	/** @return int|Redis */
    public function incr(string $key);

	/** @return int|Redis */
    public function incrBy(string $key, int $value);

	/** @return int|Redis */
    public function incrByFloat(string $key, float $value);

    public function info(string $opt = null): array;

    public function isConnected(): bool;

	/** @return array|Redis */
    public function keys(string $pattern);

    /**
     * @param mixed $elements
     * @return int|Redis
     */
    public function lInsert(string $key, string $pos, mixed $pivot, mixed $value);


    public function lLen(string $key): int;

    public function lMove(string $src, string $dst, string $wherefrom, string $whereto): string;

	/** @return string|Redis */
    public function lPop(string $key);

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

	/** @return int|Redis */
    public function lPushx(string $key, mixed $value);

	/** @return int|Redis */
    public function rPushx(string $key, mixed $value);

    public function lSet(string $key, int $index, string $value): bool;

    public function lastSave(): int;

    public function lindex(string $key, int $index): string;

    public function lrange(string $key, int $start , int $end): array;

    public function lrem(string $key, string $value, int $count = 0): bool;

    public function ltrim(string $key, int $start , int $end): bool;

	/** @return array|Redis */
    public function mget(array $keys);

    public function migrate(string $host, int $port, string $key, string $dst, int $timeout, bool $copy = false, bool $replace = false): bool;

    public function move(string $key, int $index): bool;

    public function mset(array $key_values): bool;

    public function msetnx(array $key_values): int;

    public function multi(int $value = Redis::MULTI): bool|Redis;

    public function object(string $subcommand, string $key): int|string;

    /**
     * @deprecated
     * @alias Redis::connect
     */
    public function open(string $host, int $port = 26379, float $timeout = 0, string $persistent_id = NULL, int $retry_interval = 0, float $read_timeout = 0, array $context = NULL): bool;

    public function pconnect(string $host, int $port = 26379, float $timeout = 0, string $persistent_id = NULL, int $retry_interval = 0, float $read_timeout = 0, array $context = NULL): bool;

public function persist(string $key): bool;

    public function pexpire(string $key, int $timeout): bool;

    public function pexpireAt(string $key, int $timestamp): bool;

    public function pfadd(string $key, array $elements): int;

    public function pfcount(string $key): int;

    public function pfmerge(string $dst, array $keys): bool;

	/** @return string|Redis */
    public function ping(string $key = NULL);

    public function pipeline(): bool|Redis;

    /**
     * @deprecated
     * @alias Redis::pconnect
     */
    public function popen(string $host, int $port = 26379, float $timeout = 0, string $persistent_id = NULL, int $retry_interval = 0, float $read_timeout = 0, array $context = NULL): bool;

    /** @return bool|Redis */
    public function psetex(string $key, int $expire, mixed $value);

    public function psubscribe(array $patterns): void;

    public function pttl(string $key): int;

    public function publish(string $channel, string $message): int;

    public function pubsub(string $command, mixed $arg = null): mixed;

    public function punsubscribe(array $patterns): array;

	/** @return string|Redis */
    public function rPop(string $key);

	/** @return string|Redis */
    public function randomKey();

    public function rawcommand(string $command, mixed ...$args): mixed;

	/** @return bool|Redis */
    public function rename(string $key_src, string $key_dst);

	/** @return bool|Redis */
    public function renameNx(string $key_src, string $key_dst);

    public function restore(string $key, int $timeout, string $value): bool;

    public function role(): mixed;

    public function rpoplpush(string $src, string $dst): string;

    public function sAdd(string $key, mixed $value, mixed ...$other_values): int;

    public function sAddArray(string $key, array $values): int;

    public function sDiff(string $key, string ...$other_keys): array;

    public function sDiffStore(string $dst, string $key, string ...$other_keys): int;

    public function sInter(string $key, string ...$other_keys): array;

    public function sInterStore(string $dst, string $key, string ...$other_keys): int;

    public function sMembers(string $key): array;

    public function sMisMember(string $key, string $member, string ...$other_members): array;

    public function sMove(string $src, string $dst, mixed $value): bool;

    public function sPop(string $key, int $count = 0): string|array;

    public function sRandMember(string $key, int $count = 0): string|array;

    public function sUnion(string $key, string ...$other_keys): array;

    public function sUnionStore(string $dst, string $key, string ...$other_keys): int;

    public function save(): bool;

    public function scan(int &$iterator, ?string $pattern = null, int $count = 0): array;

    public function scard(string $key): int;

    public function script(string $command, mixed ...$args): mixed;

    public function select(int $db): bool;

    /** @return bool|Redis */
    public function set(string $key, mixed $value, mixed $opt = NULL);

	/** @return int|Redis */
    public function setBit(string $key, int $idx, bool $value);

	/** @return int|Redis */
    public function setRange(string $key, int $start, string $value);


    public function setOption(string $option, mixed $value): bool;

    /** @return bool|Redis */
    public function setex(string $key, int $expire, mixed $value);

	/** @return bool|array|Redis */
    public function setnx(string $key, mixed $value);

    public function sismember(string $key, string $value): bool;

    public function slaveof(string $host = null, int $port = 6379): bool;

    public function slowlog(string $mode, int $option = 0): mixed;

    public function sort(string $key, array $options = null): mixed;

    /**
     * @deprecated
     */
    public function sortAsc(string $key, ?string $pattern = null, mixed $get = null, int $offset = -1, int $count = -1, ?string $store = null): array;

    /**
     * @deprecated
     */
    public function sortAscAlpha(string $key, ?string $pattern = null, mixed $get = null, int $offset = -1, int $count = -1, ?string $store = null): array;

    /**
     * @deprecated
     */
    public function sortDesc(string $key, ?string $pattern = null, mixed $get = null, int $offset = -1, int $count = -1, ?string $store = null): array;

    /**
     * @deprecated
     */
    public function sortDescAlpha(string $key, ?string $pattern = null, mixed $get = null, int $offset = -1, int $count = -1, ?string $store = null): array;

    public function srem(string $key, string $value, string ...$other_values): int;

    public function sscan(string $key, int &$iterator, ?string $pattern = null, int $count = 0): array;

	/** @return int|Redis */
    public function strlen(string $key);

    public function subscribe(string $channel, string ...$other_channels): array;

    public function swapdb(string $src, string $dst): bool;

    public function time(): array;

    public function ttl(string $key): int;

	/** @return int|Redis */
    public function type(string $key);

       /**
     * @return int|Redis
     */
    public function unlink(array|string $key, string ...$other_keys);

    public function unsubscribe(string $channel, string ...$other_channels): array;

	/** @return bool|Redis */
    public function unwatch();

    /**
     * @return bool|Redis
     */
    public function watch(array|string $key, string ...$other_keys);

    public function wait(int $count, int $timeout): int;

    public function xack(string $key, string $group, array $ids): int;

    public function xadd(string $key, string $id, array $values, int $maxlen = 0, bool $approx = false): string;

    public function xclaim(string $key, string $group, string $consumer, int $min_iddle, array $ids, array $options): string|array;

    public function xdel(string $key, array $ids): int;

    public function xgroup(string $operation, string $key = null, string $arg1 = null, string $arg2 = null, bool $arg3 = false): mixed;

    public function xinfo(string $operation, string $arg1 = null, string $arg2 = null): mixed;

    public function xlen(string $key): int;

    public function xpending(string $key, string $group, string $start = null, string $end = null, int $count = -1, string $consumer = null): string;

    public function xrange(string $key, string $start, string $end, int $count = -1): bool|array;

    public function xread(array $streams, int $count = -1, int $block = -1): bool|array;

    public function xreadgroup(string $group, string $consumer, array $streams, int $count = 1, int $block = 1): bool|array;

    public function xrevrange(string $key, string $start, string $end, int $count = -1): bool|array;

    public function xtrim(string $key, int $maxlen, bool $approx = false): int;

    public function zAdd(string $key, int $score, string $value): int;

    public function zCard(string $key): int;

    public function zCount(string $key, string $start , string $end): int;

    public function zIncrBy(string $key, float $value, mixed $member): float;

    public function zLexCount(string $key, string $min, string $max): int;

    public function zMscore(string $key, string $member, string ...$other_members): array;

    public function zPopMax(string $key, int $value = null): array;

    public function zPopMin(string $key, int $value = null): array;

    public function zRange(string $key, int $start, int $end, mixed $scores = null): array;

    public function zRangeByLex(string $key, string $min, string $max, int $offset = -1, int $count = -1): array;

    public function zRangeByScore(string $key, string $start, string $end, array $options = []): array;

    public function zRank(string $key, string $member): int;

    public function zRem(string $key, string $member, string ...$other_members): int;

    public function zRemRangeByLex(string $key, string $min, string $max): int;

    public function zRemRangeByRank(string $key, int $start, int $end): int;

    public function zRemRangeByScore(string $key, string $start, string $end): int;

    public function zRevRange(string $key, int $start, int $end, mixed $scores = null): array;

    public function zRevRangeByLex(string $key, string $min, string $max, int $offset = -1, int $count = -1): array;

    public function zRevRangeByScore(string $key, string $start, string $end, array $options = []): array;

    public function zRevRank(string $key, string $member): int;

    public function zScore(string $key, mixed $member): float;

    public function zdiff(array $keys, array $options = null): array;

    public function zdiffstore(string $dst, array $keys, array $options = null): int;

    public function zinter(array $keys, array $weights = null, array $options = null): array;

    public function zinterstore(string $dst, array $keys, array $weights = null, string $aggregate = null): int;

    public function zscan(string $key, int &$iterator, ?string $pattern = null, int $count = 0): bool|array;

    public function zunion(array $keys, array $weights = null, array $options = null): array;

    public function zunionstore(string $dst, array $keys, array $weights = null, string $aggregate = null): int;
}
