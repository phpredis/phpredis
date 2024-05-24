<?php defined('PHPREDIS_TESTRUN') or die("Use TestRedis.php to run tests!\n");

/* A specific exception for when we skip a test */
class TestSkippedException extends Exception {}

// phpunit is such a pain to install, we're going with pure-PHP here.
class TestSuite
{
    /* Host and port the unit tests will use */
    private $str_host;
    private $i_port = 6379;

    /* Redis authentication we'll use */
    private $auth;

    /* Redis server version */
    protected $version;
    protected $is_keydb;

    private static $_boo_colorize = false;

    private static $BOLD_ON = "\033[1m";
    private static $BOLD_OFF = "\033[0m";

    private static $BLACK = "\033[0;30m";
    private static $DARKGRAY = "\033[1;30m";
    private static $BLUE = "\033[0;34m";
    private static $PURPLE = "\033[0;35m";
    private static $GREEN = "\033[0;32m";
    private static $YELLOW = "\033[0;33m";
    private static $RED = "\033[0;31m";

    public static $errors = [];
    public static $warnings = [];

    public function __construct($str_host, $i_port, $auth) {
        $this->str_host = $str_host;
        $this->i_port = $i_port;
        $this->auth = $auth;
    }

    public function getHost() { return $this->str_host; }
    public function getPort() { return $this->i_port; }
    public function getAuth() { return $this->auth; }

    public static function make_bold($str_msg) {
        return self::$_boo_colorize
            ? self::$BOLD_ON . $str_msg . self::$BOLD_OFF
            : $str_msg;
    }

    public static function make_success($str_msg) {
        return self::$_boo_colorize
            ? self::$GREEN . $str_msg . self::$BOLD_OFF
            : $str_msg;
    }

    public static function make_fail($str_msg) {
        return self::$_boo_colorize
            ? self::$RED . $str_msg . self::$BOLD_OFF
            : $str_msg;
    }

