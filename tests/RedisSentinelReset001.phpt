--TEST--
Test RedisSentinel::reset
--SKIPIF--
<?php if (!extension_loaded("redis")) print "skip"; ?>
--FILE--
<?php

$sentinel = new RedisSentinel('127.0.0.1');

var_dump($sentinel->connect());

$result = $sentinel->reset('mymaster');

assert(is_int($result));
assert($result === 1);

?>
--EXPECT--
bool(true)
