<?php

/**
 * @generate-function-entries
 * @generate-legacy-arginfo
 * @generate-class-entries
 */

class RedisSentinel {

    public function __construct(string $host, int $port = 26379, float $timeout = 0, mixed $persistent = null, int $retry_interval = 0, float $read_timeout = 0, #[\SensitiveParameter] mixed $auth = null, array $context = null);

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

	/** @return int|RedisSentinel */
    public function reset(string $pattern);

	/** @return array|bool|RedisSentinel */
    public function sentinels(string $master);

	/** @return array|bool|RedisSentinel */
    public function slaves(string $master);
}
