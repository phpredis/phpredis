<?php defined('PHPREDIS_TESTRUN') or die('Use TestRedis.php to run tests!\n');

require_once __DIR__ . '/TestSuite.php';
require_once __DIR__ . '/SessionHelpers.php';

class Redis_Test extends TestSuite {
    /**
     * @var Redis
     */
    public $redis;

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

    protected function getNilValue() {
        return FALSE;
    }

    protected function getSerializers() {
        $result = [Redis::SERIALIZER_NONE, Redis::SERIALIZER_PHP];

        if (defined('Redis::SERIALIZER_IGBINARY'))
            $result[] = Redis::SERIALIZER_IGBINARY;
        if (defined('Redis::SERIALIZER_JSON'))
            $result[] = Redis::SERIALIZER_JSON;
        if (defined('Redis::SERIALIZER_MSGPACK'))
            $result[] = Redis::SERIALIZER_MSGPACK;

        return $result;
    }

    protected function getCompressors() {
        $result['none'] = Redis::COMPRESSION_NONE;
        if (defined('Redis::COMPRESSION_LZF'))
            $result['lzf'] = Redis::COMPRESSION_LZF;
        if (defined('Redis::COMPRESSION_LZ4'))
            $result['lz4'] = Redis::COMPRESSION_LZ4;
        if (defined('Redis::COMPRESSION_ZSTD'))
            $result['zstd'] = Redis::COMPRESSION_ZSTD;

        return $result;
    }

    /* Overridable left/right constants */
    protected function getLeftConstant() {
        return Redis::LEFT;
    }

    protected function getRightConstant() {
        return Redis::RIGHT;
    }

    protected function detectKeyDB(array $info) {
        return strpos($info['executable'] ?? '', 'keydb') !== false ||
               isset($info['keydb']) ||
               isset($info['mvcc_depth']);
    }

