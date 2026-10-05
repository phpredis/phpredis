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

class RedisClusterPipelineReentrantControlValue {
    public static $redis;
    public static $method;

    public function __wakeup() {
        self::$redis->{self::$method}();
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

    private function startMalformedReplyServer($scenario, $slot = null) {
        if (!function_exists('proc_open')) {
            $this->markTestSkipped('proc_open is required');
        }

        $process = proc_open(
            [PHP_BINARY, '-n', __DIR__ . '/RedisClusterMalformedReplyServer.php',
                $scenario, (string)$slot],
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
            /* Check abort cleanup before a new pipeline can refresh the map. */
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
                ->strlen($key)
                ->exec();
        } else {
            $pipe->eval($script, [$key], 1)->strlen($key);
        }
        $pipe->set($control, 'queued-on-other-node')->strlen($control);

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
        $this->assertEquals(
            ['still-readable', 'queued-on-other-node'],
            $this->redis->pipeline()->get($key)->get($control)->exec()
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

    public function testNonPipelineDistributedMultiDiscardAndDestruction() {
        [$a, $b] = $this->keysOnDistinctMasters('legacy-dist-cleanup');
        $new = "$a:new";
        $this->redis->mset([$a => 'A', $b => 'B']);
        $this->redis->del($new);

        foreach (['discard', 'destroy'] as $cleanup) {
            $redis = $this->newInstance();
            $redis->multi()->mget([$a, $b])->mset([$a => 'changed', $b => 'changed'])
                ->msetnx([$new => 'queued', "$b:new" => 'queued'])->del([$a, $b]);
            if ($cleanup === 'discard') {
                $this->assertTrue($redis->discard());
                $this->assertEquals(['A', 'B'], $redis->mget([$a, $b]));
                $redis->close();
            }
            unset($redis);
            $this->assertEquals(['A', 'B'], $this->redis->mget([$a, $b]));
            $this->assertFalse($this->redis->get($new));
            $this->assertFalse($this->redis->get("$b:new"));
        }
        $this->redis->del([$a, $b]);
    }

    public function testNonPipelineDistributedMgetLastChunkWatchAbort() {
        [$a, $b] = $this->keysOnDistinctMasters('legacy-dist-watch');
        $other = $this->newInstance();
        try {
            $this->redis->mset([$a => 'A', $b => 'B']);
            $this->redis->watch($b);
            $other->set($b, 'changed');
            $this->assertEquals([false, 'A'],
                $this->redis->multi()->mget([$a, $b])->get($a)->exec());
            $this->assertEquals(Redis::ATOMIC, $this->redis->getMode());
            $this->assertEquals(['A', 'changed'], $this->redis->mget([$a, $b]));
        } finally {
            $other->close();
            $this->redis->unwatch();
            $this->redis->del([$a, $b]);
        }
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
        [$hash, $zset, $stream] = $this->keysOnDistinctMasters('pipe-replies', 3);

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
        foreach ([$hash, $zset, $stream] as $key) {
            $this->assertTrue($this->redis->set($key, 'after'));
            $this->assertEquals('after', $this->redis->get($key));
        }
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
        $serializer = $this->redis->getOption(Redis::OPT_SERIALIZER);

        try {
            $this->redis->setOption(Redis::OPT_SERIALIZER, Redis::SERIALIZER_PHP);
            $this->redis->set($key, $value);

            $pipe = $this->redis->pipeline()->get($key);
            $this->redis->setOption(Redis::OPT_SERIALIZER, Redis::SERIALIZER_NONE);
            $pipe->get($key);

            /* Match Redis: Pipeline replies use the serializer active at exec. */
            $serialized = serialize($value);
            $this->assertEquals([$serialized, $serialized], $pipe->exec());
        } finally {
            $this->redis->setOption(Redis::OPT_SERIALIZER, $serializer);
            $this->redis->del($key);
        }
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
            $this->redis->setOption(Redis::OPT_PACK_IGNORE_NUMBERS, false);

            $packed = $this->redis->_pack(['answer' => 42]);
            $pipe = $this->redis->pipeline()->set($keys[0], ['answer' => 42]);

            /* Packing options are applied while each command is built. */
            $this->redis->setOption(Redis::OPT_PACK_IGNORE_NUMBERS, true);
            $pipe->set($keys[1], 42);
            $this->redis->setOption(Redis::OPT_PACK_IGNORE_NUMBERS, false);

            $this->assertEquals([true, true], $pipe->exec());
            $this->redis->setOption(Redis::OPT_PACK_IGNORE_NUMBERS, true);
            $this->assertEquals(
                [['answer' => 42], 42],
                [
                    $this->redis->get($keys[0]),
                    $this->redis->get($keys[1]),
                ]
            );
            /* Decoding alone cannot distinguish '42' from a serialized integer. */
            $this->redis->setOption(Redis::OPT_SERIALIZER, Redis::SERIALIZER_NONE);
            $this->redis->setOption(Redis::OPT_COMPRESSION, Redis::COMPRESSION_NONE);
            $this->assertEquals($packed, $this->redis->get($keys[0]));
            $this->assertEquals('42', $this->redis->get($keys[1]));
        } finally {
            $this->redis->setOption(
                Redis::OPT_PACK_IGNORE_NUMBERS, $ignoreNumbers
            );
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
                    Redis::OPT_NULL_MULTIBULK_AS_NULL, !$option
                );
                $pipe = $this->redis->pipeline()->blpop([$blockingKey], .01);
                $this->redis->setOption(Redis::OPT_NULL_MULTIBULK_AS_NULL, $option);
                $this->assertEquals([$expected], $pipe->exec());
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
        $keys = $this->keysOnDistinctMasters('pipe-failover');
        $failover = $this->redis->getOption(RedisCluster::OPT_SLAVE_FAILOVER);
        $modes = [
            RedisCluster::FAILOVER_NONE,
            RedisCluster::FAILOVER_ERROR,
            RedisCluster::FAILOVER_DISTRIBUTE,
            RedisCluster::FAILOVER_DISTRIBUTE_SLAVES,
        ];
        $masterGetCalls = function () {
            $calls = 0;
            foreach ($this->redis->_masters() as $master) {
                $stats = $this->redis->info($master, 'COMMANDSTATS');
                $entry = $stats['cmdstat_get'] ?? '';
                if (preg_match('/calls=(\d+)/', $entry, $matches)) {
                    $calls += (int)$matches[1];
                }
            }
            return $calls;
        };

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
                $before = $masterGetCalls();
                $result = $this->redis->pipeline()
                    ->set($keys[0], $value)
                    ->get($keys[0])
                    ->set($keys[1], $value)
                    ->get($keys[1])
                    ->exec();
                $this->assertEquals([true, $value, true, $value], $result);
                /* Count reads on masters instead of relying on replica lag. */
                $this->assertEquals($before + 2, $masterGetCalls());
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

        /* Preserve RedisCluster's duplicate-MULTI rejection, unlike standalone. */
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

    public function testNonPipelineMultiStructuredErrorsPreserveLegacyResults() {
        $wrongType = '{legacy-structured-error}wrong-type';
        $this->redis->set($wrongType, 'still-readable');

        /* Pin legacy MULTI's empty arrays for these errors, not pipeline semantics. */
        $this->assertEquals(
            [[], [], [], 'still-readable'],
            $this->redis->multi()
                ->xrange($wrongType, '-', '+')
                ->xread([$wrongType => '0-0'])
                ->xinfo('STREAM', $wrongType)
                ->get($wrongType)
                ->exec()
        );
        $this->assertEquals('still-readable', $this->redis->get($wrongType));
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

    public function testPipelineReplyFamiliesMatchAtomicAndLegacyModes() {
        if (!$this->minVersionCheck('6.2')) $this->markTestSkipped();
        $keys = [];
        foreach (['string', 'other', 'missing', 'hash', 'list', 'zset', 'set',
                  'geo', 'stream', 'target', 'hll', 'control'] as $name)
        {
            $keys[$name] = "{pipe-reply-matrix}$name";
        }
        $k = $keys;
        $expiry = time() + 3600;
        $prepare = function () use ($k) {
            $this->redis->del(array_values($k));
            $this->redis->mset([$k['string'] => '12', $k['other'] => '123', $k['control'] => 'control']);
            $this->redis->hset($k['hash'], 'field', '1');
            $this->redis->rpush($k['list'], 'a', 'a', 'b');
            $this->redis->zadd($k['zset'], 1, 'a');
            $this->redis->sadd($k['set'], 'a');
            $this->redis->geoadd($k['geo'], 13.361389, 38.115556, 'Palermo');
            $this->redis->xadd($k['stream'], '1000-0', ['field' => 'one']);
            $this->redis->xadd($k['stream'], '2000-0', ['field' => 'two']);
            $this->redis->xgroup('CREATE', $k['stream'], 'group', '0');
            $this->redis->xreadgroup('group', 'original', [$k['stream'] => '>'], 1);
        };
        $prepare();
        $dump = $this->redis->dump($k['string']);
        /* Deterministic data keeps random-member, stream ID and expiry replies comparable. */
        $cases = [
            ['get', [$k['control']], '6.2'],
            ['xpending', [$k['stream'], 'group'], '6.2'],
            ['xclaim', [$k['stream'], 'group', 'claim', 0, ['1000-0'], []], '6.2'],
            ['xautoclaim', [$k['stream'], 'group', 'auto', 0, '0-0', 1], '6.2'],
            ['xreadgroup', ['group', 'next', [$k['stream'] => '>'], 1], '6.2'],
            ['xreadgroup', ['group', 'next', [$k['stream'] => '>'], 1], '6.2'],
            ['xgroup', ['CREATECONSUMER', $k['stream'], 'group', 'empty'], '6.2'],
            ['xack', [$k['stream'], 'group', ['1000-0']], '6.2'],
            ['xlen', [$k['stream']], '6.2'],
            ['xtrim', [$k['stream'], 1, false], '6.2'],
            ['xread', [[$k['missing'] => '$'], 1, 1], '6.2'],
            ['geopos', [$k['geo'], 'Palermo', 'missing'], '6.2'],
            ['geopos', [$k['missing'], 'missing'], '6.2'],
            ['geodist', [$k['geo'], 'Palermo', 'Palermo'], '6.2'],
            ['geodist', [$k['missing'], 'a', 'b'], '6.2'],
            ['geohash', [$k['geo'], 'Palermo'], '6.2'],
            ['georadius', [$k['geo'], 13.361389, 38.115556, 1, 'km', ['withcoord', 'withdist']], '6.2'],
            ['georadius_ro', [$k['geo'], 13.361389, 38.115556, 1, 'km'], '6.2'],
            ['georadiusbymember', [$k['geo'], 'Palermo', 1, 'km'], '6.2'],
            ['georadiusbymember_ro', [$k['geo'], 'Palermo', 1, 'km'], '6.2'],
            ['geosearch', [$k['geo'], 'Palermo', 1, 'km', ['withcoord', 'withdist', 'withhash']], '6.2'],
            ['geosearchstore', [$k['target'], $k['geo'], 'Palermo', 1, 'km'], '6.2'],
            ['lpos', [$k['list'], 'a', ['count' => 2, 'rank' => -1]], '6.2'],
            ['lpos', [$k['list'], 'missing'], '6.2'],
            ['lpop', [$k['missing'], 2], '6.2'],
            ['lmpop', [[$k['list']], 'LEFT', 2], '7.0'],
            ['lmpop', [[$k['missing']], 'LEFT'], '7.0'],
            ['zmscore', [$k['zset'], 'a', 'missing'], '6.2'],
            ['zscore', [$k['missing'], 'missing'], '6.2'],
            ['zadd', [$k['zset'], ['NX', 'INCR'], 1, 'a'], '6.2'],
            ['zrandmember', [$k['zset'], ['count' => 1, 'withscores' => true]], '6.2'],
            ['zrandmember', [$k['missing'], ['count' => 2]], '6.2'],
            ['zpopmin', [$k['zset'], 1], '6.2'],
            ['zadd', [$k['zset'], 1, 'a'], '6.2'],
            ['zpopmax', [$k['zset'], 1], '6.2'],
            ['zadd', [$k['zset'], 1, 'a'], '7.0'],
            ['zmpop', [[$k['zset']], 'MIN'], '7.0'],
            ['zmpop', [[$k['missing']], 'MIN'], '7.0'],
            ['smismember', [$k['set'], 'a', 'missing'], '6.2'],
            ['srandmember', [$k['set'], -2], '6.2'],
            ['hrandfield', [$k['hash'], ['count' => 1, 'withvalues' => true]], '6.2'],
            ['hrandfield', [$k['missing']], '6.2'],
            ['hrandfield', [$k['missing'], ['count' => 2]], '6.2'],
            ['hincrbyfloat', [$k['hash'], 'field', .5], '6.2'],
            ['incrbyfloat', [$k['string'], .5], '6.2'],
            ['sort', [$k['list'], ['alpha' => true, 'store' => $k['target']]], '6.2'],
            ['sort_ro', [$k['list'], ['alpha' => true]], '7.0'],
            ['dump', [$k['string']], '6.2'],
            ['dump', [$k['missing']], '6.2'],
            ['copy', [$k['string'], $k['target'], ['replace' => true]], '6.2'],
            ['restore', [$k['target'], 0, $dump, ['REPLACE']], '6.2'],
            ['getex', [$k['missing']], '6.2'],
            ['getex', [$k['string'], ['EXAT' => $expiry]], '6.2'],
            ['expiretime', [$k['string']], '7.0'],
            ['persist', [$k['string']], '6.2'],
            ['getdel', [$k['string']], '6.2'],
            ['set', [$k['other'], 'ignored', ['nx', 'get']], '7.0'],
            ['lcs', [$k['target'], $k['other'], ['idx', 'withmatchlen']], '7.0'],
            ['pfadd', [$k['hll'], ['a', 'b']], '6.2'],
            ['pfcount', [[$k['hll']]], '6.2'],
            ['del', [$k['target']], '6.2'],
            ['pfmerge', [$k['target'], [$k['hll']]], '6.2'],
            ['bitcount', [$k['other']], '6.2'],
            ['bitpos', [$k['other'], 1], '6.2'],
            ['getbit', [$k['other'], 0], '6.2'],
            ['setbit', [$k['other'], 0, 1], '6.2'],
            ['bitop', ['AND', $k['target'], $k['other']], '6.2'],
            ['httl', [$k['hash'], ['field', 'missing']], '7.4'],
            ['hpttl', [$k['hash'], ['field', 'missing']], '7.4'],
            ['hexpiretime', [$k['hash'], ['field', 'missing']], '7.4'],
            ['hpexpiretime', [$k['hash'], ['field', 'missing']], '7.4'],
            ['hexpire', [$k['hash'], 3600, ['field']], '7.4'],
            ['hpexpire', [$k['hash'], 3600000, ['field']], '7.4'],
            ['hexpireat', [$k['hash'], $expiry, ['field']], '7.4'],
            ['hpexpireat', [$k['hash'], $expiry * 1000, ['field']], '7.4'],
            ['hpersist', [$k['hash'], ['field']], '7.4'],
            ['evalsha', [str_repeat('0', 40), [$k['control']], 1], '6.2'],
            ['eval', ["return {1, {2, false}, redis.status_reply('OK')}", [$k['control']], 1], '6.2'],
            ['command', ['INFO', 'get'], '6.2'],
            ['get', [$k['control']], '6.2'],
        ];
        $cases = array_values(array_filter($cases, function ($case) {
            return $this->minVersionCheck($case[2]);
        }));
        $nullOption = $this->redis->getOption(Redis::OPT_NULL_MULTIBULK_AS_NULL);
        try {
            foreach ([false, true] as $null) {
                $this->redis->setOption(Redis::OPT_NULL_MULTIBULK_AS_NULL, $null);
                $prepare();
                $expected = [];
                foreach ($cases as [$method, $args]) {
                    $expected[] = $this->redis->$method(...$args);
                    if ($method !== 'evalsha') $this->assertNull($this->redis->getLastError());
                }
                foreach (['pipeline', 'nested', 'multi'] as $mode) {
                    $prepare();
                    $pipe = $mode === 'multi' ? $this->redis->multi() : $this->redis->pipeline();
                    if ($mode === 'nested') $pipe->multi();
                    foreach ($cases as [$method, $args]) $pipe->$method(...$args);
                    if ($mode === 'nested') $pipe->exec();
                    $actual = $pipe->exec();
                    if ($mode === 'nested') $actual = $actual[0];
                    $this->assertEquals(count($expected), count($actual));
                    foreach ($cases as $i => [$method]) {
                        $this->assertEquals($expected[$i], $actual[$i], "$method/$mode/null=$null");
                    }
                }
            }
        } finally {
            $this->redis->setOption(Redis::OPT_NULL_MULTIBULK_AS_NULL, $nullOption);
            $this->redis->del(array_values($k));
        }
    }

    public function testPipelineNullableIntegerRepliesDoNotAbort() {
        $zset = '{pipe-null-integer}zset';
        $missing = '{pipe-null-integer}missing';
        $control = '{pipe-null-integer}control';

        $this->redis->del([$zset, $missing, $control]);
        $this->redis->zadd($zset, 1, 'present');

        foreach ([false, true] as $nested) {
            $pipe = $this->redis->pipeline();
            if ($nested) $pipe->multi();
            $pipe->set($control, 'ok')
                ->zrank($zset, 'missing')
                ->zrevrank($zset, 'missing')
                ->zrank($missing, 'missing')
                ->object('refcount', $missing)
                ->object('idletime', $missing)
                ->zrank($zset, 'present')
                ->get($control);
            if ($nested) $pipe->exec();

            $expected = [true, false, false, false, false, false, 0, 'ok'];
            $this->assertEquals($nested ? [$expected] : $expected, $pipe->exec());
            $this->assertEquals('ok', $this->redis->get($control));
        }
    }

    public function testPipelineBlockingNullRepliesMatchAtomicAndLegacyModes() {
        $missing = '{pipe-block-null}missing';
        $control = '{pipe-block-null}control';
        $this->redis->del($missing);
        $this->redis->set($control, 'ok');
        $commands = [
            ['blpop', [[$missing], .01]],
            ['brpop', [[$missing], .01]],
            ['bzpopmin', [[$missing], .01]],
            ['bzpopmax', [[$missing], .01]],
        ];
        if ($this->minVersionCheck('7.0')) {
            $commands[] = ['blmpop', [.01, [$missing], 'LEFT']];
            $commands[] = ['bzmpop', [.01, [$missing], 'MIN']];
        }
        $option = $this->redis->getOption(Redis::OPT_NULL_MULTIBULK_AS_NULL);
        try {
            foreach ([false, true] as $null) {
                $this->redis->setOption(Redis::OPT_NULL_MULTIBULK_AS_NULL, $null);
                $expected = ['ok'];
                foreach ($commands as [$method, $args]) $expected[] = $this->redis->$method(...$args);
                $expected[] = 'ok';
                foreach (['pipeline', 'nested', 'multi'] as $mode) {
                    $pipe = $mode === 'multi' ? $this->redis->multi() : $this->redis->pipeline();
                    if ($mode === 'nested') $pipe->multi();
                    $pipe->get($control);
                    foreach ($commands as [$method, $args]) $pipe->$method(...$args);
                    $pipe->get($control);
                    if ($mode === 'nested') $pipe->exec();
                    $actual = $pipe->exec();
                    $this->assertEquals($expected, $mode === 'nested' ? $actual[0] : $actual, "$mode/null=$null");
                }
            }
        } finally {
            $this->redis->setOption(Redis::OPT_NULL_MULTIBULK_AS_NULL, $option);
            $this->redis->del($control);
        }
    }

    public function testPipelineClientCrossSlotRejectionPreservesQueueAndBlockSlot() {
        [$a, $b] = $this->keysOnDistinctMasters('pipe-client-crossslot');
        $commands = [
            ['rename', [$a, $b]],
            ['smove', [$a, $b, 'member']],
            ['sinterstore', [[$a, $b]]],
            ['zunionstore', [$a, [$a, $b]]],
        ];
        foreach ($commands as [$method, $args]) {
            foreach ([false, true] as $nested) {
                $pipe = $this->redis->pipeline();
                if ($nested) $pipe->multi();
                $this->assertFalse(@$pipe->$method(...$args));
                /* A rejected first command must not bind the MULTI block to A. */
                $pipe->set($b, 'accepted')->get($b);
                if ($nested) $pipe->exec();
                $this->assertEquals($nested ? [[true, 'accepted']] : [true, 'accepted'], $pipe->exec());
                $this->assertEquals(Redis::ATOMIC, $this->redis->getMode());
            }
        }
        $this->redis->del([$a, $b]);
    }

    public function testPipelineRemainingDirectedCommandsPreserveQueuedState() {
        $key = '{pipe-directed-matrix}key';
        $control = '{pipe-directed-matrix}control';
        $this->redis->set($control, 'unchanged');
        $commands = [
            ['save', [$key]], ['bgsave', [$key]], ['bgrewriteaof', [$key]],
            ['lastsave', [$key]], ['flushdb', [$key]], ['flushall', [$key]],
            ['dbsize', [$key]], ['info', [$key]], ['client', [$key, 'LIST']],
            ['config', [$key, 'GET', 'maxmemory']], ['pubsub', [$key, 'CHANNELS']],
            ['slowlog', [$key, 'LEN']], ['role', [$key]], ['time', [$key]],
            ['randomkey', [$key]], ['echo', [$key, 'echo']],
            ['rawCommand', [$key, 'GET', $key]], ['acl', [$key, 'WHOAMI']],
            ['cluster', [$key, 'KEYSLOT', $key]],
            ['script', [$key, 'EXISTS', str_repeat('0', 40)]],
        ];
        foreach ($commands as [$method, $args]) {
            foreach ([false, true] as $nested) {
                $this->redis->clearTransferredBytes();
                $pipe = $this->redis->pipeline();
                if ($nested) $pipe->multi();
                $pipe->set($key, $method);
                $exception = null;
                $result = null;
                $warning = null;
                set_error_handler(function ($errno, $message) use (&$warning) {
                    $warning = $message;
                    return true;
                }, E_WARNING);
                try {
                    $result = $pipe->$method(...$args);
                } catch (RedisClusterException $e) {
                    $exception = $e;
                } finally {
                    restore_error_handler();
                }
                $this->assertTrue($result === false || $exception instanceof RedisClusterException);
                if ($warning !== null) $this->assertStringContains('PIPELINE mode', $warning);
                $this->assertEquals([0, 0], $this->redis->getTransferredBytes());
                $pipe->get($key)->get($control);
                if ($nested) $pipe->exec();
                $expected = [true, $method, 'unchanged'];
                $this->assertEquals($nested ? [$expected] : $expected, $pipe->exec());
            }
        }
        $this->redis->del([$key, $control]);
    }

    public function testPipelineLargeCrossNodeResponseOrder() {
        $keys = $this->keysOnDistinctMasters('pipe-large', 2);
        $payload = str_repeat('x', 1024);
        $this->redis->clearTransferredBytes();
        $pipe = $this->redis->pipeline();
        $expected = [];
        for ($i = 0; $i < 12000; $i++) {
            $value = "$i:$payload";
            $pipe->set($keys[$i % 2], $value)->get($keys[$i % 2]);
            $expected[] = true;
            $expected[] = $value;
        }
        $this->assertEquals([0, 0], $this->redis->getTransferredBytes());
        $actual = $pipe->exec();
        $this->assertEquals(count($expected), count($actual));
        foreach ($expected as $i => $value) $this->assertEquals($value, $actual[$i], "reply $i");
        [$tx, $rx] = $this->redis->getTransferredBytes();
        $this->assertGT(10 * 1024 * 1024, $tx);
        $this->assertGT(10 * 1024 * 1024, $rx);
        $this->redis->del($keys);
    }

    public function testPipelineBlockingMoveTimeoutDoesNotAbort() {
        $source = '{pipe-null-move}source';
        $target = '{pipe-null-move}target';
        $control = '{pipe-null-move}control';
        $this->redis->del([$source, $target]);

        foreach ([false, true] as $nested) {
            $pipe = $this->redis->pipeline();
            if ($nested) $pipe->multi();
            $pipe->set($control, 'ok')->brpoplpush($source, $target, 1);
            if ($this->minVersionCheck('6.2')) {
                $pipe->blmove($source, $target, 'LEFT', 'RIGHT', 0.01);
            }
            $pipe->get($control);
            if ($nested) $pipe->exec();

            $expected = $this->minVersionCheck('6.2')
                ? [true, false, false, 'ok'] : [true, false, 'ok'];
            $this->assertEquals($nested ? [$expected] : $expected, $pipe->exec());
            $this->assertEquals('ok', $this->redis->get($control));
        }
    }

    public function testPipelineMgetChunkErrorFailsLogicalCommand() {
        if (!$this->minVersionCheck('6.0')) $this->markTestSkipped();

        $user = 'phpredis-pipeline-mget-' . uniqid();
        $pass = uniqid('secret-', true);
        $restricted = null;
        $masters = $this->redis->_masters();
        $allowed = '{pipe-mget-allowed}';
        $denied = '{pipe-mget-denied}';
        $this->redis->mset([
            "$allowed:a" => 'A',
            "$allowed:b" => 'B',
            "$denied:a" => 'denied',
        ]);
        $this->redis->del("$allowed:missing");

        try {
            foreach ($masters as $master) {
                $this->assertTrue($this->redis->acl(
                    $master, 'SETUSER', $user, 'reset', 'on', ">$pass",
                    "~$allowed*", '+@all'
                ));
            }
            $restricted = new RedisCluster(
                null, self::$seeds, 1, 2, false, [$user, $pass]
            );

            foreach ([
                ["$denied:a", "$allowed:a"],
                ["$allowed:a", "$denied:a"],
                ["$allowed:a", "$denied:a", "$allowed:b", "$denied:a", "$allowed:a"],
            ] as $keys) {
                $this->assertFalse($restricted->mget($keys));
                $this->assertStringContains('NOPERM', $restricted->getLastError());
                $this->assertEquals(
                    [false, true, ['A', false], 'ok'],
                    $restricted->pipeline()
                        ->mget($keys)
                        ->set("$allowed:control", 'ok')
                        ->mget(["$allowed:a", "$allowed:missing"])
                        ->get("$allowed:control")
                        ->exec()
                );
                $this->assertEquals('ok', $restricted->get("$allowed:control"));
            }
        } finally {
            if ($restricted) $restricted->close();
            foreach ($masters as $master) {
                $this->redis->acl($master, 'DELUSER', $user);
            }
            $this->redis->del([
                "$allowed:a", "$allowed:b", "$denied:a", "$allowed:control",
            ]);
        }
    }

    public function testPipelineMalformedReplyDisconnectsPersistentSocket() {
        $this->assertMalformedPipelineReplyAborts('pipeline');
    }

    public function testPipelineEofDisconnectsPersistentSocket() {
        $pooling = ini_get('redis.pconnect.pooling_enabled');
        ini_set('redis.pconnect.pooling_enabled', 1);

        try {
            foreach (['eof-header', 'eof-body'] as $scenario) {
                [$process, $pipes, $port] = $this->startMalformedReplyServer($scenario);
                $redis = null;
                try {
                    $redis = new RedisCluster(null, ["127.0.0.1:$port"], 1, 1, true);
                    $exception = null;
                    try {
                        $redis->pipeline()->get('first')->get('second')->get('third')->exec();
                    } catch (RedisException | RedisClusterException $e) {
                        $exception = $e;
                    }
                    $this->assertTrue($exception instanceof Exception);
                    $this->assertStringContains($scenario === 'eof-body'
                        ? 'socket error on read socket' : 'Error reading pipeline response',
                        $exception->getMessage());
                    $this->assertEquals(Redis::ATOMIC, $redis->getMode());
                    $this->assertEquals(['actual'], $redis->pipeline()->get('after')->exec());
                } finally {
                    if ($redis) $redis->close();
                    $this->stopMalformedReplyServer($process, $pipes);
                }
            }
        } finally {
            ini_set('redis.pconnect.pooling_enabled', $pooling);
        }
    }

    public function testPipelinePartialSendDoesNotReplayOrReuseSocket() {
        $keys = [];
        for ($i = 0; count($keys) < 2; $i++) {
            $key = "{partial-send-$i}key";
            $slot = $this->redis->cluster($key, 'KEYSLOT', $key);
            $keys[$slot < 8192 ? 0 : 1] = $key;
        }
        $pooling = ini_get('redis.pconnect.pooling_enabled');
        ini_set('redis.pconnect.pooling_enabled', 1);

        try {
            foreach ([false, true] as $persistent) {
                [$process, $pipes, $port] = $this->startMalformedReplyServer('send-failure');
                $redis = null;
                try {
                    $redis = new RedisCluster(null, ["127.0.0.1:$port"], 1, 1, $persistent);
                    $exception = null;
                    try {
                        $redis->pipeline()->set($keys[0], 'sent')->get($keys[1])->exec();
                    } catch (RedisClusterException $e) {
                        $exception = $e;
                    }
                    $this->assertTrue($exception instanceof RedisClusterException);
                    $this->assertStringContains('Unable to send pipeline to node', $exception->getMessage());
                    $this->assertEquals(Redis::ATOMIC, $redis->getMode());
                    /* The first write executed once; the failed batch was not replayed. */
                    $this->assertEquals(['1', '1'], $redis->pipeline()->get($keys[0])->get($keys[1])->exec());
                } finally {
                    if ($redis) $redis->close();
                    $this->stopMalformedReplyServer($process, $pipes);
                }
            }
        } finally {
            ini_set('redis.pconnect.pooling_enabled', $pooling);
        }
    }

    public function testPipelineFailureInvalidatesLoadedCacheAndRefreshesOnce() {
        [$process, $pipes, $port] = $this->startMalformedReplyServer('cache-refresh');
        $cacheSlots = ini_get('redis.clusters.cache_slots');
        ini_set('redis.clusters.cache_slots', 1);
        $populator = $cached = $fresh = null;
        $key = '{cache-refresh}control';

        try {
            $seeds = ["127.0.0.1:$port"];
            $populator = new RedisCluster(null, $seeds, 1, 1, false);
            $this->assertEquals('1', $populator->get($key));
            $populator->close();
            $cached = new RedisCluster(null, $seeds, 1, 1, false);
            $this->assertEquals('1', $cached->get($key));
            $exception = null;
            try {
                $cached->pipeline()->eval('return 1', [$key], 1)->exec();
            } catch (RedisClusterException $e) {
                $exception = $e;
            }
            $this->assertTrue($exception instanceof RedisClusterException);

            $fresh = new RedisCluster(null, $seeds, 1, 1, false);
            $this->assertEquals('2', $fresh->get($key));
            $fresh->close();
            /* Atomic commands must not consume the pending pipeline refresh. */
            $this->assertEquals('2', $cached->get($key));
            $this->assertEquals(['3'], $cached->multi(Redis::PIPELINE)->get($key)->exec());
            $this->assertEquals(['3'], $cached->pipeline()->get($key)->exec());
            $this->assertEquals([false, '3'], $cached->pipeline()->incr($key)->get($key)->exec());
            $this->assertEquals(['3'], $cached->pipeline()->get($key)->exec());

            foreach (['TRYAGAIN', 'ASK'] as $error) {
                $exception = null;
                try {
                    $result = $cached->pipeline()
                        ->eval("return redis.error_reply('$error fixture')", [$key], 1)
                        ->get($key)->exec();
                } catch (RedisClusterException $e) {
                    $exception = $e;
                }
                if ($error === 'ASK') {
                    $this->assertTrue($exception instanceof RedisClusterException);
                } else {
                    $this->assertEquals([false, '3'], $result);
                }
                $this->assertEquals(['3'], $cached->pipeline()->get($key)->exec());
            }
        } finally {
            foreach ([$populator, $cached, $fresh] as $redis) {
                if ($redis) $redis->close();
            }
            $this->stopMalformedReplyServer($process, $pipes);
            ini_set('redis.clusters.cache_slots', $cacheSlots);
        }
    }

    public function testPipelineReadTimeoutDoesNotReuseLateReplies() {
        [$key, $control] = $this->keysOnDistinctMasters('pipe-read-timeout');
        $empty = "$key:empty";
        $this->redis->del($empty);
        $this->redis->set($control, 'other-node');
        $pooling = ini_get('redis.pconnect.pooling_enabled');
        $pattern = ini_get('redis.pconnect.pool_pattern');
        ini_set('redis.pconnect.pooling_enabled', 1);
        /* Keep short-timeout streams separate from the suite's existing pool. */
        ini_set('redis.pconnect.pool_pattern', $pattern . 'uu');

        try {
            foreach ([false, true] as $persistent) {
                $redis = new RedisCluster(null, self::$seeds, 1, .2, $persistent, $this->getAuth());
                $fresh = null;
                try {
                    $exception = null;
                    try {
                        $redis->pipeline()->set($key, 'before-block')
                            ->blpop([$empty], 1)->get($control)->exec();
                    } catch (RedisClusterException $e) {
                        $exception = $e;
                    }
                    $this->assertTrue($exception instanceof RedisClusterException);
                    $this->assertStringContains('Error reading pipeline response', $exception->getMessage());
                    $this->assertEquals(Redis::ATOMIC, $redis->getMode());
                    $this->assertEquals('before-block', $redis->get($key));
                    $this->assertEquals('other-node', $redis->get($control));
                    $fresh = new RedisCluster(null, self::$seeds, 1, 1, $persistent, $this->getAuth());
                    $this->assertEquals(['before-block', 'other-node'],
                        $fresh->pipeline()->get($key)->get($control)->exec());
                    $fresh->close();
                    usleep(1100000);
                    $this->assertEquals(['before-block', 'other-node'],
                        $redis->pipeline()->get($key)->get($control)->exec());
                    $this->assertEquals('before-block', $fresh->get($key));
                    $this->assertEquals('other-node', $fresh->get($control));
                } finally {
                    $redis->close();
                    if ($fresh) $fresh->close();
                }
            }
        } finally {
            ini_set('redis.pconnect.pool_pattern', $pattern);
            ini_set('redis.pconnect.pooling_enabled', $pooling);
            $this->redis->del([$key, $control, $empty]);
        }
    }

    public function testPipelineReconnectsKilledIdleParticipants() {
        $keys = $this->keysOnDistinctMasters('pipe-idle-reconnect', 3);
        $masters = $this->redis->_masters();
        $pooling = ini_get('redis.pconnect.pooling_enabled');
        ini_set('redis.pconnect.pooling_enabled', 1);

        try {
            foreach ([false, true] as $persistent) {
                $redis = new RedisCluster(null, self::$seeds, 1, 1, $persistent, $this->getAuth());
                try {
                    $ids = [];
                    foreach ($masters as $master) {
                        $ids[] = $redis->rawCommand($master, 'CLIENT', 'ID');
                    }
                    $pipe = $redis->pipeline();
                    foreach ($keys as $key) $pipe->set($key, 'reconnected')->get($key);
                    foreach ($masters as $i => $master) {
                        $node = $this->connectToNode($master);
                        try {
                            /* Kill only this client's sockets, not other test clients. */
                            $this->assertEquals(1, $node->rawCommand('CLIENT', 'KILL', 'ID', $ids[$i]));
                        } finally {
                            $node->close();
                        }
                    }
                    $this->assertEquals([true, 'reconnected', true, 'reconnected', true, 'reconnected'], $pipe->exec());
                    $this->assertEquals(Redis::ATOMIC, $redis->getMode());
                } finally {
                    $redis->close();
                }
            }
        } finally {
            ini_set('redis.pconnect.pooling_enabled', $pooling);
            $this->redis->del($keys);
        }
    }

    public function testPipelineFailedTopologyRefreshCanRecover() {
        /* Denying individual ACL subcommands requires Redis 7.0. */
        if (!$this->minVersionCheck('7.0')) $this->markTestSkipped();

        $cacheSlots = ini_get('redis.clusters.cache_slots');
        /* Exercise refresh failures using the configured seed list. */
        ini_set('redis.clusters.cache_slots', 0);
        $user = 'phpredis-pipeline-refresh-' . uniqid();
        $pass = uniqid('secret-', true);
        $masters = $this->redis->_masters();
        $restricted = null;
        $key = '{pipe-refresh}control';
        $this->redis->set($key, 'still-readable');

        try {
            foreach ($masters as $master) {
                $this->assertTrue($this->redis->acl(
                    $master, 'SETUSER', $user, 'reset', 'on', ">$pass", '~*', '+@all'
                ));
            }
            $restricted = new RedisCluster(
                null, self::$seeds, 1, 2, true, [$user, $pass]
            );
            $exception = null;
            try {
                $restricted->pipeline()
                    ->eval("return redis.error_reply('CLUSTERDOWN simulated failure')", [$key], 1)
                    ->exec();
            } catch (RedisClusterException $e) {
                $exception = $e;
            }
            $this->assertTrue($exception instanceof RedisClusterException);
            $this->assertStringContains('CLUSTERDOWN', $exception->getMessage());

            foreach ($masters as $master) {
                $this->assertTrue($this->redis->acl(
                    $master, 'SETUSER', $user, '-cluster|slots'
                ));
            }
            foreach ([1, 2] as $attempt) {
                $exception = null;
                try {
                    $restricted->pipeline();
                } catch (RedisClusterException $e) {
                    $exception = $e;
                }
                $this->assertTrue($exception instanceof RedisClusterException);
                $this->assertStringContains('map cluster keyspace', $exception->getMessage());
                $this->assertEquals(Redis::ATOMIC, $restricted->getMode());
            }

            foreach ($masters as $master) {
                $this->assertTrue($this->redis->acl(
                    $master, 'SETUSER', $user, '+cluster|slots'
                ));
            }
            $this->assertEquals(
                ['still-readable'], $restricted->pipeline()->get($key)->exec()
            );
        } finally {
            if ($restricted) $restricted->close();
            ini_set('redis.clusters.cache_slots', $cacheSlots);
            foreach ($masters as $master) {
                $this->redis->acl($master, 'DELUSER', $user);
            }
            $this->redis->del($key);
        }
    }

    public function testPipelineRefreshRejectsUncoveredSlot() {
        [$process, $pipes, $port] = $this->startMalformedReplyServer('partial-slots');
        $redis = null;

        try {
            $redis = new RedisCluster(null, ["127.0.0.1:$port"], 1, 1, false);
            $key = '{malformed}hole';
            $exception = null;
            try {
                $redis->pipeline()->eval('return 1', [$key], 1)->exec();
            } catch (RedisClusterException $e) {
                $exception = $e;
            }
            $this->assertTrue($exception instanceof RedisClusterException);
            $this->assertStringContains('CLUSTERDOWN', $exception->getMessage());

            $exception = null;
            try {
                $redis->pipeline()->get($key);
            } catch (RedisClusterException $e) {
                $exception = $e;
            }
            $this->assertTrue($exception instanceof RedisClusterException);
            $this->assertStringContains('Pipeline slot is not covered', $exception->getMessage());
            $this->assertEquals(Redis::ATOMIC, $redis->getMode());
        } finally {
            if ($redis) $redis->close();
            $this->stopMalformedReplyServer($process, $pipes);
        }
    }

    public function testNonPipelineMovedRemapRejectsUncoveredSlot() {
        $key = '{failover-partial-slots}hole';
        $slot = $this->redis->cluster($key, 'KEYSLOT', $key);
        $this->assertGT(0, $slot);
        $cacheSlots = ini_get('redis.clusters.cache_slots');
        ini_set('redis.clusters.cache_slots', 0);

        try {
            foreach ([false, true] as $persistent) {
                [$process, $pipes, $port] = $this->startMalformedReplyServer(
                    'failover-partial-slots', $slot
                );
                $redis = null;

                try {
                    $redis = new RedisCluster(null, ["127.0.0.1:$port"], 1, 1, $persistent);
                    $exception = null;
                    try {
                        $redis->get($key);
                    } catch (RedisClusterException $e) {
                        $exception = $e;
                    }
                    $this->assertTrue($exception instanceof RedisClusterException);
                    $this->assertStringContains(
                        'Socket for slot is NULL after MOVED redirection',
                        $exception->getMessage()
                    );
                    $this->assertEquals(Redis::ATOMIC, $redis->getMode());

                    /* An empty key hashes to slot zero, which remains covered. */
                    $this->assertEquals('actual', $redis->get(''));
                } finally {
                    if ($redis) $redis->close();
                    $this->stopMalformedReplyServer($process, $pipes);
                }
            }
        } finally {
            ini_set('redis.clusters.cache_slots', $cacheSlots);
        }
    }

    public function testPipelineRefreshAuthenticatesCacheLoadedSeeds() {
        $cacheSlots = ini_get('redis.clusters.cache_slots');
        ini_set('redis.clusters.cache_slots', 1);

        try {
            foreach ([false, true] as $persistent) {
                [$process, $pipes, $port] = $this->startMalformedReplyServer('cache-auth');
                $populator = $cached = null;

                try {
                    $seeds = ["127.0.0.1:$port"];
                    $auth = ['pipeline-user', 'secret'];
                    $populator = new RedisCluster(null, $seeds, 1, 1, false, $auth);
                    $cached = new RedisCluster(null, $seeds, 1, 1, $persistent, $auth);
                    $key = '{cache-auth}key';
                    $this->assertEquals(['actual'], $cached->pipeline()->get($key)->exec());

                    $exception = null;
                    try {
                        $cached->pipeline()->eval('return 1', [$key], 1)->exec();
                    } catch (RedisClusterException $e) {
                        $exception = $e;
                    }
                    $this->assertTrue($exception instanceof RedisClusterException);
                    $this->assertStringContains('CLUSTERDOWN', $exception->getMessage());

                    for ($attempt = 0; $attempt < 3; $attempt++) {
                        $this->assertEquals(['actual'], $cached->pipeline()->get($key)->exec());
                    }
                } finally {
                    if ($cached) $cached->close();
                    if ($populator) $populator->close();
                    $this->stopMalformedReplyServer($process, $pipes);
                }
            }
        } finally {
            ini_set('redis.clusters.cache_slots', $cacheSlots);
        }
    }

    public function testPipelineRefreshDoesNotLeakNonParticipantConnections() {
        [$key] = $this->keysOnDistinctMasters('pipe-remap-connections', 3);
        $masters = $this->redis->_masters();
        $pooling = ini_get('redis.pconnect.pooling_enabled');
        ini_set('redis.pconnect.pooling_enabled', 1);

        try {
            foreach ([false, true] as $persistent) {
                $redis = new RedisCluster(null, self::$seeds, 1, 1, $persistent, $this->getAuth());
                $name = 'phpredis-pipeline-remap-' . uniqid();

                try {
                    for ($attempt = 0; $attempt < 3; $attempt++) {
                        foreach ($masters as $master) {
                            $this->assertTrue($redis->client($master, 'SETNAME', $name));
                        }
                        $connections = 0;
                        foreach ($masters as $master) {
                            foreach ($this->redis->client($master, 'LIST') as $client) {
                                if (($client['name'] ?? '') === $name) $connections++;
                            }
                        }
                        $this->assertEquals(count($masters), $connections);

                        $exception = null;
                        try {
                            $redis->pipeline()
                                ->eval("return redis.error_reply('CLUSTERDOWN simulated failure')", [$key], 1)
                                ->exec();
                        } catch (RedisClusterException $e) {
                            $exception = $e;
                        }
                        $this->assertTrue($exception instanceof RedisClusterException);
                        $redis->pipeline()->get($key)->exec();
                    }
                } finally {
                    foreach ($masters as $master) $redis->client($master, 'SETNAME', '');
                    $redis->close();
                }
            }
        } finally {
            ini_set('redis.pconnect.pooling_enabled', $pooling);
        }
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
                ->strlen($key)
                ->set($control, 'queued-on-other-node')
                ->strlen($control)
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
        $this->assertEquals(
            ['still-readable', 'queued-on-other-node'],
            $this->redis->pipeline()->get($key)->get($control)->exec()
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
            ->strlen($key)
            ->exec();
        $pipe->set($control, 'queued-on-other-node')->strlen($control);

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

    private function emptySlotKey($prefix) {
        for ($i = 0; $i < 256; $i++) {
            $key = "{{$prefix}-$i}key";
            $slot = $this->redis->cluster($key, 'KEYSLOT', $key);
            if ($this->redis->cluster($key, 'COUNTKEYSINSLOT', $slot) === 0) {
                return [$key, $slot];
            }
        }
        throw new RuntimeException('Unable to find an empty slot for migration');
    }

    private function waitForSlotOwner($slot, $owner) {
        $nodes = [];

        try {
            foreach (self::$seeds as $seed) {
                $separator = strrpos($seed, ':');
                $nodes[$seed] = $this->connectToNode([
                    substr($seed, 0, $separator), (int)substr($seed, $separator + 1)
                ]);
            }

            $deadline = microtime(true) + 30;
            do {
                $pending = [];
                foreach ($nodes as $seed => $node) {
                    $actual = null;
                    foreach ($node->rawCommand('CLUSTER', 'SLOTS') as $range) {
                        if ($slot >= $range[0] && $slot <= $range[1]) {
                            $actual = $range[2][2];
                            break;
                        }
                    }
                    if ($actual !== $owner) $pending[$seed] = $actual;
                }
                if (!$pending) return;
                usleep(50000);
            } while (microtime(true) < $deadline);

            throw new RuntimeException(
                "Slot $slot did not converge to owner $owner: " . json_encode($pending)
            );
        } finally {
            foreach ($nodes as $node) $node->close();
        }
    }

    private function clusterSlotsCalls() {
        $calls = 0;
        /* A topology refresh can query a replica seed as well as a master. */
        foreach (self::$seeds as $seed) {
            $separator = strrpos($seed, ':');
            $node = $this->connectToNode([
                substr($seed, 0, $separator), (int)substr($seed, $separator + 1)
            ]);
            try {
                $stats = $node->info('COMMANDSTATS');
            } finally {
                $node->close();
            }
            /* Older servers group CLUSTER subcommands; these checks run between slot changes. */
            $stat = $stats['cmdstat_cluster|slots'] ?? $stats['cmdstat_cluster'] ?? '';
            if (preg_match('/calls=(\d+)/', $stat, $match)) {
                $calls += (int)$match[1];
            }
        }
        return $calls;
    }

    public function testPipelineRealMovedRefreshesRouting() {
        $cacheSlots = ini_get('redis.clusters.cache_slots');
        $nodes = [];
        foreach ($this->redis->_masters() as $master) $nodes[] = $this->connectToNode($master);
        $ids = array_map(function ($node) { return $node->rawCommand('CLUSTER', 'MYID'); }, $nodes);

        try {
            foreach (['plain', 'nested', 'atomic-between', 'cached'] as $variant) {
                ini_set('redis.clusters.cache_slots', $variant === 'cached' ? 1 : 0);
                [$key, $slot] = $this->emptySlotKey('pipe-real-moved-' . $variant);
                $owner = array_search($this->redis->cluster($key, 'MYID'), $ids, true);
                $target = ($owner + 1) % count($nodes);
                $populator = $fresh = null;
                if ($variant === 'cached') {
                    $populator = new RedisCluster(null, self::$seeds, 1, 1, false, $this->getAuth());
                }
                $redis = new RedisCluster(null, self::$seeds, 1, 1, false, $this->getAuth());

                try {
                    $pipe = $redis->pipeline();
                    if ($variant === 'nested') $pipe->multi();
                    $pipe->get($key);
                    if ($variant === 'nested') $pipe->exec();
                    /* No keys need MIGRATE: the slot is empty before ownership changes. */
                    foreach ($nodes as $node) {
                        $this->assertTrue($node->rawCommand('CLUSTER', 'SETSLOT', $slot, 'NODE', $ids[$target]) !== false);
                    }
                    $this->assertTrue($nodes[$target]->set($key, 'new-owner'));
                    /* Replica seed maps converge through cluster gossip, not SETSLOT. */
                    $this->waitForSlotOwner($slot, $ids[$target]);
                    $before = $this->clusterSlotsCalls();
                    $exception = null;
                    try {
                        $pipe->exec();
                    } catch (RedisClusterException $e) {
                        $exception = $e;
                    }
                    $this->assertTrue($exception instanceof RedisClusterException);
                    $this->assertStringContains('redirected', $exception->getMessage());
                    $this->assertEquals($before, $this->clusterSlotsCalls());

                    if ($variant === 'atomic-between') {
                        $this->assertEquals('new-owner', $redis->get($key));
                    } else if ($variant === 'cached') {
                        $fresh = new RedisCluster(null, self::$seeds, 1, 1, false, $this->getAuth());
                        $this->assertEquals($before + 1, $this->clusterSlotsCalls());
                        $this->assertEquals('new-owner', $fresh->get($key));
                    }
                    $beforeRefresh = $this->clusterSlotsCalls();
                    $this->assertEquals(['new-owner'], $redis->pipeline()->get($key)->exec());
                    $this->assertEquals($beforeRefresh + 1, $this->clusterSlotsCalls());
                    $this->assertEquals(['new-owner'], $redis->pipeline()->get($key)->exec());
                    $this->assertEquals($beforeRefresh + 1, $this->clusterSlotsCalls());
                } finally {
                    $nodes[$target]->del($key);
                    foreach ($nodes as $node) {
                        $this->assertTrue($node->rawCommand('CLUSTER', 'SETSLOT', $slot, 'NODE', $ids[$owner]) !== false);
                    }
                    $this->waitForSlotOwner($slot, $ids[$owner]);
                    foreach ([$redis, $populator, $fresh] as $client) {
                        if ($client) $client->close();
                    }
                }
            }
        } finally {
            foreach ($nodes as $node) $node->close();
            ini_set('redis.clusters.cache_slots', $cacheSlots);
        }
    }

    public function testPipelineRealAskDoesNotRefreshRouting() {
        [$key, $slot] = $this->emptySlotKey('pipe-real-ask');
        $nodes = [];
        foreach ($this->redis->_masters() as $master) $nodes[] = $this->connectToNode($master);
        $ids = array_map(function ($node) { return $node->rawCommand('CLUSTER', 'MYID'); }, $nodes);
        $owner = array_search($this->redis->cluster($key, 'MYID'), $ids, true);
        $target = ($owner + 1) % count($nodes);
        $redis = new RedisCluster(null, self::$seeds, 1, 1, false, $this->getAuth());

        try {
            $this->assertTrue($nodes[$target]->rawCommand('CLUSTER', 'SETSLOT', $slot, 'IMPORTING', $ids[$owner]) !== false);
            $this->assertTrue($nodes[$owner]->rawCommand('CLUSTER', 'SETSLOT', $slot, 'MIGRATING', $ids[$target]) !== false);
            $nodes[$target]->rawCommand('ASKING');
            $this->assertTrue($nodes[$target]->set($key, 'importing-node'));
            $before = $this->clusterSlotsCalls();

            for ($attempt = 0; $attempt < 2; $attempt++) {
                $exception = null;
                try {
                    $redis->pipeline()->get($key)->get($key)->exec();
                } catch (RedisClusterException $e) {
                    $exception = $e;
                }
                $this->assertTrue($exception instanceof RedisClusterException);
                $this->assertStringContains('redirected', $exception->getMessage());
                $this->assertEquals(Redis::ATOMIC, $redis->getMode());
                $this->assertEquals($before, $this->clusterSlotsCalls());
                $this->assertEquals('importing-node', $redis->get($key));
            }
            foreach ($nodes as $node) {
                $node->rawCommand('CLUSTER', 'SETSLOT', $slot, 'NODE', $ids[$target]);
            }
            $this->waitForSlotOwner($slot, $ids[$target]);
            $exception = null;
            try {
                $redis->pipeline()->get($key)->exec();
            } catch (RedisClusterException $e) {
                $exception = $e;
            }
            $this->assertTrue($exception instanceof RedisClusterException);
            $this->assertEquals(['importing-node'], $redis->pipeline()->get($key)->exec());
        } finally {
            $nodes[$target]->rawCommand('ASKING');
            $nodes[$target]->del($key);
            foreach ($nodes as $node) {
                $this->assertTrue($node->rawCommand('CLUSTER', 'SETSLOT', $slot, 'STABLE') !== false);
                $this->assertTrue($node->rawCommand('CLUSTER', 'SETSLOT', $slot, 'NODE', $ids[$owner]) !== false);
                $node->close();
            }
            $this->waitForSlotOwner($slot, $ids[$owner]);
            $redis->close();
        }
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
        /* Like legacy MULTI, getLastError describes the final reply only. */
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

    public function testPipelineDistributedPartialKeyPermissionFailures() {
        if (!$this->minVersionCheck('6.0')) $this->markTestSkipped();
        [$allowed, $denied] = $this->keysOnDistinctMasters('pipe-partial-permission');
        $user = 'phpredis-partial-' . uniqid();
        $pass = uniqid('secret-', true);
        $masters = $this->redis->_masters();
        $restricted = null;
        $keys = ["$allowed:set", "$denied:set", "$allowed:nx", "$denied:nx",
                 "$allowed:del", "$denied:del", "$allowed:unlink", "$denied:unlink"];

        try {
            foreach ($masters as $master) {
                $this->assertTrue($this->redis->acl($master, 'SETUSER', $user,
                    'reset', 'on', ">$pass", "~$allowed*", '+@all'));
            }
            $restricted = new RedisCluster(null, self::$seeds, 1, 1, false, [$user, $pass]);
            $this->redis->del($keys);
            /* Legacy immediate MSETNX stops on a denied fragment, but is not globally atomic. */
            $this->assertFalse($restricted->msetnx([$keys[2] => 'one', $keys[3] => 'two']));
            $this->assertEquals(['one', false], $this->redis->mget([$keys[2], $keys[3]]));
            $this->redis->del([$keys[2], $keys[3]]);
            $this->redis->mset([$keys[4] => 'allowed', $keys[5] => 'denied',
                                $keys[6] => 'allowed', $keys[7] => 'denied']);
            $this->assertEquals([false, [1, false], false, false, 'one'],
                $restricted->pipeline()
                    ->mset([$keys[0] => 'one', $keys[1] => 'two'])
                    ->msetnx([$keys[2] => 'one', $keys[3] => 'two'])
                    ->del([$keys[4], $keys[5]])->unlink([$keys[6], $keys[7]])
                    ->get($keys[0])->exec());
            $this->assertEquals(['one', false, 'one', false, false, 'denied', false, 'denied'],
                $this->redis->mget($keys));
            $this->assertEquals(['one'], $restricted->pipeline()->get($keys[0])->exec());
        } finally {
            if ($restricted) $restricted->close();
            foreach ($masters as $master) $this->redis->acl($master, 'DELUSER', $user);
            $this->redis->del($keys);
        }
    }

    public function testPipelineLastErrorMatchesLegacyMulti() {
        $key = '{pipe-last-error}key';
        $this->redis->set($key, 'not-an-integer');
        foreach ([false, true] as $errorLast) {
            $expectedError = null;
            foreach (['multi', 'pipeline'] as $mode) {
                $pipe = $this->redis->$mode();
                if ($errorLast) $pipe->get($key)->incr($key);
                else $pipe->incr($key)->get($key);
                $this->assertEquals($errorLast ? ['not-an-integer', false] : [false, 'not-an-integer'], $pipe->exec());
                if ($mode === 'multi') $expectedError = $this->redis->getLastError();
                else $this->assertEquals($expectedError, $this->redis->getLastError());
            }
        }
        $this->redis->del($key);
    }

    public function testPipelineNonTopologyFailuresDoNotRefreshRouting() {
        $key = '{pipe-no-refresh}key';
        $object = '{pipe-no-refresh}object';
        $this->redis->set($key, 'still-readable');
        $before = $this->clusterSlotsCalls();
        $this->assertEquals(['still-readable'], $this->redis->pipeline()->get($key)->exec());
        $this->assertEquals([false, 'still-readable'], $this->redis->pipeline()->hget($key, 'field')->get($key)->exec());
        $pipe = $this->redis->pipeline()->get($key);
        $this->assertFalse(@$pipe->script($key, 'EXISTS', str_repeat('0', 40)));
        $this->assertEquals(['still-readable'], $pipe->exec());

        $serializer = $this->redis->getOption(Redis::OPT_SERIALIZER);
        try {
            $this->redis->setOption(Redis::OPT_SERIALIZER, Redis::SERIALIZER_PHP);
            $this->redis->set($object, new RedisClusterPipelineReentrantControlValue());
            RedisClusterPipelineReentrantControlValue::$redis = $this->redis;
            RedisClusterPipelineReentrantControlValue::$method = 'exec';
            $exception = null;
            try {
                $this->redis->pipeline()->get($object)->get($key)->exec();
            } catch (RedisClusterException $e) {
                $exception = $e;
            }
            $this->assertTrue($exception instanceof RedisClusterException);
        } finally {
            RedisClusterPipelineReentrantControlValue::$redis = null;
            RedisClusterPipelineReentrantControlValue::$method = null;
            $this->redis->setOption(Redis::OPT_SERIALIZER, $serializer);
            $this->redis->del($object);
        }
        $this->assertEquals(['still-readable'], $this->redis->pipeline()->get($key)->exec());
        $this->assertEquals($before, $this->clusterSlotsCalls());
        $this->redis->del($key);
    }

    public function testPipelineRejectsReentrantCommandsWhileReading() {
        [$valueKey, $sideEffectKey] = $this->keysOnDistinctMasters('pipe-reentrant');
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

    public function testPipelineRejectsReentrantControlMethodsWhileReading() {
        [$key, $control] = $this->keysOnDistinctMasters('pipe-reentrant-control');

        foreach (['exec', 'discard', 'multi', 'pipeline', 'close'] as $method) {
            foreach ([false, true] as $nested) {
                $redis = $this->newInstance();
                try {
                    $redis->setOption(Redis::OPT_SERIALIZER, Redis::SERIALIZER_PHP);
                    $redis->set($key, new RedisClusterPipelineReentrantControlValue());
                    $redis->set($control, 'clean');
                    RedisClusterPipelineReentrantControlValue::$redis = $redis;
                    RedisClusterPipelineReentrantControlValue::$method = $method;
                    $pipe = $redis->pipeline();
                    if ($nested) $pipe->multi();
                    $pipe->get($key)->get($key);
                    if ($nested) $pipe->exec();
                    $pipe->get($control);
                    $exception = null;
                    try {
                        $pipe->exec();
                    } catch (RedisClusterException $e) {
                        $exception = $e;
                    }
                    $this->assertTrue($exception instanceof RedisClusterException);
                    $this->assertStringContains('already executing a pipeline', $exception->getMessage());
                    $this->assertEquals(Redis::ATOMIC, $redis->getMode());
                    $this->assertEquals(['clean'], $redis->pipeline()->get($control)->exec());
                } finally {
                    RedisClusterPipelineReentrantControlValue::$redis = null;
                    RedisClusterPipelineReentrantControlValue::$method = null;
                    $redis->del([$key, $control]);
                    $redis->close();
                }
            }
        }
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
        $keys = $this->keysOnDistinctMasters('pipe-watch-node');
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

    /* The replicas serving $key, as [host, port] pairs */
    private function replicasForKey(string $key): array {
        $master = $this->redis->_masters()[0];
        $slot = $this->redis->rawCommand($master, 'cluster', 'keyslot', $key);

        foreach ($this->redis->rawCommand($master, 'cluster', 'slots') as $entry) {
            if ($slot < $entry[0] || $slot > $entry[1])
                continue;

            /* [start, end, master, replica, ...], each node [host, port, ...] */
            return array_map(function ($node) {
                return [$node[0], $node[1]];
            }, array_slice($entry, 3));
        }

        return [];
    }

    /* Directed RedisCluster commands can only address a node that owns slots,
     * so talk to a replica over a plain Redis connection instead. */
    private function connectToNode(array $node) {
        $redis = new Redis(['host' => $node[0], 'port' => $node[1]]);

        if ($this->getAuth())
            $this->assertTrue($redis->auth($this->getAuth()));

        return $redis;
    }

    /* The number of MOVED replies the passed nodes have sent */
    private function movedCount(array $nodes): int {
        $count = 0;

        foreach ($nodes as $node) {
            $info = $node->info('errorstats');

            if (preg_match('/count=(\d+)/', $info['errorstat_MOVED'] ?? '', $match))
                $count += (int)$match[1];
        }

        return $count;
    }

    /* A replica that hangs up on us is reconnected transparently, but the
     * READONLY we sent doesn't survive the new connection, so we have to send
     * it again or the replica answers MOVED to every read we send it. */
    public function testReplicaReadonlyResentAfterReconnect() {
        if ( ! $this->minVersionCheck('6.2.0'))
            $this->markTestSkipped('INFO ERRORSTATS requires Redis >= 6.2.0');

        $key = 'readonly-reconnect';

        if ( ! ($replicas = $this->replicasForKey($key)))
            $this->markTestSkipped("No replicas serving '$key'");

        $nodes = array_map([$this, 'connectToNode'], $replicas);

        /* Without READONLY every read bounces on MOVED until the redirection
         * loop gives up after timeout + read_timeout, so keep those short or a
         * regression takes the default 60 seconds per read to surface. */
        $client = new RedisCluster(NULL, self::$seeds, 1, 1, false, $this->getAuth());
        $client->setOption(RedisCluster::OPT_SLAVE_FAILOVER,
                           RedisCluster::FAILOVER_DISTRIBUTE_SLAVES);

        $this->assertTrue($client->set($key, 'bar'));

        /* Replication is asynchronous, so make sure every replica has the
         * value before we start reading from them */
        $this->assertEquals(count($nodes),
                            $client->rawCommand($key, 'wait', count($nodes), 1000));

        /* Reads are distributed at random, so make enough of them that we're
         * almost certainly connected to every replica */
        $reads = 10 * count($nodes);

        for ($i = 0; $i < $reads; $i++)
            $this->assertEquals('bar', $client->get($key));

        $moved = $this->movedCount($nodes);

        /* CLIENT KILL skips the connection it's issued on, which is ours */
        foreach ($nodes as $node)
            $this->assertGT(0, $node->rawCommand('client', 'kill', 'type', 'normal'));

        /* Give the close time to reach us, so we detect it before we send
         * rather than while waiting for a reply */
        usleep(100000);

        for ($i = 0; $i < $reads; $i++)
            $this->assertEquals('bar', $client->get($key));

        /* Reconnecting cost us nothing if we sent READONLY again */
        $this->assertEquals($moved, $this->movedCount($nodes));

        $client->del($key);
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

        /* Do not return a connection awaiting the server's close to the pool. */
        $this->redis->close();
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
