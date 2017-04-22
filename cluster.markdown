Redis Cluster
=============

Redis introduces cluster support as of version 3.0.0, and to communicate with a cluster using phpredis one needs to use the RedisCluster class.  For the majority of operations the RedisCluster class can act as a drop-in replacement for the Redis class without needing to modify how it's called. **This feature was added as the result of a generous sponsorship by [Tradesy](https://www.tradesy.com/)**

## Creating and connecting to a cluster

To maintain consistency with the RedisArray class, one can create and connect to a cluster either by passing it one or more 'seed' nodes, or by defining these in redis.ini as a 'named' cluster.

#### Declaring a cluster with an array of seeds
<pre>
// Create a cluster setting two nodes as seeds
$obj_cluster = new RedisCluster(NULL, Array('host:7000', 'host:7001', 'host:7003'));

// Connect and specify timeout and read_timeout
$obj_cluster = new RedisCluster(NULL, Array("host:7000", "host:7001"), 1.5, 1.5);

// Connect with read/write timeout as well as specify that phpredis should use
// persistent connections to each node.
$obj_cluster = new RedisCluster(NULL, Array("host:7000", "host:7001"), 1.5, 1.5, true);

</pre>

#### Loading a cluster configuration by name
In order to load a named array, one must first define the seed nodes in redis.ini.  The following lines would define the cluster 'mycluster', and be loaded automatically by phpredis.

<pre>
# In redis.ini
redis.clusters.seeds = "mycluster[]=localhost:7000&test[]=localhost:7001"
redis.clusters.timeout = "mycluster=5"
redis.clusters.read_timeout = "mycluster=10"
</pre>

Then, this cluster can be loaded by doing the following

<pre>
$obj_cluster = new RedisCluster('mycluster');
</pre>

## Connection process

