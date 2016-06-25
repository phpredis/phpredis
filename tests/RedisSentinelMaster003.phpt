--TEST--
Test RedisSentinel::master with empty master name
--SKIPIF--
<?php if (!extension_loaded("redis")) print "skip"; ?>
--FILE--
<?php

$sentinel = new RedisSentinel('127.0.0.1');

var_dump($sentinel->connect());

$master = $sentinel->master('');

assert(is_bool($master));
assert($master === false);

?>
--EXPECT--
bool(true)

