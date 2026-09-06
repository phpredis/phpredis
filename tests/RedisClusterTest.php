<?php defined('PHPREDIS_TESTRUN') or die("Use TestRedis.php to run tests!\n");

require_once __DIR__ . "/RedisTest.php";

class RedisClusterPipelineReentrantValue {
    public static $redis;
    public static $key;

    public function __wakeup() {
        self::$redis->set(self::$key, 'side-effect');
    }
}

class RedisClusterPipelineSetOptionValue {
    public static $redis;

    public function __wakeup() {
        self::$redis->setOption(
            Redis::OPT_SERIALIZER,
            Redis::SERIALIZER_NONE
        );
    }
}

/**
 * Most RedisCluster tests should work the same as the standard Redis object
 * so we only override specific functions where the prototype is different or
 * where we're validating specific cluster mechanisms
 */
class Redis_Cluster_Test extends Redis_Test {
    private $redis_types = [
        Redis::REDIS_STRING,
        Redis::REDIS_SET,
        Redis::REDIS_LIST,
        Redis::REDIS_ZSET,
        Redis::REDIS_HASH
    ];

    private $failover_types = [
        RedisCluster::FAILOVER_NONE,
        RedisCluster::FAILOVER_ERROR,
        RedisCluster::FAILOVER_DISTRIBUTE
    ];

    protected static array $seeds = [];

    private static array  $seed_messages = [];
    private static string $seed_source = '';

    protected function havePipeline() {
        return true;
    }

    private function startMalformedReplyServer($scenario) {
        if (!function_exists('proc_open')) {
            $this->markTestSkipped('proc_open is required');
        }

        $process = proc_open(
            [PHP_BINARY, '-n', __DIR__ . '/RedisClusterMalformedReplyServer.php', $scenario],
            [
                0 => ['pipe', 'r'],
                1 => ['pipe', 'w'],
                2 => ['pipe', 'w'],
            ],
            $pipes
        );

        if (!is_resource($process)) {
            $this->markTestSkipped('Unable to start malformed reply server');
        }

        fclose($pipes[0]);
        unset($pipes[0]);
        stream_set_timeout($pipes[1], 5);
        $port = (int)trim((string)fgets($pipes[1]));

        if ($port <= 0) {
            $this->stopMalformedReplyServer($process, $pipes);
            throw new RuntimeException('Malformed reply server did not start');
        }

        return [$process, $pipes, $port];
    }

    private function stopMalformedReplyServer($process, $pipes) {
        $status = proc_get_status($process);
        if ($status['running']) {
            proc_terminate($process);
        }
        foreach ($pipes as $pipe) {
            if (is_resource($pipe)) fclose($pipe);
        }
        proc_close($process);
    }

    private function assertMalformedPipelineReplyAborts(
        $scenario,
        $message = 'Error reading pipeline response'
    ) {
        [$process, $pipes, $port] = $this->startMalformedReplyServer($scenario);
        $redis = $exception = null;

        try {
            $redis = new RedisCluster(
                null, ["127.0.0.1:$port"], 1, 1, true
            );

            $pipe = $redis->pipeline();
            if ($scenario !== 'pipeline') {
                $pipe->multi()
                    ->lmpop(['{malformed}list'], 'LEFT')
                    ->get('{malformed}queued')
                    ->exec();
            } else {
                $pipe->lmpop(['{malformed}list'], 'LEFT')
                    ->get('{malformed}queued');
            }

            try {
                @$pipe->exec();
            } catch (RedisClusterException $e) {
                $exception = $e;
            }

            $this->assertTrue($exception instanceof RedisClusterException);
            $this->assertStringContains($message, $exception->getMessage());
            $this->assertEquals(Redis::ATOMIC, $redis->getMode());
            $this->assertEquals('actual', $redis->get('{malformed}after'));
        } finally {
            if ($redis) @$redis->close();
            $this->stopMalformedReplyServer($process, $pipes);
        }
    }

    private function keysOnDistinctMasters($prefix, $count = 2) {
        $keys = [];

        for ($i = 0; $i < 256 && count($keys) < $count; $i++) {
            $key = "{{$prefix}-{$i}}key";
            $master = $this->redis->cluster($key, 'MYID');
            if (is_string($master)) {
                $keys[$master] = $key;
            }
        }

        if (count($keys) < $count) {
            throw new RuntimeException("Unable to locate $count cluster masters");
        }

        return array_values($keys);
    }

    private function assertPipelineRedirectAbortsAcrossNodes(
        $type,
        $nested = false
    ) {
        [$key, $control] = $this->keysOnDistinctMasters(
            'pipe-' . strtolower($type) . ($nested ? '-multi' : '')
        );
        $slot = $this->redis->cluster($key, 'KEYSLOT', $key);
        $master = $this->redis->_masters()[0];
        $endpoint = sprintf('%s:%d', $master[0], $master[1]);
        $script = sprintf(
            "return redis.error_reply('%s %d %s')",
            $type,
            $slot,
            $endpoint
        );
        $exception = NULL;

        $this->redis->set($key, 'still-readable');
        $this->redis->del($control);
        $pipe = $this->redis->pipeline();

        if ($nested) {
            $pipe->multi()
                ->eval($script, [$key], 1)
                ->get($key)
                ->exec();
        } else {
            $pipe->eval($script, [$key], 1)->get($key);
        }
        $pipe->set($control, 'queued-on-other-node')->get($control);

        try {
            $pipe->exec();
        } catch (RedisClusterException $e) {
            $exception = $e;
        }

        $this->assertTrue($exception instanceof RedisClusterException);
        $this->assertStringContains('redirected', $exception->getMessage());
        $this->assertEquals(Redis::ATOMIC, $this->redis->getMode());

        /* Both sockets had unread replies when the redirect was observed. */
        $this->assertEquals('still-readable', $this->redis->get($key));
        $this->assertEquals(
            'queued-on-other-node',
            $this->redis->get($control)
        );
    }

    public function testServerInfo() { $this->markTestSkipped(); }
    public function testServerInfoOldRedis() { $this->markTestSkipped(); }

    /* Tests we'll skip all together in the context of RedisCluster.  The
     * RedisCluster class doesn't implement specialized (non-redis) commands
     * such as sortAsc, or sortDesc and other commands such as SELECT are
     * simply invalid in Redis Cluster */
    public function testSortAsc()  { $this->markTestSkipped(); }
    public function testSortDesc() { $this->markTestSkipped(); }
    public function testWait()     { $this->markTestSkipped(); }
    public function testSelect()   { $this->markTestSkipped(); }
    public function testReconnectSelect() { $this->markTestSkipped(); }
    public function testMultipleConnect() { $this->markTestSkipped(); }
    public function testSwapDB() { $this->markTestSkipped(); }
    public function testConnectException() { $this->markTestSkipped(); }
    public function testTlsConnect() { $this->markTestSkipped(); }
    public function testTlsReconnect() { $this->markTestSkipped(); }
    public function testReset() { $this->markTestSkipped(); }
    public function testInvalidAuthArgs() { $this->markTestSkipped(); }
    public function testScanErrors() { $this->markTestSkipped(); }
    public function testConnectDatabaseSelect() { $this->markTestSkipped(); }

    /* These 'directed node' commands work differently in RedisCluster */
    public function testConfig() { $this->markTestSkipped(); }
    public function testFlushDB() { $this->markTestSkipped(); }
    public function testFunction() { $this->markTestSkipped(); }

    /* Session locking feature is currently not supported in in context of Redis Cluster.
       The biggest issue for this is the distribution nature of Redis cluster */
    public function testSession_lockKeyCorrect() { $this->markTestSkipped(); }
    public function testSession_lockingDisabledByDefault() { $this->markTestSkipped(); }
    public function testSession_lockReleasedOnClose() { $this->markTestSkipped(); }
    public function testSession_lock_ttlMaxExecutionTime() { $this->markTestSkipped(); }
    public function testSession_lock_ttlLockExpire() { $this->markTestSkipped(); }
    public function testSession_lockHoldCheckBeforeWrite_otherProcessHasLock() { $this->markTestSkipped(); }
    public function testSession_lockHoldCheckBeforeWrite_nobodyHasLock() { $this->markTestSkipped(); }
    public function testSession_correctLockRetryCount() { $this->markTestSkipped(); }
    public function testSession_defaultLockRetryCount() { $this->markTestSkipped(); }
    public function testSession_noUnlockOfOtherProcess() { $this->markTestSkipped(); }
    public function testSession_lockWaitTime() { $this->markTestSkipped(); }

    /* Regression test for GH #2810 */
    public function testConstructNullSeeds() {
        /* new RedisCluster(null, null) must not throw TypeError.
         * $seeds is declared ?array so null is a valid argument. */
        $thrown = false;
        try {
            new RedisCluster(null, null);
        } catch (\Throwable $e) {
            $thrown = true;
            $this->assertFalse($e instanceof \TypeError);
        }
        $this->assertTrue($thrown);

        /* Passing an empty array must also not throw TypeError (control). */
        $thrown = false;
        try {
            new RedisCluster(null, []);
        } catch (\Throwable $e) {
            $thrown = true;
            $this->assertFalse($e instanceof \TypeError);
        }
        $this->assertTrue($thrown);

        /* Both (null) and (null, null) mean "no name, no seeds" and must
         * produce the same exception type. */
        $ex1 = $ex2 = null;
        try { new RedisCluster(null); }
        catch (\Throwable $e) { $ex1 = get_class($e); }
        try { new RedisCluster(null, null); }
        catch (\Throwable $e) { $ex2 = get_class($e); }
        $this->assertTrue($ex1 !== null);
        $this->assertEquals($ex1, $ex2);
    }

    private function loadSeedsFromHostPort($host, $port) {
        try {
            $rc = new RedisCluster(NULL, ["$host:$port"], 1, 1, true, $this->getAuth());
            self::$seed_source = "Host: $host, Port: $port";
            return array_map(function($master) {
                return sprintf('%s:%s', $master[0], $master[1]);
            }, $rc->_masters());
        } catch (Exception $ex) {
            /* fallthrough */
        }

        self::$seed_messages[] = "--host=$host, --port=$port";

        return false;
    }

    private function loadSeedsFromEnv() {
        $seeds = getenv('REDIS_CLUSTER_NODES');
        if ( ! $seeds) {
            self::$seed_messages[] = "environment variable REDIS_CLUSTER_NODES ($seeds)";
            return false;
        }

        self::$seed_source = 'Environment variable REDIS_CLUSTER_NODES';
        return array_filter(explode(' ', $seeds));
    }

    private function loadSeedsFromNodeMap() {
        $nodemap_file = dirname($_SERVER['PHP_SELF']) . '/nodes/nodemap';
        if ( ! file_exists($nodemap_file)) {
            self::$seed_messages[] = "nodemap file '$nodemap_file'";
            return false;
        }

        self::$seed_source = "Nodemap file '$nodemap_file'";
        return array_filter(explode("\n", file_get_contents($nodemap_file)));
    }

    private function loadSeeds($host, $port) {
        if (($seeds = $this->loadSeedsFromNodeMap()))
            return $seeds;
        if (($seeds = $this->loadSeedsFromEnv()))
            return $seeds;
        if (($seeds = $this->loadSeedsFromHostPort($host, $port)))
            return $seeds;

        TestSuite::errorMessage("Error:  Unable to load seeds for RedisCluster tests");
        foreach (self::$seed_messages as $msg) {
            TestSuite::errorMessage("   Tried: %s", $msg);
        }

        exit(1);
    }

    /* Load our seeds on construction */
    public function __construct($host, $port, $auth, $tls_port = 6378) {
        parent::__construct($host, $port, $auth, $tls_port);

        self::$seeds = $this->loadSeeds($host, $port);
    }

    protected function queryServerInfo($redis) {
        return $redis->info(uniqid());
    }

    private function findCliExe() {
        foreach (['redis-cli', 'valkey-cli'] as $candidate) {
            $path = trim(shell_exec("command -v $candidate 2>/dev/null"));
            if (is_executable($path)) {
                return $path;
            }
        }

        return NULL;
    }

    private function getServerReply($host, $port, $cmd) {
        $cli = $this->findCliExe();
        if ( ! $cli) {
            return '(no redis-cli or valkey-cli found)';
        }

        $args = [$cli, '-h', $host, '-p', $port];

        $this->getAuthParts($user, $pass);

        if ($user) $args = array_merge($args, ['--user', $user]);
        if ($pass) $args = array_merge($args, ['-a', $pass]);

        $resp = shell_exec(implode(' ', $args) . ' ' . $cmd . ' 2>/dev/null');

        return is_string($resp) ? trim($resp) : $resp;
    }

