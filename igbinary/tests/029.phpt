--TEST--
Igbinary module info
--SKIPIF--
<?php if (!extension_loaded("igbinary")) print "skip"; ?>
--FILE--
<?php 
ob_start();
phpinfo(INFO_MODULES);
$str = ob_get_clean();

$array = explode("\n", $str);
$array = preg_grep('/^igbinary/', $array);

echo implode("\n", $array);

--EXPECTF--
igbinary
igbinary support => enabled
igbinary version => %s
igbinary revision => %s
igbinary.compact_strings => %s => %s
