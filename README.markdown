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

* Redis::get
Get the value related to the specified key
**Parameters
key : key
** Return Value
If key didn't exist, False is returned. Else, the value related to this key is returned.
** Examples
$redis->get('key');


* Redis::set
* Redis::setnx
* Redis::del
* Redis::exists
* Redis::incr
* Redis::decr
* Redis::mget
* Redis::rpush
* Redis::lpush
* Redis::rpop
* Redis::lpop
* Redis::llen
* Redis::lindex
* Redis::lset
* Redis::lrange
* Redis::ltrim
* Redis::lrem
* Redis::sadd
* Redis::srem
* Redis::smove
* Redis::sismember
* Redis::scard
* Redis::spop
* Redis::sinter
* Redis::sinterstore
* Redis::sunion
* Redis::sunionstore
* Redis::sdiff
* Redis::sdiffstore
* Redis::smembers
* Redis::incrby
* Redis::decrby
* Redis::getset
* Redis::randomkey
* Redis::select
* Redis::move
* Redis::rename
* Redis::renamenx
* Redis::expire
* Redis::keys
* Redis::dbsize
* Redis::auth
* Redis::save
* Redis::bgsave
* Redis::lastsave
* Redis::type
* Redis::flushdb
* Redis::flushall
* Redis::sort
* Redis::info
* Redis::ttl