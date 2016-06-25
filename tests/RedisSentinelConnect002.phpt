--TEST--
Test RedisSentinel::connect fail
--SKIPIF--
<?php if (!extension_loaded("redis")) print "skip"; ?>
--FILE--
<?php

$sentinel = new RedisSentinel('127.0.0.1', 1337);

var_dump($sentinel->connect());

?>
--EXPECT--
bool(false)
