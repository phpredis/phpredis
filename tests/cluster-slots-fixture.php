<?php
/* One-shot seed advertising a supplied slot map. Data commands use real nodes. */
$slots = json_decode(fgets(STDIN), true);
$server = stream_socket_server('tcp://127.0.0.1:0', $errno, $error);
if (!$server) throw new RuntimeException($error, $errno);
echo stream_socket_get_name($server, false), "\n";
flush();

$socket = stream_socket_accept($server, 5);
if (!$socket) throw new RuntimeException('Seed connection timed out');
stream_set_timeout($socket, 5);

$encode = function ($value) use (&$encode) {
    if (is_array($value)) {
        $reply = '*' . count($value) . "\r\n";
        foreach ($value as $item) $reply .= $encode($item);
        return $reply;
    }
    if (is_int($value)) return ":$value\r\n";
    return '$' . strlen($value) . "\r\n$value\r\n";
};

while (($line = fgets($socket)) !== false) {
    $command = [];
    for ($i = 0, $count = (int)substr($line, 1); $i < $count; $i++) {
        $length = (int)substr(fgets($socket), 1) + 2;
        $argument = '';
        while (strlen($argument) < $length) {
            $chunk = fread($socket, $length - strlen($argument));
            if ($chunk === false || $chunk === '') throw new RuntimeException('Incomplete command');
            $argument .= $chunk;
        }
        $command[] = substr($argument, 0, -2);
    }
    if (array_map('strtoupper', $command) === ['CLUSTER', 'SLOTS']) {
        fwrite($socket, $encode($slots));
        break;
    }
    if (strtoupper($command[0]) !== 'AUTH') throw new RuntimeException('Unexpected seed command');
    fwrite($socket, "+OK\r\n");
}
fclose($socket);
fclose($server);
