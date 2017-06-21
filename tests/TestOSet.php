<?php
ini_set('display_errors', '1');
error_reporting(E_ALL);
$redis = new Redis();
//$redis->connect('jelly01', '19736');
$redis->connect('jelly02', '10019');

$redis->del('oset');

$int64 = 1 << 62;

$n = $redis->oadd('oset', $int64, $int64+1);
echo "OADD:\n";
var_dump($n);
assert($n === 2);

$n = $redis->ocard('oset');
echo "OCARD:\n";
var_dump($n);
assert($n === 2);

$a = $redis->orange('oset', 0, -1);
echo "ORANGE:\n";
var_dump($a);
assert(count($a) === 2);
for ($i = 0; $i < count($a); $i++) {
    assert($a[$i] === $int64 + $i);
}

$n = $redis->oadd('oset', $int64+2, $int64+3);
echo "OADD:\n";
var_dump($n);
assert($n === 2);

$a = $redis->orevrange('oset', 0, -1);
echo "OREVRANGE:\n";
var_dump($a);
assert(count($a) === 4);
for ($i = 0; $i < count($a); $i++) {
    assert($a[$i] === $int64 + count($a) - $i - 1);
}

$n = $redis->ocard('oset');
echo "OCARD:\n";
var_dump($n);
assert($n === 4);

?>
