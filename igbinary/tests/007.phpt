--TEST--
Check for simple array serialization
--SKIPIF--
--FILE--
<?php 
if(!extension_loaded('igbinary')) {
	dl('igbinary.' . PHP_SHLIB_SUFFIX);
}

function test($type, $variable) {
	$serialized = igbinary_serialize($variable);
	$unserialized = igbinary_unserialize($serialized);

	echo $type, "\n";
	echo substr(bin2hex($serialized), 8), "\n";
	echo $unserialized == $variable ? 'OK' : 'ERROR';
	echo "\n";
}

test('empty array: ', array());
test('array(1, 2, 3)', array(1, 2, 3));
test('array(array(1, 2, 3), arr...', array(array(1, 2, 3), array(4, 5, 6), array(7, 8, 9)));

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
--EXPECT--
empty array: 
1400
OK
array(1, 2, 3)
1403060006010601060206020603
OK
array(array(1, 2, 3), arr...
1403060014030600060106010602060206030601140306000604060106050602060606021403060006070601060806020609
OK
