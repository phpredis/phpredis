<?php
error_reporting(E_ERROR | E_WARNING);

$redisHost = $argv[1];
$saveHandler = $argv[2];
$sessionId = $argv[3];
$sessionLifetime = $argv[4];

if (empty($redisHost)) {
    $redisHost = 'tcp://localhost:6379';
}

ini_set('session.save_handler', $saveHandler);
ini_set('session.save_path', $redisHost);
ini_set('session.gc_maxlifetime', $sessionLifetime);

session_id($sessionId);
if (!session_start()) {
    echo "session_start() was nut successful";
} else {
    echo isset($_SESSION['redis_test']) ? $_SESSION['redis_test'] : 'Key redis_test not found';
}