    /* Try to gat a new RedisCluster instance. The strange logic is an attempt
       to solve a problem where this sometimes fails but only ever on GitHub
       runners. If we're not on a runner we just get a new instance. Otherwise
       we allow for two tries to get the instance. */
    private function getNewInstance() {
        if (getenv('GITHUB_ACTIONS') === 'true') {
            try {
                return new RedisCluster(NULL, self::$seeds, 30, 30, true,
                                        $this->getAuth());
            } catch (Exception $ex) {
                TestSuite::errorMessage("Failed to connect: %s", $ex->getMessage());
            }
        }

        return new RedisCluster(NULL, self::$seeds, 30, 30, true, $this->getAuth());
    }

    /* Override newInstance as we want a RedisCluster object */
    protected function newInstance() {
        try {
            return $this->getNewInstance();
        } catch (Exception $ex) {
            TestSuite::errorMessage("");
            TestSuite::errorMessage("Fatal error: %s", $ex->getMessage());
            TestSuite::errorMessage("Seeds: %s", implode(' ', self::$seeds));
            TestSuite::errorMessage("Seed source: %s", self::$seed_source);
            TestSuite::errorMessage("");

            TestSuite::errorMessage("Backtrace:");
            foreach (debug_backtrace(DEBUG_BACKTRACE_IGNORE_ARGS) as $i => $frame) {
                $file = isset($frame['file']) ? basename($frame['file']) : '[internal]';
                $line = $frame['line'] ?? '?';
                $func = $frame['function'] ?? 'unknown';
                TestSuite::errorMessage("  %s:%d [%s]", $file, $line, $func);
            }

            TestSuite::errorMessage("\nServer responses:");

            /* See if we can shed some light on whether Redis is available */
            foreach (self::$seeds as $seed) {
                list($host, $port) = explode(':', $seed);

                $st = microtime(true);
                $reply = $this->getServerReply($host, $port, 'PING');
                $et = microtime(true);

                TestSuite::errorMessage("  [%s:%d] PING -> %s (%.4f)", $host,
                                        $port, var_export($reply, true),
                                        $et - $st);
            }

            exit(1);
        }
    }

    /* Overrides for RedisTest where the function signature is different.  This
     * is only true for a few commands, which by definition have to be directed
     * at a specific node */

    public function testPing() {
        for ($i = 0; $i < 20; $i++) {
            $this->assertTrue($this->redis->ping("key:$i"));
            $this->assertEquals('BEEP', $this->redis->ping("key:$i", 'BEEP'));
        }

        /* Make sure both variations work in MULTI mode */
        $this->redis->multi();
        $this->redis->ping('{ping-test}');
        $this->redis->ping('{ping-test}', 'BEEP');
        $this->assertEquals([true, 'BEEP'], $this->redis->exec());
    }

    /* Keep the historical immediate-command path independent of pipeline
     * support.  In particular, distributed commands must still fold replies
     * in argument order and leave each participating socket reusable. */
    public function testNonPipelineAtomicUpgradeRegression() {
        $keys = [
            '{non-pipeline-a}atomic-one',
            '{non-pipeline-b}atomic-two',
        ];

        $this->assertEquals(Redis::ATOMIC, $this->redis->getMode());
        $this->redis->del($keys);

        $this->assertTrue($this->redis->mset([
            $keys[0] => 'one',
            $keys[1] => 'two',
        ]));
        $this->assertEquals(['two', 'one'], $this->redis->mget([
            $keys[1], $keys[0]
        ]));
        $this->assertEquals([0, 0], $this->redis->msetnx([
            $keys[0] => 'changed-one',
            $keys[1] => 'changed-two',
        ]));
        $this->assertEquals(2, $this->redis->del($keys));

        /* A Redis error is a normal false return and must not dirty the socket. */
        $this->assertTrue($this->redis->set($keys[0], 'not-a-list'));
        $this->assertFalse($this->redis->lpush($keys[0], 'value'));
        $this->assertTrue($this->redis->set($keys[0], 'after-error'));
        $this->assertEquals('after-error', $this->redis->get($keys[0]));
        $this->assertEquals(Redis::ATOMIC, $this->redis->getMode());
    }

    /* Default RedisCluster::multi() remains the historical distributed
     * transaction implementation.  These commands intentionally span nodes
     * and exercise the shared distributed-command response context. */
    public function testNonPipelineMultiUpgradeRegression() {
        $keys = [
            '{non-pipeline-a}multi-one',
            '{non-pipeline-b}multi-two',
        ];

        $this->redis->del($keys);
        $tx = $this->redis->multi();

        $this->assertEquals(Redis::MULTI, $this->redis->getMode());
        $this->assertTrue($tx === $this->redis);

        $tx->mset([$keys[0] => 'one', $keys[1] => 'two'])
            ->mget([$keys[1], $keys[0]])
            ->msetnx([$keys[0] => 'changed-one', $keys[1] => 'changed-two'])
            ->del($keys)
            ->set($keys[0], 'after')
            ->get($keys[0]);

        $this->assertEquals([
            true,
            ['two', 'one'],
            [0, 0],
            2,
            true,
            'after',
        ], $tx->exec());
        $this->assertEquals(Redis::ATOMIC, $this->redis->getMode());
        $this->assertEquals('after', $this->redis->get($keys[0]));
        $this->assertFalse($this->redis->get($keys[1]));
    }

    public function testNonPipelineMultiErrorAndDiscardUpgradeRegression() {
        $wrongType = '{non-pipeline-a}multi-wrong-type';
        $control = '{non-pipeline-b}multi-control';

        $this->redis->del([$wrongType, $control]);
        $this->assertTrue($this->redis->set($wrongType, 'not-a-list'));

        $ret = $this->redis->multi()
            ->lpush($wrongType, 'value')
            ->set($control, 'committed')
            ->get($control)
            ->exec();

        $this->assertEquals([false, true, 'committed'], $ret);
        $this->assertEquals('committed', $this->redis->get($control));

        $this->redis->multi()
            ->set($wrongType, 'discarded')
            ->set($control, 'discarded');

        $this->assertTrue($this->redis->discard());
        $this->assertEquals(Redis::ATOMIC, $this->redis->getMode());
        $this->assertEquals('not-a-list', $this->redis->get($wrongType));
        $this->assertEquals('committed', $this->redis->get($control));

        /* DISCARD must consume/reset transaction state on every node. */
        $this->assertTrue($this->redis->set($control, 'after-discard'));
        $this->assertEquals('after-discard', $this->redis->get($control));
    }

    public function testPipelineSameSlot() {
        $key1 = '{pipe}one';
        $key2 = '{pipe}two';

        $this->redis->del([$key1, $key2]);

        $ret = $this->redis->pipeline()
            ->set($key1, '1')
            ->set($key2, '2')
            ->mget([$key1, $key2])
            ->exec();

        $this->assertEquals([true, true, ['1', '2']], $ret);
    }

    public function testPipelineMultiExec() {
        $first = '{pipe-inherited-a}key';
        $second = '{pipe-inherited-b}key';

        $this->redis->del([$first, $second]);
        $this->assertEquals(
            [[]],
            $this->redis->pipeline()->multi()->exec()->exec()
        );

        $ret = $this->redis->pipeline()
            ->get($first)
            ->multi()->set($first, 42)->incr($first)->exec()
            ->get($first)
            ->multi()->set($second, 'value')->get($second)->exec()
            ->get($second)
            ->exec();

        $this->assertEquals(
            [false, [true, 43], '43', [true, 'value'], 'value'],
            $ret
        );
    }

    public function testPipelineDifferentSlots() {
        $key1 = '{pipeA}key1';
        $key2 = '{pipeB}key2';

        $this->redis->del([$key1, $key2]);

        $ret = $this->redis->pipeline()
            ->set($key1, '1')
            ->set($key2, '2')
            ->get($key2)
            ->get($key1)
            ->exec();

        $this->assertEquals([true, true, '2', '1'], $ret);
        $this->assertEquals(Redis::ATOMIC, $this->redis->getMode());
    }

    public function testPipelineInterleavedNodeResponseOrder() {
        $pipe = $this->redis->pipeline();
        $expected = [];

        for ($i = 0; $i < 12; $i++) {
            $tag = $i % 2 === 0 ? 'pipeA' : 'pipeB';
            $key = "{{$tag}}order-{$i}";
            $value = "value-{$i}";

            $pipe->set($key, $value)->get($key);
            $expected[] = true;
            $expected[] = $value;
        }

        $this->assertEquals($expected, $pipe->exec());
    }

    public function testPipelineTransferredBytes() {
        $values = [
            '{pipe-bytes-a}key' => 'one',
            '{pipe-bytes-b}key' => 'two',
        ];

        $this->redis->mset($values);
        $this->redis->clearTransferredBytes();

        $pipe = $this->redis->pipeline();
        foreach ($values as $key => $_) {
            $pipe->get($key);
        }

        /* Queueing is entirely client-side. */
        $this->assertEquals([0, 0], $this->redis->getTransferredBytes());
        $this->assertEquals(array_values($values), $pipe->exec());

        $expectedTx = $expectedRx = 0;
        foreach ($values as $key => $value) {
            $expectedTx += strlen(
                "*2\r\n$3\r\nGET\r\n$" . strlen($key) . "\r\n$key\r\n"
            );
            $expectedRx += strlen(
                '$' . strlen($value) . "\r\n$value\r\n"
            );
        }

        [$tx, $rx] = $this->redis->getTransferredBytes();
        $this->assertEquals($expectedTx, $tx);
        $this->assertEquals($expectedRx, $rx);
    }

    public function testPipelineStructuredAndKeylessReplies() {
        $hash = '{pipe-replies-a}hash';
        $zset = '{pipe-replies-b}zset';
        $stream = '{pipe-replies-c}stream';

        $this->redis->del([$hash, $zset, $stream]);

        $ret = $this->redis->pipeline()
            ->hset($hash, 'field', 'value')
            ->hgetall($hash)
            ->zadd($zset, 1.5, 'member')
            ->zrange($zset, 0, -1, true)
            ->xadd($stream, '1-0', ['field' => 'value'])
            ->xrange($stream, '-', '+')
            ->eval('return {1, "ok"}')
            ->command('COUNT')
            ->exec();

        $this->assertEquals(1, $ret[0]);
        $this->assertEquals(['field' => 'value'], $ret[1]);
        $this->assertEquals(1, $ret[2]);
        $this->assertEquals(['member' => 1.5], $ret[3]);
        $this->assertEquals('1-0', $ret[4]);
        $this->assertEquals(['1-0' => ['field' => 'value']], $ret[5]);
        $this->assertEquals([1, 'ok'], $ret[6]);
        $this->assertIsInt($ret[7]);

        /* Every variable-length reply must be fully consumed. */
        $this->assertTrue($this->redis->set($hash, 'after'));
        $this->assertEquals('after', $this->redis->get($hash));
    }

    public function testPipelineBlockingCommandWithReadyData() {
        $list = '{pipe-blocking-a}list';
        $control = '{pipe-blocking-b}control';

        $this->redis->del([$list, $control]);
        $this->assertEquals(1, $this->redis->rpush($list, 'ready'));

        $ret = $this->redis->pipeline()
            ->blpop([$list], .5)
            ->set($control, 'ok')
            ->get($control)
            ->exec();

        $this->assertEquals([[$list, 'ready'], true, 'ok'], $ret);
        $this->assertEquals('ok', $this->redis->get($control));
    }

    public function testRejectedPipelineCommandFamiliesLeaveQueueClean() {
        $key = '{pipe-rejected}key';
        $commands = [
            'raw' => function () use ($key) {
                return $this->redis->cluster($key, 'KEYSLOT', $key);
            },
            'script' => function () use ($key) {
                return $this->redis->script($key, 'EXISTS', str_repeat('0', 40));
            },
            'scan' => function () use ($key) {
                $iterator = NULL;
                return $this->redis->scan($iterator, $key);
            },
            'key-scan' => function () use ($key) {
                $iterator = NULL;
                return $this->redis->hscan($key, $iterator);
            },
        ];

        foreach ($commands as $name => $command) {
            $exception = NULL;
            $result = NULL;
            $pipe = $this->redis->pipeline();

            try {
                $result = @$command();
            } catch (RedisClusterException $e) {
                $exception = $e;
            }

            $this->assertTrue(
                $result === false || $exception instanceof RedisClusterException
            );
            $this->assertEquals(Redis::PIPELINE, $this->redis->getMode());
            $this->assertEquals(
                [true, $name],
                $pipe->set($key, $name)->get($key)->exec()
            );
            $this->assertEquals(Redis::ATOMIC, $this->redis->getMode());
        }
    }

    public function testPipelineCrossSlotMgetPreservesResultOrder() {
        $keys = [
            '{pipeA}mget-first',
            '{pipeB}mget-middle',
            '{pipeA}mget-last',
        ];

        $this->redis->mset([
            $keys[0] => 'first',
            $keys[1] => 'middle',
            $keys[2] => 'last',
        ]);

        $ret = $this->redis->pipeline()
            ->mget($keys)
            ->get($keys[1])
            ->exec();

        $this->assertEquals([['first', 'middle', 'last'], 'middle'], $ret);
    }

