<?php defined('PHPREDIS_TESTRUN') or die("Use TestRedis.php to run tests!\n");

require_once __DIR__ . "/RedisTest.php";

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


    /* Tests we'll skip all together in the context of RedisCluster.  The
     * RedisCluster class doesn't implement specialized (non-redis) commands
     * such as sortAsc, or sortDesc and other commands such as SELECT are
     * simply invalid in Redis Cluster */
    public function testPipelinePublish() { $this->markTestSkipped(); }
    public function testSortAsc()  { $this->markTestSkipped(); }
    public function testSortDesc() { $this->markTestSkipped(); }
    public function testWait()     { $this->markTestSkipped(); }
    public function testSelect()   { $this->markTestSkipped(); }
    public function testReconnectSelect() { $this->markTestSkipped(); }
    public function testMultipleConnect() { $this->markTestSkipped(); }
    public function testDoublePipeNoOp() { $this->markTestSkipped(); }
    public function testSwapDB() { $this->markTestSkipped(); }
    public function testConnectException() { $this->markTestSkipped(); }
    public function testTlsConnect() { $this->markTestSkipped(); }
    public function testReset() { $this->markTestSkipped(); }
    public function testInvalidAuthArgs() { $this->markTestSkipped(); }
    public function testScanErrors() { $this->markTestSkipped(); }

    /* These 'directed node' commands work differently in RedisCluster */
    public function testConfig() { $this->markTestSkipped(); }
    public function testFlushDB() { $this->markTestSkipped(); }
    public function testFunction() { $this->markTestSkipped(); }

    /* Session locking feature is currently not supported in in context of Redis Cluster.
       The biggest issue for this is the distribution nature of Redis cluster */
    public function testSession_lockKeyCorrect() { $this->markTestSkipped(); }
    public function testSession_lockingDisabledByDefault() { $this->markTestSkipped(); }
    public function testSession_lockReleasedOnClose() { $this->markTestSkipped(); }
    public function testSession_ttlMaxExecutionTime() { $this->markTestSkipped(); }
    public function testSession_ttlLockExpire() { $this->markTestSkipped(); }
    public function testSession_lockHoldCheckBeforeWrite_otherProcessHasLock() { $this->markTestSkipped(); }
    public function testSession_lockHoldCheckBeforeWrite_nobodyHasLock() { $this->markTestSkipped(); }
    public function testSession_correctLockRetryCount() { $this->markTestSkipped(); }
    public function testSession_defaultLockRetryCount() { $this->markTestSkipped(); }
    public function testSession_noUnlockOfOtherProcess() { $this->markTestSkipped(); }
    public function testSession_lockWaitTime() { $this->markTestSkipped(); }

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
    public function __construct($host, $port, $auth) {
        parent::__construct($host, $port, $auth);

        self::$seeds = $this->loadSeeds($host, $port);
    }

    /* Override setUp to get info from a specific node */
    public function setUp() {
        $this->redis    = $this->newInstance();
        $info           = $this->redis->info(uniqid());
        $this->version  = $info['redis_version'] ?? '0.0.0';
        $this->is_keydb = $this->detectKeyDB($info);
        $this->is_valkey = $this->detectValkey($info);
    }

    /* Override newInstance as we want a RedisCluster object */
    protected function newInstance() {
        try {
            return new RedisCluster(NULL, self::$seeds, 30, 30, true, $this->getAuth());
        } catch (Exception $ex) {
            TestSuite::errorMessage("Fatal error: %s\n", $ex->getMessage());
            TestSuite::errorMessage("Seeds: %s\n", implode(' ', self::$seeds));
            TestSuite::errorMessage("Seed source: %s\n", self::$seed_source);
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
