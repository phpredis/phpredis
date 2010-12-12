--TEST--
Object test, incomplete class
--SKIPIF--
--FILE--
<?php 
if(!extension_loaded('igbinary')) {
	dl('igbinary.' . PHP_SHLIB_SUFFIX);
}

function test($type, $variable, $test) {
	$serialized = pack('H*', $variable);
	$unserialized = igbinary_unserialize($serialized);

	echo $type, "\n";
	echo substr(bin2hex($serialized), 8), "\n";
	var_dump($unserialized);
//	echo "\n";
}

test('incom', '0000000217034f626a140211016106011101620602', false);

/*
 * you can add regression tests for your extension here
 *
 * the output of your test code has to be equal to the
 * text in the --EXPECT-- section below for the tests
 * to pass, differences between the output and the
 * expected text are interpreted as failure
 *
 * see php5/README.TESTING for further information on
 * writing regression tests
 */
?>
--EXPECTF--
incom
17034f626a140211016106011101620602
object(__PHP_Incomplete_Class)#%d (3) {
  ["__PHP_Incomplete_Class_Name"]=>
  string(3) "Obj"
  ["a"]=>
  int(1)
  ["b"]=>
  int(2)
}
