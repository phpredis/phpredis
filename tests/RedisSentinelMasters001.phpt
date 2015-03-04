--TEST--
Test RedisSentinel::masters
--SKIPIF--
<?php if (!extension_loaded("redis")) print "skip"; ?>
--FILE--
<?php

$sentinel = new RedisSentinel('127.0.0.1');

var_dump($sentinel->connect());

$masters = $sentinel->masters();

var_dump(count($masters));

assert(isset($masters[0]['name']));

?>
--EXPECT--
bool(true)
int(1)
