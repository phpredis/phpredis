<?php defined('PHPREDIS_TESTRUN') or die("Use TestRedis.php to run tests!\n");

require_once __DIR__ . "/TestSuite.php";

define('REDIS_ARRAY_DATA_SIZE', 1000);
define('REDIS_RA_DEFAULT_PORTS', [6379, 6380, 6381, 6382]);

function custom_hash($str) {
    // str has the following format: $APPID_fb$FACEBOOKID_$key.
    $pos = strpos($str, '_fb');
    if (preg_match("#\w+_fb(?<facebook_id>\d+)_\w+#", $str, $out)) {
            return $out['facebook_id'];
    }
    return $str;
}

function parseHostPort($str, &$host, &$port) {
    $pos = strrpos($str, ':');
    $host = substr($str, 0, $pos);
    $port = substr($str, $pos+1);
}

function getRedisVersion(object $client) {
    $arr_info = $client->info();
    if ( ! $arr_info || !isset($arr_info['redis_version'])) {
        return '0.0.0';
    }
    return $arr_info['redis_version'];
}

/* Determine the lowest redis version attached to this RedisArray object */
function getMinVersion(object $ra) {
    $min_version = '0.0.0';
    foreach ($ra->_hosts() as $host) {
        $version = getRedisVersion($ra->_instance($host));
        if (version_compare($version, $min_version) > 0) {
            $min_version = $version;
        }
    }

    return $min_version;
}

class Redis_Array_Test extends TestSuite
{
    private $min_version;
    private $strings;
    public  $ra = NULL;
    private $data = NULL;

    public function setUp() {
        // initialize strings.
        $n = REDIS_ARRAY_DATA_SIZE;
        $this->strings = [];
        for ($i = 0; $i < $n; $i++) {
            $this->strings['key-'.$i] = 'val-'.$i;
        }

        global $new_ring, $old_ring, $use_index;
        $options = ['previous' => $old_ring, 'index' => $use_index];
        if ($this->getAuth()) {
            $options['auth'] = $this->getAuth();
        }

        $this->ra = new RedisArray($new_ring, $options);
        $this->min_version = getMinVersion($this->ra);
    }

    public function testMSet() {
        // run mset
        $this->assertTrue($this->ra->mset($this->strings));

        // check each key individually using the array
        foreach ($this->strings as $k => $v) {
            $this->assertEquals($v, $this->ra->get($k));
        }

        // check each key individually using a new connection
        foreach ($this->strings as $k => $v) {
            parseHostPort($this->ra->_target($k), $host, $port);

            $target = $this->ra->_target($k);
            $pos = strrpos($target, ':');

            $host = substr($target, 0, $pos);
            $port = substr($target, $pos+1);

            $r = new Redis;
            $r->pconnect($host, (int)$port);
            if ($this->getAuth()) {
                $this->assertTrue($r->auth($this->getAuth()));
            }
            $this->assertEquals($v, $r->get($k));
        }
    }

    public function testMGet() {
        $this->assertEquals(array_values($this->strings), $this->ra->mget(array_keys($this->strings)));
    }

    private function addData($commonString) {
        $this->data = [];
        for ($i = 0; $i < REDIS_ARRAY_DATA_SIZE; $i++) {
            $k = rand().'_'.$commonString.'_'.rand();
            $this->data[$k] = rand();
        }
        $this->ra->mset($this->data);
    }

    private function checkCommonLocality() {
        // check that they're all on the same node.
        $lastNode = NULL;
        foreach ($this->data as $k => $v) {
                $node = $this->ra->_target($k);
                if ($lastNode) {
                    $this->assertEquals($node, $lastNode);
                }
                $this->assertEqualsWeak($v, $this->ra->get($k));
                $lastNode = $node;
        }
    }

    public function testKeyLocality() {

        // basic key locality with default hash
        $this->addData('{hashed part of the key}');
        $this->checkCommonLocality();

        // with common hashing function
        global $new_ring, $old_ring, $use_index;
        $options = [
            'previous' => $old_ring,
            'index' => $use_index,
            'function' => 'custom_hash'
        ];
        if ($this->getAuth()) {
            $options['auth'] = $this->getAuth();
        }
        $this->ra = new RedisArray($new_ring, $options);

        // basic key locality with custom hash
        $this->addData('fb'.rand());
        $this->checkCommonLocality();
    }