    public function setUp() {
        $this->redis = $this->newInstance();
        $info = $this->redis->info();
        $this->version = (isset($info['redis_version'])?$info['redis_version']:'0.0.0');
        $this->is_keydb = $this->detectKeyDB($info);
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

    protected function sessionPrefix(): string {
        return 'PHPREDIS_SESSION:';
    }

    protected function sessionSaveHandler(): string {
        return 'redis';
    }

    protected function sessionSavePath(): string {
        return sprintf('tcp://%s:%d?%s', $this->getHost(), $this->getPort(),
                       $this->getAuthFragment());
    }

    protected function getAuthFragment() {
        $this->getAuthParts($user, $pass);

        if ($user && $pass) {
            return sprintf('auth[user]=%s&auth[pass]=%s', $user, $pass);
        } else if ($pass) {
            return sprintf('auth[pass]=%s', $pass);
        } else {
            return '';
        }
    }

    protected function newInstance() {
        $r = new Redis([
            'host' => $this->getHost(),
            'port' => $this->getPort(),
        ]);

        if ($this->getAuth()) {
            $this->assertTrue($r->auth($this->getAuth()));
        }
        return $r;
    }

    public function tearDown() {
        if ($this->redis) {
            $this->redis->close();
        }
    }

    public function reset() {
        $this->setUp();
        $this->tearDown();
    }

    /* Helper function to determine if the class has pipeline support */
    protected function havePipeline() {
        return defined(get_class($this->redis) . '::PIPELINE');
    }

    protected function haveMulti() {
        return defined(get_class($this->redis) . '::MULTI');
    }

    public function testMinimumVersion() {
        $this->assertTrue(version_compare($this->version, '2.4.0') >= 0);
    }

    public function testPing() {
        /* Reply literal off */
        $this->assertTrue($this->redis->ping());
        $this->assertTrue($this->redis->ping(NULL));
        $this->assertEquals('BEEP', $this->redis->ping('BEEP'));

        /* Make sure we're good in MULTI mode */
        if ($this->haveMulti()) {
            $this->assertEquals(
                [true, 'BEEP'],
                $this->redis->multi()
                    ->ping()
                    ->ping('BEEP')
                    ->exec()
            );
        }
    }

    public function testPipelinePublish() {
        $ret = $this->redis->pipeline()
            ->publish('chan', 'msg')
            ->exec();

        $this->assertIsArray($ret, 1);
        $this->assertGT(-1, $ret[0] ?? -1);
    }

    // Run some simple tests against the PUBSUB command.  This is problematic, as we
    // can't be sure what's going on in the instance, but we can do some things.
    public function testPubSub() {
        // Only available since 2.8.0
        if (version_compare($this->version, '2.8.0') < 0)
            $this->markTestSkipped();

        // PUBSUB CHANNELS ...
        $result = $this->redis->pubsub('channels', '*');
        $this->assertIsArray($result);
        $result = $this->redis->pubsub('channels');
        $this->assertIsArray($result);

        // PUBSUB NUMSUB

        $c1 = uniqid();
        $c2 = uniqid();

        $result = $this->redis->pubsub('numsub', [$c1, $c2]);

        // Should get an array back, with two elements
        $this->assertIsArray($result);
        $this->assertEquals(2, count($result));

        // Make sure the elements are correct, and have zero counts
        foreach ([$c1,$c2] as $channel) {
            $this->assertArrayKeyEquals($result, $channel, 0);
        }

        // PUBSUB NUMPAT
        $result = $this->redis->pubsub('numpat');
        $this->assertIsInt($result);

        // Invalid calls
        $this->assertFalse(@$this->redis->pubsub('notacommand'));
        $this->assertFalse(@$this->redis->pubsub('numsub', 'not-an-array'));
    }

    /* These test cases were generated randomly.  We're just trying to test
       that PhpRedis handles all combination of arguments correctly. */
    public function testBitcount() {
        /* key */
        $this->redis->set('bitcountkey', hex2bin('bd906b854ca76cae'));
        $this->assertEquals(33, $this->redis->bitcount('bitcountkey'));

        /* key, start */
        $this->redis->set('bitcountkey', hex2bin('400aac171382a29bebaab554f178'));
        $this->assertEquals(4, $this->redis->bitcount('bitcountkey', 13));

        /* key, start, end */
        $this->redis->set('bitcountkey', hex2bin('b1f32405'));
        $this->assertEquals(2, $this->redis->bitcount('bitcountkey', 3, 3));

        /* key, start, end BYTE */
        $this->redis->set('bitcountkey', hex2bin('10eb8939e68bfdb640260f0629f3'));
        $this->assertEquals(1, $this->redis->bitcount('bitcountkey', 8, 8, false));

        if ( ! $this->is_keydb && $this->minVersionCheck('7.0')) {
            /* key, start, end, BIT */
            $this->redis->set('bitcountkey', hex2bin('cd0e4c80f9e4590d888a10'));
            $this->assertEquals(5, $this->redis->bitcount('bitcountkey', 0, 9, true));
        }
    }

    public function testBitop() {
        if ( ! $this->minVersionCheck('2.6.0'))
            $this->markTestSkipped();

        $this->redis->set('{key}1', 'foobar');
        $this->redis->set('{key}2', 'abcdef');

        // Regression test for GitHub issue #2210
        $this->assertEquals(6, $this->redis->bitop('AND', '{key}1', '{key}2'));

        // Make sure RedisCluster doesn't even send the command.  We don't care
        // about what Redis returns
        @$this->redis->bitop('AND', 'key1', 'key2', 'key3');
        $this->assertNull($this->redis->getLastError());

        $this->redis->del('{key}1', '{key}2');
    }

    public function testBitsets() {
        $this->redis->del('key');
        $this->assertEquals(0, $this->redis->getBit('key', 0));
        $this->assertFalse($this->redis->getBit('key', -1));
        $this->assertEquals(0, $this->redis->getBit('key', 100000));

        $this->redis->set('key', "\xff");
        for ($i = 0; $i < 8; $i++) {
            $this->assertEquals(1, $this->redis->getBit('key', $i));
        }
        $this->assertEquals(0, $this->redis->getBit('key', 8));

        // change bit 0
        $this->assertEquals(1, $this->redis->setBit('key', 0, 0));
        $this->assertEquals(0, $this->redis->setBit('key', 0, 0));
        $this->assertEquals(0, $this->redis->getBit('key', 0));
        $this->assertKeyEquals("\x7f", 'key');

        // change bit 1
        $this->assertEquals(1, $this->redis->setBit('key', 1, 0));
        $this->assertEquals(0, $this->redis->setBit('key', 1, 0));
        $this->assertEquals(0, $this->redis->getBit('key', 1));
        $this->assertKeyEquals("\x3f", 'key');

        // change bit > 1
        $this->assertEquals(1, $this->redis->setBit('key', 2, 0));
        $this->assertEquals(0, $this->redis->setBit('key', 2, 0));
        $this->assertEquals(0, $this->redis->getBit('key', 2));
        $this->assertKeyEquals("\x1f", 'key');

        // values above 1 are changed to 1 but don't overflow on bits to the right.
        $this->assertEquals(0, $this->redis->setBit('key', 0, 0xff));
        $this->assertKeyEquals("\x9f", 'key');

        // Verify valid offset ranges
        $this->assertFalse($this->redis->getBit('key', -1));

        $this->redis->setBit('key', 0x7fffffff, 1);
        $this->assertEquals(1, $this->redis->getBit('key', 0x7fffffff));
    }

    public function testLcs() {
        if ( ! $this->minVersionCheck('7.0.0') || $this->is_keydb)
            $this->markTestSkipped();

        $key1 = '{lcs}1'; $key2 = '{lcs}2';
        $this->assertTrue($this->redis->set($key1, '12244447777777'));
        $this->assertTrue($this->redis->set($key2, '6666662244441'));

        $this->assertEquals('224444', $this->redis->lcs($key1, $key2));

        $this->assertEquals(
            ['matches', [[[1, 6], [6, 11]]], 'len', 6],
            $this->redis->lcs($key1, $key2, ['idx'])
        );
        $this->assertEquals(
            ['matches', [[[1, 6], [6, 11], 6]], 'len', 6],
            $this->redis->lcs($key1, $key2, ['idx', 'withmatchlen'])
        );

        $this->assertEquals(6, $this->redis->lcs($key1, $key2, ['len']));

        $this->redis->del([$key1, $key2]);
    }

    public function testLmpop() {
        if (version_compare($this->version, '7.0.0') < 0)
            $this->markTestSkipped();

        $key1 = '{l}1';
        $key2 = '{l}2';

        $this->redis->del($key1, $key2);

        $this->assertEquals(6, $this->redis->rpush($key1, 'A', 'B', 'C', 'D', 'E', 'F'));
        $this->assertEquals(6, $this->redis->rpush($key2, 'F', 'E', 'D', 'C', 'B', 'A'));

        $this->assertEquals([$key1, ['A']], $this->redis->lmpop([$key1, $key2], 'LEFT'));
        $this->assertEquals([$key1, ['F']], $this->redis->lmpop([$key1, $key2], 'RIGHT'));
        $this->assertEquals([$key1, ['B', 'C', 'D']], $this->redis->lmpop([$key1, $key2], 'LEFT',  3));

        $this->assertEquals(2, $this->redis->del($key1, $key2));
    }

    public function testBLmpop() {
        if (version_compare($this->version, '7.0.0') < 0)
            $this->markTestSkipped();

        $key1 = '{bl}1';
        $key2 = '{bl}2';

        $this->redis->del($key1, $key2);

        $this->assertEquals(2, $this->redis->rpush($key1, 'A', 'B'));
        $this->assertEquals(2, $this->redis->rpush($key2, 'C', 'D'));

        $this->assertEquals([$key1, ['B', 'A']], $this->redis->blmpop(.2, [$key1, $key2], 'RIGHT', 2));
        $this->assertEquals([$key2, ['C']], $this->redis->blmpop(.2, [$key1, $key2], 'LEFT'));
        $this->assertEquals([$key2, ['D']], $this->redis->blmpop(.2, [$key1, $key2], 'LEFT'));

        $st = microtime(true);
        $this->assertFalse($this->redis->blmpop(.2, [$key1, $key2], 'LEFT'));
        $et = microtime(true);

        // Very loose tolerance because CI is run on a potato
        $this->assertBetween($et - $st, .05, .75);
    }

    function testZmpop() {
        if (version_compare($this->version, '7.0.0') < 0)
            $this->markTestSkipped();

        $key1 = '{z}1';
        $key2 = '{z}2';

        $this->redis->del($key1, $key2);

        $this->assertEquals(4, $this->redis->zadd($key1, 0, 'zero', 2, 'two', 4, 'four', 6, 'six'));
        $this->assertEquals(4, $this->redis->zadd($key2, 1, 'one', 3, 'three', 5, 'five', 7, 'seven'));

        $this->assertEquals([$key1, ['zero' => 0.0]], $this->redis->zmpop([$key1, $key2], 'MIN'));
        $this->assertEquals([$key1, ['six' => 6.0]], $this->redis->zmpop([$key1, $key2], 'MAX'));
        $this->assertEquals([$key1, ['two' => 2.0, 'four' => 4.0]], $this->redis->zmpop([$key1, $key2], 'MIN', 3));

        $this->assertEquals(
            [$key2, ['one' => 1.0, 'three' => 3.0, 'five' => 5.0, 'seven' => 7.0]],
            $this->redis->zmpop([$key1, $key2], 'MIN', 128)
        );

        $this->assertFalse($this->redis->zmpop([$key1, $key2], 'MIN'));

        $this->redis->setOption(Redis::OPT_NULL_MULTIBULK_AS_NULL, true);
        $this->assertNull($this->redis->zmpop([$key1, $key2], 'MIN'));
        $this->redis->setOption(Redis::OPT_NULL_MULTIBULK_AS_NULL, false);
    }

    function testBZmpop() {
        if (version_compare($this->version, '7.0.0') < 0)
            $this->markTestSkipped();

        $key1 = '{z}1';
        $key2 = '{z}2';

        $this->redis->del($key1, $key2);

        $this->assertEquals(2, $this->redis->zadd($key1, 0, 'zero', 2, 'two'));
        $this->assertEquals(2, $this->redis->zadd($key2, 1, 'one', 3, 'three'));

        $this->assertEquals(
            [$key1, ['zero' => 0.0, 'two' => 2.0]],
            $this->redis->bzmpop(.1, [$key1, $key2], 'MIN', 2)
        );

        $this->assertEquals([$key2, ['three' => 3.0]], $this->redis->bzmpop(.1, [$key1, $key2], 'MAX'));
        $this->assertEquals([$key2, ['one' => 1.0]], $this->redis->bzmpop(.1, [$key1, $key2], 'MAX'));

        $st = microtime(true);
        $this->assertFalse($this->redis->bzmpop(.2, [$key1, $key2], 'MIN'));
        $et = microtime(true);

        $this->assertBetween($et - $st, .05, .75);
    }

    public function testBitPos() {
        if (version_compare($this->version, '2.8.7') < 0) {
            $this->MarkTestSkipped();
            return;
        }

        $this->redis->del('bpkey');

        $this->redis->set('bpkey', "\xff\xf0\x00");
        $this->assertEquals(12, $this->redis->bitpos('bpkey', 0));

        $this->redis->set('bpkey', "\x00\xff\xf0");
        $this->assertEquals(8, $this->redis->bitpos('bpkey', 1, 0));
        $this->assertEquals(8, $this->redis->bitpos('bpkey', 1, 1));

        $this->redis->set('bpkey', "\x00\x00\x00");
        $this->assertEquals(-1, $this->redis->bitpos('bpkey', 1));

        if ( ! $this->minVersionCheck('7.0.0'))
            return;

        $this->redis->set('bpkey', "\xF");
        $this->assertEquals(4, $this->redis->bitpos('bpkey', 1, 0, -1, true));
        $this->assertEquals(-1,  $this->redis->bitpos('bpkey', 1, 1, -1));
        $this->assertEquals(-1,  $this->redis->bitpos('bpkey', 1, 1, -1, false));
    }

    public function testSetLargeKeys() {
        foreach ([1000, 100000, 1000000] as $size) {
            $value = str_repeat('A', $size);
            $this->assertTrue($this->redis->set('x', $value));
            $this->assertKeyEquals($value, 'x');
        }
    }

    public function testEcho() {
        $this->assertEquals('hello', $this->redis->echo('hello'));
        $this->assertEquals('', $this->redis->echo(''));
        $this->assertEquals(' 0123 ', $this->redis->echo(' 0123 '));
    }

    public function testErr() {
        $this->redis->set('x', '-ERR');
        $this->assertKeyEquals('-ERR', 'x');
    }

    public function testSet() {
        $this->assertTrue($this->redis->set('key', 'nil'));
        $this->assertKeyEquals('nil', 'key');

        $this->assertTrue($this->redis->set('key', 'val'));

        $this->assertKeyEquals('val', 'key');
        $this->assertKeyEquals('val', 'key');
        $this->redis->del('keyNotExist');
        $this->assertKeyMissing('keyNotExist');

        $this->redis->set('key2', 'val');
        $this->assertKeyEquals('val', 'key2');

        $value1 = bin2hex(random_bytes(rand(64, 128)));
        $value2 = random_bytes(rand(65536, 65536 * 2));;

        $this->redis->set('key2', $value1);
        $this->assertKeyEquals($value1, 'key2');
        $this->assertKeyEquals($value1, 'key2');

        $this->redis->del('key');
        $this->redis->del('key2');


        $this->redis->set('key', $value2);
        $this->assertKeyEquals($value2, 'key');
        $this->redis->del('key');
        $this->assertKeyMissing('key');

        $data = gzcompress('42');
        $this->assertTrue($this->redis->set('key', $data));
        $this->assertEquals('42', gzuncompress($this->redis->get('key')));

        $this->redis->del('key');
        $data = gzcompress('value1');
        $this->assertTrue($this->redis->set('key', $data));
        $this->assertEquals('value1', gzuncompress($this->redis->get('key')));

        $this->redis->del('key');
        $this->assertTrue($this->redis->set('key', 0));
        $this->assertKeyEquals('0', 'key');
        $this->assertTrue($this->redis->set('key', 1));
        $this->assertKeyEquals('1', 'key');
        $this->assertTrue($this->redis->set('key', 0.1));
        $this->assertKeyEquals('0.1', 'key');
        $this->assertTrue($this->redis->set('key', '0.1'));
        $this->assertKeyEquals('0.1', 'key');
        $this->assertTrue($this->redis->set('key', true));
        $this->assertKeyEquals('1', 'key');

        $this->assertTrue($this->redis->set('key', ''));
        $this->assertKeyEquals('', 'key');
        $this->assertTrue($this->redis->set('key', NULL));
        $this->assertKeyEquals('', 'key');

        $this->assertTrue($this->redis->set('key', gzcompress('42')));
        $this->assertEquals('42', gzuncompress($this->redis->get('key')));
    }

    /* Extended SET options for Redis >= 2.6.12 */
    public function testExtendedSet() {
        // Skip the test if we don't have a new enough version of Redis
        if (version_compare($this->version, '2.6.12') < 0)
            $this->markTestSkipped();

        /* Legacy SETEX redirection */
        $this->redis->del('foo');
        $this->assertTrue($this->redis->set('foo', 'bar', 20));
        $this->assertKeyEquals('bar', 'foo');
        $this->assertEquals(20, $this->redis->ttl('foo'));

        /* Should coerce doubles into long */
        $this->assertTrue($this->redis->set('foo', 'bar-20.5', 20.5));
        $this->assertEquals(20, $this->redis->ttl('foo'));
        $this->assertKeyEquals('bar-20.5', 'foo');

        /* Invalid third arguments */
        $this->assertFalse(@$this->redis->set('foo', 'bar', 'baz'));
        $this->assertFalse(@$this->redis->set('foo', 'bar',new StdClass()));

        /* Set if not exist */
        $this->redis->del('foo');
        $this->assertTrue($this->redis->set('foo', 'bar', ['nx']));
        $this->assertKeyEquals('bar', 'foo');
        $this->assertFalse($this->redis->set('foo', 'bar', ['nx']));

        /* Set if exists */
        $this->assertTrue($this->redis->set('foo', 'bar', ['xx']));
        $this->assertKeyEquals('bar', 'foo');
        $this->redis->del('foo');
        $this->assertFalse($this->redis->set('foo', 'bar', ['xx']));

        /* Set with a TTL */
        $this->assertTrue($this->redis->set('foo', 'bar', ['ex' => 100]));
        $this->assertEquals(100, $this->redis->ttl('foo'));

        /* Set with a PTTL */
        $this->assertTrue($this->redis->set('foo', 'bar', ['px' => 100000]));
        $this->assertBetween($this->redis->pttl('foo'), 99000, 100001);

        /* Set if exists, with a TTL */
        $this->assertTrue($this->redis->set('foo', 'bar', ['xx', 'ex' => 105]));
        $this->assertEquals(105, $this->redis->ttl('foo'));
        $this->assertKeyEquals('bar', 'foo');

        /* Set if not exists, with a TTL */
        $this->redis->del('foo');
        $this->assertTrue($this->redis->set('foo', 'bar', ['nx', 'ex' => 110]));
        $this->assertEquals(110, $this->redis->ttl('foo'));
        $this->assertKeyEquals('bar', 'foo');
        $this->assertFalse($this->redis->set('foo', 'bar', ['nx', 'ex' => 110]));

        /* Throw some nonsense into the array, and check that the TTL came through */
        $this->redis->del('foo');
        $this->assertTrue($this->redis->set('foo', 'barbaz', ['not-valid', 'nx', 'invalid', 'ex' => 200]));
        $this->assertEquals(200, $this->redis->ttl('foo'));
        $this->assertKeyEquals('barbaz', 'foo');

        /* Pass NULL as the optional arguments which should be ignored */
        $this->redis->del('foo');
        $this->redis->set('foo', 'bar', NULL);
        $this->assertKeyEquals('bar', 'foo');
        $this->assertLT(0, $this->redis->ttl('foo'));

        /* Make sure we ignore bad/non-string options (regression test for #1835) */
        $this->assertTrue($this->redis->set('foo', 'bar', [NULL, 'EX' => 60]));
        $this->assertTrue($this->redis->set('foo', 'bar', [NULL, new stdClass(), 'EX' => 60]));
        $this->assertFalse(@$this->redis->set('foo', 'bar', [NULL, 'EX' => []]));

        if (version_compare($this->version, '6.0.0') < 0)
            return;

        /* KEEPTTL works by itself */
        $this->redis->set('foo', 'bar', ['EX' => 100]);
        $this->redis->set('foo', 'bar', ['KEEPTTL']);
        $this->assertBetween($this->redis->ttl('foo'), 90, 100);

        /* Works with other options */
        $this->redis->set('foo', 'bar', ['XX', 'KEEPTTL']);
        $this->assertBetween($this->redis->ttl('foo'), 90, 100);
        $this->redis->set('foo', 'bar', ['XX']);
        $this->assertEquals(-1, $this->redis->ttl('foo'));

        if (version_compare($this->version, '6.2.0') < 0)
            return;

        $this->assertEquals('bar', $this->redis->set('foo', 'baz', ['GET']));
    }

    public function testGetSet() {
        $this->redis->del('key');
        $this->assertFalse($this->redis->getSet('key', '42'));
        $this->assertEquals('42', $this->redis->getSet('key', '123'));
        $this->assertEquals('123', $this->redis->getSet('key', '123'));
    }

    public function testRandomKey() {
        for ($i = 0; $i < 1000; $i++) {
            $k = $this->redis->randomKey();
            $this->assertKeyExists($k);
        }
    }

    public function testRename() {
        // strings
        $this->redis->del('{key}0');
        $this->redis->set('{key}0', 'val0');
        $this->redis->rename('{key}0', '{key}1');
        $this->assertKeyMissing('{key}0');
        $this->assertKeyEquals('val0', '{key}1');
    }

    public function testRenameNx() {
        // strings
        $this->redis->del('{key}0', '{key}1');
        $this->redis->set('{key}0', 'val0');
        $this->redis->set('{key}1', 'val1');
        $this->assertFalse($this->redis->renameNx('{key}0', '{key}1'));
        $this->assertKeyEquals('val0', '{key}0');
        $this->assertKeyEquals('val1', '{key}1');

        // lists
        $this->redis->del('{key}0');
        $this->redis->del('{key}1');
        $this->redis->lPush('{key}0', 'val0');
        $this->redis->lPush('{key}0', 'val1');
        $this->redis->lPush('{key}1', 'val1-0');
        $this->redis->lPush('{key}1', 'val1-1');
        $this->assertFalse($this->redis->renameNx('{key}0', '{key}1'));
        $this->assertEquals(['val1', 'val0'], $this->redis->lRange('{key}0', 0, -1));
        $this->assertEquals(['val1-1', 'val1-0'], $this->redis->lRange('{key}1', 0, -1));

        $this->redis->del('{key}2');
        $this->assertTrue($this->redis->renameNx('{key}0', '{key}2'));
        $this->assertEquals([], $this->redis->lRange('{key}0', 0, -1));
        $this->assertEquals(['val1', 'val0'], $this->redis->lRange('{key}2', 0, -1));
    }

    public function testMultiple() {
        $kvals = [
            'mget1' => 'v1',
            'mget2' => 'v2',
            'mget3' => 'v3'
        ];

        $this->redis->mset($kvals);

        $this->redis->set(1, 'test');

        $this->assertEquals([$kvals['mget1']], $this->redis->mget(['mget1']));

        $this->assertEquals(['v1', 'v2', false], $this->redis->mget(['mget1', 'mget2', 'NoKey']));
        $this->assertEquals(['v1', 'v2', 'v3'], $this->redis->mget(['mget1', 'mget2', 'mget3']));
        $this->assertEquals(['v1', 'v2', 'v3'], $this->redis->mget(['mget1', 'mget2', 'mget3']));

        $this->redis->set('k5', '$1111111111');
        $this->assertEquals(['$1111111111'], $this->redis->mget(['k5']));

        $this->assertEquals(['test'], $this->redis->mget([1])); // non-string
    }

    public function testMultipleBin() {
        $kvals = [
            'binkey-1' => random_bytes(16),
            'binkey-2' => random_bytes(16),
            'binkey-3' => random_bytes(16),
        ];

        foreach ($kvals as $k => $v) {
            $this->redis->set($k, $v);
        }

        $this->assertEquals(array_values($kvals),
                            $this->redis->mget(array_keys($kvals)));
    }

    public function testExpireMember() {
        if ( ! $this->is_keydb)
            $this->markTestSkipped();

        $this->redis->del('h');
        $this->redis->hmset('h', ['f1' => 'v1', 'f2' => 'v2', 'f3' => 'v3', 'f4' => 'v4']);

        $this->assertEquals(1, $this->redis->expiremember('h', 'f1', 1));
        $this->assertEquals(1, $this->redis->expiremember('h', 'f2', 1000, 'ms'));
        $this->assertEquals(1, $this->redis->expiremember('h', 'f3', 1000,  null));
        $this->assertEquals(0, $this->redis->expiremember('h', 'nk', 10));

        $this->assertEquals(1, $this->redis->expirememberat('h', 'f4', time() + 1));
        $this->assertEquals(0, $this->redis->expirememberat('h', 'nk', time() + 1));
    }

    public function testExpire() {
        $this->redis->del('key');
        $this->redis->set('key', 'value');

        $this->assertKeyEquals('value', 'key');
        $this->redis->expire('key', 1);
        $this->assertKeyEquals('value', 'key');
        sleep(2);
        $this->assertKeyMissing('key');
    }

    /* This test is prone to failure in the Travis container, so attempt to
       mitigate this by running more than once */
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

    function testExpireOptions() {
        if ( ! $this->minVersionCheck('7.0.0'))
            $this->markTestSkipped();

        $this->redis->set('eopts', 'value');

        /* NX -- Only if expiry isn't set so success, then failure */
        $this->assertTrue($this->redis->expire('eopts', 1000, 'NX'));
        $this->assertFalse($this->redis->expire('eopts', 1000, 'NX'));

        /* XX -- Only set if the key has an existing expiry */
        $this->assertTrue($this->redis->expire('eopts', 1000, 'XX'));
        $this->assertTrue($this->redis->persist('eopts'));
        $this->assertFalse($this->redis->expire('eopts', 1000, 'XX'));

        /* GT -- Only set when new expiry > current expiry */
        $this->assertTrue($this->redis->expire('eopts', 200));
        $this->assertTrue($this->redis->expire('eopts', 300, 'GT'));
        $this->assertFalse($this->redis->expire('eopts', 100, 'GT'));

        /* LT -- Only set when expiry < current expiry */
        $this->assertTrue($this->redis->expire('eopts', 200));
        $this->assertTrue($this->redis->expire('eopts', 100, 'LT'));
        $this->assertFalse($this->redis->expire('eopts', 300, 'LT'));

        /* Sending a nonsensical mode fails without sending a command */
        $this->redis->clearLastError();
        $this->assertFalse(@$this->redis->expire('eopts', 999, 'nonsense'));
        $this->assertNull($this->redis->getLastError());

        $this->redis->del('eopts');
    }

    public function testExpiretime() {
        if (version_compare($this->version, '7.0.0') < 0)
            $this->markTestSkipped();

        $now = time();

        $this->assertTrue($this->redis->set('key1', 'value'));
        $this->assertTrue($this->redis->expireat('key1', $now + 10));
        $this->assertEquals($now + 10, $this->redis->expiretime('key1'));
        $this->assertEquals(1000 * ($now + 10), $this->redis->pexpiretime('key1'));

        $this->redis->del('key1');
    }

    public function testGetEx() {
        if (version_compare($this->version, '6.2.0') < 0)
            $this->markTestSkipped();

        $this->assertTrue($this->redis->set('key', 'value'));

        $this->assertEquals('value', $this->redis->getEx('key', ['EX' => 100]));
        $this->assertBetween($this->redis->ttl('key'), 95, 100);

        $this->assertEquals('value', $this->redis->getEx('key', ['PX' => 100000]));
        $this->assertBetween($this->redis->pttl('key'), 95000, 100000);

        $this->assertEquals('value', $this->redis->getEx('key', ['EXAT' => time() + 200]));
        $this->assertBetween($this->redis->ttl('key'), 195, 200);

        $this->assertEquals('value', $this->redis->getEx('key', ['PXAT' => (time()*1000) + 25000]));
        $this->assertBetween($this->redis->pttl('key'), 24000, 25000);

        $this->assertEquals('value', $this->redis->getEx('key', ['PERSIST' => true]));
        $this->assertEquals(-1, $this->redis->ttl('key'));

        $this->assertTrue($this->redis->expire('key', 100));
        $this->assertBetween($this->redis->ttl('key'), 95, 100);

        $this->assertEquals('value', $this->redis->getEx('key', ['PERSIST']));
        $this->assertEquals(-1, $this->redis->ttl('key'));
    }

    public function testSetEx() {
        $this->redis->del('key');
        $this->assertTrue($this->redis->setex('key', 7, 'val'));
        $this->assertEquals(7, $this->redis->ttl('key'));
        $this->assertKeyEquals('val', 'key');
    }

    public function testPSetEx() {
        $this->redis->del('key');
        $this->assertTrue($this->redis->psetex('key', 7 * 1000, 'val'));
        $this->assertEquals(7, $this->redis->ttl('key'));
        $this->assertKeyEquals('val', 'key');
    }

    public function testSetNX() {

        $this->redis->set('key', 42);
        $this->assertFalse($this->redis->setnx('key', 'err'));
        $this->assertKeyEquals('42', 'key');

        $this->redis->del('key');
        $this->assertTrue($this->redis->setnx('key', '42'));
        $this->assertKeyEquals('42', 'key');
    }

    public function testExpireAtWithLong() {
        if (PHP_INT_SIZE != 8)
            $this->markTestSkipped('64 bits only');

        $large_expiry = 3153600000;
        $this->redis->del('key');
        $this->assertTrue($this->redis->setex('key', $large_expiry, 'val'));
        $this->assertEquals($large_expiry, $this->redis->ttl('key'));
    }

    public function testIncr() {
        $this->redis->set('key', 0);

        $this->redis->incr('key');
        $this->assertKeyEqualsWeak(1, 'key');

        $this->redis->incr('key');
        $this->assertKeyEqualsWeak(2, 'key');

        $this->redis->incrBy('key', 3);
        $this->assertKeyEqualsWeak(5, 'key');

        $this->redis->incrBy('key', 1);
        $this->assertKeyEqualsWeak(6, 'key');

        $this->redis->incrBy('key', -1);
        $this->assertKeyEqualsWeak(5, 'key');

        $this->redis->incr('key', 5);
        $this->assertKeyEqualsWeak(10, 'key');

        $this->redis->del('key');

        $this->redis->set('key', 'abc');

        $this->redis->incr('key');
        $this->assertKeyEquals('abc', 'key');

        $this->redis->incr('key');
        $this->assertKeyEquals('abc', 'key');

        $this->redis->set('key', 0);
        $this->assertEquals(PHP_INT_MAX, $this->redis->incrby('key', PHP_INT_MAX));
    }

    public function testIncrByFloat() {
        // incrbyfloat is new in 2.6.0
        if (version_compare($this->version, '2.5.0') < 0)
            $this->markTestSkipped();

        $this->redis->del('key');

        $this->redis->set('key', 0);

        $this->redis->incrbyfloat('key', 1.5);
        $this->assertKeyEquals('1.5', 'key');

        $this->redis->incrbyfloat('key', 2.25);
        $this->assertKeyEquals('3.75', 'key');

        $this->redis->incrbyfloat('key', -2.25);
        $this->assertKeyEquals('1.5', 'key');

        $this->redis->set('key', 'abc');

        $this->redis->incrbyfloat('key', 1.5);
        $this->assertKeyEquals('abc', 'key');

        $this->redis->incrbyfloat('key', -1.5);
        $this->assertKeyEquals('abc', 'key');

        // Test with prefixing
        $this->redis->setOption(Redis::OPT_PREFIX, 'someprefix:');
        $this->redis->del('key');
        $this->redis->incrbyfloat('key',1.8);
        $this->assertKeyEqualsWeak(1.8, 'key');
        $this->redis->setOption(Redis::OPT_PREFIX, '');
        $this->assertKeyExists('someprefix:key');
        $this->redis->del('someprefix:key');
    }

    public function testDecr() {
        $this->redis->set('key', 5);

        $this->redis->decr('key');
        $this->assertKeyEqualsWeak(4, 'key');

        $this->redis->decr('key');
        $this->assertKeyEqualsWeak(3, 'key');

        $this->redis->decrBy('key', 2);
        $this->assertKeyEqualsWeak(1, 'key');

        $this->redis->decrBy('key', 1);
        $this->assertKeyEqualsWeak(0, 'key');

        $this->redis->decrBy('key', -10);
        $this->assertKeyEqualsWeak(10, 'key');

        $this->redis->decr('key', 10);
        $this->assertKeyEqualsWeak(0, 'key');
    }


    public function testExists() {
        /* Single key */
        $this->redis->del('key');
        $this->assertKeyMissing('key');
        $this->redis->set('key', 'val');
        $this->assertKeyExists('key');

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
        if (count($mkeys))
            $this->assertEquals(count($mkeys), $this->redis->exists(...$mkeys));
    }

    public function testTouch() {
        if ( ! $this->minVersionCheck('3.2.1'))
            $this->markTestSkipped();

        $this->redis->del('notakey');

        $this->assertTrue($this->redis->mset(['{idle}1' => 'beep', '{idle}2' => 'boop']));
        usleep(1100000);
        $this->assertGT(0, $this->redis->object('idletime', '{idle}1'));
        $this->assertGT(0, $this->redis->object('idletime', '{idle}2'));

        $this->assertEquals(2, $this->redis->touch('{idle}1', '{idle}2', '{idle}notakey'));
        $idle1 = $this->redis->object('idletime', '{idle}1');
        $idle2 = $this->redis->object('idletime', '{idle}2');

        /* We're not testing if idle is 0 because CPU scheduling on GitHub CI
         * potatoes can cause that to erroneously fail. */
        $this->assertLT(2, $idle1);
        $this->assertLT(2, $idle2);
    }

    public function testKeys() {
        $pattern = 'keys-test-';
        for ($i = 1; $i < 10; $i++) {
            $this->redis->set($pattern.$i, $i);
        }
        $this->redis->del($pattern.'3');
        $keys = $this->redis->keys($pattern.'*');

        $this->redis->set($pattern.'3', 'something');

        $keys2 = $this->redis->keys($pattern.'*');

        $this->assertEquals((count($keys) + 1), count($keys2));

        // empty array when no key matches
        $this->assertEquals([], $this->redis->keys(uniqid() . '*'));
    }

    protected function genericDelUnlink($cmd) {
        $key = uniqid('key:');
        $this->redis->set($key, 'val');
        $this->assertKeyEquals('val', $key);
        $this->assertEquals(1, $this->redis->$cmd($key));
        $this->assertFalse($this->redis->get($key));

        // multiple, all existing
        $this->redis->set('x', 0);
        $this->redis->set('y', 1);
        $this->redis->set('z', 2);
        $this->assertEquals(3, $this->redis->$cmd('x', 'y', 'z'));
        $this->assertFalse($this->redis->get('x'));
        $this->assertFalse($this->redis->get('y'));
        $this->assertFalse($this->redis->get('z'));

        // multiple, none existing
        $this->assertEquals(0, $this->redis->$cmd('x', 'y', 'z'));
        $this->assertFalse($this->redis->get('x'));
        $this->assertFalse($this->redis->get('y'));
        $this->assertFalse($this->redis->get('z'));

        // multiple, some existing
        $this->redis->set('y', 1);
        $this->assertEquals(1, $this->redis->$cmd('x', 'y', 'z'));
        $this->assertFalse($this->redis->get('y'));

        $this->redis->set('x', 0);
        $this->redis->set('y', 1);
        $this->assertEquals(2, $this->redis->$cmd(['x', 'y']));
    }

    public function testDelete() {
        $this->genericDelUnlink('DEL');
    }

    public function testUnlink() {
        if (version_compare($this->version, '4.0.0') < 0)
            $this->markTestSkipped();

        $this->genericDelUnlink('UNLINK');
    }

    public function testType() {
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
        $this->assertEquals(1, $this->redis->sAdd(123, 'val0'));
        $this->assertEquals(['val0'], $this->redis->sMembers(123));

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
        if ($this->minVersionCheck('5.0')) {
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
        $this->assertEquals(8, $this->redis->append('key', 'val2'));
        $this->assertKeyEquals('val1val2', 'key');

        $this->redis->del('keyNotExist');
        $this->assertEquals(5, $this->redis->append('keyNotExist', 'value'));
        $this->assertKeyEquals('value', 'keyNotExist');

        $this->redis->set('key', 'This is a string') ;
        $this->assertEquals('This', $this->redis->getRange('key', 0, 3));
        $this->assertEquals('string', $this->redis->getRange('key', -6, -1));
        $this->assertEquals('string', $this->redis->getRange('key', -6, 100000));
        $this->assertKeyEquals('This is a string', 'key');

        $this->redis->set('key', 'This is a string') ;
        $this->assertEquals(16, $this->redis->strlen('key'));

        $this->redis->set('key', 10) ;
        $this->assertEquals(2, $this->redis->strlen('key'));
        $this->redis->set('key', '') ;
        $this->assertEquals(0, $this->redis->strlen('key'));
        $this->redis->set('key', '000') ;
        $this->assertEquals(3, $this->redis->strlen('key'));
    }

    public function testlPop() {
        $this->redis->del('list');

        $this->redis->lPush('list', 'val');
        $this->redis->lPush('list', 'val2');
        $this->redis->rPush('list', 'val3');

        $this->assertEquals('val2', $this->redis->lPop('list'));
        if (version_compare($this->version, '6.2.0') < 0) {
            $this->assertEquals('val', $this->redis->lPop('list'));
            $this->assertEquals('val3', $this->redis->lPop('list'));
        } else {
            $this->assertEquals(['val', 'val3'], $this->redis->lPop('list', 2));
        }

        $this->assertFalse($this->redis->lPop('list'));

        $this->redis->del('list');
        $this->assertEquals(1, $this->redis->lPush('list', gzcompress('val1')));
        $this->assertEquals(2, $this->redis->lPush('list', gzcompress('val2')));
        $this->assertEquals(3, $this->redis->lPush('list', gzcompress('val3')));

        $this->assertEquals('val3', gzuncompress($this->redis->lPop('list')));
        $this->assertEquals('val2', gzuncompress($this->redis->lPop('list')));
        $this->assertEquals('val1', gzuncompress($this->redis->lPop('list')));
    }

    public function testrPop() {
        $this->redis->del('list');

        $this->redis->rPush('list', 'val');
        $this->redis->rPush('list', 'val2');
        $this->redis->lPush('list', 'val3');

        $this->assertEquals('val2', $this->redis->rPop('list'));
        if (version_compare($this->version, '6.2.0') < 0) {
            $this->assertEquals('val', $this->redis->rPop('list'));
            $this->assertEquals('val3', $this->redis->rPop('list'));
        } else {
            $this->assertEquals(['val', 'val3'], $this->redis->rPop('list', 2));
        }

        $this->assertFalse($this->redis->rPop('list'));

        $this->redis->del('list');
        $this->assertEquals(1, $this->redis->rPush('list', gzcompress('val1')));
        $this->assertEquals(2, $this->redis->rPush('list', gzcompress('val2')));
        $this->assertEquals(3, $this->redis->rPush('list', gzcompress('val3')));

        $this->assertEquals('val3', gzuncompress($this->redis->rPop('list')));
        $this->assertEquals('val2', gzuncompress($this->redis->rPop('list')));
        $this->assertEquals('val1', gzuncompress($this->redis->rPop('list')));
    }

    /* Regression test for GH #2329 */
    public function testrPopSerialization() {
        $this->redis->setOption(Redis::OPT_SERIALIZER, Redis::SERIALIZER_PHP);

        $this->redis->del('rpopkey');
        $this->redis->rpush('rpopkey', ['foo'], ['bar']);
        $this->assertEquals([['bar'], ['foo']], $this->redis->rpop('rpopkey', 2));

        $this->redis->rpush('rpopkey', ['foo'], ['bar']);
        $this->assertEquals([['foo'], ['bar']], $this->redis->lpop('rpopkey', 2));

        $this->redis->setOption(Redis::OPT_SERIALIZER, Redis::SERIALIZER_NONE);
    }

    public function testblockingPop() {
        /* Test with a double timeout in Redis >= 6.0.0 */
        if (version_compare($this->version, '6.0.0') >= 0) {
            $this->redis->del('list');
            $this->redis->lpush('list', 'val1', 'val2');
            $this->assertEquals(['list', 'val2'], $this->redis->blpop(['list'], .1));
            $this->assertEquals(['list', 'val1'], $this->redis->blpop(['list'], .1));
        }

        // non blocking blPop, brPop
        $this->redis->del('list');
        $this->redis->lPush('list', 'val1', 'val2');
        $this->assertEquals(['list', 'val2'], $this->redis->blPop(['list'], 2));
        $this->assertEquals(['list', 'val1'], $this->redis->blPop(['list'], 2));

        $this->redis->del('list');
        $this->redis->lPush('list', 'val1', 'val2');
        $this->assertEquals(['list', 'val1'], $this->redis->brPop(['list'], 1));
        $this->assertEquals(['list', 'val2'], $this->redis->brPop(['list'], 1));

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

    public function testLLen() {
        $this->redis->del('list');

        $this->redis->lPush('list', 'val');
        $this->assertEquals(1, $this->redis->llen('list'));

        $this->redis->lPush('list', 'val2');
        $this->assertEquals(2, $this->redis->llen('list'));

        $this->assertEquals('val2', $this->redis->lPop('list'));
        $this->assertEquals(1, $this->redis->llen('list'));

        $this->assertEquals('val', $this->redis->lPop('list'));
        $this->assertEquals(0, $this->redis->llen('list'));

        $this->assertFalse($this->redis->lPop('list'));
        $this->assertEquals(0, $this->redis->llen('list'));    // empty returns 0

        $this->redis->del('list');
        $this->assertEquals(0, $this->redis->llen('list'));    // non-existent returns 0

        $this->redis->set('list', 'actually not a list');
        $this->assertFalse($this->redis->llen('list'));// not a list returns FALSE
    }

    public function testlPopx() {
        $this->redis->del('keyNotExists');
        $this->assertEquals(0, $this->redis->lPushx('keyNotExists', 'value'));
        $this->assertEquals(0, $this->redis->rPushx('keyNotExists', 'value'));

        $this->redis->del('key');
        $this->redis->lPush('key', 'val0');
        $this->assertEquals(2, $this->redis->lPushx('key', 'val1'));
        $this->assertEquals(3, $this->redis->rPushx('key', 'val2'));
        $this->assertEquals(['val1', 'val0', 'val2'], $this->redis->lrange('key', 0, -1));

        //test linsert
        $this->redis->del('key');
        $this->redis->lPush('key', 'val0');
        $this->assertEquals(0, $this->redis->lInsert('keyNotExists', Redis::AFTER, 'val1', 'val2'));
        $this->assertEquals(-1, $this->redis->lInsert('key', Redis::BEFORE, 'valX', 'val2'));

        $this->assertEquals(2, $this->redis->lInsert('key', Redis::AFTER, 'val0', 'val1'));
        $this->assertEquals(3, $this->redis->lInsert('key', Redis::BEFORE, 'val0', 'val2'));
        $this->assertEquals(['val2', 'val0', 'val1'], $this->redis->lrange('key', 0, -1));
    }

    public function testlPos() {
        $this->redis->del('key');
        $this->redis->lPush('key', 'val0', 'val1', 'val1');
        $this->assertEquals(2, $this->redis->lPos('key', 'val0'));
        $this->assertEquals(0, $this->redis->lPos('key', 'val1'));
        $this->assertEquals(1, $this->redis->lPos('key', 'val1', ['rank' => 2]));
        $this->assertEquals([0, 1], $this->redis->lPos('key', 'val1', ['count' => 2]));
        $this->assertEquals([0], $this->redis->lPos('key', 'val1', ['count' => 2, 'maxlen' => 1]));
        $this->assertEquals([], $this->redis->lPos('key', 'val2', ['count' => 1]));

        foreach ([[true, NULL], [false, false]] as $optpack) {
            list ($setting, $expected) = $optpack;
            $this->redis->setOption(Redis::OPT_NULL_MULTIBULK_AS_NULL, $setting);
            $this->assertEquals($expected, $this->redis->lPos('key', 'val2'));
        }
    }

    // ltrim, lLen, lpop
    public function testltrim() {
        $this->redis->del('list');

        $this->redis->lPush('list', 'val');
        $this->redis->lPush('list', 'val2');
        $this->redis->lPush('list', 'val3');
        $this->redis->lPush('list', 'val4');

        $this->assertTrue($this->redis->ltrim('list', 0, 2));
        $this->assertEquals(3, $this->redis->llen('list'));

        $this->redis->ltrim('list', 0, 0);
        $this->assertEquals(1, $this->redis->llen('list'));
        $this->assertEquals('val4', $this->redis->lPop('list'));

        $this->assertTrue($this->redis->ltrim('list', 10, 10000));
        $this->assertTrue($this->redis->ltrim('list', 10000, 10));

        // test invalid type
        $this->redis->set('list', 'not a list...');
        $this->assertFalse($this->redis->ltrim('list', 0, 2));
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
        foreach ([1, 2, 3, 4] as $id) {
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

        $this->assertEquals(['1', '2', '3'], $this->redis->sort('some-item', ['sort' => 'asc']));
        $this->assertEquals(['3', '2', '1'], $this->redis->sort('some-item', ['sort' => 'desc']));
        $this->assertEquals(['1', '2', '3'], $this->redis->sort('some-item'));

        // Kill our set/prefix
        $this->redis->del('some-item');
        $this->redis->setOption(Redis::OPT_PREFIX, '');
    }

    public function testSortAsc() {
        $this->setupSort();
        // sort by age and get IDs
        $byAgeAsc = ['3', '1', '2', '4'];
        $this->assertEquals($byAgeAsc, $this->redis->sort('person:id', ['by' => 'person:age_*']));
        $this->assertEquals($byAgeAsc, $this->redis->sort('person:id', ['by' => 'person:age_*', 'sort' => 'asc']));
        $this->assertEquals(['1', '2', '3', '4'], $this->redis->sort('person:id', ['by' => NULL]));   // check that NULL works.
        $this->assertEquals(['1', '2', '3', '4'], $this->redis->sort('person:id', ['by' => NULL, 'get' => NULL])); // for all fields.
        $this->assertEquals(['1', '2', '3', '4'], $this->redis->sort('person:id', ['sort' => 'asc']));

        // sort by age and get names
        $byAgeAsc = ['Carol', 'Alice', 'Bob', 'Dave'];
        $this->assertEquals($byAgeAsc, $this->redis->sort('person:id', ['by' => 'person:age_*', 'get' => 'person:name_*']));
        $this->assertEquals($byAgeAsc, $this->redis->sort('person:id', ['by' => 'person:age_*', 'get' => 'person:name_*', 'sort' => 'asc']));

        $this->assertEquals(array_slice($byAgeAsc, 0, 2), $this->redis->sort('person:id', ['by' => 'person:age_*', 'get' => 'person:name_*', 'limit' => [0, 2]]));
        $this->assertEquals(array_slice($byAgeAsc, 0, 2), $this->redis->sort('person:id', ['by' => 'person:age_*', 'get' => 'person:name_*', 'limit' => [0, 2], 'sort' => 'asc']));

        $this->assertEquals(array_slice($byAgeAsc, 1, 2), $this->redis->sort('person:id', ['by' => 'person:age_*', 'get' => 'person:name_*', 'limit' => [1, 2]]));
        $this->assertEquals(array_slice($byAgeAsc, 1, 2), $this->redis->sort('person:id', ['by' => 'person:age_*', 'get' => 'person:name_*', 'limit' => [1, 2], 'sort' => 'asc']));
        $this->assertEquals($byAgeAsc, $this->redis->sort('person:id', ['by' => 'person:age_*', 'get' => 'person:name_*', 'limit' => [0, 4]]));
        $this->assertEquals($byAgeAsc, $this->redis->sort('person:id', ['by' => 'person:age_*', 'get' => 'person:name_*', 'limit' => [0, '4']])); // with strings
        $this->assertEquals($byAgeAsc, $this->redis->sort('person:id', ['by' => 'person:age_*', 'get' => 'person:name_*', 'limit' => ['0', 4]]));

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
        foreach ($list as $i) {
            $this->redis->lPush('list', $i);
        }

        // SORT list → [ghi, def, abc]
        if (version_compare($this->version, '2.5.0') < 0) {
            $this->assertEquals(array_reverse($list), $this->redis->sort('list'));
            $this->assertEquals(array_reverse($list), $this->redis->sort('list', ['sort' => 'asc']));
        } else {
            // TODO rewrite, from 2.6.0 release notes:
            // SORT now will refuse to sort in numerical mode elements that can't be parsed
            // as numbers
        }

        // SORT list ALPHA → [abc, def, ghi]
        $this->assertEquals($list, $this->redis->sort('list', ['alpha' => true]));
        $this->assertEquals($list, $this->redis->sort('list', ['sort' => 'asc', 'alpha' => true]));
    }

    public function testSortDesc() {
        $this->setupSort();

        // sort by age and get IDs
        $byAgeDesc = ['4', '2', '1', '3'];
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
        foreach ($list as $i) {
            $this->redis->lPush('list', $i);
        }

        // SORT list ALPHA → [abc, def, ghi]
        $this->assertEquals(['ghi', 'def', 'abc'], $this->redis->sort('list', ['sort' => 'desc', 'alpha' => true]));
    }

    /* This test is just to make sure SORT and SORT_RO are both callable */
    public function testSortHandler() {
        $this->redis->del('list');

        $this->redis->rpush('list', 'c', 'b', 'a');

        $methods = ['sort'];
        if ($this->minVersionCheck('7.0.0')) $methods[] = 'sort_ro';

        foreach ($methods as $method) {
            $this->assertEquals(['a', 'b', 'c'], $this->redis->$method('list', ['sort' => 'asc', 'alpha' => true]));
        }
    }

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
        $this->assertFalse($this->redis->lIndex('list', -4));

        $this->redis->rPush('list', 'val4');
        $this->assertEquals('val4', $this->redis->lIndex('list', 3));
        $this->assertEquals('val4', $this->redis->lIndex('list', -1));
    }

    public function testlMove() {
        if (version_compare($this->version, '6.2.0') < 0)
            $this->markTestSkipped();

        [$list1, $list2] = ['{l}0', '{l}1'];
        $left  = $this->getLeftConstant();
        $right = $this->getRightConstant();

        $this->redis->del($list1, $list2);
        $this->redis->lPush($list1, 'a');
        $this->redis->lPush($list1, 'b');
        $this->redis->lPush($list1, 'c');

        $return = $this->redis->lMove($list1, $list2, $left, $right);
        $this->assertEquals('c', $return);

        $return = $this->redis->lMove($list1, $list2, $right, $left);
        $this->assertEquals('a', $return);

        $this->assertEquals(['b'], $this->redis->lRange($list1, 0, -1));
        $this->assertEquals(['a', 'c'], $this->redis->lRange($list2, 0, -1));

    }

    public function testBlmove() {
        if (version_compare($this->version, '6.2.0') < 0)
            $this->markTestSkipped();

        [$list1, $list2] = ['{l}0', '{l}1'];
        $left = $this->getLeftConstant();

        $this->redis->del($list1, $list2);
        $this->redis->rpush($list1, 'a');


        $this->assertEquals('a', $this->redis->blmove($list1, $list2, $left, $left, 1.));

        $st = microtime(true);
        $ret = $this->redis->blmove($list1, $list2, $left, $left, .1);
        $et = microtime(true);

        $this->assertFalse($ret);
        $this->assertGT(.09, $et - $st);
    }

    // lRem testing
    public function testLRem() {
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
        $this->assertFalse($this->redis->get('list'));

        $this->redis->set('list', 'actually not a list');
        $this->assertFalse($this->redis->lrem('list', 'x'));
    }

    public function testSAdd() {
        $this->redis->del('set');

        $this->assertEquals(1, $this->redis->sAdd('set', 'val'));
        $this->assertEquals(0, $this->redis->sAdd('set', 'val'));

        $this->assertTrue($this->redis->sismember('set', 'val'));
        $this->assertFalse($this->redis->sismember('set', 'val2'));

        $this->assertEquals(1, $this->redis->sAdd('set', 'val2'));

        $this->assertTrue($this->redis->sismember('set', 'val2'));
    }

    public function testSCard() {
        $this->redis->del('set');
        $this->assertEquals(1, $this->redis->sAdd('set', 'val'));
        $this->assertEquals(1, $this->redis->scard('set'));
        $this->assertEquals(1, $this->redis->sAdd('set', 'val2'));
        $this->assertEquals(2, $this->redis->scard('set'));
    }

    public function testSRem() {
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
        $this->assertFalse($this->redis->sPop('set0'));

        $this->redis->sAdd('set0', 'val');
        $this->redis->sAdd('set0', 'val2');

        $v0 = $this->redis->sPop('set0');
        $this->assertEquals(1, $this->redis->scard('set0'));
        $this->assertInArray($v0, ['val', 'val2']);
        $v1 = $this->redis->sPop('set0');
        $this->assertEquals(0, $this->redis->scard('set0'));
        $this->assertEqualsCanonicalizing(['val', 'val2'], [$v0, $v1]);

        $this->assertFalse($this->redis->sPop('set0'));
    }

    public function testsPopWithCount() {
        if ( ! $this->minVersionCheck('3.2'))
            $this->markTestSkipped();

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
        if ($this->assertIsArray($ret, $count)) {
            /* Probably overkill but validate the actual returned members */
            for ($i = 0; $i < $count; $i++) {
                $this->assertInArray($prefix.$i, $ret);
            }
        }
    }

    public function testsRandMember() {
        $this->redis->del('set0');
        $this->assertFalse($this->redis->sRandMember('set0'));

        $this->redis->sAdd('set0', 'val');
        $this->redis->sAdd('set0', 'val2');

        $got = [];
        while (true) {
            $v = $this->redis->sRandMember('set0');
            $this->assertEquals(2, $this->redis->scard('set0')); // no change.
            $this->assertInArray($v, ['val', 'val2']);

            $got[$v] = $v;
            if (count($got) == 2) {
                break;
            }
        }

        //
        // With and without count, while serializing
        //

        $this->redis->del('set0');
        $this->redis->setOption(Redis::OPT_SERIALIZER, Redis::SERIALIZER_PHP);
        for ($i = 0; $i < 5; $i++) {
            $member = "member:$i";
            $this->redis->sAdd('set0', $member);
            $mems[] = $member;
        }

        $member = $this->redis->srandmember('set0');
        $this->assertInArray($member, $mems);

        $rmembers = $this->redis->srandmember('set0', $i);
        foreach ($rmembers as $reply_mem) {
            $this->assertInArray($reply_mem, $mems);
        }

        /* Ensure we can handle basically any return type */
        foreach ([3.1415, new stdClass(), 42, 'hello', NULL] as $val) {
            $this->assertEquals(1, $this->redis->del('set0'));
            $this->assertEquals(1, $this->redis->sadd('set0', $val));
            $this->assertSameType($val, $this->redis->srandmember('set0'));
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
        $this->assertEquals([], $ret_pos);
        $this->assertEquals([], $ret_neg);

        // Add a few items to the set
        for ($i = 0; $i < 100; $i++) {
            $this->redis->sadd('set0', "member$i");
        }

        // Get less than the size of the list
        $ret_slice = $this->redis->srandmember('set0', 20);

        // Should be an array with 20 items
        $this->assertIsArray($ret_slice, 20);

        // Ask for more items than are in the list (but with a positive count)
        $ret_slice = $this->redis->srandmember('set0', 200);

        // Should be an array, should be however big the set is, exactly
        $this->assertIsArray($ret_slice, $i);

        // Now ask for too many items but negative
        $ret_slice = $this->redis->srandmember('set0', -200);

        // Should be an array, should have exactly the # of items we asked for (will be dups)
        $this->assertIsArray($ret_slice, 200);

        //
        // Test in a pipeline
        //

        if ($this->havePipeline()) {
            $pipe = $this->redis->pipeline();

            $pipe->srandmember('set0', 20);
            $pipe->srandmember('set0', 200);
            $pipe->srandmember('set0', -200);

            $ret = $this->redis->exec();

            $this->assertIsArray($ret[0], 20);
            $this->assertIsArray($ret[1], $i);
            $this->assertIsArray($ret[2], 200);

            // Kill the set
            $this->redis->del('set0');
        }
    }

    public function testSIsMember() {
        $this->redis->del('set');

        $this->redis->sAdd('set', 'val');

        $this->assertTrue($this->redis->sismember('set', 'val'));
        $this->assertFalse($this->redis->sismember('set', 'val2'));
    }

    public function testSMembers() {
        $this->redis->del('set');

        $data = ['val', 'val2', 'val3'];
        foreach ($data as $member) {
            $this->redis->sAdd('set', $member);
        }

        $this->assertEqualsCanonicalizing($data, $this->redis->smembers('set'));
    }

    public function testsMisMember() {
        if (version_compare($this->version, '6.2.0') < 0)
            $this->markTestSkipped();

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

        $this->assertEquals('val3', $this->redis->lIndex('list', 0));
        $this->assertEquals('val2', $this->redis->lIndex('list', 1));
        $this->assertEquals('val', $this->redis->lIndex('list', 2));

        $this->assertTrue($this->redis->lSet('list', 1, 'valx'));

        $this->assertEquals('val3', $this->redis->lIndex('list', 0));
        $this->assertEquals('valx', $this->redis->lIndex('list', 1));
        $this->assertEquals('val', $this->redis->lIndex('list', 2));
    }

    public function testsInter() {
        $this->redis->del('{set}odd');    // set of odd numbers
        $this->redis->del('{set}prime');  // set of prime numbers
        $this->redis->del('{set}square'); // set of squares
        $this->redis->del('{set}seq');    // set of numbers of the form n^2 - 1

        $x = [1, 3, 5, 7, 9, 11, 13, 15, 17, 19, 21, 23, 25];
        foreach ($x as $i) {
            $this->redis->sAdd('{set}odd', $i);
        }

        $y = [1, 2, 3, 5, 7, 11, 13, 17, 19, 23];
        foreach ($y as $i) {
            $this->redis->sAdd('{set}prime', $i);
        }

        $z = [1, 4, 9, 16, 25];
        foreach ($z as $i) {
            $this->redis->sAdd('{set}square', $i);
        }

        $t = [2, 5, 10, 17, 26];
        foreach ($t as $i) {
            $this->redis->sAdd('{set}seq', $i);
        }

        $xy = $this->redis->sInter('{set}odd', '{set}prime');   // odd prime numbers
        foreach ($xy as $i) {
            $i = (int)$i;
            $this->assertInArray($i, array_intersect($x, $y));
        }

        $xy = $this->redis->sInter(['{set}odd', '{set}prime']);    // odd prime numbers, as array.
        foreach ($xy as $i) {
            $i = (int)$i;
            $this->assertInArray($i, array_intersect($x, $y));
        }

        $yz = $this->redis->sInter('{set}prime', '{set}square');   // set of prime squares
        foreach ($yz as $i) {
            $i = (int)$i;
            $this->assertInArray($i, array_intersect($y, $z));
        }

        $yz = $this->redis->sInter(['{set}prime', '{set}square']);    // set of odd squares, as array
        foreach ($yz as $i) {
        $i = (int)$i;
            $this->assertInArray($i, array_intersect($y, $z));
        }

        $zt = $this->redis->sInter('{set}square', '{set}seq');   // prime squares
        $this->assertEquals([], $zt);
        $zt = $this->redis->sInter(['{set}square', '{set}seq']);    // prime squares, as array
        $this->assertEquals([], $zt);

        $xyz = $this->redis->sInter('{set}odd', '{set}prime', '{set}square');// odd prime squares
        $this->assertEquals(['1'], $xyz);

        $xyz = $this->redis->sInter(['{set}odd', '{set}prime', '{set}square']);// odd prime squares, with an array as a parameter
        $this->assertEquals(['1'], $xyz);

        $nil = $this->redis->sInter([]);
        $this->assertFalse($nil);
    }

    public function testsInterStore() {
        $this->redis->del('{set}x', '{set}y', '{set}z', '{set}t');

        $x = [1, 3, 5, 7, 9, 11, 13, 15, 17, 19, 21, 23, 25];
        foreach ($x as $i) {
            $this->redis->sAdd('{set}x', $i);
        }

        $y = [1, 2, 3, 5, 7, 11, 13, 17, 19, 23];
        foreach ($y as $i) {
            $this->redis->sAdd('{set}y', $i);
        }

        $z = [1, 4, 9, 16, 25];
        foreach ($z as $i) {
            $this->redis->sAdd('{set}z', $i);
        }

        $t = [2, 5, 10, 17, 26];
        foreach ($t as $i) {
            $this->redis->sAdd('{set}t', $i);
        }

        /* Regression test for passing a single array */
        $this->assertEquals(
            count(array_intersect($x,$y)),
            $this->redis->sInterStore(['{set}k', '{set}x', '{set}y'])
        );

        $count = $this->redis->sInterStore('{set}k', '{set}x', '{set}y');  // odd prime numbers
        $this->assertEquals($count, $this->redis->scard('{set}k'));
        foreach (array_intersect($x, $y) as $i) {
            $this->assertTrue($this->redis->sismember('{set}k', $i));
        }

        $count = $this->redis->sInterStore('{set}k', '{set}y', '{set}z');  // set of odd squares
        $this->assertEquals($count, $this->redis->scard('{set}k'));
        foreach (array_intersect($y, $z) as $i) {
            $this->assertTrue($this->redis->sismember('{set}k', $i));
        }

        $count = $this->redis->sInterStore('{set}k', '{set}z', '{set}t');  // squares of the form n^2 + 1
        $this->assertEquals($count, 0);
        $this->assertEquals($count, $this->redis->scard('{set}k'));

        $this->redis->del('{set}z');
        $xyz = $this->redis->sInterStore('{set}k', '{set}x', '{set}y', '{set}z'); // only z missing, expect 0.
        $this->assertEquals(0, $xyz);

        $this->redis->del('{set}y');
        $xyz = $this->redis->sInterStore('{set}k', '{set}x', '{set}y', '{set}z'); // y and z missing, expect 0.
        $this->assertEquals(0, $xyz);

        $this->redis->del('{set}x');
        $xyz = $this->redis->sInterStore('{set}k', '{set}x', '{set}y', '{set}z'); // x y and z ALL missing, expect 0.
        $this->assertEquals(0, $xyz);
    }

    public function testsUnion() {
        $this->redis->del('{set}x');  // set of odd numbers
        $this->redis->del('{set}y');  // set of prime numbers
        $this->redis->del('{set}z');  // set of squares
        $this->redis->del('{set}t');  // set of numbers of the form n^2 - 1

        $x = [1, 3, 5, 7, 9, 11, 13, 15, 17, 19, 21, 23, 25];
        foreach ($x as $i) {
            $this->redis->sAdd('{set}x', $i);
        }

        $y = [1, 2, 3, 5, 7, 11, 13, 17, 19, 23];
        foreach ($y as $i) {
            $this->redis->sAdd('{set}y', $i);
        }

        $z = [1, 4, 9, 16, 25];
        foreach ($z as $i) {
            $this->redis->sAdd('{set}z', $i);
        }

        $t = [2, 5, 10, 17, 26];
        foreach ($t as $i) {
            $this->redis->sAdd('{set}t', $i);
        }

        $xy = $this->redis->sUnion('{set}x', '{set}y');   // x U y
        foreach ($xy as $i) {
            $this->assertInArray($i, array_merge($x, $y));
        }

        $yz = $this->redis->sUnion('{set}y', '{set}z');   // y U Z
        foreach ($yz as $i) {
        $i = (int)$i;
            $this->assertInArray($i, array_merge($y, $z));
        }

        $zt = $this->redis->sUnion('{set}z', '{set}t');   // z U t
        foreach ($zt as $i) {
        $i = (int)$i;
            $this->assertInArray($i, array_merge($z, $t));
        }

        $xyz = $this->redis->sUnion('{set}x', '{set}y', '{set}z'); // x U y U z
        foreach ($xyz as $i) {
            $this->assertInArray($i, array_merge($x, $y, $z));
        }
    }

    public function testsUnionStore() {
        $this->redis->del('{set}x', '{set}y', '{set}z', '{set}t');

        $x = [1, 3, 5, 7, 9, 11, 13, 15, 17, 19, 21, 23, 25];
        foreach ($x as $i) {
            $this->redis->sAdd('{set}x', $i);
        }

        $y = [1, 2, 3, 5, 7, 11, 13, 17, 19, 23];
        foreach ($y as $i) {
            $this->redis->sAdd('{set}y', $i);
        }

        $z = [1, 4, 9, 16, 25];
        foreach ($z as $i) {
            $this->redis->sAdd('{set}z', $i);
        }

        $t = [2, 5, 10, 17, 26];
        foreach ($t as $i) {
            $this->redis->sAdd('{set}t', $i);
        }

        $count = $this->redis->sUnionStore('{set}k', '{set}x', '{set}y');  // x U y
        $xy = array_unique(array_merge($x, $y));
        $this->assertEquals($count, count($xy));
        foreach ($xy as $i) {
        $i = (int)$i;
            $this->assertTrue($this->redis->sismember('{set}k', $i));
        }

        $count = $this->redis->sUnionStore('{set}k', '{set}y', '{set}z');  // y U z
        $yz = array_unique(array_merge($y, $z));
        $this->assertEquals($count, count($yz));
        foreach ($yz as $i) {
            $this->assertTrue($this->redis->sismember('{set}k', $i));
        }

        $count = $this->redis->sUnionStore('{set}k', '{set}z', '{set}t');  // z U t
        $zt = array_unique(array_merge($z, $t));
        $this->assertEquals($count, count($zt));
        foreach ($zt as $i) {
            $this->assertTrue($this->redis->sismember('{set}k', $i));
        }

        $count = $this->redis->sUnionStore('{set}k', '{set}x', '{set}y', '{set}z'); // x U y U z
        $xyz = array_unique(array_merge($x, $y, $z));
        $this->assertEquals($count, count($xyz));
        foreach ($xyz as $i) {
            $this->assertTrue($this->redis->sismember('{set}k', $i));
        }

        $this->redis->del('{set}x');  // x missing now
        $count = $this->redis->sUnionStore('{set}k', '{set}x', '{set}y', '{set}z'); // x U y U z
        $this->assertEquals($count, count(array_unique(array_merge($y, $z))));

        $this->redis->del('{set}y');  // x and y missing
        $count = $this->redis->sUnionStore('{set}k', '{set}x', '{set}y', '{set}z'); // x U y U z
        $this->assertEquals($count, count(array_unique($z)));

        $this->redis->del('{set}z');  // x, y, and z ALL missing
        $count = $this->redis->sUnionStore('{set}k', '{set}x', '{set}y', '{set}z'); // x U y U z
        $this->assertEquals(0, $count);
    }

    public function testsDiff() {
        $this->redis->del('{set}x');  // set of odd numbers
        $this->redis->del('{set}y');  // set of prime numbers
        $this->redis->del('{set}z');  // set of squares
        $this->redis->del('{set}t');  // set of numbers of the form n^2 - 1

        $x = [1, 3, 5, 7, 9, 11, 13, 15, 17, 19, 21, 23, 25];
        foreach ($x as $i) {
            $this->redis->sAdd('{set}x', $i);
        }

        $y = [1, 2, 3, 5, 7, 11, 13, 17, 19, 23];
        foreach ($y as $i) {
            $this->redis->sAdd('{set}y', $i);
        }

        $z = [1, 4, 9, 16, 25];
        foreach ($z as $i) {
            $this->redis->sAdd('{set}z', $i);
        }

        $t = [2, 5, 10, 17, 26];
        foreach ($t as $i) {
            $this->redis->sAdd('{set}t', $i);
        }

        $xy = $this->redis->sDiff('{set}x', '{set}y');    // x U y
        foreach ($xy as $i) {
        $i = (int)$i;
            $this->assertInArray($i, array_diff($x, $y));
        }

        $yz = $this->redis->sDiff('{set}y', '{set}z');    // y U Z
        foreach ($yz as $i) {
        $i = (int)$i;
            $this->assertInArray($i, array_diff($y, $z));
        }

        $zt = $this->redis->sDiff('{set}z', '{set}t');    // z U t
        foreach ($zt as $i) {
        $i = (int)$i;
            $this->assertInArray($i, array_diff($z, $t));
        }

        $xyz = $this->redis->sDiff('{set}x', '{set}y', '{set}z'); // x U y U z
        foreach ($xyz as $i) {
        $i = (int)$i;
            $this->assertInArray($i, array_diff($x, $y, $z));
        }
    }

    public function testsDiffStore() {
        $this->redis->del('{set}x', '{set}y', '{set}z', '{set}t');

        $x = [1, 3, 5, 7, 9, 11, 13, 15, 17, 19, 21, 23, 25];
        foreach ($x as $i) {
            $this->redis->sAdd('{set}x', $i);
        }

        $y = [1, 2, 3, 5, 7, 11, 13, 17, 19, 23];
        foreach ($y as $i) {
            $this->redis->sAdd('{set}y', $i);
        }

        $z = [1, 4, 9, 16, 25];
        foreach ($z as $i) {
            $this->redis->sAdd('{set}z', $i);
        }

        $t = [2, 5, 10, 17, 26];
        foreach ($t as $i) {
            $this->redis->sAdd('{set}t', $i);
        }

        $count = $this->redis->sDiffStore('{set}k', '{set}x', '{set}y');   // x - y
        $xy = array_unique(array_diff($x, $y));
        $this->assertEquals($count, count($xy));
        foreach ($xy as $i) {
            $this->assertTrue($this->redis->sismember('{set}k', $i));
        }

        $count = $this->redis->sDiffStore('{set}k', '{set}y', '{set}z');   // y - z
        $yz = array_unique(array_diff($y, $z));
        $this->assertEquals($count, count($yz));
        foreach ($yz as $i) {
            $this->assertTrue($this->redis->sismember('{set}k', $i));
        }

        $count = $this->redis->sDiffStore('{set}k', '{set}z', '{set}t');   // z - t
        $zt = array_unique(array_diff($z, $t));
        $this->assertEquals($count, count($zt));
        foreach ($zt as $i) {
            $this->assertTrue($this->redis->sismember('{set}k', $i));
        }

        $count = $this->redis->sDiffStore('{set}k', '{set}x', '{set}y', '{set}z');  // x - y - z
        $xyz = array_unique(array_diff($x, $y, $z));
        $this->assertEquals($count, count($xyz));
        foreach ($xyz as $i) {
            $this->assertTrue($this->redis->sismember('{set}k', $i));
        }

        $this->redis->del('{set}x');  // x missing now
        $count = $this->redis->sDiffStore('{set}k', '{set}x', '{set}y', '{set}z');  // x - y - z
        $this->assertEquals(0, $count);

        $this->redis->del('{set}y');  // x and y missing
        $count = $this->redis->sDiffStore('{set}k', '{set}x', '{set}y', '{set}z');  // x - y - z
        $this->assertEquals(0, $count);

        $this->redis->del('{set}z');  // x, y, and z ALL missing
        $count = $this->redis->sDiffStore('{set}k', '{set}x', '{set}y', '{set}z');  // x - y - z
        $this->assertEquals(0, $count);
    }

    public function testInterCard() {
        if (version_compare($this->version, '7.0.0') < 0)
            $this->markTestSkipped();

        $set_data = [
            ['aardvark', 'dog', 'fish', 'squirrel', 'tiger'],
            ['bear', 'coyote', 'fish', 'gorilla', 'dog']
        ];

        $ssets = $zsets = [];

        foreach ($set_data as $n => $values) {
            $sset = "s{set}:$n";
            $zset = "z{set}:$n";

            $this->redis->del([$sset, $zset]);

            $ssets[] = $sset;
            $zsets[] = $zset;

            foreach ($values as $score => $value) {
                $this->assertEquals(1, $this->redis->sAdd("s{set}:$n", $value));
                $this->assertEquals(1, $this->redis->zAdd("z{set}:$n", $score, $value));
            }
        }

        $exp = count(array_intersect(...$set_data));

        $act = $this->redis->sintercard($ssets);
        $this->assertEquals($exp, $act);
        $act = $this->redis->zintercard($zsets);
        $this->assertEquals($exp, $act);

        $this->assertEquals(1, $this->redis->sintercard($ssets, 1));
        $this->assertEquals(2, $this->redis->sintercard($ssets, 2));

        $this->assertEquals(1, $this->redis->zintercard($zsets, 1));
        $this->assertEquals(2, $this->redis->zintercard($zsets, 2));

        $this->assertFalse(@$this->redis->sintercard($ssets, -1));
        $this->assertFalse(@$this->redis->zintercard($ssets, -1));

        $this->assertFalse(@$this->redis->sintercard([]));
        $this->assertFalse(@$this->redis->zintercard([]));

        $this->redis->del(array_merge($ssets, $zsets));
    }

    public function testLRange() {
        $this->redis->del('list');
        $this->redis->lPush('list', 'val');
        $this->redis->lPush('list', 'val2');
        $this->redis->lPush('list', 'val3');

        $this->assertEquals(['val3'], $this->redis->lrange('list', 0, 0));
        $this->assertEquals(['val3', 'val2'], $this->redis->lrange('list', 0, 1));
        $this->assertEquals(['val3', 'val2', 'val'], $this->redis->lrange('list', 0, 2));
        $this->assertEquals(['val3', 'val2', 'val'], $this->redis->lrange('list', 0, 3));

        $this->assertEquals(['val3', 'val2', 'val'], $this->redis->lrange('list', 0, -1));
        $this->assertEquals(['val3', 'val2'], $this->redis->lrange('list', 0, -2));
        $this->assertEquals(['val2', 'val'], $this->redis->lrange('list', -2, -1));

        $this->redis->del('list');
        $this->assertEquals([], $this->redis->lrange('list', 0, -1));
    }

    public function testdbSize() {
        $this->assertTrue($this->redis->flushDB());
        $this->redis->set('x', 'y');
        $this->assertEquals(1, $this->redis->dbSize());
    }

    public function testFlushDB() {
        $this->assertTrue($this->redis->flushdb());
        $this->assertTrue($this->redis->flushdb(NULL));
        $this->assertTrue($this->redis->flushdb(false));
        $this->assertTrue($this->redis->flushdb(true));
    }

    public function testTTL() {
        $this->redis->set('x', 'y');
        $this->redis->expire('x', 5);
        $ttl = $this->redis->ttl('x');
        $this->assertBetween($ttl, 1, 5);

        // A key with no TTL
        $this->redis->del('x'); $this->redis->set('x', 'bar');
        $this->assertEquals(-1, $this->redis->ttl('x'));

        // A key that doesn't exist (> 2.8 will return -2)
        if (version_compare($this->version, '2.8.0') >= 0) {
            $this->redis->del('x');
            $this->assertEquals(-2, $this->redis->ttl('x'));
        }
    }

    public function testPersist() {
        $this->redis->set('x', 'y');
        $this->redis->expire('x', 100);
        $this->assertTrue($this->redis->persist('x'));     // true if there is a timeout
        $this->assertEquals(-1, $this->redis->ttl('x'));       // -1: timeout has been removed.
        $this->assertFalse($this->redis->persist('x'));    // false if there is no timeout
        $this->redis->del('x');
        $this->assertFalse($this->redis->persist('x'));    // false if the key doesn’t exist.
    }

    public function testClient() {
        /* CLIENT SETNAME */
        $this->assertTrue($this->redis->client('setname', 'phpredis_unit_tests'));

        /* CLIENT LIST */
        $clients = $this->redis->client('list');
        $this->assertIsArray($clients);

        // Figure out which ip:port is us!
        $address = NULL;
        foreach ($clients as $client) {
            if ($client['name'] == 'phpredis_unit_tests') {
                $address = $client['addr'];
            }
        }

        // We should have found our connection
        $this->assertIsString($address);

        /* CLIENT GETNAME */
        $this->assertEquals('phpredis_unit_tests', $this->redis->client('getname'));

        if (version_compare($this->version, '5.0.0') >= 0) {
            $this->assertGT(0, $this->redis->client('id'));
            if (version_compare($this->version, '6.0.0') >= 0) {
                $this->assertEquals(-1, $this->redis->client('getredir'));
                $this->assertTrue($this->redis->client('tracking', 'on', ['optin' => true]));
                $this->assertEquals(0, $this->redis->client('getredir'));
                $this->assertTrue($this->redis->client('caching', 'yes'));
                $this->assertTrue($this->redis->client('tracking', 'off'));
                if (version_compare($this->version, '6.2.0') >= 0) {
                    $this->assertFalse(empty($this->redis->client('info')));
                    $this->assertEquals([
                        'flags' => ['off'],
                        'redirect' => -1,
                        'prefixes' => [],
                    ], $this->redis->client('trackinginfo'));

                    if (version_compare($this->version, '7.0.0') >= 0) {
                        $this->assertTrue($this->redis->client('no-evict', 'on'));
                    }
                }
            }
        }

        /* CLIENT KILL -- phpredis will reconnect, so we can do this */
        $this->assertTrue($this->redis->client('kill', $address));

    }

    public function testSlowlog() {
        // We don't really know what's going to be in the slowlog, but make sure
        // the command returns proper types when called in various ways
        $this->assertIsArray($this->redis->slowlog('get'));
        $this->assertIsArray($this->redis->slowlog('get', 10));
        $this->assertIsInt($this->redis->slowlog('len'));
        $this->assertTrue($this->redis->slowlog('reset'));
        $this->assertFalse(@$this->redis->slowlog('notvalid'));
    }

    public function testWait() {
        // Closest we can check based on redis commit history
        if (version_compare($this->version, '2.9.11') < 0)
            $this->markTestSkipped();

        // We could have slaves here, so determine that
        $info     = $this->redis->info();
        $replicas = $info['connected_slaves'];

        // Send a couple commands
        $this->redis->set('wait-foo', 'over9000');
        $this->redis->set('wait-bar', 'revo9000');

        // Make sure we get the right replication count
        $this->assertEquals($replicas, $this->redis->wait($replicas, 100));

        // Pass more slaves than are connected
        $this->redis->set('wait-foo', 'over9000');
        $this->redis->set('wait-bar', 'revo9000');
        $this->assertLT($replicas + 1, $this->redis->wait($replicas + 1, 100));

        // Make sure when we pass with bad arguments we just get back false
        $this->assertFalse($this->redis->wait(-1, -1));
        $this->assertEquals(0, $this->redis->wait(-1, 20));
    }

    public function testInfo() {
        $sequence = [false];
        if ($this->haveMulti())
            $sequence[] = true;

        foreach ($sequence as $boo_multi) {
            if ($boo_multi) {
                $this->redis->multi();
                $this->redis->info();
                $info = $this->redis->exec();
                $info = $info[0];
            } else {
                $info = $this->redis->info();
            }

            $keys = [
                'redis_version',
                'arch_bits',
                'uptime_in_seconds',
                'uptime_in_days',
                'connected_clients',
                'connected_slaves',
                'used_memory',
                'total_connections_received',
                'total_commands_processed',
                'role'
            ];
            if (version_compare($this->version, '2.5.0') < 0) {
                array_push($keys,
                    'changes_since_last_save',
                    'bgsave_in_progress',
                    'last_save_time'
                );
            } else {
                array_push($keys,
                    'rdb_changes_since_last_save',
                    'rdb_bgsave_in_progress',
                    'rdb_last_save_time'
                );
            }

            foreach ($keys as $k) {
                $this->assertInArray($k, array_keys($info));
            }
        }

        if ( ! $this->minVersionCheck('7.0.0'))
            return;

        $res = $this->redis->info('server', 'memory');
        $this->assertTrue(is_array($res) && isset($res['redis_version']) && isset($res['used_memory']));
    }

    public function testInfoCommandStats() {
        // INFO COMMANDSTATS is new in 2.6.0
        if (version_compare($this->version, '2.5.0') < 0)
            $this->markTestSkipped();

        $info = $this->redis->info('COMMANDSTATS');
        if ( ! $this->assertIsArray($info))
            return;

        foreach ($info as $k => $value) {
            $this->assertStringContains('cmdstat_', $k);
        }
    }

    public function testSelect() {
        $this->assertFalse(@$this->redis->select(-1));
        $this->assertTrue($this->redis->select(0));
    }

    public function testSwapDB() {
        if (version_compare($this->version, '4.0.0') < 0)
            $this->markTestSkipped();

        $this->assertTrue($this->redis->swapdb(0, 1));
        $this->assertTrue($this->redis->swapdb(0, 1));
    }

    public function testMset() {
        $this->redis->del('x', 'y', 'z');    // remove x y z
        $this->assertTrue($this->redis->mset(['x' => 'a', 'y' => 'b', 'z' => 'c']));   // set x y z

        $this->assertEquals(['a', 'b', 'c'], $this->redis->mget(['x', 'y', 'z']));    // check x y z

        $this->redis->del('x');  // delete just x
        $this->assertTrue($this->redis->mset(['x' => 'a', 'y' => 'b', 'z' => 'c']));   // set x y z
        $this->assertEquals(['a', 'b', 'c'], $this->redis->mget(['x', 'y', 'z']));    // check x y z

        $this->assertFalse($this->redis->mset([])); // set ø → FALSE

        /*
         * Integer keys
         */

        // No prefix
        $set_array = [-1 => 'neg1', -2 => 'neg2', -3 => 'neg3', 1 => 'one', 2 => 'two', '3' => 'three'];
        $this->redis->del(array_keys($set_array));
        $this->assertTrue($this->redis->mset($set_array));
        $this->assertEquals(array_values($set_array), $this->redis->mget(array_keys($set_array)));
        $this->redis->del(array_keys($set_array));

        // With a prefix
        $this->redis->setOption(Redis::OPT_PREFIX, 'pfx:');
        $this->redis->del(array_keys($set_array));
        $this->assertTrue($this->redis->mset($set_array));
        $this->assertEquals(array_values($set_array), $this->redis->mget(array_keys($set_array)));
        $this->redis->del(array_keys($set_array));
        $this->redis->setOption(Redis::OPT_PREFIX, '');
    }

    public function testMsetNX() {
        $this->redis->del('x', 'y', 'z');    // remove x y z
        $this->assertTrue($this->redis->msetnx(['x' => 'a', 'y' => 'b', 'z' => 'c']));    // set x y z

        $this->assertEquals(['a', 'b', 'c'], $this->redis->mget(['x', 'y', 'z']));    // check x y z

        $this->redis->del('x');  // delete just x
        $this->assertFalse($this->redis->msetnx(['x' => 'A', 'y' => 'B', 'z' => 'C']));   // set x y z
        $this->assertEquals([FALSE, 'b', 'c'], $this->redis->mget(['x', 'y', 'z']));  // check x y z

        $this->assertFalse($this->redis->msetnx([])); // set ø → FALSE
    }

    public function testRpopLpush() {
        // standard case.
        $this->redis->del('{list}x', '{list}y');
        $this->redis->lpush('{list}x', 'abc');
        $this->redis->lpush('{list}x', 'def');    // x = [def, abc]

        $this->redis->lpush('{list}y', '123');
        $this->redis->lpush('{list}y', '456');    // y = [456, 123]

        $this->assertEquals('abc', $this->redis->rpoplpush('{list}x', '{list}y'));  // we RPOP x, yielding abc.
        $this->assertEquals(['def'], $this->redis->lrange('{list}x', 0, -1)); // only def remains in x.
        $this->assertEquals(['abc', '456', '123'], $this->redis->lrange('{list}y', 0, -1));   // abc has been lpushed to y.

        // with an empty source, expecting no change.
        $this->redis->del('{list}x', '{list}y');
        $this->assertFalse($this->redis->rpoplpush('{list}x', '{list}y'));
        $this->assertEquals([], $this->redis->lrange('{list}x', 0, -1));
        $this->assertEquals([], $this->redis->lrange('{list}y', 0, -1));
    }

    public function testBRpopLpush() {
        // standard case.
        $this->redis->del('{list}x', '{list}y');
        $this->redis->lpush('{list}x', 'abc');
        $this->redis->lpush('{list}x', 'def');    // x = [def, abc]

        $this->redis->lpush('{list}y', '123');
        $this->redis->lpush('{list}y', '456');    // y = [456, 123]

        $this->assertEquals('abc', $this->redis->brpoplpush('{list}x', '{list}y', 1));  // we RPOP x, yielding abc.

        $this->assertEquals(['def'], $this->redis->lrange('{list}x', 0, -1)); // only def remains in x.
        $this->assertEquals(['abc', '456', '123'], $this->redis->lrange('{list}y', 0, -1));   // abc has been lpushed to y.

        // with an empty source, expecting no change.
        $this->redis->del('{list}x', '{list}y');
        $this->assertFalse($this->redis->brpoplpush('{list}x', '{list}y', 1));
        $this->assertEquals([], $this->redis->lrange('{list}x', 0, -1));
        $this->assertEquals([], $this->redis->lrange('{list}y', 0, -1));

        if ( ! $this->minVersionCheck('6.0.0'))
            return;

        // Redis >= 6.0.0 allows floating point timeouts
        $st = microtime(true);
        $this->assertFalse($this->redis->brpoplpush('{list}x', '{list}y', .1));
        $et = microtime(true);
        $this->assertLT(1.0, $et - $st);
    }

    public function testZAddFirstArg() {
        $this->redis->del('key');

        $zsetName = 100; // not a string!
        $this->assertEquals(1, $this->redis->zAdd($zsetName, 0, 'val0'));
        $this->assertEquals(1, $this->redis->zAdd($zsetName, 1, 'val1'));

        $this->assertEquals(['val0', 'val1'], $this->redis->zRange($zsetName, 0, -1));
    }

    public function testZaddIncr() {
        $this->redis->del('zset');

        $this->assertEquals(10.0, $this->redis->zAdd('zset', ['incr'], 10, 'value'));
        $this->assertEquals(20.0, $this->redis->zAdd('zset', ['incr'], 10, 'value'));

        $this->assertFalse($this->redis->zAdd('zset', ['incr'], 10, 'value', 20, 'value2'));
    }

    public function testZX() {
        $this->redis->del('key');

        $this->assertEquals([], $this->redis->zRange('key', 0, -1));
        $this->assertEquals([], $this->redis->zRange('key', 0, -1, true));

        $this->assertEquals(1, $this->redis->zAdd('key', 0, 'val0'));
        $this->assertEquals(1, $this->redis->zAdd('key', 2, 'val2'));
        $this->assertEquals(2, $this->redis->zAdd('key', 4, 'val4', 5, 'val5')); // multiple parameters
        if (version_compare($this->version, '3.0.2') < 0) {
            $this->assertEquals(1, $this->redis->zAdd('key', 1, 'val1'));
            $this->assertEquals(1, $this->redis->zAdd('key', 3, 'val3'));
        } else {
            $this->assertEquals(1, $this->redis->zAdd('key', [], 1, 'val1')); // empty options
            $this->assertEquals(1, $this->redis->zAdd('key', ['nx'], 3, 'val3')); // nx option
            $this->assertEquals(0, $this->redis->zAdd('key', ['xx'], 3, 'val3')); // xx option

            if (version_compare($this->version, '6.2.0') >= 0) {
                $this->assertEquals(0, $this->redis->zAdd('key', ['lt'], 4, 'val3')); // lt option
                $this->assertEquals(0, $this->redis->zAdd('key', ['gt'], 2, 'val3')); // gt option
            }
        }

        $this->assertEquals(['val0', 'val1', 'val2', 'val3', 'val4', 'val5'], $this->redis->zRange('key', 0, -1));

        // withscores
        $ret = $this->redis->zRange('key', 0, -1, true);
        $this->assertEquals(6, count($ret));
        $this->assertEquals(0.0, $ret['val0']);
        $this->assertEquals(1.0, $ret['val1']);
        $this->assertEquals(2.0, $ret['val2']);
        $this->assertEquals(3.0, $ret['val3']);
        $this->assertEquals(4.0, $ret['val4']);
        $this->assertEquals(5.0, $ret['val5']);

        $this->assertEquals(0, $this->redis->zRem('key', 'valX'));
        $this->assertEquals(1, $this->redis->zRem('key', 'val3'));
        $this->assertEquals(1, $this->redis->zRem('key', 'val4'));
        $this->assertEquals(1, $this->redis->zRem('key', 'val5'));

        $this->assertEquals(['val0', 'val1', 'val2'], $this->redis->zRange('key', 0, -1));

        // zGetReverseRange

        $this->assertEquals(1, $this->redis->zAdd('key', 3, 'val3'));
        $this->assertEquals(1, $this->redis->zAdd('key', 3, 'aal3'));

        $zero_to_three = $this->redis->zRangeByScore('key', 0, 3);
        $this->assertEquals(['val0', 'val1', 'val2', 'aal3', 'val3'], $zero_to_three);

        $three_to_zero = $this->redis->zRevRangeByScore('key', 3, 0);
        $this->assertEquals(array_reverse(['val0', 'val1', 'val2', 'aal3', 'val3']), $three_to_zero);

        $this->assertEquals(5, $this->redis->zCount('key', 0, 3));

        // withscores
        $this->redis->zRem('key', 'aal3');
        $zero_to_three = $this->redis->zRangeByScore('key', 0, 3, ['withscores' => true]);
        $this->assertEquals(['val0' => 0.0, 'val1' => 1.0, 'val2' => 2.0, 'val3' => 3.0], $zero_to_three);
        $this->assertEquals(4, $this->redis->zCount('key', 0, 3));

        // limit
        $this->assertEquals(['val0'], $this->redis->zRangeByScore('key', 0, 3, ['limit' => [0, 1]]));
        $this->assertEquals(['val0', 'val1'],
                            $this->redis->zRangeByScore('key', 0, 3, ['limit' => [0, 2]]));
        $this->assertEquals(['val1', 'val2'],
                            $this->redis->zRangeByScore('key', 0, 3, ['limit' => [1, 2]]));
        $this->assertEquals(['val0', 'val1'],
                            $this->redis->zRangeByScore('key', 0, 1, ['limit' => [0, 100]]));

        if ($this->minVersionCheck('6.2.0'))
            $this->assertEquals(['val0', 'val1'], $this->redis->zrange('key', 0, 1, ['byscore', 'limit' => [0, 100]]));

        // limits as references
        $limit = [0, 100];
        foreach ($limit as &$val) {}
        $this->assertEquals(['val0', 'val1'], $this->redis->zRangeByScore('key', 0, 1, ['limit' => $limit]));

        $this->assertEquals(
            ['val3'], $this->redis->zRevRangeByScore('key', 3, 0, ['limit' => [0, 1]])
        );
        $this->assertEquals(
            ['val3', 'val2'], $this->redis->zRevRangeByScore('key', 3, 0, ['limit' => [0, 2]])
        );
        $this->assertEquals(
            ['val2', 'val1'], $this->redis->zRevRangeByScore('key', 3, 0, ['limit' => [1, 2]])
        );
        $this->assertEquals(
            ['val1', 'val0'], $this->redis->zRevRangeByScore('key', 1, 0, ['limit' => [0, 100]])
        );

        if ($this->minVersionCheck('6.2.0')) {
            $this->assertEquals(['val1', 'val0'],
                                $this->redis->zrange('key', 1, 0, ['byscore', 'rev', 'limit' => [0, 100]]));
            $this->assertEquals(2, $this->redis->zrangestore('dst{key}', 'key', 1, 0,
                                ['byscore', 'rev', 'limit' => [0, 100]]));
            $this->assertEquals(['val0', 'val1'], $this->redis->zRange('dst{key}', 0, -1));

            $this->assertEquals(1, $this->redis->zrangestore('dst{key}', 'key', 1, 0,
                                ['byscore', 'rev', 'limit' => [0, 1]]));
            $this->assertEquals(['val1'], $this->redis->zrange('dst{key}', 0, -1));
        }

        $this->assertEquals(4, $this->redis->zCard('key'));
        $this->assertEquals(1.0, $this->redis->zScore('key', 'val1'));
        $this->assertFalse($this->redis->zScore('key', 'val'));
        $this->assertFalse($this->redis->zScore(3, 2));

        // with () and +inf, -inf
        $this->redis->del('zset');
        $this->redis->zAdd('zset', 1, 'foo');
        $this->redis->zAdd('zset', 2, 'bar');
        $this->redis->zAdd('zset', 3, 'biz');
        $this->redis->zAdd('zset', 4, 'foz');
        $this->assertEquals(
            ['foo' => 1.0, 'bar' => 2.0, 'biz' => 3.0, 'foz' => 4.0],
            $this->redis->zRangeByScore('zset', '-inf', '+inf', ['withscores' => true])
        );
        $this->assertEquals(
            ['foo' => 1.0, 'bar' => 2.0],
            $this->redis->zRangeByScore('zset', 1, 2, ['withscores' => true])
        );
        $this->assertEquals(
            ['bar' => 2.0],
            $this->redis->zRangeByScore('zset', '(1', 2, ['withscores' => true])
        );
        $this->assertEquals([], $this->redis->zRangeByScore('zset', '(1', '(2', ['withscores' => true]));

        $this->assertEquals(4, $this->redis->zCount('zset', '-inf', '+inf'));
        $this->assertEquals(2, $this->redis->zCount('zset', 1, 2));
        $this->assertEquals(1, $this->redis->zCount('zset', '(1', 2));
        $this->assertEquals(0, $this->redis->zCount('zset', '(1', '(2'));

        // zincrby
        $this->redis->del('key');
        $this->assertEquals(1.0, $this->redis->zIncrBy('key', 1, 'val1'));
        $this->assertEquals(1.0, $this->redis->zScore('key', 'val1'));
        $this->assertEquals(2.5, $this->redis->zIncrBy('key', 1.5, 'val1'));
        $this->assertEquals(2.5, $this->redis->zScore('key', 'val1'));

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

        $this->assertEquals(4, $this->redis->zUnionStore('{zset}U', ['{zset}1', '{zset}3']));
        $this->assertEquals(['val0', 'val1', 'val4', 'val5'], $this->redis->zRange('{zset}U', 0, -1));

        // Union on non existing keys
        $this->redis->del('{zset}U');
        $this->assertEquals(0, $this->redis->zUnionStore('{zset}U', ['{zset}X', '{zset}Y']));
        $this->assertEquals([],$this->redis->zRange('{zset}U', 0, -1));

        // !Exist U Exist → copy of existing zset.
        $this->redis->del('{zset}U', 'X');
        $this->assertEquals(2, $this->redis->zUnionStore('{zset}U', ['{zset}1', '{zset}X']));

        // test weighted zUnion
        $this->redis->del('{zset}Z');
        $this->assertEquals(4, $this->redis->zUnionStore('{zset}Z', ['{zset}1', '{zset}2'], [1, 1]));
        $this->assertEquals(['val0', 'val1', 'val2', 'val3'], $this->redis->zRange('{zset}Z', 0, -1));

        $this->redis->zRemRangeByScore('{zset}Z', 0, 10);
        $this->assertEquals(4, $this->redis->zUnionStore('{zset}Z', ['{zset}1', '{zset}2'], [5, 1]));
        $this->assertEquals(['val0', 'val2', 'val3', 'val1'], $this->redis->zRange('{zset}Z', 0, -1));

        $this->redis->del('{zset}1');
        $this->redis->del('{zset}2');
        $this->redis->del('{zset}3');

        //test zUnion with weights and aggegration function
        $this->redis->zadd('{zset}1', 1, 'duplicate');
        $this->redis->zadd('{zset}2', 2, 'duplicate');
        $this->redis->zUnionStore('{zset}U', ['{zset}1', '{zset}2'], [1, 1], 'MIN');
        $this->assertEquals(1.0, $this->redis->zScore('{zset}U', 'duplicate'));
        $this->redis->del('{zset}U');

        //now test zUnion *without* weights but with aggregate function
        $this->redis->zUnionStore('{zset}U', ['{zset}1', '{zset}2'], null, 'MIN');
        $this->assertEquals(1.0, $this->redis->zScore('{zset}U', 'duplicate'));
        $this->redis->del('{zset}U', '{zset}1', '{zset}2');

        // test integer and float weights (GitHub issue #109).
        $this->redis->del('{zset}1', '{zset}2', '{zset}3');

        $this->redis->zadd('{zset}1', 1, 'one');
        $this->redis->zadd('{zset}1', 2, 'two');
        $this->redis->zadd('{zset}2', 1, 'one');
        $this->redis->zadd('{zset}2', 2, 'two');
        $this->redis->zadd('{zset}2', 3, 'three');

        $this->assertEquals(3, $this->redis->zUnionStore('{zset}3', ['{zset}1', '{zset}2'], [2, 3.0]));

        $this->redis->del('{zset}1');
        $this->redis->del('{zset}2');
        $this->redis->del('{zset}3');

        // Test 'inf', '-inf', and '+inf' weights (GitHub issue #336)
        $this->redis->zadd('{zset}1', 1, 'one', 2, 'two', 3, 'three');
        $this->redis->zadd('{zset}2', 3, 'three', 4, 'four', 5, 'five');

        // Make sure phpredis handles these weights
        $this->assertEquals(5, $this->redis->zUnionStore('{zset}3', ['{zset}1', '{zset}2'], [1, 'inf']) );
        $this->assertEquals(5, $this->redis->zUnionStore('{zset}3', ['{zset}1', '{zset}2'], [1, '-inf']));
        $this->assertEquals(5, $this->redis->zUnionStore('{zset}3', ['{zset}1', '{zset}2'], [1, '+inf']));

        // Now, confirm that they're being sent, and that it works
        $weights = ['inf', '-inf', '+inf'];

        foreach ($weights as $weight) {
            $r = $this->redis->zUnionStore('{zset}3', ['{zset}1', '{zset}2'], [1, $weight]);
            $this->assertEquals(5, $r);
            $r = $this->redis->zrangebyscore('{zset}3', '(-inf', '(inf',['withscores'=>true]);
            $this->assertEquals(2, count($r));
            $this->assertArrayKey($r, 'one');
            $this->assertArrayKey($r, 'two');
        }

        $this->redis->del('{zset}1', '{zset}2', '{zset}3');

        $this->redis->zadd('{zset}1', 2000.1, 'one');
        $this->redis->zadd('{zset}1', 3000.1, 'two');
        $this->redis->zadd('{zset}1', 4000.1, 'three');

        $ret = $this->redis->zRange('{zset}1', 0, -1, true);
        $this->assertEquals(3, count($ret));
        $retValues = array_keys($ret);

        $this->assertEquals(['one', 'two', 'three'], $retValues);

        // + 0 converts from string to float OR integer
        $this->assertArrayKeyEquals($ret, 'one', 2000.1);
        $this->assertArrayKeyEquals($ret, 'two', 3000.1);
        $this->assertArrayKeyEquals($ret, 'three', 4000.1);

        $this->redis->del('{zset}1');

        // ZREMRANGEBYRANK
        $this->redis->zAdd('{zset}1', 1, 'one');
        $this->redis->zAdd('{zset}1', 2, 'two');
        $this->redis->zAdd('{zset}1', 3, 'three');
        $this->assertEquals(2, $this->redis->zremrangebyrank('{zset}1', 0, 1));
        $this->assertEquals(['three' => 3.], $this->redis->zRange('{zset}1', 0, -1, true));

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
        $this->assertEquals(2, $this->redis->zInterStore('{zset}I', ['{zset}1', '{zset}2']));
        $this->assertEquals(['val1', 'val3'], $this->redis->zRange('{zset}I', 0, -1));

        // Union on non existing keys
        $this->assertEquals(0, $this->redis->zInterStore('{zset}X', ['{zset}X', '{zset}Y']));
        $this->assertEquals([], $this->redis->zRange('{zset}X', 0, -1));

        // !Exist U Exist
        $this->assertEquals(0, $this->redis->zInterStore('{zset}Y', ['{zset}1', '{zset}X']));
        $this->assertEquals([], $this->redis->zRange('keyY', 0, -1));


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
        $this->assertEquals(2, $this->redis->zInterStore('{zset}I', ['{zset}1', '{zset}2'], [1, 1]));
        $this->assertEquals(['val1', 'val3'], $this->redis->zRange('{zset}I', 0, -1));

        $this->redis->del('{zset}I');
        $this->assertEquals(2, $this->redis->zInterStore('{zset}I', ['{zset}1', '{zset}2', '{zset}3'], [1, 5, 1], 'min'));
        $this->assertEquals(['val1', 'val3'], $this->redis->zRange('{zset}I', 0, -1));
        $this->redis->del('{zset}I');
        $this->assertEquals(2, $this->redis->zInterStore('{zset}I', ['{zset}1', '{zset}2', '{zset}3'], [1, 5, 1], 'max'));
        $this->assertEquals(['val3', 'val1'], $this->redis->zRange('{zset}I', 0, -1));

        $this->redis->del('{zset}I');
        $this->assertEquals(2, $this->redis->zInterStore('{zset}I', ['{zset}1', '{zset}2', '{zset}3'], null, 'max'));
        $this->assertEquals(7., $this->redis->zScore('{zset}I', 'val1'));

        // zrank, zrevrank
        $this->redis->del('z');
        $this->redis->zadd('z', 1, 'one');
        $this->redis->zadd('z', 2, 'two');
        $this->redis->zadd('z', 5, 'five');

        $this->assertEquals(0, $this->redis->zRank('z', 'one'));
        $this->assertEquals(1, $this->redis->zRank('z', 'two'));
        $this->assertEquals(2, $this->redis->zRank('z', 'five'));

        $this->assertEquals(2, $this->redis->zRevRank('z', 'one'));
        $this->assertEquals(1, $this->redis->zRevRank('z', 'two'));
        $this->assertEquals(0, $this->redis->zRevRank('z', 'five'));
    }

    public function testZRangeScoreArg() {
        $this->redis->del('{z}');

        $mems = ['one' => 1.0, 'two' => 2.0, 'three' => 3.0];
        foreach ($mems as $mem => $score) {
            $this->redis->zAdd('{z}', $score, $mem);
        }

        /* Verify we can pass true and ['withscores' => true] */
        $this->assertEquals($mems, $this->redis->zRange('{z}', 0, -1, true));
        $this->assertEquals($mems, $this->redis->zRange('{z}', 0, -1, ['withscores' => true]));
    }

    public function testZRangeByLex() {
        /* ZRANGEBYLEX available on versions >= 2.8.9 */
        if (version_compare($this->version, '2.8.9') < 0) {
            $this->MarkTestSkipped();
            return;
        }

        $this->redis->del('key');
        foreach (range('a', 'g') as $c) {
            $this->redis->zAdd('key', 0, $c);
        }

        $this->assertEquals(['a', 'b', 'c'], $this->redis->zRangeByLex('key', '-', '[c'));
        $this->assertEquals(['f', 'g'], $this->redis->zRangeByLex('key', '(e', '+'));


        // with limit offset
        $this->assertEquals(['b', 'c'], $this->redis->zRangeByLex('key', '-', '[c', 1, 2) );
        $this->assertEquals(['b'], $this->redis->zRangeByLex('key', '-', '(c', 1, 2));

        /* Test getting the same functionality via ZRANGE and options */
        if ($this->minVersionCheck('6.2.0')) {
            $this->assertEquals(['a', 'b', 'c'], $this->redis->zRange('key', '-', '[c', ['BYLEX']));
            $this->assertEquals(['b', 'c'], $this->redis->zRange('key', '-', '[c', ['BYLEX', 'LIMIT' => [1, 2]]));
            $this->assertEquals(['b'], $this->redis->zRange('key', '-', '(c', ['BYLEX', 'LIMIT' => [1, 2]]));

            $this->assertEquals(['b', 'a'], $this->redis->zRange('key', '[c', '-', ['BYLEX', 'REV', 'LIMIT' => [1, 2]]));
        }
    }

    public function testZLexCount() {
        if (version_compare($this->version, '2.8.9') < 0) {
            $this->MarkTestSkipped();
            return;
        }

        $this->redis->del('key');
        foreach (range('a', 'g') as $c) {
            $entries[] = $c;
            $this->redis->zAdd('key', 0, $c);
        }

        /* Special -/+ values */
        $this->assertEquals(0, $this->redis->zLexCount('key', '-', '-'));
        $this->assertEquals(count($entries), $this->redis->zLexCount('key', '-', '+'));

        /* Verify invalid arguments return FALSE */
        $this->assertFalse(@$this->redis->zLexCount('key', '[a', 'bad'));
        $this->assertFalse(@$this->redis->zLexCount('key', 'bad', '[a'));

        /* Now iterate through */
        $start = $entries[0];
        for ($i = 1; $i < count($entries); $i++) {
            $end = $entries[$i];
            $this->assertEquals($i + 1, $this->redis->zLexCount('key', "[$start", "[$end"));
            $this->assertEquals($i, $this->redis->zLexCount('key', "[$start", "($end"));
            $this->assertEquals($i - 1, $this->redis->zLexCount('key', "($start", "($end"));
        }
    }

    public function testzDiff() {
        // Only available since 6.2.0
        if (version_compare($this->version, '6.2.0') < 0)
            $this->markTestSkipped();

        $this->redis->del('key');
        foreach (range('a', 'c') as $c) {
            $this->redis->zAdd('key', 1, $c);
        }

        $this->assertEquals(['a', 'b', 'c'], $this->redis->zDiff(['key']));
        $this->assertEquals(['a' => 1.0, 'b' => 1.0, 'c' => 1.0], $this->redis->zDiff(['key'], ['withscores' => true]));
    }

    public function testzInter() {
        // Only available since 6.2.0
        if (version_compare($this->version, '6.2.0') < 0)
            $this->markTestSkipped();

        $this->redis->del('key');
        foreach (range('a', 'c') as $c) {
            $this->redis->zAdd('key', 1, $c);
        }

        $this->assertEquals(['a', 'b', 'c'], $this->redis->zInter(['key']));
        $this->assertEquals(['a' => 1.0, 'b' => 1.0, 'c' => 1.0], $this->redis->zInter(['key'], null, ['withscores' => true]));
    }

    public function testzUnion() {
        // Only available since 6.2.0
        if (version_compare($this->version, '6.2.0') < 0)
            $this->markTestSkipped();

        $this->redis->del('key');
        foreach (range('a', 'c') as $c) {
            $this->redis->zAdd('key', 1, $c);
        }

        $this->assertEquals(['a', 'b', 'c'], $this->redis->zUnion(['key']));
        $this->assertEquals(['a' => 1.0, 'b' => 1.0, 'c' => 1.0], $this->redis->zUnion(['key'], null, ['withscores' => true]));
    }

    public function testzDiffStore() {
        // Only available since 6.2.0
        if (version_compare($this->version, '6.2.0') < 0)
            $this->markTestSkipped();

        $this->redis->del('{zkey}src');
        foreach (range('a', 'c') as $c) {
            $this->redis->zAdd('{zkey}src', 1, $c);
        }
        $this->assertEquals(3, $this->redis->zDiffStore('{zkey}dst', ['{zkey}src']));
        $this->assertEquals(['a', 'b', 'c'], $this->redis->zRange('{zkey}dst', 0, -1));
    }

    public function testzMscore() {
        // Only available since 6.2.0
        if (version_compare($this->version, '6.2.0') < 0)
            $this->markTestSkipped();

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
        if (version_compare($this->version, '2.8.9') < 0) {
            $this->MarkTestSkipped();
            return;
        }

        $this->redis->del('key');
        $this->redis->zAdd('key', 0, 'a', 0, 'b', 0, 'c');
        $this->assertEquals(3, $this->redis->zRemRangeByLex('key', '-', '+'));

        $this->redis->zAdd('key', 0, 'a', 0, 'b', 0, 'c');
        $this->assertEquals(3, $this->redis->zRemRangeByLex('key', '[a', '[c'));

        $this->redis->zAdd('key', 0, 'a', 0, 'b', 0, 'c');
        $this->assertEquals(0, $this->redis->zRemRangeByLex('key', '[a', '(a'));
        $this->assertEquals(1, $this->redis->zRemRangeByLex('key', '(a', '(c'));
        $this->assertEquals(2, $this->redis->zRemRangeByLex('key', '[a', '[c'));
    }

    public function testBZPop() {
        if (version_compare($this->version, '5.0.0') < 0) {
            $this->MarkTestSkipped();
            return;
        }

        $this->redis->del('{zs}1', '{zs}2');
        $this->redis->zAdd('{zs}1', 0, 'a', 1, 'b', 2, 'c');
        $this->redis->zAdd('{zs}2', 3, 'A', 4, 'B', 5, 'D');

        $this->assertEquals(['{zs}1', 'a', '0'], $this->redis->bzPopMin('{zs}1', '{zs}2', 0));
        $this->assertEquals(['{zs}1', 'c', '2'], $this->redis->bzPopMax(['{zs}1', '{zs}2'], 0));
        $this->assertEquals(['{zs}2', 'A', '3'], $this->redis->bzPopMin('{zs}2', '{zs}1', 0));

        /* Verify timeout is being sent */
        $this->redis->del('{zs}1', '{zs}2');
        $st = microtime(true) * 1000;
        $this->redis->bzPopMin('{zs}1', '{zs}2', 1);
        $et = microtime(true) * 1000;
        $this->assertGT(100, $et - $st);
    }

    public function testZPop() {
        if (version_compare($this->version, '5.0.0') < 0) {
            $this->MarkTestSkipped();
            return;
        }

        // zPopMax and zPopMin without a COUNT argument
        $this->redis->del('key');
        $this->redis->zAdd('key', 0, 'a', 1, 'b', 2, 'c', 3, 'd', 4, 'e');
        $this->assertEquals(['e' => 4.0], $this->redis->zPopMax('key'));
        $this->assertEquals(['a' => 0.0], $this->redis->zPopMin('key'));

        // zPopMax with a COUNT argument
        $this->redis->del('key');
        $this->redis->zAdd('key', 0, 'a', 1, 'b', 2, 'c', 3, 'd', 4, 'e');
        $this->assertEquals(['e' => 4.0, 'd' => 3.0, 'c' => 2.0], $this->redis->zPopMax('key', 3));

        // zPopMin with a COUNT argument
        $this->redis->del('key');
        $this->redis->zAdd('key', 0, 'a', 1, 'b', 2, 'c', 3, 'd', 4, 'e');
        $this->assertEquals(['a' => 0.0, 'b' => 1.0, 'c' => 2.0], $this->redis->zPopMin('key', 3));
    }

    public function testZRandMember() {
        if (version_compare($this->version, '6.2.0') < 0) {
            $this->MarkTestSkipped();
            return;
        }
        $this->redis->del('key');
        $this->redis->zAdd('key', 0, 'a', 1, 'b', 2, 'c', 3, 'd', 4, 'e');
        $this->assertInArray($this->redis->zRandMember('key'), ['a', 'b', 'c', 'd', 'e']);

        $result = $this->redis->zRandMember('key', ['count' => 3]);
        $this->assertEquals(3, count($result));
        $this->assertEquals(array_intersect($result, ['a', 'b', 'c', 'd', 'e']), $result);

        $result = $this->redis->zRandMember('key', ['count' => 2, 'withscores' => true]);
        $this->assertEquals(2, count($result));
        $this->assertEquals(array_intersect_key($result, ['a' => 0, 'b' => 1, 'c' => 2, 'd' => 3, 'e' => 4]), $result);
    }

    public function testHashes() {
        $this->redis->del('h', 'key');
        $this->assertEquals(0, $this->redis->hLen('h'));
        $this->assertEquals(1, $this->redis->hSet('h', 'a', 'a-value'));
        $this->assertEquals(1, $this->redis->hLen('h'));
        $this->assertEquals(1, $this->redis->hSet('h', 'b', 'b-value'));
        $this->assertEquals(2, $this->redis->hLen('h'));

        $this->assertEquals('a-value', $this->redis->hGet('h', 'a'));  // simple get
        $this->assertEquals('b-value', $this->redis->hGet('h', 'b'));  // simple get

        $this->assertEquals(0, $this->redis->hSet('h', 'a', 'another-value')); // replacement
        $this->assertEquals('another-value', $this->redis->hGet('h', 'a'));    // get the new value

        $this->assertEquals('b-value', $this->redis->hGet('h', 'b'));  // simple get
        $this->assertFalse($this->redis->hGet('h', 'c'));  // unknown hash member
        $this->assertFalse($this->redis->hGet('key', 'c'));    // unknownkey

        // hDel
        $this->assertEquals(1, $this->redis->hDel('h', 'a')); // 1 on success
        $this->assertEquals(0, $this->redis->hDel('h', 'a')); // 0 on failure

        $this->redis->del('h');
        $this->redis->hSet('h', 'x', 'a');
        $this->redis->hSet('h', 'y', 'b');
        $this->assertEquals(2, $this->redis->hDel('h', 'x', 'y')); // variadic

        // hsetnx
        $this->redis->del('h');
        $this->assertTrue($this->redis->hSetNx('h', 'x', 'a'));
        $this->assertTrue($this->redis->hSetNx('h', 'y', 'b'));
        $this->assertFalse($this->redis->hSetNx('h', 'x', '?'));
        $this->assertFalse($this->redis->hSetNx('h', 'y', '?'));
        $this->assertEquals('a', $this->redis->hGet('h', 'x'));
        $this->assertEquals('b', $this->redis->hGet('h', 'y'));

        // keys
        $keys = $this->redis->hKeys('h');
        $this->assertEqualsCanonicalizing(['x', 'y'], $keys);

        // values
        $values = $this->redis->hVals('h');
        $this->assertEqualsCanonicalizing(['a', 'b'], $values);

        // keys + values
        $all = $this->redis->hGetAll('h');
        $this->assertEqualsCanonicalizing(['x' => 'a', 'y' => 'b'], $all, true);

        // hExists
        $this->assertTrue($this->redis->hExists('h', 'x'));
        $this->assertTrue($this->redis->hExists('h', 'y'));
        $this->assertFalse($this->redis->hExists('h', 'w'));
        $this->redis->del('h');
        $this->assertFalse($this->redis->hExists('h', 'x'));

        // hIncrBy
        $this->redis->del('h');
        $this->assertEquals(2, $this->redis->hIncrBy('h', 'x', 2));
        $this->assertEquals(3, $this->redis->hIncrBy('h', 'x', 1));
        $this->assertEquals(2, $this->redis->hIncrBy('h', 'x', -1));
        $this->assertEquals('2', $this->redis->hGet('h', 'x'));
        $this->assertEquals(PHP_INT_MAX, $this->redis->hIncrBy('h', 'x', PHP_INT_MAX-2));
        $this->assertEquals(''.PHP_INT_MAX, $this->redis->hGet('h', 'x'));

        $this->redis->hSet('h', 'y', 'not-a-number');
        $this->assertFalse($this->redis->hIncrBy('h', 'y', 1));

        if (version_compare($this->version, '2.5.0') >= 0) {
            // hIncrByFloat
            $this->redis->del('h');
            $this->assertEquals(1.5, $this->redis->hIncrByFloat('h', 'x', 1.5));
            $this->assertEquals(3.0, $this->redis->hincrByFloat('h', 'x', 1.5));
            $this->assertEquals(1.5, $this->redis->hincrByFloat('h', 'x', -1.5));
            $this->assertEquals(1000000000001.5, $this->redis->hincrByFloat('h', 'x', 1000000000000));

            $this->redis->hset('h', 'y', 'not-a-number');
            $this->assertFalse($this->redis->hIncrByFloat('h', 'y', 1.5));
        }

        // hmset
        $this->redis->del('h');
        $this->assertTrue($this->redis->hMset('h', ['x' => 123, 'y' => 456, 'z' => 'abc']));
        $this->assertEquals('123', $this->redis->hGet('h', 'x'));
        $this->assertEquals('456', $this->redis->hGet('h', 'y'));
        $this->assertEquals('abc', $this->redis->hGet('h', 'z'));
        $this->assertFalse($this->redis->hGet('h', 't'));

        // hmget
        $this->assertEquals(['x' => '123', 'y' => '456'], $this->redis->hMget('h', ['x', 'y']));
        $this->assertEquals(['z' => 'abc'], $this->redis->hMget('h', ['z']));
        $this->assertEquals(['x' => '123', 't' => FALSE, 'y' => '456'], $this->redis->hMget('h', ['x', 't', 'y']));
        $this->assertEquals(['x' => '123', 't' => FALSE, 'y' => '456'], $this->redis->hMget('h', ['x', 't', 'y']));
        $this->assertNotEquals([123 => 'x'], $this->redis->hMget('h', [123]));
        $this->assertEquals([123 => FALSE], $this->redis->hMget('h', [123]));

        // Test with an array populated with things we can't use as keys
        $this->assertFalse($this->redis->hmget('h', [false,NULL,false]));

        // Test with some invalid keys mixed in (which should just be ignored)
        $this->assertEquals(
            ['x' => '123', 'y' => '456', 'z' => 'abc'],
            $this->redis->hMget('h', ['x', null, 'y', '', 'z', false])
        );

        // hmget/hmset with numeric fields
        $this->redis->del('h');
        $this->assertTrue($this->redis->hMset('h', [123 => 'x', 'y' => 456]));
        $this->assertEquals('x', $this->redis->hGet('h', 123));
        $this->assertEquals('x', $this->redis->hGet('h', '123'));
        $this->assertEquals('456', $this->redis->hGet('h', 'y'));
        $this->assertEquals([123 => 'x', 'y' => '456'], $this->redis->hMget('h', ['123', 'y']));

        // references
        $keys = [123, 'y'];
        foreach ($keys as &$key) {}
        $this->assertEquals([123 => 'x', 'y' => '456'], $this->redis->hMget('h', $keys));

        // check non-string types.
        $this->redis->del('h1');
        $this->assertTrue($this->redis->hMSet('h1', ['x' => 0, 'y' => [], 'z' => new stdclass(), 't' => NULL]));
        $h1 = $this->redis->hGetAll('h1');
        $this->assertEquals('0', $h1['x']);
        $this->assertEquals('Array', $h1['y']);
        $this->assertEquals('Object', $h1['z']);
        $this->assertEquals('', $h1['t']);

        // hset with fields + values as an associative array
        if (version_compare($this->version, '4.0.0') >= 0) {
            $this->redis->del('h');
            $this->assertEquals(3, $this->redis->hSet('h', ['x' => 123, 'y' => 456, 'z' => 'abc']));
            $this->assertEquals(['x' => '123', 'y' => '456', 'z' => 'abc'], $this->redis->hGetAll('h'));
            $this->assertEquals(0, $this->redis->hSet('h', ['x' => 789]));
            $this->assertEquals(['x' => '789', 'y' => '456', 'z' => 'abc'], $this->redis->hGetAll('h'));
        }

        // hset with variadic fields + values
        if (version_compare($this->version, '4.0.0') >= 0) {
            $this->redis->del('h');
            $this->assertEquals(3, $this->redis->hSet('h', 'x', 123, 'y', 456, 'z', 'abc'));
            $this->assertEquals(['x' => '123', 'y' => '456', 'z' => 'abc'], $this->redis->hGetAll('h'));
            $this->assertEquals(0, $this->redis->hSet('h', 'x', 789));
            $this->assertEquals(['x' => '789', 'y' => '456', 'z' => 'abc'], $this->redis->hGetAll('h'));
        }

        // hstrlen
        if (version_compare($this->version, '3.2.0') >= 0) {
            $this->redis->del('h');
            $this->assertEquals(0, $this->redis->hStrLen('h', 'x')); // key doesn't exist
            $this->redis->hSet('h', 'foo', 'bar');
            $this->assertEquals(0, $this->redis->hStrLen('h', 'x')); // field is not present in the hash
            $this->assertEquals(3, $this->redis->hStrLen('h', 'foo'));
	}
    }

    public function testHRandField() {
        if (version_compare($this->version, '6.2.0') < 0)
            $this->MarkTestSkipped();

        $this->redis->del('key');
        $this->redis->hMSet('key', ['a' => 0, 'b' => 1, 'c' => 'foo', 'd' => 'bar', 'e' => null]);
        $this->assertInArray($this->redis->hRandField('key'), ['a', 'b', 'c', 'd', 'e']);

        $result = $this->redis->hRandField('key', ['count' => 3]);
        $this->assertEquals(3, count($result));
        $this->assertEquals(array_intersect($result, ['a', 'b', 'c', 'd', 'e']), $result);

        $result = $this->redis->hRandField('key', ['count' => 2, 'withvalues' => true]);
        $this->assertEquals(2, count($result));
        $this->assertEquals(array_intersect_key($result, ['a' => 0, 'b' => 1, 'c' => 'foo', 'd' => 'bar', 'e' => null]), $result);

        /* Make sure PhpRedis sends COUNt (1) when `WITHVALUES` is set */
        $result = $this->redis->hRandField('key', ['withvalues' => true]);
        $this->assertNull($this->redis->getLastError());
        $this->assertIsArray($result);
        $this->assertEquals(1, count($result));

        /* We can return false if the key doesn't exist */
        $this->assertIsInt($this->redis->del('notahash'));
        $this->assertFalse($this->redis->hRandField('notahash'));
    }

    public function testSetRange() {

        $this->redis->del('key');
        $this->redis->set('key', 'hello world');
        $this->redis->setRange('key', 6, 'redis');
        $this->assertKeyEquals('hello redis', 'key');
        $this->redis->setRange('key', 6, 'you'); // don't cut off the end
        $this->assertKeyEquals('hello youis', 'key');

        $this->redis->set('key', 'hello world');

        // fill with zeros if needed
        $this->redis->del('key');
        $this->redis->setRange('key', 6, 'foo');
        $this->assertKeyEquals("\x00\x00\x00\x00\x00\x00foo", 'key');
    }

    public function testObject() {
        /* Version 3.0.0 (represented as >= 2.9.0 in redis info)  and moving
         * forward uses 'embstr' instead of 'raw' for small string values */
        if (version_compare($this->version, '2.9.0') < 0) {
            $small_encoding = 'raw';
        } else {
            $small_encoding = 'embstr';
        }

        $this->redis->del('key');
        $this->assertFalse($this->redis->object('encoding', 'key'));
        $this->assertFalse($this->redis->object('refcount', 'key'));
        $this->assertFalse($this->redis->object('idletime', 'key'));

        $this->redis->set('key', 'value');
        $this->assertEquals($small_encoding, $this->redis->object('encoding', 'key'));
        $this->assertEquals(1, $this->redis->object('refcount', 'key'));
        $this->assertTrue(is_numeric($this->redis->object('idletime', 'key')));

        $this->redis->del('key');
        $this->redis->lpush('key', 'value');

        /* Redis has improved the encoding here throughout the various versions.  The value
           can either be 'ziplist', 'quicklist', or 'listpack' */
        $encoding = $this->redis->object('encoding', 'key');
        $this->assertInArray($encoding, ['ziplist', 'quicklist', 'listpack']);

        $this->assertEquals(1, $this->redis->object('refcount', 'key'));
        $this->assertTrue(is_numeric($this->redis->object('idletime', 'key')));

        $this->redis->del('key');
        $this->redis->sadd('key', 'value');

        /* Redis 7.2.0 switched to 'listpack' for small sets */
        $encoding = $this->redis->object('encoding', 'key');
        $this->assertInArray($encoding, ['hashtable', 'listpack']);
        $this->assertEquals(1, $this->redis->object('refcount', 'key'));
        $this->assertTrue(is_numeric($this->redis->object('idletime', 'key')));

        $this->redis->del('key');
        $this->redis->sadd('key', 42);
        $this->redis->sadd('key', 1729);
        $this->assertEquals('intset', $this->redis->object('encoding', 'key'));
        $this->assertEquals(1, $this->redis->object('refcount', 'key'));
        $this->assertTrue(is_numeric($this->redis->object('idletime', 'key')));

        $this->redis->del('key');
        $this->redis->lpush('key', str_repeat('A', pow(10, 6))); // 1M elements, too big for a ziplist.

        $encoding = $this->redis->object('encoding', 'key');
        $this->assertInArray($encoding, ['linkedlist', 'quicklist']);

        $this->assertEquals(1, $this->redis->object('refcount', 'key'));
        $this->assertTrue(is_numeric($this->redis->object('idletime', 'key')));
    }

    public function testMultiExec() {
        $this->sequence(Redis::MULTI);
        $this->differentType(Redis::MULTI);

        // with prefix as well
        $this->redis->setOption(Redis::OPT_PREFIX, 'test:');
        $this->sequence(Redis::MULTI);
        $this->differentType(Redis::MULTI);
        $this->redis->setOption(Redis::OPT_PREFIX, '');

        $this->redis->set('x', '42');

        $this->assertTrue($this->redis->watch('x'));
        $ret = $this->redis->multi()->get('x')->exec();

        // successful transaction
        $this->assertEquals(['42'], $ret);
    }

    public function testFailedTransactions() {
        $this->redis->set('x', 42);

        // failed transaction
        $this->redis->watch('x');

        $r = $this->newInstance(); // new instance, modifying `x'.
        $r->incr('x');

        $ret = $this->redis->multi()->get('x')->exec();
        $this->assertFalse($ret); // failed because another client changed our watched key between WATCH and EXEC.

        // watch and unwatch
        $this->redis->watch('x');
        $r->incr('x'); // other instance
        $this->redis->unwatch(); // cancel transaction watch

        $ret = $this->redis->multi()->get('x')->exec();

        // succeeded since we've cancel the WATCH command.
        $this->assertEquals(['44'], $ret);
    }

    public function testPipeline() {
        if ( ! $this->havePipeline())
            $this->markTestSkipped();

        $this->sequence(Redis::PIPELINE);
        $this->differentType(Redis::PIPELINE);

        // with prefix as well
        $this->redis->setOption(Redis::OPT_PREFIX, 'test:');
        $this->sequence(Redis::PIPELINE);
        $this->differentType(Redis::PIPELINE);
        $this->redis->setOption(Redis::OPT_PREFIX, '');
    }

    public function testPipelineMultiExec() {
        if ( ! $this->havePipeline())
            $this->markTestSkipped();

        $ret = $this->redis->pipeline()->multi()->exec()->exec();
        $this->assertIsArray($ret);
        $this->assertEquals(1, count($ret)); // empty transaction

        $ret = $this->redis->pipeline()
            ->ping()
            ->multi()->set('x', 42)->incr('x')->exec()
            ->ping()
            ->multi()->get('x')->del('x')->exec()
            ->ping()
            ->exec();
        $this->assertIsArray($ret);
        $this->assertEquals(5, count($ret)); // should be 5 atomic operations
    }

    /* GitHub issue #1211 (ignore redundant calls to pipeline or multi) */
    public function testDoublePipeNoOp() {
        /* Only the first pipeline should be honored */
        for ($i = 0; $i < 6; $i++) {
            $this->redis->pipeline();
        }

        /* Set and get in our pipeline */
        $this->redis->set('pipecount', 'over9000')->get('pipecount');

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

    public function testDiscard() {
        foreach ([Redis::PIPELINE, Redis::MULTI] as $mode) {
            /* start transaction */
            $this->redis->multi($mode);

            /* Set and get in our transaction */
            $this->redis->set('pipecount', 'over9000')->get('pipecount');

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

        $this->assertIsArray($ret);
        $i = 0;
        $this->assertTrue($ret[$i++]);
        $this->assertEquals(Redis::REDIS_STRING, $ret[$i++]);
        $this->assertEqualsWeak('42', $ret[$i]);

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
        $this->assertIsArray($ret);
        $this->assertTrue(is_long($ret[$i++]));
        $this->assertEqualsWeak(true, $ret[$i++]);
        $this->assertEqualsWeak('value1', $ret[$i++]);
        $this->assertEqualsWeak('value1', $ret[$i++]);
        $this->assertEqualsWeak('value2', $ret[$i++]);
        $this->assertEqualsWeak(true, $ret[$i++]);
        $this->assertEqualsWeak(5, $ret[$i++]);
        $this->assertEqualsWeak(5, $ret[$i++]);
        $this->assertEqualsWeak(4, $ret[$i++]);
        $this->assertEqualsWeak(4, $ret[$i++]);
        $this->assertEqualsWeak(true, $ret[$i++]);
        $this->assertEqualsWeak(4, $ret[$i++]);
        $this->assertEqualsWeak(FALSE, $ret[$i++]);
        $this->assertEqualsWeak(true, $ret[$i++]);
        $this->assertEqualsWeak(true, $ret[$i++]);
        $this->assertEqualsWeak(9, $ret[$i++]);
        $this->assertEqualsWeak(true, $ret[$i++]);
        $this->assertEqualsWeak(4, $ret[$i++]);
        $this->assertEquals($i, count($ret));

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

        $this->assertIsArray($ret);
        $this->assertEqualsWeak(true, $ret[0]);
        $this->assertEqualsWeak(true, $ret[1]);
        $this->assertEqualsWeak(true, $ret[2]);
        $this->assertEqualsWeak(false, $ret[3]);
        $this->assertEqualsWeak(true, $ret[4]);
        $this->assertEqualsWeak(true, $ret[5]);
        $this->assertEqualsWeak(false, $ret[6]);

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

        $this->assertIsArray($ret);
        $i = 0;
        $ttl = $ret[$i++];
        $this->assertBetween($ttl, -2, -1);
        $this->assertEquals(['val1', 'valX', false], $ret[$i++]); // mget
        $this->assertTrue($ret[$i++]); // mset
        $this->assertTrue($ret[$i++]); // set
        $this->assertTrue($ret[$i++]); // expire
        $this->assertEquals(5, $ret[$i++]);    // ttl
        $this->assertTrue($ret[$i++]); // expireAt
        $this->assertEquals($i, count($ret));

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
            ->lSet('{list}lkey', 1, 'newValue')    // check errors on key not exists
            ->lrange('{list}lkey', 0, -1)
            ->llen('{list}lkey')
            ->exec();

        $this->assertIsArray($ret);
        $i = 0;
        $this->assertTrue($ret[$i++]); // SET
        $this->assertTrue($ret[$i++]); // SET
        $this->assertEquals(2, $ret[$i++]); // deleting 2 keys
        $this->assertEquals(1, $ret[$i++]); // rpush, now 1 element
        $this->assertEquals(2, $ret[$i++]); // lpush, now 2 elements
        $this->assertEquals(3, $ret[$i++]); // lpush, now 3 elements
        $this->assertEquals(4, $ret[$i++]); // lpush, now 4 elements
        $this->assertEquals(5, $ret[$i++]); // lpush, now 5 elements
        $this->assertEquals(6, $ret[$i++]); // lpush, now 6 elements
        $this->assertEquals('lvalue', $ret[$i++]); // rpoplpush returns the element: 'lvalue'
        $this->assertEquals(['lvalue'], $ret[$i++]); // lDest contains only that one element.
        $this->assertEquals('lvalue', $ret[$i++]); // removing a second element from lkey, now 4 elements left ↓
        $this->assertEquals(4, $ret[$i++]); // 4 elements left, after 2 pops.
        $this->assertEquals(3, $ret[$i++]); // removing 3 elements, now 1 left.
        $this->assertEquals(1, $ret[$i++]); // 1 element left
        $this->assertEquals('lvalue', $ret[$i++]); // this is the current head.
        $this->assertEquals(['lvalue'], $ret[$i++]); // this is the current list.
        $this->assertFalse($ret[$i++]); // updating a non-existent element fails.
        $this->assertEquals(['lvalue'], $ret[$i++]); // this is the current list.
        $this->assertEquals(1, $ret[$i++]); // 1 element left
        $this->assertEquals($i, count($ret));

        $ret = $this->redis->multi($mode)
            ->del('{list}lkey', '{list}lDest')
            ->rpush('{list}lkey', 'lvalue')
            ->lpush('{list}lkey', 'lvalue')
            ->lpush('{list}lkey', 'lvalue')
            ->rpoplpush('{list}lkey', '{list}lDest')
            ->lrange('{list}lDest', 0, -1)
            ->lpop('{list}lkey')
            ->exec();
        $this->assertIsArray($ret);

        $i = 0;

        $this->assertLTE(2, $ret[$i++]);      // deleting 2 keys
        $this->assertEquals(1, $ret[$i++]); // 1 element in the list
        $this->assertEquals(2, $ret[$i++]); // 2 elements in the list
        $this->assertEquals(3, $ret[$i++]); // 3 elements in the list
        $this->assertEquals('lvalue', $ret[$i++]); // rpoplpush returns the element: 'lvalue'
        $this->assertEquals(['lvalue'], $ret[$i++]); // rpoplpush returns the element: 'lvalue'
        $this->assertEquals('lvalue', $ret[$i++]); // pop returns the front element: 'lvalue'
        $this->assertEquals($i, count($ret));


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
        $this->assertIsArray($ret);
        $this->assertLTE(1, $ret[$i++]);
        $this->assertEqualsWeak(true, $ret[$i++]);
        $this->assertEquals('value1', $ret[$i++]);
        $this->assertEquals('value1', $ret[$i++]);
        $this->assertEquals('value2', $ret[$i++]);
        $this->assertEqualsWeak(true, $ret[$i++]);
        $this->assertEqualsWeak(5, $ret[$i++]);
        $this->assertEqualsWeak(5, $ret[$i++]);
        $this->assertEqualsWeak(4, $ret[$i++]);
        $this->assertEqualsWeak(4, $ret[$i++]);
        $this->assertTrue($ret[$i++]);
        $this->assertEqualsWeak(4, $ret[$i++]);
        $this->assertFalse($ret[$i++]);
        $this->assertTrue($ret[$i++]);
        $this->assertEquals(9, $ret[$i++]);          // incrby('{key}2', 5)
        $this->assertEqualsWeak(9, $ret[$i++]);      // get('{key}2')
        $this->assertEquals(4, $ret[$i++]);          // decrby('{key}2', 5)
        $this->assertEqualsWeak(4, $ret[$i++]);      // get('{key}2')
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

        $this->assertIsArray($ret);
        $this->assertEquals(1, $ret[0]); // del('{key}1')
        $this->assertEquals(1, $ret[1]); // del('{key}2')
        $this->assertEquals(1, $ret[2]); // del('{key}3')
        $this->assertTrue($ret[3]);      // set('{key}1', 'val1')
        $this->assertFalse($ret[4]);     // setnx('{key}1', 'valX')
        $this->assertTrue($ret[5]);      // setnx('{key}2', 'valX')
        $this->assertEquals(1, $ret[6]); // exists('{key}1')
        $this->assertEquals(0, $ret[7]); // exists('{key}3')

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
        $this->assertIsArray($ret);
        $this->assertTrue(is_long($ret[$i++]));
        $this->assertIsArray($ret[$i++], 3);
//        $i++;
        $this->assertTrue($ret[$i++]); // mset always returns true
        $this->assertTrue($ret[$i++]); // set always returns true
        $this->assertTrue($ret[$i++]); // expire always returns true
        $this->assertEquals(5, $ret[$i++]); // TTL was just set.
        $this->assertTrue($ret[$i++]); // expireAt returns true for an existing key
        $this->assertEquals($i, count($ret));

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
            ->lSet('{l}key', 1, 'newValue')    // check errors on missing key
            ->lrange('{l}key', 0, -1)
            ->llen('{l}key')
            ->exec();

        $this->assertIsArray($ret);
        $i = 0;
        $this->assertBetween($ret[$i++], 0, 2); // del
        $this->assertEquals(1, $ret[$i++]); // 1 value
        $this->assertEquals(2, $ret[$i++]); // 2 values
        $this->assertEquals(3, $ret[$i++]); // 3 values
        $this->assertEquals(4, $ret[$i++]); // 4 values
        $this->assertEquals(5, $ret[$i++]); // 5 values
        $this->assertEquals(6, $ret[$i++]); // 6 values
        $this->assertEquals('lvalue', $ret[$i++]);
        $this->assertEquals(['lvalue'], $ret[$i++]); // 1 value only in lDest
        $this->assertEquals('lvalue', $ret[$i++]); // now 4 values left
        $this->assertEquals(4, $ret[$i++]);
        $this->assertEquals(3, $ret[$i++]); // removing 3 elements.
        $this->assertEquals(1, $ret[$i++]); // length is now 1
        $this->assertEquals('lvalue', $ret[$i++]); // this is the head
        $this->assertEquals(['lvalue'], $ret[$i++]); // 1 value only in lkey
        $this->assertFalse($ret[$i++]); // can't set list[1] if we only have a single value in it.
        $this->assertEquals(['lvalue'], $ret[$i++]); // the previous error didn't touch anything.
        $this->assertEquals(1, $ret[$i++]); // the previous error didn't change the length
        $this->assertEquals($i, count($ret));


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
        $this->assertIsArray($ret);
        $this->assertBetween($ret[$i++], 0, 5); // we deleted at most 5 values.
        $this->assertEquals(1, $ret[$i++]);     // skey1 now has 1 element.
        $this->assertEquals(1, $ret[$i++]);     // skey1 now has 2 elements.
        $this->assertEquals(1, $ret[$i++]);     // skey1 now has 3 elements.
        $this->assertEquals(1, $ret[$i++]);     // skey1 now has 4 elements.
        $this->assertEquals(1, $ret[$i++]);     // skey2 now has 1 element.
        $this->assertEquals(1, $ret[$i++]);     // skey2 now has 2 elements.
        $this->assertEquals(4, $ret[$i++]);
        $this->assertEquals(1, $ret[$i++]);     // we did remove that value.
        $this->assertEquals(3, $ret[$i++]);     // now 3 values only.

        $this->assertTrue($ret[$i++]); // the move did succeed.
        $this->assertEquals(3, $ret[$i++]); // sKey2 now has 3 values.
        $this->assertTrue($ret[$i++]); // sKey2 does contain sValue4.
        foreach (['sValue1', 'sValue3'] as $k) { // sKey1 contains sValue1 and sValue3.
            $this->assertInArray($k, $ret[$i]);
        }
        $this->assertEquals(2, count($ret[$i++]));
        foreach (['sValue1', 'sValue2', 'sValue4'] as $k) { // sKey2 contains sValue1, sValue2, and sValue4.
            $this->assertInArray($k, $ret[$i]);
        }
        $this->assertEquals(3, count($ret[$i++]));
        $this->assertEquals(['sValue1'], $ret[$i++]); // intersection
        $this->assertEquals(1, $ret[$i++]); // intersection + store → 1 value in the destination set.
        $this->assertEquals(['sValue1'], $ret[$i++]); // sinterstore destination contents

        foreach (['sValue1', 'sValue2', 'sValue4'] as $k) { // (skeydest U sKey2) contains sValue1, sValue2, and sValue4.
            $this->assertInArray($k, $ret[$i]);
        }
        $this->assertEquals(3, count($ret[$i++])); // union size

        $this->assertEquals(3, $ret[$i++]); // unionstore size
        foreach (['sValue1', 'sValue2', 'sValue4'] as $k) { // (skeyUnion) contains sValue1, sValue2, and sValue4.
            $this->assertInArray($k, $ret[$i]);
        }
        $this->assertEquals(3, count($ret[$i++])); // skeyUnion size

        $this->assertEquals(['sValue3'], $ret[$i++]); // diff skey1, skey2 : only sValue3 is not shared.
        $this->assertEquals(1, $ret[$i++]); // sdiffstore size == 1
        $this->assertEquals(['sValue3'], $ret[$i++]); // contents of sDiffDest

        $this->assertInArray($ret[$i++], ['sValue1', 'sValue2', 'sValue4']); // we removed an element from sKey2
        $this->assertEquals(2, $ret[$i++]); // sKey2 now has 2 elements only.

        $this->assertEquals($i, count($ret));

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
        $this->assertIsArray($ret);
        $this->assertBetween($ret[$i++], 0, 5); // we deleted at most 5 values.
        $this->assertEquals(1, $ret[$i++]);
        $this->assertEquals(1, $ret[$i++]);
        $this->assertEquals(1, $ret[$i++]);
        $this->assertEquals(['zValue1', 'zValue2', 'zValue5'], $ret[$i++]);
        $this->assertEquals(1, $ret[$i++]);
        $this->assertEquals(['zValue1', 'zValue5'], $ret[$i++]);
        $this->assertEquals(1, $ret[$i++]); // adding zValue11
        $this->assertEquals(1, $ret[$i++]); // adding zValue12
        $this->assertEquals(1, $ret[$i++]); // adding zValue13
        $this->assertEquals(1, $ret[$i++]); // adding zValue14
        $this->assertEquals(1, $ret[$i++]); // adding zValue15
        $this->assertEquals(3, $ret[$i++]); // deleted zValue11, zValue12, zValue13
        $this->assertEquals(['zValue1', 'zValue5', 'zValue14', 'zValue15'], $ret[$i++]);
        $this->assertEquals(['zValue15', 'zValue14', 'zValue5', 'zValue1'], $ret[$i++]);
        $this->assertEquals(['zValue1', 'zValue5'], $ret[$i++]);
        $this->assertEquals(4, $ret[$i++]); // 4 elements
        $this->assertEquals(15.0, $ret[$i++]);
        $this->assertEquals(1, $ret[$i++]); // added value
        $this->assertEquals(1, $ret[$i++]); // added value
        $this->assertEquals(1, $ret[$i++]); // zinter only has 1 value
        $this->assertEquals(['zValue1', 'zValue5', 'zValue14', 'zValue15'], $ret[$i++]); // {z}key1 contents
        $this->assertEquals(['zValue2', 'zValue5'], $ret[$i++]); // {z}key2 contents
        $this->assertEquals(['zValue5'], $ret[$i++]); // {z}inter contents
        $this->assertEquals(5, $ret[$i++]); // {z}Union has 5 values (1, 2, 5, 14, 15)
        $this->assertEquals(['zValue1', 'zValue2', 'zValue5', 'zValue14', 'zValue15'], $ret[$i++]); // {z}Union contents
        $this->assertEquals(1, $ret[$i++]); // added value to {z}key5, with score 5
        $this->assertEquals(8.0, $ret[$i++]); // incremented score by 3 → it is now 8.
        $this->assertEquals(8.0, $ret[$i++]); // current score is 8.
        $this->assertFalse($ret[$i++]); // score for unknown element.

        $this->assertEquals($i, count($ret));

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
        $this->assertIsArray($ret);
        $this->assertLT(2, $ret[$i++]); // delete
        $this->assertEquals(1, $ret[$i++]); // added 1 element
        $this->assertEquals(1, $ret[$i++]); // added 1 element
        $this->assertEquals(1, $ret[$i++]); // added 1 element
        $this->assertEquals(['key1' => 'value1', 'key2' => 'value2', 'key3' => 'value3'], $ret[$i++]); // hmget, 3 elements
        $this->assertEquals('value1', $ret[$i++]); // hget
        $this->assertEquals(3, $ret[$i++]); // hlen
        $this->assertEquals(1, $ret[$i++]); // hdel succeeded
        $this->assertEquals(0, $ret[$i++]); // hdel failed
        $this->assertFalse($ret[$i++]); // hexists didn't find the deleted key
        $this->assertEqualsCanonicalizing(['key1', 'key3'], $ret[$i++]); // hkeys
        $this->assertEqualsCanonicalizing(['value1', 'value3'], $ret[$i++]); // hvals
        $this->assertEqualsCanonicalizing(['key1' => 'value1', 'key3' => 'value3'], $ret[$i++]); // hgetall
        $this->assertEquals(1, $ret[$i++]); // added 1 element
        $this->assertEquals(1, $ret[$i++]); // added the element, so 1.
        $this->assertEquals('non-string', $ret[$i++]); // hset succeeded
        $this->assertEquals($i, count($ret));

        $ret = $this->redis->multi($mode) // default to MULTI, not PIPELINE.
            ->del('test')
            ->set('test', 'xyz')
            ->get('test')
            ->exec();
        $i = 0;
        $this->assertIsArray($ret);
        $this->assertLTE(1, $ret[$i++]); // delete
        $this->assertTrue($ret[$i++]); // added 1 element
        $this->assertEquals('xyz', $ret[$i++]);
        $this->assertEquals($i, count($ret));

        // GitHub issue 78
        $this->redis->del('test');
        for ($i = 1; $i <= 5; $i++)
            $this->redis->zadd('test', $i, (string)$i);

        $result = $this->redis->multi($mode)
            ->zscore('test', '1')
            ->zscore('test', '6')
            ->zscore('test', '8')
            ->zscore('test', '2')
            ->exec();

        $this->assertEquals([1.0, FALSE, FALSE, 2.0], $result);
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
            ->lSet($key, 0, 'newValue')
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
        $this->assertIsArray($ret);
        $this->assertTrue(is_long($ret[$i++])); // delete
        $this->assertTrue($ret[$i++]); // set

        $this->assertFalse($ret[$i++]); // rpush
        $this->assertFalse($ret[$i++]); // lpush
        $this->assertFalse($ret[$i++]); // llen
        $this->assertFalse($ret[$i++]); // lpop
        $this->assertFalse($ret[$i++]); // lrange
        $this->assertFalse($ret[$i++]); // ltrim
        $this->assertFalse($ret[$i++]); // lindex
        $this->assertFalse($ret[$i++]); // lset
        $this->assertFalse($ret[$i++]); // lrem
        $this->assertFalse($ret[$i++]); // lpop
        $this->assertFalse($ret[$i++]); // rpop
        $this->assertFalse($ret[$i++]); // rpoplush

        $this->assertFalse($ret[$i++]); // sadd
        $this->assertFalse($ret[$i++]); // srem
        $this->assertFalse($ret[$i++]); // spop
        $this->assertFalse($ret[$i++]); // smove
        $this->assertFalse($ret[$i++]); // scard
        $this->assertFalse($ret[$i++]); // sismember
        $this->assertFalse($ret[$i++]); // sinter
        $this->assertFalse($ret[$i++]); // sunion
        $this->assertFalse($ret[$i++]); // sdiff
        $this->assertFalse($ret[$i++]); // smembers
        $this->assertFalse($ret[$i++]); // srandmember

        $this->assertFalse($ret[$i++]); // zadd
        $this->assertFalse($ret[$i++]); // zrem
        $this->assertFalse($ret[$i++]); // zincrby
        $this->assertFalse($ret[$i++]); // zrank
        $this->assertFalse($ret[$i++]); // zrevrank
        $this->assertFalse($ret[$i++]); // zrange
        $this->assertFalse($ret[$i++]); // zreverserange
        $this->assertFalse($ret[$i++]); // zrangebyscore
        $this->assertFalse($ret[$i++]); // zcount
        $this->assertFalse($ret[$i++]); // zcard
        $this->assertFalse($ret[$i++]); // zscore
        $this->assertFalse($ret[$i++]); // zremrangebyrank
        $this->assertFalse($ret[$i++]); // zremrangebyscore

        $this->assertFalse($ret[$i++]); // hset
        $this->assertFalse($ret[$i++]); // hget
        $this->assertFalse($ret[$i++]); // hmget
        $this->assertFalse($ret[$i++]); // hmset
        $this->assertFalse($ret[$i++]); // hincrby
        $this->assertFalse($ret[$i++]); // hexists
        $this->assertFalse($ret[$i++]); // hdel
        $this->assertFalse($ret[$i++]); // hlen
        $this->assertFalse($ret[$i++]); // hkeys
        $this->assertFalse($ret[$i++]); // hvals
        $this->assertFalse($ret[$i++]); // hgetall

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
        $this->assertIsArray($ret);
        $this->assertTrue(is_long($ret[$i++])); // delete
        $this->assertEquals(1, $ret[$i++]); // lpush

        $this->assertFalse($ret[$i++]); // get
        $this->assertFalse($ret[$i++]); // getset
        $this->assertFalse($ret[$i++]); // append
        $this->assertFalse($ret[$i++]); // getRange
        $this->assertEquals([false], $ret[$i++]); // mget
        $this->assertFalse($ret[$i++]); // incr
        $this->assertFalse($ret[$i++]); // incrBy
        $this->assertFalse($ret[$i++]); // decr
        $this->assertFalse($ret[$i++]); // decrBy

        $this->assertFalse($ret[$i++]); // sadd
        $this->assertFalse($ret[$i++]); // srem
        $this->assertFalse($ret[$i++]); // spop
        $this->assertFalse($ret[$i++]); // smove
        $this->assertFalse($ret[$i++]); // scard
        $this->assertFalse($ret[$i++]); // sismember
        $this->assertFalse($ret[$i++]); // sinter
        $this->assertFalse($ret[$i++]); // sunion
        $this->assertFalse($ret[$i++]); // sdiff
        $this->assertFalse($ret[$i++]); // smembers
        $this->assertFalse($ret[$i++]); // srandmember

        $this->assertFalse($ret[$i++]); // zadd
        $this->assertFalse($ret[$i++]); // zrem
        $this->assertFalse($ret[$i++]); // zincrby
        $this->assertFalse($ret[$i++]); // zrank
        $this->assertFalse($ret[$i++]); // zrevrank
        $this->assertFalse($ret[$i++]); // zrange
        $this->assertFalse($ret[$i++]); // zreverserange
        $this->assertFalse($ret[$i++]); // zrangebyscore
        $this->assertFalse($ret[$i++]); // zcount
        $this->assertFalse($ret[$i++]); // zcard
        $this->assertFalse($ret[$i++]); // zscore
        $this->assertFalse($ret[$i++]); // zremrangebyrank
        $this->assertFalse($ret[$i++]); // zremrangebyscore

        $this->assertFalse($ret[$i++]); // hset
        $this->assertFalse($ret[$i++]); // hget
        $this->assertFalse($ret[$i++]); // hmget
        $this->assertFalse($ret[$i++]); // hmset
        $this->assertFalse($ret[$i++]); // hincrby
        $this->assertFalse($ret[$i++]); // hexists
        $this->assertFalse($ret[$i++]); // hdel
        $this->assertFalse($ret[$i++]); // hlen
        $this->assertFalse($ret[$i++]); // hkeys
        $this->assertFalse($ret[$i++]); // hvals
        $this->assertFalse($ret[$i++]); // hgetall

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
            ->lSet($key, 0, 'newValue')
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
        $this->assertIsArray($ret);
        $this->assertTrue(is_long($ret[$i++])); // delete
        $this->assertEquals(1, $ret[$i++]); // zadd

        $this->assertFalse($ret[$i++]); // get
        $this->assertFalse($ret[$i++]); // getset
        $this->assertFalse($ret[$i++]); // append
        $this->assertFalse($ret[$i++]); // getRange
        $this->assertEquals([false], $ret[$i++]); // mget
        $this->assertFalse($ret[$i++]); // incr
        $this->assertFalse($ret[$i++]); // incrBy
        $this->assertFalse($ret[$i++]); // decr
        $this->assertFalse($ret[$i++]); // decrBy

        $this->assertFalse($ret[$i++]); // rpush
        $this->assertFalse($ret[$i++]); // lpush
        $this->assertFalse($ret[$i++]); // llen
        $this->assertFalse($ret[$i++]); // lpop
        $this->assertFalse($ret[$i++]); // lrange
        $this->assertFalse($ret[$i++]); // ltrim
        $this->assertFalse($ret[$i++]); // lindex
        $this->assertFalse($ret[$i++]); // lset
        $this->assertFalse($ret[$i++]); // lrem
        $this->assertFalse($ret[$i++]); // lpop
        $this->assertFalse($ret[$i++]); // rpop
        $this->assertFalse($ret[$i++]); // rpoplush

        $this->assertFalse($ret[$i++]); // zadd
        $this->assertFalse($ret[$i++]); // zrem
        $this->assertFalse($ret[$i++]); // zincrby
        $this->assertFalse($ret[$i++]); // zrank
        $this->assertFalse($ret[$i++]); // zrevrank
        $this->assertFalse($ret[$i++]); // zrange
        $this->assertFalse($ret[$i++]); // zreverserange
        $this->assertFalse($ret[$i++]); // zrangebyscore
        $this->assertFalse($ret[$i++]); // zcount
        $this->assertFalse($ret[$i++]); // zcard
        $this->assertFalse($ret[$i++]); // zscore
        $this->assertFalse($ret[$i++]); // zremrangebyrank
        $this->assertFalse($ret[$i++]); // zremrangebyscore

        $this->assertFalse($ret[$i++]); // hset
        $this->assertFalse($ret[$i++]); // hget
        $this->assertFalse($ret[$i++]); // hmget
        $this->assertFalse($ret[$i++]); // hmset
        $this->assertFalse($ret[$i++]); // hincrby
        $this->assertFalse($ret[$i++]); // hexists
        $this->assertFalse($ret[$i++]); // hdel
        $this->assertFalse($ret[$i++]); // hlen
        $this->assertFalse($ret[$i++]); // hkeys
        $this->assertFalse($ret[$i++]); // hvals
        $this->assertFalse($ret[$i++]); // hgetall

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
            ->lSet($key, 0, 'newValue')
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
        $this->assertIsArray($ret);
        $this->assertTrue(is_long($ret[$i++])); // delete
        $this->assertEquals(1, $ret[$i++]); // zadd

        $this->assertFalse($ret[$i++]); // get
        $this->assertFalse($ret[$i++]); // getset
        $this->assertFalse($ret[$i++]); // append
        $this->assertFalse($ret[$i++]); // getRange
        $this->assertEquals([false], $ret[$i++]); // mget
        $this->assertFalse($ret[$i++]); // incr
        $this->assertFalse($ret[$i++]); // incrBy
        $this->assertFalse($ret[$i++]); // decr
        $this->assertFalse($ret[$i++]); // decrBy

        $this->assertFalse($ret[$i++]); // rpush
        $this->assertFalse($ret[$i++]); // lpush
        $this->assertFalse($ret[$i++]); // llen
        $this->assertFalse($ret[$i++]); // lpop
        $this->assertFalse($ret[$i++]); // lrange
        $this->assertFalse($ret[$i++]); // ltrim
        $this->assertFalse($ret[$i++]); // lindex
        $this->assertFalse($ret[$i++]); // lset
        $this->assertFalse($ret[$i++]); // lrem
        $this->assertFalse($ret[$i++]); // lpop
        $this->assertFalse($ret[$i++]); // rpop
        $this->assertFalse($ret[$i++]); // rpoplush

        $this->assertFalse($ret[$i++]); // sadd
        $this->assertFalse($ret[$i++]); // srem
        $this->assertFalse($ret[$i++]); // spop
        $this->assertFalse($ret[$i++]); // smove
        $this->assertFalse($ret[$i++]); // scard
        $this->assertFalse($ret[$i++]); // sismember
        $this->assertFalse($ret[$i++]); // sinter
        $this->assertFalse($ret[$i++]); // sunion
        $this->assertFalse($ret[$i++]); // sdiff
        $this->assertFalse($ret[$i++]); // smembers
        $this->assertFalse($ret[$i++]); // srandmember

        $this->assertFalse($ret[$i++]); // hset
        $this->assertFalse($ret[$i++]); // hget
        $this->assertFalse($ret[$i++]); // hmget
        $this->assertFalse($ret[$i++]); // hmset
        $this->assertFalse($ret[$i++]); // hincrby
        $this->assertFalse($ret[$i++]); // hexists
        $this->assertFalse($ret[$i++]); // hdel
        $this->assertFalse($ret[$i++]); // hlen
        $this->assertFalse($ret[$i++]); // hkeys
        $this->assertFalse($ret[$i++]); // hvals
        $this->assertFalse($ret[$i++]); // hgetall

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
            ->lSet($key, 0, 'newValue')
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
        $this->assertIsArray($ret);
        $this->assertTrue(is_long($ret[$i++])); // delete
        $this->assertEquals(1, $ret[$i++]); // hset

        $this->assertFalse($ret[$i++]); // get
        $this->assertFalse($ret[$i++]); // getset
        $this->assertFalse($ret[$i++]); // append
        $this->assertFalse($ret[$i++]); // getRange
        $this->assertEquals([false], $ret[$i++]); // mget
        $this->assertFalse($ret[$i++]); // incr
        $this->assertFalse($ret[$i++]); // incrBy
        $this->assertFalse($ret[$i++]); // decr
        $this->assertFalse($ret[$i++]); // decrBy

        $this->assertFalse($ret[$i++]); // rpush
        $this->assertFalse($ret[$i++]); // lpush
        $this->assertFalse($ret[$i++]); // llen
        $this->assertFalse($ret[$i++]); // lpop
        $this->assertFalse($ret[$i++]); // lrange
        $this->assertFalse($ret[$i++]); // ltrim
        $this->assertFalse($ret[$i++]); // lindex
        $this->assertFalse($ret[$i++]); // lset
        $this->assertFalse($ret[$i++]); // lrem
        $this->assertFalse($ret[$i++]); // lpop
        $this->assertFalse($ret[$i++]); // rpop
        $this->assertFalse($ret[$i++]); // rpoplush

        $this->assertFalse($ret[$i++]); // sadd
        $this->assertFalse($ret[$i++]); // srem
        $this->assertFalse($ret[$i++]); // spop
        $this->assertFalse($ret[$i++]); // smove
        $this->assertFalse($ret[$i++]); // scard
        $this->assertFalse($ret[$i++]); // sismember
        $this->assertFalse($ret[$i++]); // sinter
        $this->assertFalse($ret[$i++]); // sunion
        $this->assertFalse($ret[$i++]); // sdiff
        $this->assertFalse($ret[$i++]); // smembers
        $this->assertFalse($ret[$i++]); // srandmember

        $this->assertFalse($ret[$i++]); // zadd
        $this->assertFalse($ret[$i++]); // zrem
        $this->assertFalse($ret[$i++]); // zincrby
        $this->assertFalse($ret[$i++]); // zrank
        $this->assertFalse($ret[$i++]); // zrevrank
        $this->assertFalse($ret[$i++]); // zrange
        $this->assertFalse($ret[$i++]); // zreverserange
        $this->assertFalse($ret[$i++]); // zrangebyscore
        $this->assertFalse($ret[$i++]); // zcount
        $this->assertFalse($ret[$i++]); // zcard
        $this->assertFalse($ret[$i++]); // zscore
        $this->assertFalse($ret[$i++]); // zremrangebyrank
        $this->assertFalse($ret[$i++]); // zremrangebyscore

        $this->assertEquals($i, count($ret));
    }

    public function testDifferentTypeString() {
        $key = '{hash}string';
        $dkey = '{hash}' . __FUNCTION__;

        $this->redis->del($key);
        $this->assertTrue($this->redis->set($key, 'value'));

        // lists I/F
        $this->assertFalse($this->redis->rPush($key, 'lvalue'));
        $this->assertFalse($this->redis->lPush($key, 'lvalue'));
        $this->assertFalse($this->redis->lLen($key));
        $this->assertFalse($this->redis->lPop($key));
        $this->assertFalse($this->redis->lrange($key, 0, -1));
        $this->assertFalse($this->redis->lTrim($key, 0, 1));
        $this->assertFalse($this->redis->lIndex($key, 0));
        $this->assertFalse($this->redis->lSet($key, 0, 'newValue'));
        $this->assertFalse($this->redis->lrem($key, 'lvalue', 1));
        $this->assertFalse($this->redis->lPop($key));
        $this->assertFalse($this->redis->rPop($key));
        $this->assertFalse($this->redis->rPoplPush($key, $dkey . 'lkey1'));

        // sets I/F
        $this->assertFalse($this->redis->sAdd($key, 'sValue1'));
        $this->assertFalse($this->redis->srem($key, 'sValue1'));
        $this->assertFalse($this->redis->sPop($key));
        $this->assertFalse($this->redis->sMove($key, $dkey . 'skey1', 'sValue1'));
        $this->assertFalse($this->redis->scard($key));
        $this->assertFalse($this->redis->sismember($key, 'sValue1'));
        $this->assertFalse($this->redis->sInter($key, $dkey. 'skey2'));
        $this->assertFalse($this->redis->sUnion($key, $dkey . 'skey4'));
        $this->assertFalse($this->redis->sDiff($key, $dkey . 'skey7'));
        $this->assertFalse($this->redis->sMembers($key));
        $this->assertFalse($this->redis->sRandMember($key));

        // sorted sets I/F
        $this->assertFalse($this->redis->zAdd($key, 1, 'zValue1'));
        $this->assertFalse($this->redis->zRem($key, 'zValue1'));
        $this->assertFalse($this->redis->zIncrBy($key, 1, 'zValue1'));
        $this->assertFalse($this->redis->zRank($key, 'zValue1'));
        $this->assertFalse($this->redis->zRevRank($key, 'zValue1'));
        $this->assertFalse($this->redis->zRange($key, 0, -1));
        $this->assertFalse($this->redis->zRevRange($key, 0, -1));
        $this->assertFalse($this->redis->zRangeByScore($key, 1, 2));
        $this->assertFalse($this->redis->zCount($key, 0, -1));
        $this->assertFalse($this->redis->zCard($key));
        $this->assertFalse($this->redis->zScore($key, 'zValue1'));
        $this->assertFalse($this->redis->zRemRangeByRank($key, 1, 2));
        $this->assertFalse($this->redis->zRemRangeByScore($key, 1, 2));

        // hash I/F
        $this->assertFalse($this->redis->hSet($key, 'key1', 'value1'));
        $this->assertFalse($this->redis->hGet($key, 'key1'));
        $this->assertFalse($this->redis->hMGet($key, ['key1']));
        $this->assertFalse($this->redis->hMSet($key, ['key1' => 'value1']));
        $this->assertFalse($this->redis->hIncrBy($key, 'key2', 1));
        $this->assertFalse($this->redis->hExists($key, 'key2'));
        $this->assertFalse($this->redis->hDel($key, 'key2'));
        $this->assertFalse($this->redis->hLen($key));
        $this->assertFalse($this->redis->hKeys($key));
        $this->assertFalse($this->redis->hVals($key));
        $this->assertFalse($this->redis->hGetAll($key));
    }

    public function testDifferentTypeList() {
        $key = '{hash}list';
        $dkey = '{hash}' . __FUNCTION__;

        $this->redis->del($key);
        $this->assertEquals(1, $this->redis->lPush($key, 'value'));

        // string I/F
        $this->assertFalse($this->redis->get($key));
        $this->assertFalse($this->redis->getset($key, 'value2'));
        $this->assertFalse($this->redis->append($key, 'append'));
        $this->assertFalse($this->redis->getRange($key, 0, 8));
        $this->assertEquals([FALSE], $this->redis->mget([$key]));
        $this->assertFalse($this->redis->incr($key));
        $this->assertFalse($this->redis->incrBy($key, 1));
        $this->assertFalse($this->redis->decr($key));
        $this->assertFalse($this->redis->decrBy($key, 1));

        // sets I/F
        $this->assertFalse($this->redis->sAdd($key, 'sValue1'));
        $this->assertFalse($this->redis->srem($key, 'sValue1'));
        $this->assertFalse($this->redis->sPop($key));
        $this->assertFalse($this->redis->sMove($key, $dkey . 'skey1', 'sValue1'));
        $this->assertFalse($this->redis->scard($key));
        $this->assertFalse($this->redis->sismember($key, 'sValue1'));
        $this->assertFalse($this->redis->sInter($key, $dkey . 'skey2'));
        $this->assertFalse($this->redis->sUnion($key, $dkey . 'skey4'));
        $this->assertFalse($this->redis->sDiff($key, $dkey . 'skey7'));
        $this->assertFalse($this->redis->sMembers($key));
        $this->assertFalse($this->redis->sRandMember($key));

        // sorted sets I/F
        $this->assertFalse($this->redis->zAdd($key, 1, 'zValue1'));
        $this->assertFalse($this->redis->zRem($key, 'zValue1'));
        $this->assertFalse($this->redis->zIncrBy($key, 1, 'zValue1'));
        $this->assertFalse($this->redis->zRank($key, 'zValue1'));
        $this->assertFalse($this->redis->zRevRank($key, 'zValue1'));
        $this->assertFalse($this->redis->zRange($key, 0, -1));
        $this->assertFalse($this->redis->zRevRange($key, 0, -1));
        $this->assertFalse($this->redis->zRangeByScore($key, 1, 2));
        $this->assertFalse($this->redis->zCount($key, 0, -1));
        $this->assertFalse($this->redis->zCard($key));
        $this->assertFalse($this->redis->zScore($key, 'zValue1'));
        $this->assertFalse($this->redis->zRemRangeByRank($key, 1, 2));
        $this->assertFalse($this->redis->zRemRangeByScore($key, 1, 2));

        // hash I/F
        $this->assertFalse($this->redis->hSet($key, 'key1', 'value1'));
        $this->assertFalse($this->redis->hGet($key, 'key1'));
        $this->assertFalse($this->redis->hMGet($key, ['key1']));
        $this->assertFalse($this->redis->hMSet($key, ['key1' => 'value1']));
        $this->assertFalse($this->redis->hIncrBy($key, 'key2', 1));
        $this->assertFalse($this->redis->hExists($key, 'key2'));
        $this->assertFalse($this->redis->hDel($key, 'key2'));
        $this->assertFalse($this->redis->hLen($key));
        $this->assertFalse($this->redis->hKeys($key));
        $this->assertFalse($this->redis->hVals($key));
        $this->assertFalse($this->redis->hGetAll($key));
    }

    public function testDifferentTypeSet() {
        $key = '{hash}set';
        $dkey = '{hash}' . __FUNCTION__;
        $this->redis->del($key);
        $this->assertEquals(1, $this->redis->sAdd($key, 'value'));

        // string I/F
        $this->assertFalse($this->redis->get($key));
        $this->assertFalse($this->redis->getset($key, 'value2'));
        $this->assertFalse($this->redis->append($key, 'append'));
        $this->assertFalse($this->redis->getRange($key, 0, 8));
        $this->assertEquals([FALSE], $this->redis->mget([$key]));
        $this->assertFalse($this->redis->incr($key));
        $this->assertFalse($this->redis->incrBy($key, 1));
        $this->assertFalse($this->redis->decr($key));
        $this->assertFalse($this->redis->decrBy($key, 1));

        // lists I/F
        $this->assertFalse($this->redis->rPush($key, 'lvalue'));
        $this->assertFalse($this->redis->lPush($key, 'lvalue'));
        $this->assertFalse($this->redis->lLen($key));
        $this->assertFalse($this->redis->lPop($key));
        $this->assertFalse($this->redis->lrange($key, 0, -1));
        $this->assertFalse($this->redis->lTrim($key, 0, 1));
        $this->assertFalse($this->redis->lIndex($key, 0));
        $this->assertFalse($this->redis->lSet($key, 0, 'newValue'));
        $this->assertFalse($this->redis->lrem($key, 'lvalue', 1));
        $this->assertFalse($this->redis->lPop($key));
        $this->assertFalse($this->redis->rPop($key));
        $this->assertFalse($this->redis->rPoplPush($key, $dkey  . 'lkey1'));

        // sorted sets I/F
        $this->assertFalse($this->redis->zAdd($key, 1, 'zValue1'));
        $this->assertFalse($this->redis->zRem($key, 'zValue1'));
        $this->assertFalse($this->redis->zIncrBy($key, 1, 'zValue1'));
        $this->assertFalse($this->redis->zRank($key, 'zValue1'));
        $this->assertFalse($this->redis->zRevRank($key, 'zValue1'));
        $this->assertFalse($this->redis->zRange($key, 0, -1));
        $this->assertFalse($this->redis->zRevRange($key, 0, -1));
        $this->assertFalse($this->redis->zRangeByScore($key, 1, 2));
        $this->assertFalse($this->redis->zCount($key, 0, -1));
        $this->assertFalse($this->redis->zCard($key));
        $this->assertFalse($this->redis->zScore($key, 'zValue1'));
        $this->assertFalse($this->redis->zRemRangeByRank($key, 1, 2));
        $this->assertFalse($this->redis->zRemRangeByScore($key, 1, 2));

        // hash I/F
        $this->assertFalse($this->redis->hSet($key, 'key1', 'value1'));
        $this->assertFalse($this->redis->hGet($key, 'key1'));
        $this->assertFalse($this->redis->hMGet($key, ['key1']));
        $this->assertFalse($this->redis->hMSet($key, ['key1' => 'value1']));
        $this->assertFalse($this->redis->hIncrBy($key, 'key2', 1));
        $this->assertFalse($this->redis->hExists($key, 'key2'));
        $this->assertFalse($this->redis->hDel($key, 'key2'));
        $this->assertFalse($this->redis->hLen($key));
        $this->assertFalse($this->redis->hKeys($key));
        $this->assertFalse($this->redis->hVals($key));
        $this->assertFalse($this->redis->hGetAll($key));
    }

    public function testDifferentTypeSortedSet() {
        $key = '{hash}sortedset';
        $dkey = '{hash}' . __FUNCTION__;

        $this->redis->del($key);
        $this->assertEquals(1, $this->redis->zAdd($key, 0, 'value'));

        // string I/F
        $this->assertFalse($this->redis->get($key));
        $this->assertFalse($this->redis->getset($key, 'value2'));
        $this->assertFalse($this->redis->append($key, 'append'));
        $this->assertFalse($this->redis->getRange($key, 0, 8));
        $this->assertEquals([FALSE], $this->redis->mget([$key]));
        $this->assertFalse($this->redis->incr($key));
        $this->assertFalse($this->redis->incrBy($key, 1));
        $this->assertFalse($this->redis->decr($key));
        $this->assertFalse($this->redis->decrBy($key, 1));

        // lists I/F
        $this->assertFalse($this->redis->rPush($key, 'lvalue'));
        $this->assertFalse($this->redis->lPush($key, 'lvalue'));
        $this->assertFalse($this->redis->lLen($key));
        $this->assertFalse($this->redis->lPop($key));
        $this->assertFalse($this->redis->lrange($key, 0, -1));
        $this->assertFalse($this->redis->lTrim($key, 0, 1));
        $this->assertFalse($this->redis->lIndex($key, 0));
        $this->assertFalse($this->redis->lSet($key, 0, 'newValue'));
        $this->assertFalse($this->redis->lrem($key, 'lvalue', 1));
        $this->assertFalse($this->redis->lPop($key));
        $this->assertFalse($this->redis->rPop($key));
        $this->assertFalse($this->redis->rPoplPush($key, $dkey . 'lkey1'));

        // sets I/F
        $this->assertFalse($this->redis->sAdd($key, 'sValue1'));
        $this->assertFalse($this->redis->srem($key, 'sValue1'));
        $this->assertFalse($this->redis->sPop($key));
        $this->assertFalse($this->redis->sMove($key, $dkey . 'skey1', 'sValue1'));
        $this->assertFalse($this->redis->scard($key));
        $this->assertFalse($this->redis->sismember($key, 'sValue1'));
        $this->assertFalse($this->redis->sInter($key, $dkey . 'skey2'));
        $this->assertFalse($this->redis->sUnion($key, $dkey . 'skey4'));
        $this->assertFalse($this->redis->sDiff($key, $dkey . 'skey7'));
        $this->assertFalse($this->redis->sMembers($key));
        $this->assertFalse($this->redis->sRandMember($key));

        // hash I/F
        $this->assertFalse($this->redis->hSet($key, 'key1', 'value1'));
        $this->assertFalse($this->redis->hGet($key, 'key1'));
        $this->assertFalse($this->redis->hMGet($key, ['key1']));
        $this->assertFalse($this->redis->hMSet($key, ['key1' => 'value1']));
        $this->assertFalse($this->redis->hIncrBy($key, 'key2', 1));
        $this->assertFalse($this->redis->hExists($key, 'key2'));
        $this->assertFalse($this->redis->hDel($key, 'key2'));
        $this->assertFalse($this->redis->hLen($key));
        $this->assertFalse($this->redis->hKeys($key));
        $this->assertFalse($this->redis->hVals($key));
        $this->assertFalse($this->redis->hGetAll($key));
    }

    public function testDifferentTypeHash() {
        $key = '{hash}hash';
        $dkey = '{hash}hash';

        $this->redis->del($key);
        $this->assertEquals(1, $this->redis->hSet($key, 'key', 'value'));

        // string I/F
        $this->assertFalse($this->redis->get($key));
        $this->assertFalse($this->redis->getset($key, 'value2'));
        $this->assertFalse($this->redis->append($key, 'append'));
        $this->assertFalse($this->redis->getRange($key, 0, 8));
        $this->assertEquals([FALSE], $this->redis->mget([$key]));
        $this->assertFalse($this->redis->incr($key));
        $this->assertFalse($this->redis->incrBy($key, 1));
        $this->assertFalse($this->redis->decr($key));
        $this->assertFalse($this->redis->decrBy($key, 1));

        // lists I/F
        $this->assertFalse($this->redis->rPush($key, 'lvalue'));
        $this->assertFalse($this->redis->lPush($key, 'lvalue'));
        $this->assertFalse($this->redis->lLen($key));
        $this->assertFalse($this->redis->lPop($key));
        $this->assertFalse($this->redis->lrange($key, 0, -1));
        $this->assertFalse($this->redis->lTrim($key, 0, 1));
        $this->assertFalse($this->redis->lIndex($key, 0));
        $this->assertFalse($this->redis->lSet($key, 0, 'newValue'));
        $this->assertFalse($this->redis->lrem($key, 'lvalue', 1));
        $this->assertFalse($this->redis->lPop($key));
        $this->assertFalse($this->redis->rPop($key));
        $this->assertFalse($this->redis->rPoplPush($key, $dkey . 'lkey1'));

        // sets I/F
        $this->assertFalse($this->redis->sAdd($key, 'sValue1'));
        $this->assertFalse($this->redis->srem($key, 'sValue1'));
        $this->assertFalse($this->redis->sPop($key));
        $this->assertFalse($this->redis->sMove($key, $dkey . 'skey1', 'sValue1'));
        $this->assertFalse($this->redis->scard($key));
        $this->assertFalse($this->redis->sismember($key, 'sValue1'));
        $this->assertFalse($this->redis->sInter($key, $dkey . 'skey2'));
        $this->assertFalse($this->redis->sUnion($key, $dkey . 'skey4'));
        $this->assertFalse($this->redis->sDiff($key, $dkey . 'skey7'));
        $this->assertFalse($this->redis->sMembers($key));
        $this->assertFalse($this->redis->sRandMember($key));

        // sorted sets I/F
        $this->assertFalse($this->redis->zAdd($key, 1, 'zValue1'));
        $this->assertFalse($this->redis->zRem($key, 'zValue1'));
        $this->assertFalse($this->redis->zIncrBy($key, 1, 'zValue1'));
        $this->assertFalse($this->redis->zRank($key, 'zValue1'));
        $this->assertFalse($this->redis->zRevRank($key, 'zValue1'));
        $this->assertFalse($this->redis->zRange($key, 0, -1));
        $this->assertFalse($this->redis->zRevRange($key, 0, -1));
        $this->assertFalse($this->redis->zRangeByScore($key, 1, 2));
        $this->assertFalse($this->redis->zCount($key, 0, -1));
        $this->assertFalse($this->redis->zCard($key));
        $this->assertFalse($this->redis->zScore($key, 'zValue1'));
        $this->assertFalse($this->redis->zRemRangeByRank($key, 1, 2));
        $this->assertFalse($this->redis->zRemRangeByScore($key, 1, 2));
    }

    public function testSerializerPHP() {
        $this->checkSerializer(Redis::SERIALIZER_PHP);

        // with prefix
        $this->redis->setOption(Redis::OPT_PREFIX, 'test:');
        $this->checkSerializer(Redis::SERIALIZER_PHP);
        $this->redis->setOption(Redis::OPT_PREFIX, '');
    }

    public function testSerializerIGBinary() {
        if ( ! defined('Redis::SERIALIZER_IGBINARY'))
            $this->markTestSkipped('Redis::SERIALIZER_IGBINARY is not defined');

        $this->checkSerializer(Redis::SERIALIZER_IGBINARY);

        // with prefix
        $this->redis->setOption(Redis::OPT_PREFIX, 'test:');
        $this->checkSerializer(Redis::SERIALIZER_IGBINARY);
        $this->redis->setOption(Redis::OPT_PREFIX, '');

        /* Test our igbinary header check logic.  The check allows us to do
           simple INCR type operations even with the serializer enabled, and
           should also protect against igbinary-like data from being erroneously
           deserialized */
        $this->redis->del('incrkey');

        $this->redis->set('spoof-1', "\x00\x00\x00\x00");
        $this->redis->set('spoof-2', "\x00\x00\x00\x00bad-version1");
        $this->redis->set('spoof-3', "\x00\x00\x00\x05bad-version2");
        $this->redis->setOption(Redis::OPT_SERIALIZER, Redis::SERIALIZER_IGBINARY);

        $this->assertEquals(16, $this->redis->incrby('incrkey', 16));
        $this->assertKeyEquals('16', 'incrkey');

        $this->assertKeyEquals("\x00\x00\x00\x00", 'spoof-1');
        $this->assertKeyEquals("\x00\x00\x00\x00bad-version1", 'spoof-2');
        $this->assertKeyEquals("\x00\x00\x00\x05bad-version2", 'spoof-3');
        $this->redis->setOption(Redis::OPT_SERIALIZER, Redis::SERIALIZER_NONE);

        $this->redis->del('incrkey', 'spoof-1', 'spoof-2', 'spoof-3');
    }

    public function testSerializerMsgPack() {
        if ( ! defined('Redis::SERIALIZER_MSGPACK'))
            $this->markTestSkipped('Redis::SERIALIZER_MSGPACK is not defined');

        $this->checkSerializer(Redis::SERIALIZER_MSGPACK);

        // with prefix
        $this->redis->setOption(Redis::OPT_PREFIX, 'test:');
        $this->checkSerializer(Redis::SERIALIZER_MSGPACK);
        $this->redis->setOption(Redis::OPT_PREFIX, '');
    }

    public function testSerializerJSON() {
        $this->checkSerializer(Redis::SERIALIZER_JSON);

        // with prefix
        $this->redis->setOption(Redis::OPT_PREFIX, 'test:');
        $this->checkSerializer(Redis::SERIALIZER_JSON);
        $this->redis->setOption(Redis::OPT_PREFIX, '');
    }

    private function checkSerializer($mode) {
        $this->redis->del('key');
        $this->assertEquals(Redis::SERIALIZER_NONE, $this->redis->getOption(Redis::OPT_SERIALIZER));   // default

        $this->assertTrue($this->redis->setOption(Redis::OPT_SERIALIZER, $mode));  // set ok
        $this->assertEquals($mode, $this->redis->getOption(Redis::OPT_SERIALIZER));    // get ok

        // lPush, rPush
        $a = ['hello world', 42, true, ['<tag>' => 1729]];
        $this->redis->del('key');
        $this->redis->lPush('key', $a[0]);
        $this->redis->rPush('key', $a[1]);
        $this->redis->rPush('key', $a[2]);
        $this->redis->rPush('key', $a[3]);

        // lrange
        $this->assertEquals($a, $this->redis->lrange('key', 0, -1));

        // lIndex
        $this->assertEquals($a[0], $this->redis->lIndex('key', 0));
        $this->assertEquals($a[1], $this->redis->lIndex('key', 1));
        $this->assertEquals($a[2], $this->redis->lIndex('key', 2));
        $this->assertEquals($a[3], $this->redis->lIndex('key', 3));

        // lrem
        $this->assertEquals(1, $this->redis->lrem('key', $a[3]));
        $this->assertEquals(array_slice($a, 0, 3), $this->redis->lrange('key', 0, -1));

        // lSet
        $a[0] = ['k' => 'v']; // update
        $this->assertTrue($this->redis->lSet('key', 0, $a[0]));
        $this->assertEquals($a[0], $this->redis->lIndex('key', 0));

        // lInsert
        $this->assertEquals(4, $this->redis->lInsert('key', Redis::BEFORE, $a[0], [1, 2, 3]));
        $this->assertEquals(5, $this->redis->lInsert('key', Redis::AFTER, $a[0], [4, 5, 6]));

        $a = [[1, 2, 3], $a[0], [4, 5, 6], $a[1], $a[2]];
        $this->assertEquals($a, $this->redis->lrange('key', 0, -1));

        // sAdd
        $this->redis->del('{set}key');
        $s = [1,'a', [1, 2, 3], ['k' => 'v']];

        $this->assertEquals(1, $this->redis->sAdd('{set}key', $s[0]));
        $this->assertEquals(1, $this->redis->sAdd('{set}key', $s[1]));
        $this->assertEquals(1, $this->redis->sAdd('{set}key', $s[2]));
        $this->assertEquals(1, $this->redis->sAdd('{set}key', $s[3]));

        // variadic sAdd
        $this->redis->del('k');
        $this->assertEquals(3, $this->redis->sAdd('k', 'a', 'b', 'c'));
        $this->assertEquals(1, $this->redis->sAdd('k', 'a', 'b', 'c', 'd'));

        // srem
        $this->assertEquals(1, $this->redis->srem('{set}key', $s[3]));
        $this->assertEquals(0, $this->redis->srem('{set}key', $s[3]));

        // variadic
        $this->redis->del('k');
        $this->redis->sAdd('k', 'a', 'b', 'c', 'd');
        $this->assertEquals(2, $this->redis->sRem('k', 'a', 'd'));
        $this->assertEquals(2, $this->redis->sRem('k', 'b', 'c', 'e'));
        $this->assertKeyMissing('k');

        // sismember
        $this->assertTrue($this->redis->sismember('{set}key', $s[0]));
        $this->assertTrue($this->redis->sismember('{set}key', $s[1]));
        $this->assertTrue($this->redis->sismember('{set}key', $s[2]));
        $this->assertFalse($this->redis->sismember('{set}key', $s[3]));
        unset($s[3]);

        // sMove
        $this->redis->del('{set}tmp');
        $this->redis->sMove('{set}key', '{set}tmp', $s[0]);
        $this->assertFalse($this->redis->sismember('{set}key', $s[0]));
        $this->assertTrue($this->redis->sismember('{set}tmp', $s[0]));
        unset($s[0]);

        // sorted sets
        $z = ['z0', ['k' => 'v'], FALSE, NULL];
        $this->redis->del('key');

        // zAdd
        $this->assertEquals(1, $this->redis->zAdd('key', 0, $z[0]));
        $this->assertEquals(1, $this->redis->zAdd('key', 1, $z[1]));
        $this->assertEquals(1, $this->redis->zAdd('key', 2, $z[2]));
        $this->assertEquals(1, $this->redis->zAdd('key', 3, $z[3]));

        // zRem
        $this->assertEquals(1, $this->redis->zRem('key', $z[3]));
        $this->assertEquals(0, $this->redis->zRem('key', $z[3]));
        unset($z[3]);

        // variadic
        $this->redis->del('k');
        $this->redis->zAdd('k', 0, 'a');
        $this->redis->zAdd('k', 1, 'b');
        $this->redis->zAdd('k', 2, 'c');
        $this->assertEquals(2, $this->redis->zRem('k', 'a', 'c'));
        $this->assertEquals(1.0, $this->redis->zScore('k', 'b'));
        $this->assertEquals(['b' => 1.0], $this->redis->zRange('k', 0, -1, true));

        // zRange
        $this->assertEquals($z, $this->redis->zRange('key', 0, -1));

        // zScore
        $this->assertEquals(0.0, $this->redis->zScore('key', $z[0]));
        $this->assertEquals(1.0, $this->redis->zScore('key', $z[1]));
        $this->assertEquals(2.0, $this->redis->zScore('key', $z[2]));

        // zRank
        $this->assertEquals(0, $this->redis->zRank('key', $z[0]));
        $this->assertEquals(1, $this->redis->zRank('key', $z[1]));
        $this->assertEquals(2, $this->redis->zRank('key', $z[2]));

        // zRevRank
        $this->assertEquals(2, $this->redis->zRevRank('key', $z[0]));
        $this->assertEquals(1, $this->redis->zRevRank('key', $z[1]));
        $this->assertEquals(0, $this->redis->zRevRank('key', $z[2]));

        // zIncrBy
        $this->assertEquals(3.0, $this->redis->zIncrBy('key', 1.0, $z[2]));
        $this->assertEquals(3.0, $this->redis->zScore('key', $z[2]));

        $this->assertEquals(5.0, $this->redis->zIncrBy('key', 2.0, $z[2]));
        $this->assertEquals(5.0, $this->redis->zScore('key', $z[2]));

        $this->assertEquals(2.0, $this->redis->zIncrBy('key', -3.0, $z[2]));
        $this->assertEquals(2.0, $this->redis->zScore('key', $z[2]));

        // mset
        $a = ['k0' => 1, 'k1' => 42, 'k2' => NULL, 'k3' => FALSE, 'k4' => ['a' => 'b']];
        $this->assertTrue($this->redis->mset($a));
        foreach ($a as $k => $v) {
            $this->assertKeyEquals($v, $k);
        }

        $a = ['f0' => 1, 'f1' => 42, 'f2' => NULL, 'f3' => FALSE, 'f4' => ['a' => 'b']];

        // hSet
        $this->redis->del('hash');
        foreach ($a as $k => $v) {
            $this->assertEquals(1, $this->redis->hSet('hash', $k, $v));
        }

        // hGet
        foreach ($a as $k => $v) {
            $this->assertEquals($v, $this->redis->hGet('hash', $k));
        }

        // hGetAll
        $this->assertEquals($a, $this->redis->hGetAll('hash'));
        $this->assertTrue($this->redis->hExists('hash', 'f0'));
        $this->assertTrue($this->redis->hExists('hash', 'f1'));
        $this->assertTrue($this->redis->hExists('hash', 'f2'));
        $this->assertTrue($this->redis->hExists('hash', 'f3'));
        $this->assertTrue($this->redis->hExists('hash', 'f4'));

        // hMSet
        $this->redis->del('hash');
        $this->redis->hMSet('hash', $a);
        foreach ($a as $k => $v) {
            $this->assertEquals($v, $this->redis->hGet('hash', $k));
        }

        // hMget
        $hmget = $this->redis->hMget('hash', array_keys($a));
        foreach ($hmget as $k => $v) {
            $this->assertEquals($a[$k], $v);
        }

        // mGet
        $this->redis->set('a', NULL);
        $this->redis->set('b', FALSE);
        $this->redis->set('c', 42);
        $this->redis->set('d', ['x' => 'y']);

        $this->assertEquals([NULL, FALSE, 42, ['x' => 'y']], $this->redis->mGet(['a', 'b', 'c', 'd']));

        // pipeline
        if ($this->havePipeline()) {
            $this->sequence(Redis::PIPELINE);
        }

        // multi-exec
        if ($this->haveMulti()) {
            $this->sequence(Redis::MULTI);
        }

        $this->assertIsArray($this->redis->keys('*'));

        // issue #62, hgetall
        $this->redis->del('hash1');
        $this->redis->hSet('hash1', 'data', 'test 1');
        $this->redis->hSet('hash1', 'session_id', 'test 2');

        $data = $this->redis->hGetAll('hash1');
        $this->assertEquals('test 1', $data['data']);
        $this->assertEquals('test 2', $data['session_id']);

        // issue #145, serializer with objects.
        $this->redis->set('x', [new stdClass, new stdClass]);
        $x = $this->redis->get('x');
        $this->assertIsArray($x);
        if ($mode === Redis::SERIALIZER_JSON) {
            $this->assertIsArray($x[0]);
            $this->assertIsArray($x[1]);
        } else {
            $this->assertIsObject($x[0], 'stdClass');
            $this->assertIsObject($x[1], 'stdClass');
        }

        // revert
        $this->assertTrue($this->redis->setOption(Redis::OPT_SERIALIZER, Redis::SERIALIZER_NONE));     // set ok
        $this->assertEquals(Redis::SERIALIZER_NONE, $this->redis->getOption(Redis::OPT_SERIALIZER));       // get ok
    }

    public function testCompressionLZF() {
        if ( ! defined('Redis::COMPRESSION_LZF'))
            $this->markTestSkipped();

        /* Don't crash on improperly compressed LZF data */
        $payload = 'not-actually-lzf-compressed';
        $this->redis->set('badlzf', $payload);
        $this->redis->setOption(Redis::OPT_COMPRESSION, Redis::COMPRESSION_LZF);
        $this->assertKeyEquals($payload, 'badlzf');
        $this->redis->setOption(Redis::OPT_COMPRESSION, Redis::COMPRESSION_NONE);

        $this->checkCompression(Redis::COMPRESSION_LZF, 0);
    }

    public function testCompressionZSTD() {
        if ( ! defined('Redis::COMPRESSION_ZSTD'))
            $this->markTestSkipped();

        /* Issue 1936 regression.  Make sure we don't overflow on bad data */
        $this->redis->del('badzstd');
        $this->redis->set('badzstd', '123');
        $this->redis->setOption(Redis::OPT_COMPRESSION, Redis::COMPRESSION_ZSTD);
        $this->assertKeyEquals('123', 'badzstd');
        $this->redis->setOption(Redis::OPT_COMPRESSION, Redis::COMPRESSION_NONE);

        $this->checkCompression(Redis::COMPRESSION_ZSTD, 0);
        $this->checkCompression(Redis::COMPRESSION_ZSTD, 9);
    }


    public function testCompressionLZ4() {
        if ( ! defined('Redis::COMPRESSION_LZ4'))
            $this->markTestSkipped();

        $this->checkCompression(Redis::COMPRESSION_LZ4, 0);
        $this->checkCompression(Redis::COMPRESSION_LZ4, 9);
    }

    private function checkCompression($mode, $level) {
        $set_cmp = $this->redis->setOption(Redis::OPT_COMPRESSION, $mode);
        $this->assertTrue($set_cmp);
        if ($set_cmp !== true)
            return;

        $get_cmp = $this->redis->getOption(Redis::OPT_COMPRESSION);
        $this->assertEquals($get_cmp, $mode);
        if ($get_cmp !== $mode)
            return;

        $set_lvl = $this->redis->setOption(Redis::OPT_COMPRESSION_LEVEL, $level);
        $this->assertTrue($set_lvl);
        if ($set_lvl !== true)
            return;

        $get_lvl = $this->redis->getOption(Redis::OPT_COMPRESSION_LEVEL);
        $this->assertEquals($get_lvl, $level);
        if ($get_lvl !== $level)
            return;

        $val = 'xxxxxxxxxx';
        $this->redis->set('key', $val);
        $this->assertKeyEquals($val, 'key');

        /* Empty data */
        $this->redis->set('key', '');
        $this->assertKeyEquals('', 'key');

        /* Iterate through class sizes */
        for ($i = 1; $i <= 65536; $i *= 2) {
            foreach ([str_repeat('A', $i), random_bytes($i)] as $val) {
                $this->redis->set('key', $val);
                $this->assertKeyEquals($val, 'key');
            }
        }

        // Issue 1945. Ensure we decompress data with hmget.
        $this->redis->hset('hkey', 'data', 'abc');
        $this->assertEquals('abc', current($this->redis->hmget('hkey', ['data'])));
    }

    public function testDumpRestore() {

        if (version_compare($this->version, '2.5.0') < 0)
            $this->markTestSkipped();

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
        $this->assertKeyEquals('this-is-bar', 'foo');
        $this->assertKeyEquals('this-is-foo', 'bar');

        /* Test that we can REPLACE a key */
        $this->assertTrue($this->redis->set('foo', 'some-value'));
        $this->assertTrue($this->redis->restore('foo', 0, $d_bar, ['REPLACE']));

        /* Ensure we can set an absolute TTL */
        $this->assertTrue($this->redis->restore('foo', time() + 10, $d_bar, ['REPLACE', 'ABSTTL']));
        $this->assertLTE(10, $this->redis->ttl('foo'));

        /* Ensure we can set an IDLETIME */
        $this->assertTrue($this->redis->restore('foo', 0, $d_bar, ['REPLACE', 'IDLETIME' => 200]));
        $this->assertGT(100, $this->redis->object('idletime', 'foo'));

        /* We can't neccissarily check this depending on LRU policy, but at least attempt to use
           the FREQ option */
        $this->assertTrue($this->redis->restore('foo', 0, $d_bar, ['REPLACE', 'FREQ' => 200]));

        $this->redis->del('foo');
        $this->redis->del('bar');
    }

    public function testGetLastError() {
        // We shouldn't have any errors now
        $this->assertNull($this->redis->getLastError());

        // test getLastError with a regular command
        $this->redis->set('x', 'a');
        $this->assertFalse($this->redis->incr('x'));
        $incrError = $this->redis->getLastError();
        $this->assertGT(0, strlen($incrError));

        // clear error
        $this->redis->clearLastError();
        $this->assertNull($this->redis->getLastError());
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
        if (version_compare($this->version, '2.5.0') < 0)
            $this->markTestSkipped();

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
        $this->assertIsArray($result, 3);
        $this->assertTrue(is_array($result) && count(array_filter($result)) == 0);

        // Load them up
        $this->assertEquals($s1_sha, $this->redis->script('load', $s1_src));
        $this->assertEquals($s2_sha, $this->redis->script('load', $s2_src));
        $this->assertEquals($s3_sha, $this->redis->script('load', $s3_src));

        // They should all exist
        $result = $this->redis->script('exists', $s1_sha, $s2_sha, $s3_sha);
        $this->assertTrue(is_array($result) && count(array_filter($result)) == 3);
    }

    public function testEval() {
        if (version_compare($this->version, '2.5.0') < 0)
            $this->markTestSkipped();

        /* The eval_ro method uses the same underlying handlers as eval so we
           only need to verify we can call it. */
        if ($this->minVersionCheck('7.0.0'))
            $this->assertEquals('1.55', $this->redis->eval_ro("return '1.55'"));

        // Basic single line response tests
        $this->assertEquals(1, $this->redis->eval('return 1'));
        $this->assertEqualsWeak(1.55, $this->redis->eval("return '1.55'"));
        $this->assertEquals('hello, world', $this->redis->eval("return 'hello, world'"));

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
        $this->assertEquals(['a', 'b', 'c'], $list);

        // Use a script to return our zset
        $zset = $this->redis->eval("return redis.call('zrange', KEYS[1], 0, -1)", ['{eval-key}-zset'], 1);
        $this->assertEquals(['d', 'e', 'f'], $zset);

        // Test an empty MULTI BULK response
        $this->redis->del('{eval-key}-nolist');
        $empty_resp = $this->redis->eval("return redis.call('lrange', '{eval-key}-nolist', 0, -1)",
            ['{eval-key}-nolist'], 1);
        $this->assertEquals([], $empty_resp);

        // Now test a nested reply
        $nested_script = "
            return {
                1,2,3, {
                    redis.call('get', '{eval-key}-str1'),
                    redis.call('get', '{eval-key}-str2'),
                    redis.call('lrange', 'not-any-kind-of-list', 0, -1),
                    {
                        redis.call('zrange', '{eval-key}-zset', 0, -1),
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
                    ['d', 'e', 'f'],
                    ['a', 'b', 'c']
                ]
            ]
        ];

        // Now run our script, and check our values against each other
        $eval_result = $this->redis->eval($nested_script, ['{eval-key}-str1', '{eval-key}-str2', '{eval-key}-zset', '{eval-key}-list'], 4);
        $this->assertTrue(
            is_array($eval_result) &&
            count($this->array_diff_recursive($eval_result, $expected)) == 0
        );

        /*
         * Nested reply wihin a multi/pipeline block
         */

        $num_scripts = 10;

        $modes = [Redis::MULTI];
        if ($this->havePipeline()) $modes[] = Redis::PIPELINE;

        foreach ($modes as $mode) {
            $this->redis->multi($mode);
            for ($i = 0; $i < $num_scripts; $i++) {
                $this->redis->eval($nested_script, ['{eval-key}-dummy'], 1);
            }
            $replies = $this->redis->exec();

            foreach ($replies as $reply) {
                $this->assertTrue(
                    is_array($reply) &&
                    count($this->array_diff_recursive($reply, $expected)) == 0
                );
            }
        }

        /*
         * KEYS/ARGV
         */

        $args_script = 'return {KEYS[1],KEYS[2],KEYS[3],ARGV[1],ARGV[2],ARGV[3]}';
        $args_args   = ['{k}1', '{k}2', '{k}3', 'v1', 'v2', 'v3'];
        $args_result = $this->redis->eval($args_script, $args_args, 3);
        $this->assertEquals($args_args, $args_result);

        // turn on key prefixing
        $this->redis->setOption(Redis::OPT_PREFIX, 'prefix:');
        $args_result = $this->redis->eval($args_script, $args_args, 3);

        // Make sure our first three are prefixed
        for ($i = 0; $i < count($args_result); $i++) {
            if ($i < 3) {
                $this->assertEquals('prefix:' . $args_args[$i], $args_result[$i]);
            } else {
                $this->assertEquals($args_args[$i], $args_result[$i]);
            }
        }
    }

    public function testEvalSHA() {
        if (version_compare($this->version, '2.5.0') < 0)
            $this->markTestSkipped();

        // Flush any loaded scripts
        $this->redis->script('flush');

        // Non existent script (but proper sha1), and a random (not) sha1 string
        $this->assertFalse($this->redis->evalsha(sha1(uniqid())));
        $this->assertFalse($this->redis->evalsha('some-random-data'));

        // Load a script
        $cb  = uniqid(); // To ensure the script is new
        $scr = "local cb='$cb' return 1";
        $sha = sha1($scr);

        // Run it when it doesn't exist, run it with eval, and then run it with sha1
        $this->assertFalse($this->redis->evalsha($scr));
        $this->assertEquals(1, $this->redis->eval($scr));
        $this->assertEquals(1, $this->redis->evalsha($sha));

        /* Our evalsha_ro handler is the same as evalsha so just make sure
           we can invoke the command */
        if ($this->minVersionCheck('7.0.0'))
            $this->assertEquals(1, $this->redis->evalsha_ro($sha));
    }

    public function testSerialize() {
        $vals = [1, 1.5, 'one', ['here', 'is', 'an', 'array']];

        // Test with no serialization at all
        $this->assertEquals('test', $this->redis->_serialize('test'));
        $this->assertEquals('1', $this->redis->_serialize(1));
        $this->assertEquals('Array', $this->redis->_serialize([]));
        $this->assertEquals('Object', $this->redis->_serialize(new stdClass));

        foreach ($this->getSerializers() as $mode) {
            $enc = [];
            $dec = [];

            foreach ($vals as $k => $v) {
                $enc = $this->redis->_serialize($v);
                $dec = $this->redis->_unserialize($enc);

                // They should be the same
                $this->assertEquals($enc, $dec);
            }
        }
    }

    public function testUnserialize() {
        $vals = [1, 1.5,'one',['this', 'is', 'an', 'array']];

        /* We want to skip SERIALIZER_NONE because strict type checking will
           fail on the assertions (which is expected). */
        $serializers = array_filter($this->getSerializers(), function($v) {
            return $v != Redis::SERIALIZER_NONE;
        });

        foreach ($serializers as $mode) {
            $vals_enc = [];

            // Pass them through redis so they're serialized
            foreach ($vals as $key => $val) {
                $this->redis->setOption(Redis::OPT_SERIALIZER, $mode);

                $key = 'key' . ++$key;
                $this->redis->del($key);
                $this->redis->set($key, $val);

                // Clear serializer, get serialized value
                $this->redis->setOption(Redis::OPT_SERIALIZER, Redis::SERIALIZER_NONE);
                $vals_enc[] = $this->redis->get($key);
            }

            // Run through our array comparing values
            for ($i = 0; $i < count($vals); $i++) {
                // reset serializer
                $this->redis->setOption(Redis::OPT_SERIALIZER, $mode);
                $this->assertEquals($vals[$i], $this->redis->_unserialize($vals_enc[$i]));
                $this->redis->setOption(Redis::OPT_SERIALIZER, Redis::SERIALIZER_NONE);
            }
        }
    }

    public function testCompressHelpers() {
        $compressors = $this->getCompressors();

        $vals = ['foo', 12345, random_bytes(128), ''];

        $oldcmp = $this->redis->getOption(Redis::OPT_COMPRESSION);

        foreach ($compressors as $cmp) {
            foreach ($vals as $val) {
                $this->redis->setOption(Redis::OPT_COMPRESSION, $cmp);
                $this->redis->set('cmpkey', $val);

                /* Get the value raw */
                $this->redis->setOption(Redis::OPT_COMPRESSION, Redis::COMPRESSION_NONE);
                $raw = $this->redis->get('cmpkey');
                $this->redis->setOption(Redis::OPT_COMPRESSION, $cmp);

                $this->assertEquals($raw, $this->redis->_compress($val));

                $uncompressed = $this->redis->get('cmpkey');
                $this->assertEquals($uncompressed, $this->redis->_uncompress($raw));
            }
        }

        $this->redis->setOption(Redis::OPT_COMPRESSION, $oldcmp);
    }

    public function testPackHelpers() {
        list ($oldser, $oldcmp) = [
            $this->redis->getOption(Redis::OPT_SERIALIZER),
            $this->redis->getOption(Redis::OPT_COMPRESSION)
        ];

        foreach ($this->getSerializers() as $ser) {
            $compressors = $this->getCompressors();
            foreach ($compressors as $cmp) {
                $this->redis->setOption(Redis::OPT_SERIALIZER, $ser);
                $this->redis->setOption(Redis::OPT_COMPRESSION, $cmp);

		foreach (['foo', 12345, random_bytes(128), '', ['an', 'array']] as $v) {
                    /* Can only attempt the array if we're serializing */
                    if (is_array($v) && $ser == Redis::SERIALIZER_NONE)
                        continue;

                    $this->redis->set('packkey', $v);

                    /* Get the value raw */
                    $this->redis->setOption(Redis::OPT_SERIALIZER, Redis::SERIALIZER_NONE);
                    $this->redis->setOption(Redis::OPT_COMPRESSION, Redis::COMPRESSION_NONE);
                    $raw = $this->redis->get('packkey');
                    $this->redis->setOption(Redis::OPT_SERIALIZER, $ser);
                    $this->redis->setOption(Redis::OPT_COMPRESSION, $cmp);

                    $this->assertEquals($raw, $this->redis->_pack($v));

                    $unpacked = $this->redis->get('packkey');
		    $this->assertEquals($unpacked, $this->redis->_unpack($raw));
		}
	    }
        }

        $this->redis->setOption(Redis::OPT_SERIALIZER, $oldser);
        $this->redis->setOption(Redis::OPT_COMPRESSION, $oldcmp);
    }

    public function testPrefix() {
        // no prefix
        $this->redis->setOption(Redis::OPT_PREFIX, '');
        $this->assertEquals('key', $this->redis->_prefix('key'));

        // with a prefix
        $this->redis->setOption(Redis::OPT_PREFIX, 'some-prefix:');
        $this->assertEquals('some-prefix:key', $this->redis->_prefix('key'));

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
        $key = 'key:arr';
        $this->redis->del($key);

        foreach ([false => [], true => NULL] as $opt => $test) {
            $this->redis->setOption(Redis::OPT_NULL_MULTIBULK_AS_NULL, $opt);

            $r = $this->redis->rawCommand('BLPOP', $key, .05);
            $this->assertEquals($test, $r);

            $this->redis->multi();
            $this->redis->rawCommand('BLPOP', $key, .05);
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

    public function testConfig() {
        /* GET */
        $cfg = $this->redis->config('GET', 'timeout');
        $this->assertArrayKey($cfg, 'timeout');
        $sec = $cfg['timeout'];

        /* SET */
        foreach ([$sec + 30, $sec] as $val) {
            $this->assertTrue($this->redis->config('SET', 'timeout', $val));
            $cfg = $this->redis->config('GET', 'timeout');
            $this->assertArrayKey($cfg, 'timeout', function ($v) use ($val) {
                return $v == $val;
            });
        }

        /* RESETSTAT */
        $c1 = count($this->redis->info('commandstats'));
        $this->assertTrue($this->redis->config('resetstat'));
        $this->assertLT($c1, count($this->redis->info('commandstats')));

        /* Ensure invalid calls are handled by PhpRedis */
        foreach (['notacommand', 'get', 'set'] as $cmd) {
            $this->assertFalse(@$this->redis->config($cmd));
        }
        $this->assertFalse(@$this->redis->config('set', 'foo'));

        /* REWRITE.  We don't care if it actually works, just that the
           command be attempted */
        $res = $this->redis->config('rewrite');
        $this->assertIsBool($res);
        if ($res == false) {
            $this->assertPatternMatch('/.*config.*/', $this->redis->getLastError());
            $this->redis->clearLastError();
        }

        if ( ! $this->minVersionCheck('7.0.0'))
            return;

        /* Test getting multiple values */
        $settings = $this->redis->config('get', ['timeout', 'databases', 'set-max-intset-entries']);
        $this->assertTrue(is_array($settings) && isset($settings['timeout']) &&
                          isset($settings['databases']) && isset($settings['set-max-intset-entries']));

        /* Short circuit if the above assertion would have failed */
        if ( ! is_array($settings) || ! isset($settings['timeout']) || ! isset($settings['set-max-intset-entries']))
            return;

        list($timeout, $max_intset) = [$settings['timeout'], $settings['set-max-intset-entries']];

        $updates = [
            ['timeout' => $timeout + 30, 'set-max-intset-entries' => $max_intset + 128],
            ['timeout' => $timeout,      'set-max-intset-entries' => $max_intset],
        ];

        foreach ($updates as $update) {
            $this->assertTrue($this->redis->config('set', $update));
            $vals = $this->redis->config('get', array_keys($update));
            $this->assertEqualsWeak($vals, $update, true);
        }

        /* Make sure PhpRedis catches malformed multiple get/set calls */
        $this->assertFalse(@$this->redis->config('get', []));
        $this->assertFalse(@$this->redis->config('set', []));
        $this->assertFalse(@$this->redis->config('set', [0, 1, 2]));
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

        // Wait for the connection to time out.  On very old versions
        // of Redis we need to wait much longer (TODO:  Investigate
        // which version exactly)
        sleep($this->minVersionCheck('3.0.0') ? 2 : 11);

        // Make sure we're still using the same DB.
        $this->assertKeyEquals($value, $key);

        // Revert the setting.
        $this->redis->config('SET', 'timeout', $original_cfg['timeout']);
    }

    public function testTime() {
        if (version_compare($this->version, '2.5.0') < 0)
            $this->markTestSkipped();

        $time_arr = $this->redis->time();
        $this->assertTrue(is_array($time_arr) && count($time_arr) == 2 &&
                          strval(intval($time_arr[0])) === strval($time_arr[0]) &&
                          strval(intval($time_arr[1])) === strval($time_arr[1]));
    }

    public function testReadTimeoutOption() {
        $this->assertTrue(defined('Redis::OPT_READ_TIMEOUT'));

        $this->redis->setOption(Redis::OPT_READ_TIMEOUT, '12.3');
        $this->assertEquals(12.3, $this->redis->getOption(Redis::OPT_READ_TIMEOUT));
    }

    public function testIntrospection() {
        // Simple introspection tests
        $this->assertEquals($this->getHost(), $this->redis->getHost());
        $this->assertEquals($this->getPort(), $this->redis->getPort());
        $this->assertEquals($this->getAuth(), $this->redis->getAuth());
    }

    public function testTransferredBytes() {
        $this->redis->set('key', 'val');

        $this->redis->clearTransferredBytes();

        $get_tx_resp = "*3\r\n$3\r\nGET\r\n$3\r\nkey\r\n";
        $get_rx_resp = "$3\r\nval\r\n";

        $this->assertKeyEquals('val', 'key');
        list ($tx, $rx) = $this->redis->getTransferredBytes();
        $this->assertEquals(strlen($get_tx_resp), $tx);
        $this->assertEquals(strlen($get_rx_resp), $rx);

        $this->redis->clearTransferredBytes();

        $this->redis->multi()->get('key')->get('key')->exec();
        list($tx, $rx) = $this->redis->getTransferredBytes();

        $this->assertEquals($tx, strlen("*1\r\n$5\r\nMULTI\r\n*1\r\n$4\r\nEXEC\r\n") +
                                 2 * strlen($get_tx_resp));

        $this->assertEquals($rx, strlen("+OK\r\n") + strlen("+QUEUED\r\n+QUEUED\r\n") +
                                 strlen("*2\r\n")  + 2 * strlen($get_rx_resp));
    }

    /**
     * Scan and variants
     */

    protected function get_keyspace_count($db) {
        $info = $this->redis->info();
        if (isset($info[$db])) {
            $info = $info[$db];
            $info = explode(',', $info);
            $info = explode('=', $info[0]);
            return $info[1];
        } else {
            return 0;
        }
    }

    public function testScan() {
        if (version_compare($this->version, '2.8.0') < 0)
            $this->markTestSkipped();

        // Key count
        $key_count = $this->get_keyspace_count('db0');

        // Have scan retry
        $this->redis->setOption(Redis::OPT_SCAN, Redis::SCAN_RETRY);

        // Scan them all
        $it = NULL;
        while ($keys = $this->redis->scan($it)) {
            $key_count -= count($keys);
        }
        // Should have iterated all keys
        $this->assertEquals(0, $key_count);

        // Unique keys, for pattern matching
        $uniq = uniqid();
        for ($i = 0; $i < 10; $i++) {
            $this->redis->set($uniq . "::$i", "bar::$i");
        }

        // Scan just these keys using a pattern match
        $it = NULL;
        while ($keys = $this->redis->scan($it, "*$uniq*")) {
            $i -= count($keys);
        }
        $this->assertEquals(0, $i);

        // SCAN with type is scheduled for release in Redis 6.
        if (version_compare($this->version, '6.0.0') >= 0) {
            // Use a unique ID so we can find our type keys
            $id = uniqid();

            // Create some simple keys and lists
            for ($i = 0; $i < 3; $i++) {
                $simple = "simple:{$id}:$i";
                $list = "list:{$id}:$i";

                $this->redis->set($simple, $i);
                $this->redis->del($list);
                $this->redis->rpush($list, ['foo']);

                $keys['STRING'][] = $simple;
                $keys['LIST'][] = $list;
            }

            // Make sure we can scan for specific types
            foreach ($keys as $type => $vals) {
                foreach ([0, 10] as $count) {
                    $resp = [];

                    $it = NULL;
                    while ($scan = $this->redis->scan($it, "*$id*", $count, $type)) {
                        $resp = array_merge($resp, $scan);
                    }

                    $this->assertEqualsCanonicalizing($vals, $resp);
                }
            }
        }
    }

    public function testScanPrefix() {
        $keyid = uniqid();

        /* Set some keys with different prefixes */
        $prefixes = ['prefix-a:', 'prefix-b:'];
        foreach ($prefixes as $prefix) {
            $this->redis->setOption(Redis::OPT_PREFIX, $prefix);
            $this->redis->set("$keyid", 'LOLWUT');
            $all_keys["{$prefix}{$keyid}"] = true;
        }

        $this->redis->setOption(Redis::OPT_SCAN, Redis::SCAN_RETRY);
        $this->redis->setOption(Redis::OPT_SCAN, Redis::SCAN_PREFIX);

        foreach ($prefixes as $prefix) {
            $this->redis->setOption(Redis::OPT_PREFIX, $prefix);
            $it = NULL;
            $keys = $this->redis->scan($it, "*$keyid*");
            $this->assertEquals($keys, ["{$prefix}{$keyid}"]);
        }

        /* Unset the prefix option */
        $this->redis->setOption(Redis::OPT_SCAN, Redis::SCAN_NOPREFIX);

        $it = NULL;
        while ($keys = $this->redis->scan($it, "*$keyid*")) {
            foreach ($keys as $key) {
                unset($all_keys[$key]);
            }
        }

        /* Should have touched every key */
        $this->assertEquals(0, count($all_keys));
    }

    public function testMaxRetriesOption() {
        $maxRetriesExpected = 5;
        $this->redis->setOption(Redis::OPT_MAX_RETRIES, $maxRetriesExpected);
        $maxRetriesActual=$this->redis->getOption(Redis::OPT_MAX_RETRIES);
        $this->assertEquals($maxRetriesActual, $maxRetriesExpected);
    }

    public function testBackoffOptions() {
        $algorithms = [
            Redis::BACKOFF_ALGORITHM_DEFAULT,
            Redis::BACKOFF_ALGORITHM_CONSTANT,
            Redis::BACKOFF_ALGORITHM_UNIFORM,
            Redis::BACKOFF_ALGORITHM_EXPONENTIAL,
            Redis::BACKOFF_ALGORITHM_EQUAL_JITTER,
            Redis::BACKOFF_ALGORITHM_FULL_JITTER,
            Redis::BACKOFF_ALGORITHM_DECORRELATED_JITTER
        ];

        foreach ($algorithms as $algorithm) {
            $this->assertTrue($this->redis->setOption(Redis::OPT_BACKOFF_ALGORITHM, $algorithm));
            $this->assertEquals($algorithm, $this->redis->getOption(Redis::OPT_BACKOFF_ALGORITHM));
        }

        // Invalid algorithm
        $this->assertFalse($this->redis->setOption(Redis::OPT_BACKOFF_ALGORITHM, 55555));

        foreach ([Redis::OPT_BACKOFF_BASE, Redis::OPT_BACKOFF_CAP] as $option) {
            foreach ([500, 750] as $value) {
                $this->redis->setOption($option, $value);
                $this->assertEquals($value, $this->redis->getOption($option));
            }
        }
    }

    public function testHScan() {
        if (version_compare($this->version, '2.8.0') < 0)
            $this->markTestSkipped();

        // Never get empty sets
        $this->redis->setOption(Redis::OPT_SCAN, Redis::SCAN_RETRY);

        $this->redis->del('hash');
        $foo_mems = 0;

        for ($i = 0; $i < 100; $i++) {
            if ($i > 3) {
                $this->redis->hset('hash', "member:$i", "value:$i");
            } else {
                $this->redis->hset('hash', "foomember:$i", "value:$i");
                $foo_mems++;
            }
        }

        // Scan all of them
        $it = NULL;
        while ($keys = $this->redis->hscan('hash', $it)) {
            $i -= count($keys);
        }
        $this->assertEquals(0, $i);

        // Scan just *foomem* (should be 4)
        $it = NULL;
        while ($keys = $this->redis->hscan('hash', $it, '*foomember*')) {
            $foo_mems -= count($keys);
            foreach ($keys as $mem => $val) {
                $this->assertStringContains('member', $mem);
                $this->assertStringContains('value', $val);
            }
        }
        $this->assertEquals(0, $foo_mems);
    }

    public function testSScan() {
        if (version_compare($this->version, '2.8.0') < 0)
            $this->markTestSkipped();

        $this->redis->setOption(Redis::OPT_SCAN, Redis::SCAN_RETRY);

        $this->redis->del('set');
        for ($i = 0; $i < 100; $i++) {
            $this->redis->sadd('set', "member:$i");
        }

        // Scan all of them
        $it = NULL;
        while ($keys = $this->redis->sscan('set', $it)) {
            $i -= count($keys);
            foreach ($keys as $mem) {
                $this->assertStringContains('member', $mem);
            }
        }
        $this->assertEquals(0, $i);

        // Scan just ones with zero in them (0, 10, 20, 30, 40, 50, 60, 70, 80, 90)
        $it = NULL;
        $w_zero = 0;
        while ($keys = $this->redis->sscan('set', $it, '*0*')) {
            $w_zero += count($keys);
        }
        $this->assertEquals(10, $w_zero);
    }

    public function testZScan() {
        if (version_compare($this->version, '2.8.0') < 0)
            $this->markTestSkipped();

        $this->redis->setOption(Redis::OPT_SCAN, Redis::SCAN_RETRY);

        $this->redis->del('zset');

        [$t_score, $p_score, $p_count] = [0, 0, 0];
        for ($i = 0; $i < 2000; $i++) {
            if ($i < 10) {
                $this->redis->zadd('zset', $i, "pmem:$i");
                $p_score += $i;
                $p_count++;
            } else {
                $this->redis->zadd('zset', $i, "mem:$i");
            }

            $t_score += $i;
        }

        // Scan them all
        $it = NULL;
        while ($keys = $this->redis->zscan('zset', $it)) {
            foreach ($keys as $mem => $f_score) {
                $t_score -= $f_score;
                $i--;
            }
        }

        $this->assertEquals(0, $i);
        $this->assertEquals(0., $t_score);

        // Just scan 'pmem' members
        $it = NULL;
        $p_score_old = $p_score;
        $p_count_old = $p_count;
        while ($keys = $this->redis->zscan('zset', $it, '*pmem*')) {
            foreach ($keys as $mem => $f_score) {
                $p_score -= $f_score;
                $p_count -= 1;
            }
        }
        $this->assertEquals(0., $p_score);
        $this->assertEquals(0, $p_count);

        // Turn off retrying and we should get some empty results
        $this->redis->setOption(Redis::OPT_SCAN, Redis::SCAN_NORETRY);
        [$skips, $p_score, $p_count] = [0, $p_score_old, $p_count_old];

        $it = NULL;
        while (($keys = $this->redis->zscan('zset', $it, '*pmem*')) !== FALSE) {
            if (count($keys) == 0) $skips++;
            foreach ($keys as $mem => $f_score) {
                $p_score -= $f_score;
                $p_count -= 1;
            }
        }
        // We should still get all the keys, just with several empty results
        $this->assertGT(0, $skips);
        $this->assertEquals(0., $p_score);
        $this->assertEquals(0, $p_count);
    }

    /* Make sure we capture errors when scanning */
    public function testScanErrors() {
        $this->redis->set('scankey', 'simplekey');

        foreach (['sScan', 'hScan', 'zScan'] as $method) {
            $it = NULL;
            $this->redis->$method('scankey', $it);
            $this->assertEquals(0, strpos($this->redis->getLastError(), 'WRONGTYPE'));
        }
    }

    //
    // HyperLogLog (PF) commands
    //

    protected function createPFKey($key, $count) {
        $mems = [];
        for ($i = 0; $i < $count; $i++) {
            $mems[] = uniqid('pfmem:');
        }

        // Estimation by Redis
        $this->redis->pfAdd($key, $count);
    }

    public function testPFCommands() {
        if (version_compare($this->version, '2.8.9') < 0)
            $this->markTestSkipped();

        $mems = [];

        for ($i = 0; $i < 1000; $i++) {
            if ($i % 2 == 0) {
                $mems[] = uniqid();
            } else {
                $mems[] = $i;
            }
        }

        // How many keys to create
        $key_count = 10;

        // Iterate prefixing/serialization options
        foreach ($this->getSerializers() as $ser) {
            foreach (['', 'hl-key-prefix:'] as $prefix) {
                $keys = [];

                // Now add for each key
                for ($i = 0; $i < $key_count; $i++) {
                    $key    = "{key}:$i";
                    $keys[] = $key;

                    // Clean up this key
                    $this->redis->del($key);

                    // Add to our cardinality set, and confirm we got a valid response
                    $this->assertGT(0, $this->redis->pfadd($key, $mems));

                    // Grab estimated cardinality
                    $card = $this->redis->pfcount($key);
                    $this->assertIsInt($card);

                    // Count should be close
                    $this->assertBetween($card, count($mems) * .9, count($mems) * 1.1);

                    // The PFCOUNT on this key should be the same as the above returned response
                    $this->assertEquals($card, $this->redis->pfcount($key));
                }

                // Clean up merge key
                $this->redis->del('pf-merge-{key}');

                // Merge the counters
                $this->assertTrue($this->redis->pfmerge('pf-merge-{key}', $keys));

                // Validate our merged count
                $redis_card = $this->redis->pfcount('pf-merge-{key}');

                // Merged cardinality should still be roughly 1000
                $this->assertBetween($redis_card, count($mems) * .9,
                                     count($mems) * 1.1);

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
        if ( ! $this->minVersionCheck('3.2'))
            $this->markTestSkipped();

        $this->redis->del('geokey');

        /* Add them one at a time */
        foreach ($this->cities as $city => $longlat) {
            $this->assertEquals(1, $this->redis->geoadd('geokey', $longlat[0], $longlat[1], $city));
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
        if ( ! $this->minVersionCheck('3.2.0'))
            $this->markTestSkipped();

        /* Chico */
        $city = 'Chico';
        $lng = -121.837478;
        $lat = 39.728494;

        $this->addCities('{gk}');

        /* Pre tested with redis-cli.  We're just verifying proper delivery of distance and unit */
        if ($cmd == 'georadius' || $cmd == 'georadius_ro') {
            $this->assertEquals(['Chico'], $this->redis->$cmd('{gk}', $lng, $lat, 10, 'mi'));
            $this->assertEquals(['Gridley', 'Chico'], $this->redis->$cmd('{gk}', $lng, $lat, 30, 'mi'));
            $this->assertEquals(['Gridley', 'Chico'], $this->redis->$cmd('{gk}', $lng, $lat, 50, 'km'));
            $this->assertEquals(['Gridley', 'Chico'], $this->redis->$cmd('{gk}', $lng, $lat, 50000, 'm'));
            $this->assertEquals(['Gridley', 'Chico'], $this->redis->$cmd('{gk}', $lng, $lat, 150000, 'ft'));
            $args = [$cmd, '{gk}', $lng, $lat, 500, 'mi'];

            /* Test a bad COUNT argument */
            foreach ([-1, 0, 'notanumber'] as $count) {
                $this->assertFalse(@$this->redis->$cmd('{gk}', $lng, $lat, 10, 'mi', ['count' => $count]));
            }
        } else {
            $this->assertEquals(['Chico'], $this->redis->$cmd('{gk}', $city, 10, 'mi'));
            $this->assertEquals(['Gridley', 'Chico'], $this->redis->$cmd('{gk}', $city, 30, 'mi'));
            $this->assertEquals(['Gridley', 'Chico'], $this->redis->$cmd('{gk}', $city, 50, 'km'));
            $this->assertEquals(['Gridley', 'Chico'], $this->redis->$cmd('{gk}', $city, 50000, 'm'));
            $this->assertEquals(['Gridley', 'Chico'], $this->redis->$cmd('{gk}', $city, 150000, 'ft'));
            $args = [$cmd, '{gk}', $city, 500, 'mi'];

            /* Test a bad COUNT argument */
            foreach ([-1, 0, 'notanumber'] as $count) {
                $this->assertFalse(@$this->redis->$cmd('{gk}', $city, 10, 'mi', ['count' => $count]));
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
        if ( ! $this->minVersionCheck('3.2.0'))
            $this->markTestSkipped();

        $this->genericGeoRadiusTest('georadius');
        $this->genericGeoRadiusTest('georadius_ro');
    }

    public function testGeoRadiusByMember() {
        if ( ! $this->minVersionCheck('3.2.0'))
            $this->markTestSkipped();

        $this->genericGeoRadiusTest('georadiusbymember');
        $this->genericGeoRadiusTest('georadiusbymember_ro');
    }

    public function testGeoPos() {
        if ( ! $this->minVersionCheck('3.2.0'))
            $this->markTestSkipped();

        $this->addCities('gk');
        $this->assertEquals($this->rawCommandArray('gk', ['geopos', 'gk', 'Chico', 'Sacramento']), $this->redis->geopos('gk', 'Chico', 'Sacramento'));
        $this->assertEquals($this->rawCommandArray('gk', ['geopos', 'gk', 'Cupertino']), $this->redis->geopos('gk', 'Cupertino'));
    }

    public function testGeoHash() {
        if ( ! $this->minVersionCheck('3.2.0'))
            $this->markTestSkipped();

        $this->addCities('gk');
        $this->assertEquals($this->rawCommandArray('gk', ['geohash', 'gk', 'Chico', 'Sacramento']), $this->redis->geohash('gk', 'Chico', 'Sacramento'));
        $this->assertEquals($this->rawCommandArray('gk', ['geohash', 'gk', 'Chico']), $this->redis->geohash('gk', 'Chico'));
    }

    public function testGeoDist() {
        if ( ! $this->minVersionCheck('3.2.0'))
            $this->markTestSkipped();

        $this->addCities('gk');

        $r1 = $this->redis->geodist('gk', 'Chico', 'Cupertino');
        $r2 = $this->rawCommandArray('gk', ['geodist', 'gk', 'Chico', 'Cupertino']);
        $this->assertEquals(round($r1, 8), round($r2, 8));

        $r1 = $this->redis->geodist('gk', 'Chico', 'Cupertino', 'km');
        $r2 = $this->rawCommandArray('gk', ['geodist', 'gk', 'Chico', 'Cupertino', 'km']);
        $this->assertEquals(round($r1, 8), round($r2, 8));
    }

    public function testGeoSearch() {
        if ( ! $this->minVersionCheck('6.2.0'))
            $this->markTestSkipped();

        $this->addCities('gk');

        $this->assertEquals(['Chico'], $this->redis->geosearch('gk', 'Chico', 1, 'm'));
        $this->assertValidate($this->redis->geosearch('gk', 'Chico', 1, 'm', ['withcoord', 'withdist', 'withhash']), function ($v) {
            $this->assertArrayKey($v, 'Chico', 'is_array');
            $this->assertEquals(count($v['Chico']), 3);
            $this->assertArrayKey($v['Chico'], 0, 'is_float');
            $this->assertArrayKey($v['Chico'], 1, 'is_int');
            $this->assertArrayKey($v['Chico'], 2, 'is_array');
            return true;
        });
    }

    public function testGeoSearchStore() {
        if ( ! $this->minVersionCheck('6.2.0'))
            $this->markTestSkipped();

        $this->addCities('{gk}src');
        $this->assertEquals(3, $this->redis->geosearchstore('{gk}dst', '{gk}src', 'Chico', 100, 'km'));
        $this->assertEquals(['Chico'], $this->redis->geosearch('{gk}dst', 'Chico', 1, 'm'));
    }

    /* Test a 'raw' command */
    public function testRawCommand() {
        $key = uniqid();

        $this->redis->set($key,'some-value');
        $result = $this->redis->rawCommand('get', $key);
        $this->assertEquals($result, 'some-value');

        $this->redis->del('mylist');
        $this->redis->rpush('mylist', 'A', 'B', 'C', 'D');
        $this->assertEquals(['A', 'B', 'C', 'D'], $this->redis->lrange('mylist', 0, -1));
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

    protected function addStreamsAndGroups($streams, $count, $groups) {
        $ids = [];

        foreach ($streams as $stream) {
            $ids[$stream] = $this->addStreamEntries($stream, $count);
            foreach ($groups as $group => $id) {
                $this->redis->xGroup('CREATE', $stream, $group, $id);
            }
        }

        return $ids;
    }

    public function testXAdd() {
        if ( ! $this->minVersionCheck('5.0'))
            $this->markTestSkipped();

        $this->redis->del('stream');
        for ($i = 0; $i < 5; $i++) {
            $id = $this->redis->xAdd('stream', '*', ['k1' => 'v1', 'k2' => 'v2']);
            $this->assertEquals($i+1, $this->redis->xLen('stream'));

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
        $this->assertEquals(10, $this->redis->xLen('stream'));

        /* Not the greatest test but I'm unsure if approximate trimming is
         * totally deterministic, so just make sure we are able to add with
         * an approximate maxlen argument structure */
        $id = $this->redis->xAdd('stream', '*', ['k' => 'v'], 10, true);
        $this->assertEquals(count(explode('-', $id)), 2);

        /* Empty message should fail */
        @$this->redis->xAdd('stream', '*', []);
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
        if ( ! $this->minVersionCheck('5.0'))
            $this->markTestSkipped();

        foreach ([false, true] as $reverse) {
            foreach ($this->getSerializers() as $serializer) {
                foreach ([NULL, 'prefix:'] as $prefix) {
                    $this->redis->setOption(Redis::OPT_PREFIX, $prefix);
                    $this->redis->setOption(Redis::OPT_SERIALIZER, $serializer);
                    $this->doXRangeTest($reverse);
                }
            }
        }
    }

    protected function testXLen() {
        if ( ! $this->minVersionCheck('5.0'))
            $this->markTestSkipped();

        $this->redis->del('{stream}');
        for ($i = 0; $i < 5; $i++) {
            $this->redis->xadd('{stream}', '*', ['foo' => 'bar']);
            $this->assertEquals($i+1, $this->redis->xLen('{stream}'));
        }
    }

    public function testXGroup() {
        if ( ! $this->minVersionCheck('5.0'))
            $this->markTestSkipped();

        /* CREATE MKSTREAM */
        $key = 's:' . uniqid();
        $this->assertFalse($this->redis->xGroup('CREATE', $key, 'g0', 0));
        $this->assertTrue($this->redis->xGroup('CREATE', $key, 'g1', 0, true));

        /* XGROUP DESTROY */
        $this->assertEquals(1, $this->redis->xGroup('DESTROY', $key, 'g1'));

        /* Populate some entries in stream 's' */
        $this->addStreamEntries('s', 2);

        /* CREATE */
        $this->assertTrue($this->redis->xGroup('CREATE', 's', 'mygroup', '$'));
        $this->assertFalse($this->redis->xGroup('CREATE', 's', 'mygroup', 'BAD_ID'));

        /* BUSYGROUP */
        $this->redis->xGroup('CREATE', 's', 'mygroup', '$');
        $this->assertEquals(0, strpos($this->redis->getLastError(), 'BUSYGROUP'));

        /* SETID */
        $this->assertTrue($this->redis->xGroup('SETID', 's', 'mygroup', '$'));
        $this->assertFalse($this->redis->xGroup('SETID', 's', 'mygroup', 'BAD_ID'));

        $this->assertEquals(0, $this->redis->xGroup('DELCONSUMER', 's', 'mygroup', 'myconsumer'));

        if ( ! $this->minVersionCheck('6.2.0'))
            return;

        /* CREATECONSUMER */
        $this->assertEquals(1, $this->redis->del('s'));
        $this->assertTrue($this->redis->xgroup('create', 's', 'mygroup', '$', true));
        for ($i = 0; $i < 3; $i++) {
            $this->assertEquals(1, $this->redis->xgroup('createconsumer', 's', 'mygroup', "c:$i"));
            $info = $this->redis->xinfo('consumers', 's', 'mygroup');
            $this->assertIsArray($info, $i + 1);
            for ($j = 0; $j <= $i; $j++) {
                $this->assertTrue(isset($info[$j]) && isset($info[$j]['name']) && $info[$j]['name'] == "c:$j");
            }
        }

        /* Make sure we don't erroneously send options that don't belong to the operation */
        $this->assertEquals(1,
            $this->redis->xGroup('CREATECONSUMER', 's', 'mygroup', 'fake-consumer', true, 1337));

        /* Make sure we handle the case where the user doesn't send enough arguments */
        $this->redis->clearLastError();
        $this->assertFalse(@$this->redis->xGroup('CREATECONSUMER'));
        $this->assertNull($this->redis->getLastError());
        $this->assertFalse(@$this->redis->xGroup('create'));
        $this->assertNull($this->redis->getLastError());

        if ( ! $this->minVersionCheck('7.0.0'))
            return;

        /* ENTRIESREAD */
        $this->assertEquals(1, $this->redis->del('s'));
        $this->assertTrue($this->redis->xGroup('create', 's', 'mygroup', '$', true, 1337));
        $info = $this->redis->xinfo('groups', 's');
        $this->assertTrue(isset($info[0]['entries-read']) && 1337 == (int)$info[0]['entries-read']);
    }

    public function testXAck() {
        if ( ! $this->minVersionCheck('5.0'))
            $this->markTestSkipped();

        for ($n = 1; $n <= 3; $n++) {
            $this->addStreamsAndGroups(['{s}'], 3, ['g1' => 0]);
            $msg = $this->redis->xReadGroup('g1', 'c1', ['{s}' => '>']);

            /* Extract IDs */
            $smsg = array_shift($msg);
            $ids = array_keys($smsg);

            /* Now ACK $n messages */
            $ids = array_slice($ids, 0, $n);
            $this->assertEquals($n, $this->redis->xAck('{s}', 'g1', $ids));
        }

        /* Verify sending no IDs is a failure */
        $this->assertFalse($this->redis->xAck('{s}', 'g1', []));
    }

    protected function doXReadTest() {
        if ( ! $this->minVersionCheck('5.0'))
            $this->markTestSkipped();

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

        /* Empty query should fail */
        $this->assertFalse(@$this->redis->xRead([]));
    }

    public function testXRead() {
        if ( ! $this->minVersionCheck('5.0'))
            $this->markTestSkipped();

        foreach ($this->getSerializers() as $serializer) {
            $this->redis->setOption(Redis::OPT_SERIALIZER, $serializer);
            $this->doXReadTest();
        }

        /* Don't need to test BLOCK multiple times */
        $m1 = round(microtime(true)*1000);
        $this->redis->xRead(['somestream' => '$'], -1, 100);
        $m2 = round(microtime(true)*1000);
        $this->assertGT(99, $m2 - $m1);
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
        if ( ! $this->minVersionCheck('5.0'))
            $this->markTestSkipped();

        /* Create some streams and groups */
        $streams = ['{s}-1', '{s}-2'];
        $groups = ['group1' => 0, 'group2' => 0];

        /* I'm not totally sure why Redis behaves this way, but we have to
         * send '>' first and then send ID '0' for subsequent xReadGroup calls
         * or Redis will not return any messages.  This behavior changed from
         * redis 5.0.1 and 5.0.2 but doing it this way works for both versions. */
        $qcount = 0;
        $query1 = ['{s}-1' => '>', '{s}-2' => '>'];
        $query2 = ['{s}-1' => '0', '{s}-2' => '0'];

        $ids = $this->addStreamsAndGroups($streams, 1, $groups);

        /* Test that we get get the IDs we should */
        foreach (['group1', 'group2'] as $group) {
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
            $resp = $this->redis->xReadGroup('group1', 'consumer', $query1, $c);

            foreach ($resp as $stream => $smsg) {
                $this->assertEquals(count($smsg), $c);
            }
        }

        /* Test COUNT option with NULL (should be ignored) */
        $this->addStreamsAndGroups($streams, 3, $groups, NULL);
        $resp = $this->redis->xReadGroup('group1', 'consumer', $query1, NULL);
        foreach ($resp as $stream => $smsg) {
            $this->assertEquals(count($smsg), 3);
        }

        /* Finally test BLOCK with a sloppy timing test */
        $tm1 = $this->mstime();
        $qnew = ['{s}-1' => '>', '{s}-2' => '>'];
        $this->redis->xReadGroup('group1', 'c1', $qnew, 0, 100);
        $this->assertGTE(100, $this->mstime() - $tm1);

        /* Make sure passing NULL to block doesn't block */
        $tm1 = $this->mstime();
        $this->redis->xReadGroup('group1', 'c1', $qnew, NULL, NULL);
        $this->assertLT(100, $this->mstime() - $tm1);

        /* Make sure passing bad values to BLOCK or COUNT immediately fails */
        $this->assertFalse(@$this->redis->xReadGroup('group1', 'c1', $qnew, -1));
        $this->assertFalse(@$this->redis->xReadGroup('group1', 'c1', $qnew, NULL, -1));
    }

    public function testXPending() {
        if ( ! $this->minVersionCheck('5.0'))
            $this->markTestSkipped();

        $rows = 5;
        $this->addStreamsAndGroups(['s'], $rows, ['group' => 0]);

        $msg = $this->redis->xReadGroup('group', 'consumer', ['s' => 0]);
        $ids = array_keys($msg['s']);

        for ($n = count($ids); $n >= 0; $n--) {
            $xp = $this->redis->xPending('s', 'group');

            $this->assertEquals(count($ids), $xp[0]);

            /* Verify we're seeing the IDs themselves */
            for ($idx = 1; $idx <= 2; $idx++) {
                if ($xp[$idx]) {
                    $this->assertPatternMatch('/^[0-9].*-[0-9].*/', $xp[$idx]);
                }
            }

            if ($ids) {
                $id = array_shift($ids);
                $this->redis->xAck('s', 'group', [$id]);
            }
        }

        /* Ensure we can have NULL trailing arguments */
        $this->assertTrue(is_array($this->redis->xpending('s', 'group', '-', '+', 1, null)));
        $this->assertTrue(is_array($this->redis->xpending('s', 'group', NULL, NULL, -1, NULL)));
    }

    public function testXDel() {
        if ( ! $this->minVersionCheck('5.0'))
            $this->markTestSkipped();

        for ($n = 5; $n > 0; $n--) {
            $ids = $this->addStreamEntries('s', 5);
            $todel = array_slice($ids, 0, $n);
            $this->assertEquals(count($todel), $this->redis->xDel('s', $todel));
        }

        /* Empty array should fail */
        $this->assertFalse(@$this->redis->xDel('s', []));
    }

    public function testXTrim() {
        if ( ! $this->minVersionCheck('5.0'))
            $this->markTestSkipped();

        for ($maxlen = 0; $maxlen <= 50; $maxlen += 10) {
            $this->addStreamEntries('stream', 100);
            $trimmed = $this->redis->xTrim('stream', $maxlen);
            $this->assertEquals(100 - $maxlen, $trimmed);
        }

        /* APPROX trimming isn't easily deterministic, so just make sure we
           can call it with the flag */
        $this->addStreamEntries('stream', 100);
        $this->assertEquals(0, $this->redis->xTrim('stream', 1, true));

        /* We need Redis >= 6.2.0 for MINID and LIMIT options */
        if ( ! $this->minVersionCheck('6.2.0'))
            return;

        $this->assertEquals(1, $this->redis->del('stream'));

        /* Test minid by generating a stream with more than one */
        for ($i = 1; $i < 3; $i++) {
            for ($j = 0; $j < 3; $j++) {
                $this->redis->xadd('stream', "$i-$j", ['foo' => 'bar']);
            }
        }

        /* MINID of 2-0 */
        $this->assertEquals(3, $this->redis->xtrim('stream', 2, false, true));
        $this->assertEquals(['2-0', '2-1', '2-2'], array_keys($this->redis->xrange('stream', '0', '+')));

        /* TODO:  Figure oiut how to test LIMIT deterministically.  For now just
                  send a LIMIT and verify we don't get a failure from Redis. */
        $this->assertIsInt(@$this->redis->xtrim('stream', 2, false, false, 3));
    }

    /* XCLAIM is one of the most complicated commands, with a great deal of different options
     * The following test attempts to verify every combination of every possible option. */
    public function testXClaim() {
        if ( ! $this->minVersionCheck('5.0'))
            $this->markTestSkipped();

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
                        if ( ! $justid) $cids = array_keys($cids);

                        if ($min_idle_time == 0) {
                            $this->assertEquals($cids, $oids);

                            /* Append the FORCE option to our second stream where we have not already
                             * assigned to a PEL group */
                            $opts[] = 'FORCE';
                            $freturn = $this->redis->xClaim('f', 'group1', 'Test', 0, $fids, $opts);
                            if ( ! $justid) $freturn = array_keys($freturn);
                            $this->assertEquals($freturn, $fids);

                            if ($retrycount || $tvalue !== NULL) {
                                $pending = $this->redis->xPending('s', 'group1', 0, '+', 1, 'Pavlo');

                                if ($retrycount) {
                                    $this->assertEquals($pending[0][3], $retrycount);
                                }
                                if ($tvalue !== NULL) {
                                    if ($ttype == 'IDLE') {
                                        /* If testing IDLE the value must be >= what we set */
                                        $this->assertGTE($tvalue, $pending[0][2]);
                                    } else {
                                        /* Timing tests are notoriously irritating but I don't see
                                         * how we'll get >= 20,000 ms between XCLAIM and XPENDING no
                                         * matter how slow the machine/VM running the tests is */
                                        $this->assertLT(20000, $pending[0][2]);
                                    }
                                }
                            }
                        } else {
                            /* We're verifying that we get no messages when we've set 100 seconds
                             * as our idle time, which should match nothing */
                            $this->assertEquals([], $cids);
                        }
                    }
                }
            }
        }
    }

    /* Make sure our XAUTOCLAIM handler works */
    public function testXAutoClaim() {
        $this->redis->del('ships');
        $this->redis->xGroup('CREATE', 'ships', 'combatants', '0-0', true);

        // Test an empty xautoclaim reply
        $res = $this->redis->xAutoClaim('ships', 'combatants', 'Sisko', 0, '0-0');
        $this->assertTrue(is_array($res) && (count($res) == 2 || count($res) == 3));
        if (count($res) == 2) {
            $this->assertEquals(['0-0', []], $res);
        } else {
            $this->assertEquals(['0-0', [], []], $res);
        }

        $this->redis->xAdd('ships', '1424-74205', ['name' => 'Defiant']);

        // Consume the ['name' => 'Defiant'] message
        $this->redis->xReadGroup('combatants', "Jem'Hadar", ['ships' => '>'], 1);

        // The "Jem'Hadar" consumer has the message presently
        $pending = $this->redis->xPending('ships', 'combatants');
        $this->assertTrue($pending && isset($pending[3][0][0]) && $pending[3][0][0] == "Jem'Hadar");

        // Assume control of the pending message with a different consumer.
        $res = $this->redis->xAutoClaim('ships', 'combatants', 'Sisko', 0, '0-0');

        $this->assertTrue($res && (count($res) == 2 || count($res) == 3));
        $this->assertTrue(isset($res[1]['1424-74205']['name']) &&
                          $res[1]['1424-74205']['name'] == 'Defiant');

        // Now the 'Sisko' consumer should own the message
        $pending = $this->redis->xPending('ships', 'combatants');
        $this->assertTrue(isset($pending[3][0][0]) && $pending[3][0][0] == 'Sisko');
    }

    public function testXInfo() {
        if ( ! $this->minVersionCheck('5.0'))
            $this->markTestSkipped();

        /* Create some streams and groups */
        $stream = 's';
        $groups = ['g1' => 0, 'g2' => 0];
        $this->addStreamsAndGroups([$stream], 1, $groups);

        $info = $this->redis->xInfo('GROUPS', $stream);
        $this->assertIsArray($info);
        $this->assertEquals(count($info), count($groups));
        foreach ($info as $group) {
            $this->assertArrayKey($group, 'name');
            $this->assertArrayKey($groups, $group['name']);
        }

        $info = $this->redis->xInfo('STREAM', $stream);
        $this->assertIsArray($info);
        $this->assertArrayKey($info, 'groups', function ($v) use ($groups) {
            return count($groups) == $v;
        });

        foreach (['first-entry', 'last-entry'] as $key) {
            $this->assertArrayKey($info, $key, 'is_array');
        }

        /* Ensure that default/NULL arguments are ignored */
        $info = $this->redis->xInfo('STREAM', $stream, NULL);
        $this->assertIsArray($info);
        $info = $this->redis->xInfo('STREAM', $stream, NULL, -1);
        $this->assertIsArray($info);

        /* XINFO STREAM FULL [COUNT N] Requires >= 6.0.0 */
        if ( ! $this->minVersionCheck('6.0'))
            return;

        /* Add some items to the stream so we can test COUNT */
        for ($i = 0; $i < 5; $i++) {
            $this->redis->xAdd($stream, '*', ['foo' => 'bar']);
        }

        $info = $this->redis->xInfo('STREAM', $stream, 'full');
        $this->assertArrayKey($info, 'length', 'is_numeric');

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

        /* Make sure we can't erroneously send non-null args after null ones */
        $this->redis->clearLastError();
        $this->assertFalse(@$this->redis->xInfo('FOO', NULL, 'fail', 25));
        $this->assertNull($this->redis->getLastError());
        $this->assertFalse(@$this->redis->xInfo('FOO', NULL, NULL, -2));
        $this->assertNull($this->redis->getLastError());
    }

    /* Regression test for issue-1831 (XINFO STREAM on an empty stream) */
    public function testXInfoEmptyStream() {
        /* Configure an empty stream */
        $this->redis->del('s');
        $this->redis->xAdd('s', '*', ['foo' => 'bar']);
        $this->redis->xTrim('s', 0);

        $info = $this->redis->xInfo('STREAM', 's');

        $this->assertIsArray($info);
        $this->assertEquals(0, $info['length']);
        $this->assertNull($info['first-entry']);
        $this->assertNull($info['last-entry']);
    }

    public function testInvalidAuthArgs() {
        $client = $this->newInstance();

        $args = [
            [],
            [NULL, NULL],
            ['foo', 'bar', 'baz'],
            ['a', 'b', 'c', 'd'],
            ['a', 'b', 'c'],
            [['a', 'b'], 'a'],
            [['a', 'b', 'c']],
            [[NULL, 'pass']],
            [[NULL, NULL]],
        ];

        foreach ($args as $arg) {
            try {
                if (is_array($arg)) {
                    @call_user_func_array([$client, 'auth'], $arg);
                }
            } catch (Exception $ex) {
                unset($ex); /* Suppress intellisense warning */
            } catch (ArgumentCountError $ex) {
                unset($ex); /* Suppress intellisense warning */
            }
        }
    }

    public function testAcl() {
        if ( ! $this->minVersionCheck('6.0'))
            $this->markTestSkipped();

        /* ACL USERS/SETUSER */
        $this->assertTrue($this->redis->acl('SETUSER', 'admin', 'on', '>admin', '+@all'));
        $this->assertTrue($this->redis->acl('SETUSER', 'noperm', 'on', '>noperm', '-@all'));
        $this->assertInArray('default', $this->redis->acl('USERS'));

        /* Verify ACL GETUSER has the correct hash and is in 'nice' format */
        $admin = $this->redis->acl('GETUSER', 'admin');
        $this->assertInArray(hash('sha256', 'admin'), $admin['passwords']);

        /* Now nuke our 'admin' user and make sure it went away */
        $this->assertEquals(1, $this->redis->acl('DELUSER', 'admin'));
        $this->assertFalse(in_array('admin', $this->redis->acl('USERS')));

        /* Try to log in with a bad username/password */
        $this->assertThrowsMatch($this->redis,
            function($o) { $o->auth(['1337haxx00r', 'lolwut']); }, '/^WRONGPASS.*$/');

        /* We attempted a bad login.  We should have an ACL log entry */
        $log = $this->redis->acl('log');
        if ( !  $log || !is_array($log)) {
            $this->assert('Expected an array from ACL LOG, got: ' . var_export($log, true));
            return;
        }

        /* Make sure our ACL LOG entries are nice for the user */
        $entry = array_shift($log);
        $this->assertArrayKey($entry, 'age-seconds', 'is_numeric');
        $this->assertArrayKey($entry, 'count', 'is_int');

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
        if ( ! file_exists('/tmp/redis.sock'))
            $this->markTestSkipped();

        $sock_tests = [
            ['/tmp/redis.sock'],
            ['/tmp/redis.sock', null],
            ['/tmp/redis.sock', 0],
            ['/tmp/redis.sock', -1],
        ];

        try {
            foreach ($sock_tests as $args) {
                $redis = new Redis();

                if (count($args) == 2) {
                    @$redis->connect($args[0], $args[1]);
                } else {
                    @$redis->connect($args[0]);
                }
                if ($this->getAuth()) {
                    $this->assertTrue($redis->auth($this->getAuth()));
                }
                $this->assertTrue($redis->ping());
            }
        } catch (Exception $ex) {
            $this->assert("Exception: {$ex}");
        }
    }

    protected function detectRedis($host, $port) {
        $sock = @fsockopen($host, $port, $errno, $errstr, .1);
        if ( !  $sock)
            return false;

        stream_set_timeout($sock, 0, 100000);

        $ping_cmd = "*1\r\n$4\r\nPING\r\n";
        if (fwrite($sock, $ping_cmd) != strlen($ping_cmd))
            return false;

        return fread($sock, strlen("+PONG\r\n")) == "+PONG\r\n";
    }

    /* Test high ports if we detect Redis running there */
    public function testHighPorts() {
        $ports = array_filter(array_map(function ($port) {
            return $this->detectRedis('localhost', $port) ? $port : 0;
        }, [32768, 32769, 32770]));

        if ( ! $ports)
            $this->markTestSkipped();

        foreach ($ports as $port) {
            $redis = new Redis();
            try {
                @$redis->connect('localhost', $port);
                if ($this->getAuth()) {
                    $this->assertTrue($redis->auth($this->getAuth()));
                }
                $this->assertTrue($redis->ping());
            } catch(Exception $ex) {
                $this->assert("Exception: $ex");
            }
        }
    }

    protected function sessionRunner() {
        $this->getAuthParts($user, $pass);

        return (new SessionHelpers\Runner())
            ->prefix($this->sessionPrefix())
            ->handler($this->sessionSaveHandler())
            ->savePath($this->sessionSavePath());
    }

    protected function testRequiresMode(string $mode) {
        if (php_sapi_name() != $mode) {
            $this->markTestSkipped("Test requires PHP running in '$mode' mode");
        }
    }

    public function testSession_compression() {
        $this->testRequiresMode('cli');

        foreach ($this->getCompressors() as $name => $val) {
            $data = "testing_compression_$name";

            $runner = $this->sessionRunner()
                ->maxExecutionTime(300)
                ->lockingEnabled(true)
                ->lockWaitTime(-1)
                ->lockExpires(0)
                ->data($data)
                ->compression($name);

            $this->assertEquals('SUCCESS', $runner->execFg());

            $this->redis->setOption(Redis::OPT_COMPRESSION, $val);
            $this->assertPatternMatch("/.*$data.*/", $this->redis->get($runner->getSessionKey()));
            $this->redis->setOption(Redis::OPT_COMPRESSION, Redis::COMPRESSION_NONE);
        }
    }

    public function testSession_savedToRedis() {
        $this->testRequiresMode('cli');

        $runner = $this->sessionRunner();

        $this->assertEquals('SUCCESS', $runner->execFg());
        $this->assertKeyExists($runner->getSessionKey());
    }

    protected function sessionWaitUsec() {
        return ini_get('redis.session.lock_wait_time') *
               ini_get('redis.session.lock_retries');
    }

    protected function sessionWaitSec() {
        return $this->sessionWaitUsec() / 1000000.0;
    }

    public function testSession_lockKeyCorrect() {
        $this->testRequiresMode('cli');

        $runner = $this->sessionRunner()->sleep(5);

        $this->assertTrue($runner->execBg());

        if ( ! $runner->waitForLockKey($this->redis, $this->sessionWaitSec())) {
            $this->externalCmdFailure($runner->getCmd(), $runner->output(),
                                 "Failed waiting for session lock key '{$runner->getSessionLockKey()}'",
                                 $runner->getExitCode());
        }
    }

    public function testSession_lockingDisabledByDefault() {
        $this->testRequiresMode('cli');

        $runner = $this->sessionRunner()
            ->lockingEnabled(false)
            ->sleep(5);

        $this->assertEquals('SUCCESS', $runner->execFg());
        $this->assertKeyMissing($runner->getSessionLockKey());
    }

    public function testSession_lockReleasedOnClose() {
        $this->testRequiresMode('cli');

        $runner = $this->sessionRunner()
            ->sleep(1)
            ->lockingEnabled(true);

        $this->assertTrue($runner->execBg());
        usleep($this->sessionWaitUsec() + 100000);
        $this->assertKeyMissing($runner->getSessionLockKey());
    }

    public function testSession_lock_ttlMaxExecutionTime() {
        $this->testRequiresMode('cli');

        $runner1 = $this->sessionRunner()
            ->sleep(10)
            ->maxExecutionTime(2);

        $this->assertTrue($runner1->execBg());
        usleep(100000);

        $runner2 = $this->sessionRunner()
            ->id($runner1->getId())
            ->sleep(0);

        $st = microtime(true);
        $this->assertEquals('SUCCESS', $runner2->execFg());
        $el = microtime(true) - $st;
        $this->assertLT(4, $el);
    }

    public function testSession_lock_ttlLockExpire() {
        $this->testRequiresMode('cli');

        $runner1 = $this->sessionRunner()
            ->sleep(10)
            ->maxExecutionTime(300)
            ->lockingEnabled(true)
            ->lockExpires(2);

        $this->assertTrue($runner1->execBg());
        usleep(100000);

        $runner2 = $this->sessionRunner()
            ->id($runner1->getId())
            ->sleep(0);

        $st = microtime(true);
        $this->assertEquals('SUCCESS', $runner2->execFg());
        $this->assertLT(3, microtime(true) - $st);
    }

    public function testSession_lockHoldCheckBeforeWrite_otherProcessHasLock() {
        $this->testRequiresMode('cli');

        $id = 'test-id';

        $runner = $this->sessionRunner()
            ->sleep(2)
            ->lockingEnabled(true)
            ->lockExpires(1)
            ->data('firstProcess');

        $runner2 = $this->sessionRunner()
            ->id($runner->getId())
            ->sleep(0)
            ->lockingEnabled(true)
            ->lockExpires(10)
            ->data('secondProcess');

        $this->assertTrue($runner->execBg());
        usleep(1500000); // 1.5 sec
        $this->assertEquals('SUCCESS', $runner2->execFg());

        $this->assertEquals('secondProcess', $runner->getData());
    }

    public function testSession_lockHoldCheckBeforeWrite_nobodyHasLock() {
        $this->testRequiresMode('cli');

        $runner = $this->sessionRunner()
            ->sleep(2)
            ->lockingEnabled(true)
            ->lockExpires(1)
            ->data('firstProcess');

        $this->assertNotEquals('SUCCESS', $runner->execFg());
        $this->assertNotEquals('firstProcess', $runner->getData());
    }

    public function testSession_correctLockRetryCount() {
        $this->testRequiresMode('cli');

        $runner = $this->sessionRunner()
            ->sleep(10);

        $this->assertTrue($runner->execBg());
        if ( ! $runner->waitForLockKey($this->redis, 2)) {
            $this->externalCmdFailure($runner->getCmd(), $runner->output(),
                                      'Failed waiting for session lock key',
                                      $runner->getExitCode());
        }

        $runner2 = $this->sessionRunner()
            ->id($runner->getId())
            ->sleep(0)
            ->maxExecutionTime(10)
            ->lockingEnabled(true)
            ->lockWaitTime(100000)
            ->lockRetries(10);

        $st = microtime(true);
        $ex = $runner2->execFg();
        if (stripos($ex, 'SUCCESS') !== false) {
            $this->externalCmdFailure($runner2->getCmd(), $ex,
                                      'Expected failure but lock was acquired!',
                                      $runner2->getExitCode());
        }
        $et = microtime(true);

        $this->assertBetween($et - $st, 1, 3);
    }

    public function testSession_defaultLockRetryCount() {
        $this->testRequiresMode('cli');

        $runner = $this->sessionRunner()
            ->sleep(10);

        $runner2 = $this->sessionRunner()
            ->id($runner->getId())
            ->sleep(0)
            ->lockingEnabled(true)
            ->maxExecutionTime(10)
            ->lockWaitTime(20000)
            ->lockRetries(0);

        $this->assertTrue($runner->execBg());

        if ( ! $runner->waitForLockKey($this->redis, 3)) {
            $this->externalCmdFailure($runner->getCmd(), $runner->output(),
                                      'Failed waiting for session lock key',
                                      $runner->getExitCode());
        }

        $st = microtime(true);
        $this->assertNotEquals('SUCCESS', $runner2->execFg());
        $et = microtime(true);
        $this->assertBetween($et - $st, 2, 3);
    }

    public function testSession_noUnlockOfOtherProcess() {
        $this->testRequiresMode('cli');

        $st = microtime(true);

        $sleep = 3;

        $runner = $this->sessionRunner()
            ->sleep($sleep)
            ->maxExecutionTime(3);

        $tm1 = microtime(true);

        /* 1.  Start a background process, and wait until we are certain
         *     the lock was attained. */
        $this->assertTrue($runner->execBg());
        if ( ! $runner->waitForLockKey($this->redis, 1)) {
            $this->assert('Failed waiting for session lock key');
            return;
        }

        /* 2.  Attempt to lock the same session.  This should force us to
         *     wait until the first lock is released. */
        $runner2 = $this->sessionRunner()
            ->id($runner->getId())
            ->sleep(0);

        $tm2 = microtime(true);
        $this->assertEquals('SUCCESS', $runner2->execFg());
        $tm3 = microtime(true);

        /* 3. Verify we had to wait for this lock */
        $this->assertGTE($sleep - ($tm2 - $tm1), $tm3 - $tm2);
    }

    public function testSession_lockWaitTime() {
        $this->testRequiresMode('cli');

        $runner = $this->sessionRunner()
            ->sleep(1)
            ->maxExecutionTime(300);

        $runner2 = $this->sessionRunner()
            ->id($runner->getId())
            ->sleep(0)
            ->maxExecutionTime(300)
            ->lockingEnabled(true)
            ->lockWaitTime(3000000);

        $this->assertTrue($runner->execBg());
        usleep(100000);

        $st = microtime(true);
        $this->assertEquals('SUCCESS', $runner2->execFg());
        $et = microtime(true);

        $this->assertBetween($et - $st, 2.5, 3.5);
    }

    public function testMultipleConnect() {
        $host = $this->redis->GetHost();
        $port = $this->redis->GetPort();

        for ($i = 0; $i < 5; $i++) {
            $this->redis->connect($host, $port);
            if ($this->getAuth()) {
                $this->assertTrue($this->redis->auth($this->getAuth()));
            }
            $this->assertTrue($this->redis->ping());
        }
    }

    public function testConnectException() {
        $host = 'github.com';
        if (gethostbyname($host) === $host)
            $this->markTestSkipped('online test');

        $redis = new Redis();
        try {
            $redis->connect($host, 6379, 0.01);
        }  catch (Exception $e) {
            $this->assertStringContains('timed out', $e->getMessage());
        }
    }

    public function testTlsConnect() {
        if (($fp = @fsockopen($this->getHost(), 6378)) == NULL)
            $this->markTestSkipped();

        fclose($fp);

        foreach (['localhost' => true, '127.0.0.1' => false] as $host => $verify) {
            $redis = new Redis();
            $this->assertTrue($redis->connect('tls://' . $host, 6378, 0, null, 0, 0, [
                'stream' => ['verify_peer_name' => $verify, 'verify_peer' => false]
            ]));
        }
    }

    public function testReset() {
        if (version_compare($this->version, '6.2.0') < 0)
            $this->markTestSkipped();

        $this->assertTrue($this->redis->multi()->select(2)->set('foo', 'bar')->reset());
        $this->assertEquals(Redis::ATOMIC, $this->redis->getMode());
        $this->assertEquals(0, $this->redis->getDBNum());
    }

    public function testCopy() {
        if (version_compare($this->version, '6.2.0') < 0)
            $this->markTestSkipped();

        $this->redis->del('{key}dst');
        $this->redis->set('{key}src', 'foo');
        $this->assertTrue($this->redis->copy('{key}src', '{key}dst'));
        $this->assertKeyEquals('foo', '{key}dst');

        $this->redis->set('{key}src', 'bar');
        $this->assertFalse($this->redis->copy('{key}src', '{key}dst'));
        $this->assertKeyEquals('foo', '{key}dst');

        $this->assertTrue($this->redis->copy('{key}src', '{key}dst', ['replace' => true]));
        $this->assertKeyEquals('bar', '{key}dst');
    }

    public function testCommand() {
        $commands = $this->redis->command();
        $this->assertIsArray($commands);
        $this->assertEquals(count($commands), $this->redis->command('count'));

        if ( ! $this->is_keydb && $this->minVersionCheck('7.0')) {
            $infos = $this->redis->command('info');
            $this->assertIsArray($infos);
            $this->assertEquals(count($infos), count($commands));
        }

        if (version_compare($this->version, '7.0') >= 0) {
            $docs = $this->redis->command('docs');
            $this->assertIsArray($docs);
            $this->assertEquals(count($docs), 2 * count($commands));

            $list = $this->redis->command('list', 'filterby', 'pattern', 'lol*');
            $this->assertIsArray($list);
            $this->assertEquals(['lolwut'], $list);
        }
    }

    public function testFunction() {
        if (version_compare($this->version, '7.0') < 0)
            $this->markTestSkipped();

        $this->assertTrue($this->redis->function('flush', 'sync'));
        $this->assertEquals('mylib', $this->redis->function('load', "#!lua name=mylib\nredis.register_function('myfunc', function(keys, args) return args[1] end)"));
        $this->assertEquals('foo', $this->redis->fcall('myfunc', [], ['foo']));
        $payload = $this->redis->function('dump');
        $this->assertEquals('mylib', $this->redis->function('load', 'replace', "#!lua name=mylib\nredis.register_function{function_name='myfunc', callback=function(keys, args) return args[1] end, flags={'no-writes'}}"));
        $this->assertEquals('foo', $this->redis->fcall_ro('myfunc', [], ['foo']));
        $this->assertEquals(['running_script' => false, 'engines' => ['LUA' => ['libraries_count' => 1, 'functions_count' => 1]]], $this->redis->function('stats'));
        $this->assertTrue($this->redis->function('delete', 'mylib'));
        $this->assertTrue($this->redis->function('restore', $payload));
        $this->assertEquals([['library_name' => 'mylib', 'engine' => 'LUA', 'functions' => [['name' => 'myfunc', 'description' => false,'flags' => []]]]], $this->redis->function('list'));
        $this->assertTrue($this->redis->function('delete', 'mylib'));
    }

    protected function execWaitAOF() {
        return $this->redis->waitaof(0, 0, 0);
    }

    public function testWaitAOF() {
        if ( ! $this->minVersionCheck('7.2.0'))
            $this->markTestSkipped();

        $res = $this->execWaitAOF();
        $this->assertValidate($res, function ($v) {
            if ( ! is_array($v) || count($v) != 2)
                return false;
            return isset($v[0]) && is_int($v[0]) &&
                   isset($v[1]) && is_int($v[1]);
        });
    }

    /* Make sure we handle a bad option value gracefully */
    public function testBadOptionValue() {
        $this->assertFalse(@$this->redis->setOption(pow(2, 32), false));
    }

    protected function regenerateIdHelper(bool $lock, bool $destroy, bool $proxy) {
        $this->testRequiresMode('cli');

        $data   = uniqid('regenerate-id:');
        $runner = $this->sessionRunner()
            ->sleep(0)
            ->maxExecutionTime(300)
            ->lockingEnabled(true)
            ->lockRetries(1)
            ->data($data);

        $this->assertEquals('SUCCESS', $runner->execFg());

        $new_id = $runner->regenerateId($lock, $destroy, $proxy);

        $this->assertNotEquals($runner->getId(), $new_id);
        $this->assertEquals($runner->getData(), $runner->getData());
    }

    public  function testSession_regenerateSessionId_noLock_noDestroy() {
        $this->regenerateIdHelper(false, false, false);
    }

    public function testSession_regenerateSessionId_noLock_withDestroy() {
        $this->regenerateIdHelper(false, true, false);
    }

    public function testSession_regenerateSessionId_withLock_noDestroy() {
        $this->regenerateIdHelper(true, false, false);
    }

    public  function testSession_regenerateSessionId_withLock_withDestroy() {
        $this->regenerateIdHelper(true, true, false);
    }

    public  function testSession_regenerateSessionId_noLock_noDestroy_withProxy() {
        $this->regenerateIdHelper(false, false, true);
    }

    public  function testSession_regenerateSessionId_noLock_withDestroy_withProxy() {
        $this->regenerateIdHelper(false, true, true);
    }

    public  function testSession_regenerateSessionId_withLock_noDestroy_withProxy() {
        $this->regenerateIdHelper(true, false, true);
    }

    public  function testSession_regenerateSessionId_withLock_withDestroy_withProxy() {
        $this->regenerateIdHelper(true, true, true);
    }

    public function testSession_ttl_equalsToSessionLifetime() {
        $this->testRequiresMode('cli');

        $runner = $this->sessionRunner()->lifetime(600);
        $this->assertEquals('SUCCESS', $runner->execFg());
        $this->assertEquals(600, $this->redis->ttl($runner->getSessionKey()));
    }

    public function testSession_ttl_resetOnWrite() {
        $this->testRequiresMode('cli');

        $runner1 = $this->sessionRunner()->lifetime(600);
        $this->assertEquals('SUCCESS', $runner1->execFg());

        $runner2 = $this->sessionRunner()->id($runner1->getId())->lifetime(1800);
        $this->assertEquals('SUCCESS', $runner2->execFg());

        $this->assertEquals(1800, $this->redis->ttl($runner2->getSessionKey()));
    }

    public function testSession_ttl_resetOnRead() {
        $this->testRequiresMode('cli');

        $data = uniqid(__FUNCTION__);

        $runner = $this->sessionRunner()->lifetime(600)->data($data);
        $this->assertEquals('SUCCESS', $runner->execFg());
        $this->redis->expire($runner->getSessionKey(), 9999);

        $this->assertEquals($data, $runner->getData());
        $this->assertEquals(600, $this->redis->ttl($runner->getSessionKey()));
    }
}
?>
