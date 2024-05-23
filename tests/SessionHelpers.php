<?php

namespace SessionHelpers;

class PhpSpawner {
    protected static function appendPhpArgs(string $php): string {
        $modules   = shell_exec("$php --no-php-ini -m");

        /* Determine if we need to specifically add extensions */
        $extensions = array_filter(
            ['igbinary', 'msgpack', 'json', 'redis'],
            function ($module) use ($modules) {
                return strpos($modules, $module) === false;
            }
        );

        /* If any are needed add them to the command */
        if ($extensions) {
            $php .= ' --no-php-ini';
            foreach ($extensions as $extension) {
                /* We want to use the locally built redis extension */
                if ($extension == 'redis') {
                    $path = dirname(__DIR__) . '/modules/redis';
                    if (is_file("{$path}.so"))
                        $extension = $path;
                }

                $php .= " -dextension=$extension.so";
            }
        }

        return $php;
    }

    /**
     * Return command to launch PHP with built extension enabled
     * taking care of environment (TEST_PHP_EXECUTABLE and TEST_PHP_ARGS)
     *
     * @param string $script
     *
     * @return string
     */
    public static function cmd(string $script): string {
        static $cmd = NULL;

        if ( ! $cmd) {
            $cmd = getenv('TEST_PHP_EXECUTABLE') ?: PHP_BINARY;

            if ($test_args = getenv('TEST_PHP_ARGS')) {
                $cmd .= ' ' . $test_args;
            } else {
                $cmd = self::appendPhpArgs($cmd);
            }
        }

        return $cmd . ' ' . __DIR__ . '/' . $script . ' ';
    }
}

class Runner {
    const start_script = 'startSession.php';
    const regenerate_id_script = 'regenerateSessionId.php';
    const get_data_script = 'getSessionData.php';

    private $required = ['host', 'handler', 'id'];

    private $args = [
        'handler' => null,
        'save-path' => null,
        'id' => null,
        'sleep' => 0,
        'max-execution-time' => 300,
        'locking-enabled' => true,
        'lock-wait-time' => null,
        'lock-retries' => -1,
        'lock-expires' => 0,
        'data' => '',
        'lifetime' => 1440,
        'compression' => 'none',
    ];

    private $prefix = NULL;
    private $output_file;
    private $exit_code = -1;
    private $cmd = NULL;
    private $pid;
    private $output;

    public function __construct() {
        $this->args['id'] = $this->createId();
    }

    public function getExitCode(): int {
        return $this->exit_code;
    }

    public function getCmd(): ?string {
        return $this->cmd;
    }

    public function getId(): ?string {
        return $this->args['id'];
    }

    public function prefix(string $prefix): self {
        $this->prefix = $prefix;
        return $this;
    }

    public function getSessionKey(): string {
        return $this->prefix . $this->getId();
    }

    public function getSessionLockKey(): string {
        return $this->getSessionKey() . '_LOCK';
    }

    protected function set($setting, $v): self {
        $this->args[$setting] = $v;
        return $this;
    }

    public function handler(string $handler): self {
        return $this->set('handler', $handler);
    }

    public function savePath(string $path): self {
        return $this->set('save-path', $path);
    }

    public function id(string $id): self {
        return $this->set('id', $id);
    }

    public function sleep(int $sleep): self {
        return $this->set('sleep', $sleep);
    }

    public function maxExecutionTime(int $time): self {
        return $this->set('max-execution-time', $time);
    }

    public function lockingEnabled(bool $enabled): self {
        return $this->set('locking-enabled', $enabled);
    }

    public function lockWaitTime(int $time): self {
        return $this->set('lock-wait-time', $time);
    }

    public function lockRetries(int $retries): self {
        return $this->set('lock-retries', $retries);
    }

    public function lockExpires(int $expires): self {
        return $this->set('lock-expires', $expires);
    }

    public function data(string $data): self {
        return $this->set('data', $data);
    }

    public function lifetime(int $lifetime): self {
        return $this->set('lifetime', $lifetime);
    }

    public function compression(string $compression): self {
        return $this->set('compression', $compression);
    }

    protected function validateArgs(array $required) {
        foreach ($required as $req) {
            if ( ! isset($this->args[$req]) || $this->args[$req] === null)
                throw new \Exception("Command requires '$req' arg");
        }
    }

    private function createId(): string {
        if (function_exists('session_create_id'))
            return session_create_id();

        return uniqid();
    }

