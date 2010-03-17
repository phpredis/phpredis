<?php
require_once 'PHPUnit/Framework/TestCase.php';

echo "Note: these tests might take up to a minute. Don't worry :-)\n";

class Redis_Test extends PHPUnit_Framework_TestCase
{
    /**
     * @var Redis
     */
    public $redis;

    public function setUp()
    {
	$this->redis = new Redis();
	$this->redis->connect('127.0.0.1', 6379);

	// uncomment the following if you want to use authentication
	// $this->assertTrue($this->redis->auth('foobared'));
    }

    public function tearDown()
    {
        $this->redis->close();
        unset($this->redis);
    }

    public function reset()
    {
        $this->setUp();
        $this->tearDown();
    }

    public function testPing()
    {

    	$this->assertEquals('+PONG', $this->redis->ping());

	$count = 1000;
	while($count --) {
	    	$this->assertEquals('+PONG', $this->redis->ping());
	}

    }

    public function test1000() {

	 $s = str_repeat('A', 1000);
	 $this->redis->set('x', $s);
	 $this->assertEquals($s, $this->redis->get('x'));

	 $s = str_repeat('A', 1000000);
	 $this->redis->set('x', $s);
	 $this->assertEquals($s, $this->redis->get('x'));
    }

    public function testErr() {

	 $this->redis->set('x', '-ERR');
	 $this->assertEquals($this->redis->get('x'), '-ERR');

    }

    public function testSet()
    {
      	$this->assertEquals(TRUE, $this->redis->set('key', 'nil'));
	$this->assertEquals('nil', $this->redis->get('key'));

      	$this->assertEquals(TRUE, $this->redis->set('key', 'val'));

	$this->assertEquals('val', $this->redis->get('key'));
	$this->assertEquals('val', $this->redis->get('key'));
	$this->assertEquals(FALSE, $this->redis->get('keyNotExist'));

	$this->redis->set('key2', 'val');
	$this->assertEquals('val', $this->redis->get('key2'));

     	$value = 'AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA';
	$this->redis->set('key2', $value);
	$this->assertEquals($value, $this->redis->get('key2'));
	$this->assertEquals($value, $this->redis->get('key2'));

	$this->redis->delete('key');
	$this->redis->delete('key2');


	$i = 66000;
	$value2 = 'X';
	while($i--) {
		$value2 .= 'A';
	}
	$value2 .= 'X';

	$this->redis->set('key', $value2);
        $this->assertEquals($value2, $this->redis->get('key'));
	$this->redis->delete('key');
	$this->assertEquals(False, $this->redis->get('key'));

	$data = gzcompress('42');
        $this->assertEquals(True, $this->redis->set('key', $data));
	$this->assertEquals('42', gzuncompress($this->redis->get('key')));

	$this->redis->delete('key');
	$data = gzcompress('value1');
        $this->assertEquals(True, $this->redis->set('key', $data));
	$this->assertEquals('value1', gzuncompress($this->redis->get('key')));

	$this->redis->delete('key');
       	$this->assertEquals(TRUE, $this->redis->set('key', 0));
	$this->assertEquals('0', $this->redis->get('key'));
       	$this->assertEquals(TRUE, $this->redis->set('key', 1));
	$this->assertEquals('1', $this->redis->get('key'));
	$this->assertEquals(TRUE, $this->redis->set('key', 0.1));
	$this->assertEquals('0.1', $this->redis->get('key'));
	$this->assertEquals(TRUE, $this->redis->set('key', '0.1'));
	$this->assertEquals('0.1', $this->redis->get('key'));
       	$this->assertEquals(TRUE, $this->redis->set('key', TRUE));
	$this->assertEquals('1', $this->redis->get('key'));

	$this->assertEquals(True, $this->redis->set('key', ''));
       	$this->assertEquals('', $this->redis->get('key'));
	$this->assertEquals(True, $this->redis->set('key', NULL));
	$this->assertEquals('', $this->redis->get('key'));

        $this->assertEquals(True, $this->redis->set('key', gzcompress('42')));
        $this->assertEquals('42', gzuncompress($this->redis->get('key')));

    }
    public function testGetSet() {

	$this->redis->delete('key');
	$this->assertTrue($this->redis->getSet('key', '42') === FALSE);
	$this->assertTrue($this->redis->getSet('key', '123') === '42');
	$this->assertTrue($this->redis->getSet('key', '123') === '123');
    }

    public function testRandomKey() {

        for($i = 0; $i < 1000; $i++) {
            $k = $this->redis->randomKey();
	    $this->assertTrue($this->redis->exists($k));
	}
    }

    public function testRename() {

	// strings
	$this->redis->delete('key0');
	$this->redis->set('key0', 'val0');
	$this->redis->renameKey('key0', 'key1');
	$this->assertTrue($this->redis->get('key0') === FALSE);
	$this->assertTrue($this->redis->get('key1') === 'val0');


	// lists
	$this->redis->delete('key0');
	$this->redis->lPush('key0', 'val0');
	$this->redis->lPush('key0', 'val1');
	$this->redis->renameKey('key0', 'key1');
	$this->assertTrue($this->redis->lGetRange('key0', 0, -1) === array());
	$this->assertTrue($this->redis->lGetRange('key1', 0, -1) === array('val1', 'val0'));

    }

    public function testRenameNx() {

	// strings
	$this->redis->delete('key0');
	$this->redis->set('key0', 'val0');
	$this->redis->set('key1', 'val1');
	$this->assertTrue($this->redis->renameNx('key0', 'key1') === FALSE);
	$this->assertTrue($this->redis->get('key0') === 'val0');
	$this->assertTrue($this->redis->get('key1') === 'val1');

	// lists
	$this->redis->delete('key0');
	$this->redis->delete('key1');
	$this->redis->lPush('key0', 'val0');
	$this->redis->lPush('key0', 'val1');
	$this->redis->lPush('key1', 'val1-0');
	$this->redis->lPush('key1', 'val1-1');
	$this->redis->renameNx('key0', 'key1');
	$this->assertTrue($this->redis->lGetRange('key0', 0, -1) === array('val1', 'val0'));
	$this->assertTrue($this->redis->lGetRange('key1', 0, -1) === array('val1-1', 'val1-0'));

	$this->redis->delete('key2');
	$this->redis->renameNx('key0', 'key2');
	$this->assertTrue($this->redis->lGetRange('key0', 0, -1) === array());
	$this->assertTrue($this->redis->lGetRange('key2', 0, -1) === array('val1', 'val0'));

    }

