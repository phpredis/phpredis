<?php

/**
 * @generate-function-entries
 * @generate-legacy-arginfo
 * @generate-class-entries
 */

class RedisCluster {

    public function __construct(string|null $name, array $seeds = NULL, int|float $timeout = 0, int|float $read_timeout = 0, bool $persistent = false, #[\SensitiveParameter] mixed $auth = NULL, array $context = NULL);

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

    public function append(string $key, mixed $value): RedisCluster|bool|int;

    public function bgrewriteaof(string|array $key_or_address): RedisCluster|bool;

    public function bgsave(string|array $key_or_address): RedisCluster|bool;

    public function bitcount(string $key, int $start = 0, int $end = -1, bool $bybit = false): RedisCluster|bool|int;

    public function bitop(string $operation, string $deskey, string $srckey, string ...$otherkeys): RedisCluster|bool|int;

    public function bitpos(string $key, int $bit, int $start = NULL, int $end = NULL): RedisCluster|bool|int;

    public function blpop(string|array $key, string|float|int $timeout_or_key, mixed ...$extra_args): RedisCluster|array|null|false;
    public function brpop(string|array $key, string|float|int $timeout_or_key, mixed ...$extra_args): RedisCluster|array|null|false;

    public function brpoplpush(string $srckey, string $deskey, int $timeout): mixed;

    public function bzpopmax(string|array $key, string|int $timeout_or_key, mixed ...$extra_args): array;

    public function bzpopmin(string|array $key, string|int $timeout_or_key, mixed ...$extra_args): array;

    public function bzmpop(float $timeout, array $keys, string $from, int $count = 1): RedisCluster|array|null|false;

    public function zmpop(array $keys, string $from, int $count = 1): RedisCluster|array|null|false;

    public function blmpop(float $timeout, array $keys, string $from, int $count = 1): RedisCluster|array|null|false;

    public function lmpop(array $keys, string $from, int $count = 1): RedisCluster|array|null|false;

    public function clearlasterror(): bool;

    public function client(string|array $key_or_address, string $subcommand, ?string $arg = NULL): array|string|bool;

    public function close(): bool;

    public function cluster(string|array $key_or_address, string $command, mixed ...$extra_args): mixed;

    public function command(mixed ...$extra_args): mixed;

    public function config(string|array $key_or_address, string $subcommand, mixed ...$extra_args): mixed;

    public function dbsize(string|array $key_or_address): RedisCluster|int;

    public function decr(string $key, int $by = 1): RedisCluster|int|false;

    public function decrby(string $key, int $value): RedisCluster|int|false;

    public function decrbyfloat(string $key, float $value): float;

    public function del(array|string $key, string ...$other_keys): RedisCluster|int|false;

    public function discard(): bool;

    public function dump(string $key): RedisCluster|string|false;

    public function echo(string|array $key_or_address, string $msg): RedisCluster|string|false;

    public function eval(string $script, array $args = [], int $num_keys = 0): mixed;

    public function evalsha(string $script_sha, array $args = [], int $num_keys = 0): mixed;

    public function exec(): array|false;

    public function exists(mixed $key, mixed ...$other_keys): RedisCluster|int|bool;

    public function expire(string $key, int $timeout): RedisCluster|bool;

    public function expireat(string $key, int $timestamp): RedisCluster|bool;

    public function expiretime(string $key): RedisCluster|int|false;

    public function pexpiretime(string $key): RedisCluster|int|false;

    public function flushall(string|array $key_or_address, bool $async = false): RedisCluster|bool;

    public function flushdb(string|array $key_or_address, bool $async = false): RedisCluster|bool;

    public function geoadd(string $key, float $lng, float $lat, string $member, mixed ...$other_triples): RedisCluster|int;

    public function geodist(string $key, string $src, string $dest, ?string $unit = null): RedisCluster|float|false;

    public function geohash(string $key, string $member, string ...$other_members): RedisCluster|array|false;

    public function geopos(string $key, string $member, string ...$other_members): RedisCluster|array|false;

    public function georadius(string $key, float $lng, float $lat, float $radius, string $unit, array $options = []): mixed;

    public function georadius_ro(string $key, float $lng, float $lat, float $radius, string $unit, array $options = []): mixed;

    public function georadiusbymember(string $key, string $member, float $radius, string $unit, array $options = []): mixed;

    public function georadiusbymember_ro(string $key, string $member, float $radius, string $unit, array $options = []): mixed;

    public function get(string $key): mixed;

    public function getbit(string $key, int $value): RedisCluster|int|false;

    public function getlasterror(): string|null;

    public function getmode(): int;

    public function getoption(int $option): mixed;

    public function getrange(string $key, int $start, int $end): RedisCluster|string|false;

    public function lcs(string $key1, string $key2, ?array $options = NULL): RedisCluster|string|array|int|false;

    public function getset(string $key, mixed $value): RedisCluster|string|bool;

    public function hdel(string $key, string $member, string ...$other_members): RedisCluster|int|false;

    public function hexists(string $key, string $member): RedisCluster|bool;

    public function hget(string $key, string $member): mixed;

    public function hgetall(string $key): RedisCluster|array|false;

    public function hincrby(string $key, string $member, int $value): RedisCluster|int|false;

    public function hincrbyfloat(string $key, string $member, float $value): RedisCluster|float|false;

    public function hkeys(string $key): RedisCluster|array|false;

    public function hlen(string $key): RedisCluster|int|false;

    public function hmget(string $key, array $keys): RedisCluster|array|false;

    public function hmset(string $key, array $key_values): RedisCluster|bool;

    public function hscan(string $key, ?int &$iterator, ?string $pattern = null, int $count = 0): array|bool;

    public function hset(string $key, string $member, mixed $value): RedisCluster|int|false;

    public function hsetnx(string $key, string $member, mixed $value): RedisCluster|bool;

    public function hstrlen(string $key, string $field): RedisCluster|int|false;

    public function hvals(string $key): RedisCluster|array|false;

    public function incr(string $key, int $by = 1): RedisCluster|int|false;

    public function incrby(string $key, int $value): RedisCluster|int|false;

    public function incrbyfloat(string $key, float $value): RedisCluster|float|false;

    public function info(string|array $key_or_address, ?string $section = null): RedisCluster|array|false;

    public function keys(string $pattern): RedisCluster|array|false;

    public function lastsave(string|array $key_or_address): RedisCluster|int|false;

    public function lget(string $key, int $index): RedisCluster|string|bool;

    public function lindex(string $key, int $index): mixed;

    public function linsert(string $key, string $pos, mixed $pivot, mixed $value): RedisCluster|int|false;

    public function llen(string $key): RedisCluster|int|bool;

    public function lpop(string $key, int $count = 0): RedisCluster|bool|string|array;

    public function lpush(string $key, mixed $value, mixed ...$other_values): RedisCluster|int|bool;

    public function lpushx(string $key, mixed $value): RedisCluster|int|bool;

    public function lrange(string $key, int $start, int $end): RedisCluster|array|false;

    public function lrem(string $key, mixed $value, int $count = 0): RedisCluster|int|bool;

    public function lset(string $key, int $index, mixed $value): RedisCluster|bool;

    public function ltrim(string $key, int $start, int $end): RedisCluster|bool;

    public function mget(array $keys): RedisCluster|array|false;

    public function mset(array $key_values): RedisCluster|bool;

    public function msetnx(array $key_values): RedisCluster|array|false;

    /* We only support Redis::MULTI in RedisCluster but take the argument
       so we can test MULTI..EXEC with RedisTest.php and in the event
       we add pipeline support in the future. */
    public function multi(int $value = Redis::MULTI): RedisCluster|bool;

    public function object(string $subcommand, string $key): RedisCluster|int|string|false;

    public function persist(string $key): RedisCluster|bool;

    public function pexpire(string $key, int $timeout): RedisCluster|bool;

    public function pexpireat(string $key, int $timestamp): RedisCluster|bool;

    public function pfadd(string $key, array $elements): RedisCluster|bool;

    public function pfcount(string $key): RedisCluster|int|false;

    public function pfmerge(string $key, array $keys): RedisCluster|bool;

    public function ping(string|array $key_or_address, ?string $message = NULL): mixed;

    public function psetex(string $key, int $timeout, string $value): RedisCluster|bool;

    public function psubscribe(array $patterns, callable $callback): void;

    public function pttl(string $key): RedisCluster|int|false;

    public function publish(string $channel, string $message): RedisCluster|bool;

    public function pubsub(string|array $key_or_address, string ...$values): mixed;

    public function punsubscribe(string $pattern, string ...$other_patterns): bool|array;

    public function randomkey(string|array $key_or_address): RedisCluster|bool|string;

    public function rawcommand(string|array $key_or_address, string $command, mixed ...$args): mixed;

    public function rename(string $key_src, string $key_dst): RedisCluster|bool;

    public function renamenx(string $key, string $newkey): RedisCluster|bool;

    public function restore(string $key, int $timeout, string $value, ?array $options = NULL): RedisCluster|bool;

    public function role(string|array $key_or_address): mixed;

    public function rpop(string $key, int $count = 0): RedisCluster|bool|string|array;

    public function rpoplpush(string $src, string $dst): RedisCluster|bool|string;

    public function rpush(string $key, mixed ...$elements): RedisCluster|int|false;

    public function rpushx(string $key, string $value): RedisCluster|bool|int;

    public function sadd(string $key, mixed $value, mixed ...$other_values): RedisCluster|int|false;

    public function saddarray(string $key, array $values): RedisCluster|bool|int;

    public function save(string|array $key_or_address): RedisCluster|bool;

    public function scan(?int &$iterator, string|array $key_or_address, ?string $pattern = null, int $count = 0): bool|array;

    public function scard(string $key): RedisCluster|int|false;

    public function script(string|array $key_or_address, mixed ...$args): mixed;

    public function sdiff(string $key, string ...$other_keys): RedisCluster|array|false;

    public function sdiffstore(string $dst, string $key, string ...$other_keys): RedisCluster|int|false;

    public function set(string $key, mixed $value, mixed $options = NULL): RedisCluster|string|bool;

    public function setbit(string $key, int $offset, bool $onoff): RedisCluster|int|false;

    public function setex(string $key, int $expire, mixed $value): RedisCluster|bool;

    public function setnx(string $key, mixed $value): RedisCluster|bool;

    public function setoption(int $option, mixed $value): bool;

    public function setrange(string $key, int $offset, string $value): RedisCluster|int|false;

    public function sinter(array|string $key, string ...$other_keys): RedisCluster|array|false;

    public function sintercard(array $keys, int $limit = -1): RedisCluster|int|false;

    public function sinterstore(array|string $key, string ...$other_keys): RedisCluster|int|false;

    public function sismember(string $key, mixed $value): RedisCluster|bool;

    public function slowlog(string|array $key_or_address, mixed ...$args): mixed;

    public function smembers(string $key): RedisCluster|array|false;

    public function smove(string $src, string $dst, string $member): RedisCluster|bool;

    public function sort(string $key, ?array $options = NULL): RedisCluster|array|bool|int|string;

    public function spop(string $key, int $count = 0): RedisCluster|string|array|false;

    public function srandmember(string $key, int $count = 0): RedisCluster|string|array|false;

    public function srem(string $key, mixed $value, mixed ...$other_values): RedisCluster|int|false;

    public function sscan(string $key, ?int &$iterator, ?string $pattern = null, int $count = 0): array|false;

    public function strlen(string $key): RedisCluster|int|false;

    public function subscribe(array $channels, callable $cb): void;

    public function sunion(string $key, string ...$other_keys): RedisCluster|bool|array;

    public function sunionstore(string $dst, string $key, string ...$other_keys): RedisCluster|int|false;

    public function time(string|array $key_or_address): RedisCluster|bool|array;

    public function ttl(string $key): RedisCluster|int|false;

    public function type(string $key): RedisCluster|int|false;

    public function unsubscribe(array $channels): bool|array;

    public function unlink(array|string $key, string ...$other_keys): RedisCluster|int|false;

    public function unwatch(): bool;

    public function watch(string $key, string ...$other_keys): RedisCluster|bool;

    public function xack(string $key, string $group, array $ids): RedisCluster|int|false;

    public function xadd(string $key, string $id, array $values, int $maxlen = 0, bool $approx = false): RedisCluster|string|false;

    public function xclaim(string $key, string $group, string $consumer, int $min_iddle, array $ids, array $options): RedisCluster|string|array|false;

    public function xdel(string $key, array $ids): RedisCluster|int|false;

    public function xgroup(string $operation, string $key = null, string $arg1 = null, string $arg2 = null, bool $arg3 = false): mixed;

    public function xinfo(string $operation, ?string $arg1 = null, ?string $arg2 = null, int $count = -1): mixed;

    public function xlen(string $key): RedisCluster|int|false;

    public function xpending(string $key, string $group, ?string $start = null, ?string $end = null, int $count = -1, ?string $consumer = null): RedisCluster|array|false;

    public function xrange(string $key, string $start, string $end, int $count = -1): RedisCluster|bool|array;

    public function xread(array $streams, int $count = -1, int $block = -1): RedisCluster|bool|array;

    public function xreadgroup(string $group, string $consumer, array $streams, int $count = 1, int $block = 1): RedisCluster|bool|array;

    public function xrevrange(string $key, string $start, string $end, int $count = -1): RedisCluster|bool|array;

    public function xtrim(string $key, int $maxlen, bool $approx = false, bool $minid = false, int $limit = -1): RedisCluster|int|false;

    public function zadd(string $key, array|float $score_or_options, mixed ...$more_scores_and_mems): RedisCluster|int|false;

    public function zcard(string $key): RedisCluster|int|false;

    public function zcount(string $key, string $start, string $end): RedisCluster|int|false;

    public function zincrby(string $key, float $value, string $member): RedisCluster|float|false;

    public function zinterstore(string $dst, array $keys, ?array $weights = null, ?string $aggregate = null): RedisCluster|int|false;

    public function zintercard(array $keys, int $limit = -1): RedisCluster|int|false;

    public function zlexcount(string $key, string $min, string $max): RedisCluster|int|false;

    public function zpopmax(string $key, int $value = null): RedisCluster|bool|array;

    public function zpopmin(string $key, int $value = null): RedisCluster|bool|array;

    public function zrange(string $key, int $start, int $end, mixed $options_withscores = null): RedisCluster|array|bool;

    public function zrangebylex(string $key, string $min, string $max, int $offset = -1, int $count = -1): RedisCluster|array|false;

    public function zrangebyscore(string $key, string $start, string $end, array $options = []): RedisCluster|array|false;

    public function zrank(string $key, mixed $member): RedisCluster|int|false;

    public function zrem(string $key, string $value, string ...$other_values): RedisCluster|int|false;

    public function zremrangebylex(string $key, string $min, string $max): RedisCluster|int|false;

    public function zremrangebyrank(string $key, string $min, string $max): RedisCluster|int|false;

    public function zremrangebyscore(string $key, string $min, string $max): RedisCluster|int|false;

    public function zrevrange(string $key, string $min, string $max, array $options = null): RedisCluster|bool|array;

    public function zrevrangebylex(string $key, string $min, string $max, array $options = null): RedisCluster|bool|array;

    public function zrevrangebyscore(string $key, string $min, string $max, array $options = null): RedisCluster|bool|array;

    public function zrevrank(string $key, mixed $member): RedisCluster|int|false;

    public function zscan(string $key, ?int &$iterator, ?string $pattern = null, int $count = 0): RedisCluster|bool|array;

    public function zscore(string $key, mixed $member): RedisCluster|float|false;

    public function zunionstore(string $dst, array $keys, ?array $weights = NULL, ?string $aggregate = NULL): RedisCluster|int|false;
}

class RedisClusterException extends RuntimeException {}
