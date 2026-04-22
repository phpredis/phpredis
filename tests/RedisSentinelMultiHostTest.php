<?php defined('PHPREDIS_TESTRUN') or die("Use TestRedis.php to run tests!\n");

require_once __DIR__ . "/TestSuite.php";

class Redis_Sentinel_Multi_Host_Test extends TestSuite
{
    const NAME = 'mymaster';

    protected static function sentinelPorts(): array
    {
        return [26379, 26380, 26381];
    }

    protected static function sentinelEnvAvailable(): bool
    {
        foreach (self::sentinelPorts() as $port) {
            $fp = @fsockopen('127.0.0.1', $port, $errno, $errstr, 0.2);
            if (!$fp) return false;
            fclose($fp);
        }
        return true;
    }

    public function setUp()
    {
        if (!self::sentinelEnvAvailable()) {
            $this->markTestSkipped(
                'Sentinel env not available. Start via: '
                . 'docker compose -f tests/sentinel-multihost/docker-compose.yml up -d'
            );
        }
    }

    protected function hostsList(): array
    {
        return array_map(
            fn($p) => ['host' => '127.0.0.1', 'port' => $p],
            self::sentinelPorts()
        );
    }

    protected function deadPort(): array
    {
        /* ECONNREFUSED deterministically — no service binds port 1 */
        return ['host' => '127.0.0.1', 'port' => 1];
    }

    public function testConstructAcceptsHostsArray()
    {
        $s = new RedisSentinel(['hosts' => $this->hostsList()]);
        $this->assertIsObject($s, RedisSentinel::class);
    }

    public function testConnectsToFirstAvailableHost()
    {
        $s = new RedisSentinel(['hosts' => $this->hostsList()]);
        $this->assertTrue($s->ping());
    }

    public function testFallsBackWhenFirstHostDown()
    {
        $s = new RedisSentinel(['hosts' => [
            $this->deadPort(),
            ['host' => '127.0.0.1', 'port' => 26379],
        ]]);
        $addr = $s->getMasterAddrByName(self::NAME);
        $this->assertIsArray($addr, 2);
    }

    public function testFallsBackThroughMultipleFailures()
    {
        $s = new RedisSentinel(['hosts' => [
            $this->deadPort(),
            $this->deadPort(),
            ['host' => '127.0.0.1', 'port' => 26379],
        ]]);
        $this->assertTrue($s->ping());
    }

    public function testThrowsWhenAllHostsDown()
    {
        $s = new RedisSentinel(['hosts' => [
            $this->deadPort(),
            $this->deadPort(),
            $this->deadPort(),
        ]]);
        $threw = false;
        try {
            $s->ping();
        } catch (RedisException $e) {
            $threw = true;
            $this->assertStringContains('Sentinel', $e->getMessage());
            $this->assertStringContains('hosts', $e->getMessage());
        }
        $this->assertTrue($threw);
    }

    public function testRejectsEmptyHostsArray()
    {
        $threw = false;
        try {
            new RedisSentinel(['hosts' => []]);
        } catch (RedisException $e) {
            $threw = true;
            $this->assertStringContains('empty', strtolower($e->getMessage()));
        }
        $this->assertTrue($threw);
    }

    public function testRejectsHostsEntryMissingHostKey()
    {
        $threw = false;
        try {
            new RedisSentinel(['hosts' => [['port' => 26379]]]);
        } catch (RedisException $e) {
            $threw = true;
            $this->assertStringContains("'host'", $e->getMessage());
            $this->assertStringContains('0', $e->getMessage());
        }
        $this->assertTrue($threw);
    }

    public function testRejectsNonStringHost()
    {
        $threw = false;
        try {
            new RedisSentinel(['hosts' => [['host' => 123, 'port' => 26379]]]);
        } catch (RedisException $e) {
            $threw = true;
            $this->assertStringContains("'host'", $e->getMessage());
        }
        $this->assertTrue($threw);
    }

    public function testRejectsNonIntPort()
    {
        $threw = false;
        try {
            new RedisSentinel(['hosts' => [
                ['host' => '127.0.0.1', 'port' => 'abc'],
            ]]);
        } catch (RedisException $e) {
            $threw = true;
            $this->assertStringContains("'port'", $e->getMessage());
        }
        $this->assertTrue($threw);
    }

