<?php

/**
 * @generate-function-entries
 * @generate-legacy-arginfo
 * @generate-class-entries
 */

class Redis {

    /**
     * Create a new Redis instance.  If passed sufficient information in the
     * options array it is also possible to connect to an instance at the same
     * time.
     *
     * @see Redis::connect()
     * @see https://aws.amazon.com/blogs/architecture/exponential-backoff-and-jitter/
     *
     * Following is an example of an options array with the supported
     * configuration values. Note that all of these values are optional, and you
     * can instead connect to Redis via PhpRedis' connect() method.
     *
     * <code>
     * <?php
     * $options = [
     *     'host'           => 'localhost',
     *     'port'           => 6379,
     *     'readTimeout'    => 2.5,
     *     'connectTimeout' => 2.5,
     *     'persistent'     => true,
     *
     *     // Valid formats: NULL, ['user', 'pass'], 'pass', or ['pass']
     *     'auth' => ['phpredis', 'phpredis'],
     *
     *     // See PHP stream options for valid SSL configuration settings.
     *     'ssl' => ['verify_peer' => false],
     *
     *     // How quickly to retry a connection after we time out or it  closes.
     *     // Note that this setting is overridden by 'backoff' strategies.
     *     'retryInterval'  => 100,
     *
     *      // Which backoff algorithm to use.  'decorrelated jitter' is
     *      // likely the best one for most solutiona, but there are many
     *      // to choose from:
     *      //     REDIS_BACKOFF_ALGORITHM_DEFAULT
     *      //     REDIS_BACKOFF_ALGORITHM_CONSTANT
     *      //     REDIS_BACKOFF_ALGORITHM_UNIFORM
     *      //     REDIS_BACKOFF_ALGORITHM_EXPONENTIAL
     *      //     REDIS_BACKOFF_ALGORITHM_FULL_JITTER
     *      //     REDIS_BACKOFF_ALGORITHM_EQUAL_JITTER
     *      //     REDIS_BACKOFF_ALGORITHM_DECORRELATED_JITTER
     *      //
     *      // 'base', and 'cap' are in milliseconds and represent the first
     *      // delay redis will use when reconnecting, and the maximum delay
     *      // we will reach while retrying.
     *     'backoff' => [
     *         'algorithm' => Redis::BACKOFF_ALGORITHM_DECORRELATED_JITTER,
     *         'base'      => 500,
     *         'cap'       => 750,
     *     ]
     * ];
     * ?>
     * </code>
     *
     * Note: If you do wish to connect via the constructor, only 'host' is
     *       strictly required, which will cause PhpRedis to connect to that
     *       host on Redis' default port (6379).
     */
    public function __construct(array $options = null);

    public function __destruct();

    /**
     * Compress a value with the currently configured compressor as set with
     * Redis::setOption().
     *
     * @see Redis::setOption()
     *
     * @param  string $value The value to be compressed
     * @return string        The compressed result
     *
     */
    public function _compress(string $value): string;

    /**
     * Uncompress the provided argument that has been compressed with the
     * currently configured compressor as set with Redis::setOption().
     *
     * @see Redis::setOption()
     *
     * @param  string $value  The compressed value to uncompress.
     * @return string         The uncompressed result.
     *
     */
    public function _uncompress(string $value): string;

    /**
     * Prefix the passed argument with the currently set key prefix as set
     * with Redis::setOption().
     *
     * @param string  $key The key/string to prefix
     * @return string      The prefixed string
     *
     */
    public function _prefix(string $key): string;

    /**
     * Serialize the provided value with the currently set serializer as set
     * with Redis::setOption().
     *
     * @see Redis::setOption()
     *
     * @param mixed $value The value to serialize
     * @return string      The serialized result
     *
     */
    public function _serialize(mixed $value): string;

    /**
     * Unserialize the passed argument with the currently set serializer as set
     * with Redis::setOption().
     *
     * @see Redis::setOption()
     *
     * @param string $value The value to unserialize
     * @return mixed        The unserialized result
     *
     */
    public function _unserialize(string $value): mixed;

    /**
     * Pack the provided value with the configured serializer and compressor
     * as set with Redis::setOption().
     *
     * @param  mixed $value  The value to pack
     * @return string        The packed result having been serialized and
     *                       compressed.
     */
    public function _pack(mixed $value): string;

    /**
     * Unpack the provided value with the configured compressor and serializer
     * as set with Redis::setOption().
     *
     * @param  string $value  The value which has been serialized and compressed.
     * @return mixed          The uncompressed and deserialized value.
     *
     */
    public function _unpack(string $value): mixed;

    public function acl(string $subcmd, string ...$args): mixed;

    public function append(string $key, mixed $value): Redis|int|false;

