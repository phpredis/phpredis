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

        $result = $this->redis->pubsub("{pubsub}", "numsub", Array($c1, $c2));

        // Should get an array back, with two elements
        $this->assertTrue(is_array($result));
        $this->assertEquals(count($result), 2);

        // Make sure the elements are correct, and have zero counts
        foreach(Array($c1,$c2) as $channel) {
            $this->assertTrue(isset($result[$channel]));
            $this->assertEquals($result[$channel], 0);
        }

        // PUBSUB NUMPAT
        $result = $this->redis->pubsub("somekey", "numpat");
        $this->assertTrue(is_int($result));

        // Invalid calls
        $this->assertFalse($this->redis->pubsub("somekey", "notacommand"));
        $this->assertFalse($this->redis->pubsub("somekey", "numsub", "not-an-array"));
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
}
?>
