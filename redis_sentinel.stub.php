<?php

/**
 * @generate-function-entries
 * @generate-legacy-arginfo
 * @generate-class-entries
 */

class RedisSentinel {

    public function __construct(?array $options = null);

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
