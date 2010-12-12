--TEST--
Object test, __sleep
--SKIPIF--
--FILE--
<?php 
if(!extension_loaded('igbinary')) {
	dl('igbinary.' . PHP_SHLIB_SUFFIX);
}

function test($type, $variable, $test) {
	$serialized = igbinary_serialize($variable);
	$unserialized = igbinary_unserialize($serialized);

	echo $type, "\n";
	echo substr(bin2hex($serialized), 8), "\n";
	echo $test || $unserialized == $variable ? 'OK' : 'ERROR';
	echo "\n";
}

class Obj {
	public $a;
	protected $b;
	private $c;
	var $d;

	function __construct($a, $b, $c, $d) {
		$this->a = $a;
		$this->b = $b;
		$this->c = $c;
		$this->d = $d;
	}

	function __sleep() {
		return array('a', 'b', 'c');
	}

#	function __wakeup() {
#		$this->d = $this->a + $this->b + $this->c;
#	}
}

$o = new Obj(1, 2, 3, 4);


test('object', $o, true);

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
object
17034f626a140311016106011104002a006206021106004f626a00630603
OK
