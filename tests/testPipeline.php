<?php

$r = new Redis();
$r->connect('127.0.0.1', 6379);

/* get, set, getSet, incr, incrby, decr, rename, renameNx */
function test1($r, $type) {

    $ret = $r->multi($type)
	->delete('key1')
	->set('key1', 'value1')
	->get('key1')
	->getSet('key1', 'value2')
	->get('key1')
	->set('key2', 4)
	->incr('key2')
	->get('key2')
	->decr('key2')
	->get('key2')
	->renameKey('key2', 'key3')
	->get('key3')
	->renameNx('key3', 'key1')
	->renameKey('key3', 'key2')
	->incr('key2', 5)
	->get('key2')
	->decr('key2', 5)
	->get('key2')
	->exec();

    assert(is_array($ret));
    $i = 0;
    assert($ret[$i++] == TRUE);
    assert($ret[$i++] == TRUE);
    assert($ret[$i++] == 'value1');
    assert($ret[$i++] == 'value1');
    assert($ret[$i++] == 'value2');
    assert($ret[$i++] == TRUE);
    assert($ret[$i++] == 5);
    assert($ret[$i++] == 5);
    assert($ret[$i++] == 4);
    assert($ret[$i++] == 4);
    assert($ret[$i++] == TRUE);
    assert($ret[$i++] == 4);
    assert($ret[$i++] == FALSE);
    assert($ret[$i++] == TRUE);
    assert($ret[$i++] == TRUE);
    assert($ret[$i++] == 9);
    assert($ret[$i++] == TRUE);
    assert($ret[$i++] == 4);
    assert(count($ret) == $i);

    $ret = $r->multi($type)
	->delete('key1')
	->delete('key2')
	->set('key1', 'val1')
	->setnx('key1', 'valX')
	->setnx('key2', 'valX')
	->exists('key1')
	->exists('key3')
	->ping()
	->exec();

    assert(is_array($ret));
    assert($ret[0] == TRUE);
    assert($ret[1] == TRUE);
    assert($ret[2] == TRUE);
    assert($ret[3] == FALSE);
    assert($ret[4] == TRUE);
    assert($ret[5] == TRUE);
    assert($ret[6] == FALSE);
    assert($ret[7] == '+PONG');

    $ret = $r->multi($type)
	->randomKey()
	->exec();
    $ret = $r->multi($type)
	->exec();
    assert($ret == array());

    // ttl, mget, mset, msetnx, expire, expireAt
    $ret = $r->multi($type)
			->ttl('key')
			->mget(array('key1', 'key2', 'key3'))
			->mset(array('key3' => 'value3', 'key4' => 'value4'))
			->set('key', 'value')
			->expire('key', 5)
			->ttl('key')
			->expireAt('key', '0000')
			->exec();
    assert(is_array($ret));
    $i = 0;
    assert($ret[$i++] == -1);
    assert($ret[$i++] === array('val1', 'valX', FALSE)); // mget
    assert($ret[$i++] === TRUE); // mset
    assert($ret[$i++] === TRUE); // set
    assert($ret[$i++] === TRUE); // expire
    assert($ret[$i++] === 5);    // ttl
    assert($ret[$i++] === TRUE); // expireAt
    assert(count($ret) == $i);

    $ret = $r->multi($type)
			->set('lkey', 'x')
			->set('lDest', 'y')
			->delete('lkey', 'lDest')
			->rpush('lkey', 'lvalue')
			->lpush('lkey', 'lvalue')
			->lpush('lkey', 'lvalue')
			->lpush('lkey', 'lvalue')
			->lpush('lkey', 'lvalue')
			->lpush('lkey', 'lvalue')
			->rpoplpush('lkey', 'lDest')
	  		->lGetRange('lDest', 0, -1)
	  		->lpop('lkey')
			->llen('lkey')
			->lRemove('lkey', 'lvalue', 3)
			->llen('lkey')
	  		->lget('lkey', 0)
			->lGetRange('lkey', 0, -1)
			->lSet('lkey', 1, "newValue")	 // check errors on key not exists
			->lGetRange('lkey', 0, -1)
			->llen('lkey')
			->exec();

    assert(is_array($ret));
    $i = 0;
    assert($ret[$i++] === TRUE); // SET
    assert($ret[$i++] === TRUE); // SET
    assert($ret[$i++] === 2); // deleting 2 keys
    assert($ret[$i++] === 1); // rpush, now 1 element
    assert($ret[$i++] === 2); // lpush, now 2 elements
    assert($ret[$i++] === 3); // lpush, now 3 elements
    assert($ret[$i++] === 4); // lpush, now 4 elements
    assert($ret[$i++] === 5); // lpush, now 5 elements
    assert($ret[$i++] === 6); // lpush, now 6 elements
    assert($ret[$i++] === 'lvalue'); // rpoplpush returns the element: "lvalue"
    assert($ret[$i++] === array('lvalue')); // lDest contains only that one element.
    assert($ret[$i++] === 'lvalue'); // removing a second element from lkey, now 4 elements left ↓
    assert($ret[$i++] === 4); // 4 elements left, after 2 pops.
    assert($ret[$i++] === 3); // removing 3 elements, now 1 left.
    assert($ret[$i++] === 1); // 1 element left
    assert($ret[$i++] === "lvalue"); // this is the current head.
    assert($ret[$i++] === array("lvalue")); // this is the current list.
    assert($ret[$i++] === FALSE); // updating a non-existent element fails.
    assert($ret[$i++] === array("lvalue")); // this is the current list.
    assert($ret[$i++] === 1); // 1 element left
    assert(count($ret) == $i);


    $ret = $r->multi(Redis::PIPELINE)
	    ->delete('lkey', 'lDest')
	    ->rpush('lkey', 'lvalue')
	    ->lpush('lkey', 'lvalue')
	    ->lpush('lkey', 'lvalue')
	    ->rpoplpush('lkey', 'lDest')
	    ->lGetRange('lDest', 0, -1)
	    ->lpop('lkey')
	    ->exec();
    assert(is_array($ret));
    $i = 0;
    assert($ret[$i++] <= 2); // deleted 0, 1, or 2 items
    assert($ret[$i++] === 1); // 1 element in the list
    assert($ret[$i++] === 2); // 2 elements in the list
    assert($ret[$i++] === 3); // 3 elements in the list
    assert($ret[$i++] === 'lvalue'); // rpoplpush returns the element: "lvalue"
    assert($ret[$i++] === array('lvalue')); // rpoplpush returns the element: "lvalue"
    assert($ret[$i++] === 'lvalue'); // pop returns the front element: "lvalue"
    assert(count($ret) == $i);

}

