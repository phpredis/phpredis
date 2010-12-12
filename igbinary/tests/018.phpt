--TEST--
Object test, __sleep error cases
--SKIPIF--
--FILE--
<?php 
if(!extension_loaded('igbinary')) {
	dl('igbinary.' . PHP_SHLIB_SUFFIX);
}

error_reporting(0);

function test($type, $variable, $test) {
	$serialized = igbinary_serialize($variable);
	$unserialized = igbinary_unserialize($serialized);

	echo $type, "\n";
	echo substr(bin2hex($serialized), 8), "\n";
	echo $test || $unserialized == $variable ? 'OK' : 'ERROR';
	echo "\n";
}

class Obj {
	var $a;
	var $b;

	function __construct($a, $b) {
		$this->a = $a;
		$this->b = $b;
	}

	function __sleep() {
		return array('c');
	}

#	function __wakeup() {
#		$this->b = $this->a * 3;
#	}
}

class Opj {
	var $a;
	var $b;

	function __construct($a, $b) {
		$this->a = $a;
		$this->b = $b;
	}

	function __sleep() {
		return array(1);
	}

#	function __wakeup() {
#
#	}
}

$o = new Obj(1, 2);
$p = new Opj(1, 2);

test('nonexisting', $o, true);
test('wrong', $p, true);

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
nonexisting
17034f626a140111016300
OK
wrong
17034f706a140100
OK
