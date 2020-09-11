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

if (empty($redisHost)) {
    $redisHost = 'tcp://localhost:6379';
}

ini_set('session.save_handler', $saveHandler);
ini_set('session.save_path', $redisHost);
ini_set('max_execution_time', $maxExecutionTime);
ini_set('redis.session.lock_retries', $lock_retries);
ini_set('redis.session.lock_expire', $lock_expire);
ini_set('session.gc_maxlifetime', $sessionLifetime);

if (isset($argv[10])) {
    ini_set('redis.session.locking_enabled', $argv[10]);
}

if (isset($argv[11])) {
    ini_set('redis.session.lock_wait_time', $argv[11]);
}

session_id($sessionId);
$sessionStartSuccessful = session_start();
sleep($sleepTime);
if (!empty($sessionData)) {
    $_SESSION['redis_test'] = $sessionData;
}
session_write_close();

echo $sessionStartSuccessful ? 'SUCCESS' : 'FAILURE';
