<?php
require_once(dirname($_SERVER['PHP_SELF'])."/RedisTest.php");

/**
 * Most RedisCluster tests should work the same as the standard Redis object
 * so we only override specific functions where the prototype is different or
 * where we're validating specific cluster mechanisms
 */
class Redis_Cluster_Test extends Redis_Test {
    private $_arr_node_map = Array();

    /* Tests we'll skip all together in the context of RedisCluster.  The 
     * RedisCluster class doesn't implement specialized (non-redis) commands
     * such as sortAsc, or sortDesc and other commands such as SELECT are
     * simply invalid in Redis Cluster */
    public function testSortAsc()  { return $this->markTestSkipped(); }
    public function testSortDesc() { return $this->markTestSkipped(); }
    public function testWait()     { return $this->markTestSkipped(); }
    public function testSelect()   { return $this->markTestSkipped(); }
    public function testReconnectSelect() { return $this->markTestSkipped(); }
    public function testIntrospection() { return $this->markTestSkipped(); }

    /* Skips for now, which need attention */
    public function testClient()   { return $this->markTestSkipped(); }

    /* Load our seeds on construction */
    public function __construct() {
        $str_nodemap_file = dirname($_SERVER['PHP_SELF']) . '/nodes/nodemap';

        if (!file_exists($str_nodemap_file)) {
            fprintf(STDERR, "Error:  Can't find nodemap file for seeds!\n");
            exit(1);
        }

        /* Store our node map */
        $this->_arr_node_map = array_filter(
            explode("\n", file_get_contents($str_nodemap_file)
        ));
    }

    /* Override setUp to get info from a specific node */
    public function setUp() {
        $this->redis = $this->newInstance();
        $info = $this->redis->info(uniqid());
        $this->version = (isset($info['redis_version'])?$info['redis_version']:'0.0.0');
    }

    /* Override newInstance as we want a RedisCluster object */
    protected function newInstance() {
        return new RedisCluster(NULL, $this->_arr_node_map);
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

    public function testTime() {
        $time_arr = $this->redis->time("k:" . rand(1,100));
        $this->assertTrue(is_array($time_arr) && count($time_arr) == 2 &&
                          strval(intval($time_arr[0])) === strval($time_arr[0]) &&
                          strval(intval($time_arr[1])) === strval($time_arr[1]));
    }

    public function testScan() {
        $this->markTestSkipped(); // this will be implemented
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

        $ret = $this->redis->multi()->get('x')->exec();
        $this->assertTrue($ret === Array(FALSE)); // failed because another client changed our watched key between WATCH and EXEC.

        // watch and unwatch
        $this->redis->watch('x');
        $r->incr('x'); // other instance
        $this->redis->unwatch('x'); // cancel transaction watch

        $ret = $this->redis->multi()->get('x')->exec();
        $this->assertTrue($ret === array('44')); // succeeded since we've cancel the WATCH command.
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
}
?>
