--TEST--
Test RedisSentinel::reset with not exists pattern
--SKIPIF--
<?php if (!extension_loaded("redis")) print "skip"; ?>
--FILE--
<?php

$sentinel = new RedisSentinel('127.0.0.1');

var_dump($sentinel->connect());

$result = $sentinel->reset('mymaster1');

assert(is_int($result));
assert($result === 0);

?>
--EXPECT--
bool(true)