    public function customDistributor($key)
    {
        $a = unpack("N*", md5($key, true));
        global $new_ring;
        $pos = abs($a[1]) % count($new_ring);

        return $pos;
    }

    public function testKeyDistributor()
    {
        global $new_ring, $useIndex;

        $options = [
            'index'       => $useIndex,
            'function'    => 'custom_hash',
            'distributor' => [$this, "customDistributor"]
        ];

        if ($this->getAuth()) {
            $options['auth'] = $this->getAuth();
        }

        $this->ra = new RedisArray($new_ring, $options);

        // custom key distribution function.
        $this->addData('fb'.rand());

        // check that they're all on the expected node.
        $lastNode = NULL;
        foreach ($this->data as $k => $v) {
            $node = $this->ra->_target($k);
            $pos = $this->customDistributor($k);
            $this->assertEquals($node, $new_ring[$pos]);
        }
    }

    /* Scan a whole key and return the overall result */
    protected function execKeyScan($cmd, $key) {
        $res = [];

        $it = NULL;
        do {
            $chunk = $this->ra->$cmd($key, $it);
            foreach ($chunk as $field => $value) {
                $res[$field] = $value;
            }
        } while ($it !== 0);

        return $res;
    }

    public function testKeyScanning() {
        $h_vals = ['foo' => 'bar', 'baz' => 'bop'];
        $z_vals = ['one' => 1, 'two' => 2, 'three' => 3];
        $s_vals = ['mem1', 'mem2', 'mem3'];

        $this->ra->del(['scan-hash', 'scan-set', 'scan-zset']);

        $this->ra->hMSet('scan-hash', $h_vals);
        foreach ($z_vals as $k => $v)
            $this->ra->zAdd('scan-zset', $v, $k);
        $this->ra->sAdd('scan-set', ...$s_vals);

        $s_scan = $this->execKeyScan('sScan', 'scan-set');
        $this->assertTrue(count(array_diff_key(array_flip($s_vals), array_flip($s_scan))) == 0);

        $this->assertEquals($h_vals, $this->execKeyScan('hScan', 'scan-hash'));

        $z_scan = $this->execKeyScan('zScan', 'scan-zset');
        $this->assertTrue(count($z_scan) == count($z_vals) &&
                          count(array_diff_key($z_vals, $z_scan)) == 0 &&
                          array_sum($z_scan) == array_sum($z_vals));
    }
}

class Redis_Rehashing_Test extends TestSuite
{

    public $ra = NULL;
    private $useIndex;

    private $min_version;

    // data
    private $strings;
    private $sets;
    private $lists;
    private $hashes;
    private $zsets;

    public function setUp() {

        // initialize strings.
        $n = REDIS_ARRAY_DATA_SIZE;
        $this->strings = [];
        for ($i = 0; $i < $n; $i++) {
            $this->strings['key-'.$i] = 'val-'.$i;
        }

        // initialize sets
        for ($i = 0; $i < $n; $i++) {
            // each set has 20 elements
            $this->sets['set-'.$i] = range($i, $i+20);
        }

        // initialize lists
        for ($i = 0; $i < $n; $i++) {
            // each list has 20 elements
            $this->lists['list-'.$i] = range($i, $i+20);
        }

        // initialize hashes
        for ($i = 0; $i < $n; $i++) {
            // each hash has 5 keys
            $this->hashes['hash-'.$i] = ['A' => $i, 'B' => $i+1, 'C' => $i+2, 'D' => $i+3, 'E' => $i+4];
        }

        // initialize sorted sets
        for ($i = 0; $i < $n; $i++) {
            // each sorted sets has 5 elements
            $this->zsets['zset-'.$i] = [$i, 'A', $i+1, 'B', $i+2, 'C', $i+3, 'D', $i+4, 'E'];
        }

        global $new_ring, $old_ring, $useIndex;
        $options = [
            'previous' => $old_ring,
            'index' => $useIndex
        ];
        if ($this->getAuth()) {
            $options['auth'] = $this->getAuth();
        }
        // create array
        $this->ra = new RedisArray($new_ring, $options);
        $this->min_version = getMinVersion($this->ra);
    }

