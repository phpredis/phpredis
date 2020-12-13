<?php

/**
 * @generate-function-entries
 * @generate-legacy-arginfo
 */

class RedisCluster {

    public function __construct(string|null $name, array $seeds = NULL, int|float $timeout = 0, int|float $read_timeout = 0, bool $persistant = false, mixed $auth = NULL, array $context = NULL);

    public function _masters(): array;

    public function _prefix(string $key): bool|string;

    public function _redir(): string|null;

    public function _serialize(mixed $value): bool|string;

    public function _unserialize(string $value): mixed;

    public function acl(string|array $key_or_address, string $subcmd, string ...$args): mixed;

    public function append(string $key, mixed $value): bool|int;

    public function bgrewriteaof(string|array $key_or_address): bool;

    public function bgsave(string|array $key_or_address): bool;

    public function bitcount(string $key, int $start = 0, int $end = -1): bool|int;

    public function bitop(string $operation, string $deskey, string $srckey, string ...$otherkeys): bool|int;

    public function bitpos(string $key, int $bit, int $start = NULL, int $end = NULL): bool|int;
}

/*
    TODO:
    public function brpop
    public function brpoplpush
    public function clearlasterror
    public function bzpopmax
    public function bzpopmin
    public function client
    public function close
    public function cluster
    public function command
    public function config
    public function dbsize
    public function decr
    public function decrby
    public function del
    public function discard
    public function dump
    public function echo
    public function eval
    public function evalsha
    public function exec
    public function exists
    public function expire
    public function expireat
    public function flushall
    public function flushdb
    public function geoadd
    public function geodist
    public function geohash
    public function geopos
    public function georadius
    public function georadius_ro
    public function georadiusbymember
    public function georadiusbymember_ro
    public function get
    public function getbit
    public function getlasterror
    public function getmode
    public function getoption
    public function getrange
    public function getset
    public function hdel
    public function hexists
    public function hget
    public function hgetall
    public function hincrby
    public function hincrbyfloat
    public function hkeys
    public function hlen
    public function hmget
    public function hmset
    public function hscan
    public function hset
    public function hsetnx
    public function hstrlen
    public function hvals
    public function incr
    public function incrby
    public function incrbyfloat
    public function info
    public function keys
    public function lastsave
    public function lget
    public function lindex
    public function linsert
    public function llen
    public function lpop
    public function lpush
    public function lpushx
    public function lrange
    public function lrem
    public function lset
    public function ltrim
    public function mget
    public function mset
    public function msetnx
    public function multi
    public function object
    public function persist
    public function pexpire
    public function pexpireat
    public function pfadd
    public function pfcount
    public function pfmerge
    public function ping
    public function psetex
    public function psubscribe
    public function pttl
    public function publish
    public function pubsub
    public function punsubscribe
    public function randomkey
    public function rawcommand
    public function rename
    public function renamenx
    public function restore
    public function role
    public function rpop
    public function rpoplpush
    public function rpush
    public function rpushx
    public function sadd
    public function saddarray
    public function save
    public function scan
    public function scard
    public function script
    public function sdiff
    public function sdiffstore
    public function set
    public function setbit
    public function setex
    public function setnx
    public function setoption
    public function setrange
    public function sinter
    public function sinterstore
    public function sismember
    public function slowlog
    public function smembers
    public function smove
    public function sort
    public function spop
    public function srandmember
    public function srem
    public function sscan
    public function strlen
    public function subscribe
    public function sunion
    public function sunionstore
    public function time
    public function ttl
    public function type
    public function unsubscribe
    public function unlink
    public function unwatch
    public function watch
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
    public function zadd
    public function zcard
    public function zcount
    public function zincrby
    public function zinterstore
    public function zlexcount
    public function zpopmax
    public function zpopmin
    public function zrange
    public function zrangebylex
    public function zrangebyscore
    public function zrank
    public function zrem
    public function zremrangebylex
    public function zremrangebyrank
    public function zremrangebyscore
    public function zrevrange
    public function zrevrangebylex
    public function zrevrangebyscore
    public function zrevrank
    public function zscan
    public function zscore
    public function zunionstore
*/
