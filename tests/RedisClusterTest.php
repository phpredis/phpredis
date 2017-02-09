<?php defined('PHPREDIS_TESTRUN') or die("Use TestRedis.php to run tests!\n");
require_once(dirname($_SERVER['PHP_SELF'])."/RedisTest.php");

/**
 * Most RedisCluster tests should work the same as the standard Redis object
 * so we only override specific functions where the prototype is different or
 * where we're validating specific cluster mechanisms
 */
class Redis_Cluster_Test extends Redis_Test {
    private static $_arr_node_map = Array();

    private $_arr_redis_types = Array(
        Redis::REDIS_STRING,
        Redis::REDIS_SET,
        Redis::REDIS_LIST,  
        Redis::REDIS_ZSET,
        Redis::REDIS_HASH
    );

    private $_arr_failover_types = Array(
        RedisCluster::FAILOVER_NONE,
        RedisCluster::FAILOVER_ERROR,
        RedisCluster::FAILOVER_DISTRIBUTE
    );

    /* Tests we'll skip all together in the context of RedisCluster.  The 
     * RedisCluster class doesn't implement specialized (non-redis) commands
     * such as sortAsc, or sortDesc and other commands such as SELECT are
     * simply invalid in Redis Cluster */
    public function testSortAsc()  { return $this->markTestSkipped(); }
    public function testSortDesc() { return $this->markTestSkipped(); }
    public function testWait()     { return $this->markTestSkipped(); }
    public function testSelect()   { return $this->markTestSkipped(); }
    public function testReconnectSelect() { return $this->markTestSkipped(); }
    public function testMultipleConnect() { return $this->markTestSkipped(); }

    /* Load our seeds on construction */
    public function __construct() {
        $str_nodemap_file = dirname($_SERVER['PHP_SELF']) . '/nodes/nodemap';

        if (!file_exists($str_nodemap_file)) {
            fprintf(STDERR, "Error:  Can't find nodemap file for seeds!\n");
            exit(1);
        }

        /* Store our node map */
        if (!self::$_arr_node_map) {
            self::$_arr_node_map = array_filter(
                explode("\n", file_get_contents($str_nodemap_file)
            ));
        }
    }

    /* Override setUp to get info from a specific node */
    public function setUp() {
        $this->redis = $this->newInstance();
        $info = $this->redis->info(uniqid());
        $this->version = (isset($info['redis_version'])?$info['redis_version']:'0.0.0');
    }

    /* Override newInstance as we want a RedisCluster object */
    protected function newInstance() {
        return new RedisCluster(NULL, self::$_arr_node_map);
    }

    /* Overrides for RedisTest where the function signature is different.  This
     * is only true for a few commands, which by definition have to be directed
     * at a specific node */

