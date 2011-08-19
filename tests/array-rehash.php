<?php

$firstRing = array('localhost:6379', 'localhost:6380', 'localhost:6381');
$secondRing = array('localhost:6379', 'localhost:6380', 'localhost:6381', 'localhost:6382');

// prepare by flushing db
foreach($secondRing as $s) {
	list($host, $port) = explode(':', $s);

	$r = new Redis;
	$r->connect($host, (int)$port);
	$r->flushdb();
}

function report_info($ra) {
	$infos = $ra->info();
	foreach($infos as $host => $info) {
		echo "Host $host: ".(isset($info['db0'])?$info['db0']:0)."\n";
	}
}

// initial key distribution
$n = 10;
$data = array();
for($i = 0; $i < $n; $i++) {
	$tmp = $i; //rand();
	$data['key-'.$tmp] = 'val-'.$tmp;
}

echo "Distributing $n keys around the ring.\n";
// first, distribute the keys to the original ring
$ra = new RedisArray($firstRing, NULL, array(), TRUE);
foreach($data as $k => $v) {
	$ra->set($k, $v); // strings
}

// sets
$ra->sadd('sx', 'a', 'b', 'c');
$ra->sadd('sy', 'd', 'e', 'f');
$ra->sadd('sz', 'g', 'h', 'i');

// lists
$ra->rpush('lx', 'a', 'b', 'c');
$ra->rpush('ly', 'd', 'e', 'f');
$ra->rpush('lz', 'g', 'h', 'i');

// hashes
$ra->hmset('hx', array('a' => 'A', 'b' => 'B', 'c' => 'C'));
$ra->hmset('hy', array('d' => 'D', 'e' => 'E', 'f' => 'F'));
$ra->hmset('hz', array('g' => 'G', 'h' => 'H', 'i' => 'I'));

// sorted sets.
$ra->zadd('zx', 1.0, 'a', 2.0, 'b', 3.0, 'c');
$ra->zadd('zy', 1.0, 'd', 5.0, 'e', 6.0, 'f');
$ra->zadd('zz', 1.0, 'g', 8.0, 'h', 6.0, 'i');

report_info($ra);

echo "Reading back all the values.\n";
foreach($data as $k => $v) {
	$ret = $ra->get($k);
	if($ret !== $v) {
		echo "Could not match expected value $v for key $k, instead got:"; var_dump($ret);
		die;
	}
}
echo "All key/value pairs read successfuly.\n";

echo "Creating a new, larger ring and reading back all the values.\n";
$ra = new RedisArray($secondRing, NULL, $firstRing, TRUE);
foreach($data as $k => $v) {
	$ret = $ra->get($k);
	if($ret !== $v) {
		echo "Could not match expected value $v for key $k, instead got:"; var_dump($ret);
		die;
	}
}
echo "All key/value pairs read successfuly using the new setup with a fallback\n";
report_info($ra);

echo "Rehash array.\n";
$ra->_rehash();

echo "Creating a new ring without fallback and reading back all the values.\n";
$ra = new RedisArray($secondRing);
foreach($data as $k => $v) {
	$ret = $ra->get($k);
	if($ret !== $v) {
		echo "Could not match expected value $v for key $k, instead got:"; var_dump($ret);
		die;
	}
}
echo "All key/value pairs read successfuly using the new setup without a fallback\n";
report_info($ra);

?>
