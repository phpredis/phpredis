<?php

/**
 * @generate-function-entries
 * @generate-legacy-arginfo
 */

class RedisSentinel {

    public function __construct(string $host, int $port = 26379, float $timeout = 0, mixed $persistent = NULL, int $retry_interval = 0, float $read_timeout = 0);

	/** @return bool|RedisSentinel */
    public function ckquorum(string $master);

	/** @return bool|RedisSentinel */
    public function failover(string $master);

	/** @return bool|RedisSentinel */
    public function flushconfig();

	/** @return array|bool|RedisSentinel */
    public function getMasterAddrByName(string $master);

	/** @return array|bool|RedisSentinel */
    public function master(string $master);

	/** @return array|bool|RedisSentinel */
    public function masters();

    public function myid(): string;

	/** @return bool|RedisSentinel */
    public function ping();

	/** @return bool|RedisSentinel */
    public function reset(string $pattern);

	/** @return array|bool|RedisSentinel */
    public function sentinels(string $master);

	/** @return array|bool|RedisSentinel */
    public function slaves(string $master);
}
