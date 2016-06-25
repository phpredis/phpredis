--TEST--
Test RedisSentinel::getMasterAddrByName with not exists master name
--SKIPIF--
<?php if (!extension_loaded("redis")) print "skip"; ?>
--FILE--
<?php

$sentinel = new RedisSentinel('127.0.0.1');

var_dump($sentinel->connect());

$master = $sentinel->getMasterAddrByName('mymaster1');

assert(is_array($master));
assert(empty($master));

?>
--EXPECT--
bool(true)
