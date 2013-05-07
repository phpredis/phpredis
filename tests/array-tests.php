<?php

require_once(dirname($_SERVER['PHP_SELF'])."/test.php");
echo "Redis Array tests.\n\n";

function custom_hash($str) {
	// str has the following format: $APPID_fb$FACEBOOKID_$key.
	$pos = strpos($str, '_fb');
	if(preg_match("#\w+_fb(?<facebook_id>\d+)_\w+#", $str, $out)) {
			return $out['facebook_id'];
	}
	return $str;
}

class Redis_Array_Test extends TestSuite
{
	private $strings;
	public $ra = NULL;
	private $data = NULL;

	public function setUp() {

		// initialize strings.
		$n = REDIS_ARRAY_DATA_SIZE;
		$this->strings = array();
		for($i = 0; $i < $n; $i++) {
			$this->strings['key-'.$i] = 'val-'.$i;
		}

		global $newRing, $oldRing, $useIndex;
		$this->ra = new RedisArray($newRing, array('previous' => $oldRing, 'index' => $useIndex));
	}

	public function testMSet() {
		// run mset
		$this->assertTrue(TRUE === $this->ra->mset($this->strings));

		// check each key individually using the array
		foreach($this->strings as $k => $v) {
			$this->assertTrue($v === $this->ra->get($k));
		}

		// check each key individually using a new connection
		foreach($this->strings as $k => $v) {
			list($host, $port) = split(':', $this->ra->_target($k));

			$r = new Redis;
			$r->pconnect($host, (int)$port);
			$this->assertTrue($v === $r->get($k));
		}
	}

	public function testMGet() {
		$this->assertTrue(array_values($this->strings) === $this->ra->mget(array_keys($this->strings)));
	}

	private function addData($commonString) {
		$this->data = array();
		for($i = 0; $i < REDIS_ARRAY_DATA_SIZE; $i++) {
			$k = rand().'_'.$commonString.'_'.rand();
			$this->data[$k] = rand();
		}
		$this->ra->mset($this->data);
	}

	private function checkCommonLocality() {
		// check that they're all on the same node.
		$lastNode = NULL;
		foreach($this->data as $k => $v) {
				$node = $this->ra->_target($k);
				if($lastNode) {
					$this->assertTrue($node === $lastNode);
				}
				$this->assertTrue($this->ra->get($k) == $v);
				$lastNode = $node;
		}
	}

	public function testKeyLocality() {

		// basic key locality with default hash
		$this->addData('{hashed part of the key}');
		$this->checkCommonLocality();

		// with common hashing function
		global $newRing, $oldRing, $useIndex;
		$this->ra = new RedisArray($newRing, array('previous' => $oldRing,
				'index' => $useIndex,
				'function' => 'custom_hash'));

		// basic key locality with custom hash
		$this->addData('fb'.rand());
		$this->checkCommonLocality();
	}

	public function customDistributor($key)
	{
		$a = unpack("N*", md5($key, true));
		global $newRing;
		$pos = abs($a[1]) % count($newRing);

		return $pos;
	}

	public function testKeyDistributor()
	{
		global $newRing, $useIndex;
		$this->ra = new RedisArray($newRing, array(
				'index' => $useIndex,
				'function' => 'custom_hash',
				'distributor' => array($this, "customDistributor")));

		// custom key distribution function.
		$this->addData('fb'.rand());

		// check that they're all on the expected node.
		$lastNode = NULL;
		foreach($this->data as $k => $v) {
			$node = $this->ra->_target($k);
			$pos = $this->customDistributor($k);
			$this->assertTrue($node === $newRing[$pos]);
		}
	}

}

class Redis_Rehashing_Test extends TestSuite
{

	public $ra = NULL;
	private $useIndex;

	// data
	private $strings;
	private $sets;
	private $lists;
	private $hashes;
	private $zsets;