    public function testMultiple() {

    	$this->redis->delete('k1');
      	$this->redis->delete('k2');
      	$this->redis->delete('k3');

	$this->redis->set('k1', 'v1');
	$this->redis->set('k2', 'v2');
	$this->redis->set('k3', 'v3');

	$this->assertEquals(array('v1'), $this->redis->getMultiple(array('k1')));
	$this->assertEquals(array('v1', 'v3', false), $this->redis->getMultiple(array('k1', 'k3', 'NoKey')));
	$this->assertEquals(array('v1', 'v2', 'v3'), $this->redis->getMultiple(array('k1', 'k2', 'k3')));
	$this->assertEquals(array('v1', 'v2', 'v3'), $this->redis->getMultiple(array('k1', 'k2', 'k3')));

	$this->redis->set('k5', '$1111111111');
	$this->assertEquals(array(0 => '$1111111111'), $this->redis->getMultiple(array('k5')));
    }

    public function testMultipleBin() {

   	$this->redis->delete('k1');
      	$this->redis->delete('k2');
      	$this->redis->delete('k3');

	$this->redis->set('k1', gzcompress('v1'));
	$this->redis->set('k2', gzcompress('v2'));
	$this->redis->set('k3', gzcompress('v3'));

	$this->assertEquals(array(gzcompress('v1'), gzcompress('v2'), gzcompress('v3')), $this->redis->getMultiple(array('k1', 'k2', 'k3')));
	$this->assertEquals(array(gzcompress('v1'), gzcompress('v2'), gzcompress('v3')), $this->redis->getMultiple(array('k1', 'k2', 'k3')));

    }

    public function testSetTimeout() {

	$this->redis->delete('key');
        $this->redis->set('key', 'value');
	$this->assertEquals('value', $this->redis->get('key'));
	$this->redis->setTimeout('key', 1);
	$this->assertEquals('value', $this->redis->get('key'));
	sleep(2);
	$this->assertEquals(False, $this->redis->get('key'));
    }

    public function testExpireAt() {

	$this->redis->delete('key');
        $this->redis->set('key', 'value');
	$now = time(NULL);
	$this->redis->expireAt('key', $now + 1);
	$this->assertEquals('value', $this->redis->get('key'));
	sleep(2);
	$this->assertEquals(FALSE, $this->redis->get('key'));
    }

    public function testSetNX() {

	    $this->redis->set('key', 42);
	    $this->assertTrue($this->redis->setnx('key', 'err') === FALSE);
	    $this->assertTrue($this->redis->get('key') === '42');

	    $this->redis->delete('key');
	    $this->assertTrue($this->redis->setnx('key', '42') === TRUE);
	    $this->assertTrue($this->redis->get('key') === '42');
    }

    public function testAdd()
    {
        $key = 'key' . rand();

        $this->assertTrue($this->redis->add($key, 'val'));
        $this->assertFalse($this->redis->add($key, 'val'));
    }

    public function testIncr()
    {
        $this->redis->set('key', 0);

        $this->redis->incr('key');

        $this->assertEquals(1, $this->redis->get('key'));

        $this->redis->incr('key');

        $this->assertEquals(2, $this->redis->get('key'));

        $this->redis->incr('key', 3);

        $this->assertEquals(5, $this->redis->get('key'));

	$this->redis->delete('key');

	$this->redis->set('key', 'abc');

	$this->redis->incr('key');

	$this->assertTrue("1" === $this->redis->get('key'));

	$this->redis->incr('key');

	$this->assertTrue("2" === $this->redis->get('key'));

    }

    public function testDecr()
    {
        $this->redis->set('key', 5);

        $this->redis->decr('key');

        $this->assertEquals(4, $this->redis->get('key'));

        $this->redis->decr('key');

        $this->assertEquals(3, $this->redis->get('key'));

        $this->redis->decr('key', 2);

        $this->assertEquals(1, $this->redis->get('key'));
    }

    public function testExists()
    {

    	$this->redis->delete('key');
        $this->assertFalse($this->redis->exists('key'));
        $this->redis->set('key', 'val');
        $this->assertEquals(True, $this->redis->exists('key'));
    }

    public function testGetKeys()
    {

        $pattern = 'getKeys-test-';
	for($i = 1; $i < 10; $i++) {
	    $this->redis->set($pattern.$i, $i);
        }
        $this->redis->delete($pattern.'3');
        $keys = $this->redis->getKeys($pattern.'*');

        $this->redis->add($pattern.'3', 'something');

        $keys2 = $this->redis->getKeys($pattern.'*');

        $this->assertEquals((count($keys) + 1), count($keys2));

	// empty array when no key matches
        $this->assertEquals(array(), $this->redis->getKeys(rand().rand().rand().'*'));
    }

    public function testDelete()
    {
      	$key = 'key' . rand();
        $this->redis->set($key, 'val');
        $this->assertEquals('val', $this->redis->get($key));
	$this->assertEquals(1, $this->redis->delete($key));
        $this->assertEquals(null, $this->redis->get($key));

	// multiple, all existing
	$this->redis->set('x', 0);
	$this->redis->set('y', 1);
	$this->redis->set('z', 2);
	$this->assertEquals(3, $this->redis->delete('x', 'y', 'z'));
	$this->assertEquals(NULL, $this->redis->get('x'));
	$this->assertEquals(NULL, $this->redis->get('y'));
	$this->assertEquals(NULL, $this->redis->get('z'));

	// multiple, none existing
	$this->assertEquals(0, $this->redis->delete('x', 'y', 'z'));
	$this->assertEquals(NULL, $this->redis->get('x'));
	$this->assertEquals(NULL, $this->redis->get('y'));
	$this->assertEquals(NULL, $this->redis->get('z'));

	// multiple, some existing
	$this->redis->set('y', 1);
	$this->assertEquals(1, $this->redis->delete('x', 'y', 'z'));
	$this->assertEquals(NULL, $this->redis->get('y'));

	$this->redis->set('x', 0);
	$this->redis->set('y', 1);
	$this->assertEquals(2, $this->redis->delete(array('x', 'y')));

    }

    public function testType()
    {
	// 0 => none, (key didn't exist)
	// 1=> string,
	// 2 => set,
	// 3 => list

	// string
	$this->redis->set('key', 'val');
	$this->assertEquals(Redis::REDIS_STRING, $this->redis->type('key'));

	// list
	$this->redis->lPush('keyList', "val0");
	$this->redis->lPush('keyList', "val1");
	$this->assertEquals(Redis::REDIS_LIST, $this->redis->type('keyList'));

	// set
	$this->redis->delete('keySet');
	$this->redis->sAdd('keySet', "val0");
	$this->redis->sAdd('keySet', "val1");
	$this->assertEquals(Redis::REDIS_SET, $this->redis->type('keySet'));

	//None
	$this->assertEquals(Redis::REDIS_NOT_FOUND, $this->redis->type('keyNotExists'));
    }

