<?php
require_once 'PHPUnit.php';

echo "Redis Array rehashing tests.\n\n";

class Redis_Rehashing_Test extends PHPUnit_TestCase
{

	public $ra = NULL;
	private $useIndex;

	// data
	private $strings;
	private $sets;
	private $lists;
	private $hashes;
	private $zsets;

	public function setUp()
	{
		// initialize strings.
		$n = 1000;
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

		global $newRing, $oldRing;

		// create array
		$this->ra = new RedisArray($newRing, array('previous' => $oldRing, 'index' => $this->useIndex));
	}

	public function testFlush() {

		// flush all servers first.
		global $serverList;
		foreach($serverList as $s) {
			list($host, $port) = explode(':', $s);

			$r = new Redis;
			$r->connect($host, (int)$port);
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

	public function testReadRedistributedKeys() {
		$this->readAllvalues(); // we shouldn't have any missed reads now.
	}
}

// Test auto-migration of keys
class Redis_Auto_Rehashing_Test extends PHPUnit_TestCase {

	public $ra = NULL;

	// data
	private $strings;

	public function setUp()
	{
		// initialize strings.
		$n = 1000;
		$this->strings = array();
		for($i = 0; $i < $n; $i++) {
			$this->strings['key-'.$i] = 'val-'.$i;
		}

		global $newRing, $oldRing;

		// create array
		$this->ra = new RedisArray($newRing, array('previous' => $oldRing, 'index' => TRUE, 'autorehash' => TRUE));
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
			$r->connect($host, $port);

			$this->assertTrue($v === $r->get($k));	// check that the key has actually been migrated to the new node.
		}
	}
}

// Test node-specific multi/exec
class Redis_Multi_Exec_Test extends PHPUnit_TestCase {

	public $ra = NULL;

	public function setUp()
	{
		global $newRing, $oldRing;
		// create array
		$this->ra = new RedisArray($newRing, array('previous' => $oldRing, 'index' => FALSE));
	}

	public function testInit() {
		$this->ra->set('group:managers', 2);
		$this->ra->set('group:executives', 3);

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
		$newGroup = $this->ra->get('group:executives');
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

}


function run_tests($className) {
		// reset rings
		global $newRing, $oldRing, $serverList;
		$newRing = array('localhost:6379', 'localhost:6380', 'localhost:6381');
		$oldRing = array();
		$serverList = array('localhost:6379', 'localhost:6380', 'localhost:6381', 'localhost:6382');

		// run
		$suite  = new PHPUnit_TestSuite($className);
		$result = PHPUnit::run($suite);
		echo $result->toString();
		echo "\n";
}

run_tests('Redis_Rehashing_Test');
run_tests('Redis_Auto_Rehashing_Test');
run_tests('Redis_Multi_Exec_Test');

?>
