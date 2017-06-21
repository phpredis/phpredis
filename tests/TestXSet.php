<?php
ini_set('display_errors', '1');
error_reporting(E_ALL);
/*
generic_xadd_cmd(INTERNAL_FUNCTION_PARAM_PASSTHRU, "XADD", redis_xadd_cmd);
generic_xadd_cmd(INTERNAL_FUNCTION_PARAM_PASSTHRU, "XINCRBY", redis_xadd_cmd);
generic_zrange_cmd(INTERNAL_FUNCTION_PARAM_PASSTHRU, "XRANGE",
generic_zrange_cmd(INTERNAL_FUNCTION_PARAM_PASSTHRU, "XREVRANGE",
generic_xadd_cmd(INTERNAL_FUNCTION_PARAM_PASSTHRU, "XSETOPTIONS", redis_xadd_cmd);
generic_zrange_cmd(INTERNAL_FUNCTION_PARAM_PASSTHRU, "XRANGEBYSCORE",
generic_zrange_cmd(INTERNAL_FUNCTION_PARAM_PASSTHRU, "XREVRANGEBYSCORE",
cluster_kscan_cmd(INTERNAL_FUNCTION_PARAM_PASSTHRU, TYPE_XSCAN);
*/
$redis = new Redis();
$redis->connect('jelly01', '6888');
$redis->xadd("addTest", 'FINITY', 200, 1, "a");
$redis->xadd("addTest", 'FINITY', 200, 4, "b");
$redis->xadd("addTest", 'FINITY', 200, 2, "c");
$redis->xadd("addTest", 'FINITY', 200, 3, "f");
$redis->xadd("addTest", 'FINITY', 200, 3, "e");
$redis->xadd("addTest", 'FINITY', 200, 3, "e");
$redis->xadd("addTest", 'FINITY', 200, 3, "h");
$redis->xadd("addTest", 'FINITY', 200, 3, "z");
$redis->xadd("addTest", 'FINITY', 200, 3, "x");
$redis->xadd("addTest", 'FINITY', 200, 3, "t");
$res = $redis->xrange("addTest", 0, -1);
assert($res == ["a","c","e","f","g","h","t","x","z","b"]);

$res = $redis->xrevrange("addTest", 0, -1);
assert($res == ["b","z","x","t","h","g","f","e","c","a"]);

$res = $redis->xincrby("addTest", 1000, 'a');
assert($res == false); //存在 false， 不存在 true

$res = $redis->xscore("addTest", 'a');
assert($res == 1001);

$res = $redis->xDelete("addTest", 'a');
assert($res == true);

$res = $redis->xrangebyscore("addTest", 0, '+INF');
//var_dump(json_encode($res));
assert($res == ["c","e","f","g","h","t","x","z","b"]);
$res = $redis->xrevrangebyscore("addTest", '+INF', '-INF');
//var_dump(json_encode($res));
assert($res == ["b","z","x","t","h","g","f","e","c"]);
$res = $redis->xCard("addTest");
//var_dump($res);
assert($res == 9);
$res = $redis->xrank("addTest", 'b');
assert($res == 8);
//var_dump($res);
