<?php

echo "Memory usage should remain stable after the first iteration:\n";

function ra(){
	return new RedisArray(array('localhost:6379', 'localhost:6380', 'localhost:6381', 'localhost:6382'));
}

function data() {
	srand(1);
	$data = array();
	for($i = 0; $i < 10; $i++) {
		$data['key-'.$i] = rand();
	}
	return $data;
}

$data = data();
$last = memory_get_usage();

for($i = 0; $i < 10; $i++) {
		$ra = ra();

		echo "$i) " . (memory_get_usage() - $last) . " bytes\n";
		$ra->mset($data);
		foreach($data as $k => $v) {
			if($v != $ra->get($k)) {
				echo "Expected $v\n";
				die("FAIL");
			}
		}

		$ra = ra();
		$data = data();
		if(array_values($data) != $ra->mget(array_keys($data))) {
			die("FAIL");
		}
}
?>