    public function testRejectsNullHostsOption()
    {
        /* hosts => null should be treated as invalid, not silently ignored —
         * avoids a foot-gun where a misconfigured config yields default
         * single-host behavior. */
        $threw = false;
        try {
            new RedisSentinel(['hosts' => null]);
        } catch (RedisException $e) {
            $threw = true;
        }
        $this->assertTrue($threw);
    }

    public function testRejectsHostsAsString()
    {
        $threw = false;
        try {
            new RedisSentinel(['hosts' => '127.0.0.1:26379']);
        } catch (RedisException $e) {
            $threw = true;
            $this->assertStringContains('array', strtolower($e->getMessage()));
        }
        $this->assertTrue($threw);
    }

    public function testRejectsNonArrayHostsEntry()
    {
        $threw = false;
        try {
            new RedisSentinel(['hosts' => ['127.0.0.1:26379']]);
        } catch (RedisException $e) {
            $threw = true;
            $this->assertStringContains('array', strtolower($e->getMessage()));
        }
        $this->assertTrue($threw);
    }

    public function testRejectsHostsArrayTooLarge()
    {
        /* DoS guard: 1024-entry limit prevents runaway allocation when user
         * accidentally passes an unbounded list. */
        $threw = false;
        try {
            new RedisSentinel([
                'hosts' => array_fill(0, 2048, ['host' => '127.0.0.1']),
            ]);
        } catch (RedisException $e) {
            $threw = true;
            $this->assertStringContains('max', strtolower($e->getMessage()));
        }
        $this->assertTrue($threw);
    }

    public function testDefaultPortWhenOmitted()
    {
        /* hosts[0] omits port → defaults to 26379 */
        $s = new RedisSentinel(['hosts' => [
            ['host' => '127.0.0.1'],
        ]]);
        $this->assertTrue($s->ping());
    }

    public function testSingleHostPathUnchanged()
    {
        /* BC check: no 'hosts' option, classic single-host usage */
        $s = new RedisSentinel(['host' => '127.0.0.1', 'port' => 26379]);
        $this->assertTrue($s->ping());
    }

    public function testPassesAuthToAllHosts()
    {
        /* Skipped unless AUTH-protected Sentinel env provided */
        if (!getenv('SENTINEL_AUTH_PASS')) {
            $this->markTestSkipped('Requires SENTINEL_AUTH_PASS env var');
        }
        $s = new RedisSentinel([
            'hosts' => $this->hostsList(),
            'auth'  => getenv('SENTINEL_AUTH_PASS'),
        ]);
        $this->assertTrue($s->ping());
    }

    public function testStickyAfterFailover()
    {
        /* hosts[0] dead → hosts[1] alive. First call pays the ECONNREFUSED +
         * fallback cost; second call must go directly to hosts[1] with no
         * retry cycle. Timing is the externally observable proxy for stickiness:
         * a non-sticky impl would re-hit hosts[0] on the second call. */
        $s = new RedisSentinel(['hosts' => [
            $this->deadPort(),
            ['host' => '127.0.0.1', 'port' => 26379],
        ]]);

        $t0 = microtime(true);
        $addr1 = $s->getMasterAddrByName(self::NAME);
        $t1 = microtime(true);
        $addr2 = $s->getMasterAddrByName(self::NAME);
        $t2 = microtime(true);

        $this->assertEquals($addr1, $addr2);

        /* First call: connect-refused on hosts[0] + fallback + command on hosts[1].
         * Second call: single command on hosts[1]. The second must not exceed
         * the first by more than noise — a re-visited hosts[0] would double it. */
        $first  = $t1 - $t0;
        $second = $t2 - $t1;
        $this->assertLT($first * 2 + 0.05, $second);
    }

    public function testBoundedRetries()
    {
        /* All hosts unreachable — must throw after trying each once, not loop.
         * ECONNREFUSED on port 1 returns an RST immediately; even 5 attempts
         * should complete in tens of milliseconds. 2s is diagnostic: any
         * regression introducing a per-host sleep or secondary loop is caught. */
        $t0 = microtime(true);
        $threw = false;
        try {
            $s = new RedisSentinel([
                'hosts' => array_fill(0, 5, $this->deadPort()),
                'connectTimeout' => 0.1,
            ]);
            $s->ping();
        } catch (RedisException $e) {
            $threw = true;
            $elapsed = microtime(true) - $t0;
            $this->assertLT(2.0, $elapsed);
        }
        $this->assertTrue($threw);
    }
}
