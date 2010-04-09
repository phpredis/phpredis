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
    var_dump($ret);


}
//test1($r, Redis::MULTI);
test1($r, Redis::PIPELINE);
?>