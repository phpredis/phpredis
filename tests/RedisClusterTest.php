<?php
require_once(dirname($_SERVER['PHP_SELF'])."/RedisTest.php");

/**
 * Most RedisCluster tests should work the same as the standard Redis object
 * so we only override specific functions where the prototype is different or
 * where we're validating specific cluster mechanisms
 */
class Redis_Cluster_Test extends Redis_Test {
    private $_arr_node_map = Array();

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
}
?>
