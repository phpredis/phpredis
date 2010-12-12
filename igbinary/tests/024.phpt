--TEST--
Recursive objects
--SKIPIF--
--INI--
error_reporting = E_NONE
--FILE--
<?php 
if(!extension_loaded('igbinary')) {
	dl('igbinary.' . PHP_SHLIB_SUFFIX);
}

function test($type, $variable, $test) {
	$serialized = igbinary_serialize($variable);
	$unserialized = igbinary_unserialize($serialized);
//	$serialized = serialize($variable);
//	$unserialized = unserialize($serialized);

	echo $type, "\n";
	echo substr(bin2hex($serialized), 8), "\n";
//	echo $serialized, "\n";
	echo $test || $unserialized == $variable ? 'OK' : 'ERROR';
	echo "\n";
}

class Obj {
	public $a;
	protected $b;
	private $c;

	function __construct($a, $b, $c) {
		$this->a = $a;
		$this->b = $b;
		$this->c = $c;
	}
}

class Obj2 {
	public $aa;
	protected $bb;
	private $cc;
	private $obj;

	function __construct($a, $b, $c) {
		$this->a = $a;
		$this->b = $b;
		$this->c = $c;

		$this->obj = new Obj($a, $b, $c);
	}
}

class Obj3 {
	private $objs;

	function __construct($a, $b, $c) {
		$this->objs = array();

		for ($i = $a; $i < $c; $i += $b) {
			$this->objs[] = new Obj($a, $i, $c);
		}
	}
}

class Obj4 {
	private $a;
	private $obj;

	function __construct($a) {
		$this->a = $a;
	}

	public function set($obj) {
		$this->obj = $obj;
	}
}

$o2 = new Obj2(1, 2, 3);
test('objectrec', $o2, false);

$o3 = new Obj3(0, 1, 4);
test('objectrecarr', $o3, false);

$o4 = new Obj4(100);
$o4->set(&$o4);
test('objectselfrec', $o4, true);

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
objectrec
17044f626a32140711026161001105002a006262001108004f626a32006363001109004f626a32006f626a17034f626a140311016106011104002a006206021106004f626a006306030e06060111016206021101630603
OK
objectrecarr
17044f626a331401110a004f626a33006f626a731404060017034f626a140311016106001104002a006206001106004f626a0063060406011a0214030e0306000e0406010e05060406021a0214030e0306000e0406020e05060406031a0214030e0306000e0406030e050604
OK
objectselfrec
17044f626a3414021107004f626a34006106641109004f626a34006f626a2200
OK
