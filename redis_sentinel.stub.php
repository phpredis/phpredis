<?php

/**
 * @generate-function-entries
 * @generate-legacy-arginfo
 * @generate-class-entries
 */

class RedisSentinel {

    /**
     * @param array|null $options Connection options. Accepts:
     *   - 'host'            (string, default '127.0.0.1') - single Sentinel host
     *   - 'port'            (int, default 26379)          - single Sentinel port
     *   - 'hosts'           (list<array{host:string,port?:int}>) - multiple Sentinel
     *                       hosts. When provided, 'host' and 'port' are ignored and
     *                       the client automatically falls back to the next host
     *                       on network failure. See issue #2819.
     *   - 'connectTimeout'  (float)
     *   - 'persistent'      (?string)
     *   - 'retryInterval'   (int)
     *   - 'readTimeout'     (float)
     *   - 'auth'            (string|array)
     *   - 'ssl'             (array)
     */
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
