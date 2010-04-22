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

    assert($ret == array(TRUE, TRUE, 'value1', 'value1', 'value2', TRUE, 5, 5, 4, 4, TRUE, 4, FALSE, TRUE, TRUE, 9, TRUE, 4 ));

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
    var_dump($ret);
    assert($ret == array(TRUE, TRUE, TRUE, FALSE, TRUE, TRUE, FALSE, '+PONG'));

    $ret = $r->multi($type)
	->randomKey()
	->exec();
    //var_dump($ret);
	
	/* ttl, mget, mset, msetnx, expire, expireAt */
    $ret = $r->multi($type)
			->ttl('key')
			->mget(array('key1', 'key2', 'key3'))
			//->mset(array('key3' => 'value3', 'key4' => 'value4'))
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

//test1($r, Redis::MULTI);
test1($r, Redis::PIPELINE);
?>