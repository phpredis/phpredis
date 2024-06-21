<?php

require_once __DIR__ . '/SessionHelpers.php';

error_reporting(E_ERROR | E_WARNING);

$opt = getopt('', ['handler:', 'save-path:', 'id:', 'lifetime:']);

$handler = $opt['handler'] ?? NULL;
$save_path = $opt['save-path'] ?? NULL;
$id = $opt['id'] ?? NULL;
$lifetime = $opt['lifetime'] ?? NULL;

if ( ! $handler) {
    fprintf(STDERR, "--handler is required\n");
    exit(1);
} else if ( ! $save_path) {
    fprintf(STDERR, "--save-path is required\n");
    exit(1);
}

ini_set('session.save_handler', $handler);
ini_set('session.save_path', $save_path);
ini_set('session.gc_maxlifetime', $lifetime);

session_id($id);
if ( ! session_start()) {
    fprintf(STDERR, "session_start() was nut successful");
    exit(1);
} else {
    echo isset($_SESSION['redis_test']) ? $_SESSION['redis_test'] : 'Key redis_test not found';
    exit(0);
}
