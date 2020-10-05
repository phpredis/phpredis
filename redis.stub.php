<?php

/** @generate-function-entries */

class Redis {

    public function __construct();

    public function __destruct();

    public function connect(string $host, int $port = 26379, float $timeout = 0, string $persistent_id = NULL, int $retry_interval = 0, float $read_timeout = 0, array $context = NULL): bool;

    public function pconnect(string $host, int $port = 26379, float $timeout = 0, string $persistent_id = NULL, int $retry_interval = 0, float $read_timeout = 0, array $context = NULL): bool;

    /**
     * @param string $otherkeys 
     * @return int|Redis
     */
    public function bitop(string $operation, string $deskey, string $srckey, ...$otherkeys): int;

    /** @return int|Redis */
    public function bitcount(string $key, int $start = 0, int $end = -1);

    /** @return int|Redis */
    public function bitpos(string $key, int $bit, int $start = 0, int $end = -1);

    public function close(): bool;

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

    /**
     * @param string $args 
     * @return mixed|Redis
     */
    public function acl(string $subcmd, ...$args);

	/** @return int|Redis */
    public function append(string $key, mixed $value);

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
     * @param string $otherkeys 
     * @deprecated
     * @alias Redis::del
     * @return int|Redis
     */
    public function delete(array|string $key, ...$otherkeys);

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