    /**
     * Authenticate a Redis connection after its been established.
     *
     * @see https://redis.io/commands/auth
     *
     * @param mixed $credentials A string password, or an array with one or two string elements.
     *
     * @return Redis|bool Whether the AUTH was successful.
     *
     * See below for various examples about how this method may be called.
     *
     * <code>
     * <?php>
     * $redis->auth('password');
     * $redis->auth(['password']);
     * $redis->auth(['username', 'password']);
     * ?>
     * </code>
     *
     */
    public function auth(#[\SensitiveParameter] mixed $credentials): Redis|bool;

    /**
     * Execute a save of the Redis database in the background.
     *
     * @see https://redis.io/commands/bgsave
     *
     * @return Redis|bool Whether the command was successful.
     */
    public function bgSave(): Redis|bool;

    /**
     * Asynchronously rewrite Redis' append-only file
     *
     * @see https://redis.io/commands/bgrewriteaof
     *
     * @return Redis|bool Whether the command was successful.
     */
    public function bgrewriteaof(): Redis|bool;

    /**
     * Count the number of set bits in a Redis string.
     *
     * @see https://redis.io/commands/bitcount/
     *
     * @param string $key     The key in question (must be a string key)
     * @param int    $start   The index where Redis should start counting.  If omitted it
     *                        defaults to zero, which means the start of the string.
     * @param int    $end     The index where Redis should stop counting.  If omitted it
     *                        defaults to -1, meaning the very end of the string.
     *
     * @param bool   $bybit   Whether or not Redis should treat $start and $end as bit
     *                        positions, rather than bytes.
     *
     * @return Redis|int|false The number of bits set in the requested range.
     *
     */
    public function bitcount(string $key, int $start = 0, int $end = -1, bool $bybit = false): Redis|int|false;

    public function bitop(string $operation, string $deskey, string $srckey, string ...$other_keys): Redis|int|false;

    /**
     * Return the position of the first bit set to 0 or 1 in a string.
     *
     * @see https://redis.io/commands/bitpos/
     *
     * @param string $key   The key to check (must be a string)
     * @param bool   $bit   Whether to look for an unset (0) or set (1) bit.
     * @param int    $start Where in the string to start looking.
     * @param int    $end   Where in the string to stop looking.
     * @param bool   $bybit If true, Redis will treat $start and $end as BIT values and not bytes, so if start
     *                      was 0 and end was 2, Redis would only search the first two bits.
     *
     * @return Redis|int|false The position of the first set or unset bit.
     **/
    public function bitpos(string $key, bool $bit, int $start = 0, int $end = -1, bool $bybit = false): Redis|int|false;

    /**
     * Pop an element off the beginning of a Redis list or lists, potentially blocking up to a specified
     * timeout.  This method may be called in two distinct ways, of which examples are provided below.
     *
     * @see https://redis.io/commands/blpop/
     *
     * @param string|array     $key_or_keys    This can either be a string key or an array of one or more
     *                                         keys.
     * @param string|float|int $timeout_or_key If the previous argument was a string key, this can either
     *                                         be an additional key, or the timeout you wish to send to
     *                                         the command.
     *
     * <code>
     * <?php>
     * // One way to call this method is in a variadic way, with the final argument being
     * // the intended timeout.
     * $redis->blPop('list1', 'list2', 'list3', 1.5);
     *
     * // Alternatively, you can send an array of keys
     * $relay->blPop(['list1', 'list2', 'list3'], 1.5);
     * ?>
     * </code>
     */
    public function blPop(string|array $key_or_keys, string|float|int $timeout_or_key, mixed ...$extra_args): Redis|array|null|false;

    /**
     * Pop an element off of the end of a Redis list or lists, potentially blocking up to a specified timeout.
     * The calling convention is identical to Redis::blPop() so see that documentation for more details.
     *
     * @see https://redis.io/commands/brpop/
     * @see Redis::blPop()
     *
     */
    public function brPop(string|array $key_or_keys, string|float|int $timeout_or_key, mixed ...$extra_args): Redis|array|null|false;

    /**
     * Pop an element from the end of a Redis list, pushing it to the beginning of another Redis list,
     * optionally blocking up to a specified timeout.
     *
     * @see https://redis.io/commands/brpoplpush/
     *
     * @param string    $src     The source list
     * @param string    $dst     The destination list
     * @param int|float $timeout The number of seconds to wait.  Note that you must be connected
     *                           to Redis >= 6.0.0 to send a floating point timeout.
     *
     */
    public function brpoplpush(string $src, string $dst, int|float $timeout): Redis|string|false;

    /**
     * POP the maximum scoring element off of one or more sorted sets, blocking up to a specified
     * timeout if no elements are available.
     *
     * @see https://redis.io/commands/bzpopmax
     *
     * @param string|array $key_or_keys    Either a string key or an array of one or more keys.
     * @param string|int  $timeout_or_key  If the previous argument was an array, this argument
     *                                     must be a timeout value.  Otherwise it could also be
     *                                     another key.
     * @param mixed       $extra_args      Can consist of additional keys, until the last argument
     *                                     which needs to be a timeout.
     *
     * Following are examples of the two main ways to call this method.
     *
     * <code>
     * // Method 1 - Variadic, with the last argument being our timeout
     * $redis->bzPopMax('key1', 'key2', 'key3', 1.5);
     *
     * // Method 2 - A single array of keys, followed by the timeout
     * $redis->bzPopMax(['key1', 'key2', 'key3'], 1.5);
     * <?php>
     *
     * NOTE:  We reccomend calling this function with an array and a timeout as the other strategy
     *        may be deprecated in future versions of PhpRedis
     * ?>
     */
    public function bzPopMax(string|array $key, string|int $timeout_or_key, mixed ...$extra_args): Redis|array|false;

    /**
     * POP the minimum scoring element off of one or more sorted sets, blocking up to a specified timeout
     * if no elements are available
     *
     * This command is identical in semantics to bzPopMax so please see that method for more information.
     *
     * @see https://redis.io/commands/bzpopmin
     * @see Redis::bzPopMax()
     *
     */
    public function bzPopMin(string|array $key, string|int $timeout_or_key, mixed ...$extra_args): Redis|array|false;

    /**
     * POP one or more elements from one or more sorted sets, blocking up to a specified amount of time
     * when no elements are available.
     *
     * @param float  $timeout How long to block if there are no element available
     * @param array  $keys    The sorted sets to pop from
     * @param string $from    The string 'MIN' or 'MAX' (case insensitive) telling Redis whether you wish to
     *                        pop the lowest or highest scoring members from the set(s).
     * @param int    $count   Pop up to how many elements.
     *
     * @return Redis|array|null|false This function will return an array of popped elements, or false
     *                                depending on whether any elements could be popped within the
     *                                specified timeout.
     *
     * NOTE:  If Redis::OPT_NULL_MULTIBULK_AS_NULL is set to true via Redis::setOption(), this method will
     *        instead return NULL when Redis doesn't pop any elements.
     */
    public function bzmpop(float $timeout, array $keys, string $from, int $count = 1): Redis|array|null|false;

    /**
     * POP one or more of the highest or lowest scoring elements from one or more sorted sets.
     *
     * @see https://redis.io/commands/zmpop
     *
     * @param array  $keys  One or more sorted sets
     * @param string $from  The string 'MIN' or 'MAX' (case insensitive) telling Redis whether you want to
     *                      pop the lowest or highest scoring elements.
     * @param int    $count Pop up to how many elements at once.
     *
     * @return Redis|array|null|false An array of popped elements or false if none could be popped.
     */
    public function zmpop(array $keys, string $from, int $count = 1): Redis|array|null|false;

    /**
     * Pop one or more elements from one or more Redis LISTs, blocking up to a specified timeout when
     * no elements are available.
     *
     * @see https://redis.io/commands/blmpop
     *
     * @param float  $timeout The number of seconds Redis will block when no elements are available.
     * @param array  $keys    One or more Redis LISTs to pop from.
     * @param string $from    The string 'LEFT' or 'RIGHT' (case insensitive), telling Redis whether
     *                        to pop elements from the beginning or end of the LISTs.
     * @param int    $count   Pop up to how many elements at once.
     *
     * @return Redis|array|null|false One or more elements popped from the list(s) or false if all LISTs
     *                                were empty.
     */
    public function blmpop(float $timeout, array $keys, string $from, int $count = 1): Redis|array|null|false;

    /**
     * Pop one or more elements off of one or more Redis LISTs.
     *
     * @see https://redis.io/commands/lmpop
     *
     * @param array  $keys  An array with one or more Redis LIST key names.
     * @param string $from  The string 'LEFT' or 'RIGHT' (case insensitive), telling Redis whether to pop\
     *                      elements from the beginning or end of the LISTs.
     * @param int    $count The maximum number of elements to pop at once.
     *
     * @return Redis|array|null|false One or more elements popped from the LIST(s) or false if all the LISTs
     *                                were empty.
     *
     */
    public function lmpop(array $keys, string $from, int $count = 1): Redis|array|null|false;

    /**
     * Reset any last error on the connection to NULL
     *
     * @return bool This should always return true or throw an exception if we're not connected.
     *
     */
    public function clearLastError(): bool;

    public function client(string $opt, mixed ...$args): mixed;

    public function close(): bool;

    public function command(string $opt = null, string|array $arg): mixed;

    /**
      Execute the Redis CONFIG command in a variety of ways.  What the command does in particular depends
      on the `$operation` qualifier.

      Operations that PhpRedis supports are: RESETSTAT, REWRITE, GET, and SET.

      @param string            $operation      The CONFIG subcommand to execute
      @param array|string|null $key_or_setting Can either be a setting string for the GET/SET operation or
                                               an array of settings or settings and values.
                                               Note:  Redis 7.0.0 is required to send an array of settings.
      @param ?string           $value          The setting value when the operation is SET.

      <code>
      <?php
      $redis->config('GET', 'timeout');
      $redis->config('GET', ['timeout', 'databases']);

      $redis->config('SET', 'timeout', 30);
      $redis->config('SET', ['timeout' => 30, 'loglevel' => 'warning']);
      ?>
      </code>
     */
    public function config(string $operation, array|string|null $key_or_settings = NULL, ?string $value = NULL): mixed;

    public function connect(string $host, int $port = 6379, float $timeout = 0, string $persistent_id = null, int $retry_interval = 0, float $read_timeout = 0, array $context = null): bool;

    public function copy(string $src, string $dst, array $options = null): Redis|bool;

    public function dbSize(): Redis|int;

    public function debug(string $key): Redis|string;

    public function decr(string $key, int $by = 1): Redis|int|false;

    public function decrBy(string $key, int $value): Redis|int|false;

    public function del(array|string $key, string ...$other_keys): Redis|int|false;

    /**
     * @deprecated
     * @alias Redis::del
     */
    public function delete(array|string $key, string ...$other_keys): Redis|int|false;

    public function discard(): Redis|bool;

    public function dump(string $key): Redis|string;

    public function echo(string $str): Redis|string|false;

    /**
     * Execute a LUA script on the redis server.
     *
     * @see https://redis.io/commands/eval/
     *
     * @param string $script   A string containing the LUA script
     * @param array  $args     An array of arguments to pass to this script
     * @param int    $num_keys How many of the arguments are keys.  This is needed
     *                         as redis distinguishes between key name arguments
     *                         and other data.
     *
     * @return mixed LUA scripts may return arbitrary data so this method can return
     *               strings, arrays, nested arrays, etc.
     */
    public function eval(string $script, array $args = [], int $num_keys = 0): mixed;

    /**
     * This is simply the read-only variant of eval, meaning the underlying script
     * may not modify data in redis.
     *
     * @see Redis::eval()
     */
    public function eval_ro(string $script_sha, array $args = [], int $num_keys = 0): mixed;

    /**
     * Execute a LUA script on the server but instead of sending the script, send
     * the SHA1 hash of the script.
     *
     * @see https://redis.io/commands/evalsha/
     * @see Redis::eval();
     *
     * @param string $script_sha The SHA1 hash of the lua code.  Note that the script
     *                           must already exist on the server, either having been
     *                           loaded with `SCRIPT LOAD` or having been executed directly
     *                           with `EVAL` first.
     * @param array  $args       Arguments to send to the script.
     * @param int    $num_keys   The number of arguments that are keys
     */
    public function evalsha(string $sha1, array $args = [], int $num_keys = 0): mixed;

    /**
     * This is simply the read-only variant of evalsha, meaning the underlying script
     * may not modify data in redis.
     *
     * @see Redis::evalsha()
     */
    public function evalsha_ro(string $sha1, array $args = [], int $num_keys = 0): mixed;

    public function exec(): Redis|array|false;

    public function exists(mixed $key, mixed ...$other_keys): Redis|int|bool;

    /**
       Sets an expiration in seconds on the key in question.  If connected to
       redis-server >= 7.0.0 you may send an additional "mode" argument which
       modifies how the command will execute.

       @param string  $key  The key to set an expiration on.
       @param ?string $mode A two character modifier that changes how the
                            command works.
                            NX - Set expiry only if key has no expiry
                            XX - Set expiry only if key has an expiry
                            LT - Set expiry only when new expiry is < current expiry
                            GT - Set expiry only when new expiry is > current expiry
     */
    public function expire(string $key, int $timeout, ?string $mode = NULL): Redis|bool;

    /**
      Set a key's expiration to a specific Unix timestamp in seconds.  If
      connected to Redis >= 7.0.0 you can pass an optional 'mode' argument.

      @see Redis::expire() For a description of the mode argument.

       @param string  $key  The key to set an expiration on.
       @param ?string $mode A two character modifier that changes how the
                            command works.
     */
    public function expireAt(string $key, int $timestamp, ?string $mode = NULL): Redis|bool;

    public function failover(?array $to = null, bool $abort = false, int $timeout = 0): Redis|bool;

    public function expiretime(string $key): Redis|int|false;

    public function pexpiretime(string $key): Redis|int|false;

    /**
     * Deletes every key in all Redis databases
     *
     * @param  bool  $sync Whether to perform the task in a blocking or non-blocking way.
     *               when TRUE, PhpRedis will execute `FLUSHALL SYNC`, and when FALSE we
     *               will execute `FLUSHALL ASYNC`.  If the argument is omitted, we
     *               simply execute `FLUSHALL` and whether it is SYNC or ASYNC depends
     *               on Redis' `lazyfree-lazy-user-flush` config setting.
     * @return bool
     */
    public function flushAll(?bool $sync = null): Redis|bool;

    /**
     * Deletes all the keys of the currently selected database.
     *
     * @param  bool  $sync Whether to perform the task in a blocking or non-blocking way.
     *               when TRUE, PhpRedis will execute `FLUSHDB SYNC`, and when FALSE we
     *               will execute `FLUSHDB ASYNC`.  If the argument is omitted, we
     *               simply execute `FLUSHDB` and whether it is SYNC or ASYNC depends
     *               on Redis' `lazyfree-lazy-user-flush` config setting.
     * @return bool
     */
    public function flushDB(?bool $sync = null): Redis|bool;

    public function geoadd(string $key, float $lng, float $lat, string $member, mixed ...$other_triples): Redis|int|false;

    public function geodist(string $key, string $src, string $dst, ?string $unit = null): Redis|float|false;

    public function geohash(string $key, string $member, string ...$other_members): Redis|array|false;

    public function geopos(string $key, string $member, string ...$other_members): Redis|array|false;

    public function georadius(string $key, float $lng, float $lat, float $radius, string $unit, array $options = []): mixed;

    public function georadius_ro(string $key, float $lng, float $lat, float $radius, string $unit, array $options = []): mixed;

    public function georadiusbymember(string $key, string $member, float $radius, string $unit, array $options = []): mixed;

    public function georadiusbymember_ro(string $key, string $member, float $radius, string $unit, array $options = []): mixed;

    public function geosearch(string $key, array|string $position, array|int|float $shape, string $unit, array $options = []): array;

    public function geosearchstore(string $dst, string $src, array|string $position, array|int|float $shape, string $unit, array $options = []): Redis|array|int|false;

    public function get(string $key): mixed;

    /**
     * Get the authentication information on the connection, if any.
     *
     * @see Redis::auth()
     *
     * @return mixed The authentication information used to authenticate the connection.
     */
    public function getAuth(): mixed;

    public function getBit(string $key, int $idx): Redis|int|false;

    public function getEx(string $key, array $options = []): Redis|string|bool;

    public function getDBNum(): int;

    public function getDel(string $key): Redis|string|bool;

    /**
     * Return the host or Unix socket we are connected to.
     *
     * @return string The host or Unix socket.
     */
    public function getHost(): string;

    /**
     * Get the last error returned to us from Redis, if any.
     *
     * @return string The error string or NULL if there is none.
     */
    public function getLastError(): ?string;

    /**
     * Returns whether the connection is in ATOMIC, MULTI, or PIPELINE mode
     *
     * @return int The mode we're in.
     *
     */
    public function getMode(): int;

    /**
     * Retrieve the value of a configuration setting as set by Redis::setOption()
     *
     * @see Redis::setOption() for a detailed list of options and their values.
     *
     * @return mixed The setting itself or false on failure
     */
    public function getOption(int $option): mixed;

    public function getPersistentID(): ?string;

    public function getPort(): int;

    public function getRange(string $key, int $start, int $end): Redis|string|false;

    public function lcs(string $key1, string $key2, ?array $options = NULL): Redis|string|array|int|false;

    public function getReadTimeout(): int;

    public function getset(string $key, mixed $value): Redis|string|false;

    public function getTimeout(): int;

    public function getTransferredBytes(): int|false;

    public function hDel(string $key, string $member, string ...$other_members): Redis|int|false;

    public function hExists(string $key, string $member): Redis|bool;

    public function hGet(string $key, string $member): mixed;

    public function hGetAll(string $key): Redis|array|false;

    public function hIncrBy(string $key, string $member, int $value): Redis|int|false;

    public function hIncrByFloat(string $key, string $member, float $value): Redis|float|false;

    public function hKeys(string $key): Redis|array|false;

    public function hLen(string $key): Redis|int|false;

    public function hMget(string $key, array $keys): Redis|array|false;

    public function hMset(string $key, array $keyvals): Redis|bool;

    public function hRandField(string $key, array $options = null): Redis|string|array;

    public function hSet(string $key, string $member, mixed $value): Redis|int|false;

    public function hSetNx(string $key, string $member, string $value): Redis|bool;

    public function hStrLen(string $key, string $member): Redis|int|false;

    public function hVals(string $key): Redis|array|false;

    public function hscan(string $key, ?int &$iterator, ?string $pattern = null, int $count = 0): Redis|bool|array;

    /** @return Redis|int|false */
    public function incr(string $key, int $by = 1);

    /** @return Redis|int|false */
    public function incrBy(string $key, int $value);

    /** @return Redis|int|false */
    public function incrByFloat(string $key, float $value);

    /**
     * Retrieve information about the connected redis-server.  If no arguments are passed to
     * this function, redis will return every info field.  Alternatively you may pass a specific
     * section you want returned (e.g. 'server', or 'memory') to receive only information pertaining
     * to that section.
     *
     * If connected to Redis server >= 7.0.0 you may pass multiple optional sections.
     *
     * @see https://redis.io/commands/info/
     *
     * @param string $sections Optional section(s) you wish Redis server to return.
     *
     * @return Redis|array|false
     */
    public function info(string ...$sections): Redis|array|false;

    public function isConnected(): bool;

    /** @return Redis|array|false */
    public function keys(string $pattern);

    /**
     * @param mixed $elements
     * @return Redis|int|false
     */
    public function lInsert(string $key, string $pos, mixed $pivot, mixed $value);

    public function lLen(string $key): Redis|int|false;

    public function lMove(string $src, string $dst, string $wherefrom, string $whereto): Redis|string|false;

    public function lPop(string $key, int $count = 0): Redis|bool|string|array;

    public function lPos(string $key, mixed $value, array $options = null): Redis|null|bool|int|array;

    /**
     * @param mixed $elements
     * @return int|Redis
     */
    public function lPush(string $key, ...$elements);

    /**
     * @param mixed $elements
     * @return Redis|int|false
     */
    public function rPush(string $key, ...$elements);

    /** @return Redis|int|false*/
    public function lPushx(string $key, mixed $value);

    /** @return Redis|int|false*/
    public function rPushx(string $key, mixed $value);

    public function lSet(string $key, int $index, mixed $value): Redis|bool;

    public function lastSave(): int;

    public function lindex(string $key, int $index): mixed;

    public function lrange(string $key, int $start , int $end): Redis|array|false;

    /**
     * @return int|Redis|false
     */
    public function lrem(string $key, mixed $value, int $count = 0);

    public function ltrim(string $key, int $start , int $end): Redis|bool;

    /** @return array|Redis */
    public function mget(array $keys);

    public function migrate(string $host, int $port, string|array $key, int $dstdb, int $timeout,
                            bool $copy = false, bool $replace = false,
                            #[\SensitiveParameter] mixed $credentials = NULL): Redis|bool;

    public function move(string $key, int $index): bool;

    public function mset(array $key_values): Redis|bool;

    public function msetnx(array $key_values): Redis|bool;

    public function multi(int $value = Redis::MULTI): bool|Redis;

    public function object(string $subcommand, string $key): Redis|int|string|false;

    /**
     * @deprecated
     * @alias Redis::connect
     */
    public function open(string $host, int $port = 6379, float $timeout = 0, string $persistent_id = NULL, int $retry_interval = 0, float $read_timeout = 0, array $context = NULL): bool;

    public function pconnect(string $host, int $port = 6379, float $timeout = 0, string $persistent_id = NULL, int $retry_interval = 0, float $read_timeout = 0, array $context = NULL): bool;

    public function persist(string $key): bool;

    /**
     *  Sets an expiration in milliseconds on a given key.  If connected to Redis >= 7.0.0
     *  you can pass an optional mode argument that modifies how the command will execute.
     *
     *  @see Redis::expire() for a description of the mode argument.
     *
     *  @param string  $key  The key to set an expiration on.
     *  @param string $mode  A two character modifier that changes how the
     *                       command works.
     *
     *  @return Redis|bool   True if an expiry was set on the key, and false otherwise.
     */
    public function pexpire(string $key, int $timeout, ?string $mode = NULL): bool;

    /**
     * Set a key's expiration to a specific Unix Timestamp in milliseconds.  If connected to
     * Redis >= 7.0.0 you can pass an optional 'mode' argument.
     *
     * @see Redis::expire() For a description of the mode argument.
     *
     *  @param string  $key  The key to set an expiration on.
     *  @param string  $mode A two character modifier that changes how the
     *                       command works.
     *
     *  @return Redis|bool   True if an expiration was set on the key, false otherwise.
     */
    public function pexpireAt(string $key, int $timestamp, ?string $mode = NULL): Redis|bool;

    /**
     * Add one or more elements to a Redis HyperLogLog key
     *
     * @see https://redis.io/commands/pfadd
     *
     * @param string $key      The key in question.
     *
     * @param array  $elements One or more elements to add.
     *
     * @return Redis|int Returns 1 if the set was altered, and zero if not.
     */
    public function pfadd(string $key, array $elements): Redis|int;

    /**
     * Retrieve the cardinality of a Redis HyperLogLog key.
     *
     * @see https://redis.io/commands/pfcount
     *
     * @param string $key The key name we wish to query.
     *
     * @return Redis|int The estimated cardinality of the set.
     */
    public function pfcount(string $key): Redis|int;

    /**
     * Merge one or more source HyperLogLog sets into a destination set.
     *
     * @see https://redis.io/commands/pfmerge
     *
     * @param string $dst     The destination key.
     * @param array  $srckeys One or more source keys.
     *
     * @return Redis|bool Always returns true.
     */
    public function pfmerge(string $dst, array $srckeys): Redis|bool;

    /**
     * PING the redis server with an optional string argument.
     *
     * @see https://redis.io/commands/ping
     *
     * @param string $message An optional string message that Redis will reply with, if passed.
     *
     * @return Redis|string|false If passed no message, this command will simply return `true`.
     *                            If a message is passed, it will return the message.
     *
     */
    public function ping(string $message = NULL): Redis|string|bool;

    public function pipeline(): bool|Redis;

    /**
     * @deprecated
     * @alias Redis::pconnect
     */
    public function popen(string $host, int $port = 6379, float $timeout = 0, string $persistent_id = NULL, int $retry_interval = 0, float $read_timeout = 0, array $context = NULL): bool;

    /** @return bool|Redis */
    public function psetex(string $key, int $expire, mixed $value);

    public function psubscribe(array $patterns, callable $cb): bool;

    public function pttl(string $key): Redis|int|false;

    public function publish(string $channel, string $message): mixed;

    public function pubsub(string $command, mixed $arg = null): mixed;

    public function punsubscribe(array $patterns): Redis|array|bool;

    /**
     * Pop one or more elements from the end of a Redis LIST.
     *
     * @see https://redis.io/commands/rpop
     *
     * @param string $key   A redis LIST key name.
     * @param int    $count The maximum number of elements to pop at once.
     *
     * NOTE:  The `count` argument requires Redis >= 6.2.0
     *
     * @return Redis|array|string|bool One ore more popped elements or false if all were empty.
     */
    public function rPop(string $key, int $count = 0): Redis|array|string|bool;

    /** @return string|Redis */
    public function randomKey();

    public function rawcommand(string $command, mixed ...$args): mixed;

    /** @return bool|Redis */
    public function rename(string $key_src, string $key_dst);

    /** @return bool|Redis */
    public function renameNx(string $key_src, string $key_dst);

    public function reset(): bool;

    public function restore(string $key, int $timeout, string $value, ?array $options = NULL): bool;

    public function role(): mixed;

    /**
     * Atomically pop an element off the end of a Redis LIST and push it to the beginning of
     * another.
     *
     * @see https://redis.io/commands/rpoplpush
     *
     * @param string $srckey The source key to pop from.
     * @param string $dstkey The destination key to push to.
     *
     * @return Redis|string|false The popped element or false if the source key was empty.
     *
     * <code>
     * <?php
     * $redis = new Redis(['host' => 'localhost']);
     *
     * $redis->pipeline()
     *       ->del('list1', 'list2')
     *       ->rpush('list1', 'list1-1', 'list1-2')
     *       ->rpush('list2', 'list2-1', 'list2-2')
     *       ->exec();
     *
     * var_dump($redis->rpoplpush('list2', 'list1'));
     * var_dump($redis->lrange('list1', 0, -1));
     *
     * // --- OUTPUT ---
     * // string(7) "list2-2"
     * //
     * // array(3) {
     * //   [0]=>
     * //   string(7) "list2-2"
     * //   [1]=>
     * //   string(7) "list1-1"
     * //   [2]=>
     * //   string(7) "list1-2"
     * // }
     * ?>
     * </code>
     */
    public function rpoplpush(string $srckey, string $dstkey): Redis|string|false;

    public function sAdd(string $key, mixed $value, mixed ...$other_values): Redis|int|false;

    public function sAddArray(string $key, array $values): int;

    public function sDiff(string $key, string ...$other_keys): Redis|array|false;

    public function sDiffStore(string $dst, string $key, string ...$other_keys): Redis|int|false;

    public function sInter(array|string $key, string ...$other_keys): Redis|array|false;

    public function sintercard(array $keys, int $limit = -1): Redis|int|false;

    public function sInterStore(array|string $key, string ...$other_keys): Redis|int|false;

    public function sMembers(string $key): Redis|array|false;

    public function sMisMember(string $key, string $member, string ...$other_members): array;

    public function sMove(string $src, string $dst, mixed $value): Redis|bool;

    public function sPop(string $key, int $count = 0): Redis|string|array|false;

    public function sRandMember(string $key, int $count = 0): Redis|string|array|false;

    public function sUnion(string $key, string ...$other_keys): Redis|array|false;

    public function sUnionStore(string $dst, string $key, string ...$other_keys): Redis|int|false;

    public function save(): bool;

    public function scan(?int &$iterator, ?string $pattern = null, int $count = 0, string $type = NULL): array|false;

    public function scard(string $key): Redis|int|false;

    public function script(string $command, mixed ...$args): mixed;

    public function select(int $db): Redis|bool;

    public function set(string $key, mixed $value, mixed $opt = NULL): Redis|string|bool;

    /** @return Redis|int|false*/
    public function setBit(string $key, int $idx, bool $value);

    /** @return Redis|int|false*/
    public function setRange(string $key, int $start, string $value);


    /**
     * Set a configurable option on the Redis object.
     *
     * Following are a list of options you can set:
     *
     *  OPTION                     TYPE     DESCRIPTION
     *  OPT_MAX_RETRIES            int      The maximum number of times Redis will attempt to reconnect
     *                                      if it gets disconnected, before throwing an exception.
     *
     *  OPT_SCAN                   enum     Redis::OPT_SCAN_RETRY, or Redis::OPT_SCAN_NORETRY
     *
     *  OPT_SERIALIZER             int      One of the installed serializers, which can vary depending
     *                                      on how PhpRedis was compiled.  All of the supported serializers
     *                                      are as follows:
     *
     *                                      Redis::SERIALIZER_NONE
     *                                      Redis::SERIALIZER_PHP
     *                                      Redis::SERIALIZER_IGBINARY
     *                                      Redis::SERIALIZER_MSGPACK
     *                                      Redis::SERIALIZER_JSON
     *
     *                                      Note:  The PHP and JSON serializers are always available.
     *
     *  OPT_PREFIX                  string  A string PhpRedis will use to prefix every key we read or write.
     *                                      To disable the prefix, you may pass an empty string or NULL.
     *
     *  OPT_READ_TIMEOUT            double  How long PhpRedis will block for a response from Redis before
     *                                      throwing a 'read error on connection' exception.
     *
     *  OPT_TCP_KEEPALIVE           bool    Set or disable TCP_KEEPALIVE on the connection.
     *
     *  OPT_COMPRESSION             enum    Set an automatic compression algorithm to use when reading/writing
     *                                      data to Redis.  All of the supported compressors are as follows:
     *
     *                                      Redis::COMPRESSION_NONE
     *                                      Redis::COMPRESSION_LZF
     *                                      Redis::COMPRESSION_LZ4
     *                                      Redis::COMPRESSION_ZSTD
     *
     *                                      Note:  Some of these may not be available depending on how Redis
     *                                             was compiled.
     *
     *  OPT_REPLY_LITERAL           bool    If set to true, PhpRedis will return the literal string Redis returns
     *                                      for LINE replies (e.g. '+OK'), rather than `true`.
     *
     *  OPT_COMPRESSION_LEVEL       int     Set a specific compression level if Redis is compressing data.
     *
     *  OPT_NULL_MULTIBULK_AS_NULL  bool    Causes PhpRedis to return `NULL` rather than `false` for NULL MULTIBULK
     *                                      RESP replies (i.e. `*-1`).
     *
     *  OPT_BACKOFF_ALGORITHM       enum    The exponential backoff strategy to use.
     *  OPT_BACKOFF_BASE            int     The minimum delay between retries when backing off.
     *  OPT_BACKOFF_CAP             int     The maximum delay between replies when backing off.
     *
     * @see Redis::__construct() for details about backoff strategies.
     *
     * @param int    $option The option constant.
     * @param mixed  $value  The option value.
     *
     * @return bool  True if the setting was updated, false if not.
     *
     */
    public function setOption(int $option, mixed $value): bool;

    /** @return bool|Redis */
    public function setex(string $key, int $expire, mixed $value);

    /** @return bool|array|Redis */
    public function setnx(string $key, mixed $value);

    public function sismember(string $key, mixed $value): Redis|bool;

    public function slaveof(string $host = null, int $port = 6379): bool;

    /**
     * Update one or more keys last modified metadata.
     *
     * @see https://redis.io/commands/touch/
     *
     * @param array|string $key    Either the first key or if passed as the only argument
     *                             an array of keys.
     * @param string $more_keys    One or more keys to send to the command.
     *
     * @return Redis|int|false     This command returns the number of keys that exist and
     *                             had their last modified time reset
     */
    public function touch(array|string $key_or_array, string ...$more_keys): Redis|int|false;

    /**
     * Interact with Redis' slowlog functionality in various ways, depending
     * on the value of 'operation'.
     *
     * @see https://redis.io/commands/slowlog/
     * @category administration
     *
     * @param string $operation  The operation you wish to perform.  This can
     *                           be one of the following values:
     *                           'GET'   - Retreive the Redis slowlog as an array.
     *                           'LEN'   - Retreive the length of the slowlog.
     *                           'RESET' - Remove all slowlog entries.
     * <code>
     * <?php
     * $redis->slowlog('get', -1);  // Retreive all slowlog entries.
     * $redis->slowlog('len');       // Retreive slowlog length.
     * $redis->slowlog('reset');     // Reset the slowlog.
     * ?>
     * </code>
     *
     * @param int    $length     This optional argument can be passed when operation
     *                           is 'get' and will specify how many elements to retreive.
     *                           If omitted Redis will send up to a default number of
     *                           entries, which is configurable.
     *
     *                           Note:  With Redis >= 7.0.0 you can send -1 to mean "all".
     *
     * @return mixed
     */
    public function slowlog(string $operation, int $length = 0): mixed;

    /**
     * Sort the contents of a Redis key in various ways.
     *
     * @see https://redis.io/commands/sort/
     *
     * @param string $key     The key you wish to sort
     * @param array  $options Various options controlling how you would like the
     *                        data sorted.  See blow for a detailed description
     *                        of this options array.
     *
     * @return mixed This command can either return an array with the sorted data
     *               or the number of elements placed in a destination set when
     *               using the STORE option.
     *
     * <code>
     * <?php
     * $options = [
     *     'SORT'  => 'ASC'|| 'DESC' // Sort in descending or descending order.
     *     'ALPHA' => true || false  // Whether to sort alphanumerically.
     *     'LIMIT' => [0, 10]        // Return a subset of the data at offset, count
     *     'BY'    => 'weight_*'     // For each element in the key, read data from the
     *                                  external key weight_* and sort based on that value.
     *     'GET'   => 'weight_*'     // For each element in the source key, retrieve the
     *                                  data from key weight_* and return that in the result
     *                                  rather than the source keys' element.  This can
     *                                  be used in combination with 'BY'
     * ];
     * ?>
     * </code>
     *
     */
    public function sort(string $key, ?array $options = null): mixed;

    /**
     * This is simply a read-only variant of the sort command
     *
     * @see Redis::sort()
     */
    public function sort_ro(string $key, ?array $options = null): mixed;

    /**
     * @deprecated
     */
    public function sortAsc(string $key, ?string $pattern = null, mixed $get = null, int $offset = -1, int $count = -1, ?string $store = null): array;

    /**
     * @deprecated
     */
    public function sortAscAlpha(string $key, ?string $pattern = null, mixed $get = null, int $offset = -1, int $count = -1, ?string $store = null): array;

    /**
     * @deprecated
     */
    public function sortDesc(string $key, ?string $pattern = null, mixed $get = null, int $offset = -1, int $count = -1, ?string $store = null): array;

    /**
     * @deprecated
     */
    public function sortDescAlpha(string $key, ?string $pattern = null, mixed $get = null, int $offset = -1, int $count = -1, ?string $store = null): array;

    public function srem(string $key, mixed $value, mixed ...$other_values): Redis|int|false;

    public function sscan(string $key, ?int &$iterator, ?string $pattern = null, int $count = 0): array|false;

    /** @return Redis|int|false*/
    public function strlen(string $key);

    public function subscribe(array $channels, callable $cb): bool;

    public function swapdb(string $src, string $dst): bool;

    public function time(): array;

    public function ttl(string $key): Redis|int|false;

    /** @return Redis|int|false*/
    public function type(string $key);

    /**
     * @return Redis|int|false
     */
    public function unlink(array|string $key, string ...$other_keys);

    public function unsubscribe(array $channels): Redis|array|bool;

    /** @return bool|Redis */
    public function unwatch();

    /**
     * @return bool|Redis
     */
    public function watch(array|string $key, string ...$other_keys);

    public function wait(int $count, int $timeout): int|false;

    public function xack(string $key, string $group, array $ids): int|false;

    public function xadd(string $key, string $id, array $values, int $maxlen = 0, bool $approx = false, bool $nomkstream = false): Redis|string|false;

    public function xautoclaim(string $key, string $group, string $consumer, int $min_idle, string $start, int $count = -1, bool $justid = false): Redis|bool|array;

    public function xclaim(string $key, string $group, string $consumer, int $min_idle, array $ids, array $options): Redis|bool|array;

    public function xdel(string $key, array $ids): Redis|int|false;

    /**
     * XGROUP
     *
     * Perform various operation on consumer groups for a particular Redis STREAM.  What the command does
     * is primarily based on which operation is passed.
     *
     * @see https://redis.io/commands/xgroup/
     *
     * @param string $operation      The subcommand you intend to execute.  Valid options are as follows
     *                               'HELP'          - Redis will return information about the command
     *                                                 Requires: none
     *                               'CREATE'        - Create a consumer group.
     *                                                 Requires:  Key, group, consumer.
     *                               'SETID'         - Set the ID of an existing consumer group for the stream.
     *                                                 Requires:  Key, group, id.
     *                               'CREATECONSUMER - Create a new consumer group for the stream.  You must
     *                                                 also pass key, group, and the consumer name you wish to
     *                                                 create.
     *                                                 Requires:  Key, group, consumer.
     *                               'DELCONSUMER'   - Delete a consumer from group attached to the stream.
     *                                                 Requires:  Key, group, consumer.
     *                               'DESTROY'       - Delete a consumer group from a stream.
     *                                                  Requires:  Key, group.
     * @param string $key            The STREAM we're operating on.
     * @param string $group          The consumer group we want to create/modify/delete.
     * @param string $id_or_consumer The STREAM id (e.g. '$') or consumer group.  See the operation section
     *                               for information about which to send.
     * @param bool   $mkstream       This flag may be sent in combination with the 'CREATE' operation, and
     *                               cause Redis to also create the STREAM if it doesn't currently exist.
     *
     * @param bool   $entriesread    Allows you to set Redis' 'entries-read' STREAM value.  This argument is
     *                               only relevant to the 'CREATE' and 'SETID' operations.
     *                               Note:  Requires Redis >= 7.0.0.
     *
     * @return mixed                 This command return various results depending on the operation performed.
     */
    public function xgroup(string $operation, string $key = null, string $group = null, string $id_or_consumer = null,
                           bool $mkstream = false, int $entries_read = -2): mixed;

    public function xinfo(string $operation, ?string $arg1 = null, ?string $arg2 = null, int $count = -1): mixed;

    public function xlen(string $key): Redis|int|false;

    public function xpending(string $key, string $group, ?string $start = null, ?string $end = null, int $count = -1, ?string $consumer = null): Redis|array|false;

    public function xrange(string $key, string $start, string $end, int $count = -1): Redis|array|bool;

    public function xread(array $streams, int $count = -1, int $block = -1): Redis|array|bool;

    public function xreadgroup(string $group, string $consumer, array $streams, int $count = 1, int $block = 1): Redis|array|bool;

    public function xrevrange(string $key, string $start, string $end, int $count = -1): Redis|array|bool;

    public function xtrim(string $key, int $maxlen, bool $approx = false, bool $minid = false, int $limit = -1): Redis|int|false;

    public function zAdd(string $key, array|float $score_or_options, mixed ...$more_scores_and_mems): Redis|int|false;

    public function zCard(string $key): Redis|int|false;

    public function zCount(string $key, string $start , string $end): Redis|int|false;

    public function zIncrBy(string $key, float $value, mixed $member): Redis|float|false;

    public function zLexCount(string $key, string $min, string $max): Redis|int|false;

    public function zMscore(string $key, string $member, string ...$other_members): Redis|array|false;

    public function zPopMax(string $key, int $value = null): Redis|array|false;

    public function zPopMin(string $key, int $value = null): Redis|array|false;

    /**
     * Retreive a range of elements of a sorted set between a start and end point.
     * How the command works in particular is greatly affected by the options that
     * are passed in.
     *
     * @see https://redis.io/commands/zrange/
     * @category zset
     *
     * @param string          $key     The sorted set in question.
     * @param mixed           $start   The starting index we want to return.
     * @param mixed           $end     The final index we want to return.
     *
     * @param array|bool|null $options This value may either be an array of options to pass to
     *                                 the command, or for historical purposes a boolean which
     *                                 controls just the 'WITHSCORES' option.
     *
     * @return Redis|array|false  An array with matching elements or false on failure.
     *
     * Detailed description of options array:
     *
     * <code>
     * <?php
     * $options = [
     *     'WITHSCORES' => true,     // Return both scores and members.
     *     'LIMIT'      => [10, 10], // Start at offset 10 and return 10 elements.
     *     'REV'                     // Return the elements in reverse order
     *     'BYSCORE',                // Treat `start` and `end` as scores instead
     *     'BYLEX'                   // Treat `start` and `end` as lexicographical values.
     * ];
     * ?>
     * </code>
     *
     * Note:  'BYLEX' and 'BYSCORE' are mutually exclusive.
     *
     */
    public function zRange(string $key, mixed $start, mixed $end, array|bool|null $options = null): Redis|array|false;

    public function zRangeByLex(string $key, string $min, string $max, int $offset = -1, int $count = -1): Redis|array|false;

    public function zRangeByScore(string $key, string $start, string $end, array $options = []): Redis|array|false;

    /**
     * This command is similar to ZRANGE except that instead of returning the values directly
     * it will store them in a destination key provided by the user
     *
     * @see https://redis.io/commands/zrange/
     * @see Redis::zRange
     * @category zset
     *
     * @param string           $dstkey  The key to store the resulting element(s)
     * @param string           $srckey  The source key with element(s) to retrieve
     * @param string           $start   The starting index to store
     * @param string           $end     The ending index to store
     * @param array|bool|null  $options Our options array that controls how the command will function.
     *
     * @return Redis|int|false The number of elements stored in dstkey or false on failure.
     *
     * See Redis::zRange for a full description of the possible options.
     */
    public function zrangestore(string $dstkey, string $srckey, string $start, string $end,
                                array|bool|null $options = NULL): Redis|int|false;

    public function zRandMember(string $key, array $options = null): Redis|string|array;

    public function zRank(string $key, mixed $member): Redis|int|false;

    public function zRem(mixed $key, mixed $member, mixed ...$other_members): Redis|int|false;

    public function zRemRangeByLex(string $key, string $min, string $max): Redis|int|false;

    public function zRemRangeByRank(string $key, int $start, int $end): Redis|int|false;

    public function zRemRangeByScore(string $key, string $start, string $end): Redis|int|false;

    public function zRevRange(string $key, int $start, int $end, mixed $scores = null): Redis|array|false;

    public function zRevRangeByLex(string $key, string $min, string $max, int $offset = -1, int $count = -1): Redis|array|false;

    public function zRevRangeByScore(string $key, string $start, string $end, array $options = []): Redis|array|false;

    public function zRevRank(string $key, mixed $member): Redis|int|false;

    public function zScore(string $key, mixed $member): Redis|float|false;

    public function zdiff(array $keys, array $options = null): Redis|array|false;

    public function zdiffstore(string $dst, array $keys, array $options = null): Redis|int|false;

    public function zinter(array $keys, ?array $weights = null, ?array $options = null): Redis|array|false;

    public function zintercard(array $keys, int $limit = -1): Redis|int|false;

    public function zinterstore(string $dst, array $keys, ?array $weights = null, ?string $aggregate = null): Redis|int|false;

    public function zscan(string $key, ?int &$iterator, ?string $pattern = null, int $count = 0): Redis|bool|array;

    public function zunion(array $keys, ?array $weights = null, ?array $options = null): Redis|array|false;

    public function zunionstore(string $dst, array $keys, ?array $weights = NULL, ?string $aggregate = NULL): Redis|int|false;
}

class RedisException extends RuntimeException {}
