# Changelog

All changes to phpredis will be documented in this file.

We're basing this format on [Keep a
Changelog](https://keepachangelog.com/en/1.0.0/), and PhpRedis adhears to
[Semantic Versioning](https://semver.org/spec/v2.0.0.html).


## [5.0.0] - 2019-07-02 (__GITHUB__, __PECL__)

This release contains important improvements and breaking changes.  The most
interesting are: drop PHP5 support, RedisCluster slots caching, JSON and msgpack
serializers, soft deprecation of non-Redis commands.

### Added
- Adds OPT_REPLY_LITERAL for rawCommand and EVAL [5cb30fb2] (Michael Grunder)
- JSON serializer \[98bd2886, 96c57139, 235a27\] (Pavlo Yatsukhnenko, Michael
  Grunder)
- msgpack serializer \[d5b8f833, 545250f3, 52bae8ab\] (@bgort, Pavlo Yatsukhnenko,
  Michael Grunder)
- Add support for STREAM to the type command [d7450b2f, 068ce978, 8a45d18c]
  (Michael Grunder, Pavlo Yatsukhnenko)
- Add Cluster slots caching [9f0d7bc0, ea081e05] (Michael Grunder)

### Changed

- Add server address to exception message [e8fb49be, 34d6403d] 
  (Pavlo Yatsukhnenko)
- Allow to specify server address as `schema://host` [418428fa] 
  (Pavlo Yatsukhnenko).
- Update Fedora installation instructions [90aa067c] (@remicollet)
- Enable connection pooling by default [8206b147] (Pavlo Yatsukhnenko)
- Allow PING to take an optional argument [6e494170] (Michael Grunder)
- Allow ZRANGE to be called either with `true` or `['withscores' => true]`
  [19f3efcf] (Michael Grunder)
- Documentation improvements (@alexander-schranz, @cookieguru, Pavlo
  Yatsukhnenko, Michael Grunder)

### Deprecated

- Soft deprecate methods that aren't actually Redis commands [a81b4f2d,
  95c8aab9, 235a27] (@michael-grunder, @yatsukhnenko)
- Remove HAVE_SPL define [55c5586c] (@petk)

### Removed

- Drop PHP5 support [f9928642, 46a50c12, 4601887d, 6ebb36ce, fdbe9d29] (Michael
  Grunder)

### Fixed

- Reworked PHP msgpack >= 2.0.3 version requirement. [6973478..a537df8]
  (@michael-grunder).
- Enable pooling for cluster slave nodes [17600dd1] (Michael Grunder)
- xInfo response format [4852a510, ac9dca0a] (Pavlo Yatsukhnenko)
- Make the XREADGROUP optional COUNT and BLOCK arguments nullable [0c17bd27]
  (Michael Grunder)
- Allow persistent_id to be passed as NULL with strict_types enabled [60223762]
  (Michael Grunder)
- Fix TypeError when using built-in constants in `setOption` [4c7643ee]
  (@JoyceBabu)
- Handle references in MGET [60d8b679] (Michael Grunder)

---

## [4.3.0] - 2019-03-13 (__GITHUB__, __PECL__)

This is probably the latest release with PHP 5 suport!!!

### Added

- RedisArray auth [b5549cff, 339cfa2b, 6b411aa8] (Pavlo Yatsukhnenko)
- Add ZPOPMAX and ZPOPMIN support [46f03561, f89e941a, 2ec7d91a] (@mbezhanov,
  Michael Grunder)
- Implement GEORADIUS_RO and GEORADIUSBYMEMBER_RO [22d81a94] (Michael Grunder)
- RedisCluster auth [c5994f2a] (Pavlo Yatsukhnenko)
- Different key hashing algorithms from hash extension [850027ff] (Pavlo
  Yatsukhnenko)

### Changed

- Proper persistent connections pooling implementation [a3703820, c76e00fb,
  0433dc03, c75b3b93] (Pavlo Yatsukhnenko)
- Use zend_string for storing key hashing algorithm [8cd165df, 64e6a57f] (Pavlo
  Yatsukhnenko)
- Add callback parameter to subscribe/psubscribe arginfo [0653ff31] (Pavlo
  Yatsukhnenko)
- Xgroup updates [15995c06] (Michael Grunder)
- Use zend_string for pipeline_cmd [e98f5116] (Pavlo Yatsukhnenko)
- Implement consistent hashing algorithm for RedisArray [bb32e6f3, 71922bf1]
  (Pavlo Yatsukhnenko)
- Use zend_string for storing RedisArray hosts [602740d3, 3e7e1c83] (Pavlo
  Yatsukhnenko)
- Refactor redis_sock_read_bulk_reply [bc4dbc4b] (Pavlo Yatsukhnenko)

### Fixed

- Don't check the number affected keys in PS_UPDATE_TIMESTAMP_FUNC [b00060ce]
  (Pavlo Yatsukhnenko)
- Cancel pipeline mode without executing commands [789256d7] (Pavlo
  Yatsukhnenko)
- Breaking the lock acquire loop in case of network problems [61889cd7]
  (@SkydiveMarius)
- Update lzf_compress to be compatible with PECL lzf extension [b27fd430]
  (@jrchamp)
- Fix RedisCluster keys memory leak [3b56b7db] (Michael Grunder)
- Directly use return_value in RedisCluster::keys method [ad10a49e] (Pavlo
  Yatsukhnenko)
- Fix segfault in Redis Cluster with inconsistent configuration [72749916,
  6e455e2e] (Pavlo Yatsukhnenko)
- Masters info leakfix [91bd7426] (Michael Grunder)
- Remove unused parameter lazy_connect from redis_sock_create [c0793e8b] (Pavlo
  Yatsukhnenko)
- Remove useless `ZEND_ACC_[C|D]TOR`. [bc9b5597] (@twosee)
- Documentation improvements (@fanjiapeng, @alexander-schranz, @hmc, 
  Pavlo Yatsukhnenko, Michael Grunder)

---

## [4.2.0] - 2018-11-08 (__GITHUB__, __PECL__)

The main feature of this release is new Streams API implemented by 
Michael Grunder.

### Added

- Streams API [2c9e0572, 0b97ec37] (Michael Grunder)
- Display ini entries in output of phpinfo [908ac4b3] (Pavlo Yatsukhnenko)
- Persistant connections can be closed via close method + change reconnection 
  logic [1d997873] (Pavlo Yatsukhnenko)

### Changed

- Optimize close method [2a1ef961] (fanjiapeng)
- Use a ZSET insted of SET for EVAL tests [2e412373] (Michael Grunder)
- Modify session testing logic [bfd27471] (Michael Grunder)
- Documentation improvements (@mg, @elcheco, @lucascourot, @nolimitdev, 
  Michael Grunder)

### Fixed

- Prevent potential infinite loop for sessions [4e2de158] (Pavlo Yatsukhnenko)
- Fix coverty warnings [6f7ddd27] (Pavlo Yatsukhnenko)
- Fix session memory leaks [071a1d54, 92f14b14] (Pavlo Yatsukhnenko, 
  Michael Grunder)
- Fix XCLAIM on 32-bit installs [18dc2aac] (Michael Grunder)
- Build warning fixes [b5093910, 51027044, 8b0f28cd] (Pavlo Yatsukhnenko, 
  Remi Collet, twosee)
- Fix incorrect arginfo for `Redis::sRem` and `Redis::multi` [25b043ce] 
  (Pavlo Yatsukhnenko)
- Treat a -1 response from cluster_check_response as a timeout. [27df9220, 
  07ef7f4e, d1172426] (Michael Grunder).
- Missing space between command and args [0af2a7fe] (@remicollet)
- Reset the socket after a timeout to make sure no wrong data is received 
  [cd6ebc6d] (@marcdejonge)
- Allow '-' and '+' arguments and add tests for zLexCount and zRemRangeByLex 
  [d4a08697] (Michael Grunder)
- Fix printf format warnings [dcde9331] (Pavlo Yatsukhnenko)
- Session module is required [58bd8cc8] (@remicollet)
- Set default values for ini entries [e206ce9c] (Pavlo Yatsukhnenko)

---

## [4.0.0] - 2018-03-07 (__GITHUB__, __PECL__)

*WARNING:* THIS RELEASE CONTAINS BREAKING API CHANGES!

### Added

- Add proper ARGINFO for all methods. (Pavlo Yatsukhnenko, Michael Grunder)
- Let EXISTS take multiple keys [cccc39] (Michael Grunder)
- Implement SWAPDB and UNLINK commands [84f1f28b, 9e65c429] (Michael Grunder)
- Add LZF compression (experimental) [e2c51251, 8cb2d5bd, 8657557] (Pavlo Yatsukhnenko)
- Allow mixing MULTI and PIPELINE modes (experimental) [5874b0] (Pavlo Yatsukhnenko)

### Changed

- Use zend_string as returning value for ra_extract_key and ra_call_extractor 
  [9cd05911] (Pavlo Yatsukhnenko)
- Return real connection error as exception [5b9c0c60] (Pavlo Yatsukhnenko, 
  Michael Grunder)
- Use zend_string for storing auth and prefix members [4b8336f7] 
  (Pavlo Yatsukhnenko)
- Add tcp_keepalive option to redis sock [68c58513, 5101172a, 010336d5, 
  51e48729] (@git-hulk, Michael Grunder)
- More robust GEORADIUS COUNT validation [f7edee5d] (Michael Grunder)
- Allow to use empty string as persistant_id [ec4fd1bd] (Pavlo Yatsukhnenko)
- Documentation improvements (Michael Grunder, @TomA-R)

### Fixed

- Disallow using empty string as session name. [485db46f] (Pavlo Yatsukhnenko)
- The element of z_seeds may be a reference on php7 [367bc6aa, 1e63717a] 
  (@janic716)
- Avoid connection in helper methods [91e9cfe1] (Pavlo Yatsukhnenko)
- Don't use convert_to_string in redis_hmget_cmd [99335d6] (Pavlo Yatsukhnenko)
- PHP >=7.3.0 uses zend_string to store `php_url` elements [b566fb44] (@fmk)

---

## [3.1.5] - 2017-09-27 (__GITHUB__, __PECL__)

This is interim release which contains only bug fixes.

### Fixed

- Fix segfault when extending Redis class in PHP 5 [d23eff] (Pavlo Yatsukhnenko)
- Fix RedisCluster constructor with PHP 7 strict scalar type [5c21d7] 
  (Pavlo Yatsukhnenko)
- Allow to use empty string as persistant_id [344de5] (Pavlo Yatsukhnenko)
- Fix cluster_init_seeds. [db1347] (@adlagares)
- Fix z_seeds may be a reference [42581a] (@janic716)
- PHP >=7.3 uses zend_string for php_url elements [b566fb] (@fmk)

---

## [3.1.4] - 2017-09-27 (__GITHUB__, __PECL__)

The primary new feature phpredis 3.1.4 is the ability to send MULTI .. EXEC 
blocks in pipeline mode.  There are also many bugfixes and minor improvements 
to the api, listed below.

### Added

- Allow mixing MULTI and PIPELINE modes (experimental)!  [5874b0] 
  (Pavlo Yatsukhnenko)
- Added integration for coverty static analysis and fixed several warnings
  [faac8b0, eff7398, 4766c25, 0438ab4, 1e0b065, 733732a, 26eeda5, 735025, 
   42f1c9, af71d4] (Pavlo Yatsukhnenko).
- Added arginfo introspection structures [81a0303, d5609fc, e5660be, 3c60e1f, 
  50dcb15, 6c2c6fa, 212e323, e23be2c, 682593d, f8de702, 4ef3acd, f116be9, 
  5c111dd, 9caa029, 0d69650, 6859828, 024e593, 3643ab6, f576fab, 122d41f, 
  a09d0e6] (Tyson Andre, Pavlo Yatsukhnenko).
- Added a github issue template [61aba9] (Pavlo Yatsukhnenko)

### Changed

- Refactor redis_send_discard [ea15ce] (Pavlo Yatsukhnenko)
- Improve redis array rehash [577a91] (Pavlo Yatsukhnenko)
- Change redis array pure_cmds from zval to hashtable [a56ed7] (Pavlo Yatsukhnenko)
- Use zend_string rather than char for various context fields (err, prefix, etc) 
  [2bf7b2] (Pavlo Yatsukhnenko)
- Updated runtime exception handling [8dcaa4, 7c1407] (Pavlo Yatsukhnenko)

### Fixed
- Fixed link to redis cluster documentation [3b0b06] (Pavlo Yatsukhnenko)
- Removed duplicate HGET in redis array hash table, formatting [d0b9c5]
  (Pavlo Yatsukhnenko).
- Treat NULL bulk as success for session read [659450] (Pavlo Yatsukhnenko)
- Fix valgrind warnings [471ce07, 1ab89e1, b624a8b] (Pavlo Yatsukhnenko)
- Fix php5/php7 compatibility layer [1ab89e, 4e3225] (Pavlo Yatsukhnenko)
- Fix typo in README.markdown [e47e44] (Mark Shehata)
- Initialize gc member of zend_string [37f569] (Pavlo Yatsukhnenko).
- Don't try to set TCP_NODELAY on a unix socket and don't warn on multiple
  calls to pipeline [d11798, 77aeba] (Michael Grunder)
- Various other library fixes [142b51, 4452f6, e672f4, 658ee3, c9df77, 4a0a46] 
  (Pavlo Yatsukhnenko)

### Removed
- Remove unused PHP_RINIT and PHP_RSHUTDOWN functions [c760bf] (Pavlo Yatsukhnenko)

---

## [3.1.3] - 2017-07-15 (__GITHUB__, __PECL__)

This release contains two big improvements:

1. Adding a new printf like command construction function with additionaly 
   format specifiers specific to phpredis.
2. Implementation of custom objects for Redis and RedisArray wich eliminates 
   double hash lookup.

Also many small improvements and bug fixes were made.

### Added

- Add hStrLen command [c52077, fb88e1] (Pavlo Yatsukhnenko)
- A printf like method to construct a Redis RESP command [a4a0ed, d75081, 
  bdd287, 0eaeae, b3d00d] (Michael Grunder)
- Use custom objects instead of zend_list for storing Redis/RedisArray [a765f8, 
  8fa85a] (Pavlo Yatsukhnenko)
- Add configureoption tag to package.xml [750963] (Pavlo Yatsukhnenko)

### Changed

- Add optional COUNT argument to sPop [d2e203] (Michael Grunder)
- Allow sInterStore to take one arg [26aec4, 4cd06b] (Michael Grunder)
- Allow MIGRATE to accept multiple keys [9aa3db] (Michael Grunder)
- Use crc32 table from PHP distro [f81694] (Pavlo Yatsukhnenko)
- Throw exception for all non recoverable errors [e37239] (Pavlo Yatsukhnenko)
- Increase read buffers size [520e06] (Pavlo Yatsukhnenko)
- Better documentation [f0c25a, c5991f, 9ec9ae] (Michael Grunder)
- Better TravisCI integration [e37c08] (Pavlo Yatsukhnenko)

### Fixed

- Make sure redisCluster members are all initialized on (re)creation [162d88] 
- (Michael Grunder).
- Fix Null Bulk String response parsing in cluster library [058753] 
- (Alberto Fernández)
- Allow using numeric string in zInter command [ba0070] (Pavlo Yatsukhnenko)
- Use ZVAL_DEREF macros for dereference input variables [ad4596] 
- (Pavlo Yatsukhnenko)
- Fix read_timeout [18149e, b56dc4] (Pavlo Yatsukhnenko)
- Fix zval_get_string impl for PHP5 [4e56ba] (Pavlo Yatsukhnenko)
- Fix Redis/RedisArray segfaults [be5c1f, 635c3a, 1f8dde, 43e1e0] 
- (Pavlo Yatsukhnenko)
- Fix memory leak and potential segfault [aa6ff7, 88efaa] (Michael Grunder)
- Assume "NULL bulk" reply as success (empty session data) [4a81e1] 
  (Pavlo Yatsukhnenko)
- Refactoring (Pavlo Yatsukhnenko, Michael Grunder)

---

## [3.1.2] - 2017-03-16 (__GITHUB__, __PECL__)

### Changes

- Re allow single array for sInterStore [6ef0c2, d01966] (Michael Grunder)
- Better TravisCI integration [4fd2f6] (Pavlo Yatsukhnenko)

### Fixes

- RedisArray segfault fix [564ce3] (Pavlo Yatsukhnenko)
- Small memory leak fix [645888b] (Mike Grunder)
- Segfault fix when recreating RedisCluster objects [abf7d4] (Michael Grunder)
- Fix for RedisCluster bulk response parsing [4121c4] (Alberto Fernández)

---

## [3.1.1] - 2017-02-01 (__GITHUB__, __PECL__)

This release contains mostly fixes for issues introduced when merging
the php 5 and 7 codebase into a single branch.

- Additional test updates for 32 bit systems (@remicollet)
- ARM rounding issue in tests (@remicollet)
- Use new zend_list_close instead of zend_list_delete when reconnecting.
- Refactoring of redis_boolean_response_impl and redis_sock_write (@yatsukhnenko)
- Fixed a segfault in igbinary serialization (@yatsukhnenko)
- Restore 2.2.8/3.0.0 functionality to distinguish between an error
  and simply empty session data. (@remicollet)
- Fix double to string conversion function (@yatsukhnenko)
- Use PHP_FE_END definition when available (@remicollet)
- Fixed various 'static function declared but not used' warnings
- Fixes to various calls which were typecasting pointers to the
- wrong size. (@remicollet)
-
- Added php session unit test (@yatsukhnenko)
- Added explicit module dependancy for igbinary (@remicollet)
- Added phpinfo serialization information (@remicollet)

---

## [3.1.0] - 2016-12-14 (__GITHUB__, __PECL__)

In this version of phpredis codebase was unified to work with all versions of php \o/
Also many bug fixes and some improvements has been made.

### Added

- Support the client to Redis Cluster just having one master (andyli) [892e5646]
- Allow both long and strings that are longs for zrangebyscore offset/limit 
  (Michael Grunder) [bdcdd2aa]
- Process NX|XX, CH and INCR options in zAdd command [71c9f7c8] 
  (Pavlo Yatsukhnenko) 

### Fixes

- Fix incrby/decrby for large integers (Michael Grunder) [3a12758a]
- Use static declarations for spl_ce_RuntimeException decl [a9857d69] 
  (Jeremy Mikola) 
- Fixed method call problem causes session handler to display two times 
  [24f86c49] (ZiHang Gao).
- PSETEX method returns '+OK' on success, not true [afcd8445] (sitri@ndxbn) 
- Fix integer overflow for long (>32bit) increments in hIncrBy [58e1d799]
  (@iyesin) 
- Move zend_object handler to the end (Michael Grunder) [34107966]
- Using setOption on redis array causes immediate connection [f1a85b38]
  (Pavlo Yatsukhnenko)

---

## [2.2.8] - 2016-06-02 (__GITHUB__, __PECL__)

The main improvement in this version of phpredis is support for Redis
Cluster.  This version of phpredis is intended for versions of php older
than 7.

### Added

- Added randomization to our seed nodes to balance which instance is used
  to map the keyspace [32eb1c5f] (Vitaliy Stepanyuk)
- Added support for IPv6 addresses

### Fixed

- PHP liveness checking workaround (Shafreeck Sea) [c18d58b9]
- Various documentation and code formatting and style fixes (ares333,
  anpili, Bryan Nelson, linfangrong, Romero Malaquias, Viktor Szépe)
- Fix scan reply processing to use long instead of int to avoid overflow
  mixiaojiong).
