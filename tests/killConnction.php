<?php
/*
 * Kills the latest connected client
 */

$fullHostAddress = $argv[1];
$sleepTime = $argv[2];

if (!empty($fullHostAddress)) {
    $fullHostAddress = str_replace('tcp://', '', $fullHostAddress);
    $fullHostAddress = explode(':', $fullHostAddress);
    $host = $fullHostAddress[0];
    $port = $fullHostAddress[1];
} else {
    $host = 'localhost';
    $port = 6379;
}

$redis = new Redis();
$redis->connect($host, $port);
sleep($sleepTime);

$clients = $redis->client('list');
usort($clients, function (array $a, array $b) {
    return ($a['id'] < $b['id']) ? -1 : 1;
});
$clientPort = array_pop($clients)['addr'];
$clientPort = explode(':', $clientPort)[1];

exec("/usr/sbin/tcpkill -i lo port $clientPort");