    // PUSH, POP : LPUSH, LPOP
    public function testlPop()
    {

	//	rpush  => tail
	//	lpush => head


        $this->redis->delete('list');

        $this->redis->lPush('list', 'val');
        $this->redis->lPush('list', 'val2');
	$this->redis->rPush('list', 'val3');

	// 'list' = [ 'val2', 'val', 'val3']

	$this->assertEquals('val2', $this->redis->lPop('list'));
        $this->assertEquals('val', $this->redis->lPop('list'));
        $this->assertEquals('val3', $this->redis->lPop('list'));
        $this->assertEquals(FALSE, $this->redis->lPop('list'));

	// testing binary data

	$this->redis->delete('list');
	$this->assertEquals(1, $this->redis->lPush('list', gzcompress('val1')));
	$this->assertEquals(2, $this->redis->lPush('list', gzcompress('val2')));
	$this->assertEquals(3, $this->redis->lPush('list', gzcompress('val3')));

	$this->assertEquals('val3', gzuncompress($this->redis->lPop('list')));
	$this->assertEquals('val2', gzuncompress($this->redis->lPop('list')));
	$this->assertEquals('val1', gzuncompress($this->redis->lPop('list')));

    }

    // PUSH, POP : RPUSH, RPOP
    public function testrPop()
    {
	//	rpush  => tail
	//	lpush => head

        $this->redis->delete('list');

        $this->redis->rPush('list', 'val');
        $this->redis->rPush('list', 'val2');
	$this->redis->lPush('list', 'val3');

	// 'list' = [ 'val3', 'val', 'val2']

	$this->assertEquals('val2', $this->redis->rPop('list'));
        $this->assertEquals('val', $this->redis->rPop('list'));
        $this->assertEquals('val3', $this->redis->rPop('list'));
        $this->assertEquals(FALSE, $this->redis->rPop('list'));

	// testing binary data

	$this->redis->delete('list');
	$this->assertEquals(1, $this->redis->rPush('list', gzcompress('val1')));
	$this->assertEquals(2, $this->redis->rPush('list', gzcompress('val2')));
	$this->assertEquals(3, $this->redis->rPush('list', gzcompress('val3')));

	$this->assertEquals('val3', gzuncompress($this->redis->rPop('list')));
	$this->assertEquals('val2', gzuncompress($this->redis->rPop('list')));
	$this->assertEquals('val1', gzuncompress($this->redis->rPop('list')));

    }

    public function testlSize()
    {

        $this->redis->delete('list');

        $this->redis->lPush('list', 'val');
        $this->assertEquals(1, $this->redis->lSize('list'));

        $this->redis->lPush('list', 'val2');
        $this->assertEquals(2, $this->redis->lSize('list'));

	$this->assertEquals('val2', $this->redis->lPop('list'));
        $this->assertEquals(1, $this->redis->lSize('list'));

	$this->assertEquals('val', $this->redis->lPop('list'));
        $this->assertEquals(0, $this->redis->lSize('list'));

        $this->assertEquals(FALSE, $this->redis->lPop('list'));
        $this->assertEquals(0, $this->redis->lSize('list'));	// empty returns 0

        $this->redis->delete('list');
        $this->assertEquals(0, $this->redis->lSize('list'));	// non-existent returns 0

        $this->redis->set('list', 'actually not a list');
        $this->assertEquals(FALSE, $this->redis->lSize('list'));// not a list returns FALSE
    }


    // ltrim, lsize, lpop
    public function testlistTrim()
    {

    	$this->redis->delete('list');

        $this->redis->lPush('list', 'val');
        $this->redis->lPush('list', 'val2');
        $this->redis->lPush('list', 'val3');
        $this->redis->lPush('list', 'val4');

	$this->assertEquals(TRUE, $this->redis->listTrim('list', 0, 2));
	$this->assertEquals(3, $this->redis->lSize('list'));

        $this->redis->listTrim('list', 0, 0);
        $this->assertEquals(1, $this->redis->lSize('list'));
	$this->assertEquals('val4', $this->redis->lPop('list'));

	$this->assertEquals(TRUE, $this->redis->listTrim('list', 10, 10000));
	$this->assertEquals(TRUE, $this->redis->listTrim('list', 10000, 10));

	// test invalid type
	$this->redis->set('list', 'not a list...');
	$this->assertEquals(FALSE, $this->redis->listTrim('list', 0, 2));

    }

    public function setupSort() {
	// people with name, age, salary
	$this->redis->set('person:name_1', 'Alice');
	$this->redis->set('person:age_1', 27);
	$this->redis->set('person:salary_1', 2500);

	$this->redis->set('person:name_2', 'Bob');
	$this->redis->set('person:age_2', 34);
	$this->redis->set('person:salary_2', 2000);

	$this->redis->set('person:name_3', 'Carol');
	$this->redis->set('person:age_3', 25);
	$this->redis->set('person:salary_3', 2800);

	$this->redis->set('person:name_4', 'Dave');
	$this->redis->set('person:age_4', 41);
	$this->redis->set('person:salary_4', 3100);

	// set-up
	$this->redis->delete('person:id');
	foreach(array(1,2,3,4) as $id) {
	    $this->redis->lPush('person:id', $id);
	}

    }

    public function testSortAsc() {

	$this->setupSort();

	$this->assertTrue(FALSE === $this->redis->sortAsc(NULL));

	// sort by age and get IDs
	$byAgeAsc = array('3','1','2','4');
	$this->assertEquals($byAgeAsc, $this->redis->sortAsc('person:id', 'person:age_*'));
	$this->assertEquals(array('1', '2', '3', '4'), $this->redis->sortAsc('person:id', NULL));	// check that NULL works.
	$this->assertEquals(array('1', '2', '3', '4'), $this->redis->sortAsc('person:id', NULL, NULL));	// for all fields.

	// sort by age and get names
	$byAgeAsc = array('Carol','Alice','Bob','Dave');
	$this->assertEquals($byAgeAsc, $this->redis->sortAsc('person:id', 'person:age_*', 'person:name_*'));

	$this->assertEquals(array_slice($byAgeAsc, 0, 2), $this->redis->sortAsc('person:id', 'person:age_*', 'person:name_*', 0, 2));
	$this->assertEquals(array_slice($byAgeAsc, 1, 2), $this->redis->sortAsc('person:id', 'person:age_*', 'person:name_*', 1, 2));
	$this->assertEquals(array_slice($byAgeAsc, 0, 3), $this->redis->sortAsc('person:id', 'person:age_*', 'person:name_*', NULL, 3)); // NULL is transformed to 0 if there is something after it.
	$this->assertEquals($byAgeAsc, $this->redis->sortAsc('person:id', 'person:age_*', 'person:name_*', 1, NULL));
	$this->assertEquals(array(), $this->redis->sortAsc('person:id', 'person:age_*', 'person:name_*', NULL, NULL)); // NULL, NULL is the same as (0,0). That returns no element.

	// sort by salary and get ages
	$agesBySalaryAsc = array('34', '27', '25', '41');
	$this->assertEquals($agesBySalaryAsc, $this->redis->sortAsc('person:id', 'person:salary_*', 'person:age_*'));


	// sort non-alpha doesn't change all-string lists
	// list → [ghi, def, abc]
	$list = array('abc', 'def', 'ghi');
	$this->redis->delete('list');
	foreach($list as $i) {
	    $this->redis->lPush('list', $i);
	}

	// SORT list → [ghi, def, abc]
	$this->assertEquals(array_reverse($list), $this->redis->sortAsc('list'));

	// SORT list ALPHA → [abc, def, ghi]
	$this->assertEquals($list, $this->redis->sortAscAlpha('list'));
    }

