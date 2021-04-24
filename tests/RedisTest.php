<?php defined('PHPREDIS_TESTRUN') or die("Use TestRedis.php to run tests!\n");

require_once(dirname($_SERVER['PHP_SELF'])."/TestSuite.php");

class Redis_Test extends TestSuite
{
    /* City lat/long */
    protected $cities = [
        'Chico'         => [-121.837478, 39.728494],
        'Sacramento'    => [-121.494400, 38.581572],
        'Gridley'       => [-121.693583, 39.363777],
        'Marysville'    => [-121.591355, 39.145725],
        'Cupertino'     => [-122.032182, 37.322998]
    ];

    protected $serializers = [
        Redis::SERIALIZER_NONE,
        Redis::SERIALIZER_PHP,
    ];

    /**
     * @var Redis
     */
    public $redis;

    /**
     * @var string
     */
    protected $sessionPrefix = 'PHPREDIS_SESSION:';

    /**
     * @var string
     */
    protected $sessionSaveHandler = 'redis';

    public function setUp() {
        $this->redis = $this->newInstance();
        $info = $this->redis->info();
        $this->version = (isset($info['redis_version'])?$info['redis_version']:'0.0.0');

        if (defined('Redis::SERIALIZER_IGBINARY')) {
            $this->serializers[] = Redis::SERIALIZER_IGBINARY;
        }
    }

    protected function minVersionCheck($version) {
        return version_compare($this->version, $version) >= 0;
    }

    protected function mstime() {
        return round(microtime(true)*1000);
    }

    protected function getAuthParts(&$user, &$pass) {
        $user = $pass = NULL;

        $auth = $this->getAuth();
        if ( ! $auth)
            return;

        if (is_array($auth)) {
            if (count($auth) > 1) {
                list($user, $pass) = $auth;
            } else {
                $pass = $auth[0];
            }
        } else {
            $pass = $auth;
        }
    }

    protected function getAuthFragment() {
        static $_authidx = 0;
        $_authidx++;

        $this->getAuthParts($user, $pass);

        if ($user && $pass) {
            if ($_authidx % 2 == 0)
                return "auth[user]=$user&auth[pass]=$pass";
            else
                return "auth[]=$user&auth[]=$pass";
        } else if ($pass) {
            if ($_authidx % 3 == 0)
                return "auth[pass]=$pass";
            if ($_authidx % 2 == 0)
                return "auth[]=$pass";
            else
                return "auth=$pass";
        } else {
            return NULL;
        }
    }

    protected function getFullHostPath()
    {
        $fullHostPath = parent::getFullHostPath();
        $authFragment = $this->getAuthFragment();

        if (isset($fullHostPath) && $authFragment) {
            $fullHostPath .= "?$authFragment";
        }
        return $fullHostPath;
    }

    protected function newInstance() {
        $r = new Redis();

        $r->connect($this->getHost(), $this->getPort());

        if($this->getAuth()) {
            $this->assertTrue($r->auth($this->getAuth()));
        }
        return $r;
    }

    public function tearDown() {
        if($this->redis) {
            $this->redis->close();
        }
    }

    public function reset()
    {
        $this->setUp();
        $this->tearDown();
    }

    /* Helper function to determine if the clsas has pipeline support */
    protected function havePipeline() {
        $str_constant = get_class($this->redis) . '::PIPELINE';
        return defined($str_constant);
    }

    public function testMinimumVersion()
    {
        // Minimum server version required for tests
        $this->assertTrue(version_compare($this->version, "2.4.0") >= 0);
    }

    public function testPing() {
        /* Reply literal off */
        $this->assertTrue($this->redis->ping());
        $this->assertTrue($this->redis->ping(NULL));
        $this->assertEquals('BEEP', $this->redis->ping('BEEP'));

        /* Make sure we're good in MULTI mode */
        $this->redis->multi();

        $this->redis->ping();
        $this->redis->ping('BEEP');
        $this->assertEquals([true, 'BEEP'], $this->redis->exec());
    }

    public function testPipelinePublish() {
        if (!$this->havePipeline()) {
            $this->markTestSkipped();
        }

        $ret = $this->redis->pipeline()
            ->publish('chan', 'msg')
            ->exec();

        $this->assertTrue(is_array($ret) && count($ret) === 1 && $ret[0] >= 0);
    }

    // Run some simple tests against the PUBSUB command.  This is problematic, as we
    // can't be sure what's going on in the instance, but we can do some things.
    public function testPubSub() {
        // Only available since 2.8.0
        if (version_compare($this->version, "2.8.0") < 0) {
            $this->markTestSkipped();
            return;
        }

        // PUBSUB CHANNELS ...
        $result = $this->redis->pubsub("channels", "*");
        $this->assertTrue(is_array($result));
        $result = $this->redis->pubsub("channels");
        $this->assertTrue(is_array($result));

        // PUBSUB NUMSUB

        $c1 = uniqid() . '-' . rand(1,100);
        $c2 = uniqid() . '-' . rand(1,100);

        $result = $this->redis->pubsub("numsub", [$c1, $c2]);

        // Should get an array back, with two elements
        $this->assertTrue(is_array($result));
        $this->assertEquals(count($result), 2);

        // Make sure the elements are correct, and have zero counts
        foreach([$c1,$c2] as $channel) {
            $this->assertTrue(isset($result[$channel]));
            $this->assertEquals($result[$channel], 0);
        }

        // PUBSUB NUMPAT
        $result = $this->redis->pubsub("numpat");
        $this->assertTrue(is_int($result));

        // Invalid calls
        $this->assertFalse($this->redis->pubsub("notacommand"));
        $this->assertFalse($this->redis->pubsub("numsub", "not-an-array"));
    }

    public function testBitsets() {

        $this->redis->del('key');
        $this->assertEquals(0, $this->redis->getBit('key', 0));
        $this->assertEquals(FALSE, $this->redis->getBit('key', -1));
        $this->assertEquals(0, $this->redis->getBit('key', 100000));

        $this->redis->set('key', "\xff");
        for($i = 0; $i < 8; $i++) {
            $this->assertEquals(1, $this->redis->getBit('key', $i));
        }
        $this->assertEquals(0, $this->redis->getBit('key', 8));

        // change bit 0
        $this->assertEquals(1, $this->redis->setBit('key', 0, 0));
        $this->assertEquals(0, $this->redis->setBit('key', 0, 0));
        $this->assertEquals(0, $this->redis->getBit('key', 0));
        $this->assertEquals("\x7f", $this->redis->get('key'));

        // change bit 1
        $this->assertEquals(1, $this->redis->setBit('key', 1, 0));
        $this->assertEquals(0, $this->redis->setBit('key', 1, 0));
        $this->assertEquals(0, $this->redis->getBit('key', 1));
        $this->assertEquals("\x3f", $this->redis->get('key'));

        // change bit > 1
        $this->assertEquals(1, $this->redis->setBit('key', 2, 0));
        $this->assertEquals(0, $this->redis->setBit('key', 2, 0));
        $this->assertEquals(0, $this->redis->getBit('key', 2));
        $this->assertEquals("\x1f", $this->redis->get('key'));

        // values above 1 are changed to 1 but don't overflow on bits to the right.
        $this->assertEquals(0, $this->redis->setBit('key', 0, 0xff));
        $this->assertEquals("\x9f", $this->redis->get('key'));

        // Verify valid offset ranges
        $this->assertFalse($this->redis->getBit('key', -1));

        $this->redis->setBit('key', 0x7fffffff, 1);
        $this->assertEquals(1, $this->redis->getBit('key', 0x7fffffff));
    }

    public function testBitPos() {
        if (version_compare($this->version, "2.8.7") < 0) {
            $this->MarkTestSkipped();
            return;
        }

        $this->redis->del('bpkey');

        $this->redis->set('bpkey', "\xff\xf0\x00");
        $this->assertEquals($this->redis->bitpos('bpkey', 0), 12);

        $this->redis->set('bpkey', "\x00\xff\xf0");
        $this->assertEquals($this->redis->bitpos('bpkey', 1, 0), 8);
        $this->assertEquals($this->redis->bitpos('bpkey', 1, 1), 8);

        $this->redis->set('bpkey', "\x00\x00\x00");
        $this->assertEquals($this->redis->bitpos('bpkey', 1), -1);
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
        $this->redis->del('keyNotExist');
        $this->assertEquals(FALSE, $this->redis->get('keyNotExist'));

        $this->redis->set('key2', 'val');
        $this->assertEquals('val', $this->redis->get('key2'));

        $value = 'AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA';

        $this->redis->set('key2', $value);
        $this->assertEquals($value, $this->redis->get('key2'));
        $this->assertEquals($value, $this->redis->get('key2'));

        $this->redis->del('key');
        $this->redis->del('key2');


        $i = 66000;
        $value2 = 'X';
        while($i--) {
            $value2 .= 'A';
        }
        $value2 .= 'X';

        $this->redis->set('key', $value2);
        $this->assertEquals($value2, $this->redis->get('key'));
        $this->redis->del('key');
        $this->assertEquals(False, $this->redis->get('key'));

        $data = gzcompress('42');
        $this->assertEquals(True, $this->redis->set('key', $data));
        $this->assertEquals('42', gzuncompress($this->redis->get('key')));

        $this->redis->del('key');
        $data = gzcompress('value1');
        $this->assertEquals(True, $this->redis->set('key', $data));
        $this->assertEquals('value1', gzuncompress($this->redis->get('key')));

        $this->redis->del('key');
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
        if (version_compare($this->version, '2.6.12') < 0) {
            $this->markTestSkipped();
            return;
        }

        /* Legacy SETEX redirection */
        $this->redis->del('foo');
        $this->assertTrue($this->redis->set('foo','bar', 20));
        $this->assertEquals($this->redis->get('foo'), 'bar');
        $this->assertEquals($this->redis->ttl('foo'), 20);

        /* Should coerce doubles into long */
        $this->assertTrue($this->redis->set('foo', 'bar-20.5', 20.5));
        $this->assertEquals($this->redis->ttl('foo'), 20);
        $this->assertEquals($this->redis->get('foo'), 'bar-20.5');

        /* Invalid third arguments */
        $this->assertFalse(@$this->redis->set('foo','bar','baz'));
        $this->assertFalse(@$this->redis->set('foo','bar',new StdClass()));

        /* Set if not exist */
        $this->redis->del('foo');
        $this->assertTrue($this->redis->set('foo','bar', ['nx']));
        $this->assertEquals($this->redis->get('foo'), 'bar');
        $this->assertFalse($this->redis->set('foo','bar', ['nx']));

        /* Set if exists */
        $this->assertTrue($this->redis->set('foo','bar', ['xx']));
        $this->assertEquals($this->redis->get('foo'), 'bar');
        $this->redis->del('foo');
        $this->assertFalse($this->redis->set('foo','bar', ['xx']));

        /* Set with a TTL */
        $this->assertTrue($this->redis->set('foo','bar', ['ex'=>100]));
        $this->assertEquals($this->redis->ttl('foo'), 100);

        /* Set with a PTTL */
        $this->assertTrue($this->redis->set('foo','bar',['px'=>100000]));
        $this->assertTrue(100000 - $this->redis->pttl('foo') < 1000);

        /* Set if exists, with a TTL */
        $this->assertTrue($this->redis->set('foo','bar',['xx','ex'=>105]));
        $this->assertEquals($this->redis->ttl('foo'), 105);
        $this->assertEquals($this->redis->get('foo'), 'bar');

        /* Set if not exists, with a TTL */
        $this->redis->del('foo');
        $this->assertTrue($this->redis->set('foo','bar', ['nx', 'ex'=>110]));
        $this->assertEquals($this->redis->ttl('foo'), 110);
        $this->assertEquals($this->redis->get('foo'), 'bar');
        $this->assertFalse($this->redis->set('foo','bar', ['nx', 'ex'=>110]));

        /* Throw some nonsense into the array, and check that the TTL came through */
        $this->redis->del('foo');
        $this->assertTrue($this->redis->set('foo','barbaz', ['not-valid','nx','invalid','ex'=>200]));
        $this->assertEquals($this->redis->ttl('foo'), 200);
        $this->assertEquals($this->redis->get('foo'), 'barbaz');

        /* Pass NULL as the optional arguments which should be ignored */
        $this->redis->del('foo');
        $this->redis->set('foo','bar', NULL);
        $this->assertEquals($this->redis->get('foo'), 'bar');
        $this->assertTrue($this->redis->ttl('foo')<0);

        /* Make sure we ignore bad/non-string options (regression test for #1835) */
        $this->assertTrue($this->redis->set('foo', 'bar', [NULL, 'EX' => 60]));
        $this->assertTrue($this->redis->set('foo', 'bar', [NULL, new stdClass(), 'EX' => 60]));
        $this->assertFalse(@$this->redis->set('foo', 'bar', [NULL, 'EX' => []]));

        if (version_compare($this->version, "6.0.0") < 0)
            return;

        /* KEEPTTL works by itself */
        $this->redis->set('foo', 'bar', ['EX' => 100]);
        $this->redis->set('foo', 'bar', ['KEEPTTL']);
        $this->assertTrue($this->redis->ttl('foo') > -1);

        /* Works with other options */
        $this->redis->set('foo', 'bar', ['XX', 'KEEPTTL']);
        $this->assertTrue($this->redis->ttl('foo') > -1);
        $this->redis->set('foo', 'bar', ['XX']);
        $this->assertTrue($this->redis->ttl('foo') == -1);

        if (version_compare($this->version, "6.2.0") < 0)
            return;

        $this->assertTrue($this->redis->set('foo', 'baz', ['GET']) === 'bar');
    }

    public function testGetSet() {
        $this->redis->del('key');
        $this->assertTrue($this->redis->getSet('key', '42') === FALSE);
        $this->assertTrue($this->redis->getSet('key', '123') === '42');
        $this->assertTrue($this->redis->getSet('key', '123') === '123');
    }

    public function testRandomKey() {
        for($i = 0; $i < 1000; $i++) {
            $k = $this->redis->randomKey();
            $this->assertEquals($this->redis->exists($k), 1);
        }
    }

    public function testRename() {
        // strings
        $this->redis->del('{key}0');
        $this->redis->set('{key}0', 'val0');
        $this->redis->rename('{key}0', '{key}1');
        $this->assertEquals(FALSE, $this->redis->get('{key}0'));
        $this->assertEquals('val0', $this->redis->get('{key}1'));
    }

    public function testRenameNx() {
        // strings
        $this->redis->del('{key}0', '{key}1');
        $this->redis->set('{key}0', 'val0');
        $this->redis->set('{key}1', 'val1');
        $this->assertTrue($this->redis->renameNx('{key}0', '{key}1') === FALSE);
        $this->assertTrue($this->redis->get('{key}0') === 'val0');
        $this->assertTrue($this->redis->get('{key}1') === 'val1');

        // lists
        $this->redis->del('{key}0');
        $this->redis->del('{key}1');
        $this->redis->lPush('{key}0', 'val0');
        $this->redis->lPush('{key}0', 'val1');
        $this->redis->lPush('{key}1', 'val1-0');
        $this->redis->lPush('{key}1', 'val1-1');
        $this->assertTrue($this->redis->renameNx('{key}0', '{key}1') === FALSE);
        $this->assertTrue($this->redis->lRange('{key}0', 0, -1) === ['val1', 'val0']);
        $this->assertTrue($this->redis->lRange('{key}1', 0, -1) === ['val1-1', 'val1-0']);

        $this->redis->del('{key}2');
        $this->assertTrue($this->redis->renameNx('{key}0', '{key}2') === TRUE);
        $this->assertTrue($this->redis->lRange('{key}0', 0, -1) === []);
        $this->assertTrue($this->redis->lRange('{key}2', 0, -1) === ['val1', 'val0']);
    }

    public function testMultiple() {
        $this->redis->del('k1');
        $this->redis->del('k2');
        $this->redis->del('k3');

        $this->redis->set('k1', 'v1');
        $this->redis->set('k2', 'v2');
        $this->redis->set('k3', 'v3');
        $this->redis->set(1, 'test');

        $this->assertEquals(['v1'], $this->redis->mget(['k1']));
        $this->assertEquals(['v1', 'v3', false], $this->redis->mget(['k1', 'k3', 'NoKey']));
        $this->assertEquals(['v1', 'v2', 'v3'], $this->redis->mget(['k1', 'k2', 'k3']));
        $this->assertEquals(['v1', 'v2', 'v3'], $this->redis->mget(['k1', 'k2', 'k3']));

        $this->redis->set('k5', '$1111111111');
        $this->assertEquals([0 => '$1111111111'], $this->redis->mget(['k5']));

        $this->assertEquals([0 => 'test'], $this->redis->mget([1])); // non-string
    }

    public function testMultipleBin() {

    $this->redis->del('k1');
        $this->redis->del('k2');
        $this->redis->del('k3');

        $this->redis->set('k1', gzcompress('v1'));
        $this->redis->set('k2', gzcompress('v2'));
        $this->redis->set('k3', gzcompress('v3'));

        $this->assertEquals([gzcompress('v1'), gzcompress('v2'), gzcompress('v3')], $this->redis->mget(['k1', 'k2', 'k3']));
        $this->assertEquals([gzcompress('v1'), gzcompress('v2'), gzcompress('v3')], $this->redis->mget(['k1', 'k2', 'k3']));

    }

    public function testSetTimeout() {
        $this->redis->del('key');
        $this->redis->set('key', 'value');
        $this->assertEquals('value', $this->redis->get('key'));
        $this->redis->expire('key', 1);
        $this->assertEquals('value', $this->redis->get('key'));
        sleep(2);
        $this->assertEquals(False, $this->redis->get('key'));
    }

    /* This test is prone to failure in the Travis container, so attempt to mitigate this by running more than once */
    public function testExpireAt() {
        $success = false;

        for ($i = 0; !$success && $i < 3; $i++) {
            $this->redis->del('key');
            $this->redis->set('key', 'value');
            $this->redis->expireAt('key', time() + 1);
            usleep(1500000);
            $success = FALSE === $this->redis->get('key');
        }

        $this->assertTrue($success);
    }

    public function testSetEx() {

        $this->redis->del('key');
        $this->assertTrue($this->redis->setex('key', 7, 'val') === TRUE);
        $this->assertTrue($this->redis->ttl('key') ===7);
        $this->assertTrue($this->redis->get('key') === 'val');
    }

    public function testPSetEx() {
        $this->redis->del('key');
        $this->assertTrue($this->redis->psetex('key', 7 * 1000, 'val') === TRUE);
        $this->assertTrue($this->redis->ttl('key') ===7);
        $this->assertTrue($this->redis->get('key') === 'val');
    }

    public function testSetNX() {

        $this->redis->set('key', 42);
        $this->assertTrue($this->redis->setnx('key', 'err') === FALSE);
        $this->assertTrue($this->redis->get('key') === '42');

        $this->redis->del('key');
        $this->assertTrue($this->redis->setnx('key', '42') === TRUE);
        $this->assertTrue($this->redis->get('key') === '42');
    }

    public function testExpireAtWithLong() {
        if (PHP_INT_SIZE != 8) {
            $this->markTestSkipped('64 bits only');
        }
        $longExpiryTimeExceedingInt = 3153600000;
        $this->redis->del('key');
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

        $this->redis->incrBy('key', 3);
        $this->assertEquals(5, (int)$this->redis->get('key'));

        $this->redis->incrBy('key', 1);
        $this->assertEquals(6, (int)$this->redis->get('key'));

        $this->redis->incrBy('key', -1);
        $this->assertEquals(5, (int)$this->redis->get('key'));

        $this->redis->incr('key', 5);
        $this->assertEquals(10, (int)$this->redis->get('key'));

        $this->redis->del('key');

        $this->redis->set('key', 'abc');

        $this->redis->incr('key');
        $this->assertTrue("abc" === $this->redis->get('key'));

        $this->redis->incr('key');
        $this->assertTrue("abc" === $this->redis->get('key'));

        $this->redis->set('key', 0);
        $this->assertEquals(PHP_INT_MAX, $this->redis->incrby('key', PHP_INT_MAX));
    }

    public function testIncrByFloat()
    {
        // incrbyfloat is new in 2.6.0
        if (version_compare($this->version, "2.5.0") < 0) {
            $this->markTestSkipped();
        }

        $this->redis->del('key');

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
        $this->assertEquals(1.8, floatval($this->redis->get('key'))); // convert to float to avoid rounding issue on arm
        $this->redis->setOption(Redis::OPT_PREFIX, '');
        $this->assertEquals(1, $this->redis->exists('someprefix:key'));
        $this->redis->del('someprefix:key');

    }

    public function testDecr()
    {
        $this->redis->set('key', 5);

        $this->redis->decr('key');
        $this->assertEquals(4, (int)$this->redis->get('key'));

        $this->redis->decr('key');
        $this->assertEquals(3, (int)$this->redis->get('key'));

        $this->redis->decrBy('key', 2);
        $this->assertEquals(1, (int)$this->redis->get('key'));

        $this->redis->decrBy('key', 1);
        $this->assertEquals(0, (int)$this->redis->get('key'));

        $this->redis->decrBy('key', -10);
        $this->assertEquals(10, (int)$this->redis->get('key'));

        $this->redis->decr('key', 10);
        $this->assertEquals(0, (int)$this->redis->get('key'));
    }


    public function testExists()
    {
        /* Single key */
        $this->redis->del('key');
        $this->assertEquals(0, $this->redis->exists('key'));
        $this->redis->set('key', 'val');
        $this->assertEquals(1, $this->redis->exists('key'));

        /* Add multiple keys */
        $mkeys = [];
        for ($i = 0; $i < 10; $i++) {
            if (rand(1, 2) == 1) {
                $mkey = "{exists}key:$i";
                $this->redis->set($mkey, $i);
                $mkeys[] = $mkey;
            }
        }

        /* Test passing an array as well as the keys variadic */
        $this->assertEquals(count($mkeys), $this->redis->exists($mkeys));
        $this->assertEquals(count($mkeys), call_user_func_array([$this->redis, 'exists'], $mkeys));
    }

    public function testKeys()
    {
        $pattern = 'keys-test-';
        for($i = 1; $i < 10; $i++) {
            $this->redis->set($pattern.$i, $i);
        }
        $this->redis->del($pattern.'3');
        $keys = $this->redis->keys($pattern.'*');

        $this->redis->set($pattern.'3', 'something');

        $keys2 = $this->redis->keys($pattern.'*');

        $this->assertEquals((count($keys) + 1), count($keys2));

        // empty array when no key matches
        $this->assertEquals([], $this->redis->keys(rand().rand().rand().'*'));
    }

    protected function genericDelUnlink($cmd) {
        $key = 'key' . rand();
        $this->redis->set($key, 'val');
        $this->assertEquals('val', $this->redis->get($key));
        $this->assertEquals(1, $this->redis->$cmd($key));
        $this->assertEquals(false, $this->redis->get($key));

        // multiple, all existing
        $this->redis->set('x', 0);
        $this->redis->set('y', 1);
        $this->redis->set('z', 2);
        $this->assertEquals(3, $this->redis->$cmd('x', 'y', 'z'));
        $this->assertEquals(false, $this->redis->get('x'));
        $this->assertEquals(false, $this->redis->get('y'));
        $this->assertEquals(false, $this->redis->get('z'));

        // multiple, none existing
        $this->assertEquals(0, $this->redis->$cmd('x', 'y', 'z'));
        $this->assertEquals(false, $this->redis->get('x'));
        $this->assertEquals(false, $this->redis->get('y'));
        $this->assertEquals(false, $this->redis->get('z'));

        // multiple, some existing
        $this->redis->set('y', 1);
        $this->assertEquals(1, $this->redis->$cmd('x', 'y', 'z'));
        $this->assertEquals(false, $this->redis->get('y'));

        $this->redis->set('x', 0);
        $this->redis->set('y', 1);
        $this->assertEquals(2, $this->redis->$cmd(['x', 'y']));
    }

    public function testDelete() {
        $this->genericDelUnlink("DEL");
    }

    public function testUnlink() {
        if (version_compare($this->version, "4.0.0") < 0) {
            $this->markTestSkipped();
            return;
        }

        $this->genericDelUnlink("UNLINK");
    }

    public function testType()
    {
        // 0 => none, (key didn't exist)
        // 1=> string,
        // 2 => set,
        // 3 => list,
        // 4 => zset,
        // 5 => hash
        // 6 => stream

        // string
        $this->redis->set('key', 'val');
        $this->assertEquals(Redis::REDIS_STRING, $this->redis->type('key'));

        // list
        $this->redis->lPush('keyList', 'val0');
        $this->redis->lPush('keyList', 'val1');
        $this->assertEquals(Redis::REDIS_LIST, $this->redis->type('keyList'));

        // set
        $this->redis->del('keySet');
        $this->redis->sAdd('keySet', 'val0');
        $this->redis->sAdd('keySet', 'val1');
        $this->assertEquals(Redis::REDIS_SET, $this->redis->type('keySet'));

        // sadd with numeric key
        $this->redis->del(123);
        $this->assertTrue(1 === $this->redis->sAdd(123, 'val0'));
        $this->assertTrue(['val0'] === $this->redis->sMembers(123));

        // zset
        $this->redis->del('keyZSet');
        $this->redis->zAdd('keyZSet', 0, 'val0');
        $this->redis->zAdd('keyZSet', 1, 'val1');
        $this->assertEquals(Redis::REDIS_ZSET, $this->redis->type('keyZSet'));

        // hash
        $this->redis->del('keyHash');
        $this->redis->hSet('keyHash', 'key0', 'val0');
        $this->redis->hSet('keyHash', 'key1', 'val1');
        $this->assertEquals(Redis::REDIS_HASH, $this->redis->type('keyHash'));

        // stream
        if ($this->minVersionCheck("5.0")) {
            $this->redis->del('stream');
            $this->redis->xAdd('stream', '*', ['foo' => 'bar']);
            $this->assertEquals(Redis::REDIS_STREAM, $this->redis->type('stream'));
        }

        // None
        $this->redis->del('keyNotExists');
        $this->assertEquals(Redis::REDIS_NOT_FOUND, $this->redis->type('keyNotExists'));

    }

    public function testStr() {

        $this->redis->set('key', 'val1');
        $this->assertTrue($this->redis->append('key', 'val2') === 8);
        $this->assertTrue($this->redis->get('key') === 'val1val2');

        $this->redis->del('keyNotExist');
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

    //  rpush  => tail
    //  lpush => head


        $this->redis->del('list');

        $this->redis->lPush('list', 'val');
        $this->redis->lPush('list', 'val2');
    $this->redis->rPush('list', 'val3');

    // 'list' = [ 'val2', 'val', 'val3']

    $this->assertEquals('val2', $this->redis->lPop('list'));
        $this->assertEquals('val', $this->redis->lPop('list'));
        $this->assertEquals('val3', $this->redis->lPop('list'));
        $this->assertEquals(FALSE, $this->redis->lPop('list'));

    // testing binary data

    $this->redis->del('list');
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
    //  rpush  => tail
    //  lpush => head

        $this->redis->del('list');

        $this->redis->rPush('list', 'val');
        $this->redis->rPush('list', 'val2');
    $this->redis->lPush('list', 'val3');

    // 'list' = [ 'val3', 'val', 'val2']

    $this->assertEquals('val2', $this->redis->rPop('list'));
        $this->assertEquals('val', $this->redis->rPop('list'));
        $this->assertEquals('val3', $this->redis->rPop('list'));
        $this->assertEquals(FALSE, $this->redis->rPop('list'));

    // testing binary data

    $this->redis->del('list');
    $this->assertEquals(1, $this->redis->rPush('list', gzcompress('val1')));
    $this->assertEquals(2, $this->redis->rPush('list', gzcompress('val2')));
    $this->assertEquals(3, $this->redis->rPush('list', gzcompress('val3')));

    $this->assertEquals('val3', gzuncompress($this->redis->rPop('list')));
    $this->assertEquals('val2', gzuncompress($this->redis->rPop('list')));
    $this->assertEquals('val1', gzuncompress($this->redis->rPop('list')));

    }

    public function testblockingPop() {
        // non blocking blPop, brPop
        $this->redis->del('list');
        $this->redis->lPush('list', 'val1');
        $this->redis->lPush('list', 'val2');
        $this->assertTrue($this->redis->blPop(['list'], 2) === ['list', 'val2']);
        $this->assertTrue($this->redis->blPop(['list'], 2) === ['list', 'val1']);

        $this->redis->del('list');
        $this->redis->lPush('list', 'val1');
        $this->redis->lPush('list', 'val2');
        $this->assertTrue($this->redis->brPop(['list'], 1) === ['list', 'val1']);
        $this->assertTrue($this->redis->brPop(['list'], 1) === ['list', 'val2']);

        // blocking blpop, brpop
        $this->redis->del('list');

        /* Also test our option that we want *-1 to be returned as NULL */
        foreach ([false => [], true => NULL] as $opt => $val) {
            $this->redis->setOption(Redis::OPT_NULL_MULTIBULK_AS_NULL, $opt);
            $this->assertEquals($val, $this->redis->blPop(['list'], 1));
            $this->assertEquals($val, $this->redis->brPop(['list'], 1));
        }

        $this->redis->setOption(Redis::OPT_NULL_MULTIBULK_AS_NULL, false);
    }

    public function testllen()
    {
        $this->redis->del('list');

        $this->redis->lPush('list', 'val');
        $this->assertEquals(1, $this->redis->llen('list'));

        $this->redis->lPush('list', 'val2');
        $this->assertEquals(2, $this->redis->llen('list'));

        $this->assertEquals('val2', $this->redis->lPop('list'));
        $this->assertEquals(1, $this->redis->llen('list'));

        $this->assertEquals('val', $this->redis->lPop('list'));
        $this->assertEquals(0, $this->redis->llen('list'));

        $this->assertEquals(FALSE, $this->redis->lPop('list'));
        $this->assertEquals(0, $this->redis->llen('list'));    // empty returns 0

        $this->redis->del('list');
        $this->assertEquals(0, $this->redis->llen('list'));    // non-existent returns 0

        $this->redis->set('list', 'actually not a list');
        $this->assertEquals(FALSE, $this->redis->llen('list'));// not a list returns FALSE
    }

    //lInsert, lPopx, rPopx
    public function testlPopx() {
        //test lPushx/rPushx
        $this->redis->del('keyNotExists');
        $this->assertTrue($this->redis->lPushx('keyNotExists', 'value') === 0);
        $this->assertTrue($this->redis->rPushx('keyNotExists', 'value') === 0);

        $this->redis->del('key');
        $this->redis->lPush('key', 'val0');
        $this->assertTrue($this->redis->lPushx('key', 'val1') === 2);
        $this->assertTrue($this->redis->rPushx('key', 'val2') === 3);
        $this->assertTrue($this->redis->lrange('key', 0, -1) === ['val1', 'val0', 'val2']);

        //test linsert
        $this->redis->del('key');
        $this->redis->lPush('key', 'val0');
        $this->assertTrue($this->redis->lInsert('keyNotExists', Redis::AFTER, 'val1', 'val2') === 0);
        $this->assertTrue($this->redis->lInsert('key', Redis::BEFORE, 'valX', 'val2') === -1);

        $this->assertTrue($this->redis->lInsert('key', Redis::AFTER, 'val0', 'val1') === 2);
        $this->assertTrue($this->redis->lInsert('key', Redis::BEFORE, 'val0', 'val2') === 3);
        $this->assertTrue($this->redis->lrange('key', 0, -1) === ['val2', 'val0', 'val1']);
    }

