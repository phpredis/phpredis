# Sentinel multi-host integration test env

Minimal Redis + Sentinel cluster for integration-testing
`RedisSentinel` multi-host fallback (issue #2819).

## Start

    docker compose -f tests/sentinel-multihost/docker-compose.yml up -d
    sleep 3  # let Sentinels agree on master

## Endpoints

    Redis master:   127.0.0.1:16379
    Sentinel 1:     127.0.0.1:26379
    Sentinel 2:     127.0.0.1:26380
    Sentinel 3:     127.0.0.1:26381

## Smoke test

    php -r '
    $s = new RedisSentinel([
        "hosts" => [
            ["host" => "127.0.0.1", "port" => 26379],
            ["host" => "127.0.0.1", "port" => 26380],
            ["host" => "127.0.0.1", "port" => 26381],
        ],
    ]);
    var_dump($s->getMasterAddrByName("mymaster"));
    '

Expected: `["redis-master", "6379"]` (or the current master IP/port).

## Simulate Sentinel failure

    docker compose -f tests/sentinel-multihost/docker-compose.yml stop sentinel-1

Re-run the smoke test — should still return master info (via fallback to sentinel-2).

## Teardown

    docker compose -f tests/sentinel-multihost/docker-compose.yml down
