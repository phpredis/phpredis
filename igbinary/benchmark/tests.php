<?php

if (!extension_loaded('igbinary')) {
	dl('igbinary.' . PHP_SHLIB_SUFFIX);
}

function test_speed_print($testname, $php, $igbinary) {
	sort($php);
	sort($igbinary);
	reset($php);
	reset($igbinary);

	$phptime = ($php[1] + $php[2] + $php[3]) / 3;
	$igbinarytime = ($igbinary[1] + $igbinary[2] + $igbinary[3]) / 3;

	$phpstr = sprintf('%.3f', $php[0]);
	$igbinarystr = sprintf('%.3f', $igbinary[0]);

	for ($i = 1; $i < 5; ++$i) {
		$phpstr .= sprintf(' %.3f', $php[$i]);
		$igbinarystr .= sprintf(' %.3f', $igbinary[$i]);

	}

	echo $testname, ":\n";
	printf("  php     : %6.3f         [%s]\n", $phptime, $phpstr);
	printf("  igbinary: %6.3f %6.02f%% [%s]\n", $igbinarytime, $igbinarytime * 100 / $phptime, $igbinarystr);
}

function test_speed_serialize_php($data, $loops) {
	$start = microtime(true);
	for ($i = 0; $i < $loops; ++$i) {
		$tmp = serialize($data);
	}
	$end = microtime(true);
	return $end - $start;
}

function test_speed_serialize_igbinary($data, $loops) {
	$start = microtime(true);
	for ($i = 0; $i < $loops; ++$i) {
		$tmp = igbinary_serialize($data);
	}
	$end = microtime(true);
	return $end - $start;
}

function test_speed_serialize($testname, $data, $loops) {
	$php = array();
	$igbinary = array();

	for ($i = 0; $i < 5; ++$i) {
		$php[] = test_speed_serialize_php($data, $loops);
		$igbinary[] = test_speed_serialize_igbinary($data, $loops);
	}

	test_speed_print($testname, $php, $igbinary);
}

function test_speed_unserialize_php($data, $loops) {
	$serdata = serialize($data);
	$start = microtime(true);
	for ($i = 0; $i < $loops; ++$i) {
		$tmp = unserialize($serdata);
	}
	$end = microtime(true);
	return $end - $start;
}

function test_speed_unserialize_igbinary($data, $loops) {
	$serdata = igbinary_serialize($data);
	$start = microtime(true);
	for ($i = 0; $i < $loops; ++$i) {
		$tmp = igbinary_unserialize($serdata);
	}
	$end = microtime(true);
	return $end - $start;
}

function test_speed_unserialize($testname, $data, $loops) {
	$php = array();
	$igbinary = array();

	for ($i = 0; $i < 5; ++$i) {
		$php[] = test_speed_unserialize_php($data, $loops);
		$igbinary[] = test_speed_unserialize_igbinary($data, $loops);
	}

	test_speed_print($testname, $php, $igbinary);
}


function test_space($testname, $data) {
	$phplen = strlen(serialize($data));
	$igbinarylen = strlen(igbinary_serialize($data));

	printf("  php     : %5u\n", $phplen);
	printf("  igbinary: %5u (%.02f%%)\n", $igbinarylen, $igbinarylen * 100 / $phplen);
}

function usage() {
	global $argv;

	die('Usage: php '.$argv[0].' space|serialize|unserialize [data] [loops]'."\n");
}

if (count($argv) < 3) {
	usage();
}

$t = $argv[1];
$i = $argv[2];

$loops = 100000;
if (count($argv) > 3) {
	$loops = (int) $argv[3];

	if ($loops < 100 || $loops > 1000000) {
		$loops = 100000;
	}
}

if ((@include($i . '.php')) != 1) {
	usage();
}

switch ($t) {
	case 'space':
		test_space($i, $data);
		break;
	case 'serialize':
		test_speed_serialize($i, $data, $loops);
		break;
	case 'unserialize':
		test_speed_unserialize($i, $data, $loops);
		break;
	default:
		usage();
}

?>
