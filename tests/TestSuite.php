<?php defined('PHPREDIS_TESTRUN') or die("Use TestRedis.php to run tests!\n");

// phpunit is such a pain to install, we're going with pure-PHP here.
class TestSuite {
    /* Host the tests will use */
    private $str_host;

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

    public static $errors = array();
    public static $warnings = array();

    public function __construct($str_host) {
        $this->str_host = $str_host;
    }

    public function getHost() { return $this->str_host; }

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
        return $this->assertTrue(!$bool);
    }

    protected function assertTrue($bool) {
        if($bool)
            return true;

        $bt = debug_backtrace(false);
        self::$errors []= sprintf("Assertion failed: %s:%d (%s)\n",
            $bt[0]["file"], $bt[0]["line"], $bt[1]["function"]);

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

    protected function markTestSkipped($msg='') {
        $bt = debug_backtrace(false);
        self::$warnings []= sprintf("Skipped test: %s:%d (%s) %s\n",
            $bt[0]["file"], $bt[0]["line"], $bt[1]["function"], $msg);

        throw new Exception($msg);
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

    public static function run($className, $str_limit = NULL, $str_host = NULL) {
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
            $rt = new $className($str_host);

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
                if ($e instanceof RedisException) {
                    $className::$errors[] = "Uncaught exception '".$e->getMessage()."' ($name)\n";
                    $str_msg = self::make_fail('FAILED');
                } else {
                    $str_msg = self::make_warning('SKIPPED');
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
