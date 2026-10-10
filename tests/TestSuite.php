<?php defined('PHPREDIS_TESTRUN') or die("Use TestRedis.php to run tests!\n");

/* A specific exception for when we skip a test */
class TestSkippedException extends Exception {}

/* Stop the current test on the first failed assertion. */
class TestFailedException extends Exception {}

// phpunit is such a pain to install, we're going with pure-PHP here.
class TestSuite
{
    /* Host and port the unit tests will use */
    private string $host;
    private ?int $port = 6379;
    private ?int $tls_port = 6378;

    /* Redis authentication we'll use */
    private $auth;

    /* Redis server version */
    protected $version;
    protected string $valkey_version;
    protected bool $is_keydb;
    protected bool $is_valkey;

    private static bool $colorize = false;

    private static $BOLD_ON = "\033[1m";
    private static $BOLD_OFF = "\033[0m";

    private static $BLACK = "\033[0;30m";
    private static $DARKGRAY = "\033[1;30m";
    private static $BLUE = "\033[0;34m";
    private static $PURPLE = "\033[0;35m";
    private static $GREEN = "\033[0;32m";
    private static $YELLOW = "\033[0;33m";
    private static $RED = "\033[0;31m";

    public static array $errors = [];
    public static array $warnings = [];
    private static array $failed_tests = [];

    public function __construct(string $host, ?int $port, $auth, ?int $tls_port = 6378) {
        $this->host = $host;
        $this->port = $port;
        $this->auth = $auth;
        $this->tls_port = $tls_port;
    }

    public function getHost() { return $this->host; }
    public function getPort() { return $this->port; }
    public function getTlsPort() { return $this->tls_port; }
    public function getAuth() { return $this->auth; }

    /* Override these independently: cluster and sentinel tests also use
     * standalone clients for administrative commands. */
    protected function getRedisClass() { return Redis::class; }
    protected function getRedisClusterClass() { return RedisCluster::class; }
    protected function getRedisSentinelClass() { return RedisSentinel::class; }

    /* Test classes that can query INFO may override this. */
    public function getServerInfo() {
        return NULL;
    }

    /* Return a human-readable server/version string when INFO identifies it. */
    public function getServerVersion() {
        $info = $this->getServerInfo();

        if ( ! is_array($info))
            return NULL;

        if (isset($info['dragonfly_version']))
            return 'Dragonfly version ' . $info['dragonfly_version'];

        if (($info['server_name'] ?? NULL) === 'valkey' ||
            isset($info['valkey_version']))
        {
            $version = $info['valkey_version'] ?? $info['redis_version'] ?? NULL;
            return $version === NULL ? 'Valkey' : 'Valkey version ' . $version;
        }

        if (isset($info['redis_version']))
            return 'Redis version ' . $info['redis_version'];

        return NULL;
    }

    public static function errorMessage(string $fmt, ...$args) {
        $msg = vsprintf($fmt . "\n", $args);

        if (defined('STDERR')) {
            fwrite(STDERR, $msg);
        } else {
            echo $msg;
        }
    }

    public static function make_bold(string $msg) {
        return self::$colorize ? self::$BOLD_ON . $msg . self::$BOLD_OFF : $msg;
    }

    public static function make_success(string $msg) {
        return self::$colorize ? self::$GREEN . $msg . self::$BOLD_OFF : $msg;
    }

    public static function make_fail(string $msg) {
        return self::$colorize ? self::$RED . $msg . self::$BOLD_OFF : $msg;
    }

    public static function make_warning(string $msg) {
        return self::$colorize ? self::$YELLOW . $msg . self::$BOLD_OFF : $msg;
    }

    protected function printArg($v) {
        if (is_null($v))
            return '(null)';
        else if ($v === false || $v === true)
            return $v ? '(true)' : '(false)';
        else if (is_string($v))
            return "'$v'";
        else
            return print_r($v, true);
    }

    protected function findTestFunction($bt) {
        $i = 0;
        while (isset($bt[$i])) {
            if (substr($bt[$i]['function'], 0, 4) == 'test')
                return $bt[$i]['function'];
            $i++;
        }
        return NULL;
    }