    public function testPipelineEmptyExec() {
        $ret = $this->redis->pipeline()->exec();
        $this->assertEquals([], $ret);
    }

    public function testPipelineDiscard() {
        $key = '{pipe}discard';
        $this->redis->del($key);

        $this->redis->pipeline()->set($key, 'value');
        $this->assertTrue($this->redis->discard());
        $this->assertFalse($this->redis->get($key));
    }

    public function testPipelineDiscardDistributedContexts() {
        $keys = ['{pipeA}discard-one', '{pipeB}discard-two'];

        $this->redis->del($keys);
        $this->redis->pipeline()
            ->mget($keys)
            ->mset([$keys[0] => 'one', $keys[1] => 'two']);

        $this->assertTrue($this->redis->discard());
        $this->assertEquals([false, false], $this->redis->mget($keys));
    }

    public function testPipelineDiscardInsideMultiDiscardsOuterPipeline() {
        $outer = '{pipeA}discard-outer';
        $transaction = '{pipeB}discard-transaction';

        $this->redis->del([$outer, $transaction]);

        $pipe = $this->redis->pipeline()->set($outer, 'outer')->multi();
        $pipe->set($transaction, 'transaction');

        $this->assertTrue($pipe->discard());
        $this->assertEquals(Redis::ATOMIC, $this->redis->getMode());
        $this->assertEquals(
            [false, false],
            $this->redis->mget([$outer, $transaction])
        );
    }

    public function testPipelineObjectDestructionWithDistributedContexts() {
        $redis = $this->getNewInstance();
        $redis->pipeline()
            ->mget(['{pipeA}destruct-one', '{pipeB}destruct-two'])
            ->mset([
                '{pipeA}destruct-one' => 'one',
                '{pipeB}destruct-two' => 'two',
            ]);

        unset($redis);
        gc_collect_cycles();

        /* Destruction must not affect a different live cluster context. */
        $this->assertTrue($this->redis->set('{pipe}destruct-control', 'ok'));
    }

    public function testPipelineObjectDestructionWithOpenMultiState() {
        $key = '{pipe-multi-destruct}key';
        $this->redis->del($key);

        $deferred = $this->getNewInstance();
        $deferred->pipeline()->multi()->eval('return 1');
        unset($deferred);

        $bound = $this->getNewInstance();
        $bound->pipeline()->multi()->set($key, 'must-not-run');
        unset($bound);
        gc_collect_cycles();

        $this->assertFalse($this->redis->get($key));
        $this->assertTrue($this->redis->set($key, 'clean'));
        $this->assertEquals('clean', $this->redis->get($key));
    }

    public function testInvalidPipelineCommandsPreserveQueuedState() {
        $key = '{pipe-invalid-command}key';
        $invalid = [
            ['mget', [[]]],
            ['mset', [[]]],
            ['msetnx', [[]]],
            ['del', [[]]],
            ['unlink', [[]]],
        ];

        $this->redis->del($key);
        $pipe = $this->redis->pipeline()->set($key, 'outer');
        foreach ($invalid as [$method, $args]) {
            $this->assertFalse(@$pipe->$method(...$args));
            $this->assertEquals(Redis::PIPELINE, $pipe->getMode());
        }
        $this->assertEquals([true, 'outer'], $pipe->get($key)->exec());

        $pipe = $this->redis->pipeline()->multi();
        $this->assertFalse(@$pipe->mget([]));
        $pipe->set($key, 'transaction')->get($key)->exec();
        $this->assertEquals([[true, 'transaction']], $pipe->exec());
    }

    public function testPipelineSameSlotMultiKeyOps() {
        $key1 = '{pipeM}one';
        $key2 = '{pipeM}two';

        $this->redis->del([$key1, $key2]);

        $ret = $this->redis->pipeline()
            ->mset([$key1 => '1', $key2 => '2'])
            ->msetnx([$key1 => '3', $key2 => '4'])
            ->del([$key1, $key2])
            ->exec();

        $this->assertEquals([true, [0, 0], 2], $ret);
    }

    public function testPipelineMissingKey() {
        $key = '{pipe}missing';
        $this->redis->del($key);

        $ret = $this->redis->pipeline()
            ->get($key)
            ->exec();

        $this->assertTrue(is_array($ret) && array_key_exists(0, $ret));
        $this->assertFalse($ret[0]);
    }

    public function testPipelineWithPrefix() {
        $key = '{pipe}prefix';
        $prefixA = 'pipeline-a:';
        $prefixB = 'pipeline-b:';

        try {
            $this->redis->setOption(Redis::OPT_PREFIX, '');
            $this->redis->del([$prefixA . $key, $prefixB . $key]);

            $this->redis->setOption(Redis::OPT_PREFIX, $prefixA);
            $pipe = $this->redis->pipeline()->set($key, 'a');

            /* Prefixing is command construction state, so changing it while
             * queueing affects only commands constructed afterward. */
            $this->redis->setOption(Redis::OPT_PREFIX, $prefixB);
            $ret = $pipe->set($key, 'b')->get($key)->exec();
            $this->assertEquals([true, true, 'b'], $ret);

            $this->redis->setOption(Redis::OPT_PREFIX, '');
            $this->assertEquals(
                ['a', 'b'],
                $this->redis->mget([$prefixA . $key, $prefixB . $key])
            );
        } finally {
            $this->redis->setOption(Redis::OPT_PREFIX, '');
            $this->redis->del([$prefixA . $key, $prefixB . $key]);
        }
    }

    public function testPipelineUsesSerializerSelectedAtExec() {
        $key = '{pipe}serializer-options';
        $value = ['answer' => 42];

        $this->redis->setOption(Redis::OPT_SERIALIZER, Redis::SERIALIZER_PHP);
        $this->redis->set($key, $value);

        $pipe = $this->redis->pipeline()->get($key);
        $this->redis->setOption(Redis::OPT_SERIALIZER, Redis::SERIALIZER_NONE);
        $pipe->get($key);

        /* Match Redis: Pipeline replies use the serializer active at exec. */
        $serialized = serialize($value);
        $this->assertEquals([$serialized, $serialized], $pipe->exec());
    }

    public function testPipelineUsesCompressionSelectedAtExec() {
        $compressors = array_values(array_filter(
            $this->getCompressors(),
            function ($compressor) {
                return $compressor !== Redis::COMPRESSION_NONE;
            }
        ));

        if (!$compressors) {
            $this->markTestSkipped();
        }

        $key = '{pipe}compression-options';
        $value = str_repeat('pipeline-compression-', 32);
        $serializer = $this->redis->getOption(Redis::OPT_SERIALIZER);
        $compression = $this->redis->getOption(Redis::OPT_COMPRESSION);

        try {
            $this->redis->setOption(Redis::OPT_SERIALIZER, Redis::SERIALIZER_NONE);
            $this->redis->setOption(Redis::OPT_COMPRESSION, $compressors[0]);
            $encoded = $this->redis->_compress($value);
            $this->assertTrue($this->redis->set($key, $value));

            $pipe = $this->redis->pipeline()->get($key);
            $this->redis->setOption(
                Redis::OPT_COMPRESSION,
                Redis::COMPRESSION_NONE
            );

            /* Like standalone Redis, pipeline replies use the option state at
             * exec time, not the state when the read was queued. */
            $this->assertEquals([$encoded], $pipe->exec());
        } finally {
            $this->redis->setOption(Redis::OPT_COMPRESSION, $compression);
            $this->redis->setOption(Redis::OPT_SERIALIZER, $serializer);
            $this->redis->del($key);
        }
    }

    public function testPipelineUsesPackingOptionsSelectedWhenQueued() {
        $keys = [
            '{pipe-packing-options}serialized',
            '{pipe-packing-options}number',
        ];
        $serializer = $this->redis->getOption(Redis::OPT_SERIALIZER);
        $compression = $this->redis->getOption(Redis::OPT_COMPRESSION);
        $level = $this->redis->getOption(Redis::OPT_COMPRESSION_LEVEL);
        $ignoreNumbers = $this->redis->getOption(
            Redis::OPT_PACK_IGNORE_NUMBERS
        );
        $compressors = $this->getCompressors();

        try {
            $this->redis->setOption(
                Redis::OPT_SERIALIZER, Redis::SERIALIZER_PHP
            );
            $this->redis->setOption(
                Redis::OPT_COMPRESSION, end($compressors)
            );
            $this->redis->setOption(Redis::OPT_COMPRESSION_LEVEL, 1);
            $this->redis->setOption(Redis::OPT_PACK_IGNORE_NUMBERS, false);

            $pipe = $this->redis->pipeline()->set($keys[0], ['answer' => 42]);

            /* Packing options are applied while each command is built. */
            $this->redis->setOption(Redis::OPT_COMPRESSION_LEVEL, 3);
            $this->redis->setOption(Redis::OPT_PACK_IGNORE_NUMBERS, true);
            $pipe->set($keys[1], 42);

            $this->assertEquals([true, true], $pipe->exec());
            $this->assertEquals(
                [['answer' => 42], 42],
                [
                    $this->redis->get($keys[0]),
                    $this->redis->get($keys[1]),
                ]
            );
        } finally {
            $this->redis->setOption(
                Redis::OPT_PACK_IGNORE_NUMBERS, $ignoreNumbers
            );
            $this->redis->setOption(Redis::OPT_COMPRESSION_LEVEL, $level);
            $this->redis->setOption(Redis::OPT_COMPRESSION, $compression);
            $this->redis->setOption(Redis::OPT_SERIALIZER, $serializer);
            $this->redis->del($keys);
        }
    }

    public function testPipelineUsesReplyOptionsSelectedAtExec() {
        $key = '{pipe-reply-options}key';
        $blockingKey = '{pipe-reply-options}empty-list';
        $replyLiteral = $this->redis->getOption(Redis::OPT_REPLY_LITERAL);
        $nullMbulk = $this->redis->getOption(Redis::OPT_NULL_MULTIBULK_AS_NULL);

        try {
            $this->redis->setOption(Redis::OPT_REPLY_LITERAL, false);
            $pipe = $this->redis->pipeline()->eval(
                "return redis.call('set', KEYS[1], 'value')", [$key], 1
            );
            $this->redis->setOption(Redis::OPT_REPLY_LITERAL, true);
            $this->assertEquals(['OK'], $pipe->exec());

            $this->redis->del($blockingKey);
            foreach ([false => [], true => NULL] as $option => $expected) {
                $this->redis->setOption(
                    Redis::OPT_NULL_MULTIBULK_AS_NULL, $option
                );
                $result = $this->redis->pipeline()
                    ->blpop([$blockingKey], .01)
                    ->exec();
                $this->assertEquals([$expected], $result);
            }
        } finally {
            $this->redis->setOption(Redis::OPT_REPLY_LITERAL, $replyLiteral);
            $this->redis->setOption(
                Redis::OPT_NULL_MULTIBULK_AS_NULL, $nullMbulk
            );
            $this->redis->del([$key, $blockingKey]);
        }
    }

    public function testPipelineTransportOptionsDoNotDiscardQueuedState() {
        $key = '{pipe-transport-options}key';
        $options = [
            Redis::OPT_READ_TIMEOUT => .5,
            Redis::OPT_MAX_RETRIES => 2,
            Redis::OPT_BACKOFF_ALGORITHM => Redis::BACKOFF_ALGORITHM_CONSTANT,
            Redis::OPT_BACKOFF_BASE => 1,
            Redis::OPT_BACKOFF_CAP => 2,
        ];
        $original = [];

        try {
            $pipe = $this->redis->pipeline()->set($key, 'queued');
            foreach ($options as $option => $value) {
                $original[$option] = $this->redis->getOption($option);
                $this->assertTrue($this->redis->setOption($option, $value));
                $this->assertEquals($value, $this->redis->getOption($option));
                $this->assertEquals(Redis::PIPELINE, $this->redis->getMode());
            }

            $this->assertEquals([true, 'queued'], $pipe->get($key)->exec());
            $this->assertEquals(Redis::ATOMIC, $this->redis->getMode());
        } finally {
            foreach ($original as $option => $value) {
                $this->redis->setOption($option, $value);
            }
            if ($this->redis->getMode() === Redis::PIPELINE) {
                $this->redis->discard();
            }
            $this->redis->del($key);
        }
    }