    public function testSortDesc() {

	$this->setupSort();

	// sort by age and get IDs
	$byAgeDesc = array('4','2','1','3');
	$this->assertEquals($byAgeDesc, $this->redis->sortDesc('person:id', 'person:age_*'));

	// sort by age and get names
	$byAgeDesc = array('Dave', 'Bob', 'Alice', 'Carol');
	$this->assertEquals($byAgeDesc, $this->redis->sortDesc('person:id', 'person:age_*', 'person:name_*'));

	$this->assertEquals(array_slice($byAgeDesc, 0, 2), $this->redis->sortDesc('person:id', 'person:age_*', 'person:name_*', 0, 2));
	$this->assertEquals(array_slice($byAgeDesc, 1, 2), $this->redis->sortDesc('person:id', 'person:age_*', 'person:name_*', 1, 2));

	// sort by salary and get ages
	$agesBySalaryDesc = array('41', '25', '27', '34');
	$this->assertEquals($agesBySalaryDesc, $this->redis->sortDesc('person:id', 'person:salary_*', 'person:age_*'));

	// sort non-alpha doesn't change all-string lists
	$list = array('def', 'abc', 'ghi');
	$this->redis->delete('list');
	foreach($list as $i) {
	    $this->redis->lPush('list', $i);
	}

	// SORT list → [ghi, abc, def]
	$this->assertEquals(array_reverse($list), $this->redis->sortDesc('list'));

	// SORT list ALPHA → [abc, def, ghi]
	$this->assertEquals(array('ghi', 'def', 'abc'), $this->redis->sortDescAlpha('list'));
    }

    // LINDEX
    public function testlGet() {

        $this->redis->delete('list');

        $this->redis->lPush('list', 'val');
        $this->redis->lPush('list', 'val2');
        $this->redis->lPush('list', 'val3');

	$this->assertEquals('val3', $this->redis->lGet('list', 0));
        $this->assertEquals('val2', $this->redis->lGet('list', 1));
	$this->assertEquals('val', $this->redis->lGet('list', 2));
	$this->assertEquals('val', $this->redis->lGet('list', -1));
	$this->assertEquals('val2', $this->redis->lGet('list', -2));
	$this->assertEquals('val3', $this->redis->lGet('list', -3));
	$this->assertEquals(FALSE, $this->redis->lGet('list', -4));

        $this->redis->rPush('list', 'val4');
	$this->assertEquals('val4', $this->redis->lGet('list', 3));
	$this->assertEquals('val4', $this->redis->lGet('list', -1));
    }

    // lRem testing
    public function testlRemove() {
    	$this->redis->delete('list');
        $this->redis->lPush('list', 'a');
        $this->redis->lPush('list', 'b');
        $this->redis->lPush('list', 'c');
        $this->redis->lPush('list', 'c');
        $this->redis->lPush('list', 'b');
        $this->redis->lPush('list', 'c');
	// ['c', 'b', 'c', 'c', 'b', 'a']
	$return = $this->redis->lRemove('list', 'b', 2);
	// ['c', 'c', 'c', 'a']
	$this->assertEquals(2, $return);
	$this->assertEquals('c', $this->redis->lGET('list', 0));
	$this->assertEquals('c', $this->redis->lGET('list', 1));
	$this->assertEquals('c', $this->redis->lGET('list', 2));
	$this->assertEquals('a', $this->redis->lGET('list', 3));

    	$this->redis->delete('list');
        $this->redis->lPush('list', 'a');
        $this->redis->lPush('list', 'b');
        $this->redis->lPush('list', 'c');
        $this->redis->lPush('list', 'c');
        $this->redis->lPush('list', 'b');
        $this->redis->lPush('list', 'c');
	// ['c', 'b', 'c', 'c', 'b', 'a']
	$this->redis->lRemove('list', 'c', -2);
	// ['c', 'b', 'b', 'a']
	$this->assertEquals(2, $return);
	$this->assertEquals('c', $this->redis->lGET('list', 0));
	$this->assertEquals('b', $this->redis->lGET('list', 1));
	$this->assertEquals('b', $this->redis->lGET('list', 2));
	$this->assertEquals('a', $this->redis->lGET('list', 3));

	// remove each element
	$this->assertEquals(1, $this->redis->lRemove('list', 'a', 0));
	$this->assertEquals(0, $this->redis->lRemove('list', 'x', 0));
	$this->assertEquals(2, $this->redis->lRemove('list', 'b', 0));
	$this->assertEquals(1, $this->redis->lRemove('list', 'c', 0));
	$this->assertEquals(FALSE, $this->redis->get('list'));

	$this->redis->set('list', 'actually not a list');
	$this->assertEquals(FALSE, $this->redis->lRemove('list', 'x'));

    }

    public function testsAdd()
    {
        $this->redis->delete('set');

	$this->assertEquals(TRUE, $this->redis->sAdd('set', 'val'));
	$this->assertEquals(FALSE, $this->redis->sAdd('set', 'val'));

        $this->assertTrue($this->redis->sContains('set', 'val'));
        $this->assertFalse($this->redis->sContains('set', 'val2'));

	$this->assertEquals(TRUE, $this->redis->sAdd('set', 'val2'));

        $this->assertTrue($this->redis->sContains('set', 'val2'));
    }
    public function testsSize()
    {
        $this->redis->delete('set');

	$this->assertEquals(TRUE, $this->redis->sAdd('set', 'val'));

        $this->assertEquals(1, $this->redis->sSize('set'));

	$this->assertEquals(TRUE, $this->redis->sAdd('set', 'val2'));

        $this->assertEquals(2, $this->redis->sSize('set'));
    }

    public function testsRemove()
    {
        $this->redis->delete('set');

        $this->redis->sAdd('set', 'val');
        $this->redis->sAdd('set', 'val2');

        $this->redis->sRemove('set', 'val');

        $this->assertEquals(1, $this->redis->sSize('set'));

        $this->redis->sRemove('set', 'val2');

        $this->assertEquals(0, $this->redis->sSize('set'));
    }

