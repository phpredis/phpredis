Redis Sentinel
==============

Redis Sentinel provides high availability for Redis. In practical terms this means that using Sentinel you can create a Redis deployment that resists without human intervention certain kinds of failures.

Redis Sentinel also provides other collateral tasks such as monitoring, notifications and acts as a configuration provider for clients.

## Class RedisSentinel
-----

##### *Parameters*

*host*: String, IP address or hostname  
*port*: Int (optional, default is 26379)  
*hosts*: Array of `['host' => string, 'port' => int]` entries (optional). When provided, `host` and `port` are ignored and the client automatically falls back to the next host on network failure. See [Multi-host support](#multi-host-support) below.  
*timeout*: Float, value in seconds (optional, default is 0 meaning unlimited)  
*persistent*: String, persistent connection id (optional, default is NULL meaning not persistent)  
*retry_interval*: Int, value in milliseconds (optional, default is 0)  
*read_timeout*: Float, value in seconds (optional, default is 0 meaning unlimited)  
*auth*:String, or an Array with one or two elements, used to authenticate with the redis-sentinel. (optional, default is NULL meaning NOAUTH)

##### *Examples for version 6.0 or later*

~~~php
$sentinel = new RedisSentinel([
  'host' => '127.0.0.1',
]); // default parameters
$sentinel = new RedisSentinel([
  'host' => '127.0.0.1',
  'port' => 26379,
  'connectTimeout' => 2.5,
]); // 2.5 sec timeout.
$sentinel = new RedisSentinel([
  'host' => '127.0.0.1',
  'port' => 26379,
  'connectTimeout' => 2.5,
  'persistent' => 'sentinel',
]); // persistent connection with id 'sentinel'
$sentinel = new RedisSentinel([
  'host' => '127.0.0.1',
  'port' => 26379,
  'connectTimeout' => 2.5,
  'persistent' => '',
]); // also persistent connection with id ''
$sentinel = new RedisSentinel([
  'host' => '127.0.0.1',
  'port' => 26379,
  'connectTimeout' => 1,
  'persistent' => null,
  'retryInterval' => 100,
]); // 1 sec timeout, 100ms delay between reconnection attempts.
$sentinel = new RedisSentinel([
  'host' => '127.0.0.1',
  'port' => 26379,
  'connectTimeout' => 0,
  'persistent' => null,
  'retryInterval' => 0,
  'readTimeout' => 0,
  'auth' => 'secret',
]); // connect sentinel with password authentication
~~~

### Multi-host support
-----

For high-availability deployments (Kubernetes, multi-AZ), `RedisSentinel` accepts a `hosts` array of Sentinel endpoints. On network failure the client transparently falls back to the next entry in the list, so a single dead Sentinel no longer takes down the client.

~~~php
$sentinel = new RedisSentinel([
  'hosts' => [
    ['host' => '10.0.0.1', 'port' => 26379],
    ['host' => '10.0.0.2', 'port' => 26379],
    ['host' => '10.0.0.3', 'port' => 26379],
  ],
  'connectTimeout' => 0.1,
  'auth' => 'secret',
]);

// Auto-falls-back to a reachable host if 10.0.0.1 is down.
$master = $sentinel->getMasterAddrByName('mymaster');
~~~

##### *Semantics*

* When `hosts` is provided, `host` and `port` are ignored.
* The client tries hosts in order. The first reachable host is used for all subsequent calls ("sticky" connection).
* If the current host becomes unreachable during a method call, the client transparently advances to the next host in the list and retries the call once.
* Skipped hosts are NOT revisited for the lifetime of the `RedisSentinel` instance.
* When all hosts are exhausted, a `RedisException` is thrown with a message mentioning the host count.
* Retry is triggered only on network errors (connection refused, socket EOF, stream broken). Redis protocol errors (NOAUTH, WRONGPASS, unknown command) are propagated without retry.
* Validation errors at construct time (empty `hosts`, missing `host` key, wrong types, `hosts` too large) throw `RedisException`, consistent with the rest of phpredis.

##### *Examples for versions older than 6.0*

~~~php
$sentinel = new RedisSentinel('127.0.0.1'); // default parameters
$sentinel = new RedisSentinel('127.0.0.1', 26379, 2.5); // 2.5 sec timeout.
$sentinel = new RedisSentinel('127.0.0.1', 26379, 0, 'sentinel'); // persistent connection with id 'sentinel'
$sentinel = new RedisSentinel('127.0.0.1', 26379, 0, ''); // also persistent connection with id ''
$sentinel = new RedisSentinel('127.0.0.1', 26379, 1, null, 100); // 1 sec timeout, 100ms delay between reconnection attempts.
$sentinel = new RedisSentinel('127.0.0.1', 26379, 0, NULL, 0, 0, "secret"); // connect sentinel with password authentication
~~~

### Usage
-----

* [ckquorum](#ckquorum) - Check if the current Sentinel configuration is able to reach the quorum needed to failover.
* [failover](#failover) - Force a failover as if the master was not reachable.
* [flushconfig](#flushconfig) - Force Sentinel to rewrite its configuration on disk.
* [getMasterAddrByName](#getMasterAddrByName) - Return the ip and port number of the master with that name.
* [master](#master) - Return the state and info of the specified master.
* [masters](#masters) - Return a list of monitored masters and their state.
* [ping](#ping) - Ping the sentinel.
* [reset](#reset) - Reset all the masters with matching name.
* [sentinels](#sentinels) - Return a list of sentinel instances for this master, and their state.
* [slaves](#slaves) - Return a list of replicas for this master, and their state.

-----

### ckquorum
-----
_**Description**_: Check if the current Sentinel configuration is able to reach the quorum needed to failover a master, and the majority needed to authorize the failover. This command should be used in monitoring systems to check if a Sentinel deployment is ok.

##### *Parameters*
*String*: master name

##### *Return value*
*Bool*: `TRUE` in case of success, `FALSE` in case of failure.

##### *Example*
~~~php
$sentinel->ckquorum('mymaster');
~~~

### failover
-----
_**Description**_: Force a failover as if the master was not reachable, and without asking for agreement to other Sentinels (however a new version of the configuration will be published so that the other Sentinels will update their configurations).

##### *Parameters*
*String*: master name

##### *Return value*
*Bool*: `TRUE` in case of success, `FALSE` in case of failure.

##### *Example*
~~~php
$sentinel->failover('mymaster');
~~~

### flushconfig
-----
_**Description**_: Force Sentinel to rewrite its configuration on disk, including the current Sentinel state. Normally Sentinel rewrites the configuration every time something changes in its state (in the context of the subset of the state which is persisted on disk across restart). However sometimes it is possible that the configuration file is lost because of operation errors, disk failures, package upgrade scripts or configuration managers. In those cases a way to to force Sentinel to rewrite the configuration file is handy. This command works even if the previous configuration file is completely missing.

##### *Parameters*
(none)

##### *Return value*
*Bool*: `TRUE` in case of success, `FALSE` in case of failure.

##### *Example*
~~~php
$sentinel->flushconfig();
~~~

### getMasterAddrByName
-----
_**Description**_: Return the ip and port number of the master with that name. If a failover is in progress or terminated successfully for this master it returns the address and port of the promoted replica.

##### *Parameters*
*String*: master name

##### *Return value*
*Array*, *Bool*: ['address', 'port'] in case of success, `FALSE` in case of failure.

##### *Example*
~~~php
$sentinel->getMasterAddrByName('mymaster');
~~~

### master
-----
_**Description**_: Return the state and info of the specified master.

##### *Parameters*
*String*: master name

##### *Return value*
*Array*, *Bool*: Associative array with info in case of success, `FALSE` in case of failure.

##### *Example*
~~~php
$sentinel->master('mymaster');
~~~

### masters
-----
_**Description**_: Return a list of monitored masters and their state.

##### *Parameters*
(none)

##### *Return value*
*Array*, *Bool*: List of arrays with info for each master in case of success, `FALSE` in case of failure.

##### *Example*
~~~php
$sentinel->masters();
~~~

### ping
-----
_**Description**_: Ping the sentinel.

##### *Parameters*
(none)

##### *Return value*
*Bool*: `TRUE` in case of success, `FALSE` in case of failure.

##### *Example*
~~~php
$sentinel->ping();
~~~

### reset
-----
_**Description**_: This command will reset all the masters with matching name. The pattern argument is a glob-style pattern. The reset process clears any previous state in a master (including a failover in progress), and removes every replica and sentinel already discovered and associated with the master.

##### *Parameters*
*String*: pattern

##### *Return value*
*Bool*: `TRUE` in case of success, `FALSE` in case of failure.

##### *Example*
~~~php
$sentinel->reset('*');
~~~

### sentinels
-----
_**Description**_: Return a list of sentinel instances for this master, and their state.

##### *Parameters*
*String*: master name

##### *Return value*
*Array*, *Bool*: List of arrays with info for each sentinels in case of success, `FALSE` in case of failure.

##### *Example*
~~~php
$sentinel->sentinels('mymaster');
~~~

### slaves
-----
_**Description**_: Return a list of replicas for this master, and their state.

##### *Parameters*
*String*: master name

##### *Return value*
*Array*, *Bool*: List of arrays with info for each replicas in case of success, `FALSE` in case of failure.

##### *Example*
~~~php
$sentinel->slaves('mymaster');
~~~