    public function testPipelineControlOptionsDoNotDiscardQueuedState() {
        $key = '{pipe-control-options}key';
        $scan = $this->redis->getOption(Redis::OPT_SCAN);
        $keepalive = $this->redis->getOption(Redis::OPT_TCP_KEEPALIVE);

        try {
            $pipe = $this->redis->pipeline()->set($key, 'queued');

            $this->assertTrue($this->redis->setOption(
                Redis::OPT_SCAN, Redis::SCAN_RETRY
            ));
            $this->assertEquals(
                Redis::SCAN_RETRY,
                $this->redis->getOption(Redis::OPT_SCAN)
            );
            $this->assertTrue($this->redis->setOption(
                Redis::OPT_TCP_KEEPALIVE, !$keepalive
            ));
            $this->assertFalse(@$this->redis->setOption(-1, true));
            $this->assertEquals(Redis::PIPELINE, $this->redis->getMode());

            $this->assertEquals([true, 'queued'], $pipe->get($key)->exec());
            $this->assertEquals(Redis::ATOMIC, $this->redis->getMode());
        } finally {
            $this->redis->setOption(
                Redis::OPT_SCAN, Redis::SCAN_NORETRY
            );
            $this->redis->setOption(
                Redis::OPT_SCAN, Redis::SCAN_NOPREFIX
            );
            if ($scan & Redis::SCAN_RETRY) {
                $this->redis->setOption(
                    Redis::OPT_SCAN, Redis::SCAN_RETRY
                );
            }
            if ($scan & Redis::SCAN_PREFIX) {
                $this->redis->setOption(
                    Redis::OPT_SCAN, Redis::SCAN_PREFIX
                );
            }
            $this->redis->setOption(Redis::OPT_TCP_KEEPALIVE, $keepalive);
            if ($this->redis->getMode() === Redis::PIPELINE) {
                $this->redis->discard();
            }
            $this->redis->del($key);
        }
    }

    public function testPipelineUsesMastersForEveryFailoverMode() {
        $keys = ['{pipe-failover-a}key', '{pipe-failover-b}key'];
        $failover = $this->redis->getOption(RedisCluster::OPT_SLAVE_FAILOVER);
        $modes = [
            RedisCluster::FAILOVER_NONE,
            RedisCluster::FAILOVER_ERROR,
            RedisCluster::FAILOVER_DISTRIBUTE,
            RedisCluster::FAILOVER_DISTRIBUTE_SLAVES,
        ];

        try {
            foreach ($modes as $mode) {
                $this->assertTrue($this->redis->setOption(
                    RedisCluster::OPT_SLAVE_FAILOVER, $mode
                ));
                $this->assertEquals(
                    $mode,
                    $this->redis->getOption(RedisCluster::OPT_SLAVE_FAILOVER)
                );

                $value = "mode-$mode";
                $result = $this->redis->pipeline()
                    ->set($keys[0], $value)
                    ->get($keys[0])
                    ->set($keys[1], $value)
                    ->get($keys[1])
                    ->exec();
                $this->assertEquals([true, $value, true, $value], $result);
            }
        } finally {
            $this->redis->setOption(
                RedisCluster::OPT_SLAVE_FAILOVER, $failover
            );
            $this->redis->del($keys);
        }
    }

    public function testPipelineViaMulti() {
        $key1 = '{pipeA}multi1';
        $key2 = '{pipeB}multi2';

        $this->redis->del([$key1, $key2]);

        $m = $this->redis->multi(Redis::PIPELINE);
        $m->set($key1, 'a');
        $m->set($key2, 'b');
        $m->get($key1);
        $ret = $m->exec();

        $this->assertEquals([true, true, 'a'], $ret);
    }

    public function testPipelineActivationMethodsAreEquivalent() {
        $key1 = '{pipeA}equivalent-one';
        $key2 = '{pipeB}equivalent-two';

        $run = function ($pipe) use ($key1, $key2) {
            return $pipe
                ->set($key1, 1)
                ->incr($key1)
                ->set($key2, 'two')
                ->mget([$key1, $key2])
                ->exec();
        };

        $this->redis->del([$key1, $key2]);
        $pipeline = $run($this->redis->pipeline());

        $this->redis->del([$key1, $key2]);
        $multiPipeline = $run($this->redis->multi(Redis::PIPELINE));

        $this->assertEquals([true, 2, true, ['2', 'two']], $pipeline);
        $this->assertEquals($pipeline, $multiPipeline);
    }

    public function testDoublePipeNoOp() {
        $key = '{pipe}activation-aliases';
        $this->redis->del($key);

        $pipe = $this->redis->pipeline()->set($key, 'one');
        $pipe->multi(Redis::PIPELINE)->get($key);
        $pipe->pipeline()->set($key, 'two')->get($key);

        $this->assertEquals([true, 'one', true, 'two'], $pipe->exec());
    }

    public function testPipelineMultiSameSlot() {
        $key1 = '{pipe-tx}one';
        $key2 = '{pipe-tx}two';

        $this->redis->del([$key1, $key2]);

        $this->redis->clearTransferredBytes();
        $pipe = $this->redis->pipeline();
        $pipe->multi()
            ->set($key1, 'one')
            ->set($key2, 'two')
            ->mget([$key1, $key2])
            ->exec();

        $this->assertEquals([0, 0], $this->redis->getTransferredBytes());
        $ret = $pipe->exec();
        $this->assertEquals([[true, true, ['one', 'two']]], $ret);
    }

    public function testPipelineRejectsDuplicateMultiWithoutCorruptingBlock() {
        $key = '{pipe-duplicate-multi}key';

        $this->redis->del($key);
        $pipe = $this->redis->pipeline()->multi()->set($key, 'value');

        $this->assertFalse(@$pipe->multi());
        $this->assertEquals(Redis::PIPELINE, $pipe->getMode());

        $pipe->get($key)->exec();
        $this->assertEquals([[true, 'value']], $pipe->exec());
        $this->assertEquals('value', $this->redis->get($key));
        $this->assertEquals(Redis::ATOMIC, $this->redis->getMode());
    }

    public function testPipelineMultiWithOuterCommands() {
        $outer = '{pipe-outer}key';
        $txkey = '{pipe-tx}key';

        $this->redis->del([$outer, $txkey]);

        $ret = $this->redis->pipeline()
            ->set($outer, 'outer')
            ->multi()
                ->set($txkey, 'transaction')
                ->get($txkey)
                ->exec()
            ->get($outer)
            ->exec();

        $this->assertEquals(
            [true, [true, 'transaction'], 'outer'],
            $ret
        );
    }

    public function testPipelineMultiReplyFramingOnSameSocket() {
        $key = '{pipe-frame}key';
        $this->redis->del($key);

        $ret = $this->redis->pipeline()
            ->set($key, 0)
            ->get($key)
            ->multi()
                ->incr($key)
                ->get($key)
                ->exec()
            ->get($key)
            ->multi()
                ->incr($key)
                ->get($key)
                ->exec()
            ->get($key)
            ->exec();

        $this->assertEquals(
            [true, '0', [1, '1'], '1', [2, '2'], '2'],
            $ret
        );
    }

    public function testPipelineMultipleMultiBlocksAcrossSlots() {
        $key1 = '{pipeA}first-tx';
        $key2 = '{pipeB}second-tx';

        $this->redis->del([$key1, $key2]);

        $pipe = $this->redis->pipeline();
        $pipe->multi()->set($key1, 'one')->get($key1)->exec();
        $pipe->multi()->set($key2, 'two')->get($key2)->exec();

        $this->assertEquals(
            [[true, 'one'], [true, 'two']],
            $pipe->exec()
        );
    }

    public function testPipelineMultiKeylessCommandRouting() {
        $first = '{pipe-keyless-a}key';
        $second = '{pipe-keyless-b}key';
        $script = 'return 99';
        $sha = sha1($script);

        foreach ($this->redis->_masters() as $master) {
            $this->assertEquals(
                $sha,
                $this->redis->script($master, 'load', $script)
            );
        }

        $this->redis->del([$first, $second]);
        $pipe = $this->redis->pipeline();

        /* Leading keyless commands defer the block's socket selection until
         * the first key-derived command is seen. */
        $pipe->multi()
            ->eval('return 1')
            ->evalsha($sha)
            ->command('COUNT')
            ->set($first, 'one')
            ->get($first)
            ->exec();

        /* Once a keyed command binds the block, keyless commands inherit its
         * socket instead of selecting a new random slot. */
        $pipe->multi()
            ->set($second, 'two')
            ->eval('return 2')
            ->command('COUNT')
            ->get($second)
            ->exec();

        /* A wholly keyless block selects one socket for all its commands. */
        $pipe->multi()
            ->eval('return 3')
            ->evalsha($sha)
            ->command('COUNT')
            ->exec();

        $ret = $pipe->exec();

        $this->assertEquals(3, count($ret));
        $this->assertEquals([1, 99], array_slice($ret[0], 0, 2));
        $this->assertIsInt($ret[0][2]);
        $this->assertEquals([true, 'one'], array_slice($ret[0], 3));
        $this->assertEquals(true, $ret[1][0]);
        $this->assertEquals(2, $ret[1][1]);
        $this->assertIsInt($ret[1][2]);
        $this->assertEquals('two', $ret[1][3]);
        $this->assertEquals([3, 99], array_slice($ret[2], 0, 2));
        $this->assertIsInt($ret[2][2]);
        $this->assertEquals(Redis::ATOMIC, $this->redis->getMode());
        $this->assertEquals(['one', 'two'], $this->redis->mget([$first, $second]));
    }

    public function testPipelineDeferredKeylessBlockCleanup() {
        $key = '{pipe-keyless-cleanup}key';

        $pipe = $this->redis->pipeline()->multi()->eval('return 1');
        $this->assertTrue($pipe->discard());
        $this->assertEquals(Redis::ATOMIC, $pipe->getMode());
        $this->assertFalse(@$pipe->exec());

        $pipe->pipeline()->multi()->eval('return 2');
        $this->assertTrue($pipe->close());
        $this->assertEquals(Redis::ATOMIC, $pipe->getMode());
        $this->assertFalse(@$pipe->exec());
        $this->assertTrue($pipe->set($key, 'clean'));
        $this->assertEquals('clean', $pipe->get($key));
    }

    public function testPipelineMultiEmpty() {
        $ret = $this->redis->pipeline()->multi()->exec()->exec();
        $this->assertEquals([[]], $ret);
    }

    public function testPipelineEmptyMultiPreservesOuterResponseOrder() {
        [$first, $second] = $this->keysOnDistinctMasters(
            'pipe-empty-multi-order'
        );

        $this->redis->del([$first, $second]);
        $ret = $this->redis->pipeline()
            ->set($first, 'first')
            ->multi()->exec()
            ->get($first)
            ->multi()->exec()
            ->set($second, 'second')
            ->get($second)
            ->exec();

        $this->assertEquals(
            [true, [], 'first', [], true, 'second'],
            $ret
        );
    }

    public function testPipelineMultiDifferentSlotsThrows() {
        $key1 = '{pipeA}tx-one';
        $key2 = '{pipeB}tx-two';
        $outer = '{pipeC}tx-outer';
        $threw = false;

        $this->redis->del([$key1, $key2, $outer]);

        try {
            $this->redis->pipeline()
                ->set($outer, 'outer')
                ->multi()
                    ->eval('return 1')
                    ->set($key1, 'one')
                    ->set($key2, 'two');
        } catch (RedisClusterException $ex) {
            $threw = true;
            $this->assertStringContains('same hash slot', $ex->getMessage());
        }

        $this->assertTrue($threw);
        $this->assertEquals(Redis::ATOMIC, $this->redis->getMode());
        $this->assertEquals(
            [false, false, false],
            $this->redis->mget([$key1, $key2, $outer])
        );
    }

    public function testPipelineMultiRejectsCrossSlotDistributedCommands() {
        $keys = ['{pipeA}tx-dist-one', '{pipeB}tx-dist-two'];
        $values = [$keys[0] => 'one', $keys[1] => 'two'];
        $commands = [
            ['mget', [$keys]],
            ['mset', [$values]],
            ['msetnx', [$values]],
            ['del', [$keys]],
            ['unlink', [$keys]],
        ];

        $this->redis->del($keys);

        foreach ($commands as [$method, $args]) {
            $threw = false;

            try {
                $pipe = $this->redis->pipeline()->multi();
                $pipe->$method(...$args);
            } catch (RedisClusterException $ex) {
                $threw = true;
            }

            $this->assertTrue($threw);
            $this->assertEquals(Redis::ATOMIC, $this->redis->getMode());
        }

        $this->assertEquals([false, false], $this->redis->mget($keys));
    }

    public function testPipelineViaMultiEmpty() {
        $ret = $this->redis->multi(Redis::PIPELINE)->exec();
        $this->assertEquals([], $ret);
    }

    public function testPipelineMgetDifferentSlots() {
        $keys = ['{pipeA}one', '{pipeB}two'];

        $this->redis->mset([$keys[0] => 'one', $keys[1] => 'two']);
        $ret = $this->redis->pipeline()->mget($keys)->exec();

        $this->assertEquals([['one', 'two']], $ret);
    }

    public function testPipelineDirectedCommandThrows() {
        $key = '{pipe-directed-reject}key';
        $threw = false;
        $pipe = $this->redis->pipeline()->set($key, 'queued');

        try {
            $pipe->ping('{pipe}directed');
        } catch (RedisClusterException $ex) {
            $threw = true;
        } catch (Exception $ex) {
            $this->assert("Unexpected exception: {$ex}");
            return;
        }

        $this->assertTrue($threw);
        $this->assertEquals(Redis::PIPELINE, $pipe->getMode());
        $this->assertEquals([true, 'queued'], $pipe->get($key)->exec());
    }

