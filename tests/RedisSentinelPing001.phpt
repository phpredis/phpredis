--TEST--
Test RedisSentinel::ping
--SKIPIF--
<?php if (!extension_loaded("redis")) print "skip"; ?>
--FILE--
<?php

$sentinel = new RedisSentinel('127.0.0.1');

var_dump($sentinel->connect());

var_dump($sentinel->ping());

?>
--EXPECT--
bool(true)
string(5) "+PONG"
