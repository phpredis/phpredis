--TEST--
Test RedisMasterDiscovery::addSentinel
--SKIPIF--
<?php if (!extension_loaded("redis")) print "skip"; ?>
--FILE--
<?php

$masterDiscovery = new RedisMasterDiscovery();

var_dump(count($masterDiscovery->getSentinels()));

$masterDiscovery->addSentinel(new RedisSentinel('127.0.0.1', 26379));
$masterDiscovery->addSentinel(new RedisSentinel('192.168.50.41', 26379));

var_dump(count($masterDiscovery->getSentinels()));

?>
--EXPECT--
int(0)
int(2)
