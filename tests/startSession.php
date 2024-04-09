<?php
error_reporting(E_ERROR | E_WARNING);

$redisHost = $argv[1];
$saveHandler = $argv[2];
$sessionId = $argv[3];
$sleepTime = $argv[4];
$maxExecutionTime = $argv[5];
$lock_retries = $argv[6];
$lock_expire = $argv[7];
$sessionData = $argv[8];
$sessionLifetime = $argv[9];
$lockingEnabled = $argv[10];
$lockWaitTime = $argv[11];
$sessionCompression = $argv[12];

if (empty($redisHost)) {
    $redisHost = 'tcp://localhost:6379';
}

ini_set('session.save_handler', $saveHandler);
ini_set('session.save_path', $redisHost);
ini_set('max_execution_time', $maxExecutionTime);
ini_set("{$saveHandler}.session.lock_retries", $lock_retries);
ini_set("{$saveHandler}.session.lock_expire", $lock_expire);
ini_set('session.gc_maxlifetime', $sessionLifetime);
ini_set("{$saveHandler}.session.locking_enabled", $lockingEnabled);
ini_set("{$saveHandler}.session.lock_wait_time", $lockWaitTime);
ini_set('redis.session.compression', $sessionCompression);

session_id($sessionId);
$sessionStartSuccessful = session_start();
sleep($sleepTime);
if (!empty($sessionData)) {
    $_SESSION['redis_test'] = $sessionData;
}

session_write_close();

echo $sessionStartSuccessful ? 'SUCCESS' : 'FAILURE';
