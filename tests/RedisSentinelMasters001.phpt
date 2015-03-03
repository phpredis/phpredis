--TEST--
Test RedisSentinel::masters
--SKIPIF--
<?php if (!extension_loaded("redis")) print "skip"; ?>
--FILE--
<?php

$sentinel = new RedisSentinel('127.0.0.1');

var_dump($sentinel->connect());

var_dump(count($sentinel->masters()));

?>
--EXPECT--
bool(true)
int(1)
