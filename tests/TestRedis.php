<?php define('PHPREDIS_TESTRUN', true);

require_once(dirname($_SERVER['PHP_SELF'])."/TestSuite.php");
require_once(dirname($_SERVER['PHP_SELF'])."/RedisTest.php");
require_once(dirname($_SERVER['PHP_SELF'])."/RedisArrayTest.php");
require_once(dirname($_SERVER['PHP_SELF'])."/RedisClusterTest.php");
require_once(dirname($_SERVER['PHP_SELF'])."/RedisSentinelTest.php");

function getClassArray($classes) {
    $result = [];

    if ( ! is_array($classes))
        $classes = [$classes];

    foreach ($classes as $class) {
        $result = array_merge($result, explode(',', $class));
    }

    return array_unique(
        array_map(function ($v) { return strtolower($v); },
            $result
        )
    );
}

function getTestClass($class) {
    $arr_valid_classes = [
        'redis'         => 'Redis_Test',
        'redisarray'    => 'Redis_Array_Test',
        'rediscluster'  => 'Redis_Cluster_Test',
        'redissentinel' => 'Redis_Sentinel_Test'
    ];

    /* Return early if the class is one of our built-in ones */
    if (isset($arr_valid_classes[$class]))
        return $arr_valid_classes[$class];

    /* Try to load it */
    return TestSuite::loadTestClass($class);
}

/* Make sure errors go to stdout and are shown */
error_reporting(E_ALL);
ini_set( 'display_errors','1');

/* Grab options */
$arr_args = getopt('', ['host:', 'port:', 'class:', 'test:', 'nocolors', 'user:', 'auth:']);

/* The test class(es) we want to run */
$arr_classes = getClassArray($arr_args['class'] ?? 'redis');

$boo_colorize = !isset($arr_args['nocolors']);

/* Get our test filter if provided one */
$str_filter = isset($arr_args['test']) ? $arr_args['test'] : NULL;

/* Grab override host/port if it was passed */
$str_host = isset($arr_args['host']) ? $arr_args['host'] : '127.0.0.1';
$i_port = isset($arr_args['port']) ? intval($arr_args['port']) : 6379;

/* Get optional username and auth (password) */
$str_user = isset($arr_args['user']) ? $arr_args['user'] : NULL;
$str_auth = isset($arr_args['auth']) ? $arr_args['auth'] : NULL;

/* Massage the actual auth arg */
$auth = NULL;
if ($str_user && $str_auth) {
    $auth = [$str_user, $str_auth];
} else if ($str_auth) {
    $auth = $str_auth;
} else if ($str_user) {
    echo TestSuite::make_warning("User passed without a password, ignoring!\n");
}

/* Toggle colorization in our TestSuite class */
TestSuite::flagColorization($boo_colorize);

/* Let the user know this can take a bit of time */
echo "Note: these tests might take up to a minute. Don't worry :-)\n";
echo "Using PHP version " . PHP_VERSION . " (" . (PHP_INT_SIZE*8) . " bits)\n";

foreach ($arr_classes as $str_class) {
    $str_class = getTestClass($str_class);

    /* Depending on the classes being tested, run our tests on it */
    echo "Testing class ";
    if ($str_class == 'Redis_Array_Test') {
        echo TestSuite::make_bold("RedisArray") . "\n";

        foreach(array(true, false) as $useIndex) {
            echo "\n". ($useIndex ? "WITH" : "WITHOUT") . " per-node index:\n";

            /* The various RedisArray subtests we can run */
            $arr_ra_tests = [
                'Redis_Array_Test', 'Redis_Rehashing_Test', 'Redis_Auto_Rehashing_Test',
                'Redis_Multi_Exec_Test', 'Redis_Distributor_Test'
            ];

            foreach ($arr_ra_tests as $str_test) {
                /* Run until we encounter a failure */
                if (run_tests($str_test, $str_filter, $str_host, $auth) != 0) {
                    exit(1);
                }
            }
        }
    } else {
        echo TestSuite::make_bold($str_class) . "\n";
        if (TestSuite::run("$str_class", $str_filter, $str_host, $i_port, $auth))
            exit(1);
    }
}

/* Success */
exit(0);

?>
