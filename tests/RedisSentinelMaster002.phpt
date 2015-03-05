--TEST--
Test RedisSentinel::master with not exists master name
--SKIPIF--
<?php if (!extension_loaded("redis")) print "skip"; ?>
--FILE--
<?php

$sentinel = new RedisSentinel('127.0.0.1');

var_dump($sentinel->connect());

$master = $sentinel->master('mymaster1');

assert(is_bool($master));
assert($master === false);

?>
--EXPECT--
bool(true)

