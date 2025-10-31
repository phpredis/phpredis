<?php

/**
 * @generate-function-entries
 * @generate-legacy-arginfo
 * @generate-class-entries
 */

/**
 * @method mixed acl(string $subcmd, string ...$args)
 * @method RedisArray|int|false append(string $key, mixed $value)
 * @method RedisArray|bool auth(#[\SensitiveParameter] mixed $credentials)
 * @method RedisArray|bool bgrewriteaof()
 * @method RedisArray|array|false waitaof(int $numlocal, int $numreplicas, int $timeout)
 * @method RedisArray|int|false bitcount(string $key, int $start = 0, int $end = -1, bool $bybit = false)
 * @method RedisArray|int|false bitop(string $operation, string $deskey, string $srckey, string ...$other_keys)
 * @method RedisArray|int|false bitpos(string $key, bool $bit, int $start = 0, int $end = -1, bool $bybit = false)
 * @method RedisArray|array|null|false blPop(string|array $key_or_keys, string|float|int $timeout_or_key, mixed ...$extra_args)
 * @method RedisArray|array|null|false brPop(string|array $key_or_keys, string|float|int $timeout_or_key, mixed ...$extra_args)
 * @method RedisArray|string|false brpoplpush(string $src, string $dst, int|float $timeout)
 * @method RedisArray|array|false bzPopMax(string|array $key, string|int $timeout_or_key, mixed ...$extra_args)
 * @method RedisArray|array|false bzPopMin(string|array $key, string|int $timeout_or_key, mixed ...$extra_args)
 * @method RedisArray|array|null|false bzmpop(float $timeout, array $keys, string $from, int $count = 1)
 * @method RedisArray|array|null|false zmpop(array $keys, string $from, int $count = 1)
 * @method RedisArray|array|null|false blmpop(float $timeout, array $keys, string $from, int $count = 1)
 * @method RedisArray|array|null|false lmpop(array $keys, string $from, int $count = 1)
 * @method bool clearLastError()
 * @method mixed client(string $opt, mixed ...$args)
 * @method bool close()
 * @method mixed command(?string $opt = null, mixed ...$args)
 * @method mixed config(string $operation, array|string|null $key_or_settings = null, ?string $value = null)
 * @method RedisArray|bool copy(string $src, string $dst, ?array $options = null)
 * @method RedisArray|int|false dbSize()
 * @method RedisArray|string debug(string $key)
 * @method RedisArray|int|false decr(string $key, int $by = 1)
 * @method RedisArray|int|false decrBy(string $key, int $value)
 * @method RedisArray|int|false delifeq(string $key, mixed $value)
 * @method RedisArray|int|false delete(array|string $key, string ...$other_keys)
 * @method RedisArray|string|false dump(string $key)
 * @method RedisArray|string|false echo(string $str)
 * @method mixed eval(string $script, array $args = [], int $num_keys = 0)
 * @method mixed eval_ro(string $script_sha, array $args = [], int $num_keys = 0)
 * @method mixed evalsha(string $sha1, array $args = [], int $num_keys = 0)
 * @method mixed evalsha_ro(string $sha1, array $args = [], int $num_keys = 0)
 * @method RedisArray|int|bool exists(mixed $key, mixed ...$other_keys)
 * @method RedisArray|bool expire(string $key, int $timeout, ?string $mode = null)
 * @method RedisArray|bool expireAt(string $key, int $timestamp, ?string $mode = null)
 * @method RedisArray|bool failover(?array $to = null, bool $abort = false, int $timeout = 0)
 * @method RedisArray|int|false expiretime(string $key)
 * @method RedisArray|int|false pexpiretime(string $key)
 * @method mixed fcall(string $fn, array $keys = [], array $args = [])
 * @method mixed fcall_ro(string $fn, array $keys = [], array $args = [])
 * @method RedisArray|bool|string|array function(string $operation, mixed ...$args)
 * @method RedisArray|int|false geoadd(string $key, float $lng, float $lat, string $member, mixed ...$other_triples_and_options)
 * @method RedisArray|float|false geodist(string $key, string $src, string $dst, ?string $unit = null)
 * @method RedisArray|array|false geohash(string $key, string $member, string ...$other_members)
 * @method RedisArray|array|false geopos(string $key, string $member, string ...$other_members)
 * @method mixed georadius(string $key, float $lng, float $lat, float $radius, string $unit, array $options = [])
 * @method mixed georadius_ro(string $key, float $lng, float $lat, float $radius, string $unit, array $options = [])
 * @method mixed georadiusbymember(string $key, string $member, float $radius, string $unit, array $options = [])
 * @method mixed georadiusbymember_ro(string $key, string $member, float $radius, string $unit, array $options = [])
 * @method array geosearch(string $key, array|string $position, array|int|float $shape, string $unit, array $options = [])
 * @method RedisArray|array|int|false geosearchstore(string $dst, string $src, array|string $position, array|int|float $shape, string $unit, array $options = [])
 * @method mixed get(string $key)
 * @method RedisArray|array|false getWithMeta(string $key)
 * @method mixed getAuth()
 * @method RedisArray|int|false getBit(string $key, int $idx)
 * @method RedisArray|string|bool getEx(string $key, array $options = [])
 * @method int getDBNum()
 * @method RedisArray|string|bool getDel(string $key)
 * @method string getHost()
 * @method string|null getLastError()
 * @method int getMode()
 * @method string|null getPersistentID()
 * @method int getPort()
 * @method string|false serverName()
 * @method string|false serverVersion()
 * @method RedisArray|string|false getRange(string $key, int $start, int $end)
 * @method RedisArray|string|array|int|false lcs(string $key1, string $key2, ?array $options = null)
 * @method float getReadTimeout()
 * @method RedisArray|string|false getset(string $key, mixed $value)
 * @method float|false getTimeout()
 * @method array getTransferredBytes()
 * @method void clearTransferredBytes()
 * @method RedisArray|int|false hDel(string $key, string $field, string ...$other_fields)
 * @method RedisArray|bool hExists(string $key, string $field)
 * @method mixed hGet(string $key, string $member)
 * @method RedisArray|array|false hGetAll(string $key)
 * @method mixed hGetWithMeta(string $key, string $member)
 * @method RedisArray|int|false hIncrBy(string $key, string $field, int $value)
 * @method RedisArray|float|false hIncrByFloat(string $key, string $field, float $value)
 * @method RedisArray|array|false hKeys(string $key)
 * @method RedisArray|int|false hLen(string $key)
 * @method RedisArray|array|false hMget(string $key, array $fields)
 * @method RedisArray|array|false hgetex(string $key, array $fields, string|array|null $expiry = null)
 * @method RedisArray|int|false hsetex(string $key, array $fields, ?array $expiry = null)
 * @method RedisArray|array|false hgetdel(string $key, array $fields)
 * @method RedisArray|bool hMset(string $key, array $fieldvals)
 * @method RedisArray|string|array|false hRandField(string $key, ?array $options = null)
 * @method RedisArray|int|false hSet(string $key, mixed ...$fields_and_vals)
 * @method RedisArray|bool hSetNx(string $key, string $field, mixed $value)
 * @method RedisArray|int|false hStrLen(string $key, string $field)
 * @method RedisArray|array|false hVals(string $key)
 * @method RedisArray|array|false httl(string $key, array $fields)
 * @method RedisArray|array|false hpttl(string $key, array $fields)
 * @method RedisArray|array|false hexpiretime(string $key, array $fields)
 * @method RedisArray|array|false hpexpiretime(string $key, array $fields)
 * @method RedisArray|array|false hpersist(string $key, array $fields)
 * @method RedisArray|int|false expiremember(string $key, string $field, int $ttl, ?string $unit = null)
 * @method RedisArray|int|false expirememberat(string $key, string $field, int $timestamp)
 * @method RedisArray|int|false incr(string $key, int $by = 1)
 * @method RedisArray|int|false incrBy(string $key, int $value)
 * @method RedisArray|float|false incrByFloat(string $key, float $value)
 * @method bool isConnected()
 * @method lInsert(string $key, string $pos, mixed $pivot, mixed $value)
 * @method RedisArray|int|false lLen(string $key)
 * @method RedisArray|string|false lMove(string $src, string $dst, string $wherefrom, string $whereto)
 * @method RedisArray|string|false blmove(string $src, string $dst, string $wherefrom, string $whereto, float $timeout)
 * @method RedisArray|bool|string|array lPop(string $key, int $count = 0)
 * @method RedisArray|null|bool|int|array lPos(string $key, mixed $value, ?array $options = null)
 * @method RedisArray|int|false lPush(string $key, mixed ...$elements)
 * @method RedisArray|int|false rPush(string $key, mixed ...$elements)
 * @method RedisArray|int|false lPushx(string $key, mixed $value)
 * @method RedisArray|int|false rPushx(string $key, mixed $value)
 * @method RedisArray|bool lSet(string $key, int $index, mixed $value)
 * @method int lastSave()
 * @method mixed lindex(string $key, int $index)
 * @method RedisArray|array|false lrange(string $key, int $start , int $end)
 * @method RedisArray|int|false lrem(string $key, mixed $value, int $count = 0)
 * @method RedisArray|bool ltrim(string $key, int $start , int $end)
 * @method RedisArray|bool move(string $key, int $index)
 * @method RedisArray|bool msetnx(array $key_values)
 * @method RedisArray|int|string|false object(string $subcommand, string $key)
 * @method bool open(string $host, int $port = 6379, float $timeout = 0, ?string $persistent_id = null, int $retry_interval = 0, float $read_timeout = 0, ?array $context = null)
 * @method bool pconnect(string $host, int $port = 6379, float $timeout = 0, ?string $persistent_id = null, int $retry_interval = 0, float $read_timeout = 0, ?array $context = null)
 * @method RedisArray|bool persist(string $key)
 * @method bool pexpire(string $key, int $timeout, ?string $mode = null)
 * @method RedisArray|bool pexpireAt(string $key, int $timestamp, ?string $mode = null)
 * @method RedisArray|int pfadd(string $key, array $elements)
 * @method RedisArray|int|false pfcount(array|string $key_or_keys)
 * @method RedisArray|bool pfmerge(string $dst, array $srckeys)
 * @method bool|RedisArray pipeline()
 * @method bool popen(string $host, int $port = 6379, float $timeout = 0, ?string $persistent_id = null, int $retry_interval = 0, float $read_timeout = 0, ?array $context = null)
 * @method RedisArray|bool psetex(string $key, int $expire, mixed $value)
 * @method bool psubscribe(array $patterns, callable $cb)
 * @method RedisArray|int|false pttl(string $key)
 * @method RedisArray|int|false publish(string $channel, string $message)
 * @method mixed pubsub(string $command, mixed $arg = null)
 * @method RedisArray|array|bool punsubscribe(array $patterns)
 * @method RedisArray|array|string|bool rPop(string $key, int $count = 0)
 * @method RedisArray|string|false randomKey()
 * @method mixed rawcommand(string $command, mixed ...$args)
 * @method RedisArray|bool rename(string $old_name, string $new_name)
 * @method RedisArray|bool renameNx(string $key_src, string $key_dst)
 * @method RedisArray|bool reset()
 * @method RedisArray|bool restore(string $key, int $ttl, string $value, ?array $options = null)
 * @method mixed role()
 * @method RedisArray|string|false rpoplpush(string $srckey, string $dstkey)
 * @method RedisArray|int|false sAdd(string $key, mixed $value, mixed ...$other_values)
 * @method int sAddArray(string $key, array $values)
 * @method RedisArray|array|false sDiff(string $key, string ...$other_keys)
 * @method RedisArray|int|false sDiffStore(string $dst, string $key, string ...$other_keys)
 * @method RedisArray|array|false sInter(array|string $key, string ...$other_keys)
 * @method RedisArray|int|false sintercard(array $keys, int $limit = -1)
 * @method RedisArray|int|false sInterStore(array|string $key, string ...$other_keys)
 * @method RedisArray|array|false sMembers(string $key)
 * @method RedisArray|array|false sMisMember(string $key, string $member, string ...$other_members)
 * @method RedisArray|bool sMove(string $src, string $dst, mixed $value)
 * @method RedisArray|string|array|false sPop(string $key, int $count = 0)
 * @method mixed sRandMember(string $key, int $count = 0)
 * @method RedisArray|array|false sUnion(string $key, string ...$other_keys)
 * @method RedisArray|int|false sUnionStore(string $dst, string $key, string ...$other_keys)
 * @method RedisArray|int|false scard(string $key)
 * @method mixed script(string $command, mixed ...$args)
 * @method RedisArray|string|bool set(string $key, mixed $value, mixed $options = null)
 * @method RedisArray|int|false setBit(string $key, int $idx, bool $value)
 * @method RedisArray|int|false setRange(string $key, int $index, string $value)
 * @method setex(string $key, int $expire, mixed $value)
 * @method RedisArray|bool setnx(string $key, mixed $value)
 * @method RedisArray|bool sismember(string $key, mixed $value)
 * @method RedisArray|bool slaveof(?string $host = null, int $port = 6379)
 * @method RedisArray|bool replicaof(?string $host = null, int $port = 6379)
 * @method RedisArray|int|false touch(array|string $key_or_array, string ...$more_keys)
 * @method mixed slowlog(string $operation, int $length = 0)
 * @method mixed sort(string $key, ?array $options = null)
 * @method mixed sort_ro(string $key, ?array $options = null)
 * @method array sortAsc(string $key, ?string $pattern = null, mixed $get = null, int $offset = -1, int $count = -1, ?string $store = null)
 * @method array sortAscAlpha(string $key, ?string $pattern = null, mixed $get = null, int $offset = -1, int $count = -1, ?string $store = null)
 * @method array sortDesc(string $key, ?string $pattern = null, mixed $get = null, int $offset = -1, int $count = -1, ?string $store = null)
 * @method array sortDescAlpha(string $key, ?string $pattern = null, mixed $get = null, int $offset = -1, int $count = -1, ?string $store = null)
 * @method RedisArray|int|false srem(string $key, mixed $value, mixed ...$other_values)
 * @method bool ssubscribe(array $channels, callable $cb)
 * @method RedisArray|int|false strlen(string $key)
 * @method bool subscribe(array $channels, callable $cb)
 * @method RedisArray|array|bool sunsubscribe(array $channels)
 * @method RedisArray|bool swapdb(int $src, int $dst)
 * @method RedisArray|array time()
 * @method RedisArray|int|false ttl(string $key)
 * @method RedisArray|int|false type(string $key)
 * @method RedisArray|array|bool unsubscribe(array $channels)
 * @method RedisArray|bool watch(array|string $key, string ...$other_keys)
 * @method int|false wait(int $numreplicas, int $timeout)
 * @method int|false xack(string $key, string $group, array $ids)
 * @method RedisArray|string|false xadd(string $key, string $id, array $values, int $maxlen = 0, bool $approx = false, bool $nomkstream = false)
 * @method RedisArray|bool|array xautoclaim(string $key, string $group, string $consumer, int $min_idle, string $start, int $count = -1, bool $justid = false)
 * @method RedisArray|array|bool xclaim(string $key, string $group, string $consumer, int $min_idle, array $ids, array $options)
 * @method RedisArray|int|false xdel(string $key, array $ids)
 * @method mixed xinfo(string $operation, ?string $arg1 = null, ?string $arg2 = null, int $count = -1)
 * @method RedisArray|int|false xlen(string $key)
 * @method RedisArray|array|false xpending(string $key, string $group, ?string $start = null, ?string $end = null, int $count = -1, ?string $consumer = null)
 * @method RedisArray|array|bool xrange(string $key, string $start, string $end, int $count = -1)
 * @method RedisArray|array|bool xread(array $streams, int $count = -1, int $block = -1)
 * @method RedisArray|array|bool xreadgroup(string $group, string $consumer, array $streams, int $count = 1, int $block = 1)
 * @method RedisArray|array|bool xrevrange(string $key, string $end, string $start, int $count = -1)
 * @method RedisArray|int|false vadd(string $key, array $values, mixed $element, array|null $options = null)
 * @method RedisArray|array|false vsim(string $key, mixed $member, array|null $options = null)
 * @method RedisArray|int|false vcard(string $key)
 * @method RedisArray|int|false vdim(string $key)
 * @method RedisArray|array|false vinfo(string $key)
 * @method RedisArray|bool vismember(string $key, mixed $member)
 * @method RedisArray|array|false vemb(string $key, mixed $member, bool $raw = false)
 * @method RedisArray|array|string|false vrandmember(string $key, int $count = 0)
 * @method RedisArray|array|false vrange(string $key, string $min, string $max, int $count = -1)
 * @method RedisArray|int|false vrem(string $key, mixed $member)
 * @method RedisArray|int|false vsetattr(string $key, mixed $member, array|string $attributes)
 * @method RedisArray|array|string|false vgetattr(string $key, mixed $member, bool $decode = true)
 * @method RedisArray|array|false vlinks(string $key, mixed $member, bool $withscores = false)
 * @method RedisArray|int|false xtrim(string $key, string $threshold, bool $approx = false, bool $minid = false, int $limit = -1)
 * @method RedisArray|int|float|false zAdd(string $key, array|float $score_or_options, mixed ...$more_scores_and_mems)
 * @method RedisArray|int|false zCard(string $key)
 * @method RedisArray|int|false zCount(string $key, int|string $start, int|string $end)
 * @method RedisArray|float|false zIncrBy(string $key, float $value, mixed $member)
 * @method RedisArray|int|false zLexCount(string $key, string $min, string $max)
 * @method RedisArray|array|false zMscore(string $key, mixed $member, mixed ...$other_members)
 * @method RedisArray|array|false zPopMax(string $key, ?int $count = null)
 * @method RedisArray|array|false zPopMin(string $key, ?int $count = null)
 * @method RedisArray|array|false zRange(string $key, string|int $start, string|int $end, array|bool|null $options = null)
 * @method RedisArray|array|false zRangeByLex(string $key, string $min, string $max, int $offset = -1, int $count = -1)
 * @method RedisArray|array|false zRangeByScore(string $key, string $start, string $end, array $options = [])
 * @method RedisArray|string|array zRandMember(string $key, ?array $options = null)
 * @method RedisArray|int|false zRank(string $key, mixed $member)
 * @method RedisArray|int|false zRem(mixed $key, mixed $member, mixed ...$other_members)
 * @method RedisArray|int|false zRemRangeByLex(string $key, string $min, string $max)
 * @method RedisArray|int|false zRemRangeByRank(string $key, int $start, int $end)
 * @method RedisArray|int|false zRemRangeByScore(string $key, string $start, string $end)
 * @method RedisArray|array|false zRevRange(string $key, int $start, int $end, mixed $scores = null)
 * @method RedisArray|array|false zRevRangeByLex(string $key, string $max, string $min, int $offset = -1, int $count = -1)
 * @method RedisArray|array|false zRevRangeByScore(string $key, string $max, string $min, array|bool $options = [])
 * @method RedisArray|int|false zRevRank(string $key, mixed $member)
 * @method RedisArray|float|false zScore(string $key, mixed $member)
 * @method RedisArray|array|false zdiff(array $keys, ?array $options = null)
 * @method RedisArray|int|false zdiffstore(string $dst, array $keys)
 * @method RedisArray|array|false zinter(array $keys, ?array $weights = null, ?array $options = null)
 * @method RedisArray|int|false zintercard(array $keys, int $limit = -1)
 * @method RedisArray|int|false zinterstore(string $dst, array $keys, ?array $weights = null, ?string $aggregate = null)
 * @method RedisArray|array|false zunion(array $keys, ?array $weights = null, ?array $options = null)
 * @method RedisArray|int|false zunionstore(string $dst, array $keys, ?array $weights = null, ?string $aggregate = null)
 */
