<?php
error_reporting(E_ERROR | E_WARNING);

$redisHost = $argv[1];
$sessionId = $argv[2];

if (empty($redisHost)) {
    $redisHost = 'localhost';
}

ini_set('session.save_handler', 'redis');
ini_set('session.save_path', 'tcp://' . $redisHost . ':6379');

session_id($sessionId);
if (!session_start()) {
    echo "session_start() was nut successful";
} else {
    echo isset($_SESSION['redis_test']) ? $_SESSION['redis_test'] : 'Key redis_test not found';
}
