# PhpRedis

The phpredis extension provides an API for communicating with the [Redis](http://redis.io/) key-value store. It is released under the [PHP License, version 3.01](http://www.php.net/license/3_01.txt).
This code has been developed and maintained by Owlient from November 2009 to March 2011.

You can send comments, patches, questions [here on github](https://github.com/nicolasff/phpredis/issues) or to n.favrefelix@gmail.com ([@yowgi](http://twitter.com/yowgi)).


# Table of contents
-----
1. [Installing/Configuring](#installingconfiguring)
   * [Installation](#installation)
   * [Installation on OSX](#installation-on-osx)
   * [Building on Windows](#building-on-windows)
   * [PHP Session handler](#php-session-handler)
   * [Distributed Redis Array](#distributed-redis-array)
1. [Classes and methods](#classes-and-methods)
   * [Usage](#usage)
   * [Connection](#connection)
   * [Server](#server)
   * [Keys and strings](#keys-and-strings)
   * [Hashes](#hashes)
   * [Lists](#lists)
   * [Sets](#sets)
   * [Sorted sets](#sorted-sets)
   * [Pub/sub](#pubsub)
   * [Transactions](#transactions)
   * [Scripting](#scripting)
   * [Introspection](#introspection) 

-----

# Installing/Configuring
-----

Everything you should need to install PhpRedis on your system.

## Installation

~~~
phpize
./configure [--enable-redis-igbinary]
make && make install
~~~

If you would like phpredis to serialize your data using the igbinary library, run configure with `--enable-redis-igbinary`.
`make install` copies `redis.so` to an appropriate location, but you still need to enable the module in the PHP config file. To do so, either edit your php.ini or add a redis.ini file in `/etc/php5/conf.d` with the following contents: `extension=redis.so`.

You can generate a debian package for PHP5, accessible from Apache 2 by running `./mkdeb-apache2.sh` or with `dpkg-buildpackage` or `svn-buildpackage`.

This extension exports a single class, [Redis](#class-redis) (and [RedisException](#class-redisexception) used in case of errors). Check out https://github.com/ukko/phpredis-phpdoc for a PHP stub that you can use in your IDE for code completion.


## Installation on OSX

If the install fails on OSX, type the following commands in your shell before trying again:
~~~
MACOSX_DEPLOYMENT_TARGET=10.6
CFLAGS="-arch i386 -arch x86_64 -g -Os -pipe -no-cpp-precomp"
CCFLAGS="-arch i386 -arch x86_64 -g -Os -pipe"
CXXFLAGS="-arch i386 -arch x86_64 -g -Os -pipe"
LDFLAGS="-arch i386 -arch x86_64 -bind_at_load"
export CFLAGS CXXFLAGS LDFLAGS CCFLAGS MACOSX_DEPLOYMENT_TARGET
~~~

If that still fails and you are running Zend Server CE, try this right before "make": `./configure CFLAGS="-arch i386"`.

Taken from [Compiling phpredis on Zend Server CE/OSX ](http://www.tumblr.com/tagged/phpredis).

See also: [Install Redis & PHP Extension PHPRedis with Macports](http://www.lecloud.net/post/3378834922/install-redis-php-extension-phpredis-with-macports).


## PHP Session handler

phpredis can be used to store PHP sessions. To do this, configure `session.save_handler` and `session.save_path` in your php.ini to tell phpredis where to store the sessions:
~~~
session.save_handler = redis
session.save_path = "tcp://host1:6379?weight=1, tcp://host2:6379?weight=2&timeout=2.5, tcp://host3:6379?weight=2"
~~~

`session.save_path` can have a simple `host:port` format too, but you need to provide the `tcp://` scheme if you want to use the parameters. The following parameters are available:

* weight (integer): the weight of a host is used in comparison with the others in order to customize the session distribution on several hosts. If host A has twice the weight of host B, it will get twice the amount of sessions. In the example, *host1* stores 20% of all the sessions (1/(1+2+2)) while *host2* and *host3* each store 40% (2/1+2+2). The target host is determined once and for all at the start of the session, and doesn't change. The default weight is 1.
* timeout (float): the connection timeout to a redis host, expressed in seconds. If the host is unreachable in that amount of time, the session storage will be unavailable for the client. The default timeout is very high (86400 seconds).
* persistent (integer, should be 1 or 0): defines if a persistent connection should be used. **(experimental setting)**
* prefix (string, defaults to "PHPREDIS_SESSION:"): used as a prefix to the Redis key in which the session is stored. The key is composed of the prefix followed by the session ID.
* auth (string, empty by default): used to authenticate with the server prior to sending commands.
* database (integer): selects a different database.

Sessions have a lifetime expressed in seconds and stored in the INI variable "session.gc_maxlifetime". You can change it with [`ini_set()`](http://php.net/ini_set).
The session handler requires a version of Redis with the `SETEX` command (at least 2.0).
phpredis can also connect to a unix domain socket: `session.save_path = "unix:///var/run/redis/redis.sock?persistent=1&weight=1&database=0`.


## Building on Windows

See [instructions from @char101](https://github.com/nicolasff/phpredis/issues/213#issuecomment-11361242) on how to build phpredis on Windows.


## Distributed Redis Array

See [dedicated page](https://github.com/nicolasff/phpredis/blob/master/arrays.markdown#readme).



# Classes and methods
-----

## Usage

1. [Class Redis](#class-redis)
1. [Class RedisException](#class-redisexception)
1. [Predefined constants](#predefined-constants)

### Class Redis
-----
_**Description**_: Creates a Redis client

##### *Example*

~~~
$redis = new Redis();
~~~

### Class RedisException
-----
phpredis throws a [RedisException](#class-redisexception) object if it can't reach the Redis server. That can happen in case of connectivity issues,
if the Redis service is down, or if the redis host is overloaded. In any other problematic case that does not involve an
unreachable server (such as a key not existing, an invalid command, etc), phpredis will return `FALSE`.

### Predefined constants
-----
_**Description**_: Available Redis Constants

Redis data types, as returned by [type](#type)
~~~
Redis::REDIS_STRING - String
Redis::REDIS_SET - Set
Redis::REDIS_LIST - List
Redis::REDIS_ZSET - Sorted set
Redis::REDIS_HASH - Hash
Redis::REDIS_NOT_FOUND - Not found / other
~~~

@TODO: OPT_SERIALIZER, AFTER, BEFORE,...

## Connection

1. [connect, open](#connect-open) - Connect to a server
1. [pconnect, popen](#pconnect-popen) - Connect to a server (persistent)
1. [auth](#auth) - Authenticate to the server
1. [select](#select) - Change the selected database for the current connection
1. [close](#close) - Close the connection
1. [setOption](#setoption) - Set client option
1. [getOption](#getoption) - Get client option
1. [ping](#ping) - Ping the server
1. [echo](#echo) - Echo the given string

### connect, open
-----
_**Description**_: Connects to a Redis instance.

##### *Parameters*

*host*: string. can be a host, or the path to a unix domain socket  
*port*: int, optional  
*timeout*: float, value in seconds (optional, default is 0 meaning unlimited)  
*reserved*: should be NULL if retry_interval is specified
*retry_interval*: int, value in milliseconds (optional)

##### *Return value*

*BOOL*: `TRUE` on success, `FALSE` on error.

##### *Example*

~~~
$redis->connect('127.0.0.1', 6379);
$redis->connect('127.0.0.1'); // port 6379 by default
$redis->connect('127.0.0.1', 6379, 2.5); // 2.5 sec timeout.
$redis->connect('/tmp/redis.sock'); // unix domain socket.
$redis->connect('127.0.0.1', 6379, 1, NULL, 100); // 1 sec timeout, 100ms delay between reconnection attempts.
~~~

### pconnect, popen
-----
_**Description**_: Connects to a Redis instance or reuse a connection already established with `pconnect`/`popen`.

The connection will not be closed on `close` or end of request until the php process ends.
So be patient on to many open FD's (specially on redis server side) when using persistent
connections on many servers connecting to one redis server.

Also more than one persistent connection can be made identified by either host + port + timeout
or host + persistent_id or unix socket + timeout.

This feature is not available in threaded versions. `pconnect` and `popen` then working like their non
persistent equivalents.

##### *Parameters*

*host*: string. can be a host, or the path to a unix domain socket  
*port*: int, optional  
*timeout*: float, value in seconds (optional, default is 0 meaning unlimited)  
*persistent_id*: string. identity for the requested persistent connection
*retry_interval*: int, value in milliseconds (optional)

##### *Return value*

*BOOL*: `TRUE` on success, `FALSE` on error.

##### *Example*

~~~
$redis->pconnect('127.0.0.1', 6379);
$redis->pconnect('127.0.0.1'); // port 6379 by default - same connection like before.
$redis->pconnect('127.0.0.1', 6379, 2.5); // 2.5 sec timeout and would be another connection than the two before.
$redis->pconnect('127.0.0.1', 6379, 2.5, 'x'); // x is sent as persistent_id and would be another connection the the three before.
$redis->pconnect('/tmp/redis.sock'); // unix domain socket - would be another connection than the four before.
~~~

### auth
-----
_**Description**_: Authenticate the connection using a password.
*Warning*: The password is sent in plain-text over the network.

##### *Parameters*
*STRING*: password

##### *Return value*
*BOOL*: `TRUE` if the connection is authenticated, `FALSE` otherwise.

##### *Example*
~~~
$redis->auth('foobared');
~~~

### select
-----
_**Description**_: Change the selected database for the current connection.

##### *Parameters*
*INTEGER*: dbindex, the database number to switch to.

##### *Return value*
`TRUE` in case of success, `FALSE` in case of failure.
##### *Example*
See method for example: [move](#move)

### close
-----
_**Description**_: Disconnects from the Redis instance, except when `pconnect` is used.

### setOption
-----
_**Description**_: Set client option.

##### *Parameters*
*parameter name*  
*parameter value*  

##### *Return value*
*BOOL*: `TRUE` on success, `FALSE` on error.

##### *Example*
~~~
$redis->setOption(Redis::OPT_SERIALIZER, Redis::SERIALIZER_NONE);	// don't serialize data
$redis->setOption(Redis::OPT_SERIALIZER, Redis::SERIALIZER_PHP);	// use built-in serialize/unserialize
$redis->setOption(Redis::OPT_SERIALIZER, Redis::SERIALIZER_IGBINARY);	// use igBinary serialize/unserialize

$redis->setOption(Redis::OPT_PREFIX, 'myAppName:');	// use custom prefix on all keys
~~~


### getOption
-----
_**Description**_: Get client option.

##### *Parameters*
*parameter name*  

##### *Return value*
Parameter value.

##### *Example*
~~~
$redis->getOption(Redis::OPT_SERIALIZER);	// return Redis::SERIALIZER_NONE, Redis::SERIALIZER_PHP, or Redis::SERIALIZER_IGBINARY.
~~~

### ping
-----
_**Description**_: Check the current connection status

##### *Parameters*

(none)

##### *Return value*

*STRING*: `+PONG` on success. Throws a [RedisException](#class-redisexception) object on connectivity error, as described above.


### echo
-----
_**Description**_: Sends a string to Redis, which replies with the same string

##### *Parameters*

*STRING*: The message to send.

##### *Return value*

*STRING*: the same message.


## Server

1. [bgrewriteaof](#bgrewriteaof) - Asynchronously rewrite the append-only file
1. [bgsave](#bgsave) - Asynchronously save the dataset to disk (in background)
1. [config](#config) - Get or Set the Redis server configuration parameters
1. [dbSize](#dbsize) - Return the number of keys in selected database
1. [flushAll](#flushall) - Remove all keys from all databases
1. [flushDB](#flushdb) - Remove all keys from the current database
1. [info](#info) - Get information and statistics about the server
1. [lastSave](#lastsave) - Get the timestamp of the last disk save
1. [resetStat](#resetstat) - Reset the stats returned by [info](#info) method.
1. [save](#save) - Synchronously save the dataset to disk (wait to complete)
1. [slaveof](#slaveof) - Make the server a slave of another instance, or promote it to master
1. [time](#time) - Return the current server time
1. [slowlog](#slowlog) - Access the Redis slowlog entries

### bgrewriteaof
-----
_**Description**_: Start the background rewrite of AOF (Append-Only File)

##### *Parameters*
None.

##### *Return value*
*BOOL*: `TRUE` in case of success, `FALSE` in case of failure.

##### *Example*
~~~
$redis->bgrewriteaof();
~~~

### bgsave
-----
_**Description**_: Asynchronously save the dataset to disk (in background)

##### *Parameters*
None.

##### *Return value*
*BOOL*: `TRUE` in case of success, `FALSE` in case of failure. If a save is already running, this command will fail and return `FALSE`.

##### *Example*
~~~
$redis->bgSave();
~~~

### config
-----
_**Description**_: Get or Set the Redis server configuration parameters.

##### *Parameters*
*operation* (string) either `GET` or `SET`  
*key* string for `SET`, glob-pattern for `GET`. See http://redis.io/commands/config-get for examples.  
*value* optional string (only for `SET`)

##### *Return value*
*Associative array* for `GET`, key -> value  
*bool* for `SET`

##### *Examples*
~~~
$redis->config("GET", "*max-*-entries*");
$redis->config("SET", "dir", "/var/run/redis/dumps/");
~~~

### dbSize
-----
_**Description**_: Return the number of keys in selected database.

##### *Parameters*
None.

##### *Return value*
*INTEGER*: DB size, in number of keys.

##### *Example*
~~~
$count = $redis->dbSize();
echo "Redis has $count keys\n";
~~~

### flushAll
-----
_**Description**_: Remove all keys from all databases.

##### *Parameters*
None.

##### *Return value*
*BOOL*: Always `TRUE`.

##### *Example*
~~~
$redis->flushAll();
~~~

### flushDB
-----
_**Description**_: Remove all keys from the current database.

##### *Parameters*
None.

##### *Return value*
*BOOL*: Always `TRUE`.

##### *Example*
~~~
$redis->flushDB();
~~~

### info
-----
_**Description**_: Get information and statistics about the server

Returns an associative array that provides information about the server. Passing no arguments to
INFO will call the standard REDIS INFO command, which returns information such as the following:

* redis_version
* arch_bits
* uptime_in_seconds
* uptime_in_days
* connected_clients
* connected_slaves
* used_memory
* changes_since_last_save
* bgsave_in_progress
* last_save_time
* total_connections_received
* total_commands_processed
* role

You can pass a variety of options to INFO ([per the Redis documentation](http://redis.io/commands/info)),
which will modify what is returned.

##### *Parameters*
*option*: The option to provide redis (e.g. "COMMANDSTATS", "CPU")

##### *Example*
~~~
$redis->info(); /* standard redis INFO command */
$redis->info("COMMANDSTATS"); /* Information on the commands that have been run (>=2.6 only)
$redis->info("CPU"); /* just CPU information from Redis INFO */
~~~

### lastSave
-----
_**Description**_: Returns the timestamp of the last disk save.

##### *Parameters*
None.

##### *Return value*
*INT*: timestamp.

##### *Example*
~~~
$redis->lastSave();
~~~

### resetStat
-----
_**Description**_: Reset the stats returned by [info](#info) method.

These are the counters that are reset:

* Keyspace hits
* Keyspace misses
* Number of commands processed
* Number of connections received
* Number of expired keys


##### *Parameters*
None.

##### *Return value*
*BOOL*: `TRUE` in case of success, `FALSE` in case of failure.

##### *Example*
~~~
$redis->resetStat();
~~~

### save
-----
_**Description**_: Synchronously save the dataset to disk (wait to complete)

##### *Parameters*
None.

##### *Return value*
*BOOL*: `TRUE` in case of success, `FALSE` in case of failure. If a save is already running, this command will fail and return `FALSE`.

##### *Example*
~~~
$redis->save();
~~~

### slaveof
-----
_**Description**_: Changes the slave status

##### *Parameters*
Either host (string) and port (int), or no parameter to stop being a slave.

##### *Return value*
*BOOL*: `TRUE` in case of success, `FALSE` in case of failure.

##### *Example*
~~~
$redis->slaveof('10.0.1.7', 6379);
/* ... */
$redis->slaveof();
~~~

### time
-----
_**Description**_: Return the current server time.

##### *Parameters*
(none)  

##### *Return value*
If successfull, the time will come back as an associative array with element zero being
the unix timestamp, and element one being microseconds.

##### *Examples*
~~~
$redis->time();
~~~

### slowlog
-----
_**Description**_: Access the Redis slowlog

##### *Parameters*
*Operation* (string): This can be either `GET`, `LEN`, or `RESET` 
*Length* (integer), optional: If executing a `SLOWLOG GET` command, you can pass an optional length.
#####

##### *Return value*
The return value of SLOWLOG will depend on which operation was performed.
SLOWLOG GET: Array of slowlog entries, as provided by Redis
SLOGLOG LEN: Integer, the length of the slowlog
SLOWLOG RESET: Boolean, depending on success
#####

##### *Examples*
~~~
// Get ten slowlog entries
$redis->slowlog('get', 10); 
// Get the default number of slowlog entries

$redis->slowlog('get');
// Reset our slowlog
$redis->slowlog('reset');

// Retrieve slowlog length
$redis->slowlog('len');
~~~

## Keys and Strings

### Strings
-----

* [append](#append) - Append a value to a key
* [bitcount](#bitcount) - Count set bits in a string
* [bitop](#bitop) - Perform bitwise operations between strings
* [decr, decrBy](#decr-decrby) - Decrement the value of a key
* [get](#get) - Get the value of a key
* [getBit](#getbit) - Returns the bit value at offset in the string value stored at key
* [getRange](#getrange) - Get a substring of the string stored at a key
* [getSet](#getset) - Set the string value of a key and return its old value
* [incr, incrBy](#incr-incrby) - Increment the value of a key
* [incrByFloat](#incrbyfloat) - Increment the float value of a key by the given amount
* [mGet, getMultiple](#mget-getmultiple) - Get the values of all the given keys
* [mSet, mSetNX](#mset-msetnx) - Set multiple keys to multiple values
* [set](#set) - Set the string value of a key
* [setBit](#setbit) - Sets or clears the bit at offset in the string value stored at key
* [setex, psetex](#setex-psetex) - Set the value and expiration of a key
* [setnx](#setnx) - Set the value of a key, only if the key does not exist
* [setRange](#setrange) - Overwrite part of a string at key starting at the specified offset
* [strlen](#strlen) - Get the length of the value stored in a key

### Keys
-----

* [del, delete](#del-delete) - Delete a key
* [dump](#dump) - Return a serialized version of the value stored at the specified key.
* [exists](#exists) - Determine if a key exists
* [expire, setTimeout, pexpire](#expire-settimeout-pexpire) - Set a key's time to live in seconds
* [expireAt, pexpireAt](#expireat-pexpireat) - Set the expiration for a key as a UNIX timestamp
* [keys, getKeys](#keys-getkeys) - Find all keys matching the given pattern
* [migrate](#migrate) - Atomically transfer a key from a Redis instance to another one
* [move](#move) - Move a key to another database
* [object](#object) - Inspect the internals of Redis objects
* [persist](#persist) - Remove the expiration from a key
* [randomKey](#randomkey) - Return a random key from the keyspace
* [rename, renameKey](#rename-renamekey) - Rename a key
* [renameNx](#renamenx) - Rename a key, only if the new key does not exist
* [type](#type) - Determine the type stored at key
* [sort](#sort) - Sort the elements in a list, set or sorted set
* [ttl, pttl](#ttl-pttl) - Get the time to live for a key
* [restore](#restore) - Create a key using the provided serialized value, previously obtained with [dump](#dump).

-----

### get
-----
_**Description**_: Get the value related to the specified key

##### *Parameters*
*key*

##### *Return value*
*String* or *Bool*: If key didn't exist, `FALSE` is returned. Otherwise, the value related to this key is returned.

##### *Examples*

~~~
$redis->get('key');
~~~

### set
-----
_**Description**_: Set the string value in argument as value of the key.  If you're using Redis >= 2.6.12, you can pass extended options as explained below

##### *Parameters*
*Key*  
*Value*  
*Timeout or Options Array* (optional). If you pass an integer, phpredis will redirect to SETEX, and will try to use Redis >= 2.6.12 extended options if you pass an array with valid values

##### *Return value*
*Bool* `TRUE` if the command is successful.

##### *Examples*
~~~
// Simple key -> value set
$redis->set('key', 'value');

// Will redirect, and actually make an SETEX call
$redis->set('key','value', 10);

// Will set the key, if it doesn't exist, with a ttl of 10 seconds
$redis->set('key', 'value', Array('nx', 'ex'=>10));

// Will set a key, if it does exist, with a ttl of 1000 miliseconds
$redis->set('key', 'value', Array('xx', 'px'=>1000));

~~~

### setex, psetex
-----
_**Description**_: Set the string value in argument as value of the key, with a time to live. PSETEX uses a TTL in milliseconds.

##### *Parameters*
*Key*
*TTL*
*Value*

##### *Return value*
*Bool* `TRUE` if the command is successful.

##### *Examples*

~~~
$redis->setex('key', 3600, 'value'); // sets key → value, with 1h TTL.
$redis->psetex('key', 100, 'value'); // sets key → value, with 0.1 sec TTL.
~~~

### setnx
-----
_**Description**_: Set the string value in argument as value of the key if the key doesn't already exist in the database.

##### *Parameters*
*key*
*value*

##### *Return value*
*Bool* `TRUE` in case of success, `FALSE` in case of failure.

##### *Examples*
~~~
$redis->setnx('key', 'value'); /* return TRUE */
$redis->setnx('key', 'value'); /* return FALSE */
~~~

### del, delete
-----
_**Description**_: Remove specified keys.

##### *Parameters*
An array of keys, or an undefined number of parameters, each a key: *key1* *key2* *key3* ... *keyN*

##### *Return value*
*Long* Number of keys deleted.

##### *Examples*
~~~
$redis->set('key1', 'val1');
$redis->set('key2', 'val2');
$redis->set('key3', 'val3');
$redis->set('key4', 'val4');

$redis->delete('key1', 'key2'); /* return 2 */
$redis->delete(array('key3', 'key4')); /* return 2 */
~~~


### exists
-----
_**Description**_: Verify if the specified key exists.

##### *Parameters*
*key*

##### *Return value*
*BOOL*: If the key exists, return `TRUE`, otherwise return `FALSE`.

##### *Examples*
~~~
$redis->set('key', 'value');
$redis->exists('key'); /*  TRUE */
$redis->exists('NonExistingKey'); /* FALSE */
~~~

### incr, incrBy
-----
_**Description**_: Increment the number stored at key by one. If the second argument is filled, it will be used as the integer value of the increment.

##### *Parameters*
*key*  
*value*: value that will be added to key (only for incrBy)

##### *Return value*
*INT* the new value

##### *Examples*
~~~
$redis->incr('key1'); /* key1 didn't exists, set to 0 before the increment */
					  /* and now has the value 1  */

$redis->incr('key1'); /* 2 */
$redis->incr('key1'); /* 3 */
$redis->incr('key1'); /* 4 */
$redis->incrBy('key1', 10); /* 14 */
~~~

### incrByFloat
-----
_**Description**_: Increment the key with floating point precision.

##### *Parameters*
*key*  
*value*: (float) value that will be added to the key  

##### *Return value*
*FLOAT* the new value

##### *Examples*
~~~
$redis->incrByFloat('key1', 1.5); /* key1 didn't exist, so it will now be 1.5 */


$redis->incrByFloat('key1', 1.5); /* 3 */
$redis->incrByFloat('key1', -1.5); /* 1.5 */
$redis->incrByFloat('key1', 2.5); /* 4 */
~~~

### decr, decrBy
-----
_**Description**_: Decrement the number stored at key by one. If the second argument is filled, it will be used as the integer value of the decrement.

##### *Parameters*
*key*  
*value*: value that will be substracted to key (only for decrBy)

##### *Return value*
*INT* the new value

##### *Examples*
~~~
$redis->decr('key1'); /* key1 didn't exists, set to 0 before the increment */
					  /* and now has the value -1  */

$redis->decr('key1'); /* -2 */
$redis->decr('key1'); /* -3 */
$redis->decrBy('key1', 10); /* -13 */
~~~

### mGet, getMultiple
-----
_**Description**_: Get the values of all the specified keys. If one or more keys dont exist, the array will contain `FALSE` at the position of the key.

##### *Parameters*
*Array*: Array containing the list of the keys

##### *Return value*
*Array*: Array containing the values related to keys in argument

##### *Examples*
~~~
$redis->set('key1', 'value1');
$redis->set('key2', 'value2');
$redis->set('key3', 'value3');
$redis->mGet(array('key1', 'key2', 'key3')); /* array('value1', 'value2', 'value3');
$redis->mGet(array('key0', 'key1', 'key5')); /* array(`FALSE`, 'value2', `FALSE`);
~~~

### getSet
-----
_**Description**_: Sets a value and returns the previous entry at that key.
##### *Parameters*
*Key*: key

*STRING*: value

##### *Return value*
A string, the previous value located at this key.
##### *Example*
~~~
$redis->set('x', '42');
$exValue = $redis->getSet('x', 'lol');	// return '42', replaces x by 'lol'
$newValue = $redis->get('x')'		// return 'lol'
~~~

### randomKey
-----
_**Description**_: Returns a random key.

##### *Parameters*
None.
##### *Return value*
*STRING*: an existing key in redis.

##### *Example*
~~~
$key = $redis->randomKey();
$surprise = $redis->get($key);	// who knows what's in there.
~~~

### move
-----
_**Description**_: Moves a key to a different database.

##### *Parameters*
*Key*: key, the key to move.

*INTEGER*: dbindex, the database number to move the key to.

##### *Return value*
*BOOL*: `TRUE` in case of success, `FALSE` in case of failure.
##### *Example*

~~~
$redis->select(0);	// switch to DB 0
$redis->set('x', '42');	// write 42 to x
$redis->move('x', 1);	// move to DB 1
$redis->select(1);	// switch to DB 1
$redis->get('x');	// will return 42
~~~

### rename, renameKey
-----
_**Description**_: Renames a key.
##### *Parameters*
*STRING*: srckey, the key to rename.

*STRING*: dstkey, the new name for the key.

##### *Return value*
*BOOL*: `TRUE` in case of success, `FALSE` in case of failure.
##### *Example*
~~~
$redis->set('x', '42');
$redis->rename('x', 'y');
$redis->get('y'); 	// → 42
$redis->get('x'); 	// → `FALSE`
~~~

### renameNx
-----
_**Description**_: Same as rename, but will not replace a key if the destination already exists. This is the same behaviour as setNx.

### expire, setTimeout, pexpire
-----
_**Description**_: Sets an expiration date (a timeout) on an item. pexpire requires a TTL in milliseconds.

##### *Parameters*
*Key*: key. The key that will disappear.

*Integer*: ttl. The key's remaining Time To Live, in seconds.

##### *Return value*
*BOOL*: `TRUE` in case of success, `FALSE` in case of failure.
##### *Example*
~~~
$redis->set('x', '42');
$redis->setTimeout('x', 3);	// x will disappear in 3 seconds.
sleep(5);				// wait 5 seconds
$redis->get('x'); 		// will return `FALSE`, as 'x' has expired.
~~~

### expireAt, pexpireAt
-----
_**Description**_: Sets an expiration date (a timestamp) on an item. pexpireAt requires a timestamp in milliseconds.

##### *Parameters*
*Key*: key. The key that will disappear.

*Integer*: Unix timestamp. The key's date of death, in seconds from Epoch time.

##### *Return value*
*BOOL*: `TRUE` in case of success, `FALSE` in case of failure.
##### *Example*
~~~
$redis->set('x', '42');
$now = time(NULL); // current timestamp
$redis->expireAt('x', $now + 3);	// x will disappear in 3 seconds.
sleep(5);				// wait 5 seconds
$redis->get('x'); 		// will return `FALSE`, as 'x' has expired.
~~~

### keys, getKeys
-----
_**Description**_: Returns the keys that match a certain pattern.

##### *Parameters*
*STRING*: pattern, using '*' as a wildcard.

##### *Return value*
*Array of STRING*: The keys that match a certain pattern.

##### *Example*
~~~
$allKeys = $redis->keys('*');	// all keys will match this.
$keyWithUserPrefix = $redis->keys('user*');
~~~



### object
-----
_**Description**_: Describes the object pointed to by a key.

##### *Parameters*
The information to retrieve (string) and the key (string). Info can be one of the following:

* "encoding"
* "refcount"
* "idletime"

##### *Return value*
*STRING* for "encoding", *LONG* for "refcount" and "idletime", `FALSE` if the key doesn't exist.

##### *Example*
~~~
$redis->object("encoding", "l"); // → ziplist
$redis->object("refcount", "l"); // → 1
$redis->object("idletime", "l"); // → 400 (in seconds, with a precision of 10 seconds).
~~~

### type
-----
_**Description**_: Returns the type of data pointed by a given key.

##### *Parameters*
*Key*: key

##### *Return value*

Depending on the type of the data pointed by the key, this method will return the following value:  
string: Redis::REDIS_STRING  
set: Redis::REDIS_SET  
list: Redis::REDIS_LIST  
zset: Redis::REDIS_ZSET  
hash: Redis::REDIS_HASH  
other: Redis::REDIS_NOT_FOUND  

##### *Example*
~~~
$redis->type('key');
~~~

### append
-----
_**Description**_: Append specified string to the string stored in specified key.

##### *Parameters*
*Key*
*Value*

##### *Return value*
*INTEGER*: Size of the value after the append

##### *Example*
~~~
$redis->set('key', 'value1');
$redis->append('key', 'value2'); /* 12 */
$redis->get('key'); /* 'value1value2' */
~~~

### getRange
-----
_**Description**_: Return a substring of a larger string 

*Note*: substr also supported but deprecated in redis.

##### *Parameters*
*key*
*start*
*end*

##### *Return value*
*STRING*: the substring 

##### *Example*
~~~
$redis->set('key', 'string value');
$redis->getRange('key', 0, 5); /* 'string' */
$redis->getRange('key', -5, -1); /* 'value' */
~~~

### setRange
-----
_**Description**_: Changes a substring of a larger string.

##### *Parameters*
*key*  
*offset*  
*value*  

##### *Return value*
*STRING*: the length of the string after it was modified.

##### *Example*
~~~
$redis->set('key', 'Hello world');
$redis->setRange('key', 6, "redis"); /* returns 11 */
$redis->get('key'); /* "Hello redis" */
~~~

### strlen
-----
_**Description**_: Get the length of a string value.

##### *Parameters*
*key*

##### *Return value*
*INTEGER*

##### *Example*
~~~
$redis->set('key', 'value');
$redis->strlen('key'); /* 5 */
~~~

### getBit
-----
_**Description**_: Return a single bit out of a larger string

##### *Parameters*
*key*  
*offset*  

##### *Return value*
*LONG*: the bit value (0 or 1)

##### *Example*
~~~
$redis->set('key', "\x7f"); // this is 0111 1111
$redis->getBit('key', 0); /* 0 */
$redis->getBit('key', 1); /* 1 */
~~~

### setBit
-----
_**Description**_: Changes a single bit of a string.

##### *Parameters*
*key*  
*offset*  
*value*: bool or int (1 or 0)  

##### *Return value*
*LONG*: 0 or 1, the value of the bit before it was set.

##### *Example*
~~~
$redis->set('key', "*");	// ord("*") = 42 = 0x2f = "0010 1010"
$redis->setBit('key', 5, 1); /* returns 0 */
$redis->setBit('key', 7, 1); /* returns 0 */
$redis->get('key'); /* chr(0x2f) = "/" = b("0010 1111") */
~~~

### bitop
-----
_**Description**_: Bitwise operation on multiple keys.

##### *Parameters*
*operation*: either "AND", "OR", "NOT", "XOR"  
*ret_key*: return key  
*key1*  
*key2...*  

##### *Return value*
*LONG*: The size of the string stored in the destination key.

### bitcount
-----
_**Description**_: Count bits in a string.

##### *Parameters*
*key*  

##### *Return value*
*LONG*: The number of bits set to 1 in the value behind the input key.

### sort
-----
_**Description**_: Sort the elements in a list, set or sorted set.

##### *Parameters*
*Key*: key
*Options*: array(key => value, ...) - optional, with the following keys and values:
~~~
    'by' => 'some_pattern_*',
    'limit' => array(0, 1),
    'get' => 'some_other_pattern_*' or an array of patterns,
    'sort' => 'asc' or 'desc',
    'alpha' => TRUE,
    'store' => 'external-key'
~~~
##### *Return value*
An array of values, or a number corresponding to the number of elements stored if that was used.

##### *Example*
~~~
$redis->delete('s');
$redis->sadd('s', 5);
$redis->sadd('s', 4);
$redis->sadd('s', 2);
$redis->sadd('s', 1);
$redis->sadd('s', 3);

var_dump($redis->sort('s')); // 1,2,3,4,5
var_dump($redis->sort('s', array('sort' => 'desc'))); // 5,4,3,2,1
var_dump($redis->sort('s', array('sort' => 'desc', 'store' => 'out'))); // (int)5
~~~




### ttl, pttl
-----
_**Description**_: Returns the time to live left for a given key in seconds (ttl), or milliseconds (pttl).

##### *Parameters*
*Key*: key

##### *Return value*
*LONG*:  The time to live in seconds.  If the key has no ttl, `-1` will be returned, and `-2` if the key doesn't exist.

##### *Example*
~~~
$redis->ttl('key');
~~~

### persist
-----
_**Description**_: Remove the expiration timer from a key.

##### *Parameters*
*Key*: key

##### *Return value*
*BOOL*: `TRUE` if a timeout was removed, `FALSE` if the key didn’t exist or didn’t have an expiration timer.

##### *Example*
~~~
$redis->persist('key');
~~~

### mset, msetnx
-----
_**Description**_: Sets multiple key-value pairs in one atomic command. MSETNX only returns TRUE if all the keys were set (see SETNX).

##### *Parameters*
*Pairs*: array(key => value, ...)

##### *Return value*
*Bool* `TRUE` in case of success, `FALSE` in case of failure.

##### *Example*
~~~

$redis->mset(array('key0' => 'value0', 'key1' => 'value1'));
var_dump($redis->get('key0'));
var_dump($redis->get('key1'));

~~~
Output:
~~~
string(6) "value0"
string(6) "value1"
~~~



### dump
-----
_**Description**_: Dump a key out of a redis database, the value of which can later be passed into redis using the RESTORE command.  The data
that comes out of DUMP is a binary representation of the key as Redis stores it.
##### *Parameters*
*key* string  
##### *Return value*
The Redis encoded value of the key, or FALSE if the key doesn't exist
##### *Examples*
~~~
$redis->set('foo', 'bar');
$val = $redis->dump('foo'); // $val will be the Redis encoded key value
~~~

### restore
-----
_**Description**_: Restore a key from the result of a DUMP operation.
##### *Parameters*
*key* string.  The key name  
*ttl* integer.  How long the key should live (if zero, no expire will be set on the key)  
*value* string (binary).  The Redis encoded key value (from DUMP)  
##### *Examples*
~~~
$redis->set('foo', 'bar');
$val = $redis->dump('foo');
$redis->restore('bar', 0, $val); // The key 'bar', will now be equal to the key 'foo'
~~~

### migrate
-----
_**Description**_: Migrates a key to a different Redis instance.
##### *Parameters*
*host* string.  The destination host  
*port* integer.  The TCP port to connect to.  
*key* string. The key to migrate.  
*destination-db* integer.  The target DB.  
*timeout* integer.  The maximum amount of time given to this transfer.  
##### *Examples*
~~~
$redis->migrate('backup', 6379, 'foo', 0, 3600);
~~~



## Hashes

* [hDel](#hdel) - Delete one or more hash fields
* [hExists](#hexists) - Determine if a hash field exists
* [hGet](#hget) - Get the value of a hash field
* [hGetAll](#hgetall) - Get all the fields and values in a hash
* [hIncrBy](#hincrby) - Increment the integer value of a hash field by the given number
* [hIncrByFloat](#hincrbyfloat) - Increment the float value of a hash field by the given amount
* [hKeys](#hkeys) - Get all the fields in a hash
* [hLen](#hlen) - Get the number of fields in a hash
* [hMGet](#hmget) - Get the values of all the given hash fields
* [hMSet](#hmset) - Set multiple hash fields to multiple values
* [hSet](#hset) - Set the string value of a hash field
* [hSetNx](#hsetnx) - Set the value of a hash field, only if the field does not exist
* [hVals](#hvals) - Get all the values in a hash

### hSet
-----
_**Description**_: Adds a value to the hash stored at key. If this value is already in the hash, `FALSE` is returned.  
##### *Parameters*
*key*  
*hashKey*  
*value*  

##### *Return value*
*LONG* `1` if value didn't exist and was added successfully, `0` if the value was already present and was replaced, `FALSE` if there was an error.
##### *Example*
~~~
$redis->delete('h')
$redis->hSet('h', 'key1', 'hello'); /* 1, 'key1' => 'hello' in the hash at "h" */
$redis->hGet('h', 'key1'); /* returns "hello" */

$redis->hSet('h', 'key1', 'plop'); /* 0, value was replaced. */
$redis->hGet('h', 'key1'); /* returns "plop" */
~~~

### hSetNx
-----
_**Description**_: Adds a value to the hash stored at key only if this field isn't already in the hash.

##### *Return value*
*BOOL* `TRUE` if the field was set, `FALSE` if it was already present.

##### *Example*
~~~
$redis->delete('h')
$redis->hSetNx('h', 'key1', 'hello'); /* TRUE, 'key1' => 'hello' in the hash at "h" */
$redis->hSetNx('h', 'key1', 'world'); /* FALSE, 'key1' => 'hello' in the hash at "h". No change since the field wasn't replaced. */
~~~


### hGet
-----
_**Description**_: Gets a value from the hash stored at key. If the hash table doesn't exist, or the key doesn't exist, `FALSE` is returned.  
##### *Parameters*
*key*  
*hashKey*  

##### *Return value*
*STRING* The value, if the command executed successfully
*BOOL* `FALSE` in case of failure


### hLen
-----
_**Description**_: Returns the length of a hash, in number of items
##### *Parameters*
*key*

##### *Return value*
*LONG* the number of items in a hash, `FALSE` if the key doesn't exist or isn't a hash.
##### *Example*
~~~
$redis->delete('h')
$redis->hSet('h', 'key1', 'hello');
$redis->hSet('h', 'key2', 'plop');
$redis->hLen('h'); /* returns 2 */
~~~

### hDel
-----
_**Description**_: Removes a value from the hash stored at key. If the hash table doesn't exist, or the key doesn't exist, `FALSE` is returned.  
##### *Parameters*
*key*  
*hashKey*  

##### *Return value*
*BOOL* `TRUE` in case of success, `FALSE` in case of failure


### hKeys
-----
_**Description**_: Returns the keys in a hash, as an array of strings.

##### *Parameters*
*Key*: key

##### *Return value*
An array of elements, the keys of the hash. This works like PHP's array_keys().

##### *Example*
~~~
$redis->delete('h');
$redis->hSet('h', 'a', 'x');
$redis->hSet('h', 'b', 'y');
$redis->hSet('h', 'c', 'z');
$redis->hSet('h', 'd', 't');
var_dump($redis->hKeys('h'));
~~~

Output:
~~~
array(4) {
  [0]=>
  string(1) "a"
  [1]=>
  string(1) "b"
  [2]=>
  string(1) "c"
  [3]=>
  string(1) "d"
}
~~~
The order is random and corresponds to redis' own internal representation of the set structure.

### hVals
-----
_**Description**_: Returns the values in a hash, as an array of strings.

##### *Parameters*
*Key*: key

##### *Return value*
An array of elements, the values of the hash. This works like PHP's array_values().

##### *Example*
~~~
$redis->delete('h');
$redis->hSet('h', 'a', 'x');
$redis->hSet('h', 'b', 'y');
$redis->hSet('h', 'c', 'z');
$redis->hSet('h', 'd', 't');
var_dump($redis->hVals('h'));
~~~

Output:
~~~
array(4) {
  [0]=>
  string(1) "x"
  [1]=>
  string(1) "y"
  [2]=>
  string(1) "z"
  [3]=>
  string(1) "t"
}
~~~
The order is random and corresponds to redis' own internal representation of the set structure.

### hGetAll
-----
_**Description**_: Returns the whole hash, as an array of strings indexed by strings.

##### *Parameters*
*Key*: key

##### *Return value*
An array of elements, the contents of the hash.

##### *Example*
~~~
$redis->delete('h');
$redis->hSet('h', 'a', 'x');
$redis->hSet('h', 'b', 'y');
$redis->hSet('h', 'c', 'z');
$redis->hSet('h', 'd', 't');
var_dump($redis->hGetAll('h'));
~~~

Output:
~~~
array(4) {
  ["a"]=>
  string(1) "x"
  ["b"]=>
  string(1) "y"
  ["c"]=>
  string(1) "z"
  ["d"]=>
  string(1) "t"
}
~~~
The order is random and corresponds to redis' own internal representation of the set structure.

### hExists
-----
_**Description**_: Verify if the specified member exists in a key.
##### *Parameters*
*key*  
*memberKey*
##### *Return value*
*BOOL*: If the member exists in the hash table, return `TRUE`, otherwise return `FALSE`.
##### *Examples*
~~~
$redis->hSet('h', 'a', 'x');
$redis->hExists('h', 'a'); /*  TRUE */
$redis->hExists('h', 'NonExistingKey'); /* FALSE */
~~~

### hIncrBy
-----
_**Description**_: Increments the value of a member from a hash by a given amount.
##### *Parameters*
*key*  
*member*  
*value*: (integer) value that will be added to the member's value  
##### *Return value*
*LONG* the new value
##### *Examples*
~~~
$redis->delete('h');
$redis->hIncrBy('h', 'x', 2); /* returns 2: h[x] = 2 now. */
$redis->hIncrBy('h', 'x', 1); /* h[x] ← 2 + 1. Returns 3 */
~~~

### hIncrByFloat
-----
_**Description**_: Increments the value of a hash member by the provided float value
##### *Parameters*
*key*  
*member*  
*value*: (float) value that will be added to the member's value  
##### *Return value*
*FLOAT* the new value
##### *Examples*
~~~
$redis->delete('h');
$redis->hIncrByFloat('h','x', 1.5); /* returns 1.5: h[x] = 1.5 now */
$redis->hIncrByFLoat('h', 'x', 1.5); /* returns 3.0: h[x] = 3.0 now */
$redis->hIncrByFloat('h', 'x', -3.0); /* returns 0.0: h[x] = 0.0 now */
~~~

### hMSet
-----
_**Description**_: Fills in a whole hash. Non-string values are converted to string, using the standard `(string)` cast. NULL values are stored as empty strings.
##### *Parameters*
*key*  
*members*: key → value array  
##### *Return value*
*BOOL*  
##### *Examples*
~~~
$redis->delete('user:1');
$redis->hMset('user:1', array('name' => 'Joe', 'salary' => 2000));
$redis->hIncrBy('user:1', 'salary', 100); // Joe earns 100 more now.
~~~

### hMGet
-----
_**Description**_: Retrieve the values associated to the specified fields in the hash.
##### *Parameters*
*key*  
*memberKeys* Array  
##### *Return value*
*Array* An array of elements, the values of the specified fields in the hash, with the hash keys as array keys.
##### *Examples*
~~~
$redis->delete('h');
$redis->hSet('h', 'field1', 'value1');
$redis->hSet('h', 'field2', 'value2');
$redis->hmGet('h', array('field1', 'field2')); /* returns array('field1' => 'value1', 'field2' => 'value2') */
~~~



## Lists

* [blPop, brPop](#blpop-brpop) - Remove and get the first/last element in a list
* [brpoplpush](#brpoplpush) - Pop a value from a list, push it to another list and return it
* [lIndex, lGet](#lindex-lget) - Get an element from a list by its index
* [lInsert](#linsert) - Insert an element before or after another element in a list
* [lLen, lSize](#llen-lsize) - Get the length/size of a list
* [lPop](#lpop) - Remove and get the first element in a list
* [lPush](#lpush) - Prepend one or multiple values to a list
* [lPushx](#lpushx) - Prepend a value to a list, only if the list exists
* [lRange, lGetRange](#lrange-lgetrange) - Get a range of elements from a list
* [lRem, lRemove](#lrem-lremove) - Remove elements from a list
* [lSet](#lset) - Set the value of an element in a list by its index
* [lTrim, listTrim](#ltrim-listtrim) - Trim a list to the specified range
* [rPop](#rpop) - Remove and get the last element in a list
* [rpoplpush](#rpoplpush) - Remove the last element in a list, append it to another list and return it (redis >= 1.1)
* [rPush](#rpush) - Append one or multiple values to a list
* [rPushx](#rpushx) - Append a value to a list, only if the list exists

### blPop, brPop
-----
_**Description**_: Is a blocking lPop(rPop) primitive. If at least one of the lists contains at least one element, the element will be popped from the head of the list and returned to the caller.
Il all the list identified by the keys passed in arguments are empty, blPop will block during the specified timeout until an element is pushed to one of those lists. This element will be popped.

##### *Parameters*
*ARRAY* Array containing the keys of the lists
*INTEGER* Timeout
Or
*STRING* Key1
*STRING* Key2
*STRING* Key3
...
*STRING* Keyn
*INTEGER* Timeout

##### *Return value*
*ARRAY* array('listName', 'element')

##### *Example*
~~~
/* Non blocking feature */
$redis->lPush('key1', 'A');
$redis->delete('key2');

$redis->blPop('key1', 'key2', 10); /* array('key1', 'A') */
/* OR */
$redis->blPop(array('key1', 'key2'), 10); /* array('key1', 'A') */

$redis->brPop('key1', 'key2', 10); /* array('key1', 'A') */
/* OR */
$redis->brPop(array('key1', 'key2'), 10); /* array('key1', 'A') */

/* Blocking feature */

/* process 1 */
$redis->delete('key1');
$redis->blPop('key1', 10);
/* blocking for 10 seconds */

/* process 2 */
$redis->lPush('key1', 'A');

/* process 1 */
/* array('key1', 'A') is returned*/
~~~

### brpoplpush
-----
_**Description**_: A blocking version of `rpoplpush`, with an integral timeout in the third parameter.

##### *Parameters*
*Key*: srckey  
*Key*: dstkey  
*Long*: timeout

##### *Return value*
*STRING* The element that was moved in case of success, `FALSE` in case of timeout.

### lIndex, lGet
-----
_**Description**_: Return the specified element of the list stored at the specified key.

0 the first element, 1 the second ...
-1 the last element, -2 the penultimate ...

Return `FALSE` in case of a bad index or a key that doesn't point to a list.

##### *Parameters*
*key*
*index*

##### *Return value*
*String* the element at this index  
*Bool* `FALSE` if the key identifies a non-string data type, or no value corresponds to this index in the list `Key`.

##### *Example*
~~~
$redis->rPush('key1', 'A');
$redis->rPush('key1', 'B');
$redis->rPush('key1', 'C'); /* key1 => [ 'A', 'B', 'C' ] */
$redis->lGet('key1', 0); /* 'A' */
$redis->lGet('key1', -1); /* 'C' */
$redis->lGet('key1', 10); /* `FALSE` */
~~~

### lInsert
-----
_**Description**_: Insert value in the list before or after the pivot value.

The parameter options specify the position of the insert (before or after).
If the list didn't exists, or the pivot didn't exists, the value is not inserted.

##### *Parameters*
*key*
*position*  Redis::BEFORE | Redis::AFTER
*pivot*
*value*

##### *Return value*
The number of the elements in the list, -1 if the pivot didn't exists.

##### *Example*
~~~
$redis->delete('key1');
$redis->lInsert('key1', Redis::AFTER, 'A', 'X'); /* 0 */

$redis->lPush('key1', 'A');
$redis->lPush('key1', 'B');
$redis->lPush('key1', 'C');

$redis->lInsert('key1', Redis::BEFORE, 'C', 'X'); /* 4 */
$redis->lRange('key1', 0, -1); /* array('A', 'B', 'X', 'C') */

$redis->lInsert('key1', Redis::AFTER, 'C', 'Y'); /* 5 */
$redis->lRange('key1', 0, -1); /* array('A', 'B', 'X', 'C', 'Y') */

$redis->lInsert('key1', Redis::AFTER, 'W', 'value'); /* -1 */
~~~

### lPop
-----
_**Description**_: Return and remove the first element of the list.

##### *Parameters*
*key*

##### *Return value*
*STRING* if command executed successfully
*BOOL* `FALSE` in case of failure (empty list)

##### *Example*
~~~
$redis->rPush('key1', 'A');
$redis->rPush('key1', 'B');
$redis->rPush('key1', 'C'); /* key1 => [ 'A', 'B', 'C' ] */
$redis->lPop('key1'); /* key1 => [ 'B', 'C' ] */
~~~

### lPush
-----
_**Description**_: Adds the string value to the head (left) of the list. Creates the list if the key didn't exist. If the key exists and is not a list, `FALSE` is returned.

##### *Parameters*
*key*  
*value* String, value to push in key

##### *Return value*
*LONG* The new length of the list in case of success, `FALSE` in case of Failure.

##### *Examples*
~~~
$redis->delete('key1');
$redis->lPush('key1', 'C'); // returns 1
$redis->lPush('key1', 'B'); // returns 2
$redis->lPush('key1', 'A'); // returns 3
/* key1 now points to the following list: [ 'A', 'B', 'C' ] */
~~~

### lPushx
-----
_**Description**_: Adds the string value to the head (left) of the list if the list exists.

##### *Parameters*
*key*  
*value* String, value to push in key

##### *Return value*
*LONG* The new length of the list in case of success, `FALSE` in case of Failure.

##### *Examples*
~~~
$redis->delete('key1');
$redis->lPushx('key1', 'A'); // returns 0
$redis->lPush('key1', 'A'); // returns 1
$redis->lPushx('key1', 'B'); // returns 2
$redis->lPushx('key1', 'C'); // returns 3
/* key1 now points to the following list: [ 'A', 'B', 'C' ] */
~~~

### lRange, lGetRange
-----
_**Description**_: Returns the specified elements of the list stored at the specified key in the range [start, end]. start and stop are interpretated as indices:
0 the first element, 1 the second ...
-1 the last element, -2 the penultimate ...

##### *Parameters*
*key*
*start*
*end*

##### *Return value*
*Array* containing the values in specified range. 

##### *Example*
~~~
$redis->rPush('key1', 'A');
$redis->rPush('key1', 'B');
$redis->rPush('key1', 'C');
$redis->lRange('key1', 0, -1); /* array('A', 'B', 'C') */
~~~

### lRem, lRemove
-----
_**Description**_: Removes the first `count` occurences of the value element from the list. If count is zero, all the matching elements are removed. If count is negative, elements are removed from tail to head.

**Note**: The argument order is not the same as in the Redis documentation. This difference is kept for compatibility reasons.

##### *Parameters*
*key*  
*value*  
*count*  

##### *Return value*
*LONG* the number of elements to remove  
*BOOL* `FALSE` if the value identified by key is not a list.

##### *Example*
~~~
$redis->lPush('key1', 'A');
$redis->lPush('key1', 'B');
$redis->lPush('key1', 'C'); 
$redis->lPush('key1', 'A'); 
$redis->lPush('key1', 'A'); 

$redis->lRange('key1', 0, -1); /* array('A', 'A', 'C', 'B', 'A') */
$redis->lRem('key1', 'A', 2); /* 2 */
$redis->lRange('key1', 0, -1); /* array('C', 'B', 'A') */
~~~

### lSet
-----
_**Description**_: Set the list at index with the new value.

##### *Parameters*
*key*
*index*
*value*

##### *Return value*
*BOOL* `TRUE` if the new value is setted. `FALSE` if the index is out of range, or data type identified by key is not a list.

##### *Example*
~~~
$redis->rPush('key1', 'A');
$redis->rPush('key1', 'B');
$redis->rPush('key1', 'C'); /* key1 => [ 'A', 'B', 'C' ] */
$redis->lGet('key1', 0); /* 'A' */
$redis->lSet('key1', 0, 'X');
$redis->lGet('key1', 0); /* 'X' */ 
~~~

### lTrim, listTrim
-----
_**Description**_: Trims an existing list so that it will contain only a specified range of elements.

##### *Parameters*
*key*
*start*
*stop*

##### *Return value*
*Array*  
*Bool* return `FALSE` if the key identify a non-list value.

##### *Example*
~~~
$redis->rPush('key1', 'A');
$redis->rPush('key1', 'B');
$redis->rPush('key1', 'C');
$redis->lRange('key1', 0, -1); /* array('A', 'B', 'C') */
$redis->lTrim('key1', 0, 1);
$redis->lRange('key1', 0, -1); /* array('A', 'B') */
~~~

### rPop
-----
_**Description**_: Returns and removes the last element of the list.

##### *Parameters*
*key*

##### *Return value*
*STRING* if command executed successfully
*BOOL* `FALSE` in case of failure (empty list)

##### *Example*
~~~
$redis->rPush('key1', 'A');
$redis->rPush('key1', 'B');
$redis->rPush('key1', 'C'); /* key1 => [ 'A', 'B', 'C' ] */
$redis->rPop('key1'); /* key1 => [ 'A', 'B' ] */
~~~

### rpoplpush
-----
_**Description**_: Pops a value from the tail of a list, and pushes it to the front of another list. Also return this value. (redis >= 1.1)

##### *Parameters*
*Key*: srckey  
*Key*: dstkey

##### *Return value*
*STRING* The element that was moved in case of success, `FALSE` in case of failure.

##### *Example*
~~~
$redis->delete('x', 'y');

$redis->lPush('x', 'abc');
$redis->lPush('x', 'def');
$redis->lPush('y', '123');
$redis->lPush('y', '456');

// move the last of x to the front of y.
var_dump($redis->rpoplpush('x', 'y'));
var_dump($redis->lRange('x', 0, -1));
var_dump($redis->lRange('y', 0, -1));

~~~
Output:
~~~
string(3) "abc"
array(1) {
  [0]=>
  string(3) "def"
}
array(3) {
  [0]=>
  string(3) "abc"
  [1]=>
  string(3) "456"
  [2]=>
  string(3) "123"
}
~~~

### rPush
-----
_**Description**_: Adds the string value to the tail (right) of the list. Creates the list if the key didn't exist. If the key exists and is not a list, `FALSE` is returned.

##### *Parameters*
*key*  
*value* String, value to push in key

##### *Return value*
*LONG* The new length of the list in case of success, `FALSE` in case of Failure.

##### *Examples*
~~~
$redis->delete('key1');
$redis->rPush('key1', 'A'); // returns 1
$redis->rPush('key1', 'B'); // returns 2
$redis->rPush('key1', 'C'); // returns 3
/* key1 now points to the following list: [ 'A', 'B', 'C' ] */
~~~

### rPushx
-----
_**Description**_: Adds the string value to the tail (right) of the list if the ist exists. `FALSE` in case of Failure.

##### *Parameters*
*key*  
*value* String, value to push in key

##### *Return value*
*LONG* The new length of the list in case of success, `FALSE` in case of Failure.

##### *Examples*
~~~
$redis->delete('key1');
$redis->rPushx('key1', 'A'); // returns 0
$redis->rPush('key1', 'A'); // returns 1
$redis->rPushx('key1', 'B'); // returns 2
$redis->rPushx('key1', 'C'); // returns 3
/* key1 now points to the following list: [ 'A', 'B', 'C' ] */
~~~

### lLen, lSize
-----
_**Description**_: Returns the size of a list identified by Key.

If the list didn't exist or is empty, the command returns 0. If the data type identified by Key is not a list, the command return `FALSE`.

##### *Parameters*
*Key*

##### *Return value*
*LONG* The size of the list identified by Key exists.  
*BOOL* `FALSE` if the data type identified by Key is not list

##### *Example*
~~~
$redis->rPush('key1', 'A');
$redis->rPush('key1', 'B');
$redis->rPush('key1', 'C'); /* key1 => [ 'A', 'B', 'C' ] */
$redis->lSize('key1');/* 3 */
$redis->rPop('key1'); 
$redis->lSize('key1');/* 2 */
~~~


## Sets

* [sAdd](#sadd) - Add one or more members to a set
* [sCard, sSize](#scard-ssize) - Get the number of members in a set
* [sDiff](#sdiff) - Subtract multiple sets
* [sDiffStore](#sdiffstore) - Subtract multiple sets and store the resulting set in a key
* [sInter](#sinter) - Intersect multiple sets
* [sInterStore](#sinterstore) - Intersect multiple sets and store the resulting set in a key
* [sIsMember, sContains](#sismember-scontains) - Determine if a given value is a member of a set
* [sMembers, sGetMembers](#smembers-sgetmembers) - Get all the members in a set
* [sMove](#smove) - Move a member from one set to another
* [sPop](#spop) - Remove and return a random member from a set
* [sRandMember](#srandmember) - Get one or multiple random members from a set
* [sRem, sRemove](#srem-sremove) - Remove one or more members from a set
* [sUnion](#sunion) - Add multiple sets
* [sUnionStore](#sunionstore) - Add multiple sets and store the resulting set in a key

### sAdd
-----
_**Description**_: Adds a value to the set value stored at key. If this value is already in the set, `FALSE` is returned.  
##### *Parameters*
*key*
*value*

##### *Return value*
*LONG* the number of elements added to the set.
##### *Example*
~~~
$redis->sAdd('key1' , 'member1'); /* 1, 'key1' => {'member1'} */
$redis->sAdd('key1' , 'member2', 'member3'); /* 2, 'key1' => {'member1', 'member2', 'member3'}*/
$redis->sAdd('key1' , 'member2'); /* 0, 'key1' => {'member1', 'member2', 'member3'}*/
~~~

### sCard, sSize
-----
_**Description**_: Returns the cardinality of the set identified by key.
##### *Parameters*
*key*
##### *Return value*
*LONG* the cardinality of the set identified by key, 0 if the set doesn't exist.
##### *Example*
~~~
$redis->sAdd('key1' , 'member1');
$redis->sAdd('key1' , 'member2');
$redis->sAdd('key1' , 'member3'); /* 'key1' => {'member1', 'member2', 'member3'}*/
$redis->sCard('key1'); /* 3 */
$redis->sCard('keyX'); /* 0 */
~~~

### sDiff
-----
_**Description**_: Performs the difference between N sets and returns it.

##### *Parameters*
*Keys*: key1, key2, ... , keyN: Any number of keys corresponding to sets in redis.

##### *Return value*
*Array of strings*: The difference of the first set will all the others.

##### *Example*
~~~
$redis->delete('s0', 's1', 's2');

$redis->sAdd('s0', '1');
$redis->sAdd('s0', '2');
$redis->sAdd('s0', '3');
$redis->sAdd('s0', '4');

$redis->sAdd('s1', '1');
$redis->sAdd('s2', '3');

var_dump($redis->sDiff('s0', 's1', 's2'));
~~~
Return value: all elements of s0 that are neither in s1 nor in s2.
~~~
array(2) {
  [0]=>
  string(1) "4"
  [1]=>
  string(1) "2"
}
~~~

### sDiffStore
-----
_**Description**_: Performs the same action as sDiff, but stores the result in the first key
##### *Parameters*
*Key*: dstkey, the key to store the diff into.

*Keys*: key1, key2, ... , keyN: Any number of keys corresponding to sets in redis
##### *Return value*
*INTEGER*: The cardinality of the resulting set, or `FALSE` in case of a missing key.

##### *Example*
~~~
$redis->delete('s0', 's1', 's2');

$redis->sAdd('s0', '1');
$redis->sAdd('s0', '2');
$redis->sAdd('s0', '3');
$redis->sAdd('s0', '4');

$redis->sAdd('s1', '1');
$redis->sAdd('s2', '3');

var_dump($redis->sDiffStore('dst', 's0', 's1', 's2'));
var_dump($redis->sMembers('dst'));
~~~
Return value: the number of elements of s0 that are neither in s1 nor in s2.
~~~
int(2)
array(2) {
  [0]=>
  string(1) "4"
  [1]=>
  string(1) "2"
}
~~~

### sInter
-----
_**Description**_: Returns the members of a set resulting from the intersection of all the sets held at the specified keys.

If just a single key is specified, then this command produces the members of this set. If one of the keys
is missing, `FALSE` is returned.

##### *Parameters*

key1, key2, keyN: keys identifying the different sets on which we will apply the intersection.
		
##### *Return value*

Array, contain the result of the intersection between those keys. If the intersection beteen the different sets is empty, the return value will be empty array.

##### *Examples*
~~~
$redis->sAdd('key1', 'val1');
$redis->sAdd('key1', 'val2');
$redis->sAdd('key1', 'val3');
$redis->sAdd('key1', 'val4');

$redis->sAdd('key2', 'val3');
$redis->sAdd('key2', 'val4');

$redis->sAdd('key3', 'val3');
$redis->sAdd('key3', 'val4');

var_dump($redis->sInter('key1', 'key2', 'key3'));
~~~

Output:

~~~
array(2) {
  [0]=>
  string(4) "val4"
  [1]=>
  string(4) "val3"
}
~~~

### sInterStore
-----
_**Description**_: Performs a sInter command and stores the result in a new set.
##### *Parameters*
*Key*: dstkey, the key to store the diff into.

*Keys*: key1, key2... keyN. key1..keyN are intersected as in sInter.

##### *Return value*
*INTEGER*: The cardinality of the resulting set, or `FALSE` in case of a missing key.

##### *Example*
~~~
$redis->sAdd('key1', 'val1');
$redis->sAdd('key1', 'val2');
$redis->sAdd('key1', 'val3');
$redis->sAdd('key1', 'val4');

$redis->sAdd('key2', 'val3');
$redis->sAdd('key2', 'val4');

$redis->sAdd('key3', 'val3');
$redis->sAdd('key3', 'val4');

var_dump($redis->sInterStore('output', 'key1', 'key2', 'key3'));
var_dump($redis->sMembers('output'));
~~~

Output:

~~~
int(2)

array(2) {
  [0]=>
  string(4) "val4"
  [1]=>
  string(4) "val3"
}
~~~

### sIsMember, sContains
-----
_**Description**_: Checks if `value` is a member of the set stored at the key `key`.
##### *Parameters*
*key*
*value*

##### *Return value*
*BOOL* `TRUE` if `value` is a member of the set at key `key`, `FALSE` otherwise.
##### *Example*
~~~
$redis->sAdd('key1' , 'member1');
$redis->sAdd('key1' , 'member2');
$redis->sAdd('key1' , 'member3'); /* 'key1' => {'member1', 'member2', 'member3'}*/

$redis->sIsMember('key1', 'member1'); /* TRUE */
$redis->sIsMember('key1', 'memberX'); /* FALSE */
~~~

### sMembers, sGetMembers
-----
_**Description**_: Returns the contents of a set.

##### *Parameters*
*Key*: key

##### *Return value*
An array of elements, the contents of the set.

##### *Example*
~~~
$redis->delete('s');
$redis->sAdd('s', 'a');
$redis->sAdd('s', 'b');
$redis->sAdd('s', 'a');
$redis->sAdd('s', 'c');
var_dump($redis->sMembers('s'));
~~~

Output:
~~~
array(3) {
  [0]=>
  string(1) "c"
  [1]=>
  string(1) "a"
  [2]=>
  string(1) "b"
}
~~~
The order is random and corresponds to redis' own internal representation of the set structure.

### sMove
-----
_**Description**_: Moves the specified member from the set at srcKey to the set at dstKey.
##### *Parameters*
*srcKey*
*dstKey*
*member*
##### *Return value*
*BOOL* If the operation is successful, return `TRUE`. If the srcKey and/or dstKey didn't exist, and/or the member didn't exist in srcKey, `FALSE` is returned.
##### *Example*
~~~
$redis->sAdd('key1' , 'member11');
$redis->sAdd('key1' , 'member12');
$redis->sAdd('key1' , 'member13'); /* 'key1' => {'member11', 'member12', 'member13'}*/
$redis->sAdd('key2' , 'member21');
$redis->sAdd('key2' , 'member22'); /* 'key2' => {'member21', 'member22'}*/
$redis->sMove('key1', 'key2', 'member13'); /* 'key1' =>  {'member11', 'member12'} */
					/* 'key2' =>  {'member21', 'member22', 'member13'} */

~~~

### sPop
-----
_**Description**_: Removes and returns a random element from the set value at Key.
##### *Parameters*
*key*
##### *Return value*
*String* "popped" value  
*Bool* `FALSE` if set identified by key is empty or doesn't exist.
##### *Example*
~~~
$redis->sAdd('key1' , 'member1');
$redis->sAdd('key1' , 'member2');
$redis->sAdd('key1' , 'member3'); /* 'key1' => {'member3', 'member1', 'member2'}*/
$redis->sPop('key1'); /* 'member1', 'key1' => {'member3', 'member2'} */
$redis->sPop('key1'); /* 'member3', 'key1' => {'member2'} */
~~~

### sRandMember
-----
_**Description**_: Returns a random element from the set value at Key, without removing it.
##### *Parameters*
*key*  
*count* (Integer, optional)  
##### *Return value*
If no count is provided, a random *String* value from the set will be returned.  If a count
is provided, an array of values from the set will be returned.  Read about the different
ways to use the count here: [SRANDMEMBER](http://redis.io/commands/srandmember)  
*Bool* `FALSE` if set identified by key is empty or doesn't exist.
##### *Example*
~~~
$redis->sAdd('key1' , 'member1');
$redis->sAdd('key1' , 'member2');
$redis->sAdd('key1' , 'member3'); /* 'key1' => {'member3', 'member1', 'member2'}*/

// No count
$redis->sRandMember('key1'); /* 'member1', 'key1' => {'member3', 'member1', 'member2'} */
$redis->sRandMember('key1'); /* 'member3', 'key1' => {'member3', 'member1', 'member2'} */

// With a count
$redis->sRandMember('key1', 3); // Will return an array with all members from the set
$redis->sRandMember('key1', 2); // Will an array with 2 members of the set
$redis->sRandMember('key1', -100); // Will return an array of 100 elements, picked from our set (with dups)
$redis->sRandMember('empty-set', 100); // Will return an empty array
$redis->sRandMember('not-a-set', 100); // Will return FALSE
~~~

### sRem, sRemove
-----
_**Description**_: Removes the specified member from the set value stored at key.
##### *Parameters*
*key*
*member*
##### *Return value*
*LONG* The number of elements removed from the set.
##### *Example*
~~~
$redis->sAdd('key1' , 'member1');
$redis->sAdd('key1' , 'member2');
$redis->sAdd('key1' , 'member3'); /* 'key1' => {'member1', 'member2', 'member3'}*/
$redis->sRem('key1', 'member2', 'member3'); /*return 2. 'key1' => {'member1'} */
~~~

### sUnion
-----
_**Description**_: Performs the union between N sets and returns it.

##### *Parameters*
*Keys*: key1, key2, ... , keyN: Any number of keys corresponding to sets in redis.

##### *Return value*
*Array of strings*: The union of all these sets.

##### *Example*
~~~
$redis->delete('s0', 's1', 's2');

$redis->sAdd('s0', '1');
$redis->sAdd('s0', '2');
$redis->sAdd('s1', '3');
$redis->sAdd('s1', '1');
$redis->sAdd('s2', '3');
$redis->sAdd('s2', '4');

var_dump($redis->sUnion('s0', 's1', 's2'));
~~~
Return value: all elements that are either in s0 or in s1 or in s2.
~~~
array(4) {
  [0]=>
  string(1) "3"
  [1]=>
  string(1) "4"
  [2]=>
  string(1) "1"
  [3]=>
  string(1) "2"
}
~~~

### sUnionStore
-----
_**Description**_: Performs the same action as sUnion, but stores the result in the first key

##### *Parameters*
*Key*: dstkey, the key to store the diff into.

*Keys*: key1, key2, ... , keyN: Any number of keys corresponding to sets in redis.

##### *Return value*
*INTEGER*: The cardinality of the resulting set, or `FALSE` in case of a missing key.

##### *Example*
~~~
$redis->delete('s0', 's1', 's2');

$redis->sAdd('s0', '1');
$redis->sAdd('s0', '2');
$redis->sAdd('s1', '3');
$redis->sAdd('s1', '1');
$redis->sAdd('s2', '3');
$redis->sAdd('s2', '4');

var_dump($redis->sUnionStore('dst', 's0', 's1', 's2'));
var_dump($redis->sMembers('dst'));
~~~
Return value: the number of elements that are either in s0 or in s1 or in s2.
~~~
int(4)
array(4) {
  [0]=>
  string(1) "3"
  [1]=>
  string(1) "4"
  [2]=>
  string(1) "1"
  [3]=>
  string(1) "2"
}
~~~


## Sorted sets

* [zAdd](#zadd) - Add one or more members to a sorted set or update its score if it already exists
* [zCard, zSize](#zcard-zsize) - Get the number of members in a sorted set
* [zCount](#zcount) - Count the members in a sorted set with scores within the given values
* [zIncrBy](#zincrby) - Increment the score of a member in a sorted set
* [zInter](#zinter) - Intersect multiple sorted sets and store the resulting sorted set in a new key
* [zRange](#zrange) - Return a range of members in a sorted set, by index
* [zRangeByScore, zRevRangeByScore](#zrangebyscore-zrevrangebyscore) - Return a range of members in a sorted set, by score
* [zRank, zRevRank](#zrank-zrevrank) - Determine the index of a member in a sorted set
* [zRem, zDelete](#zrem-zdelete) - Remove one or more members from a sorted set
* [zRemRangeByRank, zDeleteRangeByRank](#zremrangebyrank-zdeleterangebyrank) - Remove all members in a sorted set within the given indexes
* [zRemRangeByScore, zDeleteRangeByScore](#zremrangebyscore-zdeleterangebyscore) - Remove all members in a sorted set within the given scores
* [zRevRange](#zrevrange) - Return a range of members in a sorted set, by index, with scores ordered from high to low
* [zScore](#zscore) - Get the score associated with the given member in a sorted set
* [zUnion](#zunion) - Add multiple sorted sets and store the resulting sorted set in a new key

### zAdd
-----
_**Description**_: Add one or more members to a sorted set or update its score if it already exists

##### *Parameters*
*key*  
*score* : double  
*value*: string  

##### *Return value*
*Long* 1 if the element is added. 0 otherwise.

##### *Example*
~~~
$redis->zAdd('key', 1, 'val1');
$redis->zAdd('key', 0, 'val0');
$redis->zAdd('key', 5, 'val5');
$redis->zRange('key', 0, -1); // array(val0, val1, val5)
~~~

### zCard, zSize
-----
_**Description**_: Returns the cardinality of an ordered set.

##### *Parameters*
*key*

##### *Return value*
*Long*, the set's cardinality

##### *Example*
~~~
$redis->zAdd('key', 0, 'val0');
$redis->zAdd('key', 2, 'val2');
$redis->zAdd('key', 10, 'val10');
$redis->zSize('key'); /* 3 */
~~~

### zCount
-----
_**Description**_: Returns the *number* of elements of the sorted set stored at the specified key which have scores in the range [start,end]. Adding a parenthesis before `start` or `end` excludes it from the range. +inf and -inf are also valid limits.

##### *Parameters*
*key*  
*start*: string  
*end*: string  

##### *Return value*
*LONG* the size of a corresponding zRangeByScore.  

##### *Example*
~~~
$redis->zAdd('key', 0, 'val0');
$redis->zAdd('key', 2, 'val2');
$redis->zAdd('key', 10, 'val10');
$redis->zCount('key', 0, 3); /* 2, corresponding to array('val0', 'val2') */
~~~

### zIncrBy
-----
_**Description**_: Increments the score of a member from a sorted set by a given amount.

##### *Parameters*
*key*  
*value*: (double) value that will be added to the member's score  
*member*  

##### *Return value*
*DOUBLE* the new value

##### *Examples*
~~~
$redis->delete('key');
$redis->zIncrBy('key', 2.5, 'member1'); /* key or member1 didn't exist, so member1's score is to 0 before the increment */
					  /* and now has the value 2.5  */
$redis->zIncrBy('key', 1, 'member1'); /* 3.5 */
~~~

### zInter
-----
_**Description**_: Creates an intersection of sorted sets given in second argument. The result of the union will be stored in the sorted set defined by the first argument.

The third optionnel argument defines `weights` to apply to the sorted sets in input. In this case, the `weights` will be multiplied by the score of each element in the sorted set before applying the aggregation.
The forth argument defines the `AGGREGATE` option which specify how the results of the union are aggregated.

##### *Parameters*
*keyOutput*  
*arrayZSetKeys*  
*arrayWeights*  
*aggregateFunction* Either "SUM", "MIN", or "MAX": defines the behaviour to use on duplicate entries during the zInter.  

##### *Return value*
*LONG* The number of values in the new sorted set.

##### *Example*
~~~
$redis->delete('k1');
$redis->delete('k2');
$redis->delete('k3');

$redis->delete('ko1');
$redis->delete('ko2');
$redis->delete('ko3');
$redis->delete('ko4');

$redis->zAdd('k1', 0, 'val0');
$redis->zAdd('k1', 1, 'val1');
$redis->zAdd('k1', 3, 'val3');

$redis->zAdd('k2', 2, 'val1');
$redis->zAdd('k2', 3, 'val3');

$redis->zInter('ko1', array('k1', 'k2')); 				/* 2, 'ko1' => array('val1', 'val3') */
$redis->zInter('ko2', array('k1', 'k2'), array(1, 1)); 	/* 2, 'ko2' => array('val1', 'val3') */

/* Weighted zInter */
$redis->zInter('ko3', array('k1', 'k2'), array(1, 5), 'min'); /* 2, 'ko3' => array('val1', 'val3') */
$redis->zInter('ko4', array('k1', 'k2'), array(1, 5), 'max'); /* 2, 'ko4' => array('val3', 'val1') */
~~~

### zRange
-----
_**Description**_: Returns a range of elements from the ordered set stored at the specified key, with values in the range [start, end].

Start and stop are interpreted as zero-based indices:
0 the first element, 1 the second ...
-1 the last element, -2 the penultimate ...

##### *Parameters*
*key*  
*start*: long  
*end*: long  
*withscores*: bool = false  

##### *Return value*
*Array* containing the values in specified range. 

##### *Example*
~~~
$redis->zAdd('key1', 0, 'val0');
$redis->zAdd('key1', 2, 'val2');
$redis->zAdd('key1', 10, 'val10');
$redis->zRange('key1', 0, -1); /* array('val0', 'val2', 'val10') */

// with scores
$redis->zRange('key1', 0, -1, true); /* array('val0' => 0, 'val2' => 2, 'val10' => 10) */
~~~

### zRangeByScore, zRevRangeByScore
-----
_**Description**_: Returns the elements of the sorted set stored at the specified key which have scores in the range [start,end]. Adding a parenthesis before `start` or `end` excludes it from the range. +inf and -inf are also valid limits. zRevRangeByScore returns the same items in reverse order, when the `start` and `end` parameters are swapped.

##### *Parameters*
*key*  
*start*: string  
*end*: string  
*options*: array  

Two options are available: `withscores => TRUE`, and `limit => array($offset, $count)`

##### *Return value*
*Array* containing the values in specified range. 

##### *Example*
~~~
$redis->zAdd('key', 0, 'val0');
$redis->zAdd('key', 2, 'val2');
$redis->zAdd('key', 10, 'val10');
$redis->zRangeByScore('key', 0, 3); /* array('val0', 'val2') */
$redis->zRangeByScore('key', 0, 3, array('withscores' => TRUE); /* array('val0' => 0, 'val2' => 2) */
$redis->zRangeByScore('key', 0, 3, array('limit' => array(1, 1)); /* array('val2') */
$redis->zRangeByScore('key', 0, 3, array('withscores' => TRUE, 'limit' => array(1, 1)); /* array('val2' => 2) */
~~~

### zRank, zRevRank
-----
_**Description**_: Returns the rank of a given member in the specified sorted set, starting at 0 for the item with the smallest score. zRevRank starts at 0 for the item with the *largest* score.

##### *Parameters*
*key*  
*member*  

##### *Return value*
*Long*, the item's score.

##### *Example*
~~~
$redis->delete('z');
$redis->zAdd('key', 1, 'one');
$redis->zAdd('key', 2, 'two');
$redis->zRank('key', 'one'); /* 0 */
$redis->zRank('key', 'two'); /* 1 */
$redis->zRevRank('key', 'one'); /* 1 */
$redis->zRevRank('key', 'two'); /* 0 */
~~~

### zRem, zDelete
-----
_**Description**_: Deletes a specified member from the ordered set.

##### *Parameters*
*key*  
*member*  

##### *Return value*
*LONG* 1 on success, 0 on failure.

##### *Example*
~~~
$redis->zAdd('key', 0, 'val0');
$redis->zAdd('key', 2, 'val2');
$redis->zAdd('key', 10, 'val10');
$redis->zDelete('key', 'val2');
$redis->zRange('key', 0, -1); /* array('val0', 'val10') */
~~~

### zRemRangeByRank, zDeleteRangeByRank
-----
_**Description**_: Deletes the elements of the sorted set stored at the specified key which have rank in the range [start,end].

##### *Parameters*
*key*  
*start*: LONG  
*end*: LONG  

##### *Return value*
*LONG* The number of values deleted from the sorted set

##### *Example*
~~~
$redis->zAdd('key', 1, 'one');
$redis->zAdd('key', 2, 'two');
$redis->zAdd('key', 3, 'three');
$redis->zRemRangeByRank('key', 0, 1); /* 2 */
$redis->zRange('key', 0, -1, array('withscores' => TRUE)); /* array('three' => 3) */
~~~

### zRemRangeByScore, zDeleteRangeByScore
-----
_**Description**_: Deletes the elements of the sorted set stored at the specified key which have scores in the range [start,end].

##### *Parameters*
*key*  
*start*: double or "+inf" or "-inf" string  
*end*: double or "+inf" or "-inf" string  

##### *Return value*
*LONG* The number of values deleted from the sorted set

##### *Example*
~~~
$redis->zAdd('key', 0, 'val0');
$redis->zAdd('key', 2, 'val2');
$redis->zAdd('key', 10, 'val10');
$redis->zRemRangeByScore('key', 0, 3); /* 2 */
~~~

### zRevRange
-----
_**Description**_: Returns the elements of the sorted set stored at the specified key in the range [start, end] in reverse order. start and stop are interpretated as zero-based indices:
0 the first element, 1 the second ...
-1 the last element, -2 the penultimate ...

##### *Parameters*
*key*  
*start*: long  
*end*: long  
*withscores*: bool = false  

##### *Return value*
*Array* containing the values in specified range. 

##### *Example*
~~~
$redis->zAdd('key', 0, 'val0');
$redis->zAdd('key', 2, 'val2');
$redis->zAdd('key', 10, 'val10');
$redis->zRevRange('key', 0, -1); /* array('val10', 'val2', 'val0') */

// with scores
$redis->zRevRange('key', 0, -1, true); /* array('val10' => 10, 'val2' => 2, 'val0' => 0) */
~~~

### zScore
-----
_**Description**_: Returns the score of a given member in the specified sorted set.

##### *Parameters*
*key*  
*member*  

##### *Return value*
*Double*

##### *Example*
~~~
$redis->zAdd('key', 2.5, 'val2');
$redis->zScore('key', 'val2'); /* 2.5 */
~~~

### zUnion
-----
_**Description**_: Creates an union of sorted sets given in second argument. The result of the union will be stored in the sorted set defined by the first argument.

The third optionnel argument defines `weights` to apply to the sorted sets in input. In this case, the `weights` will be multiplied by the score of each element in the sorted set before applying the aggregation.
The forth argument defines the `AGGREGATE` option which specify how the results of the union are aggregated.

##### *Parameters*
*keyOutput*  
*arrayZSetKeys*  
*arrayWeights*  
*aggregateFunction* Either "SUM", "MIN", or "MAX": defines the behaviour to use on duplicate entries during the zUnion.  

##### *Return value*
*LONG* The number of values in the new sorted set.

##### *Example*
~~~
$redis->delete('k1');
$redis->delete('k2');
$redis->delete('k3');
$redis->delete('ko1');
$redis->delete('ko2');
$redis->delete('ko3');

$redis->zAdd('k1', 0, 'val0');
$redis->zAdd('k1', 1, 'val1');

$redis->zAdd('k2', 2, 'val2');
$redis->zAdd('k2', 3, 'val3');

$redis->zUnion('ko1', array('k1', 'k2')); /* 4, 'ko1' => array('val0', 'val1', 'val2', 'val3') */

/* Weighted zUnion */
$redis->zUnion('ko2', array('k1', 'k2'), array(1, 1)); /* 4, 'ko2' => array('val0', 'val1', 'val2', 'val3') */
$redis->zUnion('ko3', array('k1', 'k2'), array(5, 1)); /* 4, 'ko3' => array('val0', 'val2', 'val3', 'val1') */
~~~

## Pub/sub

* [psubscribe](#psubscribe) - Subscribe to channels by pattern
* [publish](#publish) - Post a message to a channel
* [subscribe](#subscribe) - Subscribe to channels

### psubscribe
-----
_**Description**_: Subscribe to channels by pattern

##### *Parameters*
*patterns*: An array of patterns to match
*callback*: Either a string or an array with an object and method.  The callback will get four arguments ($redis, $pattern, $channel, $message)

##### *Example*
~~~
function psubscribe($redis, $pattern, $chan, $msg) {
	echo "Pattern: $pattern\n";
	echo "Channel: $chan\n";
	echo "Payload: $msg\n";
}
~~~

### publish
-----
_**Description**_: Publish messages to channels. Warning: this function will probably change in the future.

##### *Parameters*
*channel*: a channel to publish to  
*messsage*: string  

##### *Example*
~~~
$redis->publish('chan-1', 'hello, world!'); // send message.
~~~

### subscribe
-----
_**Description**_: Subscribe to channels. Warning: this function will probably change in the future.

##### *Parameters*
*channels*: an array of channels to subscribe to  
*callback*: either a string or an array($instance, 'method_name'). The callback function receives 3 parameters: the redis instance, the channel name, and the message.  

##### *Example*
~~~
function f($redis, $chan, $msg) {
	switch($chan) {
		case 'chan-1':
			...
			break;

		case 'chan-2':
			...
			break;

		case 'chan-2':
			...
			break;
	}
}

$redis->subscribe(array('chan-1', 'chan-2', 'chan-3'), 'f'); // subscribe to 3 chans
~~~


## Transactions

1. [multi, exec, discard](#multi-exec-discard) - Enter and exit transactional mode
2. [watch, unwatch](#watch-unwatch) - Watches a key for modifications by another client.

### multi, exec, discard.
-----
_**Description**_: Enter and exit transactional mode.  

##### *Parameters*
(optional) `Redis::MULTI` or `Redis::PIPELINE`. Defaults to `Redis::MULTI`. A `Redis::MULTI` block of commands runs as a single transaction; a `Redis::PIPELINE` block is simply transmitted faster to the server, but without any guarantee of atomicity. `discard` cancels a transaction.  

##### *Return value*
`multi()` returns the Redis instance and enters multi-mode. Once in multi-mode, all subsequent method calls return the same object until `exec()` is called.

##### *Example*
~~~
$ret = $redis->multi()
    ->set('key1', 'val1')
    ->get('key1')
    ->set('key2', 'val2')
    ->get('key2')
    ->exec();

/*
$ret == array(
    0 => TRUE,
    1 => 'val1',
    2 => TRUE,
    3 => 'val2');
*/
~~~

### watch, unwatch
-----
_**Description**_: Watches a key for modifications by another client.

If the key is modified between `WATCH` and `EXEC`, the MULTI/EXEC transaction will fail (return `FALSE`). `unwatch` cancels all the watching of all keys by this client.

##### *Parameters*
*keys*: a list of keys

##### *Example*
~~~
$redis->watch('x');
/* long code here during the execution of which other clients could well modify `x` */
$ret = $redis->multi()
    ->incr('x')
    ->exec();
/*
$ret = FALSE if x has been modified between the call to WATCH and the call to EXEC.
*/
~~~



## Scripting

* [eval](#) - Evaluate a LUA script serverside
* [evalSha](#) - Evaluate a LUA script serverside, from the SHA1 hash of the script instead of the script itself
* [script](#) - Execute the Redis SCRIPT command to perform various operations on the scripting subsystem
* [getLastError](#) - The last error message (if any)
* [clearLastError](#) - Clear the last error message
* [_prefix](#) - A utility method to prefix the value with the prefix setting for phpredis
* [_unserialize](#) - A utility method to unserialize data with whatever serializer is set up

### eval
-----
_**Description**_: Evaluate a LUA script serverside

##### *Parameters*
*script* string.  
*args* array, optional.  
*num_keys* int, optional.  

##### *Return value*
Mixed.  What is returned depends on what the LUA script itself returns, which could be a scalar value (int/string), or an array.
Arrays that are returned can also contain other arrays, if that's how it was set up in your LUA script.  If there is an error
executing the LUA script, the getLastError() function can tell you the message that came back from Redis (e.g. compile error).

##### *Examples*
~~~
$redis->eval("return 1"); // Returns an integer: 1
$redis->eval("return {1,2,3}"); // Returns Array(1,2,3)
$redis->del('mylist');
$redis->rpush('mylist','a');
$redis->rpush('mylist','b');
$redis->rpush('mylist','c');
// Nested response:  Array(1,2,3,Array('a','b','c'));
$redis->eval("return {1,2,3,redis.call('lrange','mylist',0,-1)}}");
~~~

### evalSha
-----
_**Description**_: Evaluate a LUA script serverside, from the SHA1 hash of the script instead of the script itself.

In order to run this command Redis will have to have already loaded the script,
either by running it or via the SCRIPT LOAD command.

##### *Parameters*
*script_sha* string.  The sha1 encoded hash of the script you want to run.  
*args* array, optional.  Arguments to pass to the LUA script.  
*num_keys* int, optional.  The number of arguments that should go into the KEYS array, vs. the ARGV array when Redis spins the script  

##### *Return value*
Mixed.  See EVAL

##### *Examples*
~~~
$script = 'return 1';
$sha = $redis->script('load', $script);
$redis->evalSha($sha); // Returns 1
~~~

### script
-----
_**Description**_: Execute the Redis SCRIPT command to perform various operations on the scripting subsystem.

##### *Usage*
~~~
$redis->script('load', $script);
$redis->script('flush');
$redis->script('kill');
$redis->script('exists', $script1, [$script2, $script3, ...]);
~~~

##### *Return value*
* SCRIPT LOAD will return the SHA1 hash of the passed script on success, and FALSE on failure.
* SCRIPT FLUSH should always return TRUE
* SCRIPT KILL will return true if a script was able to be killed and false if not
* SCRIPT EXISTS will return an array with TRUE or FALSE for each passed script

### client
-----
_**Description**_: Issue the CLIENT command with various arguments.

The Redis CLIENT command can be used in four ways.  
* CLIENT LIST
* CLIENT GETNAME
* CLIENT SETNAME [name]
* CLIENT KILL [ip:port]

##### *Usage*
~~~
$redis->client('list'); // Get a list of clients
$redis->client('getname'); // Get the name of the current connection
$redis->client('setname', 'somename'); // Set the name of the current connection
$redis->client('kill', <ip:port>); // Kill the process at ip:port
~~~

##### *Return value* 
This will vary depending on which client command was executed.  

* CLIENT LIST will return an array of arrays with client information.
* CLIENT GETNAME will return the client name or false if none has been set
* CLIENT SETNAME will return true if it can be set and false if not
* CLIENT KILL will return true if the client can be killed, and false if not

Note:  phpredis will attempt to reconnect so you can actually kill your own connection
but may not notice losing it!
### getLastError
-----
_**Description**_: The last error message (if any)

##### *Parameters*
*none*  

##### *Return value*
A string with the last returned script based error message, or NULL if there is no error

##### *Examples*
~~~
$redis->eval('this-is-not-lua');
$err = $redis->getLastError(); 
// "ERR Error compiling script (new function): user_script:1: '=' expected near '-'"
~~~

### clearLastError
-----
_**Description**_: Clear the last error message  

##### *Parameters*
*none*  

##### *Return value*
*BOOL* TRUE  

##### *Examples*
~~~
$redis->set('x', 'a');
$redis->incr('x');
$err = $redis->getLastError();
// "ERR value is not an integer or out of range"
$redis->clearLastError();
$err = $redis->getLastError();
// NULL
~~~

### _prefix
-----
_**Description**_: A utility method to prefix the value with the prefix setting for phpredis.

##### *Parameters*
*value* string.  The value you wish to prefix  

##### *Return value*
If a prefix is set up, the value now prefixed.  If there is no prefix, the value will be returned unchanged.

##### *Examples*
~~~
$redis->setOption(Redis::OPT_PREFIX, 'my-prefix:');
$redis->_prefix('my-value'); // Will return 'my-prefix:my-value'
~~~

### _unserialize
-----
_**Description**_: A utility method to unserialize data with whatever serializer is set up.

If there is no serializer set, the value will be returned unchanged.  If there is a serializer set up,
and the data passed in is malformed, an exception will be thrown. This can be useful if phpredis is
serializing values, and you return something from redis in a LUA script that is serialized.

##### *Parameters*
*value* string.  The value to be unserialized  

##### *Examples*
~~~
$redis->setOption(Redis::OPT_SERIALIZER, Redis::SERIALIZER_PHP);
$redis->_unserialize('a:3:{i:0;i:1;i:1;i:2;i:2;i:3;}'); // Will return Array(1,2,3)
~~~



## Introspection  

### IsConnected
-----
_**Description**_:  A method to determine if a phpredis object thinks it's connected to a server

##### *Parameters*
None  

##### *Return value*
*Boolean* Returns TRUE if phpredis thinks it's connected and FALSE if not

### GetHost
-----
_**Description**_:  Retreive our host or unix socket that we're connected to

##### *Parameters*
None  

##### *Return value*
*Mixed* The host or unix socket we're connected to or FALSE if we're not connected


### GetPort
-----
_**Description**_:  Get the port we're connected to

##### *Parameters*
None  

##### *Return value*
*Mixed* Returns the port we're connected to or FALSE if we're not connected

### getDBNum
-----
_**Description**_:  Get the database number phpredis is pointed to

##### *Parameters*
None  

##### *Return value*
*Mixed* Returns the database number (LONG) phpredis thinks it's pointing to or FALSE if we're not connected

### GetTimeout
-----
_**Description**_:  Get the (write) timeout in use for phpreids

##### *Parameters*
None  

##### *Return value*
*Mixed* The timeout (DOUBLE) specified in our connect call or FALSE if we're not connected

### GetReadTimeout
_**Description**_:  Get the read timeout specified to phpredis or FALSE if we're not connected

##### *Parameters*
None  

##### *Return value*
*Mixed*  Returns the read timeout (which can be set using setOption and Redis::OPT_READ_TIMOUT) or FALSE if we're not connected

### GetPersistentID
-----
_**Description**_:  Gets the persistent ID that phpredis is using

##### *Parameters*
None  

##### *Return value*
*Mixed* Returns the persistent id phpredis is using (which will only be set if connected with pconnect), NULL if we're not
using a persistent ID, and FALSE if we're not connected

### GetAuth
-----
_**Description**_:  Get the password used to authenticate the phpredis connection

### *Parameters*
None  

### *Return value*
*Mixed*  Returns the password used to authenticate a phpredis session or NULL if none was used, and FALSE if we're not connected
