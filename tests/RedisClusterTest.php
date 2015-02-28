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
        if (!file_exists('nodes/nodemap')) {
            fprintf(STDERR, "Error:  Can't find nodemap file for seeds!\n");
            exit(1);
        }

        /* Store our node map */
        $this->_arr_node_map = array_filter(file_get_contents('nodes/nodemap'));
    }

    /* Override newInstance as we want a RedisCluster object */
    protected function newInstance() {
        return new RedisCluster(NULL, $this->_arr_node_map);
    }
}