    public function testsMove()
    {
        $this->redis->delete('set0');
        $this->redis->delete('set1');

        $this->redis->sAdd('set0', 'val');
        $this->redis->sAdd('set0', 'val2');

        $this->assertTrue($this->redis->sMove('set0', 'set1', 'val'));
        $this->assertFalse($this->redis->sMove('set0', 'set1', 'val'));
        $this->assertFalse($this->redis->sMove('set0', 'set1', 'val-what'));

        $this->assertEquals(1, $this->redis->sSize('set0'));
        $this->assertEquals(1, $this->redis->sSize('set1'));

	$this->assertEquals(array('val2'), $this->redis->sGetMembers('set0'));
	$this->assertEquals(array('val'), $this->redis->sGetMembers('set1'));
    }

    public function testsPop()
    {
        $this->redis->delete('set0');
	$this->assertTrue($this->redis->sPop('set0') === FALSE);

        $this->redis->sAdd('set0', 'val');
        $this->redis->sAdd('set0', 'val2');

	$v0 = $this->redis->sPop('set0');
	$this->assertTrue($v0 === 'val' || $v0 === 'val2');
	$v1 = $this->redis->sPop('set0');
	$this->assertTrue(($v0 === 'val' && $v1 === 'val2') || ($v1 === 'val' && $v0 === 'val2'));

	$this->assertTrue($this->redis->sPop('set0') === FALSE);
    }

    public function testsContains()
    {
        $this->redis->delete('set');

        $this->redis->sAdd('set', 'val');

        $this->assertTrue($this->redis->sContains('set', 'val'));
        $this->assertFalse($this->redis->sContains('set', 'val2'));
    }

    public function testsGetMembers()
    {
        $this->redis->delete('set');

        $this->redis->sAdd('set', 'val');
        $this->redis->sAdd('set', 'val2');
        $this->redis->sAdd('set', 'val3');

        $array = array('val', 'val2', 'val3');

        $this->assertEquals($array, $this->redis->sGetMembers('set'));
	$this->assertEquals($array, $this->redis->sMembers('set'));	// test alias
    }

    public function testlSet() {

      	$this->redis->delete('list');
        $this->redis->lPush('list', 'val');
        $this->redis->lPush('list', 'val2');
	$this->redis->lPush('list', 'val3');

	$this->assertEquals($this->redis->lGet('list', 0), 'val3');
	$this->assertEquals($this->redis->lGet('list', 1), 'val2');
	$this->assertEquals($this->redis->lGet('list', 2), 'val');

	$this->assertEquals(TRUE, $this->redis->lSet('list', 1, 'valx'));

	$this->assertEquals($this->redis->lGet('list', 0), 'val3');
	$this->assertEquals($this->redis->lGet('list', 1), 'valx');
	$this->assertEquals($this->redis->lGet('list', 2), 'val');

    }

    public function testsInter() {
        $this->redis->delete('x');	// set of odd numbers
        $this->redis->delete('y');	// set of prime numbers
        $this->redis->delete('z');	// set of squares
        $this->redis->delete('t');	// set of numbers of the form n^2 - 1

        $x = array(1,3,5,7,9,11,13,15,17,19,21,23,25);
        foreach($x as $i) {
            $this->redis->sAdd('x', $i);
        }

        $y = array(1,2,3,5,7,11,13,17,19,23);
        foreach($y as $i) {
            $this->redis->sAdd('y', $i);
        }

        $z = array(1,4,9,16,25);
        foreach($z as $i) {
            $this->redis->sAdd('z', $i);
        }

        $t = array(2,5,10,17,26);
        foreach($t as $i) {
            $this->redis->sAdd('t', $i);
        }

        $xy = $this->redis->sInter('x', 'y');	// odd prime numbers
        foreach($xy as $i) {
	    $i = (int)$i;
            $this->assertTrue(in_array($i, array_intersect($x, $y)));
        }

        $yz = $this->redis->sInter('y', 'z');	// set of odd squares
        foreach($yz as $i) {
	    $i = (int)$i;
            $this->assertTrue(in_array($i, array_intersect($y, $z)));
        }

        $zt = $this->redis->sInter('z', 't');	// prime squares
        $this->assertTrue($zt === array());

        $xyz = $this->redis->sInter('x', 'y', 'z');// odd prime squares
        $this->assertTrue($xyz === array('1'));

        $nil = $this->redis->sInter();
        $this->assertTrue($nil === FALSE);
    }

    public function testsInterStore() {
        $this->redis->delete('x');	// set of odd numbers
        $this->redis->delete('y');	// set of prime numbers
        $this->redis->delete('z');	// set of squares
        $this->redis->delete('t');	// set of numbers of the form n^2 - 1

        $x = array(1,3,5,7,9,11,13,15,17,19,21,23,25);
        foreach($x as $i) {
            $this->redis->sAdd('x', $i);
        }

        $y = array(1,2,3,5,7,11,13,17,19,23);
        foreach($y as $i) {
            $this->redis->sAdd('y', $i);
        }

        $z = array(1,4,9,16,25);
        foreach($z as $i) {
            $this->redis->sAdd('z', $i);
        }

        $t = array(2,5,10,17,26);
        foreach($t as $i) {
            $this->redis->sAdd('t', $i);
        }

        $count = $this->redis->sInterStore('k', 'x', 'y');	// odd prime numbers
	$this->assertEquals($count, $this->redis->sSize('k'));
        foreach(array_intersect($x, $y) as $i) {
            $this->assertTrue($this->redis->sContains('k', $i));
        }

        $count = $this->redis->sInterStore('k', 'y', 'z');	// set of odd squares
	$this->assertEquals($count, $this->redis->sSize('k'));
        foreach(array_intersect($y, $z) as $i) {
            $this->assertTrue($this->redis->sContains('k', $i));
        }

        $count = $this->redis->sInterStore('k', 'z', 't');	// squares of the form n^2 + 1
	$this->assertEquals($count, 0);
	$this->assertEquals($count, $this->redis->sSize('k'));

	$this->redis->delete('z');
	$xyz = $this->redis->sInterStore('k', 'x', 'y', 'z'); // only z missing, expect 0.
	$this->assertTrue($xyz === 0);

	$this->redis->delete('y');
	$xyz = $this->redis->sInterStore('k', 'x', 'y', 'z'); // y and z missing, expect 0.
	$this->assertTrue($xyz === 0);

	$this->redis->delete('x');
	$xyz = $this->redis->sInterStore('k', 'x', 'y', 'z'); // x y and z ALL missing, expect 0.
	$this->assertTrue($xyz === 0);

        $o = $this->redis->sInterStore('k');
	$this->assertTrue($o === FALSE);	// error, wrong parameter count
    }

