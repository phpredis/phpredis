--TEST--
Test RedisMasterDiscovery::getMasterAddrByName without sentinels
--SKIPIF--
<?php if (!extension_loaded("redis")) print "skip"; ?>
--FILE--
<?php

$masterDiscovery = new RedisMasterDiscovery();

try {
    $masterDiscovery->getMasterAddrByName('mymaster');
} catch (RedisException $ex) {
    var_dump($ex->getMessage());
}

?>
--EXPECT--
string(66) "You need to add sentinel nodes before attempting to fetch a master"