- Fix potential segfault in Redis Cluster session storage [cc15aae] 
  (Sergei Lomakov).
- Fixed memory leak in discard function [17b1f427]
- Sanity check for igbinary unserialization (Maurus Cuelenaere) 
  [3266b222, 528297a] (Marus Cuelenaere).
- Fix segfault occuring from unclosed socket connection for Redis Cluster
  [04196aee] (CatKang)
- Case insensitive zRangeByScore options
- Fixed dreaded size_t vs long long compiler warning

---

## [2.2.7] - 2015-03-03 (__GITHUB__, __PECL__)

### Added

- Implemented PFADD, PFMERGE, and PFCOUNT command handling
- Implemented ZRANGEBYLEX command (holding off on ZREVRANGEBYLEX
  as that won't be out until 3.0)
- Implemented getMode() so clients can detect whether we're in
  ATOMIC/MULTI/PIPELINE mode.
- Implemented rawCommand() so clients can send arbitrary things to
  the redis server
- Implemented DEBUG OBJECT (@michael-grunder, @isage)
- Added/abide by connect timeout for RedisArray

### Fixes

- Select to the last selected DB when phpredis reconnects
- Fix a possible invalid free in \_serialize()
- Added SAVE and BGSAVE to "distributable" commands for RedisArray
- Fixed invalid "argc" calculation in HLL commands (@welting)
- Allow clients to break out of the subscribe loop and return context.
- Fixes a memory leak in SCAN when OPT_SCAN_RETRY id.
- Fix possible segfault when igbinary is enabled (@remicollet).
- Add a couple of cases where we throw on an error (LOADING/NOAUTH/MASTERDOWN)
- Fix several issues with serialization NARY
- Fix missing TSRMLS_CC and a TSRMLS_DC/TSRMLS_CC typo (@itcom)

---

## [2.2.5] - 2014-03-15 (__GITHUB__, __PECL__)

### Added

- Support for the BITPOS command
- Connection timeout option for RedisArray (@MikeToString)
- A \_serialize method, to complement our existing \_unserialize method
- Support for the PUBSUB command
- Support for SCAN, SSCAN, HSCAN, and ZSCAN
- Support for the WAIT command

### Fixes

- Handle the COPY and REPLACE arguments for the MIGRATE command
- Fix syntax error in documentation for the SET command (@mithunsatheesh)
- Fix Homebrew documentation instructions (@mathias)

---

## [2.2.4] - 2013-09-01 (__GITHUB__, __PECL__)

### Added

- Randomized reconnect delay for RedisArray @mobli
- Lazy connections to RedisArray servers @mobli
- Allow LONG and STRING keys in MGET/MSET
- Extended SET options for Redis &gt;= 2.6.12
- Persistent connections and UNIX SOCKET support for RedisArray
- Allow aggregates for ZUNION/ZINTER without weights @mheijkoop
- Support for SLOWLOG command

### Changed
- Reworked MGET algorithm to run in linear time regardless of key count.
- Reworked ZINTERSTORE/ZUNIONSTORE algorithm to run in linear time

### Fixes

- C99 Compliance (or rather lack thereof) fix @mobli
- Added ZEND_ACC_CTOR and ZEND_ACC_DTOR @euskadi31
- Stop throwing and clearing an exception on connect failure @matmoi
- Fix a false positive unit test failure having to do with TTL returns
