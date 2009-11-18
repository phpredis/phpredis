<?php
require_once 'PHPUnit/Framework/TestCase.php';

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

    public function testSet()
    {
      	$this->assertEquals(TRUE, $this->redis->set('key', 'nil'));
	$this->assertEquals('nil', $this->redis->get('key'));	

      	$this->assertEquals(TRUE, $this->redis->set('key', 'val'));

	$this->assertEquals('val', $this->redis->get('key'));	
	$this->assertEquals('val', $this->redis->get('key'));	
	$this->assertEquals(FALSE, $this->redis->get('keyNotExist'));

	$this->tearDown();
	$this->setUp();
	
        $this->redis->set('key2', 'val');
	$this->assertEquals('val', $this->redis->get('key2'));
   
    	$this->tearDown();
    	$this->setUp();      

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

    public function testMultiple() {
      
    	$this->redis->delete('k1');
      	$this->redis->delete('k2');
      	$this->redis->delete('k3');

	$this->redis->set('k1', 'v1');
	$this->redis->set('k2', 'v2');
	$this->redis->set('k3', 'v3');

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
        $this->redis->delete($key);
        $this->assertEquals(null, $this->redis->get($key));
    }
    
    public function testType()
    {
      /*
	0 => none, (key didn't exist)
	1=> string,
	2 => set,
	3 => list
      */

      	/* string */	
        $this->redis->set('key', 'val');
        $this->assertEquals(1, $this->redis->type('key'));	

	/* list*/
	$this->redis->lPush('keyList', "val0");
	$this->redis->lPush('keyList', "val1");
	$this->assertEquals(3, $this->redis->type('keyList'));

	/* None */
        $this->assertEquals(0, $this->redis->type('keyNotExists'));
    }

    /* PUSH, POP : LPUSH, LPOP */
    public function testlPop()
    {
	/*
		rpush  => tail
		lpush => head
	*/

        $this->redis->delete('list');

        $this->redis->lPush('list', 'val');
        $this->redis->lPush('list', 'val2');
	$this->redis->rPush('list', 'val3');

	/* 'list' = [ 'val2', 'val', 'val3'] */

	$this->assertEquals('val2', $this->redis->lPop('list'));
        $this->assertEquals('val', $this->redis->lPop('list'));
        $this->assertEquals('val3', $this->redis->lPop('list'));
        $this->assertEquals(FALSE, $this->redis->lPop('list'));

	/* testing binary data */

	$this->redis->delete('list');
        $this->assertEquals(TRUE, $this->redis->lPush('list', gzcompress('val1')));
        $this->assertEquals(TRUE, $this->redis->lPush('list', gzcompress('val2')));
        $this->assertEquals(TRUE, $this->redis->lPush('list', gzcompress('val3')));

	$this->assertEquals('val3', gzuncompress($this->redis->lPop('list')));
	$this->assertEquals('val2', gzuncompress($this->redis->lPop('list')));
	$this->assertEquals('val1', gzuncompress($this->redis->lPop('list')));
	
    }
    
    /* PUSH, POP : RPUSH, RPOP */
    public function testrPop()
    {
	/*
		rpush  => tail
		lpush => head
	*/

        $this->redis->delete('list');

        $this->redis->rPush('list', 'val');
        $this->redis->rPush('list', 'val2');
	$this->redis->lPush('list', 'val3');

	/* 'list' = [ 'val3', 'val', 'val2'] */

	$this->assertEquals('val2', $this->redis->rPop('list'));
        $this->assertEquals('val', $this->redis->rPop('list'));
        $this->assertEquals('val3', $this->redis->rPop('list'));
        $this->assertEquals(FALSE, $this->redis->rPop('list'));

	/* testing binary data */

	$this->redis->delete('list');
        $this->assertEquals(TRUE, $this->redis->rPush('list', gzcompress('val1')));
        $this->assertEquals(TRUE, $this->redis->rPush('list', gzcompress('val2')));
        $this->assertEquals(TRUE, $this->redis->rPush('list', gzcompress('val3')));

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
    

    /* ltrim, lsize, lpop */
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
    
    /* LINDEX */
    public function testlGet()
    {

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

    /* lRem testing */
    public function testlRemove() 
    {
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

        $this->redis->sAdd('set', 'val');

        $this->assertTrue($this->redis->sContains('set', 'val'));
        $this->assertFalse($this->redis->sContains('set', 'val2'));

        $this->redis->sAdd('set', 'val2');

        $this->assertTrue($this->redis->sContains('set', 'val2'));
    }
    public function testsSize()
    {
        $this->redis->delete('set');

        $this->redis->sAdd('set', 'val');
        
        $this->assertEquals(1, $this->redis->sSize('set'));

        $this->redis->sAdd('set', 'val2');
        
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

    }
}

?>
