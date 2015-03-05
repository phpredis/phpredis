--TEST--
Test Redis::role
--SKIPIF--
<?php if (!extension_loaded("redis")) print "skip"; ?>
--FILE--
<?php

$redis = new Redis();

var_dump($redis->connect('127.0.0.1'));

$role = $redis->role();

assert(is_array($role));
assert(count($role) === 3);
assert($role[0] === "master");

?>
--EXPECT--
bool(true)
