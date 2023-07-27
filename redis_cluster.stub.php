<?php

/**
 * @generate-function-entries
 * @generate-legacy-arginfo
 * @generate-class-entries
 */

class RedisCluster {
    /**
     *
     * @var int
     * @cvalue REDIS_OPT_FAILOVER
     *
     */
    public const OPT_SLAVE_FAILOVER = UNKNOWN;

    /**
     *
     * @var int
     * @cvalue REDIS_FAILOVER_NONE
     *
     */
    public const FAILOVER_NONE = UNKNOWN;

    /**
     *
     * @var int
     * @cvalue REDIS_FAILOVER_ERROR
     *
     */
    public const FAILOVER_ERROR = UNKNOWN;

    /**
     *
     * @var int
     * @cvalue REDIS_FAILOVER_DISTRIBUTE
     *
     */
    public const FAILOVER_DISTRIBUTE = UNKNOWN;

    /**
     *
     * @var int
     * @cvalue REDIS_FAILOVER_DISTRIBUTE_SLAVES
     *
     */
    public const FAILOVER_DISTRIBUTE_SLAVES = UNKNOWN;

    public function __construct(string|null $name, ?array $seeds = null, int|float $timeout = 0, int|float $read_timeout = 0, bool $persistent = false, #[\SensitiveParameter] mixed $auth = null, ?array $context = null);

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

    /**
     * @see Redis::acl
     */
    public function acl(string|array $key_or_address, string $subcmd, string ...$args): mixed;

    /**
     * @see Redis::append()
     */
    public function append(string $key, mixed $value): RedisCluster|bool|int;

    /**
     * @see Redis::bgrewriteaof
     */
    public function bgrewriteaof(string|array $key_or_address): RedisCluster|bool;

    /**
     * @see Redis::bgsave
     */
    public function bgsave(string|array $key_or_address): RedisCluster|bool;

    /**
     * @see Redis::bitcount
     */
    public function bitcount(string $key, int $start = 0, int $end = -1, bool $bybit = false): RedisCluster|bool|int;

    /**
     * @see Redis::bitop
     */
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

    /**
     * Move an element from one list into another.
     *
     * @see Redis::lmove
     */
    public function lmove(string $src, string $dst, string $wherefrom, string $whereto): Redis|string|false;

    /**
     * Move an element from one list to another, blocking up to a timeout until an element is available.
     *
     * @see Redis::blmove
     *
     */
    public function blmove(string $src, string $dst, string $wherefrom, string $whereto, float $timeout): Redis|string|false;

    /**
     * @see Redis::bzpopmax
     */
    public function bzpopmax(string|array $key, string|int $timeout_or_key, mixed ...$extra_args): array;

    /**
     * @see Redis::bzpopmin
     */
    public function bzpopmin(string|array $key, string|int $timeout_or_key, mixed ...$extra_args): array;

    /**
     * @see Redis::bzmpop
     */
    public function bzmpop(float $timeout, array $keys, string $from, int $count = 1): RedisCluster|array|null|false;

    /**
     * @see Redis::zmpop
     */
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

    /**
     * @see Redis::client
     */
    public function client(string|array $key_or_address, string $subcommand, ?string $arg = null): array|string|bool;

    /**
     * @see Redis::close
     */
    public function close(): bool;

    /**
     * @see Redis::cluster
     */
    public function cluster(string|array $key_or_address, string $command, mixed ...$extra_args): mixed;

    /**
     * @see Redis::command
     */
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
     * @see https://redis.io/commands/copy
     */
    public function copy(string $src, string $dst, ?array $options = null): RedisCluster|bool;

    /**
     * @see Redis::decr()
     */
    public function decr(string $key, int $by = 1): RedisCluster|int|false;

    /**
     * @see Redis::decrby()
     */
    public function decrby(string $key, int $value): RedisCluster|int|false;

    /**
     * @see Redis::decrbyfloat
     */
    public function decrbyfloat(string $key, float $value): float;

    /**
     * @see Redis::del()
     */
    public function del(array|string $key, string ...$other_keys): RedisCluster|int|false;

    /**
     * @see Redis::discard
     */
    public function discard(): bool;

    /**
     * @see Redis::dump
     */
    public function dump(string $key): RedisCluster|string|false;

    /**
     * @see Redis::echo()
     */
    public function echo(string|array $key_or_address, string $msg): RedisCluster|string|false;

    /**
     * @see Redis::eval
     */
    public function eval(string $script, array $args = [], int $num_keys = 0): mixed;

    /**
     * @see Redis::eval_ro
     */
    public function eval_ro(string $script, array $args = [], int $num_keys = 0): mixed;

    /**
     * @see Redis::evalsha
     */
    public function evalsha(string $script_sha, array $args = [], int $num_keys = 0): mixed;

    /**
     * @see Redis::evalsha_ro
     */
    public function evalsha_ro(string $script_sha, array $args = [], int $num_keys = 0): mixed;

    /**
     * @see Redis::exec()
     */
    public function exec(): array|false;

    /**
     * @see Redis::exists
     */
    public function exists(mixed $key, mixed ...$other_keys): RedisCluster|int|bool;

    /**
     * @see Redis::touch()
     */
    public function touch(mixed $key, mixed ...$other_keys): RedisCluster|int|bool;

    /**
     * @see Redis::expire
     */
    public function expire(string $key, int $timeout, ?string $mode = null): RedisCluster|bool;

    /**
     * @see Redis::expireat
     */
    public function expireat(string $key, int $timestamp, ?string $mode = null): RedisCluster|bool;

    /**
     * @see Redis::expiretime()
     */
    public function expiretime(string $key): RedisCluster|int|false;

    /**
     * @see Redis::pexpiretime()
     */
    public function pexpiretime(string $key): RedisCluster|int|false;

    /**
     * @see Redis::flushall
     */
    public function flushall(string|array $key_or_address, bool $async = false): RedisCluster|bool;

    /**
     * @see Redis::flushdb
     */
    public function flushdb(string|array $key_or_address, bool $async = false): RedisCluster|bool;

    /**
     * @see Redis::geoadd
     */
    public function geoadd(string $key, float $lng, float $lat, string $member, mixed ...$other_triples_and_options): RedisCluster|int|false;

    /**
     * @see Redis::geodist
     */
    public function geodist(string $key, string $src, string $dest, ?string $unit = null): RedisCluster|float|false;

    /**
     * @see Redis::geohash
     */
    public function geohash(string $key, string $member, string ...$other_members): RedisCluster|array|false;

    /**
     * @see Redis::geopos
     */
    public function geopos(string $key, string $member, string ...$other_members): RedisCluster|array|false;

    /**
     * @see Redis::georadius
     */
    public function georadius(string $key, float $lng, float $lat, float $radius, string $unit, array $options = []): mixed;

    /**
     * @see Redis::georadius_ro
     */
    public function georadius_ro(string $key, float $lng, float $lat, float $radius, string $unit, array $options = []): mixed;

    /**
     * @see Redis::georadiusbymember
     */
    public function georadiusbymember(string $key, string $member, float $radius, string $unit, array $options = []): mixed;

    /**
     * @see Redis::georadiusbymember_ro
     */
    public function georadiusbymember_ro(string $key, string $member, float $radius, string $unit, array $options = []): mixed;

    /**
     * @see https://redis.io/commands/geosearch
     */
    public function geosearch(string $key, array|string $position, array|int|float $shape, string $unit, array $options = []): RedisCluster|array;

    /**
     * @see https://redis.io/commands/geosearchstore
     */
    public function geosearchstore(string $dst, string $src, array|string $position, array|int|float $shape, string $unit, array $options = []): RedisCluster|array|int|false;

    /**
     * @see Redis::get
     */
    public function get(string $key): mixed;

    /**
     * @see Redis::getbit
     */
    public function getbit(string $key, int $value): RedisCluster|int|false;

    /**
     * @see Redis::getlasterror
     */
    public function getlasterror(): string|null;

    /**
     * @see Redis::getmode
     */
    public function getmode(): int;

    /**
     * @see Redis::getoption
     */
    public function getoption(int $option): mixed;

    /**
     * @see Redis::getrange
     */
    public function getrange(string $key, int $start, int $end): RedisCluster|string|false;

    /**
     * @see Redis::lcs
     */
    public function lcs(string $key1, string $key2, ?array $options = null): RedisCluster|string|array|int|false;

    /**
     * @see Redis::getset
     */
    public function getset(string $key, mixed $value): RedisCluster|string|bool;

    /**
     * @see Redis::gettransferredbytes
     */
    public function gettransferredbytes(): array|false;

    /**
     * @see Redis::cleartransferredbytes
     */
    public function cleartransferredbytes(): void;

    /**
     * @see Redis::hdel
     */
    public function hdel(string $key, string $member, string ...$other_members): RedisCluster|int|false;

    /**
     * @see Redis::hexists
     */
    public function hexists(string $key, string $member): RedisCluster|bool;

    /**
     * @see Redis::hget
     */
    public function hget(string $key, string $member): mixed;

    /**
     * @see Redis::hgetall
     */
    public function hgetall(string $key): RedisCluster|array|false;

    /**
     * @see Redis::hincrby
     */
    public function hincrby(string $key, string $member, int $value): RedisCluster|int|false;

    /**
     * @see Redis::hincrbyfloat
     */
    public function hincrbyfloat(string $key, string $member, float $value): RedisCluster|float|false;

    /**
     * @see Redis::hkeys
     */
    public function hkeys(string $key): RedisCluster|array|false;

    /**
     * @see Redis::hlen
     */
    public function hlen(string $key): RedisCluster|int|false;

    /**
     * @see Redis::hmget
     */
    public function hmget(string $key, array $keys): RedisCluster|array|false;

    /**
     * @see Redis::hmset
     */
    public function hmset(string $key, array $key_values): RedisCluster|bool;

    /**
     * @see Redis::hscan
     */
    public function hscan(string $key, ?int &$iterator, ?string $pattern = null, int $count = 0): array|bool;

    /**
     * @see https://redis.io/commands/hrandfield
     */
    public function hrandfield(string $key, ?array $options = null): RedisCluster|string|array;

    /**
     * @see Redis::hset
     */
    public function hset(string $key, string $member, mixed $value): RedisCluster|int|false;

    /**
     * @see Redis::hsetnx
     */
    public function hsetnx(string $key, string $member, mixed $value): RedisCluster|bool;

    /**
     * @see Redis::hstrlen
     */
    public function hstrlen(string $key, string $field): RedisCluster|int|false;

    /**
     * @see Redis::hvals
     */
    public function hvals(string $key): RedisCluster|array|false;

    /**
     * @see Redis::incr
     */
    public function incr(string $key, int $by = 1): RedisCluster|int|false;

    /**
     * @see Redis::incrby
     */
    public function incrby(string $key, int $value): RedisCluster|int|false;

    /**
     * @see Redis::incrbyfloat
     */
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
     * @return RedisCluster|array|false
     */
    public function info(string|array $key_or_address, string ...$sections): RedisCluster|array|false;

    /**
     * @see Redis::keys
     */
    public function keys(string $pattern): RedisCluster|array|false;

    /**
     * @see Redis::lastsave
     */
    public function lastsave(string|array $key_or_address): RedisCluster|int|false;

    /**
     * @see Redis::lget
     */
    public function lget(string $key, int $index): RedisCluster|string|bool;

    /**
     * @see Redis::lindex
     */
    public function lindex(string $key, int $index): mixed;

    /**
     * @see Redis::linsert
     */
    public function linsert(string $key, string $pos, mixed $pivot, mixed $value): RedisCluster|int|false;

    /**
     * @see Redis::llen
     */
    public function llen(string $key): RedisCluster|int|bool;

    /**
     * @see Redis::lpop
     */
    public function lpop(string $key, int $count = 0): RedisCluster|bool|string|array;

    /**
     * @see Redis::lpos
     */
    public function lpos(string $key, mixed $value, ?array $options = null): Redis|null|bool|int|array;

    /**
     * @see Redis::lpush
     */
    public function lpush(string $key, mixed $value, mixed ...$other_values): RedisCluster|int|bool;

    /**
     * @see Redis::lpushx
     */
    public function lpushx(string $key, mixed $value): RedisCluster|int|bool;

    /**
     * @see Redis::lrange
     */
    public function lrange(string $key, int $start, int $end): RedisCluster|array|false;

    /**
     * @see Redis::lrem
     */
    public function lrem(string $key, mixed $value, int $count = 0): RedisCluster|int|bool;

    /**
     * @see Redis::lset
     */
    public function lset(string $key, int $index, mixed $value): RedisCluster|bool;

    /**
     * @see Redis::ltrim
     */
    public function ltrim(string $key, int $start, int $end): RedisCluster|bool;

    /**
     * @see Redis::mget
     */
    public function mget(array $keys): RedisCluster|array|false;

    /**
     * @see Redis::mset
     */
    public function mset(array $key_values): RedisCluster|bool;

    /**
     * @see Redis::msetnx
     */
    public function msetnx(array $key_values): RedisCluster|array|false;

    /* We only support Redis::MULTI in RedisCluster but take the argument
       so we can test MULTI..EXEC with RedisTest.php and in the event
       we add pipeline support in the future. */
    public function multi(int $value = Redis::MULTI): RedisCluster|bool;

    /**
     * @see Redis::object
     */
    public function object(string $subcommand, string $key): RedisCluster|int|string|false;

    /**
     * @see Redis::persist
     */
    public function persist(string $key): RedisCluster|bool;

    /**
     * @see Redis::pexpire
     */
    public function pexpire(string $key, int $timeout, ?string $mode = null): RedisCluster|bool;

    /**
     * @see Redis::pexpireat
     */
    public function pexpireat(string $key, int $timestamp, ?string $mode = null): RedisCluster|bool;


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
    public function ping(string|array $key_or_address, ?string $message = null): mixed;

    /**
     * @see Redis::psetex
     */
    public function psetex(string $key, int $timeout, string $value): RedisCluster|bool;

    /**
     * @see Redis::psubscribe
     */
    public function psubscribe(array $patterns, callable $callback): void;

    /**
     * @see Redis::pttl
     */
    public function pttl(string $key): RedisCluster|int|false;

    /**
     * @see Redis::publish
     */
    public function publish(string $channel, string $message): RedisCluster|bool;

    /**
     * @see Redis::pubsub
     */
    public function pubsub(string|array $key_or_address, string ...$values): mixed;

    /**
     * @see Redis::punsubscribe
     */
    public function punsubscribe(string $pattern, string ...$other_patterns): bool|array;

    /**
     * @see Redis::randomkey
     */
    public function randomkey(string|array $key_or_address): RedisCluster|bool|string;

    /**
     * @see Redis::rawcommand
     */
    public function rawcommand(string|array $key_or_address, string $command, mixed ...$args): mixed;

    /**
     * @see Redis::rename
     */
    public function rename(string $key_src, string $key_dst): RedisCluster|bool;

    /**
     * @see Redis::renamenx
     */
    public function renamenx(string $key, string $newkey): RedisCluster|bool;

    /**
     * @see Redis::restore
     */
    public function restore(string $key, int $timeout, string $value, ?array $options = null): RedisCluster|bool;

    /**
     * @see Redis::role
     */
    public function role(string|array $key_or_address): mixed;

    /**
     * @see Redis::rpop()
     */
    public function rpop(string $key, int $count = 0): RedisCluster|bool|string|array;

    /**
     * @see Redis::rpoplpush()
     */
    public function rpoplpush(string $src, string $dst): RedisCluster|bool|string;

    /**
     * @see Redis::rpush
     */
    public function rpush(string $key, mixed ...$elements): RedisCluster|int|false;

    /**
     * @see Redis::rpushx
     */
    public function rpushx(string $key, string $value): RedisCluster|bool|int;

    /**
     * @see Redis::sadd()
     */
    public function sadd(string $key, mixed $value, mixed ...$other_values): RedisCluster|int|false;

    /**
     * @see Redis::saddarray()
     */
    public function saddarray(string $key, array $values): RedisCluster|bool|int;

    /**
     * @see Redis::save
     */
    public function save(string|array $key_or_address): RedisCluster|bool;

    /**
     * @see Redis::scan
     */
    public function scan(?int &$iterator, string|array $key_or_address, ?string $pattern = null, int $count = 0): bool|array;

    /**
     * @see Redis::scard
     */
    public function scard(string $key): RedisCluster|int|false;

    /**
     * @see Redis::script
     */
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
    public function set(string $key, mixed $value, mixed $options = null): RedisCluster|string|bool;

    /**
     * @see Redis::setbit
     */
    public function setbit(string $key, int $offset, bool $onoff): RedisCluster|int|false;

    /**
     * @see Redis::setex
     */
    public function setex(string $key, int $expire, mixed $value): RedisCluster|bool;

    /**
     * @see Redis::setnx
     */
    public function setnx(string $key, mixed $value): RedisCluster|bool;

    /**
     * @see Redis::setoption
     */
    public function setoption(int $option, mixed $value): bool;

    /**
     * @see Redis::setrange
     */
    public function setrange(string $key, int $offset, string $value): RedisCluster|int|false;

    /**
     * @see Redis::sinter()
     */
    public function sinter(array|string $key, string ...$other_keys): RedisCluster|array|false;

    /**
     * @see Redis::sintercard
     */
    public function sintercard(array $keys, int $limit = -1): RedisCluster|int|false;

    /**
     * @see Redis::sinterstore()
     */
    public function sinterstore(array|string $key, string ...$other_keys): RedisCluster|int|false;

    /**
     * @see Redis::sismember
     */
    public function sismember(string $key, mixed $value): RedisCluster|bool;

    /**
     * @see Redis::smismember
     */
    public function smismember(string $key, string $member, string ...$other_members): RedisCluster|array|false;

    /**
     * @see Redis::slowlog
     */
    public function slowlog(string|array $key_or_address, mixed ...$args): mixed;

    /**
     * @see Redis::smembers()
     */
    public function smembers(string $key): RedisCluster|array|false;

    /**
     * @see Redis::smove()
     */
    public function smove(string $src, string $dst, string $member): RedisCluster|bool;

    /**
     * @see Redis::sort()
     */
    public function sort(string $key, ?array $options = null): RedisCluster|array|bool|int|string;

    /**
     * @see Redis::sort_ro()
     */
    public function sort_ro(string $key, ?array $options = null): RedisCluster|array|bool|int|string;

    /**
     * @see Redis::spop
     */
    public function spop(string $key, int $count = 0): RedisCluster|string|array|false;

    /**
     * @see Redis::srandmember
     */
    public function srandmember(string $key, int $count = 0): RedisCluster|string|array|false;

    /**
     * @see Redis::srem
     */
    public function srem(string $key, mixed $value, mixed ...$other_values): RedisCluster|int|false;

    /**
     * @see Redis::sscan
     */
    public function sscan(string $key, ?int &$iterator, ?string $pattern = null, int $count = 0): array|false;

    /**
     * @see Redis::strlen
     */
    public function strlen(string $key): RedisCluster|int|false;

    /**
     * @see Redis::subscribe
     */
    public function subscribe(array $channels, callable $cb): void;

    /**
     * @see Redis::sunion()
     */
    public function sunion(string $key, string ...$other_keys): RedisCluster|bool|array;

    /**
     * @see Redis::sunionstore()
     */
    public function sunionstore(string $dst, string $key, string ...$other_keys): RedisCluster|int|false;

    /**
     * @see Redis::time
     */
    public function time(string|array $key_or_address): RedisCluster|bool|array;

    /**
     * @see Redis::ttl
     */
    public function ttl(string $key): RedisCluster|int|false;

    /**
     * @see Redis::type
     */
    public function type(string $key): RedisCluster|int|false;

    /**
     * @see Redis::unsubscribe
     */
    public function unsubscribe(array $channels): bool|array;

    /**
     * @see Redis::unlink
     */
    public function unlink(array|string $key, string ...$other_keys): RedisCluster|int|false;

    /**
     * @see Redis::unwatch
     */
    public function unwatch(): bool;

    /**
     * @see Redis::watch
     */
    public function watch(string $key, string ...$other_keys): RedisCluster|bool;

    /**
     * @see Redis::xack
     */
    public function xack(string $key, string $group, array $ids): RedisCluster|int|false;

    /**
     * @see Redis::xadd
     */
    public function xadd(string $key, string $id, array $values, int $maxlen = 0, bool $approx = false): RedisCluster|string|false;

    /**
     * @see Redis::xclaim
     */
    public function xclaim(string $key, string $group, string $consumer, int $min_iddle, array $ids, array $options): RedisCluster|string|array|false;

    /**
     * @see Redis::xdel
     */
    public function xdel(string $key, array $ids): RedisCluster|int|false;

    /**
     * @see Redis::xgroup
     */
    public function xgroup(string $operation, ?string $key = null, ?string $group = null, ?string $id_or_consumer = null,
                           bool $mkstream = false, int $entries_read = -2): mixed;

    /**
     * @see Redis::xautoclaim
     */
    public function xautoclaim(string $key, string $group, string $consumer, int $min_idle, string $start, int $count = -1, bool $justid = false): RedisCluster|bool|array;

    /**
     * @see Redis::xinfo
     */
    public function xinfo(string $operation, ?string $arg1 = null, ?string $arg2 = null, int $count = -1): mixed;

    /**
     * @see Redis::xlen
     */
    public function xlen(string $key): RedisCluster|int|false;

    /**
     * @see Redis::xpending
     */
    public function xpending(string $key, string $group, ?string $start = null, ?string $end = null, int $count = -1, ?string $consumer = null): RedisCluster|array|false;

    /**
     * @see Redis::xrange
     */
    public function xrange(string $key, string $start, string $end, int $count = -1): RedisCluster|bool|array;

    /**
     * @see Redis::xread
     */
    public function xread(array $streams, int $count = -1, int $block = -1): RedisCluster|bool|array;

    /**
     * @see Redis::xreadgroup
     */
    public function xreadgroup(string $group, string $consumer, array $streams, int $count = 1, int $block = 1): RedisCluster|bool|array;

    /**
     * @see Redis::xrevrange
     */
    public function xrevrange(string $key, string $start, string $end, int $count = -1): RedisCluster|bool|array;

    /**
     * @see Redis::xtrim
     */
    public function xtrim(string $key, int $maxlen, bool $approx = false, bool $minid = false, int $limit = -1): RedisCluster|int|false;

    /**
     * @see Redis::zadd
     */
    public function zadd(string $key, array|float $score_or_options, mixed ...$more_scores_and_mems): RedisCluster|int|float|false;

    /**
     * @see Redis::zcard
     */
    public function zcard(string $key): RedisCluster|int|false;

    /**
     * @see Redis::zcount
     */
    public function zcount(string $key, string $start, string $end): RedisCluster|int|false;

    /**
     * @see Redis::zincrby
     */
    public function zincrby(string $key, float $value, string $member): RedisCluster|float|false;

    /**
     * @see Redis::zinterstore
     */
    public function zinterstore(string $dst, array $keys, ?array $weights = null, ?string $aggregate = null): RedisCluster|int|false;

    /**
     * @see Redis::zintercard
     */
    public function zintercard(array $keys, int $limit = -1): RedisCluster|int|false;

    /**
     * @see Redis::zlexcount
     */
    public function zlexcount(string $key, string $min, string $max): RedisCluster|int|false;

    /**
     * @see Redis::zpopmax
     */
    public function zpopmax(string $key, ?int $value = null): RedisCluster|bool|array;

    /**
     * @see Redis::zpopmin
     */
    public function zpopmin(string $key, ?int $value = null): RedisCluster|bool|array;

    /**
     * @see Redis::zrange
     */
    public function zrange(string $key, mixed $start, mixed $end, array|bool|null $options = null): RedisCluster|array|bool;

    /**
     * @see Redis::zrangestore
     */
    public function zrangestore(string $dstkey, string $srckey, int $start, int $end,
                                array|bool|null $options = null): RedisCluster|int|false;

    /**
     * @see https://redis.io/commands/zRandMember
     */
    public function zrandmember(string $key, ?array $options = null): RedisCluster|string|array;

    /**
     * @see Redis::zrangebylex
     */
    public function zrangebylex(string $key, string $min, string $max, int $offset = -1, int $count = -1): RedisCluster|array|false;

    /**
     * @see Redis::zrangebyscore
     */
    public function zrangebyscore(string $key, string $start, string $end, array $options = []): RedisCluster|array|false;

    /**
     * @see Redis::zrank
     */
    public function zrank(string $key, mixed $member): RedisCluster|int|false;

    /**
     * @see Redis::zrem
     */
    public function zrem(string $key, string $value, string ...$other_values): RedisCluster|int|false;

    /**
     * @see Redis::zremrangebylex
     */
    public function zremrangebylex(string $key, string $min, string $max): RedisCluster|int|false;

    /**
     * @see Redis::zremrangebyrank
     */
    public function zremrangebyrank(string $key, string $min, string $max): RedisCluster|int|false;

    /**
     * @see Redis::zremrangebyscore
     */
    public function zremrangebyscore(string $key, string $min, string $max): RedisCluster|int|false;

    /**
     * @see Redis::zrevrange
     */
    public function zrevrange(string $key, string $min, string $max, ?array $options = null): RedisCluster|bool|array;

    /**
     * @see Redis::zrevrangebylex
     */
    public function zrevrangebylex(string $key, string $min, string $max, ?array $options = null): RedisCluster|bool|array;

    /**
     * @see Redis::zrevrangebyscore
     */
    public function zrevrangebyscore(string $key, string $min, string $max, ?array $options = null): RedisCluster|bool|array;

    /**
     * @see Redis::zrevrank
     */
    public function zrevrank(string $key, mixed $member): RedisCluster|int|false;

    /**
     * @see Redis::zscan
     */
    public function zscan(string $key, ?int &$iterator, ?string $pattern = null, int $count = 0): RedisCluster|bool|array;

    /**
     * @see Redis::zscore
     */
    public function zscore(string $key, mixed $member): RedisCluster|float|false;

    /**
     * @see https://redis.io/commands/zMscore
     */
    public function zmscore(string $key, mixed $member, mixed ...$other_members): Redis|array|false;

    /**
     * @see Redis::zunionstore
     */
    public function zunionstore(string $dst, array $keys, ?array $weights = null, ?string $aggregate = null): RedisCluster|int|false;

    /**
     * @see https://redis.io/commands/zinter
     */
    public function zinter(array $keys, ?array $weights = null, ?array $options = null): RedisCluster|array|false;

    /**
     * @see https://redis.io/commands/zdiffstore
     */
    public function zdiffstore(string $dst, array $keys): RedisCluster|int|false;

    /**
     * @see https://redis.io/commands/zunion
     */
    public function zunion(array $keys, ?array $weights = null, ?array $options = null): RedisCluster|array|false;

    /**
     * @see https://redis.io/commands/zdiff
     */
    public function zdiff(array $keys, ?array $options = null): RedisCluster|array|false;
}

class RedisClusterException extends RuntimeException {}
