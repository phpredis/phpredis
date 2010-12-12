--TEST--
Object Serializable interface throws exceptions
--SKIPIF--
--FILE--
<?php 
if(!extension_loaded('igbinary')) {
	dl('igbinary.' . PHP_SHLIB_SUFFIX);
}

function test($variable) {
	$serialized = igbinary_serialize($variable);
	$unserialized = igbinary_unserialize($serialized);
}

class Obj implements Serializable {
	private static $count = 1;

	var $a;
	var $b;

	function __construct($a, $b) {
		$this->a = $a;
		$this->b = $b;
	}

	public function serialize() {
		$c = self::$count++;
		echo "call serialize, ", ($this->a ? "throw" : "no throw"),"\n";
		if ($this->a) {
			throw new Exception("exception in serialize $c");
		}
		return pack('NN', $this->a, $this->b);
	}

	public function unserialize($serialized) {
		$tmp = unpack('N*', $serialized);
		$this->__construct($tmp[1], $tmp[2]);
		$c = self::$count++;
		echo "call unserialize, ", ($this->b ? "throw" : "no throw"),"\n";
		if ($this->b) {
			throw new Exception("exception in unserialize $c");
		}
	}
}

$a = new Obj(1, 0);
$a = new Obj(0, 0);
$b = new Obj(0, 0);
$c = new Obj(1, 0);
$d = new Obj(0, 1);

echo "a, a, c\n";
try {
	test(array($a, $a, $c));
} catch (Exception $e) {
	if (version_compare(phpversion(), "5.3.0", ">=")) {
		if ($e->getPrevious()) {
			$e = $e->getPrevious();
		}
	}

	echo $e->getMessage(), "\n";
}

echo "b, b, d\n";

try {
	test(array($b, $b, $d));
} catch (Exception $e) {
	if (version_compare(phpversion(), "5.3.0", ">=")) {
		if ($e->getPrevious()) {
			$e = $e->getPrevious();
		}
	}

	echo $e->getMessage(), "\n";
}

--EXPECT--
a, a, c
call serialize, no throw
call serialize, throw
exception in serialize 2
b, b, d
call serialize, no throw
call serialize, no throw
call unserialize, no throw
call unserialize, throw
exception in unserialize 6