    public function testFlush() {
        // flush all servers first.
        global $server_list;
        foreach ($server_list as $s) {
            parseHostPort($s, $host, $port);

            $r = new Redis();
            $r->pconnect($host, (int)$port, 0);
            if ($this->getAuth()) {
                $this->assertTrue($r->auth($this->getAuth()));
            }
            $this->assertTrue($r->flushdb());
        }
    }


    private function distributeKeys() {

        // strings
        foreach ($this->strings as $k => $v) {
            $this->ra->set($k, $v);
        }

        // sets
        foreach ($this->sets as $k => $v) {
            call_user_func_array([$this->ra, 'sadd'], array_merge([$k], $v));
        }

        // lists
        foreach ($this->lists as $k => $v) {
            call_user_func_array([$this->ra, 'rpush'], array_merge([$k], $v));
        }

        // hashes
        foreach ($this->hashes as $k => $v) {
            $this->ra->hmset($k, $v);
        }

        // sorted sets
        foreach ($this->zsets as $k => $v) {
            call_user_func_array([$this->ra, 'zadd'], array_merge([$k], $v));
        }
    }

    public function testDistribution() {
        $this->distributeKeys();
    }

    public function testSimpleRead() {
        $this->readAllvalues();
    }

    private function readAllvalues() {

        // strings
        foreach ($this->strings as $k => $v) {
            $this->assertEquals($v, $this->ra->get($k));
        }

        // sets
        foreach ($this->sets as $k => $v) {
            $ret = $this->ra->smembers($k); // get values

            $this->assertEqualsWeak($v, $ret);
        }

        // lists
        foreach ($this->lists as $k => $v) {
            $ret = $this->ra->lrange($k, 0, -1);
            $this->assertEqualsWeak($v, $ret);
        }

        // hashes
        foreach ($this->hashes as $k => $v) {
            $ret = $this->ra->hgetall($k); // get values
            $this->assertEqualsWeak($v, $ret);
        }

        // sorted sets
        foreach ($this->zsets as $k => $v) {
            $ret = $this->ra->zrange($k, 0, -1, true);

            // create assoc array from local dataset
            $tmp = [];
            for ($i = 0; $i < count($v); $i += 2) {
                $tmp[$v[$i+1]] = $v[$i];
            }

            // compare to RA value
            $this->assertEqualsWeak($tmp, $ret);
        }
    }

    // add a new node.
    public function testCreateSecondRing() {
        global $new_ring, $old_ring, $server_list;
        $old_ring = $new_ring; // back up the original.
        $new_ring = $server_list; // add a new node to the main ring.
    }

    public function testReadUsingFallbackMechanism() {
        $this->readAllvalues(); // some of the reads will fail and will go to another target node.
    }

    public function testRehash() {
        $this->ra->_rehash(); // this will redistribute the keys
    }

    public function testRehashWithCallback() {
        $total = 0;
        $this->ra->_rehash(function ($host, $count) use (&$total) {
            $total += $count;
        });
        $this->assertGT(0, $total);
    }

    public function testReadRedistributedKeys() {
        $this->readAllvalues(); // we shouldn't have any missed reads now.
    }
}

// Test auto-migration of keys
class Redis_Auto_Rehashing_Test extends TestSuite {

    public $ra = NULL;
    private $min_version;

    // data
    private $strings;

    public function setUp() {
        // initialize strings.
        $n = REDIS_ARRAY_DATA_SIZE;
        $this->strings = [];
        for ($i = 0; $i < $n; $i++) {
            $this->strings['key-'.$i] = 'val-'.$i;
        }

        global $new_ring, $old_ring, $useIndex;
        $options = [
            'previous' => $old_ring,
            'index' => $useIndex,
            'autorehash' => true
        ];
        if ($this->getAuth()) {
            $options['auth'] = $this->getAuth();
        }
        // create array
        $this->ra = new RedisArray($new_ring, $options);
        $this->min_version = getMinVersion($this->ra);
    }

