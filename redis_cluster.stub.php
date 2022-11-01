<?php

/**
 * @generate-function-entries
 * @generate-legacy-arginfo
 * @generate-class-entries
 */

class RedisCluster {

    public function __construct(string|null $name, array $seeds = NULL, int|float $timeout = 0, int|float $read_timeout = 0, bool $persistent = false, #[\SensitiveParameter] mixed $auth = NULL, array $context = NULL);

    /**
     * @see Redis::_compress()
     */
    public function _compress(string $value): string;

    /**
     * @see Redis::_uncompress()
     */
    public function _uncompress(string $value): string;

    /**
     * @see Redis::_serialize()
     */
    public function _serialize(mixed $value): bool|string;

    /**
     * @see Redis::_unserialize()
     */
    public function _unserialize(string $value): mixed;

    /**
     * @see Redis::_pack()
     */
    public function _pack(mixed $value): string;

    /**
     * @see Redis::_unpack()
     */
    public function _unpack(string $value): mixed;

    /**
     * @see Redis::_prefix()
     */
    public function _prefix(string $key): bool|string;

    public function _masters(): array;

    public function _redir(): string|null;

    public function acl(string|array $key_or_address, string $subcmd, string ...$args): mixed;

    /**
     * @see Redis::append()
     */
    public function append(string $key, mixed $value): RedisCluster|bool|int;

    public function bgrewriteaof(string|array $key_or_address): RedisCluster|bool;

    public function bgsave(string|array $key_or_address): RedisCluster|bool;

    public function bitcount(string $key, int $start = 0, int $end = -1, bool $bybit = false): RedisCluster|bool|int;

    public function bitop(string $operation, string $deskey, string $srckey, string ...$otherkeys): RedisCluster|bool|int;

    /**
     * Return the position of the first bit set to 0 or 1 in a string.
     *
     * @see https://https://redis.io/commands/bitpos/
     *
     * @param string $key   The key to check (must be a string)
     * @param bool   $bit   Whether to look for an unset (0) or set (1) bit.
     * @param int    $start Where in the string to start looking.
     * @param int    $end   Where in the string to stop looking.
     * @param bool   $bybit If true, Redis will treat $start and $end as BIT values and not bytes, so if start
     *                      was 0 and end was 2, Redis would only search the first two bits.
     */
    public function bitpos(string $key, bool $bit, int $start = 0, int $end = -1, bool $bybit = false): RedisCluster|int|false;

    /**
     * See Redis::blpop()
     */
    public function blpop(string|array $key, string|float|int $timeout_or_key, mixed ...$extra_args): RedisCluster|array|null|false;

    /**
     * See Redis::brpop()
     */
    public function brpop(string|array $key, string|float|int $timeout_or_key, mixed ...$extra_args): RedisCluster|array|null|false;

    /**
     * See Redis::brpoplpush()
     */
    public function brpoplpush(string $srckey, string $deskey, int $timeout): mixed;

    public function bzpopmax(string|array $key, string|int $timeout_or_key, mixed ...$extra_args): array;

    public function bzpopmin(string|array $key, string|int $timeout_or_key, mixed ...$extra_args): array;

    public function bzmpop(float $timeout, array $keys, string $from, int $count = 1): RedisCluster|array|null|false;

    public function zmpop(array $keys, string $from, int $count = 1): RedisCluster|array|null|false;

    /**
     * @see Redis::blmpop()
     */
    public function blmpop(float $timeout, array $keys, string $from, int $count = 1): RedisCluster|array|null|false;

    /**
     * @see Redis::lmpop()
     */
    public function lmpop(array $keys, string $from, int $count = 1): RedisCluster|array|null|false;

    /**
     * @see Redis::clearlasterror()
     */
    public function clearlasterror(): bool;

    public function client(string|array $key_or_address, string $subcommand, ?string $arg = NULL): array|string|bool;

    public function close(): bool;

    public function cluster(string|array $key_or_address, string $command, mixed ...$extra_args): mixed;

    public function command(mixed ...$extra_args): mixed;

    /**
     * @see Redis::config()
     */
    public function config(string|array $key_or_address, string $subcommand, mixed ...$extra_args): mixed;

    /**
     * @see Redis::dbsize()
     */
    public function dbsize(string|array $key_or_address): RedisCluster|int;

    /**
     * @see Redis::decr()
     */
    public function decr(string $key, int $by = 1): RedisCluster|int|false;

    /**
     * @see Redis::decrby()
     */
    public function decrby(string $key, int $value): RedisCluster|int|false;

    public function decrbyfloat(string $key, float $value): float;

    /**
     * @see Redis::del()
     */
    public function del(array|string $key, string ...$other_keys): RedisCluster|int|false;

    public function discard(): bool;

    public function dump(string $key): RedisCluster|string|false;

    /**
     * @see Redis::echo()
     */
    public function echo(string|array $key_or_address, string $msg): RedisCluster|string|false;

    public function eval(string $script, array $args = [], int $num_keys = 0): mixed;

    public function eval_ro(string $script, array $args = [], int $num_keys = 0): mixed;

    public function evalsha(string $script_sha, array $args = [], int $num_keys = 0): mixed;

    public function evalsha_ro(string $script_sha, array $args = [], int $num_keys = 0): mixed;

    /**
     * @see Redis::exec()
     */
    public function exec(): array|false;

    public function exists(mixed $key, mixed ...$other_keys): RedisCluster|int|bool;

    /**
     * @see Redis::touch()
     */
    public function touch(mixed $key, mixed ...$other_keys): RedisCluster|int|bool;

    public function expire(string $key, int $timeout, ?string $mode = NULL): RedisCluster|bool;

    public function expireat(string $key, int $timestamp, ?string $mode = NULL): RedisCluster|bool;

    /**
     * @see Redis::expiretime()
     */
    public function expiretime(string $key): RedisCluster|int|false;

    /**
     * @see Redis::pexpiretime()
     */
    public function pexpiretime(string $key): RedisCluster|int|false;

    public function flushall(string|array $key_or_address, bool $async = false): RedisCluster|bool;

    public function flushdb(string|array $key_or_address, bool $async = false): RedisCluster|bool;

    public function geoadd(string $key, float $lng, float $lat, string $member, mixed ...$other_triples_and_options): RedisCluster|int|false;

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

    public function gettransferredbytes(): int|false;

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

    /**
     * Retrieve information about the connected redis-server.  If no arguments are passed to
     * this function, redis will return every info field.  Alternatively you may pass a specific
     * section you want returned (e.g. 'server', or 'memory') to receive only information pertaining
     * to that section.
     *
     * If connected to Redis server >= 7.0.0 you may pass multiple optional sections.
     *
     * @see https://redis.io/commands/info/
     *
     * @param string|array $key_or_address Either a key name or array with host and port indicating
     *                                     which cluster node we want to send the command to.
     * @param string       $sections       Optional section(s) you wish Redis server to return.
     *
     * @return Redis|array|false
     */
    public function info(string|array $key_or_address, string ...$sections): RedisCluster|array|false;

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

    public function pexpire(string $key, int $timeout, ?string $mode = NULL): RedisCluster|bool;

    public function pexpireat(string $key, int $timestamp, ?string $mode = NULL): RedisCluster|bool;


    /**
     * @see Redis::pfadd()
     */
    public function pfadd(string $key, array $elements): RedisCluster|bool;

    /**
     * @see Redis::pfcount()
     */
    public function pfcount(string $key): RedisCluster|int|false;

    /**
     * @see Redis::pfmerge()
     */
    public function pfmerge(string $key, array $keys): RedisCluster|bool;

    /**
     * PING an instance in the redis cluster.
     *
     * @see Redis::ping()
     *
     * @param string|array $key_or_address Either a key name or a two element array with host and
     *                                     address, informing RedisCluster which node to ping.
     *
     * @param string       $message        An optional message to send.
     *
     * @return mixed This method always returns `true` if no message was sent, and the message itself
     *               if one was.
     */
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

    /**
     * @see Redis::rpop()
     */
    public function rpop(string $key, int $count = 0): RedisCluster|bool|string|array;

    /**
     * @see Redis::rpoplpush()
     */
    public function rpoplpush(string $src, string $dst): RedisCluster|bool|string;

    public function rpush(string $key, mixed ...$elements): RedisCluster|int|false;

    public function rpushx(string $key, string $value): RedisCluster|bool|int;

    /**
     * @see Redis::sadd()
     */
    public function sadd(string $key, mixed $value, mixed ...$other_values): RedisCluster|int|false;

    /**
     * @see Redis::saddarray()
     */
    public function saddarray(string $key, array $values): RedisCluster|bool|int;

    public function save(string|array $key_or_address): RedisCluster|bool;

    public function scan(?int &$iterator, string|array $key_or_address, ?string $pattern = null, int $count = 0): bool|array;

    public function scard(string $key): RedisCluster|int|false;

    public function script(string|array $key_or_address, mixed ...$args): mixed;

    /**
     * @see Redis::sdiff()
     */
    public function sdiff(string $key, string ...$other_keys): RedisCluster|array|false;

    /**
     * @see Redis::sdiffstore()
     */
    public function sdiffstore(string $dst, string $key, string ...$other_keys): RedisCluster|int|false;

    /**
     * @see https://redis.io/commands/set
     */
    public function set(string $key, mixed $value, mixed $options = NULL): RedisCluster|string|bool;

    public function setbit(string $key, int $offset, bool $onoff): RedisCluster|int|false;

    public function setex(string $key, int $expire, mixed $value): RedisCluster|bool;

    public function setnx(string $key, mixed $value): RedisCluster|bool;

    public function setoption(int $option, mixed $value): bool;

    public function setrange(string $key, int $offset, string $value): RedisCluster|int|false;

    /**
     * @see Redis::sinter()
     */
    public function sinter(array|string $key, string ...$other_keys): RedisCluster|array|false;

    public function sintercard(array $keys, int $limit = -1): RedisCluster|int|false;

    /**
     * @see Redis::sinterstore()
     */
    public function sinterstore(array|string $key, string ...$other_keys): RedisCluster|int|false;

    public function sismember(string $key, mixed $value): RedisCluster|bool;

    public function slowlog(string|array $key_or_address, mixed ...$args): mixed;

    public function smembers(string $key): RedisCluster|array|false;

    public function smove(string $src, string $dst, string $member): RedisCluster|bool;

    /**
     * @see Redis::sort()
     */
    public function sort(string $key, ?array $options = NULL): RedisCluster|array|bool|int|string;

    /**
     * @see Redis::sort_ro()
     */
    public function sort_ro(string $key, ?array $options = NULL): RedisCluster|array|bool|int|string;

    public function spop(string $key, int $count = 0): RedisCluster|string|array|false;

    public function srandmember(string $key, int $count = 0): RedisCluster|string|array|false;

    /**
     * @see Redis::srem
     */
    public function srem(string $key, mixed $value, mixed ...$other_values): RedisCluster|int|false;

    public function sscan(string $key, ?int &$iterator, ?string $pattern = null, int $count = 0): array|false;

    public function strlen(string $key): RedisCluster|int|false;

    public function subscribe(array $channels, callable $cb): void;

    /**
     * @see Redis::sunion()
     */
    public function sunion(string $key, string ...$other_keys): RedisCluster|bool|array;

    /**
     * @see Redis::sunionstore()
     */
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

    /**
     * @see Redis::zrange
     */
    public function zrange(string $key, mixed $start, mixed $end, array|bool|null $options = null): RedisCluster|array|bool;

    /**
     * @see Redis::zrangestore
     */
    public function zrangestore(string $dstkey, string $srckey, int $start, int $end,
                                array|bool|null $options = null): RedisCluster|int|false;

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
