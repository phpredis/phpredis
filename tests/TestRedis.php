<?php

require_once(dirname($_SERVER['PHP_SELF'])."/test.php");

echo "Note: these tests might take up to a minute. Don't worry :-)\n";

class Redis_Test extends TestSuite
{
	const HOST = '127.0.0.1';
	const PORT = 6379;
	const AUTH = NULL; //replace with a string to use Redis authentication

    /**
     * @var Redis
     */
    public $redis;

    public function setUp()
    {
	$this->redis = $this->newInstance();
	$info = $this->redis->info();
	$this->version = (isset($info['redis_version'])?$info['redis_version']:'0.0.0');
    }

	private function newInstance() {
	$r = new Redis();
	$r->connect(self::HOST, self::PORT);

	if(self::AUTH) {
		$this->assertTrue($r->auth(self::AUTH));
	}
	return $r;
    }

    public function tearDown()
    {
	if($this->redis) {
	    $this->redis->close();
	}
   //     unset($this->redis);
    }

    public function reset()
    {
        $this->setUp();
        $this->tearDown();
    }

    public function testMinimumVersion()
    {
	// Minimum server version required for tests
	$this->assertTrue(version_compare($this->version, "2.4.0", "ge"));
    }

    public function testPing()
    {

	$this->assertEquals('+PONG', $this->redis->ping());

	$count = 1000;
	while($count --) {
	    	$this->assertEquals('+PONG', $this->redis->ping());
	}
    }

	public function testPipelinePublish() {

		$ret = $this->redis->pipeline()
			->publish('chan', 'msg')
			->exec();

		$this->assertTrue(is_array($ret) && count($ret) === 1 && $ret[0] >= 0);
	}

    public function testBitsets() {

	    $this->redis->delete('key');
	    $this->assertTrue(0 === $this->redis->getBit('key', 0));
	    $this->assertTrue(FALSE === $this->redis->getBit('key', -1));
	    $this->assertTrue(0 === $this->redis->getBit('key', 100000));

	    $this->redis->set('key', "\xff");
	    for($i = 0; $i < 8; $i++) {
		    $this->assertTrue(1 === $this->redis->getBit('key', $i));
	    }
	    $this->assertTrue(0 === $this->redis->getBit('key', 8));

	    // negative offset doesn't work
	    $this->assertTrue(FALSE === $this->redis->setBit('key', -1, 0));
	    $this->assertTrue(1 === $this->redis->getBit('key', 0));

	    // change bit 0
	    $this->assertTrue(1 === $this->redis->setBit('key', 0, 0));
	    $this->assertTrue(0 === $this->redis->setBit('key', 0, 0));
	    $this->assertTrue(0 === $this->redis->getBit('key', 0));
	    $this->assertTrue("\x7f" === $this->redis->get('key'));

	    // change bit 1
	    $this->assertTrue(1 === $this->redis->setBit('key', 1, 0));
	    $this->assertTrue(0 === $this->redis->setBit('key', 1, 0));
	    $this->assertTrue(0 === $this->redis->getBit('key', 1));
	    $this->assertTrue("\x3f" === $this->redis->get('key'));

	    // change bit > 1
	    $this->assertTrue(1 === $this->redis->setBit('key', 2, 0));
	    $this->assertTrue(0 === $this->redis->setBit('key', 2, 0));
	    $this->assertTrue(0 === $this->redis->getBit('key', 2));
	    $this->assertTrue("\x1f" === $this->redis->get('key'));

	    // values above 1 are changed to 1 but don't overflow on bits to the right.
	    $this->assertTrue(0 === $this->redis->setBit('key', 0, 0xff));
        $this->assertTrue("\x9f" === $this->redis->get('key'));

        // Verify valid offset ranges
        $this->assertFalse($this->redis->getBit('key', -1));
        $this->assertFalse($this->redis->getBit('key', 4294967296));
        $this->assertFalse($this->redis->setBit('key', -1, 1));
        $this->assertFalse($this->redis->setBit('key', 4294967296, 1));
    }

    public function test1000() {

	 $s = str_repeat('A', 1000);
	 $this->redis->set('x', $s);
	 $this->assertEquals($s, $this->redis->get('x'));

	 $s = str_repeat('A', 1000000);
	 $this->redis->set('x', $s);
	 $this->assertEquals($s, $this->redis->get('x'));
    }