    public function testDistribute() {
        // strings
        foreach ($this->strings as $k => $v) {
            $this->ra->set($k, $v);
        }
    }

    private function readAllvalues() {
        foreach ($this->strings as $k => $v) {
            $this->assertEquals($v, $this->ra->get($k));
        }
    }


    public function testReadAll() {
        $this->readAllvalues();
    }

    // add a new node.
    public function testCreateSecondRing() {
        global $new_ring, $old_ring, $server_list;
        $old_ring = $new_ring; // back up the original.
        $new_ring = $server_list; // add a new node to the main ring.
    }

    // Read and migrate keys on fallback, causing the whole ring to be rehashed.
    public function testReadAndMigrateAll() {
        $this->readAllvalues();
    }

    // Read and migrate keys on fallback, causing the whole ring to be rehashed.
    public function testAllKeysHaveBeenMigrated() {
        foreach ($this->strings as $k => $v) {
            parseHostPort($this->ra->_target($k), $host, $port);

            $r = new Redis;
            $r->pconnect($host, $port);
            if ($this->getAuth()) {
                $this->assertTrue($r->auth($this->getAuth()));
            }

            // check that the key has actually been migrated to the new node.
            $this->assertEquals($v, $r->get($k));
        }
    }
}

// Test node-specific multi/exec
class Redis_Multi_Exec_Test extends TestSuite {
    private $min_version;

    public $ra = NULL;

    private static $new_group  = NULL;
    private static $new_salary = NULL;

    public function setUp() {
        global $new_ring, $old_ring, $useIndex;
        $options = ['previous' => $old_ring, 'index' => $useIndex];
        if ($this->getAuth()) {
            $options['auth'] = $this->getAuth();
        }
        // create array
        $this->ra = new RedisArray($new_ring, $options);
        $this->min_version = getMinVersion($this->ra);
    }

    public function testInit() {
        $this->ra->set('{groups}:managers', 2);
        $this->ra->set('{groups}:executives', 3);

        $this->ra->set('1_{employee:joe}_name', 'joe');
        $this->ra->set('1_{employee:joe}_group', 2);
        $this->ra->set('1_{employee:joe}_salary', 2000);
    }

    public function testKeyDistribution() {
        // check that all of joe's keys are on the same instance
        $lastNode = NULL;
        foreach (['name', 'group', 'salary'] as $field) {
            $node = $this->ra->_target('1_{employee:joe}_'.$field);
            if ($lastNode) {
                $this->assertEquals($node, $lastNode);
            }
            $lastNode = $node;
        }
    }

    public function testMultiExec() {
        // Joe gets a promotion
        self::$new_group  = $this->ra->get('{groups}:executives');
        self::$new_salary = 4000;

        // change both in a transaction.
        // transactions are per-node, so we need a reference to it.
        $host = $this->ra->_target('{employee:joe}');
        $this->ra->multi($host)
            ->set('1_{employee:joe}_group', self::$new_group)
            ->set('1_{employee:joe}_salary', self::$new_salary)
            ->exec();

        // check that the group and salary have been changed
        $this->assertEquals(self::$new_group, $this->ra->get('1_{employee:joe}_group'));
        $this->assertEqualsWeak(self::$new_salary, $this->ra->get('1_{employee:joe}_salary'));

    }

    public function testMultiExecMSet() {
        self::$new_group = 1;
        self::$new_salary = 10000;

        // test MSET, making Joe a top-level executive
        $out = $this->ra->multi($this->ra->_target('{employee:joe}'))
                ->mset([
                    '1_{employee:joe}_group' => self::$new_group,
                    '1_{employee:joe}_salary' => self::$new_salary
                ])
                ->exec();

        $this->assertTrue($out[0]);
    }