    protected function assertionTrace(?string $fmt = NULL, ...$args) {
        $prefix = 'Assertion failed:';

        $lines = [];

        $bt = debug_backtrace(DEBUG_BACKTRACE_IGNORE_ARGS);

        $msg = $args ? vsprintf($fmt, $args) : $fmt;

        $fn = $this->findTestFunction($bt) ?? '(unknown test)';
        $lines []= sprintf("%s %s - %s", $prefix, self::make_bold($fn),
                           $msg ? $msg : '(no message)');

        array_shift($bt);

        for ($i = 0; $i < count($bt); $i++) {
            $file = $bt[$i]['file'] ?? '[internal]';
            $line = $bt[$i]['line'] ?? 0;
            $fn   = $bt[$i+1]['function'] ?? $bt[$i]['function'];

            $lines []= sprintf("%s %s:%d (%s)%s",
                str_repeat(' ', strlen($prefix)), $file, $line,
                $fn, $msg ? " $msg" : '');

            if (substr($fn, 0, 4) == 'test')
                break;
        }

        return implode("\n", $lines) . "\n";
    }

    protected function assert($fmt, ...$args): void {
        throw new TestFailedException($this->assertionTrace($fmt, ...$args));
    }

    protected function assertKeyEquals($expected, $key, $redis = NULL): void {
        $actual = ($redis ??= $this->redis)->get($key);
        if ($actual === $expected)
            return;

        $this->assert("%s !== %s", $this->printArg($actual),
                      $this->printArg($expected));
    }

    protected function assertKeyEqualsWeak($expected, $key, $redis = NULL): void {
        $actual = ($redis ??= $this->redis)->get($key);
        if ($actual == $expected)
            return;

        $this->assert("%s != %s", $this->printArg($actual),
                      $this->printArg($expected));
    }

    protected function assertKeyExists($key, $redis = NULL): void {
        if (($redis ??= $this->redis)->exists($key))
            return;

        $this->assert("Key '%s' does not exist.", $key);
    }

    protected function assertKeyMissing($key, $redis = NULL): void {
        if ( ! ($redis ??= $this->redis)->exists($key))
            return;

        $this->assert("Key '%s' exists but shouldn't.", $key);
    }

    protected function assertTrue($value): void {
        if ($value === true)
            return;

        $this->assert("%s !== %s", $this->printArg($value),
                      $this->printArg(true));
    }

    protected function assertFalse($value): void {
        if ($value === false)
            return;

        $this->assert("%s !== %s", $this->printArg($value),
                      $this->printArg(false));
    }

    protected function assertNull($value): void {
        if ($value === NULL)
            return;

        $this->assert("%s !== %s", $this->printArg($value),
                      $this->printArg(NULL));
    }

    protected function assertInArray($ele, $arr, ?callable $cb = NULL): void {
        $this->assertIsArray($arr);

        $key = array_search($ele, $arr);

        if ($key !== false && ($cb === NULL || $cb($arr[$key])))
            return;

        $this->assert("%s %s %s", $this->printArg($ele),
                      $key === false ? 'missing from' : 'is invalid in',
                      $this->printArg($arr));
    }

    protected function assertIsString($v): void {
        if (is_string($v))
            return;

        $this->assert("%s is not a string", $this->printArg($v));
    }

    protected function assertIsBool($v): void {
        if (is_bool($v))
            return;

        $this->assert("%s is not a boolean", $this->printArg($v));
    }

    protected function assertIsInt($v): void {
        if (is_int($v))
            return;

        $this->assert("%s is not an integer", $this->printArg($v));
    }

    protected function assertIsFloat($v): void {
        if (is_float($v))
            return;

        $this->assert("%s is not a float", $this->printArg($v));
    }

    protected function assertIsObject($v, ?string $type = NULL): void {
        if ( ! is_object($v)) {
            $this->assert("%s is not an object", $this->printArg($v));
        } else if ( $type !== NULL && !($v InstanceOf $type)) {
            $this->assert("%s is not an instance of %s",
                          $this->printArg($v), $type);
        }
    }