    private function getTmpFileName() {
        return '/tmp/sessiontmp.txt';
        return tempnam(sys_get_temp_dir(), 'session');
    }

    /*
     * @param $client Redis client
     * @param string $max_wait_sec
     *
     * Sometimes we want to block until a session lock has been detected
     * This is better and faster than arbitrarily sleeping.  If we don't
     * detect the session key within the specified maximum number of
     * seconds, the function returns failure.
     *
     * @return bool
     */
    public function waitForLockKey($redis, $max_wait_sec) {
        $now = microtime(true);

        do {
            if ($redis->exists($this->getSessionLockKey()))
                return true;
            usleep(10000);
        } while (microtime(true) <= $now + $max_wait_sec);

        return false;
    }

    private function appendCmdArgs(array $args): string {
        $append = [];

        foreach ($args as $arg => $val) {
            if ( ! $val)
                continue;

            if (is_string($val))
                $val = escapeshellarg($val);

            $append[] = "--$arg";
            $append[] = $val;
        }

        return implode(' ', $append);
    }

    private function buildPhpCmd(string $script, array $args): string {
        return PhpSpawner::cmd($script) . ' ' . $this->appendCmdArgs($args);
    }

    private function startSessionCmd(): string {
        return $this->buildPhpCmd(self::start_script, $this->args);
    }

    public function output(?int $timeout = NULL): ?string {
        if ($this->output) {
            var_dump("early return");
            return $this->output;
        }

        if ( ! $this->output_file || ! $this->pid) {
            throw new \Exception("Process was not started in the background");
        }

        $st = microtime(true);

        do {
            if (pcntl_waitpid($this->pid, $exit_code, WNOHANG) == 0)
                break;
            usleep(100000);
        } while ((microtime(true) - $st) < $timeout);

        if ( ! file_exists($this->output_file))
            return "";

        $this->output      = file_get_contents($this->output_file);
        $this->output_file = NULL;
        $this->exit_code   = $exit_code;
        $this->pid         = NULL;

        return $this->output;
    }

    public function execBg(): bool {
        if ($this->cmd)
            throw new \Exception("Command already executed!");

        $output_file = $this->getTmpFileName();

        $this->cmd  = $this->startSessionCmd();
        $this->cmd .= " >$output_file 2>&1 & echo $!";

        $pid = exec($this->cmd, $output, $exit_code);
        $this->exit_code = $exit_code;

        if ($this->exit_code || !is_numeric($pid))
            return false;

        $this->pid = (int)$pid;
        $this->output_file = $output_file;

        return true;
    }

    public function execFg() {
        if ($this->cmd)
            throw new \Exception("Command already executed!");

        $this->cmd = $this->startSessionCmd() . ' 2>&1';

        exec($this->cmd, $output, $exit_code);
        $this->exit_code = $exit_code;
        $this->output = implode("\n", array_filter($output));

        return $this->output;
    }

    private function regenerateIdCmd($locking, $destroy, $proxy): string {
        $this->validateArgs(['handler', 'id', 'save-path']);

        $args = [
            'handler' => $this->args['handler'],
            'save-path' => $this->args['save-path'],
            'id' => $this->args['id'],
            'locking-enabled' => !!$locking,
            'destroy' => !!$destroy,
            'proxy' => !!$proxy,
        ];

        return $this->buildPhpCmd(self::regenerate_id_script, $args);
    }

    public function regenerateId($locking = false, $destroy = false, $proxy = false) {
        if ( ! $this->cmd)
            throw new \Exception("Cannot regenerate id before starting session!");

        $cmd = $this->regenerateIdCmd($locking, $destroy, $proxy);

        exec($cmd, $output, $exit_code);

        if ($exit_code != 0)
            return false;

        return $output[0];
    }

    private function getDataCmd(?int $lifetime): string {
        $this->validateArgs(['handler', 'save-path', 'id']);

        $args = [
            'handler' => $this->args['handler'],
            'save-path' => $this->args['save-path'],
            'id' => $this->args['id'],
            'lifetime' => is_int($lifetime) ? $lifetime : $this->args['lifetime'],
        ];

        return $this->buildPhpCmd(self::get_data_script, $args);
    }

    public function getData(?int $lifetime = NULL): string {
        $cmd = $this->getDataCmd($lifetime);

        exec($cmd, $output, $exit_code);
        if ($exit_code != 0) {
            return implode("\n", $output);
        }

        return $output[0];
    }
}
