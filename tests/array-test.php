<?php

// can't set anything that hasn't been declared when the extension loads.
ini_set('redis.array.names', 'users,friends');
ini_set('redis.array.hosts', 'users=localhost:6379,localhost:6380,localhost:6381,localhost:6382');

// var_dump(ini_get('redis.arrays.names'));
// var_dump(ini_get('redis.arrays.hosts'));
// var_dump(ini_get('redis.arrays.functions'));

// different redis arrays
$ra = new RedisArray('users');
$ra = new RedisArray(array('localhost:6379', 'localhost:6380', 'localhost:6381'));
$ra = new RedisArray(array('localhost', 'localhost:6380', 'localhost:6381')/*, 'hash_key' */);

var_dump($ra->set('hello', 'world'));
var_dump($ra->get('hello'));
while(true) {
	($ra->set('hello', 'world'));
	($ra->get('hello'));
}

var_dump($ra->_hosts());
var_dump($ra->_target('a'));
var_dump($ra->_target('b'));
var_dump($ra->_target('c'));

$r0 = new Redis;
$r0->connect('127.0.0.1', 6379);
$r0->set('c', 'z');

$r1 = new Redis;
$r1->connect('127.0.0.1', 6380);
$r1->set('b', 'y');

$r2 = new Redis;
$r2->connect('127.0.0.1', 6381);
$r2->set('a', 'x');

var_dump(array('x', 'y', 'z') === $ra->mget(array('a', 'b', 'c')));
$ra->mset(array('a' => 'X', 'b' => 'Y', 'c' => 'Z'));
var_dump(array('X', 'Y', 'Z') === $ra->mget(array('a', 'b', 'c')));

$ra->del(array('a','c'));
$ra->del('a','c');

var_dump($ra->mget(array('a', 'b', 'c')));
?>
