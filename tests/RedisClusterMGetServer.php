<?php

function readCommand($stream): ?array {
    $header = fgets($stream);
    if ($header === false && feof($stream)) {
        return NULL;
    }
    if ($header === false || $header[0] !== '*') {
        throw new RuntimeException('Expected all MGET batches before sending the first response');
    }

    $command = [];
    for ($i = 0, $count = (int)substr($header, 1); $i < $count; $i++) {
        $header = fgets($stream);
        if ($header === false || $header[0] !== '$') {
            throw new RuntimeException('Invalid command argument');
        }

        $length = (int)substr($header, 1) + 2;
        $argument = '';
        while (strlen($argument) < $length) {
            $chunk = fread($stream, $length - strlen($argument));
            if ($chunk === false || $chunk === '') {
                throw new RuntimeException('Incomplete command argument');
            }
            $argument .= $chunk;
        }
        $command[] = substr($argument, 0, -2);
    }

    return $command;
}

function bulkResponse($value): string {
    return $value === false ? "$-1\r\n" : '$' . strlen($value) . "\r\n" . $value . "\r\n";
}

function nextCommand(&$stream, $server): array {
    while (($command = readCommand($stream)) === NULL) {
        fclose($stream);
        $stream = stream_socket_accept($server, 5);
        if ($stream === false) {
            throw new RuntimeException('No connection received after MGET');
        }
        stream_set_timeout($stream, 3);
    }

    return $command;
}

function acceptConnection($server) {
    $stream = stream_socket_accept($server, 5);
    if ($stream === false) {
        throw new RuntimeException('No cluster connection received');
    }
    stream_set_timeout($stream, 3);

    return $stream;
}

$scenario = $argv[1] ?? 'pipeline';
$server = stream_socket_server('tcp://127.0.0.1:0', $errno, $error);
if ($server === false) {
    throw new RuntimeException($error, $errno);
}

$address = stream_socket_get_name($server, false);
$port = (int)substr(strrchr($address, ':'), 1);
echo $address . "\n";
flush();

if ($scenario === 'multiple' || $scenario === 'multiple-truncated') {
    $other = stream_socket_server('tcp://127.0.0.1:0', $errno, $error);
    if ($other === false) {
        throw new RuntimeException($error, $errno);
    }
    $otherAddress = stream_socket_get_name($other, false);
    $otherPort = (int)substr(strrchr($otherAddress, ':'), 1);
    $seed = acceptConnection($server);
    if (readCommand($seed) !== ['CLUSTER', 'SLOTS']) {
        throw new RuntimeException('Expected CLUSTER SLOTS');
    }
    fwrite($seed, "*2\r\n*3\r\n:0\r\n:8191\r\n*2\r\n$9\r\n127.0.0.1\r\n:" . $port . "\r\n" .
        "*3\r\n:8192\r\n:16383\r\n*2\r\n$9\r\n127.0.0.1\r\n:" . $otherPort . "\r\n");
    fclose($seed);

    $first = acceptConnection($other);
    $commands = [readCommand($first), readCommand($first)];
    $second = acceptConnection($server);
    $command = readCommand($second);
    if ($commands !== [
        ['MGET', 'pipeline_{one}a', 'pipeline_{one}b'],
        ['MGET', 'pipeline_{one}a'],
    ] || $command !== ['MGET', 'pipeline_{two}c']) {
        throw new RuntimeException('MGET batches must use their mapped node connections');
    }

    $binary = "a\0\xFF\r\n";
    if ($scenario === 'multiple-truncated') {
        fwrite($second, "*1\r\n" . bulkResponse('c'));
        fwrite($first, "*2\r\n$5\r\nx");
        fclose($first);
        if (nextCommand($second, $server) !== ['GET', 'pipeline_after{two}']) {
            throw new RuntimeException('Expected GET on the other node after the truncated reply');
        }
        fwrite($second, bulkResponse('after-value'));
        fclose($second);
        fclose($other);
        fclose($server);
        exit(0);
    }

    fwrite($first, "*2\r\n" . bulkResponse($binary) . bulkResponse(false));
    fwrite($first, "*1\r\n" . bulkResponse($binary));
    fwrite($second, "*1\r\n" . bulkResponse('c'));
    if (readCommand($first) !== ['GET', 'pipeline_after']) {
        throw new RuntimeException('Expected GET on its mapped node after MGET');
    }
    fwrite($first, bulkResponse('after-value'));
    fclose($first);
    fclose($second);
    fclose($other);
    fclose($server);
    exit(0);
}