	public function setUp() {

		// initialize strings.
		$n = REDIS_ARRAY_DATA_SIZE;
		$this->strings = array();
		for($i = 0; $i < $n; $i++) {
			$this->strings['key-'.$i] = 'val-'.$i;
		}

		// initialize sets
		for($i = 0; $i < $n; $i++) {
			// each set has 20 elements
			$this->sets['set-'.$i] = range($i, $i+20);
		}

		// initialize lists
		for($i = 0; $i < $n; $i++) {
			// each list has 20 elements
			$this->lists['list-'.$i] = range($i, $i+20);
		}

		// initialize hashes
		for($i = 0; $i < $n; $i++) {
			// each hash has 5 keys
			$this->hashes['hash-'.$i] = array('A' => $i, 'B' => $i+1, 'C' => $i+2, 'D' => $i+3, 'E' => $i+4);
		}

		// initialize sorted sets
		for($i = 0; $i < $n; $i++) {
			// each sorted sets has 5 elements
			$this->zsets['zset-'.$i] = array($i, 'A', $i+1, 'B', $i+2, 'C', $i+3, 'D', $i+4, 'E');
		}

		global $newRing, $oldRing, $useIndex;

		// create array
		$this->ra = new RedisArray($newRing, array('previous' => $oldRing, 'index' => $useIndex));
	}

	public function testFlush() {

		// flush all servers first.
		global $serverList;
		foreach($serverList as $s) {
			list($host, $port) = explode(':', $s);

			$r = new Redis;
			$r->pconnect($host, (int)$port);
			$r->flushdb();
		}
	}


	private function distributeKeys() {

		// strings
		foreach($this->strings as $k => $v) {
			$this->ra->set($k, $v);
		}

		// sets
		foreach($this->sets as $k => $v) {
			call_user_func_array(array($this->ra, 'sadd'), array_merge(array($k), $v));
		}

		// lists
		foreach($this->lists as $k => $v) {
			call_user_func_array(array($this->ra, 'rpush'), array_merge(array($k), $v));
		}

		// hashes
		foreach($this->hashes as $k => $v) {
			$this->ra->hmset($k, $v);
		}

		// sorted sets
		foreach($this->zsets as $k => $v) {
			call_user_func_array(array($this->ra, 'zadd'), array_merge(array($k), $v));
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
		foreach($this->strings as $k => $v) {
			$this->assertTrue($this->ra->get($k) === $v);
		}

		// sets
		foreach($this->sets as $k => $v) {
			$ret = $this->ra->smembers($k); // get values

			// sort sets
			sort($v);
			sort($ret);

			$this->assertTrue($ret == $v);
		}

		// lists
		foreach($this->lists as $k => $v) {
			$ret = $this->ra->lrange($k, 0, -1);
			$this->assertTrue($ret == $v);
		}

		// hashes
		foreach($this->hashes as $k => $v) {
			$ret = $this->ra->hgetall($k); // get values
			$this->assertTrue($ret == $v);
		}

		// sorted sets
		foreach($this->zsets as $k => $v) {
			$ret = $this->ra->zrange($k, 0, -1, TRUE); // get values with scores

			// create assoc array from local dataset
			$tmp = array();
			for($i = 0; $i < count($v); $i += 2) {
				$tmp[$v[$i+1]] = $v[$i];
			}

			// compare to RA value
			$this->assertTrue($ret == $tmp);
		}
	}

	// add a new node.
	public function testCreateSecondRing() {

		global $newRing, $oldRing, $serverList;
		$oldRing = $newRing; // back up the original.
		$newRing = $serverList; // add a new node to the main ring.
	}

	public function testReadUsingFallbackMechanism() {
		$this->readAllvalues();	// some of the reads will fail and will go to another target node.
	}

	public function testRehash() {
		$this->ra->_rehash(); // this will redistribute the keys
	}

	public function testRehashWithCallback() {
		$total = 0;
		$this->ra->_rehash(function ($host, $count) use (&$total) {
			$total += $count;
		});
		$this->assertTrue($total > 0);
	}

	public function testReadRedistributedKeys() {
		$this->readAllvalues(); // we shouldn't have any missed reads now.
	}
}

// Test auto-migration of keys
class Redis_Auto_Rehashing_Test extends TestSuite {

