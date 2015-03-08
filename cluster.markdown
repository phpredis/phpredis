Redis Cluster
=============

Redis introduces cluster support as of version 3.0.0, and to communicate with a cluster using phpredis one needs to use the RedisCluster class.  For the majority of operations the RedisCluster class can act as a drop-in replacement for the Redis class without needing to modify how it's called. **This feature was added as the result of a generous sponsorship by [Tradsey](https://www.tradesy.com/)**

## Creating and connecting to a cluster

To maintain consistency with the RedisArray class, one can create and connect to a cluster either by passing it one or more 'seed' nodes, or by defining these in redis.ini as a 'named' cluster.

#### Declaring a cluster with an array of seeds
<pre>
$obj_cluster = new RedisCluster(NULL, Array('host:7000', 'host:7001', 'host:7003'));
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

## Keyspace map
As previously described, RedisCluster makes an initial mapping of every master (and any slaves) on construction, which it uses to determine which nodes to direct a given command.  However, one of the core functionalities of Redis cluster is that this keyspace can change while the cluster is running.

Because of this, the RedisCluster class will update it's keyspace mapping whenever it receives a MOVED error when requesting data.  In the case that we receive ASK redirection, it follows the Redis specification and requests the key from the ASK node, prefixed with an ASKING command.

## Automatic slave failover / distribution
By default, RedisCluster will only ever send commands to master nodes, but can be configured differently for readonly commands if requested.

<pre>
// The default option, only send commands to master nodes
$obj_cluster->setOption(RedisCluster::OPT_FAILOVER, RedisCluster::FAILOVER_NONE);

// In the event we can't reach a master, and it has slaves, failover for read commands
$obj_cluster->setOption(RedisCluster::OPT_FAILOVER, RedisCluster::FAILOVER_ERROR);

// Always distribute readonly commands between masters and slaves, at random
$obj_cluster->setOption(
    RedisCluster::OPT_FAILOVER, RedsiCluster::FAILOVER_DISTRIBUTE
);
</pre>

## Main command loop
With the exception of commands that are directed to a specific node, each command executed via RedisCluster is processed through a command loop, where we make the request, handle any MOVED or ASK redirection, and repeat if neccsary.  This continues until one of the following conditions is met:

1.  We fail to communicate with *any* node that we are aware of, in which case a ```RedisClusterExecption``` is raised.
2.  We have been bounced around longer than the timeout which was set on construction.
3.  Redis cluster returns us a ```CLUSTERDOWN``` error, in which case a ```RedisClusterException``` is raised.
4.  We receive a valid response, in which case the data is returned to the caller.

## Transactions
The RedisCluster class fully supports MULTI ... EXEC transactions, including commands such as MGET and MSET which operate on multiple keys.  There are considerations that must be taken into account here however.

When you call ```RedisCluster->multi()```, the cluster is put into a MULTI state, but the MULTI command is not delivered to any nodes until a key is requested on that node.  In addition, calls to EXEC will always return an array (even in the event that a transaction to a given node failed), as the commands can be going to any number of nodes depending on what is called.

Consider the following example:

<pre>
$obj_cluster->multi(); // Cluster is put into MULTI state locally
for ($i = 0; $i < 10; $i++) {
    // MULTI will be delivered only once per node
    $obj_cluster->get("key:$i");
}
// This always returns an array of 10 elements, of which some could be false 
// (for example if a slot had been migrated)
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

