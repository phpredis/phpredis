--TEST--
Object test, cyclic references
--SKIPIF--
<?php
if(!extension_loaded('igbinary')) {
	echo "skip no igbinary";
}
--FILE--
<?php 

class Foo {
	public $parent;
	public $children;

	public function __construct() {
		$this->parent = null;
		$this->children = array();
	}

	public function addChild(Foo $obj) {
		$this->children[] = $obj;
		$obj->setParent($this);
	}

	public function setParent(Foo $obj) {
		$this->parent = $obj;
	}
}

$obj1 = new Foo();

for ($i = 0; $i < 10; $i++) {
	$obj = new Foo();
	$obj1->addChild($obj);
}

$o = igbinary_unserialize(igbinary_serialize($obj1->children));

foreach ($obj1->children as $k => $v) {
	$obj_v = $v;
	$o_v = $o[$k];

	echo gettype($obj_v), "\t", gettype($o_v), "\n";
}
--EXPECT--
object	object
object	object
object	object
object	object
object	object
object	object
object	object
object	object
object	object
object	object
