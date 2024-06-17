<?php define('PHPREDIS_TESTRUN', true);

require_once __DIR__ . "/TestSuite.php";
require_once __DIR__ . "/RedisTest.php";
require_once __DIR__ . "/RedisArrayTest.php";
require_once __DIR__ . "/RedisClusterTest.php";
require_once __DIR__ . "/RedisSentinelTest.php";

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
    $valid_classes = [
        'redis'         => 'Redis_Test',
        'redisarray'    => 'Redis_Array_Test',
        'rediscluster'  => 'Redis_Cluster_Test',
        'redissentinel' => 'Redis_Sentinel_Test'
    ];

    /* Return early if the class is one of our built-in ones */
    if (isset($valid_classes[$class]))
        return $valid_classes[$class];

    /* Try to load it */
    return TestSuite::loadTestClass($class);
}

function raHosts($host, $ports) {
    if ( ! is_array($ports))
        $ports = [6379, 6380, 6381, 6382];

    return array_map(function ($port) use ($host) {
        return sprintf("%s:%d", $host, $port);
    }, $ports);
}

/* Make sure errors go to stdout and are shown */
error_reporting(E_ALL);
ini_set( 'display_errors','1');

/* Grab options */
$opt = getopt('', ['host:', 'port:', 'class:', 'test:', 'nocolors', 'user:', 'auth:']);

/* The test class(es) we want to run */
$classes = getClassArray($opt['class'] ?? 'redis');

$colorize = !isset($opt['nocolors']);

/* Get our test filter if provided one */
$filter = $opt['test'] ?? NULL;

/* Grab override host/port if it was passed */
$host = $opt['host'] ?? '127.0.0.1';
$port = $opt['port'] ?? 6379;

/* Get optional username and auth (password) */
$user = $opt['user'] ?? NULL;
$auth = $opt['auth'] ?? NULL;

if ($user && $auth) {
    $auth = [$user, $auth];
} else if ($user && ! $auth) {
    echo TestSuite::make_warning("User passed without a password!\n");
}

/* Toggle colorization in our TestSuite class */
TestSuite::flagColorization($colorize);

/* Let the user know this can take a bit of time */
echo "Note: these tests might take up to a minute. Don't worry :-)\n";
echo "Using PHP version " . PHP_VERSION . " (" . (PHP_INT_SIZE * 8) . " bits)\n";

foreach ($classes as $class) {
    $class = getTestClass($class);

    /* Depending on the classes being tested, run our tests on it */
    echo "Testing class ";
    if ($class == 'Redis_Array_Test') {
        echo TestSuite::make_bold("RedisArray") . "\n";

        $full_ring = raHosts($host, $port);
        $sub_ring  = array_slice($full_ring, 0, -1);

        echo TestSuite::make_bold("Full Ring: ") . implode(' ', $full_ring) . "\n";
        echo TestSuite::make_bold(" New Ring: ") . implode(' ',  $sub_ring) . "\n";

        foreach([true, false] as $useIndex) {
            echo "\n". ($useIndex ? "WITH" : "WITHOUT") . " per-node index:\n";

            /* The various RedisArray subtests we can run */
            $test_classes = [
                'Redis_Array_Test', 'Redis_Rehashing_Test', 'Redis_Auto_Rehashing_Test',
                'Redis_Multi_Exec_Test', 'Redis_Distributor_Test'
            ];

            foreach ($test_classes as $test_class) {
                /* Run until we encounter a failure */
                if (run_ra_tests($test_class, $filter, $host, $full_ring, $sub_ring, $auth) != 0) {
                    exit(1);
                }
            }
        }
    } else {
        echo TestSuite::make_bold($class) . "\n";
        if (TestSuite::run("$class", $filter, $host, $port, $auth))
            exit(1);
    }
}

/* Success */
exit(0);

?>