    public function testsUnion() {
        $this->redis->delete('x');	// set of odd numbers
        $this->redis->delete('y');	// set of prime numbers
        $this->redis->delete('z');	// set of squares
        $this->redis->delete('t');	// set of numbers of the form n^2 - 1

        $x = array(1,3,5,7,9,11,13,15,17,19,21,23,25);
        foreach($x as $i) {
            $this->redis->sAdd('x', $i);
        }

        $y = array(1,2,3,5,7,11,13,17,19,23);
        foreach($y as $i) {
            $this->redis->sAdd('y', $i);
        }

        $z = array(1,4,9,16,25);
        foreach($z as $i) {
            $this->redis->sAdd('z', $i);
        }

        $t = array(2,5,10,17,26);
        foreach($t as $i) {
            $this->redis->sAdd('t', $i);
        }

        $xy = $this->redis->sUnion('x', 'y');	// x U y
        foreach($xy as $i) {
	    $i = (int)$i;
            $this->assertTrue(in_array($i, array_merge($x, $y)));
        }

        $yz = $this->redis->sUnion('y', 'z');	// y U Z
        foreach($yz as $i) {
	    $i = (int)$i;
            $this->assertTrue(in_array($i, array_merge($y, $z)));
        }

        $zt = $this->redis->sUnion('z', 't');	// z U t
        foreach($zt as $i) {
	    $i = (int)$i;
            $this->assertTrue(in_array($i, array_merge($z, $t)));
        }

        $xyz = $this->redis->sUnion('x', 'y', 'z'); // x U y U z
        foreach($xyz as $i) {
	    $i = (int)$i;
            $this->assertTrue(in_array($i, array_merge($x, $y, $z)));
        }

        $nil = $this->redis->sUnion();
        $this->assertTrue($nil === FALSE);
    }

    public function testsUnionStore() {
        $this->redis->delete('x');	// set of odd numbers
        $this->redis->delete('y');	// set of prime numbers
        $this->redis->delete('z');	// set of squares
        $this->redis->delete('t');	// set of numbers of the form n^2 - 1

        $x = array(1,3,5,7,9,11,13,15,17,19,21,23,25);
        foreach($x as $i) {
            $this->redis->sAdd('x', $i);
        }

        $y = array(1,2,3,5,7,11,13,17,19,23);
        foreach($y as $i) {
            $this->redis->sAdd('y', $i);
        }

        $z = array(1,4,9,16,25);
        foreach($z as $i) {
            $this->redis->sAdd('z', $i);
        }

        $t = array(2,5,10,17,26);
        foreach($t as $i) {
            $this->redis->sAdd('t', $i);
        }

        $count = $this->redis->sUnionStore('k', 'x', 'y');	// x U y
	$xy = array_unique(array_merge($x, $y));
	$this->assertEquals($count, count($xy));
        foreach($xy as $i) {
	    $i = (int)$i;
            $this->assertTrue($this->redis->sContains('k', $i));
        }

        $count = $this->redis->sUnionStore('k', 'y', 'z');	// y U z
	$yz = array_unique(array_merge($y, $z));
	$this->assertEquals($count, count($yz));
        foreach($yz as $i) {
	    $i = (int)$i;
            $this->assertTrue($this->redis->sContains('k', $i));
        }

        $count = $this->redis->sUnionStore('k', 'z', 't');	// z U t
	$zt = array_unique(array_merge($z, $t));
	$this->assertEquals($count, count($zt));
        foreach($zt as $i) {
	    $i = (int)$i;
            $this->assertTrue($this->redis->sContains('k', $i));
        }

        $count = $this->redis->sUnionStore('k', 'x', 'y', 'z');	// x U y U z
	$xyz = array_unique(array_merge($x, $y, $z));
	$this->assertEquals($count, count($xyz));
        foreach($xyz as $i) {
	    $i = (int)$i;
            $this->assertTrue($this->redis->sContains('k', $i));
        }

	$this->redis->delete('x');	// x missing now
        $count = $this->redis->sUnionStore('k', 'x', 'y', 'z');	// x U y U z
	$this->assertTrue($count === count(array_unique(array_merge($y, $z))));

	$this->redis->delete('y');	// x and y missing
        $count = $this->redis->sUnionStore('k', 'x', 'y', 'z');	// x U y U z
	$this->assertTrue($count === count(array_unique($z)));

	$this->redis->delete('z');	// x, y, and z ALL missing
        $count = $this->redis->sUnionStore('k', 'x', 'y', 'z');	// x U y U z
	$this->assertTrue($count === 0);

        $count = $this->redis->sUnionStore('k');	// Union on nothing...
	$this->assertTrue($count === FALSE);
    }

    public function testsDiff() {
        $this->redis->delete('x');	// set of odd numbers
        $this->redis->delete('y');	// set of prime numbers
        $this->redis->delete('z');	// set of squares
        $this->redis->delete('t');	// set of numbers of the form n^2 - 1

        $x = array(1,3,5,7,9,11,13,15,17,19,21,23,25);
        foreach($x as $i) {
            $this->redis->sAdd('x', $i);
        }

        $y = array(1,2,3,5,7,11,13,17,19,23);
        foreach($y as $i) {
            $this->redis->sAdd('y', $i);
        }

        $z = array(1,4,9,16,25);
        foreach($z as $i) {
            $this->redis->sAdd('z', $i);
        }

        $t = array(2,5,10,17,26);
        foreach($t as $i) {
            $this->redis->sAdd('t', $i);
        }

        $xy = $this->redis->sDiff('x', 'y');	// x U y
        foreach($xy as $i) {
	    $i = (int)$i;
            $this->assertTrue(in_array($i, array_diff($x, $y)));
        }

        $yz = $this->redis->sDiff('y', 'z');	// y U Z
        foreach($yz as $i) {
	    $i = (int)$i;
            $this->assertTrue(in_array($i, array_diff($y, $z)));
        }

        $zt = $this->redis->sDiff('z', 't');	// z U t
        foreach($zt as $i) {
	    $i = (int)$i;
            $this->assertTrue(in_array($i, array_diff($z, $t)));
        }

        $xyz = $this->redis->sDiff('x', 'y', 'z'); // x U y U z
        foreach($xyz as $i) {
	    $i = (int)$i;
            $this->assertTrue(in_array($i, array_diff($x, $y, $z)));
        }

        $nil = $this->redis->sDiff();
        $this->assertTrue($nil === FALSE);
    }

