<?php
require_once(dirname($_SERVER['PHP_SELF'])."/TestSuite.php");
require_once(dirname($_SERVER['PHP_SELF'])."/RedisTest.php");
require_once(dirname($_SERVER['PHP_SELF'])."/RedisArrayTest.php");
require_once(dirname($_SERVER['PHP_SELF'])."/RedisClusterTest.php");

/* Grab the test the user is trying to run */
$arr_valid_classes = Array('redis', 'redisarray', 'rediscluster');
$str_class = isset($argv[1]) ? strtolower($argv[1]) : 'redis';

/* Validate the class is known */
if (!in_array($str_class, $arr_valid_classes)) {
    echo "Error:  Valid test classes are Redis, RedisArray, and RedisCluster!\n";
    exit(1);
}

/* Let the user know this can take a bit of time */
echo "Note: these tests might take up to a minute. Don't worry :-)\n";

/* Depending on the classes being tested, run our tests on it */
if ($str_class = 'redis') {
    exit(TestSuite::run("Redis_Test"));
} else if ($str_class == 'redisarray') {
    global $useIndex;
    foreach(array(true, false) as $useIndex) {
        echo "\n".($useIndex?"WITH":"WITHOUT"). " per-node index:\n";

        run_tests('Redis_Array_Test');
        run_tests('Redis_Rehashing_Test');
        run_tests('Redis_Auto_Rehashing_Test');
        run_tests('Redis_Multi_Exec_Test');
        run_tests('Redis_Distributor_Test');
    }
} else {

}
?>
