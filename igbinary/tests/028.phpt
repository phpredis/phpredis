--TEST--
Serialize object into session, full set
--SKIPIF--
--FILE--
<?php 

class Foo {
	private static $s1 = array();
	protected static $s2 = array();
	public static $s3 = array();

	private $d1;
	protected $d2;
	public $d3;

	public function __construct($foo) {
		$this->d1 = $foo;
		$this->d2 = $foo;
		$this->d3 = $foo;
	}
}

class Bar {
	private static $s1 = array();
	protected static $s2 = array();
	public static $s3 = array();

	public $d1;
	private $d2;
	protected $d3;

	public function __construct() {
	}

	public function set($foo) {
		$this->d1 = $foo;
		$this->d2 = $foo;
		$this->d3 = $foo;
	}
}

if(!extension_loaded('igbinary')) {
	dl('igbinary.' . PHP_SHLIB_SUFFIX);
}

$output = '';

function open($path, $name) {
	return true;
}

function close() {
	return true;
}

function read($id) {
	global $output;
	$output .= "read\n";
	$a = new Bar();
	$b = new Foo($a);
	$a->set($b);
	$session = array('old' => $b);
	return igbinary_serialize($session);
}

function write($id, $data) {
	global $output;
	$output .= "write: ";
	$output .= substr(bin2hex($data), 8). "\n";
	return true;
}

function destroy($id) {
	return true;
}

function gc($time) {
	return true;
}

ini_set('session.serialize_handler', 'igbinary');

session_set_save_handler('open', 'close', 'read', 'write', 'destroy', 'gc');

session_start();

$_SESSION['test'] = "foobar";
$a = new Bar();
$b = new Foo($a);
$a->set($b);
$_SESSION['new'] = $a;

session_write_close();

echo $output;

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
read
write: 140311036f6c641703466f6f1403110700466f6f0064311703426172140311026431220111070042617200643222011105002a00643322011105002a00643222021102643322021104746573741106666f6f62617211036e65771a0314030e041a0114030e0222030e0722030e0822030e0522040e062204
