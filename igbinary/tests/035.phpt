--TEST--
Profiling perf test.
--SKIPIF--
<?php
	if (!extension_loaded("igbinary") || !extension_loaded("mcrypt")) {
		print "skip";
	}
	if (!isset($_ENV['TEST_PERFORMANCE']) || !$_ENV['TEST_PERFORMANCE']) {
		echo "skip set TEST_PERFORMANCE=1 environment to enable trivial performance test";
	}
?>
--INI--
igbinary.compact_strings=Off
--FILE--
<?php
$data_array = array();
for ($i = 0; $i < 5000; $i++) {
	$data_array[mcrypt_create_iv(10, MCRYPT_DEV_URANDOM)] = mcrypt_create_iv(10, MCRYPT_DEV_URANDOM);
}

$time_start = microtime(true);
for ($i = 0; $i < 4000; $i++) {
	$s = igbinary_serialize($data_array);
	$array = igbinary_unserialize($s);
	unset($array);
	unset($s);
}
$time_end = microtime(true);

if ($time_end <= $time_start) {
	echo "Strange, $i iterations ran in infinite speed: $time_end <= $time_start\n";
} else {
	$speed = $i / ($time_end - $time_start);
	printf("%d iterations took %f seconds: %d/s (%s)\n",
		$i, $time_end - $time_start, $speed, ($speed > 400 ? "GOOD" : "BAD"));
}


--EXPECTF--
%d iterations took %f seconds: %d/s (GOOD)
