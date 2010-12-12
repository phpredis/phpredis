--TEST--
Object test, __sleep and __wakeup exceptions
--SKIPIF--
<?php
if(!extension_loaded('igbinary')) {
	echo "skip no igbinary";
}
--FILE--
<?php 

function test($variable) {
	$serialized = igbinary_serialize($variable);
	$unserialized = igbinary_unserialize($serialized);
}

class Obj {
	private static $count = 0;
	var $a;
	var $b;

	function __construct($a, $b) {
		$this->a = $a;
		$this->b = $b;
	}

	function __sleep() {
		$c = self::$count++;
		if ($this->a) {
			throw new Exception("exception in __sleep $c");
		}
		return array('a', 'b');
	}

	function __wakeup() {
		$c = self::$count++;
		if ($this->b) {
			throw new Exception("exception in __wakeup $c");
		}
		$this->b = $this->a * 3;
	}
}


$a = new Obj(1, 0);
$b = new Obj(0, 1);
$c = new Obj(0, 0);

try {
	test($a);
} catch (Exception $e) {
	echo $e->getMessage(), "\n";
}

try {
	test($b);
} catch (Exception $e) {
	echo $e->getMessage(), "\n";
}

try {
	test($c);
} catch (Exception $e) {
	echo $e->getMessage(), "\n";
}

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
exception in __sleep 0
exception in __wakeup 2
