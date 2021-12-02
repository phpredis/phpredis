<?php

/**
 * @generate-function-entries
 * @generate-legacy-arginfo
 */

class RedisCluster {

    public function __construct(string|null $name, array $seeds = NULL, int|float $timeout = 0, int|float $read_timeout = 0, bool $persistant = false, mixed $auth = NULL, array $context = NULL);

    public function _compress(string $value): string;

    public function _masters(): array;

    public function _pack(mixed $value): string;

    public function _prefix(string $key): bool|string;

    public function _redir(): string|null;

    public function _serialize(mixed $value): bool|string;

    public function _uncompress(string $value): string;

    public function _unpack(string $value): mixed;

    public function _unserialize(string $value): mixed;

    public function acl(string|array $key_or_address, string $subcmd, string ...$args): mixed;

    public function append(string $key, mixed $value): bool|int;

    public function bgrewriteaof(string|array $key_or_address): bool;

    public function bgsave(string|array $key_or_address): bool;

    public function bitcount(string $key, int $start = 0, int $end = -1): bool|int;

    public function bitop(string $operation, string $deskey, string $srckey, string ...$otherkeys): bool|int;

    public function bitpos(string $key, int $bit, int $start = NULL, int $end = NULL): bool|int;

    public function blpop(string|array $key, string|int $timeout_or_key, mixed ...$extra_args): array;

    public function brpop(string|array $key, string|int $timeout_or_key, mixed ...$extra_args): array;

    public function brpoplpush(string $srckey, string $deskey, int $timeout): mixed;

    public function bzpopmax(string|array $key, string|int $timeout_or_key, mixed ...$extra_args): array;

    public function bzpopmin(string|array $key, string|int $timeout_or_key, mixed ...$extra_args): array;

    public function clearlasterror(): bool;

    public function client(string|array $node, string $subcommand, string|null $arg): array|string|bool;

    public function close(): bool;

    public function cluster(string|array $node, string $command, mixed ...$extra_args): mixed;

    public function command(mixed ...$extra_args): mixed;

    public function config(string|array $node, string $subcommand, mixed ...$extra_args): mixed;

    public function dbsize(string|array $key_or_address): int;

    public function decr(string $key): int;

    public function decrby(string $key, int $value): int;

    public function decrbyfloat(string $key, float $value): float;

    public function del(string $key, string ...$other_keys): array;

    public function discard(): bool;

    public function dump(string $key): string;

    public function echo(string|array $node, string $msg): string;

    public function eval(string $script, array $args = [], int $num_keys = 0): mixed;

    public function evalsha(string $script_sha, array $args = [], int $num_keys = 0): mixed;

    public function exec(): array;

    public function exists(string $key): int;

    public function expire(string $key, int $timeout): bool;

    public function expireat(string $key, int $timestamp): bool;

    public function flushall(string|array $node, bool $async = false): bool;

    public function flushdb(string|array $node, bool $async = false): bool;

    public function geoadd(string $key, float $lng, float $lat, string $member, mixed ...$other_triples): int;

    public function geodist(string $key, string $src, string $dest, ?string $unit = null): array;

    public function geohash(string $key, string $member, string ...$other_members): array;

    public function geopos(string $key, string $member, string ...$other_members): array;

    public function georadius(string $key, float $lng, float $lat, float $radius, string $unit, array $options = []): array;

    public function georadius_ro(string $key, float $lng, float $lat, float $radius, string $unit, array $options = []): array;

    public function georadiusbymember(string $key, string $member, float $radius, string $unit, array $options = []): array;

    public function georadiusbymember_ro(string $key, string $member, float $radius, string $unit, array $options = []): array;

    public function get(string $key): string;

    public function getbit(string $key, int $value): int;

    public function getlasterror(): string|null;

    public function getmode(): int;

    public function getoption(int $option): mixed;

    public function getrange(string $key, int $start, int $end): string;

    public function getset(string $key, mixed $value): string;

    public function hdel(string $key, string $member, string ...$other_members): int;

    public function hexists(string $key, string $member): bool;

    public function hget(string $key, string $member): string;

    public function hgetall(string $key): array;

    public function hincrby(string $key, string $member, int $value): int;

    public function hincrbyfloat(string $key, string $member, float $value): float;

    public function hkeys(string $key): array;

    public function hlen(string $key): int;

    public function hmget(string $key, array $members): array;

    public function hmset(string $key, array $key_values): bool;

    public function hscan(string $key, int &$iterator, ?string $pattern = null, int $count = 0): array|bool;

    public function hset(string $key, string $member, mixed $value): int;

    public function hsetnx(string $key, string $member, mixed $value): bool;

    public function hstrlen(string $key, string $field): int;

    public function hvals(string $key): array;

    public function incr(string $key): int;

    public function incrby(string $key, int $value): int;

    public function incrbyfloat(string $key, float $value): float;

    public function info(string|array $node, ?string $section = null): array;

    public function keys(string $pattern): array;

    public function lastsave(string|array $node): int;

    public function lget(string $key, int $index): string|bool;

    public function lindex(string $key, int $index): string|bool;

    public function linsert(string $key, string $pos, mixed $pivot, mixed $value): int;

    public function llen(string $key): int|bool;

    public function lpop(string $key): string|bool;

    public function lpush(string $key, mixed $value, mixed ...$other_values): int|bool;

    public function lpushx(string $key, mixed $value): int|bool;

    public function lrange(string $key, int $start, int $end): array;

