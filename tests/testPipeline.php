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


$count = 1000000;
// $count = 10;

for($i = 1; $i <= $count; $i++) {
	test1($r, Redis::MULTI);
	test1($r, Redis::PIPELINE);
	$r->multi(Redis::MULTI)
		->get('x')
		->exec();
	if($i >= 1000 && $i % ($count/1000) == 0) {
		echo sprintf("%0.1f", 100 * $i/$count)."% (sent ". number_format($i) ." commands)\n";
	}
}


?>
