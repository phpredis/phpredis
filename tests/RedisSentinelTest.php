<?php defined('PHPREDIS_TESTRUN') or die("Use TestRedis.php to run tests!\n");

require_once(dirname($_SERVER['PHP_SELF'])."/TestSuite.php");

class Redis_Sentinel_Test extends TestSuite
{
    const NAME = 'mymaster';

    /**
     * @var RedisSentinel
     */
    public $sentinel;

    /**
     * Common fields
     */
    protected $fields = [
        'name',
        'ip',
        'port',
        'runid',
        'flags',
        'link-pending-commands',
        'link-refcount',
        'last-ping-sent',
        'last-ok-ping-reply',
        'last-ping-reply',
        'down-after-milliseconds',
    ];

    protected function newInstance()
    {
        return new RedisSentinel($this->getHost());
    }

    public function setUp()
    {
        $this->sentinel = $this->newInstance();
    }

    public function testCkquorum()
    {
        $this->assertTrue($this->sentinel->ckquorum(self::NAME));
    }

    public function testFailover()
    {
        $this->assertFalse($this->sentinel->failover(self::NAME));
    }

    public function testFlushconfig()
    {
        $this->assertTrue($this->sentinel->flushconfig());
    }

    public function testGetMasterAddrByName()
    {
        $result = $this->sentinel->getMasterAddrByName(self::NAME);
        $this->assertTrue(is_array($result));
        $this->assertEquals(2, count($result));
    }

    protected function checkFields(array $fields)
    {
        foreach ($this->fields as $k) {
            $this->assertTrue(array_key_exists($k, $fields));
        }
    }

    public function testMaster()
    {
        $result = $this->sentinel->master(self::NAME);
        $this->assertTrue(is_array($result));
        $this->checkFields($result);
    }

    public function testMasters()
    {
        $result = $this->sentinel->masters();
        $this->assertTrue(is_array($result));
        foreach ($result as $master) {
            $this->checkFields($master);
        }
    }

    public function testMyid()
    {
        $result = $this->sentinel->myid();
        $this->assertTrue(is_string($result));
    }

    public function testPing()
    {
        $this->assertTrue($this->sentinel->ping());
    }

    public function testReset()
    {
        $this->assertFalse($this->sentinel->reset('*'));
    }

    public function testSentinels()
    {
        $result = $this->sentinel->sentinels(self::NAME);
        $this->assertTrue(is_array($result));
        foreach ($result as $sentinel) {
            $this->checkFields($sentinel);
        }
    }

    public function testSlaves()
    {
        $result = $this->sentinel->slaves(self::NAME);
        $this->assertTrue(is_array($result));
        foreach ($result as $slave) {
            $this->checkFields($slave);
        }
    }
}
