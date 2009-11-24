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
##### Description

Set the string value in argument as value of the key.

##### Parameters
*Key* : key
*Value* : Value

##### Return value
*Bool* : TRUE if the SET is successful.

##### Examples

<pre>
$redis->set('key', 'value')
</pre>

## setnx
##### Description
Set the string value in argument as value of the key if the target key already exists.

##### Parameters
*key*
*value*

##### Return value
*BOOL* True in case of success, False in case of failure.

##### Examples
<pre>
$this->redis->setnx('key', 'value'); /* return TRUE */
$this->redis->setnx('key', 'value'); /* return FALSE */
</pre>

## delete
##### Description
##### Parameters
##### Return value
##### Examples


## exists
##### Description
Verify if the specified key exists.
##### Parameters
*key* : key 
##### Return value
*BOOL* : If the key exists, return TRUE, else return FALSE.
##### Examples
<pre>
$this->set('key', 'value');
$this->exists('key'); /* TRUE*/
$this->exists('NonExistingKey'); /* FALSE*/
</pre>

## incr
##### Description
Increment the number stored at key by one. If the second argument is filled, it will be used as the integer value of the increment.
##### Parameters
*key* : key
*value* : value that will be incremented to key
##### Return value
*INT* the new value of incremented value
##### Examples
<pre>
$redis->incr('key1'); /*key1 didn't exists, setted to 0 before the increment */
					  /*and now has the value 1 							 */

$redis->incr('key1'); /* 2 */
$redis->incr('key1'); /* 3 */
$redis->incr('key1'); /* 4 */
</pre>

## decr
##### Description
Decremetn the number stored at key by one. If the second argument is filled, it will be used as the integer value of the decrement.
##### Parameters
*key* : key
*value* : value that will be decremented to key
##### Return value
*INT* the new value of decremented value
##### Examples
<pre>
$redis->decr('key1'); /*key1 didn't exists, setted to 0 before the increment */
					  /*and now has the value -1 							 */

$redis->incr('key1'); /* -2 */
$redis->incr('key1'); /* -3 */
</pre>

## getMultple
##### Description
Get the values of all the specified keys. If one or more keys dont exists, the array will be filled at the position of the key by a FALSE.
##### Parameters
*Array* : Array containing the list of the keys
##### Return value
*Array* : Array containing the values related to keys in argument
##### Examples
<pre>
$redis->set('key1', 'value1');
$redis->set('key2', 'value2');
$redis->set('key3', 'value3');
$redis->getMultiple(array('key1', 'key2', 'key3')); /* array('value1', 'value2', 'value3');
$redis->getMultiple(array('key0', 'key1', 'key5')); /* array(FALSE, 'value2', FALSE);
</pre>

## lpush
##### Description
Add the string value to the head(right) of the list. Create the list if the key didn't exist. Il the key exists and is not a list, FALSE is returned.
##### Parameters
*key* string
*value* string
##### Return value
*BOOL*
##### Examples
<pre>
$redis->rpush('key1', 'A');
$redis->rpush('key1', 'B');
$redis->rpush('key1', 'C'); /* key1 => [ 'A', 'B', 'C' ] */
</pre>

## rpush
##### Description
Add the string value to the tail(left) of the list. Create the list if the key didn't exist. Il the key exists and is not a list, FALSE is returned.
##### Parameters
*key* string
*value* string
##### Return value
*BOOL* Tue in case of success, False in case of Failure.
##### Examples
<pre>
$redis->rpush('key1', 'A');
$redis->rpush('key1', 'B');
$redis->rpush('key1', 'C'); /* key1 => [ 'C', 'B', 'A' ] */
</pre>

## rpop
##### Description
Return and remove the last element of the list.
##### Parameters
*key*
##### Return value
*STRING* 
*BOOL*
##### Examples
<pre>
$redis->rpush('key1', 'A');
$redis->rpush('key1', 'B');
$redis->rpush('key1', 'C'); /* key1 => [ 'A', 'B', 'C' ] */
$redis->rpop('key1'); /* key1 => [ 'A', 'B' ] */
</pre>

