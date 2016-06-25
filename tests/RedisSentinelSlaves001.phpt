--TEST--
Test RedisSentinel::slaves
--SKIPIF--
<?php if (!extension_loaded("redis")) print "skip"; ?>
--FILE--
<?php

$sentinel = new RedisSentinel('127.0.0.1');

var_dump($sentinel->connect());

$slaves = $sentinel->slaves('mymaster');

assert(is_array($slaves));
assert(empty($slaves));

?>
--EXPECT--
bool(true)