	public $ra = NULL;

	// data
	private $strings;

	public function setUp() {

		// initialize strings.
		$n = REDIS_ARRAY_DATA_SIZE;
		$this->strings = array();
		for($i = 0; $i < $n; $i++) {
			$this->strings['key-'.$i] = 'val-'.$i;
		}

		global $newRing, $oldRing, $useIndex;

		// create array
		$this->ra = new RedisArray($newRing, array('previous' => $oldRing, 'index' => $useIndex, 'autorehash' => TRUE));
	}

	public function testDistribute() {
		// strings
		foreach($this->strings as $k => $v) {
			$this->ra->set($k, $v);
		}
	}

	private function readAllvalues() {
		foreach($this->strings as $k => $v) {
			$this->assertTrue($this->ra->get($k) === $v);
		}
	}


	public function testReadAll() {
		$this->readAllvalues();
	}

	// add a new node.
	public function testCreateSecondRing() {

		global $newRing, $oldRing, $serverList;
		$oldRing = $newRing; // back up the original.
		$newRing = $serverList; // add a new node to the main ring.
	}

	// Read and migrate keys on fallback, causing the whole ring to be rehashed.
	public function testReadAndMigrateAll() {
		$this->readAllvalues();
	}

	// Read and migrate keys on fallback, causing the whole ring to be rehashed.
	public function testAllKeysHaveBeenMigrated() {
		foreach($this->strings as $k => $v) {
			// get the target for each key
			$target = $this->ra->_target($k);

			// connect to the target host
			list($host,$port) = split(':', $target);
			$r = new Redis;
			$r->pconnect($host, $port);

			$this->assertTrue($v === $r->get($k));	// check that the key has actually been migrated to the new node.
		}
	}
}

// Test node-specific multi/exec
class Redis_Multi_Exec_Test extends TestSuite {

	public $ra = NULL;

	public function setUp() {

		global $newRing, $oldRing, $useIndex;
		// create array
		$this->ra = new RedisArray($newRing, array('previous' => $oldRing, 'index' => $useIndex));
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
		foreach(array('name', 'group', 'salary') as $field) {
				$node = $this->ra->_target('1_{employee:joe}_'.$field);
				if($lastNode) {
					$this->assertTrue($node === $lastNode);
				}
				$lastNode = $node;
		}
	}

	public function testMultiExec() {

		// Joe gets a promotion
		$newGroup = $this->ra->get('{groups}:executives');
		$newSalary = 4000;

		// change both in a transaction.
		$host = $this->ra->_target('{employee:joe}');	// transactions are per-node, so we need a reference to it.
		$tr = $this->ra->multi($host)
				->set('1_{employee:joe}_group', $newGroup)
				->set('1_{employee:joe}_salary', $newSalary)
				->exec();

		// check that the group and salary have been changed
		$this->assertTrue($this->ra->get('1_{employee:joe}_group') === $newGroup);
		$this->assertTrue($this->ra->get('1_{employee:joe}_salary') == $newSalary);

	}

	public function testMultiExecMSet() {

		global $newGroup, $newSalary;
		$newGroup = 1;
		$newSalary = 10000;

		// test MSET, making Joe a top-level executive
		$out = $this->ra->multi($this->ra->_target('{employee:joe}'))
				->mset(array('1_{employee:joe}_group' => $newGroup, '1_{employee:joe}_salary' => $newSalary))
				->exec();

		$this->assertTrue($out[0] === TRUE);
	}

