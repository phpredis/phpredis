--TEST--
Test RedisSentinel::__construct with bad timeout
--SKIPIF--
<?php if (!extension_loaded("redis")) print "skip"; ?>
--FILE--
<?php

try {
    $sentinel = new RedisSentinel('127.0.0.1', 1337, -1);
} catch (RedisException $ex) {
    var_dump($ex->getMessage());
}

?>
--EXPECT--
string(15) "Invalid timeout"