while (($stream = stream_socket_accept($server, 5)) !== false) {
    stream_set_timeout($stream, 3);
    $command = readCommand($stream);

    if ($command === ['CLUSTER', 'SLOTS']) {
        fwrite($stream, "*1\r\n*3\r\n:0\r\n:16383\r\n*2\r\n$9\r\n127.0.0.1\r\n:" . $port . "\r\n");
        fclose($stream);
        continue;
    }

    $commands = [$command, readCommand($stream), readCommand($stream)];
    if ($commands !== [
        ['MGET', 'pipeline_{one}a', 'pipeline_{one}b'],
        ['MGET', 'pipeline_{two}c'],
        ['MGET', 'pipeline_{one}a'],
    ]) {
        throw new RuntimeException('MGET must preserve consecutive same-slot batches');
    }

    $binary = "a\0\xFF\r\n";
    $first = "*2\r\n" . bulkResponse($binary) . bulkResponse(false);
    if (in_array($scenario, ['truncated', 'error-truncated', 'clusterdown-truncated'], true)) {
        if ($scenario === 'error-truncated') {
            fwrite($stream, "-ERR planned MGET failure\r\n");
        } elseif ($scenario === 'clusterdown-truncated') {
            fwrite($stream, "-CLUSTERDOWN planned MGET failure\r\n");
        }
        fwrite($stream, ($scenario === 'truncated' ? "*2\r\n" : "*1\r\n") . "$5\r\nx");
        fclose($stream);
        $stream = stream_socket_accept($server, 5);
        if ($stream === false) {
            throw new RuntimeException('No connection received after the truncated reply');
        }
        stream_set_timeout($stream, 3);
    } else {
        switch ($scenario) {
            case 'moved':
            case 'ask':
                fwrite($stream, '-' . strtoupper($scenario) . ' 9084 ' . $address . "\r\n");
                break;
            case 'error':
                fwrite($stream, "-ERR planned MGET failure\r\n");
                break;
            case 'clusterdown':
                fwrite($stream, "-CLUSTERDOWN planned MGET failure\r\n");
                break;
            default:
                fwrite($stream, $first);
        }
        fwrite($stream, "*1\r\n" . bulkResponse('c'));
        fwrite($stream, "*1\r\n" . bulkResponse($binary));

        if ($scenario === 'moved' || $scenario === 'ask') {
            $command = readCommand($stream);
            if ($command === $commands[0]) {
                fwrite($stream, '-' . strtoupper($scenario) . ' 9084 ' . $address . "\r\n");
                $command = readCommand($stream);
            }
            if ($scenario === 'ask') {
                if ($command !== ['ASKING']) {
                    throw new RuntimeException('Expected ASKING before retrying MGET');
                }
                fwrite($stream, "+OK\r\n");
                $command = readCommand($stream);
            }
            if ($command !== $commands[0]) {
                throw new RuntimeException('Expected the original same-slot MGET on retry');
            }
            fwrite($stream, $first);
            $after = readCommand($stream);
            if ($after === $commands[1]) {
                fwrite($stream, "*1\r\n" . bulkResponse('c'));
                if (readCommand($stream) !== $commands[2]) {
                    throw new RuntimeException('Expected the final MGET batch on retry');
                }
                fwrite($stream, "*1\r\n" . bulkResponse($binary));
                $after = readCommand($stream);
            }
        }
    }

    if (($after ?? nextCommand($stream, $server)) !== ['GET', 'pipeline_after']) {
        throw new RuntimeException('Expected GET after MGET');
    }
    fwrite($stream, bulkResponse('after-value'));
    fclose($stream);
    fclose($server);
    exit(0);
}

throw new RuntimeException('No MGET connection received');
