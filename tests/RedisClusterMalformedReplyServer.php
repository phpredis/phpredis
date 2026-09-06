<?php

/* Emit intentionally malformed RESP to test pipeline socket invalidation. */

function readLineFromClient($client) {
    $line = @fgets($client);
    return $line === false ? false : rtrim($line, "\r\n");
}

function readBytesFromClient($client, $length) {
    $result = '';

    while (strlen($result) < $length) {
        $chunk = @fread($client, $length - strlen($result));
        if ($chunk === false || $chunk === '') {
            return false;
        }
        $result .= $chunk;
    }

    return $result;
}

function readCommandFromClient($client) {
    $header = readLineFromClient($client);
    if ($header === false) {
        return false;
    }
    if ($header === '' || $header[0] !== '*') {
        return false;
    }

    $command = [];
    for ($i = 0, $argc = (int)substr($header, 1); $i < $argc; $i++) {
        $bulk = readLineFromClient($client);
        if ($bulk === false || $bulk === '' || $bulk[0] !== '$') {
            return false;
        }

        $length = (int)substr($bulk, 1);
        $argument = readBytesFromClient($client, $length);
        if ($argument === false || readBytesFromClient($client, 2) !== "\r\n") {
            return false;
        }
        $command[] = $argument;
    }

    return $command;
}

function writeToClient($client, $response) {
    for ($offset = 0, $length = strlen($response); $offset < $length;) {
        $written = @fwrite($client, substr($response, $offset));
        if ($written === false || $written === 0) {
            return false;
        }
        $offset += $written;
    }

    return true;
}

$scenario = $argv[1] ?? '';
if (!in_array($scenario, ['pipeline', 'multi', 'multi-framing'], true)) {
    exit(1);
}

$errno = $errstr = null;
$server = @stream_socket_server('tcp://127.0.0.1:0', $errno, $errstr);
if ($server === false) {
    fwrite(STDERR, "$errstr ($errno)\n");
    exit(1);
}

$address = stream_socket_get_name($server, false);
$port = (int)substr(strrchr($address, ':'), 1);
echo $port, "\n";
flush();

$slots = "*1\r\n*3\r\n:0\r\n:16383\r\n*2\r\n" .
         "$9\r\n127.0.0.1\r\n:$port\r\n";
$malformed = "*3\r\n$3\r\none\r\n$3\r\ntwo\r\n$5\r\nthree\r\n";
$pipelineConnection = null;
$pipelineQueuedGetPending = false;
$connectionId = 0;
$deadline = microtime(true) + 15;

while (microtime(true) < $deadline) {
    $client = @stream_socket_accept($server, 1);
    if ($client === false) {
        continue;
    }

    $connectionId++;
    $inMulti = false;
    stream_set_timeout($client, 5);

    while (($command = readCommandFromClient($client)) !== false) {
        $verb = strtoupper($command[0] ?? '');

        if ($verb === 'CLUSTER') {
            writeToClient($client, $slots);
        } else if ($scenario === 'pipeline' && $verb === 'LMPOP') {
            $pipelineConnection = $connectionId;
            $pipelineQueuedGetPending = true;
            writeToClient($client, $malformed);
        } else if ($scenario !== 'pipeline' && $verb === 'MULTI') {
            $pipelineConnection = $connectionId;
            $inMulti = true;
            writeToClient($client, "+OK\r\n");
        } else if ($inMulti && ($verb === 'LMPOP' || $verb === 'GET')) {
            writeToClient($client, "+QUEUED\r\n");
        } else if ($inMulti && $verb === 'EXEC') {
            $inMulti = false;
            if ($scenario === 'multi-framing') {
                writeToClient($client, "*1\r\n$6\r\nactual\r\n");
            } else {
                writeToClient($client, "*2\r\n" . $malformed . "$6\r\nactual\r\n");
            }
        } else if ($scenario === 'pipeline' && $verb === 'GET' &&
                   $connectionId === $pipelineConnection &&
                   $pipelineQueuedGetPending)
        {
            $pipelineQueuedGetPending = false;
            writeToClient($client, "$6\r\nactual\r\n");
        } else if ($verb === 'GET') {
            $reused = $connectionId === $pipelineConnection;
            writeToClient($client, $reused
                ? "$6\r\nreused\r\n"
                : "$6\r\nactual\r\n");

            if ($pipelineConnection !== null && !$reused) {
                fclose($client);
                fclose($server);
                exit(0);
            }
        } else {
            writeToClient($client, "+OK\r\n");
        }
    }

    fclose($client);
}

fclose($server);
exit(1);