    public function testsDiffStore() {
        $this->redis->delete('x');	// set of odd numbers
        $this->redis->delete('y');	// set of prime numbers
        $this->redis->delete('z');	// set of squares
        $this->redis->delete('t');	// set of numbers of the form n^2 - 1

        $x = array(1,3,5,7,9,11,13,15,17,19,21,23,25);
        foreach($x as $i) {
            $this->redis->sAdd('x', $i);
        }

        $y = array(1,2,3,5,7,11,13,17,19,23);
        foreach($y as $i) {
            $this->redis->sAdd('y', $i);
        }

        $z = array(1,4,9,16,25);
        foreach($z as $i) {
            $this->redis->sAdd('z', $i);
        }

        $t = array(2,5,10,17,26);
        foreach($t as $i) {
            $this->redis->sAdd('t', $i);
        }

        $count = $this->redis->sDiffStore('k', 'x', 'y');	// x - y
	$xy = array_unique(array_diff($x, $y));
	$this->assertEquals($count, count($xy));
        foreach($xy as $i) {
	    $i = (int)$i;
            $this->assertTrue($this->redis->sContains('k', $i));
        }

        $count = $this->redis->sDiffStore('k', 'y', 'z');	// y - z
	$yz = array_unique(array_diff($y, $z));
	$this->assertEquals($count, count($yz));
        foreach($yz as $i) {
	    $i = (int)$i;
            $this->assertTrue($this->redis->sContains('k', $i));
        }

        $count = $this->redis->sDiffStore('k', 'z', 't');	// z - t
	$zt = array_unique(array_diff($z, $t));
	$this->assertEquals($count, count($zt));
        foreach($zt as $i) {
	    $i = (int)$i;
            $this->assertTrue($this->redis->sContains('k', $i));
        }

        $count = $this->redis->sDiffStore('k', 'x', 'y', 'z');	// x - y - z
	$xyz = array_unique(array_diff($x, $y, $z));
	$this->assertEquals($count, count($xyz));
        foreach($xyz as $i) {
	    $i = (int)$i;
            $this->assertTrue($this->redis->sContains('k', $i));
        }

	$this->redis->delete('x');	// x missing now
        $count = $this->redis->sDiffStore('k', 'x', 'y', 'z');	// x - y - z
	$this->assertTrue($count === 0);

	$this->redis->delete('y');	// x and y missing
        $count = $this->redis->sDiffStore('k', 'x', 'y', 'z');	// x - y - z
	$this->assertTrue($count === 0);

	$this->redis->delete('z');	// x, y, and z ALL missing
        $count = $this->redis->sDiffStore('k', 'x', 'y', 'z');	// x - y - z
	$this->assertTrue($count === 0);

        $count = $this->redis->sDiffStore('k');	// diff on nothing...
	$this->assertTrue($count === FALSE);
    }

    public function testlGetRange() {

      	$this->redis->delete('list');
        $this->redis->lPush('list', 'val');
        $this->redis->lPush('list', 'val2');
	$this->redis->lPush('list', 'val3');

	// pos :   0     1     2
	// pos :  -3    -2    -1
	// list: [val3, val2, val]

	$this->assertEquals($this->redis->lGetRange('list', 0, 0), array('val3'));
	$this->assertEquals($this->redis->lGetRange('list', 0, 1), array('val3', 'val2'));
	$this->assertEquals($this->redis->lGetRange('list', 0, 2), array('val3', 'val2', 'val'));
	$this->assertEquals($this->redis->lGetRange('list', 0, 3), array('val3', 'val2', 'val'));

	$this->assertEquals($this->redis->lGetRange('list', 0, -1), array('val3', 'val2', 'val'));
	$this->assertEquals($this->redis->lGetRange('list', 0, -2), array('val3', 'val2'));
	$this->assertEquals($this->redis->lGetRange('list', -2, -1), array('val2', 'val'));

	$this->redis->delete('list');
	$this->assertEquals($this->redis->lGetRange('list', 0, -1), array());
    }

    public function testsave() {
	$this->assertTrue($this->redis->save() === TRUE);	// don't really know how else to test this...
    }
    public function testbgSave() {
	// let's try to fill the DB and then bgSave twice. We expect the second one to fail.
	for($i = 0; $i < 10e+4; $i++) {
	    $s = md5($i);
	    $this->redis->set($s, $s);
	}
	$this->assertTrue($this->redis->bgSave() === TRUE);	// the first one should work.
	$this->assertTrue($this->redis->bgSave() === FALSE);	// the second one should fail (still working on the first one)
    }

    public function testlastSave() {
	while(!$this->redis->save()) {
	    sleep(1);
	}
	$t_php = microtime(TRUE);
	$t_redis = $this->redis->lastSave();

	$this->assertTrue($t_php - $t_redis < 10000); // check that it's approximately what we've measured in PHP.
    }

    public function testflushDb() {
	$this->redis->set('x', 'y');
	$this->assertTrue($this->redis->flushDb());
	$this->assertTrue($this->redis->getKeys('*') === array());
    }

    public function testflushAll() {
	$this->redis->set('x', 'y');
	$this->assertTrue($this->redis->flushAll());
	$this->assertTrue($this->redis->getKeys('*') === array());
    }

    public function testdbSize() {
	$this->assertTrue($this->redis->flushDB());
	$this->redis->set('x', 'y');
	$this->assertTrue($this->redis->dbSize() === 1);
    }

    public function testttl() {
	$this->redis->set('x', 'y');
	$this->redis->setTimeout('x', 5);
	for($i = 5; $i > 0; $i--) {
		$this->assertEquals($i, $this->redis->ttl('x'));
		sleep(1);
	}
    }

    public function testinfo() {
	$info = $this->redis->info();

	$keys = array(
	    "redis_version",
	    "arch_bits",
	    "uptime_in_seconds",
	    "uptime_in_days",
	    "connected_clients",
	    "connected_slaves",
	    "used_memory",
	    "changes_since_last_save",
	    "bgsave_in_progress",
	    "last_save_time",
	    "total_connections_received",
	    "total_commands_processed",
	    "role");


	foreach($keys as $k) {
	    $this->assertTrue(in_array($k, array_keys($info)));
	}
    }

    public function testSelect() {
	$this->assertFalse($this->redis->select(-1));
	$this->assertTrue($this->redis->select(0));
    }

    public function testMset() {
	$this->redis->delete('x', 'y', 'z');	// remove x y z
	$this->assertTrue($this->redis->mset(array('x' => 'a', 'y' => 'b', 'z' => 'c')));	// set x y z

	$this->assertEquals($this->redis->mget(array('x', 'y', 'z')), array('a', 'b', 'c'));	// check x y z

	$this->redis->delete('x');	// delete just x
	$this->assertTrue($this->redis->mset(array('x' => 'a', 'y' => 'b', 'z' => 'c')));	// set x y z
	$this->assertEquals($this->redis->mget(array('x', 'y', 'z')), array('a', 'b', 'c'));	// check x y z

	$this->assertFalse($this->redis->mset(array())); // set ø → FALSE
    }

