PhpRedis
=============

### Introduction

### Installing/Configuring

* Requirements
* Installation
* Runtime Configuration
* Resource Types

### Predefined Constants

### Examples

* overview example
	
### PhpRedis Functions

#### Redis::get
* *Description*

	Get the value related to the specified key

* *Parameters*

	key : key

* *Return Value*

	If key didn't exist, FALSE is returned. Else, the value related to this key is returned.

* *Examples*

	$redis->get('key');

#### Redis::set
#### Redis::setnx
#### Redis::del
#### Redis::exists
#### Redis::incr
#### Redis::decr
#### Redis::mget
#### Redis::rpush
#### Redis::lpush
#### Redis::rpop
#### Redis::lpop
#### Redis::llen
#### Redis::lindex
#### Redis::lset
#### Redis::lrange
#### Redis::ltrim
#### Redis::lrem
#### Redis::sadd
#### Redis::srem
#### Redis::smove
#### Redis::sismember
#### Redis::scard
#### Redis::spop
#### Redis::sinter

* *Description*

	Return the members of a set resulting from the intersection of all the sets hold at the specified keys.
	if just a single key is specified, then this command produces the members of this set. If one of the keys
	is missing, FALSE is returned.

* *Parameters*

	key1, key2, keyN : keys identifying the different set on which we will apply the intersection.
		
* *Return value*

	Array, contain the result of the intersesction between those keys. If the Intersection beteen the different set is empty, the return value will be empty array.

* *Examples*

	<pre>
	$redis = new Redis();
	$redis->connect('127.0.0.1', 6379);
	$redis->sadd('key1', 'val1');
	$redis->sadd('key1', 'val2');
	$redis->sadd('key1', 'val3');
	$redis->sadd('key1', 'val4');

	$redis->sadd('key2', 'val3');
	$redis->sadd('key2', 'val4');

	$redis->sadd('key3', 'val3');
	$redis->sadd('key3', 'val4');

	var_dump($redis->sinter('key1', 'key2', 'key3'));
	</pre>
	the output :
	<pre>
	array(2) {
	  [0]=>
	  string(4) "val4"
	  [1]=>
	  string(4) "val3"
	}
	</pre>

#### Redis::sinterstore
#### Redis::sunion
#### Redis::sunionstore
#### Redis::sdiff
#### Redis::sdiffstore
#### Redis::smembers
#### Redis::incrby
#### Redis::decrby
#### Redis::getset
#### Redis::randomkey
#### Redis::select
#### Redis::move
#### Redis::rename
#### Redis::renamenx
#### Redis::expire
#### Redis::keys
#### Redis::dbsize
#### Redis::auth
#### Redis::save
#### Redis::bgsave
#### Redis::lastsave
#### Redis::type
#### Redis::flushdb
#### Redis::flushall
#### Redis::sort
#### Redis::info
#### Redis::ttl