	public function testEcho() {
		$this->assertEquals($this->redis->echo("hello"), "hello");
		$this->assertEquals($this->redis->echo(""), "");
		$this->assertEquals($this->redis->echo(" 0123 "), " 0123 ");
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
        $this->redis->delete('keyNotExist');
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

    /* Extended SET options for Redis >= 2.6.12 */
    public function testExtendedSet() {
        // Skip the test if we don't have a new enough version of Redis
        if(version_compare($this->version, '2.6.12', 'lt')) {
            $this->markTestSkipped();
            return;
        }

        /* Legacy SETEX redirection */
        $this->redis->del('foo');
        $this->assertTrue($this->redis->set('foo','bar', 20));
        $this->assertEquals($this->redis->get('foo'), 'bar');
        $this->assertEquals($this->redis->ttl('foo'), 20);

        /* Invalid third arguments */
        $this->assertFalse($this->redis->set('foo','bar','baz'));
        $this->assertFalse($this->redis->set('foo','bar',new StdClass()));

        /* Set if not exist */
        $this->redis->del('foo');
        $this->assertTrue($this->redis->set('foo','bar',Array('nx')));
        $this->assertEquals($this->redis->get('foo'), 'bar');
        $this->assertFalse($this->redis->set('foo','bar',Array('nx')));

        /* Set if exists */
        $this->assertTrue($this->redis->set('foo','bar',Array('xx')));
        $this->assertEquals($this->redis->get('foo'), 'bar');
        $this->redis->del('foo');
        $this->assertFalse($this->redis->set('foo','bar',Array('xx')));

        /* Set with a TTL */
        $this->assertTrue($this->redis->set('foo','bar',Array('ex'=>100)));
        $this->assertEquals($this->redis->ttl('foo'), 100);

        /* Set with a PTTL */
        $this->assertTrue($this->redis->set('foo','bar',Array('px'=>100000)));
        $this->assertTrue(100000 - $this->redis->pttl('foo') < 1000);

        /* Set if exists, with a TTL */
        $this->assertTrue($this->redis->set('foo','bar',Array('xx','ex'=>105)));
        $this->assertEquals($this->redis->ttl('foo'), 105);
        $this->assertEquals($this->redis->get('foo'), 'bar');

        /* Set if not exists, with a TTL */
        $this->redis->del('foo');
        $this->assertTrue($this->redis->set('foo','bar', Array('nx', 'ex'=>110)));
        $this->assertEquals($this->redis->ttl('foo'), 110);
        $this->assertEquals($this->redis->get('foo'), 'bar');
        $this->assertFalse($this->redis->set('foo','bar', Array('nx', 'ex'=>110)));

        /* Throw some nonsense into the array, and check that the TTL came through */
        $this->redis->del('foo');
        $this->assertTrue($this->redis->set('foo','barbaz', Array('not-valid','nx','invalid','ex'=>200)));
        $this->assertEquals($this->redis->ttl('foo'), 200);
        $this->assertEquals($this->redis->get('foo'), 'barbaz');

        /* Pass NULL as the optional arguments which should be ignored */
        $this->redis->del('foo');
        $this->redis->set('foo','bar', NULL);
        $this->assertEquals($this->redis->get('foo'), 'bar');
        $this->assertTrue($this->redis->ttl('foo')<0);
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

	// variadic
	$this->redis->delete('key0');
	$this->assertTrue(3 === $this->redis->lPush('key0', 'val0', 'val1', 'val2'));
	$this->assertTrue(array('val2', 'val1', 'val0') === $this->redis->lrange('key0', 0, -1));

	$this->redis->delete('key0');
	$this->assertTrue(3 === $this->redis->rPush('key0', 'val0', 'val1', 'val2'));
	$this->assertTrue(array('val0', 'val1', 'val2') === $this->redis->lrange('key0', 0, -1));
    }

    public function testRenameNx() {

	// strings
	$this->redis->delete('key0', 'key1');
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
	$this->assertTrue($this->redis->renameNx('key0', 'key1') === FALSE);
	$this->assertTrue($this->redis->lGetRange('key0', 0, -1) === array('val1', 'val0'));
	$this->assertTrue($this->redis->lGetRange('key1', 0, -1) === array('val1-1', 'val1-0'));

	$this->redis->delete('key2');
	$this->assertTrue($this->redis->renameNx('key0', 'key2') === TRUE);
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
	$this->redis->set(1, 'test');

	$this->assertEquals(array('v1'), $this->redis->getMultiple(array('k1')));
	$this->assertEquals(array('v1', 'v3', false), $this->redis->getMultiple(array('k1', 'k3', 'NoKey')));
	$this->assertEquals(array('v1', 'v2', 'v3'), $this->redis->getMultiple(array('k1', 'k2', 'k3')));
	$this->assertEquals(array('v1', 'v2', 'v3'), $this->redis->getMultiple(array('k1', 'k2', 'k3')));

	$this->redis->set('k5', '$1111111111');
	$this->assertEquals(array(0 => '$1111111111'), $this->redis->getMultiple(array('k5')));

	$this->assertEquals(array(0 => 'test'), $this->redis->getMultiple(array(1))); // non-string
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

    public function testSetEx() {

	    $this->redis->delete('key');
	    $this->assertTrue($this->redis->setex('key', 7, 'val') === TRUE);
	    $this->assertTrue($this->redis->ttl('key') ===7);
	    $this->assertTrue($this->redis->get('key') === 'val');
    }

    public function testSetNX() {

	    $this->redis->set('key', 42);
	    $this->assertTrue($this->redis->setnx('key', 'err') === FALSE);
	    $this->assertTrue($this->redis->get('key') === '42');

	    $this->redis->delete('key');
	    $this->assertTrue($this->redis->setnx('key', '42') === TRUE);
	    $this->assertTrue($this->redis->get('key') === '42');
    }

    public function testExpireAtWithLong() {
        $longExpiryTimeExceedingInt = 3153600000;
        $this->redis->delete('key');
        $this->assertTrue($this->redis->setex('key', $longExpiryTimeExceedingInt, 'val') === TRUE);
        $this->assertTrue($this->redis->ttl('key') === $longExpiryTimeExceedingInt);
    }

    public function testIncr()
    {
        $this->redis->set('key', 0);

        $this->redis->incr('key');
	$this->assertEquals(1, (int)$this->redis->get('key'));

        $this->redis->incr('key');
	$this->assertEquals(2, (int)$this->redis->get('key'));

       $this->redis->incr('key', 3);
	$this->assertEquals(5, (int)$this->redis->get('key'));

	$this->redis->incrBy('key', 3);
	$this->assertEquals(8, (int)$this->redis->get('key'));

	$this->redis->incrBy('key', 1);
	$this->assertEquals(9, (int)$this->redis->get('key'));

	$this->redis->incrBy('key', -1);
	$this->assertEquals(8, (int)$this->redis->get('key'));

	$this->redis->delete('key');

	$this->redis->set('key', 'abc');

	$this->redis->incr('key');
	$this->assertTrue("abc" === $this->redis->get('key'));

	$this->redis->incr('key');
	$this->assertTrue("abc" === $this->redis->get('key'));
	}

    public function testIncrByFloat()
    {
	    // incrbyfloat is new in 2.6.0
	    if (version_compare($this->version, "2.5.0", "lt")) {
            $this->markTestSkipped();
        }

        $this->redis->delete('key');
        
        $this->redis->set('key', 0);

        $this->redis->incrbyfloat('key', 1.5);
        $this->assertEquals('1.5', $this->redis->get('key'));

        $this->redis->incrbyfloat('key', 2.25);
        $this->assertEquals('3.75', $this->redis->get('key'));

        $this->redis->incrbyfloat('key', -2.25);
        $this->assertEquals('1.5', $this->redis->get('key'));

        $this->redis->set('key', 'abc');

        $this->redis->incrbyfloat('key', 1.5);
        $this->assertTrue("abc" === $this->redis->get('key'));

        $this->redis->incrbyfloat('key', -1.5);
        $this->assertTrue("abc" === $this->redis->get('key'));

        // Test with prefixing
        $this->redis->setOption(Redis::OPT_PREFIX, 'someprefix:');
        $this->redis->del('key');
        $this->redis->incrbyfloat('key',1.8);
        $this->assertEquals('1.8', $this->redis->get('key'));
        $this->redis->setOption(Redis::OPT_PREFIX, '');
        $this->assertTrue($this->redis->exists('someprefix:key'));
        $this->redis->del('someprefix:key');

    }

    public function testDecr()
    {
        $this->redis->set('key', 5);

        $this->redis->decr('key');
	$this->assertEquals(4, (int)$this->redis->get('key'));

        $this->redis->decr('key');
	$this->assertEquals(3, (int)$this->redis->get('key'));

        $this->redis->decr('key', 2);
	$this->assertEquals(1, (int)$this->redis->get('key'));

	$this->redis->decr('key', 2);
	$this->assertEquals(-1, (int)$this->redis->get('key'));

	$this->redis->decrBy('key', 2);
	$this->assertEquals(-3, (int)$this->redis->get('key'));

	$this->redis->decrBy('key', 1);
	$this->assertEquals(-4, (int)$this->redis->get('key'));

	$this->redis->decr('key', -10);
	$this->assertEquals(6, (int)$this->redis->get('key'));
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

	$this->redis->set($pattern.'3', 'something');

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
        $this->assertEquals(false, $this->redis->get($key));

	// multiple, all existing
	$this->redis->set('x', 0);
	$this->redis->set('y', 1);
	$this->redis->set('z', 2);
	$this->assertEquals(3, $this->redis->delete('x', 'y', 'z'));
	$this->assertEquals(false, $this->redis->get('x'));
	$this->assertEquals(false, $this->redis->get('y'));
	$this->assertEquals(false, $this->redis->get('z'));

	// multiple, none existing
	$this->assertEquals(0, $this->redis->delete('x', 'y', 'z'));
	$this->assertEquals(false, $this->redis->get('x'));
	$this->assertEquals(false, $this->redis->get('y'));
	$this->assertEquals(false, $this->redis->get('z'));

	// multiple, some existing
	$this->redis->set('y', 1);
	$this->assertEquals(1, $this->redis->delete('x', 'y', 'z'));
	$this->assertEquals(false, $this->redis->get('y'));

	$this->redis->set('x', 0);
	$this->redis->set('y', 1);
	$this->assertEquals(2, $this->redis->delete(array('x', 'y')));

    }

    public function testType()
    {
	// 0 => none, (key didn't exist)
	// 1=> string,
	// 2 => set,
	// 3 => list,
	// 4 => zset,
	// 5 => hash

	// string
	$this->redis->set('key', 'val');
	$this->assertEquals(Redis::REDIS_STRING, $this->redis->type('key'));

	// list
	$this->redis->lPush('keyList', 'val0');
	$this->redis->lPush('keyList', 'val1');
	$this->assertEquals(Redis::REDIS_LIST, $this->redis->type('keyList'));

	// set
	$this->redis->delete('keySet');
	$this->redis->sAdd('keySet', 'val0');
	$this->redis->sAdd('keySet', 'val1');
	$this->assertEquals(Redis::REDIS_SET, $this->redis->type('keySet'));

    // sadd with numeric key
	$this->redis->delete(123);
	$this->assertTrue(1 === $this->redis->sAdd(123, 'val0'));
	$this->assertTrue(array('val0') === $this->redis->sMembers(123));

	// zset
	$this->redis->delete('keyZSet');
	$this->redis->zAdd('keyZSet', 0, 'val0');
	$this->redis->zAdd('keyZSet', 1, 'val1');
	$this->assertEquals(Redis::REDIS_ZSET, $this->redis->type('keyZSet'));

	// hash
	$this->redis->delete('keyHash');
	$this->redis->hSet('keyHash', 'key0', 'val0');
	$this->redis->hSet('keyHash', 'key1', 'val1');
	$this->assertEquals(Redis::REDIS_HASH, $this->redis->type('keyHash'));

	//None
	$this->assertEquals(Redis::REDIS_NOT_FOUND, $this->redis->type('keyNotExists'));
    }

	public function testStr() {

		$this->redis->set('key', 'val1');
		$this->assertTrue($this->redis->append('key', 'val2') === 8);
		$this->assertTrue($this->redis->get('key') === 'val1val2');

		$this->assertTrue($this->redis->append('keyNotExist', 'value') === 5);
		$this->assertTrue($this->redis->get('keyNotExist') === 'value');

		$this->redis->set('key', 'This is a string') ;
		$this->assertTrue($this->redis->getRange('key', 0, 3) === 'This');
		$this->assertTrue($this->redis->getRange('key', -6, -1) === 'string');
		$this->assertTrue($this->redis->getRange('key', -6, 100000) === 'string');
		$this->assertTrue($this->redis->get('key') === 'This is a string');

		$this->redis->set('key', 'This is a string') ;
		$this->assertTrue($this->redis->strlen('key') === 16);

		$this->redis->set('key', 10) ;
		$this->assertTrue($this->redis->strlen('key') === 2);
		$this->redis->set('key', '') ;
		$this->assertTrue($this->redis->strlen('key') === 0);
		$this->redis->set('key', '000') ;
		$this->assertTrue($this->redis->strlen('key') === 3);
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

	public function testblockingPop() {

	// non blocking blPop, brPop
        $this->redis->delete('list');
        $this->redis->lPush('list', 'val1');
        $this->redis->lPush('list', 'val2');
		$this->assertTrue($this->redis->blPop(array('list'), 2) === array('list', 'val2'));
		$this->assertTrue($this->redis->blPop(array('list'), 2) === array('list', 'val1'));

        $this->redis->delete('list');
        $this->redis->lPush('list', 'val1');
        $this->redis->lPush('list', 'val2');
	$this->assertTrue($this->redis->brPop(array('list'), 1) === array('list', 'val1'));
	$this->assertTrue($this->redis->brPop(array('list'), 1) === array('list', 'val2'));

	// blocking blpop, brpop
        $this->redis->delete('list');
	$this->assertTrue($this->redis->blPop(array('list'), 1) === array());
	$this->assertTrue($this->redis->brPop(array('list'), 1) === array());

	// TODO: fix this broken test.
//		$this->redis->delete('list');
//		$params = array(
//			0 => array("pipe", "r"),
//			1 => array("pipe", "w"),
//			2 => array("file", "/dev/null", "w")
//		);
//		if(function_exists('proc_open')) {
//			$env = array('PHPREDIS_key' =>'list', 'PHPREDIS_value' => 'value');
//			$process = proc_open('php', $params, $pipes, '/tmp', $env);
//
//			if (is_resource($process)) {
//				fwrite($pipes[0],  '<?php
//	sleep(2);
//	$r = new Redis;
//	$r->connect("'.self::HOST.'", '.self::PORT.');
//	if("'.addslashes(self::AUTH).'") {
//		$r->auth("'.addslashes(self::AUTH).'");
//	}
//	$r->lPush($_ENV["PHPREDIS_key"], $_ENV["PHPREDIS_value"]);
//	?' . '>');
//
//				fclose($pipes[0]);
//				fclose($pipes[1]);
//				$re = proc_close($process);
//
//				$this->assertTrue($this->redis->blPop(array('list'), 5) === array("list", "value"));
//			}
//		}

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

    //lInsert, lPopx, rPopx
    public function testlPopx() {
		//test lPushx/rPushx
		$this->redis->delete('keyNotExists');
		$this->assertTrue($this->redis->lPushx('keyNotExists', 'value') === 0);
		$this->assertTrue($this->redis->rPushx('keyNotExists', 'value') === 0);

		$this->redis->delete('key');
		$this->redis->lPush('key', 'val0');
		$this->assertTrue($this->redis->lPushx('key', 'val1') === 2);
		$this->assertTrue($this->redis->rPushx('key', 'val2') === 3);
		$this->assertTrue($this->redis->lGetRange('key', 0, -1) === array('val1', 'val0', 'val2'));

		//test linsert
		$this->redis->delete('key');
		$this->redis->lPush('key', 'val0');
		$this->assertTrue($this->redis->lInsert('keyNotExists', Redis::AFTER, 'val1', 'val2') === 0);
		$this->assertTrue($this->redis->lInsert('key', Redis::BEFORE, 'valX', 'val2') === -1);

		$this->assertTrue($this->redis->lInsert('key', Redis::AFTER, 'val0', 'val1') === 2);
		$this->assertTrue($this->redis->lInsert('key', Redis::BEFORE, 'val0', 'val2') === 3);
		$this->assertTrue($this->redis->lGetRange('key', 0, -1) === array('val2', 'val0', 'val1'));
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

    public function testSortPrefix() {
	// Make sure that sorting works with a prefix
        $this->redis->setOption(Redis::OPT_PREFIX, 'some-prefix:');
        $this->redis->del('some-item');
        $this->redis->sadd('some-item', 1);
        $this->redis->sadd('some-item', 2);
        $this->redis->sadd('some-item', 3);

        $this->assertEquals(array('1','2','3'), $this->redis->sortAsc('some-item'));
	$this->assertEquals(array('3','2','1'), $this->redis->sortDesc('some-item'));
	$this->assertEquals(array('1','2','3'), $this->redis->sort('some-item'));

	// Kill our set/prefix
	$this->redis->del('some-item');
	$this->redis->setOption(Redis::OPT_PREFIX, '');
    }

    public function testSortAsc() {

	$this->setupSort();

	$this->assertTrue(FALSE === $this->redis->sortAsc(NULL));

	// sort by age and get IDs
	$byAgeAsc = array('3','1','2','4');
	$this->assertEquals($byAgeAsc, $this->redis->sortAsc('person:id', 'person:age_*'));
	$this->assertEquals($byAgeAsc, $this->redis->sort('person:id', array('by' => 'person:age_*', 'sort' => 'asc')));
	$this->assertEquals(array('1', '2', '3', '4'), $this->redis->sortAsc('person:id', NULL));	// check that NULL works.
	$this->assertEquals(array('1', '2', '3', '4'), $this->redis->sortAsc('person:id', NULL, NULL));	// for all fields.
	$this->assertEquals(array('1', '2', '3', '4'), $this->redis->sort('person:id', array('sort' => 'asc')));

	// sort by age and get names
	$byAgeAsc = array('Carol','Alice','Bob','Dave');
	$this->assertEquals($byAgeAsc, $this->redis->sortAsc('person:id', 'person:age_*', 'person:name_*'));
	$this->assertEquals($byAgeAsc, $this->redis->sort('person:id', array('by' => 'person:age_*', 'get' => 'person:name_*', 'sort' => 'asc')));

	$this->assertEquals(array_slice($byAgeAsc, 0, 2), $this->redis->sortAsc('person:id', 'person:age_*', 'person:name_*', 0, 2));
	$this->assertEquals(array_slice($byAgeAsc, 0, 2), $this->redis->sort('person:id', array('by' => 'person:age_*', 'get' => 'person:name_*', 'limit' => array(0, 2), 'sort' => 'asc')));

	$this->assertEquals(array_slice($byAgeAsc, 1, 2), $this->redis->sortAsc('person:id', 'person:age_*', 'person:name_*', 1, 2));
	$this->assertEquals(array_slice($byAgeAsc, 1, 2), $this->redis->sort('person:id', array('by' => 'person:age_*', 'get' => 'person:name_*', 'limit' => array(1, 2), 'sort' => 'asc')));
	$this->assertEquals(array_slice($byAgeAsc, 0, 3), $this->redis->sortAsc('person:id', 'person:age_*', 'person:name_*', NULL, 3)); // NULL is transformed to 0 if there is something after it.
	$this->assertEquals($byAgeAsc, $this->redis->sortAsc('person:id', 'person:age_*', 'person:name_*', 0, 4));
	$this->assertEquals($byAgeAsc, $this->redis->sort('person:id', array('by' => 'person:age_*', 'get' => 'person:name_*', 'limit' => array(0, 4))));
	$this->assertEquals($byAgeAsc, $this->redis->sort('person:id', array('by' => 'person:age_*', 'get' => 'person:name_*', 'limit' => array(0, "4")))); // with strings
	$this->assertEquals($byAgeAsc, $this->redis->sort('person:id', array('by' => 'person:age_*', 'get' => 'person:name_*', 'limit' => array("0", 4))));
	$this->assertEquals(array(), $this->redis->sortAsc('person:id', 'person:age_*', 'person:name_*', NULL, NULL)); // NULL, NULL is the same as (0,0). That returns no element.

	// sort by salary and get ages
	$agesBySalaryAsc = array('34', '27', '25', '41');
	$this->assertEquals($agesBySalaryAsc, $this->redis->sortAsc('person:id', 'person:salary_*', 'person:age_*'));
	$this->assertEquals($agesBySalaryAsc, $this->redis->sort('person:id', array('by' => 'person:salary_*', 'get' => 'person:age_*', 'sort' => 'asc')));

	$agesAndSalaries = $this->redis->sort('person:id', array('by' => 'person:salary_*', 'get' => array('person:age_*', 'person:salary_*'), 'sort' => 'asc'));
	$this->assertEquals(array('34', '2000', '27', '2500', '25', '2800', '41', '3100'), $agesAndSalaries);


	// sort non-alpha doesn't change all-string lists
	// list → [ghi, def, abc]
	$list = array('abc', 'def', 'ghi');
	$this->redis->delete('list');
	foreach($list as $i) {
	    $this->redis->lPush('list', $i);
	}

	// SORT list → [ghi, def, abc]
	if (version_compare($this->version, "2.5.0", "lt")) {
		$this->assertEquals(array_reverse($list), $this->redis->sortAsc('list'));
		$this->assertEquals(array_reverse($list), $this->redis->sort('list', array('sort' => 'asc')));
	} else {
		// TODO rewrite, from 2.6.0 release notes:
		// SORT now will refuse to sort in numerical mode elements that can't be parsed
		// as numbers
	}

	// SORT list ALPHA → [abc, def, ghi]
	$this->assertEquals($list, $this->redis->sortAscAlpha('list'));
	$this->assertEquals($list, $this->redis->sort('list', array('sort' => 'asc', 'alpha' => TRUE)));
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
	if (version_compare($this->version, "2.5.0", "lt")) {
		$this->assertEquals(array_reverse($list), $this->redis->sortDesc('list'));
	} else {
		// TODO rewrite, from 2.6.0 release notes:
		// SORT now will refuse to sort in numerical mode elements that can't be parsed
		// as numbers
	}

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

	$this->assertEquals(1, $this->redis->sAdd('set', 'val'));
	$this->assertEquals(0, $this->redis->sAdd('set', 'val'));

        $this->assertTrue($this->redis->sContains('set', 'val'));
        $this->assertFalse($this->redis->sContains('set', 'val2'));

	$this->assertEquals(1, $this->redis->sAdd('set', 'val2'));

        $this->assertTrue($this->redis->sContains('set', 'val2'));
    }
    public function testsSize()
    {
        $this->redis->delete('set');

	$this->assertEquals(1, $this->redis->sAdd('set', 'val'));

        $this->assertEquals(1, $this->redis->sSize('set'));

	$this->assertEquals(1, $this->redis->sAdd('set', 'val2'));

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
	$this->assertTrue(1 === $this->redis->sSize('set0'));
	$this->assertTrue($v0 === 'val' || $v0 === 'val2');
	$v1 = $this->redis->sPop('set0');
	$this->assertTrue(0 === $this->redis->sSize('set0'));
	$this->assertTrue(($v0 === 'val' && $v1 === 'val2') || ($v1 === 'val' && $v0 === 'val2'));

	$this->assertTrue($this->redis->sPop('set0') === FALSE);
    }

    public function testsRandMember() {
        $this->redis->delete('set0');
        $this->assertTrue($this->redis->sRandMember('set0') === FALSE);

        $this->redis->sAdd('set0', 'val');
        $this->redis->sAdd('set0', 'val2');

        $got = array();
        while(true) {
            $v = $this->redis->sRandMember('set0');
            $this->assertTrue(2 === $this->redis->sSize('set0')); // no change.
            $this->assertTrue($v === 'val' || $v === 'val2');

            $got[$v] = $v;
            if(count($got) == 2) {
                break;
            }
        }

        // 
        // With and without count, while serializing
        // 

        $this->redis->delete('set0');
        $this->redis->setOption(Redis::OPT_SERIALIZER, Redis::SERIALIZER_PHP);
        for($i=0;$i<5;$i++) {
            $member = "member:$i";
            $this->redis->sAdd('set0', $member);
            $mems[] = $member;
        }

        $member = $this->redis->srandmember('set0');
        $this->assertTrue(in_array($member, $mems));

        $rmembers = $this->redis->srandmember('set0', $i);
        foreach($rmembers as $reply_mem) {
            $this->assertTrue(in_array($reply_mem, $mems));
        }

        $this->redis->setOption(Redis::OPT_SERIALIZER, Redis::SERIALIZER_NONE);
    }

    public function testSRandMemberWithCount() {
        // Make sure the set is nuked
        $this->redis->delete('set0');
        
        // Run with a count (positive and negative) on an empty set
        $ret_pos = $this->redis->sRandMember('set0', 10);
        $ret_neg = $this->redis->sRandMember('set0', -10);
        
        // Should both be empty arrays
        $this->assertTrue(is_array($ret_pos) && empty($ret_pos));
        $this->assertTrue(is_array($ret_neg) && empty($ret_neg));

        // Add a few items to the set
        for($i=0;$i<100;$i++) {
            $this->redis->sadd('set0', "member$i");
        }

        // Get less than the size of the list
        $ret_slice = $this->redis->srandmember('set0', 20);

        // Should be an array with 20 items
        $this->assertTrue(is_array($ret_slice) && count($ret_slice) == 20);

        // Ask for more items than are in the list (but with a positive count)
        $ret_slice = $this->redis->srandmember('set0', 200);

        // Should be an array, should be however big the set is, exactly
        $this->assertTrue(is_array($ret_slice) && count($ret_slice) == $i);

        // Now ask for too many items but negative
        $ret_slice = $this->redis->srandmember('set0', -200);

        // Should be an array, should have exactly the # of items we asked for (will be dups)
        $this->assertTrue(is_array($ret_slice) && count($ret_slice) == 200);

        //
        // Test in a pipeline
        //

        $pipe = $this->redis->pipeline();

        $pipe->srandmember('set0', 20);
        $pipe->srandmember('set0', 200);
        $pipe->srandmember('set0', -200);

        $ret = $this->redis->exec();

        $this->assertTrue(is_array($ret[0]) && count($ret[0]) == 20);
        $this->assertTrue(is_array($ret[1]) && count($ret[1]) == $i);
        $this->assertTrue(is_array($ret[2]) && count($ret[2]) == 200);

        // Kill the set
        $this->redis->del('set0');
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

		$sGetMembers = $this->redis->sGetMembers('set');
		sort($sGetMembers);
		$this->assertEquals($array, $sGetMembers);

		$sMembers = $this->redis->sMembers('set');
		sort($sMembers);
		$this->assertEquals($array, $sMembers);	// test alias
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
	$xy = $this->redis->sInter(array('x', 'y'));	// odd prime numbers, as array.
	foreach($xy as $i) {
	    $i = (int)$i;
            $this->assertTrue(in_array($i, array_intersect($x, $y)));
	}

        $yz = $this->redis->sInter('y', 'z');	// set of odd squares
        foreach($yz as $i) {
	    $i = (int)$i;
            $this->assertTrue(in_array($i, array_intersect($y, $z)));
        }
	$yz = $this->redis->sInter(array('y', 'z'));	// set of odd squares, as array
        foreach($yz as $i) {
	    $i = (int)$i;
            $this->assertTrue(in_array($i, array_intersect($y, $z)));
        }

        $zt = $this->redis->sInter('z', 't');	// prime squares
        $this->assertTrue($zt === array());
	$zt = $this->redis->sInter(array('z', 't'));	// prime squares, as array
        $this->assertTrue($zt === array());

        $xyz = $this->redis->sInter('x', 'y', 'z');// odd prime squares
        $this->assertTrue($xyz === array('1'));

	$xyz = $this->redis->sInter(array('x', 'y', 'z'));// odd prime squares, with an array as a parameter
        $this->assertTrue($xyz === array('1'));

        $nil = $this->redis->sInter();
        $this->assertTrue($nil === FALSE);
	$nil = $this->redis->sInter(array());
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


//    public function testsave() {
//	$this->assertTrue($this->redis->save() === TRUE);	// don't really know how else to test this...
//    }
//    public function testbgSave() {
//	// let's try to fill the DB and then bgSave twice. We expect the second one to fail.
//	for($i = 0; $i < 10e+4; $i++) {
//	    $s = md5($i);
//	    $this->redis->set($s, $s);
//	}
//	$this->assertTrue($this->redis->bgSave() === TRUE);	// the first one should work.
//	$this->assertTrue($this->redis->bgSave() === FALSE);	// the second one should fail (still working on the first one)
//    }
//
//    public function testlastSave() {
//	while(!$this->redis->save()) {
//	    sleep(1);
//	}
//	$t_php = microtime(TRUE);
//	$t_redis = $this->redis->lastSave();
//
//	$this->assertTrue($t_php - $t_redis < 10000); // check that it's approximately what we've measured in PHP.
//    }
//
//    public function testflushDb() {
//	$this->redis->set('x', 'y');
//	$this->assertTrue($this->redis->flushDb());
//	$this->assertTrue($this->redis->getKeys('*') === array());
//    }
//
//    public function testflushAll() {
//	$this->redis->set('x', 'y');
//	$this->assertTrue($this->redis->flushAll());
//	$this->assertTrue($this->redis->getKeys('*') === array());
//    }

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

        // A key with no TTL
        $this->redis->del('x'); $this->redis->set('x', 'bar');
        $this->assertEquals($this->redis->ttl('x'), -1);

        // A key that doesn't exist (> 2.8 will return -2)
        if(version_compare($this->version, "2.8.0", "gte")) {
            $this->redis->del('x');
            $this->assertEquals($this->redis->ttl('x'), -2);
        }
    }

    public function testPersist() {
	$this->redis->set('x', 'y');
	$this->redis->setTimeout('x', 100);
	$this->assertTrue(TRUE === $this->redis->persist('x'));		// true if there is a timeout
	$this->assertTrue(-1 === $this->redis->ttl('x'));		// -1: timeout has been removed.
	$this->assertTrue(FALSE === $this->redis->persist('x'));	// false if there is no timeout
	$this->redis->delete('x');
	$this->assertTrue(FALSE === $this->redis->persist('x'));	// false if the key doesn’t exist.
    }

    public function testClient() {
        /* CLIENT SETNAME */
        $this->assertTrue($this->redis->client('setname', 'phpredis_unit_tests'));

        /* CLIENT LIST */
        $arr_clients = $this->redis->client('list');
        $this->assertTrue(is_array($arr_clients));

        // Figure out which ip:port is us!
        $str_addr = NULL;
        foreach($arr_clients as $arr_client) {
            if($arr_client['name'] == 'phpredis_unit_tests') {
                $str_addr = $arr_client['addr'];
            }
        }

        // We should have found our connection
        $this->assertFalse(empty($str_addr));
        
        /* CLIENT GETNAME */
        $this->assertTrue($this->redis->client('getname'), 'phpredis_unit_tests');
         
        /* CLIENT KILL -- phpredis will reconnect, so we can do this */
        $this->assertTrue($this->redis->client('kill', $str_addr));
    }

    public function testSlowlog() {
        // We don't really know what's going to be in the slowlog, but make sure
        // the command returns proper types when called in various ways
        $this->assertTrue(is_array($this->redis->slowlog('get')));
        $this->assertTrue(is_array($this->redis->slowlog('get', 10)));
        $this->assertTrue(is_int($this->redis->slowlog('len')));
        $this->assertTrue($this->redis->slowlog('reset'));
        $this->assertFalse($this->redis->slowlog('notvalid'));
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
	    "total_connections_received",
	    "total_commands_processed",
	    "role");
	if (version_compare($this->version, "2.5.0", "lt")) {
		array_push($keys,
		    "changes_since_last_save",
		    "bgsave_in_progress",
		    "last_save_time"
		);
	} else {
		array_push($keys,
		    "rdb_changes_since_last_save",
		    "rdb_bgsave_in_progress",
		    "rdb_last_save_time"
		);
	}

	foreach($keys as $k) {
	    $this->assertTrue(in_array($k, array_keys($info)));
	}
	}

    public function testInfoCommandStats() {

	// INFO COMMANDSTATS is new in 2.6.0
	if (version_compare($this->version, "2.5.0", "lt")) {
		$this->markTestSkipped();
	}

	$info = $this->redis->info("COMMANDSTATS");

	$this->assertTrue(is_array($info));
	if (is_array($info)) {
		foreach($info as $k => $value) {
			$this->assertTrue(strpos($k, 'cmdstat_') !== false);
	    }
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


	/*
	 * Integer keys
	 */

	// No prefix
	$set_array = Array(-1 => 'neg1', -2 => 'neg2', -3 => 'neg3', 1 => 'one', 2 => 'two', '3' => 'three');
	$this->redis->delete(array_keys($set_array));
	$this->assertTrue($this->redis->mset($set_array));
	$this->assertEquals($this->redis->mget(array_keys($set_array)), array_values($set_array));
	$this->redis->delete(array_keys($set_array));

	// With a prefix
	$this->redis->setOption(Redis::OPT_PREFIX, 'pfx:');
	$this->redis->delete(array_keys($set_array));
	$this->assertTrue($this->redis->mset($set_array));
	$this->assertEquals($this->redis->mget(array_keys($set_array)), array_values($set_array));
	$this->redis->delete(array_keys($set_array));
	$this->redis->setOption(Redis::OPT_PREFIX, '');
    }

    public function testMsetNX() {
	$this->redis->delete('x', 'y', 'z');	// remove x y z
	$this->assertTrue(TRUE === $this->redis->msetnx(array('x' => 'a', 'y' => 'b', 'z' => 'c')));	// set x y z

	$this->assertEquals($this->redis->mget(array('x', 'y', 'z')), array('a', 'b', 'c'));	// check x y z

	$this->redis->delete('x');	// delete just x
	$this->assertTrue(FALSE === $this->redis->msetnx(array('x' => 'A', 'y' => 'B', 'z' => 'C')));	// set x y z
	$this->assertEquals($this->redis->mget(array('x', 'y', 'z')), array(FALSE, 'b', 'c'));	// check x y z

	$this->assertFalse($this->redis->msetnx(array())); // set ø → FALSE
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

    public function testBRpopLpush() {

	// standard case.
	$this->redis->delete('x', 'y');
	$this->redis->lpush('x', 'abc');
	$this->redis->lpush('x', 'def');	// x = [def, abc]

	$this->redis->lpush('y', '123');
	$this->redis->lpush('y', '456');	// y = [456, 123]

	$this->assertEquals($this->redis->brpoplpush('x', 'y', 1), 'abc');	// we RPOP x, yielding abc.
	$this->assertEquals($this->redis->lgetRange('x', 0, -1), array('def'));	// only def remains in x.
	$this->assertEquals($this->redis->lgetRange('y', 0, -1), array('abc', '456', '123'));	// abc has been lpushed to y.

	// with an empty source, expecting no change.
	$this->redis->delete('x', 'y');
	$this->assertTrue(FALSE === $this->redis->brpoplpush('x', 'y', 1));
	$this->assertTrue(array() === $this->redis->lgetRange('x', 0, -1));
	$this->assertTrue(array() === $this->redis->lgetRange('y', 0, -1));

    }

	public function testZAddFirstArg() {

		$this->redis->delete('key');

		$zsetName = 100; // not a string!
		$this->assertTrue(1 === $this->redis->zAdd($zsetName, 0, 'val0'));
		$this->assertTrue(1 === $this->redis->zAdd($zsetName, 1, 'val1'));

		$this->assertTrue(array('val0', 'val1') === $this->redis->zRange($zsetName, 0, -1));
	}

    public function testZX() {

	$this->redis->delete('key');

	$this->assertTrue(array() === $this->redis->zRange('key', 0, -1));
	$this->assertTrue(array() === $this->redis->zRange('key', 0, -1, true));

	$this->assertTrue(1 === $this->redis->zAdd('key', 0, 'val0'));
	$this->assertTrue(1 === $this->redis->zAdd('key', 2, 'val2'));
	$this->assertTrue(1 === $this->redis->zAdd('key', 1, 'val1'));
	$this->assertTrue(1 === $this->redis->zAdd('key', 3, 'val3'));
	$this->assertTrue(2 === $this->redis->zAdd('key', 4, 'val4', 5, 'val5')); // multiple parameters

	$this->assertTrue(array('val0', 'val1', 'val2', 'val3', 'val4', 'val5') === $this->redis->zRange('key', 0, -1));

	// withscores
	$ret = $this->redis->zRange('key', 0, -1, true);
	$this->assertTrue(count($ret) == 6);
	$this->assertTrue($ret['val0'] == 0);
	$this->assertTrue($ret['val1'] == 1);
	$this->assertTrue($ret['val2'] == 2);
	$this->assertTrue($ret['val3'] == 3);
	$this->assertTrue($ret['val4'] == 4);
	$this->assertTrue($ret['val5'] == 5);

	$this->assertTrue(0 === $this->redis->zDelete('key', 'valX'));
	$this->assertTrue(1 === $this->redis->zDelete('key', 'val3'));
	$this->assertTrue(1 === $this->redis->zDelete('key', 'val4'));
	$this->assertTrue(1 === $this->redis->zDelete('key', 'val5'));

	$this->assertTrue(array('val0', 'val1', 'val2') === $this->redis->zRange('key', 0, -1));

	// zGetReverseRange

	$this->assertTrue(1 === $this->redis->zAdd('key', 3, 'val3'));
	$this->assertTrue(1 === $this->redis->zAdd('key', 3, 'aal3'));

	$zero_to_three = $this->redis->zRangeByScore('key', 0, 3);
	$this->assertTrue(array('val0', 'val1', 'val2', 'aal3', 'val3') === $zero_to_three || array('val0', 'val1', 'val2', 'val3', 'aal3') === $zero_to_three);

	$three_to_zero = $this->redis->zRevRangeByScore('key', 3, 0);
	$this->assertTrue(array_reverse(array('val0', 'val1', 'val2', 'aal3', 'val3')) === $three_to_zero || array_reverse(array('val0', 'val1', 'val2', 'val3', 'aal3')) === $three_to_zero);

	$this->assertTrue(5 === $this->redis->zCount('key', 0, 3));

	// withscores
	$this->redis->zRemove('key', 'aal3');
	$zero_to_three = $this->redis->zRangeByScore('key', 0, 3, array('withscores' => TRUE));
	$this->assertTrue(array('val0' => 0, 'val1' => 1, 'val2' => 2, 'val3' => 3) == $zero_to_three);
	$this->assertTrue(4 === $this->redis->zCount('key', 0, 3));

	// limit
	$this->assertTrue(array('val0') === $this->redis->zRangeByScore('key', 0, 3, array('limit' => array(0, 1))));
	$this->assertTrue(array('val0', 'val1') === $this->redis->zRangeByScore('key', 0, 3, array('limit' => array(0, 2))));
	$this->assertTrue(array('val1', 'val2') === $this->redis->zRangeByScore('key', 0, 3, array('limit' => array(1, 2))));
	$this->assertTrue(array('val0', 'val1') === $this->redis->zRangeByScore('key', 0, 1, array('limit' => array(0, 100))));

	$this->assertTrue(array('val3') === $this->redis->zRevRangeByScore('key', 3, 0, array('limit' => array(0, 1))));
	$this->assertTrue(array('val3', 'val2') === $this->redis->zRevRangeByScore('key', 3, 0, array('limit' => array(0, 2))));
	$this->assertTrue(array('val2', 'val1') === $this->redis->zRevRangeByScore('key', 3, 0, array('limit' => array(1, 2))));
	$this->assertTrue(array('val1', 'val0') === $this->redis->zRevRangeByScore('key', 1, 0, array('limit' => array(0, 100))));

	$this->assertTrue(4 === $this->redis->zSize('key'));
	$this->assertTrue(1.0 === $this->redis->zScore('key', 'val1'));
	$this->assertFalse($this->redis->zScore('key', 'val'));
	$this->assertFalse($this->redis->zScore(3, 2));

	// with () and +inf, -inf
	$this->redis->delete('zset');
	$this->redis->zAdd('zset', 1, 'foo');
	$this->redis->zAdd('zset', 2, 'bar');
	$this->redis->zAdd('zset', 3, 'biz');
	$this->redis->zAdd('zset', 4, 'foz');
	$this->assertTrue(array('foo' => 1, 'bar' => 2, 'biz' => 3, 'foz' => 4) == $this->redis->zRangeByScore('zset', '-inf', '+inf', array('withscores' => TRUE)));
	$this->assertTrue(array('foo' => 1, 'bar' => 2) == $this->redis->zRangeByScore('zset', 1, 2, array('withscores' => TRUE)));
	$this->assertTrue(array('bar' => 2) == $this->redis->zRangeByScore('zset', '(1', 2, array('withscores' => TRUE)));
	$this->assertTrue(array() == $this->redis->zRangeByScore('zset', '(1', '(2', array('withscores' => TRUE)));

	$this->assertTrue(4 == $this->redis->zCount('zset', '-inf', '+inf'));
	$this->assertTrue(2 == $this->redis->zCount('zset', 1, 2));
	$this->assertTrue(1 == $this->redis->zCount('zset', '(1', 2));
	$this->assertTrue(0 == $this->redis->zCount('zset', '(1', '(2'));


	// zincrby
	$this->redis->delete('key');
	$this->assertTrue(1.0 === $this->redis->zIncrBy('key', 1, 'val1'));
	$this->assertTrue(1.0 === $this->redis->zScore('key', 'val1'));
	$this->assertTrue(2.5 === $this->redis->zIncrBy('key', 1.5, 'val1'));
	$this->assertTrue(2.5 === $this->redis->zScore('key', 'val1'));

	//zUnion
	$this->redis->delete('key1');
	$this->redis->delete('key2');
	$this->redis->delete('key3');
	$this->redis->delete('keyU');

	$this->redis->zAdd('key1', 0, 'val0');
	$this->redis->zAdd('key1', 1, 'val1');

	$this->redis->zAdd('key2', 2, 'val2');
	$this->redis->zAdd('key2', 3, 'val3');

	$this->redis->zAdd('key3', 4, 'val4');
	$this->redis->zAdd('key3', 5, 'val5');

	$this->assertTrue(4 === $this->redis->zUnion('keyU', array('key1', 'key3')));
	$this->assertTrue(array('val0', 'val1', 'val4', 'val5') === $this->redis->zRange('keyU', 0, -1));

	// Union on non existing keys
	$this->redis->delete('keyU');
	$this->assertTrue(0 === $this->redis->zUnion('keyU', array('X', 'Y')));
	$this->assertTrue(array() === $this->redis->zRange('keyU', 0, -1));

	// !Exist U Exist → copy of existing zset.
	$this->redis->delete('keyU', 'X');
	$this->assertTrue(2 === $this->redis->zUnion('keyU', array('key1', 'X')));

	// test weighted zUnion
	$this->redis->delete('keyZ');
	$this->assertTrue(4 === $this->redis->zUnion('keyZ', array('key1', 'key2'), array(1, 1)));
	$this->assertTrue(array('val0', 'val1', 'val2', 'val3') === $this->redis->zRange('keyZ', 0, -1));

	$this->redis->zDeleteRangeByScore('keyZ', 0, 10);
	$this->assertTrue(4 === $this->redis->zUnion('keyZ', array('key1', 'key2'), array(5, 1)));
	$this->assertTrue(array('val0', 'val2', 'val3', 'val1') === $this->redis->zRange('keyZ', 0, -1));

	$this->redis->delete('key1');
	$this->redis->delete('key2');
	$this->redis->delete('key3');

	//test zUnion with weights and aggegration function
	$this->redis->zadd('key1', 1, 'duplicate');
	$this->redis->zadd('key2', 2, 'duplicate');
	$this->redis->zUnion('keyU', array('key1','key2'), array(1,1), 'MIN');
	$this->assertTrue($this->redis->zScore('keyU', 'duplicate')===1.0);
	$this->redis->delete('keyU');

	//now test zUnion *without* weights but with aggregrate function
	$this->redis->zUnion('keyU', array('key1','key2'), null, 'MIN');
	$this->assertTrue($this->redis->zScore('keyU', 'duplicate')===1.0);
	$this->redis->delete('keyU', 'key1', 'key2');



	// test integer and float weights (GitHub issue #109).
	$this->redis->del('key1', 'key2', 'key3');

	$this->redis->zadd('key1', 1, 'one');
	$this->redis->zadd('key1', 2, 'two');
	$this->redis->zadd('key2', 1, 'one');
	$this->redis->zadd('key2', 2, 'two');
	$this->redis->zadd('key2', 3, 'three');

	$this->assertTrue($this->redis->zunion('key3', array('key1', 'key2'), array(2, 3.0)) === 3);

	$this->redis->delete('key1');
	$this->redis->delete('key2');
	$this->redis->delete('key3');

    // Test 'inf', '-inf', and '+inf' weights (GitHub issue #336)
    $this->redis->zadd('key1', 1, 'one', 2, 'two', 3, 'three');
    $this->redis->zadd('key2', 3, 'three', 4, 'four', 5, 'five');

    // Make sure phpredis handles these weights
    $this->assertTrue($this->redis->zunion('key3', array('key1','key2'), array(1, 'inf'))  === 5);
    $this->assertTrue($this->redis->zunion('key3', array('key1','key2'), array(1, '-inf')) === 5);
    $this->assertTrue($this->redis->zunion('key3', array('key1','key2'), array(1, '+inf')) === 5);

    // Now, confirm that they're being sent, and that it works
    $arr_weights = Array('inf','-inf','+inf');

    foreach($arr_weights as $str_weight) {
        $r = $this->redis->zunionstore('key3', array('key1','key2'), array(1,$str_weight));
        $this->assertTrue($r===5);
        $r = $this->redis->zrangebyscore('key3', '(-inf', '(inf',array('withscores'=>true));
        $this->assertTrue(count($r)===2);
        $this->assertTrue(isset($r['one']));
        $this->assertTrue(isset($r['two']));
    }

    $this->redis->del('key1','key2','key3');

	$this->redis->zadd('key1', 2000.1, 'one');
	$this->redis->zadd('key1', 3000.1, 'two');
	$this->redis->zadd('key1', 4000.1, 'three');

	$ret = $this->redis->zRange('key1', 0, -1, TRUE);
	$this->assertTrue(count($ret) === 3);
	$retValues = array_keys($ret);

	$this->assertTrue(array('one', 'two', 'three') === $retValues);

	// + 0 converts from string to float OR integer
	$this->assertTrue(is_float($ret['one'] + 0));
	$this->assertTrue(is_float($ret['two'] + 0));
	$this->assertTrue(is_float($ret['three'] + 0));

	$this->redis->delete('key1');

	// ZREMRANGEBYRANK
	$this->redis->zAdd('key1', 1, 'one');
	$this->redis->zAdd('key1', 2, 'two');
	$this->redis->zAdd('key1', 3, 'three');
	$this->assertTrue(2 === $this->redis->zremrangebyrank('key1', 0, 1));
	$this->assertTrue(array('three' => 3) == $this->redis->zRange('key1', 0, -1, TRUE));

	$this->redis->delete('key1');

	// zInter

	$this->redis->zAdd('key1', 0, 'val0');
	$this->redis->zAdd('key1', 1, 'val1');
	$this->redis->zAdd('key1', 3, 'val3');

	$this->redis->zAdd('key2', 2, 'val1');
	$this->redis->zAdd('key2', 3, 'val3');

	$this->redis->zAdd('key3', 4, 'val3');
	$this->redis->zAdd('key3', 5, 'val5');

	$this->redis->delete('keyI');
	$this->assertTrue(2 === $this->redis->zInter('keyI', array('key1', 'key2')));
	$this->assertTrue(array('val1', 'val3') === $this->redis->zRange('keyI', 0, -1));

	// Union on non existing keys
	$this->assertTrue(0 === $this->redis->zInter('keyX', array('X', 'Y')));
	$this->assertTrue(array() === $this->redis->zRange('keyX', 0, -1));

	// !Exist U Exist
	$this->assertTrue(0 === $this->redis->zInter('keyY', array('key1', 'X')));
	$this->assertTrue(array() === $this->redis->zRange('keyY', 0, -1));


	// test weighted zInter
	$this->redis->delete('key1');
	$this->redis->delete('key2');
	$this->redis->delete('key3');

	$this->redis->zAdd('key1', 0, 'val0');
	$this->redis->zAdd('key1', 1, 'val1');
	$this->redis->zAdd('key1', 3, 'val3');


	$this->redis->zAdd('key2', 2, 'val1');
	$this->redis->zAdd('key2', 1, 'val3');

	$this->redis->zAdd('key3', 7, 'val1');
	$this->redis->zAdd('key3', 3, 'val3');

	$this->redis->delete('keyI');
	$this->assertTrue(2 === $this->redis->zInter('keyI', array('key1', 'key2'), array(1, 1)));
	$this->assertTrue(array('val1', 'val3') === $this->redis->zRange('keyI', 0, -1));

	$this->redis->delete('keyI');
	$this->assertTrue( 2 === $this->redis->zInter('keyI', array('key1', 'key2', 'key3'), array(1, 5, 1), 'min'));
	$this->assertTrue(array('val1', 'val3') === $this->redis->zRange('keyI', 0, -1));
	$this->redis->delete('keyI');
	$this->assertTrue( 2 === $this->redis->zInter('keyI', array('key1', 'key2', 'key3'), array(1, 5, 1), 'max'));
	$this->assertTrue(array('val3', 'val1') === $this->redis->zRange('keyI', 0, -1));

	$this->redis->delete('keyI');
	$this->assertTrue(2 === $this->redis->zInter('keyI', array('key1', 'key2', 'key3'), null, 'max'));
	$this->assertTrue($this->redis->zScore('keyI', 'val1') === floatval(7));

	// zrank, zrevrank
	$this->redis->delete('z');
	$this->redis->zadd('z', 1, 'one');
	$this->redis->zadd('z', 2, 'two');
	$this->redis->zadd('z', 5, 'five');

	$this->assertTrue(0 === $this->redis->zRank('z', 'one'));
	$this->assertTrue(1 === $this->redis->zRank('z', 'two'));
	$this->assertTrue(2 === $this->redis->zRank('z', 'five'));

	$this->assertTrue(2 === $this->redis->zRevRank('z', 'one'));
	$this->assertTrue(1 === $this->redis->zRevRank('z', 'two'));
	$this->assertTrue(0 === $this->redis->zRevRank('z', 'five'));

    }

    public function testHashes() {
	$this->redis->delete('h', 'key');

	$this->assertTrue(0 === $this->redis->hLen('h'));
	$this->assertTrue(1 === $this->redis->hSet('h', 'a', 'a-value'));
	$this->assertTrue(1 === $this->redis->hLen('h'));
	$this->assertTrue(1 === $this->redis->hSet('h', 'b', 'b-value'));
	$this->assertTrue(2 === $this->redis->hLen('h'));

	$this->assertTrue('a-value' === $this->redis->hGet('h', 'a')); 	// simple get
	$this->assertTrue('b-value' === $this->redis->hGet('h', 'b')); 	// simple get

	$this->assertTrue(0 === $this->redis->hSet('h', 'a', 'another-value')); // replacement
	$this->assertTrue('another-value' === $this->redis->hGet('h', 'a')); 	// get the new value

	$this->assertTrue('b-value' === $this->redis->hGet('h', 'b')); 	// simple get
	$this->assertTrue(FALSE === $this->redis->hGet('h', 'c'));	// unknown hash member
	$this->assertTrue(FALSE === $this->redis->hGet('key', 'c'));	// unknownkey

	// hDel
	$this->assertTrue(1 === $this->redis->hDel('h', 'a')); // 1 on success
	$this->assertTrue(0 === $this->redis->hDel('h', 'a')); // 0 on failure

	$this->redis->delete('h');
	$this->redis->hSet('h', 'x', 'a');
	$this->redis->hSet('h', 'y', 'b');
	$this->assertTrue(2 === $this->redis->hDel('h', 'x', 'y')); // variadic

	// hsetnx
	$this->redis->delete('h');
	$this->assertTrue(TRUE === $this->redis->hSetNx('h', 'x', 'a'));
	$this->assertTrue(TRUE === $this->redis->hSetNx('h', 'y', 'b'));
	$this->assertTrue(FALSE === $this->redis->hSetNx('h', 'x', '?'));
	$this->assertTrue(FALSE === $this->redis->hSetNx('h', 'y', '?'));
	$this->assertTrue('a' === $this->redis->hGet('h', 'x'));
	$this->assertTrue('b' === $this->redis->hGet('h', 'y'));

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

	// hIncrBy
	$this->redis->delete('h');
	$this->assertTrue(2 === $this->redis->hIncrBy('h', 'x', 2));
	$this->assertTrue(3 === $this->redis->hIncrBy('h', 'x', 1));
	$this->assertTrue(2 === $this->redis->hIncrBy('h', 'x', -1));
	$this->assertTrue(FALSE === $this->redis->hIncrBy('h', 'x', "not-a-number"));
	$this->assertTrue("2" === $this->redis->hGet('h', 'x'));

	$this->redis->hSet('h', 'y', 'not-a-number');
	$this->assertTrue(FALSE === $this->redis->hIncrBy('h', 'y', 1));

	if (version_compare($this->version, "2.5.0", "ge")) {
	// hIncrByFloat
	$this->redis->delete('h');
	$this->assertTrue(1.5 === $this->redis->hIncrByFloat('h','x', 1.5));
	$this->assertTrue(3.0 === $this->redis->hincrByFloat('h','x', 1.5));
	$this->assertTrue(1.5 === $this->redis->hincrByFloat('h','x', -1.5));

	$this->redis->hset('h','y','not-a-number');
	$this->assertTrue(FALSE === $this->redis->hIncrByFloat('h', 'y', 1.5));
	}

	// hmset
	$this->redis->delete('h');
	$this->assertTrue(TRUE === $this->redis->hMset('h', array('x' => 123, 'y' => 456, 'z' => 'abc')));
	$this->assertTrue('123' === $this->redis->hGet('h', 'x'));
	$this->assertTrue('456' === $this->redis->hGet('h', 'y'));
	$this->assertTrue('abc' === $this->redis->hGet('h', 'z'));
	$this->assertTrue(FALSE === $this->redis->hGet('h', 't'));

	// hmget
	$this->assertTrue(array('x' => '123', 'y' => '456') === $this->redis->hMget('h', array('x', 'y')));
	$this->assertTrue(array('z' => 'abc') === $this->redis->hMget('h', array('z')));
	$this->assertTrue(array('x' => '123', 't' => FALSE, 'y' => '456') === $this->redis->hMget('h', array('x', 't', 'y')));
	$this->assertFalse(array(123 => 'x') === $this->redis->hMget('h', array(123)));
	$this->assertTrue(array(123 => FALSE) === $this->redis->hMget('h', array(123)));

    // Test with an array populated with things we can't use as keys
    $this->assertTrue($this->redis->hmget('h', Array(false,NULL,false)) === FALSE);

    // Test with some invalid keys mixed in (which should just be ignored)
    $this->assertTrue(array('x'=>'123','y'=>'456','z'=>'abc') === $this->redis->hMget('h',Array('x',null,'y','','z',false)));

	// hmget/hmset with numeric fields
	$this->redis->del('h');
	$this->assertTrue(TRUE === $this->redis->hMset('h', array(123 => 'x', 'y' => 456)));
	$this->assertTrue('x' === $this->redis->hGet('h', 123));
	$this->assertTrue('x' === $this->redis->hGet('h', '123'));
	$this->assertTrue('456' === $this->redis->hGet('h', 'y'));
	$this->assertTrue(array(123 => 'x', 'y' => '456') === $this->redis->hMget('h', array('123', 'y')));

	// check non-string types.
	$this->redis->delete('h1');
	$this->assertTrue(TRUE === $this->redis->hMSet('h1', array('x' => 0, 'y' => array(), 'z' => new stdclass(), 't' => NULL)));
	$h1 = $this->redis->hGetAll('h1');
	$this->assertTrue('0' === $h1['x']);
	$this->assertTrue('Array' === $h1['y']);
	$this->assertTrue('Object' === $h1['z']);
	$this->assertTrue('' === $h1['t']);

    }

    public function testSetRange() {

	    $this->redis->delete('key');
	    $this->redis->set('key', 'hello world');
	    $this->redis->setRange('key', 6, 'redis');
	    $this->assertTrue('hello redis' === $this->redis->get('key'));
	    $this->redis->setRange('key', 6, 'you'); // don't cut off the end
	    $this->assertTrue('hello youis' === $this->redis->get('key'));

	    $this->redis->set('key', 'hello world');
	    // $this->assertTrue(11 === $this->redis->setRange('key', -6, 'redis')); // works with negative offsets too! (disabled because not all versions support this)
	    // $this->assertTrue('hello redis' === $this->redis->get('key'));

	    // fill with zeros if needed
	    $this->redis->delete('key');
	    $this->redis->setRange('key', 6, 'foo');
	    $this->assertTrue("\x00\x00\x00\x00\x00\x00foo" === $this->redis->get('key'));
    }

    public function testObject() {
	    $this->redis->del('key');
	    $this->assertTrue($this->redis->object('encoding', 'key') === FALSE);
	    $this->assertTrue($this->redis->object('refcount', 'key') === FALSE);
	    $this->assertTrue($this->redis->object('idletime', 'key') === FALSE);

	    $this->redis->set('key', 'value');
	    $this->assertTrue($this->redis->object('encoding', 'key') === "raw");
	    $this->assertTrue($this->redis->object('refcount', 'key') === 1);
	    $this->assertTrue($this->redis->object('idletime', 'key') === 0);

	    $this->redis->del('key');
	    $this->redis->lpush('key', 'value');
	    $this->assertTrue($this->redis->object('encoding', 'key') === "ziplist");
	    $this->assertTrue($this->redis->object('refcount', 'key') === 1);
	    $this->assertTrue($this->redis->object('idletime', 'key') === 0);

	    $this->redis->del('key');
	    $this->redis->sadd('key', 'value');
	    $this->assertTrue($this->redis->object('encoding', 'key') === "hashtable");
	    $this->assertTrue($this->redis->object('refcount', 'key') === 1);
	    $this->assertTrue($this->redis->object('idletime', 'key') === 0);

	    $this->redis->del('key');
	    $this->redis->sadd('key', 42);
	    $this->redis->sadd('key', 1729);
	    $this->assertTrue($this->redis->object('encoding', 'key') === "intset");
	    $this->assertTrue($this->redis->object('refcount', 'key') === 1);
	    $this->assertTrue($this->redis->object('idletime', 'key') === 0);

	    $this->redis->del('key');
	    $this->redis->lpush('key', str_repeat('A', pow(10,6))); // 1M elements, too big for a ziplist.
	    $this->assertTrue($this->redis->object('encoding', 'key') === "linkedlist");
	    $this->assertTrue($this->redis->object('refcount', 'key') === 1);
	    $this->assertTrue($this->redis->object('idletime', 'key') === 0);
    }

    public function testMultiExec() {
	$this->sequence(Redis::MULTI);
	$this->differentType(Redis::MULTI);

	// with prefix as well
	$this->redis->setOption(Redis::OPT_PREFIX, "test:");
	$this->sequence(Redis::MULTI);
	$this->differentType(Redis::MULTI);
	$this->redis->setOption(Redis::OPT_PREFIX, "");

	$this->redis->set('x', '42');

	$this->assertTrue(TRUE === $this->redis->watch('x'));
	$ret = $this->redis->multi()
		->get('x')
		->exec();

	// successful transaction
	$this->assertTrue($ret === array('42'));

	// failed transaction
	$this->redis->watch('x');

	$r = $this->newInstance(); // new instance, modifying `x'.
	$r->incr('x');

	$ret = $this->redis->multi()
		->get('x')
		->exec();
	$this->assertTrue($ret === FALSE); // failed because another client changed our watched key between WATCH and EXEC.

	// watch and unwatch
	$this->redis->watch('x');
	$r->incr('x'); // other instance
	$this->redis->unwatch(); // cancel transaction watch

	$ret = $this->redis->multi()
		->get('x')
		->exec();
	$this->assertTrue($ret === array('44')); // succeeded since we've cancel the WATCH command.
    }

    public function testPipeline() {
	$this->sequence(Redis::PIPELINE);
	$this->differentType(Redis::PIPELINE);

	// with prefix as well
	$this->redis->setOption(Redis::OPT_PREFIX, "test:");
	$this->sequence(Redis::PIPELINE);
	$this->differentType(Redis::PIPELINE);
	$this->redis->setOption(Redis::OPT_PREFIX, "");
    }

    protected function sequence($mode) {

	    $ret = $this->redis->multi($mode)
		    ->set('x', 42)
		    ->info()
		    ->type('x')
		    ->get('x')
		    ->exec();

	    $this->assertTrue(is_array($ret));
	    $i = 0;
	    $this->assertTrue($ret[$i++] == TRUE);
	    $this->assertTrue(is_array($ret[$i++]));
	    $this->assertTrue($ret[$i++] === Redis::REDIS_STRING);
	    $this->assertTrue($ret[$i] === '42' || $ret[$i] === 42);

	    $serializer = $this->redis->getOption(Redis::OPT_SERIALIZER);
	    $this->redis->setOption(Redis::OPT_SERIALIZER, Redis::SERIALIZER_NONE); // testing incr, which doesn't work with the serializer
	    $ret = $this->redis->multi($mode)
		    ->delete('key1')
		    ->set('key1', 'value1')
		    ->get('key1')
		    ->getSet('key1', 'value2')
		    ->get('key1')
		    ->set('key2', 4)
		    ->incr('key2')
		    ->get('key2')
		    ->decr('key2')
		    ->get('key2')
		    ->renameKey('key2', 'key3')
		    ->get('key3')
		    ->renameNx('key3', 'key1')
		    ->renameKey('key3', 'key2')
		    ->incr('key2', 5)
		    ->get('key2')
		    ->decr('key2', 5)
		    ->get('key2')
		    ->exec();

	    $this->assertTrue(is_array($ret));
	    $i = 0;
	    $this->assertTrue(is_long($ret[$i++]));
	    $this->assertTrue($ret[$i++] == TRUE);
	    $this->assertTrue($ret[$i++] == 'value1');
	    $this->assertTrue($ret[$i++] == 'value1');
	    $this->assertTrue($ret[$i++] == 'value2');
	    $this->assertTrue($ret[$i++] == TRUE);
	    $this->assertTrue($ret[$i++] == 5);
	    $this->assertTrue($ret[$i++] == 5);
	    $this->assertTrue($ret[$i++] == 4);
	    $this->assertTrue($ret[$i++] == 4);
	    $this->assertTrue($ret[$i++] == TRUE);
	    $this->assertTrue($ret[$i++] == 4);
	    $this->assertTrue($ret[$i++] == FALSE);
	    $this->assertTrue($ret[$i++] == TRUE);
	    $this->assertTrue($ret[$i++] == TRUE);
	    $this->assertTrue($ret[$i++] == 9);
	    $this->assertTrue($ret[$i++] == TRUE);
	    $this->assertTrue($ret[$i++] == 4);
	    $this->assertTrue(count($ret) == $i);

	    $this->redis->setOption(Redis::OPT_SERIALIZER, $serializer);

	    $ret = $this->redis->multi($mode)
		    ->delete('key1')
		    ->delete('key2')
		    ->set('key1', 'val1')
		    ->setnx('key1', 'valX')
		    ->setnx('key2', 'valX')
		    ->exists('key1')
		    ->exists('key3')
		    ->ping()
		    ->exec();

	    $this->assertTrue(is_array($ret));
	    $this->assertTrue($ret[0] == TRUE);
	    $this->assertTrue($ret[1] == TRUE);
	    $this->assertTrue($ret[2] == TRUE);
	    $this->assertTrue($ret[3] == FALSE);
	    $this->assertTrue($ret[4] == TRUE);
	    $this->assertTrue($ret[5] == TRUE);
	    $this->assertTrue($ret[6] == FALSE);
	    $this->assertTrue($ret[7] == '+PONG');

	    $ret = $this->redis->multi($mode)
		    ->randomKey()
		    ->exec();
	    $ret = $this->redis->multi($mode)
		    ->exec();
	    $this->assertTrue($ret == array());

	    // ttl, mget, mset, msetnx, expire, expireAt
	    $this->redis->delete('key');
	    $ret = $this->redis->multi($mode)
		    ->ttl('key')
		    ->mget(array('key1', 'key2', 'key3'))
		    ->mset(array('key3' => 'value3', 'key4' => 'value4'))
		    ->set('key', 'value')
		    ->expire('key', 5)
		    ->ttl('key')
		    ->expireAt('key', '0000')
		    ->exec();
	    $this->assertTrue(is_array($ret));
		$i = 0;
		$ttl = $ret[$i++];
	    $this->assertTrue($ttl === -1 || $ttl === -2);
	    $this->assertTrue($ret[$i++] === array('val1', 'valX', FALSE)); // mget
	    $this->assertTrue($ret[$i++] === TRUE); // mset
	    $this->assertTrue($ret[$i++] === TRUE); // set
	    $this->assertTrue($ret[$i++] === TRUE); // expire
	    $this->assertTrue($ret[$i++] === 5);    // ttl
	    $this->assertTrue($ret[$i++] === TRUE); // expireAt
	    $this->assertTrue(count($ret) == $i);

	    $ret = $this->redis->multi($mode)
		    ->set('lkey', 'x')
		    ->set('lDest', 'y')
		    ->delete('lkey', 'lDest')
		    ->rpush('lkey', 'lvalue')
		    ->lpush('lkey', 'lvalue')
		    ->lpush('lkey', 'lvalue')
		    ->lpush('lkey', 'lvalue')
		    ->lpush('lkey', 'lvalue')
		    ->lpush('lkey', 'lvalue')
		    ->rpoplpush('lkey', 'lDest')
		    ->lGetRange('lDest', 0, -1)
		    ->lpop('lkey')
		    ->llen('lkey')
		    ->lRemove('lkey', 'lvalue', 3)
		    ->llen('lkey')
		    ->lget('lkey', 0)
		    ->lGetRange('lkey', 0, -1)
		    ->lSet('lkey', 1, "newValue")	 // check errors on key not exists
		    ->lGetRange('lkey', 0, -1)
		    ->llen('lkey')
		    ->exec();

	    $this->assertTrue(is_array($ret));
	    $i = 0;
	    $this->assertTrue($ret[$i++] === TRUE); // SET
	    $this->assertTrue($ret[$i++] === TRUE); // SET
	    $this->assertTrue($ret[$i++] === 2); // deleting 2 keys
	    $this->assertTrue($ret[$i++] === 1); // rpush, now 1 element
	    $this->assertTrue($ret[$i++] === 2); // lpush, now 2 elements
	    $this->assertTrue($ret[$i++] === 3); // lpush, now 3 elements
	    $this->assertTrue($ret[$i++] === 4); // lpush, now 4 elements
	    $this->assertTrue($ret[$i++] === 5); // lpush, now 5 elements
	    $this->assertTrue($ret[$i++] === 6); // lpush, now 6 elements
	    $this->assertTrue($ret[$i++] === 'lvalue'); // rpoplpush returns the element: "lvalue"
	    $this->assertTrue($ret[$i++] === array('lvalue')); // lDest contains only that one element.
	    $this->assertTrue($ret[$i++] === 'lvalue'); // removing a second element from lkey, now 4 elements left ↓
	    $this->assertTrue($ret[$i++] === 4); // 4 elements left, after 2 pops.
	    $this->assertTrue($ret[$i++] === 3); // removing 3 elements, now 1 left.
	    $this->assertTrue($ret[$i++] === 1); // 1 element left
	    $this->assertTrue($ret[$i++] === "lvalue"); // this is the current head.
	    $this->assertTrue($ret[$i++] === array("lvalue")); // this is the current list.
	    $this->assertTrue($ret[$i++] === FALSE); // updating a non-existent element fails.
	    $this->assertTrue($ret[$i++] === array("lvalue")); // this is the current list.
	    $this->assertTrue($ret[$i++] === 1); // 1 element left
	    $this->assertTrue(count($ret) == $i);


	    $ret = $this->redis->multi(Redis::PIPELINE)
		    ->delete('lkey', 'lDest')
		    ->rpush('lkey', 'lvalue')
		    ->lpush('lkey', 'lvalue')
		    ->lpush('lkey', 'lvalue')
		    ->rpoplpush('lkey', 'lDest')
		    ->lGetRange('lDest', 0, -1)
		    ->lpop('lkey')
		    ->exec();
	    $this->assertTrue(is_array($ret));
	    $i = 0;
	    $this->assertTrue($ret[$i++] <= 2); // deleted 0, 1, or 2 items
	    $this->assertTrue($ret[$i++] === 1); // 1 element in the list
	    $this->assertTrue($ret[$i++] === 2); // 2 elements in the list
	    $this->assertTrue($ret[$i++] === 3); // 3 elements in the list
	    $this->assertTrue($ret[$i++] === 'lvalue'); // rpoplpush returns the element: "lvalue"
	    $this->assertTrue($ret[$i++] === array('lvalue')); // rpoplpush returns the element: "lvalue"
	    $this->assertTrue($ret[$i++] === 'lvalue'); // pop returns the front element: "lvalue"
	    $this->assertTrue(count($ret) == $i);


	    // general command
	    $ret = $this->redis->multi($mode)
		    ->select(3)
		    ->set('keyAAA', 'value')
		    ->set('keyAAB', 'value')
		    ->dbSize()
		    ->lastsave()
		    ->exec();

	    $this->redis->select(0); // back to normal

	    $i = 0;
	    $this->assertTrue(is_array($ret));
	    $this->assertTrue($ret[$i++] === TRUE); // select
	    $this->assertTrue($ret[$i++] === TRUE); // set
	    $this->assertTrue($ret[$i++] === TRUE); // set
	    $this->assertTrue(is_long($ret[$i++])); // dbsize
	    $this->assertTrue(is_long($ret[$i++])); // lastsave

	    $this->assertTrue(count($ret) === $i);

	    $serializer = $this->redis->getOption(Redis::OPT_SERIALIZER);
	    $this->redis->setOption(Redis::OPT_SERIALIZER, Redis::SERIALIZER_NONE); // testing incr, which doesn't work with the serializer
	    $ret = $this->redis->multi($mode)
		    ->delete('key1')
		    ->set('key1', 'value1')
		    ->get('key1')
		    ->getSet('key1', 'value2')
		    ->get('key1')
		    ->set('key2', 4)
		    ->incr('key2')
		    ->get('key2')
		    ->decr('key2')
		    ->get('key2')
		    ->renameKey('key2', 'key3')
		    ->get('key3')
		    ->renameNx('key3', 'key1')
		    ->renameKey('key3', 'key2')
		    ->incr('key2', 5)
		    ->get('key2')
		    ->decr('key2', 5)
		    ->get('key2')
		    ->exec();

	    $i = 0;
	    $this->assertTrue(is_array($ret));
	    $this->assertTrue(is_long($ret[$i]) && $ret[$i] <= 1); $i++;
	    $this->assertTrue($ret[$i++] == TRUE);
	    $this->assertTrue($ret[$i++] == 'value1');
	    $this->assertTrue($ret[$i++] == 'value1');
	    $this->assertTrue($ret[$i++] == 'value2');
	    $this->assertTrue($ret[$i++] == TRUE);
	    $this->assertTrue($ret[$i++] == 5);
	    $this->assertTrue($ret[$i++] == 5);
	    $this->assertTrue($ret[$i++] == 4);
	    $this->assertTrue($ret[$i++] == 4);
	    $this->assertTrue($ret[$i++] == TRUE);
	    $this->assertTrue($ret[$i++] == 4);
	    $this->assertTrue($ret[$i++] == FALSE);
	    $this->assertTrue($ret[$i++] == TRUE);
	    $this->assertTrue($ret[$i++] == TRUE);
	    $this->assertTrue($ret[$i++] == 9);
	    $this->assertTrue($ret[$i++] == TRUE);
	    $this->assertTrue($ret[$i++] == 4);
	    $this->redis->setOption(Redis::OPT_SERIALIZER, $serializer);

	    $ret = $this->redis->multi($mode)
		    ->delete('key1')
		    ->delete('key2')
		    ->set('key1', 'val1')
		    ->setnx('key1', 'valX')
		    ->setnx('key2', 'valX')
		    ->exists('key1')
		    ->exists('key3')
		    ->ping()
		    ->exec();

	    $this->assertTrue(is_array($ret));
	    $this->assertTrue($ret[0] == TRUE);
	    $this->assertTrue($ret[1] == TRUE);
	    $this->assertTrue($ret[2] == TRUE);
	    $this->assertTrue($ret[3] == FALSE);
	    $this->assertTrue($ret[4] == TRUE);
	    $this->assertTrue($ret[5] == TRUE);
	    $this->assertTrue($ret[6] == FALSE);
	    $this->assertTrue($ret[7] == '+PONG');

	    $ret = $this->redis->multi($mode)
		    ->randomKey()
		    ->exec();

	    $this->assertTrue(is_array($ret) && count($ret) === 1);
	    $this->assertTrue(is_string($ret[0]));

	    // ttl, mget, mset, msetnx, expire, expireAt
	    $ret = $this->redis->multi($mode)
		    ->ttl('key')
		    ->mget(array('key1', 'key2', 'key3'))
		    ->mset(array('key3' => 'value3', 'key4' => 'value4'))
		    ->set('key', 'value')
		    ->expire('key', 5)
		    ->ttl('key')
		    ->expireAt('key', '0000')
		    ->exec();
	    $i = 0;
	    $this->assertTrue(is_array($ret));
	    $this->assertTrue(is_long($ret[$i++]));
	    $this->assertTrue(is_array($ret[$i]) && count($ret[$i]) === 3); // mget
	    $i++;
	    $this->assertTrue($ret[$i++] === TRUE); // mset always returns TRUE
	    $this->assertTrue($ret[$i++] === TRUE); // set always returns TRUE
	    $this->assertTrue($ret[$i++] === TRUE); // expire always returns TRUE
	    $this->assertTrue($ret[$i++] === 5); // TTL was just set.
	    $this->assertTrue($ret[$i++] === TRUE); // expireAt returns TRUE for an existing key
	    $this->assertTrue(count($ret) === $i);

	    // lists
	    $ret = $this->redis->multi($mode)
		    ->delete('lkey', 'lDest')
		    ->rpush('lkey', 'lvalue')
		    ->lpush('lkey', 'lvalue')
		    ->lpush('lkey', 'lvalue')
		    ->lpush('lkey', 'lvalue')
		    ->lpush('lkey', 'lvalue')
		    ->lpush('lkey', 'lvalue')
		    ->rpoplpush('lkey', 'lDest')
		    ->lGetRange('lDest', 0, -1)
		    ->lpop('lkey')
		    ->llen('lkey')
		    ->lRemove('lkey', 'lvalue', 3)
		    ->llen('lkey')
		    ->lget('lkey', 0)
		    ->lGetRange('lkey', 0, -1)
		    ->lSet('lkey', 1, "newValue")	 // check errors on missing key
		    ->lGetRange('lkey', 0, -1)
		    ->llen('lkey')
		    ->exec();

	    $this->assertTrue(is_array($ret));
	    $i = 0;
	    $this->assertTrue($ret[$i] >= 0 && $ret[$i] <= 2); // delete
	    $i++;
	    $this->assertTrue($ret[$i++] === 1); // 1 value
	    $this->assertTrue($ret[$i++] === 2); // 2 values
	    $this->assertTrue($ret[$i++] === 3); // 3 values
	    $this->assertTrue($ret[$i++] === 4); // 4 values
	    $this->assertTrue($ret[$i++] === 5); // 5 values
	    $this->assertTrue($ret[$i++] === 6); // 6 values
	    $this->assertTrue($ret[$i++] === 'lvalue');
	    $this->assertTrue($ret[$i++] === array('lvalue')); // 1 value only in lDest
	    $this->assertTrue($ret[$i++] === 'lvalue'); // now 4 values left
	    $this->assertTrue($ret[$i++] === 4);
	    $this->assertTrue($ret[$i++] === 3); // removing 3 elements.
	    $this->assertTrue($ret[$i++] === 1); // length is now 1
	    $this->assertTrue($ret[$i++] === 'lvalue'); // this is the head
	    $this->assertTrue($ret[$i++] === array('lvalue')); // 1 value only in lkey
	    $this->assertTrue($ret[$i++] === FALSE); // can't set list[1] if we only have a single value in it.
	    $this->assertTrue($ret[$i++] === array('lvalue')); // the previous error didn't touch anything.
	    $this->assertTrue($ret[$i++] === 1); // the previous error didn't change the length
	    $this->assertTrue(count($ret) === $i);


	    // sets
	    $ret = $this->redis->multi($mode)
		    ->delete('skey1', 'skey2', 'skeydest', 'skeyUnion', 'sDiffDest')
		    ->sadd('skey1', 'sValue1')
		    ->sadd('skey1', 'sValue2')
		    ->sadd('skey1', 'sValue3')
		    ->sadd('skey1', 'sValue4')

		    ->sadd('skey2', 'sValue1')
		    ->sadd('skey2', 'sValue2')

		    ->sSize('skey1')
		    ->sRemove('skey1', 'sValue2')
		    ->sSize('skey1')
		    ->sMove('skey1', 'skey2', 'sValue4')
		    ->sSize('skey2')
		    ->sContains('skey2', 'sValue4')
		    ->sMembers('skey1')
		    ->sMembers('skey2')
		    ->sInter('skey1', 'skey2')
		    ->sInterStore('skeydest', 'skey1', 'skey2')
		    ->sMembers('skeydest')
		    ->sUnion('skey2', 'skeydest')
		    ->sUnionStore('skeyUnion', 'skey2', 'skeydest')
		    ->sMembers('skeyUnion')
		    ->sDiff('skey1', 'skey2')
		    ->sDiffStore('sDiffDest', 'skey1', 'skey2')
		    ->sMembers('sDiffDest')
		    ->sPop('skey2')
		    ->sSize('skey2')
		    ->exec();

	    $i = 0;
	    $this->assertTrue(is_array($ret));
	    $this->assertTrue(is_long($ret[$i]) && $ret[$i] >= 0 && $ret[$i] <= 5); $i++; // deleted at most 5 values.
	    $this->assertTrue($ret[$i++] === 1); // skey1 now has 1 element.
	    $this->assertTrue($ret[$i++] === 1); // skey1 now has 2 elements.
	    $this->assertTrue($ret[$i++] === 1); // skey1 now has 3 elements.
	    $this->assertTrue($ret[$i++] === 1); // skey1 now has 4 elements.

	    $this->assertTrue($ret[$i++] === 1); // skey2 now has 1 element.
	    $this->assertTrue($ret[$i++] === 1); // skey2 now has 2 elements.

	    $this->assertTrue($ret[$i++] === 4);
	    $this->assertTrue($ret[$i++] === 1); // we did remove that value.
	    $this->assertTrue($ret[$i++] === 3); // now 3 values only.
	    $this->assertTrue($ret[$i++] === TRUE); // the move did succeed.
	    $this->assertTrue($ret[$i++] === 3); // sKey2 now has 3 values.
	    $this->assertTrue($ret[$i++] === TRUE); // sKey2 does contain sValue4.
	    foreach(array('sValue1', 'sValue3') as $k) { // sKey1 contains sValue1 and sValue3.
		    $this->assertTrue(in_array($k, $ret[$i]));
	    }
	    $this->assertTrue(count($ret[$i++]) === 2);
	    foreach(array('sValue1', 'sValue2', 'sValue4') as $k) { // sKey2 contains sValue1, sValue2, and sValue4.
		    $this->assertTrue(in_array($k, $ret[$i]));
	    }
	    $this->assertTrue(count($ret[$i++]) === 3);
	    $this->assertTrue($ret[$i++] === array('sValue1')); // intersection
	    $this->assertTrue($ret[$i++] === 1); // intersection + store → 1 value in the destination set.
	    $this->assertTrue($ret[$i++] === array('sValue1')); // sinterstore destination contents

	    foreach(array('sValue1', 'sValue2', 'sValue4') as $k) { // (skeydest U sKey2) contains sValue1, sValue2, and sValue4.
		    $this->assertTrue(in_array($k, $ret[$i]));
	    }
	    $this->assertTrue(count($ret[$i++]) === 3); // union size

	    $this->assertTrue($ret[$i++] === 3); // unionstore size
	    foreach(array('sValue1', 'sValue2', 'sValue4') as $k) { // (skeyUnion) contains sValue1, sValue2, and sValue4.
		    $this->assertTrue(in_array($k, $ret[$i]));
	    }
	    $this->assertTrue(count($ret[$i++]) === 3); // skeyUnion size

	    $this->assertTrue($ret[$i++] === array('sValue3')); // diff skey1, skey2 : only sValue3 is not shared.
	    $this->assertTrue($ret[$i++] === 1); // sdiffstore size == 1
	    $this->assertTrue($ret[$i++] === array('sValue3')); // contents of sDiffDest

	    $this->assertTrue(in_array($ret[$i++], array('sValue1', 'sValue2', 'sValue4'))); // we removed an element from sKey2
	    $this->assertTrue($ret[$i++] === 2); // sKey2 now has 2 elements only.

	    $this->assertTrue(count($ret) === $i);

	    // sorted sets
	    $ret = $this->redis->multi($mode)
			->delete('zkey1', 'zkey2', 'zkey5', 'zInter', 'zUnion')
		    ->zadd('zkey1', 1, 'zValue1')
		    ->zadd('zkey1', 5, 'zValue5')
		    ->zadd('zkey1', 2, 'zValue2')
		    ->zRange('zkey1', 0, -1)
		    ->zDelete('zkey1', 'zValue2')
		    ->zRange('zkey1', 0, -1)
		    ->zadd('zkey1', 11, 'zValue11')
		    ->zadd('zkey1', 12, 'zValue12')
		    ->zadd('zkey1', 13, 'zValue13')
		    ->zadd('zkey1', 14, 'zValue14')
		    ->zadd('zkey1', 15, 'zValue15')
		    ->zDeleteRangeByScore('zkey1', 11, 13)
		    ->zrange('zkey1', 0, -1)
		    ->zReverseRange('zkey1', 0, -1)
		    ->zRangeByScore('zkey1', 1, 6)
		    ->zCard('zkey1')
		    ->zScore('zkey1', 'zValue15')
		    ->zadd('zkey2', 5, 'zValue5')
		    ->zadd('zkey2', 2, 'zValue2')
		    ->zInter('zInter', array('zkey1', 'zkey2'))
		    ->zRange('zkey1', 0, -1)
		    ->zRange('zkey2', 0, -1)
		    ->zRange('zInter', 0, -1)
		    ->zUnion('zUnion', array('zkey1', 'zkey2'))
		    ->zRange('zUnion', 0, -1)
		    ->zadd('zkey5', 5, 'zValue5')
		    ->zIncrBy('zkey5', 3, 'zValue5') // fix this
		    ->zScore('zkey5', 'zValue5')
			->zScore('zkey5', 'unknown')
		    ->exec();

	    $i = 0;
	    $this->assertTrue(is_array($ret));
	    $this->assertTrue(is_long($ret[$i]) && $ret[$i] >= 0 && $ret[$i] <= 5); $i++; // deleting at most 5 keys
	    $this->assertTrue($ret[$i++] === 1);
	    $this->assertTrue($ret[$i++] === 1);
	    $this->assertTrue($ret[$i++] === 1);
	    $this->assertTrue($ret[$i++] === array('zValue1', 'zValue2', 'zValue5'));
	    $this->assertTrue($ret[$i++] === 1);
	    $this->assertTrue($ret[$i++] === array('zValue1', 'zValue5'));
	    $this->assertTrue($ret[$i++] === 1); // adding zValue11
	    $this->assertTrue($ret[$i++] === 1); // adding zValue12
	    $this->assertTrue($ret[$i++] === 1); // adding zValue13
	    $this->assertTrue($ret[$i++] === 1); // adding zValue14
	    $this->assertTrue($ret[$i++] === 1); // adding zValue15
	    $this->assertTrue($ret[$i++] === 3); // deleted zValue11, zValue12, zValue13
	    $this->assertTrue($ret[$i++] === array('zValue1', 'zValue5', 'zValue14', 'zValue15'));
	    $this->assertTrue($ret[$i++] === array('zValue15', 'zValue14', 'zValue5', 'zValue1'));
	    $this->assertTrue($ret[$i++] === array('zValue1', 'zValue5'));
	    $this->assertTrue($ret[$i++] === 4); // 4 elements
	    $this->assertTrue($ret[$i++] === 15.0);
	    $this->assertTrue($ret[$i++] === 1); // added value
	    $this->assertTrue($ret[$i++] === 1); // added value
	    $this->assertTrue($ret[$i++] === 1); // zinter only has 1 value
	    $this->assertTrue($ret[$i++] === array('zValue1', 'zValue5', 'zValue14', 'zValue15')); // zkey1 contents
	    $this->assertTrue($ret[$i++] === array('zValue2', 'zValue5')); // zkey2 contents
	    $this->assertTrue($ret[$i++] === array('zValue5')); // zinter contents
	    $this->assertTrue($ret[$i++] === 5); // zUnion has 5 values (1,2,5,14,15)
	    $this->assertTrue($ret[$i++] === array('zValue1', 'zValue2', 'zValue5', 'zValue14', 'zValue15')); // zunion contents
	    $this->assertTrue($ret[$i++] === 1); // added value to zkey5, with score 5
	    $this->assertTrue($ret[$i++] === 8.0); // incremented score by 3 → it is now 8.
	    $this->assertTrue($ret[$i++] === 8.0); // current score is 8.
		$this->assertTrue($ret[$i++] === FALSE); // score for unknown element.

	    $this->assertTrue(count($ret) === $i);

		// hash
	    $ret = $this->redis->multi($mode)
		    ->delete('hkey1')
		    ->hset('hkey1', 'key1', 'value1')
		    ->hset('hkey1', 'key2', 'value2')
		    ->hset('hkey1', 'key3', 'value3')
		    ->hmget('hkey1', array('key1', 'key2', 'key3'))
		    ->hget('hkey1', 'key1')
		    ->hlen('hkey1')
		    ->hdel('hkey1', 'key2')
		    ->hdel('hkey1', 'key2')
		    ->hexists('hkey1', 'key2')
		    ->hkeys('hkey1')
		    ->hvals('hkey1')
		    ->hgetall('hkey1')
		    ->hset('hkey1', 'valn', 1)
			->hset('hkey1', 'val-fail', 'non-string')
		    ->hget('hkey1', 'val-fail')
			->exec();

	    $i = 0;
	    $this->assertTrue(is_array($ret));
	    $this->assertTrue($ret[$i++] <= 1); // delete
	    $this->assertTrue($ret[$i++] === 1); // added 1 element
	    $this->assertTrue($ret[$i++] === 1); // added 1 element
	    $this->assertTrue($ret[$i++] === 1); // added 1 element
	    $this->assertTrue($ret[$i++] === array('key1' => 'value1', 'key2' => 'value2', 'key3' => 'value3')); // hmget, 3 elements
	    $this->assertTrue($ret[$i++] === 'value1'); // hget
	    $this->assertTrue($ret[$i++] === 3); // hlen
	    $this->assertTrue($ret[$i++] === 1); // hdel succeeded
	    $this->assertTrue($ret[$i++] === 0); // hdel failed
	    $this->assertTrue($ret[$i++] === FALSE); // hexists didn't find the deleted key
	    $this->assertTrue($ret[$i] === array('key1', 'key3') || $ret[$i] === array('key3', 'key1')); $i++; // hkeys
	    $this->assertTrue($ret[$i] === array('value1', 'value3') || $ret[$i] === array('value3', 'value1')); $i++; // hvals
	    $this->assertTrue($ret[$i] === array('key1' => 'value1', 'key3' => 'value3') || $ret[$i] === array('key3' => 'value3', 'key1' => 'value1')); $i++; // hgetall
	    $this->assertTrue($ret[$i++] === 1); // added 1 element
		$this->assertTrue($ret[$i++] === 1); // added the element, so 1.
	    $this->assertTrue($ret[$i++] === 'non-string'); // hset succeeded
		$this->assertTrue(count($ret) === $i);

		$ret = $this->redis->multi($mode) // default to MULTI, not PIPELINE.
		    ->delete('test')
		    ->set('test', 'xyz')
		    ->get('test')
		    ->exec();
	    $i = 0;
	    $this->assertTrue(is_array($ret));
	    $this->assertTrue($ret[$i++] <= 1); // delete
	    $this->assertTrue($ret[$i++] === TRUE); // added 1 element
	    $this->assertTrue($ret[$i++] === 'xyz');
	    $this->assertTrue(count($ret) === $i);

        // GitHub issue 78
        $this->redis->del('test');
        for($i = 1; $i <= 5; $i++)
            $this->redis->zadd('test', $i, (string)$i);

        $result = $this->redis->multi($mode)
            ->zscore('test', "1")
            ->zscore('test', "6")
            ->zscore('test', "8")
            ->zscore('test', "2")
            ->exec();

        $this->assertTrue($result === array(1.0, FALSE, FALSE, 2.0));
    }

    protected function differentType($mode) {

        // string
        $key = 'string';
        $ret = $this->redis->multi($mode)
            ->delete($key)
            ->set($key, 'value')

            // lists I/F
            ->rPush($key, 'lvalue')
            ->lPush($key, 'lvalue')
            ->lLen($key)
            ->lPop($key)
            ->lGetRange($key, 0, -1)
            ->lTrim($key, 0, 1)
            ->lGet($key, 0)
            ->lSet($key, 0, "newValue")
            ->lRemove($key, 'lvalue', 1)
            ->lPop($key)
            ->rPop($key)
            ->rPoplPush($key, __FUNCTION__ . 'lkey1')

            // sets I/F
            ->sAdd($key, 'sValue1')
            ->sRemove($key, 'sValue1')
            ->sPop($key)
            ->sMove($key, __FUNCTION__ . 'skey1', 'sValue1')
            ->sSize($key)
            ->sContains($key, 'sValue1')
            ->sInter($key, __FUNCTION__ . 'skey2')
            ->sUnion($key, __FUNCTION__ . 'skey4')
            ->sDiff($key, __FUNCTION__ . 'skey7')
            ->sMembers($key)
            ->sRandMember($key)

            // sorted sets I/F
            ->zAdd($key, 1, 'zValue1')
            ->zDelete($key, 'zValue1')
            ->zIncrBy($key, 1, 'zValue1')
            ->zRank($key, 'zValue1')
            ->zRevRank($key, 'zValue1')
            ->zRange($key, 0, -1)
            ->zReverseRange($key, 0, -1)
            ->zRangeByScore($key, 1, 2)
            ->zCount($key, 0, -1)
            ->zCard($key)
            ->zScore($key, 'zValue1')
            ->zDeleteRangeByRank($key, 1, 2)
            ->zDeleteRangeByScore($key, 1, 2)

            // hash I/F
            ->hSet($key, 'key1', 'value1')
            ->hGet($key, 'key1')
            ->hMGet($key, array('key1'))
            ->hMSet($key, array('key1' => 'value1'))
            ->hIncrBy($key, 'key2', 1)
            ->hExists($key, 'key2')
            ->hDel($key, 'key2')
            ->hLen($key)
            ->hKeys($key)
            ->hVals($key)
            ->hGetAll($key)

            ->exec();

        $i = 0;
        $this->assertTrue(is_array($ret));
        $this->assertTrue(is_long($ret[$i++])); // delete
        $this->assertTrue($ret[$i++] === TRUE); // set

        $this->assertTrue($ret[$i++] === FALSE); // rpush
        $this->assertTrue($ret[$i++] === FALSE); // lpush
        $this->assertTrue($ret[$i++] === FALSE); // llen
        $this->assertTrue($ret[$i++] === FALSE); // lpop
        $this->assertTrue($ret[$i++] === FALSE); // lgetrange
        $this->assertTrue($ret[$i++] === FALSE); // ltrim
        $this->assertTrue($ret[$i++] === FALSE); // lget
        $this->assertTrue($ret[$i++] === FALSE); // lset
        $this->assertTrue($ret[$i++] === FALSE); // lremove
        $this->assertTrue($ret[$i++] === FALSE); // lpop
        $this->assertTrue($ret[$i++] === FALSE); // rpop
        $this->assertTrue($ret[$i++] === FALSE); // rpoplush

        $this->assertTrue($ret[$i++] === FALSE); // sadd
        $this->assertTrue($ret[$i++] === FALSE); // sremove
        $this->assertTrue($ret[$i++] === FALSE); // spop
        $this->assertTrue($ret[$i++] === FALSE); // smove
        $this->assertTrue($ret[$i++] === FALSE); // ssize
        $this->assertTrue($ret[$i++] === FALSE); // scontains
        $this->assertTrue($ret[$i++] === FALSE); // sinter
        $this->assertTrue($ret[$i++] === FALSE); // sunion
        $this->assertTrue($ret[$i++] === FALSE); // sdiff
        $this->assertTrue($ret[$i++] === FALSE); // smembers
        $this->assertTrue($ret[$i++] === FALSE); // srandmember

        $this->assertTrue($ret[$i++] === FALSE); // zadd
        $this->assertTrue($ret[$i++] === FALSE); // zdelete
        $this->assertTrue($ret[$i++] === FALSE); // zincrby
        $this->assertTrue($ret[$i++] === FALSE); // zrank
        $this->assertTrue($ret[$i++] === FALSE); // zrevrank
        $this->assertTrue($ret[$i++] === FALSE); // zrange
        $this->assertTrue($ret[$i++] === FALSE); // zreverserange
        $this->assertTrue($ret[$i++] === FALSE); // zrangebyscore
        $this->assertTrue($ret[$i++] === FALSE); // zcount
        $this->assertTrue($ret[$i++] === FALSE); // zcard
        $this->assertTrue($ret[$i++] === FALSE); // zscore
        $this->assertTrue($ret[$i++] === FALSE); // zdeleterangebyrank
        $this->assertTrue($ret[$i++] === FALSE); // zdeleterangebyscore

        $this->assertTrue($ret[$i++] === FALSE); // hset
        $this->assertTrue($ret[$i++] === FALSE); // hget
        $this->assertTrue($ret[$i++] === FALSE); // hmget
        $this->assertTrue($ret[$i++] === FALSE); // hmset
        $this->assertTrue($ret[$i++] === FALSE); // hincrby
        $this->assertTrue($ret[$i++] === FALSE); // hexists
        $this->assertTrue($ret[$i++] === FALSE); // hdel
        $this->assertTrue($ret[$i++] === FALSE); // hlen
        $this->assertTrue($ret[$i++] === FALSE); // hkeys
        $this->assertTrue($ret[$i++] === FALSE); // hvals
        $this->assertTrue($ret[$i++] === FALSE); // hgetall

        $this->assertEquals($i, count($ret));

        // list
        $key = 'list';
        $ret = $this->redis->multi($mode)
            ->delete($key)
            ->lpush($key, 'lvalue')

            // string I/F
            ->get($key)
            ->getset($key, 'value2')
            ->append($key, 'append')
            ->substr($key, 0, 8)
            ->mget(array($key))
            ->incr($key)
            ->incrBy($key, 1)
            ->decr($key)
            ->decrBy($key, 1)

            // sets I/F
            ->sAdd($key, 'sValue1')
            ->sRemove($key, 'sValue1')
            ->sPop($key)
            ->sMove($key, __FUNCTION__ . 'skey1', 'sValue1')
            ->sSize($key)
            ->sContains($key, 'sValue1')
            ->sInter($key, __FUNCTION__ . 'skey2')
            ->sUnion($key, __FUNCTION__ . 'skey4')
            ->sDiff($key, __FUNCTION__ . 'skey7')
            ->sMembers($key)
            ->sRandMember($key)

            // sorted sets I/F
            ->zAdd($key, 1, 'zValue1')
            ->zDelete($key, 'zValue1')
            ->zIncrBy($key, 1, 'zValue1')
            ->zRank($key, 'zValue1')
            ->zRevRank($key, 'zValue1')
            ->zRange($key, 0, -1)
            ->zReverseRange($key, 0, -1)
            ->zRangeByScore($key, 1, 2)
            ->zCount($key, 0, -1)
            ->zCard($key)
            ->zScore($key, 'zValue1')
            ->zDeleteRangeByRank($key, 1, 2)
            ->zDeleteRangeByScore($key, 1, 2)

            // hash I/F
            ->hSet($key, 'key1', 'value1')
            ->hGet($key, 'key1')
            ->hMGet($key, array('key1'))
            ->hMSet($key, array('key1' => 'value1'))
            ->hIncrBy($key, 'key2', 1)
            ->hExists($key, 'key2')
            ->hDel($key, 'key2')
            ->hLen($key)
            ->hKeys($key)
            ->hVals($key)
            ->hGetAll($key)

            ->exec();

        $i = 0;
        $this->assertTrue(is_array($ret));
        $this->assertTrue(is_long($ret[$i++])); // delete
        $this->assertTrue($ret[$i++] === 1); // lpush

        $this->assertTrue($ret[$i++] === FALSE); // get
        $this->assertTrue($ret[$i++] === FALSE); // getset
        $this->assertTrue($ret[$i++] === FALSE); // append
        $this->assertTrue($ret[$i++] === FALSE); // substr
        $this->assertTrue(is_array($ret[$i]) && count($ret[$i]) === 1 && $ret[$i][0] === FALSE); // mget
        $i++;
        $this->assertTrue($ret[$i++] === FALSE); // incr
        $this->assertTrue($ret[$i++] === FALSE); // incrBy
        $this->assertTrue($ret[$i++] === FALSE); // decr
        $this->assertTrue($ret[$i++] === FALSE); // decrBy

        $this->assertTrue($ret[$i++] === FALSE); // sadd
        $this->assertTrue($ret[$i++] === FALSE); // sremove
        $this->assertTrue($ret[$i++] === FALSE); // spop
        $this->assertTrue($ret[$i++] === FALSE); // smove
        $this->assertTrue($ret[$i++] === FALSE); // ssize
        $this->assertTrue($ret[$i++] === FALSE); // scontains
        $this->assertTrue($ret[$i++] === FALSE); // sinter
        $this->assertTrue($ret[$i++] === FALSE); // sunion
        $this->assertTrue($ret[$i++] === FALSE); // sdiff
        $this->assertTrue($ret[$i++] === FALSE); // smembers
        $this->assertTrue($ret[$i++] === FALSE); // srandmember

        $this->assertTrue($ret[$i++] === FALSE); // zadd
        $this->assertTrue($ret[$i++] === FALSE); // zdelete
        $this->assertTrue($ret[$i++] === FALSE); // zincrby
        $this->assertTrue($ret[$i++] === FALSE); // zrank
        $this->assertTrue($ret[$i++] === FALSE); // zrevrank
        $this->assertTrue($ret[$i++] === FALSE); // zrange
        $this->assertTrue($ret[$i++] === FALSE); // zreverserange
        $this->assertTrue($ret[$i++] === FALSE); // zrangebyscore
        $this->assertTrue($ret[$i++] === FALSE); // zcount
        $this->assertTrue($ret[$i++] === FALSE); // zcard
        $this->assertTrue($ret[$i++] === FALSE); // zscore
        $this->assertTrue($ret[$i++] === FALSE); // zdeleterangebyrank
        $this->assertTrue($ret[$i++] === FALSE); // zdeleterangebyscore

        $this->assertTrue($ret[$i++] === FALSE); // hset
        $this->assertTrue($ret[$i++] === FALSE); // hget
        $this->assertTrue($ret[$i++] === FALSE); // hmget
        $this->assertTrue($ret[$i++] === FALSE); // hmset
        $this->assertTrue($ret[$i++] === FALSE); // hincrby
        $this->assertTrue($ret[$i++] === FALSE); // hexists
        $this->assertTrue($ret[$i++] === FALSE); // hdel
        $this->assertTrue($ret[$i++] === FALSE); // hlen
        $this->assertTrue($ret[$i++] === FALSE); // hkeys
        $this->assertTrue($ret[$i++] === FALSE); // hvals
        $this->assertTrue($ret[$i++] === FALSE); // hgetall

        $this->assertEquals($i, count($ret));

        // set
        $key = 'set';
        $ret = $this->redis->multi($mode)
            ->delete($key)
            ->sAdd($key, 'sValue')

            // string I/F
            ->get($key)
            ->getset($key, 'value2')
            ->append($key, 'append')
            ->substr($key, 0, 8)
            ->mget(array($key))
            ->incr($key)
            ->incrBy($key, 1)
            ->decr($key)
            ->decrBy($key, 1)

            // lists I/F
            ->rPush($key, 'lvalue')
            ->lPush($key, 'lvalue')
            ->lLen($key)
            ->lPop($key)
            ->lGetRange($key, 0, -1)
            ->lTrim($key, 0, 1)
            ->lGet($key, 0)
            ->lSet($key, 0, "newValue")
            ->lRemove($key, 'lvalue', 1)
            ->lPop($key)
            ->rPop($key)
            ->rPoplPush($key, __FUNCTION__ . 'lkey1')

            // sorted sets I/F
            ->zAdd($key, 1, 'zValue1')
            ->zDelete($key, 'zValue1')
            ->zIncrBy($key, 1, 'zValue1')
            ->zRank($key, 'zValue1')
            ->zRevRank($key, 'zValue1')
            ->zRange($key, 0, -1)
            ->zReverseRange($key, 0, -1)
            ->zRangeByScore($key, 1, 2)
            ->zCount($key, 0, -1)
            ->zCard($key)
            ->zScore($key, 'zValue1')
            ->zDeleteRangeByRank($key, 1, 2)
            ->zDeleteRangeByScore($key, 1, 2)

            // hash I/F
            ->hSet($key, 'key1', 'value1')
            ->hGet($key, 'key1')
            ->hMGet($key, array('key1'))
            ->hMSet($key, array('key1' => 'value1'))
            ->hIncrBy($key, 'key2', 1)
            ->hExists($key, 'key2')
            ->hDel($key, 'key2')
            ->hLen($key)
            ->hKeys($key)
            ->hVals($key)
            ->hGetAll($key)

            ->exec();

        $i = 0;
        $this->assertTrue(is_array($ret));
        $this->assertTrue(is_long($ret[$i++])); // delete
        $this->assertTrue($ret[$i++] === 1); // zadd

        $this->assertTrue($ret[$i++] === FALSE); // get
        $this->assertTrue($ret[$i++] === FALSE); // getset
        $this->assertTrue($ret[$i++] === FALSE); // append
        $this->assertTrue($ret[$i++] === FALSE); // substr
        $this->assertTrue(is_array($ret[$i]) && count($ret[$i]) === 1 && $ret[$i][0] === FALSE); // mget
        $i++;
        $this->assertTrue($ret[$i++] === FALSE); // incr
        $this->assertTrue($ret[$i++] === FALSE); // incrBy
        $this->assertTrue($ret[$i++] === FALSE); // decr
        $this->assertTrue($ret[$i++] === FALSE); // decrBy

        $this->assertTrue($ret[$i++] === FALSE); // rpush
        $this->assertTrue($ret[$i++] === FALSE); // lpush
        $this->assertTrue($ret[$i++] === FALSE); // llen
        $this->assertTrue($ret[$i++] === FALSE); // lpop
        $this->assertTrue($ret[$i++] === FALSE); // lgetrange
        $this->assertTrue($ret[$i++] === FALSE); // ltrim
        $this->assertTrue($ret[$i++] === FALSE); // lget
        $this->assertTrue($ret[$i++] === FALSE); // lset
        $this->assertTrue($ret[$i++] === FALSE); // lremove
        $this->assertTrue($ret[$i++] === FALSE); // lpop
        $this->assertTrue($ret[$i++] === FALSE); // rpop
        $this->assertTrue($ret[$i++] === FALSE); // rpoplush

        $this->assertTrue($ret[$i++] === FALSE); // zadd
        $this->assertTrue($ret[$i++] === FALSE); // zdelete
        $this->assertTrue($ret[$i++] === FALSE); // zincrby
        $this->assertTrue($ret[$i++] === FALSE); // zrank
        $this->assertTrue($ret[$i++] === FALSE); // zrevrank
        $this->assertTrue($ret[$i++] === FALSE); // zrange
        $this->assertTrue($ret[$i++] === FALSE); // zreverserange
        $this->assertTrue($ret[$i++] === FALSE); // zrangebyscore
        $this->assertTrue($ret[$i++] === FALSE); // zcount
        $this->assertTrue($ret[$i++] === FALSE); // zcard
        $this->assertTrue($ret[$i++] === FALSE); // zscore
        $this->assertTrue($ret[$i++] === FALSE); // zdeleterangebyrank
        $this->assertTrue($ret[$i++] === FALSE); // zdeleterangebyscore

        $this->assertTrue($ret[$i++] === FALSE); // hset
        $this->assertTrue($ret[$i++] === FALSE); // hget
        $this->assertTrue($ret[$i++] === FALSE); // hmget
        $this->assertTrue($ret[$i++] === FALSE); // hmset
        $this->assertTrue($ret[$i++] === FALSE); // hincrby
        $this->assertTrue($ret[$i++] === FALSE); // hexists
        $this->assertTrue($ret[$i++] === FALSE); // hdel
        $this->assertTrue($ret[$i++] === FALSE); // hlen
        $this->assertTrue($ret[$i++] === FALSE); // hkeys
        $this->assertTrue($ret[$i++] === FALSE); // hvals
        $this->assertTrue($ret[$i++] === FALSE); // hgetall

        $this->assertEquals($i, count($ret));

        // sorted set
        $key = 'sortedset';
        $ret = $this->redis->multi($mode)
            ->delete($key)
            ->zAdd($key, 0, 'zValue')

            // string I/F
            ->get($key)
            ->getset($key, 'value2')
            ->append($key, 'append')
            ->substr($key, 0, 8)
            ->mget(array($key))
            ->incr($key)
            ->incrBy($key, 1)
            ->decr($key)
            ->decrBy($key, 1)

            // lists I/F
            ->rPush($key, 'lvalue')
            ->lPush($key, 'lvalue')
            ->lLen($key)
            ->lPop($key)
            ->lGetRange($key, 0, -1)
            ->lTrim($key, 0, 1)
            ->lGet($key, 0)
            ->lSet($key, 0, "newValue")
            ->lRemove($key, 'lvalue', 1)
            ->lPop($key)
            ->rPop($key)
            ->rPoplPush($key, __FUNCTION__ . 'lkey1')

            // sets I/F
            ->sAdd($key, 'sValue1')
            ->sRemove($key, 'sValue1')
            ->sPop($key)
            ->sMove($key, __FUNCTION__ . 'skey1', 'sValue1')
            ->sSize($key)
            ->sContains($key, 'sValue1')
            ->sInter($key, __FUNCTION__ . 'skey2')
            ->sUnion($key, __FUNCTION__ . 'skey4')
            ->sDiff($key, __FUNCTION__ . 'skey7')
            ->sMembers($key)
            ->sRandMember($key)

            // hash I/F
            ->hSet($key, 'key1', 'value1')
            ->hGet($key, 'key1')
            ->hMGet($key, array('key1'))
            ->hMSet($key, array('key1' => 'value1'))
            ->hIncrBy($key, 'key2', 1)
            ->hExists($key, 'key2')
            ->hDel($key, 'key2')
            ->hLen($key)
            ->hKeys($key)
            ->hVals($key)
            ->hGetAll($key)

            ->exec();

        $i = 0;
        $this->assertTrue(is_array($ret));
        $this->assertTrue(is_long($ret[$i++])); // delete
        $this->assertTrue($ret[$i++] === 1); // zadd

        $this->assertTrue($ret[$i++] === FALSE); // get
        $this->assertTrue($ret[$i++] === FALSE); // getset
        $this->assertTrue($ret[$i++] === FALSE); // append
        $this->assertTrue($ret[$i++] === FALSE); // substr
        $this->assertTrue(is_array($ret[$i]) && count($ret[$i]) === 1 && $ret[$i][0] === FALSE); // mget
        $i++;
        $this->assertTrue($ret[$i++] === FALSE); // incr
        $this->assertTrue($ret[$i++] === FALSE); // incrBy
        $this->assertTrue($ret[$i++] === FALSE); // decr
        $this->assertTrue($ret[$i++] === FALSE); // decrBy

        $this->assertTrue($ret[$i++] === FALSE); // rpush
        $this->assertTrue($ret[$i++] === FALSE); // lpush
        $this->assertTrue($ret[$i++] === FALSE); // llen
        $this->assertTrue($ret[$i++] === FALSE); // lpop
        $this->assertTrue($ret[$i++] === FALSE); // lgetrange
        $this->assertTrue($ret[$i++] === FALSE); // ltrim
        $this->assertTrue($ret[$i++] === FALSE); // lget
        $this->assertTrue($ret[$i++] === FALSE); // lset
        $this->assertTrue($ret[$i++] === FALSE); // lremove
        $this->assertTrue($ret[$i++] === FALSE); // lpop
        $this->assertTrue($ret[$i++] === FALSE); // rpop
        $this->assertTrue($ret[$i++] === FALSE); // rpoplush

        $this->assertTrue($ret[$i++] === FALSE); // sadd
        $this->assertTrue($ret[$i++] === FALSE); // sremove
        $this->assertTrue($ret[$i++] === FALSE); // spop
        $this->assertTrue($ret[$i++] === FALSE); // smove
        $this->assertTrue($ret[$i++] === FALSE); // ssize
        $this->assertTrue($ret[$i++] === FALSE); // scontains
        $this->assertTrue($ret[$i++] === FALSE); // sinter
        $this->assertTrue($ret[$i++] === FALSE); // sunion
        $this->assertTrue($ret[$i++] === FALSE); // sdiff
        $this->assertTrue($ret[$i++] === FALSE); // smembers
        $this->assertTrue($ret[$i++] === FALSE); // srandmember

        $this->assertTrue($ret[$i++] === FALSE); // hset
        $this->assertTrue($ret[$i++] === FALSE); // hget
        $this->assertTrue($ret[$i++] === FALSE); // hmget
        $this->assertTrue($ret[$i++] === FALSE); // hmset
        $this->assertTrue($ret[$i++] === FALSE); // hincrby
        $this->assertTrue($ret[$i++] === FALSE); // hexists
        $this->assertTrue($ret[$i++] === FALSE); // hdel
        $this->assertTrue($ret[$i++] === FALSE); // hlen
        $this->assertTrue($ret[$i++] === FALSE); // hkeys
        $this->assertTrue($ret[$i++] === FALSE); // hvals
        $this->assertTrue($ret[$i++] === FALSE); // hgetall

        $this->assertEquals($i, count($ret));

        // hash
        $key = 'hash';
        $ret = $this->redis->multi($mode)
            ->delete($key)
            ->hset($key, 'key1', 'hValue')

            // string I/F
            ->get($key)
            ->getset($key, 'value2')
            ->append($key, 'append')
            ->substr($key, 0, 8)
            ->mget(array($key))
            ->incr($key)
            ->incrBy($key, 1)
            ->decr($key)
            ->decrBy($key, 1)

            // lists I/F
            ->rPush($key, 'lvalue')
            ->lPush($key, 'lvalue')
            ->lLen($key)
            ->lPop($key)
            ->lGetRange($key, 0, -1)
            ->lTrim($key, 0, 1)
            ->lGet($key, 0)
            ->lSet($key, 0, "newValue")
            ->lRemove($key, 'lvalue', 1)
            ->lPop($key)
            ->rPop($key)
            ->rPoplPush($key, __FUNCTION__ . 'lkey1')

            // sets I/F
            ->sAdd($key, 'sValue1')
            ->sRemove($key, 'sValue1')
            ->sPop($key)
            ->sMove($key, __FUNCTION__ . 'skey1', 'sValue1')
            ->sSize($key)
            ->sContains($key, 'sValue1')
            ->sInter($key, __FUNCTION__ . 'skey2')
            ->sUnion($key, __FUNCTION__ . 'skey4')
            ->sDiff($key, __FUNCTION__ . 'skey7')
            ->sMembers($key)
            ->sRandMember($key)

            // sorted sets I/F
            ->zAdd($key, 1, 'zValue1')
            ->zDelete($key, 'zValue1')
            ->zIncrBy($key, 1, 'zValue1')
            ->zRank($key, 'zValue1')
            ->zRevRank($key, 'zValue1')
            ->zRange($key, 0, -1)
            ->zReverseRange($key, 0, -1)
            ->zRangeByScore($key, 1, 2)
            ->zCount($key, 0, -1)
            ->zCard($key)
            ->zScore($key, 'zValue1')
            ->zDeleteRangeByRank($key, 1, 2)
            ->zDeleteRangeByScore($key, 1, 2)

            ->exec();

        $i = 0;
        $this->assertTrue(is_array($ret));
        $this->assertTrue(is_long($ret[$i++])); // delete
        $this->assertTrue($ret[$i++] === 1); // hset

        $this->assertTrue($ret[$i++] === FALSE); // get
        $this->assertTrue($ret[$i++] === FALSE); // getset
        $this->assertTrue($ret[$i++] === FALSE); // append
        $this->assertTrue($ret[$i++] === FALSE); // substr
        $this->assertTrue(is_array($ret[$i]) && count($ret[$i]) === 1 && $ret[$i][0] === FALSE); // mget
        $i++;
        $this->assertTrue($ret[$i++] === FALSE); // incr
        $this->assertTrue($ret[$i++] === FALSE); // incrBy
        $this->assertTrue($ret[$i++] === FALSE); // decr
        $this->assertTrue($ret[$i++] === FALSE); // decrBy

        $this->assertTrue($ret[$i++] === FALSE); // rpush
        $this->assertTrue($ret[$i++] === FALSE); // lpush
        $this->assertTrue($ret[$i++] === FALSE); // llen
        $this->assertTrue($ret[$i++] === FALSE); // lpop
        $this->assertTrue($ret[$i++] === FALSE); // lgetrange
        $this->assertTrue($ret[$i++] === FALSE); // ltrim
        $this->assertTrue($ret[$i++] === FALSE); // lget
        $this->assertTrue($ret[$i++] === FALSE); // lset
        $this->assertTrue($ret[$i++] === FALSE); // lremove
        $this->assertTrue($ret[$i++] === FALSE); // lpop
        $this->assertTrue($ret[$i++] === FALSE); // rpop
        $this->assertTrue($ret[$i++] === FALSE); // rpoplush

        $this->assertTrue($ret[$i++] === FALSE); // sadd
        $this->assertTrue($ret[$i++] === FALSE); // sremove
        $this->assertTrue($ret[$i++] === FALSE); // spop
        $this->assertTrue($ret[$i++] === FALSE); // smove
        $this->assertTrue($ret[$i++] === FALSE); // ssize
        $this->assertTrue($ret[$i++] === FALSE); // scontains
        $this->assertTrue($ret[$i++] === FALSE); // sinter
        $this->assertTrue($ret[$i++] === FALSE); // sunion
        $this->assertTrue($ret[$i++] === FALSE); // sdiff
        $this->assertTrue($ret[$i++] === FALSE); // smembers
        $this->assertTrue($ret[$i++] === FALSE); // srandmember

        $this->assertTrue($ret[$i++] === FALSE); // zadd
        $this->assertTrue($ret[$i++] === FALSE); // zdelete
        $this->assertTrue($ret[$i++] === FALSE); // zincrby
        $this->assertTrue($ret[$i++] === FALSE); // zrank
        $this->assertTrue($ret[$i++] === FALSE); // zrevrank
        $this->assertTrue($ret[$i++] === FALSE); // zrange
        $this->assertTrue($ret[$i++] === FALSE); // zreverserange
        $this->assertTrue($ret[$i++] === FALSE); // zrangebyscore
        $this->assertTrue($ret[$i++] === FALSE); // zcount
        $this->assertTrue($ret[$i++] === FALSE); // zcard
        $this->assertTrue($ret[$i++] === FALSE); // zscore
        $this->assertTrue($ret[$i++] === FALSE); // zdeleterangebyrank
        $this->assertTrue($ret[$i++] === FALSE); // zdeleterangebyscore

        $this->assertEquals($i, count($ret));
    }

    public function testDifferentTypeString() {
        $key = 'string';
        $this->redis->del($key);
        $this->assertEquals(TRUE, $this->redis->set($key, 'value'));

        // lists I/F
        $this->assertEquals(FALSE, $this->redis->rPush($key, 'lvalue'));
        $this->assertEquals(FALSE, $this->redis->lPush($key, 'lvalue'));
        $this->assertEquals(FALSE, $this->redis->lLen($key));
        $this->assertEquals(FALSE, $this->redis->lPop($key));
        $this->assertEquals(FALSE, $this->redis->lGetRange($key, 0, -1));
        $this->assertEquals(FALSE, $this->redis->lTrim($key, 0, 1));
        $this->assertEquals(FALSE, $this->redis->lGet($key, 0));
        $this->assertEquals(FALSE, $this->redis->lSet($key, 0, "newValue"));
        $this->assertEquals(FALSE, $this->redis->lRemove($key, 'lvalue', 1));
        $this->assertEquals(FALSE, $this->redis->lPop($key));
        $this->assertEquals(FALSE, $this->redis->rPop($key));
        $this->assertEquals(FALSE, $this->redis->rPoplPush($key, __FUNCTION__ . 'lkey1'));

        // sets I/F
        $this->assertEquals(FALSE, $this->redis->sAdd($key, 'sValue1'));
        $this->assertEquals(FALSE, $this->redis->sRemove($key, 'sValue1'));
        $this->assertEquals(FALSE, $this->redis->sPop($key));
        $this->assertEquals(FALSE, $this->redis->sMove($key, __FUNCTION__ . 'skey1', 'sValue1'));
        $this->assertEquals(FALSE, $this->redis->sSize($key));
        $this->assertEquals(FALSE, $this->redis->sContains($key, 'sValue1'));
        $this->assertEquals(FALSE, $this->redis->sInter($key, __FUNCTION__ . 'skey2'));
        $this->assertEquals(FALSE, $this->redis->sUnion($key, __FUNCTION__ . 'skey4'));
        $this->assertEquals(FALSE, $this->redis->sDiff($key, __FUNCTION__ . 'skey7'));
        $this->assertEquals(FALSE, $this->redis->sMembers($key));
        $this->assertEquals(FALSE, $this->redis->sRandMember($key));

        // sorted sets I/F
        $this->assertEquals(FALSE, $this->redis->zAdd($key, 1, 'zValue1'));
        $this->assertEquals(FALSE, $this->redis->zDelete($key, 'zValue1'));
        $this->assertEquals(FALSE, $this->redis->zIncrBy($key, 1, 'zValue1'));
        $this->assertEquals(FALSE, $this->redis->zRank($key, 'zValue1'));
        $this->assertEquals(FALSE, $this->redis->zRevRank($key, 'zValue1'));
        $this->assertEquals(FALSE, $this->redis->zRange($key, 0, -1));
        $this->assertEquals(FALSE, $this->redis->zReverseRange($key, 0, -1));
        $this->assertEquals(FALSE, $this->redis->zRangeByScore($key, 1, 2));
        $this->assertEquals(FALSE, $this->redis->zCount($key, 0, -1));
        $this->assertEquals(FALSE, $this->redis->zCard($key));
        $this->assertEquals(FALSE, $this->redis->zScore($key, 'zValue1'));
        $this->assertEquals(FALSE, $this->redis->zDeleteRangeByRank($key, 1, 2));
        $this->assertEquals(FALSE, $this->redis->zDeleteRangeByScore($key, 1, 2));

        // hash I/F
        $this->assertEquals(FALSE, $this->redis->hSet($key, 'key1', 'value1'));
        $this->assertEquals(FALSE, $this->redis->hGet($key, 'key1'));
        $this->assertEquals(FALSE, $this->redis->hMGet($key, array('key1')));
        $this->assertEquals(FALSE, $this->redis->hMSet($key, array('key1' => 'value1')));
        $this->assertEquals(FALSE, $this->redis->hIncrBy($key, 'key2', 1));
        $this->assertEquals(FALSE, $this->redis->hExists($key, 'key2'));
        $this->assertEquals(FALSE, $this->redis->hDel($key, 'key2'));
        $this->assertEquals(FALSE, $this->redis->hLen($key));
        $this->assertEquals(FALSE, $this->redis->hKeys($key));
        $this->assertEquals(FALSE, $this->redis->hVals($key));
        $this->assertEquals(FALSE, $this->redis->hGetAll($key));
    }

    public function testDifferentTypeList() {
        $key = 'list';
        $this->redis->del($key);
        $this->assertEquals(1, $this->redis->lPush($key, 'value'));

        // string I/F
        $this->assertEquals(FALSE, $this->redis->get($key));
        $this->assertEquals(FALSE, $this->redis->getset($key, 'value2'));
        $this->assertEquals(FALSE, $this->redis->append($key, 'append'));
        $this->assertEquals(FALSE, $this->redis->substr($key, 0, 8));
        $this->assertEquals(array(FALSE), $this->redis->mget(array($key)));
        $this->assertEquals(FALSE, $this->redis->incr($key));
        $this->assertEquals(FALSE, $this->redis->incrBy($key, 1));
        $this->assertEquals(FALSE, $this->redis->decr($key));
        $this->assertEquals(FALSE, $this->redis->decrBy($key, 1));

        // sets I/F
        $this->assertEquals(FALSE, $this->redis->sAdd($key, 'sValue1'));
        $this->assertEquals(FALSE, $this->redis->sRemove($key, 'sValue1'));
        $this->assertEquals(FALSE, $this->redis->sPop($key));
        $this->assertEquals(FALSE, $this->redis->sMove($key, __FUNCTION__ . 'skey1', 'sValue1'));
        $this->assertEquals(FALSE, $this->redis->sSize($key));
        $this->assertEquals(FALSE, $this->redis->sContains($key, 'sValue1'));
        $this->assertEquals(FALSE, $this->redis->sInter($key, __FUNCTION__ . 'skey2'));
        $this->assertEquals(FALSE, $this->redis->sUnion($key, __FUNCTION__ . 'skey4'));
        $this->assertEquals(FALSE, $this->redis->sDiff($key, __FUNCTION__ . 'skey7'));
        $this->assertEquals(FALSE, $this->redis->sMembers($key));
        $this->assertEquals(FALSE, $this->redis->sRandMember($key));

        // sorted sets I/F
        $this->assertEquals(FALSE, $this->redis->zAdd($key, 1, 'zValue1'));
        $this->assertEquals(FALSE, $this->redis->zDelete($key, 'zValue1'));
        $this->assertEquals(FALSE, $this->redis->zIncrBy($key, 1, 'zValue1'));
        $this->assertEquals(FALSE, $this->redis->zRank($key, 'zValue1'));
        $this->assertEquals(FALSE, $this->redis->zRevRank($key, 'zValue1'));
        $this->assertEquals(FALSE, $this->redis->zRange($key, 0, -1));
        $this->assertEquals(FALSE, $this->redis->zReverseRange($key, 0, -1));
        $this->assertEquals(FALSE, $this->redis->zRangeByScore($key, 1, 2));
        $this->assertEquals(FALSE, $this->redis->zCount($key, 0, -1));
        $this->assertEquals(FALSE, $this->redis->zCard($key));
        $this->assertEquals(FALSE, $this->redis->zScore($key, 'zValue1'));
        $this->assertEquals(FALSE, $this->redis->zDeleteRangeByRank($key, 1, 2));
        $this->assertEquals(FALSE, $this->redis->zDeleteRangeByScore($key, 1, 2));

        // hash I/F
        $this->assertEquals(FALSE, $this->redis->hSet($key, 'key1', 'value1'));
        $this->assertEquals(FALSE, $this->redis->hGet($key, 'key1'));
        $this->assertEquals(FALSE, $this->redis->hMGet($key, array('key1')));
        $this->assertEquals(FALSE, $this->redis->hMSet($key, array('key1' => 'value1')));
        $this->assertEquals(FALSE, $this->redis->hIncrBy($key, 'key2', 1));
        $this->assertEquals(FALSE, $this->redis->hExists($key, 'key2'));
        $this->assertEquals(FALSE, $this->redis->hDel($key, 'key2'));
        $this->assertEquals(FALSE, $this->redis->hLen($key));
        $this->assertEquals(FALSE, $this->redis->hKeys($key));
        $this->assertEquals(FALSE, $this->redis->hVals($key));
        $this->assertEquals(FALSE, $this->redis->hGetAll($key));
    }

    public function testDifferentTypeSet() {
        $key = 'set';
        $this->redis->del($key);
        $this->assertEquals(1, $this->redis->sAdd($key, 'value'));

        // string I/F
        $this->assertEquals(FALSE, $this->redis->get($key));
        $this->assertEquals(FALSE, $this->redis->getset($key, 'value2'));
        $this->assertEquals(FALSE, $this->redis->append($key, 'append'));
        $this->assertEquals(FALSE, $this->redis->substr($key, 0, 8));
        $this->assertEquals(array(FALSE), $this->redis->mget(array($key)));
        $this->assertEquals(FALSE, $this->redis->incr($key));
        $this->assertEquals(FALSE, $this->redis->incrBy($key, 1));
        $this->assertEquals(FALSE, $this->redis->decr($key));
        $this->assertEquals(FALSE, $this->redis->decrBy($key, 1));

        // lists I/F
        $this->assertEquals(FALSE, $this->redis->rPush($key, 'lvalue'));
        $this->assertEquals(FALSE, $this->redis->lPush($key, 'lvalue'));
        $this->assertEquals(FALSE, $this->redis->lLen($key));
        $this->assertEquals(FALSE, $this->redis->lPop($key));
        $this->assertEquals(FALSE, $this->redis->lGetRange($key, 0, -1));
        $this->assertEquals(FALSE, $this->redis->lTrim($key, 0, 1));
        $this->assertEquals(FALSE, $this->redis->lGet($key, 0));
        $this->assertEquals(FALSE, $this->redis->lSet($key, 0, "newValue"));
        $this->assertEquals(FALSE, $this->redis->lRemove($key, 'lvalue', 1));
        $this->assertEquals(FALSE, $this->redis->lPop($key));
        $this->assertEquals(FALSE, $this->redis->rPop($key));
        $this->assertEquals(FALSE, $this->redis->rPoplPush($key, __FUNCTION__ . 'lkey1'));

        // sorted sets I/F
        $this->assertEquals(FALSE, $this->redis->zAdd($key, 1, 'zValue1'));
        $this->assertEquals(FALSE, $this->redis->zDelete($key, 'zValue1'));
        $this->assertEquals(FALSE, $this->redis->zIncrBy($key, 1, 'zValue1'));
        $this->assertEquals(FALSE, $this->redis->zRank($key, 'zValue1'));
        $this->assertEquals(FALSE, $this->redis->zRevRank($key, 'zValue1'));
        $this->assertEquals(FALSE, $this->redis->zRange($key, 0, -1));
        $this->assertEquals(FALSE, $this->redis->zReverseRange($key, 0, -1));
        $this->assertEquals(FALSE, $this->redis->zRangeByScore($key, 1, 2));
        $this->assertEquals(FALSE, $this->redis->zCount($key, 0, -1));
        $this->assertEquals(FALSE, $this->redis->zCard($key));
        $this->assertEquals(FALSE, $this->redis->zScore($key, 'zValue1'));
        $this->assertEquals(FALSE, $this->redis->zDeleteRangeByRank($key, 1, 2));
        $this->assertEquals(FALSE, $this->redis->zDeleteRangeByScore($key, 1, 2));

        // hash I/F
        $this->assertEquals(FALSE, $this->redis->hSet($key, 'key1', 'value1'));
        $this->assertEquals(FALSE, $this->redis->hGet($key, 'key1'));
        $this->assertEquals(FALSE, $this->redis->hMGet($key, array('key1')));
        $this->assertEquals(FALSE, $this->redis->hMSet($key, array('key1' => 'value1')));
        $this->assertEquals(FALSE, $this->redis->hIncrBy($key, 'key2', 1));
        $this->assertEquals(FALSE, $this->redis->hExists($key, 'key2'));
        $this->assertEquals(FALSE, $this->redis->hDel($key, 'key2'));
        $this->assertEquals(FALSE, $this->redis->hLen($key));
        $this->assertEquals(FALSE, $this->redis->hKeys($key));
        $this->assertEquals(FALSE, $this->redis->hVals($key));
        $this->assertEquals(FALSE, $this->redis->hGetAll($key));
    }

    public function testDifferentTypeSortedSet() {
        $key = 'sortedset';
        $this->redis->del($key);
        $this->assertEquals(1, $this->redis->zAdd($key, 0, 'value'));

        // string I/F
        $this->assertEquals(FALSE, $this->redis->get($key));
        $this->assertEquals(FALSE, $this->redis->getset($key, 'value2'));
        $this->assertEquals(FALSE, $this->redis->append($key, 'append'));
        $this->assertEquals(FALSE, $this->redis->substr($key, 0, 8));
        $this->assertEquals(array(FALSE), $this->redis->mget(array($key)));
        $this->assertEquals(FALSE, $this->redis->incr($key));
        $this->assertEquals(FALSE, $this->redis->incrBy($key, 1));
        $this->assertEquals(FALSE, $this->redis->decr($key));
        $this->assertEquals(FALSE, $this->redis->decrBy($key, 1));

        // lists I/F
        $this->assertEquals(FALSE, $this->redis->rPush($key, 'lvalue'));
        $this->assertEquals(FALSE, $this->redis->lPush($key, 'lvalue'));
        $this->assertEquals(FALSE, $this->redis->lLen($key));
        $this->assertEquals(FALSE, $this->redis->lPop($key));
        $this->assertEquals(FALSE, $this->redis->lGetRange($key, 0, -1));
        $this->assertEquals(FALSE, $this->redis->lTrim($key, 0, 1));
        $this->assertEquals(FALSE, $this->redis->lGet($key, 0));
        $this->assertEquals(FALSE, $this->redis->lSet($key, 0, "newValue"));
        $this->assertEquals(FALSE, $this->redis->lRemove($key, 'lvalue', 1));
        $this->assertEquals(FALSE, $this->redis->lPop($key));
        $this->assertEquals(FALSE, $this->redis->rPop($key));
        $this->assertEquals(FALSE, $this->redis->rPoplPush($key, __FUNCTION__ . 'lkey1'));

        // sets I/F
        $this->assertEquals(FALSE, $this->redis->sAdd($key, 'sValue1'));
        $this->assertEquals(FALSE, $this->redis->sRemove($key, 'sValue1'));
        $this->assertEquals(FALSE, $this->redis->sPop($key));
        $this->assertEquals(FALSE, $this->redis->sMove($key, __FUNCTION__ . 'skey1', 'sValue1'));
        $this->assertEquals(FALSE, $this->redis->sSize($key));
        $this->assertEquals(FALSE, $this->redis->sContains($key, 'sValue1'));
        $this->assertEquals(FALSE, $this->redis->sInter($key, __FUNCTION__ . 'skey2'));
        $this->assertEquals(FALSE, $this->redis->sUnion($key, __FUNCTION__ . 'skey4'));
        $this->assertEquals(FALSE, $this->redis->sDiff($key, __FUNCTION__ . 'skey7'));
        $this->assertEquals(FALSE, $this->redis->sMembers($key));
        $this->assertEquals(FALSE, $this->redis->sRandMember($key));

        // hash I/F
        $this->assertEquals(FALSE, $this->redis->hSet($key, 'key1', 'value1'));
        $this->assertEquals(FALSE, $this->redis->hGet($key, 'key1'));
        $this->assertEquals(FALSE, $this->redis->hMGet($key, array('key1')));
        $this->assertEquals(FALSE, $this->redis->hMSet($key, array('key1' => 'value1')));
        $this->assertEquals(FALSE, $this->redis->hIncrBy($key, 'key2', 1));
        $this->assertEquals(FALSE, $this->redis->hExists($key, 'key2'));
        $this->assertEquals(FALSE, $this->redis->hDel($key, 'key2'));
        $this->assertEquals(FALSE, $this->redis->hLen($key));
        $this->assertEquals(FALSE, $this->redis->hKeys($key));
        $this->assertEquals(FALSE, $this->redis->hVals($key));
        $this->assertEquals(FALSE, $this->redis->hGetAll($key));
    }

    public function testDifferentTypeHash() {
        $key = 'hash';
        $this->redis->del($key);
        $this->assertEquals(1, $this->redis->hSet($key, 'key', 'value'));

        // string I/F
        $this->assertEquals(FALSE, $this->redis->get($key));
        $this->assertEquals(FALSE, $this->redis->getset($key, 'value2'));
        $this->assertEquals(FALSE, $this->redis->append($key, 'append'));
        $this->assertEquals(FALSE, $this->redis->substr($key, 0, 8));
        $this->assertEquals(array(FALSE), $this->redis->mget(array($key)));
        $this->assertEquals(FALSE, $this->redis->incr($key));
        $this->assertEquals(FALSE, $this->redis->incrBy($key, 1));
        $this->assertEquals(FALSE, $this->redis->decr($key));
        $this->assertEquals(FALSE, $this->redis->decrBy($key, 1));

        // lists I/F
        $this->assertEquals(FALSE, $this->redis->rPush($key, 'lvalue'));
        $this->assertEquals(FALSE, $this->redis->lPush($key, 'lvalue'));
        $this->assertEquals(FALSE, $this->redis->lLen($key));
        $this->assertEquals(FALSE, $this->redis->lPop($key));
        $this->assertEquals(FALSE, $this->redis->lGetRange($key, 0, -1));
        $this->assertEquals(FALSE, $this->redis->lTrim($key, 0, 1));
        $this->assertEquals(FALSE, $this->redis->lGet($key, 0));
        $this->assertEquals(FALSE, $this->redis->lSet($key, 0, "newValue"));
        $this->assertEquals(FALSE, $this->redis->lRemove($key, 'lvalue', 1));
        $this->assertEquals(FALSE, $this->redis->lPop($key));
        $this->assertEquals(FALSE, $this->redis->rPop($key));
        $this->assertEquals(FALSE, $this->redis->rPoplPush($key, __FUNCTION__ . 'lkey1'));

        // sets I/F
        $this->assertEquals(FALSE, $this->redis->sAdd($key, 'sValue1'));
        $this->assertEquals(FALSE, $this->redis->sRemove($key, 'sValue1'));
        $this->assertEquals(FALSE, $this->redis->sPop($key));
        $this->assertEquals(FALSE, $this->redis->sMove($key, __FUNCTION__ . 'skey1', 'sValue1'));
        $this->assertEquals(FALSE, $this->redis->sSize($key));
        $this->assertEquals(FALSE, $this->redis->sContains($key, 'sValue1'));
        $this->assertEquals(FALSE, $this->redis->sInter($key, __FUNCTION__ . 'skey2'));
        $this->assertEquals(FALSE, $this->redis->sUnion($key, __FUNCTION__ . 'skey4'));
        $this->assertEquals(FALSE, $this->redis->sDiff($key, __FUNCTION__ . 'skey7'));
        $this->assertEquals(FALSE, $this->redis->sMembers($key));
        $this->assertEquals(FALSE, $this->redis->sRandMember($key));

        // sorted sets I/F
        $this->assertEquals(FALSE, $this->redis->zAdd($key, 1, 'zValue1'));
        $this->assertEquals(FALSE, $this->redis->zDelete($key, 'zValue1'));
        $this->assertEquals(FALSE, $this->redis->zIncrBy($key, 1, 'zValue1'));
        $this->assertEquals(FALSE, $this->redis->zRank($key, 'zValue1'));
        $this->assertEquals(FALSE, $this->redis->zRevRank($key, 'zValue1'));
        $this->assertEquals(FALSE, $this->redis->zRange($key, 0, -1));
        $this->assertEquals(FALSE, $this->redis->zReverseRange($key, 0, -1));
        $this->assertEquals(FALSE, $this->redis->zRangeByScore($key, 1, 2));
        $this->assertEquals(FALSE, $this->redis->zCount($key, 0, -1));
        $this->assertEquals(FALSE, $this->redis->zCard($key));
        $this->assertEquals(FALSE, $this->redis->zScore($key, 'zValue1'));
        $this->assertEquals(FALSE, $this->redis->zDeleteRangeByRank($key, 1, 2));
        $this->assertEquals(FALSE, $this->redis->zDeleteRangeByScore($key, 1, 2));
    }

    public function testSerializerPHP() {

	    $this->checkSerializer(Redis::SERIALIZER_PHP);

	    // with prefix
	    $this->redis->setOption(Redis::OPT_PREFIX, "test:");
	    $this->checkSerializer(Redis::SERIALIZER_PHP);
	    $this->redis->setOption(Redis::OPT_PREFIX, "");
    }

    public function testSerializerIGBinary() {

	    if(defined('Redis::SERIALIZER_IGBINARY')) {
		    $this->checkSerializer(Redis::SERIALIZER_IGBINARY);

		    // with prefix
		    $this->redis->setOption(Redis::OPT_PREFIX, "test:");
		    $this->checkSerializer(Redis::SERIALIZER_IGBINARY);
		    $this->redis->setOption(Redis::OPT_PREFIX, "");
	    }
    }

    private function checkSerializer($mode) {

	    $this->redis->delete('key');
	    $this->assertTrue($this->redis->getOption(Redis::OPT_SERIALIZER) === Redis::SERIALIZER_NONE); 	// default

	    $this->assertTrue($this->redis->setOption(Redis::OPT_SERIALIZER, $mode) === TRUE); 	// set ok
	    $this->assertTrue($this->redis->getOption(Redis::OPT_SERIALIZER) === $mode);	// get ok

	    // lPush, rPush
	    $a = array('hello world', 42, TRUE, array('<tag>' => 1729));
	    $this->redis->delete('key');
	    $this->redis->lPush('key', $a[0]);
	    $this->redis->rPush('key', $a[1]);
	    $this->redis->rPush('key', $a[2]);
	    $this->redis->rPush('key', $a[3]);

	    // lGetRange
	    $this->assertTrue($a === $this->redis->lGetRange('key', 0, -1));

	    // lGet
	    $this->assertTrue($a[0] === $this->redis->lGet('key', 0));
	    $this->assertTrue($a[1] === $this->redis->lGet('key', 1));
	    $this->assertTrue($a[2] === $this->redis->lGet('key', 2));
	    $this->assertTrue($a[3] === $this->redis->lGet('key', 3));

	    // lRemove
	    $this->assertTrue($this->redis->lRemove('key', $a[3]) === 1);
	    $this->assertTrue(array_slice($a, 0, 3) === $this->redis->lGetRange('key', 0, -1));

	    // lSet
	    $a[0] = array('k' => 'v'); // update
	    $this->assertTrue(TRUE === $this->redis->lSet('key', 0, $a[0]));
	    $this->assertTrue($a[0] === $this->redis->lGet('key', 0));

	    // lInsert
	    $this->assertTrue($this->redis->lInsert('key', Redis::BEFORE, $a[0], array(1,2,3)) === 4);
	    $this->assertTrue($this->redis->lInsert('key', Redis::AFTER, $a[0], array(4,5,6)) === 5);

	    $a = array(array(1,2,3), $a[0], array(4,5,6), $a[1], $a[2]);
	    $this->assertTrue($a === $this->redis->lGetRange('key', 0, -1));

	    // sAdd
	    $this->redis->delete('key');
	    $s = array(1,'a', array(1,2,3), array('k' => 'v'));

	    $this->assertTrue(1 === $this->redis->sAdd('key', $s[0]));
	    $this->assertTrue(1 === $this->redis->sAdd('key', $s[1]));
	    $this->assertTrue(1 === $this->redis->sAdd('key', $s[2]));
	    $this->assertTrue(1 === $this->redis->sAdd('key', $s[3]));

	    // variadic sAdd
	    $this->redis->delete('k');
	    $this->assertTrue(3 === $this->redis->sAdd('k', 'a', 'b', 'c'));
	    $this->assertTrue(1 === $this->redis->sAdd('k', 'a', 'b', 'c', 'd'));

	    // sRemove
	    $this->assertTrue(1 === $this->redis->sRemove('key', $s[3]));
	    $this->assertTrue(0 === $this->redis->sRemove('key', $s[3]));
	    // variadic
	    $this->redis->delete('k');
	    $this->redis->sAdd('k', 'a', 'b', 'c', 'd');
	    $this->assertTrue(2 === $this->redis->sRem('k', 'a', 'd'));
	    $this->assertTrue(2 === $this->redis->sRem('k', 'b', 'c', 'e'));
	    $this->assertTrue(FALSE === $this->redis->exists('k'));

	    // sContains
	    $this->assertTrue(TRUE === $this->redis->sContains('key', $s[0]));
	    $this->assertTrue(TRUE === $this->redis->sContains('key', $s[1]));
	    $this->assertTrue(TRUE === $this->redis->sContains('key', $s[2]));
	    $this->assertTrue(FALSE === $this->redis->sContains('key', $s[3]));
	    unset($s[3]);

	    // sMove
	    $this->redis->delete('tmp');
	    $this->redis->sMove('key', 'tmp', $s[0]);
	    $this->assertTrue(FALSE === $this->redis->sContains('key', $s[0]));
	    $this->assertTrue(TRUE === $this->redis->sContains('tmp', $s[0]));
	    unset($s[0]);


	    // sorted sets
	    $z = array('z0', array('k' => 'v'), FALSE, NULL);
	    $this->redis->delete('key');

	    // zAdd
	    $this->assertTrue(1 === $this->redis->zAdd('key', 0, $z[0]));
	    $this->assertTrue(1 === $this->redis->zAdd('key', 1, $z[1]));
	    $this->assertTrue(1 === $this->redis->zAdd('key', 2, $z[2]));
	    $this->assertTrue(1 === $this->redis->zAdd('key', 3, $z[3]));

	    // zDelete
	    $this->assertTrue(1 === $this->redis->zDelete('key', $z[3]));
	    $this->assertTrue(0 === $this->redis->zDelete('key', $z[3]));
	    unset($z[3]);

		// check that zDelete doesn't crash with a missing parameter (GitHub issue #102):
		$this->assertTrue(FALSE === @$this->redis->zDelete('key'));

	    // variadic
	    $this->redis->delete('k');
	    $this->redis->zAdd('k', 0, 'a');
	    $this->redis->zAdd('k', 1, 'b');
	    $this->redis->zAdd('k', 2, 'c');
	    $this->assertTrue(2 === $this->redis->zDelete('k', 'a', 'c'));
	    $this->assertTrue(1.0 === $this->redis->zScore('k', 'b'));
	    $this->assertTrue($this->redis->zRange('k', 0, -1, true) == array('b' => 1.0));

	    // zRange
	    $this->assertTrue($z === $this->redis->zRange('key', 0, -1));

	    // zScore
	    $this->assertTrue(0.0 === $this->redis->zScore('key', $z[0]));
	    $this->assertTrue(1.0 === $this->redis->zScore('key', $z[1]));
	    $this->assertTrue(2.0 === $this->redis->zScore('key', $z[2]));

	    // zRank
	    $this->assertTrue(0 === $this->redis->zRank('key', $z[0]));
	    $this->assertTrue(1 === $this->redis->zRank('key', $z[1]));
	    $this->assertTrue(2 === $this->redis->zRank('key', $z[2]));

	    // zRevRank
	    $this->assertTrue(2 === $this->redis->zRevRank('key', $z[0]));
	    $this->assertTrue(1 === $this->redis->zRevRank('key', $z[1]));
	    $this->assertTrue(0 === $this->redis->zRevRank('key', $z[2]));

	    // zIncrBy
	    $this->assertTrue(3.0 === $this->redis->zIncrBy('key', 1.0, $z[2]));
	    $this->assertTrue(3.0 === $this->redis->zScore('key', $z[2]));

	    $this->assertTrue(5.0 === $this->redis->zIncrBy('key', 2.0, $z[2]));
	    $this->assertTrue(5.0 === $this->redis->zScore('key', $z[2]));

	    $this->assertTrue(2.0 === $this->redis->zIncrBy('key', -3.0, $z[2]));
	    $this->assertTrue(2.0 === $this->redis->zScore('key', $z[2]));

	    // mset
	    $a = array('k0' => 1, 'k1' => 42, 'k2' => NULL, 'k3' => FALSE, 'k4' => array('a' => 'b'));
	    $this->assertTrue(TRUE === $this->redis->mset($a));
	    foreach($a as $k => $v) {
		    $this->assertTrue($this->redis->get($k) === $v);
	    }

	    $a = array('k0' => 1, 'k1' => 42, 'k2' => NULL, 'k3' => FALSE, 'k4' => array('a' => 'b'));

	    // hSet
	    $this->redis->delete('key');
	    foreach($a as $k => $v) {
		    $this->assertTrue(1 === $this->redis->hSet('key', $k, $v));
	    }

	    // hGet
	    foreach($a as $k => $v) {
		    $this->assertTrue($v === $this->redis->hGet('key', $k));
	    }

	    // hGetAll
	    $this->assertTrue($a === $this->redis->hGetAll('key'));
	    $this->assertTrue(TRUE === $this->redis->hExists('key', 'k0'));
	    $this->assertTrue(TRUE === $this->redis->hExists('key', 'k1'));
	    $this->assertTrue(TRUE === $this->redis->hExists('key', 'k2'));
	    $this->assertTrue(TRUE === $this->redis->hExists('key', 'k3'));
	    $this->assertTrue(TRUE === $this->redis->hExists('key', 'k4'));

	    // hMSet
	    $this->redis->delete('key');
	    $this->redis->hMSet('key', $a);
	    foreach($a as $k => $v) {
		    $this->assertTrue($v === $this->redis->hGet('key', $k));
	    }

	    // hMget
	    $hmget = $this->redis->hMget('key', array_keys($a));
	    foreach($hmget as $k => $v) {
		    $this->assertTrue($v === $a[$k]);
	    }


	    // getMultiple
	    $this->redis->set('a', NULL);
	    $this->redis->set('b', FALSE);
	    $this->redis->set('c', 42);
	    $this->redis->set('d', array('x' => 'y'));
	    $this->assertTrue(array(NULL, FALSE, 42, array('x' => 'y')) === $this->redis->getMultiple(array('a', 'b', 'c', 'd')));

	    // pipeline
	    $this->sequence(Redis::PIPELINE);

	    // multi-exec
	    $this->sequence(Redis::MULTI);

	    // keys
	    $this->assertTrue(is_array($this->redis->keys('*')));

		// issue #62, hgetall
		$this->redis->del('hash1');
		$this->redis->hSet('hash1','data', 'test 1');
		$this->redis->hSet('hash1','session_id', 'test 2');

		$data = $this->redis->hGetAll('hash1');
		$this->assertTrue($data['data'] === 'test 1');
		$this->assertTrue($data['session_id'] === 'test 2');

		// issue #145, serializer with objects.
		$this->redis->set('x', array(new stdClass, new stdClass));
		$x = $this->redis->get('x');
		$this->assertTrue(is_array($x));
		$this->assertTrue(is_object($x[0]) && get_class($x[0]) === 'stdClass');
		$this->assertTrue(is_object($x[1]) && get_class($x[1]) === 'stdClass');

	    // revert
	    $this->assertTrue($this->redis->setOption(Redis::OPT_SERIALIZER, Redis::SERIALIZER_NONE) === TRUE); 	// set ok
	    $this->assertTrue($this->redis->getOption(Redis::OPT_SERIALIZER) === Redis::SERIALIZER_NONE);		// get ok
    }

    public function testDumpRestore() {

		if (version_compare($this->version, "2.5.0", "lt")) {
			$this->markTestSkipped();
		}

    	$this->redis->del('foo');
    	$this->redis->del('bar');

    	$this->redis->set('foo', 'this-is-foo');
    	$this->redis->set('bar', 'this-is-bar');

    	$d_foo = $this->redis->dump('foo');
    	$d_bar = $this->redis->dump('bar');

    	$this->redis->del('foo');
    	$this->redis->del('bar');

    	// Assert returns from restore
    	$this->assertTrue($this->redis->restore('foo', 0, $d_bar));
    	$this->assertTrue($this->redis->restore('bar', 0, $d_foo));

    	// Now check that the keys have switched
    	$this->assertTrue($this->redis->get('foo') === 'this-is-bar');
    	$this->assertTrue($this->redis->get('bar') === 'this-is-foo');

    	$this->redis->del('foo');
    	$this->redis->del('bar');
    }

    public function testGetLastError() {
    	// We shouldn't have any errors now
    	$this->assertTrue($this->redis->getLastError() === NULL);

    	// Throw some invalid lua at redis
    	$this->redis->eval("not-a-lua-script");

    	// Now we should have an error
		$evalError = $this->redis->getLastError();
		$this->assertTrue(strlen($evalError) > 0);

		// test getLastError with a regular command
		$this->redis->set('x', 'a');
		$this->assertFalse($this->redis->incr('x'));
		$incrError = $this->redis->getLastError();
		$this->assertTrue($incrError !== $evalError); // error has changed
		$this->assertTrue(strlen($incrError) > 0);

		// clear error
		$this->redis->clearLastError();
		$this->assertTrue($this->redis->getLastError() === NULL);
	}

    // Helper function to compare nested results -- from the php.net array_diff page, I believe
    private function array_diff_recursive($aArray1, $aArray2) {
    	$aReturn = array();

    	foreach ($aArray1 as $mKey => $mValue) {
    		if (array_key_exists($mKey, $aArray2)) {
    			if (is_array($mValue)) {
    				$aRecursiveDiff = $this->array_diff_recursive($mValue, $aArray2[$mKey]);
    				if (count($aRecursiveDiff)) {
    					$aReturn[$mKey] = $aRecursiveDiff;
    				}
    			} else {
    				if ($mValue != $aArray2[$mKey]) {
    					$aReturn[$mKey] = $mValue;
    				}
    			}
    		} else {
    			$aReturn[$mKey] = $mValue;
    		}
    	}

    	return $aReturn;
    }

    public function testScript() {

		if (version_compare($this->version, "2.5.0", "lt")) {
			$this->markTestSkipped();
		}

		// Flush any scripts we have
		$this->assertTrue($this->redis->script('flush'));

		// Silly scripts to test against
		$s1_src = 'return 1';
		$s1_sha = sha1($s1_src);
		$s2_src = 'return 2';
		$s2_sha = sha1($s2_src);
		$s3_src = 'return 3';
		$s3_sha = sha1($s3_src);

    	// None should exist
		$result = $this->redis->script('exists', $s1_sha, $s2_sha, $s3_sha);
		$this->assertTrue(is_array($result) && count($result) == 3);
		$this->assertTrue(is_array($result) && count(array_filter($result)) == 0);

		// Load them up
		$this->assertTrue($this->redis->script('load', $s1_src) == $s1_sha);
		$this->assertTrue($this->redis->script('load', $s2_src) == $s2_sha);
		$this->assertTrue($this->redis->script('load', $s3_src) == $s3_sha);

		// They should all exist
		$result = $this->redis->script('exists', $s1_sha, $s2_sha, $s3_sha);
		$this->assertTrue(is_array($result) && count(array_filter($result)) == 3);
    }

    public function testEval() {

		if (version_compare($this->version, "2.5.0", "lt")) {
			$this->markTestSkipped();
		}

    	// Basic single line response tests
    	$this->assertTrue(1 == $this->redis->eval('return 1'));
    	$this->assertTrue(1.55 == $this->redis->eval("return '1.55'"));
    	$this->assertTrue("hello, world" == $this->redis->eval("return 'hello, world'"));

    	/*
    	 * Keys to be incorporated into lua results
    	 */

		// Make a list
		$this->redis->del('mylist');
		$this->redis->rpush('mylist', 'a');
		$this->redis->rpush('mylist', 'b');
		$this->redis->rpush('mylist', 'c');

		// Make a set
		$this->redis->del('myset');
		$this->redis->sadd('myset', 'd');
		$this->redis->sadd('myset', 'e');
		$this->redis->sadd('myset', 'f');

		// Basic keys
		$this->redis->set('key1', 'hello, world');
		$this->redis->set('key2', 'hello again!');

		// Use a script to return our list, and verify its response
		$list = $this->redis->eval("return redis.call('lrange', 'mylist', 0, -1)");
		$this->assertTrue($list === Array('a','b','c'));

		// Use a script to return our set
		$set = $this->redis->eval("return redis.call('smembers', 'myset')");
		$this->assertTrue($set == Array('d','e','f'));

		// Test an empty MULTI BULK response
		$this->redis->del('not-any-kind-of-list');
		$empty_resp = $this->redis->eval("return redis.call('lrange', 'not-any-kind-of-list', 0, -1)");
		$this->assertTrue(is_array($empty_resp) && empty($empty_resp));

		// Now test a nested reply
		$nested_script = "
			return {
				1,2,3, {
					redis.call('get', 'key1'),
					redis.call('get', 'key2'),
					redis.call('lrange', 'not-any-kind-of-list', 0, -1),
					{
						redis.call('smembers','myset'),
						redis.call('lrange', 'mylist', 0, -1)
					}
				}
			}
		";

		$expected = Array(
			1, 2, 3, Array(
				'hello, world',
				'hello again!',
				Array(),
				Array(
					Array('d','e','f'),
					Array('a','b','c')
				)
			)
		);

		// Now run our script, and check our values against each other
		$eval_result = $this->redis->eval($nested_script);
		$this->assertTrue(is_array($eval_result) && count($this->array_diff_recursive($eval_result, $expected)) == 0);

		/*
		 * Nested reply wihin a multi/pipeline block
		 */

		$num_scripts = 10;

		foreach(Array(Redis::PIPELINE, Redis::MULTI) as $mode) {
			$this->redis->multi($mode);
			for($i=0;$i<$num_scripts;$i++) {
				$this->redis->eval($nested_script);
			}
			$replies = $this->redis->exec();

			foreach($replies as $reply) {
				$this->assertTrue(is_array($reply) && count($this->array_diff_recursive($reply, $expected)) == 0);
			}
		}

		/*
		 * KEYS/ARGV
		 */

		$args_script = "return {KEYS[1],KEYS[2],KEYS[3],ARGV[1],ARGV[2],ARGV[3]}";
		$args_args   = Array('k1','k2','k3','v1','v2','v3');
		$args_result = $this->redis->eval($args_script, $args_args, 3);
		$this->assertTrue($args_result === $args_args);

		// turn on key prefixing
		$this->redis->setOption(Redis::OPT_PREFIX, 'prefix:');
		$args_result = $this->redis->eval($args_script, $args_args, 3);

		// Make sure our first three are prefixed
		for($i=0;$i<count($args_result);$i++) {
			if($i<3) {
				// Should be prefixed
				$this->assertTrue($args_result[$i] == 'prefix:' . $args_args[$i]);
			} else {
				// Should not be prefixed
				$this->assertTrue($args_result[$i] == $args_args[$i]);
			}
		}
    }

    public function testEvalSHA() {

		if (version_compare($this->version, "2.5.0", "lt")) {
			$this->markTestSkipped();
		}

    	// Flush any loaded scripts
    	$this->redis->script('flush');

    	// Non existant script (but proper sha1), and a random (not) sha1 string
    	$this->assertFalse($this->redis->evalsha(sha1(uniqid())));
    	$this->assertFalse($this->redis->evalsha('some-random-data'));

    	// Load a script
    	$cb  = uniqid(); // To ensure the script is new
    	$scr = "local cb='$cb' return 1";
    	$sha = sha1($scr);

    	// Run it when it doesn't exist, run it with eval, and then run it with sha1
    	$this->assertTrue(false === $this->redis->evalsha($scr));
    	$this->assertTrue(1 === $this->redis->eval($scr));
    	$this->assertTrue(1 === $this->redis->evalsha($sha));
    }

    public function testUnserialize() {
    	$vals = Array(
    		1,1.5,'one',Array('this','is','an','array')
    	);

    	$serializers = Array(Redis::SERIALIZER_PHP);
    	if(defined('Redis::SERIALIZER_IGBINARY')) {
    		$serializers[] = Redis::SERIALIZER_IGBINARY;
    	}

    	foreach($serializers as $mode) {
    		$vals_enc = Array();

    		// Pass them through redis so they're serialized
    		foreach($vals as $key => $val) {
    			$this->redis->setOption(Redis::OPT_SERIALIZER, $mode);

    			$key = "key" . ++$key;
    			$this->redis->del($key);
    			$this->redis->set($key, $val);

    			// Clear serializer, get serialized value
    			$this->redis->setOption(Redis::OPT_SERIALIZER, Redis::SERIALIZER_NONE);
    			$vals_enc[] = $this->redis->get($key);
    		}

    		// Run through our array comparing values
    		for($i=0;$i<count($vals);$i++) {
    			// reset serializer
    			$this->redis->setOption(Redis::OPT_SERIALIZER, $mode);
    			$this->assertTrue($vals[$i] == $this->redis->_unserialize($vals_enc[$i]));
    			$this->redis->setOption(Redis::OPT_SERIALIZER, Redis::SERIALIZER_NONE);
    		}
    	}
    }

    public function testPrefix() {
    	// no prefix
    	$this->redis->setOption(Redis::OPT_PREFIX, '');
    	$this->assertTrue('key' == $this->redis->_prefix('key'));

    	// with a prefix
    	$this->redis->setOption(Redis::OPT_PREFIX, 'some-prefix:');
    	$this->assertTrue('some-prefix:key' == $this->redis->_prefix('key'));

    	// Clear prefix
    	$this->redis->setOption(Redis::OPT_PREFIX, '');

    }

	public function testReconnectSelect() {
		$key = 'reconnect-select';
		$value = 'Has been set!';

		$original_cfg = $this->redis->config('GET', 'timeout');

		// Make sure the default DB doesn't have the key.
		$this->redis->select(0);
		$this->redis->delete($key);

		// Set the key on a different DB.
		$this->redis->select(5);
		$this->redis->set($key, $value);

		// Time out after 1 second.
		$this->redis->config('SET', 'timeout', '1');

		// Wait for the timeout. With Redis 2.4, we have to wait up to 10 s
		// for the server to close the connection, regardless of the timeout
		// setting.
		sleep(11);

		// Make sure we're still using the same DB.
		$this->assertEquals($value, $this->redis->get($key));

		// Revert the setting.
		$this->redis->config('SET', 'timeout', $original_cfg['timeout']);
	}

	public function testTime() {

		if (version_compare($this->version, "2.5.0", "lt")) {
			$this->markTestSkipped();
		}

		$time_arr = $this->redis->time();
		$this->assertTrue(is_array($time_arr) && count($time_arr) == 2 &&
				          strval(intval($time_arr[0])) === strval($time_arr[0]) &&
						  strval(intval($time_arr[1])) === strval($time_arr[1]));
	}

	public function testReadTimeoutOption() {

		$this->assertTrue(defined('Redis::OPT_READ_TIMEOUT'));

		$this->redis->setOption(Redis::OPT_READ_TIMEOUT, "12.3");
		$this->assertEquals(12.3, $this->redis->getOption(Redis::OPT_READ_TIMEOUT));
	}

    public function testIntrospection() {
        // Simple introspection tests
        $this->assertTrue($this->redis->getHost() === self::HOST);
        $this->assertTrue($this->redis->getPort() === self::PORT);
        $this->assertTrue($this->redis->getAuth() === self::AUTH);
    }

}

exit(TestSuite::run("Redis_Test"));

?>
