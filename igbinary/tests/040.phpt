--TEST--
b0rked random data test
--SKIPIF--
--FILE--
<?php
error_reporting(E_ERROR | E_PARSE);

if(!extension_loaded('igbinary')) {
	dl('igbinary.' . PHP_SHLIB_SUFFIX);
}

function test() {
	$serialized = igbinary_serialize(null);
	$serialized = substr($serialized, 0, -1);

	$length = mt_rand(1, 255);
	for ($i = 0; $i < $length; ++$i) {
		$serialized .= chr(mt_rand(0, 255));
	}

	// if returned null everything is OK
	if (($unserialized = igbinary_unserialize($serialized)) === null) {
		return true;
	}

	// whole data is read?
	if ($serialized !== igbinary_serialize($unserialized)) {
		return true;
	}

	echo bin2hex($serialized), "\n";
	var_dump($unserialized);

	return false;
}

mt_srand(0xface);
for ($i = 0; $i < 100; ++$i) {
	if (!test()) break;
}

?>
--EXPECT--
