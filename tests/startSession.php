<?php
error_reporting(E_ERROR | E_WARNING);

$opt = getopt('', [
    'handler:', 'save-path:', 'id:', 'sleep:', 'max-execution-time:' ,
    'locking-enabled:', 'lock-wait-time:', 'lock-retries:', 'lock-expires:',
    'data:', 'lifetime:', 'compression:'
]);

$handler = $opt['handler'] ?? NULL;
$save_path = $opt['save-path'] ?? NULL;
$id = $opt['id'] ?? NULL;
$sleep = $opt['sleep'] ?? 0;
$max_execution_time = $opt['max-execution-time'] ?? 0;
$lock_retries = $opt['lock-retries'] ?? 0;
$lock_expire = $opt['lock-expires'] ?? 0;
$data = $opt['data'] ?? NULL;
$lifetime = $opt['lifetime'] ?? 0;
$locking_enabled = $opt['locking-enabled'] ?? NULL;
$lock_wait_time = $opt['lock-wait-time'] ?? 0;
$compression = $opt['compression'] ?? NULL;

if ( ! $handler) {
    fprintf(STDERR, "--handler is required\n");
    exit(1);
} else if ( ! $save_path) {
    fprintf(STDERR, "--save-path is required\n");
    exit(1);
}

ini_set('session.save_handler', $handler);
ini_set('session.save_path', $save_path);
ini_set('max_execution_time', $max_execution_time);
ini_set("{$handler}.session.lock_retries", $lock_retries);
ini_set("{$handler}.session.lock_expire", $lock_expire);
ini_set('session.gc_maxlifetime', $lifetime);
ini_set("{$handler}.session.locking_enabled", $locking_enabled);
ini_set("{$handler}.session.lock_wait_time", $lock_wait_time);
ini_set('redis.session.compression', $compression);

session_id($id);
$status = session_start();

sleep($sleep);

if ($data) {
    $_SESSION['redis_test'] = $data;
}

session_write_close();

echo $status ? 'SUCCESS' : 'FAILURE';
