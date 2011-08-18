<?php

// can't set anything that hasn't been declared when the extension loads.
ini_set('redis.array.names', 'users,friends');
ini_set('redis.array.hosts', 'users=localhost:6379,localhost:6380,localhost:6381,localhost:6382');

var_dump(ini_get('redis.arrays.names'));
var_dump(ini_get('redis.arrays.hosts'));
var_dump(ini_get('redis.arrays.functions'));

// different redis arrays
$ra = new RedisArray('users');
$ra = new RedisArray(array('localhost:6379', 'localhost:6380'));
$ra = new RedisArray(array('localhost', 'localhost:6380'), 'hash_key');

var_dump($ra->set('hello', 'world'));
var_dump($ra->get('hello'));

?>