    public function testRpopLpush() {

	// standard case.
	$this->redis->delete('x', 'y');
	$this->redis->lpush('x', 'abc');
	$this->redis->lpush('x', 'def');	// x = [def, abc]

	$this->redis->lpush('y', '123');
	$this->redis->lpush('y', '456');	// y = [456, 123]

	$this->assertEquals($this->redis->rpoplpush('x', 'y'), 'abc');	// we RPOP x, yielding abc.
	$this->assertEquals($this->redis->lgetRange('x', 0, -1), array('def'));	// only def remains in x.
	$this->assertEquals($this->redis->lgetRange('y', 0, -1), array('abc', '456', '123'));	// abc has been lpushed to y.

	// with an empty source, expecting no change.
	$this->redis->delete('x', 'y');
	$this->assertTrue(FALSE === $this->redis->rpoplpush('x', 'y'));
	$this->assertTrue(array() === $this->redis->lgetRange('x', 0, -1));
	$this->assertTrue(array() === $this->redis->lgetRange('y', 0, -1));

    }

    public function testZX() {

	$this->redis->delete('key');

	$this->assertTrue(array() === $this->redis->zRange('key', 0, -1));

	$this->assertTrue(1 === $this->redis->zAdd('key', 0, 'val0'));
	$this->assertTrue(1 === $this->redis->zAdd('key', 2, 'val2'));
	$this->assertTrue(1 === $this->redis->zAdd('key', 1, 'val1'));
	$this->assertTrue(1 === $this->redis->zAdd('key', 3, 'val3'));

	$this->assertTrue(array('val0', 'val1', 'val2', 'val3') === $this->redis->zRange('key', 0, -1));

	$this->assertTrue(0 === $this->redis->zDelete('key', 'valX'));
	$this->assertTrue(1 === $this->redis->zDelete('key', 'val3'));

	$this->assertTrue(array('val0', 'val1', 'val2') === $this->redis->zRange('key', 0, -1));

	// zGetReverseRange

	$this->assertTrue(1 === $this->redis->zAdd('key', 3, 'val3'));
	$this->assertTrue(1 === $this->redis->zAdd('key', 3, 'aal3'));

	$zero_to_three = $this->redis->zRangeByScore('key', 0, 3);
	$this->assertTrue(array('val0', 'val1', 'val2', 'aal3', 'val3') === $zero_to_three || array('val0', 'val1', 'val2', 'val3', 'aal3') === $zero_to_three);

	// withscores
	$this->redis->zRemove('key', 'aal3');
	$zero_to_three = $this->redis->zRangeByScore('key', 0, 3, array('withscores' => TRUE));
	$this->assertTrue(array('val0' => 0, 'val1' => 1, 'val2' => 2, 'val3' => 3) == $zero_to_three);

	// limit
	$this->assertTrue(array('val0') === $this->redis->zRangeByScore('key', 0, 3, array('limit' => array(0, 1))));
	$this->assertTrue(array('val0', 'val1') === $this->redis->zRangeByScore('key', 0, 3, array('limit' => array(0, 2))));
	$this->assertTrue(array('val1', 'val2') === $this->redis->zRangeByScore('key', 0, 3, array('limit' => array(1, 2))));
	$this->assertTrue(array('val0', 'val1') === $this->redis->zRangeByScore('key', 0, 1, array('limit' => array(0, 100))));

	$this->assertTrue(4 === $this->redis->zSize('key'));
	$this->assertTrue(1.0 === $this->redis->zScore('key', 'val1'));
	$this->assertFalse($this->redis->zScore('key', 'val'));
	$this->assertFalse($this->redis->zScore(3, 2));

	// zincrby
	$this->redis->delete('key');
	$this->assertTrue(1.0 === $this->redis->zIncrBy('key', 1, 'val1'));
	$this->assertTrue(1.0 === $this->redis->zScore('key', 'val1'));
	$this->assertTrue(2.5 === $this->redis->zIncrBy('key', 1.5, 'val1'));
	$this->assertTrue(2.5 === $this->redis->zScore('key', 'val1'));
    }

    public function testHashes() {
	$this->redis->delete('h', 'key');

	$this->assertTrue(0 === $this->redis->hLen('h'));
	$this->assertTrue(TRUE === $this->redis->hSet('h', 'a', 'a-value'));
	$this->assertTrue(1 === $this->redis->hLen('h'));
	$this->assertTrue(TRUE === $this->redis->hSet('h', 'b', 'b-value'));
	$this->assertTrue(2 === $this->redis->hLen('h'));

	$this->assertTrue('a-value' === $this->redis->hGet('h', 'a')); 	// simple get
	$this->assertTrue('b-value' === $this->redis->hGet('h', 'b')); 	// simple get

	$this->assertTrue(FALSE === $this->redis->hSet('h', 'a', 'another-value')); // replacement
	$this->assertTrue('another-value' === $this->redis->hGet('h', 'a')); 	// get the new value

	$this->assertTrue('b-value' === $this->redis->hGet('h', 'b')); 	// simple get
	$this->assertTrue(FALSE === $this->redis->hGet('h', 'c'));	// unknown hash member
	$this->assertTrue(FALSE === $this->redis->hGet('key', 'c'));	// unknownkey

	// hDel
	$this->assertTrue(TRUE === $this->redis->hDel('h', 'a')); // TRUE on success
	$this->assertTrue(FALSE === $this->redis->hDel('h', 'a')); // FALSE on failure

	$this->redis->delete('h');
	$this->redis->hSet('h', 'x', 'a');
	$this->redis->hSet('h', 'y', 'b');

	// keys
	$keys = $this->redis->hKeys('h');
	$this->assertTrue($keys === array('x', 'y') || $keys === array('y', 'x'));

	// values
	$values = $this->redis->hVals('h');
	$this->assertTrue($values === array('a', 'b') || $values === array('b', 'a'));

	// keys + values
	$all = $this->redis->hGetAll('h');
	$this->assertTrue($all === array('x' => 'a', 'y' => 'b') || $all === array('y' => 'b', 'x' => 'a'));

	// hExists
	$this->assertTrue(TRUE === $this->redis->hExists('h', 'x'));
	$this->assertTrue(TRUE === $this->redis->hExists('h', 'y'));
	$this->assertTrue(FALSE === $this->redis->hExists('h', 'w'));
	$this->redis->delete('h');
	$this->assertTrue(FALSE === $this->redis->hExists('h', 'x'));

	// hIncrBy - not even in redis yet.
	/*
	$this->redis->delete('h');
	$this->assertTrue(2.5 === $this->redis->hIncrBy('h', 2.5, 'x'));
	$this->assertTrue(3.5 === $this->redis->hIncrBy('h', 1, 'x'));

	$this->redis->hSet('h', 'y', 'not-a-number');
	$this->assertTrue(FALSE === $this->redis->hIncrBy('h', 1, 'y'));
	*/

    }

}

?>
