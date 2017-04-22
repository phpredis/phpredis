<?php define('PHPREDIS_TESTRUN', true);

require_once(dirname($_SERVER['PHP_SELF'])."/TestSuite.php");
require_once(dirname($_SERVER['PHP_SELF'])."/RedisTest.php");
require_once(dirname($_SERVER['PHP_SELF'])."/RedisArrayTest.php");
require_once(dirname($_SERVER['PHP_SELF'])."/RedisClusterTest.php");

/* Make sure errors go to stdout and are shown */
error_reporting(E_ALL);
ini_set( 'display_errors','1');

/* Grab options */
$arr_args = getopt('', Array('host:', 'class:', 'test:', 'nocolors'));

/* Grab the test the user is trying to run */
$arr_valid_classes = Array('redis', 'redisarray', 'rediscluster');
$str_class = isset($arr_args['class']) ? strtolower($arr_args['class']) : 'redis';
$boo_colorize = !isset($arr_args['nocolors']);

/* Get our test filter if provided one */
$str_filter = isset($arr_args['test']) ? $arr_args['test'] : NULL;

/* Grab override test host if it was passed */
$str_host = isset($arr_args['host']) ? $arr_args['host'] : '127.0.0.1';

/* Validate the class is known */
if (!in_array($str_class, $arr_valid_classes)) {
    echo "Error:  Valid test classes are Redis, RedisArray, and RedisCluster!\n";
    exit(1);
}

/* Toggle colorization in our TestSuite class */
TestSuite::flagColorization($boo_colorize);

/* Let the user know this can take a bit of time */
echo "Note: these tests might take up to a minute. Don't worry :-)\n";
echo "Using PHP version " . PHP_VERSION . " (" . (PHP_INT_SIZE*8) . " bits)\n";

/* Depending on the classes being tested, run our tests on it */
echo "Testing class ";
if ($str_class == 'redis') {
    echo TestSuite::make_bold("Redis") . "\n";
    exit(TestSuite::run("Redis_Test", $str_filter, $str_host));
} else if ($str_class == 'redisarray') {
    echo TestSuite::make_bold("RedisArray") . "\n";
    global $useIndex;
    foreach(array(true, false) as $useIndex) {
        echo "\n".($useIndex?"WITH":"WITHOUT"). " per-node index:\n";

        /* The various RedisArray subtests we can run */
        $arr_ra_tests = Array('Redis_Array_Test', 'Redis_Rehashing_Test', 'Redis_Auto_Rehashing_Test', 'Redis_Multi_Exec_Test', 'Redis_Distributor_Test');
        foreach ($arr_ra_tests as $str_test) {
            /* Run until we encounter a failure */
            if (run_tests($str_test, $str_filter, $str_host) != 0) {
                exit(1);
            }
        }
    }
} else {
    echo TestSuite::make_bold("RedisCluster") . "\n";
    exit(TestSuite::run("Redis_Cluster_Test", $str_filter, $str_host));
}
?>