    public function testPing() {
        for ($i = 0; $i < 100; $i++) {
            $this->assertTrue($this->redis->ping("key:$i"));
        }
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
            $this->assertTrue($this->redis->exists($k));
        }
    }

    public function testEcho() {
        $this->assertEquals($this->redis->echo('k1', 'hello'), 'hello');
        $this->assertEquals($this->redis->echo('k2', 'world'), 'world');
        $this->assertEquals($this->redis->echo('k3', " 0123 "), " 0123 ");
    }

    public function testSortPrefix() {
        $this->redis->setOption(Redis::OPT_PREFIX, 'some-prefix:');
        $this->redis->del('some-item');
        $this->redis->sadd('some-item', 1);
        $this->redis->sadd('some-item', 2);
        $this->redis->sadd('some-item', 3);

        $this->assertEquals(array('1','2','3'), $this->redis->sort('some-item'));

        // Kill our set/prefix
        $this->redis->del('some-item');
        $this->redis->setOption(Redis::OPT_PREFIX, '');
    }

    public function testDBSize() {
        for ($i = 0; $i < 10; $i++) {
            $str_key = "key:$i";
            $this->assertTrue($this->redis->flushdb($str_key));
            $this->redis->set($str_key, "val:$i");
            $this->assertEquals(1, $this->redis->dbsize($str_key));
        }
    }

    public function testInfo() {
        $arr_check_keys = Array(
            "redis_version", "arch_bits", "uptime_in_seconds", "uptime_in_days",
            "connected_clients", "connected_slaves", "used_memory",
            "total_connections_received", "total_commands_processed",
            "role"
        );

        for ($i = 0; $i < 3; $i++) {
            $arr_info = $this->redis->info("k:$i");
            foreach ($arr_check_keys as $str_check_key) {
                $this->assertTrue(isset($arr_info[$str_check_key]));
            }
        }
    }

    public function testClient() {
        $str_key = 'key-' . rand(1,100);

        $this->assertTrue($this->redis->client($str_key, 'setname', 'cluster_tests'));

        $arr_clients = $this->redis->client($str_key, 'list');
        $this->assertTrue(is_array($arr_clients));

        /* Find us in the list */
        $str_addr = NULL;
        foreach ($arr_clients as $arr_client) {
            if ($arr_client['name'] == 'cluster_tests') {
                $str_addr = $arr_client['addr'];
                break;
            }
        }

        /* We should be in there */
        $this->assertFalse(empty($str_addr));

        /* Kill our own client! */
        $this->assertTrue($this->redis->client($str_key, 'kill', $str_addr));
    }

    public function testTime() {
        $time_arr = $this->redis->time("k:" . rand(1,100));
        $this->assertTrue(is_array($time_arr) && count($time_arr) == 2 &&
                          strval(intval($time_arr[0])) === strval($time_arr[0]) &&
                          strval(intval($time_arr[1])) === strval($time_arr[1]));
    }

    public function testScan() {
        $i_key_count = 0;
        $i_scan_count = 0;

        /* Have scan retry for us */
        $this->redis->setOption(Redis::OPT_SCAN, Redis::SCAN_RETRY);

        /* Iterate over our masters, scanning each one */
        foreach ($this->redis->_masters() as $arr_master) {
            /* Grab the number of keys we have */
            $i_key_count += $this->redis->dbsize($arr_master);

            /* Scan the keys here */
            $it = NULL;
            while ($arr_keys = $this->redis->scan($it, $arr_master)) {
                $i_scan_count += count($arr_keys);
            }
        }

        /* Our total key count should match */
        $this->assertEquals($i_scan_count, $i_key_count);
    }

    // Run some simple tests against the PUBSUB command.  This is problematic, as we
    // can't be sure what's going on in the instance, but we can do some things.
    public function testPubSub() {
        // PUBSUB CHANNELS ...
        $result = $this->redis->pubsub("somekey", "channels", "*");
        $this->assertTrue(is_array($result));
        $result = $this->redis->pubsub("somekey", "channels");
        $this->assertTrue(is_array($result));

        // PUBSUB NUMSUB

        $c1 = '{pubsub}-' . rand(1,100);
        $c2 = '{pubsub}-' . rand(1,100);

        $result = $this->redis->pubsub("{pubsub}", "numsub", $c1, $c2);

        // Should get an array back, with two elements
        $this->assertTrue(is_array($result));
        $this->assertEquals(count($result), 4);

        $arr_zipped = Array();
        for ($i = 0; $i <= count($result) / 2; $i+=2) {
            $arr_zipped[$result[$i]] = $result[$i+1];
        }
        $result = $arr_zipped;

        // Make sure the elements are correct, and have zero counts
        foreach(Array($c1,$c2) as $channel) {
            $this->assertTrue(isset($result[$channel]));
            $this->assertEquals($result[$channel], 0);
        }

        // PUBSUB NUMPAT
        $result = $this->redis->pubsub("somekey", "numpat");
        $this->assertTrue(is_int($result));

        // Invalid call
        $this->assertFalse($this->redis->pubsub("somekey", "notacommand"));
    }

    /* Unlike Redis proper, MsetNX won't always totally fail if all keys can't
     * be set, but rather will only fail per-node when that is the case */
    public function testMSetNX() {
        /* All of these keys should get set */
        $this->redis->del('x','y','z');
        $ret = $this->redis->msetnx(Array('x'=>'a','y'=>'b','z'=>'c'));
        $this->assertTrue(is_array($ret));
        $this->assertEquals(array_sum($ret),count($ret));

        /* Delete one key */
        $this->redis->del('x');
        $ret = $this->redis->msetnx(Array('x'=>'a','y'=>'b','z'=>'c'));
        $this->assertTrue(is_array($ret));
        $this->assertEquals(array_sum($ret),1);
        
        $this->assertFalse($this->redis->msetnx(array())); // set ø → FALSE
    }

    /* Slowlog needs to take a key or Array(ip, port), to direct it to a node */
    public function testSlowlog() {
        $str_key = uniqid() . '-' . rand(1, 1000);

        $this->assertTrue(is_array($this->redis->slowlog($str_key, 'get')));
        $this->assertTrue(is_array($this->redis->slowlog($str_key, 'get', 10)));
        $this->assertTrue(is_int($this->redis->slowlog($str_key, 'len')));
        $this->assertTrue($this->redis->slowlog($str_key, 'reset'));
        $this->assertFalse($this->redis->slowlog($str_key, 'notvalid'));
    }

    /* INFO COMMANDSTATS requires a key or ip:port for node direction */
    public function testInfoCommandStats() {
        $str_key = uniqid() . '-' . rand(1,1000);
        $arr_info = $this->redis->info($str_key, "COMMANDSTATS");

        $this->assertTrue(is_array($arr_info));
        if (is_array($arr_info)) {
            foreach($arr_info as $k => $str_value) {
                $this->assertTrue(strpos($k, 'cmdstat_') !== false);
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
        $this->assertTrue($ret === Array(FALSE));
        // watch and unwatch
        $this->redis->watch('x');
        $r->incr('x'); // other instance
        $this->redis->unwatch('x'); // cancel transaction watch

        // This should succeed as the watch has been cancelled
        $ret = $this->redis->multi()->get('x')->exec();
        $this->assertTrue($ret === array('44'));
    }

    /* RedisCluster::script() is a 'raw' command, which requires a key such that
     * we can direct it to a given node */
    public function testScript() {
        $str_key = uniqid() . '-' . rand(1,1000);

        // Flush any scripts we have
        $this->assertTrue($this->redis->script($str_key, 'flush'));

        // Silly scripts to test against
        $s1_src = 'return 1';
        $s1_sha = sha1($s1_src);
        $s2_src = 'return 2';
        $s2_sha = sha1($s2_src);
        $s3_src = 'return 3';
        $s3_sha = sha1($s3_src);

        // None should exist
        $result = $this->redis->script($str_key, 'exists', $s1_sha, $s2_sha, $s3_sha);
        $this->assertTrue(is_array($result) && count($result) == 3);
        $this->assertTrue(is_array($result) && count(array_filter($result)) == 0);

        // Load them up
        $this->assertTrue($this->redis->script($str_key, 'load', $s1_src) == $s1_sha);
        $this->assertTrue($this->redis->script($str_key, 'load', $s2_src) == $s2_sha);
        $this->assertTrue($this->redis->script($str_key, 'load', $s3_src) == $s3_sha);

        // They should all exist
        $result = $this->redis->script($str_key, 'exists', $s1_sha, $s2_sha, $s3_sha);
        $this->assertTrue(is_array($result) && count(array_filter($result)) == 3);
    }

    /* RedisCluster::EVALSHA needs a 'key' to let us know which node we want to
     * direct the command at */
    public function testEvalSHA() {
        $str_key = uniqid() . '-' . rand(1,1000);

        // Flush any loaded scripts
        $this->redis->script($str_key, 'flush');

        // Non existant script (but proper sha1), and a random (not) sha1 string
        $this->assertFalse($this->redis->evalsha(sha1(uniqid()),Array($str_key), 1));
        $this->assertFalse($this->redis->evalsha('some-random-data'),Array($str_key), 1);

        // Load a script
        $cb  = uniqid(); // To ensure the script is new
        $scr = "local cb='$cb' return 1";
        $sha = sha1($scr);

        // Run it when it doesn't exist, run it with eval, and then run it with sha1
        $this->assertTrue(false === $this->redis->evalsha($scr,Array($str_key), 1));
        $this->assertTrue(1 === $this->redis->eval($scr,Array($str_key), 1));
        $this->assertTrue(1 === $this->redis->evalsha($sha,Array($str_key), 1));
    }
    
    public function testEvalBulkResponse() {
        $str_key1 = uniqid() . '-' . rand(1,1000) . '{hash}';
        $str_key2 = uniqid() . '-' . rand(1,1000) . '{hash}';

        $this->redis->script($str_key1, 'flush');
        $this->redis->script($str_key2, 'flush');

        $scr = "return {KEYS[1],KEYS[2]}";

        $result = $this->redis->eval($scr,Array($str_key1, $str_key2), 2);

        $this->assertTrue($str_key1 === $result[0]);
        $this->assertTrue($str_key2 === $result[1]);
    }

    public function testEvalBulkResponseMulti() {
        $str_key1 = uniqid() . '-' . rand(1,1000) . '{hash}';
        $str_key2 = uniqid() . '-' . rand(1,1000) . '{hash}';

        $this->redis->script($str_key1, 'flush');
        $this->redis->script($str_key2, 'flush');

        $scr = "return {KEYS[1],KEYS[2]}";

        $this->redis->multi();
        $this->redis->eval($scr,Array($str_key1, $str_key2), 2);

        $result = $this->redis->exec();

        $this->assertTrue($str_key1 === $result[0][0]);
        $this->assertTrue($str_key2 === $result[0][1]);
    }

    public function testEvalBulkEmptyResponse() {
        $str_key1 = uniqid() . '-' . rand(1,1000) . '{hash}';
        $str_key2 = uniqid() . '-' . rand(1,1000) . '{hash}';

        $this->redis->script($str_key1, 'flush');
        $this->redis->script($str_key2, 'flush');

        $scr = "for _,key in ipairs(KEYS) do redis.call('SET', key, 'value') end";

        $result = $this->redis->eval($scr,Array($str_key1, $str_key2), 2);

        $this->assertTrue(null === $result);
    }

    public function testEvalBulkEmptyResponseMulti() {
        $str_key1 = uniqid() . '-' . rand(1,1000) . '{hash}';
        $str_key2 = uniqid() . '-' . rand(1,1000) . '{hash}';

        $this->redis->script($str_key1, 'flush');
        $this->redis->script($str_key2, 'flush');

        $scr = "for _,key in ipairs(KEYS) do redis.call('SET', key, 'value') end";

        $this->redis->multi();
        $this->redis->eval($scr,Array($str_key1, $str_key2), 2);
        $result = $this->redis->exec();

        $this->assertTrue(null === $result[0]);
    }

    /* Cluster specific introspection stuff */
    public function testIntrospection() {
        $arr_masters = $this->redis->_masters();
        $this->assertTrue(is_array($arr_masters));
        
        foreach ($arr_masters as $arr_info) {
            $this->assertTrue(is_array($arr_info));
            $this->assertTrue(is_string($arr_info[0]));
            $this->assertTrue(is_long($arr_info[1]));
        }
    }

    protected function genKeyName($i_key_idx, $i_type) {
        switch ($i_type) { 
            case Redis::REDIS_STRING: 
                return "string-$i_key_idx";
            case Redis::REDIS_SET: 
                return "set-$i_key_idx";
            case Redis::REDIS_LIST:
                return "list-$i_key_idx";
            case Redis::REDIS_ZSET:
                return "zset-$i_key_idx";
            case Redis::REDIS_HASH:
                return "hash-$i_key_idx";
            default:
                return "unknown-$i_key_idx";
        }
    }

    protected function setKeyVals($i_key_idx, $i_type, &$arr_ref) {
        $str_key = $this->genKeyName($i_key_idx, $i_type);

        $this->redis->del($str_key);

        switch ($i_type) {
            case Redis::REDIS_STRING:
                $value = "$str_key-value";
                $this->redis->set($str_key, $value); 
                break;
            case Redis::REDIS_SET:
                $value = Array(
                    $str_key . '-mem1', $str_key . '-mem2', $str_key . '-mem3',
                    $str_key . '-mem4', $str_key . '-mem5', $str_key . '-mem6'
                );
                $arr_args = $value;
                array_unshift($arr_args, $str_key);
                call_user_func_array(Array($this->redis, 'sadd'), $arr_args);
                break;
            case Redis::REDIS_HASH:
                $value = Array(
                    $str_key . '-mem1' => $str_key . '-val1',
                    $str_key . '-mem2' => $str_key . '-val2',
                    $str_key . '-mem3' => $str_key . '-val3'
                );
                $this->redis->hmset($str_key, $value);
                break; 
            case Redis::REDIS_LIST:
                $value = Array(
                    $str_key . '-ele1', $str_key . '-ele2', $str_key . '-ele3',
                    $str_key . '-ele4', $str_key . '-ele5', $str_key . '-ele6'
                );
                $arr_args = $value;
                array_unshift($arr_args, $str_key);
                call_user_func_array(Array($this->redis, 'rpush'), $arr_args);
                break;
            case Redis::REDIS_ZSET:
                $i_score = 1;
                $value = Array(
                    $str_key . '-mem1' => 1, $str_key . '-mem2' => 2,
                    $str_key . '-mem3' => 3, $str_key . '-mem3' => 3
                );
                foreach ($value as $str_mem => $i_score) {
                    $this->redis->zadd($str_key, $i_score, $str_mem);
                }
                break; 
        }

        /* Update our reference array so we can verify values */
        $arr_ref[$str_key] = $value;
        return $str_key;
    }

    /* Verify that our ZSET values are identical */
    protected function checkZSetEquality($a, $b) {
        /* If the count is off, the array keys are different or the sums are
         * different, we know there is something off */
        $boo_diff = count($a) != count($b) ||
            count(array_diff(array_keys($a), array_keys($b))) != 0 ||
            array_sum($a) != array_sum($b);

        if ($boo_diff) {
            $this->assertEquals($a,$b);
            return;
        }
    }

    protected function checkKeyValue($str_key, $i_type, $value) {
        switch ($i_type) {
            case Redis::REDIS_STRING:
                $this->assertEquals($value, $this->redis->get($str_key));
                break;
            case Redis::REDIS_SET:
                $arr_r_values = $this->redis->sMembers($str_key);
                $arr_l_values = $value;
                sort($arr_r_values);
                sort($arr_l_values);
                $this->assertEquals($arr_r_values, $arr_l_values);
                break;
            case Redis::REDIS_LIST:
                $this->assertEquals($value, $this->redis->lrange($str_key,0,-1));
                break;
            case Redis::REDIS_HASH:
                $this->assertEquals($value, $this->redis->hgetall($str_key));
                break;
            case Redis::REDIS_ZSET:
                $this->checkZSetEquality($value, $this->redis->zrange($str_key,0,-1,true));
                break;
            default:
                throw new Exception("Unknown type " . $i_type);
        }
    }

    /* Test automatic load distributor */
    public function testFailOver() {
        $arr_value_ref = Array();
        $arr_type_ref  = Array();

        /* Set a bunch of keys of various redis types*/
        for ($i = 0; $i < 200; $i++) {
            foreach ($this->_arr_redis_types as $i_type) {
                $str_key = $this->setKeyVals($i, $i_type, $arr_value_ref);
                $arr_type_ref[$str_key] = $i_type;                
            }
        }

        /* Iterate over failover options */
        foreach ($this->_arr_failover_types as $i_opt) {
            $this->redis->setOption(RedisCluster::OPT_SLAVE_FAILOVER, $i_opt);

            foreach ($arr_value_ref as $str_key => $value) {
                $this->checkKeyValue($str_key, $arr_type_ref[$str_key], $value);
            }

            break;
        }    
    }

    /* Test a 'raw' command */
    public function testRawCommand() {
        $this->redis->rawCommand('mykey', 'set', 'mykey', 'my-value');
        $this->assertEquals($this->redis->get('mykey'), 'my-value');

        $this->redis->del('mylist');
        $this->redis->rpush('mylist', 'A','B','C','D');
        $this->assertEquals($this->redis->lrange('mylist', 0, -1), Array('A','B','C','D'));
    }

    protected function rawCommandArray($key, $args) {
        array_unshift($args, $key);
        return call_user_func_array(Array($this->redis, 'rawCommand'), $args);
    }

    public function testSession()
    {
        ini_set('session.save_handler', 'rediscluster');
        ini_set('session.save_path', implode('&', array_map(function ($seed) {
            return 'seed[]=' . $seed;
        }, self::$_arr_node_map)) . '&failover=error');
        if (!@session_start()) {
            return $this->markTestSkipped();
        }
        session_write_close();
        $this->assertTrue($this->redis->exists('PHPREDIS_CLUSTER_SESSION:' . session_id()));
    }
}
?>
