<?php defined('PHPREDIS_TESTRUN') or die("Use TestRedis.php to run tests!\n");

/* A specific exception for when we skip a test */
class TestSkippedException extends Exception {}

// phpunit is such a pain to install, we're going with pure-PHP here.
class TestSuite
{
    /* Host and port the unit tests will use */
    private string $host;
    private ?int $port = 6379;

    /* Redis authentication we'll use */
    private $auth;

    /* Redis server version */
    protected $version;
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

    public function __construct(string $host, ?int $port, $auth) {
        $this->host = $host;
        $this->port = $port;
        $this->auth = $auth;
    }

    public function getHost() { return $this->host; }
    public function getPort() { return $this->port; }
    public function getAuth() { return $this->auth; }

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

        $bt = debug_backtrace();

        $msg = $fmt ? vsprintf($fmt, $args) : NULL;

        $fn = $this->findTestFunction($bt);
        $lines []= sprintf("%s %s - %s", $prefix, self::make_bold($fn),
                           $msg ? $msg : '(no message)');

        array_shift($bt);

        for ($i = 0; $i < count($bt); $i++) {
            $file = $bt[$i]['file'];
            $line = $bt[$i]['line'];
            $fn   = $bt[$i+1]['function'] ?? $bt[$i]['function'];

            $lines []= sprintf("%s %s:%d (%s)%s",
                str_repeat(' ', strlen($prefix)), $file, $line,
                $fn, $msg ? " $msg" : '');

            if (substr($fn, 0, 4) == 'test')
                break;
        }

