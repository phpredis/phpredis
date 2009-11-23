PhpRedis
=============


Installing/Configuring
======================

Functions
=========


## get
##### *Description*

	Get the value related to the specified key

##### *Parameters*

	key : key

##### *Return Value*

	If key didn't exist, FALSE is returned. Else, the value related to this key is returned.

##### *Examples*

	$redis->get('key');

## set
## setnx
## del
## exists
## incr
## decr
## mget
## rpush
## lpush
## rpop
## lpop
## llen
## lindex
## lset
## lrange
## ltrim
## lrem
## sadd
## srem
## smove
## sismember
## scard
## spop
## sinter

##### *Description*

	Return the members of a set resulting from the intersection of all the sets hold at the specified keys.
	if just a single key is specified, then this command produces the members of this set. If one of the keys
	is missing, FALSE is returned.

##### *Parameters*

	key1, key2, keyN : keys identifying the different set on which we will apply the intersection.
		
##### *Return value*

	Array, contain the result of the intersesction between those keys. If the Intersection beteen the different set is empty, the return value will be empty array.

##### *Examples*

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

## sinterstore
## sunion
## sunionstore
## sdiff
## sdiffstore
## smembers
## incrby
## decrby
## getset
## randomkey
## select
## move
## rename
## renamenx
## expire
## keys
## dbsize
## auth
## save
## bgsave
## lastsave
## type
## flushdb
## flushall
## sort
## info
## ttl