    public function testPipelineCrossSlotMset() {
        $values = ['{pipeA}one' => '1', '{pipeB}two' => '2'];

        $this->redis->del(array_keys($values));
        $ret = $this->redis->pipeline()->mset($values)->exec();

        $this->assertEquals([true], $ret);
        $this->assertEquals(['1', '2'], $this->redis->mget(array_keys($values)));
    }

    public function testPipelineCrossSlotMsetnx() {
        $keys = ['{pipeA}nx-one', '{pipeB}nx-two'];
        $values = [$keys[0] => '1', $keys[1] => '2'];

        $this->redis->del(array_keys($values));
        $ret = $this->redis->pipeline()->msetnx($values)->exec();

        $this->assertEquals([[1, 1]], $ret);
        $this->assertEquals(['1', '2'], $this->redis->mget(array_keys($values)));

        /* Cross-slot MSETNX is intentionally per-node and non-atomic. */
        $this->redis->set($keys[0], 'existing');
        $this->redis->del($keys[1]);
        $ret = $this->redis->pipeline()
            ->msetnx([$keys[0] => 'ignored', $keys[1] => 'inserted'])
            ->exec();

        $this->assertEquals([[0, 1]], $ret);
        $this->assertEquals(
            ['existing', 'inserted'],
            $this->redis->mget($keys)
        );
    }

    public function testPipelineCrossSlotDelUnlink() {
        $keys = ['{pipeA}del-one', '{pipeB}del-two'];

        $this->redis->mset([$keys[0] => '1', $keys[1] => '2']);
        $ret = $this->redis->pipeline()->del($keys)->exec();
        $this->assertEquals([2], $ret);

        $this->redis->mset([$keys[0] => '1', $keys[1] => '2']);
        $ret = $this->redis->pipeline()->unlink($keys)->exec();
        $this->assertEquals([2], $ret);
    }

    public function testPipelineKeysThrows() {
        $key = '{pipe-keys-reject}key';
        $threw = false;
        $pipe = $this->redis->pipeline()->set($key, 'queued');

        try {
            $pipe->keys('*');
        } catch (RedisClusterException $ex) {
            $threw = true;
        } catch (Exception $ex) {
            $this->assert("Unexpected exception: {$ex}");
            return;
        }

        $this->assertTrue($threw);
        $this->assertEquals(Redis::PIPELINE, $pipe->getMode());
        $this->assertEquals([true, 'queued'], $pipe->get($key)->exec());
    }

    public function testPipelineWatchUnwatchThrows() {
        $commands = [
            'watch' => function () {
                return $this->redis->watch('{pipe}watch');
            },
            'unwatch' => function () {
                return $this->redis->unwatch();
            },
        ];

        foreach ($commands as $name => $command) {
            $key = "{pipe-$name-reject}key";
            $exception = NULL;
            $pipe = $this->redis->pipeline()->set($key, $name);

            try {
                $command();
            } catch (RedisClusterException $ex) {
                $exception = $ex;
            }

            $this->assertTrue($exception instanceof RedisClusterException);
            $this->assertEquals(Redis::PIPELINE, $pipe->getMode());
            $this->assertEquals([true, $name], $pipe->get($key)->exec());
        }
    }

    public function testPipelineSubscribeCommandsThrow() {
        $cb = function () {};
        $commands = [
            'subscribe' => function () use ($cb) {
                return $this->redis->subscribe(['chan'], $cb);
            },
            'psubscribe' => function () use ($cb) {
                return $this->redis->psubscribe(['chan*'], $cb);
            },
            'unsubscribe' => function () {
                return $this->redis->unsubscribe(['chan']);
            },
            'punsubscribe' => function () {
                return $this->redis->punsubscribe('chan*');
            },
        ];

        foreach ($commands as $name => $command) {
            $key = "{pipe-$name-reject}key";
            $exception = NULL;
            $pipe = $this->redis->pipeline()->set($key, $name);

            try {
                $command();
            } catch (RedisClusterException $ex) {
                $exception = $ex;
            }

            $this->assertTrue($exception instanceof RedisClusterException);
            $this->assertEquals(Redis::PIPELINE, $pipe->getMode());
            $this->assertEquals([true, $name], $pipe->get($key)->exec());
        }
    }

    public function testPipelineModeTransitions() {
        $this->assertEquals(Redis::ATOMIC, $this->redis->getMode());

        $this->redis->pipeline();
        $this->assertEquals(Redis::PIPELINE, $this->redis->getMode());
        $this->redis->exec();
        $this->assertEquals(Redis::ATOMIC, $this->redis->getMode());

        $warning = NULL;
        set_error_handler(function ($errno, $message) use (&$warning) {
            if ($errno === E_WARNING) $warning = $message;
            return true;
        });
        try {
            $this->redis->multi(Redis::PIPELINE);
        } finally {
            restore_error_handler();
        }
        $this->assertNull($warning);
        $this->assertEquals(Redis::PIPELINE, $this->redis->getMode());
        $this->assertTrue($this->redis->discard());
        $this->assertEquals(Redis::ATOMIC, $this->redis->getMode());

        $this->redis->pipeline()->multi();
        $this->assertEquals(Redis::PIPELINE, $this->redis->getMode());
        $this->redis->exec()->exec();
        $this->assertEquals(Redis::ATOMIC, $this->redis->getMode());

        /* Preserve RedisCluster's historical behavior for unsupported modes:
         * it warns but still enters regular MULTI mode. */
        $warning = NULL;
        set_error_handler(function ($errno, $message) use (&$warning) {
            if ($errno === E_WARNING) $warning = $message;
            return true;
        });
        try {
            $multi = $this->redis->multi(12345);
        } finally {
            restore_error_handler();
        }
        $this->assertTrue($multi === $this->redis);
        $this->assertStringContains('Unknown mode', $warning);
        $this->assertEquals(Redis::MULTI, $this->redis->getMode());
        $this->assertTrue($this->redis->discard());
        $this->assertEquals(Redis::ATOMIC, $this->redis->getMode());
    }

    public function testMultiModeTransitions() {
        $this->assertEquals(Redis::ATOMIC, $this->redis->getMode());

        $this->redis->multi();
        $this->assertEquals(Redis::MULTI, $this->redis->getMode());
        $this->assertTrue($this->redis->discard());
        $this->assertEquals(Redis::ATOMIC, $this->redis->getMode());
    }

    public function testPipelineCommandErrorLeavesSocketClean() {
        $key = '{pipe-error}key';
        $this->redis->set($key, 'not-a-list');

        $ret = $this->redis->pipeline()
            ->lpush($key, 'value')
            ->set($key, 'ok')
            ->get($key)
            ->exec();

        $this->assertEquals([false, true, 'ok'], $ret);

        /* A following command must not consume a stale pipeline response. */
        $this->assertEquals('ok', $this->redis->get($key));

        $ret = $this->redis->pipeline()
            ->set('{pipe}reset', 'ok')
            ->get('{pipe}reset')
            ->exec();
        $this->assertEquals([true, 'ok'], $ret);
    }

    public function testPipelineStructuredCommandErrorsReturnFalse() {
        $wrongType = '{pipe-structured-error}wrong-type';
        $control = '{pipe-structured-error}control';

        $this->redis->mset([
            $wrongType => 'not-a-stream',
            $control => 'still-readable',
        ]);

        $ret = $this->redis->pipeline()
            ->xrange($wrongType, '-', '+')
            ->xread([$wrongType => '0-0'])
            ->xinfo('STREAM', $wrongType)
            ->get($control)
            ->exec();

        $this->assertEquals(
            [false, false, false, 'still-readable'],
            $ret
        );
        $this->assertEquals('still-readable', $this->redis->get($control));

        $pipe = $this->redis->pipeline();
        $pipe->multi()
            ->xrange($wrongType, '-', '+')
            ->xread([$wrongType => '0-0'])
            ->xinfo('STREAM', $wrongType)
            ->get($control)
            ->exec();

        $this->assertEquals(
            [[false, false, false, 'still-readable']],
            $pipe->exec()
        );
        $this->assertEquals('still-readable', $this->redis->get($control));
    }

    public function testPipelineConditionalSetFalseDoesNotAbort() {
        $existing = '{pipe-valid-false}existing';
        $missing = '{pipe-valid-false}missing';
        $control = '{pipe-valid-false}control';

        $this->redis->del($missing);
        $this->redis->mset([
            $existing => 'old',
            $control => 'ok',
        ]);

        $ret = $this->redis->pipeline()
            ->set($existing, 'new', ['nx'])
            ->set($missing, 'new', ['xx'])
            ->get($control)
            ->exec();

        $this->assertEquals([false, false, 'ok'], $ret);

        $pipe = $this->redis->pipeline();
        $pipe->multi()
            ->set($existing, 'new', ['nx'])
            ->set($missing, 'new', ['xx'])
            ->get($control)
            ->exec();

        $this->assertEquals([[false, false, 'ok']], $pipe->exec());
    }

    public function testPipelineVectorNullRepliesDoNotAbort() {
        if (!$this->minVersionCheck('8.0')) {
            $this->markTestSkipped();
        }

        $missing = '{pipe-vector-null}missing';
        $control = '{pipe-vector-null}control';

        $this->redis->del($missing);
        $this->redis->set($control, 'ok');

        $ret = $this->redis->pipeline()
            ->vinfo($missing)
            ->vlinks($missing, 'element')
            ->vemb($missing, 'element')
            ->vemb($missing, 'element', true)
            ->get($control)
            ->exec();

        $this->assertEquals([false, false, false, false, 'ok'], $ret);
    }

    public function testPipelineMalformedReplyDisconnectsPersistentSocket() {
        $this->assertMalformedPipelineReplyAborts('pipeline');
    }

    public function testPipelinedMultiMalformedReplyDisconnectsPersistentSocket() {
        $this->assertMalformedPipelineReplyAborts('multi');
    }

    public function testPipelinedMultiFramingErrorDisconnectsPersistentSocket() {
        $this->assertMalformedPipelineReplyAborts(
            'multi-framing',
            'Error executing pipelined MULTI block'
        );
    }

    public function testPipelineClusterDownUsesClusterFailureSemantics() {
        [$key, $control] = $this->keysOnDistinctMasters('pipe-clusterdown');
        $script = "return redis.error_reply('CLUSTERDOWN simulated failure')";
        $exception = NULL;

        $this->redis->set($key, 'still-readable');
        $this->redis->del($control);

        try {
            $this->redis->pipeline()
                ->eval($script, [$key], 1)
                ->get($key)
                ->set($control, 'queued-on-other-node')
                ->get($control)
                ->exec();
        } catch (RedisClusterException $e) {
            $exception = $e;
        }

        $this->assertTrue($exception instanceof RedisClusterException);
        $this->assertStringContains(
            'The Redis Cluster is down (CLUSTERDOWN)',
            $exception->getMessage()
        );
        $this->assertEquals(Redis::ATOMIC, $this->redis->getMode());
        $this->assertEquals('still-readable', $this->redis->get($key));
        $this->assertEquals(
            'queued-on-other-node',
            $this->redis->get($control)
        );
    }

    public function testPipelinedMultiClusterDownUsesClusterFailureSemantics() {
        [$key, $control] = $this->keysOnDistinctMasters(
            'pipe-multi-clusterdown'
        );
        $script = "return redis.error_reply('CLUSTERDOWN simulated failure')";
        $exception = NULL;

        $this->redis->set($key, 'still-readable');
        $this->redis->del($control);
        $pipe = $this->redis->pipeline();
        $pipe->multi()
            ->eval($script, [$key], 1)
            ->get($key)
            ->exec();
        $pipe->set($control, 'queued-on-other-node')->get($control);

        try {
            $pipe->exec();
        } catch (RedisClusterException $e) {
            $exception = $e;
        }

        $this->assertTrue($exception instanceof RedisClusterException);
        $this->assertStringContains(
            'The Redis Cluster is down (CLUSTERDOWN)',
            $exception->getMessage()
        );
        $this->assertEquals(Redis::ATOMIC, $this->redis->getMode());
        $this->assertEquals('still-readable', $this->redis->get($key));
        $this->assertEquals(
            'queued-on-other-node',
            $this->redis->get($control)
        );
    }

    public function testPipelineMovedAbortsAndLeavesConnectionClean() {
        $this->assertPipelineRedirectAbortsAcrossNodes('MOVED');
    }

    public function testPipelineAskAbortsAndLeavesConnectionClean() {
        $this->assertPipelineRedirectAbortsAcrossNodes('ASK');
    }

    public function testPipelinedMultiRedirectsAbortAcrossNodes() {
        $this->assertPipelineRedirectAbortsAcrossNodes('MOVED', true);
        $this->assertPipelineRedirectAbortsAcrossNodes('ASK', true);
    }