    protected function assertSameType($expected, $actual): void {
        if (gettype($expected) === gettype($actual))
            return;

        $this->assert("%s is not the same type as %s",
                      $this->printArg($actual),
                      $this->printArg($expected));
    }

    protected function assertIsArray($v, ?int $size = null): void {
        if ( ! is_array($v)) {
            $this->assert("%s is not an array", $this->printArg($v));
        }

        if ( ! is_null($size) && count($v) != $size) {
            $this->assert("Array size %d != %d", count($v), $size);
        }
    }

    protected function assertArrayKey($arr, $key, ?callable $cb = NULL): void {
        $this->assertIsArray($arr);

        if ( ! is_int($key) && ! is_string($key)) {
            $this->assert("%s is not an array key", $this->printArg($key));
        }

        if (($exists = array_key_exists($key, $arr)) && ($cb === NULL || $cb($arr[$key])))
            return;

        if ($exists) {
            $msg = sprintf("%s is invalid in %s", $this->printArg($arr[$key]),
                                                  $this->printArg($arr));
        } else {
            $msg = sprintf("%s is not a key in %s", $this->printArg($key),
                                                    $this->printArg($arr));
        }

        $this->assert('%s', $msg);
    }

    protected function assertArrayKeyEquals($arr, $key, $value): void {
        $this->assertArrayKey($arr, $key);

        if ($arr[$key] !== $value) {
            $this->assert(
                "Value %s !== %s for key %s in %s",
                $this->printArg($arr[$key]), $this->printArg($value),
                $this->printArg($key), $this->printArg($arr));
        }
    }

    protected function assertValidate($val, callable $cb): void {
        if ($cb($val))
            return;

        $this->assert("%s is invalid.", $this->printArg($val));
    }

    protected function assertNotSerializable(object $object): void {
        $this->assertThrowsMatch($object, function ($object) {
            serialize($object);
        });

        /* Before PHP 8.1, only the C: format reaches the handler that throws */
        $format = PHP_VERSION_ID < 80100 ? 'C' : 'O';

        $this->assertThrowsMatch(get_class($object), function ($class) use ($format) {
            unserialize($format . ':' . strlen($class) . ':"' . $class . '":0:{}');
        });
    }

    protected function assertThrowsMatch($arg, callable $cb, $regex = NULL): void {
        $threw = $match = false;

        try {
            $cb($arg);
        } catch (TestFailedException | TestSkippedException | ErrorException $ex) {
            throw $ex;
        } catch (Exception $ex) {
            $threw = true;
            $match = !$regex || preg_match($regex, $ex->getMessage());
        }

        if ($threw && $match)
            return;

        $ex = !$threw ? 'no exception' : "no match '$regex'";

        $this->assert("[$ex]");
    }

    protected function assertLTE($maximum, $value): void {
        if ($value <= $maximum)
            return;

        $this->assert("%s > %s", $this->printArg($value), $this->printArg($maximum));
    }

    protected function assertLT($maximum, $value): void {
        if ($value < $maximum)
            return;

        $this->assert("%s >= %s", $this->printArg($value), $this->printArg($maximum));
    }

    protected function assertGT($minimum, $value): void {
        if ($value > $minimum)
            return;

        $this->assert("%s <= %s", $this->printArg($value), $this->printArg($minimum));
    }

    protected function assertGTE($minimum, $value): void {
        if ($value >= $minimum)
            return;

        $this->assert("%s < %s", $this->printArg($value), $this->printArg($minimum));
    }

    protected function externalCmdFailure($cmd, $output, $msg = NULL, $exit_code = NULL) {
        $bt = debug_backtrace(false);

        $lines[] = sprintf("Assertion failed: %s:%d (%s)",
                           $bt[0]['file'], $bt[0]['line'],
                           self::make_bold($bt[0]['function']));


        if ($msg)
            $lines[] = sprintf("         Message: %s", $msg);
        if ($exit_code !== NULL)
            $lines[] = sprintf("       Exit code: %d", $exit_code);
        $lines[] = sprintf(    "         Command: %s", $cmd);
        if ($output)
            $lines[] = sprintf("          Output: %s", $output);

        throw new TestFailedException(implode("\n", $lines) . "\n");
    }

