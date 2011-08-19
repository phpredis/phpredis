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

$n = 10000;
$data = array();
for($i = 0; $i < $n; $i++) {
	$tmp = $i; //rand();
	$data['key-'.$tmp] = 'val-'.$tmp;
}

echo "Distributing $n keys around the ring.\n";
// first, distribute the keys to the original ring
$ra = new RedisArray($firstRing, NULL, array(), TRUE);
foreach($data as $k => $v) {
	$ra->set($k, $v);
}
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