On construction, the RedisCluster class will iterate over the provided seed nodes until it can attain a connection to the cluster and run CLUSTER SLOTS to map every node in the cluster locally.  Once the keyspace is mapped, RedisCluster will only connect to nodes when it needs to (e.g. you're getting a key that we believe is on that node.)

## Timeouts
Because Redis cluster is intended to provide high availability, timeouts do not work in the same way they do in normal socket communication.  It's fully possible to have a timeout or even exception on a given socket (say in the case that a master node has failed), and continue to serve the request if and when a slave can be promoted as the new master.

The way RedisCluster handles user specified timeout values is that every time a command is sent to the cluster, we record the the time at the start of the request and then again every time we have to re-issue the command to a different node (either because Redis cluster responded with MOVED/ASK or because we failed to communicate with a given node).  Once we detect having been in the command loop for longer than our specified timeout, an error is raised.

## Keyspace map
As previously described, RedisCluster makes an initial mapping of every master (and any slaves) on construction, which it uses to determine which nodes to direct a given command.  However, one of the core functionalities of Redis cluster is that this keyspace can change while the cluster is running.

Because of this, the RedisCluster class will update it's keyspace mapping whenever it receives a MOVED error when requesting data.  In the case that we receive ASK redirection, it follows the Redis specification and requests the key from the ASK node, prefixed with an ASKING command.

## Automatic slave failover / distribution
By default, RedisCluster will only ever send commands to master nodes, but can be configured differently for readonly commands if requested.

<pre>
// The default option, only send commands to master nodes
$obj_cluster->setOption(RedisCluster::OPT_SLAVE_FAILOVER, RedisCluster::FAILOVER_NONE);

// In the event we can't reach a master, and it has slaves, failover for read commands
$obj_cluster->setOption(RedisCluster::OPT_SLAVE_FAILOVER, RedisCluster::FAILOVER_ERROR);

// Always distribute readonly commands between masters and slaves, at random
$obj_cluster->setOption(
    RedisCluster::OPT_SLAVE_FAILOVER, RedisCluster::FAILOVER_DISTRIBUTE
);
</pre>

## Main command loop
With the exception of commands that are directed to a specific node, each command executed via RedisCluster is processed through a command loop, where we make the request, handle any MOVED or ASK redirection, and repeat if necessary.  This continues until one of the following conditions is met:

1.  We fail to communicate with *any* node that we are aware of, in which case a ```RedisClusterExecption``` is raised.
2.  We have been bounced around longer than the timeout which was set on construction.
3.  Redis cluster returns us a ```CLUSTERDOWN``` error, in which case a ```RedisClusterException``` is raised.
4.  We receive a valid response, in which case the data is returned to the caller.

## Transactions
The RedisCluster class fully supports MULTI ... EXEC transactions, including commands such as MGET and MSET which operate on multiple keys.  There are considerations that must be taken into account here however.

When you call ```RedisCluster->multi()```, the cluster is put into a MULTI state, but the MULTI command is not delivered to any nodes until a key is requested on that node.  In addition, calls to EXEC will always return an array (even in the event that a transaction to a given node failed), as the commands can be going to any number of nodes depending on what is called.

Consider the following example:

<pre>
// Cluster is put into MULTI state locally
$obj_cluster->multi();

// The cluster will issue MULTI on this node first (and only once)
$obj_cluster->get("mykey");
$obj_cluster->set("mykey", "new_value");

// If 'myotherkey' maps to a different node, MULTI will be issued there
// before requesting the key
$obj_cluster->get("myotherkey");

// This will always return an array, even in the event of a failed transaction
// on one of the nodes, in which case that element will be FALSE
print_r($obj_cluster->exec());
</pre>

## Pipelining
The RedisCluster class does not support pipelining as there is no way to detect whether the keys still live where our map indicates that they do and would therefore be inherently unsafe.  It would be possible to implement this support as an option if there is demand for such a feature.

## Multiple key commands
Redis cluster does allow commands that operate on multiple keys, but only if all of those keys hash to the same slot.  Note that it is not enough that the keys are all on the same node, but must actually hash to the exact same hash slot.

For all of these multiple key commands (with the exception of MGET and MSET), the RedisCluster class will verify each key maps to the same hash slot and raise a "CROSSSLOT" warning, returning false if they don't.

### MGET and MSET
RedisCluster has specialized processing for MGET and MSET which allows you to send any number of keys (hashing to whichever slots) without having to consider where they live.  The way this works, is that the RedisCluster class will split the command as it iterates through keys, delivering a subset of commands per each key's slot.

<pre>
// This will be delivered in two commands.  First for all of the {hash1} keys, 
// and then to grab 'otherkey'
$obj_cluster->mget(Array("{hash1}key1","{hash1}key2","{hash1}key3","otherkey"));
</pre>

This operation can also be done in MULTI mode transparently.

## Directed node commands
There are a variety of commands which have to be directed at a specific node.  In the case of these commands, the caller can either pass a key (which will be hashed and used to direct our command), or an array with host:port.

<pre>
// This will be directed at the slot/node which would store "mykey"
$obj_cluster->echo("mykey","Hello World!");

// Here we're iterating all of our known masters, and delivering the command there
foreach ($obj_cluster->_masters() as $arr_master) {
    $obj_cluster->echo($arr_master, "Hello: " . implode(':', $arr_master));
}
</pre>

In the case of all commands which need to be directed at a node, the calling convention is identical to the Redis call, except that they require an additional (first) argument in order to deliver the command.  Following is a list of each of these commands:

1.  SAVE
2.  BGSAVE
3.  FLUSHDB
4.  FLUSHALL
5.  DBSIZE
6.  BGREWRITEAOF
7.  LASTSAVE
8.  INFO
9.  CLIENT
10.  CLUSTER
11.  CONFIG
12.  PUBSUB
13.  SLOWLOG
14.  RANDOMKEY
15.  PING

## Session Handler
You can use the cluster functionality of phpredis to store PHP session information in a Redis cluster as you can with a non cluster-enabled Redis instance.

To do this, you must configure your `session.save_handler` and `session.save_path` INI variables to give phpredis enough information to communicate with the cluster.

~~~
session.save_handler = rediscluster
session.save_path = "seed[]=host1:port1&seed[]=host2:port2&seed[]=hostN:portN&timeout=2&read_timeout=2&failover=error&persistent=1"
~~~

### session.session_handler
Set this variable to "rediscluster" to inform phpredis that this is a cluster instance.

### session.save_path
The save path for cluster based session storage takes the form of a PHP GET request, and requires that you specify at least on `seed` node.  Other options you can specify are as follows:

* _timeout (double)_:  The amount of time phpredis will wait when connecting or writing to the cluster.
* _read_timeout (double)_: The amount of time phpredis will wait for a result from the cluster.
* _persistent_: Tells phpredis whether persistent connections should be used.
* _distribute_: phpredis will randomly distribute session reads between masters and any attached slaves (load balancing).
* _failover (string)_:  How phpredis should distribute session reads between master and slave nodes.
* * _none_ : phpredis will only communicate with master nodes
* * _error_: phpredis will communicate with master nodes unless one failes, in which case an attempt will be made to read session information from a slave. 
