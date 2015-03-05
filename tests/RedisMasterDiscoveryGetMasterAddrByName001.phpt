--TEST--
Test RedisMasterDiscovery::getMasterAddrNyName with first sentinel connected
--SKIPIF--
<?php if (!extension_loaded("redis")) print "skip"; ?>
--FILE--
<?php

$masterDiscovery = new RedisMasterDiscovery();

$masterDiscovery->addSentinel(new RedisSentinel('127.0.0.1', 26379));
$masterDiscovery->addSentinel(new RedisSentinel('192.168.50.41', 26379));

var_dump($masterDiscovery->getMasterAddrByName('mymaster'));

?>
--EXPECT--
array(2) {
  [0]=>
  string(9) "127.0.0.1"
  [1]=>
  string(4) "6379"
}
