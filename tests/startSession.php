<?php
error_reporting(E_ERROR | E_WARNING);

$opt = getopt('', [
    'handler:', 'save-path:', 'id:', 'sleep:', 'max-execution-time:' ,
    'locking-enabled:', 'lock-wait-time:', 'lock-retries:', 'lock-expires:',
    'lock-release-cmd:',
    'data:', 'data-key:', 'lifetime:', 'compression:', 'strict-mode:', 'early-refresh:'
]);

$handler = $opt['handler'] ?? NULL;
$save_path = $opt['save-path'] ?? NULL;
$id = $opt['id'] ?? NULL;
$sleep = $opt['sleep'] ?? 0;
$max_execution_time = $opt['max-execution-time'] ?? 0;
$lock_retries = $opt['lock-retries'] ?? 0;
$lock_expire = $opt['lock-expires'] ?? 0;
$lock_release_cmd = $opt['lock-release-cmd'] ?? NULL;
$data = $opt['data'] ?? NULL;
$data_key = $opt['data-key'] ?? 'redis_test';
$lifetime = $opt['lifetime'] ?? 0;
$locking_enabled = $opt['locking-enabled'] ?? NULL;
$lock_wait_time = $opt['lock-wait-time'] ?? 0;
$compression = $opt['compression'] ?? NULL;
$strict_mode = !!($opt['strict-mode'] ?? false);
$early_refresh = !!($opt['early-refresh'] ?? false);

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
/* Both 'redis' (standalone) and 'rediscluster' handlers share the
 * 'redis.session.*' INI namespace registered in redis.c. */
ini_set("redis.session.lock_retries", $lock_retries);
ini_set("redis.session.lock_expire", $lock_expire);
ini_set('session.gc_maxlifetime', $lifetime);
ini_set("redis.session.locking_enabled", $locking_enabled);
ini_set("redis.session.lock_wait_time", $lock_wait_time);
if ($lock_release_cmd !== NULL && $lock_release_cmd !== '') {
    ini_set('redis.session.lock_release_cmd', $lock_release_cmd);
}
ini_set('redis.session.compression', $compression);
ini_set('session.use_strict_mode', $strict_mode);
ini_set('redis.session.early_refresh', $early_refresh);

session_id($id);
$status = session_start();

sleep($sleep);

if ($data) {
    $_SESSION[$data_key] = $data;
}

session_write_close();

echo $status ? 'SUCCESS' : 'FAILURE';
