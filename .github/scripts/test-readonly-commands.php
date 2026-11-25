<?php
/* Standalone generator checks; no Redis extension or server required. */
declare(strict_types=1);
require __DIR__ . '/generate-readonly-commands.php';

function check(bool $condition, string $message): void {
    if (!$condition) throw new RuntimeException($message);
}

$source = <<<'STUB'
<?php
class RedisCluster {
    /** @redis-readonly */
    public function get(string $key): mixed;
    /**
     * @see \Redis::get()
     * @redis-readonly GET
     */
    public function getWithMeta(string $key): array;
    /** @see \Redis::set() */
    public function set(string $key, mixed $value): bool;
    /** @redis-readonly MODULE.READ */
    public function moduleRead(): mixed;
    public function local(#[SensitiveParameter] mixed $argument = null): void;
}
STUB;
check(redisReadonlyCommands($source) === ['GET', 'MODULE.READ'],
      'Inference, override, deduplication, or untagged methods failed');

foreach ([
    str_replace('@redis-readonly MODULE.READ', '@redis-readonly MODULE READ', $source),
    str_replace('@redis-readonly MODULE.READ', '@redis-readonyl', $source),
    str_replace('/** @redis-readonly */', '/** @redis-readonly @redis-readonly */', $source),
    '<?php class RedisCluster {}',
    '<?php class RedisCluster',
    '<?php class RedisCluster { /** @redis-readonly */ public const X = 1; }',
    '<?php class RedisCluster { /** @redis-readonly */ public function',
] as $invalid) {
    $thrown = false;
    try { redisReadonlyCommands($invalid); }
    catch (RuntimeException $error) { $thrown = true; }
    check($thrown, "Invalid metadata was accepted: $invalid");
}

$readonly = redisReadonlyCommands(file_get_contents(__DIR__ . '/../../redis_cluster.stub.php'));
foreach (['GET', 'HGET', 'MGET', 'SORT_RO', 'EVAL_RO', 'FCALL_RO', 'GEORADIUS_RO'] as $command)
    check(in_array($command, $readonly, true), "$command should be readonly");
foreach (['SET', 'GETDEL', 'GETEX', 'SORT', 'EVAL', 'FCALL', 'GEORADIUS', 'XREADGROUP', 'ACL'] as $command)
    check(!in_array($command, $readonly, true), "$command must not be readonly");
/* Writes the C test uses as near-miss negatives for the exact lookup. */
$commands = array_fill_keys($readonly, true) +
            array_fill_keys(['SET', 'GETDEL', 'GETEX', 'SORT', 'EVAL', 'FCALL', 'GEORADIUS', 'XREADGROUP', 'ACL'], false);

/* Test the generated C against exact membership, including arbitrary binary input. */
$c = '#include "' . realpath(__DIR__ . '/../../redis_readonly_commands.h') . '"' . "\n";
$c .= <<<'C'
#include <assert.h>
#include <stdint.h>
#include <string.h>
static const struct { const char *name; int readonly; } commands[] = {
C;
foreach ($commands as $command => $readonly)
    $c .= "\n    {\"$command\", " . ($readonly ? '1' : '0') . '},';
$c .= <<<'C'

};
static int exact(const char *input, size_t len) {
    for (size_t i = 0; i < sizeof(commands) / sizeof(*commands); i++) {
        if (commands[i].readonly && strlen(commands[i].name) == len &&
            memcmp(commands[i].name, input, len) == 0) return 1;
    }
    return 0;
}
int main(void) {
    char input[64];
    for (size_t i = 0; i < sizeof(commands) / sizeof(*commands); i++) {
        size_t len = strlen(commands[i].name);
        memcpy(input, commands[i].name, len);
        assert((redis_readonly_command(input, len) != NULL) == commands[i].readonly);
        for (size_t j = 0; j < len; j++) {
            if (input[j] >= 'A' && input[j] <= 'Z') input[j] += 'a' - 'A';
        }
        assert((redis_readonly_command(input, len) != NULL) == commands[i].readonly);
        memcpy(input, commands[i].name, len);
        for (size_t j = 0; j < len; j++) {
            char saved = input[j];
            for (unsigned int byte = 0; byte < 256; byte++) {
                input[j] = byte;
                char normalized[64];
                memcpy(normalized, input, len);
                if (byte >= 'a' && byte <= 'z') normalized[j] -= 'a' - 'A';
                assert((redis_readonly_command(input, len) != NULL) == exact(normalized, len));
            }
            input[j] = saved;
        }
        input[len] = '\0';
        assert(redis_readonly_command(input, len + 1) == NULL);
    }
    uint32_t random = 1;
    for (size_t len = 0; len <= sizeof(input); len++) {
        for (size_t j = 0; j < len; j++) {
            random = random * 1664525 + 1013904223;
            input[j] = random >> 24;
        }
        assert(redis_readonly_command(input, len) == NULL);
    }
    /* No trailing terminator, including at the end of an allocation. */
    const char get[] = {'G', 'e', 'T'};
    assert(redis_readonly_command(get, sizeof(get)) != NULL);
    return 0;
}
C;
$path = tempnam(sys_get_temp_dir(), 'phpredis-readonly-test-');
try {
    file_put_contents($path . '.c', $c);
    $args = array_merge([getenv('CC') ?: 'cc', '-std=c99', '-Wall', '-Wextra', '-Werror'],
                       preg_split('/\s+/', trim(getenv('CFLAGS') ?: ''), -1, PREG_SPLIT_NO_EMPTY),
                       [$path . '.c', '-o', $path]);
    $process = proc_open($args, [STDIN, STDOUT, STDERR], $pipes);
    check(is_resource($process) && proc_close($process) === 0, 'C lookup test compilation failed');
    $process = proc_open([$path], [STDIN, STDOUT, STDERR], $pipes);
    check(is_resource($process) && proc_close($process) === 0, 'C lookup test failed');
} finally {
    unlink($path . '.c');
    unlink($path);
}
echo "Readonly metadata and C lookup tests passed\n";
