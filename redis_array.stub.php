<?php

/**
 * @generate-function-entries
 * @generate-legacy-arginfo
 */

class RedisArray {

    public function __call(string $function_name, array $arguments): mixed;

    public function __construct(string|array $name_or_hosts, array $options = NULL);

    public function _continuum(): bool|array;

    public function _distributor(): bool|callable;

    public function _function(): bool|callable;

    public function _hosts(): bool|array;

    public function _instance(string $host): bool|null|Redis;

    public function _rehash(callable $fn = NULL): bool|null;

    public function _target(string $key): bool|string|null;

    public function bgsave(): array;

    public function del(string|array $key, string ...$otherkeys): bool|int;

    public function discard(): bool|null;

    public function exec(): bool|null;

    public function flushall(): bool|array;

    public function flushdb(): bool|array;

    public function getOption(int $opt): bool|array;

    public function info(): bool|array;

    public function keys(string $pattern): bool|array;

    public function mget(array $keys): bool|array;

    public function mset(array $pairs): bool;

    public function multi(string $host, int $mode = NULL): bool|RedisArray;

    public function ping(): bool|array;

    public function save(): bool|array;

    public function select(int $index): bool|array;

    public function setOption(int $opt, string $value): bool|array;

    public function unlink(string|array $key, string ...$otherkeys): bool|int;

    public function unwatch(): bool|null;

}
