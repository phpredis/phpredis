--TEST--
Check for reference serialisation
--INI--
report_memleaks=0
--SKIPIF--
<?php
if(!extension_loaded('igbinary')) {
	echo "skip no igbinary";
}
--FILE--
<?php 

function test($type, $variable, $test) {
	$serialized = igbinary_serialize($variable);
	$unserialized = igbinary_unserialize($serialized);

	echo $type, "\n";
	echo substr(bin2hex($serialized), 8), "\n";
	echo $test || $unserialized == $variable ? 'OK' : 'ERROR';
	echo "\n";
}

$a = array('foo');

test('array($a, $a)', array($a, $a), false);
test('array(&$a, &$a)', array(&$a, &$a), false);

$a = array(null);
$b = array(&$a);
$a[0] = &$b;

test('cyclic', $a, true);

var_dump($a);
var_dump(igbinary_unserialize(igbinary_serialize($a)));

--EXPECT--
array($a, $a)
14020600140106001103666f6f06010101
OK
array(&$a, &$a)
1402060025140106001103666f6f0601250101
OK
cyclic
1401060025140106002514010600250101
OK
array(1) {
  [0]=>
  &array(1) {
    [0]=>
    &array(1) {
      [0]=>
      &array(1) {
        [0]=>
        &array(1) {
          [0]=>
          *RECURSION*
        }
      }
    }
  }
}
array(1) {
  [0]=>
  &array(1) {
    [0]=>
    &array(1) {
      [0]=>
      &array(1) {
        [0]=>
        &array(1) {
          [0]=>
          *RECURSION*
        }
      }
    }
  }
}
