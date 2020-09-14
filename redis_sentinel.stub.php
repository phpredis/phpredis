<?php

/** @generate-function-entries */

class RedisSentinel {

    public function __construct(string $host, int $port = 26379, float $timeout = 0, mixed $persistent = NULL, int $retry_interval = 0, float $read_timeout = 0);

    public function ckquorum(string $master): bool;

    public function failover(string $master): bool;

    public function flushconfig(): bool;

    public function getMasterAddrByName(string $master): array|false;

    public function master(string $master): array|false;

    public function masters(): array|false;

    public function ping(): bool;

    public function reset(string $pattern): bool;

    public function sentinels(string $master): array|false;

    public function slaves(string $master): array|false;
}