    // ltrim, lsize, lpop
    public function testltrim()
    {

        $this->redis->del('list');

        $this->redis->lPush('list', 'val');
        $this->redis->lPush('list', 'val2');
        $this->redis->lPush('list', 'val3');
        $this->redis->lPush('list', 'val4');

    $this->assertEquals(TRUE, $this->redis->ltrim('list', 0, 2));
    $this->assertEquals(3, $this->redis->llen('list'));

        $this->redis->ltrim('list', 0, 0);
        $this->assertEquals(1, $this->redis->llen('list'));
    $this->assertEquals('val4', $this->redis->lPop('list'));

    $this->assertEquals(TRUE, $this->redis->ltrim('list', 10, 10000));
    $this->assertEquals(TRUE, $this->redis->ltrim('list', 10000, 10));

    // test invalid type
    $this->redis->set('list', 'not a list...');
    $this->assertEquals(FALSE, $this->redis->ltrim('list', 0, 2));

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
        $this->redis->del('person:id');
        foreach([1,2,3,4] as $id) {
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

        $this->assertEquals(['1','2','3'], $this->redis->sort('some-item', ['sort' => 'asc']));
        $this->assertEquals(['3','2','1'], $this->redis->sort('some-item', ['sort' => 'desc']));
        $this->assertEquals(['1','2','3'], $this->redis->sort('some-item'));

        // Kill our set/prefix
        $this->redis->del('some-item');
        $this->redis->setOption(Redis::OPT_PREFIX, '');
    }

    public function testSortAsc() {
        $this->setupSort();
        // sort by age and get IDs
        $byAgeAsc = ['3','1','2','4'];
        $this->assertEquals($byAgeAsc, $this->redis->sort('person:id', ['by' => 'person:age_*']));
        $this->assertEquals($byAgeAsc, $this->redis->sort('person:id', ['by' => 'person:age_*', 'sort' => 'asc']));
        $this->assertEquals(['1', '2', '3', '4'], $this->redis->sort('person:id', ['by' => NULL]));   // check that NULL works.
        $this->assertEquals(['1', '2', '3', '4'], $this->redis->sort('person:id', ['by' => NULL, 'get' => NULL])); // for all fields.
        $this->assertEquals(['1', '2', '3', '4'], $this->redis->sort('person:id', ['sort' => 'asc']));

        // sort by age and get names
        $byAgeAsc = ['Carol','Alice','Bob','Dave'];
        $this->assertEquals($byAgeAsc, $this->redis->sort('person:id', ['by' => 'person:age_*', 'get' => 'person:name_*']));
        $this->assertEquals($byAgeAsc, $this->redis->sort('person:id', ['by' => 'person:age_*', 'get' => 'person:name_*', 'sort' => 'asc']));

        $this->assertEquals(array_slice($byAgeAsc, 0, 2), $this->redis->sort('person:id', ['by' => 'person:age_*', 'get' => 'person:name_*', 'limit' => [0, 2]]));
        $this->assertEquals(array_slice($byAgeAsc, 0, 2), $this->redis->sort('person:id', ['by' => 'person:age_*', 'get' => 'person:name_*', 'limit' => [0, 2], 'sort' => 'asc']));

        $this->assertEquals(array_slice($byAgeAsc, 1, 2), $this->redis->sort('person:id', ['by' => 'person:age_*', 'get' => 'person:name_*', 'limit' => [1, 2]]));
        $this->assertEquals(array_slice($byAgeAsc, 1, 2), $this->redis->sort('person:id', ['by' => 'person:age_*', 'get' => 'person:name_*', 'limit' => [1, 2], 'sort' => 'asc']));
        $this->assertEquals($byAgeAsc, $this->redis->sort('person:id', ['by' => 'person:age_*', 'get' => 'person:name_*', 'limit' => [0, 4]]));
        $this->assertEquals($byAgeAsc, $this->redis->sort('person:id', ['by' => 'person:age_*', 'get' => 'person:name_*', 'limit' => [0, "4"]])); // with strings
        $this->assertEquals($byAgeAsc, $this->redis->sort('person:id', ['by' => 'person:age_*', 'get' => 'person:name_*', 'limit' => ["0", 4]]));

        // sort by salary and get ages
        $agesBySalaryAsc = ['34', '27', '25', '41'];
        $this->assertEquals($agesBySalaryAsc, $this->redis->sort('person:id', ['by' => 'person:salary_*', 'get' => 'person:age_*']));
        $this->assertEquals($agesBySalaryAsc, $this->redis->sort('person:id', ['by' => 'person:salary_*', 'get' => 'person:age_*', 'sort' => 'asc']));

        $agesAndSalaries = $this->redis->sort('person:id', ['by' => 'person:salary_*', 'get' => ['person:age_*', 'person:salary_*'], 'sort' => 'asc']);
        $this->assertEquals(['34', '2000', '27', '2500', '25', '2800', '41', '3100'], $agesAndSalaries);

        // sort non-alpha doesn't change all-string lists
        // list → [ghi, def, abc]
        $list = ['abc', 'def', 'ghi'];
        $this->redis->del('list');
        foreach($list as $i) {
            $this->redis->lPush('list', $i);
        }

        // SORT list → [ghi, def, abc]
        if (version_compare($this->version, "2.5.0") < 0) {
            $this->assertEquals(array_reverse($list), $this->redis->sort('list'));
            $this->assertEquals(array_reverse($list), $this->redis->sort('list', ['sort' => 'asc']));
        } else {
            // TODO rewrite, from 2.6.0 release notes:
            // SORT now will refuse to sort in numerical mode elements that can't be parsed
            // as numbers
        }

        // SORT list ALPHA → [abc, def, ghi]
        $this->assertEquals($list, $this->redis->sort('list', ['alpha' => TRUE]));
        $this->assertEquals($list, $this->redis->sort('list', ['sort' => 'asc', 'alpha' => TRUE]));
    }

    public function testSortDesc() {

    $this->setupSort();

    // sort by age and get IDs
    $byAgeDesc = ['4','2','1','3'];
    $this->assertEquals($byAgeDesc, $this->redis->sort('person:id', ['by' => 'person:age_*', 'sort' => 'desc']));

    // sort by age and get names
    $byAgeDesc = ['Dave', 'Bob', 'Alice', 'Carol'];
    $this->assertEquals($byAgeDesc, $this->redis->sort('person:id', ['by' => 'person:age_*', 'get' => 'person:name_*', 'sort' => 'desc']));

    $this->assertEquals(array_slice($byAgeDesc, 0, 2), $this->redis->sort('person:id', ['by' => 'person:age_*', 'get' => 'person:name_*', 'limit' => [0, 2], 'sort' => 'desc']));
    $this->assertEquals(array_slice($byAgeDesc, 1, 2), $this->redis->sort('person:id', ['by' => 'person:age_*', 'get' => 'person:name_*', 'limit' => [1, 2], 'sort' => 'desc']));

    // sort by salary and get ages
    $agesBySalaryDesc = ['41', '25', '27', '34'];
    $this->assertEquals($agesBySalaryDesc, $this->redis->sort('person:id', ['by' => 'person:salary_*', 'get' => 'person:age_*', 'sort' => 'desc']));

    // sort non-alpha doesn't change all-string lists
    $list = ['def', 'abc', 'ghi'];
    $this->redis->del('list');
    foreach($list as $i) {
        $this->redis->lPush('list', $i);
    }

    // SORT list → [ghi, abc, def]
    if (version_compare($this->version, "2.5.0") < 0) {
        $this->assertEquals(array_reverse($list), $this->redis->sort('list', ['sort' => 'desc']));
    } else {
        // TODO rewrite, from 2.6.0 release notes:
        // SORT now will refuse to sort in numerical mode elements that can't be parsed
        // as numbers
    }

    // SORT list ALPHA → [abc, def, ghi]
    $this->assertEquals(['ghi', 'def', 'abc'], $this->redis->sort('list', ['sort' => 'desc', 'alpha' => TRUE]));
    }

    // LINDEX
    public function testLindex() {

        $this->redis->del('list');

        $this->redis->lPush('list', 'val');
        $this->redis->lPush('list', 'val2');
        $this->redis->lPush('list', 'val3');

        $this->assertEquals('val3', $this->redis->lIndex('list', 0));
        $this->assertEquals('val2', $this->redis->lIndex('list', 1));
        $this->assertEquals('val', $this->redis->lIndex('list', 2));
        $this->assertEquals('val', $this->redis->lIndex('list', -1));
        $this->assertEquals('val2', $this->redis->lIndex('list', -2));
        $this->assertEquals('val3', $this->redis->lIndex('list', -3));
        $this->assertEquals(FALSE, $this->redis->lIndex('list', -4));

        $this->redis->rPush('list', 'val4');
        $this->assertEquals('val4', $this->redis->lIndex('list', 3));
        $this->assertEquals('val4', $this->redis->lIndex('list', -1));
    }

    public function testlMove()
    {
        // Only available since 6.2.0
        if (version_compare($this->version, '6.2.0') < 0) {
            $this->markTestSkipped();
            return;
        }

        $this->redis->del('list0', 'list1');
        $this->redis->lPush('list0', 'a');
        $this->redis->lPush('list0', 'b');
        $this->redis->lPush('list0', 'c');

        $return = $this->redis->lMove('list0', 'list1', Redis::LEFT, Redis::RIGHT);
        $this->assertEquals('c', $return);

        $return = $this->redis->lMove('list0', 'list1', Redis::RIGHT, Redis::LEFT);
        $this->assertEquals('a', $return);

        $this->assertEquals(['b'], $this->redis->lRange('list0', 0, -1));
        $this->assertEquals(['a', 'c'], $this->redis->lRange('list1', 0, -1));

    }

    // lRem testing
    public function testlrem() {
        $this->redis->del('list');
        $this->redis->lPush('list', 'a');
        $this->redis->lPush('list', 'b');
        $this->redis->lPush('list', 'c');
        $this->redis->lPush('list', 'c');
        $this->redis->lPush('list', 'b');
        $this->redis->lPush('list', 'c');

        // ['c', 'b', 'c', 'c', 'b', 'a']
        $return = $this->redis->lrem('list', 'b', 2);
        // ['c', 'c', 'c', 'a']
        $this->assertEquals(2, $return);
        $this->assertEquals('c', $this->redis->lIndex('list', 0));
        $this->assertEquals('c', $this->redis->lIndex('list', 1));
        $this->assertEquals('c', $this->redis->lIndex('list', 2));
        $this->assertEquals('a', $this->redis->lIndex('list', 3));

        $this->redis->del('list');
        $this->redis->lPush('list', 'a');
        $this->redis->lPush('list', 'b');
        $this->redis->lPush('list', 'c');
        $this->redis->lPush('list', 'c');
        $this->redis->lPush('list', 'b');
        $this->redis->lPush('list', 'c');

        // ['c', 'b', 'c', 'c', 'b', 'a']
        $this->redis->lrem('list', 'c', -2);
        // ['c', 'b', 'b', 'a']
        $this->assertEquals(2, $return);
        $this->assertEquals('c', $this->redis->lIndex('list', 0));
        $this->assertEquals('b', $this->redis->lIndex('list', 1));
        $this->assertEquals('b', $this->redis->lIndex('list', 2));
        $this->assertEquals('a', $this->redis->lIndex('list', 3));

        // remove each element
        $this->assertEquals(1, $this->redis->lrem('list', 'a', 0));
        $this->assertEquals(0, $this->redis->lrem('list', 'x', 0));
        $this->assertEquals(2, $this->redis->lrem('list', 'b', 0));
        $this->assertEquals(1, $this->redis->lrem('list', 'c', 0));
        $this->assertEquals(FALSE, $this->redis->get('list'));

        $this->redis->set('list', 'actually not a list');
        $this->assertEquals(FALSE, $this->redis->lrem('list', 'x'));
    }

    public function testsAdd() {
        $this->redis->del('set');

        $this->assertEquals(1, $this->redis->sAdd('set', 'val'));
        $this->assertEquals(0, $this->redis->sAdd('set', 'val'));

        $this->assertTrue($this->redis->sismember('set', 'val'));
        $this->assertFalse($this->redis->sismember('set', 'val2'));

        $this->assertEquals(1, $this->redis->sAdd('set', 'val2'));

        $this->assertTrue($this->redis->sismember('set', 'val2'));
    }

    public function testscard() {
        $this->redis->del('set');
        $this->assertEquals(1, $this->redis->sAdd('set', 'val'));
        $this->assertEquals(1, $this->redis->scard('set'));
        $this->assertEquals(1, $this->redis->sAdd('set', 'val2'));
        $this->assertEquals(2, $this->redis->scard('set'));
    }

    public function testsrem() {
        $this->redis->del('set');
        $this->redis->sAdd('set', 'val');
        $this->redis->sAdd('set', 'val2');
        $this->redis->srem('set', 'val');
        $this->assertEquals(1, $this->redis->scard('set'));
        $this->redis->srem('set', 'val2');
        $this->assertEquals(0, $this->redis->scard('set'));
    }

    public function testsMove() {
        $this->redis->del('{set}0');
        $this->redis->del('{set}1');

        $this->redis->sAdd('{set}0', 'val');
        $this->redis->sAdd('{set}0', 'val2');

        $this->assertTrue($this->redis->sMove('{set}0', '{set}1', 'val'));
        $this->assertFalse($this->redis->sMove('{set}0', '{set}1', 'val'));
        $this->assertFalse($this->redis->sMove('{set}0', '{set}1', 'val-what'));

        $this->assertEquals(1, $this->redis->scard('{set}0'));
        $this->assertEquals(1, $this->redis->scard('{set}1'));

        $this->assertEquals(['val2'], $this->redis->smembers('{set}0'));
        $this->assertEquals(['val'], $this->redis->smembers('{set}1'));
    }

    public function testsPop() {
        $this->redis->del('set0');
        $this->assertTrue($this->redis->sPop('set0') === FALSE);

        $this->redis->sAdd('set0', 'val');
        $this->redis->sAdd('set0', 'val2');

        $v0 = $this->redis->sPop('set0');
        $this->assertTrue(1 === $this->redis->scard('set0'));
        $this->assertTrue($v0 === 'val' || $v0 === 'val2');
        $v1 = $this->redis->sPop('set0');
        $this->assertTrue(0 === $this->redis->scard('set0'));
        $this->assertTrue(($v0 === 'val' && $v1 === 'val2') || ($v1 === 'val' && $v0 === 'val2'));

        $this->assertTrue($this->redis->sPop('set0') === FALSE);
    }

    public function testsPopWithCount() {
        if (!$this->minVersionCheck("3.2")) {
            return $this->markTestSkipped();
        }

        $set = 'set0';
        $prefix = 'member';
        $count = 5;

        /* Add a few members */
        $this->redis->del($set);
        for ($i = 0; $i < $count; $i++) {
            $this->redis->sadd($set, $prefix.$i);
        }

        /* Pop them all */
        $ret = $this->redis->sPop($set, $i);

        /* Make sure we got an arary and the count is right */
        if ($this->assertTrue(is_array($ret)) && $this->assertTrue(count($ret) == $count)) {
            /* Probably overkill but validate the actual returned members */
            for ($i = 0; $i < $count; $i++) {
                $this->assertTrue(in_array($prefix.$i, $ret));
            }
        }
    }

    public function testsRandMember() {
        $this->redis->del('set0');
        $this->assertTrue($this->redis->sRandMember('set0') === FALSE);

        $this->redis->sAdd('set0', 'val');
        $this->redis->sAdd('set0', 'val2');

        $got = [];
        while(true) {
            $v = $this->redis->sRandMember('set0');
            $this->assertTrue(2 === $this->redis->scard('set0')); // no change.
            $this->assertTrue($v === 'val' || $v === 'val2');

            $got[$v] = $v;
            if(count($got) == 2) {
                break;
            }
        }

        //
        // With and without count, while serializing
        //

        $this->redis->del('set0');
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
        $this->redis->del('set0');

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

        if ($this->havePipeline()) {
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
    }

    public function testsismember()
    {
        $this->redis->del('set');

        $this->redis->sAdd('set', 'val');

        $this->assertTrue($this->redis->sismember('set', 'val'));
        $this->assertFalse($this->redis->sismember('set', 'val2'));
    }

    public function testsmembers()
    {
        $this->redis->del('set');

        $this->redis->sAdd('set', 'val');
        $this->redis->sAdd('set', 'val2');
        $this->redis->sAdd('set', 'val3');

        $array = ['val', 'val2', 'val3'];

        $smembers = $this->redis->smembers('set');
        sort($smembers);
        $this->assertEquals($array, $smembers);

        $sMembers = $this->redis->sMembers('set');
        sort($sMembers);
        $this->assertEquals($array, $sMembers); // test alias
    }

    public function testsMisMember()
    {
        // Only available since 6.2.0
        if (version_compare($this->version, '6.2.0') < 0) {
            $this->markTestSkipped();
            return;
        }

        $this->redis->del('set');

        $this->redis->sAdd('set', 'val');
        $this->redis->sAdd('set', 'val2');
        $this->redis->sAdd('set', 'val3');

        $misMembers = $this->redis->sMisMember('set', 'val', 'notamember', 'val3');
        $this->assertEquals([1, 0, 1], $misMembers);

        $misMembers = $this->redis->sMisMember('wrongkey', 'val', 'val2', 'val3');
        $this->assertEquals([0, 0, 0], $misMembers);
    }

    public function testlSet() {

        $this->redis->del('list');
        $this->redis->lPush('list', 'val');
        $this->redis->lPush('list', 'val2');
    $this->redis->lPush('list', 'val3');

    $this->assertEquals($this->redis->lIndex('list', 0), 'val3');
    $this->assertEquals($this->redis->lIndex('list', 1), 'val2');
    $this->assertEquals($this->redis->lIndex('list', 2), 'val');

    $this->assertEquals(TRUE, $this->redis->lSet('list', 1, 'valx'));

    $this->assertEquals($this->redis->lIndex('list', 0), 'val3');
    $this->assertEquals($this->redis->lIndex('list', 1), 'valx');
    $this->assertEquals($this->redis->lIndex('list', 2), 'val');

    }

    public function testsInter() {
        $this->redis->del('{set}odd');    // set of odd numbers
        $this->redis->del('{set}prime');  // set of prime numbers
        $this->redis->del('{set}square'); // set of squares
        $this->redis->del('{set}seq');    // set of numbers of the form n^2 - 1

        $x = [1,3,5,7,9,11,13,15,17,19,21,23,25];
        foreach($x as $i) {
            $this->redis->sAdd('{set}odd', $i);
        }

        $y = [1,2,3,5,7,11,13,17,19,23];
        foreach($y as $i) {
            $this->redis->sAdd('{set}prime', $i);
        }

        $z = [1,4,9,16,25];
        foreach($z as $i) {
            $this->redis->sAdd('{set}square', $i);
        }

        $t = [2,5,10,17,26];
        foreach($t as $i) {
            $this->redis->sAdd('{set}seq', $i);
        }

        $xy = $this->redis->sInter('{set}odd', '{set}prime');   // odd prime numbers
        foreach($xy as $i) {
            $i = (int)$i;
            $this->assertTrue(in_array($i, array_intersect($x, $y)));
        }

        $xy = $this->redis->sInter(['{set}odd', '{set}prime']);    // odd prime numbers, as array.
        foreach($xy as $i) {
            $i = (int)$i;
            $this->assertTrue(in_array($i, array_intersect($x, $y)));
        }

        $yz = $this->redis->sInter('{set}prime', '{set}square');   // set of prime squares
        foreach($yz as $i) {
            $i = (int)$i;
            $this->assertTrue(in_array($i, array_intersect($y, $z)));
        }

        $yz = $this->redis->sInter(['{set}prime', '{set}square']);    // set of odd squares, as array
        foreach($yz as $i) {
        $i = (int)$i;
            $this->assertTrue(in_array($i, array_intersect($y, $z)));
        }

        $zt = $this->redis->sInter('{set}square', '{set}seq');   // prime squares
        $this->assertTrue($zt === []);
        $zt = $this->redis->sInter(['{set}square', '{set}seq']);    // prime squares, as array
        $this->assertTrue($zt === []);

        $xyz = $this->redis->sInter('{set}odd', '{set}prime', '{set}square');// odd prime squares
        $this->assertTrue($xyz === ['1']);

        $xyz = $this->redis->sInter(['{set}odd', '{set}prime', '{set}square']);// odd prime squares, with an array as a parameter
        $this->assertTrue($xyz === ['1']);

        $nil = $this->redis->sInter([]);
        $this->assertTrue($nil === FALSE);
    }

    public function testsInterStore() {
        $this->redis->del('{set}x');  // set of odd numbers
        $this->redis->del('{set}y');  // set of prime numbers
        $this->redis->del('{set}z');  // set of squares
        $this->redis->del('{set}t');  // set of numbers of the form n^2 - 1

        $x = [1,3,5,7,9,11,13,15,17,19,21,23,25];
        foreach($x as $i) {
            $this->redis->sAdd('{set}x', $i);
        }

        $y = [1,2,3,5,7,11,13,17,19,23];
        foreach($y as $i) {
            $this->redis->sAdd('{set}y', $i);
        }

        $z = [1,4,9,16,25];
        foreach($z as $i) {
            $this->redis->sAdd('{set}z', $i);
        }

        $t = [2,5,10,17,26];
        foreach($t as $i) {
            $this->redis->sAdd('{set}t', $i);
        }

        /* Regression test for passing a single array */
        $this->assertEquals($this->redis->sInterStore(['{set}k', '{set}x', '{set}y']), count(array_intersect($x,$y)));

        $count = $this->redis->sInterStore('{set}k', '{set}x', '{set}y');  // odd prime numbers
        $this->assertEquals($count, $this->redis->scard('{set}k'));
        foreach(array_intersect($x, $y) as $i) {
            $this->assertTrue($this->redis->sismember('{set}k', $i));
        }

        $count = $this->redis->sInterStore('{set}k', '{set}y', '{set}z');  // set of odd squares
        $this->assertEquals($count, $this->redis->scard('{set}k'));
        foreach(array_intersect($y, $z) as $i) {
            $this->assertTrue($this->redis->sismember('{set}k', $i));
        }

        $count = $this->redis->sInterStore('{set}k', '{set}z', '{set}t');  // squares of the form n^2 + 1
        $this->assertEquals($count, 0);
        $this->assertEquals($count, $this->redis->scard('{set}k'));

        $this->redis->del('{set}z');
        $xyz = $this->redis->sInterStore('{set}k', '{set}x', '{set}y', '{set}z'); // only z missing, expect 0.
        $this->assertTrue($xyz === 0);

        $this->redis->del('{set}y');
        $xyz = $this->redis->sInterStore('{set}k', '{set}x', '{set}y', '{set}z'); // y and z missing, expect 0.
        $this->assertTrue($xyz === 0);

        $this->redis->del('{set}x');
        $xyz = $this->redis->sInterStore('{set}k', '{set}x', '{set}y', '{set}z'); // x y and z ALL missing, expect 0.
        $this->assertTrue($xyz === 0);
    }

    public function testsUnion() {
        $this->redis->del('{set}x');  // set of odd numbers
        $this->redis->del('{set}y');  // set of prime numbers
        $this->redis->del('{set}z');  // set of squares
        $this->redis->del('{set}t');  // set of numbers of the form n^2 - 1

        $x = [1,3,5,7,9,11,13,15,17,19,21,23,25];
        foreach($x as $i) {
            $this->redis->sAdd('{set}x', $i);
        }

        $y = [1,2,3,5,7,11,13,17,19,23];
        foreach($y as $i) {
            $this->redis->sAdd('{set}y', $i);
        }

        $z = [1,4,9,16,25];
        foreach($z as $i) {
            $this->redis->sAdd('{set}z', $i);
        }

        $t = [2,5,10,17,26];
        foreach($t as $i) {
            $this->redis->sAdd('{set}t', $i);
        }

        $xy = $this->redis->sUnion('{set}x', '{set}y');   // x U y
        foreach($xy as $i) {
        $i = (int)$i;
            $this->assertTrue(in_array($i, array_merge($x, $y)));
        }

        $yz = $this->redis->sUnion('{set}y', '{set}z');   // y U Z
        foreach($yz as $i) {
        $i = (int)$i;
            $this->assertTrue(in_array($i, array_merge($y, $z)));
        }

        $zt = $this->redis->sUnion('{set}z', '{set}t');   // z U t
        foreach($zt as $i) {
        $i = (int)$i;
            $this->assertTrue(in_array($i, array_merge($z, $t)));
        }

        $xyz = $this->redis->sUnion('{set}x', '{set}y', '{set}z'); // x U y U z
        foreach($xyz as $i) {
        $i = (int)$i;
            $this->assertTrue(in_array($i, array_merge($x, $y, $z)));
        }
    }

    public function testsUnionStore() {
        $this->redis->del('{set}x');  // set of odd numbers
        $this->redis->del('{set}y');  // set of prime numbers
        $this->redis->del('{set}z');  // set of squares
        $this->redis->del('{set}t');  // set of numbers of the form n^2 - 1

        $x = [1,3,5,7,9,11,13,15,17,19,21,23,25];
        foreach($x as $i) {
            $this->redis->sAdd('{set}x', $i);
        }

        $y = [1,2,3,5,7,11,13,17,19,23];
        foreach($y as $i) {
            $this->redis->sAdd('{set}y', $i);
        }

        $z = [1,4,9,16,25];
        foreach($z as $i) {
            $this->redis->sAdd('{set}z', $i);
        }

        $t = [2,5,10,17,26];
        foreach($t as $i) {
            $this->redis->sAdd('{set}t', $i);
        }

        $count = $this->redis->sUnionStore('{set}k', '{set}x', '{set}y');  // x U y
        $xy = array_unique(array_merge($x, $y));
        $this->assertEquals($count, count($xy));
        foreach($xy as $i) {
        $i = (int)$i;
            $this->assertTrue($this->redis->sismember('{set}k', $i));
        }

        $count = $this->redis->sUnionStore('{set}k', '{set}y', '{set}z');  // y U z
        $yz = array_unique(array_merge($y, $z));
        $this->assertEquals($count, count($yz));
        foreach($yz as $i) {
        $i = (int)$i;
            $this->assertTrue($this->redis->sismember('{set}k', $i));
        }

        $count = $this->redis->sUnionStore('{set}k', '{set}z', '{set}t');  // z U t
        $zt = array_unique(array_merge($z, $t));
        $this->assertEquals($count, count($zt));
        foreach($zt as $i) {
        $i = (int)$i;
            $this->assertTrue($this->redis->sismember('{set}k', $i));
        }

        $count = $this->redis->sUnionStore('{set}k', '{set}x', '{set}y', '{set}z'); // x U y U z
        $xyz = array_unique(array_merge($x, $y, $z));
        $this->assertEquals($count, count($xyz));
        foreach($xyz as $i) {
        $i = (int)$i;
            $this->assertTrue($this->redis->sismember('{set}k', $i));
        }

        $this->redis->del('{set}x');  // x missing now
        $count = $this->redis->sUnionStore('{set}k', '{set}x', '{set}y', '{set}z'); // x U y U z
        $this->assertTrue($count === count(array_unique(array_merge($y, $z))));

        $this->redis->del('{set}y');  // x and y missing
        $count = $this->redis->sUnionStore('{set}k', '{set}x', '{set}y', '{set}z'); // x U y U z
        $this->assertTrue($count === count(array_unique($z)));

        $this->redis->del('{set}z');  // x, y, and z ALL missing
        $count = $this->redis->sUnionStore('{set}k', '{set}x', '{set}y', '{set}z'); // x U y U z
        $this->assertTrue($count === 0);
    }

    public function testsDiff() {
        $this->redis->del('{set}x');  // set of odd numbers
        $this->redis->del('{set}y');  // set of prime numbers
        $this->redis->del('{set}z');  // set of squares
        $this->redis->del('{set}t');  // set of numbers of the form n^2 - 1

        $x = [1,3,5,7,9,11,13,15,17,19,21,23,25];
        foreach($x as $i) {
            $this->redis->sAdd('{set}x', $i);
        }

        $y = [1,2,3,5,7,11,13,17,19,23];
        foreach($y as $i) {
            $this->redis->sAdd('{set}y', $i);
        }

        $z = [1,4,9,16,25];
        foreach($z as $i) {
            $this->redis->sAdd('{set}z', $i);
        }

        $t = [2,5,10,17,26];
        foreach($t as $i) {
            $this->redis->sAdd('{set}t', $i);
        }

        $xy = $this->redis->sDiff('{set}x', '{set}y');    // x U y
        foreach($xy as $i) {
        $i = (int)$i;
            $this->assertTrue(in_array($i, array_diff($x, $y)));
        }

        $yz = $this->redis->sDiff('{set}y', '{set}z');    // y U Z
        foreach($yz as $i) {
        $i = (int)$i;
            $this->assertTrue(in_array($i, array_diff($y, $z)));
        }

        $zt = $this->redis->sDiff('{set}z', '{set}t');    // z U t
        foreach($zt as $i) {
        $i = (int)$i;
            $this->assertTrue(in_array($i, array_diff($z, $t)));
        }

        $xyz = $this->redis->sDiff('{set}x', '{set}y', '{set}z'); // x U y U z
        foreach($xyz as $i) {
        $i = (int)$i;
            $this->assertTrue(in_array($i, array_diff($x, $y, $z)));
        }
    }

    public function testsDiffStore() {
        $this->redis->del('{set}x');  // set of odd numbers
        $this->redis->del('{set}y');  // set of prime numbers
        $this->redis->del('{set}z');  // set of squares
        $this->redis->del('{set}t');  // set of numbers of the form n^2 - 1

        $x = [1,3,5,7,9,11,13,15,17,19,21,23,25];
        foreach($x as $i) {
            $this->redis->sAdd('{set}x', $i);
        }

        $y = [1,2,3,5,7,11,13,17,19,23];
        foreach($y as $i) {
            $this->redis->sAdd('{set}y', $i);
        }

        $z = [1,4,9,16,25];
        foreach($z as $i) {
            $this->redis->sAdd('{set}z', $i);
        }

        $t = [2,5,10,17,26];
        foreach($t as $i) {
            $this->redis->sAdd('{set}t', $i);
        }

        $count = $this->redis->sDiffStore('{set}k', '{set}x', '{set}y');   // x - y
        $xy = array_unique(array_diff($x, $y));
        $this->assertEquals($count, count($xy));
        foreach($xy as $i) {
            $i = (int)$i;
            $this->assertTrue($this->redis->sismember('{set}k', $i));
        }

        $count = $this->redis->sDiffStore('{set}k', '{set}y', '{set}z');   // y - z
        $yz = array_unique(array_diff($y, $z));
        $this->assertEquals($count, count($yz));
        foreach($yz as $i) {
        $i = (int)$i;
            $this->assertTrue($this->redis->sismember('{set}k', $i));
        }

        $count = $this->redis->sDiffStore('{set}k', '{set}z', '{set}t');   // z - t
        $zt = array_unique(array_diff($z, $t));
        $this->assertEquals($count, count($zt));
        foreach($zt as $i) {
        $i = (int)$i;
            $this->assertTrue($this->redis->sismember('{set}k', $i));
        }

        $count = $this->redis->sDiffStore('{set}k', '{set}x', '{set}y', '{set}z');  // x - y - z
        $xyz = array_unique(array_diff($x, $y, $z));
        $this->assertEquals($count, count($xyz));
        foreach($xyz as $i) {
        $i = (int)$i;
            $this->assertTrue($this->redis->sismember('{set}k', $i));
        }

        $this->redis->del('{set}x');  // x missing now
        $count = $this->redis->sDiffStore('{set}k', '{set}x', '{set}y', '{set}z');  // x - y - z
        $this->assertTrue($count === 0);

        $this->redis->del('{set}y');  // x and y missing
        $count = $this->redis->sDiffStore('{set}k', '{set}x', '{set}y', '{set}z');  // x - y - z
        $this->assertTrue($count === 0);

        $this->redis->del('{set}z');  // x, y, and z ALL missing
        $count = $this->redis->sDiffStore('{set}k', '{set}x', '{set}y', '{set}z');  // x - y - z
        $this->assertTrue($count === 0);
    }

    public function testlrange() {
        $this->redis->del('list');
        $this->redis->lPush('list', 'val');
        $this->redis->lPush('list', 'val2');
        $this->redis->lPush('list', 'val3');

        // pos :   0     1     2
        // pos :  -3    -2    -1
        // list: [val3, val2, val]

        $this->assertEquals($this->redis->lrange('list', 0, 0), ['val3']);
        $this->assertEquals($this->redis->lrange('list', 0, 1), ['val3', 'val2']);
        $this->assertEquals($this->redis->lrange('list', 0, 2), ['val3', 'val2', 'val']);
        $this->assertEquals($this->redis->lrange('list', 0, 3), ['val3', 'val2', 'val']);

        $this->assertEquals($this->redis->lrange('list', 0, -1), ['val3', 'val2', 'val']);
        $this->assertEquals($this->redis->lrange('list', 0, -2), ['val3', 'val2']);
        $this->assertEquals($this->redis->lrange('list', -2, -1), ['val2', 'val']);

        $this->redis->del('list');
        $this->assertEquals($this->redis->lrange('list', 0, -1), []);
    }

    public function testdbSize() {
        $this->assertTrue($this->redis->flushDB());
        $this->redis->set('x', 'y');
        $this->assertTrue($this->redis->dbSize() === 1);
    }

    public function testttl() {
        $this->redis->set('x', 'y');
        $this->redis->expire('x', 5);
        for($i = 5; $i > 0; $i--) {
            $this->assertEquals($i, $this->redis->ttl('x'));
            sleep(1);
        }

        // A key with no TTL
        $this->redis->del('x'); $this->redis->set('x', 'bar');
        $this->assertEquals($this->redis->ttl('x'), -1);

        // A key that doesn't exist (> 2.8 will return -2)
        if(version_compare($this->version, "2.8.0") >= 0) {
            $this->redis->del('x');
            $this->assertEquals($this->redis->ttl('x'), -2);
        }
    }

    public function testPersist() {
        $this->redis->set('x', 'y');
        $this->redis->expire('x', 100);
        $this->assertTrue(TRUE === $this->redis->persist('x'));     // true if there is a timeout
        $this->assertTrue(-1 === $this->redis->ttl('x'));       // -1: timeout has been removed.
        $this->assertTrue(FALSE === $this->redis->persist('x'));    // false if there is no timeout
        $this->redis->del('x');
        $this->assertTrue(FALSE === $this->redis->persist('x'));    // false if the key doesn’t exist.
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

        if (version_compare($this->version, "6.2.0") >= 0) {
            $this->assertFalse(empty($this->redis->client('info')));
        }

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

    public function testWait() {
        // Closest we can check based on redis commmit history
        if(version_compare($this->version, '2.9.11') < 0) {
            $this->markTestSkipped();
            return;
        }

        // We could have slaves here, so determine that
        $arr_slaves = $this->redis->info();
        $i_slaves   = $arr_slaves['connected_slaves'];

        // Send a couple commands
        $this->redis->set('wait-foo', 'over9000');
        $this->redis->set('wait-bar', 'revo9000');

        // Make sure we get the right replication count
        $this->assertEquals($this->redis->wait($i_slaves, 100), $i_slaves);

        // Pass more slaves than are connected
        $this->redis->set('wait-foo','over9000');
        $this->redis->set('wait-bar','revo9000');
        $this->assertTrue($this->redis->wait($i_slaves+1, 100) < $i_slaves+1);

        // Make sure when we pass with bad arguments we just get back false
        $this->assertFalse($this->redis->wait(-1, -1));
        $this->assertFalse($this->redis->wait(-1, 20));
    }

    public function testInfo() {
        foreach ([false, true] as $boo_multi) {
            if ($boo_multi) {
                $this->redis->multi();
                $this->redis->info();
                $info = $this->redis->exec();
                $info = $info[0];
            } else {
                $info = $this->redis->info();
            }

            $keys = [
                "redis_version",
                "arch_bits",
                "uptime_in_seconds",
                "uptime_in_days",
                "connected_clients",
                "connected_slaves",
                "used_memory",
                "total_connections_received",
                "total_commands_processed",
                "role"
            ];
            if (version_compare($this->version, "2.5.0") < 0) {
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
    }

    public function testInfoCommandStats() {

    // INFO COMMANDSTATS is new in 2.6.0
    if (version_compare($this->version, "2.5.0") < 0) {
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

    public function testSwapDB() {
        if (version_compare($this->version, "4.0.0") < 0) {
            $this->markTestSkipped();
        }

        $this->assertTrue($this->redis->swapdb(0, 1));
        $this->assertTrue($this->redis->swapdb(0, 1));
    }

    public function testMset() {
    $this->redis->del('x', 'y', 'z');    // remove x y z
    $this->assertTrue($this->redis->mset(['x' => 'a', 'y' => 'b', 'z' => 'c']));   // set x y z

    $this->assertEquals($this->redis->mget(['x', 'y', 'z']), ['a', 'b', 'c']);    // check x y z

    $this->redis->del('x');  // delete just x
    $this->assertTrue($this->redis->mset(['x' => 'a', 'y' => 'b', 'z' => 'c']));   // set x y z
    $this->assertEquals($this->redis->mget(['x', 'y', 'z']), ['a', 'b', 'c']);    // check x y z

    $this->assertFalse($this->redis->mset([])); // set ø → FALSE


    /*
     * Integer keys
     */

    // No prefix
    $set_array = [-1 => 'neg1', -2 => 'neg2', -3 => 'neg3', 1 => 'one', 2 => 'two', '3' => 'three'];
    $this->redis->del(array_keys($set_array));
    $this->assertTrue($this->redis->mset($set_array));
    $this->assertEquals($this->redis->mget(array_keys($set_array)), array_values($set_array));
    $this->redis->del(array_keys($set_array));

    // With a prefix
    $this->redis->setOption(Redis::OPT_PREFIX, 'pfx:');
    $this->redis->del(array_keys($set_array));
    $this->assertTrue($this->redis->mset($set_array));
    $this->assertEquals($this->redis->mget(array_keys($set_array)), array_values($set_array));
    $this->redis->del(array_keys($set_array));
    $this->redis->setOption(Redis::OPT_PREFIX, '');
    }

    public function testMsetNX() {
        $this->redis->del('x', 'y', 'z');    // remove x y z
        $this->assertTrue(TRUE === $this->redis->msetnx(['x' => 'a', 'y' => 'b', 'z' => 'c']));    // set x y z

        $this->assertEquals($this->redis->mget(['x', 'y', 'z']), ['a', 'b', 'c']);    // check x y z

        $this->redis->del('x');  // delete just x
        $this->assertTrue(FALSE === $this->redis->msetnx(['x' => 'A', 'y' => 'B', 'z' => 'C']));   // set x y z
        $this->assertEquals($this->redis->mget(['x', 'y', 'z']), [FALSE, 'b', 'c']);  // check x y z

        $this->assertFalse($this->redis->msetnx([])); // set ø → FALSE
    }

    public function testRpopLpush() {
        // standard case.
        $this->redis->del('{list}x', '{list}y');
        $this->redis->lpush('{list}x', 'abc');
        $this->redis->lpush('{list}x', 'def');    // x = [def, abc]

        $this->redis->lpush('{list}y', '123');
        $this->redis->lpush('{list}y', '456');    // y = [456, 123]

        $this->assertEquals($this->redis->rpoplpush('{list}x', '{list}y'), 'abc');  // we RPOP x, yielding abc.
        $this->assertEquals($this->redis->lrange('{list}x', 0, -1), ['def']); // only def remains in x.
        $this->assertEquals($this->redis->lrange('{list}y', 0, -1), ['abc', '456', '123']);   // abc has been lpushed to y.

        // with an empty source, expecting no change.
        $this->redis->del('{list}x', '{list}y');
        $this->assertTrue(FALSE === $this->redis->rpoplpush('{list}x', '{list}y'));
        $this->assertTrue([] === $this->redis->lrange('{list}x', 0, -1));
        $this->assertTrue([] === $this->redis->lrange('{list}y', 0, -1));
    }

    public function testBRpopLpush() {
        // standard case.
        $this->redis->del('{list}x', '{list}y');
        $this->redis->lpush('{list}x', 'abc');
        $this->redis->lpush('{list}x', 'def');    // x = [def, abc]

        $this->redis->lpush('{list}y', '123');
        $this->redis->lpush('{list}y', '456');    // y = [456, 123]

        $this->assertEquals($this->redis->brpoplpush('{list}x', '{list}y', 1), 'abc');  // we RPOP x, yielding abc.
        $this->assertEquals($this->redis->lrange('{list}x', 0, -1), ['def']); // only def remains in x.
        $this->assertEquals($this->redis->lrange('{list}y', 0, -1), ['abc', '456', '123']);   // abc has been lpushed to y.

        // with an empty source, expecting no change.
        $this->redis->del('{list}x', '{list}y');
        $this->assertTrue(FALSE === $this->redis->brpoplpush('{list}x', '{list}y', 1));
        $this->assertTrue([] === $this->redis->lrange('{list}x', 0, -1));
        $this->assertTrue([] === $this->redis->lrange('{list}y', 0, -1));
    }

    public function testZAddFirstArg() {

        $this->redis->del('key');

        $zsetName = 100; // not a string!
        $this->assertTrue(1 === $this->redis->zAdd($zsetName, 0, 'val0'));
        $this->assertTrue(1 === $this->redis->zAdd($zsetName, 1, 'val1'));

        $this->assertTrue(['val0', 'val1'] === $this->redis->zRange($zsetName, 0, -1));
    }

    public function testZX() {
        $this->redis->del('key');

        $this->assertTrue([] === $this->redis->zRange('key', 0, -1));
        $this->assertTrue([] === $this->redis->zRange('key', 0, -1, true));

        $this->assertTrue(1 === $this->redis->zAdd('key', 0, 'val0'));
        $this->assertTrue(1 === $this->redis->zAdd('key', 2, 'val2'));
        $this->assertTrue(2 === $this->redis->zAdd('key', 4, 'val4', 5, 'val5')); // multiple parameters
        if (version_compare($this->version, "3.0.2") < 0) {
            $this->assertTrue(1 === $this->redis->zAdd('key', 1, 'val1'));
            $this->assertTrue(1 === $this->redis->zAdd('key', 3, 'val3'));
        } else {
            $this->assertTrue(1 === $this->redis->zAdd('key', [], 1, 'val1')); // empty options
            $this->assertTrue(1 === $this->redis->zAdd('key', ['nx'], 3, 'val3')); // nx option
            $this->assertTrue(0 === $this->redis->zAdd('key', ['xx'], 3, 'val3')); // xx option
        }

        $this->assertTrue(['val0', 'val1', 'val2', 'val3', 'val4', 'val5'] === $this->redis->zRange('key', 0, -1));

        // withscores
        $ret = $this->redis->zRange('key', 0, -1, true);
        $this->assertTrue(count($ret) == 6);
        $this->assertTrue($ret['val0'] == 0);
        $this->assertTrue($ret['val1'] == 1);
        $this->assertTrue($ret['val2'] == 2);
        $this->assertTrue($ret['val3'] == 3);
        $this->assertTrue($ret['val4'] == 4);
        $this->assertTrue($ret['val5'] == 5);

        $this->assertTrue(0 === $this->redis->zRem('key', 'valX'));
        $this->assertTrue(1 === $this->redis->zRem('key', 'val3'));
        $this->assertTrue(1 === $this->redis->zRem('key', 'val4'));
        $this->assertTrue(1 === $this->redis->zRem('key', 'val5'));

        $this->assertTrue(['val0', 'val1', 'val2'] === $this->redis->zRange('key', 0, -1));

        // zGetReverseRange

        $this->assertTrue(1 === $this->redis->zAdd('key', 3, 'val3'));
        $this->assertTrue(1 === $this->redis->zAdd('key', 3, 'aal3'));

        $zero_to_three = $this->redis->zRangeByScore('key', 0, 3);
        $this->assertTrue(['val0', 'val1', 'val2', 'aal3', 'val3'] === $zero_to_three || ['val0', 'val1', 'val2', 'val3', 'aal3'] === $zero_to_three);

        $three_to_zero = $this->redis->zRevRangeByScore('key', 3, 0);
        $this->assertTrue(array_reverse(['val0', 'val1', 'val2', 'aal3', 'val3']) === $three_to_zero || array_reverse(['val0', 'val1', 'val2', 'val3', 'aal3']) === $three_to_zero);

        $this->assertTrue(5 === $this->redis->zCount('key', 0, 3));

        // withscores
        $this->redis->zRem('key', 'aal3');
        $zero_to_three = $this->redis->zRangeByScore('key', 0, 3, ['withscores' => TRUE]);
        $this->assertTrue(['val0' => 0, 'val1' => 1, 'val2' => 2, 'val3' => 3] == $zero_to_three);
        $this->assertTrue(4 === $this->redis->zCount('key', 0, 3));

        // limit
        $this->assertTrue(['val0'] === $this->redis->zRangeByScore('key', 0, 3, ['limit' => [0, 1]]));
        $this->assertTrue(['val0', 'val1'] === $this->redis->zRangeByScore('key', 0, 3, ['limit' => [0, 2]]));
        $this->assertTrue(['val1', 'val2'] === $this->redis->zRangeByScore('key', 0, 3, ['limit' => [1, 2]]));
        $this->assertTrue(['val0', 'val1'] === $this->redis->zRangeByScore('key', 0, 1, ['limit' => [0, 100]]));

        // limits as references
        $limit = [0, 100];
        foreach ($limit as &$val) {}
        $this->assertTrue(['val0', 'val1'] === $this->redis->zRangeByScore('key', 0, 1, ['limit' => $limit]));

        $this->assertTrue(['val3'] === $this->redis->zRevRangeByScore('key', 3, 0, ['limit' => [0, 1]]));
        $this->assertTrue(['val3', 'val2'] === $this->redis->zRevRangeByScore('key', 3, 0, ['limit' => [0, 2]]));
        $this->assertTrue(['val2', 'val1'] === $this->redis->zRevRangeByScore('key', 3, 0, ['limit' => [1, 2]]));
        $this->assertTrue(['val1', 'val0'] === $this->redis->zRevRangeByScore('key', 1, 0, ['limit' => [0, 100]]));

        $this->assertTrue(4 === $this->redis->zCard('key'));
        $this->assertTrue(1.0 === $this->redis->zScore('key', 'val1'));
        $this->assertFalse($this->redis->zScore('key', 'val'));
        $this->assertFalse($this->redis->zScore(3, 2));

        // with () and +inf, -inf
        $this->redis->del('zset');
        $this->redis->zAdd('zset', 1, 'foo');
        $this->redis->zAdd('zset', 2, 'bar');
        $this->redis->zAdd('zset', 3, 'biz');
        $this->redis->zAdd('zset', 4, 'foz');
        $this->assertTrue(['foo' => 1, 'bar' => 2, 'biz' => 3, 'foz' => 4] == $this->redis->zRangeByScore('zset', '-inf', '+inf', ['withscores' => TRUE]));
        $this->assertTrue(['foo' => 1, 'bar' => 2] == $this->redis->zRangeByScore('zset', 1, 2, ['withscores' => TRUE]));
        $this->assertTrue(['bar' => 2] == $this->redis->zRangeByScore('zset', '(1', 2, ['withscores' => TRUE]));
        $this->assertTrue([] == $this->redis->zRangeByScore('zset', '(1', '(2', ['withscores' => TRUE]));

        $this->assertTrue(4 == $this->redis->zCount('zset', '-inf', '+inf'));
        $this->assertTrue(2 == $this->redis->zCount('zset', 1, 2));
        $this->assertTrue(1 == $this->redis->zCount('zset', '(1', 2));
        $this->assertTrue(0 == $this->redis->zCount('zset', '(1', '(2'));


        // zincrby
        $this->redis->del('key');
        $this->assertTrue(1.0 === $this->redis->zIncrBy('key', 1, 'val1'));
        $this->assertTrue(1.0 === $this->redis->zScore('key', 'val1'));
        $this->assertTrue(2.5 === $this->redis->zIncrBy('key', 1.5, 'val1'));
        $this->assertTrue(2.5 === $this->redis->zScore('key', 'val1'));

        // zUnionStore
        $this->redis->del('{zset}1');
        $this->redis->del('{zset}2');
        $this->redis->del('{zset}3');
        $this->redis->del('{zset}U');

        $this->redis->zAdd('{zset}1', 0, 'val0');
        $this->redis->zAdd('{zset}1', 1, 'val1');

        $this->redis->zAdd('{zset}2', 2, 'val2');
        $this->redis->zAdd('{zset}2', 3, 'val3');

        $this->redis->zAdd('{zset}3', 4, 'val4');
        $this->redis->zAdd('{zset}3', 5, 'val5');

        $this->assertTrue(4 === $this->redis->zUnionStore('{zset}U', ['{zset}1', '{zset}3']));
        $this->assertTrue(['val0', 'val1', 'val4', 'val5'] === $this->redis->zRange('{zset}U', 0, -1));

        // Union on non existing keys
        $this->redis->del('{zset}U');
        $this->assertTrue(0 === $this->redis->zUnionStore('{zset}U', ['{zset}X', '{zset}Y']));
        $this->assertTrue([] === $this->redis->zRange('{zset}U', 0, -1));

        // !Exist U Exist → copy of existing zset.
        $this->redis->del('{zset}U', 'X');
        $this->assertTrue(2 === $this->redis->zUnionStore('{zset}U', ['{zset}1', '{zset}X']));

        // test weighted zUnion
        $this->redis->del('{zset}Z');
        $this->assertTrue(4 === $this->redis->zUnionStore('{zset}Z', ['{zset}1', '{zset}2'], [1, 1]));
        $this->assertTrue(['val0', 'val1', 'val2', 'val3'] === $this->redis->zRange('{zset}Z', 0, -1));

        $this->redis->zRemRangeByScore('{zset}Z', 0, 10);
        $this->assertTrue(4 === $this->redis->zUnionStore('{zset}Z', ['{zset}1', '{zset}2'], [5, 1]));
        $this->assertTrue(['val0', 'val2', 'val3', 'val1'] === $this->redis->zRange('{zset}Z', 0, -1));

        $this->redis->del('{zset}1');
        $this->redis->del('{zset}2');
        $this->redis->del('{zset}3');

        //test zUnion with weights and aggegration function
        $this->redis->zadd('{zset}1', 1, 'duplicate');
        $this->redis->zadd('{zset}2', 2, 'duplicate');
        $this->redis->zUnionStore('{zset}U', ['{zset}1','{zset}2'], [1,1], 'MIN');
        $this->assertTrue($this->redis->zScore('{zset}U', 'duplicate')===1.0);
        $this->redis->del('{zset}U');

        //now test zUnion *without* weights but with aggregrate function
        $this->redis->zUnionStore('{zset}U', ['{zset}1','{zset}2'], null, 'MIN');
        $this->assertTrue($this->redis->zScore('{zset}U', 'duplicate')===1.0);
        $this->redis->del('{zset}U', '{zset}1', '{zset}2');

        // test integer and float weights (GitHub issue #109).
        $this->redis->del('{zset}1', '{zset}2', '{zset}3');

        $this->redis->zadd('{zset}1', 1, 'one');
        $this->redis->zadd('{zset}1', 2, 'two');
        $this->redis->zadd('{zset}2', 1, 'one');
        $this->redis->zadd('{zset}2', 2, 'two');
        $this->redis->zadd('{zset}2', 3, 'three');

        $this->assertTrue($this->redis->zUnionStore('{zset}3', ['{zset}1', '{zset}2'], [2, 3.0]) === 3);

        $this->redis->del('{zset}1');
        $this->redis->del('{zset}2');
        $this->redis->del('{zset}3');

        // Test 'inf', '-inf', and '+inf' weights (GitHub issue #336)
        $this->redis->zadd('{zset}1', 1, 'one', 2, 'two', 3, 'three');
        $this->redis->zadd('{zset}2', 3, 'three', 4, 'four', 5, 'five');

        // Make sure phpredis handles these weights
        $this->assertTrue($this->redis->zUnionStore('{zset}3', ['{zset}1','{zset}2'], [1, 'inf'])  === 5);
        $this->assertTrue($this->redis->zUnionStore('{zset}3', ['{zset}1','{zset}2'], [1, '-inf']) === 5);
        $this->assertTrue($this->redis->zUnionStore('{zset}3', ['{zset}1','{zset}2'], [1, '+inf']) === 5);

        // Now, confirm that they're being sent, and that it works
        $arr_weights = ['inf','-inf','+inf'];

        foreach($arr_weights as $str_weight) {
            $r = $this->redis->zUnionStore('{zset}3', ['{zset}1','{zset}2'], [1,$str_weight]);
            $this->assertTrue($r===5);
            $r = $this->redis->zrangebyscore('{zset}3', '(-inf', '(inf',['withscores'=>true]);
            $this->assertTrue(count($r)===2);
            $this->assertTrue(isset($r['one']));
            $this->assertTrue(isset($r['two']));
        }

        $this->redis->del('{zset}1','{zset}2','{zset}3');

        $this->redis->zadd('{zset}1', 2000.1, 'one');
        $this->redis->zadd('{zset}1', 3000.1, 'two');
        $this->redis->zadd('{zset}1', 4000.1, 'three');

        $ret = $this->redis->zRange('{zset}1', 0, -1, TRUE);
        $this->assertTrue(count($ret) === 3);
        $retValues = array_keys($ret);

        $this->assertTrue(['one', 'two', 'three'] === $retValues);

        // + 0 converts from string to float OR integer
        $this->assertTrue(is_float($ret['one'] + 0));
        $this->assertTrue(is_float($ret['two'] + 0));
        $this->assertTrue(is_float($ret['three'] + 0));

        $this->redis->del('{zset}1');

        // ZREMRANGEBYRANK
        $this->redis->zAdd('{zset}1', 1, 'one');
        $this->redis->zAdd('{zset}1', 2, 'two');
        $this->redis->zAdd('{zset}1', 3, 'three');
        $this->assertTrue(2 === $this->redis->zremrangebyrank('{zset}1', 0, 1));
        $this->assertTrue(['three' => 3] == $this->redis->zRange('{zset}1', 0, -1, TRUE));

        $this->redis->del('{zset}1');

        // zInterStore

        $this->redis->zAdd('{zset}1', 0, 'val0');
        $this->redis->zAdd('{zset}1', 1, 'val1');
        $this->redis->zAdd('{zset}1', 3, 'val3');

        $this->redis->zAdd('{zset}2', 2, 'val1');
        $this->redis->zAdd('{zset}2', 3, 'val3');

        $this->redis->zAdd('{zset}3', 4, 'val3');
        $this->redis->zAdd('{zset}3', 5, 'val5');

        $this->redis->del('{zset}I');
        $this->assertTrue(2 === $this->redis->zInterStore('{zset}I', ['{zset}1', '{zset}2']));
        $this->assertTrue(['val1', 'val3'] === $this->redis->zRange('{zset}I', 0, -1));

        // Union on non existing keys
        $this->assertTrue(0 === $this->redis->zInterStore('{zset}X', ['{zset}X', '{zset}Y']));
        $this->assertTrue([] === $this->redis->zRange('{zset}X', 0, -1));

        // !Exist U Exist
        $this->assertTrue(0 === $this->redis->zInterStore('{zset}Y', ['{zset}1', '{zset}X']));
        $this->assertTrue([] === $this->redis->zRange('keyY', 0, -1));


        // test weighted zInterStore
        $this->redis->del('{zset}1');
        $this->redis->del('{zset}2');
        $this->redis->del('{zset}3');

        $this->redis->zAdd('{zset}1', 0, 'val0');
        $this->redis->zAdd('{zset}1', 1, 'val1');
        $this->redis->zAdd('{zset}1', 3, 'val3');


        $this->redis->zAdd('{zset}2', 2, 'val1');
        $this->redis->zAdd('{zset}2', 1, 'val3');

        $this->redis->zAdd('{zset}3', 7, 'val1');
        $this->redis->zAdd('{zset}3', 3, 'val3');

        $this->redis->del('{zset}I');
        $this->assertTrue(2 === $this->redis->zInterStore('{zset}I', ['{zset}1', '{zset}2'], [1, 1]));
        $this->assertTrue(['val1', 'val3'] === $this->redis->zRange('{zset}I', 0, -1));

        $this->redis->del('{zset}I');
        $this->assertTrue( 2 === $this->redis->zInterStore('{zset}I', ['{zset}1', '{zset}2', '{zset}3'], [1, 5, 1], 'min'));
        $this->assertTrue(['val1', 'val3'] === $this->redis->zRange('{zset}I', 0, -1));
        $this->redis->del('{zset}I');
        $this->assertTrue( 2 === $this->redis->zInterStore('{zset}I', ['{zset}1', '{zset}2', '{zset}3'], [1, 5, 1], 'max'));
        $this->assertTrue(['val3', 'val1'] === $this->redis->zRange('{zset}I', 0, -1));

        $this->redis->del('{zset}I');
        $this->assertTrue(2 === $this->redis->zInterStore('{zset}I', ['{zset}1', '{zset}2', '{zset}3'], null, 'max'));
        $this->assertTrue($this->redis->zScore('{zset}I', 'val1') === floatval(7));

        // zrank, zrevrank
        $this->redis->del('z');
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

    public function testZRangeScoreArg() {
        $this->redis->del('{z}');

        $arr_mems = ['one' => 1.0, 'two' => 2.0, 'three' => 3.0];
        foreach ($arr_mems as $str_mem => $score) {
            $this->redis->zAdd('{z}', $score, $str_mem);
        }

        /* Verify we can pass true and ['withscores' => true] */
        $this->assertEquals($arr_mems, $this->redis->zRange('{z}', 0, -1, true));
        $this->assertEquals($arr_mems, $this->redis->zRange('{z}', 0, -1, ['withscores' => true]));
    }

    public function testZRangeByLex() {
        /* ZRANGEBYLEX available on versions >= 2.8.9 */
        if(version_compare($this->version, "2.8.9") < 0) {
            $this->MarkTestSkipped();
            return;
        }

        $this->redis->del('key');
        foreach(range('a', 'g') as $c) {
            $this->redis->zAdd('key', 0, $c);
        }

        $this->assertEquals($this->redis->zRangeByLex('key', '-', '[c'), ['a', 'b', 'c']);
        $this->assertEquals($this->redis->zRangeByLex('key', '(e', '+'), ['f', 'g']);

        // with limit offset
        $this->assertEquals($this->redis->zRangeByLex('key', '-', '[c', 1, 2), ['b', 'c'] );
        $this->assertEquals($this->redis->zRangeByLex('key', '-', '(c', 1, 2), ['b']);
    }

    public function testZLexCount() {
        if (version_compare($this->version, "2.8.9") < 0) {
            $this->MarkTestSkipped();
            return;
        }

        $this->redis->del('key');
        foreach (range('a', 'g') as $c) {
            $entries[] = $c;
            $this->redis->zAdd('key', 0, $c);
        }

        /* Special -/+ values */
        $this->assertEquals($this->redis->zLexCount('key', '-', '-'), 0);
        $this->assertEquals($this->redis->zLexCount('key', '-', '+'), count($entries));

        /* Verify invalid arguments return FALSE */
        $this->assertFalse(@$this->redis->zLexCount('key', '[a', 'bad'));
        $this->assertFalse(@$this->redis->zLexCount('key', 'bad', '[a'));

        /* Now iterate through */
        $start = $entries[0];
        for ($i = 1; $i < count($entries); $i++) {
            $end = $entries[$i];
            $this->assertEquals($this->redis->zLexCount('key', "[$start", "[$end"), $i + 1);
            $this->assertEquals($this->redis->zLexCount('key', "[$start", "($end"), $i);
            $this->assertEquals($this->redis->zLexCount('key', "($start", "($end"), $i - 1);
        }
    }

    public function testzDiff()
    {
        // Only available since 6.2.0
        if (version_compare($this->version, '6.2.0') < 0) {
            $this->markTestSkipped();
            return;
        }

        $this->redis->del('key');
        foreach (range('a', 'c') as $c) {
            $this->redis->zAdd('key', 1, $c);
        }

        $this->assertEquals(['a', 'b', 'c'], $this->redis->zDiff(['key']));
        $this->assertEquals(['a' => 1.0, 'b' => 1.0, 'c' => 1.0], $this->redis->zDiff(['key'], ['withscores' => true]));
    }

    public function testzInter()
    {
        // Only available since 6.2.0
        if (version_compare($this->version, '6.2.0') < 0) {
            $this->markTestSkipped();
            return;
        }

        $this->redis->del('key');
        foreach (range('a', 'c') as $c) {
            $this->redis->zAdd('key', 1, $c);
        }

        $this->assertEquals(['a', 'b', 'c'], $this->redis->zInter(['key']));
        $this->assertEquals(['a' => 1.0, 'b' => 1.0, 'c' => 1.0], $this->redis->zInter(['key'], null, ['withscores' => true]));
    }

    public function testzUnion()
    {
        // Only available since 6.2.0
        if (version_compare($this->version, '6.2.0') < 0) {
            $this->markTestSkipped();
            return;
        }

        $this->redis->del('key');
        foreach (range('a', 'c') as $c) {
            $this->redis->zAdd('key', 1, $c);
        }

        $this->assertEquals(['a', 'b', 'c'], $this->redis->zUnion(['key']));
        $this->assertEquals(['a' => 1.0, 'b' => 1.0, 'c' => 1.0], $this->redis->zUnion(['key'], null, ['withscores' => true]));
    }

    public function testzDiffStore()
    {
        // Only available since 6.2.0
        if (version_compare($this->version, '6.2.0') < 0) {
            $this->markTestSkipped();
            return;
        }

        $this->redis->del('key');
        foreach (range('a', 'c') as $c) {
            $this->redis->zAdd('key', 1, $c);
        }
        $this->assertEquals(3, $this->redis->zDiffStore('key2', ['key']));
        $this->assertEquals(['a', 'b', 'c'], $this->redis->zRange('key2', 0, -1));
    }

    public function testzMscore()
    {
        // Only available since 6.2.0
        if (version_compare($this->version, '6.2.0') < 0) {
            $this->markTestSkipped();
            return;
        }

        $this->redis->del('key');
        foreach (range('a', 'c') as $c) {
            $this->redis->zAdd('key', 1, $c);
        }

        $scores = $this->redis->zMscore('key', 'a', 'notamember', 'c');
        $this->assertEquals([1.0, false, 1.0], $scores);

        $scores = $this->redis->zMscore('wrongkey', 'a', 'b', 'c');
        $this->assertEquals([false, false, false], $scores);
    }

    public function testZRemRangeByLex() {
        if (version_compare($this->version, "2.8.9") < 0) {
            $this->MarkTestSkipped();
            return;
        }

        $this->redis->del('key');
        $this->redis->zAdd('key', 0, 'a', 0, 'b', 0, 'c');
        $this->assertEquals($this->redis->zRemRangeByLex('key', '-', '+'), 3);

        $this->redis->zAdd('key', 0, 'a', 0, 'b', 0, 'c');
        $this->assertEquals($this->redis->zRemRangeByLex('key', '[a', '[c'), 3);

        $this->redis->zAdd('key', 0, 'a', 0, 'b', 0, 'c');
        $this->assertEquals($this->redis->zRemRangeByLex('key', '[a', '(a'), 0);
        $this->assertEquals($this->redis->zRemRangeByLex('key', '(a', '(c'), 1);
        $this->assertEquals($this->redis->zRemRangeByLex('key', '[a', '[c'), 2);
    }

    public function testBZPop() {
        if (version_compare($this->version, "5.0.0") < 0) {
            $this->MarkTestSkipped();
            return;
        }

        $this->redis->del('{zs}1', '{zs}2');
        $this->redis->zAdd('{zs}1', 0, 'a', 1, 'b', 2, 'c');
        $this->redis->zAdd('{zs}2', 3, 'A', 4, 'B', 5, 'D');

        $this->assertEquals(Array('{zs}1', 'a', '0'), $this->redis->bzPopMin('{zs}1', '{zs}2', 0));
        $this->assertEquals(Array('{zs}1', 'c', '2'), $this->redis->bzPopMax(Array('{zs}1', '{zs}2'), 0));
        $this->assertEquals(Array('{zs}2', 'A', '3'), $this->redis->bzPopMin('{zs}2', '{zs}1', 0));

        /* Verify timeout is being sent */
        $this->redis->del('{zs}1', '{zs}2');
        $st = microtime(true) * 1000;
        $this->redis->bzPopMin('{zs}1', '{zs}2', 1);
        $et = microtime(true) * 1000;
        $this->assertTrue($et - $st > 100);
    }

    public function testZPop() {
        if (version_compare($this->version, "5.0.0") < 0) {
            $this->MarkTestSkipped();
            return;
        }

        // zPopMax and zPopMin without a COUNT argument
        $this->redis->del('key');
        $this->redis->zAdd('key', 0, 'a', 1, 'b', 2, 'c', 3, 'd', 4, 'e');
        $this->assertTrue(array('e' => 4.0) === $this->redis->zPopMax('key'));
        $this->assertTrue(array('a' => 0.0) === $this->redis->zPopMin('key'));

        // zPopMax with a COUNT argument
        $this->redis->del('key');
        $this->redis->zAdd('key', 0, 'a', 1, 'b', 2, 'c', 3, 'd', 4, 'e');
        $this->assertTrue(array('e' => 4.0, 'd' => 3.0, 'c' => 2.0) === $this->redis->zPopMax('key', 3));

        // zPopMin with a COUNT argument
        $this->redis->del('key');
        $this->redis->zAdd('key', 0, 'a', 1, 'b', 2, 'c', 3, 'd', 4, 'e');
        $this->assertTrue(array('a' => 0.0, 'b' => 1.0, 'c' => 2.0) === $this->redis->zPopMin('key', 3));
    }

    public function testHashes() {
        $this->redis->del('h', 'key');
        $this->assertTrue(0 === $this->redis->hLen('h'));
        $this->assertTrue(1 === $this->redis->hSet('h', 'a', 'a-value'));
        $this->assertTrue(1 === $this->redis->hLen('h'));
        $this->assertTrue(1 === $this->redis->hSet('h', 'b', 'b-value'));
        $this->assertTrue(2 === $this->redis->hLen('h'));

        $this->assertTrue('a-value' === $this->redis->hGet('h', 'a'));  // simple get
        $this->assertTrue('b-value' === $this->redis->hGet('h', 'b'));  // simple get

        $this->assertTrue(0 === $this->redis->hSet('h', 'a', 'another-value')); // replacement
        $this->assertTrue('another-value' === $this->redis->hGet('h', 'a'));    // get the new value

        $this->assertTrue('b-value' === $this->redis->hGet('h', 'b'));  // simple get
        $this->assertTrue(FALSE === $this->redis->hGet('h', 'c'));  // unknown hash member
        $this->assertTrue(FALSE === $this->redis->hGet('key', 'c'));    // unknownkey

        // hDel
        $this->assertTrue(1 === $this->redis->hDel('h', 'a')); // 1 on success
        $this->assertTrue(0 === $this->redis->hDel('h', 'a')); // 0 on failure

        $this->redis->del('h');
        $this->redis->hSet('h', 'x', 'a');
        $this->redis->hSet('h', 'y', 'b');
        $this->assertTrue(2 === $this->redis->hDel('h', 'x', 'y')); // variadic

        // hsetnx
        $this->redis->del('h');
        $this->assertTrue(TRUE === $this->redis->hSetNx('h', 'x', 'a'));
        $this->assertTrue(TRUE === $this->redis->hSetNx('h', 'y', 'b'));
        $this->assertTrue(FALSE === $this->redis->hSetNx('h', 'x', '?'));
        $this->assertTrue(FALSE === $this->redis->hSetNx('h', 'y', '?'));
        $this->assertTrue('a' === $this->redis->hGet('h', 'x'));
        $this->assertTrue('b' === $this->redis->hGet('h', 'y'));

        // keys
        $keys = $this->redis->hKeys('h');
        $this->assertTrue($keys === ['x', 'y'] || $keys === ['y', 'x']);

        // values
        $values = $this->redis->hVals('h');
        $this->assertTrue($values === ['a', 'b'] || $values === ['b', 'a']);

        // keys + values
        $all = $this->redis->hGetAll('h');
        $this->assertTrue($all === ['x' => 'a', 'y' => 'b'] || $all === ['y' => 'b', 'x' => 'a']);

        // hExists
        $this->assertTrue(TRUE === $this->redis->hExists('h', 'x'));
        $this->assertTrue(TRUE === $this->redis->hExists('h', 'y'));
        $this->assertTrue(FALSE === $this->redis->hExists('h', 'w'));
        $this->redis->del('h');
        $this->assertTrue(FALSE === $this->redis->hExists('h', 'x'));

        // hIncrBy
        $this->redis->del('h');
        $this->assertTrue(2 === $this->redis->hIncrBy('h', 'x', 2));
        $this->assertTrue(3 === $this->redis->hIncrBy('h', 'x', 1));
        $this->assertTrue(2 === $this->redis->hIncrBy('h', 'x', -1));
        $this->assertTrue("2" === $this->redis->hGet('h', 'x'));
        $this->assertTrue(PHP_INT_MAX === $this->redis->hIncrBy('h', 'x', PHP_INT_MAX-2));
        $this->assertTrue("".PHP_INT_MAX === $this->redis->hGet('h', 'x'));

        $this->redis->hSet('h', 'y', 'not-a-number');
        $this->assertTrue(FALSE === $this->redis->hIncrBy('h', 'y', 1));

        if (version_compare($this->version, "2.5.0") >= 0) {
            // hIncrByFloat
            $this->redis->del('h');
            $this->assertTrue(1.5 === $this->redis->hIncrByFloat('h','x', 1.5));
            $this->assertTrue(3.0 === $this->redis->hincrByFloat('h','x', 1.5));
            $this->assertTrue(1.5 === $this->redis->hincrByFloat('h','x', -1.5));
            $this->assertTrue(1000000000001.5 === $this->redis->hincrByFloat('h','x', 1000000000000));

            $this->redis->hset('h','y','not-a-number');
            $this->assertTrue(FALSE === $this->redis->hIncrByFloat('h', 'y', 1.5));
        }

        // hmset
        $this->redis->del('h');
        $this->assertTrue(TRUE === $this->redis->hMset('h', ['x' => 123, 'y' => 456, 'z' => 'abc']));
        $this->assertTrue('123' === $this->redis->hGet('h', 'x'));
        $this->assertTrue('456' === $this->redis->hGet('h', 'y'));
        $this->assertTrue('abc' === $this->redis->hGet('h', 'z'));
        $this->assertTrue(FALSE === $this->redis->hGet('h', 't'));

        // hmget
        $this->assertTrue(['x' => '123', 'y' => '456'] === $this->redis->hMget('h', ['x', 'y']));
        $this->assertTrue(['z' => 'abc'] === $this->redis->hMget('h', ['z']));
        $this->assertTrue(['x' => '123', 't' => FALSE, 'y' => '456'] === $this->redis->hMget('h', ['x', 't', 'y']));
        $this->assertFalse([123 => 'x'] === $this->redis->hMget('h', [123]));
        $this->assertTrue([123 => FALSE] === $this->redis->hMget('h', [123]));

        // Test with an array populated with things we can't use as keys
        $this->assertTrue($this->redis->hmget('h', [false,NULL,false]) === FALSE);

        // Test with some invalid keys mixed in (which should just be ignored)
        $this->assertTrue(['x'=>'123','y'=>'456','z'=>'abc'] === $this->redis->hMget('h',['x',null,'y','','z',false]));

        // hmget/hmset with numeric fields
        $this->redis->del('h');
        $this->assertTrue(TRUE === $this->redis->hMset('h', [123 => 'x', 'y' => 456]));
        $this->assertTrue('x' === $this->redis->hGet('h', 123));
        $this->assertTrue('x' === $this->redis->hGet('h', '123'));
        $this->assertTrue('456' === $this->redis->hGet('h', 'y'));
        $this->assertTrue([123 => 'x', 'y' => '456'] === $this->redis->hMget('h', ['123', 'y']));

        // references
        $keys = [123, 'y'];
        foreach ($keys as &$key) {}
        $this->assertTrue([123 => 'x', 'y' => '456'] === $this->redis->hMget('h', $keys));

        // check non-string types.
        $this->redis->del('h1');
        $this->assertTrue(TRUE === $this->redis->hMSet('h1', ['x' => 0, 'y' => [], 'z' => new stdclass(), 't' => NULL]));
        $h1 = $this->redis->hGetAll('h1');
        $this->assertTrue('0' === $h1['x']);
        $this->assertTrue('Array' === $h1['y']);
        $this->assertTrue('Object' === $h1['z']);
        $this->assertTrue('' === $h1['t']);

        // hstrlen
        if (version_compare($this->version, '3.2.0') >= 0) {
            $this->redis->del('h');
            $this->assertTrue(0 === $this->redis->hStrLen('h', 'x')); // key doesn't exist
            $this->redis->hSet('h', 'foo', 'bar');
            $this->assertTrue(0 === $this->redis->hStrLen('h', 'x')); // field is not present in the hash
            $this->assertTrue(3 === $this->redis->hStrLen('h', 'foo'));
	}
    }

    public function testSetRange() {

        $this->redis->del('key');
        $this->redis->set('key', 'hello world');
        $this->redis->setRange('key', 6, 'redis');
        $this->assertTrue('hello redis' === $this->redis->get('key'));
        $this->redis->setRange('key', 6, 'you'); // don't cut off the end
        $this->assertTrue('hello youis' === $this->redis->get('key'));

        $this->redis->set('key', 'hello world');
        // $this->assertTrue(11 === $this->redis->setRange('key', -6, 'redis')); // works with negative offsets too! (disabled because not all versions support this)
        // $this->assertTrue('hello redis' === $this->redis->get('key'));

        // fill with zeros if needed
        $this->redis->del('key');
        $this->redis->setRange('key', 6, 'foo');
        $this->assertTrue("\x00\x00\x00\x00\x00\x00foo" === $this->redis->get('key'));
    }

    public function testObject() {
        /* Version 3.0.0 (represented as >= 2.9.0 in redis info)  and moving
         * forward uses "embstr" instead of "raw" for small string values */
        if (version_compare($this->version, "2.9.0") < 0) {
            $str_small_encoding = "raw";
        } else {
            $str_small_encoding = "embstr";
        }

        $this->redis->del('key');
        $this->assertTrue($this->redis->object('encoding', 'key') === FALSE);
        $this->assertTrue($this->redis->object('refcount', 'key') === FALSE);
        $this->assertTrue($this->redis->object('idletime', 'key') === FALSE);

        $this->redis->set('key', 'value');
        $this->assertTrue($this->redis->object('encoding', 'key') === $str_small_encoding);
        $this->assertTrue($this->redis->object('refcount', 'key') === 1);
        $this->assertTrue($this->redis->object('idletime', 'key') === 0);

        $this->redis->del('key');
        $this->redis->lpush('key', 'value');

        /* Newer versions of redis are going to encode lists as 'quicklists',
         * so 'quicklist' or 'ziplist' is valid here */
        $str_encoding = $this->redis->object('encoding', 'key');
        $this->assertTrue($str_encoding === "ziplist" || $str_encoding === 'quicklist');

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

        $str_encoding = $this->redis->object('encoding', 'key');
        $this->assertTrue($str_encoding === "linkedlist" || $str_encoding == "quicklist");

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
        $ret = $this->redis->multi()->get('x')->exec();

        // successful transaction
        $this->assertTrue($ret === ['42']);
    }

    public function testFailedTransactions() {
        $this->redis->set('x', 42);

        // failed transaction
        $this->redis->watch('x');

        $r = $this->newInstance(); // new instance, modifying `x'.
        $r->incr('x');

        $ret = $this->redis->multi()->get('x')->exec();
        $this->assertTrue($ret === FALSE); // failed because another client changed our watched key between WATCH and EXEC.

        // watch and unwatch
        $this->redis->watch('x');
        $r->incr('x'); // other instance
        $this->redis->unwatch(); // cancel transaction watch

        $ret = $this->redis->multi()->get('x')->exec();

        $this->assertTrue($ret === ['44']); // succeeded since we've cancel the WATCH command.
    }

    public function testPipeline() {
        if (!$this->havePipeline()) {
            $this->markTestSkipped();
        }

        $this->sequence(Redis::PIPELINE);
        $this->differentType(Redis::PIPELINE);

        // with prefix as well
        $this->redis->setOption(Redis::OPT_PREFIX, "test:");
        $this->sequence(Redis::PIPELINE);
        $this->differentType(Redis::PIPELINE);
        $this->redis->setOption(Redis::OPT_PREFIX, "");
    }

    public function testPipelineMultiExec()
    {
        if (!$this->havePipeline()) {
            $this->markTestSkipped();
        }

        $ret = $this->redis->pipeline()->multi()->exec()->exec();
        $this->assertTrue(is_array($ret));
        $this->assertEquals(1, count($ret)); // empty transaction

        $ret = $this->redis->pipeline()
            ->ping()
            ->multi()->set('x', 42)->incr('x')->exec()
            ->ping()
            ->multi()->get('x')->del('x')->exec()
            ->ping()
            ->exec();
        $this->assertTrue(is_array($ret));
        $this->assertEquals(5, count($ret)); // should be 5 atomic operations
    }

    /* Github issue #1211 (ignore redundant calls to pipeline or multi) */
    public function testDoublePipeNoOp() {
        /* Only the first pipeline should be honored */
        for ($i = 0; $i < 6; $i++) {
            $this->redis->pipeline();
        }

        /* Set and get in our pipeline */
        $this->redis->set('pipecount','over9000')->get('pipecount');

        $data = $this->redis->exec();
        $this->assertEquals([true,'over9000'], $data);

        /* Only the first MULTI should be honored */
        for ($i = 0; $i < 6; $i++) {
            $this->redis->multi();
        }

        /* Set and get in our MULTI block */
        $this->redis->set('multicount', 'over9000')->get('multicount');

        $data = $this->redis->exec();
        $this->assertEquals([true, 'over9000'], $data);
    }

    public function testDiscard()
    {
        foreach ([Redis::PIPELINE, Redis::MULTI] as $mode) {
            /* start transaction */
            $this->redis->multi($mode);

            /* Set and get in our transaction */
            $this->redis->set('pipecount','over9000')->get('pipecount');

            /* first call closes transaction and clears commands queue */
            $this->assertTrue($this->redis->discard());

            /* next call fails because mode is ATOMIC */
            $this->assertFalse($this->redis->discard());
        }
    }

    protected function sequence($mode) {
        $ret = $this->redis->multi($mode)
            ->set('x', 42)
            ->type('x')
            ->get('x')
            ->exec();

        $this->assertTrue(is_array($ret));
        $i = 0;
        $this->assertTrue($ret[$i++] == TRUE);
        $this->assertTrue($ret[$i++] === Redis::REDIS_STRING);
        $this->assertTrue($ret[$i] === '42' || $ret[$i] === 42);

        $serializer = $this->redis->getOption(Redis::OPT_SERIALIZER);
        $this->redis->setOption(Redis::OPT_SERIALIZER, Redis::SERIALIZER_NONE); // testing incr, which doesn't work with the serializer
        $ret = $this->redis->multi($mode)
            ->del('{key}1')
            ->set('{key}1', 'value1')
            ->get('{key}1')
            ->getSet('{key}1', 'value2')
            ->get('{key}1')
            ->set('{key}2', 4)
            ->incr('{key}2')
            ->get('{key}2')
            ->decr('{key}2')
            ->get('{key}2')
            ->rename('{key}2', '{key}3')
            ->get('{key}3')
            ->renameNx('{key}3', '{key}1')
            ->rename('{key}3', '{key}2')
            ->incrby('{key}2', 5)
            ->get('{key}2')
            ->decrby('{key}2', 5)
            ->get('{key}2')
            ->exec();

        $i = 0;
        $this->assertTrue(is_array($ret));
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
            ->del('{key}1')
            ->del('{key}2')
            ->set('{key}1', 'val1')
            ->setnx('{key}1', 'valX')
            ->setnx('{key}2', 'valX')
            ->exists('{key}1')
            ->exists('{key}3')
            ->exec();

        $this->assertTrue(is_array($ret));
        $this->assertTrue($ret[0] == TRUE);
        $this->assertTrue($ret[1] == TRUE);
        $this->assertTrue($ret[2] == TRUE);
        $this->assertTrue($ret[3] == FALSE);
        $this->assertTrue($ret[4] == TRUE);
        $this->assertTrue($ret[5] == TRUE);
        $this->assertTrue($ret[6] == FALSE);

        // ttl, mget, mset, msetnx, expire, expireAt
        $this->redis->del('key');
        $ret = $this->redis->multi($mode)
            ->ttl('key')
            ->mget(['{key}1', '{key}2', '{key}3'])
            ->mset(['{key}3' => 'value3', '{key}4' => 'value4'])
            ->set('key', 'value')
            ->expire('key', 5)
            ->ttl('key')
            ->expireAt('key', '0000')
            ->exec();

        $this->assertTrue(is_array($ret));
        $i = 0;
        $ttl = $ret[$i++];
        $this->assertTrue($ttl === -1 || $ttl === -2);
        $this->assertTrue($ret[$i++] === ['val1', 'valX', FALSE]); // mget
        $this->assertTrue($ret[$i++] === TRUE); // mset
        $this->assertTrue($ret[$i++] === TRUE); // set
        $this->assertTrue($ret[$i++] === TRUE); // expire
        $this->assertTrue($ret[$i++] === 5);    // ttl
        $this->assertTrue($ret[$i++] === TRUE); // expireAt
        $this->assertTrue(count($ret) == $i);

        $ret = $this->redis->multi($mode)
            ->set('{list}lkey', 'x')
            ->set('{list}lDest', 'y')
            ->del('{list}lkey', '{list}lDest')
            ->rpush('{list}lkey', 'lvalue')
            ->lpush('{list}lkey', 'lvalue')
            ->lpush('{list}lkey', 'lvalue')
            ->lpush('{list}lkey', 'lvalue')
            ->lpush('{list}lkey', 'lvalue')
            ->lpush('{list}lkey', 'lvalue')
            ->rpoplpush('{list}lkey', '{list}lDest')
            ->lrange('{list}lDest', 0, -1)
            ->lpop('{list}lkey')
            ->llen('{list}lkey')
            ->lrem('{list}lkey', 'lvalue', 3)
            ->llen('{list}lkey')
            ->lIndex('{list}lkey', 0)
            ->lrange('{list}lkey', 0, -1)
            ->lSet('{list}lkey', 1, "newValue")    // check errors on key not exists
            ->lrange('{list}lkey', 0, -1)
            ->llen('{list}lkey')
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
        $this->assertTrue($ret[$i++] === ['lvalue']); // lDest contains only that one element.
        $this->assertTrue($ret[$i++] === 'lvalue'); // removing a second element from lkey, now 4 elements left ↓
        $this->assertTrue($ret[$i++] === 4); // 4 elements left, after 2 pops.
        $this->assertTrue($ret[$i++] === 3); // removing 3 elements, now 1 left.
        $this->assertTrue($ret[$i++] === 1); // 1 element left
        $this->assertTrue($ret[$i++] === "lvalue"); // this is the current head.
        $this->assertTrue($ret[$i++] === ["lvalue"]); // this is the current list.
        $this->assertTrue($ret[$i++] === FALSE); // updating a non-existent element fails.
        $this->assertTrue($ret[$i++] === ["lvalue"]); // this is the current list.
        $this->assertTrue($ret[$i++] === 1); // 1 element left
        $this->assertTrue(count($ret) == $i);


        $ret = $this->redis->multi($mode)
            ->del('{list}lkey', '{list}lDest')
            ->rpush('{list}lkey', 'lvalue')
            ->lpush('{list}lkey', 'lvalue')
            ->lpush('{list}lkey', 'lvalue')
            ->rpoplpush('{list}lkey', '{list}lDest')
            ->lrange('{list}lDest', 0, -1)
            ->lpop('{list}lkey')
            ->exec();
        $this->assertTrue(is_array($ret));
        $i = 0;
        $this->assertTrue($ret[$i++] <= 2); // deleted 0, 1, or 2 items
        $this->assertTrue($ret[$i++] === 1); // 1 element in the list
        $this->assertTrue($ret[$i++] === 2); // 2 elements in the list
        $this->assertTrue($ret[$i++] === 3); // 3 elements in the list
        $this->assertTrue($ret[$i++] === 'lvalue'); // rpoplpush returns the element: "lvalue"
        $this->assertTrue($ret[$i++] === ['lvalue']); // rpoplpush returns the element: "lvalue"
        $this->assertTrue($ret[$i++] === 'lvalue'); // pop returns the front element: "lvalue"
        $this->assertTrue(count($ret) == $i);


        $serializer = $this->redis->getOption(Redis::OPT_SERIALIZER);
        $this->redis->setOption(Redis::OPT_SERIALIZER, Redis::SERIALIZER_NONE); // testing incr, which doesn't work with the serializer
        $ret = $this->redis->multi($mode)
            ->del('{key}1')
            ->set('{key}1', 'value1')
            ->get('{key}1')
            ->getSet('{key}1', 'value2')
            ->get('{key}1')
            ->set('{key}2', 4)
            ->incr('{key}2')
            ->get('{key}2')
            ->decr('{key}2')
            ->get('{key}2')
            ->rename('{key}2', '{key}3')
            ->get('{key}3')
            ->renameNx('{key}3', '{key}1')
            ->rename('{key}3', '{key}2')
            ->incrby('{key}2', 5)
            ->get('{key}2')
            ->decrby('{key}2', 5)
            ->get('{key}2')
            ->set('{key}3', 'value3')
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
        $this->assertTrue($ret[$i++]);
        $this->redis->setOption(Redis::OPT_SERIALIZER, $serializer);

        $ret = $this->redis->multi($mode)
            ->del('{key}1')
            ->del('{key}2')
            ->del('{key}3')
            ->set('{key}1', 'val1')
            ->setnx('{key}1', 'valX')
            ->setnx('{key}2', 'valX')
            ->exists('{key}1')
            ->exists('{key}3')
            ->exec();

        $this->assertTrue(is_array($ret));
        $this->assertTrue($ret[0] == TRUE);
        $this->assertTrue($ret[1] == TRUE);
        $this->assertTrue($ret[2] == TRUE);
        $this->assertTrue($ret[3] == TRUE);
        $this->assertTrue($ret[4] == FALSE);
        $this->assertTrue($ret[5] == TRUE);
        $this->assertTrue($ret[6] == TRUE);
        $this->assertTrue($ret[7] == FALSE);

        // ttl, mget, mset, msetnx, expire, expireAt
        $ret = $this->redis->multi($mode)
            ->ttl('key')
            ->mget(['{key}1', '{key}2', '{key}3'])
            ->mset(['{key}3' => 'value3', '{key}4' => 'value4'])
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
            ->del('{l}key', '{l}Dest')
            ->rpush('{l}key', 'lvalue')
            ->lpush('{l}key', 'lvalue')
            ->lpush('{l}key', 'lvalue')
            ->lpush('{l}key', 'lvalue')
            ->lpush('{l}key', 'lvalue')
            ->lpush('{l}key', 'lvalue')
            ->rpoplpush('{l}key', '{l}Dest')
            ->lrange('{l}Dest', 0, -1)
            ->lpop('{l}key')
            ->llen('{l}key')
            ->lrem('{l}key', 'lvalue', 3)
            ->llen('{l}key')
            ->lIndex('{l}key', 0)
            ->lrange('{l}key', 0, -1)
            ->lSet('{l}key', 1, "newValue")    // check errors on missing key
            ->lrange('{l}key', 0, -1)
            ->llen('{l}key')
            ->exec();

        $this->assertTrue(is_array($ret));
        $i = 0;
        $this->assertTrue($ret[$i] >= 0 && $ret[$i] <= 2); // del
        $i++;
        $this->assertTrue($ret[$i++] === 1); // 1 value
        $this->assertTrue($ret[$i++] === 2); // 2 values
        $this->assertTrue($ret[$i++] === 3); // 3 values
        $this->assertTrue($ret[$i++] === 4); // 4 values
        $this->assertTrue($ret[$i++] === 5); // 5 values
        $this->assertTrue($ret[$i++] === 6); // 6 values
        $this->assertTrue($ret[$i++] === 'lvalue');
        $this->assertTrue($ret[$i++] === ['lvalue']); // 1 value only in lDest
        $this->assertTrue($ret[$i++] === 'lvalue'); // now 4 values left
        $this->assertTrue($ret[$i++] === 4);
        $this->assertTrue($ret[$i++] === 3); // removing 3 elements.
        $this->assertTrue($ret[$i++] === 1); // length is now 1
        $this->assertTrue($ret[$i++] === 'lvalue'); // this is the head
        $this->assertTrue($ret[$i++] === ['lvalue']); // 1 value only in lkey
        $this->assertTrue($ret[$i++] === FALSE); // can't set list[1] if we only have a single value in it.
        $this->assertTrue($ret[$i++] === ['lvalue']); // the previous error didn't touch anything.
        $this->assertTrue($ret[$i++] === 1); // the previous error didn't change the length
        $this->assertTrue(count($ret) === $i);


        // sets
        $ret = $this->redis->multi($mode)
            ->del('{s}key1', '{s}key2', '{s}keydest', '{s}keyUnion', '{s}DiffDest')
            ->sadd('{s}key1', 'sValue1')
            ->sadd('{s}key1', 'sValue2')
            ->sadd('{s}key1', 'sValue3')
            ->sadd('{s}key1', 'sValue4')

            ->sadd('{s}key2', 'sValue1')
            ->sadd('{s}key2', 'sValue2')

            ->scard('{s}key1')
            ->srem('{s}key1', 'sValue2')
            ->scard('{s}key1')
            ->sMove('{s}key1', '{s}key2', 'sValue4')
            ->scard('{s}key2')
            ->sismember('{s}key2', 'sValue4')
            ->sMembers('{s}key1')
            ->sMembers('{s}key2')
            ->sInter('{s}key1', '{s}key2')
            ->sInterStore('{s}keydest', '{s}key1', '{s}key2')
            ->sMembers('{s}keydest')
            ->sUnion('{s}key2', '{s}keydest')
            ->sUnionStore('{s}keyUnion', '{s}key2', '{s}keydest')
            ->sMembers('{s}keyUnion')
            ->sDiff('{s}key1', '{s}key2')
            ->sDiffStore('{s}DiffDest', '{s}key1', '{s}key2')
            ->sMembers('{s}DiffDest')
            ->sPop('{s}key2')
            ->scard('{s}key2')
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
        foreach(['sValue1', 'sValue3'] as $k) { // sKey1 contains sValue1 and sValue3.
            $this->assertTrue(in_array($k, $ret[$i]));
        }
        $this->assertTrue(count($ret[$i++]) === 2);
        foreach(['sValue1', 'sValue2', 'sValue4'] as $k) { // sKey2 contains sValue1, sValue2, and sValue4.
            $this->assertTrue(in_array($k, $ret[$i]));
        }
        $this->assertTrue(count($ret[$i++]) === 3);
        $this->assertTrue($ret[$i++] === ['sValue1']); // intersection
        $this->assertTrue($ret[$i++] === 1); // intersection + store → 1 value in the destination set.
        $this->assertTrue($ret[$i++] === ['sValue1']); // sinterstore destination contents

        foreach(['sValue1', 'sValue2', 'sValue4'] as $k) { // (skeydest U sKey2) contains sValue1, sValue2, and sValue4.
            $this->assertTrue(in_array($k, $ret[$i]));
        }
        $this->assertTrue(count($ret[$i++]) === 3); // union size

        $this->assertTrue($ret[$i++] === 3); // unionstore size
        foreach(['sValue1', 'sValue2', 'sValue4'] as $k) { // (skeyUnion) contains sValue1, sValue2, and sValue4.
            $this->assertTrue(in_array($k, $ret[$i]));
        }
        $this->assertTrue(count($ret[$i++]) === 3); // skeyUnion size

        $this->assertTrue($ret[$i++] === ['sValue3']); // diff skey1, skey2 : only sValue3 is not shared.
        $this->assertTrue($ret[$i++] === 1); // sdiffstore size == 1
        $this->assertTrue($ret[$i++] === ['sValue3']); // contents of sDiffDest

        $this->assertTrue(in_array($ret[$i++], ['sValue1', 'sValue2', 'sValue4'])); // we removed an element from sKey2
        $this->assertTrue($ret[$i++] === 2); // sKey2 now has 2 elements only.

        $this->assertTrue(count($ret) === $i);

        // sorted sets
        $ret = $this->redis->multi($mode)
            ->del('{z}key1', '{z}key2', '{z}key5', '{z}Inter', '{z}Union')
            ->zadd('{z}key1', 1, 'zValue1')
            ->zadd('{z}key1', 5, 'zValue5')
            ->zadd('{z}key1', 2, 'zValue2')
            ->zRange('{z}key1', 0, -1)
            ->zRem('{z}key1', 'zValue2')
            ->zRange('{z}key1', 0, -1)
            ->zadd('{z}key1', 11, 'zValue11')
            ->zadd('{z}key1', 12, 'zValue12')
            ->zadd('{z}key1', 13, 'zValue13')
            ->zadd('{z}key1', 14, 'zValue14')
            ->zadd('{z}key1', 15, 'zValue15')
            ->zRemRangeByScore('{z}key1', 11, 13)
            ->zrange('{z}key1', 0, -1)
            ->zRevRange('{z}key1', 0, -1)
            ->zRangeByScore('{z}key1', 1, 6)
            ->zCard('{z}key1')
            ->zScore('{z}key1', 'zValue15')
            ->zadd('{z}key2', 5, 'zValue5')
            ->zadd('{z}key2', 2, 'zValue2')
            ->zInterStore('{z}Inter', ['{z}key1', '{z}key2'])
            ->zRange('{z}key1', 0, -1)
            ->zRange('{z}key2', 0, -1)
            ->zRange('{z}Inter', 0, -1)
            ->zUnionStore('{z}Union', ['{z}key1', '{z}key2'])
            ->zRange('{z}Union', 0, -1)
            ->zadd('{z}key5', 5, 'zValue5')
            ->zIncrBy('{z}key5', 3, 'zValue5') // fix this
            ->zScore('{z}key5', 'zValue5')
            ->zScore('{z}key5', 'unknown')
            ->exec();

        $i = 0;
        $this->assertTrue(is_array($ret));
        $this->assertTrue(is_long($ret[$i]) && $ret[$i] >= 0 && $ret[$i] <= 5); $i++; // deleting at most 5 keys
        $this->assertTrue($ret[$i++] === 1);
        $this->assertTrue($ret[$i++] === 1);
        $this->assertTrue($ret[$i++] === 1);
        $this->assertTrue($ret[$i++] === ['zValue1', 'zValue2', 'zValue5']);
        $this->assertTrue($ret[$i++] === 1);
        $this->assertTrue($ret[$i++] === ['zValue1', 'zValue5']);
        $this->assertTrue($ret[$i++] === 1); // adding zValue11
        $this->assertTrue($ret[$i++] === 1); // adding zValue12
        $this->assertTrue($ret[$i++] === 1); // adding zValue13
        $this->assertTrue($ret[$i++] === 1); // adding zValue14
        $this->assertTrue($ret[$i++] === 1); // adding zValue15
        $this->assertTrue($ret[$i++] === 3); // deleted zValue11, zValue12, zValue13
        $this->assertTrue($ret[$i++] === ['zValue1', 'zValue5', 'zValue14', 'zValue15']);
        $this->assertTrue($ret[$i++] === ['zValue15', 'zValue14', 'zValue5', 'zValue1']);
        $this->assertTrue($ret[$i++] === ['zValue1', 'zValue5']);
        $this->assertTrue($ret[$i++] === 4); // 4 elements
        $this->assertTrue($ret[$i++] === 15.0);
        $this->assertTrue($ret[$i++] === 1); // added value
        $this->assertTrue($ret[$i++] === 1); // added value
        $this->assertTrue($ret[$i++] === 1); // zinter only has 1 value
        $this->assertTrue($ret[$i++] === ['zValue1', 'zValue5', 'zValue14', 'zValue15']); // {z}key1 contents
        $this->assertTrue($ret[$i++] === ['zValue2', 'zValue5']); // {z}key2 contents
        $this->assertTrue($ret[$i++] === ['zValue5']); // {z}inter contents
        $this->assertTrue($ret[$i++] === 5); // {z}Union has 5 values (1,2,5,14,15)
        $this->assertTrue($ret[$i++] === ['zValue1', 'zValue2', 'zValue5', 'zValue14', 'zValue15']); // {z}Union contents
        $this->assertTrue($ret[$i++] === 1); // added value to {z}key5, with score 5
        $this->assertTrue($ret[$i++] === 8.0); // incremented score by 3 → it is now 8.
        $this->assertTrue($ret[$i++] === 8.0); // current score is 8.
        $this->assertTrue($ret[$i++] === FALSE); // score for unknown element.

        $this->assertTrue(count($ret) === $i);

        // hash
        $ret = $this->redis->multi($mode)
            ->del('hkey1')
            ->hset('hkey1', 'key1', 'value1')
            ->hset('hkey1', 'key2', 'value2')
            ->hset('hkey1', 'key3', 'value3')
            ->hmget('hkey1', ['key1', 'key2', 'key3'])
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
        $this->assertTrue($ret[$i++] === ['key1' => 'value1', 'key2' => 'value2', 'key3' => 'value3']); // hmget, 3 elements
        $this->assertTrue($ret[$i++] === 'value1'); // hget
        $this->assertTrue($ret[$i++] === 3); // hlen
        $this->assertTrue($ret[$i++] === 1); // hdel succeeded
        $this->assertTrue($ret[$i++] === 0); // hdel failed
        $this->assertTrue($ret[$i++] === FALSE); // hexists didn't find the deleted key
        $this->assertTrue($ret[$i] === ['key1', 'key3'] || $ret[$i] === ['key3', 'key1']); $i++; // hkeys
        $this->assertTrue($ret[$i] === ['value1', 'value3'] || $ret[$i] === ['value3', 'value1']); $i++; // hvals
        $this->assertTrue($ret[$i] === ['key1' => 'value1', 'key3' => 'value3'] || $ret[$i] === ['key3' => 'value3', 'key1' => 'value1']); $i++; // hgetall
        $this->assertTrue($ret[$i++] === 1); // added 1 element
        $this->assertTrue($ret[$i++] === 1); // added the element, so 1.
        $this->assertTrue($ret[$i++] === 'non-string'); // hset succeeded
        $this->assertTrue(count($ret) === $i);

        $ret = $this->redis->multi($mode) // default to MULTI, not PIPELINE.
            ->del('test')
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

        $this->assertTrue($result === [1.0, FALSE, FALSE, 2.0]);
    }

    protected function differentType($mode) {

        // string
        $key = '{hash}string';
        $dkey = '{hash}' . __FUNCTION__;

        $ret = $this->redis->multi($mode)
            ->del($key)
            ->set($key, 'value')

            // lists I/F
            ->rPush($key, 'lvalue')
            ->lPush($key, 'lvalue')
            ->lLen($key)
            ->lPop($key)
            ->lrange($key, 0, -1)
            ->lTrim($key, 0, 1)
            ->lIndex($key, 0)
            ->lSet($key, 0, "newValue")
            ->lrem($key, 'lvalue', 1)
            ->lPop($key)
            ->rPop($key)
            ->rPoplPush($key, $dkey . 'lkey1')

            // sets I/F
            ->sAdd($key, 'sValue1')
            ->srem($key, 'sValue1')
            ->sPop($key)
            ->sMove($key, $dkey . 'skey1', 'sValue1')

            ->scard($key)
            ->sismember($key, 'sValue1')
            ->sInter($key, $dkey . 'skey2')

            ->sUnion($key, $dkey . 'skey4')
            ->sDiff($key, $dkey . 'skey7')
            ->sMembers($key)
            ->sRandMember($key)

            // sorted sets I/F
            ->zAdd($key, 1, 'zValue1')
            ->zRem($key, 'zValue1')
            ->zIncrBy($key, 1, 'zValue1')
            ->zRank($key, 'zValue1')
            ->zRevRank($key, 'zValue1')
            ->zRange($key, 0, -1)
            ->zRevRange($key, 0, -1)
            ->zRangeByScore($key, 1, 2)
            ->zCount($key, 0, -1)
            ->zCard($key)
            ->zScore($key, 'zValue1')
            ->zRemRangeByRank($key, 1, 2)
            ->zRemRangeByScore($key, 1, 2)

            // hash I/F
            ->hSet($key, 'key1', 'value1')
            ->hGet($key, 'key1')
            ->hMGet($key, ['key1'])
            ->hMSet($key, ['key1' => 'value1'])
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
        $this->assertTrue($ret[$i++] === FALSE); // lrange
        $this->assertTrue($ret[$i++] === FALSE); // ltrim
        $this->assertTrue($ret[$i++] === FALSE); // lindex
        $this->assertTrue($ret[$i++] === FALSE); // lset
        $this->assertTrue($ret[$i++] === FALSE); // lremove
        $this->assertTrue($ret[$i++] === FALSE); // lpop
        $this->assertTrue($ret[$i++] === FALSE); // rpop
        $this->assertTrue($ret[$i++] === FALSE); // rpoplush

        $this->assertTrue($ret[$i++] === FALSE); // sadd
        $this->assertTrue($ret[$i++] === FALSE); // sremove
        $this->assertTrue($ret[$i++] === FALSE); // spop
        $this->assertTrue($ret[$i++] === FALSE); // smove
        $this->assertTrue($ret[$i++] === FALSE); // scard
        $this->assertTrue($ret[$i++] === FALSE); // sismember
        $this->assertTrue($ret[$i++] === FALSE); // sinter
        $this->assertTrue($ret[$i++] === FALSE); // sunion
        $this->assertTrue($ret[$i++] === FALSE); // sdiff
        $this->assertTrue($ret[$i++] === FALSE); // smembers
        $this->assertTrue($ret[$i++] === FALSE); // srandmember

        $this->assertTrue($ret[$i++] === FALSE); // zadd
        $this->assertTrue($ret[$i++] === FALSE); // zrem
        $this->assertTrue($ret[$i++] === FALSE); // zincrby
        $this->assertTrue($ret[$i++] === FALSE); // zrank
        $this->assertTrue($ret[$i++] === FALSE); // zrevrank
        $this->assertTrue($ret[$i++] === FALSE); // zrange
        $this->assertTrue($ret[$i++] === FALSE); // zreverserange
        $this->assertTrue($ret[$i++] === FALSE); // zrangebyscore
        $this->assertTrue($ret[$i++] === FALSE); // zcount
        $this->assertTrue($ret[$i++] === FALSE); // zcard
        $this->assertTrue($ret[$i++] === FALSE); // zscore
        $this->assertTrue($ret[$i++] === FALSE); // zremrangebyrank
        $this->assertTrue($ret[$i++] === FALSE); // zremrangebyscore

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
        $key = '{hash}list';
        $dkey = '{hash}' . __FUNCTION__;
        $ret = $this->redis->multi($mode)
            ->del($key)
            ->lpush($key, 'lvalue')

            // string I/F
            ->get($key)
            ->getset($key, 'value2')
            ->append($key, 'append')
            ->getRange($key, 0, 8)
            ->mget([$key])
            ->incr($key)
            ->incrBy($key, 1)
            ->decr($key)
            ->decrBy($key, 1)

            // sets I/F
            ->sAdd($key, 'sValue1')
            ->srem($key, 'sValue1')
            ->sPop($key)
            ->sMove($key, $dkey . 'skey1', 'sValue1')
            ->scard($key)
            ->sismember($key, 'sValue1')
            ->sInter($key, $dkey . 'skey2')
            ->sUnion($key, $dkey . 'skey4')
            ->sDiff($key, $dkey . 'skey7')
            ->sMembers($key)
            ->sRandMember($key)

            // sorted sets I/F
            ->zAdd($key, 1, 'zValue1')
            ->zRem($key, 'zValue1')
            ->zIncrBy($key, 1, 'zValue1')
            ->zRank($key, 'zValue1')
            ->zRevRank($key, 'zValue1')
            ->zRange($key, 0, -1)
            ->zRevRange($key, 0, -1)
            ->zRangeByScore($key, 1, 2)
            ->zCount($key, 0, -1)
            ->zCard($key)
            ->zScore($key, 'zValue1')
            ->zRemRangeByRank($key, 1, 2)
            ->zRemRangeByScore($key, 1, 2)

            // hash I/F
            ->hSet($key, 'key1', 'value1')
            ->hGet($key, 'key1')
            ->hMGet($key, ['key1'])
            ->hMSet($key, ['key1' => 'value1'])
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
        $this->assertTrue($ret[$i++] === FALSE); // getRange
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
        $this->assertTrue($ret[$i++] === FALSE); // scard
        $this->assertTrue($ret[$i++] === FALSE); // sismember
        $this->assertTrue($ret[$i++] === FALSE); // sinter
        $this->assertTrue($ret[$i++] === FALSE); // sunion
        $this->assertTrue($ret[$i++] === FALSE); // sdiff
        $this->assertTrue($ret[$i++] === FALSE); // smembers
        $this->assertTrue($ret[$i++] === FALSE); // srandmember

        $this->assertTrue($ret[$i++] === FALSE); // zadd
        $this->assertTrue($ret[$i++] === FALSE); // zrem
        $this->assertTrue($ret[$i++] === FALSE); // zincrby
        $this->assertTrue($ret[$i++] === FALSE); // zrank
        $this->assertTrue($ret[$i++] === FALSE); // zrevrank
        $this->assertTrue($ret[$i++] === FALSE); // zrange
        $this->assertTrue($ret[$i++] === FALSE); // zreverserange
        $this->assertTrue($ret[$i++] === FALSE); // zrangebyscore
        $this->assertTrue($ret[$i++] === FALSE); // zcount
        $this->assertTrue($ret[$i++] === FALSE); // zcard
        $this->assertTrue($ret[$i++] === FALSE); // zscore
        $this->assertTrue($ret[$i++] === FALSE); // zremrangebyrank
        $this->assertTrue($ret[$i++] === FALSE); // zremrangebyscore

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
        $key = '{hash}set';
        $dkey = '{hash}' . __FUNCTION__;
        $ret = $this->redis->multi($mode)
            ->del($key)
            ->sAdd($key, 'sValue')

            // string I/F
            ->get($key)
            ->getset($key, 'value2')
            ->append($key, 'append')
            ->getRange($key, 0, 8)
            ->mget([$key])
            ->incr($key)
            ->incrBy($key, 1)
            ->decr($key)
            ->decrBy($key, 1)

            // lists I/F
            ->rPush($key, 'lvalue')
            ->lPush($key, 'lvalue')
            ->lLen($key)
            ->lPop($key)
            ->lrange($key, 0, -1)
            ->lTrim($key, 0, 1)
            ->lIndex($key, 0)
            ->lSet($key, 0, "newValue")
            ->lrem($key, 'lvalue', 1)
            ->lPop($key)
            ->rPop($key)
            ->rPoplPush($key, $dkey . 'lkey1')

            // sorted sets I/F
            ->zAdd($key, 1, 'zValue1')
            ->zRem($key, 'zValue1')
            ->zIncrBy($key, 1, 'zValue1')
            ->zRank($key, 'zValue1')
            ->zRevRank($key, 'zValue1')
            ->zRange($key, 0, -1)
            ->zRevRange($key, 0, -1)
            ->zRangeByScore($key, 1, 2)
            ->zCount($key, 0, -1)
            ->zCard($key)
            ->zScore($key, 'zValue1')
            ->zRemRangeByRank($key, 1, 2)
            ->zRemRangeByScore($key, 1, 2)

            // hash I/F
            ->hSet($key, 'key1', 'value1')
            ->hGet($key, 'key1')
            ->hMGet($key, ['key1'])
            ->hMSet($key, ['key1' => 'value1'])
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
        $this->assertTrue($ret[$i++] === FALSE); // getRange
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
        $this->assertTrue($ret[$i++] === FALSE); // lrange
        $this->assertTrue($ret[$i++] === FALSE); // ltrim
        $this->assertTrue($ret[$i++] === FALSE); // lindex
        $this->assertTrue($ret[$i++] === FALSE); // lset
        $this->assertTrue($ret[$i++] === FALSE); // lremove
        $this->assertTrue($ret[$i++] === FALSE); // lpop
        $this->assertTrue($ret[$i++] === FALSE); // rpop
        $this->assertTrue($ret[$i++] === FALSE); // rpoplush

        $this->assertTrue($ret[$i++] === FALSE); // zadd
        $this->assertTrue($ret[$i++] === FALSE); // zrem
        $this->assertTrue($ret[$i++] === FALSE); // zincrby
        $this->assertTrue($ret[$i++] === FALSE); // zrank
        $this->assertTrue($ret[$i++] === FALSE); // zrevrank
        $this->assertTrue($ret[$i++] === FALSE); // zrange
        $this->assertTrue($ret[$i++] === FALSE); // zreverserange
        $this->assertTrue($ret[$i++] === FALSE); // zrangebyscore
        $this->assertTrue($ret[$i++] === FALSE); // zcount
        $this->assertTrue($ret[$i++] === FALSE); // zcard
        $this->assertTrue($ret[$i++] === FALSE); // zscore
        $this->assertTrue($ret[$i++] === FALSE); // zremrangebyrank
        $this->assertTrue($ret[$i++] === FALSE); // zremrangebyscore

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
        $key = '{hash}sortedset';
        $dkey = '{hash}' . __FUNCTION__;
        $ret = $this->redis->multi($mode)
            ->del($key)
            ->zAdd($key, 0, 'zValue')

            // string I/F
            ->get($key)
            ->getset($key, 'value2')
            ->append($key, 'append')
            ->getRange($key, 0, 8)
            ->mget([$key])
            ->incr($key)
            ->incrBy($key, 1)
            ->decr($key)
            ->decrBy($key, 1)

            // lists I/F
            ->rPush($key, 'lvalue')
            ->lPush($key, 'lvalue')
            ->lLen($key)
            ->lPop($key)
            ->lrange($key, 0, -1)
            ->lTrim($key, 0, 1)
            ->lIndex($key, 0)
            ->lSet($key, 0, "newValue")
            ->lrem($key, 'lvalue', 1)
            ->lPop($key)
            ->rPop($key)
            ->rPoplPush($key, $dkey . 'lkey1')

            // sets I/F
            ->sAdd($key, 'sValue1')
            ->srem($key, 'sValue1')
            ->sPop($key)
            ->sMove($key, $dkey . 'skey1', 'sValue1')
            ->scard($key)
            ->sismember($key, 'sValue1')
            ->sInter($key, $dkey . 'skey2')
            ->sUnion($key, $dkey . 'skey4')
            ->sDiff($key, $dkey . 'skey7')
            ->sMembers($key)
            ->sRandMember($key)

            // hash I/F
            ->hSet($key, 'key1', 'value1')
            ->hGet($key, 'key1')
            ->hMGet($key, ['key1'])
            ->hMSet($key, ['key1' => 'value1'])
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
        $this->assertTrue($ret[$i++] === FALSE); // getRange
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
        $this->assertTrue($ret[$i++] === FALSE); // lrange
        $this->assertTrue($ret[$i++] === FALSE); // ltrim
        $this->assertTrue($ret[$i++] === FALSE); // lindex
        $this->assertTrue($ret[$i++] === FALSE); // lset
        $this->assertTrue($ret[$i++] === FALSE); // lremove
        $this->assertTrue($ret[$i++] === FALSE); // lpop
        $this->assertTrue($ret[$i++] === FALSE); // rpop
        $this->assertTrue($ret[$i++] === FALSE); // rpoplush

        $this->assertTrue($ret[$i++] === FALSE); // sadd
        $this->assertTrue($ret[$i++] === FALSE); // sremove
        $this->assertTrue($ret[$i++] === FALSE); // spop
        $this->assertTrue($ret[$i++] === FALSE); // smove
        $this->assertTrue($ret[$i++] === FALSE); // scard
        $this->assertTrue($ret[$i++] === FALSE); // sismember
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
        $key = '{hash}hash';
        $dkey = '{hash}' . __FUNCTION__;
        $ret = $this->redis->multi($mode)
            ->del($key)
            ->hset($key, 'key1', 'hValue')

            // string I/F
            ->get($key)
            ->getset($key, 'value2')
            ->append($key, 'append')
            ->getRange($key, 0, 8)
            ->mget([$key])
            ->incr($key)
            ->incrBy($key, 1)
            ->decr($key)
            ->decrBy($key, 1)

            // lists I/F
            ->rPush($key, 'lvalue')
            ->lPush($key, 'lvalue')
            ->lLen($key)
            ->lPop($key)
            ->lrange($key, 0, -1)
            ->lTrim($key, 0, 1)
            ->lIndex($key, 0)
            ->lSet($key, 0, "newValue")
            ->lrem($key, 'lvalue', 1)
            ->lPop($key)
            ->rPop($key)
            ->rPoplPush($key, $dkey . 'lkey1')

            // sets I/F
            ->sAdd($key, 'sValue1')
            ->srem($key, 'sValue1')
            ->sPop($key)
            ->sMove($key, $dkey . 'skey1', 'sValue1')
            ->scard($key)
            ->sismember($key, 'sValue1')
            ->sInter($key, $dkey . 'skey2')
            ->sUnion($key, $dkey . 'skey4')
            ->sDiff($key, $dkey . 'skey7')
            ->sMembers($key)
            ->sRandMember($key)

            // sorted sets I/F
            ->zAdd($key, 1, 'zValue1')
            ->zRem($key, 'zValue1')
            ->zIncrBy($key, 1, 'zValue1')
            ->zRank($key, 'zValue1')
            ->zRevRank($key, 'zValue1')
            ->zRange($key, 0, -1)
            ->zRevRange($key, 0, -1)
            ->zRangeByScore($key, 1, 2)
            ->zCount($key, 0, -1)
            ->zCard($key)
            ->zScore($key, 'zValue1')
            ->zRemRangeByRank($key, 1, 2)
            ->zRemRangeByScore($key, 1, 2)

            ->exec();

        $i = 0;
        $this->assertTrue(is_array($ret));
        $this->assertTrue(is_long($ret[$i++])); // delete
        $this->assertTrue($ret[$i++] === 1); // hset

        $this->assertTrue($ret[$i++] === FALSE); // get
        $this->assertTrue($ret[$i++] === FALSE); // getset
        $this->assertTrue($ret[$i++] === FALSE); // append
        $this->assertTrue($ret[$i++] === FALSE); // getRange
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
        $this->assertTrue($ret[$i++] === FALSE); // lrange
        $this->assertTrue($ret[$i++] === FALSE); // ltrim
        $this->assertTrue($ret[$i++] === FALSE); // lindex
        $this->assertTrue($ret[$i++] === FALSE); // lset
        $this->assertTrue($ret[$i++] === FALSE); // lremove
        $this->assertTrue($ret[$i++] === FALSE); // lpop
        $this->assertTrue($ret[$i++] === FALSE); // rpop
        $this->assertTrue($ret[$i++] === FALSE); // rpoplush

        $this->assertTrue($ret[$i++] === FALSE); // sadd
        $this->assertTrue($ret[$i++] === FALSE); // sremove
        $this->assertTrue($ret[$i++] === FALSE); // spop
        $this->assertTrue($ret[$i++] === FALSE); // smove
        $this->assertTrue($ret[$i++] === FALSE); // scard
        $this->assertTrue($ret[$i++] === FALSE); // sismember
        $this->assertTrue($ret[$i++] === FALSE); // sinter
        $this->assertTrue($ret[$i++] === FALSE); // sunion
        $this->assertTrue($ret[$i++] === FALSE); // sdiff
        $this->assertTrue($ret[$i++] === FALSE); // smembers
        $this->assertTrue($ret[$i++] === FALSE); // srandmember

        $this->assertTrue($ret[$i++] === FALSE); // zadd
        $this->assertTrue($ret[$i++] === FALSE); // zrem
        $this->assertTrue($ret[$i++] === FALSE); // zincrby
        $this->assertTrue($ret[$i++] === FALSE); // zrank
        $this->assertTrue($ret[$i++] === FALSE); // zrevrank
        $this->assertTrue($ret[$i++] === FALSE); // zrange
        $this->assertTrue($ret[$i++] === FALSE); // zreverserange
        $this->assertTrue($ret[$i++] === FALSE); // zrangebyscore
        $this->assertTrue($ret[$i++] === FALSE); // zcount
        $this->assertTrue($ret[$i++] === FALSE); // zcard
        $this->assertTrue($ret[$i++] === FALSE); // zscore
        $this->assertTrue($ret[$i++] === FALSE); // zremrangebyrank
        $this->assertTrue($ret[$i++] === FALSE); // zremrangebyscore

        $this->assertEquals($i, count($ret));
    }

    public function testDifferentTypeString() {
        $key = '{hash}string';
        $dkey = '{hash}' . __FUNCTION__;

        $this->redis->del($key);
        $this->assertEquals(TRUE, $this->redis->set($key, 'value'));

        // lists I/F
        $this->assertEquals(FALSE, $this->redis->rPush($key, 'lvalue'));
        $this->assertEquals(FALSE, $this->redis->lPush($key, 'lvalue'));
        $this->assertEquals(FALSE, $this->redis->lLen($key));
        $this->assertEquals(FALSE, $this->redis->lPop($key));
        $this->assertEquals(FALSE, $this->redis->lrange($key, 0, -1));
        $this->assertEquals(FALSE, $this->redis->lTrim($key, 0, 1));
        $this->assertEquals(FALSE, $this->redis->lIndex($key, 0));
        $this->assertEquals(FALSE, $this->redis->lSet($key, 0, "newValue"));
        $this->assertEquals(FALSE, $this->redis->lrem($key, 'lvalue', 1));
        $this->assertEquals(FALSE, $this->redis->lPop($key));
        $this->assertEquals(FALSE, $this->redis->rPop($key));
        $this->assertEquals(FALSE, $this->redis->rPoplPush($key, $dkey . 'lkey1'));

        // sets I/F
        $this->assertEquals(FALSE, $this->redis->sAdd($key, 'sValue1'));
        $this->assertEquals(FALSE, $this->redis->srem($key, 'sValue1'));
        $this->assertEquals(FALSE, $this->redis->sPop($key));
        $this->assertEquals(FALSE, $this->redis->sMove($key, $dkey . 'skey1', 'sValue1'));
        $this->assertEquals(FALSE, $this->redis->scard($key));
        $this->assertEquals(FALSE, $this->redis->sismember($key, 'sValue1'));
        $this->assertEquals(FALSE, $this->redis->sInter($key, $dkey. 'skey2'));
        $this->assertEquals(FALSE, $this->redis->sUnion($key, $dkey . 'skey4'));
        $this->assertEquals(FALSE, $this->redis->sDiff($key, $dkey . 'skey7'));
        $this->assertEquals(FALSE, $this->redis->sMembers($key));
        $this->assertEquals(FALSE, $this->redis->sRandMember($key));

        // sorted sets I/F
        $this->assertEquals(FALSE, $this->redis->zAdd($key, 1, 'zValue1'));
        $this->assertEquals(FALSE, $this->redis->zRem($key, 'zValue1'));
        $this->assertEquals(FALSE, $this->redis->zIncrBy($key, 1, 'zValue1'));
        $this->assertEquals(FALSE, $this->redis->zRank($key, 'zValue1'));
        $this->assertEquals(FALSE, $this->redis->zRevRank($key, 'zValue1'));
        $this->assertEquals(FALSE, $this->redis->zRange($key, 0, -1));
        $this->assertEquals(FALSE, $this->redis->zRevRange($key, 0, -1));
        $this->assertEquals(FALSE, $this->redis->zRangeByScore($key, 1, 2));
        $this->assertEquals(FALSE, $this->redis->zCount($key, 0, -1));
        $this->assertEquals(FALSE, $this->redis->zCard($key));
        $this->assertEquals(FALSE, $this->redis->zScore($key, 'zValue1'));
        $this->assertEquals(FALSE, $this->redis->zRemRangeByRank($key, 1, 2));
        $this->assertEquals(FALSE, $this->redis->zRemRangeByScore($key, 1, 2));

        // hash I/F
        $this->assertEquals(FALSE, $this->redis->hSet($key, 'key1', 'value1'));
        $this->assertEquals(FALSE, $this->redis->hGet($key, 'key1'));
        $this->assertEquals(FALSE, $this->redis->hMGet($key, ['key1']));
        $this->assertEquals(FALSE, $this->redis->hMSet($key, ['key1' => 'value1']));
        $this->assertEquals(FALSE, $this->redis->hIncrBy($key, 'key2', 1));
        $this->assertEquals(FALSE, $this->redis->hExists($key, 'key2'));
        $this->assertEquals(FALSE, $this->redis->hDel($key, 'key2'));
        $this->assertEquals(FALSE, $this->redis->hLen($key));
        $this->assertEquals(FALSE, $this->redis->hKeys($key));
        $this->assertEquals(FALSE, $this->redis->hVals($key));
        $this->assertEquals(FALSE, $this->redis->hGetAll($key));
    }

    public function testDifferentTypeList() {
        $key = '{hash}list';
        $dkey = '{hash}' . __FUNCTION__;

        $this->redis->del($key);
        $this->assertEquals(1, $this->redis->lPush($key, 'value'));

        // string I/F
        $this->assertEquals(FALSE, $this->redis->get($key));
        $this->assertEquals(FALSE, $this->redis->getset($key, 'value2'));
        $this->assertEquals(FALSE, $this->redis->append($key, 'append'));
        $this->assertEquals(FALSE, $this->redis->getRange($key, 0, 8));
        $this->assertEquals([FALSE], $this->redis->mget([$key]));
        $this->assertEquals(FALSE, $this->redis->incr($key));
        $this->assertEquals(FALSE, $this->redis->incrBy($key, 1));
        $this->assertEquals(FALSE, $this->redis->decr($key));
        $this->assertEquals(FALSE, $this->redis->decrBy($key, 1));

        // sets I/F
        $this->assertEquals(FALSE, $this->redis->sAdd($key, 'sValue1'));
        $this->assertEquals(FALSE, $this->redis->srem($key, 'sValue1'));
        $this->assertEquals(FALSE, $this->redis->sPop($key));
        $this->assertEquals(FALSE, $this->redis->sMove($key, $dkey . 'skey1', 'sValue1'));
        $this->assertEquals(FALSE, $this->redis->scard($key));
        $this->assertEquals(FALSE, $this->redis->sismember($key, 'sValue1'));
        $this->assertEquals(FALSE, $this->redis->sInter($key, $dkey . 'skey2'));
        $this->assertEquals(FALSE, $this->redis->sUnion($key, $dkey . 'skey4'));
        $this->assertEquals(FALSE, $this->redis->sDiff($key, $dkey . 'skey7'));
        $this->assertEquals(FALSE, $this->redis->sMembers($key));
        $this->assertEquals(FALSE, $this->redis->sRandMember($key));

        // sorted sets I/F
        $this->assertEquals(FALSE, $this->redis->zAdd($key, 1, 'zValue1'));
        $this->assertEquals(FALSE, $this->redis->zRem($key, 'zValue1'));
        $this->assertEquals(FALSE, $this->redis->zIncrBy($key, 1, 'zValue1'));
        $this->assertEquals(FALSE, $this->redis->zRank($key, 'zValue1'));
        $this->assertEquals(FALSE, $this->redis->zRevRank($key, 'zValue1'));
        $this->assertEquals(FALSE, $this->redis->zRange($key, 0, -1));
        $this->assertEquals(FALSE, $this->redis->zRevRange($key, 0, -1));
        $this->assertEquals(FALSE, $this->redis->zRangeByScore($key, 1, 2));
        $this->assertEquals(FALSE, $this->redis->zCount($key, 0, -1));
        $this->assertEquals(FALSE, $this->redis->zCard($key));
        $this->assertEquals(FALSE, $this->redis->zScore($key, 'zValue1'));
        $this->assertEquals(FALSE, $this->redis->zRemRangeByRank($key, 1, 2));
        $this->assertEquals(FALSE, $this->redis->zRemRangeByScore($key, 1, 2));

        // hash I/F
        $this->assertEquals(FALSE, $this->redis->hSet($key, 'key1', 'value1'));
        $this->assertEquals(FALSE, $this->redis->hGet($key, 'key1'));
        $this->assertEquals(FALSE, $this->redis->hMGet($key, ['key1']));
        $this->assertEquals(FALSE, $this->redis->hMSet($key, ['key1' => 'value1']));
        $this->assertEquals(FALSE, $this->redis->hIncrBy($key, 'key2', 1));
        $this->assertEquals(FALSE, $this->redis->hExists($key, 'key2'));
        $this->assertEquals(FALSE, $this->redis->hDel($key, 'key2'));
        $this->assertEquals(FALSE, $this->redis->hLen($key));
        $this->assertEquals(FALSE, $this->redis->hKeys($key));
        $this->assertEquals(FALSE, $this->redis->hVals($key));
        $this->assertEquals(FALSE, $this->redis->hGetAll($key));
    }

    public function testDifferentTypeSet() {
        $key = '{hash}set';
        $dkey = '{hash}' . __FUNCTION__;
        $this->redis->del($key);
        $this->assertEquals(1, $this->redis->sAdd($key, 'value'));

        // string I/F
        $this->assertEquals(FALSE, $this->redis->get($key));
        $this->assertEquals(FALSE, $this->redis->getset($key, 'value2'));
        $this->assertEquals(FALSE, $this->redis->append($key, 'append'));
        $this->assertEquals(FALSE, $this->redis->getRange($key, 0, 8));
        $this->assertEquals([FALSE], $this->redis->mget([$key]));
        $this->assertEquals(FALSE, $this->redis->incr($key));
        $this->assertEquals(FALSE, $this->redis->incrBy($key, 1));
        $this->assertEquals(FALSE, $this->redis->decr($key));
        $this->assertEquals(FALSE, $this->redis->decrBy($key, 1));

        // lists I/F
        $this->assertEquals(FALSE, $this->redis->rPush($key, 'lvalue'));
        $this->assertEquals(FALSE, $this->redis->lPush($key, 'lvalue'));
        $this->assertEquals(FALSE, $this->redis->lLen($key));
        $this->assertEquals(FALSE, $this->redis->lPop($key));
        $this->assertEquals(FALSE, $this->redis->lrange($key, 0, -1));
        $this->assertEquals(FALSE, $this->redis->lTrim($key, 0, 1));
        $this->assertEquals(FALSE, $this->redis->lIndex($key, 0));
        $this->assertEquals(FALSE, $this->redis->lSet($key, 0, "newValue"));
        $this->assertEquals(FALSE, $this->redis->lrem($key, 'lvalue', 1));
        $this->assertEquals(FALSE, $this->redis->lPop($key));
        $this->assertEquals(FALSE, $this->redis->rPop($key));
        $this->assertEquals(FALSE, $this->redis->rPoplPush($key, $dkey  . 'lkey1'));

        // sorted sets I/F
        $this->assertEquals(FALSE, $this->redis->zAdd($key, 1, 'zValue1'));
        $this->assertEquals(FALSE, $this->redis->zRem($key, 'zValue1'));
        $this->assertEquals(FALSE, $this->redis->zIncrBy($key, 1, 'zValue1'));
        $this->assertEquals(FALSE, $this->redis->zRank($key, 'zValue1'));
        $this->assertEquals(FALSE, $this->redis->zRevRank($key, 'zValue1'));
        $this->assertEquals(FALSE, $this->redis->zRange($key, 0, -1));
        $this->assertEquals(FALSE, $this->redis->zRevRange($key, 0, -1));
        $this->assertEquals(FALSE, $this->redis->zRangeByScore($key, 1, 2));
        $this->assertEquals(FALSE, $this->redis->zCount($key, 0, -1));
        $this->assertEquals(FALSE, $this->redis->zCard($key));
        $this->assertEquals(FALSE, $this->redis->zScore($key, 'zValue1'));
        $this->assertEquals(FALSE, $this->redis->zRemRangeByRank($key, 1, 2));
        $this->assertEquals(FALSE, $this->redis->zRemRangeByScore($key, 1, 2));

        // hash I/F
        $this->assertEquals(FALSE, $this->redis->hSet($key, 'key1', 'value1'));
        $this->assertEquals(FALSE, $this->redis->hGet($key, 'key1'));
        $this->assertEquals(FALSE, $this->redis->hMGet($key, ['key1']));
        $this->assertEquals(FALSE, $this->redis->hMSet($key, ['key1' => 'value1']));
        $this->assertEquals(FALSE, $this->redis->hIncrBy($key, 'key2', 1));
        $this->assertEquals(FALSE, $this->redis->hExists($key, 'key2'));
        $this->assertEquals(FALSE, $this->redis->hDel($key, 'key2'));
        $this->assertEquals(FALSE, $this->redis->hLen($key));
        $this->assertEquals(FALSE, $this->redis->hKeys($key));
        $this->assertEquals(FALSE, $this->redis->hVals($key));
        $this->assertEquals(FALSE, $this->redis->hGetAll($key));
    }

    public function testDifferentTypeSortedSet() {
        $key = '{hash}sortedset';
        $dkey = '{hash}' . __FUNCTION__;

        $this->redis->del($key);
        $this->assertEquals(1, $this->redis->zAdd($key, 0, 'value'));

        // string I/F
        $this->assertEquals(FALSE, $this->redis->get($key));
        $this->assertEquals(FALSE, $this->redis->getset($key, 'value2'));
        $this->assertEquals(FALSE, $this->redis->append($key, 'append'));
        $this->assertEquals(FALSE, $this->redis->getRange($key, 0, 8));
        $this->assertEquals([FALSE], $this->redis->mget([$key]));
        $this->assertEquals(FALSE, $this->redis->incr($key));
        $this->assertEquals(FALSE, $this->redis->incrBy($key, 1));
        $this->assertEquals(FALSE, $this->redis->decr($key));
        $this->assertEquals(FALSE, $this->redis->decrBy($key, 1));

        // lists I/F
        $this->assertEquals(FALSE, $this->redis->rPush($key, 'lvalue'));
        $this->assertEquals(FALSE, $this->redis->lPush($key, 'lvalue'));
        $this->assertEquals(FALSE, $this->redis->lLen($key));
        $this->assertEquals(FALSE, $this->redis->lPop($key));
        $this->assertEquals(FALSE, $this->redis->lrange($key, 0, -1));
        $this->assertEquals(FALSE, $this->redis->lTrim($key, 0, 1));
        $this->assertEquals(FALSE, $this->redis->lIndex($key, 0));
        $this->assertEquals(FALSE, $this->redis->lSet($key, 0, "newValue"));
        $this->assertEquals(FALSE, $this->redis->lrem($key, 'lvalue', 1));
        $this->assertEquals(FALSE, $this->redis->lPop($key));
        $this->assertEquals(FALSE, $this->redis->rPop($key));
        $this->assertEquals(FALSE, $this->redis->rPoplPush($key, $dkey . 'lkey1'));

        // sets I/F
        $this->assertEquals(FALSE, $this->redis->sAdd($key, 'sValue1'));
        $this->assertEquals(FALSE, $this->redis->srem($key, 'sValue1'));
        $this->assertEquals(FALSE, $this->redis->sPop($key));
        $this->assertEquals(FALSE, $this->redis->sMove($key, $dkey . 'skey1', 'sValue1'));
        $this->assertEquals(FALSE, $this->redis->scard($key));
        $this->assertEquals(FALSE, $this->redis->sismember($key, 'sValue1'));
        $this->assertEquals(FALSE, $this->redis->sInter($key, $dkey . 'skey2'));
        $this->assertEquals(FALSE, $this->redis->sUnion($key, $dkey . 'skey4'));
        $this->assertEquals(FALSE, $this->redis->sDiff($key, $dkey . 'skey7'));
        $this->assertEquals(FALSE, $this->redis->sMembers($key));
        $this->assertEquals(FALSE, $this->redis->sRandMember($key));

        // hash I/F
        $this->assertEquals(FALSE, $this->redis->hSet($key, 'key1', 'value1'));
        $this->assertEquals(FALSE, $this->redis->hGet($key, 'key1'));
        $this->assertEquals(FALSE, $this->redis->hMGet($key, ['key1']));
        $this->assertEquals(FALSE, $this->redis->hMSet($key, ['key1' => 'value1']));
        $this->assertEquals(FALSE, $this->redis->hIncrBy($key, 'key2', 1));
        $this->assertEquals(FALSE, $this->redis->hExists($key, 'key2'));
        $this->assertEquals(FALSE, $this->redis->hDel($key, 'key2'));
        $this->assertEquals(FALSE, $this->redis->hLen($key));
        $this->assertEquals(FALSE, $this->redis->hKeys($key));
        $this->assertEquals(FALSE, $this->redis->hVals($key));
        $this->assertEquals(FALSE, $this->redis->hGetAll($key));
    }

    public function testDifferentTypeHash() {
        $key = '{hash}hash';
        $dkey = '{hash}hash';

        $this->redis->del($key);
        $this->assertEquals(1, $this->redis->hSet($key, 'key', 'value'));

        // string I/F
        $this->assertEquals(FALSE, $this->redis->get($key));
        $this->assertEquals(FALSE, $this->redis->getset($key, 'value2'));
        $this->assertEquals(FALSE, $this->redis->append($key, 'append'));
        $this->assertEquals(FALSE, $this->redis->getRange($key, 0, 8));
        $this->assertEquals([FALSE], $this->redis->mget([$key]));
        $this->assertEquals(FALSE, $this->redis->incr($key));
        $this->assertEquals(FALSE, $this->redis->incrBy($key, 1));
        $this->assertEquals(FALSE, $this->redis->decr($key));
        $this->assertEquals(FALSE, $this->redis->decrBy($key, 1));

        // lists I/F
        $this->assertEquals(FALSE, $this->redis->rPush($key, 'lvalue'));
        $this->assertEquals(FALSE, $this->redis->lPush($key, 'lvalue'));
        $this->assertEquals(FALSE, $this->redis->lLen($key));
        $this->assertEquals(FALSE, $this->redis->lPop($key));
        $this->assertEquals(FALSE, $this->redis->lrange($key, 0, -1));
        $this->assertEquals(FALSE, $this->redis->lTrim($key, 0, 1));
        $this->assertEquals(FALSE, $this->redis->lIndex($key, 0));
        $this->assertEquals(FALSE, $this->redis->lSet($key, 0, "newValue"));
        $this->assertEquals(FALSE, $this->redis->lrem($key, 'lvalue', 1));
        $this->assertEquals(FALSE, $this->redis->lPop($key));
        $this->assertEquals(FALSE, $this->redis->rPop($key));
        $this->assertEquals(FALSE, $this->redis->rPoplPush($key, $dkey . 'lkey1'));

        // sets I/F
        $this->assertEquals(FALSE, $this->redis->sAdd($key, 'sValue1'));
        $this->assertEquals(FALSE, $this->redis->srem($key, 'sValue1'));
        $this->assertEquals(FALSE, $this->redis->sPop($key));
        $this->assertEquals(FALSE, $this->redis->sMove($key, $dkey . 'skey1', 'sValue1'));
        $this->assertEquals(FALSE, $this->redis->scard($key));
        $this->assertEquals(FALSE, $this->redis->sismember($key, 'sValue1'));
        $this->assertEquals(FALSE, $this->redis->sInter($key, $dkey . 'skey2'));
        $this->assertEquals(FALSE, $this->redis->sUnion($key, $dkey . 'skey4'));
        $this->assertEquals(FALSE, $this->redis->sDiff($key, $dkey . 'skey7'));
        $this->assertEquals(FALSE, $this->redis->sMembers($key));
        $this->assertEquals(FALSE, $this->redis->sRandMember($key));

        // sorted sets I/F
        $this->assertEquals(FALSE, $this->redis->zAdd($key, 1, 'zValue1'));
        $this->assertEquals(FALSE, $this->redis->zRem($key, 'zValue1'));
        $this->assertEquals(FALSE, $this->redis->zIncrBy($key, 1, 'zValue1'));
        $this->assertEquals(FALSE, $this->redis->zRank($key, 'zValue1'));
        $this->assertEquals(FALSE, $this->redis->zRevRank($key, 'zValue1'));
        $this->assertEquals(FALSE, $this->redis->zRange($key, 0, -1));
        $this->assertEquals(FALSE, $this->redis->zRevRange($key, 0, -1));
        $this->assertEquals(FALSE, $this->redis->zRangeByScore($key, 1, 2));
        $this->assertEquals(FALSE, $this->redis->zCount($key, 0, -1));
        $this->assertEquals(FALSE, $this->redis->zCard($key));
        $this->assertEquals(FALSE, $this->redis->zScore($key, 'zValue1'));
        $this->assertEquals(FALSE, $this->redis->zRemRangeByRank($key, 1, 2));
        $this->assertEquals(FALSE, $this->redis->zRemRangeByScore($key, 1, 2));
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

    public function testSerializerMsgPack() {
        if(defined('Redis::SERIALIZER_MSGPACK')) {
            $this->checkSerializer(Redis::SERIALIZER_MSGPACK);

            // with prefix
            $this->redis->setOption(Redis::OPT_PREFIX, "test:");
            $this->checkSerializer(Redis::SERIALIZER_MSGPACK);
            $this->redis->setOption(Redis::OPT_PREFIX, "");
        }
    }

    public function testSerializerJSON()
    {
        $this->checkSerializer(Redis::SERIALIZER_JSON);

        // with prefix
        $this->redis->setOption(Redis::OPT_PREFIX, "test:");
        $this->checkSerializer(Redis::SERIALIZER_JSON);
        $this->redis->setOption(Redis::OPT_PREFIX, "");
    }

    private function checkSerializer($mode) {

        $this->redis->del('key');
        $this->assertTrue($this->redis->getOption(Redis::OPT_SERIALIZER) === Redis::SERIALIZER_NONE);   // default

        $this->assertTrue($this->redis->setOption(Redis::OPT_SERIALIZER, $mode) === TRUE);  // set ok
        $this->assertTrue($this->redis->getOption(Redis::OPT_SERIALIZER) === $mode);    // get ok

        // lPush, rPush
        $a = ['hello world', 42, TRUE, ['<tag>' => 1729]];
        $this->redis->del('key');
        $this->redis->lPush('key', $a[0]);
        $this->redis->rPush('key', $a[1]);
        $this->redis->rPush('key', $a[2]);
        $this->redis->rPush('key', $a[3]);

        // lrange
        $this->assertTrue($a === $this->redis->lrange('key', 0, -1));

        // lIndex
        $this->assertTrue($a[0] === $this->redis->lIndex('key', 0));
        $this->assertTrue($a[1] === $this->redis->lIndex('key', 1));
        $this->assertTrue($a[2] === $this->redis->lIndex('key', 2));
        $this->assertTrue($a[3] === $this->redis->lIndex('key', 3));

        // lrem
        $this->assertTrue($this->redis->lrem('key', $a[3]) === 1);
        $this->assertTrue(array_slice($a, 0, 3) === $this->redis->lrange('key', 0, -1));

        // lSet
        $a[0] = ['k' => 'v']; // update
        $this->assertTrue(TRUE === $this->redis->lSet('key', 0, $a[0]));
        $this->assertTrue($a[0] === $this->redis->lIndex('key', 0));

        // lInsert
        $this->assertTrue($this->redis->lInsert('key', Redis::BEFORE, $a[0], [1,2,3]) === 4);
        $this->assertTrue($this->redis->lInsert('key', Redis::AFTER, $a[0], [4,5,6]) === 5);

        $a = [[1,2,3], $a[0], [4,5,6], $a[1], $a[2]];
        $this->assertTrue($a === $this->redis->lrange('key', 0, -1));

        // sAdd
        $this->redis->del('{set}key');
        $s = [1,'a', [1,2,3], ['k' => 'v']];

        $this->assertTrue(1 === $this->redis->sAdd('{set}key', $s[0]));
        $this->assertTrue(1 === $this->redis->sAdd('{set}key', $s[1]));
        $this->assertTrue(1 === $this->redis->sAdd('{set}key', $s[2]));
        $this->assertTrue(1 === $this->redis->sAdd('{set}key', $s[3]));

        // variadic sAdd
        $this->redis->del('k');
        $this->assertTrue(3 === $this->redis->sAdd('k', 'a', 'b', 'c'));
        $this->assertTrue(1 === $this->redis->sAdd('k', 'a', 'b', 'c', 'd'));

        // srem
        $this->assertTrue(1 === $this->redis->srem('{set}key', $s[3]));
        $this->assertTrue(0 === $this->redis->srem('{set}key', $s[3]));

        // variadic
        $this->redis->del('k');
        $this->redis->sAdd('k', 'a', 'b', 'c', 'd');
        $this->assertTrue(2 === $this->redis->sRem('k', 'a', 'd'));
        $this->assertTrue(2 === $this->redis->sRem('k', 'b', 'c', 'e'));
        $this->assertEquals(0, $this->redis->exists('k'));

        // sismember
        $this->assertTrue(TRUE === $this->redis->sismember('{set}key', $s[0]));
        $this->assertTrue(TRUE === $this->redis->sismember('{set}key', $s[1]));
        $this->assertTrue(TRUE === $this->redis->sismember('{set}key', $s[2]));
        $this->assertTrue(FALSE === $this->redis->sismember('{set}key', $s[3]));
        unset($s[3]);

        // sMove
        $this->redis->del('{set}tmp');
        $this->redis->sMove('{set}key', '{set}tmp', $s[0]);
        $this->assertTrue(FALSE === $this->redis->sismember('{set}key', $s[0]));
        $this->assertTrue(TRUE === $this->redis->sismember('{set}tmp', $s[0]));
        unset($s[0]);

        // sorted sets
        $z = ['z0', ['k' => 'v'], FALSE, NULL];
        $this->redis->del('key');

        // zAdd
        $this->assertTrue(1 === $this->redis->zAdd('key', 0, $z[0]));
        $this->assertTrue(1 === $this->redis->zAdd('key', 1, $z[1]));
        $this->assertTrue(1 === $this->redis->zAdd('key', 2, $z[2]));
        $this->assertTrue(1 === $this->redis->zAdd('key', 3, $z[3]));

        // zRem
        $this->assertTrue(1 === $this->redis->zRem('key', $z[3]));
        $this->assertTrue(0 === $this->redis->zRem('key', $z[3]));
        unset($z[3]);

        // check that zRem doesn't crash with a missing parameter (GitHub issue #102):
        $this->assertTrue(FALSE === @$this->redis->zRem('key'));

        // variadic
        $this->redis->del('k');
        $this->redis->zAdd('k', 0, 'a');
        $this->redis->zAdd('k', 1, 'b');
        $this->redis->zAdd('k', 2, 'c');
        $this->assertTrue(2 === $this->redis->zRem('k', 'a', 'c'));
        $this->assertTrue(1.0 === $this->redis->zScore('k', 'b'));
        $this->assertTrue($this->redis->zRange('k', 0, -1, true) == ['b' => 1.0]);

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
        $a = ['k0' => 1, 'k1' => 42, 'k2' => NULL, 'k3' => FALSE, 'k4' => ['a' => 'b']];
        $this->assertTrue(TRUE === $this->redis->mset($a));
        foreach($a as $k => $v) {
            $this->assertTrue($this->redis->get($k) === $v);
        }

        $a = ['k0' => 1, 'k1' => 42, 'k2' => NULL, 'k3' => FALSE, 'k4' => ['a' => 'b']];

        // hSet
        $this->redis->del('key');
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
        $this->redis->del('key');
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
        $this->redis->set('d', ['x' => 'y']);

        $this->assertTrue([NULL, FALSE, 42, ['x' => 'y']] === $this->redis->mGet(['a', 'b', 'c', 'd']));

        // pipeline
        if ($this->havePipeline()) {
            $this->sequence(Redis::PIPELINE);
        }

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
        $this->redis->set('x', [new stdClass, new stdClass]);
        $x = $this->redis->get('x');
        $this->assertTrue(is_array($x));
        if ($mode === Redis::SERIALIZER_JSON) {
            $this->assertTrue(is_array($x[0]));
            $this->assertTrue(is_array($x[1]));
        } else {
            $this->assertTrue(is_object($x[0]) && get_class($x[0]) === 'stdClass');
            $this->assertTrue(is_object($x[1]) && get_class($x[1]) === 'stdClass');
        }

        // revert
        $this->assertTrue($this->redis->setOption(Redis::OPT_SERIALIZER, Redis::SERIALIZER_NONE) === TRUE);     // set ok
        $this->assertTrue($this->redis->getOption(Redis::OPT_SERIALIZER) === Redis::SERIALIZER_NONE);       // get ok
    }

    public function testCompressionLZF()
    {
        if (!defined('Redis::COMPRESSION_LZF')) {
            $this->markTestSkipped();
        }
        $this->checkCompression(Redis::COMPRESSION_LZF, 0);
    }

    public function testCompressionZSTD()
    {
        if (!defined('Redis::COMPRESSION_ZSTD')) {
            $this->markTestSkipped();
        }

        /* Issue 1936 regression.  Make sure we don't overflow on bad data */
        $this->redis->del('badzstd');
        $this->redis->set('badzstd', '123');
        $this->redis->setOption(Redis::OPT_COMPRESSION, Redis::COMPRESSION_ZSTD);
        $this->assertEquals('123', $this->redis->get('badzstd'));
        $this->redis->setOption(Redis::OPT_COMPRESSION, Redis::COMPRESSION_NONE);

        $this->checkCompression(Redis::COMPRESSION_ZSTD, 0);
        $this->checkCompression(Redis::COMPRESSION_ZSTD, 9);
    }


    public function testCompressionLZ4()
    {
        if (!defined('Redis::COMPRESSION_LZ4')) {
            $this->markTestSkipped();
        }
        $this->checkCompression(Redis::COMPRESSION_LZ4, 0);
        $this->checkCompression(Redis::COMPRESSION_LZ4, 9);
    }

    private function checkCompression($mode, $level)
    {
        $this->assertTrue($this->redis->setOption(Redis::OPT_COMPRESSION, $mode) === TRUE);  // set ok
        $this->assertTrue($this->redis->getOption(Redis::OPT_COMPRESSION) === $mode);    // get ok

        $this->assertTrue($this->redis->setOption(Redis::OPT_COMPRESSION_LEVEL, $level) === TRUE);
        $this->assertTrue($this->redis->getOption(Redis::OPT_COMPRESSION_LEVEL) === $level);

        $val = 'xxxxxxxxxx';
        $this->redis->set('key', $val);
        $this->assertEquals($val, $this->redis->get('key'));

        /* Empty data */
        $this->redis->set('key', '');
        $this->assertEquals('', $this->redis->get('key'));

        /* Iterate through class sizes */
        for ($i = 1; $i <= 65536; $i *= 2) {
            foreach ([str_repeat('A', $i), random_bytes($i)] as $val) {
                $this->redis->set('key', $val);
                $this->assertEquals($val, $this->redis->get('key'));
            }
        }

        // Issue 1945. Ensure we decompress data with hmget.
        $this->redis->hset('hkey', 'data', 'abc');
        $this->assertEquals('abc', current($this->redis->hmget('hkey', ['data'])));
    }

    public function testDumpRestore() {

        if (version_compare($this->version, "2.5.0") < 0) {
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

        // test getLastError with a regular command
        $this->redis->set('x', 'a');
        $this->assertFalse($this->redis->incr('x'));
        $incrError = $this->redis->getLastError();
        $this->assertTrue(strlen($incrError) > 0);

        // clear error
        $this->redis->clearLastError();
        $this->assertTrue($this->redis->getLastError() === NULL);
    }

    // Helper function to compare nested results -- from the php.net array_diff page, I believe
    private function array_diff_recursive($aArray1, $aArray2) {
        $aReturn = [];

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

        if (version_compare($this->version, "2.5.0") < 0) {
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

        if (version_compare($this->version, "2.5.0") < 0) {
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
        $this->redis->del('{eval-key}-list');
        $this->redis->rpush('{eval-key}-list', 'a');
        $this->redis->rpush('{eval-key}-list', 'b');
        $this->redis->rpush('{eval-key}-list', 'c');

        // Make a set
        $this->redis->del('{eval-key}-zset');
        $this->redis->zadd('{eval-key}-zset', 0, 'd');
        $this->redis->zadd('{eval-key}-zset', 1, 'e');
        $this->redis->zadd('{eval-key}-zset', 2, 'f');

        // Basic keys
        $this->redis->set('{eval-key}-str1', 'hello, world');
        $this->redis->set('{eval-key}-str2', 'hello again!');

        // Use a script to return our list, and verify its response
        $list = $this->redis->eval("return redis.call('lrange', KEYS[1], 0, -1)", ['{eval-key}-list'], 1);
        $this->assertTrue($list === ['a','b','c']);

        // Use a script to return our zset
        $zset = $this->redis->eval("return redis.call('zrange', KEYS[1], 0, -1)", ['{eval-key}-zset'], 1);
        $this->assertTrue($zset == ['d','e','f']);

        // Test an empty MULTI BULK response
        $this->redis->del('{eval-key}-nolist');
        $empty_resp = $this->redis->eval("return redis.call('lrange', '{eval-key}-nolist', 0, -1)",
            ['{eval-key}-nolist'], 1);
        $this->assertTrue(is_array($empty_resp) && empty($empty_resp));

        // Now test a nested reply
        $nested_script = "
            return {
                1,2,3, {
                    redis.call('get', '{eval-key}-str1'),
                    redis.call('get', '{eval-key}-str2'),
                    redis.call('lrange', 'not-any-kind-of-list', 0, -1),
                    {
                        redis.call('zrange','{eval-key}-zset', 0, -1),
                        redis.call('lrange', '{eval-key}-list', 0, -1)
                    }
                }
            }
        ";

        $expected = [
            1, 2, 3, [
                'hello, world',
                'hello again!',
                [],
                [
                    ['d','e','f'],
                    ['a','b','c']
                ]
            ]
        ];

        // Now run our script, and check our values against each other
        $eval_result = $this->redis->eval($nested_script, ['{eval-key}-str1', '{eval-key}-str2', '{eval-key}-zset', '{eval-key}-list'], 4);
        $this->assertTrue(is_array($eval_result) && count($this->array_diff_recursive($eval_result, $expected)) == 0);

        /*
         * Nested reply wihin a multi/pipeline block
         */

        $num_scripts = 10;

        $arr_modes = [Redis::MULTI];
        if ($this->havePipeline()) $arr_modes[] = Redis::PIPELINE;

        foreach($arr_modes as $mode) {
            $this->redis->multi($mode);
            for($i=0;$i<$num_scripts;$i++) {
                $this->redis->eval($nested_script, ['{eval-key}-dummy'], 1);
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
        $args_args   = ['{k}1','{k}2','{k}3','v1','v2','v3'];
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
        if (version_compare($this->version, "2.5.0") < 0) {
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

    public function testSerialize() {
        $vals = [1, 1.5, 'one', ['here','is','an','array']];

        // Test with no serialization at all
        $this->assertTrue($this->redis->_serialize('test') === 'test');
        $this->assertTrue($this->redis->_serialize(1) === '1');
        $this->assertTrue($this->redis->_serialize([]) === 'Array');
        $this->assertTrue($this->redis->_serialize(new stdClass) === 'Object');

        $arr_serializers = [Redis::SERIALIZER_PHP];
        if(defined('Redis::SERIALIZER_IGBINARY')) {
            $arr_serializers[] = Redis::SERIALIZER_IGBINARY;
        }

        if(defined('Redis::SERIALIZER_MSGPACK')) {
            $arr_serializers[] = Redis::SERIALIZER_MSGPACK;
        }

        foreach($arr_serializers as $mode) {
            $arr_enc = [];
            $arr_dec = [];

            foreach($vals as $k => $v) {
                $enc = $this->redis->_serialize($v);
                $dec = $this->redis->_unserialize($enc);

                // They should be the same
                $this->assertTrue($enc == $dec);
            }
        }
    }

    public function testUnserialize() {
        $vals = [
            1,1.5,'one',['this','is','an','array']
        ];

        $serializers = Array(Redis::SERIALIZER_PHP);

        if(defined('Redis::SERIALIZER_IGBINARY')) {
            $serializers[] = Redis::SERIALIZER_IGBINARY;
        }

        if(defined('Redis::SERIALIZER_MSGPACK')) {
            $serializers[] = Redis::SERIALIZER_MSGPACK;
        }

        foreach($serializers as $mode) {
            $vals_enc = [];

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

    public function testReplyLiteral() {
        $this->redis->setOption(Redis::OPT_REPLY_LITERAL, false);
        $this->assertTrue($this->redis->rawCommand('set', 'foo', 'bar'));
        $this->assertTrue($this->redis->eval("return redis.call('set', 'foo', 'bar')", [], 0));

        $rv = $this->redis->eval("return {redis.call('set', KEYS[1], 'bar'), redis.call('ping')}", ['foo'], 1);
        $this->assertEquals([true, true], $rv);

        $this->redis->setOption(Redis::OPT_REPLY_LITERAL, true);
        $this->assertEquals('OK', $this->redis->rawCommand('set', 'foo', 'bar'));
        $this->assertEquals('OK', $this->redis->eval("return redis.call('set', 'foo', 'bar')", [], 0));

        // Nested
        $rv = $this->redis->eval("return {redis.call('set', KEYS[1], 'bar'), redis.call('ping')}", ['foo'], 1);
        $this->assertEquals(['OK', 'PONG'], $rv);

        // Reset
        $this->redis->setOption(Redis::OPT_REPLY_LITERAL, false);
    }

    public function testNullArray() {
        $key = "key:arr";
        $this->redis->del($key);

        foreach ([false => [], true => NULL] as $opt => $test) {
            $this->redis->setOption(Redis::OPT_NULL_MULTIBULK_AS_NULL, $opt);

            $r = $this->redis->rawCommand("BLPOP", $key, .05);
            $this->assertEquals($test, $r);

            $this->redis->multi();
            $this->redis->rawCommand("BLPOP", $key, .05);
            $r = $this->redis->exec();
            $this->assertEquals([$test], $r);
        }

        $this->redis->setOption(Redis::OPT_NULL_MULTIBULK_AS_NULL, false);
    }

    /* Test that we can configure PhpRedis to return NULL for *-1 even nestedwithin replies */
    public function testNestedNullArray() {
        $this->redis->del('{notaset}');

        foreach ([false => [], true => NULL] as $opt => $test) {
            $this->redis->setOption(Redis::OPT_NULL_MULTIBULK_AS_NULL, $opt);
            $this->assertEquals([$test, $test], $this->redis->geoPos('{notaset}', 'm1', 'm2'));

            $this->redis->multi();
            $this->redis->geoPos('{notaset}', 'm1', 'm2');
            $this->assertEquals([[$test, $test]], $this->redis->exec());
        }

        $this->redis->setOption(Redis::OPT_NULL_MULTIBULK_AS_NULL, false);
    }

    public function testReconnectSelect() {
        $key = 'reconnect-select';
        $value = 'Has been set!';

        $original_cfg = $this->redis->config('GET', 'timeout');

        // Make sure the default DB doesn't have the key.
        $this->redis->select(0);
        $this->redis->del($key);

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

        if (version_compare($this->version, "2.5.0") < 0) {
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
        $this->assertTrue($this->redis->getHost() === $this->getHost());
        $this->assertTrue($this->redis->getPort() === $this->getPort());
        $this->assertTrue($this->redis->getAuth() === $this->getAuth());
    }

    /**
     * Scan and variants
     */

    protected function get_keyspace_count($str_db) {
        $arr_info = $this->redis->info();
        if (isset($arr_info[$str_db])) {
            $arr_info = $arr_info[$str_db];
            $arr_info = explode(',', $arr_info);
            $arr_info = explode('=', $arr_info[0]);
            return $arr_info[1];
        } else {
            return 0;
        }
    }

    public function testScan() {
        if(version_compare($this->version, "2.8.0") < 0) {
            $this->markTestSkipped();
            return;
        }

        // Key count
        $i_key_count = $this->get_keyspace_count('db0');

        // Have scan retry
        $this->redis->setOption(Redis::OPT_SCAN, Redis::SCAN_RETRY);

        // Scan them all
        $it = NULL;
        while($arr_keys = $this->redis->scan($it)) {
            $i_key_count -= count($arr_keys);
        }
        // Should have iterated all keys
        $this->assertEquals(0, $i_key_count);

        // Unique keys, for pattern matching
        $str_uniq = uniqid() . '-' . uniqid();
        for($i=0;$i<10;$i++) {
            $this->redis->set($str_uniq . "::$i", "bar::$i");
        }

        // Scan just these keys using a pattern match
        $it = NULL;
        while($arr_keys = $this->redis->scan($it, "*$str_uniq*")) {
            $i -= count($arr_keys);
        }
        $this->assertEquals(0, $i);

        // SCAN with type is scheduled for release in Redis 6.
        if (version_compare($this->version, "6.0.0") >= 0) {
            // Use a unique ID so we can find our type keys
            $id = uniqid();

            // Create some simple keys and lists
            for ($i = 0; $i < 3; $i++) {
                $str_simple = "simple:{$id}:$i";
                $str_list = "list:{$id}:$i";

                $this->redis->set($str_simple, $i);
                $this->redis->del($str_list);
                $this->redis->rpush($str_list, ['foo']);

                $arr_keys["STRING"][] = $str_simple;
                $arr_keys["LIST"][] = $str_list;
            }

            // Make sure we can scan for specific types
            foreach ($arr_keys as $str_type => $arr_vals) {
                foreach ([NULL, 10] as $i_count) {
                    $arr_resp = [];

                    $it = NULL;
                    while ($arr_scan = $this->redis->scan($it, "*$id*", $i_count, $str_type)) {
                        $arr_resp = array_merge($arr_resp, $arr_scan);
                    }

                    sort($arr_vals); sort($arr_resp);
                    $this->assertEquals($arr_vals, $arr_resp);
                }
            }
        }
    }

    public function testScanPrefix() {
        $keyid = uniqid();

        /* Set some keys with different prefixes */
        $arr_prefixes = ['prefix-a:', 'prefix-b:'];
        foreach ($arr_prefixes as $str_prefix) {
            $this->redis->setOption(Redis::OPT_PREFIX, $str_prefix);
            $this->redis->set("$keyid", "LOLWUT");
            $arr_all_keys["${str_prefix}${keyid}"] = true;
        }

        $this->redis->setOption(Redis::OPT_SCAN, Redis::SCAN_RETRY);
        $this->redis->setOption(Redis::OPT_SCAN, Redis::SCAN_PREFIX);

        foreach ($arr_prefixes as $str_prefix) {
            $this->redis->setOption(Redis::OPT_PREFIX, $str_prefix);
            $it = NULL;
            $arr_keys = $this->redis->scan($it, "*$keyid*");
            $this->assertEquals($arr_keys, ["${str_prefix}${keyid}"]);
        }

        /* Unset the prefix option */
        $this->redis->setOption(Redis::OPT_SCAN, Redis::SCAN_NOPREFIX);

        $it = NULL;
        while ($arr_keys = $this->redis->scan($it, "*$keyid*")) {
            foreach ($arr_keys as $str_key) {
                unset($arr_all_keys[$str_key]);
            }
        }

        /* Should have touched every key */
        $this->assertTrue(count($arr_all_keys) == 0);
    }

    public function testHScan() {
        if (version_compare($this->version, "2.8.0") < 0) {
            $this->markTestSkipped();
            return;
        }

        // Never get empty sets
        $this->redis->setOption(Redis::OPT_SCAN, Redis::SCAN_RETRY);

        $this->redis->del('hash');
        $i_foo_mems = 0;

        for($i=0;$i<100;$i++) {
            if($i>3) {
                $this->redis->hset('hash', "member:$i", "value:$i");
            } else {
                $this->redis->hset('hash', "foomember:$i", "value:$i");
                $i_foo_mems++;
            }
        }

        // Scan all of them
        $it = NULL;
        while($arr_keys = $this->redis->hscan('hash', $it)) {
            $i -= count($arr_keys);
        }
        $this->assertEquals(0, $i);

        // Scan just *foomem* (should be 4)
        $it = NULL;
        while($arr_keys = $this->redis->hscan('hash', $it, '*foomember*')) {
            $i_foo_mems -= count($arr_keys);
            foreach($arr_keys as $str_mem => $str_val) {
                $this->assertTrue(strpos($str_mem, 'member')!==FALSE);
                $this->assertTrue(strpos($str_val, 'value')!==FALSE);
            }
        }
        $this->assertEquals(0, $i_foo_mems);
    }

    public function testSScan() {
        if (version_compare($this->version, "2.8.0") < 0) {
            $this->markTestSkipped();
            return;
        }

        $this->redis->setOption(Redis::OPT_SCAN, Redis::SCAN_RETRY);

        $this->redis->del('set');
        for($i=0;$i<100;$i++) {
            $this->redis->sadd('set', "member:$i");
        }

        // Scan all of them
        $it = NULL;
        while($arr_keys = $this->redis->sscan('set', $it)) {
            $i -= count($arr_keys);
            foreach($arr_keys as $str_mem) {
                $this->assertTrue(strpos($str_mem,'member')!==FALSE);
            }
        }
        $this->assertEquals(0, $i);

        // Scan just ones with zero in them (0, 10, 20, 30, 40, 50, 60, 70, 80, 90)
        $it = NULL;
        $i_w_zero = 0;
        while($arr_keys = $this->redis->sscan('set', $it, '*0*')) {
            $i_w_zero += count($arr_keys);
        }
        $this->assertEquals(10, $i_w_zero);
    }

    public function testZScan() {
        if (version_compare($this->version, "2.8.0") < 0) {
            $this->markTestSkipped();
            return;
        }

        $this->redis->setOption(Redis::OPT_SCAN, Redis::SCAN_RETRY);

        $this->redis->del('zset');
        $i_tot_score = 0;
        $i_p_score = 0;
        $i_p_count = 0;
        for($i=0;$i<2000;$i++) {
            if($i<10) {
                $this->redis->zadd('zset', $i, "pmem:$i");
                $i_p_score += $i;
                $i_p_count += 1;
            } else {
                $this->redis->zadd('zset', $i, "mem:$i");
            }

            $i_tot_score += $i;
        }

        // Scan them all
        $it = NULL;
        while($arr_keys = $this->redis->zscan('zset', $it)) {
            foreach($arr_keys as $str_mem => $f_score) {
                $i_tot_score -= $f_score;
                $i--;
            }
        }

        $this->assertEquals(0, $i);
        $this->assertEquals((float)0, $i_tot_score);

        // Just scan "pmem" members
        $it = NULL;
        $i_p_score_old = $i_p_score;
        $i_p_count_old = $i_p_count;
        while($arr_keys = $this->redis->zscan('zset', $it, "*pmem*")) {
            foreach($arr_keys as $str_mem => $f_score) {
                $i_p_score -= $f_score;
                $i_p_count -= 1;
            }
        }
        $this->assertEquals((float)0, $i_p_score);
        $this->assertEquals(0, $i_p_count);

        // Turn off retrying and we should get some empty results
        $this->redis->setOption(Redis::OPT_SCAN, Redis::SCAN_NORETRY);
        $i_skips = 0;
        $i_p_score = $i_p_score_old;
        $i_p_count = $i_p_count_old;
        $it = NULL;
        while(($arr_keys = $this->redis->zscan('zset', $it, "*pmem*")) !== FALSE) {
            if(count($arr_keys) == 0) $i_skips++;
            foreach($arr_keys as $str_mem => $f_score) {
                $i_p_score -= $f_score;
                $i_p_count -= 1;
            }
        }
        // We should still get all the keys, just with several empty results
        $this->assertTrue($i_skips > 0);
        $this->assertEquals((float)0, $i_p_score);
        $this->assertEquals(0, $i_p_count);
    }

    //
    // HyperLogLog (PF) commands
    //

    protected function createPFKey($str_key, $i_count) {
        $arr_mems = [];
        for($i=0;$i<$i_count;$i++) {
            $arr_mems[] = uniqid() . '-' . $i;
        }

        // Estimation by Redis
        $this->redis->pfadd($str_key, $i_count);
    }

    public function testPFCommands() {
        // Isn't available until 2.8.9
        if (version_compare($this->version, "2.8.9") < 0) {
            $this->markTestSkipped();
            return;
        }

        $str_uniq = uniqid();
        $arr_mems = [];

        for($i=0;$i<1000;$i++) {
            if($i%2 == 0) {
                $arr_mems[] = $str_uniq . '-' . $i;
            } else {
                $arr_mems[] = $i;
            }
        }

        // How many keys to create
        $i_keys = 10;

        // Iterate prefixing/serialization options
        foreach([Redis::SERIALIZER_NONE, Redis::SERIALIZER_PHP] as $str_ser) {
            foreach(['', 'hl-key-prefix:'] as $str_prefix) {
                $arr_keys = [];

                // Now add for each key
                for($i=0;$i<$i_keys;$i++) {
                    $str_key    = "{key}:$i";
                    $arr_keys[] = $str_key;

                    // Clean up this key
                    $this->redis->del($str_key);

                    // Add to our cardinality set, and confirm we got a valid response
                    $this->assertTrue($this->redis->pfadd($str_key, $arr_mems));

                    // Grab estimated cardinality
                    $i_card = $this->redis->pfcount($str_key);
                    $this->assertTrue(is_int($i_card));

                    // Count should be close
                    $this->assertLess(abs($i_card-count($arr_mems)), count($arr_mems) * .1);

                    // The PFCOUNT on this key should be the same as the above returned response
                    $this->assertEquals($this->redis->pfcount($str_key), $i_card);
                }

                // Clean up merge key
                $this->redis->del('pf-merge-{key}');

                // Merge the counters
                $this->assertTrue($this->redis->pfmerge('pf-merge-{key}', $arr_keys));

                // Validate our merged count
                $i_redis_card = $this->redis->pfcount('pf-merge-{key}');

                // Merged cardinality should still be roughly 1000
                $this->assertLess(abs($i_redis_card-count($arr_mems)), count($arr_mems) * .1);

                // Clean up merge key
                $this->redis->del('pf-merge-{key}');
            }
        }
    }

    //
    // GEO* command tests
    //

    protected function rawCommandArray($key, $args) {
        return call_user_func_array([$this->redis, 'rawCommand'], $args);
    }

    protected function addCities($key) {
        $this->redis->del($key);
        foreach ($this->cities as $city => $longlat) {
            $this->redis->geoadd($key, $longlat[0], $longlat[1], $city);
        }
    }

    /* GEOADD */
    public function testGeoAdd() {
        if (!$this->minVersionCheck("3.2")) {
            return $this->markTestSkipped();
        }

        $this->redis->del('geokey');

        /* Add them one at a time */
        foreach ($this->cities as $city => $longlat) {
            $this->assertEquals($this->redis->geoadd('geokey', $longlat[0], $longlat[1], $city), 1);
        }

        /* Add them again, all at once */
        $args = ['geokey'];
        foreach ($this->cities as $city => $longlat) {
            $args = array_merge($args, [$longlat[0], $longlat[1], $city]);
        }

        /* They all exist, should be nothing added */
        $this->assertEquals(call_user_func_array([$this->redis, 'geoadd'], $args), 0);
    }

    /* GEORADIUS */
    public function genericGeoRadiusTest($cmd) {
        if (!$this->minVersionCheck("3.2.0")) {
            return $this->markTestSkipped();
        }

        /* Chico */
        $city = 'Chico';
        $lng = -121.837478;
        $lat = 39.728494;

        $this->addCities('{gk}');

        /* Pre tested with redis-cli.  We're just verifying proper delivery of distance and unit */
        if ($cmd == 'georadius' || $cmd == 'georadius_ro') {
            $this->assertEquals($this->redis->$cmd('{gk}', $lng, $lat, 10, 'mi'), Array('Chico'));
            $this->assertEquals($this->redis->$cmd('{gk}', $lng, $lat, 30, 'mi'), Array('Gridley','Chico'));
            $this->assertEquals($this->redis->$cmd('{gk}', $lng, $lat, 50, 'km'), Array('Gridley','Chico'));
            $this->assertEquals($this->redis->$cmd('{gk}', $lng, $lat, 50000, 'm'), Array('Gridley','Chico'));
            $this->assertEquals($this->redis->$cmd('{gk}', $lng, $lat, 150000, 'ft'), Array('Gridley', 'Chico'));
            $args = Array($cmd, '{gk}', $lng, $lat, 500, 'mi');

            /* Test a bad COUNT argument */
            foreach (Array(-1, 0, 'notanumber') as $count) {
                $this->assertFalse(@$this->redis->$cmd('{gk}', $lng, $lat, 10, 'mi', Array('count' => $count)));
            }
        } else {
            $this->assertEquals($this->redis->$cmd('{gk}', $city, 10, 'mi'), Array('Chico'));
            $this->assertEquals($this->redis->$cmd('{gk}', $city, 30, 'mi'), Array('Gridley','Chico'));
            $this->assertEquals($this->redis->$cmd('{gk}', $city, 50, 'km'), Array('Gridley','Chico'));
            $this->assertEquals($this->redis->$cmd('{gk}', $city, 50000, 'm'), Array('Gridley','Chico'));
            $this->assertEquals($this->redis->$cmd('{gk}', $city, 150000, 'ft'), Array('Gridley', 'Chico'));
            $args = Array($cmd, '{gk}', $city, 500, 'mi');

            /* Test a bad COUNT argument */
            foreach (Array(-1, 0, 'notanumber') as $count) {
                $this->assertFalse(@$this->redis->$cmd('{gk}', $city, 10, 'mi', Array('count' => $count)));
            }
        }

        /* Options */
        $opts = ['WITHCOORD', 'WITHDIST', 'WITHHASH'];
        $sortopts = ['', 'ASC', 'DESC'];
        $storeopts = ['', 'STORE', 'STOREDIST'];

        for ($i = 0; $i < count($opts); $i++) {
            $subopts = array_slice($opts, 0, $i);
            shuffle($subopts);

            $subargs = $args;
            foreach ($subopts as $opt) {
                $subargs[] = $opt;
            }

            /* Cannot mix STORE[DIST] with the WITH* arguments */
            $realstoreopts = count($subopts) == 0 ? $storeopts : [];

            $base_subargs = $subargs;
            $base_subopts = $subopts;

            foreach ($realstoreopts as $store_type) {

                for ($c = 0; $c < 3; $c++) {
                    $subargs = $base_subargs;
                    $subopts = $base_subopts;

                    /* Add a count if we're past first iteration */
                    if ($c > 0) {
                        $subopts['count'] = $c;
                        $subargs[] = 'count';
                        $subargs[] = $c;
                    }

                    /* Adding optional sort */
                    foreach ($sortopts as $sortopt) {
                        $realargs = $subargs;
                        $realopts = $subopts;

                        if ($sortopt) {
                            $realargs[] = $sortopt;
                            $realopts[] = $sortopt;
                        }

                        if ($store_type) {
                            $realopts[$store_type] = "{gk}-$store_type";
                            $realargs[] = $store_type;
                            $realargs[] = "{gk}-$store_type";
                        }

                        $ret1 = $this->rawCommandArray('{gk}', $realargs);
                        if ($cmd == 'georadius' || $cmd == 'georadius_ro') {
                            $ret2 = $this->redis->$cmd('{gk}', $lng, $lat, 500, 'mi', $realopts);
                        } else {
                            $ret2 = $this->redis->$cmd('{gk}', $city, 500, 'mi', $realopts);
                        }

                        $this->assertEquals($ret1, $ret2);
                    }
                }
            }
        }
    }

    public function testGeoRadius() {
        if (!$this->minVersionCheck("3.2.0")) {
            return $this->markTestSkipped();
        }

        $this->genericGeoRadiusTest('georadius');
        $this->genericGeoRadiusTest('georadius_ro');
    }

    public function testGeoRadiusByMember() {
        if (!$this->minVersionCheck("3.2.0")) {
            return $this->markTestSkipped();
        }

        $this->genericGeoRadiusTest('georadiusbymember');
        $this->genericGeoRadiusTest('georadiusbymember_ro');
    }

    public function testGeoPos() {
        if (!$this->minVersionCheck("3.2.0")) {
            return $this->markTestSkipped();
        }

        $this->addCities('gk');
        $this->assertEquals($this->redis->geopos('gk', 'Chico', 'Sacramento'), $this->rawCommandArray('gk', ['geopos', 'gk', 'Chico', 'Sacramento']));
        $this->assertEquals($this->redis->geopos('gk', 'Cupertino'), $this->rawCommandArray('gk', ['geopos', 'gk', 'Cupertino']));
    }

    public function testGeoHash() {
        if (!$this->minVersionCheck("3.2.0")) {
            return $this->markTestSkipped();
        }

        $this->addCities('gk');
        $this->assertEquals($this->redis->geohash('gk', 'Chico', 'Sacramento'), $this->rawCommandArray('gk', ['geohash', 'gk', 'Chico', 'Sacramento']));
        $this->assertEquals($this->redis->geohash('gk', 'Chico'), $this->rawCommandArray('gk', ['geohash', 'gk', 'Chico']));
    }

    public function testGeoDist() {
        if (!$this->minVersionCheck("3.2.0")) {
            return $this->markTestSkipped();
        }

        $this->addCities('gk');

        $r1 = $this->redis->geodist('gk', 'Chico', 'Cupertino');
        $r2 = $this->rawCommandArray('gk', ['geodist', 'gk', 'Chico', 'Cupertino']);
        $this->assertEquals(round($r1, 8), round($r2, 8));

        $r1 = $this->redis->geodist('gk', 'Chico', 'Cupertino', 'km');
        $r2 = $this->rawCommandArray('gk', ['geodist', 'gk', 'Chico', 'Cupertino', 'km']);
        $this->assertEquals(round($r1, 8), round($r2, 8));
    }

    /* Test a 'raw' command */
    public function testRawCommand() {
        $this->redis->set('mykey','some-value');
        $str_result = $this->redis->rawCommand('get', 'mykey');
        $this->assertEquals($str_result, 'some-value');

        $this->redis->del('mylist');
        $this->redis->rpush('mylist', 'A', 'B', 'C', 'D');
        $this->assertEquals($this->redis->lrange('mylist', 0, -1), ['A','B','C','D']);
    }

    /* STREAMS */

    protected function addStreamEntries($key, $count) {
        $ids = [];

        $this->redis->del($key);

        for ($i = 0; $i < $count; $i++) {
            $ids[] = $this->redis->xAdd($key, '*', ['field' => "value:$i"]);
        }

        return $ids;
    }

    protected function addStreamsAndGroups($arr_streams, $count, $arr_groups) {
        $ids = [];

        foreach ($arr_streams as $str_stream) {
            $ids[$str_stream] = $this->addStreamEntries($str_stream, $count);
            foreach ($arr_groups as $str_group => $str_id) {
                $this->redis->xGroup('CREATE', $str_stream, $str_group, $str_id);
            }
        }

        return $ids;
    }

    public function testXAdd() {
        if (!$this->minVersionCheck("5.0"))
            return $this->markTestSkipped();

        $this->redis->del('stream');
        for ($i = 0; $i < 5; $i++) {
            $id = $this->redis->xAdd("stream", '*', ['k1' => 'v1', 'k2' => 'v2']);
            $this->assertEquals($this->redis->xLen('stream'), $i+1);

            /* Redis should return <timestamp>-<sequence> */
            $bits = explode('-', $id);
            $this->assertEquals(count($bits), 2);
            $this->assertTrue(is_numeric($bits[0]));
            $this->assertTrue(is_numeric($bits[1]));
        }

        /* Test an absolute maximum length */
        for ($i = 0; $i < 100; $i++) {
            $this->redis->xAdd('stream', '*', ['k' => 'v'], 10);
        }
        $this->assertEquals($this->redis->xLen('stream'), 10);

        /* Not the greatest test but I'm unsure if approximate trimming is
         * totally deterministic, so just make sure we are able to add with
         * an approximate maxlen argument structure */
        $id = $this->redis->xAdd('stream', '*', ['k' => 'v'], 10, true);
        $this->assertEquals(count(explode('-', $id)), 2);

        /* Empty message should fail */
        $this->redis->xAdd('stream', '*', []);
    }

    protected function doXRangeTest($reverse) {
        $key = '{stream}';

        if ($reverse) {
            list($cmd,$a1,$a2) = ['xRevRange', '+', 0];
        } else {
            list($cmd,$a1,$a2) = ['xRange', 0, '+'];
        }

        $this->redis->del($key);
        for ($i = 0; $i < 3; $i++) {
            $msg = ['field' => "value:$i"];
            $id = $this->redis->xAdd($key, '*', $msg);
            $rows[$id] = $msg;
        }

        $messages = $this->redis->$cmd($key, $a1, $a2);
        $this->assertEquals(count($messages), 3);

        $i = $reverse ? 2 : 0;
        foreach ($messages as $seq => $v) {
            $this->assertEquals(count(explode('-', $seq)), 2);
            $this->assertEquals($v, ['field' => "value:$i"]);
            $i += $reverse ? -1 : 1;
        }

        /* Test COUNT option */
        for ($count = 1; $count <= 3; $count++) {
            $messages = $this->redis->$cmd($key, $a1, $a2, $count);
            $this->assertEquals(count($messages), $count);
        }
    }

    public function testXRange() {
        if (!$this->minVersionCheck("5.0"))
            return $this->markTestSkipped();

        foreach ([false, true] as $reverse) {
            foreach ($this->serializers as $serializer) {
                foreach ([NULL, 'prefix:'] as $prefix) {
                    $this->redis->setOption(Redis::OPT_PREFIX, $prefix);
                    $this->redis->setOption(Redis::OPT_SERIALIZER, $serializer);
                    $this->doXRangeTest($reverse);
                }
            }
        }
    }

    protected function testXLen() {
        if (!$this->minVersionCheck("5.0"))
            $this->markTestSkipped();

        $this->redis->del('{stream}');
        for ($i = 0; $i < 5; $i++) {
            $this->redis->xadd('{stream}', '*', ['foo' => 'bar']);
            $this->assertEquals($this->redis->xLen('{stream}'), $i+1);
        }
    }

    public function testXGroup() {
        if (!$this->minVersionCheck("5.0"))
            return $this->markTestSkipped();

        /* CREATE MKSTREAM */
        $str_key = 's:' . uniqid();
        $this->assertFalse($this->redis->xGroup('CREATE', $str_key, 'g0', 0));
        $this->assertTrue($this->redis->xGroup('CREATE', $str_key, 'g1', 0, true));

        /* XGROUP DESTROY */
        $this->assertEquals($this->redis->xGroup('DESTROY', $str_key, 'g1'), 1);

        /* Populate some entries in stream 's' */
        $this->addStreamEntries('s', 2);

        /* CREATE */
        $this->assertTrue($this->redis->xGroup('CREATE', 's', 'mygroup', '$'));
        $this->assertFalse($this->redis->xGroup('CREATE', 's', 'mygroup', 'BAD_ID'));

        /* BUSYGROUP */
        $this->redis->xGroup('CREATE', 's', 'mygroup', '$');
        $this->assertTrue(strpos($this->redis->getLastError(), 'BUSYGROUP') === 0);

        /* SETID */
        $this->assertTrue($this->redis->xGroup('SETID', 's', 'mygroup', '$'));
        $this->assertFalse($this->redis->xGroup('SETID', 's', 'mygroup', 'BAD_ID'));

        $this->assertEquals($this->redis->xGroup('DELCONSUMER', 's', 'mygroup', 'myconsumer'),0);
    }

    public function testXAck() {
        if (!$this->minVersionCheck("5.0"))
            return $this->markTestSkipped();

        for ($n = 1; $n <= 3; $n++) {
            $this->addStreamsAndGroups(['{s}'], 3, ['g1' => 0]);
            $msg = $this->redis->xReadGroup('g1', 'c1', ['{s}' => '>']);

            /* Extract IDs */
            $smsg = array_shift($msg);
            $ids = array_keys($smsg);

            /* Now ACK $n messages */
            $ids = array_slice($ids, 0, $n);
            $this->assertEquals($this->redis->xAck('{s}', 'g1', $ids), $n);
        }

        /* Verify sending no IDs is a failure */
        $this->assertFalse($this->redis->xAck('{s}', 'g1', []));
    }

    protected function doXReadTest() {
        if (!$this->minVersionCheck("5.0"))
            return $this->markTestSkipped();

        $row = ['f1' => 'v1', 'f2' => 'v2'];
        $msgdata = [
            '{stream}-1' => $row,
            '{stream}-2' => $row,
        ];

        /* Append a bit of data and populate STREAM queries */
        $this->redis->del(array_keys($msgdata));
        foreach ($msgdata as $key => $message) {
            for ($r = 0; $r < 2; $r++) {
                $id = $this->redis->xAdd($key, '*', $message);
                $qresult[$key][$id] = $message;
            }
            $qzero[$key] = 0;
            $qnew[$key] = '$';
            $keys[] = $key;
        }

        /* Everything from both streams */
        $rmsg = $this->redis->xRead($qzero);
        $this->assertEquals($rmsg, $qresult);

        /* Test COUNT option */
        for ($count = 1; $count <= 2; $count++) {
            $rmsg = $this->redis->xRead($qzero, $count);
            foreach ($keys as $key) {
                $this->assertEquals(count($rmsg[$key]), $count);
            }
        }

        /* Should be empty (no new entries) */
        $this->assertEquals(count($this->redis->xRead($qnew)),0);

        /* Test against a specific ID */
        $id = $this->redis->xAdd('{stream}-1', '*', $row);
        $new_id = $this->redis->xAdd('{stream}-1', '*', ['final' => 'row']);
        $rmsg = $this->redis->xRead(['{stream}-1' => $id]);
        $this->assertEquals(
            $this->redis->xRead(['{stream}-1' => $id]),
            ['{stream}-1' => [$new_id => ['final' => 'row']]]
        );

        /* Emtpy query should fail */
        $this->assertFalse($this->redis->xRead([]));
    }

    public function testXRead() {
        if (!$this->minVersionCheck("5.0"))
            return $this->markTestSkipped();

        foreach ($this->serializers as $serializer) {
            $this->redis->setOption(Redis::OPT_SERIALIZER, $serializer);
            $this->doXReadTest();
        }

        /* Don't need to test BLOCK multiple times */
        $m1 = round(microtime(true)*1000);
        $this->redis->xRead(['somestream' => '$'], -1, 100);
        $m2 = round(microtime(true)*1000);
        $this->assertTrue($m2 - $m1 >= 100);
    }

    protected function compareStreamIds($redis, $control) {
        foreach ($control as $stream => $ids) {
            $rcount = count($redis[$stream]);
            $lcount = count($control[$stream]);

            /* We should have the same number of messages */
            $this->assertEquals($rcount, $lcount);

            /* We should have the exact same IDs */
            foreach ($ids as $k => $id) {
                $this->assertTrue(isset($redis[$stream][$id]));
            }
        }
    }

    public function testXReadGroup() {
        if (!$this->minVersionCheck("5.0"))
            return $this->markTestSkipped();

        /* Create some streams and groups */
        $streams = ['{s}-1', '{s}-2'];
        $groups = ['g1' => 0, 'g2' => 0];

        /* I'm not totally sure why Redis behaves this way, but we have to
         * send '>' first and then send ID '0' for subsequent xReadGroup calls
         * or Redis will not return any messages.  This behavior changed from
         * redis 5.0.1 and 5.0.2 but doing it this way works for both versions. */
        $qcount = 0;
        $query1 = ['{s}-1' => '>', '{s}-2' => '>'];
        $query2 = ['{s}-1' => '0', '{s}-2' => '0'];

        $ids = $this->addStreamsAndGroups($streams, 1, $groups);

        /* Test that we get get the IDs we should */
        foreach (['g1', 'g2'] as $group) {
            foreach ($ids as $stream => $messages) {
                while ($ids[$stream]) {
                    /* Read more messages */
                    $query = !$qcount++ ? $query1 : $query2;
                    $resp = $this->redis->xReadGroup($group, 'consumer', $query);

                    /* They should match with our local control array */
                    $this->compareStreamIds($resp, $ids);

                    /* Remove a message from our control *and* XACK it in Redis */
                    $id = array_shift($ids[$stream]);
                    $this->redis->xAck($stream, $group, [$id]);
                }
            }
        }

        /* Test COUNT option */
        for ($c = 1; $c <= 3; $c++) {
            $this->addStreamsAndGroups($streams, 3, $groups);
            $resp = $this->redis->xReadGroup('g1', 'consumer', $query1, $c);

            foreach ($resp as $stream => $smsg) {
                $this->assertEquals(count($smsg), $c);
            }
        }

        /* Test COUNT option with NULL (should be ignored) */
        $this->addStreamsAndGroups($streams, 3, $groups, NULL);
        $resp = $this->redis->xReadGroup('g1', 'consumer', $query1, NULL);
        foreach ($resp as $stream => $smsg) {
            $this->assertEquals(count($smsg), 3);
        }

        /* Finally test BLOCK with a sloppy timing test */
        $t1 = $this->mstime();
        $qnew = ['{s}-1' => '>', '{s}-2' => '>'];
        $this->redis->xReadGroup('g1', 'c1', $qnew, NULL, 100);
        $t2 = $this->mstime();
        $this->assertTrue($t2 - $t1 >= 100);

        /* Make sure passing NULL to block doesn't block */
        $t1 = $this->mstime();
        $this->redis->xReadGroup('g1', 'c1', $qnew, NULL, NULL);
        $t2 = $this->mstime();
        $this->assertTrue($t2 - $t1 < 100);

        /* Make sure passing bad values to BLOCK or COUNT immediately fails */
        $this->assertFalse(@$this->redis->xReadGroup('g1', 'c1', $qnew, -1));
        $this->assertFalse(@$this->redis->xReadGroup('g1', 'c1', $qnew, NULL, -1));
    }

    public function testXPending() {
        if (!$this->minVersionCheck("5.0")) {
            return $this->markTestSkipped();
        }

        $rows = 5;
        $this->addStreamsAndGroups(['s'], $rows, ['group' => 0]);

        $msg = $this->redis->xReadGroup('group', 'consumer', ['s' => 0]);
        $ids = array_keys($msg['s']);

        for ($n = count($ids); $n >= 0; $n--) {
            $xp = $this->redis->xPending('s', 'group');
            $this->assertEquals($xp[0], count($ids));

            /* Verify we're seeing the IDs themselves */
            for ($idx = 1; $idx <= 2; $idx++) {
                if ($xp[$idx]) {
                    $this->assertPatternMatch($xp[$idx], "/^[0-9].*-[0-9].*/");
                }
            }

            if ($ids) {
                $id = array_shift($ids);
                $this->redis->xAck('s', 'group', [$id]);
            }
        }
    }

    public function testXDel() {
        if (!$this->minVersionCheck("5.0"))
            return $this->markTestSkipped();

        for ($n = 5; $n > 0; $n--) {
            $ids = $this->addStreamEntries('s', 5);
            $todel = array_slice($ids, 0, $n);
            $this->assertEquals($this->redis->xDel('s', $todel), count($todel));
        }

        /* Empty array should fail */
        $this->assertFalse($this->redis->xDel('s', []));
    }

    public function testXTrim() {
        if (!$this->minVersionCheck("5.0"))
            return $this->markTestSkipped();

        for ($maxlen = 0; $maxlen <= 50; $maxlen += 10) {
            $this->addStreamEntries('stream', 100);
            $trimmed = $this->redis->xTrim('stream', $maxlen);
            $this->assertEquals($trimmed, 100 - $maxlen);
        }

        /* APPROX trimming isn't easily deterministic, so just make sure we
           can call it with the flag */
        $this->addStreamEntries('stream', 100);
        $this->assertFalse($this->redis->xTrim('stream', 1, true) === false);
    }

    /* XCLAIM is one of the most complicated commands, with a great deal of different options
     * The following test attempts to verify every combination of every possible option. */
    public function testXClaim() {
        if (!$this->minVersionCheck("5.0"))
            return $this->markTestSkipped();

        foreach ([0, 100] as $min_idle_time) {
            foreach ([false, true] as $justid) {
                foreach ([0, 10] as $retrycount) {
                    /* We need to test not passing TIME/IDLE as well as passing either */
                    if ($min_idle_time == 0) {
                        $topts = [[], ['IDLE', 1000000], ['TIME', time() * 1000]];
                    } else {
                        $topts = [NULL];
                    }

                    foreach ($topts as $tinfo) {
                        if ($tinfo) {
                            list($ttype, $tvalue) = $tinfo;
                        } else {
                            $ttype = NULL; $tvalue = NULL;
                        }

                        /* Add some messages and create a group */
                        $this->addStreamsAndGroups(['s'], 10, ['group1' => 0]);

                        /* Create a second stream we can FORCE ownership on */
                        $fids = $this->addStreamsAndGroups(['f'], 10, ['group1' => 0]);
                        $fids = $fids['f'];

                        /* Have consumer 'Mike' read the messages */
                        $oids = $this->redis->xReadGroup('group1', 'Mike', ['s' => '>']);
                        $oids = array_keys($oids['s']); /* We're only dealing with stream 's' */

                        /* Construct our options array */
                        $opts = [];
                        if ($justid) $opts[] = 'JUSTID';
                        if ($retrycount) $opts['RETRYCOUNT'] = $retrycount;
                        if ($tvalue !== NULL) $opts[$ttype] = $tvalue;

                        /* Now have pavlo XCLAIM them */
                        $cids = $this->redis->xClaim('s', 'group1', 'Pavlo', $min_idle_time, $oids, $opts);
                        if (!$justid) $cids = array_keys($cids);

                        if ($min_idle_time == 0) {
                            $this->assertEquals($cids, $oids);

                            /* Append the FORCE option to our second stream where we have not already
                             * assigned to a PEL group */
                            $opts[] = 'FORCE';
                            $freturn = $this->redis->xClaim('f', 'group1', 'Test', 0, $fids, $opts);
                            if (!$justid) $freturn = array_keys($freturn);
                            $this->assertEquals($freturn, $fids);

                            if ($retrycount || $tvalue !== NULL) {
                                $pending = $this->redis->xPending('s', 'group1', 0, '+', 1, 'Pavlo');

                                if ($retrycount) {
                                    $this->assertEquals($pending[0][3], $retrycount);
                                }
                                if ($tvalue !== NULL) {
                                    if ($ttype == 'IDLE') {
                                        /* If testing IDLE the value must be >= what we set */
                                        $this->assertTrue($pending[0][2] >= $tvalue);
                                    } else {
                                        /* Timing tests are notoriously irritating but I don't see
                                         * how we'll get >= 20,000 ms between XCLAIM and XPENDING no
                                         * matter how slow the machine/VM running the tests is */
                                        $this->assertTrue($pending[0][2] <= 20000);
                                    }
                                }
                            }
                        } else {
                            /* We're verifying that we get no messages when we've set 100 seconds
                             * as our idle time, which should match nothing */
                            $this->assertEquals($cids, []);
                        }
                    }
                }
            }
        }
    }

    public function testXInfo()
    {
        if (!$this->minVersionCheck("5.0")) {
            return $this->markTestSkipped();
        }
        /* Create some streams and groups */
        $stream = 's';
        $groups = ['g1' => 0, 'g2' => 0];
        $this->addStreamsAndGroups([$stream], 1, $groups);

        $info = $this->redis->xInfo('GROUPS', $stream);
        $this->assertTrue(is_array($info));
        $this->assertEquals(count($info), count($groups));
        foreach ($info as $group) {
            $this->assertTrue(array_key_exists('name', $group));
            $this->assertTrue(array_key_exists($group['name'], $groups));
        }

        $info = $this->redis->xInfo('STREAM', $stream);
        $this->assertTrue(is_array($info));
        $this->assertTrue(array_key_exists('groups', $info));
        $this->assertEquals($info['groups'], count($groups));
        foreach (['first-entry', 'last-entry'] as $key) {
            $this->assertTrue(array_key_exists($key, $info));
            $this->assertTrue(is_array($info[$key]));
        }

        /* XINFO STREAM FULL [COUNT N] Requires >= 6.0.0 */
        if (!$this->minVersionCheck("6.0"))
            return;

        /* Add some items to the stream so we can test COUNT */
        for ($i = 0; $i < 5; $i++) {
            $this->redis->xAdd($stream, '*', ['foo' => 'bar']);
        }

        $info = $this->redis->xInfo('STREAM', $stream, 'full');
        $this->assertTrue(isset($info['groups']));

        for ($count = 1; $count < 5; $count++) {
            $info = $this->redis->xInfo('STREAM', $stream, 'full', $count);
            $n = isset($info['entries']) ? count($info['entries']) : 0;
            $this->assertEquals($n, $count);
        }

        /* Count <= 0 should be ignored */
        foreach ([-1, 0] as $count) {
            $info = $this->redis->xInfo('STREAM', $stream, 'full', 0);
            $n = isset($info['entries']) ? count($info['entries']) : 0;
            $this->assertEquals($n, $this->redis->xLen($stream));
        }
    }

    /* Regression test for issue-1831 (XINFO STREAM on an empty stream) */
    public function testXInfoEmptyStream() {
        /* Configure an empty stream */
        $this->redis->del('s');
        $this->redis->xAdd('s', '*', ['foo' => 'bar']);
        $this->redis->xTrim('s', 0);

        $arr_info = $this->redis->xInfo('STREAM', 's');

        $this->assertTrue(is_array($arr_info));
        $this->assertEquals(0, $arr_info['length']);
        $this->assertEquals(NULL, $arr_info['first-entry']);
        $this->assertEquals(NULL, $arr_info['last-entry']);
    }

    public function testInvalidAuthArgs() {
        $obj_new = $this->newInstance();

        $arr_args = [
            [],
            [NULL, NULL],
            ['foo', 'bar', 'baz'],
            ['a','b','c','d'],
            ['a','b','c'],
            [['a','b'], 'a'],
            [['a','b','c']],
            [[NULL, 'pass']],
            [[NULL, NULL]],
        ];

        foreach ($arr_args as $arr_arg) {
            try {
                if (is_array($arr_arg)) {
                    @call_user_func_array([$obj_new, 'auth'], $arr_arg);
                }
            } catch (Exception $ex) {
                unset($ex); /* Suppress intellisense warning */
            } catch (ArgumentCountError $ex) {
                unset($ex); /* Suppress intellisense warning */
            }
        }
    }

    public function testAcl() {
        if ( ! $this->minVersionCheck("6.0"))
            return $this->markTestSkipped();

        /* ACL USERS/SETUSER */
        $this->assertTrue(in_array('default', $this->redis->acl('USERS')));
        $this->assertTrue($this->redis->acl('SETUSER', 'admin', 'on', '>admin', '+@all'));
        $this->assertTrue($this->redis->acl('SETUSER', 'noperm', 'on', '>noperm', '-@all'));
        $this->assertInArray('default', $this->redis->acl('USERS'));

        /* Verify ACL GETUSER has the correct hash and is in 'nice' format */
        $arr_admin = $this->redis->acl('GETUSER', 'admin');
        $this->assertInArray(hash('sha256', 'admin'), $arr_admin['passwords']);

        /* Now nuke our 'admin' user and make sure it went away */
        $this->assertTrue($this->redis->acl('DELUSER', 'admin'));
        $this->assertTrue(!in_array('admin', $this->redis->acl('USERS')));

        /* Try to log in with a bad username/password */
        $this->assertThrowsMatch($this->redis,
            function($o) { $o->auth(['1337haxx00r', 'lolwut']); }, '/^WRONGPASS.*$/');

        /* We attempted a bad login.  We should have an ACL log entry */
        $arr_log = $this->redis->acl('log');
        if (! $arr_log || !is_array($arr_log)) {
            $this->assertTrue(false);
            return;
        }

        /* Make sure our ACL LOG entries are nice for the user */
        $arr_entry = array_shift($arr_log);
        $this->assertArrayKey($arr_entry, 'age-seconds', 'is_numeric');
        $this->assertArrayKey($arr_entry, 'count', 'is_int');

        /* ACL CAT */
        $cats = $this->redis->acl('CAT');
        foreach (['read', 'write', 'slow'] as $cat) {
            $this->assertInArray($cat, $cats);
        }

        /* ACL CAT <string> */
        $cats = $this->redis->acl('CAT', 'string');
        foreach (['get', 'set', 'setnx'] as $cat) {
            $this->assertInArray($cat, $cats);
        }

        /* ctype_xdigit even if PHP doesn't have it */
        $ctype_xdigit = function($v) {
            if (function_exists('ctype_xdigit')) {
                return ctype_xdigit($v);
            } else {
                return strspn(strtoupper($v), '0123456789ABCDEF') == strlen($v);
            }
        };

        /* ACL GENPASS/ACL GENPASS <bits> */
        $this->assertValidate($this->redis->acl('GENPASS'), $ctype_xdigit);
        $this->assertValidate($this->redis->acl('GENPASS', 1024), $ctype_xdigit);

        /* ACL WHOAMI */
        $this->assertValidate($this->redis->acl('WHOAMI'), 'strlen');

        /* Finally make sure AUTH errors throw an exception */
        $r2 = $this->newInstance(true);

        /* Test NOPERM exception */
        $this->assertTrue($r2->auth(['noperm', 'noperm']));
        $this->assertThrowsMatch($r2, function($r) { $r->set('foo', 'bar'); }, '/^NOPERM.*$/');
    }

    /* If we detect a unix socket make sure we can connect to it in a variety of ways */
    public function testUnixSocket() {
        if ( ! file_exists("/tmp/redis.sock")) {
            return $this->markTestSkipped();
        }

        $arr_sock_tests = [
            ["/tmp/redis.sock"],
            ["/tmp/redis.sock", null],
            ["/tmp/redis.sock", 0],
            ["/tmp/redis.sock", -1],
        ];

        try {
            foreach ($arr_sock_tests as $arr_args) {
                $obj_r = new Redis();

                if (count($arr_args) == 2) {
                    @$obj_r->connect($arr_args[0], $arr_args[1]);
                } else {
                    @$obj_r->connect($arr_args[0]);
                }
                if ($this->getAuth()) {
                    $this->assertTrue($obj_r->auth($this->getAuth()));
                }
                $this->assertTrue($obj_r->ping());
            }
        } catch (Exception $ex) {
            $this->assertTrue(false);
        }
    }

    /* Test high ports if we detect Redis running there */
    public function testHighPorts() {
        $arr_ports = [32767, 32768, 32769];
        $arr_test_ports = [];

        foreach ($arr_ports as $port) {
            if (is_resource(@fsockopen('localhost', $port))) {
                $arr_test_ports[] = $port;
            }
        }

        if ( ! $arr_test_ports) {
            return $this->markTestSkipped();
        }

        foreach ($arr_test_ports as $port) {
            $obj_r = new Redis();
            try {
                @$obj_r->connect('localhost', $port);
                if ($this->getAuth()) {
                    $this->assertTrue($obj_r->auth($this->getAuth()));
                }
                $this->assertTrue($obj_r->ping());
            } catch(Exception $ex) {
                $this->assertTrue(false);
            }
        }
    }

    public function testSession_savedToRedis()
    {
        $this->setSessionHandler();

        $sessionId = $this->generateSessionId();
        $sessionSuccessful = $this->startSessionProcess($sessionId, 0, false);

        $this->assertTrue($this->redis->exists($this->sessionPrefix . $sessionId));
        $this->assertTrue($sessionSuccessful);
    }

    public function testSession_lockKeyCorrect()
    {
        $this->setSessionHandler();
        $sessionId = $this->generateSessionId();

        $this->startSessionProcess($sessionId, 5, true);

        $maxwait = (ini_get('redis.session.lock_wait_time') *
                    ini_get('redis.session.lock_retries') /
                    1000000.00);

        $exist = $this->waitForSessionLockKey($sessionId, $maxwait);
        $this->assertTrue($exist);
    }

    public function testSession_lockingDisabledByDefault()
    {
        $this->setSessionHandler();
        $sessionId = $this->generateSessionId();
        $this->startSessionProcess($sessionId, 5, true, 300, false);
        usleep(100000);

        $start = microtime(true);
        $sessionSuccessful = $sessionSuccessful = $this->startSessionProcess($sessionId, 0, false, 300, false);
        $end = microtime(true);
        $elapsedTime = $end - $start;

        $this->assertFalse($this->redis->exists($this->sessionPrefix . $sessionId . '_LOCK'));
        $this->assertTrue($elapsedTime < 1);
        $this->assertTrue($sessionSuccessful);
    }

    public function testSession_lockReleasedOnClose()
    {
        $this->setSessionHandler();
        $sessionId = $this->generateSessionId();
        $this->startSessionProcess($sessionId, 1, true);
        $sleep = ini_get('redis.session.lock_wait_time') * ini_get('redis.session.lock_retries');
        usleep($sleep + 10000);
        $this->assertFalse($this->redis->exists($this->sessionPrefix . $sessionId . '_LOCK'));
    }

    public function testSession_lock_ttlMaxExecutionTime()
    {
        $this->setSessionHandler();
        $sessionId = $this->generateSessionId();
        $this->startSessionProcess($sessionId, 10, true, 2);
        usleep(100000);

        $start = microtime(true);
        $sessionSuccessful = $this->startSessionProcess($sessionId, 0, false);
        $end = microtime(true);
        $elapsedTime = $end - $start;

        $this->assertTrue($elapsedTime < 3);
        $this->assertTrue($sessionSuccessful);
    }

    public function testSession_lock_ttlLockExpire()
    {
        $this->setSessionHandler();
        $sessionId = $this->generateSessionId();
        $this->startSessionProcess($sessionId, 10, true, 300, true, null, -1, 2);
        usleep(100000);

        $start = microtime(true);
        $sessionSuccessful = $this->startSessionProcess($sessionId, 0, false);
        $end = microtime(true);
        $elapsedTime = $end - $start;

        $this->assertTrue($elapsedTime < 3);
        $this->assertTrue($sessionSuccessful);
    }

    public function testSession_lockHoldCheckBeforeWrite_otherProcessHasLock()
    {
        $this->setSessionHandler();
        $sessionId = $this->generateSessionId();
        $this->startSessionProcess($sessionId, 2, true, 300, true, null, -1, 1, 'firstProcess');
        usleep(1500000); // 1.5 sec
        $writeSuccessful = $this->startSessionProcess($sessionId, 0, false, 300, true, null, -1, 10, 'secondProcess');
        sleep(1);

        $this->assertTrue($writeSuccessful);
        $this->assertEquals('secondProcess', $this->getSessionData($sessionId));
    }

    public function testSession_lockHoldCheckBeforeWrite_nobodyHasLock()
    {
        $this->setSessionHandler();
        $sessionId = $this->generateSessionId();
        $writeSuccessful = $this->startSessionProcess($sessionId, 2, false, 300, true, null, -1, 1, 'firstProcess');

        $this->assertFalse($writeSuccessful);
        $this->assertTrue('firstProcess' !== $this->getSessionData($sessionId));
    }

    public function testSession_correctLockRetryCount() {
        $this->setSessionHandler();
        $sessionId = $this->generateSessionId();

        /* Start another process and wait until it has the lock */
        $this->startSessionProcess($sessionId, 10, true);
        if ( ! $this->waitForSessionLockKey($sessionId, 2)) {
            $this->assertTrue(false);
            return;
        }

        $t1 = microtime(true);
        $ok = $this->startSessionProcess($sessionId, 0, false, 10, true, 100000, 10);
        if ( ! $this->assertFalse($ok)) return;
        $t2 = microtime(true);

        $this->assertTrue($t2 - $t1 >= 1 && $t2 - $t1 <= 3);
    }

    public function testSession_defaultLockRetryCount()
    {
        $this->setSessionHandler();
        $sessionId = $this->generateSessionId();
        $this->startSessionProcess($sessionId, 10, true);

        $keyname = $this->sessionPrefix . $sessionId . '_LOCK';
        $begin = microtime(true);

        if ( ! $this->waitForSessionLockKey($sessionId, 3)) {
            $this->assertTrue(false);
            return;
        }

        $start = microtime(true);
        $sessionSuccessful = $this->startSessionProcess($sessionId, 0, false, 10, true, 20000, 0);
        $end = microtime(true);
        $elapsedTime = $end - $start;

        $this->assertTrue($elapsedTime > 2 && $elapsedTime < 3);
        $this->assertFalse($sessionSuccessful);
    }

    public function testSession_noUnlockOfOtherProcess()
    {
        $this->setSessionHandler();
        $sessionId = $this->generateSessionId();

        $t1 = microtime(true);

        /* 1.  Start a background process, and wait until we are certain
         *     the lock was attained. */
        $nsec = 3;
        $this->startSessionProcess($sessionId, $nsec, true, $nsec);
        if ( ! $this->waitForSessionLockKey($sessionId, 1)) {
            $this->assertFalse(true);
            return;
        }

        /* 2.  Attempt to lock the same session.  This should force us to
         *     wait until the first lock is released. */
        $t2 = microtime(true);
        $ok = $this->startSessionProcess($sessionId, 0, false);
        $t3 = microtime(true);

        /* 3.  Verify that we did in fact have to wait for this lock */
        $this->assertTrue($ok);
        $this->assertTrue($t3 - $t2 >= $nsec - ($t2 - $t1));
    }

    public function testSession_lockWaitTime()
    {
        $this->setSessionHandler();
        $sessionId = $this->generateSessionId();
        $this->startSessionProcess($sessionId, 1, true, 300);
        usleep(100000);

        $start = microtime(true);
        $sessionSuccessful = $this->startSessionProcess($sessionId, 0, false, 300, true, 3000000);
        $end = microtime(true);
        $elapsedTime = $end - $start;

        $this->assertTrue($elapsedTime > 2.5);
        $this->assertTrue($elapsedTime < 3.5);
        $this->assertTrue($sessionSuccessful);
    }

    public function testMultipleConnect() {
        $host = $this->redis->GetHost();
        $port = $this->redis->GetPort();

        for($i = 0; $i < 5; $i++) {
            $this->redis->connect($host, $port);
            if ($this->getAuth()) {
                $this->assertTrue($this->redis->auth($this->getAuth()));
            }
            $this->assertTrue($this->redis->ping());
        }
    }

    public function testConnectException() {
        $host = 'github.com';
        if (gethostbyname($host) === $host) {
            return $this->markTestSkipped('online test');
        }
        $redis = new Redis();
        try {
            $redis->connect($host, 6379, 0.01);
        }  catch (Exception $e) {
            $this->assertTrue(strpos($e, "timed out") !== false);
        }
    }

    public function testTlsConnect()
    {
        if (($fp = @fsockopen($this->getHost(), 6378)) == NULL)
            return $this->markTestSkipped();

        fclose($fp);

        foreach (['localhost' => true, '127.0.0.1' => false] as $host => $verify) {
            $redis = new Redis();
            $this->assertTrue($redis->connect('tls://' . $host, 6378, 0, null, 0, 0, [
                'stream' => ['verify_peer_name' => $verify, 'verify_peer' => false]
            ]));
        }
    }

    public function testCopy()
    {
        // Only available since 6.2.0
        if (version_compare($this->version, '6.2.0') < 0) {
            $this->markTestSkipped();
            return;
        }

        $this->redis->del('key2');
        $this->redis->set('key', 'foo');
        $this->assertTrue($this->redis->copy('key', 'key2'));
        $this->assertEquals('foo', $this->redis->get('key2'));

        $this->redis->set('key', 'bar');
        $this->assertFalse($this->redis->copy('key', 'key2'));
        $this->assertEquals('foo', $this->redis->get('key2'));

        $this->assertTrue($this->redis->copy('key', 'key2', ['replace' => true]));
        $this->assertEquals('bar', $this->redis->get('key2'));
    }

    public  function testSession_regenerateSessionId_noLock_noDestroy() {
        $this->setSessionHandler();
        $sessionId = $this->generateSessionId();
        $this->startSessionProcess($sessionId, 0, false, 300, true, null, -1, 1, 'bar');

        $newSessionId = $this->regenerateSessionId($sessionId);

        $this->assertTrue($newSessionId !== $sessionId);
        $this->assertEquals('bar', $this->getSessionData($newSessionId));
    }

    public  function testSession_regenerateSessionId_noLock_withDestroy() {
        $this->setSessionHandler();
        $sessionId = $this->generateSessionId();
        $this->startSessionProcess($sessionId, 0, false, 300, true, null, -1, 1, 'bar');

        $newSessionId = $this->regenerateSessionId($sessionId, false, true);

        $this->assertTrue($newSessionId !== $sessionId);
        $this->assertEquals('bar', $this->getSessionData($newSessionId));
    }

    public  function testSession_regenerateSessionId_withLock_noDestroy() {
        $this->setSessionHandler();
        $sessionId = $this->generateSessionId();
        $this->startSessionProcess($sessionId, 0, false, 300, true, null, -1, 1, 'bar');

        $newSessionId = $this->regenerateSessionId($sessionId, true);

        $this->assertTrue($newSessionId !== $sessionId);
        $this->assertEquals('bar', $this->getSessionData($newSessionId));
    }

    public  function testSession_regenerateSessionId_withLock_withDestroy() {
        $this->setSessionHandler();
        $sessionId = $this->generateSessionId();
        $this->startSessionProcess($sessionId, 0, false, 300, true, null, -1, 1, 'bar');

        $newSessionId = $this->regenerateSessionId($sessionId, true, true);

        $this->assertTrue($newSessionId !== $sessionId);
        $this->assertEquals('bar', $this->getSessionData($newSessionId));
    }

    public  function testSession_regenerateSessionId_noLock_noDestroy_withProxy() {
        if (!interface_exists('SessionHandlerInterface')) {
            $this->markTestSkipped('session handler interface not available in PHP < 5.4');
        }

        $this->setSessionHandler();
        $sessionId = $this->generateSessionId();
        $this->startSessionProcess($sessionId, 0, false, 300, true, null, -1, 1, 'bar');

        $newSessionId = $this->regenerateSessionId($sessionId, false, false, true);

        $this->assertTrue($newSessionId !== $sessionId);
        $this->assertEquals('bar', $this->getSessionData($newSessionId));
    }

    public  function testSession_regenerateSessionId_noLock_withDestroy_withProxy() {
        if (!interface_exists('SessionHandlerInterface')) {
            $this->markTestSkipped('session handler interface not available in PHP < 5.4');
        }

        $this->setSessionHandler();
        $sessionId = $this->generateSessionId();
        $this->startSessionProcess($sessionId, 0, false, 300, true, null, -1, 1, 'bar');

        $newSessionId = $this->regenerateSessionId($sessionId, false, true, true);

        $this->assertTrue($newSessionId !== $sessionId);
        $this->assertEquals('bar', $this->getSessionData($newSessionId));
    }

    public  function testSession_regenerateSessionId_withLock_noDestroy_withProxy() {
        if (!interface_exists('SessionHandlerInterface')) {
            $this->markTestSkipped('session handler interface not available in PHP < 5.4');
        }

        $this->setSessionHandler();
        $sessionId = $this->generateSessionId();
        $this->startSessionProcess($sessionId, 0, false, 300, true, null, -1, 1, 'bar');

        $newSessionId = $this->regenerateSessionId($sessionId, true, false, true);

        $this->assertTrue($newSessionId !== $sessionId);
        $this->assertEquals('bar', $this->getSessionData($newSessionId));
    }

    public  function testSession_regenerateSessionId_withLock_withDestroy_withProxy() {
        if (!interface_exists('SessionHandlerInterface')) {
            $this->markTestSkipped('session handler interface not available in PHP < 5.4');
        }

        $this->setSessionHandler();
        $sessionId = $this->generateSessionId();
        $this->startSessionProcess($sessionId, 0, false, 300, true, null, -1, 1, 'bar');

        $newSessionId = $this->regenerateSessionId($sessionId, true, true, true);

        $this->assertTrue($newSessionId !== $sessionId);
        $this->assertEquals('bar', $this->getSessionData($newSessionId));
    }

    public function testSession_ttl_equalsToSessionLifetime()
    {
        $sessionId = $this->generateSessionId();
        $this->startSessionProcess($sessionId, 0, false, 300, true, null, -1, 0, 'test', 600);
        $ttl = $this->redis->ttl($this->sessionPrefix . $sessionId);

        $this->assertEquals(600, $ttl);
    }

    public function testSession_ttl_resetOnWrite()
    {
        $sessionId = $this->generateSessionId();
        $this->startSessionProcess($sessionId, 0, false, 300, true, null, -1, 0, 'test', 600);
        $this->redis->expire($this->sessionPrefix . $sessionId, 9999);
        $this->startSessionProcess($sessionId, 0, false, 300, true, null, -1, 0, 'test', 600);
        $ttl = $this->redis->ttl($this->sessionPrefix . $sessionId);

        $this->assertEquals(600, $ttl);
    }

    public function testSession_ttl_resetOnRead()
    {
        $sessionId = $this->generateSessionId();
        $this->startSessionProcess($sessionId, 0, false, 300, true, null, -1, 0, 'test', 600);
        $this->redis->expire($this->sessionPrefix . $sessionId, 9999);
        $this->getSessionData($sessionId, 600);
        $ttl = $this->redis->ttl($this->sessionPrefix . $sessionId);

        $this->assertEquals(600, $ttl);
    }

    private function setSessionHandler()
    {
        $host = $this->getHost() ?: 'localhost';

        @ini_set('session.save_handler', 'redis');
        @ini_set('session.save_path', 'tcp://' . $host . ':6379');
    }

    /**
     * @return string
     */
    private function generateSessionId()
    {
        if (function_exists('session_create_id')) {
            return session_create_id();
        } else if (function_exists('random_bytes')) {
            return bin2hex(random_bytes(8));
        } else if (function_exists('openssl_random_pseudo_bytes')) {
            return bin2hex(openssl_random_pseudo_bytes(8));
        } else {
            return uniqid();
        }
    }

    /**
     * @param string $sessionId
     * @param int    $sleepTime
     * @param bool   $background
     * @param int    $maxExecutionTime
     * @param bool   $locking_enabled
     * @param int    $lock_wait_time
     * @param int    $lock_retries
     * @param int    $lock_expires
     * @param string $sessionData
     *
     * @param int    $sessionLifetime
     *
     * @return bool
     * @throws Exception
     */
    private function startSessionProcess($sessionId, $sleepTime, $background, $maxExecutionTime = 300, $locking_enabled = true, $lock_wait_time = null, $lock_retries = -1, $lock_expires = 0, $sessionData = '', $sessionLifetime = 1440)
    {
        if (substr(php_uname(), 0, 7) == "Windows"){
            $this->markTestSkipped();
            return true;
        } else {
            $commandParameters = [$this->getFullHostPath(), $this->sessionSaveHandler, $sessionId, $sleepTime, $maxExecutionTime, $lock_retries, $lock_expires, $sessionData, $sessionLifetime];
            if ($locking_enabled) {
                $commandParameters[] = '1';

                if ($lock_wait_time != null) {
                    $commandParameters[] = $lock_wait_time;
                }
            }
            $commandParameters = array_map('escapeshellarg', $commandParameters);

            $command = self::getPhpCommand('startSession.php') . implode(' ', $commandParameters);
            $command .= $background ? ' 2>/dev/null > /dev/null &' : ' 2>&1';
            exec($command, $output);
            return ($background || (count($output) == 1 && $output[0] == 'SUCCESS')) ? true : false;
        }
    }

    /**
     * @param string $session_id
     * @param string $max_wait_sec
     *
     * Sometimes we want to block until a session lock has been detected
     * This is better and faster than arbitrarily sleeping.  If we don't
     * detect the session key within the specified maximum number of
     * seconds, the function returns failure.
     *
     * @return bool
     */
    private function waitForSessionLockKey($session_id, $max_wait_sec) {
        $now = microtime(true);
        $key = $this->sessionPrefix . $session_id . '_LOCK';

        do {
            usleep(10000);
            $exists = $this->redis->exists($key);
        } while (!$exists && microtime(true) <= $now + $max_wait_sec);

        return $exists || $this->redis->exists($key);
    }


    /**
     * @param string $sessionId
     * @param int    $sessionLifetime
     *
     * @return string
     */
    private function getSessionData($sessionId, $sessionLifetime = 1440)
    {
        $command = self::getPhpCommand('getSessionData.php') . escapeshellarg($this->getFullHostPath()) . ' ' . $this->sessionSaveHandler . ' ' . escapeshellarg($sessionId) . ' ' . escapeshellarg($sessionLifetime);
        exec($command, $output);

        return $output[0];
    }

    /**
     * @param string $sessionId
     * @param bool   $locking
     * @param bool   $destroyPrevious
     * @param bool   $sessionProxy
     *
     * @return string
     */
    private function regenerateSessionId($sessionId, $locking = false, $destroyPrevious = false, $sessionProxy = false)
    {
	$args = array_map('escapeshellarg', [$sessionId, $locking, $destroyPrevious, $sessionProxy]);

        $command = self::getPhpCommand('regenerateSessionId.php') . escapeshellarg($this->getFullHostPath()) . ' ' . $this->sessionSaveHandler . ' ' . implode(' ', $args);

        exec($command, $output);

        return $output[0];
    }

    /**
     * Return command to launch PHP with built extension enabled
     * taking care of environment (TEST_PHP_EXECUTABLE and TEST_PHP_ARGS)
     *
     * @param string $script
     *
     * @return string
     */
    private function getPhpCommand($script)
    {
        static $cmd = NULL;

        if (!$cmd) {
            $cmd  = (getenv('TEST_PHP_EXECUTABLE') ?: PHP_BINARY);

            if ($test_args = getenv('TEST_PHP_ARGS')) {
                $cmd .= ' ';
                $cmd .= $test_args;
            } else {
                /* Only append specific extension directives if PHP hasn't been compiled with what we need statically */
                $modules   = shell_exec("$cmd --no-php-ini -m");

                /* Determine if we need to specifically add extensions */
                $arr_extensions = array_filter(
                    ['redis', 'igbinary', 'msgpack', 'json'],
                    function ($module) use ($modules) {
                        return strpos($modules, $module) === false;
                    }
                );

                /* If any are needed add them to the command */
                if ($arr_extensions) {
                    $cmd .= ' --no-php-ini';
                    foreach ($arr_extensions as $str_extension) {
                        /* We want to use the locally built redis extension */
                        if ($str_extension == 'redis') {
                            $str_extension = dirname(__DIR__) . '/modules/redis';
                        }

                        $cmd .= " --define extension=$str_extension.so";
                    }
                }
            }
        }

        return $cmd . ' ' . __DIR__ . '/' . $script . ' ';
    }
}
?>
