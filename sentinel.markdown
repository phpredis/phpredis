Redis Sentinel
==============

Redis Sentinel provides high availability for Redis. In practical terms this means that using Sentinel you can create a Redis deployment that resists without human intervention certain kinds of failures.

Redis Sentinel also provides other collateral tasks such as monitoring, notifications and acts as a configuration provider for clients.

## Class RedisSentinel
-----

##### *Parameters*

*host*: String, IP address or hostname  
*port*: Int (optional, default is 26379)  
*timeout*: Float, value in seconds (optional, default is 0 meaning unlimited)  
*persistent*: String, persistent connection id (optional, default is NULL meaning not persistent)  
*retry_interval*: Int, value in milliseconds (optional, default is 0)  
*read_timeout*: Float, value in seconds (optional, default is 0 meaning unlimited)  

##### *Example*

~~~php
$sentinel = new RedisSentinel('127.0.0.1'); // default parameters
$sentinel = new RedisSentinel('127.0.0.1', 26379, 2.5); // 2.5 sec timeout.
$sentinel = new RedisSentinel('127.0.0.1', 26379, 0, 'sentinel'); // persistent connection with id 'sentinel'
$sentinel = new RedisSentinel('127.0.0.1', 26379, 0, ''); // also persistent connection with id ''
$sentinel = new RedisSentinel('127.0.0.1', 26379, 1, null, 100); // 1 sec timeout, 100ms delay between reconnection attempts.
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
