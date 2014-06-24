Redis Arrays
============

A Redis array is an isolated namespace in which keys are related in some manner. Keys are distributed across a number of Redis instances, using consistent hashing. A hash function is used to spread the keys across the array in order to keep a uniform distribution. **This feature was added as the result of a generous sponsorship by [A+E Networks](http://www.aetn.com/).**

An array is composed of the following:

* A list of Redis hosts.
* A key extraction function, used to hash part of the key in order to distribute related keys on the same node (optional). This is set by the "function" option.
* A list of nodes previously in the ring, only present after a node has been added or removed. When a read command is sent to the array (e.g. GET, LRANGE...), the key is first queryied in the main ring, and then in the secondary ring if it was not found in the main one. Optionally, the keys can be migrated automatically when this happens. Write commands will always go to the main ring. This is set by the "previous" option.
* An optional index in the form of a Redis set per node, used to migrate keys when nodes are added or removed; set by the "index" option.
* An option to rehash the array automatically as nodes are added or removed, set by the "autorehash" option.

## Creating an array

There are several ways of creating Redis arrays;  they can be pre-defined in redis.ini using `new RedisArray(string $name);`, or created dynamically using `new RedisArray(array $hosts, array $options);`

#### Declaring a new array with a list of nodes
<pre>
$ra = new RedisArray(array("host1", "host2:63792", "host2:6380"));
</pre>


#### Declaring a new array with a list of nodes and a function to extract a part of the key
<pre>
function extract_key_part($k) {
    return substr($k, 0, 3);	// hash only on first 3 characters.
}
$ra = new RedisArray(array("host1", "host2:63792", "host2:6380"), array("function" => "extract_key_part"));
</pre>

#### Defining a "previous" array when nodes are added or removed.
When a new node is added to an array, phpredis needs to know about it. The old list of nodes becomes the “previous” array, and the new list of nodes is used as a main ring. Right after a node has been added, some read commands will point to the wrong nodes and will need to look up the keys in the previous ring.

<pre>
// adding host3 to a ring containing host1 and host2. Read commands will look in the previous ring if the data is not found in the main ring.
$ra = new RedisArray(array("host1", "host2", "host3"), array("previous" => array("host1", "host2")));
</pre>

#### Specifying the "retry_interval" parameter
The retry_interval is used to specify a delay in milliseconds between reconnection attempts in case the client loses connection with a server
<pre>
$ra = new RedisArray(array("host1", "host2:63792", "host2:6380"), array("retry_timeout" => 100)));
</pre>

#### Specifying the "lazy_connect" parameter
This option is useful when a cluster has many shards but not of them are necessarily used at one time.
<pre>
$ra = new RedisArray(array("host1", "host2:63792", "host2:6380"), array("lazy_connect" => true)));
</pre>

#### Defining arrays in Redis.ini

Because php.ini parameters must be pre-defined, Redis Arrays must all share the same .ini settings.

<pre>
// list available Redis Arrays
ini_set('redis.array.names', 'users,friends');

// set host names for each array.
ini_set('redis.arrays.hosts', 'users[]=localhost:6379&users[]=localhost:6380&users[]=localhost:6381&users[]=localhost:6382&friends[]=localhost');

// set functions
ini_set('redis.arrays.functions', 'users=user_hash');

// use index only for users
ini_set('redis.arrays.index', 'users=1,friends=0');
</pre>

## Usage

Redis arrays can be used just as Redis objects:
<pre>
$ra = new RedisArray("users");
$ra->set("user1:name", "Joe");
$ra->set("user2:name", "Mike");
</pre>


## Key hashing
By default and in order to be compatible with other libraries, phpredis will try to find a substring enclosed in curly braces within the key name, and use it to distribute the data.

For instance, the keys “{user:1}:name” and “{user:1}:email” will be stored on the same server as only “user:1” will be hashed. You can provide a custom function name in your redis array with the "function" option; this function will be called every time a key needs to be hashed. It should take a string and return a string.


## Custom key distribution function
In order to control the distribution of keys by hand, you can provide a custom function or closure that returns the server number, which is the index in the array of servers that you created the RedisArray object with.

For instance, instanciate a RedisArray object with `new RedisArray(array("us-host", "uk-host", "de-host"), array("distributor" => "dist"));` and write a function called "dist" that will return `2` for all the keys that should end up on the "de-host" server.

### Example
<pre>
$ra = new RedisArray(array("host1", "host2", "host3", "host4", "host5", "host6", "host7", "host8"), array("distributor" => array(2, 2)));
</pre>

This declares that we started with 2 shards and moved to 4 then 8 shards. The number of initial shards is 2 and the resharding level (or number of iterations) is 2.

## Migrating keys

When a node is added or removed from a ring, RedisArray instances must be instanciated with a “previous” list of nodes. A single call to `$ra->_rehash()` causes all the keys to be redistributed according to the new list of nodes. Passing a callback function to `_rehash()` makes it possible to track the progress of that operation: the function is called with a node name and a number of keys that will be examined, e.g. `_rehash(function ($host, $count){ ... });`.

It is possible to automate this process, by setting `'autorehash' => TRUE` in the constructor options. This will cause keys to be migrated when they need to be read from the previous array.

In order to migrate keys, they must all be examined and rehashed. If the "index" option was set, a single key per node lists all keys present there. Otherwise, the `KEYS` command is used to list them.
If a “previous” list of servers is provided, it will be used as a backup ring when keys can not be found in the current ring. Writes will always go to the new ring, whilst reads will go to the new ring first, and to the second ring as a backup.

Adding and/or removing several instances is supported.

### Example
<pre>
$ra = new RedisArray("users"); // load up a new config from redis.ini, using the “.previous” listing.
$ra->_rehash();
</pre>

Running this code will:

* Create a new ring with the updated list of nodes.
* Server by server, look up all the keys in the previous list of nodes.
* Rehash each key and possibly move it to another server.
* Update the array object with the new list of nodes.

## Multi/exec
Multi/exec is still available, but must be run on a single node:
<pre>
$host = $ra->_target("{users}:user1:name");	// find host first
$ra->multi($host)	// then run transaction on that host.
   ->del("{users}:user1:name")
   ->srem("{users}:index", "user1")
   ->exec();
</pre>

## Limitations
Key arrays offer no guarantee when using Redis commands that span multiple keys. Except for the use of MGET, MSET, and DEL, a single connection will be used and all the keys read or written there.  Running KEYS() on a RedisArray object will execute the command on each node and return an associative array of keys, indexed by host name.

## Array info
RedisArray objects provide several methods to help understand the state of the cluster. These methods start with an underscore.

* `$ra->_hosts()` → returns a list of hosts for the selected array.
* `$ra->_function()` → returns the name of the function used to extract key parts during consistent hashing.
* `$ra->_target($key)` → returns the host to be used for a certain key.
* `$ra->_instance($host)` → returns a redis instance connected to a specific node; use with `_target` to get a single Redis object.

## Running the unit tests
<pre>
$ cd tests
$ ./mkring.sh start
$ php array-tests.php
</pre>