    public function testMultiExecMGet() {
        $out = $this->ra->multi($this->ra->_target('{employee:joe}'))
                ->mget(['1_{employee:joe}_group', '1_{employee:joe}_salary'])
                ->exec();

        $this->assertEqualsWeak(self::$new_group, $out[0][0]);
        $this->assertEqualsWeak(self::$new_salary, $out[0][1]);
    }

    public function testMultiExecDel() {
        $out = $this->ra->multi($this->ra->_target('{employee:joe}'))
            ->del('1_{employee:joe}_group', '1_{employee:joe}_salary')
            ->exec();

        $this->assertEquals(2, $out[0]);
        $this->assertEquals(0, $this->ra->exists('1_{employee:joe}_group'));
        $this->assertEquals(0, $this->ra->exists('1_{employee:joe}_salary'));
    }

    public function testMultiExecUnlink() {
        if (version_compare($this->min_version, "4.0.0", "lt")) {
            $this->markTestSkipped();
        }

        $this->ra->set('{unlink}:key1', 'bar');
        $this->ra->set('{unlink}:key2', 'bar');

        $out = $this->ra->multi($this->ra->_target('{unlink}'))
            ->del('{unlink}:key1', '{unlink}:key2')
            ->exec();

        $this->assertEquals(2, $out[0]);
    }

    public function testDiscard() {
        /* phpredis issue #87 */
        $key = 'test_err';

        $this->assertTrue($this->ra->set($key, 'test'));
        $this->assertEquals('test', $this->ra->get($key));

        $this->ra->watch($key);

        // After watch, same
        $this->assertEquals('test', $this->ra->get($key));

        // change in a multi/exec block.
        $ret = $this->ra->multi($this->ra->_target($key))->set($key, 'test1')->exec();
        $this->assertEquals([true], $ret);

        // Get after exec, 'test1':
        $this->assertEquals('test1', $this->ra->get($key));

        $this->ra->watch($key);

        // After second watch, still test1.
        $this->assertEquals('test1', $this->ra->get($key));

        $ret = $this->ra->multi($this->ra->_target($key))->set($key, 'test2')->discard();
        // Ret after discard: NULL";
        $this->assertNull($ret);

        // Get after discard, unchanged:
        $this->assertEquals('test1', $this->ra->get($key));
    }
}

// Test custom distribution function
class Redis_Distributor_Test extends TestSuite {

    public $ra = NULL;
    private $min_version;

    public function setUp() {
        global $new_ring, $old_ring, $useIndex;
        $options = [
            'previous' => $old_ring,
            'index' => $useIndex,
            'distributor' => [$this, 'distribute']
        ];
        if ($this->getAuth()) {
            $options['auth'] = $this->getAuth();
        }
        // create array
        $this->ra = new RedisArray($new_ring, $options);
        $this->min_version = getMinVersion($this->ra);
    }

    public function testInit() {
        $this->ra->set('{uk}test', 'joe');
        $this->ra->set('{us}test', 'bob');
    }

    public function distribute($key) {
        $matches = [];
        if (preg_match('/{([^}]+)}.*/', $key, $matches) == 1) {
            $countries = ['uk' => 0, 'us' => 1];
            if (array_key_exists($matches[1], $countries)) {
                return $countries[$matches[1]];
            }
        }
        return 2; // default server
    }

    public function testDistribution() {
        $UK_server = $this->ra->_target('{uk}test');
        $US_server = $this->ra->_target('{us}test');
        $DE_server = $this->ra->_target('{de}test');
        $XX_server = $this->ra->_target('{xx}test');

        $nodes = $this->ra->_hosts();

        $this->assertEquals($UK_server, $nodes[0]);
        $this->assertEquals($US_server, $nodes[1]);
        $this->assertEquals($DE_server, $nodes[2]);
        $this->assertEquals($XX_server, $nodes[2]);
    }
}

function run_ra_tests($test_class, $filter, $host, array $full_ring,
                      array $sub_ring, $auth)
{
    global $new_ring, $old_ring, $server_list;

    $server_list = $full_ring;
    $new_ring    = $sub_ring;
    $old_ring    = [];

    return TestSuite::run($test_class, $filter, $host, NULL, $auth);
}

?>