    public function testPipelineTryAgainReturnsFalseAndContinues() {
        [$key, $control] = $this->keysOnDistinctMasters('pipe-tryagain');
        $script = "return redis.error_reply('TRYAGAIN simulated failure')";

        $this->redis->del([$key, $control]);
        $ret = $this->redis->pipeline()
            ->eval($script, [$key], 1)
            ->set($control, 'still-executed')
            ->get($control)
            ->eval($script, [$key], 1)
            ->exec();

        $this->assertEquals(
            [false, true, 'still-executed', false],
            $ret
        );
        $this->assertStringContains('TRYAGAIN', $this->redis->getLastError());
        $this->assertEquals(Redis::ATOMIC, $this->redis->getMode());
        $this->assertEquals('still-executed', $this->redis->get($control));
    }

    public function testPipelineDistributedCommandErrorsPreserveResults() {
        if ( ! $this->minVersionCheck('6.0')) {
            $this->markTestSkipped();
        }

        $user = 'phpredis-pipeline-' . uniqid();
        $pass = uniqid('secret-', true);
        $restricted = NULL;
        $masters = $this->redis->_masters();
        $keys = [
            '{pipeA}acl-mset',
            '{pipeB}acl-mset',
            '{pipeA}acl-msetnx',
            '{pipeB}acl-msetnx',
            '{pipeA}acl-del',
            '{pipeB}acl-del',
            '{pipeA}acl-unlink',
            '{pipeB}acl-unlink',
            '{pipeA}acl-control',
        ];

        try {
            foreach ($masters as $master) {
                $this->assertTrue($this->redis->acl(
                    $master, 'SETUSER', $user, 'reset', 'on', ">$pass", '~*',
                    '+@all', '-mset', '-msetnx', '-del', '-unlink'
                ));
            }

            $this->redis->del($keys);
            $this->redis->mset([
                $keys[4] => 'del-one',
                $keys[5] => 'del-two',
                $keys[6] => 'unlink-one',
                $keys[7] => 'unlink-two',
            ]);

            $restricted = new RedisCluster(
                NULL, self::$seeds, 1, 1, false, [$user, $pass]
            );

            $ret = $restricted->pipeline()
                ->mset([$keys[0] => 'one', $keys[1] => 'two'])
                ->msetnx([$keys[2] => 'one', $keys[3] => 'two'])
                ->del([$keys[4], $keys[5]])
                ->unlink([$keys[6], $keys[7]])
                ->set($keys[8], 'ok')
                ->get($keys[8])
                ->exec();

            $this->assertEquals(
                [false, [false, false], false, false, true, 'ok'],
                $ret
            );
            $this->assertEquals(
                ['del-one', 'del-two', 'unlink-one', 'unlink-two'],
                $this->redis->mget(array_slice($keys, 4, 4))
            );

            /* A command rejected while being queued in MULTI makes Redis
             * return EXECABORT.  The outer pipeline must fail without leaving
             * unread replies on the restricted connection. */
            $pipe = $restricted->pipeline();
            $pipe->multi()
                ->mset([$keys[0] => 'must-not-run'])
                ->get($keys[8])
                ->exec();

            $exception = NULL;
            try {
                $pipe->exec();
            } catch (RedisClusterException $e) {
                $exception = $e;
            }

            $this->assertTrue($exception instanceof RedisClusterException);
            $this->assertEquals(Redis::ATOMIC, $restricted->getMode());
            $this->assertFalse($this->redis->get($keys[0]));
            $this->assertEquals('ok', $restricted->get($keys[8]));

            /* Match standalone pipeline semantics when MULTI itself is
             * rejected: following buffered commands can run outside a
             * transaction.  The client must still abort and recover cleanly. */
            foreach ($masters as $master) {
                $this->assertTrue($this->redis->acl(
                    $master, 'SETUSER', $user, '-multi'
                ));
            }

            $pipe = $restricted->pipeline();
            $pipe->multi()->set($keys[0], 'ran-outside-multi')->exec();

            $exception = NULL;
            try {
                $pipe->exec();
            } catch (RedisClusterException $e) {
                $exception = $e;
            }

            $this->assertTrue($exception instanceof RedisClusterException);
            $this->assertEquals(Redis::ATOMIC, $restricted->getMode());
            $this->assertEquals(
                'ran-outside-multi', $this->redis->get($keys[0])
            );
            $this->assertEquals(
                'ran-outside-multi', $restricted->get($keys[0])
            );
        } finally {
            if ($restricted) {
                $restricted->close();
            }
            foreach ($masters as $master) {
                @$this->redis->acl($master, 'DELUSER', $user);
            }
            $this->redis->del($keys);
        }
    }

    public function testPipelineRejectsReentrantCommandsWhileReading() {
        $valueKey = '{pipe-reentrant}value';
        $sideEffectKey = '{pipe-reentrant-side}value';
        $redis = new RedisCluster(
            NULL, self::$seeds, 1, .25, false, $this->getAuth()
        );
        $exception = NULL;

        $redis->setOption(Redis::OPT_SERIALIZER, Redis::SERIALIZER_PHP);
        $redis->set($valueKey, new RedisClusterPipelineReentrantValue());
        $redis->del($sideEffectKey);

        RedisClusterPipelineReentrantValue::$redis = $redis;
        RedisClusterPipelineReentrantValue::$key = $sideEffectKey;

        try {
            $redis->pipeline()->get($valueKey)->get($valueKey)->exec();
        } catch (RedisClusterException $e) {
            $exception = $e;
        } finally {
            RedisClusterPipelineReentrantValue::$redis = NULL;
            RedisClusterPipelineReentrantValue::$key = NULL;
        }

        $this->assertTrue($exception instanceof RedisClusterException);
        $this->assertStringContains(
            'RedisCluster is already executing a pipeline',
            $exception->getMessage()
        );
        $this->assertEquals(Redis::ATOMIC, $redis->getMode());
        $this->assertFalse($redis->get($sideEffectKey));
        $this->assertTrue($redis->set($sideEffectKey, 'after'));
        $this->assertEquals('after', $redis->get($sideEffectKey));
        $redis->close();
    }

    public function testPipelineRejectsSetOptionWhileReading() {
        $key = '{pipe-reentrant-option}value';
        $redis = new RedisCluster(
            NULL, self::$seeds, 1, .25, false, $this->getAuth()
        );
        $exception = NULL;

        $redis->setOption(Redis::OPT_SERIALIZER, Redis::SERIALIZER_PHP);
        $redis->set($key, new RedisClusterPipelineSetOptionValue());
        RedisClusterPipelineSetOptionValue::$redis = $redis;

        try {
            $redis->pipeline()->get($key)->get($key)->exec();
        } catch (RedisClusterException $e) {
            $exception = $e;
        } finally {
            RedisClusterPipelineSetOptionValue::$redis = NULL;
        }

        $this->assertTrue($exception instanceof RedisClusterException);
        $this->assertStringContains(
            'RedisCluster is already executing a pipeline',
            $exception->getMessage()
        );
        $this->assertEquals(
            Redis::SERIALIZER_PHP,
            $redis->getOption(Redis::OPT_SERIALIZER)
        );
        $this->assertEquals(Redis::ATOMIC, $redis->getMode());
        $redis->close();
    }

    public function testPipelineRejectsReconstructionWithoutChangingState() {
        $key = '{pipe-reconstruct}key';
        $exception = NULL;
        $pipe = $this->redis->pipeline()->set($key, 'queued');

        try {
            $this->redis->__construct(
                NULL, self::$seeds, 1, 1, false, $this->getAuth()
            );
        } catch (RedisClusterException $e) {
            $exception = $e;
        }

        $this->assertTrue($exception instanceof RedisClusterException);
        $this->assertStringContains('pipeline is active', $exception->getMessage());
        $this->assertEquals(Redis::PIPELINE, $this->redis->getMode());
        $this->assertEquals([true, 'queued'], $pipe->get($key)->exec());
        $this->assertEquals(Redis::ATOMIC, $this->redis->getMode());
    }

    public function testPipelineRejectsWaitCommandsWithoutChangingState() {
        $commands = [
            ['wait', ['{pipe}wait', 0, 0]],
            ['waitaof', ['{pipe}waitaof', 0, 0, 0]],
        ];

        foreach ($commands as [$method, $args]) {
            $key = "{pipe-$method-reject}key";
            $exception = NULL;
            $pipe = $this->redis->pipeline()->set($key, $method);

            try {
                $this->redis->$method(...$args);
            } catch (RedisClusterException $e) {
                $exception = $e;
            }

            $this->assertTrue($exception instanceof RedisClusterException);
            $this->assertEquals(Redis::PIPELINE, $this->redis->getMode());
            $this->assertEquals([true, $method], $pipe->get($key)->exec());
            $this->assertEquals(Redis::ATOMIC, $this->redis->getMode());
        }
    }

    public function testPipelineMultiCommandErrorLeavesSocketClean() {
        $wrongType = '{pipe-tx-error}wrong-type';
        $valueKey = '{pipe-tx-error}value';
        $this->redis->set($wrongType, 'not-a-list');

        $pipe = $this->redis->pipeline();
        $pipe->multi()
            ->lpush($wrongType, 'value')
            ->set($valueKey, 'ok')
            ->get($valueKey)
            ->exec();

        $this->assertEquals([[false, true, 'ok']], $pipe->exec());
        $this->assertEquals('ok', $this->redis->get($valueKey));
    }

    public function testPipelineActivationRejectedWhileWatchIsActive() {
        $key = '{pipe-watch-reject}key';
        $other = $this->getNewInstance();
        $activations = [
            function () { return $this->redis->pipeline(); },
            function () { return $this->redis->multi(Redis::PIPELINE); },
        ];

        foreach ($activations as $i => $activate) {
            $this->redis->set($key, "before-$i");
            $this->assertTrue($this->redis->watch($key));

            $exception = NULL;
            try {
                $activate();
            } catch (RedisClusterException $e) {
                $exception = $e;
            }

            $this->assertTrue($exception instanceof RedisClusterException);
            $this->assertStringContains('WATCH is active', $exception->getMessage());
            $this->assertEquals(Redis::ATOMIC, $this->redis->getMode());

            /* Rejection must not consume WATCH or dirty the connection. */
            $other->set($key, "changed-$i");
            $this->assertEquals(
                [false],
                $this->redis->multi()->set($key, "must-not-run-$i")->exec()
            );
            $this->assertEquals("changed-$i", $this->redis->get($key));
            $this->assertEquals(Redis::ATOMIC, $this->redis->getMode());
        }

        $other->close();
    }

    public function testPipelineStartsAfterUnwatch() {
        $key = '{pipe-watch-unwatch}key';
        $exception = NULL;

        $this->redis->set($key, 'before');
        $this->assertTrue($this->redis->watch($key));

        try {
            $this->redis->pipeline();
        } catch (RedisClusterException $e) {
            $exception = $e;
        }

        $this->assertTrue($exception instanceof RedisClusterException);
        $this->assertEquals(Redis::ATOMIC, $this->redis->getMode());
        $this->assertTrue($this->redis->unwatch());
        $this->assertEquals(
            [true, 'after'],
            $this->redis->pipeline()->set($key, 'after')->get($key)->exec()
        );
    }

    public function testPipelineRejectedWithWatchesOnMultipleNodes() {
        $keysByMaster = [];

        for ($i = 0; $i < 64 && count($keysByMaster) < 2; $i++) {
            $key = "{pipe-watch-node-$i}key";
            $master = $this->redis->cluster($key, 'MYID');
            $keysByMaster[$master] = $key;
        }

        $keys = array_values($keysByMaster);
        $this->assertTrue(count($keys) >= 2);
        $this->assertTrue($this->redis->watch($keys[0], $keys[1]));

        $exception = NULL;
        try {
            $this->redis->multi(Redis::PIPELINE);
        } catch (RedisClusterException $e) {
            $exception = $e;
        }

        $this->assertTrue($exception instanceof RedisClusterException);
        $this->assertStringContains('WATCH is active', $exception->getMessage());
        $this->assertEquals(Redis::ATOMIC, $this->redis->getMode());
        $this->assertTrue($this->redis->unwatch());
        $this->assertTrue($this->redis->set($keys[0], 'clean'));
        $this->assertEquals('clean', $this->redis->get($keys[0]));
    }

    public function testCloseDiscardsQueuedPipeline() {
        $keys = [
            '{pipe-close-a}key',
            '{pipe-close-b}key',
        ];

        $this->redis->del($keys);
        $pipe = $this->redis->pipeline()
            ->mset([$keys[0] => 'one', $keys[1] => 'two']);

        $this->assertTrue($pipe->close());
        $this->assertEquals(Redis::ATOMIC, $pipe->getMode());
        $this->assertFalse(@$pipe->exec());
        $this->assertEquals([false, false], $pipe->mget($keys));
    }

