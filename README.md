# PhpRedis

[![Build Status](https://github.com/phpredis/phpredis/actions/workflows/ci.yml/badge.svg)](https://github.com/phpredis/phpredis/actions/workflows/ci.yml)
[![Coverity Scan Build Status](https://scan.coverity.com/projects/13205/badge.svg)](https://scan.coverity.com/projects/phpredis-phpredis)
[![PHP version](https://img.shields.io/badge/php-%3E%3D%207.4-8892BF.svg)](https://github.com/phpredis/phpredis)

The phpredis extension provides an API for communicating with the [Redis](http://redis.io/) key-value store. [Valkey](https://valkey.io/) and [KeyDB](https://docs.keydb.dev/) are supported as well.

It is released under the [PHP License, version 3.01](http://www.php.net/license/3_01.txt).

You can send comments, patches, questions [here on github](https://github.com/phpredis/phpredis/issues), to [Michael](mailto:michael.grunder@gmail.com) ([Twitter](https://twitter.com/grumi78), <a rel="me" href="https://phpc.social/@mgrunder">Mastodon</a>), [Pavlo](mailto:p.yatsukhnenko@gmail.com) ([@yatsukhnenko](https://twitter.com/yatsukhnenko)), or [Nicolas](mailto:n.favrefelix@gmail.com) ([@yowgi](https://twitter.com/yowgi)).

## Sponsors

<div align="center">
  <br>
  <a href="https://relay.so">
    <img src="https://s3.us-east-1.amazonaws.com/assets.relay.so/img/wordmark-purple.svg" align="center" alt="The next-generation caching layer for PHP." width="275">
  </a>
  <br><br><br>
  <a href="https://objectcache.pro"><img src="https://objectcache.pro/assets/wordmark-padded.png" align="center" alt="Object Cache Pro" width="200"></a>&nbsp;
  <a href="https://audiomack.com"><img src="https://styleguide.audiomack.com/assets/dl/inline-orange-large.png" align="center" alt="Audiomack.com" width="150"></a>&nbsp;
  <a href="https://bluehost.com"><img src="https://upload.wikimedia.org/wikipedia/commons/2/22/Bluehost-logo.png" align="center" alt="Bluehost.com" width="150"></a>
  <a href="https://openlms.net"><img src="https://support.openlms.net/hc/theming_assets/01HZPV3CVP3Y6P7G6MT14NP6V1" align="center" alt="OpenLMS.net" width="125"></a>
  <br><br>
</div>

## Become a Sponsor
PhpRedis will always be free and open source software and if you or your company has found it useful please consider supporting the project. Developing a large, complex, and performant library like PhpRedis takes a great deal of time and effort, and support is greatly appreciated! :heart:

The ongoing development of PhpRedis is made possible thanks to the generous support of [Relay](https://relay.so), which funds the vast majority of work on the project. Relay is a high-performance in-memory cache and drop-in replacement for PhpRedis, which handles millions of requests per second without breaking a sweat.

The best way to support the project is through [GitHub Sponsors](https://github.com/sponsors/michael-grunder). Many of the reward tiers grant access to our [Slack](https://phpredis.slack.com) where [myself](https://github.com/michael-grunder) and [Pavlo](https://github.com/yatsukhnenko) are regularly available to answer questions. Additionally this will allow you to provide feedback on which fixes and features to prioritize. You can also make a one-time contribution with [PayPal](https://www.paypal.me/michaelgrunder/5).

## Table of contents
1. [Installing/Configuring](#installingconfiguring)
   * [Installation](#installation)
   * [PHP Session handler](#php-session-handler)
   * [Distributed Redis Array](./arrays.md#readme)
   * [Redis Cluster support](./cluster.md#readme)
   * [Redis Sentinel support](./sentinel.md#readme)
   * [Running the unit tests](#running-the-unit-tests)
2. [API Documentation](#api-documentation)
3. [Classes and methods](#classes-and-methods)
   * [Usage](#usage)
   * [Connection](#connection)
   * [Retry and backoff](#retry-and-backoff)
   * [Transactions](#transactions)
   * [Scripting](#scripting)
   * [Local Helper Methods](#local-helper-methods)
   * [Introspection](#introspection)

## Installing/Configuring

### Installation

For everything you should need to install PhpRedis on your system,
see the [INSTALL.md](./INSTALL.md) page.

### PHP Session handler

phpredis can be used to store PHP sessions. To do this, configure `session.save_handler` and `session.save_path` in your php.ini to tell phpredis where to store the sessions.

`session.save_path` can have a simple `host:port` format too, but you need to provide the `tcp://` scheme if you want to use the parameters. The following parameters are available:

* weight (integer): the weight of a host is used in comparison with the others to customize the session distribution on several hosts. If host A has twice the weight of host B, it will get twice the amount of sessions. In the example, *host1* stores 20% of all the sessions (1/(1+2+2)) while *host2* and *host3* each store 40% (2/(1+2+2)). The target host is determined once and for all at the start of the session, and doesn't change. The default weight is 1.
* timeout (float): the connection timeout to a redis host, expressed in seconds. If the host is unreachable in that amount of time, the session storage will be unavailable for the client. The default timeout is very high (86400 seconds).
* persistent (integer, should be 1 or 0): defines if a persistent connection should be used.
* prefix (string, defaults to "PHPREDIS_SESSION:"): used as a prefix to the Redis key in which the session is stored. The key is composed of the prefix followed by the session ID.
* auth (string, or an array with one or two elements): used to authenticate with the server prior to sending commands.
* database (integer): selects a different database.

Sessions have a lifetime expressed in seconds and stored in the INI variable "session.gc_maxlifetime". You can change it with [`ini_set()`](http://php.net/ini_set).
The session handler requires a version of Redis supporting `EX` and `NX` options of `SET` command (at least 2.6.12).
phpredis can also connect to a unix domain socket: `session.save_path = "unix:///var/run/redis/redis.sock?persistent=1&weight=1&database=0"`.

#### Examples

Multiple Redis servers:
```ini
session.save_handler = redis
session.save_path = "tcp://host1:6379?weight=1, tcp://host2:6379?weight=2&timeout=2.5, tcp://host3:6379?weight=2&read_timeout=2.5"
```

Login to Redis using username and password:
```ini
session.save_handler = redis
session.save_path = "tcp://127.0.0.1:6379?auth[]=user&auth[]=password"
```

Login to Redis using username, password, and set prefix:
```ini
session.save_handler = redis
session.save_path = "tcp://127.0.0.1:6379?auth[]=user&auth[]=password&prefix=user_PHPREDIS_SESSION:"
```

#### Session locking

**Support**: Locking feature is currently only supported for Redis setup with single master instance (e.g. classic master/slave Sentinel environment).
So locking may not work properly in RedisArray or RedisCluster environments.

The following INI variables can be used to configure session locking:
```ini
; Should the locking be enabled? Defaults to: 0.
redis.session.locking_enabled = 1
; How long should the lock live (in seconds)? Defaults to: value of max_execution_time.
redis.session.lock_expire = 60
; How long to wait between attempts to acquire lock, in microseconds (µs)?. Defaults to: 20000
redis.session.lock_wait_time = 50000
; Maximum number of times to retry (-1 means infinite). Defaults to: 100
redis.session.lock_retries = 2000
```

#### Session compression

The following INI variables can be used to configure session compression:
```ini
; Should session compression be enabled? Possible values are zstd, lzf, lz4, none. Defaults to: none
redis.session.compression = zstd
; What compression level should be used? Compression level depends on used library. For most deployments range 1-9 should be fine. Defaults to: 3
redis.session.compression_level = 3
```

### Running the unit tests

phpredis uses a small custom unit test suite for testing functionality of the various classes.  To run tests, simply do the following:

```bash
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
```

Note that it is possible to run only tests which match a substring of the test itself by passing the additional argument '--test <str>' when invoking.

```bash
# Just run the 'echo' test
php tests/TestRedis.php --class Redis --test echo
```

## API Documentation

The [API Documentation](https://phpredis.github.io/phpredis) are a work in progress, but will eventually replace our **ONE README TO RULE THEM ALL** docs.

## Classes and methods

### Usage

1. [Class Redis](#class-redis)
1. [Class RedisException](#class-redisexception)
1. [Predefined constants](#predefined-constants)

#### Class Redis
-----
_**Description**_: Creates a Redis client

###### *Example*

~~~php
$redis = new Redis();
~~~

Starting from version 6.0.0 it's possible to specify configuration options.
This allows to connect lazily to the server without explicitly invoking `connect` / `pconnect`.

###### *Example*

~~~php
$redis = new Redis([
    'host' => '127.0.0.1',
    'port' => 6379,
    'connectTimeout' => 2.5,
    'auth' => ['phpredis', 'phpredis'],
    'database' => 2,
    'ssl' => ['verify_peer' => false],
    'backoff' => [
        'algorithm' => Redis::BACKOFF_ALGORITHM_DECORRELATED_JITTER,
        'base' => 500,
        'cap' => 750,
    ],
]);
~~~

###### *Parameters* (optional)

| Parameter | Type | Description |
| --- | --- | --- |
| `host` | `string` | the hostname/FQDN/IP address or the path to a unix domain socket; since v5.0.0 it is possible to specify schema |
| `port` | `int` | actual port (e.g. `6379`) or `-1` (or `0`) for unix domain socket; setting to < `0` with non-UDS will assume default of `6379` |
| `connectTimeout` | `float` | value in seconds (default is 0 meaning unlimited) |
| `retryInterval` | `int` | value in milliseconds (optional) |
| `readTimeout` | `float` | value in seconds (default is 0 meaning unlimited) |
| `persistent` | `mixed` | If the value is a string it is used as a persistent ID. Non-strings are cast to boolean. |
| `auth` | `mixed` | authentication information |
| `database` | `int` | database number |
| `ssl` | `array` | SSL context options |

#### Class RedisException
-----
phpredis throws a [RedisException](#class-redisexception) object if it can't reach the Redis server. That can happen in case of connectivity issues,
if the Redis service is down, or if the Redis host is overloaded. In any other problematic case that does not involve an
unreachable server (such as a key not existing, an invalid command, etc), phpredis will return `FALSE`.

#### Predefined constants
-----
_**Description**_: Available Redis Constants

Redis data types, as returned by [type](#type)
~~~php
Redis::REDIS_STRING
Redis::REDIS_SET
Redis::REDIS_LIST
Redis::REDIS_ZSET
Redis::REDIS_HASH
Redis::REDIS_STREAM
Redis::REDIS_VECTORSET
Redis::REDIS_NOT_FOUND
~~~

### Connection

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

#### connect, open
-----
_**Description**_: Connects to a Redis instance. If phpredis cannot reach the server it will usually throw a [`RedisException`](#class-redisexception); only rarely will `FALSE` be returned without an exception.

###### *Parameters*

| Parameter | Type | Description |
| --- | --- | --- |
| `host` | `string` | The Redis server hostname or IP address. |
| `port` | `int` | The Redis server port. Defaults to 6379. |
| `timeout` | `float` | The connection timeout in seconds. Defaults to 0 (no timeout). |
| `persistent_id` | `?string` | If set, a persistent connection will be made with this ID. |
| `retry_interval` | `int` | The number of milliseconds to wait between connection attempts. |
| `read_timeout` | `float` | The read timeout in seconds. Defaults to 0 (no timeout). |
| `context` | `?array` | An optional stream context to use when connecting. See `\Redis::__construct()` for more details. |

###### *Return value*

| Type | Description |
| --- | --- |
| `bool` | `TRUE` on success. Failures typically raise a `RedisException`, though in some edge cases `FALSE` may be returned instead. |


###### *Example*

~~~php
$redis->connect('127.0.0.1', 6379);
$redis->connect('127.0.0.1'); // port 6379 by default
$redis->connect('tls://127.0.0.1', 6379); // enable transport level security.
$redis->connect('tls://127.0.0.1'); // enable transport level security, port 6379 by default.
$redis->connect('127.0.0.1', 6379, 2.5); // 2.5 sec timeout.
$redis->connect('/tmp/redis.sock'); // unix domain socket.
$redis->connect('127.0.0.1', 6379, 1, '', 100); // 1 sec timeout, 100ms delay between reconnection attempts.
$redis->connect('/tmp/redis.sock', 0, 1.5, NULL, 0, 1.5); // Unix socket with 1.5s timeouts (connect and read)

/* With PhpRedis >= 5.3.0 you can specify authentication and stream information on connect */
$redis->connect('127.0.0.1', 6379, 1, '', 0, 0, ['auth' => ['phpredis', 'phpredis']]);

/* TLS connections can customise the underlying PHP stream context */
$redis->connect('tls://redis.example.com', 6380, 1.5, null, 0, 0, [
    'auth' => ['app-user', 'strong-password'],
    'stream' => [
        'verify_peer' => true,                 // validate the server certificate against cafile/capath
        'verify_peer_name' => true,            // require the certificate common/SAN name to match peer_name
        'peer_name' => 'redis.example.com',    // expected hostname presented by the server certificate
        'cafile' => '/etc/ssl/redis-ca.pem',   // CA or bundle used to trust the server certificate
        'capath' => '/etc/ssl/certs',          // directory alternative to cafile
        'allow_self_signed' => false,          // set to true if you rely on a self-signed certificate
        'local_cert' => '/etc/ssl/client.crt', // client certificate for mutual TLS (optional)
        'local_pk' => '/etc/ssl/client.key',   // private key that matches local_cert (optional)
        'passphrase' => 'secret',              // passphrase for local_pk if it is encrypted (optional)
        'ciphers' => 'HIGH:!aNULL:!MD5',       // TLS cipher list provided to OpenSSL (optional)
    ],
]);

try {
    $redis->connect('redis.invalid', 6379);
} catch (RedisException $ex) {
    echo "Connection failed: {$ex->getMessage()}";
}
~~~

When you pass a `stream` key PhpRedis forwards the options to [`stream_socket_client`](https://www.php.net/manual/en/context.ssl.php).
Commonly used options include:

- `verify_peer`, `verify_peer_name`, `peer_name`: control server certificate validation behaviour.
- `cafile`/`capath`: provide the trusted certificate authority bundle when the default store is insufficient.
- `allow_self_signed`: permits self-signed certificates when set to `true`.
- `local_cert`, `local_pk`, `passphrase`: configure client-side certificates for mutual TLS.
- `ciphers`: restrict the negotiated TLS cipher suites.
- Any other SSL context option supported by PHP (e.g. `SNI_enabled`, `disable_compression`) can also be supplied.

The same array format can be used with `pconnect`/`popen`.

**Note:** `open` is an alias for `connect` and will be removed in future versions of phpredis.

#### pconnect, popen
-----
_**Description**_: Connects to a Redis instance or reuse a connection already established with `pconnect`/`popen`.

The connection will not be closed on end of request until the php process ends.
So be prepared for too many open FD's errors (specially on redis server side) when using persistent
connections on many servers connecting to one redis server.

Also more than one persistent connection can be made, identified by either host + port + timeout
or host + persistent_id or unix socket + timeout.

Since v4.2.1, it became possible to use connection pooling by setting INI variable `redis.pconnect.pooling_enabled` to 1.

This feature is not available in threaded versions. `pconnect` and `popen` then work like their non
persistent equivalents.

###### *Parameters*

| Parameter | Type | Description |
| --- | --- | --- |
| `host` | `string` | The Redis server hostname. |
| `port` | `int` | The Redis server port. |
| `timeout` | `float` | Connection timeout in seconds. |
| `persistent_id` | `?string` | An optional persistent ID to use for the connection. |
| `retry_interval` | `int` | The number of microseconds to wait before retrying a connection. |
| `read_timeout` | `float` | Read timeout in seconds. |
| `context` | `?array` | An optional stream context array. |

###### *Return value*

| Type | Description |
| --- | --- |
| `bool` | `TRUE` on success, `FALSE` on error. |


###### *Example*

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

#### auth
-----
_**Description**_: Authenticate the connection using a password or a username and password.
*Warning*: The password is sent in plain-text over the network.

###### *Parameters*

| Parameter | Type | Description |
| --- | --- | --- |
| `credentials` | `mixed` | A string password, or an array with one or two string elements. |

###### *Return value*

| Type | Description |
| --- | --- |
| `Redis\|bool` | `TRUE` if the connection is authenticated, `FALSE` otherwise. In order to authenticate with a username and password you need Redis >= 6.0. |


###### *Example*
~~~php
/* Authenticate with the password 'foobared' */
$redis->auth('foobared');

/* Authenticate with the username 'phpredis', and password 'haxx00r' */
$redis->auth(['phpredis', 'haxx00r']);

/* Authenticate with the password 'foobared' */
$redis->auth(['foobared']);

/* You can also use an associative array specifying user and pass */
$redis->auth(['user' => 'phpredis', 'pass' => 'phpredis']);
$redis->auth(['pass' => 'phpredis']);
~~~

#### select
-----
_**Description**_: Change the selected database for the current connection.

###### *Parameters*

| Parameter | Type | Description |
| --- | --- | --- |
| `db` | `int` | The database to select. Note that by default Redis has 16 databases (0-15). |

###### *Return value*

| Type | Description |
| --- | --- |
| `Redis\|bool` | `TRUE` in case of success, `FALSE` in case of failure. |

###### *Example*
See method for example: [move](#move)

#### swapdb
-----
_**Description**_:  Swap one Redis database with another atomically

###### *Parameters*

| Parameter | Type | Description |
| --- | --- | --- |
| `src` | `int` | The source database number |
| `dst` | `int` | The destination database number |

###### *Return value*

| Type | Description |
| --- | --- |
| `Redis\|bool` | `TRUE` on success and `FALSE` on failure. Requires Redis >= 4.0.0 |


###### *Example*
~~~php
$redis->swapdb(0, 1); /* Swaps DB 0 with DB 1 atomically */
~~~

#### close
-----
_**Description**_: Disconnects from the Redis instance.

*Note*: Closing a persistent connection requires PhpRedis >= 4.2.0.

###### *Parameters*

None.

###### *Return value*

| Type | Description |
| --- | --- |
| `bool` | `TRUE` on success, `FALSE` on failure. |


#### setOption
-----
_**Description**_: Set client option.

###### *Parameters*

| Parameter | Type | Description |
| --- | --- | --- |
| `option` | `int` | The option constant. |
| `value` | `mixed` | The option value. |

###### *Return value*

| Type | Description |
| --- | --- |
| `bool` | `TRUE` on success, `FALSE` on error. |


###### *Example*
~~~php
$redis->setOption(Redis::OPT_SERIALIZER, Redis::SERIALIZER_NONE);	  // Don't serialize data
$redis->setOption(Redis::OPT_SERIALIZER, Redis::SERIALIZER_PHP);	  // Use built-in serialize/unserialize
$redis->setOption(Redis::OPT_SERIALIZER, Redis::SERIALIZER_IGBINARY); // Use igBinary serialize/unserialize
$redis->setOption(Redis::OPT_SERIALIZER, Redis::SERIALIZER_MSGPACK);  // Use msgpack serialize/unserialize
$redis->setOption(Redis::OPT_SERIALIZER, Redis::SERIALIZER_JSON);  // Use JSON to serialize/unserialize

$redis->setOption(Redis::OPT_PREFIX, 'myAppName:');	// use custom prefix on all keys

/* Options for the SCAN family of commands, indicating whether to abstract
   empty results from the user. If set to SCAN_NORETRY (the default), phpredis
   will just issue one SCAN command at a time, sometimes returning an empty
   array of results. If set to SCAN_RETRY, phpredis will retry the scan command
   until keys come back OR Redis returns an iterator of zero
*/
$redis->setOption(Redis::OPT_SCAN, Redis::SCAN_NORETRY);
$redis->setOption(Redis::OPT_SCAN, Redis::SCAN_RETRY);

/* Scan can also be configured to automatically prepend the currently set PhpRedis
   prefix to any MATCH pattern. */
$redis->setOption(Redis::OPT_SCAN, Redis::SCAN_PREFIX);
$redis->setOption(Redis::OPT_SCAN, Redis::SCAN_NOPREFIX);
~~~


#### getOption
-----
_**Description**_: Get client option.

###### *Parameters*

| Parameter | Type | Description |
| --- | --- | --- |
| `option` | `int` | See command documentation. |

###### *Return value*

| Type | Description |
| --- | --- |
| `mixed` | The setting itself or false on failure |


###### *Example*
~~~php
// Retrieve the current serializer option
$redis->getOption(Redis::OPT_SERIALIZER);
~~~

#### ping
-----
_**Description**_: Check the current connection status.

###### *Prototype*
~~~php
$redis->ping([string $message]);
~~~

###### *Return value*

| Type | Description |
| --- | --- |
| `Redis\|string\|false` | This method returns `TRUE` on success, or the passed string if called with an argument. |


###### *Example*
~~~php
/* When called without an argument, PING returns `TRUE` */
$redis->ping();

/* If passed an argument, that argument is returned.  Here 'hello' will be returned */
$redis->ping('hello');
~~~

*Note*:  Prior to PhpRedis 5.0.0 this command simply returned the string `+PONG`.

#### echo
-----
_**Description**_: Sends a string to Redis, which replies with the same string

###### *Parameters*

| Parameter | Type | Description |
| --- | --- | --- |
| `str` | `string` | The string to echo |

###### *Return value*

| Type | Description |
| --- | --- |
| `string\|false` | The same message, or `false` on failure. |

### Retry and backoff

1. [Maximum retries](#maximum-retries)
1. [Backoff algorithms](#backoff-algorithms)


#### Maximum retries
You can set and get the maximum retries upon connection issues using the `OPT_MAX_RETRIES` option. Note that this is the number of _retries_, meaning if you set this option to _n_, there will be a maximum _n+1_ attempts overall. Defaults to 10.

###### *Example*

~~~php
$redis->setOption(Redis::OPT_MAX_RETRIES, 5);
$redis->getOption(Redis::OPT_MAX_RETRIES);
~~~

#### Backoff algorithms
You can set the backoff algorithm using the `Redis::OPT_BACKOFF_ALGORITHM` option and choose among the following algorithms described in this blog post by Marc Brooker from AWS: [Exponential Backoff And Jitter](https://aws.amazon.com/blogs/architecture/exponential-backoff-and-jitter):

* Default: `Redis::BACKOFF_ALGORITHM_DEFAULT`
* Decorrelated jitter: `Redis::BACKOFF_ALGORITHM_DECORRELATED_JITTER`
* Full jitter: `Redis::BACKOFF_ALGORITHM_FULL_JITTER`
* Equal jitter: `Redis::BACKOFF_ALGORITHM_EQUAL_JITTER`
* Exponential: `Redis::BACKOFF_ALGORITHM_EXPONENTIAL`
* Uniform: `Redis::BACKOFF_ALGORITHM_UNIFORM`
* Constant: `Redis::BACKOFF_ALGORITHM_CONSTANT`

These algorithms depend on the _base_ and _cap_ parameters, both in milliseconds, which you can set using the `Redis::OPT_BACKOFF_BASE` and `Redis::OPT_BACKOFF_CAP` options, respectively.

###### *Example*

~~~php
$redis->setOption(Redis::OPT_BACKOFF_ALGORITHM, Redis::BACKOFF_ALGORITHM_DECORRELATED_JITTER);
$redis->setOption(Redis::OPT_BACKOFF_BASE, 500); // base for backoff computation: 500ms
$redis->setOption(Redis::OPT_BACKOFF_CAP, 750); // backoff time capped at 750ms
~~~

Transactions

1. [multi, exec, discard](#multi-exec-discard) - Enter and exit transactional mode
2. [watch, unwatch](#watch-unwatch) - Watches a key for modifications by another client.

#### multi, exec, discard.
-----
_**Description**_: Enter and exit transactional mode.

###### *Parameters*

| Parameter | Type | Description |
| --- | --- | --- |
| `value` | `int` | The type of transaction to start. This can either be `Redis::MULTI` or `Redis::PIPELINE'. |

Defaults to `Redis::MULTI`. A `Redis::MULTI` block of commands runs as a single transaction; a `Redis::PIPELINE` block is simply transmitted faster to the server, but without any guarantee of atomicity. `discard` cancels a transaction.

###### *Return value*

| Type | Description |
| --- | --- |
| `Redis\|bool` | `multi()` returns the Redis instance and enters multi-mode. Once in multi-mode, all subsequent method calls return the same object until `exec()` is called. |


###### *Example*
~~~php
$ret = $redis->multi()
    ->set('key1', 'val1')
    ->get('key1')
    ->set('key2', 'val2')
    ->get('key2')
    ->exec();

/*
$ret == [0 => TRUE, 1 => 'val1', 2 => TRUE, 3 => 'val2'];
*/
~~~

#### watch, unwatch
-----
_**Description**_: Watches a key for modifications by another client.

If the key is modified between `WATCH` and `EXEC`, the MULTI/EXEC transaction will fail (return `FALSE`). `unwatch` cancels all the watching of all keys by this client.

###### *Parameters*

| Parameter | Type | Description |
| --- | --- | --- |
| `key` | `array\|string` | Either an array with one or more key names, or a string key name |
| `other_keys` | `string` | If the first argument was passed as a string, any number of additional string key names may be passed variadically. |

###### *Example*
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

### Local Helper Methods

These helpers expose the exact logic PhpRedis uses locally for serialization, compression, and prefixing.

* [getDBNum](#getdbnum) - Get the currently selected database number
* [getLastError](#getlasterror) - The last error message (if any)
* [clearLastError](#clearlasterror) - Clear the last error message
* [_compress](#_compress) - Compress a string with the configured compression option
* [_uncompress](#_uncompress) - Uncompress a string with the configured compression option
* [_serialize](#_serialize) - Serialize a value with the configured serializer
* [_unserialize](#_unserialize) - Unserialize a string with the configured serializer
* [_pack](#_pack) - Serialize and compress a value exactly as PhpRedis would before sending it to Redis
* [_unpack](#_unpack) - Reverse `_pack` to retrieve the original PHP value
* [_prefix](#_prefix) - Apply the configured key prefix locally
* [_digest](#_digest) - Compute the XXH3 digest of a packed value just like Redis' DIGEST command

#### getLastError
_**Description**_: Get the last error message, if any.

###### *Parameters*

None.   

###### *Return value*   
| Type | Description |
| --- | --- |
| `string\|null` | The last error message, or `NULL` if there was no error. |   

###### *Examples*
~~~php
$redis->del('hash');
$redis->hmset('hash', ['foo' => 'bar']);
$redis->rpush('hash', 'baz');
echo $redis->getLastError(); // "ERR Operation against a key holding the wrong kind of value"
~~~

#### clearLastError
-----
_**Description**_: Clear the last error message

###### *Parameters*

None.

###### *Return value*

| Type | Description |
| --- | --- |
| `bool` | This should always return true or throw an exception if we're not connected. |


###### *Examples*
~~~php
$redis->set('x', 'a');
$redis->incr('x');
$err = $redis->getLastError();
// "ERR value is not an integer or out of range"
$redis->clearLastError();
$err = $redis->getLastError();
// NULL
~~~

#### _compress
-----
_**Description**_: Compress a string using whatever `Redis::OPT_COMPRESSION` is currently set to. This is useful when you need to mimic PhpRedis' compression logic without sending data to Redis.

###### *Parameters*

| Parameter | Type | Description |
| --- | --- | --- |
| `value` | `string` | The value to be compressed |

###### *Return value*

| Type | Description |
| --- | --- |
| `string` | Returns the compressed string, or the original value if compression is disabled. |


###### *Examples*
~~~php
$redis->setOption(Redis::OPT_COMPRESSION, Redis::COMPRESSION_LZF);
$payload = $redis->_compress('large-payload');
~~~

#### _uncompress
-----
_**Description**_: Reverse `_compress` by uncompressing a string with the compression option currently in use.

###### *Parameters*

| Parameter | Type | Description |
| --- | --- | --- |
| `value` | `string` | The compressed value to uncompress. |

###### *Return value*

| Type | Description |
| --- | --- |
| `string` | Returns the uncompressed value. Throws an exception if the payload is invalid for the configured compressor. |


###### *Examples*
~~~php
$redis->setOption(Redis::OPT_COMPRESSION, Redis::COMPRESSION_LZ4);
$original = $redis->_uncompress($compressedPayload);
~~~

#### _serialize
-----
_**Description**_: Serialize a value manually using whatever serializer is configured.

This can be useful for serialization/unserialization of data going in and out of EVAL commands
as phpredis can't automatically do this itself. Note that if no serializer is set, phpredis
will change Array values to 'Array', and Objects to 'Object'.

###### *Parameters*

| Parameter | Type | Description |
| --- | --- | --- |
| `value` | `mixed` | The value to serialize |

###### *Return value*

| Type | Description |
| --- | --- |
| `string` | The serialized representation of the value. |


###### *Examples*
~~~php
$redis->setOption(Redis::OPT_SERIALIZER, Redis::SERIALIZER_NONE);
$redis->_serialize("foo"); // returns "foo"
$redis->_serialize([]); // Returns "Array"
$redis->_serialize(new stdClass()); // Returns "Object"

$redis->setOption(Redis::OPT_SERIALIZER, Redis::SERIALIZER_PHP);
$redis->_serialize("foo"); // Returns 's:3:"foo";'
~~~

#### _unserialize
-----
_**Description**_: Unserialize a string using whatever serializer is configured.

If there is no serializer set, the value will be returned unchanged. If there is a serializer set up,
and the data passed in is malformed, an exception will be thrown.

###### *Parameters*

| Parameter | Type | Description |
| --- | --- | --- |
| `value` | `string` | The value to unserialize |

###### *Return value*

| Type | Description |
| --- | --- |
| `mixed` | The PHP value represented by the serialized string. |


###### *Examples*
~~~php
$redis->setOption(Redis::OPT_SERIALIZER, Redis::SERIALIZER_PHP);
$redis->_unserialize('a:3:{i:0;i:1;i:1;i:2;i:2;i:3;}'); // Will return [1,2,3]
~~~

#### _pack
-----
_**Description**_: Pack a value the same way PhpRedis does internally by first serializing (if configured) and then compressing (if configured). This mirrors the exact bytes Redis receives.

###### *Parameters*

| Parameter | Type | Description |
| --- | --- | --- |
| `value` | `mixed` | The value to pack |

###### *Return value*

| Type | Description |
| --- | --- |
| `string` | The packed payload that would be sent to Redis. |


###### *Examples*
~~~php
$redis->setOption(Redis::OPT_SERIALIZER, Redis::SERIALIZER_PHP);
$redis->setOption(Redis::OPT_COMPRESSION, Redis::COMPRESSION_LZF);
$payload = $redis->_pack(['a' => 1]);
~~~

#### _unpack
-----
_**Description**_: Reverse `_pack` by uncompressing and then unserializing a payload using the current PhpRedis options.

###### *Parameters*

| Parameter | Type | Description |
| --- | --- | --- |
| `value` | `string` | The value which has been serialized and compressed. |

###### *Return value*

| Type | Description |
| --- | --- |
| `mixed` | The unpacked PHP value. |


###### *Examples*
~~~php
$value = $redis->_unpack($payload);
~~~

#### _prefix
-----
_**Description**_: A utility method to prefix the value with the prefix setting for PhpRedis.

###### *Parameters*

| Parameter | Type | Description |
| --- | --- | --- |
| `key` | `string` | The key/string to prefix |

###### *Return value*

| Type | Description |
| --- | --- |
| `string` | Returns the prefixed value when a prefix is configured, or the original string otherwise. |


###### *Examples*
~~~php
$redis->setOption(Redis::OPT_PREFIX, 'my-prefix:');
$redis->_prefix('my-value'); // Will return 'my-prefix:my-value'
~~~

#### _digest
-----
_**Description**_: Compute the XXH3 digest of a PHP value after it has been `_pack`ed. This produces the same output as Redis' `DIGEST` command, making it easy to verify values locally.

###### *Parameters*

| Parameter | Type | Description |
| --- | --- | --- |
| `value` | `mixed` | The value to compute the digest for. |

###### *Return value*

| Type | Description |
| --- | --- |
| `string` | A 16 character hex string containing the XXH3 digest. |


###### *Examples*
~~~php
$digest = $redis->_digest(['foo' => 'bar']);
~~~

### Introspection

#### isConnected
-----
_**Description**_:  A method to determine if a phpredis object thinks it's connected to a server

###### *Parameters*

None

###### *Return value*

| Type | Description |
| --- | --- |
| `bool` | Returns TRUE if phpredis thinks it's connected and FALSE if not |


#### getHost
-----
_**Description**_:  Retrieve our host or unix socket that we're connected to

###### *Parameters*

None

###### *Return value*

| Type | Description |
| --- | --- |
| `string` | The host or unix socket we're connected to or FALSE if we're not connected |


#### getPort
-----
_**Description**_:  Get the port we're connected to

###### *Parameters*

None

###### *Return value*

| Type | Description |
| --- | --- |
| `int` | Returns the port we're connected to or FALSE if we're not connected |


#### getDbNum
-----
_**Description**_:  Get the database number phpredis is pointed to

###### *Parameters*

None

###### *Return value*

| Type | Description |
| --- | --- |
| `int` | Returns the database number (LONG) phpredis thinks it's pointing to or FALSE if we're not connected |


#### getTimeout
-----
_**Description**_:  Get the (write) timeout in use for phpredis

###### *Parameters*

None

###### *Return value*

| Type | Description |
| --- | --- |
| `float\|false` | The timeout (DOUBLE) specified in our connect call or FALSE if we're not connected |


#### getReadTimeout
_**Description**_:  Get the read timeout specified to phpredis or FALSE if we're not connected

###### *Parameters*

None

###### *Return value*

| Type | Description |
| --- | --- |
| `float` | Returns the read timeout (which can be set using setOption and Redis::OPT_READ_TIMEOUT) or FALSE if we're not connected |


#### getPersistentID
-----
_**Description**_:  Gets the persistent ID that phpredis is using

###### *Parameters*

None

###### *Return value*

| Type | Description |
| --- | --- |
| `string\|null` | Returns the persistent id phpredis is using (which will only be set if connected with pconnect), NULL if we're not using a persistent ID, and FALSE if we're not connected |


#### getAuth
-----
_**Description**_:  Get the password (or username and password if using Redis 6 ACLs) used to authenticate the connection.

#### *Parameters*
None

#### *Return value*
*Mixed*  Returns NULL if no username/password are set, the password string if a password is set, and a `[username, password]` array if authenticated with a username and password.
