<?php
require_once 'PHPUnit.php';

echo "Redis Array rehashing tests.\n";

$newRing = array('localhost:6379', 'localhost:6380', 'localhost:6381');
$oldRing = array();
$serverList = array('localhost:6379', 'localhost:6380', 'localhost:6381', 'localhost:6382');


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
		$n = 10;
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
		$this->ra = new RedisArray($newRing, NULL, $oldRing, $this->useIndex);
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

		global $newRing, $oldRing;
		$oldRing = $newRing; // back up the original.
		$newRing []= 'localhost:6382'; // add a new node to the main ring.
	}

	public function testReadThrough() {
		$this->readAllvalues();
	}
}

/*
function report_info($ra) {
	$infos = $ra->info();
	foreach($infos as $host => $info) {
		echo "Host $host: ".(isset($info['db0'])?$info['db0']:0)."\n";
	}
}


echo "Distributing $n keys around the ring.\n";

report_info($ra);


echo "Creating a new, larger ring and reading back all the values.\n";
$ra = new RedisArray($oldRing, NULL, $newRing, TRUE);
foreach($data as $k => $v) {
	$ret = $ra->get($k);
	if($ret !== $v) {
		echo "Could not match expected value $v for key $k, instead got:"; var_dump($ret);
		die;
	}
}
echo "All key/value pairs read successfuly using the new setup with a fallback\n";
report_info($ra);

echo "Rehash array.\n";

function rehash_callback($host, $count) {
	echo "Now rehashing $count keys on node $host\n";
}
$ra->_rehash('rehash_callback');

echo "Creating a new ring without fallback and reading back all the values.\n";
$ra = new RedisArray($oldRing);
foreach($data as $k => $v) {
	$ret = $ra->get($k);
	if($ret !== $v) {
		echo "Could not match expected value $v for key $k, instead got:"; var_dump($ret);
		die;
	}
}
echo "All key/value pairs read successfuly using the new setup without a fallback\n";
report_info($ra);
 */

$suite  = new PHPUnit_TestSuite("Redis_Rehashing_Test");
$result = PHPUnit::run($suite);

echo $result->toString();

?>