## lpop
##### *Description*
Return and remove the first element of the list.
##### *Parameters*
*key*
##### *Return value*
*STRING* in case of success (key exists)
*BOOL* False in case of failure (key didn't exist)
##### *Example*
<pre>
$redis->rpush('key1', 'A');
$redis->rpush('key1', 'B');
$redis->rpush('key1', 'C'); /* key1 => [ 'C', 'B', 'A' ] */
$redis->lpop('key1'); /* key1 => [ 'B', 'A' ] */
</pre>

## rpop
##### *Description*
Return and remove the last element of the list.
##### *Parameters*
*key*
##### *Return value*
*STRING* in case of success (key exists)
*BOOL* False in case of failure (key didn't exist)
##### *Example*
<pre>
$redis->rpush('key1', 'A');
$redis->rpush('key1', 'B');
$redis->rpush('key1', 'C'); /* key1 => [ 'C', 'B', 'A' ] */
$redis->rpop('key1'); /* key1 => [ 'C', 'B' ] */
</pre>

## lSize
##### *Description*
Return the size of a list identified by Key. If the list didn't exist or is empty, the command returns 0. If the data type identified by Key is not a list, the command return False.
##### *Parameters*
*Key*
##### *Return value*
*LONG* The size of the list identified by Key exists. 
*BOOL* False if the data type identified by Key is not String

##### *Example*
<pre>
$redis->rpush('key1', 'A');
$redis->rpush('key1', 'B');
$redis->rpush('key1', 'C'); /* key1 => [ 'C', 'B', 'A' ] */
$redis->lSize('key1');/* 3 */
$redis->rpop('key1'); 
$redis->lSize('key1');/* 2 */
</pre>

## lGet
##### *Description*
Return the specified element of the list stored at the specified key.
0 the first element, 1 the second ...
-1 the last element, -2 the penultimate ...
Return False is the identify a data type 
##### *Parameters*
*key*
*index*

##### *Return value*
*String* the element at this index
*Bool* False if the key identify a non-string data type, or no values corresponds at this index in the list Key.
##### *Example*
<pre>
$redis->rpush('key1', 'A');
$redis->rpush('key1', 'B');
$redis->rpush('key1', 'C'); /* key1 => [ 'C', 'B', 'A' ] */
$redis->lGet('key1', 0); /* 'C' */
$redis->lGet('key1', -1); /* 'A' */
$redis->lGet('key1', 10); /* False */
</pre>

## lSet
##### *Description*
Set the list at index with the new value.
##### *Parameters*
*key*
*index*
*value*
##### *Return value*
*BOOL* True if the new value is setted. False if the index is out of range, or data type identified by key is not a list.
##### *Example*
<pre>
$redis->rpush('key1', 'A');
$redis->rpush('key1', 'B');
$redis->rpush('key1', 'C'); /* key1 => [ 'C', 'B', 'A' ] */
$redis->lGet('key1', 0); /* 'C' */
$redis->lSet('key1', 0, 'X');
$redis->lGet('key1', 0); /* 'X' */ 
</pre>

## lGetRange
##### *Description*
Return the specified elements of the list stored at the specified key in the range [start, end]. start and stop are interpretated like index :
0 the first element, 1 the second ...
-1 the last element, -2 the penultimate ...
##### *Parameters*
*key*
*start*
*end*

##### *Return value*
*Array* containing the values in specified range. 
##### *Example*
<pre>
$redis->rpush('key1', 'A');
$redis->rpush('key1', 'B');
$redis->rpush('key1', 'C'); 
$redis->lGetRange('key1', 0, -1); /* array('C', 'B', 'A') */
</pre>

## listTrim
##### *Description*
Trim an existing list so that it will contain only the specified range of elements specified.
##### *Parameters*
*key*
*start*
*stop*
##### *Return value*
*Array*
*Bool* return False if the key identify a non-list value.
##### *Example*
<pre>
$redis->rpush('key1', 'A');
$redis->rpush('key1', 'B');
$redis->rpush('key1', 'C'); 
$redis->lGetRange('key1', 0, -1); /* array('C', 'B', 'A') */
$redis->listTrim('key1', 0, 1);
$redis->lGetRange('key1', 0, -1); /* array('C', 'B') */
</pre>

## lRemove
##### *Description*
Remove the first count occurences of the value element from the list. If count es zero, all the element are removed. if count is negatif, elements are removed from tail to head
##### *Parameters*
*key*
*count*
*value*

##### *Return value*
*LONG* the number of removed elements
*BOOL* False if data type of the value identified by key is not a list.
##### *Example*
<pre>
$redis->lpush('key1', 'A');
$redis->lpush('key1', 'B');
$redis->lpush('key1', 'C'); 
$redis->lpush('key1', 'A'); 
$redis->lpush('key1', 'A'); 

$redis->lGetRange('key1', 0, -1); /* array('A', 'B', 'C') */
$redis->lRemove('key1', 'A', 2); /* 2 */
$redis->lGetRange('key1', 0, -1); /* array('B', 'C', 'B') */
</pre>

## sadd
##### *Description*
Add the specified member to the set value stored at key. if member already exists, False is returned. 
##### *Parameters*
*key*
*member*

##### *Return value*
*BOOL* True if value didn't exist and added successfully. False if value exists.
##### *Example*
<pre>
$redis->sadd('key1' , 'set1'); /* True, 'key1' => {'set1'} */
$redis->sadd('key1' , 'set2'); /* True, 'key1' => {'set1', 'set2'}*/
$redis->sadd('key1' , 'set2'); /* False, 'key1' => {'set1', 'set2'}*/
</pre>

## sRemove
##### *Description*
Remove the specified member from the set value stored at key.
##### *Parameters*
*key*
*member*
##### *Return value*
*BOOL* True id member exists in the set values. False if member didn't exist.
##### *Example*
<pre>
$redis->sadd('key1' , 'set1'); 
$redis->sadd('key1' , 'set2'); 
$redis->sadd('key1' , 'set3'); /* 'key1' => {'set1', 'set2', 'set3'}*/
$redis->sRemove('key1', 'set2'); /* 'key1' => {'set1', 'set3'} */
</pre>

## smove
##### *Description*
Move the specified member from the set at srcKey to the set at dstKey.
##### *Parameters*
*srcKey*
*dstKey*
*member*
##### *Return value*
*BOOL* If the operation is successful, return TRUE. If the srcKey or/and dstKey didn't exist, or/and member didn't exist in srcKey, False si returned
##### *Example*
<pre>
$redis->sadd('key1' , 'set11'); 
$redis->sadd('key1' , 'set12'); 
$redis->sadd('key1' , 'set13'); /* 'key1' => {'set11', 'set12', 'set13'}*/
$redis->sadd('key2' , 'set21'); 
$redis->sadd('key2' , 'set22'); /* 'key2' => {'set21', 'set22'}*/
$redis->sMove('key1', 'key2', 'set13'); /* 'key1' =>  {'set11', 'set12'} */
										/* 'key2' =>  {'set21', 'set22', 'set13'} */

</pre>

## sContains
##### *Description*
Verify if member is member of the set stored at the key key.
##### *Parameters*
*key*
*member*

##### *Return value*
*BOOL* True if member is member of the set at key key, False if is not a member.
##### *Example*
<pre>
$redis->sadd('key1' , 'set1'); 
$redis->sadd('key1' , 'set2'); 
$redis->sadd('key1' , 'set3'); /* 'key1' => {'set1', 'set2', 'set3'}*/

$redis->sContains('key1', 'set1'); /* True */
$redis->sContains('key1', 'setX'); /* False */

</pre>

## sSize
##### *Description*
Return the cardinality of the set identified by key.
##### *Parameters*
*key*
##### *Return value*
*LONG* the cardinality of the set identified by key, 0 if set didn't exist.
##### *Example*

## spop
##### *Description*
##### *Parameters*
##### *Return value*
##### *Example*
<pre>
$redis->sadd('key1' , 'set1'); 
$redis->sadd('key1' , 'set2'); 
$redis->sadd('key1' , 'set3'); /* 'key1' => {'set1', 'set2', 'set3'}*/
$redis->sSize('key1'); /* 3 */
$redis->sSize('keyX'); /* 0 */
</pre>

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
##### *Description*
Performs a sInter command and stores the result in a new set.
##### *Parameters*
key0, key1, key2... keyN. key0 is the destination key for the new set, while key1..keyN are intersected as in sInter.

##### *Return value*
The number of elements in the destination set in case of success, FALSE in case of failure.
##### *Example*
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

var_dump($redis->sInterStore('output', 'key1', 'key2', 'key3'));
var_dump($redis->sGetMembers('output'));
</pre>

the output :

<pre>
int(2)

array(2) {
  [0]=>
  string(4) "val4"
  [1]=>
  string(4) "val3"
}
</pre>

## sunion
##### *Description*
##### *Parameters*
##### *Return value*
##### *Example*

## sunionstore
##### *Description*
##### *Parameters*
##### *Return value*
##### *Example*

## sdiff
##### *Description*
##### *Parameters*
##### *Return value*
##### *Example*

## sdiffstore
##### *Description*
##### *Parameters*
##### *Return value*
##### *Example*

## smembers
##### *Description*
Returns the contents of a set.

##### *Parameters*
key: string

##### *Return value*
An array of elements, the contents of the set.

##### *Example*
<pre>
$redis->delete('s');
$redis->sAdd('s', 'a');
$redis->sAdd('s', 'b');
$redis->sAdd('s', 'a');
$redis->sAdd('s', 'c');
var_dump($redis->sGetMembers('s'));
</pre>

Output:
<pre>
array(3) {
  [0]=>
  string(1) "c"
  [1]=>
  string(1) "a"
  [2]=>
  string(1) "b"
}
</pre>
The order is random and corresponds to redis' own internal representation of the set structure.

## getSet
##### *Description*
Sets a value and returns the previous entry at that key.
##### *Parameters*
string: key
string: value
##### *Return value*
A string, the previous value located at this key.
##### *Example*
<pre>
$redis->set('x', '42');
$exValue = $redis->getSet('x', 'lol');	// return '42', replaces x by 'lol'
$newValue = $redis->get('x')'		// return 'lol'
</pre>

## randomKey
##### *Description*
Returns a random key.
##### *Parameters*
None.
##### *Return value*
A string, corresponding to an existing key in redis.

##### *Example*
<pre>
$key = $redis->randomKey();
$surprise = $redis->get($key);	// who knows what's in there.
</pre>

## select
##### *Description*
Switch to a given database.

##### *Parameters*
dbindex: integer. The database number to switch to.

##### *Return value*
TRUE in case of success, FALSE in case of failure.
##### *Example*
(See following function)

## move
##### *Description*
Move a key to a different database.

##### *Parameters*
key: string. The key to move.
dbindex: integer. The database number to move the key to.

##### *Return value*
TRUE in case of success, FALSE in case of failure.
##### *Example*

<pre>
$redis->select(0);	// switch to DB 0
$redis->set('x', '42');	// write 42 to x
$redis->move('x', 1);	// move to DB 1
$redis->select(1);	// switch to DB 1
$redis->get('x');	// will return 42
</pre>

## renameKey
##### *Description*
Renames a key.
##### *Parameters*
srckey: string. The key to rename.
dstkey: string. The new name for the key.

##### *Return value*
TRUE in case of success, FALSE in case of failure.
##### *Example*
<pre>
$redis->set('x', '42');
$redis->renameKey('x', 'y');
$redis->get('y'); 	// → 42
$redis->get('x'); 	// → FALSE
</pre>

## renameNx
##### *Description*
Same as rename, but will not replace a key if the destination already exists. This is the same behaviour as setNx.

## setTimeout
##### *Description*
Sets an expiration date (a timeout) on an item.

##### *Parameters*
key: string. The key that will disappear.
ttl: integer. The key's remaining Time To Live, in seconds.

##### *Return value*
TRUE in case of success, FALSE in case of failure.
##### *Example*
<pre>
$redis->set('x', '42');
$redis->setTimeout('x', 3);	// x will disappear in 3 seconds.
sleep(5);				// wait 5 seconds
$this->get('x'); 		// will return FALSE, as 'x' has expired.
</pre>

## keys
##### *Description*
Returns the keys that match a certain pattern.

##### *Parameters*
pattern: string using '*' as a wildcard.

##### *Return value*
A list of strings, corresponding to keys matching the given pattern.

##### *Example*
<pre>
$allKeys = $redis->getKeys('*');	// all keys will match this.
$keyWithUserPrefix = $redis->getKeys('user*');
</pre>

## dbSize
##### *Description*
Returns the current database's size.

##### *Parameters*
None.

##### *Return value*
DB size, in number of keys.

##### *Example*
<pre>
$count = $redis->dbSize();
echo "Redis has $count keys\n";
</pre>

## auth
##### *Description*
Authenticate the connection using a password.
*Warning*: The password is sent in plain-text over the network.

##### *Parameters*
password: string

##### *Return value*
TRUE if the connection is authenticated, FALSE otherwise.

##### *Example*
<pre>
$redis->auth('foobared');
</pre>

## save
##### *Description*
Performs a synchronous save.

##### *Parameters*
None.

##### *Return value*
TRUE in case of success, FALSE in case of failure. If a save is already running, this command will fail and return FALSE.

##### *Example*
<pre>
$redis->save();
</pre>

## bgsave

##### *Description*
Performs a background save.

##### *Parameters*
None.

##### *Return value*
TRUE in case of success, FALSE in case of failure. If a save is already running, this command will fail and return FALSE.

##### *Example*
<pre>
$redis->bgSave();
</pre>
## lastsave

##### *Description*
Returns the timestamp of the last disk save.

##### *Parameters*
None.

##### *Return value*
Integer timestamp.

##### *Example*
<pre>
$redis->lastSave();
</pre>

## type

##### *Description*
Returns the type of data pointed by a given key.

##### *Parameters*
key: string.

##### *Return value*

Depending on the type of the data pointed by the key, this method will return the following value:
* string: 1
* set: 2
* list: 3
* other: 0

##### *Example*
<pre>
$redis->type('key');
</pre>

## flushdb

##### *Description*
Removes all entries from a given database.

##### *Parameters*
dbindex: integer, the database number to delete from. The first database has number zero.

##### *Return value*
TRUE on success, FALSE on failure.

##### *Example*
<pre>
$redis->flushDB(0);
</pre>


## flushall
##### *Description*
Removes all entries from all databases.

##### *Parameters*
None.

##### *Return value*
TRUE on success, FALSE on failure.

##### *Example*
<pre>
$redis->flushAll();
</pre>

## sort

## info
##### *Description*
Returns an associative array of strings and integers, with the following keys:

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


##### *Parameters*
None.

##### *Example*
<pre>
$redis->info();
</pre>

## ttl
##### *Description*
Returns the time to live left for a given key, in seconds. If the key doesn't exist, FALSE is returned.

##### *Parameters*
key: string

##### *Return value*
Long, the time left to live in seconds.

##### *Example*
<pre>
$redis->ttl('key');
</pre>
