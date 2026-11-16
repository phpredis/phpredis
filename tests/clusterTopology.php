<?php
function check($condition, $message) {
    if (!$condition) throw new RuntimeException($message);
}
function until(callable $condition, $message) {
    $deadline = microtime(true) + 15;
    do { if ($condition()) return; usleep(20000); } while (microtime(true) < $deadline);
    throw new RuntimeException($message);
}
/* Run only against a disposable local six-node cluster: this triggers failover.
 * Usage: php tests/clusterTopology.php <first-port> [cache=0] [persistent=0]
 * REDIS_TOPOLOGY_AUTH and REDIS_TOPOLOGY_CONTEXT accept JSON constructor options.
 */
$start = (int)($argv[1] ?? 0);
if ($start < 1024 || $start > 55530) {
    fwrite(STDERR, "Supply the first port of a disposable local six-node cluster.\n");
    exit(1);
}
$auth = json_decode(getenv('REDIS_TOPOLOGY_AUTH') ?: 'null', true, 512, JSON_THROW_ON_ERROR);
$ssl = json_decode(getenv('REDIS_TOPOLOGY_CONTEXT') ?: 'null', true, 512, JSON_THROW_ON_ERROR);
$tls = $ssl !== null;
$context = $tls ? ['stream' => $ssl] : null;
$ports = range($start, $start + 5);
$peers = [];
foreach ($ports as $port) {
    $r = new Redis; $r->connect($tls ? 'tls://127.0.0.1' : '127.0.0.1', $port, 2, null, 0, 2, $context);
    if ($auth !== null) $r->auth($auth);
    $peers[$port] = $r;
}
function owner($peers, $slot) {
    foreach (reset($peers)->rawCommand('CLUSTER','SLOTS') as $range)
        if ($range[0] <= $slot && $range[1] >= $slot) return $range[2][1];
    throw new RuntimeException('slot has no owner');
}
function connections($peers) {
    $count = 0;
    foreach ($peers as $r) {
        foreach (explode("\n", trim($r->rawCommand('CLIENT','LIST'))) as $client) {
            if (preg_match('/flags=([^ ]+)/', $client, $flags) && !strpbrk($flags[1], 'MS')) $count++;
        }
    }
    return $count;
}
function redirectCount($r) {
    preg_match('/errorstat_MOVED:count=(\d+)/', $r->rawCommand('INFO','ERRORSTATS'), $match);
    return (int)($match[1] ?? 0);
}
$key = '{foo}:topology-integration'; $slot = 12182;
$cache = ($argv[2] ?? '0') === '1'; $persistent = ($argv[3] ?? '0') === '1';
ini_set('redis.clusters.cache_slots', $cache ? '1' : '0');
$c = new RedisCluster(null, array_map(fn($p) => '127.0.0.1:'.$p, range($start,$start+2)), 2, 2, $persistent, $auth, $ssl);
$c->set($key, 0); $expected = 0;
foreach (range(1, 3) as $round) {
    usleep(1200000); /* Let Redis finish the preceding election before reversing it. */
    $old = owner($peers,$slot);
    $id = $peers[$old]->rawCommand('CLUSTER','MYID');
    until(fn() => count($peers[$old]->rawCommand('CLUSTER','REPLICAS',$id))===1, 'expected one replica');
    $replicas = $peers[$old]->rawCommand('CLUSTER','REPLICAS',$id);
    preg_match('/127[.]0[.]0[.]1:(\d+)/', $replicas[0], $match); $new = (int)$match[1];
    until(fn() => (int)($peers[$new]->info('replication')['master_link_status']==='up'), 'replication link did not recover');
    check($c->incr($key)===++$expected, 'pre-failover INCR');
    check($peers[$new]->rawCommand('CLUSTER','FAILOVER')!==false, 'failover rejected');
    until(function () use ($peers, $new, $old, $slot) {
        foreach ($peers as $r) {
            $found = false;
            foreach ($r->rawCommand('CLUSTER','SLOTS') as $range) {
                if ($range[0] <= $slot && $slot <= $range[1]) $found = $range[2][1]===$new;
            }
            if (!$found) return false;
        }
        return $peers[$old]->info('replication')['role']==='slave';
    }, 'cluster views did not converge');
    check($c->incr($key)===++$expected, 'recovered INCR was lost or repeated');
    check($c->get($key)===(string)$expected, 'read after failover');
    $masters = array_column($c->_masters(),1);sort($masters);
    $actual = array_unique(array_map(fn($range) => $range[2][1], $peers[$new]->rawCommand('CLUSTER','SLOTS')));sort($actual);
    check($masters===array_values($actual), 'masters registry stale');
    $count = connections($peers);
    check($count<=12, 'orphan data connections: '.$count);
    echo "failover $round $old->$new; clients=$count; counter=$expected\n";
}
/* Move two empty slots between existing primaries. The first MOVED must repair
 * the second route too, so the second GET must not cause another MOVED. */
$slots = [1000, 1001];
$source = owner($peers,$slots[0]);
$primaryPorts = array_column($c->_masters(),1);
$target = current(array_diff($primaryPorts,[$source]));
$sourceId = $peers[$source]->rawCommand('CLUSTER','MYID');
$targetId = $peers[$target]->rawCommand('CLUSTER','MYID');
$keys = [];
foreach ($slots as $movingSlot) {
    check(owner($peers,$movingSlot)===$source, 'test slots must start on the same primary');
    check($peers[$source]->rawCommand('CLUSTER','COUNTKEYSINSLOT',$movingSlot)===0, 'test slots must be empty');
    for ($n = 0; $n < 1000000; $n++) {
        $candidate = '{topology-slot-'.$n.'}';
        if ($peers[$source]->rawCommand('CLUSTER','KEYSLOT',$candidate)===$movingSlot) break;
    }
    check($n < 1000000, 'could not find a key for the test slot');
    $keys[] = $candidate;
    check($c->get($candidate)===false, 'test key must not exist');
}
$changed = [];
try {
    foreach ($slots as $movingSlot) {
        $changed[] = $movingSlot;
        foreach (array_unique(array_merge([$target,$source],$primaryPorts)) as $port)
            check($peers[$port]->rawCommand('CLUSTER','SETSLOT',$movingSlot,'NODE',$targetId)!==false, 'slot move rejected');
    }
    $before = redirectCount($peers[$source]);
    check($c->get($keys[0])===false, 'first moved GET');
    $first = redirectCount($peers[$source]);
    check($first===$before+1, 'first GET must encounter one MOVED');
    check($c->get($keys[1])===false, 'second moved GET');
    check(redirectCount($peers[$source])===$first, 'second slot still used the stale route');
    echo "two-slot reshard: one MOVED repaired both routes\n";
} finally {
    foreach ($changed as $movingSlot) {
        foreach (array_unique(array_merge([$source,$target],$primaryPorts)) as $port)
            $peers[$port]->rawCommand('CLUSTER','SETSLOT',$movingSlot,'NODE',$sourceId);
    }
}
$c->close();
if (!$persistent) check(connections($peers)===6, 'nonpersistent close left orphan connections');
echo 'PASS cache='.(int)$cache.' persistent='.(int)$persistent."\n";
