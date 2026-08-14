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

function getOriginalCommand(array $args): array {
    $fallback = array_merge([PHP_BINARY], $args);

    /* On Linux, /proc preserves the executable and any PHP options exactly as
     * they were invoked. */
    if ( ! is_readable('/proc/self/cmdline'))
        return $fallback;

    $raw = @file_get_contents('/proc/self/cmdline');
    if ($raw === false)
        return $fallback;

    $command = explode("\0", rtrim($raw, "\0"));

    /* The PHP-visible argument vector should be the tail of the process
     * command line, beginning with the test script. */
    if (count($command) < count($args) ||
        array_slice($command, -count($args)) !== $args)
    {
        return $fallback;
    }

    return $command;
}

function escapeCommandArgument(string $arg): string {
    if (preg_match('/^[a-zA-Z0-9_@%+=:,\.\/-]+$/D', $arg))
        return $arg;

    return escapeshellarg($arg);
}

function getFailedTestCommand(array $args, array $failed_tests): string {
    $command = getOriginalCommand($args);
    $script_index = count($command) - count($args);
    $result = array_slice($command, 0, $script_index + 1);

    /* Keep the original invocation but replace its test filters with the
     * complete set of failed tests. */
    for ($i = $script_index + 1; $i < count($command); $i++) {
        if ($command[$i] === '--test') {
            $i++;
            continue;
        }

        if (strncmp($command[$i], '--test=', strlen('--test=')) === 0)
            continue;

        $result[] = $command[$i];
    }

    $result[] = '--test';
    $result[] = implode(',', $failed_tests);

    return implode(' ', array_map('escapeCommandArgument', $result));
}

function printFailedTestCommand(array $args) {
    $failed_tests = TestSuite::getFailedTests();

    if ( ! $failed_tests)
        return;

    echo "\nTo rerun only the failed tests:\n";
    echo getFailedTestCommand($args, $failed_tests) . "\n";
}

/* Make sure errors go to stdout and are shown */
error_reporting(E_ALL);
ini_set( 'display_errors','1');

/* Grab options */
$opt = getopt('', ['host:', 'port:', 'tls-port:', 'class:', 'test:', 'nocolors', 'user:', 'auth:']);

/* The test class(es) we want to run */
$classes = getClassArray($opt['class'] ?? 'redis');

$colorize = !isset($opt['nocolors']);

/* Get our test filter if provided one */
$filter = $opt['test'] ?? NULL;

/* Grab override host/port if it was passed */
$host = $opt['host'] ?? '127.0.0.1';
$port = $opt['port'] ?? 6379;
$tls_port = $opt['tls-port'] ?? 6378;

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
                    printFailedTestCommand($argv);
                    exit(1);
                }
            }
        }
    } else {
        echo TestSuite::make_bold($class) . "\n";
        if (TestSuite::run("$class", $filter, $host, $port, $auth, $tls_port)) {
            printFailedTestCommand($argv);
            exit(1);
        }
    }
}

/* Success */
exit(0);

?>
