--TEST--
Test RedisMasterDiscovery::getMasterAddr with all sentinels disconnected
--SKIPIF--
<?php if (!extension_loaded("redis")) print "skip"; ?>
--FILE--
<?php

$masterDiscovery = new RedisMasterDiscovery();

$masterDiscovery->addSentinel(new RedisSentinel('192.168.50.41', 26379, 0.5));
$masterDiscovery->addSentinel(new RedisSentinel('192.168.50.42', 26379, 0.5));

try {
    $masterDiscovery->getMasterAddr();
} catch (RedisException $ex) {
    var_dump($ex->getMessage());
}

?>
--EXPECT--
string(29) "All sentinels are unreachable"
