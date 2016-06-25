--TEST--
Test RedisSentinel::slaves with not exists master name
--SKIPIF--
<?php if (!extension_loaded("redis")) print "skip"; ?>
--FILE--
<?php

$sentinel = new RedisSentinel('127.0.0.1');

var_dump($sentinel->connect());

$slaves = $sentinel->slaves('mymaster1');

assert(is_bool($slaves));
assert($slaves === false);

?>
--EXPECT--
bool(true)

