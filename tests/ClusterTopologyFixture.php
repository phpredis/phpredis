<?php

/* A small RESP2 peer for deterministic topology and transport regressions. */
class ClusterTopologyFixture {
    private $process;
    private array $pipes = [];
    public array $nodes;

    public function __construct(callable $configure) {
        $this->process = proc_open([PHP_BINARY, '-n', __FILE__, '--server'],
            [['pipe', 'r'], ['pipe', 'w'], ['pipe', 'w']], $this->pipes);
        if (!is_resource($this->process))
            throw new RuntimeException('Could not start cluster fixture');
        stream_set_timeout($this->pipes[1], 5);
        $line = fgets($this->pipes[1]);
        if ($line === false)
            throw new RuntimeException('Cluster fixture did not start');
        $this->nodes = json_decode($line, true, 512, JSON_THROW_ON_ERROR);
        fwrite($this->pipes[0], json_encode($configure($this->nodes), JSON_THROW_ON_ERROR) . "\n");
        fflush($this->pipes[0]);
    }

    public function close(): void {
        if (is_resource($this->process)) {
            proc_terminate($this->process);
            foreach ($this->pipes as $pipe) fclose($pipe);
            proc_close($this->process);
            $this->process = null;
        }
    }

    public function __destruct() { $this->close(); }
}

if (($argv[1] ?? '') !== '--server') return;

function topologyReadCommand($stream): ?array {
    $line = fgets($stream);
    if ($line === false || $line[0] !== '*') return null;
    $args = [];
    for ($i = 0, $count = (int)substr($line, 1); $i < $count; $i++) {
        $line = fgets($stream);
        if ($line === false || $line[0] !== '$') return null;
        $left = (int)substr($line, 1) + 2;
        $arg = '';
        while ($left > 0) {
            $chunk = fread($stream, $left);
            if ($chunk === false || $chunk === '') return null;
            $arg .= $chunk;
            $left -= strlen($chunk);
        }
        $args[] = substr($arg, 0, -2);
    }
    return $args;
}

function topologyReply($value): string {
    if ($value === null) return "$-1\r\n";
    if (is_int($value)) return ":$value\r\n";
    if (is_array($value)) {
        if (array_key_exists('reply', $value)) return topologyReply($value['reply']);
        if (isset($value['error'])) return '-' . $value['error'] . "\r\n";
        if (isset($value['raw'])) return $value['raw'];
        return '*' . count($value) . "\r\n" . implode('', array_map('topologyReply', $value));
    }
    return '$' . strlen($value) . "\r\n$value\r\n";
}

$servers = $nodes = $peers = $owners = $positions = $calls = $authorized = [];
for ($i = 0; $i < 3; $i++) {
    $server = stream_socket_server('tcp://127.0.0.1:0', $errno, $error);
    if ($server === false) throw new RuntimeException($error);
    $servers[(int)$server] = $server;
    $owners[(int)$server] = $i;
    $nodes[] = ['127.0.0.1', (int)substr(strrchr(stream_socket_get_name($server, false), ':'), 1)];
}
echo json_encode($nodes) . "\n";
flush();
$config = json_decode(fgets(STDIN), true, 512, JSON_THROW_ON_ERROR);

while (true) {
    $read = array_merge(array_values($servers), array_values($peers));
    $write = $except = null;
    if (stream_select($read, $write, $except, 5) === false) break;
    foreach ($read as $stream) {
        if (!is_resource($stream)) continue;
        $id = (int)$stream;
        if (isset($servers[$id])) {
            $peer = stream_socket_accept($stream, 1);
            if ($peer) {
                stream_set_timeout($peer, 2);
                $peers[(int)$peer] = $peer;
                $owners[(int)$peer] = $owners[$id];
            }
            continue;
        }
        $args = topologyReadCommand($stream);
        if ($args === null) {
            fclose($stream);
            unset($peers[$id], $owners[$id], $authorized[$id]);
            continue;
        }
        $node = $owners[$id];
        $command = strtoupper($args[0]);
        $name = $command === 'CLUSTER' ? 'CLUSTER ' . strtoupper($args[1] ?? '') : $command;
        $calls[$node][$name] = ($calls[$node][$name] ?? 0) + 1;
        $rules = $config[$node] ?? [];
        if (isset($rules['__auth']) && $command === 'AUTH') {
            $authorized[$id] = array_slice($args, 1) === $rules['__auth'];
            $reply = ['raw' => $authorized[$id] ? "+OK\r\n" : "-WRONGPASS invalid credentials\r\n"];
        } elseif (isset($rules['__auth']) && empty($authorized[$id])) {
            $reply = ['error' => 'NOAUTH Authentication required'];
        } elseif ($command === 'FIXTURE') {
            $reply = json_encode($calls);
        } elseif ($command === 'CLIENT' && strtoupper($args[1] ?? '') === 'ID') {
            $reply = $id;
        } elseif (isset($rules[$name])) {
            $position = $positions[$node][$name] ?? 0;
            $reply = $rules[$name][min($position, count($rules[$name]) - 1)];
            $positions[$node][$name] = $position + 1;
        } else {
            $reply = ['raw' => $command === 'PING' ? "+PONG\r\n" : "+OK\r\n"];
        }
        /* Close listeners before acknowledging a stop, so reconnect failure is
         * deterministic rather than a race with a successful payload write. */
        if (is_array($reply) && !empty($reply['stop'])) {
            foreach ($servers as $serverId => $server) {
                if ($owners[$serverId] !== $node) continue;
                fclose($server);
                unset($servers[$serverId], $owners[$serverId]);
            }
        }
        $data = topologyReply($reply);
        while ($data !== '') {
            $written = @fwrite($stream, $data);
            if (!$written) break;
            $data = substr($data, $written);
        }
        if (is_array($reply) && !empty($reply['close'])) {
            fclose($stream);
            unset($peers[$id], $owners[$id], $authorized[$id]);
        }
        if (is_array($reply) && !empty($reply['stop'])) {
            foreach ($peers as $peerId => $peer) {
                if ($owners[$peerId] !== $node) continue;
                fclose($peer);
                unset($peers[$peerId], $owners[$peerId], $authorized[$peerId]);
            }
        }
    }
}
