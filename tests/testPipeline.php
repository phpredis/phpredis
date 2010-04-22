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
    var_dump($ret);
	
	/* ttl, mget, mset, msetnx, expire, expireAt */
    $ret = $r->multi($type)
			->ttl('key')
	//		->mget(array('key1', 'key2', 'key3'))
	//		->mset(array('key3' => 'value3', 'key4' => 'value4'))
			->set('key', 'value')
			->expire('key', 5)			
			->ttl('key')
			->expireAt('key', '0000')
			->exec();	
	var_dump($ret);
	/* lpush, rpush, rpop, llen, lpop, rpop */
	/*rpoplpush, lRemove, ... */
	/* LGET, lGetRange */
	/* LSET, ... */

    $ret = $r->multi($type)
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
			->lSet('lkey', 1, "newValue")	 /* check errors on key not exists */
			->lGetRange('lkey', 0, -1)
			->llen('lkey')
			->exec();
		
	var_dump($ret);

}

// test1($r, Redis::MULTI);
test1($r, Redis::PIPELINE);
?>