    public static function make_warning($str_msg) {
        return self::$_boo_colorize
            ? self::$YELLOW . $str_msg . self::$BOLD_OFF
            : $str_msg;
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

    protected function assertFalse($bool) {
        if( ! $bool)
            return true;
        self::$errors []= $this->assertionTrace();

        return false;
    }

    protected function assertKeyExists($redis, $key) {
        if ($redis->exists($key))
            return true;

        self::$errors []= $this->assertionTrace("Key '%s' does not exist.", $key);

        return false;
    }

    protected function assertKeyMissing($redis, $key) {
        if ( ! $redis->exists($key))
            return true;

        self::$errors []= $this->assertionTrace("Key '%s' exists but shouldn't.", $key);

        return false;
    }

    protected function assertTrue($bool, $msg='') {
        if($bool)
            return true;

        self::$errors []= $this->assertionTrace($msg);

        return false;
    }

    protected function assertInArray($ele, $arr, ?callable $cb = NULL) {
        $cb ??= function ($v) { return true; };

        $key = array_search($ele, $arr);

        if ($key !== false && ($valid = $cb($ele)))
            return true;

        self::$errors []= $this->assertionTrace("%s %s %s", $this->printArg($ele),
                                                $key === false ? 'missing from' : 'is invalid in',
                                                $this->printArg($arr));

        return false;
    }

    protected function assertIsInt($v) {
        if (is_int($v))
            return true;

        self::$errors []= $this->assertionTrace("%s is not an integer", $this->printArg($v));

        return false;
    }

    protected function assertIsArray($v) {
        if (is_array($v))
            return true;

        self::$errors []= $this->assertionTrace("%s is not an array", $this->printArg($v));

        return false;
    }

    protected function assertArrayKey($arr, $key, callable $cb = NULL) {
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

    protected function assertValidate($val, $cb) {
        if ( ! is_callable($cb))
            die("Fatal:  Callable assertValidate callback required\n");

        if ($cb($val))
            return true;

        self::$errors []= $this->assertionTrace("%s is invalid.", $this->printArg($val));

        return false;
    }

    protected function assertThrowsMatch($arg, $cb, $regex = NULL) {
        $threw = $match = false;

        if ( ! is_callable($cb))
            die("Fatal:  Callable assertThrows callback required\n");

        try {
            $cb($arg);
        } catch (Exception $ex) {
            $threw = true;
            $match = !$regex || preg_match($regex, $ex->getMessage());
        }

        if ($threw && $match)
            return true;

//        $bt = debug_backtrace(false);
        $ex = !$threw ? 'no exception' : "no match '$regex'";

        self::$errors []= $this->assertionTrace("[$ex]");
//
        return false;
    }

    protected function assertLess($a, $b) {
        if($a < $b)
            return true;

        self::$errors []= $this->assertionTrace("%s >= %s", $a, $b);

        return false;
    }

    protected function assertMore($a, $b) {
        if($a > $b)
            return true;

        self::$errors [] = $this->assertionTrace("%s <= %s", $a, $b);

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

    protected function assertBetween($value, $min, $max, bool $exclusive = false) {
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

    /* Replica of PHPUnit's assertion.  Basically are two arrys the same without
   '   respect to order. */
    protected function assertEqualsCanonicalizing($expected, $actual, $keep_keys = false) {
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

    protected function assertEqualsWeak($expected, $actual) {
        if ($expected == $actual)
            return true;

        self::$errors []= $this->assertionTrace("%s != %s", $this->printArg($actual),
                                                $this->printArg($expected));

        return false;
    }

    protected function assertEquals($expected, $actual) {
        if($expected === $actual)
            return true;

        self::$errors[] = $this->assertionTrace("%s !== %s", $this->printArg($actual),
                                                $this->printArg($expected));

        return false;
    }

    public function assertNotEquals($a, $b) {
        if($a !== $b)
            return true;

        self::$errors []= $this->assertionTrace("%s === %s", $this->printArg($a),
                                                $this->printArg($b));

        return false;
    }

    protected function assertStringContains(string $needle, $haystack) {
        if ( ! is_string($haystack)) {
            self::$errors []= $this->assertionTrace("'%s' is not a string", $this->printArg($haystack));
            return false;
        }

        if (strstr($haystack, $needle) !== false)
            return true;

        self::$errors []= $this->assertionTrace("'%s' not found in '%s'", $needle, $haystack);
    }

    protected function assertPatternMatch($str_test, $str_regex) {
        if (preg_match($str_regex, $str_test))
            return true;

        self::$errors []= $this->assertionTrace("'%s' doesnt match '%s'",
                                                $str_test, $str_regex);

        return false;
    }

    protected function markTestSkipped($msg='') {
        $bt = debug_backtrace(false);
        self::$warnings []= sprintf("Skipped test: %s:%d (%s) %s\n",
                                    $bt[0]["file"], $bt[0]["line"],
                                    $bt[1]["function"], $msg);

        throw new TestSkippedException($msg);
    }

    private static function getMaxTestLen($arr_methods, $str_limit) {
        $i_result = 0;

        foreach ($arr_methods as $obj_method) {
            $str_name = strtolower($obj_method->name);

            if (substr($str_name, 0, 4) != 'test')
                continue;
            if ($str_limit && !strstr($str_name, $str_limit))
                continue;

            if (strlen($str_name) > $i_result) {
                $i_result = strlen($str_name);
            }
        }
        return $i_result;
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
    public static function flagColorization($boo_override) {
        self::$_boo_colorize = $boo_override && function_exists('posix_isatty') &&
            posix_isatty(STDOUT);
    }

    public static function run($className, $str_limit = NULL, $str_host = NULL, $i_port = NULL, $auth = NULL) {
        /* Lowercase our limit arg if we're passed one */
        $str_limit = $str_limit ? strtolower($str_limit) : $str_limit;

        $rc = new ReflectionClass($className);
        $methods = $rc->GetMethods(ReflectionMethod::IS_PUBLIC);

        $i_max_len = self::getMaxTestLen($methods, $str_limit);

        foreach($methods as $m) {
            $name = $m->name;
            if(substr($name, 0, 4) !== 'test')
                continue;

            /* If we're trying to limit to a specific test and can't match the
             * substring, skip */
            if ($str_limit && strstr(strtolower($name), $str_limit)===FALSE) {
                continue;
            }

            $str_out_name = str_pad($name, $i_max_len + 1);
            echo self::make_bold($str_out_name);

            $count = count($className::$errors);
            $rt = new $className($str_host, $i_port, $auth);

            try {
                $rt->setUp();
                $rt->$name();

                if ($count === count($className::$errors)) {
                    $str_msg = self::make_success('PASSED');
                } else {
                    $str_msg = self::make_fail('FAILED');
                }
                //echo ($count === count($className::$errors)) ? "." : "F";
            } catch (Exception $e) {
                /* We may have simply skipped the test */
                if ($e instanceof TestSkippedException) {
                    $str_msg = self::make_warning('SKIPPED');
                } else {
                    $className::$errors[] = "Uncaught exception '".$e->getMessage()."' ($name)\n";
                    $str_msg = self::make_fail('FAILED');
                }
            }

            echo "[" . $str_msg . "]\n";
        }
        echo "\n";
        echo implode('', $className::$warnings) . "\n";

        if(empty($className::$errors)) {
            echo "All tests passed. \o/\n";
            return 0;
        }

        echo implode('', $className::$errors);
        return 1;
    }
}

?>