class RedisArray {

    public function __call(string $function_name, array $arguments): mixed;

    public function __construct(string|array $name_or_hosts, ?array $options = null);

    public function _continuum(): bool|array;

    public function _distributor(): bool|callable;

    public function _function(): bool|callable;

    public function _hosts(): bool|array;

    public function _instance(string $host): bool|null|Redis;

    public function _rehash(?callable $fn = null): bool|null;

    public function _target(string $key): bool|string|null;

    public function bgsave(): array;

    public function del(string|array $key, string ...$otherkeys): bool|int;

    public function discard(): bool|null;

    public function exec(): bool|null|array;

    public function flushall(): bool|array;

    public function flushdb(): bool|array;

    public function getOption(int $opt): bool|array;

    public function hscan(string $key, null|int|string &$iterator, ?string $pattern = null, int $count = 0): bool|array;

    public function info(): bool|array;

    public function keys(string $pattern): bool|array;

    public function mget(array $keys): bool|array;

    public function mset(array $pairs): bool;

    public function multi(string $host, ?int $mode = null): bool|RedisArray;

    public function ping(): bool|array;

    public function save(): bool|array;

    public function scan(null|int|string &$iterator, string $node, ?string $pattern = null, int $count = 0): bool|array;

    public function select(int $index): bool|array;

    public function setOption(int $opt, string $value): bool|array;

    public function sscan(string $key, null|int|string &$iterator, ?string $pattern = null, int $count = 0): bool|array;

    public function unlink(string|array $key, string ...$otherkeys): bool|int;

    public function unwatch(): bool|null;

    public function zscan(string $key, null|int|string &$iterator, ?string $pattern = null, int $count = 0): bool|array;
}
