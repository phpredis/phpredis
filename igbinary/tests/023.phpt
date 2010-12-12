--TEST--
Resource
--SKIPIF--
<?php 
if (!extension_loaded("igbinary")) print "skip extenion not loaded\n";
--FILE--
<?php 

function test($type, $variable, $test) {
	$serialized = igbinary_serialize($variable);
	$unserialized = igbinary_unserialize($serialized);

	echo $type, "\n";
	echo substr(bin2hex($serialized), 8), "\n";
	echo $test || $unserialized === null ? 'OK' : 'FAIL';
	echo "\n";
}

$res = tmpfile();

test('resource', $res, false);

fclose($res);

--EXPECT--
resource
00
OK