        return implode("\n", $lines) . "\n";
    }

    protected function assert($fmt, ...$args) {
        self::$errors []= $this->assertionTrace($fmt, ...$args);
    }

    protected function assertKeyEquals($expected, $key, $redis = NULL): bool {
        $actual = ($redis ??= $this->redis)->get($key);
        if ($actual === $expected)
            return true;

        self::$errors []= $this->assertionTrace("%s !== %s", $this->printArg($actual),
                                                $this->printArg($expected));

        return false;
    }

    protected function assertKeyEqualsWeak($expected, $key, $redis = NULL): bool {
        $actual = ($redis ??= $this->redis)->get($key);
        if ($actual == $expected)
            return true;

        self::$errors []= $this->assertionTrace("%s != %s", $this->printArg($actual),
                                                $this->printArg($expected));

        return false;
    }

    protected function assertKeyExists($key, $redis = NULL): bool {
        if (($redis ??= $this->redis)->exists($key))
            return true;

        self::$errors []= $this->assertionTrace("Key '%s' does not exist.", $key);

        return false;
    }

    protected function assertKeyMissing($key, $redis = NULL): bool {
        if ( ! ($redis ??= $this->redis)->exists($key))
            return true;

        self::$errors []= $this->assertionTrace("Key '%s' exists but shouldn't.", $key);

        return false;
    }

    protected function assertTrue($value): bool {
        if ($value === true)
            return true;

        self::$errors []= $this->assertionTrace("%s !== %s", $this->printArg($value),
                                                             $this->printArg(true));

        return false;
    }

    protected function assertFalse($value): bool {
        if ($value === false)
            return true;

        self::$errors []= $this->assertionTrace("%s !== %s", $this->printArg($value),
                                                             $this->printArg(false));

        return false;
    }

    protected function assertNull($value): bool {
        if ($value === NULL)
            return true;

        self::$errors []= $this->assertionTrace("%s !== %s", $this->printArg($value),
                                                             $this->printArg(NULL));

        return false;
    }

    protected function assertInArray($ele, $arr, ?callable $cb = NULL): bool {
        $cb ??= function ($v) { return true; };

        $key = array_search($ele, $arr);

        if ($key !== false && ($valid = $cb($ele)))
            return true;

        self::$errors []= $this->assertionTrace("%s %s %s", $this->printArg($ele),
                                                $key === false ? 'missing from' : 'is invalid in',
                                                $this->printArg($arr));

        return false;
    }

    protected function assertIsString($v): bool {
        if (is_string($v))
            return true;

        self::$errors []= $this->assertionTrace("%s is not a string", $this->printArg($v));

        return false;
    }

    protected function assertIsBool($v): bool {
        if (is_bool($v))
            return true;

        self::$errors []= $this->assertionTrace("%s is not a boolean", $this->printArg($v));

        return false;
    }

    protected function assertIsInt($v): bool {
        if (is_int($v))
            return true;

        self::$errors []= $this->assertionTrace("%s is not an integer", $this->printArg($v));

        return false;
    }

    protected function assertIsFloat($v): bool {
        if (is_float($v))
            return true;

        self::$errors []= $this->assertionTrace("%s is not a float", $this->printArg($v));

        return false;
    }

    protected function assertIsObject($v, ?string $type = NULL): bool {
        if ( ! is_object($v)) {
            self::$errors []= $this->assertionTrace("%s is not an object", $this->printArg($v));
            return false;
        } else if ( $type !== NULL && !($v InstanceOf $type)) {
            self::$errors []= $this->assertionTrace("%s is not an instance of %s",
                                                    $this->printArg($v), $type);
            return false;
        }

        return true;
    }

    protected function assertSameType($expected, $actual): bool {
        if (gettype($expected) === gettype($actual))
            return true;

        self::$errors []= $this->assertionTrace("%s is not the same type as %s",
                                                $this->printArg($actual),
                                                $this->printArg($expected));

        return false;
    }

    protected function assertIsArray($v, ?int $size = null): bool {
        if ( ! is_array($v)) {
            self::$errors []= $this->assertionTrace("%s is not an array", $this->printArg($v));
            return false;
        }

        if ( ! is_null($size) && count($v) != $size) {
            self::$errors []= $this->assertionTrace("Array size %d != %d", count($v), $size);
            return false;
        }

        return true;
    }

    protected function assertArrayKey($arr, $key, ?callable $cb = NULL): bool {
        $cb ??= function ($v) { return true; };

        if (($exists = isset($arr[$key])) && $cb($arr[$key]))
            return true;


        if ($exists) {
            $msg = sprintf("%s is invalid in %s", $this->printArg($arr[$key]),
                                                  $this->printArg($arr));
        } else {
            $msg = sprintf("%s is not a key in %s", $this->printArg($key),
                                                    $this->printArg($arr));
        }

        self::$errors []= $this->assertionTrace($msg);

        return false;
    }

    protected function assertArrayKeyEquals($arr, $key, $value): bool {
        if ( ! isset($arr[$key])) {
            self::$errors []= $this->assertionTrace(
                "Key '%s' not found in %s", $key, $this->printArg($arr));
            return false;
        }

        if ($arr[$key] !== $value) {
            self::$errors []= $this->assertionTrace(
                "Value '%s' != '%s' for key '%s' in %s",
                $arr[$key], $value, $key, $this->printArg($arr));
            return false;
        }

        return true;
    }

    protected function assertValidate($val, callable $cb): bool {
        if ($cb($val))
            return true;

        self::$errors []= $this->assertionTrace("%s is invalid.", $this->printArg($val));

        return false;
    }

    protected function assertThrowsMatch($arg, callable $cb, $regex = NULL): bool {
        $threw = $match = false;

        try {
            $cb($arg);
        } catch (Exception $ex) {
            $threw = true;
            $match = !$regex || preg_match($regex, $ex->getMessage());
        }

        if ($threw && $match)
            return true;

        $ex = !$threw ? 'no exception' : "no match '$regex'";

        self::$errors []= $this->assertionTrace("[$ex]");

        return false;
    }

    protected function assertLTE($maximum, $value): bool {
        if ($value <= $maximum)
            return true;

        self::$errors []= $this->assertionTrace("%s > %s", $value, $maximum);

        return false;
    }

    protected function assertLT($minimum, $value): bool {
        if ($value < $minimum)
            return true;

        self::$errors []= $this->assertionTrace("%s >= %s", $value, $minimum);

        return false;
    }

    protected function assertGT($maximum, $value): bool {
        if ($value > $maximum)
            return true;

        self::$errors [] = $this->assertionTrace("%s <= %s", $maximum, $value);

        return false;
    }

    protected function assertGTE($minimum, $value): bool {
        if ($value >= $minimum)
            return true;

        self::$errors [] = $this->assertionTrace("%s < %s", $minimum, $value);

        return false;
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

        self::$errors[] = implode("\n", $lines) . "\n";
    }

    protected function assertBetween($value, $min, $max, bool $exclusive = false): bool {
        if ($min > $max)
            [$max, $min] = [$min, $max];

        if ($exclusive) {
            if ($value > $min && $value < $max)
                return true;
        } else {
            if ($value >= $min && $value <= $max)
                return true;
        }

        self::$errors []= $this->assertionTrace(sprintf("'%s' not between '%s' and '%s'",
                                                        $value, $min, $max));

        return false;
    }

    /* Replica of PHPUnit's assertion.  Basically are two arrays the same without
   '   respect to order. */
    protected function assertEqualsCanonicalizing($expected, $actual, $keep_keys = false): bool {
        if ($expected InstanceOf Traversable)
            $expected = iterator_to_array($expected);

        if ($actual InstanceOf Traversable)
            $actual = iterator_to_array($actual);

        if ($keep_keys) {
            asort($expected);
            asort($actual);
        } else {
            sort($expected);
            sort($actual);
        }

        if ($expected === $actual)
            return true;

        self::$errors []= $this->assertionTrace("%s !== %s",
                                                $this->printArg($actual),
                                                $this->printArg($expected));

        return false;
    }

    protected function assertEqualsWeak($expected, $actual): bool {
        if ($expected == $actual)
            return true;

        self::$errors []= $this->assertionTrace("%s != %s", $this->printArg($actual),
                                                $this->printArg($expected));

        return false;
    }

    protected function assertEquals($expected, $actual): bool {
        if ($expected === $actual)
            return true;

        self::$errors[] = $this->assertionTrace("%s !== %s", $this->printArg($actual),
                                                $this->printArg($expected));

        return false;
    }

    public function assertNotEquals($wrong_value, $test_value): bool {
        if ($wrong_value !== $test_value)
            return true;

        self::$errors []= $this->assertionTrace("%s === %s", $this->printArg($wrong_value),
                                                $this->printArg($test_value));

        return false;
    }

    protected function assertStringContains(string $needle, $haystack): bool {
        if ( ! is_string($haystack)) {
            self::$errors []= $this->assertionTrace("'%s' is not a string", $this->printArg($haystack));
            return false;
        }

        if (strstr($haystack, $needle) !== false)
            return true;

        self::$errors []= $this->assertionTrace("'%s' not found in '%s'", $needle, $haystack);
    }

    protected function assertPatternMatch(string $pattern, string $value): bool {
        if (preg_match($pattern, $value))
            return true;

        self::$errors []= $this->assertionTrace("'%s' doesnt match '%s'", $value,
                                                $pattern);

        return false;
    }

    protected function markTestSkipped(string $msg = '') {
        $bt = debug_backtrace(false);

        self::$warnings []= sprintf("Skipped test: %s:%d (%s) %s\n",
                                    $bt[0]["file"], $bt[0]["line"],
                                    $bt[1]["function"], $msg);

        throw new TestSkippedException($msg);
    }

    private static function getMaxTestLen(array $methods, ?string $limit): int {
        $result = 0;

        foreach ($methods as $obj_method) {
            $name = strtolower($obj_method->name);

            if (substr($name, 0, 4) != 'test')
                continue;
            if ($limit && !strstr($name, $limit))
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

    public static function run($class_name, ?string $limit = NULL,
                               ?string $host = NULL, ?int $port = NULL,
                               $auth = NULL)
    {
        if ($limit)
            $limit = strtolower($limit);

        $rc = new ReflectionClass($class_name);
        $methods = $rc->GetMethods(ReflectionMethod::IS_PUBLIC);

        $max_test_len = self::getMaxTestLen($methods, $limit);

        foreach($methods as $m) {
            $name = $m->name;
            if (substr($name, 0, 4) !== 'test')
                continue;

            /* If we're trying to limit to a specific test and can't match the
             * substring, skip */
            if ($limit && stristr($name, $limit) === false) {
                continue;
            }

            $padded_name = str_pad($name, $max_test_len + 1);
            echo self::make_bold($padded_name);

            $count = count($class_name::$errors);
            $rt = new $class_name($host, $port, $auth);

            try {
                $rt->setUp();
                $rt->$name();

                if ($count === count($class_name::$errors)) {
                    $result = self::make_success('PASSED');
                } else {
                    $result = self::make_fail('FAILED');
                }
            } catch (Exception $e) {
                /* We may have simply skipped the test */
                if ($e instanceof TestSkippedException) {
                    $result = self::make_warning('SKIPPED');
                } else {
                    $class_name::$errors[] = "Uncaught exception '".$e->getMessage()."' ($name)\n";
                    $result = self::make_fail('FAILED');
                }
            }

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
