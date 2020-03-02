# PhpRedis

[![Build Status](https://travis-ci.org/phpredis/phpredis.svg?branch=develop)](https://travis-ci.org/phpredis/phpredis)
[![Coverity Scan Build Status](https://scan.coverity.com/projects/13205/badge.svg)](https://scan.coverity.com/projects/phpredis-phpredis)

The phpredis extension provides an API for communicating with the [Redis](http://redis.io/) key-value store. It is released under the [PHP License, version 3.01](http://www.php.net/license/3_01.txt).
This code has been developed and maintained by Owlient from November 2009 to March 2011.

You can send comments, patches, questions [here on github](https://github.com/phpredis/phpredis/issues), to n.favrefelix@gmail.com ([@yowgi](https://twitter.com/yowgi)), to michael.grunder@gmail.com ([@grumi78](https://twitter.com/grumi78)) or to p.yatsukhnenko@gmail.com ([@yatsukhnenko](https://twitter.com/yatsukhnenko)).

## Supporting the project
PhpRedis will always be free and open source software, but if you or your company has found it useful please consider supporting the project.  Developing a large, complex, and performant library like PhpRedis takes a great deal of time and effort, and support would be appreciated! :heart:

The best way to support the project is through [GitHub sponsors](https://github.com/sponsors/michael-grunder).  Many of the reward tiers grant access to our [slack channel](https://phpredis.slack.com) where [myself](https://github.com/michael-grunder) and [Pavlo](https://github.com/yatsukhnenko) are regularly available to answer questions.  Additionally this will allow you to provide feedback on which fixes and new features to prioritize.

You can also make a one-time contribution with one of the links below.

[![PayPal](https://img.shields.io/badge/Donate-PayPal-green.svg)](https://www.paypal.me/michaelgrunder/5)
[![Bitcoin](https://en.cryptobadges.io/badge/micro/1FXkYHBo5uoaztxFbajiPfbnkgKCbF3ykG)](https://en.cryptobadges.io/donate/1FXkYHBo5uoaztxFbajiPfbnkgKCbF3ykG)
[![Ethereum](https://en.cryptobadges.io/badge/micro/0x43D54E32357B96f68dFF0a6B46976d014Bd603E1)](https://en.cryptobadges.io/donate/0x43D54E32357B96f68dFF0a6B46976d014Bd603E1)

## Sponsors

<a href="https://audiomack.com">
    <img src="https://styleguide.audiomack.com/styleguide/assets/dl/inline-orange-large.png" alt="Audiomack.com" width="150">
</a>

# Table of contents
-----
1. [Installing/Configuring](#installingconfiguring)
   * [Installation](#installation)
   * [PHP Session handler](#php-session-handler)
   * [Distributed Redis Array](#distributed-redis-array)
   * [Redis Cluster support](#redis-cluster-support)
   * [Redis Sentinel support](#redis-sentinel-support)
   * [Running the unit tests](#running-the-unit-tests)
1. [Classes and methods](#classes-and-methods)
   * [Usage](#usage)
   * [Connection](#connection)
   * [Server](#server)
   * [Keys and strings](#keys-and-strings)
   * [Hashes](#hashes)
   * [Lists](#lists)
   * [Sets](#sets)
   * [Sorted sets](#sorted-sets)
   * [HyperLogLogs](#hyperloglogs)
   * [Geocoding](#geocoding)
   * [Streams](#streams)
   * [Pub/sub](#pubsub)
   * [Transactions](#transactions)
   * [Scripting](#scripting)
   * [Introspection](#introspection)

-----

# Installing/Configuring
-----

## Installation

For everything you should need to install PhpRedis on your system,
see the [INSTALL.markdown](./INSTALL.markdown) page.

## PHP Session handler

phpredis can be used to store PHP sessions. To do this, configure `session.save_handler` and `session.save_path` in your php.ini to tell phpredis where to store the sessions:
~~~
session.save_handler = redis
session.save_path = "tcp://host1:6379?weight=1, tcp://host2:6379?weight=2&timeout=2.5, tcp://host3:6379?weight=2&read_timeout=2.5"
~~~

`session.save_path` can have a simple `host:port` format too, but you need to provide the `tcp://` scheme if you want to use the parameters. The following parameters are available:

* weight (integer): the weight of a host is used in comparison with the others in order to customize the session distribution on several hosts. If host A has twice the weight of host B, it will get twice the amount of sessions. In the example, *host1* stores 20% of all the sessions (1/(1+2+2)) while *host2* and *host3* each store 40% (2/(1+2+2)). The target host is determined once and for all at the start of the session, and doesn't change. The default weight is 1.
* timeout (float): the connection timeout to a redis host, expressed in seconds. If the host is unreachable in that amount of time, the session storage will be unavailable for the client. The default timeout is very high (86400 seconds).
* persistent (integer, should be 1 or 0): defines if a persistent connection should be used. **(experimental setting)**
* prefix (string, defaults to "PHPREDIS_SESSION:"): used as a prefix to the Redis key in which the session is stored. The key is composed of the prefix followed by the session ID.
* auth (string, empty by default): used to authenticate with the server prior to sending commands.
* database (integer): selects a different database.

Sessions have a lifetime expressed in seconds and stored in the INI variable "session.gc_maxlifetime". You can change it with [`ini_set()`](http://php.net/ini_set).
The session handler requires a version of Redis supporting `EX` and `NX` options of `SET` command (at least 2.6.12).
phpredis can also connect to a unix domain socket: `session.save_path = "unix:///var/run/redis/redis.sock?persistent=1&weight=1&database=0`.

### Session locking

**Support**: Locking feature is currently only supported for Redis setup with single master instance (e.g. classic master/slave Sentinel environment).
So locking may not work properly in RedisArray or RedisCluster environments.

Following INI variables can be used to configure session locking:
~~~
; Should the locking be enabled? Defaults to: 0.
redis.session.locking_enabled = 1
; How long should the lock live (in seconds)? Defaults to: value of max_execution_time.
redis.session.lock_expire = 60
; How long to wait between attempts to acquire lock, in microseconds (µs)?. Defaults to: 2000
redis.session.lock_wait_time = 50000
; Maximum number of times to retry (-1 means infinite). Defaults to: 10
redis.session.lock_retries = 10
~~~

## Distributed Redis Array

See [dedicated page](./arrays.markdown#readme).

## Redis Cluster support

See [dedicated page](./cluster.markdown#readme).

## Redis Sentinel support

See [dedicated page](./sentinel.markdown#readme).

## Running the unit tests

phpredis uses a small custom unit test suite for testing functionality of the various classes.  To run tests, simply do the following:

~~~
# Run tests for Redis class (note this is the default)
php tests/TestRedis.php --class Redis

# Run tests for RedisArray class
tests/mkring.sh start
php tests/TestRedis.php --class RedisArray
tests/mkring.sh stop

# Run tests for the RedisCluster class
tests/make-cluster.sh start
php tests/TestRedis.php --class RedisCluster
tests/make-cluster.sh stop

# Run tests for RedisSentinel class
php tests/TestRedis.php --class RedisSentinel
~~~

Note that it is possible to run only tests which match a substring of the test itself by passing the additional argument '--test <str>' when invoking.

~~~
# Just run the 'echo' test
php tests/TestRedis.php --class Redis --test echo
~~~

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

~~~php
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
1. [swapdb](#swapdb) - Swaps two Redis databases
1. [close](#close) - Close the connection
1. [setOption](#setoption) - Set client option
1. [getOption](#getoption) - Get client option
1. [ping](#ping) - Ping the server
1. [echo](#echo) - Echo the given string

### connect, open
-----
_**Description**_: Connects to a Redis instance.

##### *Parameters*

*host*: string. can be a host, or the path to a unix domain socket. Starting from version 5.0.0 it is possible to specify schema 
*port*: int, optional  
*timeout*: float, value in seconds (optional, default is 0 meaning unlimited)  
*reserved*: should be NULL if retry_interval is specified  
*retry_interval*: int, value in milliseconds (optional)  
*read_timeout*: float, value in seconds (optional, default is 0 meaning unlimited)

##### *Return value*

*BOOL*: `TRUE` on success, `FALSE` on error.

##### *Example*

~~~php
$redis->connect('127.0.0.1', 6379);
$redis->connect('127.0.0.1'); // port 6379 by default
$redis->connect('tls://127.0.0.1', 6379); // enable transport level security.
$redis->connect('tls://127.0.0.1'); // enable transport level security, port 6379 by default.
$redis->connect('127.0.0.1', 6379, 2.5); // 2.5 sec timeout.
$redis->connect('/tmp/redis.sock'); // unix domain socket.
$redis->connect('127.0.0.1', 6379, 1, NULL, 100); // 1 sec timeout, 100ms delay between reconnection attempts.
~~~

**Note:** `open` is an alias for `connect` and will be removed in future versions of phpredis.

### pconnect, popen
-----
_**Description**_: Connects to a Redis instance or reuse a connection already established with `pconnect`/`popen`.

The connection will not be closed on end of request until the php process ends.
So be prepared for too many open FD's errors (specially on redis server side) when using persistent
connections on many servers connecting to one redis server.

Also more than one persistent connection can be made identified by either host + port + timeout
or host + persistent_id or unix socket + timeout.

Starting from version 4.2.1, it became possible to use connection pooling by setting INI variable `redis.pconnect.pooling_enabled` to 1.

This feature is not available in threaded versions. `pconnect` and `popen` then working like their non
persistent equivalents.

##### *Parameters*

*host*: string. can be a host, or the path to a unix domain socket. Starting from version 5.0.0 it is possible to specify schema 
*port*: int, optional  
*timeout*: float, value in seconds (optional, default is 0 meaning unlimited)  
*persistent_id*: string. identity for the requested persistent connection  
*retry_interval*: int, value in milliseconds (optional)  
*read_timeout*: float, value in seconds (optional, default is 0 meaning unlimited)

##### *Return value*

*BOOL*: `TRUE` on success, `FALSE` on error.

##### *Example*

~~~php
$redis->pconnect('127.0.0.1', 6379);
$redis->pconnect('127.0.0.1'); // port 6379 by default - same connection like before.
$redis->pconnect('tls://127.0.0.1', 6379); // enable transport level security.
$redis->pconnect('tls://127.0.0.1'); // enable transport level security, port 6379 by default.
$redis->pconnect('127.0.0.1', 6379, 2.5); // 2.5 sec timeout and would be another connection than the two before.
$redis->pconnect('127.0.0.1', 6379, 2.5, 'x'); // x is sent as persistent_id and would be another connection than the three before.
$redis->pconnect('/tmp/redis.sock'); // unix domain socket - would be another connection than the four before.
~~~

**Note:** `popen` is an alias for `pconnect` and will be removed in future versions of phpredis.

### auth
-----
_**Description**_: Authenticate the connection using a password.
*Warning*: The password is sent in plain-text over the network.

##### *Parameters*
*STRING*: password

##### *Return value*
*BOOL*: `TRUE` if the connection is authenticated, `FALSE` otherwise.

##### *Example*
~~~php
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

### swapdb
-----
_**Description**_:  Swap one Redis database with another atomically  

##### *Parameters*  
*INTEGER*: db1  
*INTEGER*: db2  

##### *Return value*  
`TRUE` on success and `FALSE` on failure.

*Note*: Requires Redis >= 4.0.0

##### *Example*  
~~~php
$redis->swapdb(0, 1); /* Swaps DB 0 with DB 1 atomically */
~~~

### close
-----
_**Description**_: Disconnects from the Redis instance.

*Note*: Closing a persistent connection requires PhpRedis >= 4.2.0.

##### *Parameters*
None.

##### *Return value*
*BOOL*: `TRUE` on success, `FALSE` on failure.

### setOption
-----
_**Description**_: Set client option.

##### *Parameters*
*parameter name*  
*parameter value*

##### *Return value*
*BOOL*: `TRUE` on success, `FALSE` on error.

##### *Example*
~~~php
$redis->setOption(Redis::OPT_SERIALIZER, Redis::SERIALIZER_NONE);	  // Don't serialize data
$redis->setOption(Redis::OPT_SERIALIZER, Redis::SERIALIZER_PHP);	  // Use built-in serialize/unserialize
$redis->setOption(Redis::OPT_SERIALIZER, Redis::SERIALIZER_IGBINARY); // Use igBinary serialize/unserialize
$redis->setOption(Redis::OPT_SERIALIZER, Redis::SERIALIZER_MSGPACK);  // Use msgpack serialize/unserialize

$redis->setOption(Redis::OPT_PREFIX, 'myAppName:');	// use custom prefix on all keys

/* Options for the SCAN family of commands, indicating whether to abstract
   empty results from the user.  If set to SCAN_NORETRY (the default), phpredis
   will just issue one SCAN command at a time, sometimes returning an empty
   array of results.  If set to SCAN_RETRY, phpredis will retry the scan command
   until keys come back OR Redis returns an iterator of zero
*/
$redis->setOption(Redis::OPT_SCAN, Redis::SCAN_NORETRY);
$redis->setOption(Redis::OPT_SCAN, Redis::SCAN_RETRY);
~~~


### getOption
-----
_**Description**_: Get client option.

##### *Parameters*
*parameter name*

##### *Return value*
Parameter value.

##### *Example*
~~~php
// return Redis::SERIALIZER_NONE, Redis::SERIALIZER_PHP, 
//        Redis::SERIALIZER_IGBINARY, or Redis::SERIALIZER_MSGPACK
$redis->getOption(Redis::OPT_SERIALIZER);
~~~

### ping
-----
_**Description**_: Check the current connection status.

##### *Prototype*
~~~php
$redis->ping([string $message]);
~~~

##### *Return value*
*Mixed*:  This method returns `TRUE` on success, or the passed string if called with an argument.

##### *Example*
~~~php
/* When called without an argument, PING returns `TRUE` */
$redis->ping();

/* If passed an argument, that argument is returned.  Here 'hello' will be returned */
$redis->ping('hello');
~~~

*Note*:  Prior to PhpRedis 5.0.0 this command simply returned the string `+PONG`.

### echo
-----
_**Description**_: Sends a string to Redis, which replies with the same string

##### *Parameters*

*STRING*: The message to send.

##### *Return value*

*STRING*: the same message.


## Server

1. [bgRewriteAOF](#bgrewriteaof) - Asynchronously rewrite the append-only file
1. [bgSave](#bgsave) - Asynchronously save the dataset to disk (in background)
1. [config](#config) - Get or Set the Redis server configuration parameters
1. [dbSize](#dbsize) - Return the number of keys in selected database
1. [flushAll](#flushall) - Remove all keys from all databases
1. [flushDb](#flushdb) - Remove all keys from the current database
1. [info](#info) - Get information and statistics about the server
1. [lastSave](#lastsave) - Get the timestamp of the last disk save
1. [resetStat](#resetstat) - Reset the stats returned by [info](#info) method.
1. [save](#save) - Synchronously save the dataset to disk (wait to complete)
1. [slaveOf](#slaveof) - Make the server a slave of another instance, or promote it to master
1. [time](#time) - Return the current server time
1. [slowLog](#slowlog) - Access the Redis slowLog entries

### bgRewriteAOF
-----
_**Description**_: Start the background rewrite of AOF (Append-Only File)

##### *Parameters*
None.

##### *Return value*
*BOOL*: `TRUE` in case of success, `FALSE` in case of failure.

##### *Example*
~~~php
$redis->bgRewriteAOF();
~~~

### bgSave
-----
_**Description**_: Asynchronously save the dataset to disk (in background)

##### *Parameters*
None.

##### *Return value*
*BOOL*: `TRUE` in case of success, `FALSE` in case of failure. If a save is already running, this command will fail and return `FALSE`.

##### *Example*
~~~php
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
~~~php
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
~~~php
$count = $redis->dbSize();
echo "Redis has $count keys\n";
~~~

### flushAll
-----
_**Description**_: Remove all keys from all databases.

##### *Parameters*
*async* (bool) requires server version 4.0.0 or greater

##### *Return value*
*BOOL*: Always `TRUE`.

##### *Example*
~~~php
$redis->flushAll();
~~~

### flushDb
-----
_**Description**_: Remove all keys from the current database.

##### *Parameters*
*async* (bool) requires server version 4.0.0 or greater

##### *Return value*
*BOOL*: Always `TRUE`.

##### *Example*
~~~php
$redis->flushDb();
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
~~~php
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
~~~php
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
~~~php
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
~~~php
$redis->save();
~~~

### slaveOf
-----
_**Description**_: Changes the slave status

##### *Parameters*
Either host (string) and port (int), or no parameter to stop being a slave.

##### *Return value*
*BOOL*: `TRUE` in case of success, `FALSE` in case of failure.

##### *Example*
~~~php
$redis->slaveOf('10.0.1.7', 6379);
/* ... */
$redis->slaveOf();
~~~

### time
-----
_**Description**_: Return the current server time.

##### *Parameters*
(none)

##### *Return value*
If successful, the time will come back as an associative array with element zero being
the unix timestamp, and element one being microseconds.

##### *Examples*
~~~php
$redis->time();
~~~

### slowLog
-----
_**Description**_: Access the Redis slowLog

##### *Parameters*
*Operation* (string): This can be either `GET`, `LEN`, or `RESET`  
*Length* (integer), optional: If executing a `SLOWLOG GET` command, you can pass an optional length.
#####

##### *Return value*
The return value of SLOWLOG will depend on which operation was performed.
SLOWLOG GET: Array of slowLog entries, as provided by Redis
SLOGLOG LEN: Integer, the length of the slowLog
SLOWLOG RESET: Boolean, depending on success
#####

##### *Examples*
~~~php
// Get ten slowLog entries
$redis->slowLog('get', 10);
// Get the default number of slowLog entries

$redis->slowLog('get');
// Reset our slowLog
$redis->slowLog('reset');

// Retrieve slowLog length
$redis->slowLog('len');
~~~

## Keys and Strings

### Strings
-----

* [append](#append) - Append a value to a key
* [bitCount](#bitcount) - Count set bits in a string
* [bitOp](#bitop) - Perform bitwise operations between strings
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
* [setEx, pSetEx](#setex-psetex) - Set the value and expiration of a key
* [setNx](#setnx) - Set the value of a key, only if the key does not exist
* [setRange](#setrange) - Overwrite part of a string at key starting at the specified offset
* [strLen](#strlen) - Get the length of the value stored in a key

### Keys
-----

* [del, delete, unlink](#del-delete-unlink) - Delete a key
* [dump](#dump) - Return a serialized version of the value stored at the specified key.
* [exists](#exists) - Determine if a key exists
* [expire, setTimeout, pexpire](#expire-settimeout-pexpire) - Set a key's time to live in seconds
* [expireAt, pexpireAt](#expireat-pexpireat) - Set the expiration for a key as a UNIX timestamp
* [keys, getKeys](#keys-getkeys) - Find all keys matching the given pattern
* [scan](#scan) - Scan for keys in the keyspace (Redis >= 2.8.0)
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

~~~php
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
~~~php
// Simple key -> value set
$redis->set('key', 'value');

// Will redirect, and actually make an SETEX call
$redis->set('key','value', 10);

// Will set the key, if it doesn't exist, with a ttl of 10 seconds
$redis->set('key', 'value', ['nx', 'ex'=>10]);

// Will set a key, if it does exist, with a ttl of 1000 milliseconds
$redis->set('key', 'value', ['xx', 'px'=>1000]);

~~~

### setEx, pSetEx
-----
_**Description**_: Set the string value in argument as value of the key, with a time to live. PSETEX uses a TTL in milliseconds.

##### *Parameters*
*Key*
*TTL*
*Value*

##### *Return value*
*Bool* `TRUE` if the command is successful.

##### *Examples*

~~~php
$redis->setEx('key', 3600, 'value'); // sets key → value, with 1h TTL.
$redis->pSetEx('key', 100, 'value'); // sets key → value, with 0.1 sec TTL.
~~~

### setNx
-----
_**Description**_: Set the string value in argument as value of the key if the key doesn't already exist in the database.

##### *Parameters*
*key*
*value*

##### *Return value*
*Bool* `TRUE` in case of success, `FALSE` in case of failure.

##### *Examples*
~~~php
$redis->setNx('key', 'value'); /* return TRUE */
$redis->setNx('key', 'value'); /* return FALSE */
~~~

### del, delete, unlink
-----
_**Description**_: Remove specified keys.

##### *Parameters*
An array of keys, or an undefined number of parameters, each a key: *key1* *key2* *key3* ... *keyN*

*Note*: If you are connecting to Redis server >= 4.0.0 you can remove a key with the `unlink` method in the exact same way you would use `del`.  The Redis [unlink](https://redis.io/commands/unlink) command is non-blocking and will perform the actual deletion asynchronously.

##### *Return value*
*Long* Number of keys deleted.

##### *Examples*
~~~php
$redis->set('key1', 'val1');
$redis->set('key2', 'val2');
$redis->set('key3', 'val3');
$redis->set('key4', 'val4');

$redis->del('key1', 'key2'); /* return 2 */
$redis->del(['key3', 'key4']); /* return 2 */

/* If using Redis >= 4.0.0 you can call unlink */
$redis->unlink('key1', 'key2');
$redis->unlink(['key1', 'key2']);
~~~

**Note:** `delete` is an alias for `del` and will be removed in future versions of phpredis.

### exists
-----
_**Description**_: Verify if the specified key exists.

##### *Parameters*
*key*

##### *Return value*
*long*: The number of keys tested that do exist.

##### *Examples*
~~~php
$redis->set('key', 'value');
$redis->exists('key'); /* 1 */
$redis->exists('NonExistingKey'); /* 0 */

$redis->mset(['foo' => 'foo', 'bar' => 'bar', 'baz' => 'baz']);
$redis->exists(['foo', 'bar', 'baz]); /* 3 */
$redis->exists('foo', 'bar', 'baz'); /* 3 */
~~~

**Note**: This function took a single argument and returned TRUE or FALSE in phpredis versions < 4.0.0.

### incr, incrBy
-----
_**Description**_: Increment the number stored at key by one. If the second argument is filled, it will be used as the integer value of the increment.

##### *Parameters*
*key*  
*value*: value that will be added to key (only for incrBy)

##### *Return value*
*INT* the new value

##### *Examples*
~~~php
$redis->incr('key1'); /* key1 didn't exists, set to 0 before the increment */
					  /* and now has the value 1  */

$redis->incr('key1'); /* 2 */
$redis->incr('key1'); /* 3 */
$redis->incr('key1'); /* 4 */

// Will redirect, and actually make an INCRBY call
$redis->incr('key1', 10);   /* 14 */

$redis->incrBy('key1', 10); /* 24 */
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
~~~php
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
*value*: value that will be subtracted to key (only for decrBy)

##### *Return value*
*INT* the new value

##### *Examples*
~~~php
$redis->decr('key1'); /* key1 didn't exists, set to 0 before the increment */
					  /* and now has the value -1  */

$redis->decr('key1'); /* -2 */
$redis->decr('key1'); /* -3 */

// Will redirect, and actually make an DECRBY call
$redis->decr('key1', 10);   /* -13 */

$redis->decrBy('key1', 10); /* -23 */
~~~

### mGet, getMultiple
-----
_**Description**_: Get the values of all the specified keys. If one or more keys don't exist, the array will contain `FALSE` at the position of the key.

##### *Parameters*
*Array*: Array containing the list of the keys

##### *Return value*
*Array*: Array containing the values related to keys in argument

##### *Examples*
~~~php
$redis->set('key1', 'value1');
$redis->set('key2', 'value2');
$redis->set('key3', 'value3');
$redis->mGet(['key1', 'key2', 'key3']); /* ['value1', 'value2', 'value3'];
$redis->mGet(['key0', 'key1', 'key5']); /* [`FALSE`, 'value1', `FALSE`];
~~~

**Note:** `getMultiple` is an alias for `mGet` and will be removed in future versions of phpredis.

### getSet
-----
_**Description**_: Sets a value and returns the previous entry at that key.
##### *Parameters*
*Key*: key

*STRING*: value

##### *Return value*
A string, the previous value located at this key.
##### *Example*
~~~php
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
~~~php
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

~~~php
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
~~~php
$redis->set('x', '42');
$redis->rename('x', 'y');
$redis->get('y'); 	// → 42
$redis->get('x'); 	// → `FALSE`
~~~

**Note:** `renameKey` is an alias for `rename` and will be removed in future versions of phpredis.

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
~~~php
$redis->set('x', '42');
$redis->expire('x', 3);	// x will disappear in 3 seconds.
sleep(5);				// wait 5 seconds
$redis->get('x'); 		// will return `FALSE`, as 'x' has expired.
~~~

**Note:** `setTimeout` is an alias for `expire` and will be removed in future versions of phpredis.

### expireAt, pexpireAt
-----
_**Description**_: Sets an expiration date (a timestamp) on an item. pexpireAt requires a timestamp in milliseconds.

##### *Parameters*
*Key*: key. The key that will disappear.

*Integer*: Unix timestamp. The key's date of death, in seconds from Epoch time.

##### *Return value*
*BOOL*: `TRUE` in case of success, `FALSE` in case of failure.
##### *Example*
~~~php
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
~~~php
$allKeys = $redis->keys('*');	// all keys will match this.
$keyWithUserPrefix = $redis->keys('user*');
~~~

**Note:** `getKeys` is an alias for `keys` and will be removed in future versions of phpredis.

### scan
-----
_**Description**_:  Scan the keyspace for keys

##### *Parameters*
*LONG (reference)*:  Iterator, initialized to NULL
*STRING, Optional*:  Pattern to match
*LONG, Optional*: Count of keys per iteration (only a suggestion to Redis)

##### *Return value*
*Array, boolean*:  This function will return an array of keys or FALSE if Redis returned zero keys

*Note*: SCAN is a "directed node" command in [RedisCluster](cluster.markdown#directed-node-commands)

##### *Example*
~~~php

/* Without enabling Redis::SCAN_RETRY (default condition) */
$it = NULL;
do {
    // Scan for some keys
    $arr_keys = $redis->scan($it);

    // Redis may return empty results, so protect against that
    if ($arr_keys !== FALSE) {
        foreach($arr_keys as $str_key) {
            echo "Here is a key: $str_key\n";
        }
    }
} while ($it > 0);
echo "No more keys to scan!\n";

/* With Redis::SCAN_RETRY enabled */
$redis->setOption(Redis::OPT_SCAN, Redis::SCAN_RETRY);
$it = NULL;

/* phpredis will retry the SCAN command if empty results are returned from the
   server, so no empty results check is required. */
while ($arr_keys = $redis->scan($it)) {
    foreach ($arr_keys as $str_key) {
        echo "Here is a key: $str_key\n";
    }
}
echo "No more keys to scan!\n";
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
~~~php
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
~~~php
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
~~~php
$redis->set('key', 'value1');
$redis->append('key', 'value2'); /* 12 */
$redis->get('key'); /* 'value1value2' */
~~~

### getRange
-----
_**Description**_: Return a substring of a larger string

##### *Parameters*
*key*  
*start*  
*end*

##### *Return value*
*STRING*: the substring

##### *Example*
~~~php
$redis->set('key', 'string value');
$redis->getRange('key', 0, 5); /* 'string' */
$redis->getRange('key', -5, -1); /* 'value' */
~~~

**Note**: `substr` is an alias for `getRange` and will be removed in future versions of phpredis.

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
~~~php
$redis->set('key', 'Hello world');
$redis->setRange('key', 6, "redis"); /* returns 11 */
$redis->get('key'); /* "Hello redis" */
~~~

### strLen
-----
_**Description**_: Get the length of a string value.

##### *Parameters*
*key*

##### *Return value*
*INTEGER*

##### *Example*
~~~php
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
~~~php
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
~~~php
$redis->set('key', "*");	// ord("*") = 42 = 0x2f = "0010 1010"
$redis->setBit('key', 5, 1); /* returns 0 */
$redis->setBit('key', 7, 1); /* returns 0 */
$redis->get('key'); /* chr(0x2f) = "/" = b("0010 1111") */
~~~

### bitOp
-----
_**Description**_: Bitwise operation on multiple keys.

##### *Parameters*
*operation*: either "AND", "OR", "NOT", "XOR"  
*ret_key*: return key  
*key1*  
*key2...*

##### *Return value*
*LONG*: The size of the string stored in the destination key.

### bitCount
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
*Options*: [key => value, ...] - optional, with the following keys and values:
~~~
    'by' => 'some_pattern_*',
    'limit' => [0, 1],
    'get' => 'some_other_pattern_*' or an array of patterns,
    'sort' => 'asc' or 'desc',
    'alpha' => TRUE,
    'store' => 'external-key'
~~~
##### *Return value*
An array of values, or a number corresponding to the number of elements stored if that was used.

##### *Example*
~~~php
$redis->del('s');
$redis->sAdd('s', 5);
$redis->sAdd('s', 4);
$redis->sAdd('s', 2);
$redis->sAdd('s', 1);
$redis->sAdd('s', 3);

var_dump($redis->sort('s')); // 1,2,3,4,5
var_dump($redis->sort('s', ['sort' => 'desc'])); // 5,4,3,2,1
var_dump($redis->sort('s', ['sort' => 'desc', 'store' => 'out'])); // (int)5
~~~




### ttl, pttl
-----
_**Description**_: Returns the time to live left for a given key in seconds (ttl), or milliseconds (pttl).

##### *Parameters*
*Key*: key

##### *Return value*
*LONG*:  The time to live in seconds.  If the key has no ttl, `-1` will be returned, and `-2` if the key doesn't exist.

##### *Example*
~~~php
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
~~~php
$redis->persist('key');
~~~

### mSet, mSetNx
-----
_**Description**_: Sets multiple key-value pairs in one atomic command. MSETNX only returns TRUE if all the keys were set (see SETNX).

##### *Parameters*
*Pairs*: [key => value, ...]

##### *Return value*
*Bool* `TRUE` in case of success, `FALSE` in case of failure.

##### *Example*
~~~php

$redis->mSet(['key0' => 'value0', 'key1' => 'value1']);
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
~~~php
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
~~~php
$redis->set('foo', 'bar');
$val = $redis->dump('foo');
$redis->restore('bar', 0, $val); // The key 'bar', will now be equal to the key 'foo'
~~~

### migrate
-----
_**Description**_: Migrates a key to a different Redis instance.

**Note:**: Redis introduced migrating multiple keys in 3.0.6, so you must have at least
that version in order to call `migrate` with an array of keys.

##### *Parameters*
*host* string.  The destination host  
*port* integer.  The TCP port to connect to.  
*key(s)* string or array.  
*destination-db* integer.  The target DB.  
*timeout* integer.  The maximum amount of time given to this transfer.  
*copy* boolean, optional.  Should we send the COPY flag to redis.  
*replace* boolean, optional.  Should we send the REPLACE flag to redis  
##### *Examples*
~~~php
$redis->migrate('backup', 6379, 'foo', 0, 3600);
$redis->migrate('backup', 6379, 'foo', 0, 3600, true, true); /* copy and replace */
$redis->migrate('backup', 6379, 'foo', 0, 3600, false, true); /* just REPLACE flag */

/* Migrate multiple keys (requires Redis >= 3.0.6)
$redis->migrate('backup', 6379, ['key1', 'key2', 'key3'], 0, 3600);
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
* [hScan](#hscan) - Scan a hash key for members
* [hStrLen](#hstrlen) - Get the string length of the value associated with field in the hash

### hSet
-----
_**Description**_: Adds a value to the hash stored at key.
##### *Parameters*
*key*  
*hashKey*  
*value*

##### *Return value*
*LONG* `1` if value didn't exist and was added successfully, `0` if the value was already present and was replaced, `FALSE` if there was an error.
##### *Example*
~~~php
$redis->del('h')
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
~~~php
$redis->del('h')
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
~~~php
$redis->del('h')
$redis->hSet('h', 'key1', 'hello');
$redis->hSet('h', 'key2', 'plop');
$redis->hLen('h'); /* returns 2 */
~~~

### hDel
-----
_**Description**_: Removes a value from the hash stored at key. If the hash table doesn't exist, or the key doesn't exist, `FALSE` is returned.
##### *Parameters*
*key*  
*hashKey1*  
*hashKey2*  
...

##### *Return value*
*LONG* the number of deleted keys, 0 if the key doesn't exist, `FALSE` if the key isn't a hash.


### hKeys
-----
_**Description**_: Returns the keys in a hash, as an array of strings.

##### *Parameters*
*Key*: key

##### *Return value*
An array of elements, the keys of the hash. This works like PHP's array_keys().

##### *Example*
~~~php
$redis->del('h');
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
~~~php
$redis->del('h');
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
~~~php
$redis->del('h');
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
~~~php
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
~~~php
$redis->del('h');
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
~~~php
$redis->del('h');
$redis->hIncrByFloat('h','x', 1.5); /* returns 1.5: h[x] = 1.5 now */
$redis->hIncrByFloat('h', 'x', 1.5); /* returns 3.0: h[x] = 3.0 now */
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
~~~php
$redis->del('user:1');
$redis->hMSet('user:1', ['name' => 'Joe', 'salary' => 2000]);
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
~~~php
$redis->del('h');
$redis->hSet('h', 'field1', 'value1');
$redis->hSet('h', 'field2', 'value2');
$redis->hMGet('h', ['field1', 'field2']); /* returns ['field1' => 'value1', 'field2' => 'value2'] */
~~~

### hScan
-----
_**Description**_:  Scan a HASH value for members, with an optional pattern and count
##### *Parameters*
*key*: String  
*iterator*: Long (reference)  
*pattern*: Optional pattern to match against  
*count*: How many keys to return in a go (only a suggestion to Redis)
##### *Return value*
*Array* An array of members that match our pattern

##### *Examples*
~~~php
$it = NULL;
/* Don't ever return an empty array until we're done iterating */
$redis->setOption(Redis::OPT_SCAN, Redis::SCAN_RETRY);
while($arr_keys = $redis->hScan('hash', $it)) {
    foreach($arr_keys as $str_field => $str_value) {
        echo "$str_field => $str_value\n"; /* Print the hash member and value */
    }
}
~~~

### hStrLen
-----
_**Description**_: Get the string length of the value associated with field in the hash stored at key.
##### *Parameters*
*key*: String  
*field*: String
##### *Return value*
*LONG* the string length of the value associated with field, or zero when field is not present in the hash or key does not exist at all.

## Lists

* [blPop, brPop](#blpop-brpop) - Remove and get the first/last element in a list
* [bRPopLPush](#brpoplpush) - Pop a value from a list, push it to another list and return it
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
* [rPopLPush](#rpoplpush) - Remove the last element in a list, append it to another list and return it (redis >= 1.1)
* [rPush](#rpush) - Append one or multiple values to a list
* [rPushX](#rpushx) - Append a value to a list, only if the list exists

### blPop, brPop
-----
_**Description**_: Is a blocking lPop(rPop) primitive. If at least one of the lists contains at least one element, the element will be popped from the head of the list and returned to the caller.
If all the list identified by the keys passed in arguments are empty, blPop will block during the specified timeout until an element is pushed to one of those lists. This element will be popped.

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
*ARRAY* ['listName', 'element']

##### *Example*
~~~php
/* Non blocking feature */
$redis->lPush('key1', 'A');
$redis->del('key2');

$redis->blPop('key1', 'key2', 10); /* ['key1', 'A'] */
/* OR */
$redis->blPop(['key1', 'key2'], 10); /* ['key1', 'A'] */

$redis->brPop('key1', 'key2', 10); /* ['key1', 'A'] */
/* OR */
$redis->brPop(['key1', 'key2'], 10); /* ['key1', 'A'] */

/* Blocking feature */

/* process 1 */
$redis->del('key1');
$redis->blPop('key1', 10);
/* blocking for 10 seconds */

/* process 2 */
$redis->lPush('key1', 'A');

/* process 1 */
/* ['key1', 'A'] is returned*/
~~~

### bRPopLPush
-----
_**Description**_: A blocking version of `rPopLPush`, with an integral timeout in the third parameter.

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
~~~php
$redis->rPush('key1', 'A');
$redis->rPush('key1', 'B');
$redis->rPush('key1', 'C'); /* key1 => [ 'A', 'B', 'C' ] */
$redis->lindex('key1', 0); /* 'A' */
$redis->lindex('key1', -1); /* 'C' */
$redis->lindex('key1', 10); /* `FALSE` */
~~~

**Note:** `lGet` is an alias for `lIndex` and will be removed in future versions of phpredis.

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
~~~php
$redis->del('key1');
$redis->lInsert('key1', Redis::AFTER, 'A', 'X'); /* 0 */

$redis->lPush('key1', 'A');
$redis->lPush('key1', 'B');
$redis->lPush('key1', 'C');

$redis->lInsert('key1', Redis::BEFORE, 'C', 'X'); /* 4 */
$redis->lRange('key1', 0, -1); /* ['A', 'B', 'X', 'C'] */

$redis->lInsert('key1', Redis::AFTER, 'C', 'Y'); /* 5 */
$redis->lRange('key1', 0, -1); /* ['A', 'B', 'X', 'C', 'Y'] */

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
~~~php
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
~~~php
$redis->del('key1');
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
~~~php
$redis->del('key1');
$redis->lPushx('key1', 'A'); // returns 0
$redis->lPush('key1', 'A'); // returns 1
$redis->lPushx('key1', 'B'); // returns 2
$redis->lPushx('key1', 'C'); // returns 3
/* key1 now points to the following list: [ 'A', 'B', 'C' ] */
~~~

### lRange, lGetRange
-----
_**Description**_: Returns the specified elements of the list stored at the specified key in the range [start, end]. start and stop are interpreted as indices:  
0 the first element, 1 the second ...  
-1 the last element, -2 the penultimate ...

##### *Parameters*
*key*  
*start*  
*end*

##### *Return value*
*Array* containing the values in specified range.

##### *Example*
~~~php
$redis->rPush('key1', 'A');
$redis->rPush('key1', 'B');
$redis->rPush('key1', 'C');
$redis->lRange('key1', 0, -1); /* ['A', 'B', 'C'] */
~~~

**Note:** `lGetRange` is an alias for `lRange` and will be removed in future versions of phpredis.

### lRem, lRemove
-----
_**Description**_: Removes the first `count` occurrences of the value element from the list. If count is zero, all the matching elements are removed. If count is negative, elements are removed from tail to head.

**Note**: The argument order is not the same as in the Redis documentation. This difference is kept for compatibility reasons.

##### *Parameters*
*key*  
*value*  
*count*

##### *Return value*
*LONG* the number of elements to remove  
*BOOL* `FALSE` if the value identified by key is not a list.

##### *Example*
~~~php
$redis->lPush('key1', 'A');
$redis->lPush('key1', 'B');
$redis->lPush('key1', 'C');
$redis->lPush('key1', 'A');
$redis->lPush('key1', 'A');

$redis->lRange('key1', 0, -1); /* ['A', 'A', 'C', 'B', 'A'] */
$redis->lRem('key1', 'A', 2); /* 2 */
$redis->lRange('key1', 0, -1); /* ['C', 'B', 'A'] */
~~~

**Note:** `lRemove` is an alias for `lRem` and will be removed in future versions of phpredis.

### lSet
-----
_**Description**_: Set the list at index with the new value.

##### *Parameters*
*key*  
*index*  
*value*

##### *Return value*
*BOOL* `TRUE` if the new value was set. `FALSE` if the index is out of range, or data type identified by key is not a list.

##### *Example*
~~~php
$redis->rPush('key1', 'A');
$redis->rPush('key1', 'B');
$redis->rPush('key1', 'C'); /* key1 => [ 'A', 'B', 'C' ] */
$redis->lindex('key1', 0); /* 'A' */
$redis->lSet('key1', 0, 'X');
$redis->lindex('key1', 0); /* 'X' */
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
~~~php
$redis->rPush('key1', 'A');
$redis->rPush('key1', 'B');
$redis->rPush('key1', 'C');
$redis->lRange('key1', 0, -1); /* ['A', 'B', 'C'] */
$redis->lTrim('key1', 0, 1);
$redis->lRange('key1', 0, -1); /* ['A', 'B'] */
~~~

**Note:** `listTrim` is an alias for `lTrim` and will be removed in future versions of phpredis.

### rPop
-----
_**Description**_: Returns and removes the last element of the list.

##### *Parameters*
*key*

##### *Return value*
*STRING* if command executed successfully  
*BOOL* `FALSE` in case of failure (empty list)

##### *Example*
~~~php
$redis->rPush('key1', 'A');
$redis->rPush('key1', 'B');
$redis->rPush('key1', 'C'); /* key1 => [ 'A', 'B', 'C' ] */
$redis->rPop('key1'); /* key1 => [ 'A', 'B' ] */
~~~

### rPopLPush
-----
_**Description**_: Pops a value from the tail of a list, and pushes it to the front of another list. Also return this value. (redis >= 1.1)

##### *Parameters*
*Key*: srckey  
*Key*: dstkey

##### *Return value*
*STRING* The element that was moved in case of success, `FALSE` in case of failure.

##### *Example*
~~~php
$redis->del('x', 'y');

$redis->lPush('x', 'abc');
$redis->lPush('x', 'def');
$redis->lPush('y', '123');
$redis->lPush('y', '456');

// move the last of x to the front of y.
var_dump($redis->rPopLPush('x', 'y'));
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
~~~php
$redis->del('key1');
$redis->rPush('key1', 'A'); // returns 1
$redis->rPush('key1', 'B'); // returns 2
$redis->rPush('key1', 'C'); // returns 3
/* key1 now points to the following list: [ 'A', 'B', 'C' ] */
~~~

### rPushX
-----
_**Description**_: Adds the string value to the tail (right) of the list if the list exists. `FALSE` in case of Failure.

##### *Parameters*
*key*  
*value* String, value to push in key

##### *Return value*
*LONG* The new length of the list in case of success, `FALSE` in case of Failure.

##### *Examples*
~~~php
$redis->del('key1');
$redis->rPushX('key1', 'A'); // returns 0
$redis->rPush('key1', 'A'); // returns 1
$redis->rPushX('key1', 'B'); // returns 2
$redis->rPushX('key1', 'C'); // returns 3
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
~~~php
$redis->rPush('key1', 'A');
$redis->rPush('key1', 'B');
$redis->rPush('key1', 'C'); /* key1 => [ 'A', 'B', 'C' ] */
$redis->lLen('key1');/* 3 */
$redis->rPop('key1');
$redis->lLen('key1');/* 2 */
~~~

**Note:** `lSize` is an alias for `lLen` and will be removed in future versions of phpredis.


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
* [sPop](#spop) - Remove and return one or more members of a set at random
* [sRandMember](#srandmember) - Get one or multiple random members from a set
* [sRem, sRemove](#srem-sremove) - Remove one or more members from a set
* [sUnion](#sunion) - Add multiple sets
* [sUnionStore](#sunionstore) - Add multiple sets and store the resulting set in a key
* [sScan](#sscan) - Scan a set for members

### sAdd
-----
_**Description**_: Adds a value to the set value stored at key. If this value is already in the set, `FALSE` is returned.
##### *Parameters*
*key*  
*value*

##### *Return value*
*LONG* the number of elements added to the set.
##### *Example*
~~~php
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
~~~php
$redis->sAdd('key1' , 'member1');
$redis->sAdd('key1' , 'member2');
$redis->sAdd('key1' , 'member3'); /* 'key1' => {'member1', 'member2', 'member3'}*/
$redis->sCard('key1'); /* 3 */
$redis->sCard('keyX'); /* 0 */
~~~

**Note:** `sSize` is an alias for `sCard` and will be removed in future versions of phpredis.

### sDiff
-----
_**Description**_: Performs the difference between N sets and returns it.

##### *Parameters*
*Keys*: key1, key2, ... , keyN: Any number of keys corresponding to sets in redis.

##### *Return value*
*Array of strings*: The difference of the first set will all the others.

##### *Example*
~~~php
$redis->del('s0', 's1', 's2');

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
~~~php
$redis->del('s0', 's1', 's2');

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

Array, contain the result of the intersection between those keys. If the intersection between the different sets is empty, the return value will be empty array.

##### *Examples*
~~~php
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
~~~php
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
~~~php
$redis->sAdd('key1' , 'member1');
$redis->sAdd('key1' , 'member2');
$redis->sAdd('key1' , 'member3'); /* 'key1' => {'member1', 'member2', 'member3'}*/

$redis->sIsMember('key1', 'member1'); /* TRUE */
$redis->sIsMember('key1', 'memberX'); /* FALSE */
~~~

**Note:** `sContains` is an alias for `sIsMember` and will be removed in future versions of phpredis.

### sMembers, sGetMembers
-----
_**Description**_: Returns the contents of a set.

##### *Parameters*
*Key*: key

##### *Return value*
An array of elements, the contents of the set.

##### *Example*
~~~php
$redis->del('s');
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

**Note:** `sGetMembers` is an alias for `sMembers` and will be removed in future versions of phpredis.

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
~~~php
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
*count*: Integer, optional
##### *Return value (without count argument)*
*String* "popped" value  
*Bool* `FALSE` if set identified by key is empty or doesn't exist.
##### *Return value (with count argument)*
*Array*: Member(s) returned or an empty array if the set doesn't exist  
*Bool*: `FALSE` on error if the key is not a set
##### *Example*
~~~php
$redis->sAdd('key1' , 'member1');
$redis->sAdd('key1' , 'member2');
$redis->sAdd('key1' , 'member3'); /* 'key1' => {'member3', 'member1', 'member2'}*/
$redis->sPop('key1'); /* 'member1', 'key1' => {'member3', 'member2'} */
$redis->sPop('key1'); /* 'member3', 'key1' => {'member2'} */

/* With count */
$redis->sAdd('key2', 'member1', 'member2', 'member3');
$redis->sPop('key2', 3); /* Will return all members but in no particular order */
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
~~~php
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
~~~php
$redis->sAdd('key1' , 'member1');
$redis->sAdd('key1' , 'member2');
$redis->sAdd('key1' , 'member3'); /* 'key1' => {'member1', 'member2', 'member3'}*/
$redis->sRem('key1', 'member2', 'member3'); /*return 2. 'key1' => {'member1'} */
~~~

**Note:** `sRemove` is an alias for `sRem` and will be removed in future versions of phpredis.

### sUnion
-----
_**Description**_: Performs the union between N sets and returns it.

##### *Parameters*
*Keys*: key1, key2, ... , keyN: Any number of keys corresponding to sets in redis.

##### *Return value*
*Array of strings*: The union of all these sets.

**Note:** `sUnion` can also take a single array with keys (see example below).

##### *Example*
~~~php
$redis->del('s0', 's1', 's2');

$redis->sAdd('s0', '1');
$redis->sAdd('s0', '2');
$redis->sAdd('s1', '3');
$redis->sAdd('s1', '1');
$redis->sAdd('s2', '3');
$redis->sAdd('s2', '4');

/* Get the union with variadic arguments */
var_dump($redis->sUnion('s0', 's1', 's2'));

/* Pass a single array */
var_dump($redis->sUnion(['s0', 's1', 's2']);

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
~~~php
$redis->del('s0', 's1', 's2');

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

### sScan
-----
_**Description**_: Scan a set for members

##### *Parameters*
*Key*: The set to search  
*iterator*: LONG (reference) to the iterator as we go  
*pattern*: String, optional pattern to match against  
*count*: How many members to return at a time (Redis might return a different amount)

##### *Return value*
*Array, boolean*: PHPRedis will return an array of keys or FALSE when we're done iterating

##### *Example*
~~~php
$it = NULL;
$redis->setOption(Redis::OPT_SCAN, Redis::SCAN_RETRY); /* don't return empty results until we're done */
while($arr_mems = $redis->sScan('set', $it, "*pattern*")) {
    foreach($arr_mems as $str_mem) {
        echo "Member: $str_mem\n";
    }
}

$it = NULL;
$redis->setOption(Redis::OPT_SCAN, Redis::SCAN_NORETRY); /* return after each iteration, even if empty */
while(($arr_mems = $redis->sScan('set', $it, "*pattern*"))!==FALSE) {
    if(count($arr_mems) > 0) {
        foreach($arr_mems as $str_mem) {
            echo "Member found: $str_mem\n";
        }
    } else {
        echo "No members in this iteration, iterator value: $it\n";
    }
}
~~~

## Sorted sets

* [bzPop](#bzpop) - Block until Redis can pop the highest or lowest scoring member from one or more ZSETs.
* [zAdd](#zadd) - Add one or more members to a sorted set or update its score if it already exists
* [zCard, zSize](#zcard-zsize) - Get the number of members in a sorted set
* [zCount](#zcount) - Count the members in a sorted set with scores within the given values
* [zIncrBy](#zincrby) - Increment the score of a member in a sorted set
* [zinterstore, zInter](#zinterstore-zinter) - Intersect multiple sorted sets and store the resulting sorted set in a new key
* [zPop](#zpop) - Redis can pop the highest or lowest scoring member from one a ZSET.
* [zRange](#zrange) - Return a range of members in a sorted set, by index
* [zRangeByScore, zRevRangeByScore](#zrangebyscore-zrevrangebyscore) - Return a range of members in a sorted set, by score
* [zRangeByLex](#zrangebylex) - Return a lexicographical range from members that share the same score
* [zRank, zRevRank](#zrank-zrevrank) - Determine the index of a member in a sorted set
* [zRem, zDelete, zRemove](#zrem-zdelete-zremove) - Remove one or more members from a sorted set
* [zRemRangeByRank, zDeleteRangeByRank](#zremrangebyrank-zdeleterangebyrank) - Remove all members in a sorted set within the given indexes
* [zRemRangeByScore, zDeleteRangeByScore, zRemoveRangeByScore](#zremrangebyscore-zdeleterangebyscore-zremoverangebyscore) - Remove all members in a sorted set within the given scores
* [zRevRange](#zrevrange) - Return a range of members in a sorted set, by index, with scores ordered from high to low
* [zScore](#zscore) - Get the score associated with the given member in a sorted set
* [zunionstore, zUnion](#zunionstore-zunion) - Add multiple sorted sets and store the resulting sorted set in a new key
* [zScan](#zscan) - Scan a sorted set for members

### bzPop
-----
_**Description**_: Block until Redis can pop the highest or lowest scoring members from one or more ZSETs.  There are two commands (`BZPOPMIN` and `BZPOPMAX` for popping the lowest and highest scoring elements respectively.)

##### *Prototype*
~~~php
$redis->bzPopMin(array $keys, int $timeout): array
$redis->bzPopMax(array $keys, int $timeout): array

$redis->bzPopMin(string $key1, string $key2, ... int $timeout): array
$redis->bzPopMax(string $key1, string $key2, ... int $timeout): array
~~~

##### *Return value*
*ARRAY:* Either an array with the key member and score of the highest or lowest element or an empty array if the timeout was reached without an element to pop.

##### *Example*
~~~php
/* Wait up to 5 seconds to pop the *lowest* scoring member from sets `zs1` and `zs2`. */
$redis->bzPopMin(['zs1', 'zs2'], 5);
$redis->bzPopMin('zs1', 'zs2', 5);

/* Wait up to 5 seconds to pop the *highest* scoring member from sets `zs1` and `zs2` */
$redis->bzPopMax(['zs1', 'zs2'], 5);
$redis->bzPopMax('zs1', 'zs2', 5);
~~~

**Note:** Calling these functions with an array of keys or with a variable number of arguments is functionally identical.

### zAdd
-----
_**Description**_: Add one or more members to a sorted set or update its score if it already exists

##### *Parameters*
*key*
*score*: double  
*value*: string

##### *Return value*
*Long* 1 if the element is added. 0 otherwise.

##### *Example*
~~~php
$redis->zAdd('key', 1, 'val1');
$redis->zAdd('key', 0, 'val0');
$redis->zAdd('key', 5, 'val5');
$redis->zRange('key', 0, -1); // [val0, val1, val5]
~~~

### zCard, zSize
-----
_**Description**_: Returns the cardinality of an ordered set.

##### *Parameters*
*key*

##### *Return value*
*Long*, the set's cardinality

##### *Example*
~~~php
$redis->zAdd('key', 0, 'val0');
$redis->zAdd('key', 2, 'val2');
$redis->zAdd('key', 10, 'val10');
$redis->zCard('key'); /* 3 */
~~~

**Note**: `zSize` is an alias for `zCard` and will be removed in future versions of phpredis.

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
~~~php
$redis->zAdd('key', 0, 'val0');
$redis->zAdd('key', 2, 'val2');
$redis->zAdd('key', 10, 'val10');
$redis->zCount('key', 0, 3); /* 2, corresponding to ['val0', 'val2'] */
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
~~~php
$redis->del('key');
$redis->zIncrBy('key', 2.5, 'member1'); /* key or member1 didn't exist, so member1's score is to 0 before the increment */
					  /* and now has the value 2.5  */
$redis->zIncrBy('key', 1, 'member1'); /* 3.5 */
~~~

### zinterstore, zInter
-----
_**Description**_: Creates an intersection of sorted sets given in second argument. The result of the union will be stored in the sorted set defined by the first argument.

The third optional argument defines `weights` to apply to the sorted sets in input. In this case, the `weights` will be multiplied by the score of each element in the sorted set before applying the aggregation.
The forth argument defines the `AGGREGATE` option which specify how the results of the union are aggregated.

##### *Parameters*
*keyOutput*  
*arrayZSetKeys*  
*arrayWeights*  
*aggregateFunction* Either "SUM", "MIN", or "MAX": defines the behaviour to use on duplicate entries during the zinterstore.

##### *Return value*
*LONG* The number of values in the new sorted set.

##### *Example*
~~~php
$redis->del('k1');
$redis->del('k2');
$redis->del('k3');

$redis->del('ko1');
$redis->del('ko2');
$redis->del('ko3');
$redis->del('ko4');

$redis->zAdd('k1', 0, 'val0');
$redis->zAdd('k1', 1, 'val1');
$redis->zAdd('k1', 3, 'val3');

$redis->zAdd('k2', 2, 'val1');
$redis->zAdd('k2', 3, 'val3');

$redis->zinterstore('ko1', ['k1', 'k2']); 				/* 2, 'ko1' => ['val1', 'val3'] */
$redis->zinterstore('ko2', ['k1', 'k2'], [1, 1]); 	/* 2, 'ko2' => ['val1', 'val3'] */

/* Weighted zinterstore */
$redis->zinterstore('ko3', ['k1', 'k2'], [1, 5], 'min'); /* 2, 'ko3' => ['val1', 'val3'] */
$redis->zinterstore('ko4', ['k1', 'k2'], [1, 5], 'max'); /* 2, 'ko4' => ['val3', 'val1'] */
~~~

**Note:** `zInter` is an alias for `zinterstore` and will be removed in future versions of phpredis.

### zPop
-----
_**Description**_: Can pop the highest or lowest scoring members from one ZSETs. There are two commands (`ZPOPMIN` and `ZPOPMAX` for popping the lowest and highest scoring elements respectively.)

##### *Prototype*
~~~php
$redis->zPopMin(string $key, int $count): array
$redis->zPopMax(string $key, int $count): array

$redis->zPopMin(string $key, int $count): array
$redis->zPopMax(string $key, int $count): array
~~~

##### *Return value*
*ARRAY:* Either an array with the key member and score of the highest or lowest element or an empty array if there is no element available.

##### *Example*
~~~php
/* Pop the *lowest* scoring member from set `zs1`. */
$redis->zPopMin('zs1', 5);

/* Pop the *highest* scoring member from set `zs1`. */
$redis->zPopMax('zs1', 5);
~~~

### zRange
-----
_**Description**_: Returns a range of elements from the ordered set stored at the specified key, with values in the range [start, end].

Start and stop are interpreted as zero-based indices:  
`0` the first element, `1` the second ...  
`-1` the last element, `-2` the penultimate ...  

##### *Parameters*
*key*
*start*: long  
*end*: long  
*withscores*: bool = false

##### *Return value*
*Array* containing the values in specified range.

##### *Example*
~~~php
$redis->zAdd('key1', 0, 'val0');
$redis->zAdd('key1', 2, 'val2');
$redis->zAdd('key1', 10, 'val10');
$redis->zRange('key1', 0, -1); /* ['val0', 'val2', 'val10'] */

// with scores
$redis->zRange('key1', 0, -1, true); /* ['val0' => 0, 'val2' => 2, 'val10' => 10] */
~~~

### zRangeByScore, zRevRangeByScore
-----
_**Description**_: Returns the elements of the sorted set stored at the specified key which have scores in the range [start,end]. Adding a parenthesis before `start` or `end` excludes it from the range. +inf and -inf are also valid limits. zRevRangeByScore returns the same items in reverse order, when the `start` and `end` parameters are swapped.

##### *Parameters*
*key*  
*start*: string  
*end*: string  
*options*: array

Two options are available: `withscores => TRUE`, and `limit => [$offset, $count]`

##### *Return value*
*Array* containing the values in specified range.

##### *Example*
~~~php
$redis->zAdd('key', 0, 'val0');
$redis->zAdd('key', 2, 'val2');
$redis->zAdd('key', 10, 'val10');
$redis->zRangeByScore('key', 0, 3); /* ['val0', 'val2'] */
$redis->zRangeByScore('key', 0, 3, ['withscores' => TRUE]); /* ['val0' => 0, 'val2' => 2] */
$redis->zRangeByScore('key', 0, 3, ['limit' => [1, 1]]); /* ['val2'] */
$redis->zRangeByScore('key', 0, 3, ['withscores' => TRUE, 'limit' => [1, 1]]); /* ['val2' => 2] */
~~~

### zRangeByLex
-----
_**Description**_:  Returns a lexicographical range of members in a sorted set, assuming the members have the same score.  The min and max values are required to start with '(' (exclusive), '[' (inclusive), or be exactly the values '-' (negative inf) or '+' (positive inf).  The command must be called with either three *or* five arguments or will return FALSE.

##### *Parameters*
*key*: The ZSET you wish to run against  
*min*: The minimum alphanumeric value you wish to get  
*max*: The maximum alphanumeric value you wish to get  
*offset*:  Optional argument if you wish to start somewhere other than the first element.  
*limit*: Optional argument if you wish to limit the number of elements returned.

##### *Return value*
*Array* containing the values in the specified range.

##### *Example*
~~~php
foreach(['a','b','c','d','e','f','g'] as $c)
    $redis->zAdd('key',0,$c);

$redis->zRangeByLex('key','-','[c') /* ['a','b','c']; */
$redis->zRangeByLex('key','-','(c') /* ['a','b'] */
$redis->zRangeByLex('key','-','[c',1,2) /* ['b','c'] */
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
~~~php
$redis->del('z');
$redis->zAdd('key', 1, 'one');
$redis->zAdd('key', 2, 'two');
$redis->zRank('key', 'one'); /* 0 */
$redis->zRank('key', 'two'); /* 1 */
$redis->zRevRank('key', 'one'); /* 1 */
$redis->zRevRank('key', 'two'); /* 0 */
~~~

### zRem, zDelete, zRemove
-----
_**Description**_: Delete one or more members from a sorted set.

##### *Prototype*
~~~php
$redis->zRem($key, $member [, $member ...]);
~~~

##### *Return value*
*LONG:* The number of members deleted.

##### *Example*
~~~php
$redis->zAdd('key', 0, 'val0', 1, 'val1', 2, 'val2');
$redis->zRem('key', 'val0', 'val1', 'val2'); // Returns: 3
~~~

**Note:** `zDelete` and `zRemove` are an alias for `zRem` and will be removed in future versions of phpredis.

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
~~~php
$redis->zAdd('key', 1, 'one');
$redis->zAdd('key', 2, 'two');
$redis->zAdd('key', 3, 'three');
$redis->zRemRangeByRank('key', 0, 1); /* 2 */
$redis->zRange('key', 0, -1, ['withscores' => TRUE]); /* ['three' => 3] */
~~~

**Note:** `zDeleteRangeByRank` is an alias for `zRemRangeByRank` and will be removed in future versions of phpredis.

### zRemRangeByScore, zDeleteRangeByScore, zRemoveRangeByScore
-----
_**Description**_: Deletes the elements of the sorted set stored at the specified key which have scores in the range [start,end].

##### *Parameters*
*key*  
*start*: double or "+inf" or "-inf" string  
*end*: double or "+inf" or "-inf" string

##### *Return value*
*LONG* The number of values deleted from the sorted set

##### *Example*
~~~php
$redis->zAdd('key', 0, 'val0');
$redis->zAdd('key', 2, 'val2');
$redis->zAdd('key', 10, 'val10');
$redis->zRemRangeByScore('key', 0, 3); /* 2 */
~~~

**Note:** `zDeleteRangeByScore` and `zRemoveRangeByScore` are an alias for `zRemRangeByScore` and will be removed in future versions of phpredis.

### zRevRange
-----
_**Description**_: Returns the elements of the sorted set stored at the specified key in the range [start, end] in reverse order. start and stop are interpreted as zero-based indices:  
`0` the first element, `1` the second ...  
`-1` the last element, `-2` the penultimate ...

##### *Parameters*
*key*  
*start*: long  
*end*: long  
*withscores*: bool = false

##### *Return value*
*Array* containing the values in specified range.

##### *Example*
~~~php
$redis->zAdd('key', 0, 'val0');
$redis->zAdd('key', 2, 'val2');
$redis->zAdd('key', 10, 'val10');
$redis->zRevRange('key', 0, -1); /* ['val10', 'val2', 'val0'] */

// with scores
$redis->zRevRange('key', 0, -1, true); /* ['val10' => 10, 'val2' => 2, 'val0' => 0] */
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
~~~php
$redis->zAdd('key', 2.5, 'val2');
$redis->zScore('key', 'val2'); /* 2.5 */
~~~

### zunionstore, zUnion
-----
_**Description**_: Creates an union of sorted sets given in second argument. The result of the union will be stored in the sorted set defined by the first argument.

The third optional argument defines `weights` to apply to the sorted sets in input. In this case, the `weights` will be multiplied by the score of each element in the sorted set before applying the aggregation.
The forth argument defines the `AGGREGATE` option which specify how the results of the union are aggregated.

##### *Parameters*
*keyOutput*  
*arrayZSetKeys*  
*arrayWeights*  
*aggregateFunction* Either "SUM", "MIN", or "MAX": defines the behaviour to use on duplicate entries during the zunionstore.

##### *Return value*
*LONG* The number of values in the new sorted set.

##### *Example*
~~~php
$redis->del('k1');
$redis->del('k2');
$redis->del('k3');
$redis->del('ko1');
$redis->del('ko2');
$redis->del('ko3');

$redis->zAdd('k1', 0, 'val0');
$redis->zAdd('k1', 1, 'val1');

$redis->zAdd('k2', 2, 'val2');
$redis->zAdd('k2', 3, 'val3');

$redis->zunionstore('ko1', ['k1', 'k2']); /* 4, 'ko1' => ['val0', 'val1', 'val2', 'val3'] */

/* Weighted zunionstore */
$redis->zunionstore('ko2', ['k1', 'k2'], [1, 1]); /* 4, 'ko2' => ['val0', 'val1', 'val2', 'val3'] */
$redis->zunionstore('ko3', ['k1', 'k2'], [5, 1]); /* 4, 'ko3' => ['val0', 'val2', 'val3', 'val1'] */
~~~

**Note:** `zUnion` is an alias for `zunionstore` and will be removed in future versions of phpredis.

### zScan
-----
_**Description**_: Scan a sorted set for members, with optional pattern and count

##### *Parameters*
*key*: String, the set to scan  
*iterator*: Long (reference), initialized to NULL  
*pattern*: String (optional), the pattern to match  
*count*: How many keys to return per iteration (Redis might return a different number)

##### *Return value*
*Array, boolean* PHPRedis will return matching keys from Redis, or FALSE when iteration is complete

##### *Example*
~~~php
$it = NULL;
$redis->setOption(Redis::OPT_SCAN, Redis::SCAN_RETRY);
while($arr_matches = $redis->zScan('zset', $it, '*pattern*')) {
    foreach($arr_matches as $str_mem => $f_score) {
        echo "Key: $str_mem, Score: $f_score\n";
    }
}
~~~

## HyperLogLogs

### pfAdd
-----

_**Description**_:  Adds the specified elements to the specified HyperLogLog.

##### *Prototype*  
~~~php
$redis->pfAdd($key, Array $elements);
~~~

##### *Parameters*
_Key_  
_Array of values_  

##### *Return value*
*Integer*:  1 if at least 1 HyperLogLog internal register was altered. 0 otherwise.

##### *Example*
~~~php
$redis->pfAdd('hll', ['a', 'b', 'c']); // (int) 1
$redis->pfAdd('hll', ['a', 'b']); // (int) 0
~~~

### pfCount
-----

_**Description**_:  Return the approximated cardinality of the set(s) observed by the HyperLogLog at key(s).

##### *Prototype*  
~~~php
$redis->pfCount($key);
$redis->pfCount(Array $keys);
~~~

##### *Parameters*
_Key_ or _Array of keys_  

##### *Return value*
*Integer*:  The approximated number of unique elements observed via [pfAdd](#pfAdd).

##### *Example*
~~~php
$redis->pfAdd('hll1', ['a', 'b', 'c']); // (int) 1
$redis->pfCount('hll1'); // (int) 3

$redis->pfAdd('hll2', ['d', 'e', 'a']); // (int) 1
$redis->pfCount('hll2'); // (int) 3

$redis->pfCount(['hll1', 'hll2']); // (int) 5
~~~

### pfMerge
-----

_**Description**_:  Merge N different HyperLogLogs into a single one.

##### *Prototype*  
~~~php
$redis->pfMerge($destkey, Array $sourceKeys);
~~~

##### *Parameters*
_Destination Key_  
_Array of Source Keys_  

##### *Return value*
*BOOL*: `TRUE` on success, `FALSE` on error.

##### *Example*
~~~php
$redis->pfAdd('hll1', ['a', 'b', 'c']); // (int) 1
$redis->pfAdd('hll2', ['d', 'e', 'a']); // (int) 1

$redis->pfMerge('hll3', ['hll1', 'hll2']); // true

$redis->pfCount('hll3'); // (int) 5
~~~

## Geocoding

### geoAdd
-----

##### *Prototype*  
~~~php
$redis->geoAdd($key, $longitude, $latitude, $member [, $longitude, $latitude, $member, ...]);
~~~

_**Description**_:  Add one or more geospatial items to the specified key.  This function must be called with at least one _longitude, latitude, member_ triplet.

##### *Return value*
*Integer*:  The number of elements added to the geospatial key.

##### *Example*
~~~php
$redis->del("myplaces");

/* Since the key will be new, $result will be 2 */
$result = $redis->geoAdd(
    "myplaces",
    -122.431, 37.773, "San Francisco",
    -157.858, 21.315, "Honolulu"
);
~~~  

### geoHash
-----

##### *Prototype*
~~~php
$redis->geoHash($key, $member [, $member, $member, ...]);
~~~

_**Description**_:  Retrieve Geohash strings for one or more elements of a geospatial index.  

##### *Return value*  
*Array*:  One or more Redis Geohash encoded strings.  

##### *Example*  
~~~php
$redis->geoAdd("hawaii", -157.858, 21.306, "Honolulu", -156.331, 20.798, "Maui");
$hashes = $redis->geoHash("hawaii", "Honolulu", "Maui");
var_dump($hashes);
~~~

##### *Output*  
~~~
array(2) {
  [0]=>
  string(11) "87z9pyek3y0"
  [1]=>
  string(11) "8e8y6d5jps0"
}
~~~

### geoPos
-----

##### *Prototype*  
~~~php
$redis->geoPos($key, $member [, $member, $member, ...]);
~~~

_**Description**_:  Return longitude, latitude positions for each requested member.

##### *Return value*  
*Array*:  One or more longitude/latitude positions

##### *Example*  
~~~php
$redis->geoAdd("hawaii", -157.858, 21.306, "Honolulu", -156.331, 20.798, "Maui");
$positions = $redis->geoPos("hawaii", "Honolulu", "Maui");
var_dump($positions);
~~~

##### *Output*  
~~~
array(2) {
  [0]=>
  array(2) {
    [0]=>
    string(22) "-157.85800248384475708"
    [1]=>
    string(19) "21.3060004581273077"
  }
  [1]=>
  array(2) {
    [0]=>
    string(22) "-156.33099943399429321"
    [1]=>
    string(20) "20.79799924753607598"
  }
}
~~~

### GeoDist  
-----

##### *Prototype*  
~~~php
$redis->geoDist($key, $member1, $member2 [, $unit]);
~~~


_**Description**_:  Return the distance between two members in a geospatial set.  If units are passed it must be one of the following values:

* 'm' => Meters
* 'km' => Kilometers
* 'mi' => Miles
* 'ft' => Feet

##### *Return value*
*Double*:  The distance between the two passed members in the units requested (meters by default).  

##### *Example*
~~~php
$redis->geoAdd("hawaii", -157.858, 21.306, "Honolulu", -156.331, 20.798, "Maui");

$meters = $redis->geoDist("hawaii", "Honolulu", "Maui");
$kilometers = $redis->geoDist("hawaii", "Honolulu", "Maui", 'km');
$miles = $redis->geoDist("hawaii", "Honolulu", "Maui", 'mi');
$feet = $redis->geoDist("hawaii", "Honolulu", "Maui", 'ft');

echo "Distance between Honolulu and Maui:\n";
echo "  meters    : $meters\n";
echo "  kilometers: $kilometers\n";
echo "  miles     : $miles\n";
echo "  feet      : $feet\n";

/* Bad unit */
$inches = $redis->geoDist("hawaii", "Honolulu", "Maui", 'in');
echo "Invalid unit returned:\n";
var_dump($inches);
~~~  

##### *Output*  
~~~
Distance between Honolulu and Maui:
  meters    : 168275.204
  kilometers: 168.2752
  miles     : 104.5616
  feet      : 552084.0028
Invalid unit returned:
bool(false)
~~~

### geoRadius
-----

##### *Prototype*
~~~php
$redis->geoRadius($key, $longitude, $latitude, $radius, $unit [, Array $options]);
~~~

_**Description**_:  Return members of a set with geospatial information that are within the radius specified by the caller. 

##### *Options Array*
The georadius command can be called with various options that control how Redis returns results.  The following table describes the options phpredis supports.  All options are case insensitive.  

| Key       | Value       | Description
| :---      | :---        | :---- |
| COUNT     | integer > 0 | Limit how many results are returned
|           | WITHCOORD   | Return longitude and latitude of matching members
|           | WITHDIST    | Return the distance from the center
|           | WITHHASH    | Return the raw geohash-encoded score
|           | ASC         | Sort results in ascending order
|           | DESC        | Sort results in descending order
| STORE     | _key_       | Store results in _key_
| STOREDIST | _key_       | Store the results as distances in _key_

 *Note*:  It doesn't make sense to pass both `ASC` and `DESC` options but if both are passed the last one passed will be used.  
 *Note*:  When using `STORE[DIST]` in Redis Cluster, the store key must has to the same slot as the query key or you will get a `CROSSLOT` error.

##### *Return value*
*Mixed*:  When no `STORE` option is passed, this function returns an array of results.  If it is passed this function returns the number of stored entries.
 
##### *Example*
~~~php
/* Add some cities */
$redis->geoAdd("hawaii", -157.858, 21.306, "Honolulu", -156.331, 20.798, "Maui");

echo "Within 300 miles of Honolulu:\n";
var_dump($redis->geoRadius("hawaii", -157.858, 21.306, 300, 'mi'));

echo "\nWithin 300 miles of Honolulu with distances:\n";
$options = ['WITHDIST'];
var_dump($redis->geoRadius("hawaii", -157.858, 21.306, 300, 'mi', $options));

echo "\nFirst result within 300 miles of Honolulu with distances:\n";
$options['count'] = 1;
var_dump($redis->geoRadius("hawaii", -157.858, 21.306, 300, 'mi', $options));

echo "\nFirst result within 300 miles of Honolulu with distances in descending sort order:\n";
$options[] = 'DESC';
var_dump($redis->geoRadius("hawaii", -157.858, 21.306, 300, 'mi', $options));
~~~

##### *Output*
~~~
Within 300 miles of Honolulu:
array(2) {
  [0]=>
  string(8) "Honolulu"
  [1]=>
  string(4) "Maui"
}

Within 300 miles of Honolulu with distances:
array(2) {
  [0]=>
  array(2) {
    [0]=>
    string(8) "Honolulu"
    [1]=>
    string(6) "0.0002"
  }
  [1]=>
  array(2) {
    [0]=>
    string(4) "Maui"
    [1]=>
    string(8) "104.5615"
  }
}

First result within 300 miles of Honolulu with distances:
array(1) {
  [0]=>
  array(2) {
    [0]=>
    string(8) "Honolulu"
    [1]=>
    string(6) "0.0002"
  }
}

First result within 300 miles of Honolulu with distances in descending sort order:
array(1) {
  [0]=>
  array(2) {
    [0]=>
    string(4) "Maui"
    [1]=>
    string(8) "104.5615"
  }
}
~~~

### geoRadiusByMember

##### *Prototype*
~~~php
$redis->geoRadiusByMember($key, $member, $radius, $units [, Array $options]);
~~~

_**Description**_: This method is identical to [geoRadius](#georadius) except that instead of passing a longitude and latitude as the "source" you pass an existing member in the geospatial set.

##### *Options Array*
See [geoRadius](#georadius) command for options array.

##### *Return value*
*Array*:  The zero or more entries that are close enough to the member given the distance and radius specified.  

##### *Example*
~~~php
$redis->geoAdd("hawaii", -157.858, 21.306, "Honolulu", -156.331, 20.798, "Maui");

echo "Within 300 miles of Honolulu:\n";
var_dump($redis->geoRadiusByMember("hawaii", "Honolulu", 300, 'mi'));

echo "\nFirst match within 300 miles of Honolulu:\n";
var_dump($redis->geoRadiusByMember("hawaii", "Honolulu", 300, 'mi', ['count' => 1]));
~~~

##### *Output*
~~~
Within 300 miles of Honolulu:
array(2) {
  [0]=>
  string(8) "Honolulu"
  [1]=>
  string(4) "Maui"
}

First match within 300 miles of Honolulu:
array(1) {
  [0]=>
  string(8) "Honolulu"
}
~~~

## Streams

* [xAck](#xack) - Acknowledge one or more pending messages
* [xAdd](#xadd) - Add a message to a stream
* [xClaim](#xclaim) - Acquire ownership of a pending message
* [xDel](#xdel) - Remove a message from a stream
* [xGroup](#xgroup) - Manage consumer groups
* [xInfo](#xinfo) - Get information about a stream
* [xLen](#xlen) - Get the length of a stream
* [xPending](#xpending) - Inspect pending messages in a stream
* [xRange](#xrange) - Query a range of messages from a stream
* [xRead](#xread) - Read message(s) from a stream
* [xReadGroup](#xreadgroup) - Read stream messages with a group and consumer
* [xRevRange](#xrevrange) - Query one or more messages from end to start
* [xTrim](#xtrim) - Trim a stream's size

### xAck
-----

##### *Prototype*
~~~php
$obj_redis->xAck($stream, $group, $arr_messages);
~~~

_**Description**_:  Acknowledge one or more messages on behalf of a consumer group.

##### *Return value*
*long*:  The number of messages Redis reports as acknowledged.

##### *Example*
~~~php
$obj_redis->xAck('stream', 'group1', ['1530063064286-0', '1530063064286-1']);
~~~

### xAdd
-----

##### *Prototype*
~~~php
$obj_redis->xAdd($str_key, $str_id, $arr_message[, $i_maxlen, $boo_approximate]);
~~~

_**Description**_:  Add a message to a stream

##### *Return value*
*String*:  The added message ID

##### *Example*
~~~php
$obj_redis->xAdd('mystream', "*", ['field' => 'value']);
$obj_redis->xAdd('mystream', "*", ['field' => 'value'], 1000); // set max length of stream to 1000
$obj_redis->xAdd('mystream', "*", ['field' => 'value'], 1000, true); // set max length of stream to ~1000
~~~

### xClaim
-----

##### *Prototype*
~~~php
$obj_redis->xClaim($str_key, $str_group, $str_consumer, $min_idle_time, $arr_ids, [$arr_options]);
~~~

_**Description**_:  Claim ownership of one or more pending messages.

#### *Options Array*
~~~php
$options = [
    /* Note:  'TIME', and 'IDLE' are mutually exclusive */
    'IDLE' => $value, /* Set the idle time to $value ms  */,
    'TIME' => $value, /* Set the idle time to now - $value */
    'RETRYCOUNT' => $value, /* Update message retrycount to $value */
    'FORCE', /* Claim the message(s) even if they're not pending anywhere */
    'JUSTID', /* Instruct Redis to only return IDs */
];
~~~

##### *Return value*
*Array*:  Either an array of message IDs along with corresponding data, or just an array of IDs (if the 'JUSTID' option was passed).

##### *Example*
~~~php
$ids = ['1530113681011-0', '1530113681011-1', '1530113681011-2'];

/* Without any options */
$obj_redis->xClaim(
    'mystream', 'group1', 'myconsumer1', 0, $ids
);

/* With options */
$obj_redis->xClaim(
    'mystream', 'group1', 'myconsumer2', 0, $ids,
    [
        'IDLE' => time() * 1000,
        'RETRYCOUNT' => 5,
        'FORCE',
        'JUSTID'
    ]
);
~~~

### xDel
-----

##### *Prototype*
~~~php
$obj_redis->xDel($str_key, $arr_ids);
~~~

_**Description**_:  Delete one or more messages from a stream.

##### *Return value*
*long*:  The number of messages removed

##### *Example*
~~~php
$obj_redis->xDel('mystream', ['1530115304877-0', '1530115305731-0']);
~~~

### xGroup
-----

##### *Prototype*
~~~php
$obj_redis->xGroup('HELP');
$obj_redis->xGroup('CREATE', $str_key, $str_group, $str_msg_id, [$boo_mkstream]);
$obj_redis->xGroup('SETID', $str_key, $str_group, $str_msg_id);
$obj_redis->xGroup('DESTROY', $str_key, $str_group);
$obj_redis->xGroup('DELCONSUMER', $str_key, $str_group, $str_consumer_name);
~~~

_**Description**_:  This command is used in order to create, destroy, or manage consumer groups.

##### *Return value*
*Mixed*:  This command returns different types depending on the specific XGROUP command executed.

##### *Example*
~~~php
$obj_redis->xGroup('CREATE', 'mystream', 'mygroup', 0);
$obj_redis->xGroup('CREATE', 'mystream', 'mygroup2', 0, true); /* Create stream if non-existent. */
$obj_redis->xGroup('DESTROY', 'mystream', 'mygroup');
~~~

### xInfo
-----

##### *Prototype*
~~~php
$obj_redis->xInfo('CONSUMERS', $str_stream, $str_group);
$obj_redis->xInfo('GROUPS', $str_stream);
$obj_redis->xInfo('STREAM', $str_stream);
$obj_redis->xInfo('HELP');
~~~

_**Description**_:  Get information about a stream or consumer groups.

##### *Return value*
*Mixed*:  This command returns different types depending on which subcommand is used.

##### *Example*
~~~php
$obj_redis->xInfo('STREAM', 'mystream');
~~~

### xLen
-----

##### *Prototype*
~~~php
$obj_redis->xLen($str_stream);
~~~

_**Description**_:  Get the length of a given stream

##### *Return value*
*Long*:  The number of messages in the stream.

##### *Example*
~~~php
$obj_redis->xLen('mystream');
~~~

### xPending
-----

##### *Prototype*
~~~php
$obj_redis->xPending($str_stream, $str_group [, $str_start, $str_end, $i_count, $str_consumer]);
~~~

_**Description**_:  Get information about pending messages in a given stream.

##### *Return value*
*Array*:  Information about the pending messages, in various forms depending on the specific invocation of XPENDING.

##### *Examples*
~~~php
$obj_redis->xPending('mystream', 'mygroup');
$obj_redis->xPending('mystream', 'mygroup', '-', '+', 1, 'consumer-1');
~~~

### xRange
-----

##### *Prototype*
~~~php
$obj_redis->xRange($str_stream, $str_start, $str_end [, $i_count]);
~~~

_**Description**_:  Get a range of messages from a given stream.

##### *Return value*
*Array*:  The messages in the stream within the requested range.

##### *Example*
~~~php
/* Get everything in this stream */
$obj_redis->xRange('mystream', '-', '+');

/* Only the first two messages */
$obj_redis->xRange('mystream', '-', '+', 2);
~~~

### xRead
-----

##### *Prototype*
~~~php
$obj_redis->xRead($arr_streams [, $i_count, $i_block);
~~~

_**Description**_:  Read data from one or more streams and only return IDs greater than sent in the command.

##### *Return value*
*Array*:  The messages in the stream newer than the IDs passed to Redis (if any).

##### *Example*
~~~php
$obj_redis->xRead(['stream1' => '1535222584555-0', 'stream2' => '1535222584555-0']);

/* --- Possible output  ---
Array
(
    [stream1] => Array
        (
            [1535222584555-1] => Array
                (
                    [key:1] => val:1
                )

        )

    [stream2] => Array
        (
            [1535222584555-1] => Array
                (
                    [key:1] => val:1
                )

        )

)
*/

// Receive only new message ($ = last id) and wait for one new message unlimited time
$obj_redis->xRead(['stream1' => '$'], 1, 0);
~~~

### xReadGroup
-----

##### *Prototype*
~~~php
$obj_redis->xReadGroup($str_group, $str_consumer, $arr_streams [, $i_count, $i_block]);
~~~

_**Description**_:  This method is similar to xRead except that it supports reading messages for a specific consumer group.

##### *Return value*
*Array*:  The messages delivered to this consumer group (if any).

##### *Examples*
~~~php
/* Consume messages for 'mygroup', 'consumer1' */
$obj_redis->xReadGroup('mygroup', 'consumer1', ['s1' => 0, 's2' => 0]);

/* Consume messages for 'mygroup', 'consumer1' which were not consumed yet by the group */
$obj_redis->xReadGroup('mygroup', 'consumer1', ['s1' => '>', 's2' => '>']);

/* Read a single message as 'consumer2' wait for up to a second until a message arrives. */
$obj_redis->xReadGroup('mygroup', 'consumer2', ['s1' => 0, 's2' => 0], 1, 1000);
~~~

### xRevRange
-----

##### *Prototype*
~~~php
$obj_redis->xRevRange($str_stream, $str_end, $str_start [, $i_count]);
~~~

_**Description**_:  This is identical to xRange except the results come back in reverse order.  Also note that Redis reverses the order of "start" and "end".

##### *Return value*
*Array*:  The messages in the range specified.

##### *Example*
~~~php
$obj_redis->xRevRange('mystream', '+', '-');
~~~

### xTrim
-----

##### *Prototype*
~~~php
$obj_redis->xTrim($str_stream, $i_max_len [, $boo_approximate]);
~~~

_**Description**_:  Trim the stream length to a given maximum.  If the "approximate" flag is pasesed, Redis will use your size as a hint but only trim trees in whole nodes (this is more efficient).

##### *Return value*
*long*:  The number of messages trimmed from the stream.

##### *Example*
~~~php
/* Trim to exactly 100 messages */
$obj_redis->xTrim('mystream', 100);

/* Let Redis approximate the trimming */
$obj_redis->xTrim('mystream', 100, true);
~~~

## Pub/sub

* [pSubscribe](#psubscribe) - Subscribe to channels by pattern
* [publish](#publish) - Post a message to a channel
* [subscribe](#subscribe) - Subscribe to channels
* [pubSub](#pubsub) - Introspection into the pub/sub subsystem

### pSubscribe
-----
_**Description**_: Subscribe to channels by pattern

##### *Parameters*
*patterns*: An array of patterns to match  
*callback*: Either a string or an array with an object and method.  The callback will get four arguments ($redis, $pattern, $channel, $message)  
*return value*: Mixed.  Any non-null return value in the callback will be returned to the caller.  
##### *Example*
~~~php
function pSubscribe($redis, $pattern, $chan, $msg) {
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
*message*: string

##### *Example*
~~~php
$redis->publish('chan-1', 'hello, world!'); // send message.
~~~

### subscribe
-----
_**Description**_: Subscribe to channels. Warning: this function will probably change in the future.

##### *Parameters*
*channels*: an array of channels to subscribe to  
*callback*: either a string or an Array($instance, 'method_name'). The callback function receives 3 parameters: the redis instance, the channel name, and the message.
*return value*:  Mixed.  Any non-null return value in the callback will be returned to the caller.
##### *Example*
~~~php
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

$redis->subscribe(['chan-1', 'chan-2', 'chan-3'], 'f'); // subscribe to 3 chans
~~~

### pubSub
-----
_**Description**_: A command allowing you to get information on the Redis pub/sub system.

##### *Parameters*
*keyword*: String, which can be: "channels", "numsub", or "numpat"  
*argument*:  Optional, variant.  For the "channels" subcommand, you can pass a string pattern.  For "numsub" an array of channel names.

##### *Return value*
*CHANNELS*: Returns an array where the members are the matching channels.  
*NUMSUB*:  Returns a key/value array where the keys are channel names and values are their counts.  
*NUMPAT*:  Integer return containing the number active pattern subscriptions

##### *Example*
~~~php
$redis->pubSub("channels"); /*All channels */
$redis->pubSub("channels", "*pattern*"); /* Just channels matching your pattern */
$redis->pubSub("numsub", ["chan1", "chan2"]); /*Get subscriber counts for 'chan1' and 'chan2'*/
$redis->pubSub("numpat"); /* Get the number of pattern subscribers */


~~~

## Generic
1. [rawCommand](#rawcommand) - Execute any generic command against the server.

### rawCommand
-----
_**Description**_: A method to execute any arbitrary command against the a Redis server

##### *Parameters*
This method is variadic and takes a dynamic number of arguments of various types (string, long, double), but must be passed at least one argument (the command keyword itself).

##### *Return value*
The return value can be various types depending on what the server itself returns.   No post processing is done to the returned value and must be handled by the client code.

##### *Example*
```php
/* Returns: true */
$redis->rawCommand("set", "foo", "bar");

/* Returns: "bar" */
$redis->rawCommand("get", "foo");

/* Returns: 3 */
$redis->rawCommand("rpush", "mylist", "one", 2, 3.5));

/* Returns: ["one", "2", "3.5000000000000000"] */
$redis->rawCommand("lrange", "mylist", 0, -1);
```

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
~~~php
$ret = $redis->multi()
    ->set('key1', 'val1')
    ->get('key1')
    ->set('key2', 'val2')
    ->get('key2')
    ->exec();

/*
$ret == Array(0 => TRUE, 1 => 'val1', 2 => TRUE, 3 => 'val2');
*/
~~~

### watch, unwatch
-----
_**Description**_: Watches a key for modifications by another client.

If the key is modified between `WATCH` and `EXEC`, the MULTI/EXEC transaction will fail (return `FALSE`). `unwatch` cancels all the watching of all keys by this client.

##### *Parameters*
*keys*: string for one key or array for a list of keys

##### *Example*
~~~php
$redis->watch('x'); // or for a list of keys: $redis->watch(['x','another key']);
/* long code here during the execution of which other clients could well modify `x` */
$ret = $redis->multi()
    ->incr('x')
    ->exec();
/*
$ret = FALSE if x has been modified between the call to WATCH and the call to EXEC.
*/
~~~



## Scripting

* [eval](#eval) - Evaluate a LUA script serverside
* [evalSha](#evalsha) - Evaluate a LUA script serverside, from the SHA1 hash of the script instead of the script itself
* [script](#script) - Execute the Redis SCRIPT command to perform various operations on the scripting subsystem
* [getLastError](#getlasterror) - The last error message (if any)
* [clearLastError](#clearlasterror) - Clear the last error message
* [_prefix](#_prefix) - A utility method to prefix the value with the prefix setting for phpredis
* [_unserialize](#_unserialize) - A utility method to unserialize data with whatever serializer is set up
* [_serialize](#_serialize) - A utility method to serialize data with whatever serializer is set up

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
~~~php
$redis->eval("return 1"); // Returns an integer: 1
$redis->eval("return {1,2,3}"); // Returns [1,2,3]
$redis->del('mylist');
$redis->rpush('mylist','a');
$redis->rpush('mylist','b');
$redis->rpush('mylist','c');
// Nested response:  [1,2,3,['a','b','c']];
$redis->eval("return {1,2,3,redis.call('lrange','mylist',0,-1)}");
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
~~~php
$script = 'return 1';
$sha = $redis->script('load', $script);
$redis->evalSha($sha); // Returns 1
~~~

### script
-----
_**Description**_: Execute the Redis SCRIPT command to perform various operations on the scripting subsystem.

##### *Usage*
~~~php
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
~~~php
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
~~~php
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
~~~php
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
~~~php
$redis->setOption(Redis::OPT_PREFIX, 'my-prefix:');
$redis->_prefix('my-value'); // Will return 'my-prefix:my-value'
~~~

### _serialize
-----
_**Description**_: A utility method to serialize values manually.

This method allows you to serialize a value with whatever serializer is configured, manually.
This can be useful for serialization/unserialization of data going in and out of EVAL commands
as phpredis can't automatically do this itself.  Note that if no serializer is set, phpredis
will change Array values to 'Array', and Objects to 'Object'.

##### *Parameters*
*value*:  Mixed.  The value to be serialized

##### *Examples*
~~~php
$redis->setOption(Redis::OPT_SERIALIZER, Redis::SERIALIZER_NONE);
$redis->_serialize("foo"); // returns "foo"
$redis->_serialize([]); // Returns "Array"
$redis->_serialize(new stdClass()); // Returns "Object"

$redis->setOption(Redis::OPT_SERIALIZER, Redis::SERIALIZER_PHP);
$redis->_serialize("foo"); // Returns 's:3:"foo";'
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
~~~php
$redis->setOption(Redis::OPT_SERIALIZER, Redis::SERIALIZER_PHP);
$redis->_unserialize('a:3:{i:0;i:1;i:1;i:2;i:2;i:3;}'); // Will return [1,2,3]
~~~



## Introspection

### isConnected
-----
_**Description**_:  A method to determine if a phpredis object thinks it's connected to a server

##### *Parameters*
None

##### *Return value*
*Boolean* Returns TRUE if phpredis thinks it's connected and FALSE if not

### getHost
-----
_**Description**_:  Retrieve our host or unix socket that we're connected to

##### *Parameters*
None

##### *Return value*
*Mixed* The host or unix socket we're connected to or FALSE if we're not connected


### getPort
-----
_**Description**_:  Get the port we're connected to

##### *Parameters*
None

##### *Return value*
*Mixed* Returns the port we're connected to or FALSE if we're not connected

### getDbNum
-----
_**Description**_:  Get the database number phpredis is pointed to

##### *Parameters*
None

##### *Return value*
*Mixed* Returns the database number (LONG) phpredis thinks it's pointing to or FALSE if we're not connected

### getTimeout
-----
_**Description**_:  Get the (write) timeout in use for phpredis

##### *Parameters*
None

##### *Return value*
*Mixed* The timeout (DOUBLE) specified in our connect call or FALSE if we're not connected

### getReadTimeout
_**Description**_:  Get the read timeout specified to phpredis or FALSE if we're not connected

##### *Parameters*
None

##### *Return value*
*Mixed*  Returns the read timeout (which can be set using setOption and Redis::OPT_READ_TIMEOUT) or FALSE if we're not connected

### getPersistentID
-----
_**Description**_:  Gets the persistent ID that phpredis is using

##### *Parameters*
None

##### *Return value*
*Mixed* Returns the persistent id phpredis is using (which will only be set if connected with pconnect), NULL if we're not
using a persistent ID, and FALSE if we're not connected

### getAuth
-----
_**Description**_:  Get the password used to authenticate the phpredis connection

### *Parameters*
None

### *Return value*
*Mixed*  Returns the password used to authenticate a phpredis session or NULL if none was used, and FALSE if we're not connected
