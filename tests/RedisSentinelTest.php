<?php defined('PHPREDIS_TESTRUN') or die("Use TestRedis.php to run tests!\n");

require_once __DIR__ . "/TestSuite.php";

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
        return new RedisSentinel(['host' => $this->getHost()]);
    }

    public function setUp()
    {
        $this->sentinel = $this->newInstance();
    }

    public function testNotSerializable()
    {
        $this->assertNotSerializable($this->sentinel);
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
        $this->assertIsArray($result, 2);
    }

    protected function checkFields($fields)
    {
        $this->assertIsArray($fields);

        foreach ($this->fields as $k) {
            $this->assertArrayKey($fields, $k);
        }
    }

    public function testMaster()
    {
        $result = $this->sentinel->master(self::NAME);
        $this->checkFields($result);
    }

    public function testMasters()
    {
        $result = $this->sentinel->masters();
        $this->assertIsArray($result);
        foreach ($result as $master) {
            $this->checkFields($master);
        }
    }

    public function testMyid()
    {
        $result = $this->sentinel->myid();
        $this->assertIsString($result);
    }

    public function testPing()
    {
        $this->assertTrue($this->sentinel->ping());
    }

    public function testReset()
    {
        $this->assertEquals(1, $this->sentinel->reset('*'));
    }

    public function testSentinels()
    {
        $result = $this->sentinel->sentinels(self::NAME);
        $this->assertIsArray($result);
        foreach ($result as $sentinel) {
            $this->checkFields($sentinel);
        }
    }

    public function testSlaves()
    {
        $result = $this->sentinel->slaves(self::NAME);
        $this->assertIsArray($result);
        foreach ($result as $slave) {
            $this->checkFields($slave);
        }
    }

    protected function getClients(Redis $redis, string $cmd)
    {
        $result = [];

        $clients = $redis->client('list');
        $this->assertIsArray($clients);

        foreach ($clients as $client) {
            $this->assertArrayKey($client, 'cmd');
            $this->assertArrayKey($client, 'id');

            if ($client['cmd'] !== $cmd)
                continue;

            $result[] = $client['id'];
        }

        return $result;
    }

    public function testPersistent() {
        /* I think the tests just use the default port */
        $redis = new Redis;
        $redis->connect($this->getHost(), 26379);

        $id = null;

        for ($i = 0; $i < 3; $i++) {
            $sentinel = new RedisSentinel([
                'host' => $this->getHost(),
                'persistent' => 'sentinel',
            ]);

            $this->assertTrue($sentinel->ping());

            $clients = $this->getClients($redis, 'ping');
            $this->assertArrayKey($clients, 0);

            /* Capture the ping client */
            $id ??= $clients[0];

            unset($sentinel);
        }

        /* The same client should have been reused */
        $this->assertArrayKeyEquals($clients, 0, $id);
    }
}