    protected function assertBetween($value, $min, $max, bool $exclusive = false): void {
        if ($min > $max)
            [$max, $min] = [$min, $max];

        if ($exclusive) {
            if ($value > $min && $value < $max)
                return;
        } else {
            if ($value >= $min && $value <= $max)
                return;
        }

        $this->assert("%s not between %s and %s",
            $this->printArg($value), $this->printArg($min), $this->printArg($max));
    }

    /* Compare arrays without regard to order. */
    protected function assertEqualsCanonicalizing($expected, $actual, $keep_keys = false): void {
        if ($expected InstanceOf Traversable)
            $expected = iterator_to_array($expected);

        if ($actual InstanceOf Traversable)
            $actual = iterator_to_array($actual);

        $this->assertIsArray($expected);
        $this->assertIsArray($actual);

        if ($keep_keys) {
            ksort($expected);
            ksort($actual);
        } else {
            sort($expected);
            sort($actual);
        }

        if ($expected === $actual)
            return;

        $this->assert("%s !== %s",
                      $this->printArg($actual),
                      $this->printArg($expected));
    }

    protected function assertEqualsWeak($expected, $actual): void {
        if ($expected == $actual)
            return;

        $this->assert("%s != %s", $this->printArg($actual),
                      $this->printArg($expected));
    }

    protected function assertEquals($expected, $actual, ?string $context = NULL): void {
        if ($expected === $actual)
            return;

        $context = $context === NULL ? '' : " ({$context})";

        $this->assert("%s !== %s%s", $this->printArg($actual),
                      $this->printArg($expected), $context);
    }

    public function assertNotEquals($wrong_value, $test_value): void {
        if ($wrong_value !== $test_value)
            return;

        $this->assert("%s === %s", $this->printArg($wrong_value),
                      $this->printArg($test_value));
    }

    protected function assertStringContains(string $needle, $haystack): void {
        if ( ! is_string($haystack)) {
            $this->assert("%s is not a string", $this->printArg($haystack));
        }

        if (strstr($haystack, $needle) !== false)
            return;

        $this->assert("'%s' not found in '%s'", $needle, $haystack);
    }

    protected function assertPatternMatch(string $pattern, $value): void {
        $this->assertIsString($value);

        if (preg_match($pattern, $value))
            return;

        $this->assert("'%s' doesn't match '%s'", $value,
                      $pattern);
    }

    protected function markTestSkipped(string $msg = '') {
        $bt = debug_backtrace(false);

        self::$warnings []= sprintf("Skipped test: %s:%d (%s) %s\n",
                                    $bt[0]["file"], $bt[0]["line"],
                                    $bt[1]["function"], $msg);

        throw new TestSkippedException($msg);
    }

    private static function normalizeTestFilters($limit): ?array {
        if ( ! $limit)
            return NULL;

        if ( ! is_array($limit))
            $limit = [$limit];

        $result = [
            'include' => [],
            'exclude' => [],
        ];

        foreach ($limit as $tests) {
            foreach (explode(',', $tests) as $test) {
                $test = strtolower(trim($test));

                if ($test === '')
                    continue;

                $type = $test[0] === '!' ? 'exclude' : 'include';
                if ($type === 'exclude')
                    $test = trim(substr($test, 1));

                if ($test !== '')
                    $result[$type][$test] = true;
            }
        }

        if ( ! $result['include'] && ! $result['exclude'])
            return NULL;

        return [
            'include' => array_keys($result['include']),
            'exclude' => array_keys($result['exclude']),
        ];
    }

    private static function testMatches(string $name, ?array $filters): bool {
        if ($filters === NULL)
            return true;

        $name = strtolower($name);

        foreach ($filters['exclude'] as $filter) {
            if (strstr($name, $filter) !== false)
                return false;
        }

        if ( ! $filters['include'])
            return true;

        foreach ($filters['include'] as $filter) {
            if (strstr($name, $filter) !== false)
                return true;
        }

        return false;
    }

