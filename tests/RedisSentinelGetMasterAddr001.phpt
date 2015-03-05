--TEST--
Test RedisSentinel::getMasterAddr
--SKIPIF--
<?php if (!extension_loaded("redis")) print "skip"; ?>
--FILE--
<?php

$sentinel = new RedisSentinel('127.0.0.1');

var_dump($sentinel->connect());

$master = $sentinel->getMasterAddr();

assert(is_array($master));
assert(count($master) === 2);

var_dump($master);

?>
--EXPECT--
bool(true)
array(2) {
  [0]=>
  string(9) "127.0.0.1"
  [1]=>
  string(4) "6379"
}