    public function lrem(string $key, int $count, string $value): int|bool;

    public function lset(string $key, int $index, string $value): bool;

    public function ltrim(string $key, int $start, int $end): bool;

    public function mget(array $keys): array;

    public function mset(array $key_values): bool;

    public function msetnx(array $key_values): int;

    public function multi(): RedisCluster|bool;

    public function object(string $subcommand, string $key): int|string;

    public function persist(string $key): bool;

    public function pexpire(string $key, int $timeout): bool;

    public function pexpireat(string $key, int $timestamp): bool;

    public function pfadd(string $key, array $elements): bool;

    public function pfcount(string $key): int;

    public function pfmerge(string $key, array $keys): bool;

    public function ping(string|array $key_or_address, ?string $message): mixed;

    public function psetex(string $key, int $timeout, string $value): bool;

    public function psubscribe(array $patterns, callable $callback): void;

    public function pttl(string $key): int;

    public function publish(string $channel, string $message): bool;

    public function pubsub(string|array $key_or_address, string ...$values): mixed;

    public function punsubscribe(string $pattern, string ...$other_patterns): bool|array;

    public function randomkey(string|array $key_or_address): bool|string;

    public function rawcommand(string|array $key_or_address, string $command, mixed ...$args): mixed;

    public function rename(string $key, string $newkey): bool;

    public function renamenx(string $key, string $newkey): bool;

    public function restore(string $key, int $timeout, string $value): bool;

    public function role(string|array $key_or_address): mixed;

    public function rpop(string $key): bool|string;

    public function rpoplpush(string $src, string $dst): bool|string;

    public function rpush(string $key, string $value, string ...$other_values): bool|int;

    public function rpushx(string $key, string $value): bool|int;

    public function sadd(string $key, string $value, string ...$other_values): bool|int;

    public function saddarray(string $key, array $values): bool|int;

    public function save(string|array $key_or_address): bool;

    public function scan(int &$iterator, mixed $node, ?string $pattern = null, int $count = 0): bool|array;

    public function scard(string $key): int;

    public function script(string|array $key_or_address, mixed ...$args): mixed;

    public function sdiff(string $key, string ...$other_keys): array;

    public function sdiffstore(string $dst, string $key, string ...$other_keys): int;

    public function set(string $key, string $value): bool;

    public function setbit(string $key, int $offset, bool $onoff): bool;

    public function setex(string $key, string $value, int $timeout): bool;

    public function setnx(string $key, string $value, int $timeout): bool;

    public function setoption(int $option, mixed $value): bool;

    public function setrange(string $key, int $offset, string $value): int;

    public function sinter(string $key, string ...$other_keys): array;

    public function sinterstore(string $dst, string $key, string ...$other_keys): bool;

    public function sismember(string $key): int;

    public function slowlog(string|array $key_or_address, mixed ...$args): mixed;

    public function smembers(string $key): array;

    public function smove(string $src, string $dst, string $member): bool;

    public function sort(string $key, array $options): bool|int|string;

    public function spop(string $key): string|array;

    public function srandmember(string $key, int $count = 0): string|array;

    public function srem(string $key, string $value, string ...$other_values): int;

    public function sscan(string $key, int &$iterator, mixed $node, ?string $pattern = null, int $count = 0): bool|array;

    public function strlen(string $key): int;

    public function subscribe(array $channels, callable $cb): void;

    public function sunion(string $key, string ...$other_keys): bool|array;

    public function sunionstore(string $dst, string $key, string ...$other_keys): int;

    public function time(string|array $key_or_address): bool|array;

    public function ttl(string $key): int;

    public function type(string $key): int;

    public function unsubscribe(array $channels): bool|array;

    public function unlink(string $key, string ...$other_keys): array;

    public function unwatch(): bool;

    public function watch(string $key, string ...$other_keys): bool;

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

    public function zadd(string $key, float $score, string $member, mixed ...$extra_args): int;

    public function zcard(string $key): int;

    public function zcount(string $key, string $start, string $end): int;

    public function zincrby(string $key, float $value, string $member): float;

    public function zinterstore(string $key, array $keys, array $weights = null, string $aggregate = null): int;

    public function zlexcount(string $key, string $min, string $max): int;

    public function zpopmax(string $key, int $value = null): bool|array;

    public function zpopmin(string $key, int $value = null): bool|array;

    public function zrange(string $key, int $start, int $end, array $options = null): bool|array;

    public function zrangebylex(string $key, int $start, int $end, array $options = null): bool|array;

    public function zrangebyscore(string $key, int $start, int $end, array $options = null): bool|array;

    public function zrank(string $key, mixed $member): int;

    public function zrem(string $key, string $value, string ...$other_values): int;

    public function zremrangebylex(string $key, string $min, string $max): int;

    public function zremrangebyrank(string $key, string $min, string $max): int;

    public function zremrangebyscore(string $key, string $min, string $max): int;

    public function zrevrange(string $key, string $min, string $max, array $options = null): bool|array;

    public function zrevrangebylex(string $key, string $min, string $max, array $options = null): bool|array;

    public function zrevrangebyscore(string $key, string $min, string $max, array $options = null): bool|array;

    public function zrevrank(string $key, mixed $member): int;

    public function zscan(string $key, int &$iterator, ?string $pattern = null, int $count = 0): bool|array;

    public function zscore(string $key): float;

    public function zunionstore(string $key, array $keys, array $weights = null, string $aggregate = null): int;
}