function test2($r, $type) {
	// general command
	$ret = $r->multi($type)
			->select(3)
	  		->set('keyAAA', 'value')
			->set('keyAAB', 'value')
			->dbSize()
			->lastsave()
			->exec();
	$i = 0;
	assert(is_array($ret));
	assert($ret[$i++] === TRUE); // select
	assert($ret[$i++] === TRUE); // set
	assert($ret[$i++] === TRUE); // set
	assert(is_long($ret[$i++])); // dbsize
	assert(is_long($ret[$i++])); // lastsave

	assert(count($ret) === $i);

    $ret = $r->multi($type)
	->delete('key1')
	->set('key1', 'value1')
	->get('key1')
	->getSet('key1', 'value2')
	->get('key1')
	->set('key2', 4)
	->incr('key2')
	->get('key2')
	->decr('key2')
	->get('key2')
	->renameKey('key2', 'key3')
	->get('key3')
	->renameNx('key3', 'key1')
	->renameKey('key3', 'key2')
	->incr('key2', 5)
	->get('key2')
	->decr('key2', 5)
	->get('key2')
	->exec();

    assert(is_array($ret));
    assert($ret[0] == TRUE);
    assert($ret[1] == TRUE);
    assert($ret[2] == 'value1');
    assert($ret[3] == 'value1');
    assert($ret[4] == 'value2');
    assert($ret[5] == TRUE);
    assert($ret[6] == 5);
    assert($ret[7] == 5);
    assert($ret[8] == 4);
    assert($ret[9] == 4);
    assert($ret[10] == TRUE);
    assert($ret[11] == 4);
    assert($ret[12] == FALSE);
    assert($ret[13] == TRUE);
    assert($ret[14] == TRUE);
    assert($ret[15] == 9);
    assert($ret[16] == TRUE);
    assert($ret[17] == 4);

    $ret = $r->multi($type)
	->delete('key1')
	->delete('key2')
	->set('key1', 'val1')
	->setnx('key1', 'valX')
	->setnx('key2', 'valX')
	->exists('key1')
	->exists('key3')
	->ping()
	->exec();

    assert(is_array($ret));
    assert($ret[0] == TRUE);
    assert($ret[1] == TRUE);
    assert($ret[2] == TRUE);
    assert($ret[3] == FALSE);
    assert($ret[4] == TRUE);
    assert($ret[5] == TRUE);
    assert($ret[6] == FALSE);
    assert($ret[7] == '+PONG');

    $ret = $r->multi($type)
	->randomKey()
	->exec();

    assert(is_array($ret) && count($ret) === 1);
    assert(is_string($ret[0]));

    // ttl, mget, mset, msetnx, expire, expireAt
    $ret = $r->multi($type)
			->ttl('key')
			->mget(array('key1', 'key2', 'key3'))
			->mset(array('key3' => 'value3', 'key4' => 'value4'))
			->set('key', 'value')
			->expire('key', 5)
			->ttl('key')
			->expireAt('key', '0000')
			->exec();
    $i = 0;
    assert(is_array($ret));
    assert(is_long($ret[$i++]));
    assert(is_array($ret[$i]) && count($ret[$i]) === 3); // mget
    $i++;
    assert($ret[$i++] === TRUE); // mset always returns TRUE
    assert($ret[$i++] === TRUE); // set always returns TRUE
    assert($ret[$i++] === TRUE); // expire always returns TRUE
    assert($ret[$i++] === 5); // TTL was just set.
    assert($ret[$i++] === TRUE); // expireAt returns TRUE for an existing key
    assert(count($ret) === $i);

    // lists
    $ret = $r->multi($type)
	    ->delete('lkey', 'lDest')
	    ->rpush('lkey', 'lvalue')
	    ->lpush('lkey', 'lvalue')
	    ->lpush('lkey', 'lvalue')
	    ->lpush('lkey', 'lvalue')
	    ->lpush('lkey', 'lvalue')
	    ->lpush('lkey', 'lvalue')
	    ->rpoplpush('lkey', 'lDest')
	    ->lGetRange('lDest', 0, -1)
	    ->lpop('lkey')
	    ->llen('lkey')
	    ->lRemove('lkey', 'lvalue', 3)
	    ->llen('lkey')
	    ->lget('lkey', 0)
	    ->lGetRange('lkey', 0, -1)
	    ->lSet('lkey', 1, "newValue")	 // check errors on missing key
	    ->lGetRange('lkey', 0, -1)
	    ->llen('lkey')
	    ->exec();

    assert(is_array($ret));
    $i = 0;
    assert($ret[$i] >= 0 && $ret[$i] <= 2); // delete
    $i++;
    assert($ret[$i++] === 1); // 1 value
    assert($ret[$i++] === 2); // 2 values
    assert($ret[$i++] === 3); // 3 values
    assert($ret[$i++] === 4); // 4 values
    assert($ret[$i++] === 5); // 5 values
    assert($ret[$i++] === 6); // 6 values
    assert($ret[$i++] === 'lvalue');
    assert($ret[$i++] === array('lvalue')); // 1 value only in lDest
    assert($ret[$i++] === 'lvalue'); // now 4 values left
    assert($ret[$i++] === 4);
    assert($ret[$i++] === 3); // removing 3 elements.
    assert($ret[$i++] === 1); // length is now 1
    assert($ret[$i++] === 'lvalue'); // this is the head
    assert($ret[$i++] === array('lvalue')); // 1 value only in lkey
    assert($ret[$i++] === FALSE); // can't set list[1] if we only have a single value in it.
    assert($ret[$i++] === array('lvalue')); // the previous error didn't touch anything.
    assert($ret[$i++] === 1); // the previous error didn't change the length
    assert(count($ret) === $i);


    // sets
    $ret = $r->multi($type)
	  ->delete('skey1', 'skey2', 'skeydest', 'skeyUnion', 'sDiffDest')
	  ->sadd('skey1', 'sValue1')
	  ->sadd('skey1', 'sValue2')
	  ->sadd('skey1', 'sValue3')
	  ->sadd('skey1', 'sValue4')

	  ->sadd('skey2', 'sValue1')
	  ->sadd('skey2', 'sValue2')

	  ->sSize('skey1')
	  ->sRemove('skey1', 'sValue2')
	  ->sSize('skey1')
	  ->sMove('skey1', 'skey2', 'sValue4')
	  ->sSize('skey2')
	  ->sContains('skey2', 'sValue4')
	  ->sMembers('skey1')
	  ->sMembers('skey2')
	  ->sInter('skey1', 'skey2')
	  ->sInterStore('skeydest', 'skey1', 'skey2')
	  ->sMembers('skeydest')
	  ->sUnion('skey2', 'skeydest')
	  ->sUnionStore('skeyUnion', 'skey2', 'skeydest')
	  ->sMembers('skeyUnion')
	  ->sDiff('skey1', 'skey2')
	  ->sDiffStore('sDiffDest', 'skey1', 'skey2')
	  ->sMembers('sDiffDest')
	  ->sPop('skey2')
	  ->sSize('skey2')
	  ->exec();

    $i = 0;
    assert(is_array($ret));
    assert(is_long($ret[$i]) && $ret[$i] >= 0 && $ret[$i] <= 5); $i++; // deleted at most 5 values.
    assert($ret[$i++] === TRUE); // skey1 now has 1 element.
    assert($ret[$i++] === TRUE); // skey1 now has 2 elements.
    assert($ret[$i++] === TRUE); // skey1 now has 3 elements.
    assert($ret[$i++] === TRUE); // skey1 now has 4 elements.

    assert($ret[$i++] === TRUE); // skey2 now has 1 element.
    assert($ret[$i++] === TRUE); // skey2 now has 2 elements.

    assert($ret[$i++] === 4);
    assert($ret[$i++] === TRUE); // we did remove that value.
    assert($ret[$i++] === 3); // now 3 values only.
    assert($ret[$i++] === TRUE); // the move did succeed.
    assert($ret[$i++] === 3); // sKey2 now has 3 values.
    assert($ret[$i++] === TRUE); // sKey2 does contain sValue4.
    foreach(array('sValue1', 'sValue3') as $k) { // sKey1 contains sValue1 and sValue3.
	    assert(in_array($k, $ret[$i]));
    }
    assert(count($ret[$i++]) === 2);
    foreach(array('sValue1', 'sValue2', 'sValue4') as $k) { // sKey2 contains sValue1, sValue2, and sValue4.
	    assert(in_array($k, $ret[$i]));
    }
    assert(count($ret[$i++]) === 3);
    assert($ret[$i++] === array('sValue1')); // intersection
    assert($ret[$i++] === 1); // intersection + store → 1 value in the destination set.
    assert($ret[$i++] === array('sValue1')); // sinterstore destination contents

    foreach(array('sValue1', 'sValue2', 'sValue4') as $k) { // (skeydest U sKey2) contains sValue1, sValue2, and sValue4.
	    assert(in_array($k, $ret[$i]));
    }
    assert(count($ret[$i++]) === 3); // union size

    assert($ret[$i++] === 3); // unionstore size
    foreach(array('sValue1', 'sValue2', 'sValue4') as $k) { // (skeyUnion) contains sValue1, sValue2, and sValue4.
	    assert(in_array($k, $ret[$i]));
    }
    assert(count($ret[$i++]) === 3); // skeyUnion size

    assert($ret[$i++] === array('sValue3')); // diff skey1, skey2 : only sValue3 is not shared.
    assert($ret[$i++] === 1); // sdiffstore size == 1
    assert($ret[$i++] === array('sValue3')); // contents of sDiffDest

    assert(in_array($ret[$i++], array('sValue1', 'sValue2', 'sValue4'))); // we removed an element from sKey2
    assert($ret[$i++] === 2); // sKey2 now has 2 elements only.

    assert(count($ret) === $i);

    // sorted sets
    $ret = $r->multi($type)
	    ->delete('zkey1', 'zkey2', 'zkey5')
	    ->zadd('zkey1', 1, 'zValue1')
	    ->zadd('zkey1', 5, 'zValue5')
	    ->zadd('zkey1', 2, 'zValue2')
	    ->zRange('zkey1', 0, -1)
	    ->zDelete('zkey1', 'zValue2')
	    ->zRange('zkey1', 0, -1)
	    ->zadd('zkey1', 11, 'zValue11')
	    ->zadd('zkey1', 12, 'zValue12')
	    ->zadd('zkey1', 13, 'zValue13')
	    ->zadd('zkey1', 14, 'zValue14')
	    ->zadd('zkey1', 15, 'zValue15')
	    ->zDeleteRangeByScore('zkey1', 11, 13)
	    ->zrange('zkey1', 0, -1)
	    ->zReverseRange('zkey1', 0, -1)
	    ->zRangeByScore('zkey1', 1, 6)
	    ->zCard('zkey1')
	    ->zScore('zkey1', 'zValue15')
	    ->zadd('zkey2', 5, 'zValue5')
	    ->zadd('zkey2', 2, 'zValue2')
	    //->zRange('zkey1', 'zkey2')
	    ->zInter('zInter', array('zkey1', 'zkey2'))
	    ->zRange('zkey1', 0, -1)
	    ->zRange('zkey2', 0, -1)
	    ->zRange('zInter', 0, -1)
	    ->zInter('zUnion', array('zkey1', 'zkey2'))
	    ->zRange('zUnion', 0, -1)
	    ->zadd('zkey5', 5, 'zValue5')
	    ->zIncrBy('zkey5', 3, 'zValue5') /* fix this */
	    ->zScore('zkey5', 'zValue5')
	    ->exec();
  //  var_dump($ret);

    $i = 0;
    assert(is_array($ret));
    assert(is_long($ret[$i]) && $ret[$i] >= 0 && $ret[$i] <= 3); $i++; // deleting at most 3 keys
    assert($ret[$i++] === 1);
    assert($ret[$i++] === 1);
    assert($ret[$i++] === 1);
    assert($ret[$i++] === array('zValue1', 'zValue2', 'zValue5'));
    assert($ret[$i++] === 1);
    assert($ret[$i++] === array('zValue1', 'zValue5'));
    assert($ret[$i++] === 1); // adding zValue11
    assert($ret[$i++] === 1); // adding zValue12
    assert($ret[$i++] === 1); // adding zValue13
    assert($ret[$i++] === 1); // adding zValue14
    assert($ret[$i++] === 1); // adding zValue15
    assert($ret[$i++] === 3); // deleted zValue11, zValue12, zValue13
    assert($ret[$i++] === array('zValue1', 'zValue5', 'zValue14', 'zValue15'));
    assert($ret[$i++] === array('zValue15', 'zValue14', 'zValue5', 'zValue1'));
    assert($ret[$i++] === array('zValue1', 'zValue5'));
    assert($ret[$i++] === 4); // 4 elements
    assert($ret[$i++] === 15.0);

    return;

	/* hash */
    $ret = $r->multi($type)
	  ->hset('hkey1', 'key1', 'value1')
	  ->hset('hkey1', 'key2', 'value2')
	  ->hset('hkey1', 'key3', 'value3')
	  ->hget('hkey1', 'key1')
	  ->hlen('hkey1')
	  //->hdel('hkey1', 'key2')
	  //->hexists('hkey1', 'key2')
	  ->hkeys('hkey1')
	  ->hvals('hkey1')
	  ->hgetall('hkey1') /* bug */
	  //->hincrby()
	  ->exec();
//	var_dump($ret);

}


$count = 10000;
$count = 10000;

for($i = 1; $i <= $count; $i++) {
	test1($r, Redis::MULTI);
	test1($r, Redis::PIPELINE);

	test2($r, Redis::MULTI);
	test2($r, Redis::PIPELINE);
	/*
	$r->multi(Redis::MULTI)
		->get('x')
		->exec();
	 */
	if($i >= 1000 && $i % ($count/1000) == 0) {
		echo sprintf("%0.1f", 100 * $i/$count)."% (sent ". number_format($i) ." commands)\n";
	}
}


?>
