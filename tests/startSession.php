<?php
error_reporting(E_ERROR | E_WARNING);

$redisHost = $argv[1];
$sessionId = $argv[2];
$sleepTime = $argv[3];
$maxExecutionTime = $argv[4];
$lock_retries = $argv[5];
$lock_expire = $argv[6];
$sessionData = $argv[7];

if (empty($redisHost)) {
    $redisHost = 'localhost';
}

ini_set('session.save_handler', 'redis');
ini_set('session.save_path', 'tcp://' . $redisHost . ':6379');
ini_set('max_execution_time', $maxExecutionTime);
ini_set('redis.session.lock_retries', $lock_retries);
ini_set('redis.session.lock_expire', $lock_expire);

if (isset($argv[8])) {
    ini_set('redis.session.locking_enabled', $argv[8]);
}

if (isset($argv[9])) {
    ini_set('redis.session.lock_wait_time', $argv[9]);
}

session_id($sessionId);
$sessionStartSuccessful = session_start();
sleep($sleepTime);
if (!empty($sessionData)) {
    $_SESSION['redis_test'] = $sessionData;
}
session_write_close();

echo $sessionStartSuccessful ? 'SUCCESS' : 'FAILURE';