    public function testCloseDiscardsOpenPipelinedMulti() {
        $key = '{pipe-close-open-multi}key';

        $this->redis->del($key);
        $pipe = $this->redis->pipeline()->multi()->set($key, 'must-not-run');

        $this->assertTrue($pipe->close());
        $this->assertEquals(Redis::ATOMIC, $pipe->getMode());
        $this->assertFalse(@$pipe->exec());
        $this->assertFalse($pipe->get($key));
    }

    public function testExecOutsideMultiPipeline() {
        $this->assertFalse(@$this->redis->exec());
    }

    public function testExecAfterPipelineMultiSlotErrorIsFalse() {
        $threw = false;

        try {
            $this->redis->pipeline()->multi()
                ->set('{pipeA}key1', '1')
                ->set('{pipeB}key2', '2');
        } catch (RedisClusterException $ex) {
            $threw = true;
        }

        $this->assertTrue($threw);
        $this->assertFalse(@$this->redis->exec());
    }

    public function testRandomKey() {
        /* Ensure some keys are present to test */
        for ($i = 0; $i < 1000; $i++) {
            if (rand(1, 2) == 1) {
                $this->redis->set("key:$i", "val:$i");
            }
        }

        for ($i = 0; $i < 1000; $i++) {
            $k = $this->redis->randomKey("key:$i");
            $this->assertEquals(1, $this->redis->exists($k));
        }
    }

    public function testEcho() {
        $this->assertEquals('hello', $this->redis->echo('echo1', 'hello'));
        $this->assertEquals('world', $this->redis->echo('echo2', 'world'));
        $this->assertEquals(' 0123 ', $this->redis->echo('echo3', " 0123 "));
    }

    public function testSortPrefix() {
        $this->redis->setOption(Redis::OPT_PREFIX, 'some-prefix:');
        $this->redis->del('some-item');
        $this->redis->sadd('some-item', 1);
        $this->redis->sadd('some-item', 2);
        $this->redis->sadd('some-item', 3);

        $this->assertEquals(['1', '2', '3'], $this->redis->sort('some-item'));

        // Kill our set/prefix
        $this->redis->del('some-item');
        $this->redis->setOption(Redis::OPT_PREFIX, '');
    }

    public function testDBSize() {
        for ($i = 0; $i < 10; $i++) {
            $key = "key:$i";
            $this->assertTrue($this->redis->flushdb($key));
            $this->redis->set($key, "val:$i");
            $this->assertEquals(1, $this->redis->dbsize($key));
        }
    }

    /* Regression test for GH #2890 */
    public function testDirectedFlushUsesMaster() {
        $master = $this->redis->_masters()[0];

        $this->assertTrue(
            $this->redis->setOption(
                RedisCluster::OPT_SLAVE_FAILOVER,
                RedisCluster::FAILOVER_DISTRIBUTE_SLAVES
            )
        );

        /* Should succeed being sent to the primary */
        $this->assertTrue($this->redis->flushdb($master));
        $this->assertTrue($this->redis->flushall($master));

        $this->assertTrue(
            $this->redis->setOption(
                RedisCluster::OPT_SLAVE_FAILOVER,
                RedisCluster::FAILOVER_NONE
            )
        );
    }

    /* Regression test for setting TCP_KEEPALIVE on the hostless cluster flags socket */
    public function testSetTcpKeepaliveOption() {
        $this->assertTrue(
            $this->redis->setOption(Redis::OPT_TCP_KEEPALIVE, true)
        );
    }

    /* Regression test for directed commands in MULTI mode */
    public function testDirectedCommandsInMulti() {
        $key = __METHOD__;

        $result = $this->redis
            ->multi()
            ->flushdb($key)
            ->dbsize($key)
            ->flushall($key)
            ->exec();

        $this->assertIsArray($result, 3);
        $this->assertTrue($result[0]);
        $this->assertIsInt($result[1]);
        $this->assertTrue($result[2]);
    }

    public function testInfo() {
        $fields = [
            "redis_version", "arch_bits", "uptime_in_seconds", "uptime_in_days",
            "connected_clients", "connected_slaves", "used_memory",
            "total_connections_received", "total_commands_processed",
            "role"
        ];

        for ($i = 0; $i < 3; $i++) {
            $info = $this->redis->info($i);
            foreach ($fields as $field) {
                $this->assertArrayKey($info, $field);
            }
        }
    }

    public function testClient() {
        $key = 'key-' . rand(1, 100);

        $this->assertTrue($this->redis->client($key, 'setname', 'cluster_tests'));

        $clients = $this->redis->client($key, 'list');
        $this->assertIsArray($clients);

        /* Find us in the list */
        $addr = NULL;
        foreach ($clients as $client) {
            if ($client['name'] == 'cluster_tests') {
                $addr = $client['addr'];
                break;
            }
        }

        /* We should be in there */
        $this->assertIsString($addr);

        /* Kill our own client! */
        $this->assertTrue($this->redis->client($key, 'kill', $addr));
    }

    public function testTime() {
        [$sec, $usec] = $this->redis->time(uniqid());
        $this->assertEquals(strval(intval($sec)), strval($sec));
        $this->assertEquals(strval(intval($usec)), strval($usec));
    }

    public function testExpireAt() {
        $this->redis->del('key');
        $this->redis->set('key', 'value');

        $now = $this->redis->time('key');
        $this->assertTrue($this->redis->expireAt('key', $now[0] + 10));
        $this->assertLTE(10, $this->redis->ttl('key'));

        $this->redis->del('key');
    }

    public function testScan() {
        $key_count = 0;
        $scan_count = 0;

        /* Have scan retry for us */
        $this->redis->setOption(Redis::OPT_SCAN, Redis::SCAN_RETRY);

        /* Iterate over our masters, scanning each one */
        foreach ($this->redis->_masters() as $master) {
            /* Grab the number of keys we have */
            $key_count += $this->redis->dbsize($master);

            /* Scan the keys here */
            $it = NULL;
            while ($keys = $this->redis->scan($it, $master)) {
                $scan_count += count($keys);
            }
        }

        /* Our total key count should match */
        $this->assertEquals($scan_count, $key_count);
    }

    public function testScanPrefix() {
        $prefixes = ['prefix-a:', 'prefix-b:'];
        $id = uniqid();

        $arr_keys = [];
        foreach ($prefixes as $prefix) {
            $this->redis->setOption(Redis::OPT_PREFIX, $prefix);
            $this->redis->set($id, "LOLWUT");
            $arr_keys[$prefix] = $id;
        }

        $this->redis->setOption(Redis::OPT_SCAN, Redis::SCAN_RETRY);
        $this->redis->setOption(Redis::OPT_SCAN, Redis::SCAN_PREFIX);

        foreach ($prefixes as $prefix) {
            $prefix_keys = [];
            $this->redis->setOption(Redis::OPT_PREFIX, $prefix);

            foreach ($this->redis->_masters() as $master) {
                $it = NULL;
                while ($keys = $this->redis->scan($it, $master, "*$id*")) {
                    foreach ($keys as $key) {
                        $prefix_keys[$prefix] = $key;
                    }
                }
            }

            $this->assertIsArray($prefix_keys, 1);
            $this->assertArrayKey($prefix_keys, $prefix);
        }

        $this->redis->setOption(Redis::OPT_SCAN, Redis::SCAN_NOPREFIX);

        $scan_keys = [];

        foreach ($this->redis->_masters() as $master) {
            $it = NULL;
            while ($keys = $this->redis->scan($it, $master, "*$id*")) {
                foreach ($keys as $key) {
                    $scan_keys[] = $key;
                }
            }
        }

        /* We should now have both prefixs' keys */
        foreach ($arr_keys as $prefix => $id) {
            $this->assertInArray("{$prefix}{$id}", $scan_keys);
        }
    }

    // Run some simple tests against the PUBSUB command.  This is problematic, as we
    // can't be sure what's going on in the instance, but we can do some things.
    public function testPubSub() {
        // PUBSUB CHANNELS ...
        $result = $this->redis->pubsub("somekey", "channels", "*");
        $this->assertIsArray($result);
        $result = $this->redis->pubsub("somekey", "channels");
        $this->assertIsArray($result);

        // PUBSUB NUMSUB

        $c1 = '{pubsub}-' . rand(1, 100);
        $c2 = '{pubsub}-' . rand(1, 100);

        $result = $this->redis->pubsub("{pubsub}", "numsub", $c1, $c2);

        // Should get an array back, with two elements
        $this->assertIsArray($result);
        $this->assertEquals(4, count($result));

        $zipped = [];
        for ($i = 0; $i <= count($result) / 2; $i += 2) {
            $zipped[$result[$i]] = $result[$i+1];
        }
        $result = $zipped;

        // Make sure the elements are correct, and have zero counts
        foreach([$c1,$c2] as $channel) {
            $this->assertArrayKey($result, $channel);
            $this->assertEquals(0, $result[$channel]);
        }

        // PUBSUB NUMPAT
        $result = $this->redis->pubsub("somekey", "numpat");
        $this->assertIsInt($result);

        // Invalid call
        $this->assertFalse($this->redis->pubsub("somekey", "notacommand"));
    }

    /* Unlike Redis proper, MsetNX won't always totally fail if all keys can't
     * be set, but rather will only fail per-node when that is the case */
    public function testMSetNX() {
        /* All of these keys should get set */
        $this->redis->del('x', 'y', 'z');
        $ret = $this->redis->msetnx(['x'=>'a', 'y'=>'b', 'z'=>'c']);
        $this->assertIsArray($ret);
        $this->assertEquals(array_sum($ret),count($ret));

        /* Delete one key */
        $this->redis->del('x');
        $ret = $this->redis->msetnx(['x'=>'a', 'y'=>'b', 'z'=>'c']);
        $this->assertIsArray($ret);
        $this->assertEquals(1, array_sum($ret));

        $this->assertFalse($this->redis->msetnx([])); // set ø → FALSE
    }

    /* Slowlog needs to take a key or [ip, port], to direct it to a node */
    public function testSlowlog() {
        $key = uniqid() . '-' . rand(1, 1000);

        $this->assertIsArray($this->redis->slowlog($key, 'get'));
        $this->assertIsArray($this->redis->slowlog($key, 'get', 10));
        $this->assertIsInt($this->redis->slowlog($key, 'len'));
        $this->assertTrue($this->redis->slowlog($key, 'reset'));
        $this->assertFalse(@$this->redis->slowlog($key, 'notvalid'));
    }

    /* INFO COMMANDSTATS requires a key or ip:port for node direction */
    public function testInfoCommandStats() {
        $info = $this->redis->info(uniqid(), "COMMANDSTATS");

        $this->assertIsArray($info);
        if (is_array($info)) {
            foreach($info as $k => $value) {
                $this->assertStringContains('cmdstat_', $k);
            }
        }
    }

    /* RedisCluster will always respond with an array, even if transactions
     * failed, because the commands could be coming from multiple nodes */
    public function testFailedTransactions() {
        $this->redis->set('x', 42);

        // failed transaction
        $this->redis->watch('x');

        $r = $this->newInstance(); // new instance, modifying `x'.
        $r->incr('x');

        // This transaction should fail because the other client changed 'x'
        $ret = $this->redis->multi()->get('x')->exec();
        $this->assertEquals([false], $ret);
        // watch and unwatch
        $this->redis->watch('x');
        $r->incr('x'); // other instance
        $this->redis->unwatch(); // cancel transaction watch

        // This should succeed as the watch has been cancelled
        $ret = $this->redis->multi()->get('x')->exec();
        $this->assertEquals(['44'], $ret);
    }

    /* UNWATCH cannot be issued after entering MULTI mode */
    public function testUnwatchInMulti() {
        $key = __METHOD__;

        $this->redis->multi()->set($key, 'value');
        $this->assertFalse(@$this->redis->unwatch());
        $this->assertEquals([true], $this->redis->exec());
    }

    public function testDiscard() {
        $this->redis->multi();
        $this->redis->set('pipecount', 'over9000');
        $this->redis->get('pipecount');

        $this->assertTrue($this->redis->discard());
    }

    /* RedisCluster::script() is a 'raw' command, which requires a key such that
     * we can direct it to a given node */
    public function testScript() {
        $key = uniqid() . '-' . rand(1, 1000);

        // Flush any scripts we have
        $this->assertTrue($this->redis->script($key, 'flush'));

        // Silly scripts to test against
        $s1_src = 'return 1';
        $s1_sha = sha1($s1_src);
        $s2_src = 'return 2';
        $s2_sha = sha1($s2_src);
        $s3_src = 'return 3';
        $s3_sha = sha1($s3_src);

        // None should exist
        $result = $this->redis->script($key, 'exists', $s1_sha, $s2_sha, $s3_sha);
        $this->assertIsArray($result, 3);
        $this->assertTrue(is_array($result) && count(array_filter($result)) == 0);

        // Load them up
        $this->assertEquals($s1_sha, $this->redis->script($key, 'load', $s1_src));
        $this->assertEquals($s2_sha, $this->redis->script($key, 'load', $s2_src));
        $this->assertEquals($s3_sha, $this->redis->script($key, 'load', $s3_src));

        // They should all exist
        $result = $this->redis->script($key, 'exists', $s1_sha, $s2_sha, $s3_sha);
        $this->assertTrue(is_array($result) && count(array_filter($result)) == 3);
    }