    private static function getMaxTestLen(array $methods, ?array $filters): int {
        $result = 0;

        foreach ($methods as $obj_method) {
            $name = $obj_method->name;

            if (substr($name, 0, 4) != 'test')
                continue;
            if ( ! self::testMatches($name, $filters))
                continue;

            if (strlen($name) > $result) {
                $result = strlen($name);
            }
        }

        return $result;
    }

    private static function findFile($path, $file) {
        $files = glob($path . '/*', GLOB_NOSORT);

        foreach ($files as $test) {
            $test = basename($test);
            if (strcasecmp($file, $test) == 0)
                return $path . '/' . $test;
        }

        return NULL;
    }

    /* Small helper method that tries to load a custom test case class */
    public static function loadTestClass($class) {
        $filename = "{$class}.php";

        if (($sp = getenv('PHPREDIS_TEST_SEARCH_PATH'))) {
            $fullname = self::findFile($sp, $filename);
        } else {
            $fullname = self::findFile(__DIR__, $filename);
        }

        if ( ! $fullname)
            die("Fatal:  Couldn't find $filename\n");

        require_once($fullname);

        if ( ! class_exists($class))
            die("Fatal:  Loaded '$filename' but didn't find class '$class'\n");

        /* Loaded the file and found the class, return it */
        return $class;
    }

    /* Flag colorization */
    public static function flagColorization(bool $override) {
        self::$colorize = $override && function_exists('posix_isatty') &&
                          defined('STDOUT') && posix_isatty(STDOUT);
    }

    public static function getFailedTests(): array {
        return array_keys(self::$failed_tests);
    }

    public static function run($class_name, $limit = NULL,
                               ?string $host = NULL, ?int $port = NULL,
                               $auth = NULL, ?int $tls_port = 6378)
    {
        $filters = self::normalizeTestFilters($limit);

        $rc = new ReflectionClass($class_name);
        $methods = $rc->GetMethods(ReflectionMethod::IS_PUBLIC);

        $max_test_len = self::getMaxTestLen($methods, $filters);

        foreach($methods as $m) {
            $name = $m->name;
            if (substr($name, 0, 4) !== 'test')
                continue;

            /* Skip tests that don't satisfy the requested filters */
            if ( ! self::testMatches($name, $filters)) {
                continue;
            }

            $padded_name = str_pad($name, $max_test_len + 1);
            echo self::make_bold($padded_name);

            $failed = false;

            set_error_handler(function ($severity, $message, $file, $line) {
                if ( ! (error_reporting() & $severity))
                    return false;

                throw new ErrorException($message, 0, $severity, $file, $line);
            });

            try {
                $rt = new $class_name($host, $port, $auth, $tls_port);
                $rt->setUp();
                $rt->$name();

                $result = self::make_success('PASSED');
            } catch (Throwable $e) {
                /* We may have simply skipped the test */
                if ($e instanceof TestSkippedException) {
                    $result = self::make_warning('SKIPPED');
                } else if ($e instanceof TestFailedException) {
                    $class_name::$errors[] = $e->getMessage();
                    $result = self::make_fail('FAILED');
                    $failed = true;
                } else {
                    $type = $e instanceof Exception ? 'exception' : get_class($e);
                    $class_name::$errors[] = sprintf("Uncaught %s '%s' (%s) at %s:%d\n",
                        $type, $e->getMessage(), $name, $e->getFile(), $e->getLine());
                    $result = self::make_fail('FAILED');
                    $failed = true;
                }
            } finally {
                restore_error_handler();
            }

            if ($failed)
                self::$failed_tests[$name] = true;

            echo "[" . $result . "]\n";
        }
        echo "\n";
        echo implode('', $class_name::$warnings) . "\n";

        if (empty($class_name::$errors)) {
            echo "All tests passed. \o/\n";
            return 0;
        }

        echo implode('', $class_name::$errors);
        return 1;
    }
}

?>
