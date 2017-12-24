<?php
error_reporting(E_ERROR | E_WARNING);

$redisHost = $argv[1];
$sessionId = $argv[2];
$locking = !!$argv[3];
$destroyPrevious = !!$argv[4];

if (empty($redisHost)) {
    $redisHost = 'localhost';
}

ini_set('session.save_handler', 'redis');
ini_set('session.save_path', 'tcp://' . $redisHost . ':6379');

if ($locking) {
    ini_set('redis.session.locking_enabled', true);
}

session_id($sessionId);
if (!session_start()) {
    $result = "FAILED: session_start()";
}
elseif (!session_regenerate_id($destroyPrevious)) {
    $result = "FAILED: session_regenerate_id()";
}
else {
    $result = session_id();
}
echo $result;