    /* RedisCluster::EVALSHA needs a 'key' to let us know which node we want to
     * direct the command at */
    public function testEvalSHA() {
        $key = uniqid() . '-' . rand(1, 1000);

        // Flush any loaded scripts
        $this->redis->script($key, 'flush');

        // Non existent script (but proper sha1), and a random (not) sha1 string
        $this->assertFalse($this->redis->evalsha(sha1(uniqid()),[$key], 1));
        $this->assertFalse($this->redis->evalsha('some-random-data'),[$key], 1);

        // Load a script
        $cb  = uniqid(); // To ensure the script is new
        $scr = "local cb='$cb' return 1";
        $sha = sha1($scr);

        // Run it when it doesn't exist, run it with eval, and then run it with sha1
        $this->assertFalse($this->redis->evalsha($scr,[$key], 1));
        $this->assertEquals(1, $this->redis->eval($scr,[$key], 1));
        $this->assertEquals(1, $this->redis->evalsha($sha,[$key], 1));
    }

    public function testEvalBulkResponse() {
        $key1 = uniqid() . '-' . rand(1, 1000) . '{hash}';
        $key2 = uniqid() . '-' . rand(1, 1000) . '{hash}';

        $this->redis->script($key1, 'flush');
        $this->redis->script($key2, 'flush');

        $scr = "return {KEYS[1],KEYS[2]}";

        $result = $this->redis->eval($scr,[$key1, $key2], 2);

        $this->assertEquals($key1, $result[0]);
        $this->assertEquals($key2, $result[1]);
    }

    public function testEvalBulkResponseMulti() {
        $key1 = uniqid() . '-' . rand(1, 1000) . '{hash}';
        $key2 = uniqid() . '-' . rand(1, 1000) . '{hash}';

        $this->redis->script($key1, 'flush');
        $this->redis->script($key2, 'flush');

        $scr = "return {KEYS[1],KEYS[2]}";

        $this->redis->multi();
        $this->redis->eval($scr, [$key1, $key2], 2);

        $result = $this->redis->exec();

        $this->assertEquals($key1, $result[0][0]);
        $this->assertEquals($key2, $result[0][1]);
    }

    public function testEvalBulkEmptyResponse() {
        $key1 = uniqid() . '-' . rand(1, 1000) . '{hash}';
        $key2 = uniqid() . '-' . rand(1, 1000) . '{hash}';

        $this->redis->script($key1, 'flush');
        $this->redis->script($key2, 'flush');

        $scr = "for _,key in ipairs(KEYS) do redis.call('SET', key, 'value') end";

        $result = $this->redis->eval($scr, [$key1, $key2], 2);

        $this->assertNull($result);
    }

    public function testEvalBulkEmptyResponseMulti() {
        $key1 = uniqid() . '-' . rand(1, 1000) . '{hash}';
        $key2 = uniqid() . '-' . rand(1, 1000) . '{hash}';

        $this->redis->script($key1, 'flush');
        $this->redis->script($key2, 'flush');

        $scr = "for _,key in ipairs(KEYS) do redis.call('SET', key, 'value') end";

        $this->redis->multi();
        $this->redis->eval($scr, [$key1, $key2], 2);
        $result = $this->redis->exec();

        $this->assertNull($result[0]);
    }

    /* Cluster specific introspection stuff */
    public function testIntrospection() {
        $primaries = $this->redis->_masters();
        $this->assertIsArray($primaries);

        foreach ($primaries as [$host, $port]) {
            $this->assertIsString($host);
            $this->assertIsInt($port);
        }
    }

    protected function keyTypeToString($key_type) {
        switch ($key_type) {
            case Redis::REDIS_STRING:
                return "string";
            case Redis::REDIS_SET:
                return "set";
            case Redis::REDIS_LIST:
                return "list";
            case Redis::REDIS_ZSET:
                return "zset";
            case Redis::REDIS_HASH:
                return "hash";
            case Redis::REDIS_STREAM:
                return "stream";
            case Redis::REDIS_VECTORSET:
                return "vectorset";
            default:
                return "unknown($key_type)";
        }

    }

    protected function genKeyName($key_index, $key_type) {
        return sprintf('%s-%s', $this->keyTypeToString($key_type), $key_index);
    }

    protected function setKeyVals($key_index, $key_type, &$arr_ref) {
        $key = $this->genKeyName($key_index, $key_type);

        $this->redis->del($key);

        switch ($key_type) {
            case Redis::REDIS_STRING:
                $value = "$key-value";
                $this->redis->set($key, $value);
                break;
            case Redis::REDIS_SET:
                $value = [
                    "$key-mem1", "$key-mem2", "$key-mem3",
                    "$key-mem4", "$key-mem5", "$key-mem6"
                ];
                $args = $value;
                array_unshift($args, $key);
                call_user_func_array([$this->redis, 'sadd'], $args);
                break;
            case Redis::REDIS_HASH:
                $value = [
                    "$key-mem1" => "$key-val1",
                    "$key-mem2" => "$key-val2",
                    "$key-mem3" => "$key-val3"
                ];
                $this->redis->hmset($key, $value);
                break;
            case Redis::REDIS_LIST:
                $value = [
                    "$key-ele1", "$key-ele2", "$key-ele3",
                    "$key-ele4", "$key-ele5", "$key-ele6"
                ];
                $args = $value;
                array_unshift($args, $key);
                call_user_func_array([$this->redis, 'rpush'], $args);
                break;
            case Redis::REDIS_ZSET:
                $score = 1;
                $value = [
                    "$key-mem1" => 1, "$key-mem2" => 2,
                    "$key-mem3" => 3, "$key-mem3" => 3
                ];
                foreach ($value as $mem => $score) {
                    $this->redis->zadd($key, $score, $mem);
                }
                break;
        }

        /* Update our reference array so we can verify values */
        $arr_ref[$key] = $value;

        return $key;
    }

    /* Verify that our ZSET values are identical */
    protected function checkZSetEquality($a, $b) {
        /* If the count is off, the array keys are different or the sums are
         * different, we know there is something off */
        $boo_diff = count($a) != count($b) ||
            count(array_diff(array_keys($a), array_keys($b))) != 0 ||
            array_sum($a) != array_sum($b);

        if ($boo_diff) {
            $this->assertEquals($a, $b);
            return;
        }
    }

    protected function checkKeyValue($key, $key_type, $value) {
        switch ($key_type) {
            case Redis::REDIS_STRING:
                $this->assertEquals($value, $this->redis->get($key));
                break;
            case Redis::REDIS_SET:
                $arr_r_values = $this->redis->sMembers($key);
                $arr_l_values = $value;
                sort($arr_r_values);
                sort($arr_l_values);
                $this->assertEquals($arr_r_values, $arr_l_values);
                break;
            case Redis::REDIS_LIST:
                $this->assertEquals($value, $this->redis->lrange($key, 0, -1));
                break;
            case Redis::REDIS_HASH:
                $this->assertEquals($value, $this->redis->hgetall($key));
                break;
            case Redis::REDIS_ZSET:
                $this->checkZSetEquality($value, $this->redis->zrange($key, 0, -1, true));
                break;
            default:
                throw new Exception("Unknown type " . $key_type);
        }
    }

    /* Test automatic load distributor */
    public function testFailOver() {
        $value_ref = [];
        $type_ref  = [];

        /* Set a bunch of keys of various redis types*/
        for ($i = 0; $i < 200; $i++) {
            foreach ($this->redis_types as $type) {
                $key = $this->setKeyVals($i, $type, $value_ref);
                $type_ref[$key] = $type;
            }
        }

        /* Iterate over failover options */
        foreach ($this->failover_types as $failover_type) {
            $this->redis->setOption(RedisCluster::OPT_SLAVE_FAILOVER, $failover_type);

            foreach ($value_ref as $key => $value) {
                $this->checkKeyValue($key, $type_ref[$key], $value);
            }

            break;
        }
    }

    /* Test a 'raw' command */
    public function testRawCommand() {
        $this->redis->rawCommand('mykey', 'set', 'mykey', 'my-value');
        $this->assertEquals('my-value', $this->redis->get('mykey'));

        $this->redis->del('mylist');
        $this->redis->rpush('mylist', 'A', 'B', 'C', 'D');
        $this->assertEquals(['A', 'B', 'C', 'D'], $this->redis->lrange('mylist', 0, -1));
    }

    protected function rawCommandArray($key, $args) {
        array_unshift($args, $key);
        return call_user_func_array([$this->redis, 'rawCommand'], $args);
    }

    /* Test that rawCommand and EVAL can be configured to return simple string values */
    public function testReplyLiteral() {
        $this->redis->setOption(Redis::OPT_REPLY_LITERAL, false);
        $this->assertTrue($this->redis->rawCommand('foo', 'set', 'foo', 'bar'));
        $this->assertTrue($this->redis->eval("return redis.call('set', KEYS[1], 'bar')", ['foo'], 1));

        $rv = $this->redis->eval("return {redis.call('set', KEYS[1], 'bar'), redis.call('ping')}", ['foo'], 1);
        $this->assertEquals([true, true], $rv);

        $this->redis->setOption(Redis::OPT_REPLY_LITERAL, true);
        $this->assertEquals('OK', $this->redis->rawCommand('foo', 'set', 'foo', 'bar'));
        $this->assertEquals('OK', $this->redis->eval("return redis.call('set', KEYS[1], 'bar')", ['foo'], 1));

        $rv = $this->redis->eval("return {redis.call('set', KEYS[1], 'bar'), redis.call('ping')}", ['foo'], 1);
        $this->assertEquals(['OK', 'PONG'], $rv);

        // Reset
        $this->redis->setOption(Redis::OPT_REPLY_LITERAL, false);
    }

    /* Redis and RedisCluster use the same handler for the ACL command but verify we can direct
       the command to a specific node. */
    public function testAcl() {
        if ( ! $this->minVersionCheck("6.0"))
            $this->markTestSkipped();

        $this->assertInArray('default', $this->redis->acl('foo', 'USERS'));
    }

    public function testSession()
    {
        @ini_set('session.save_handler', 'rediscluster');
        @ini_set('session.save_path', $this->sessionSavePath() . '&failover=error');

        if ( ! @session_start())
            $this->markTestSkipped();

        session_write_close();

        $this->assertKeyExists($this->sessionPrefix() . session_id());
    }


    /* Test that we are able to use the slot cache without issues */
    public function testSlotCache() {
        ini_set('redis.clusters.cache_slots', 1);

        $pong = 0;
        for ($i = 0; $i < 10; $i++) {
            $new_client = $this->newInstance();
            $pong += $new_client->ping("key:$i");
        }

        $this->assertEquals($pong, $i);

        ini_set('redis.clusters.cache_slots', 0);
    }

    /* Regression test for connection pool liveness checks */
    public function testConnectionPool() {
        $prev_value = ini_get('redis.pconnect.pooling_enabled');
        ini_set('redis.pconnect.pooling_enabled', 1);

        $pong = 0;
        for ($i = 0; $i < 10; $i++) {
            $new_client = $this->newInstance();
            $pong += $new_client->ping("key:$i");
        }

        $this->assertEquals($pong, $i);
        ini_set('redis.pconnect.pooling_enabled', $prev_value);
    }

    protected function sessionPrefix(): string {
        return 'PHPREDIS_CLUSTER_SESSION:';
    }

    protected function sessionSaveHandler(): string {
        return 'rediscluster';
    }

    /**
     * @inheritdoc
     */
    protected function sessionSavePath(): string {
        return implode('&', array_map(function ($host) {
            return 'seed[]=' . $host;
        }, self::$seeds)) . '&' . $this->getAuthFragment();
    }

    /* Test correct handling of null multibulk replies */
    public function testNullArray() {
        $key = "key:arr";
        $this->redis->del($key);

        foreach ([false => [], true => NULL] as $opt => $test) {
            $this->redis->setOption(Redis::OPT_NULL_MULTIBULK_AS_NULL, $opt);

            $r = $this->redis->rawCommand($key, "BLPOP", $key, .05);
            $this->assertEquals($test, $r);

            $this->redis->multi();
            $this->redis->rawCommand($key, "BLPOP", $key, .05);
            $r = $this->redis->exec();
            $this->assertEquals([$test], $r);
        }

        $this->redis->setOption(Redis::OPT_NULL_MULTIBULK_AS_NULL, false);
    }

    protected function execWaitAOF() {
        return $this->redis->waitaof(uniqid(), 0, 0, 0);
    }
}
?>
