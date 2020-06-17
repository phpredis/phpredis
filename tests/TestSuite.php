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

    /**
     * Returns the fully qualified host path,
     * which may be used directly for php.ini parameters like session.save_path
     *
     * @return null|string
     */
    protected function getFullHostPath()
    {
        return $this->str_host
            ? 'tcp://' . $this->str_host . ':' . $this->i_port
            : null;
    }

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

    protected function assertFalse($bool) {
        if(!$bool)
            return true;

        $bt = debug_backtrace(false);
        self::$errors []= sprintf("Assertion failed: %s:%d (%s)\n",
            $bt[0]["file"], $bt[0]["line"], $bt[1]["function"]);

        return false;
    }

    protected function assertTrue($bool) {
        if($bool)
            return true;

        $bt = debug_backtrace(false);
        self::$errors []= sprintf("Assertion failed: %s:%d (%s)\n",
            $bt[0]["file"], $bt[0]["line"], $bt[1]["function"]);

        return false;
    }

    protected function assertInArray($ele, $arr, $cb = NULL) {
        if ($cb && !is_callable($cb))
            die("Fatal:  assertInArray callback must be callable!\n");

        if (($in = in_array($ele, $arr)) && (!$cb || $cb($arr[array_search($ele, $arr)])))
            return true;


        $bt = debug_backtrace(false);
        $ex = $in ? 'validation' : 'missing';
        self::$errors []= sprintf("Assertion failed: %s:%d (%s) [%s '%s']\n",
            $bt[0]["file"], $bt[0]["line"], $bt[1]["function"], $ex, $ele);

        return false;
    }

    protected function assertArrayKey($arr, $key, $cb = NULL) {
        if ($cb && !is_callable($cb))
            die("Fatal:  assertArrayKey callback must be callable\n");

        if (($exists = isset($arr[$key])) && (!$cb || $cb($arr[$key])))
            return true;

        $bt = debug_backtrace(false);
        $ex = $exists ? 'validation' : 'missing';
        self::$errors []= sprintf("Assertion failed: %s:%d (%s) [%s '%s']\n",
            $bt[0]["file"], $bt[0]["line"], $bt[1]["function"], $ex, $key);

        return false;
    }

    protected function assertValidate($val, $cb) {
        if ( ! is_callable($cb))
            die("Fatal:  Callable assertValidate callback required\n");

        if ($cb($val))
            return true;

        $bt = debug_backtrace(false);
        self::$errors []= sprintf("Assertion failed: %s:%d (%s)\n",
            $bt[0]["file"], $bt[0]["line"], $bt[1]["function"]);

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

        $bt = debug_backtrace(false);
        $ex = !$threw ? 'no exception' : "no match '$regex'";
        self::$errors []= sprintf("Assertion failed: %s:%d (%s) [%s]\n",
            $bt[0]["file"], $bt[0]["line"], $bt[1]["function"], $ex);

        return false;
    }

    protected function assertLess($a, $b) {
        if($a < $b)
            return;

        $bt = debug_backtrace(false);
        self::$errors[] = sprintf("Assertion failed (%s >= %s): %s: %d (%s\n",
            print_r($a, true), print_r($b, true),
            $bt[0]["file"], $bt[0]["line"], $bt[1]["function"]);
    }

    protected function assertEquals($a, $b) {
        if($a === $b)
            return;

        $bt = debug_backtrace(false);
        self::$errors []= sprintf("Assertion failed (%s !== %s): %s:%d (%s)\n",
            print_r($a, true), print_r($b, true),
            $bt[0]["file"], $bt[0]["line"], $bt[1]["function"]);
    }

    protected function assertPatternMatch($str_test, $str_regex) {
        if (preg_match($str_regex, $str_test))
            return;

        $bt = debug_backtrace(false);
        self::$errors []= sprintf("Assertion failed ('%s' doesnt match '%s'): %s:%d (%s)\n",
            $str_test, $str_regex, $bt[0]["file"], $bt[0]["line"], $bt[1]["function"]);
    }

    protected function markTestSkipped($msg='') {
        $bt = debug_backtrace(false);
        self::$warnings []= sprintf("Skipped test: %s:%d (%s) %s\n",
            $bt[0]["file"], $bt[0]["line"], $bt[1]["function"], $msg);

        throw new TestSkippedException($msg);
    }

    private static function getMaxTestLen($arr_methods, $str_limit) {
        $i_result = 0;

        $str_limit = strtolower($str_limit);
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