	public function testMultiExecMGet() {

		global $newGroup, $newSalary;

		// test MGET
		$out = $this->ra->multi($this->ra->_target('{employee:joe}'))
				->mget(array('1_{employee:joe}_group', '1_{employee:joe}_salary'))
				->exec();

		$this->assertTrue($out[0][0] == $newGroup);
		$this->assertTrue($out[0][1] == $newSalary);
	}

	public function testMultiExecDel() {

		// test DEL
		$out = $this->ra->multi($this->ra->_target('{employee:joe}'))
				->del('1_{employee:joe}_group', '1_{employee:joe}_salary')
				->exec();

		$this->assertTrue($out[0] === 2);
		$this->assertTrue($this->ra->exists('1_{employee:joe}_group') === FALSE);
		$this->assertTrue($this->ra->exists('1_{employee:joe}_salary') === FALSE);
	}

	public function testDiscard() {
		/* phpredis issue #87 */
		$key = 'test_err';

		$this->assertTrue($this->ra->set($key, 'test'));
		$this->assertTrue('test' === $this->ra->get($key));

		$this->ra->watch($key);

		// After watch, same
		$this->assertTrue('test' === $this->ra->get($key));

		// change in a multi/exec block.
		$ret = $this->ra->multi($this->ra->_target($key))->set($key, 'test1')->exec();
		$this->assertTrue($ret === array(true));

		// Get after exec, 'test1':
		$this->assertTrue($this->ra->get($key) === 'test1');

		$this->ra->watch($key);

		// After second watch, still test1.
		$this->assertTrue($this->ra->get($key) === 'test1');

		$ret = $this->ra->multi($this->ra->_target($key))->set($key, 'test2')->discard();
		// Ret after discard: NULL";
		$this->assertTrue($ret === NULL);

		// Get after discard, unchanged:
		$this->assertTrue($this->ra->get($key) === 'test1');
	}

}

// Test custom distribution function
class Redis_Distributor_Test extends TestSuite {

	public $ra = NULL;

	public function setUp() {

		global $newRing, $oldRing, $useIndex;
		// create array
		$this->ra = new RedisArray($newRing, array('previous' => $oldRing, 'index' => $useIndex, 'distributor' => array($this, 'distribute')));
	}

	public function testInit() {
		$this->ra->set('{uk}test', 'joe');
		$this->ra->set('{us}test', 'bob');
	}

	public function distribute($key) {
		$matches = array();
		if (preg_match('/{([^}]+)}.*/', $key, $matches) == 1) {
			$countries = array('uk' => 0, 'us' => 1);
			if (array_key_exists($matches[1], $countries)) {
				return $countries[$matches[1]];
			}
		}
		return 2; // default server
	}

	public function testDistribution() {
		$ukServer = $this->ra->_target('{uk}test');
		$usServer = $this->ra->_target('{us}test');
		$deServer = $this->ra->_target('{de}test');
		$defaultServer = $this->ra->_target('unknown');

		$nodes = $this->ra->_hosts();
		$this->assertTrue($ukServer === $nodes[0]);
		$this->assertTrue($usServer === $nodes[1]);
		$this->assertTrue($deServer === $nodes[2]);
		$this->assertTrue($defaultServer === $nodes[2]);
	}
}

function run_tests($className) {
		// reset rings
		global $newRing, $oldRing, $serverList;
		$newRing = array('localhost:6379', 'localhost:6380', 'localhost:6381');
		$oldRing = array();
		$serverList = array('localhost:6379', 'localhost:6380', 'localhost:6381', 'localhost:6382');

		// run
		TestSuite::run($className);
}

define('REDIS_ARRAY_DATA_SIZE', 1000);

global $useIndex;
foreach(array(true, false) as $useIndex) {

	echo "\n".($useIndex?"WITH":"WITHOUT"). " per-node index:\n";

	run_tests('Redis_Array_Test');
	run_tests('Redis_Rehashing_Test');
	run_tests('Redis_Auto_Rehashing_Test');
	run_tests('Redis_Multi_Exec_Test');
	run_tests('Redis_Distributor_Test');
}

?>
