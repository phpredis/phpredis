--TEST--
Test RedisSentinel::master
--SKIPIF--
<?php if (!extension_loaded("redis")) print "skip"; ?>
--FILE--
<?php

$sentinel = new RedisSentinel('127.0.0.1');

var_dump($sentinel->connect());

$master = $sentinel->master('mymaster');

assert(isset($master['name']));
assert($master['name'] == 'mymaster');
assert(isset($master['ip']));
assert(isset($master['port']));
assert(isset($master['flags']));

?>
--EXPECT--
bool(true)

