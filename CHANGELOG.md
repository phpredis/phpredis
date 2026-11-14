# Changelog

All changes to phpredis will be documented in this file.

We're basing this format on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and PhpRedis adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

# [Unreleased]

Changes since 6.3.0 add new Redis and Valkey commands, explicit reconnection APIs, an optional phpser serializer, and persistent connections for ZTS builds. They also improve protocol validation, cluster reliability, session handling, and performance.

### Sponsors :sparkling_heart:

- [A-VISION Advisering en Begeleiding B.V.](https://github.com/A-VISION-BV)
- [Avtandil Kikabidze](https://github.com/akalongman)
- [Boring Observability](https://github.com/boring-o11y)
- [Geoffrey Hoffman](https://github.com/phpguru)
- [Open LMS](https://github.com/open-lms-open-source)
- [Salvatore Sanfilippo](https://github.com/antirez)
- [Scott Helme](https://github.com/ScottHelme)
- [Ty](https://github.com/karock)

## Fixed

- Preserve SET conditional flags and comparison options, restore expiry validation in SET and MSETEX, and reject KEEPTTL combined with an explicit expiry
  [4198d14b](https://github.com/phpredis/phpredis/commit/4198d14b360c3d3eb62b0bb8042fb55a0b5d36c8)
  ([michael-grunder](https://github.com/michael-grunder))
- Restore SET option value parsing under associative GET keys, preserving NX, XX, and KEEPTTL flags
  [48dbc1c8](https://github.com/phpredis/phpredis/commit/48dbc1c8f788b5d46493c3ee4887ea58d4549151)
  ([michael-grunder](https://github.com/michael-grunder))
- Fix truncated subscriber counts in `PUBSUB NUMSUB` and `PUBSUB SHARDNUMSUB` replies
  [82f79a43](https://github.com/phpredis/phpredis/commit/82f79a43d31b276062fa2347b23ec3e27c0b2482)
  ([michael-grunder](https://github.com/michael-grunder))
- Correct the `RedisCluster::msetex` return type to allow `RedisCluster` in transactions
  [9b4f0c28](https://github.com/phpredis/phpredis/commit/9b4f0c28abe0c9ea304d6667aafbe78d85e3df61)
  ([michael-grunder](https://github.com/michael-grunder))
- Preserve pipeline reply callbacks when reconnecting
  [d54431d1](https://github.com/phpredis/phpredis/commit/d54431d11dc547bb9d56a08c09ec6f9d71abfd1c)
  ([rechl](https://github.com/rechl))
- Fix persistent connection pool liveness checks and reconnect handling
  [146ec813](https://github.com/phpredis/phpredis/commit/146ec813ea7ca85c9e3a7cff2caf977096e16acf)
  ([michael-grunder](https://github.com/michael-grunder))
- Re-send `READONLY` after reconnecting to a cluster replica
  [3606328d](https://github.com/phpredis/phpredis/commit/3606328d96d4450abd1b0a0dc5d2f05454ca53fb)
  ([Etienne Zannelli](https://github.com/e-zannelli))
- Clear the cluster slot cache when nodes are unreachable
  [52e90650](https://github.com/phpredis/phpredis/commit/52e90650b10893e294ec0bceba697ab8a23c469a)
  ([Victor Kislov](https://github.com/primis-victork))
- Handle `RedisCluster` directed commands on uncovered slots
  [1351c824](https://github.com/phpredis/phpredis/commit/1351c824c679dc7b711d441beba091b64bf08565)
  ([Ebubekir Yiğit](https://github.com/ebubekiryigit))
- Fix `RedisCluster` directed commands in `MULTI` mode and reject `UNWATCH` during a transaction
  [e13d5d0c](https://github.com/phpredis/phpredis/commit/e13d5d0c61c7c128788682c5503fc9118e6a87e7),
  [a1d93cab](https://github.com/phpredis/phpredis/commit/a1d93cabb18eef6200095bc551067fb63356c36d)
  ([michael-grunder](https://github.com/michael-grunder))
- Fix cluster transaction result bookkeeping across slots belonging to the same master
  [a283471e](https://github.com/phpredis/phpredis/commit/a283471eb5aaf2c26781dc6f4338f0a56ccd95c4)
  ([michael-grunder](https://github.com/michael-grunder))
- Fix use-after-free defects and result ownership in cluster multi-key commands
  [2ef32c52](https://github.com/phpredis/phpredis/commit/2ef32c5244f924fad4d7527e36f808e7b364f90f),
  [ff9dede9](https://github.com/phpredis/phpredis/commit/ff9dede940be3a05052c02b15cc50c1a43d9360b)
  ([michael-grunder](https://github.com/michael-grunder))
- Fix use-after-free and double-free paths when parsing malformed cluster replies
  [b29f5c85](https://github.com/phpredis/phpredis/commit/b29f5c85b29378bf101fda26ab3afe182e71f599),
  [a14fdeca](https://github.com/phpredis/phpredis/commit/a14fdeca390c0f8616712d9291f2bf5a8c35717d)
  ([michael-grunder](https://github.com/michael-grunder))
- Validate pub/sub reply element types before using them as strings
  [cd04e21d](https://github.com/phpredis/phpredis/commit/cd04e21df94cf192bd1dd2ed1c465f8fdabe799d),
  [e2a4f90d](https://github.com/phpredis/phpredis/commit/e2a4f90df7e6fc05616e933475326a4bccbeb7f8)
  ([Raphael Paez](mailto:raphael.paez@softplan.com.br))
  ([michael-grunder](https://github.com/michael-grunder))
- Validate scan cursor types, configured node lists, and RedisArray reply values
  [57ddfefe](https://github.com/phpredis/phpredis/commit/57ddfefe67abd3504587e11d05978b5f29d97966)
  ([michael-grunder](https://github.com/michael-grunder))
- Harden RESP length parsing, detect EOF correctly, and guard against bulk length overflow
  [5c6e2d2b](https://github.com/phpredis/phpredis/commit/5c6e2d2b3cd16ad29effb7036a9984339fc5bd0c),
  [c67d3e49](https://github.com/phpredis/phpredis/commit/c67d3e493fad38e2e5751382764071636e393371),
  [7b2fdc6d](https://github.com/phpredis/phpredis/commit/7b2fdc6de13df0007c441c5d26e44567adc0f810),
  [1f02e67d](https://github.com/phpredis/phpredis/commit/1f02e67d83b0916536a6c9d57d6dfd63e1a66327)
  ([Ilia Alshanetsky](https://github.com/iliaal))
  ([michael-grunder](https://github.com/michael-grunder))
- Reject malformed protocol replies and stop reading after failures instead of looping indefinitely
  [ea3ea564](https://github.com/phpredis/phpredis/commit/ea3ea564d0a51c94a1ea0b99d3af41553f9bbc2c),
  [2298a200](https://github.com/phpredis/phpredis/commit/2298a200d3102f364678165d929e1305bc7a9f7a),
  [80093d53](https://github.com/phpredis/phpredis/commit/80093d53b154ebc83f013a97edec48f12cad0e00),
  [4bdcf155](https://github.com/phpredis/phpredis/commit/4bdcf1552bbf41fd349eeebd49260ed8e71dc4af),
  [a6b7befd](https://github.com/phpredis/phpredis/commit/a6b7befd6b196ea09778d9b4a48458480d77d6b8),
  [753a2d6d](https://github.com/phpredis/phpredis/commit/753a2d6d6752edf741ad97e885e23320c1be0e96),
  [e5d2f8d6](https://github.com/phpredis/phpredis/commit/e5d2f8d625bd464134af313ab328d6a4f13b680c)
  ([michael-grunder](https://github.com/michael-grunder))
- Validate CRLF terminators and handle short reads in standalone and cluster replies
  [a0ded702](https://github.com/phpredis/phpredis/commit/a0ded7027a1222a89acb35c0b7743608cea7e07d),
  [6e136252](https://github.com/phpredis/phpredis/commit/6e1362520a0d8d6ce5a7d5212c16c8fd22d8fcc3),
  [00c0f963](https://github.com/phpredis/phpredis/commit/00c0f963945803a991a7c865255d7192d0507df1)
  ([EJ-Research](https://github.com/EJ-Research))
  ([michael-grunder](https://github.com/michael-grunder))
- Limit nested reply depth, cap array preallocation from server-supplied counts, and fix malformed-reply leaks
  [690a1098](https://github.com/phpredis/phpredis/commit/690a10987c7e23cc52c76a0b54c587b50ca03b05),
  [01791f23](https://github.com/phpredis/phpredis/commit/01791f234c3fe00d40b48c5642b2b502e571a100),
  [34c3fd9f](https://github.com/phpredis/phpredis/commit/34c3fd9fce0ed335bfcb6bf534659fecc9a020fe)
  ([michael-grunder](https://github.com/michael-grunder))
- Harden `CLUSTER SLOTS` and redirection parsing, including oversized hostnames
  [fbf5affa](https://github.com/phpredis/phpredis/commit/fbf5affa14a7193fe6d6519339285361b2647274),
  [bb9e8769](https://github.com/phpredis/phpredis/commit/bb9e87695b9535d92a3fada820f166f46e7752a6),
  [8b1280f3](https://github.com/phpredis/phpredis/commit/8b1280f3cd440dc741482c355d35eace4787de49)
  ([Ilia Alshanetsky](https://github.com/iliaal))
  ([michael-grunder](https://github.com/michael-grunder))
- Validate array replies for cluster scans and `XCLAIM`
  [f98c0837](https://github.com/phpredis/phpredis/commit/f98c0837e571303c877779fe36c0874d0d6817e9),
  [c97d754f](https://github.com/phpredis/phpredis/commit/c97d754f27cd516eb7596ef0629322b62182c935)
  ([michael-grunder](https://github.com/michael-grunder))
- Discard connections after failed or incomplete writes
  [48a46c71](https://github.com/phpredis/phpredis/commit/48a46c71104ac17d0688ab25f2b86e8fa2c6c406)
  ([michael-grunder](https://github.com/michael-grunder))
- Fix `GEOSEARCHSTORE` reply handling
  [4e093bb0](https://github.com/phpredis/phpredis/commit/4e093bb0dc447ee8eb17a8f154d36556ea0b3412)
  ([michael-grunder](https://github.com/michael-grunder))
- Preserve integer geohashes in `GEOSEARCH` metadata and validate the reply structure
  [a283471e](https://github.com/phpredis/phpredis/commit/a283471eb5aaf2c26781dc6f4338f0a56ccd95c4)
  ([michael-grunder](https://github.com/michael-grunder))
- Handle inline multibulk values and propagate raw reply read failures correctly
  [a283471e](https://github.com/phpredis/phpredis/commit/a283471eb5aaf2c26781dc6f4338f0a56ccd95c4)
  ([michael-grunder](https://github.com/michael-grunder))
- Route `FLUSHDB` and `FLUSHALL` to cluster primaries even when replica reads are enabled
  [b30ced74](https://github.com/phpredis/phpredis/commit/b30ced74c311b8975aa395da89c103f171c88b01)
  ([michael-grunder](https://github.com/michael-grunder))
- Fix crashes when setting `RedisCluster` TCP keepalive and handle missing sockets in `setOption`
  [997d5645](https://github.com/phpredis/phpredis/commit/997d564521b3b95866c7747a971c4e357d8046e4),
  [cb4acc7d](https://github.com/phpredis/phpredis/commit/cb4acc7dec293053a9e8943badfd0858342f5586)
  ([michael-grunder](https://github.com/michael-grunder))
- Use the correct option size when setting `SO_KEEPALIVE`
  [09d3dbf1](https://github.com/phpredis/phpredis/commit/09d3dbf1046b4b3aa4f590137d30f6af026af2a2)
  ([pkramme](https://github.com/pkramme))
- Allow `getLastError()` and `clearLastError()` after a failed connection without throwing
  [fc948db8](https://github.com/phpredis/phpredis/commit/fc948db86a7ac75b43fe71c333cd74710fa88c79)
  ([Nicolas Grekas](https://github.com/nicolas-grekas))
- Accept `null` for `$seeds` in `RedisCluster::__construct`
  [409508af](https://github.com/phpredis/phpredis/commit/409508afa219852f0128387417112641c66c9ce3)
  ([derrickschoen](https://github.com/derrickschoen))
- Preserve numeric-string hash field names and use the correct format for `zend_long` values
  [7c1c1259](https://github.com/phpredis/phpredis/commit/7c1c1259742425a09ac5c7c53f64270e07171db0),
  [fe67b8cb](https://github.com/phpredis/phpredis/commit/fe67b8cb3310a83d98040c0971d52c0d1bd24f53),
  [b8b29687](https://github.com/phpredis/phpredis/commit/b8b29687c18bb9249a73136f6df46f03895d602e)
  ([Pavlo Yatsukhnenko](https://github.com/yatsukhnenko))
  ([arshidkv12](https://github.com/arshidkv12))
- Handle deserialized non-string members when pairing sorted-set values and scores
  [cab05051](https://github.com/phpredis/phpredis/commit/cab0505155e51601bf70ff9a5b1e42ca7f58188e)
  ([michael-grunder](https://github.com/michael-grunder))
- Preserve `INFO` strings that would overflow when parsed as exponent-form numbers
  [e9e338b5](https://github.com/phpredis/phpredis/commit/e9e338b567338703deb58fa24ec55a705a28e3e5)
  ([Pavlo Yatsukhnenko](https://github.com/yatsukhnenko))
- Handle PHP serialization failures for anonymous classes
  [b83af641](https://github.com/phpredis/phpredis/commit/b83af6417b4cb174014d466eb8f14fce707a912a)
  ([arshidkv12](https://github.com/arshidkv12))
- Validate LZ4 decompressed lengths and NUL-terminate decompression buffers for PHP unserializers
  [2bf673c6](https://github.com/phpredis/phpredis/commit/2bf673c64f66de514b733259ff952ddf653199d2),
  [cf95cd0a](https://github.com/phpredis/phpredis/commit/cf95cd0a79be69943292d34e7ec1d700cd06761d)
  ([michael-grunder](https://github.com/michael-grunder))
- Reject invalid vector attributes in `VSETATTR`
  [234d1e7c](https://github.com/phpredis/phpredis/commit/234d1e7cda01f6bc79be36c9b7dabedc26358597)
  ([michael-grunder](https://github.com/michael-grunder))
- Prevent backoff option overflow and handle empty keys when prefixing
  [a2936256](https://github.com/phpredis/phpredis/commit/a29362563aa88883b39d717d597ff21bbba82309)
  ([michael-grunder](https://github.com/michael-grunder))
- Prevent crashes by disabling cloning of `Redis`, `RedisArray`, and `RedisCluster` objects
  [6e214c16](https://github.com/phpredis/phpredis/commit/6e214c169862af80e0215de068afc6e5c8aa7c40)
  ([michael-grunder](https://github.com/michael-grunder))
- Avoid persistent connection ID collisions and fix a connection pool memory leak
  [730dec06](https://github.com/phpredis/phpredis/commit/730dec06b60adab297ca2acc24b69b77bae9bf2d),
  [d90cfdb6](https://github.com/phpredis/phpredis/commit/d90cfdb6bdcab41da67df581786e291d985f601b)
  ([michael-grunder](https://github.com/michael-grunder))
- Prevent undefined behavior in weighted session server selection and overflow in cluster session key lengths
  [025b0e97](https://github.com/phpredis/phpredis/commit/025b0e9772c1c2381965156306b78766b39cf50f),
  [ae74b64b](https://github.com/phpredis/phpredis/commit/ae74b64be743a1aaf8d66f627dd984fcaa0862f8)
  ([michael-grunder](https://github.com/michael-grunder))
- Report missing session servers and improve session compression errors
  [bf2047e0](https://github.com/phpredis/phpredis/commit/bf2047e0674bfb3634a602cd7fd62cf23daf7eaa),
  [453c4788](https://github.com/phpredis/phpredis/commit/453c4788d689669e3ca48752e86a37186be586c0)
  ([michael-grunder](https://github.com/michael-grunder))
- Fix PHP 8.6 compatibility for session handlers, INI APIs, and `bin2hex`
  [b8b765e6](https://github.com/phpredis/phpredis/commit/b8b765e610f89c44da6143f9a636a9d989d3fd82),
  [9a170831](https://github.com/phpredis/phpredis/commit/9a170831254c9743386df54b6d81f2d3364949f6),
  [78c1c690](https://github.com/phpredis/phpredis/commit/78c1c690e94e88b7a9eb5c964e4c1aff51979c0f),
  [e3273a90](https://github.com/phpredis/phpredis/commit/e3273a90fe0521de4c35a7410e186be68b70dff6),
  [dd15a7dd](https://github.com/phpredis/phpredis/commit/dd15a7dd90738e840ce43b5815ddbff7fe6d123b),
  [f5460cc2](https://github.com/phpredis/phpredis/commit/f5460cc2510d10bc85514ef2744ba6ed5969f5e5)
  ([Rasmus Lerdorf](https://github.com/rlerdorf))
  ([michael-grunder](https://github.com/michael-grunder))
  ([Andy Postnikov](https://github.com/andypost))
  ([Remi Collet](https://github.com/remicollet))
- Fix compiler attribute detection
  [b192b0e4](https://github.com/phpredis/phpredis/commit/b192b0e454256eac36e840f0760aa3330878c778)
  ([michael-grunder](https://github.com/michael-grunder))
- Correct RedisCluster return types and the `zrevrangebylex` signature
  [6c1fbf86](https://github.com/phpredis/phpredis/commit/6c1fbf86ae56ace773d4931ce852f6bdacf3df67),
  [2716bea7](https://github.com/phpredis/phpredis/commit/2716bea73385fa688dc2a08d8c623a1d8c9f9407)
  ([Pavlo Yatsukhnenko](https://github.com/yatsukhnenko))
  ([michael-grunder](https://github.com/michael-grunder))
- Correct `XDELEX` return types in Redis, RedisCluster, and RedisArray stubs
  [9f4302c0](https://github.com/phpredis/phpredis/commit/9f4302c0ccd81d5546db6ee29ea694f79c04d9c5),
  [8dcaedae](https://github.com/phpredis/phpredis/commit/8dcaedae0f0164f63fdfdbcd2da3c1b73e96c37f)
  ([Pavlo Yatsukhnenko](https://github.com/yatsukhnenko))
  ([michael-grunder](https://github.com/michael-grunder))

## Added

- Implement `BLESS` key eviction protection commands, including a lazy `RedisBlessScanIterator` for `BLESS SCAN`
  [4751fdf8](https://github.com/phpredis/phpredis/commit/4751fdf8d2a496ab692333ed17deac8b3b6ac43c)
  ([michael-grunder](https://github.com/michael-grunder))
- Implement `INCREX` for `Redis` and `RedisCluster`
  [90fabdf9](https://github.com/phpredis/phpredis/commit/90fabdf91447440b066fca3099836bdd1945c9e1),
  [7c186d94](https://github.com/phpredis/phpredis/commit/7c186d94a5fbabbee14f79cdcc44398feb80b55f)
  ([Simon Hammes](https://github.com/simonhammes))
  ([michael-grunder](https://github.com/michael-grunder))
- Implement `BITRANGEINTARRAY`
  [9222d5cc](https://github.com/phpredis/phpredis/commit/9222d5cc346b4439888c9b084d63551c2852d68c)
  ([michael-grunder](https://github.com/michael-grunder))
- Add `reconnect()` to `Redis` and `RedisCluster`
  [a72ec1ab](https://github.com/phpredis/phpredis/commit/a72ec1ababf789778a9953b8511c99119d742b75)
  ([michael-grunder](https://github.com/michael-grunder))
- Add optional `Redis::SERIALIZER_PHPSER` support, including signing with `phpser.key`
  [b3e890b7](https://github.com/phpredis/phpredis/commit/b3e890b786b7e2b9626b87cbc8fe636d8b7ba5fe)
  ([Ilia Alshanetsky](https://github.com/iliaal))
- Support `AGGREGATE COUNT` in `ZUNION[STORE]` and `ZINTER[STORE]`
  [810984b5](https://github.com/phpredis/phpredis/commit/810984b5a5aedc37e3ff3c19e33af8dc08c8290f)
  ([michael-grunder](https://github.com/michael-grunder))
- Implement `HIMPORT`
  [55f28493](https://github.com/phpredis/phpredis/commit/55f28493a0223b120efd71ba642cb0115f306bfd)
  ([michael-grunder](https://github.com/michael-grunder))
- Implement `LMOVEM` and `BLMOVEM`
  [7419002a](https://github.com/phpredis/phpredis/commit/7419002a165dca58e84fb79eab6cef6c9c9a4f1d)
  ([michael-grunder](https://github.com/michael-grunder))
- Implement `SUNIONCARD` and `SDIFFCARD`
  [533de1fe](https://github.com/phpredis/phpredis/commit/533de1fe5e9a4ec5dfd2b9da7435d11177d906f4),
  [e2141f88](https://github.com/phpredis/phpredis/commit/e2141f88c5c2634c8532591927f0b68870212430)
  ([michael-grunder](https://github.com/michael-grunder))
- Implement `GCRA` with the `TOKENS` option
  [10b77a42](https://github.com/phpredis/phpredis/commit/10b77a42d6bea489a7ccca04adbbeb627c14dcbc),
  [5b197316](https://github.com/phpredis/phpredis/commit/5b197316497b9eb350770a84fad045185f3e015e)
  ([michael-grunder](https://github.com/michael-grunder))
- Implement `MSETEX` for `Redis` and `RedisCluster`
  [86eabb86](https://github.com/phpredis/phpredis/commit/86eabb86eb4efad7bd3daf5735b4205ba6e13231),
  [41922b67](https://github.com/phpredis/phpredis/commit/41922b6740f605869f3a0832d0cece90bef8fc8e)
  ([michael-grunder](https://github.com/michael-grunder))
- Implement `XDELEX` and `XACKDEL` for `Redis`, `RedisCluster`, and `RedisArray`
  [635d87d5](https://github.com/phpredis/phpredis/commit/635d87d535e15626b8df5493bfa15bd1a7a661d9),
  [23cc7859](https://github.com/phpredis/phpredis/commit/23cc785989878d420539a02bdaab44e467bfe486),
  [0191f7bb](https://github.com/phpredis/phpredis/commit/0191f7bb5f3d78c226ed5f7bb7bc0a615fcbc5b0)
  ([michael-grunder](https://github.com/michael-grunder))
- Implement `DELEX`
  [00c62de2](https://github.com/phpredis/phpredis/commit/00c62de277e8c93ba6c18bfe9b713478ac49dd75),
  [93cb9211](https://github.com/phpredis/phpredis/commit/93cb921158da4d1342a5ca291dcf4d68a54a2048)
  ([michael-grunder](https://github.com/michael-grunder))
- Implement `DIGEST` and the local `_digest()` helper, requiring PHP 8.1 or newer for local hashing
  [e2dd13ce](https://github.com/phpredis/phpredis/commit/e2dd13ce7b756ffd358939b1f557f4b4b00abf6e),
  [107cc7aa](https://github.com/phpredis/phpredis/commit/107cc7aacb88597c8bfe9492387a9d0cb8417c71)
  ([michael-grunder](https://github.com/michael-grunder))
- Implement `WAIT` for `RedisCluster` and share command handling with `WAITAOF`
  [4de4f727](https://github.com/phpredis/phpredis/commit/4de4f727a46c5f4c28f46354c2ec04021a92f8e1)
  ([michael-grunder](https://github.com/michael-grunder))
- Add `redis.session.lock_release_cmd` to select Lua, `DELEX`, or `DELIFEQ` for session lock release
  [e39d9b74](https://github.com/phpredis/phpredis/commit/e39d9b74f48463f7b0fe4a5d2fc8f9b6cde4afc2)
  ([michael-grunder](https://github.com/michael-grunder))

## Changed

- Enable persistent connections in ZTS builds, with connections reused per thread
  [b37b2316](https://github.com/phpredis/phpredis/commit/b37b23169cf4790442b9f007d3c88a893af2df5f)
  ([Kevin Dunglas](https://github.com/dunglas))
- Disallow serialization of `Redis`, `RedisArray`, `RedisCluster`, and `RedisSentinel` objects, including on PHP versions before 8.1
  [befb08c0](https://github.com/phpredis/phpredis/commit/befb08c05f5841b5c3fea515a7efd20b5363e962),
  [bac00154](https://github.com/phpredis/phpredis/commit/bac00154b1f528b2d0ca42d2726d2cc31d145e3b)
  ([Nicolas Grekas](https://github.com/nicolas-grekas))
- Mark connection options and context arguments as `#[\SensitiveParameter]`
  [671e726e](https://github.com/phpredis/phpredis/commit/671e726e7d87916ffc1c8c8dbbb20c355bcd731a)
  ([Nicolas Grekas](https://github.com/nicolas-grekas))
- Use cryptographically random session lock secrets
  [b112875b](https://github.com/phpredis/phpredis/commit/b112875b70545df684931e71a7d3b4f6901ea62a)
  ([Ilia Alshanetsky](https://github.com/iliaal))
- Rework TLS context ownership and allocation for connections and cluster nodes
  [b97951cd](https://github.com/phpredis/phpredis/commit/b97951cddc68e8d195198d1d0b911573953ffa4d)
  ([michael-grunder](https://github.com/michael-grunder))

## Documentation

- Add RedisArray method annotations and correct PHPDoc parameter, return, nullable, and variadic types
  [8597d3d7](https://github.com/phpredis/phpredis/commit/8597d3d793f335b8bdc0f268407bf7322717dcf7),
  [5afa8eb7](https://github.com/phpredis/phpredis/commit/5afa8eb7e0fd51ec9901c83e72915aeca7f9ddba),
  [9c9d0286](https://github.com/phpredis/phpredis/commit/9c9d0286f420f4569378f1c5a3e1d7c00bad49f8),
  [b742bb8b](https://github.com/phpredis/phpredis/commit/b742bb8bdb05456f4b8aea80020492b00213f1da),
  [6aea98f6](https://github.com/phpredis/phpredis/commit/6aea98f632ade32e99dfefba22dc122fe3190062),
  [26a73f46](https://github.com/phpredis/phpredis/commit/26a73f46d1648cbee1daf023c0ad3316c65c11f2),
  [481f12f8](https://github.com/phpredis/phpredis/commit/481f12f84c9be756ac19440efb159f859398205c),
  [8894de5a](https://github.com/phpredis/phpredis/commit/8894de5a0b0db65415436b101e53c11b9b4759f2)
  ([Michael Telgmann](https://github.com/mitelg))
  ([michael-grunder](https://github.com/michael-grunder))
- Document TLS options and clarify that `connect()` throws on failure
  [c28430e5](https://github.com/phpredis/phpredis/commit/c28430e5f6066a86583751fc4315397ae7a84c41),
  [6efd549a](https://github.com/phpredis/phpredis/commit/6efd549a1a6d28228fa29484e342ba79e9d45f46)
  ([michael-grunder](https://github.com/michael-grunder))
- Document session `read_timeout` and the default `persistent` setting
  [0340599b](https://github.com/phpredis/phpredis/commit/0340599b90cd106cf3259c30c3a6c1166dba5903),
  [c99c8634](https://github.com/phpredis/phpredis/commit/c99c86349aac32ebe1396a57da750f3b39b7ad61)
  ([Sowmya Mallur](https://github.com/Dot2892))
  ([林博仁 Buo-ren Lin](https://github.com/brlin-tw))
- Explain transactions inside pipelines and their reply formats
  [8ef2ee92](https://github.com/phpredis/phpredis/commit/8ef2ee9283bda3b660ab23f2c66d06b3d509dbf8)
  ([g](https://github.com/gopi-ar))
- Document `WAIT` and `WAITAOF`
  [cd4cf653](https://github.com/phpredis/phpredis/commit/cd4cf653df04f02d9ed9d0492195f78dedb2791a)
  ([michael-grunder](https://github.com/michael-grunder))
- Document `UNSUBSCRIBE` and `PUNSUBSCRIBE`
  [6b481b6f](https://github.com/phpredis/phpredis/commit/6b481b6f66d9e69e11370eaf234381ae0b9bbd2a)
  ([michael-grunder](https://github.com/michael-grunder))
- Document class constants, backoff algorithms, and cluster failover strategies
  [49919b0e](https://github.com/phpredis/phpredis/commit/49919b0ec507d73f7a4d388efdb8d858579bbe69),
  [26cf6d94](https://github.com/phpredis/phpredis/commit/26cf6d9494cb13a1228ede4fcdd595a7c24e801a),
  [8ef349a2](https://github.com/phpredis/phpredis/commit/8ef349a2d9ac04cab26bf74c13053a8299d82702),
  [0981c2b7](https://github.com/phpredis/phpredis/commit/0981c2b7d76b7d4e085196cc28fb60c0db728185)
  ([michael-grunder](https://github.com/michael-grunder))
- Expand helper method documentation and examples, and correct stub wording and types
  [d6437f5e](https://github.com/phpredis/phpredis/commit/d6437f5e95df1edc4396e2b4e69fe0899342502a),
  [abce1655](https://github.com/phpredis/phpredis/commit/abce1655b96719126b2d926a42ae29b743ad18c9),
  [c17f475c](https://github.com/phpredis/phpredis/commit/c17f475caa0cbdfc5a58777c59085113f7c25add),
  [399346e5](https://github.com/phpredis/phpredis/commit/399346e51a4b8c6bff0d8c2125a776131062955c),
  [f23da62a](https://github.com/phpredis/phpredis/commit/f23da62a9b5c27e20195627b692fc31e7713c218),
  [806b7b3f](https://github.com/phpredis/phpredis/commit/806b7b3f79ace4159c69c748a85144dbb8848997),
  [1f3fb179](https://github.com/phpredis/phpredis/commit/1f3fb179cd2a3bd156169d2bdc19d118a6f70815)
  ([michael-grunder](https://github.com/michael-grunder))
- Link API methods to Redis and Valkey command references
  [a46700cf](https://github.com/phpredis/phpredis/commit/a46700cfc89d643856c72444cf57e444a33184f0),
  [76286d88](https://github.com/phpredis/phpredis/commit/76286d88a9c1d7776c7cb89ef707806948c903ca)
  ([michael-grunder](https://github.com/michael-grunder))
- Add syntax highlighting to generated API documentation and build it from `develop`
  [10c0bf0f](https://github.com/phpredis/phpredis/commit/10c0bf0fcfa3ff93065ace2fbcbd3cbcdc9f3fb0),
  [1297c48c](https://github.com/phpredis/phpredis/commit/1297c48c1473f7cba74d511b6cd57ceb640e9ca7),
  [e460a859](https://github.com/phpredis/phpredis/commit/e460a8595af666c5106cf4a9ff4d363f59a9610f),
  [4cf670fe](https://github.com/phpredis/phpredis/commit/4cf670fed3608470a5b10463596525dce736a7f7),
  [ae69e47a](https://github.com/phpredis/phpredis/commit/ae69e47a08955305a319623ed7954c149c9c2765),
  [d45c4d29](https://github.com/phpredis/phpredis/commit/d45c4d294c8d00358c63d4dcd8edf1eb37f008e7),
  [9edc2425](https://github.com/phpredis/phpredis/commit/9edc24257913d6effcdbec7cdfdc22db01b0054d),
  [38e56d57](https://github.com/phpredis/phpredis/commit/38e56d573858f713e12c3f699c77910c6091b686),
  [5fe08621](https://github.com/phpredis/phpredis/commit/5fe0862109fc085fef3801114d3ab1136e332a4b),
  [d5b1576e](https://github.com/phpredis/phpredis/commit/d5b1576e272baea1724210d640ef48e3e8e41c0f)
  ([michael-grunder](https://github.com/michael-grunder))
- Refresh generated API documentation, including new commands and iterator documentation
  [ecdc290b](https://github.com/phpredis/phpredis/commit/ecdc290bba19c48d27fc14f1fa74d2187c791a7b),
  [9e9ccbd4](https://github.com/phpredis/phpredis/commit/9e9ccbd441686ce252aa86d9c9d4a18346620542),
  [d4600a28](https://github.com/phpredis/phpredis/commit/d4600a284639968f705c0264dc855188093c5298),
  [df243455](https://github.com/phpredis/phpredis/commit/df243455e656c2fb5023e905b10511e33b754196),
  [f5ed1704](https://github.com/phpredis/phpredis/commit/f5ed17048bde7c9e8ada04005b0773bbd99e5abf),
  [e6d0d119](https://github.com/phpredis/phpredis/commit/e6d0d119385457323c83cf8182dcc7f2b99624e7),
  [4e79159e](https://github.com/phpredis/phpredis/commit/4e79159e7bb2b3ac312c602084b235a27c1c1325),
  [39472124](https://github.com/phpredis/phpredis/commit/3947212448da38813567d99b011aa8f758fab662),
  [362ee55c](https://github.com/phpredis/phpredis/commit/362ee55c8d5365c3f991794c978eca9205d822c9)
  ([michael-grunder](https://github.com/michael-grunder))
- Mention Dragonfly support in the README
  [2b71a968](https://github.com/phpredis/phpredis/commit/2b71a9688f7cf1c9169894a34b7c218212f2eeae)
  ([michael-grunder](https://github.com/michael-grunder))

## Tests/CI

- Allow multiple test filters and exclusions, and print commands for rerunning tests
  [07f620e7](https://github.com/phpredis/phpredis/commit/07f620e767739676fa34f75a48a2a7d8d3aa2d0c),
  [a2b98330](https://github.com/phpredis/phpredis/commit/a2b983304a272e1190f8ae1dab7ed870e356fc7d),
  [2343369b](https://github.com/phpredis/phpredis/commit/2343369b47a9e601252134e6930635ce38a3cc4e)
  ([michael-grunder](https://github.com/michael-grunder))
- Improve test diagnostics, fail-fast assertions, and compatibility with subclasses
  [4baad44b](https://github.com/phpredis/phpredis/commit/4baad44bcfd7dc710b9eae78e6652b0cdc1f0d50),
  [4a02f3dd](https://github.com/phpredis/phpredis/commit/4a02f3dda635c3d9faa5117166618106c444bfd5),
  [2a0569ec](https://github.com/phpredis/phpredis/commit/2a0569ece7045cfa492813168418842a17a0d206),
  [b0d534e1](https://github.com/phpredis/phpredis/commit/b0d534e1ca0b183c80925eed0b1699894dc4d806),
  [43d72dad](https://github.com/phpredis/phpredis/commit/43d72dad29e1b6e47d2bd856974546288c2c3db6),
  [c92c0422](https://github.com/phpredis/phpredis/commit/c92c04220cbb1c034fbfbd0610c1dc935111b045),
  [506715fe](https://github.com/phpredis/phpredis/commit/506715fea41f0935907ac5f416c8e3bcc1abbe14),
  [e037b7ca](https://github.com/phpredis/phpredis/commit/e037b7ca9566376357d9cd3463e64df7f0229e82),
  [10ae0080](https://github.com/phpredis/phpredis/commit/10ae0080a333e86adf1ad850d5728b619473d62e)
  ([michael-grunder](https://github.com/michael-grunder))
- Reduce timing-test delays and make expiration assertions less sensitive to scheduling
  [09d496b2](https://github.com/phpredis/phpredis/commit/09d496b239410852ef8027767d6fd2190b2ed9b6),
  [53ba9b4b](https://github.com/phpredis/phpredis/commit/53ba9b4b761084fa9111c9820c38366c671bac1c),
  [c7227339](https://github.com/phpredis/phpredis/commit/c7227339790df2c04b66a9b9bb4d45e7d21af808),
  [e5f0df63](https://github.com/phpredis/phpredis/commit/e5f0df635abdc99500693877a351458f38d086bc),
  [777f7377](https://github.com/phpredis/phpredis/commit/777f7377674a8f5d23a7ba739d165daeebc13e47),
  [7ae4014c](https://github.com/phpredis/phpredis/commit/7ae4014c5adb3e1b5857e1f5d42ffcac125e621d)
  ([michael-grunder](https://github.com/michael-grunder))
- Fix session and replication test races, and use explicit exit codes for session helpers
  [bccd60d4](https://github.com/phpredis/phpredis/commit/bccd60d451afce60e38d84ee31b5f1909ec670d9),
  [531b17e5](https://github.com/phpredis/phpredis/commit/531b17e5cfe0b67111f3977a60ee25d5cb96ed69),
  [3c65094e](https://github.com/phpredis/phpredis/commit/3c65094e1e0e581b1bd6f80499ba67cd5ec5f8df),
  [4bccf599](https://github.com/phpredis/phpredis/commit/4bccf5995e716518ea4bdd5b34266cfdbe270e4f)
  ([michael-grunder](https://github.com/michael-grunder))
- Fix flaky `EXISTS` coverage with deterministic keys
  [82bf96c3](https://github.com/phpredis/phpredis/commit/82bf96c3c059161e0a91e61dc120b76430149769)
  ([武田 憲太郎](https://github.com/KentarouTakeda))
- Add numeric-string hash field regression coverage across execution modes
  [6f42a349](https://github.com/phpredis/phpredis/commit/6f42a3493d84dddef715b117fd9810ab2b92e2ec),
  [0ef13108](https://github.com/phpredis/phpredis/commit/0ef13108fae91acbe8c2754bb2f777b749e0a8eb)
  ([michael-grunder](https://github.com/michael-grunder))
  ([Till Krüss](https://github.com/tillkruss))
- Add `MSETEX` tests and fix expiry assertions
  [bd4989e0](https://github.com/phpredis/phpredis/commit/bd4989e004bee119e70f7ed253e9e1faad9007a1),
  [a033e40b](https://github.com/phpredis/phpredis/commit/a033e40b7b0fc5e0ea3786ed0df39e32deac2f38),
  [074b7c7f](https://github.com/phpredis/phpredis/commit/074b7c7ff8c73e252afc0ecb49264a281c2b1e29)
  ([michael-grunder](https://github.com/michael-grunder))
- Add `XDELEX` tests
  [8b4cfb72](https://github.com/phpredis/phpredis/commit/8b4cfb725f0468a076e8d57a4310bdab8185a1c2)
  ([michael-grunder](https://github.com/michael-grunder))
- Improve serialization tests
  [f5835a62](https://github.com/phpredis/phpredis/commit/f5835a622a3042e671de82c114902fd9ee60cf57),
  [1844cc57](https://github.com/phpredis/phpredis/commit/1844cc5733e09917aa0dbb3624d5fe9c020d8dce)
  ([michael-grunder](https://github.com/michael-grunder))
- Fix `_digest()` tests on PHP versions before 8.1 and avoid deprecated `is_long()` on PHP 8.6
  [0debad4c](https://github.com/phpredis/phpredis/commit/0debad4c833d0fd9bac5b4763c5ed840e0483f5e),
  [57b584f6](https://github.com/phpredis/phpredis/commit/57b584f676a504842d86c4dafb6b8f9abaf21ba2)
  ([michael-grunder](https://github.com/michael-grunder))
- Account for duplicate `ZSCAN` results in scan tests
  [33ee649c](https://github.com/phpredis/phpredis/commit/33ee649c73aa02a836b99e2a284bb897c024f24a)
  ([michael-grunder](https://github.com/michael-grunder))
- Skip unsupported `DELEX` options when testing Valkey
  [88050422](https://github.com/phpredis/phpredis/commit/880504222c3f4330ff09bd8026af8e4b34beddb6)
  ([Remi Collet](https://github.com/remicollet))
- Add PHP 8.5 to macOS and Windows CI, and fix the Homebrew configuration path
  [ac54591d](https://github.com/phpredis/phpredis/commit/ac54591d1e140e33fdc681088548ba438cbceda1),
  [a6313d1b](https://github.com/phpredis/phpredis/commit/a6313d1bda6fb25bc53d8ef3b6d7dd67e05503aa)
  ([Emre Çalışkan](https://github.com/thecaliskan))
- Migrate Windows CI to `php/php-windows-builder`, use commit SHAs, and make binaries discoverable by PIE
  [8f8272db](https://github.com/phpredis/phpredis/commit/8f8272dbf1629979b63fe98c439cd5b1e5a0ec34),
  [f14ce600](https://github.com/phpredis/phpredis/commit/f14ce6007bf7576b0ca4f043b10ab303b3328126),
  [de2e1b39](https://github.com/phpredis/phpredis/commit/de2e1b390d90fface0be8455704935bec6648d61)
  ([Pavlo Yatsukhnenko](https://github.com/yatsukhnenko))
  ([michael-grunder](https://github.com/michael-grunder))
- Refactor CI jobs, install dependencies with PIE, and bound server startup waits
  [60eb01c4](https://github.com/phpredis/phpredis/commit/60eb01c4dcffa5e322f117a1ca5298035f08ae06),
  [a01dec12](https://github.com/phpredis/phpredis/commit/a01dec12636499566f47b4a5c70c438490433b89),
  [e5c0ef7c](https://github.com/phpredis/phpredis/commit/e5c0ef7cc48f230b4939e14d3e56f17b804bea56),
  [7119dce1](https://github.com/phpredis/phpredis/commit/7119dce1e05b1e1103dfa448d167f4c33e9127fa)
  ([michael-grunder](https://github.com/michael-grunder))
- Replace KeyDB coverage with Dragonfly and test Valkey and Redis unstable builds
  [8b222585](https://github.com/phpredis/phpredis/commit/8b2225852c4db8e6031e0e0978750f6f4e53a5a3),
  [a283471e](https://github.com/phpredis/phpredis/commit/a283471eb5aaf2c26781dc6f4338f0a56ccd95c4),
  [b7c382c8](https://github.com/phpredis/phpredis/commit/b7c382c82c83df3831efebfd7dc36cd7a9f6d864),
  [959dee1c](https://github.com/phpredis/phpredis/commit/959dee1ca9ed55500cf90365419f8232eca3a99c)
  ([michael-grunder](https://github.com/michael-grunder))
- Require Valgrind runs to pass and fix a flaky Sentinel test
  [a00d952d](https://github.com/phpredis/phpredis/commit/a00d952d384a6c3546abec336fcb7278f38bafa8),
  [060a2c5e](https://github.com/phpredis/phpredis/commit/060a2c5e48727e8d390cb764718aa1e041b3671d)
  ([michael-grunder](https://github.com/michael-grunder))

## Internal/Performance

- Introduce `RedisCmd` command construction and migrate standalone, cluster, and session command builders
  [2c5ef192](https://github.com/phpredis/phpredis/commit/2c5ef192575023bff372a08c3cc726e8c8df1ded),
  [4f3090f0](https://github.com/phpredis/phpredis/commit/4f3090f037432bb95e0a20bec45ed50075e9d961),
  [875c088b](https://github.com/phpredis/phpredis/commit/875c088bbab416007e3ae66f6beec06bb0fb8fa5),
  [939c8fed](https://github.com/phpredis/phpredis/commit/939c8fed6546b76d9c05f1b03229c4cf86a70ea4),
  [e81506bd](https://github.com/phpredis/phpredis/commit/e81506bdad664bdc0bd910f9b05bc9fd3cf730eb),
  [b12a64fe](https://github.com/phpredis/phpredis/commit/b12a64fe7509c08ddbcb0ebbe6b913f4ee06a622)
  ([michael-grunder](https://github.com/michael-grunder))
- Switch to the Fast Parameter Parsing API
  [c2d2254e](https://github.com/phpredis/phpredis/commit/c2d2254e56b6223c03087a4aa0902289147adb30)
  ([Pavlo Yatsukhnenko](https://github.com/yatsukhnenko))
- Read multibulk strings directly into `zend_string` buffers and pre-size reply arrays
  [0c1b8b22](https://github.com/phpredis/phpredis/commit/0c1b8b2232bf0e0811b564a12e8f4e192909a18f),
  [da514c71](https://github.com/phpredis/phpredis/commit/da514c71bb460499f8e5f48ac2c56b3d1a4cc491),
  [798aa657](https://github.com/phpredis/phpredis/commit/798aa6578403d91a250e753269bdddedbe1615d0)
  ([Ilia Alshanetsky](https://github.com/iliaal))
- Remove a redundant liveness probe when reading bulk replies
  [640ed13f](https://github.com/phpredis/phpredis/commit/640ed13fcc81fbccc632872ff00da2edfd2087ff)
  ([Ilia Alshanetsky](https://github.com/iliaal))
- Skip clock reads when no cluster timeout is set
  [44f49442](https://github.com/phpredis/phpredis/commit/44f494428b3f8ddbd0d1640c6da43fe4fc2d4121)
  ([Ilia Alshanetsky](https://github.com/iliaal))
- Avoid per-command and per-key allocations in cluster writes and RedisArray hashing, and redundant zero-filling during key prefixing
  [9623a663](https://github.com/phpredis/phpredis/commit/9623a663201917ab0e22864811990b14c212b3e2),
  [8b746bfc](https://github.com/phpredis/phpredis/commit/8b746bfc7842d73735c8695e4849f17c33612047),
  [b9320359](https://github.com/phpredis/phpredis/commit/b9320359e8903a15bfc55402bf842fa077e08dca)
  ([Ilia Alshanetsky](https://github.com/iliaal))
- Modernize Zend API usage and array traversal, and replace legacy aliases
  [8e1c2789](https://github.com/phpredis/phpredis/commit/8e1c2789a4e192907f20ce8ffbf4390b7ac4f649),
  [d24528e1](https://github.com/phpredis/phpredis/commit/d24528e190437a47b53582f5d0b246266546db3e),
  [7d4fa613](https://github.com/phpredis/phpredis/commit/7d4fa6137c0bbea8cc3357a7fed45757c558fee6),
  [3401462e](https://github.com/phpredis/phpredis/commit/3401462e00fc48c45466287eb88552462930c13a),
  [cd2a3963](https://github.com/phpredis/phpredis/commit/cd2a39635a30c6b27dd4758f982d8898a610da13),
  [5acaba93](https://github.com/phpredis/phpredis/commit/5acaba930407118e8f2aa10b02f4fdd629d1d5fd)
  ([Niels Dossche](https://github.com/ndossche))
  ([michael-grunder](https://github.com/michael-grunder))
- Consolidate scan handling
  [e64055bd](https://github.com/phpredis/phpredis/commit/e64055bd35083e7b8d21728bfbc5306c4a64dc0d)
  ([michael-grunder](https://github.com/michael-grunder))
- Consolidate key prefixing
  [cd7996fb](https://github.com/phpredis/phpredis/commit/cd7996fbaf3e37d485be2ce8410fafd38376260a)
  ([michael-grunder](https://github.com/michael-grunder))
- Improve cluster WATCH bookkeeping and cleanup on failures
  [4736b167](https://github.com/phpredis/phpredis/commit/4736b1679e50feb1efaee2d77a38cd8cc10f4de8)
  ([michael-grunder](https://github.com/michael-grunder))
- Refactor command contexts and replace function-like macros with typed helpers
  [2ce9d949](https://github.com/phpredis/phpredis/commit/2ce9d949365a49fc63dfd01d234fc5c081d3bce5),
  [bc947026](https://github.com/phpredis/phpredis/commit/bc9470264343d1d96f1ae0f54d41d4f15cdc1192),
  [910c925f](https://github.com/phpredis/phpredis/commit/910c925f3bbce0c81f40015ed74dcfb42dd0f1cc),
  [5377f588](https://github.com/phpredis/phpredis/commit/5377f588f16774d3c19bb083b9272902e87eac07),
  [a9dd268a](https://github.com/phpredis/phpredis/commit/a9dd268adb90dce82047a64fae65ce1a41a2591f),
  [10e81b33](https://github.com/phpredis/phpredis/commit/10e81b337dd7e0ff140459012c0aa0f321dc28e1),
  [d9aea09e](https://github.com/phpredis/phpredis/commit/d9aea09e4c0bbd9dc452f99bbf3c535a181a2769)
  ([michael-grunder](https://github.com/michael-grunder))
- Improve const-correctness, remove unnecessary casts, and restrict internal function linkage
  [7090338f](https://github.com/phpredis/phpredis/commit/7090338f206d5bfeb0ebe93b3432224575ddaa64),
  [e87e1668](https://github.com/phpredis/phpredis/commit/e87e1668b181e674a993a385770025edd1889a71),
  [34e9a4ae](https://github.com/phpredis/phpredis/commit/34e9a4aec0de9c482a29790f3243ce5923da2ba2),
  [4366a579](https://github.com/phpredis/phpredis/commit/4366a579e52a35a6150e2c0e61c1c3a89c6f8484),
  [887c5b19](https://github.com/phpredis/phpredis/commit/887c5b19688689b42db66a657ae3630bb0e7cb06)
  ([Simon Hammes](https://github.com/simonhammes))
  ([michael-grunder](https://github.com/michael-grunder))
- Modernize session handling and remove obsolete code and fields
  [738cedd2](https://github.com/phpredis/phpredis/commit/738cedd28455e15ad7ccd1b4f58e7d3b613dff5d),
  [741abf09](https://github.com/phpredis/phpredis/commit/741abf09ecad578608822dbd465f6ca6df078778),
  [b58ca169](https://github.com/phpredis/phpredis/commit/b58ca169406f1373ef665c4da2e24b83256f95ae),
  [976e2384](https://github.com/phpredis/phpredis/commit/976e2384d924de10b3acc3aaf6c115af83c81cbe),
  [0017df83](https://github.com/phpredis/phpredis/commit/0017df839739356edc4cdd3e6e4f1a2d174cf55d),
  [33a78a45](https://github.com/phpredis/phpredis/commit/33a78a45dbe95c5c39632dc29b5b52f4b1da5dd2),
  [4af1e20a](https://github.com/phpredis/phpredis/commit/4af1e20a83a99001de1e4b141fab1232e0663e06),
  [d596a239](https://github.com/phpredis/phpredis/commit/d596a2398fab54e0fb39a986f87fd28eeed2122f)
  ([michael-grunder](https://github.com/michael-grunder))
- Add `--enable-redis-sanitizers` and `--enable-redis-analyzer` build options
  [288e231b](https://github.com/phpredis/phpredis/commit/288e231bf01bdefefc55e9f7565a851547b407c1),
  [5fea4648](https://github.com/phpredis/phpredis/commit/5fea4648225120191ab642a358c0cf21420b82b3)
  ([michael-grunder](https://github.com/michael-grunder))
- Add agent development instructions and ignore generated dependency and vendor files
  [ea8a8672](https://github.com/phpredis/phpredis/commit/ea8a86727e3ff1b1b660b06f0162a8cd1bb974ba),
  [c8b79b16](https://github.com/phpredis/phpredis/commit/c8b79b168663f3c3e1aa960fc99e3df39676c357),
  [b30fe1d1](https://github.com/phpredis/phpredis/commit/b30fe1d1c70d28ef4bacb702cc29e06ee88c2460),
  [9b17434e](https://github.com/phpredis/phpredis/commit/9b17434e3dfc2e80e2ca87fc5e4bb6a0fdba0b4d)
  ([michael-grunder](https://github.com/michael-grunder))

# [6.3.0] - 2025-11-06 ([GitHub](https://github.com/phpredis/phpredis/releases/tag/6.3.0), [PECL](https://pecl.php.net/package/redis/6.3.0))

This release introduces support for dozens of new commands, including hash field expiration, Valkey’s DELIFEQ, and Redis vector set commands. It also includes many bug fixes and performance improvements.

### Sponsors :sparkling_heart:

- [A-VISION](https://github.com/A-VISION-BV)
- [Avtandil Kikabidze](https://github.com/akalongman)
- [Geoffrey Hoffman](https://github.com/phpguru)
- [Object Cache Pro for WordPress](https://objectcache.pro/)
- [Open LMS](https://openlms.net/)
- [Relay](https://relay.so)
- [Salvatore Sanfilippo](https://github.com/antirez)
- [Ty Karok](https://github.com/karock)

## Fixed

- Cloning our objects should not segfault
  [770034cc](https://github.com/phpredis/phpredis/commit/770034cc)
  ([michael-grunder](https://github.com/michael-grunder))
- Fix return type for `RedisCluster` `vgetattr` and `vsetattr`
  [ab593f01](https://github.com/phpredis/phpredis/commit/ab593f01)
  ([michael-grunder](https://github.com/michael-grunder))

# [6.3.0RC1] - 2025-10-15 ([GitHub](https://github.com/phpredis/phpredis/releases/tag/6.3.0RC1), [PECL](https://pecl.php.net/package/redis/6.3.0RC1))

This release introduces support for dozens of new commands, including hash field expiration, Valkey’s DELIFEQ, and Redis vector set operations. It also delivers numerous bug fixes and performance improvements.

### Sponsors :sparkling_heart:

- [A-VISION](https://github.com/A-VISION-BV)
- [Avtandil Kikabidze](https://github.com/akalongman)
- [Geoffrey Hoffman](https://github.com/phpguru)
- [Object Cache Pro for WordPress](https://objectcache.pro/)
- [Open LMS](https://openlms.net/)
- [Relay](https://relay.so)
- [Salvatore Sanfilippo](https://github.com/antirez)
- [Ty Karok](https://github.com/karock)

## Fixed

- Fix possible hash field name truncation
  [834d2b37](https://github.com/phpredis/phpredis/commit/834d2b37)
  ([michael-grunder](https://github.com/michael-grunder))
- Fix a possible segfault during failover
  [5ebb853e](https://github.com/phpredis/phpredis/commit/5ebb853e)
  ([rlerdorf](https://github.com/rlerdorf))
- Bump version and fix XGROUP test
  [f5db01b7](https://github.com/phpredis/phpredis/commit/f5db01b7),
  [4517d5f9](https://github.com/phpredis/phpredis/commit/4517d5f9)
  ([remicollet](https://github.com/remicollet))
- Fix an overflow bug in ZADD on Windows
  [35df8ad7](https://github.com/phpredis/phpredis/commit/35df8ad7c2fc54fbf2a58d486cce49e712344bb2)
  ([michael-grunder](https://github.com/michael-grunder))
- Fix errors and a warning
  [b8de91c9](https://github.com/phpredis/phpredis/commit/b8de91c9e09dfaf6850273619b5ed92ce5d88db9)
  ([michael-grunder](https://github.com/michael-grunder))
- Fix `RedisCluster` segfault
  [f61e8cd7](https://github.com/phpredis/phpredis/commit/f61e8cd7bae16a761fd5c6c4b62e1e52707e0841)
  ([michael-grunder](https://github.com/michael-grunder))
- Fix passing NULL for hash expiry argument
  [ca80ee0e](https://github.com/phpredis/phpredis/commit/ca80ee0e6781bad4b4e453a10e968491b12db579)
  ([michael-grunder](https://github.com/michael-grunder))
- Fix an off-by-one length calculation error
  [340f23b0](https://github.com/phpredis/phpredis/commit/340f23b0827b31ae32638730110323d607a29412)
  ([michael-grunder](https://github.com/michael-grunder))
- Fix hset fields handling
  [6b2f088d](https://github.com/phpredis/phpredis/commit/6b2f088d49ae295c7a787e228a7f51d62beddec0)
  ([yatsukhnenko](https://github.com/yatsukhnenko))
- Fix HSetEx expiry argument handling
  [7805da75](https://github.com/phpredis/phpredis/commit/7805da7542b4f33fd2bd0300f3666d2325307e5b)
  ([yatsukhnenko](https://github.com/yatsukhnenko))
- Fix the echo liveness check when in sentinel mode
  [2acab399](https://github.com/phpredis/phpredis/commit/2acab399cb667475569e313bd688f5f19665c7e4)
  ([michael-grunder](https://github.com/michael-grunder))
- Fix double -> int truncation warning
  [152fdda9](https://github.com/phpredis/phpredis/commit/152fdda9b15fe5e60914f43fa34f64fd6e19d90d)
  ([michael-grunder](https://github.com/michael-grunder))
- Fix `SIGABRT` in PHP 8.4 with RedisArray
  [3c64b33f](https://github.com/phpredis/phpredis/commit/3c64b33ffe06a8929d61dd2b71ae5ea08014a455)
  ([Novynn](https://github.com/Novynn))
- Fixing segfault in cluster_update_slot
  [b0ba827b](https://github.com/phpredis/phpredis/commit/b0ba827be243eae701239fa4068c05249ada6b49)
  ([JacobBrownAustin](https://github.com/JacobBrownAustin))
- Fix typo
  [d0b0c5cf](https://github.com/phpredis/phpredis/commit/d0b0c5cfdde9d49a265ca4bf7184e3998863aed0)
  ([michael-grunder](https://github.com/michael-grunder))
- Fix compiling with PHP 8.5.0 alpha3 and newer
  [1e6f5477](https://github.com/phpredis/phpredis/commit/1e6f5477cb3728724eb26cd9bb1150701e68a31e)
  ([wyattoday](https://github.com/wyattoday))
- Fix error length calculation + UB sanity check
  [e73130fe](https://github.com/phpredis/phpredis/commit/e73130fee0c22a20e11ce1596579df3f6f826974)
  ([michael-grunder](https://github.com/michael-grunder))
- Fix arguments order for `SET` command
  [f73f5fcc](https://github.com/phpredis/phpredis/commit/f73f5fcce55ab9268c4eb40bf93cccdae418c1d2)
  ([yatsukhnenko](https://github.com/yatsukhnenko))

## Added

- Add `hgetwithmeta` method
  [7d3b2e4d](https://github.com/phpredis/phpredis/commit/7d3b2e4d)
  ([Pavlo Yatsukhnenko](https://github.com/yatsukhnenko))
- Add `maxRetries` to `redis_sock_configure`
  [e9e9e495](https://github.com/phpredis/phpredis/commit/e9e9e4950946cd92aab8d0aa14d9d84adce4a646)
  ([michael-grunder](https://github.com/michael-grunder))
- Implement vectorset commands
  [92dd256f](https://github.com/phpredis/phpredis/commit/92dd256f981be863fb3811c17aad60e5b270c862),
  [d80b7258](https://github.com/phpredis/phpredis/commit/d80b725824d5cb56a44910b761c82acaec265713),
  [7f9b1f41](https://github.com/phpredis/phpredis/commit/7f9b1f416ec96e3c34fb2b18a674cb28debcf8c5),
  [92716ed0](https://github.com/phpredis/phpredis/commit/92716ed0c5ccd199fe20575989d84c582832d01c),
  [dc91631b](https://github.com/phpredis/phpredis/commit/dc91631b3f6fe689fbf2de63da96f7a5ff350048),
  [1deca628](https://github.com/phpredis/phpredis/commit/1deca62841ab0b8df0699733166bf9394d5143e0),
  [96378b70](https://github.com/phpredis/phpredis/commit/96378b70fd03b571d1952022ec07f20307bf9fe6),
  [0fda9f29](https://github.com/phpredis/phpredis/commit/0fda9f293b81f271b0f33e36303cf4df3e9dcefe),
  [0ed0fc05](https://github.com/phpredis/phpredis/commit/0ed0fc0562eccbfd53b30d466c4076bab35bc39b),
  [d1d69005](https://github.com/phpredis/phpredis/commit/d1d690053f8a468a64628da1d53cfd3ad6155344),
  [286fa630](https://github.com/phpredis/phpredis/commit/286fa630649af95c5530f5df54cc74927bdf2383),
  [38115dec](https://github.com/phpredis/phpredis/commit/38115decb97ccdd96088077fcb447db9251667f0),
  [c4b2ea6c](https://github.com/phpredis/phpredis/commit/c4b2ea6ca532d4ef2c836f9b9c0701846c3bb202),
  [6ce3bd53](https://github.com/phpredis/phpredis/commit/6ce3bd53)
  ([michael-grunder](https://github.com/michael-grunder))
- Implement `GEOSEARCH[STORE]` `BYPOLYGON`
  [8d369f4d](https://github.com/phpredis/phpredis/commit/8d369f4d6263c8fe549d6eb89c52dc148843963d),
  [f24814a4](https://github.com/phpredis/phpredis/commit/f24814a42349e6bb1b2aff8fdae4422be1cbd732)
  ([michael-grunder](https://github.com/michael-grunder))
- Implement Valkey's `DELIFEQ` command
  [b1b0c191](https://github.com/phpredis/phpredis/commit/b1b0c1914232b2b8d94cd2147b12df4429ba10bc)
  ([michael-grunder](https://github.com/michael-grunder))
- Add an INI setting returning 5.x legacy behavior -- readonly session on lock failure
  [8dada174](https://github.com/phpredis/phpredis/commit/8dada174c4c0daf7de38d7be3ccbb5f71a94ad74)
  ([arokettu](https://github.com/arokettu))
- Implement several hash expiration commands
  [7350768c](https://github.com/phpredis/phpredis/commit/7350768cd9285b7d0c5c28742eabe52cfb1b326a)
  ([michael-grunder](https://github.com/michael-grunder))
- Introduce `Redis::serverName` and `Redis::serverVersion` methods
  [056c2dbe](https://github.com/phpredis/phpredis/commit/056c2dbee7f6379a9f546e46584ace59449847c7)
  [cbaf095f](https://github.com/phpredis/phpredis/commit/cbaf095ff708caf2728541bd627399a4058d0f19)
  [fa3eb006](https://github.com/phpredis/phpredis/commit/fa3eb00683a2c8d539b52c0738db6821c74fef54)
  ([yatsukhnenko](https://github.com/yatsukhnenko))
  ([michael-grunder](https://github.com/michael-grunder))
- New option 'database' for Redis class constructor
  [4f6a3ed1](https://github.com/phpredis/phpredis/commit/4f6a3ed1e71c70f80b631a9f53749e6a9fdb457a)
  ([JakubOnderka](https://github.com/JakubOnderka))
- Implement `HGETEX`, `HSETEX`, `HGETDEL`, and refactor `HMGET`
  [ce5b0fac](https://github.com/phpredis/phpredis/commit/ce5b0facc2520457f52a29d90b84ee1fa62936d1)
  ([michael-grunder](https://github.com/michael-grunder))
- Implement `GETDEL` for `RedisCluster`
  [d342e4ac](https://github.com/phpredis/phpredis/commit/d342e4ac18723607b001deb593c8d45e40bbc4c8)
  ([michael-grunder](https://github.com/michael-grunder))

## Changed

- Check for `dragonfly_version` in `HELLO` response
  [593ba012](https://github.com/phpredis/phpredis/commit/593ba012ac49065343f6bbf10adca5047414ce85)
  ([michael-grunder](https://github.com/michael-grunder))
- Simplify redis_unpack method calling
  [0a85bd82](https://github.com/phpredis/phpredis/commit/0a85bd824a1506d54abe3c48a3ad12c34429b00d)
  ([JakubOnderka](https://github.com/JakubOnderka))
- Refactor `getWithMeta` logic
  [0445e683](https://github.com/phpredis/phpredis/commit/0445e683e7552d60dbc82f21e0ee845911844651)
  ([michael-grunder](https://github.com/michael-grunder))
- cleanup session temp file
  [3828c929](https://github.com/phpredis/phpredis/commit/3828c9293b8fcd473e3c0b6d72c8db740b32bed8)
  ([remicollet](https://github.com/remicollet))
- Make execHello protected
  [300c5fb2](https://github.com/phpredis/phpredis/commit/300c5fb218ebb55fb6eca4de91756a91e57912ea)
  ([michael-grunder](https://github.com/michael-grunder))

## Documentation

- Update Relay sponsorship
  [7a69d73](https://github.com/phpredis/phpredis/commit/7a69d73)
  ([tillkruss](https://github.com/tillkruss))
- Fix markdown headlines
  [1d662f56](https://github.com/phpredis/phpredis/commit/1d662f56aaac82e9f4a480265666e49eb592d9f6)
  ([tillkruss](https://github.com/tillkruss))
- document PECL configure options
  [2066cfad](https://github.com/phpredis/phpredis/commit/2066cfad26db9e72295697485a0756df66dfe87c)
  ([tillkruss](https://github.com/tillkruss))
- Improve wording in README about project support
  [0ac01476](https://github.com/phpredis/phpredis/commit/0ac014764dfa6529ba9778c243f787d10ffc4506)
  ([tillkruss](https://github.com/tillkruss))
- Improve install instructions
  [c0076036](https://github.com/phpredis/phpredis/commit/c0076036d99d604d3a0772a5c6314847ad0d09e5)
  ([remicollet](https://github.com/remicollet))
- Clean up README.md: fix typos, spelling, and grammar errors
  [09cd4c54](https://github.com/phpredis/phpredis/commit/09cd4c54b97941cd0f2becd5cc3ca31a6cbc6b76)
  ([Copilot](https://github.com/apps/copilot-swe-agent))
- Refine parameter descriptions and examples in README
  [f9f609e1](https://github.com/phpredis/phpredis/commit/f9f609e1ebd5c0054e50d9f9e254b21f46a2c41d)
  ([joshtrichards](https://github.com/joshtrichards))
- Link to the correct header
  [d18db84c](https://github.com/phpredis/phpredis/commit/d18db84c68d4db907fe04e6531a5129c1fe97391)
  ([wyattoday](https://github.com/wyattoday))
- Update README.md about supported PHP versions.
  [b7a97e5e](https://github.com/phpredis/phpredis/commit/b7a97e5ec37ade2481f875295e45a2e1b6dd5366)
  ([AkameOuO](https://github.com/AkameOuO))
- Broaden return type for Redis::hGetAll
  [bfbab892](https://github.com/phpredis/phpredis/commit/bfbab8925878409d0f6614c17a597e74c30574a8)
  ([mgiuffrida](https://github.com/mgiuffrida))

## Tests/CI

- Add a regression test for `EVAL[SHA]`
  [22a2914b](https://github.com/phpredis/phpredis/commit/22a2914b09e927c0641da846b01fe54ebef50334)
  ([michael-grunder](https://github.com/michael-grunder))
- Rework `HEXPIRE` test inclusion + bump Valkey
  [b83981aa](https://github.com/phpredis/phpredis/commit/b83981aaeb6614754664353d8b4db22f5a631543)
  ([michael-grunder](https://github.com/michael-grunder))
- Add PHP 8.5 to CI
  [334937cb](https://github.com/phpredis/phpredis/commit/334937cb98adb557c31830e77e0c7ad36d8bdbac)
  ([yatsukhnenko](https://github.com/yatsukhnenko))
- Attempt to fix flaky GitHub CI tests
  [80140003](https://github.com/phpredis/phpredis/commit/801400036946676e48f975468f2e9c28d2c17027)
  ([michael-grunder](https://github.com/michael-grunder))
- Reorganize tests
  [807f806f](https://github.com/phpredis/phpredis/commit/807f806fe8a4df77691c869289db24358a684f7f)
  ([yatsukhnenko](https://github.com/yatsukhnenko))

## Internal/Performance

- Introduce `[[nodiscard]]` type attribute where supported.
  [2d963e79](https://github.com/phpredis/phpredis/commit/2d963e79)
  ([michael-grunder](https://github.com/michael-grunder))
- Fix typo (s/sees/seeds/)
  [25e6d5fc](https://github.com/phpredis/phpredis/commit/25e6d5fcc2b7599907ec9571a12c1d5c8f9c0dcb)
  ([xabbuh](https://github.com/xabbuh))
- Fix an unused variable warning
  [b48aa0d4](https://github.com/phpredis/phpredis/commit/b48aa0d471bf7280a1365fb5b4cb7595b5920498)
  ([michael-grunder](https://github.com/michael-grunder))
- Fix several issues surfaced by `gcc -fanalyze`
  [8be2306e](https://github.com/phpredis/phpredis/commit/8be2306e4f9192aaa167a5338673f3c348a2caf6)
  ([michael-grunder](https://github.com/michael-grunder))
- Fix dead assignment
  [d564e8cf](https://github.com/phpredis/phpredis/commit/d564e8cf3ce33c65898bc098173358b697fa459a) ([michael-grunder](https://github.com/michael-grunder))
- Refactor `redis_replicaof_cmd`
  [659dc763](https://github.com/phpredis/phpredis/commit/659dc763e0f69f526ff0a3404c1a113c9c55e9ea)
  ([michael-grunder](https://github.com/michael-grunder))
- Refactor several command handling macros into typesafe static functions
  [9802fc0e](https://github.com/phpredis/phpredis/commit/9802fc0e46a3746e7913807029c6edcf03be3ec5),
  [8f0931bb](https://github.com/phpredis/phpredis/commit/8f0931bbed9c68a9d9316ec2dbe36852235bbb04),
  [7c953d45](https://github.com/phpredis/phpredis/commit/7c953d458b8b30bc6bb6e7877a9b07ff5607300a),
  [950d2bc7](https://github.com/phpredis/phpredis/commit/950d2bc79dd328ae4d9b55a019b9d51a312813f5),
  [601ebbff](https://github.com/phpredis/phpredis/commit/601ebbff2b654d7f73d58a5cea275f08009fd4ee),
  [c3a71631](https://github.com/phpredis/phpredis/commit/c3a71631080d0e40a66b1362bc9113f4048a9aca),
  [b0040514](https://github.com/phpredis/phpredis/commit/b004051499ffd78cd4ca678e0a061cf279dedb6c),
  [fae89fa9](https://github.com/phpredis/phpredis/commit/fae89fa9927d5d1074da45ffe019822af215969e),
  [f880e1f7](https://github.com/phpredis/phpredis/commit/f880e1f7273a230bd462824533e03c35e49a57f4),
  [b90e27f2](https://github.com/phpredis/phpredis/commit/b90e27f2858276908803b6ac65103aeb89c254f8),
  [1db39089](https://github.com/phpredis/phpredis/commit/1db390891455e2faeb5a9fef4f3da9e18a2ba754)
  ([michael-grunder](https://github.com/michael-grunder))
- Refactor `EVAL[SHA]` command and add a regression test
  [6e5faf42](https://github.com/phpredis/phpredis/commit/6e5faf4226b6a0a33ce782ae04d5d395fe233f55)
  ([michael-grunder](https://github.com/michael-grunder))
- New macros REDIS_RESPONSE_ERROR and REDIS_RETURN_ZVAL
  [614b86e4](https://github.com/phpredis/phpredis/commit/614b86e457532f0cc3c6f41322740e6125949721)
  ([JakubOnderka](https://github.com/JakubOnderka))
- Remove wrapper macro which hides branching logic
  [58e1a04f](https://github.com/phpredis/phpredis/commit/58e1a04f76340b7add977b586d18bb1f8cb23bac)
  ([michael-grunder](https://github.com/michael-grunder))
- Use continue not break if we get a NULL node
  [8685c49c](https://github.com/phpredis/phpredis/commit/8685c49c7055d538f695cb3af1bb85b4145efad5)
  ([michael-grunder](https://github.com/michael-grunder))
- Remove pointless casts
  [03837f02](https://github.com/phpredis/phpredis/commit/03837f0230a0f00d88194e373ec89e77ec1c7bbe)
  ([michael-grunder](https://github.com/michael-grunder))
- Change `int` flags to `zend_bool` since we only use them as booleans
  [77691947](https://github.com/phpredis/phpredis/commit/77691947571921ced7170abcbdc0643ad7cf7c47)
  ([michael-grunder](https://github.com/michael-grunder))
- Remove unused macros + simplify some logic
  [75acbb09](https://github.com/phpredis/phpredis/commit/75acbb0984704c0e66919153d9485ac156d382be)
  ([michael-grunder](https://github.com/michael-grunder))

# [6.2.0] - 2025-03-24 ([Github](https://github.com/phpredis/phpredis/releases/6.2.0), [PECL](https://pecl.php.net/package/redis/6.2.0))

### Sponsors :sparkling_heart:

- [A-VISION](https://github.com/A-VISION-BV)
- [Avtandil Kikabidze](https://github.com/akalongman)
- [Geoffrey Hoffman](https://github.com/phpguru)
- [Object Cache Pro for WordPress](https://objectcache.pro/)
- [Open LMS](https://openlms.net/)
- [Salvatore Sanfilippo](https://github.com/antirez)
- [Ty Karok](https://github.com/karock)
- [Vanessa Santana](https://github.com/vanessa-dev)

  Special thanks to [Jakub Onderka](https://github.com/jakubonderka) for nearly two dozen performance improvements in this release!

## Fixed

- Fix arguments order for `SET` command
  [f73f5fc](https://github.com/phpredis/phpredis/commit/f73f5fcce55ab9268c4eb40bf93cccdae418c1d2)
  ([Pavlo Yatsukhnenko](https://github.com/yatsukhnenko))
- Fix error length calculation and UB sanity check
  [e73130fe](https://github.com/phpredis/phpredis/commit/e73130fee0c22a20e11ce1596579df3f6f826974)
  ([michael-grunder](https://github.com/michael-grunder))
- Invalidate slot cache on failed cluster connections
  [c7b87843](https://github.com/phpredis/phpredis/commit/c7b878431014789f35d2fb1834b95257ca6cbba5)
  ([James Kennedy](https://github.com/jkenn99))
- Don't cast a uint64_t to a long
  [faa4bc20](https://github.com/phpredis/phpredis/commit/faa4bc20868c76be4ecc4265015104a8adafccc4)
  ([michael-grunder](https://github.com/michael-grunder))
- Fix potential NULL dereference
  [43e6cab8](https://github.com/phpredis/phpredis/commit/43e6cab8792dc01580894d85600add9b68c27a42)
  ([peter15914](https://github.com/peter15914))
- Print cursor as unsigned 64 bit integer
  [138d07b6](https://github.com/phpredis/phpredis/commit/138d07b67c5537373834f1cae99804e092db1631)
  ([Bentley O'Kane-Chase](https://github.com/bentleyo))
- Fix XAUTOCLAIM argc when sending COUNT
  [0fe45d24](https://github.com/phpredis/phpredis/commit/0fe45d24d4d8c115a5b52846be072ecb9bb43329)
  ([michael-grunder](https://github.com/michael-grunder))

### Added

- Added `serverName()` and `serverVersion()` introspection methods
  [056c2dbe](https://github.com/phpredis/phpredis/commit/056c2dbee7f6379a9f546e46584ace59449847c7)
  [cbaf095f](https://github.com/phpredis/phpredis/commit/cbaf095ff708caf2728541bd627399a4058d0f19)
  [fa3eb006](https://github.com/phpredis/phpredis/commit/fa3eb00683a2c8d539b52c0738db6821c74fef54)
  ([Pavlo Yatsukhnenko](https://github.com/yatsukhnenko))
  ([michael-grunder](https://github.com/michael-grunder))
- Added `getWithMeta` method
  [9036ffca](https://github.com/phpredis/phpredis/commit/9036ffca)
  ([Pavlo Yatsukhnenko](https://github.com/yatsukhnenko))
- Implement `GETDEL` command for RedisCluster
  [d342e4ac](https://github.com/phpredis/phpredis/commit/d342e4ac18723607b001deb593c8d45e40bbc4c8)
  ([michael-grunder](https://github.com/michael-grunder))
- Introduce `Redis::OPT_PACK_IGNORE_NUMBERS` option
  [f9ce9429](https://github.com/phpredis/phpredis/commit/f9ce9429ef9f14a3de2c3fe1d68d02fb7440093d)
  [29e5cf0d](https://github.com/phpredis/phpredis/commit/29e5cf0d8c03069aa34c2a63322951fdf2c268c2)
  ([michael-grunder](https://github.com/michael-grunder))
- Implement Valkey >= 8.1 `IFEQ` `SET` option
  [a2eef77f](https://github.com/phpredis/phpredis/commit/a2eef77f4419cda815052e75def3af81b0ccd80f)
  ([michael-grunder](https://github.com/michael-grunder))
- Implement KeyDB's EXPIREMEMBER[AT] commands
  [4cd3f593](https://github.com/phpredis/phpredis/commit/4cd3f59356582a65aec1cceed44741bd5d161d9e)
  ([michael-grunder](https://github.com/michael-grunder))
- Set priority to 60 (for PIE installations)
  [9e504ede](https://github.com/phpredis/phpredis/commit/9e504ede34749326a39f997db6cc5c4201f6a9bc)
  ([Pavlo Yatsukhnenko](https://github.com/yatsukhnenko))

### Documentation

- Fix phpdoc type of `$pattern`
  [5cad2076](https://github.com/phpredis/phpredis/commit/5cad20763710d44f8efb8e537f8f84a812935604)
  ([OHZEKI Naoki](https://github.com/zeek0x))
- Better documentation for the `$tlsOptions` parameter of RedisCluster
  [8144db37](https://github.com/phpredis/phpredis/commit/8144db374338006a316beb11549f37926bd40c5d)
  ([Jacob Brown](https://github.com/JacobBrownAustin))

### Tests/CI

- Reorganize tests
  [807f806f](https://github.com/phpredis/phpredis/commit/807f806f)
  ([Pavlo Yatsukhnenko](https://github.com/yatsukhnenko))
- Add details to the option doc block
  [abb0f6cc](https://github.com/phpredis/phpredis/commit/abb0f6ccc827f240a1de53633225abbc2848fc3a)
  ([michael-grunder](https://github.com/michael-grunder))
- Update CodeQL to v3
  [41e11417](https://github.com/phpredis/phpredis/commit/41e114177a20a03e3013db2a3b90980a1f4f1635)
  [a10bca35](https://github.com/phpredis/phpredis/commit/a10bca35bba32bb969cc1e473564695d3f8a8811)
  ([Pavlo Yatsukhnenko](https://github.com/yatsukhnenko))
- Add PHP 8.4 to CI
  [6097e7ba](https://github.com/phpredis/phpredis/commit/6097e7ba50c0a300bc4f420f84c5d2665ef99d90)
  ([Pavlo Yatsukhnenko](https://github.com/yatsukhnenko))
- Pin ubuntu version for KeyDB
  [eb66fc9e](https://github.com/phpredis/phpredis/commit/eb66fc9e2fe60f13e5980ea2ecbe9457ca5ae8b4)
  [985b0313](https://github.com/phpredis/phpredis/commit/985b0313fb664c9776c3d2c84e778ddd6733728e)
  ([michael-grunder](https://github.com/michael-grunder))
- Windows CI: update setup-php-sdk to v0.10 and enable caching
  [f89d4d8f](https://github.com/phpredis/phpredis/commit/f89d4d8f6eecbe223e158651ffffd77ffa27449b)
  ([Christoph M. Becker](https://github.com/cmb69))

### Internal/Performance

- Reduce buffer size for signed integer
  [044b3038](https://github.com/phpredis/phpredis/commit/044b30386f0418e9ed2a2bbc3b79582520d008d8)
  [35c59880](https://github.com/phpredis/phpredis/commit/35c5988027eda663167a64decde4512957cae738)
  ([Bentley O'Kane-Chase](https://github.com/bentleyo))
- Create a strncmp wrapper
  [085d61ec](https://github.com/phpredis/phpredis/commit/085d61ecfb0d484832547b46343a2e4b275a372e)
  ([michael-grunder](https://github.com/michael-grunder))
- Refactor and avoid allocation in rawcommand method
  [f68544f7](https://github.com/phpredis/phpredis/commit/f68544f70385e1d431fb0245fafe30b39ee7479a)
  ([JakubOnderka](https://github.com/JakubOnderka))
- Switch from linked list to growing array for reply callbacks
  [a551fdc9](https://github.com/phpredis/phpredis/commit/a551fdc94c14d7974f2303cd558f7bd3e0fd91d6)
  [42a42769](https://github.com/phpredis/phpredis/commit/42a427695e89577a1f1a554dba268527f3995708)
  ([JakubOnderka](https://github.com/JakubOnderka))
  ([michael-grunder](https://github.com/michael-grunder))
- Reuse redis_sock_append_auth method
  [be388562](https://github.com/phpredis/phpredis/commit/be388562058a75ed8fd31926bb0e6a60e2d8cb08)
  ([JakubOnderka](https://github.com/JakubOnderka))
- Switch pipeline_cmd from smart_str to smart_string
  [571ffbc8](https://github.com/phpredis/phpredis/commit/571ffbc8e0a5da807a6cc4a2cc5aa90af72e23b0)
  ([JakubOnderka](https://github.com/JakubOnderka))
- Remove unused redis_debug_response method from library.c
  [7895636a](https://github.com/phpredis/phpredis/commit/7895636a3a7cd3cad396a83ebe3aa5fe0208f42d)
  ([JakubOnderka](https://github.com/JakubOnderka))
- Optimise HMGET method
  [2434ba29](https://github.com/phpredis/phpredis/commit/2434ba294cbb3b2f5b4ee581c37056906902d0d9)
  ([JakubOnderka](https://github.com/JakubOnderka))
- Avoid unnecessary allocation in redis_hset_cmd
  [aba09933](https://github.com/phpredis/phpredis/commit/aba09933db05a1a36e947c6fa9dca9889c6a77ff)
  ([JakubOnderka](https://github.com/JakubOnderka))
- Avoid unnecessary allocation in redis_hdel_cmd
  [4082dd07](https://github.com/phpredis/phpredis/commit/4082dd07f714fd2f6a0918b1845eb46c403a9edd)
  ([JakubOnderka](https://github.com/JakubOnderka))
- Avoid unnecessary allocation in redis_key_varval_cmd
  [99650e15](https://github.com/phpredis/phpredis/commit/99650e15453f03b5dd99284548514551fde4c812)
  ([JakubOnderka](https://github.com/JakubOnderka))
- Use zval_get_tmp_string method that is faster when provided zval is string
  [f6906470](https://github.com/phpredis/phpredis/commit/f6906470a52e2d24b1e1b9f2574726643edd7a64)
  ([JakubOnderka](https://github.com/JakubOnderka))
- Optimise constructing Redis command string
  [2a2f908f](https://github.com/phpredis/phpredis/commit/2a2f908f2b6b695a0e6705200160e592802f0e41)
  ([JakubOnderka](https://github.com/JakubOnderka))
- If no command is issued in multi mode, return immutable empty array
  [5156e032](https://github.com/phpredis/phpredis/commit/5156e0320242ff05f327a3801667140069688c0e)
  ([JakubOnderka](https://github.com/JakubOnderka))
- Test for empty pipeline and multi
  [426de2bb](https://github.com/phpredis/phpredis/commit/426de2bb71372f665f5a5bb5a779a7b9c586892d)
  ([JakubOnderka](https://github.com/JakubOnderka))
- Optimise method array_zip_values_and_scores
  [400503b8](https://github.com/phpredis/phpredis/commit/400503b8718104b766ceb4a0b84e4a446dbee09b)
  ([JakubOnderka](https://github.com/JakubOnderka))
- Faster parameter parsing in redis_key_cmd and redis_key_long_val_cmd
  [83a19656](https://github.com/phpredis/phpredis/commit/83a19656f49aec8f354596099dbf97ba7375d7af)
  ([JakubOnderka](https://github.com/JakubOnderka))
- Use immutable empty array in Redis::hKeys
  [3a2f3f45](https://github.com/phpredis/phpredis/commit/3a2f3f45fc7bb01d1be2b9d97cf9d8bff0b0e818)
  ([JakubOnderka](https://github.com/JakubOnderka))
- Use immutable empty array in Redis::exec
  [60b5a886](https://github.com/phpredis/phpredis/commit/60b5a8860ae3ff2d02d7f06cc6f86b59cb53b2cf)
  ([JakubOnderka](https://github.com/JakubOnderka))
- Do not allocate empty string or string with one character
  [64da891e](https://github.com/phpredis/phpredis/commit/64da891e6fe5810b1aa2a47bc0632a2cd346659d)
  ([JakubOnderka](https://github.com/JakubOnderka))
- Initialize arrays with known size
  [99beb922](https://github.com/phpredis/phpredis/commit/99beb9221c815018f1d076654b033cafac22a6ce)
  ([JakubOnderka](https://github.com/JakubOnderka))
- Use smart str for constructing pipeline cmd
  [b665925e](https://github.com/phpredis/phpredis/commit/b665925eeddfdf6a6fc1de471c0789ffb60cd067)
  ([JakubOnderka](https://github.com/JakubOnderka))

## [6.1.0] - 2024-10-04 ([Github](https://github.com/phpredis/phpredis/releases/6.1.0), [PECL](https://pecl.php.net/package/redis/6.1.0))

**NOTE**: There were no changes to C code between 6.1.0RC2 and 6.1.0.

### Documentation

- Update package.xml to make it clearer that we support many key-value stores
  [52e69ede](https://github.com/phpredis/phpredis/commit/52e69ede)
  ([Remi Collet](https://github.com/remicollet))
- Fix redis.io urls
  [0bae4bb0](https://github.com/phpredis/phpredis/commit/0bae4bb0)
  ([Vincent Langlet](https://github.com/VincentLanglet))

### Tests/CI

- Fix 2 tests with redis 6.2
  [cc1be322](https://github.com/phpredis/phpredis/commit/cc1be322)
  ([Remi Collet](https://github.com/remicollet))

### Sponsors :sparkling_heart:

- [A-VISION](https://github.com/A-VISION-BV)
- [Open LMS](https://openlms.net/)
- [Avtandil Kikabidze](https://github.com/akalongman)
- [Ty Karok](https://github.com/karock)
- [Object Cache Pro for WordPress](https://objectcache.pro/)

### Contributors to this release :sparkling_heart:

  [@michael-grunder](https://github.com/michael-grunder),
  [@yatsukhnenko](https://github.com/yatsukhnenko),
  [@bitactive](https://github.com/bitactive),
  [@OrangeJuiced](https://github.com/OrangeJuiced),
  [@crocodele](https://github.com/crocodele),
  [@kalifg](https://github.com/kalifg),
  [@divinity76](https://github.com/divinity76),
  [@PlavorSeol](https://github.com/PlavorSeol),
  [@kjoe](https://github.com/kjoe),
  [@tstarling](https://github.com/tstarling),
  [@acorncom](https://github.com/acorncom),
  [@tuxmartin](https://github.com/tuxmartin),
  [@BenMorel](https://github.com/BenMorel),
  [@szepeviktor](https://github.com/szepeviktor),
  [@SplotyCode](https://github.com/SplotyCode),
  [@taka-oyama](https://github.com/taka-oyama),
  [@PROFeNoM](https://github.com/PROFeNoM),
  [@woodongwong](https://github.com/woodongwong),
  [@RobiNN1](https://github.com/RobiNN1),
  [@vtsykun](https://github.com/vtsykun),
  [@solracsf](https://github.com/solracsf),
  [@tillkruss](https://github.com/tillkruss),
  [@deiga](https://github.com/deiga),
  [@tutuna](https://github.com/tutuna)
  [@VincentLanglet](https://github.com/VincentLanglet)


## [6.1.0RC2] - 2024-09-23 ([Github](https://github.com/phpredis/phpredis/releases/6.1.0RC2), [PECL](https://pecl.php.net/package/redis/6.1.0RC2))

### Fixed

- Fixed a `SIGABRT` error in PHP 8.4
  [a75a7e5a](https://github.com/phpredis/phpredis/commit/a75a7e5a)
  ([Michael Grunder](https://github.com/michael-grunder))
- Clean up code for unsupported versions of PHP
  [37cebdd7](https://github.com/phpredis/phpredis/commit/37cebdd7)
  ([Remi Collet](https://github.com/remicollet))
- Add `SessionHelpers.php` to `package.xml`
  [e9474b80](https://github.com/phpredis/phpredis/commit/e9474b80)
  ([Remi Collet](https://github.com/remicollet))
- 8.4 implicit null fix, bump version
  [bff3a22e](https://github.com/phpredis/phpredis/commit/bff3a22e)
  [30c8f90c](https://github.com/phpredis/phpredis/commit/30c8f90c)
  ([Remi Collet](https://github.com/remicollet))

### Changed

- Raised minimum supported PHP version to 7.4
  [8b519423](https://github.com/phpredis/phpredis/commit/8b519423)
  ([Michael Grunder](https://github.com/michael-grunder))

### Removed

- Removed erroneously duplicated changelog entries
  [40c89736](https://github.com/phpredis/phpredis/commit/40c89736)
  ([Michael Grunder](https://github.com/michael-grunder))

### Tests/CI

- Move to upload artifacts v4
  [9d3805009](https://github.com/phpredis/phpredis/commit/9d3805009)
  ([Michael Grunder](https://github.com/michael-grunder))

### Added

- Added `composer.json` to support [PIE](https://github.com/php/pie) (PHP Installer for Extensions)
  [b59e35a6](https://github.com/phpredis/phpredis/commit/b59e35a6)
  ([James Titcumb](https://github.com/asgrim))

## [6.1.0RC1] - 2024-08-04 ([GitHub](https://github.com/phpredis/phpredis/releases/6.1.0RC1), [PECL](https://pecl.php.net/package/redis/6.1.0RC1))

### Fixed

- Fix random connection timeouts with Redis Cluster.
  [eb7f31e7](https://github.com/phpredis/phpredis/commit/eb7f31e7)
  ([Jozsef Koszo](https://github.com/kjoe))
  [#1142](https://github.com/phpredis/phpredis/pull/1142)
  [#1385](https://github.com/phpredis/phpredis/pull/1385)
  [#1633](https://github.com/phpredis/phpredis/pull/1633)
  [#1707](https://github.com/phpredis/phpredis/pull/1707)
  [#1811](https://github.com/phpredis/phpredis/pull/1811)
  [#2407](https://github.com/phpredis/phpredis/pull/2407)
- Fix argument count issue in HSET with associative array
  [6ea5b3e0](https://github.com/phpredis/phpredis/commit/6ea5b3e0)
  ([Viktor Djupsjöbacka](https://github.com/crocodele))
- SRANDMEMBER can return any type because of serialization.
  [6673b5b2](https://github.com/phpredis/phpredis/commit/6673b5b2)
  ([michael-grunder](https://github.com/michael-grunder))
- Fix HRANDFIELD command when WITHVALUES is used.
  [99f9fd83](https://github.com/phpredis/phpredis/commit/99f9fd83)
  ([Michael Grunder](https://github.com/michael-grunder))
  [#2524](https://github.com/phpredis/phpredis/pull/2524)
- Allow context array to be nullable
  [50529f56](https://github.com/phpredis/phpredis/commit/50529f56)
  ([michael-grunder](https://github.com/michael-grunder))
  [#2521](https://github.com/phpredis/phpredis/pull/2521)
- Fix a macOS (M1) compiler warning.
  [7de29d57](https://github.com/phpredis/phpredis/commit/7de29d57)
  ([michael-grunder](https://github.com/michael-grunder))
- `GETEX` documentation/updates and implentation in `RedisCluster`
  [981c6931](https://github.com/phpredis/phpredis/commit/981c6931)
  ([michael-grunder](https://github.com/michael-grunder))
  [#2512](https://github.com/phpredis/phpredis/pull/2512)
- Refactor redis_script_cmd and fix to `flush` subcommand.
  [7c551424](https://github.com/phpredis/phpredis/commit/7c551424)
  ([Pavlo Yatsukhnenko](https://github.com/yatsukhnenko))
- Update liveness check and fix PHP 8.4 compilation error.
  [c139de3a](https://github.com/phpredis/phpredis/commit/c139de3a)
  ([michael-grunder](https://github.com/michael-grunder))
- Rework how we declare ZSTD min/max constants.
  [34b5bd81](https://github.com/phpredis/phpredis/commit/34b5bd81)
  ([michael-grunder](https://github.com/michael-grunder))
  [#2487](https://github.com/phpredis/phpredis/pull/2487)
- Fix memory leak if we fail in ps_open_redis.
  [0e926165](https://github.com/phpredis/phpredis/commit/0e926165)
  ([michael-grunder](https://github.com/michael-grunder))
- Fix segfault and remove redundant macros
  [a9e53fd1](https://github.com/phpredis/phpredis/commit/a9e53fd1)
  ([Pavlo Yatsukhnenko](https://github.com/yatsukhnenko))
- Fix PHP 8.4 includes
  [a51215ce](https://github.com/phpredis/phpredis/commit/a51215ce)
  [#2463](https://github.com/phpredis/phpredis/pull/2463)
  ([michael-grunder](https://github.com/michael-grunder))
- Handle arbitrarily large `SCAN` cursors properly.
  [2612d444](https://github.com/phpredis/phpredis/commit/2612d444)
  [e52f0afa](https://github.com/phpredis/phpredis/commit/e52f0afa)
  [#2454](https://github.com/phpredis/phpredis/pull/2454)
  [#2458](https://github.com/phpredis/phpredis/pull/2458)
  ([michael-grunder](https://github.com/michael-grunder))
- Improve warning when we encounter an invalid EXPIRY in SET
  [732e466a](https://github.com/phpredis/phpredis/commit/732e466a)
  [#2448](https://github.com/phpredis/phpredis/pull/2448)
  ([michael-grunder](https://github.com/michael-grunder))
- Fix Arginfo / zpp mismatch for DUMP command
  [50e5405c](https://github.com/phpredis/phpredis/commit/50e5405c)
  ([Pavlo Yatsukhnenko](https://github.com/yatsukhnenko))
- RedisCluster::publish returns a cluster_long_resp
  [14f93339](https://github.com/phpredis/phpredis/commit/14f93339)
  ([Alexandre Choura](https://github.com/PROFeNoM))
- Fix segfault when passing just false to auth.
  [6dc0a0be](https://github.com/phpredis/phpredis/commit/6dc0a0be)
  [#2430](https://github.com/phpredis/phpredis/pull/2430)
  ([michael-grunder](https://github.com/michael-grunder))
- the VALUE argument type for hSetNx must be the same as for hSet
  [df074dbe](https://github.com/phpredis/phpredis/commit/df074dbe)
  ([Uładzimir Tsykun](https://github.com/vtsykun))
- Other fixes
  [e18f6c6d](https://github.com/phpredis/phpredis/commit/e18f6c6d)
  [3d7be358](https://github.com/phpredis/phpredis/commit/3d7be358)
  [2b555c89](https://github.com/phpredis/phpredis/commit/2b555c89)
  [fa1a283a](https://github.com/phpredis/phpredis/commit/fa1a283a)
  ([michael-grunder](https://github.com/michael-grunder))
  [37c5f8d4](https://github.com/phpredis/phpredis/commit/37c5f8d4)
  ([Viktor Szépe](https://github.com/szepeviktor))

### Added

- Compression support for PHP sessions.
  [da4ab0a7](https://github.com/phpredis/phpredis/commit/da4ab0a7)
  [#2473](https://github.com/phpredis/phpredis/pull/2473)
  ([bitactive](https://github.com/bitactive))
- Support for early_refresh in Redis sessions to match cluster behavior
  [b6989018](https://github.com/phpredis/phpredis/commit/b6989018)
  ([Bitactive](https://github.com/bitactive))
- Implement WAITAOF command.
  [ed7c9f6f](https://github.com/phpredis/phpredis/commit/ed7c9f6f)
  ([michael-grunder](https://github.com/michael-grunder))

### Removed

- PHP 7.1, 7.2, and 7.3 CI jobs
  [d68c30f8](https://github.com/phpredis/phpredis/commit/d68c30f8)
  [dc39bd55](https://github.com/phpredis/phpredis/commit/dc39bd55)
  [#2478](https://github.com/phpredis/phpredis/pull/2478)
  ([Michael Grunder](https://github.com/michael-grunder))

### Changed

- Fix the time unit of retry_interval
  [3fdd52b4](https://github.com/phpredis/phpredis/commit/3fdd52b4)
  ([woodong](https://github.com/woodongwong))

### Documentation

- Many documentation fixes.
  [eeb51099](https://github.com/phpredis/phpredis/commit/eeb51099)
  ([Michael Dwyer](https://github.com/kalifg))
  [#2523](https://github.com/phpredis/phpredis/pull/2523)
- fix missing \<code\> tags
  [f865d5b9](https://github.com/phpredis/phpredis/commit/f865d5b9)
  ([divinity76](https://github.com/divinity76))
- Mention Valkey support
  [5f1eecfb](https://github.com/phpredis/phpredis/commit/5f1eecfb)
  ([PlavorSeol](https://github.com/PlavorSeol))
- Mention KeyDB support in README.md
  [37fa3592](https://github.com/phpredis/phpredis/commit/37fa3592)
  ([Tim Starling](https://github.com/tstarling))
- Remove mention of pickle
  [c7a73abb](https://github.com/phpredis/phpredis/commit/c7a73abb)
  ([David Baker](https://github.com/acorncom))
- Add session.save_path examples
  [8a39caeb](https://github.com/phpredis/phpredis/commit/8a39caeb)
  ([Martin Vancl](https://github.com/tuxmartin))
- Tighter return types for Redis::(keys|hKeys|hVals|hGetAll)()
  [77ab62bc](https://github.com/phpredis/phpredis/commit/77ab62bc)
  ([Benjamin Morel](https://github.com/BenMorel))
- Update stubs
  [4d233977](https://github.com/phpredis/phpredis/commit/4d233977)
  [ff305349](https://github.com/phpredis/phpredis/commit/ff305349)
  [12966a74](https://github.com/phpredis/phpredis/commit/12966a74)
  [a4a283ab](https://github.com/phpredis/phpredis/commit/a4a283ab)
  ([michael-grunder](https://github.com/michael-grunder))
  [8f8ff72a](https://github.com/phpredis/phpredis/commit/8f8ff72a)
  ([Takayasu Oyama](https://github.com/taka-oyama))
  [5d293245](https://github.com/phpredis/phpredis/commit/5d293245)
- Fix config.m4 when using custom dep paths
  [ece3f7be](https://github.com/phpredis/phpredis/commit/ece3f7be)
  ([Michael Grunder](https://github.com/michael-grunder))
  [#2453](https://github.com/phpredis/phpredis/pull/2453)
  [#2452](https://github.com/phpredis/phpredis/pull/2452)
- Fix retry_internal documentation
  [142c1f4a](https://github.com/phpredis/phpredis/commit/142c1f4a)
  ([SplotyCode](https://github.com/SplotyCode))
- Fix anchor link
  [9b5cad31](https://github.com/phpredis/phpredis/commit/9b5cad31)
  ([Git'Fellow](https://github.com/solracsf))
- Fix typo in link
  [bfd379f0](https://github.com/phpredis/phpredis/commit/bfd379f0)
  [#2349](https://github.com/phpredis/phpredis/pull/2349)
  ([deiga](https://github.com/deiga))
- Fix Fedora package url
  [60b1ba14](https://github.com/phpredis/phpredis/commit/60b1ba14)
  [717713e1](https://github.com/phpredis/phpredis/commit/717713e1)
  ([Dmitrii Kotov](https://github.com/tutunak))
- Update Redis Sentinel documentation to reflect changes to constructor in 6.0 release
  [dc05d65c](https://github.com/phpredis/phpredis/commit/dc05d65c)
  ([Pavlo Yatsukhnenko](https://github.com/yatsukhnenko))
  [#2381](https://github.com/phpredis/phpredis/pull/2381)

### Tests/CI

- Avoid fatal error in test execution.
  [57304970](https://github.com/phpredis/phpredis/commit/57304970)
  ([Michael Grunder](https://github.com/michael-grunder))
  [#2510](https://github.com/phpredis/phpredis/pull/2510)
- Refactor unit test framework.
  [b1771def](https://github.com/phpredis/phpredis/commit/b1771def)
  ([Michael Grunder](https://github.com/michael-grunder))
  [#2509](https://github.com/phpredis/phpredis/pull/2509)
- Get unit tests working in `php-cgi`.
  [b808cc60](https://github.com/phpredis/phpredis/commit/b808cc60)
  ([michael-grunder](https://github.com/michael-grunder))
  [#2507](https://github.com/phpredis/phpredis/pull/2507)
- Switch to `ZEND_STRL` in more places.
  [7050c989](https://github.com/phpredis/phpredis/commit/7050c989)
  [f8c762e7](https://github.com/phpredis/phpredis/commit/f8c762e7)
  ([Michael Grunder](https://github.com/michael-grunder))
  [#2505](https://github.com/phpredis/phpredis/pull/2505)
- Workaround weird PHP compiler crash.
  [d3b2d87b](https://github.com/phpredis/phpredis/commit/d3b2d87b)
  ([michael-grunder](https://github.com/michael-grunder))
- Refactor tests (formatting, modernization, etc).
  [dab6a62d](https://github.com/phpredis/phpredis/commit/dab6a62d)
  [c6cd665b](https://github.com/phpredis/phpredis/commit/c6cd665b)
  [78b70ca8](https://github.com/phpredis/phpredis/commit/78b70ca8)
  [3c125b09](https://github.com/phpredis/phpredis/commit/3c125b09)
  [18b0da72](https://github.com/phpredis/phpredis/commit/18b0da72)
  [b88e72b1](https://github.com/phpredis/phpredis/commit/b88e72b1)
  [#2492](https://github.com/phpredis/phpredis/pull/2492)
  [0f94d9c1](https://github.com/phpredis/phpredis/commit/0f94d9c1)
  [59965971](https://github.com/phpredis/phpredis/commit/59965971)
  [3dbc2bd8](https://github.com/phpredis/phpredis/commit/3dbc2bd8)
  [9b90c03b](https://github.com/phpredis/phpredis/commit/9b90c03b)
  [c0d6f042](https://github.com/phpredis/phpredis/commit/c0d6f042)
  ([michael-grunder](https://github.com/michael-grunder))
- Spelling fixes
  [0d89e928](https://github.com/phpredis/phpredis/commit/0d89e928)
  ([michael-grunder](https://github.com/michael-grunder))
- Added Valkey support.
  [f350dc34](https://github.com/phpredis/phpredis/commit/f350dc34)
  ([michael-grunder](https://github.com/michael-grunder))
- Add a test for session compression.
  [9f3ca98c](https://github.com/phpredis/phpredis/commit/9f3ca98c)
  ([michael-grunder](https://github.com/michael-grunder))
  [#2473](https://github.com/phpredis/phpredis/pull/2473)
  [#2480](https://github.com/phpredis/phpredis/pull/2480)
- Test against valkey
  [a819a44b](https://github.com/phpredis/phpredis/commit/a819a44b)
  ([michael-grunder](https://github.com/michael-grunder))
- sessionSaveHandler injection.
  [9f8f80ca](https://github.com/phpredis/phpredis/commit/9f8f80ca)
  ([Pavlo Yatsukhnenko](https://github.com/yatsukhnenko))
- KeyDB addiions
  [54d62c72](https://github.com/phpredis/phpredis/commit/54d62c72)
  [d9c48b78](https://github.com/phpredis/phpredis/commit/d9c48b78)
  [#2466](https://github.com/phpredis/phpredis/pull/2466)
  ([michael-grunder](https://github.com/michael-grunder))
- Add PHP 8.3 to CI
  [78d15140](https://github.com/phpredis/phpredis/commit/78d15140)
  ([Róbert Kelčák](https://github.com/RobiNN1))
  [e051a5db](https://github.com/phpredis/phpredis/commit/e051a5db)
  [#2427](https://github.com/phpredis/phpredis/pull/2427)
  ([Pavlo Yatsukhnenko](https://github.com/yatsukhnenko))
- Use newInstance in RedisClusterTest
  [954fbab8](https://github.com/phpredis/phpredis/commit/954fbab8)
  ([Pavlo Yatsukhnenko](https://github.com/yatsukhnenko))
- Use actions/checkout@v4
  [f4c2ac26](https://github.com/phpredis/phpredis/commit/f4c2ac26)
  ([Pavlo Yatsukhnenko](https://github.com/yatsukhnenko))
- Cluster nodes from ENV
  [eda39958](https://github.com/phpredis/phpredis/commit/eda39958)
  [0672703b](https://github.com/phpredis/phpredis/commit/0672703b)
  ([Pavlo Yatsukhnenko](https://github.com/yatsukhnenko))
- Ensure we're talking to redis-server in our high ports test.
  [7825efbc](https://github.com/phpredis/phpredis/commit/7825efbc)
  ([michael-grunder](https://github.com/michael-grunder))
- Add missing option to installation example
  [2bddd84f](https://github.com/phpredis/phpredis/commit/2bddd84f)
  ([Pavlo Yatsukhnenko](https://github.com/yatsukhnenko))
  [#2378](https://github.com/phpredis/phpredis/pull/2378)
- Fix typo in link
  [8f6bc98f](https://github.com/phpredis/phpredis/commit/8f6bc98f)
  ([Timo Sand](https://github.com/deiga))
- Update tests to allow users to use a custom class.
  [5f6ce414](https://github.com/phpredis/phpredis/commit/5f6ce414)
  ([michael-grunder](https://github.com/michael-grunder))


## [6.0.2] - 2023-10-22 ([GitHub](https://github.com/phpredis/phpredis/releases/6.0.2), [PECL](https://pecl.php.net/package/redis/6.0.2))

### Sponsors :sparkling_heart:

- [Audiomack](https://audiomack.com)
- [Open LMS](https://openlms.net/)
- [BlueHost](https://bluehost.com)
- [Object Cache Pro for WordPress](https://objectcache.pro/)
- [Avtandil Kikabidze](https://github.com/akalongman)
- [Zaher Ghaibeh](https://github.com/zaherg)
- [BatchLabs](https://batch.com)
- [Stackhero](https://github.com/stackhero-io)
- [Florian Levis](https://github.com/Gounlaf)
- [Luis Zárate](https://github.com/jlzaratec)

### Fixed
- Fix deprecation error when passing null to match_type parameter.
  [b835aaa3](https://github.com/phpredis/phpredis/commit/b835aaa3)
  ([Pavlo Yatsukhnenko](https://github.com/yatsukhnenko))
- Fix flaky test and OBJECT in a pipeline.
  [a7f51f70](https://github.com/phpredis/phpredis/commit/a7f51f70)
  ([Michael Grunder](https://github.com/michael-grunder))
- Find our callback by pattern with PSUBSCRIBE
  [2f276dcd](https://github.com/phpredis/phpredis/commit/2f276dcd)
  ([Michael Grunder](https://github.com/michael-grunder))

## [6.0.1] - 2023-09-23 ([GitHub](https://github.com/phpredis/phpredis/releases/6.0.1), [PECL](https://pecl.php.net/package/redis/6.0.1))

### Sponsors :sparkling_heart:

- [Audiomack](https://audiomack.com)
- [Open LMS](https://openlms.net/)
- [BlueHost](https://bluehost.com)
- [Object Cache Pro for WordPress](https://objectcache.pro/)
- [Avtandil Kikabidze](https://github.com/akalongman)
- [Zaher Ghaibeh](https://github.com/zaherg)
- [BatchLabs](https://batch.com)
- [Stackhero](https://github.com/stackhero-io)
- [Florian Levis](https://github.com/Gounlaf)
- [Luis Zárate](https://github.com/jlzaratec)

### Fixed
- Fix memory leak and segfault in Redis::exec
  [362e1141](https://github.com/phpredis/phpredis/commit/362e1141)
  ([Pavlo Yatsukhnenko](https://github.com/yatsukhnenko)),
  ([Markus Podar](https://github.com/mfn))
- Fix unknown expiration modifier
  [264c0c7e](https://github.com/phpredis/phpredis/commit/264c0c7e),
  [95bd184b](https://github.com/phpredis/phpredis/commit/95bd184b)
  ([Pavlo Yatsukhnenko](https://github.com/yatsukhnenko))

### Added
- Add missing option to exampleA
  [3674d663](https://github.com/phpredis/phpredis/commit/3674d663)
  ([Till Krüss](https://github.com/tillkruss))
- Update sentinel documentation
  [849bedb6](https://github.com/phpredis/phpredis/commit/849bedb6),
  [1ad95b63](https://github.com/phpredis/phpredis/commit/1ad95b63)
  ([Joost OrangeJuiced](https://github.com/OrangeJuiced))

## [6.0.0] - 2023-09-09 ([GitHub](https://github.com/phpredis/phpredis/releases/6.0.0), [PECL](https://pecl.php.net/package/redis/6.0.0))

### Sponsors :sparkling_heart:

- [Audiomack](https://audiomack.com)
- [Open LMS](https://openlms.net/)
- [BlueHost](https://bluehost.com)
- [Object Cache Pro for WordPress](https://objectcache.pro/)
- [Avtandil Kikabidze](https://github.com/akalongman)
- [Zaher Ghaibeh](https://github.com/zaherg)
- [BatchLabs](https://batch.com)
- [Stackhero](https://github.com/stackhero-io)
- [Florian Levis](https://github.com/Gounlaf)
- [Luis Zárate](https://github.com/jlzaratec)

*There were no changes between 6.0.0 and 6.0.0RC2*

## [6.0.0RC2] - 2023-08-20 ([GitHub](https://github.com/phpredis/phpredis/releases/6.0.0RC2), [PECL](https://pecl.php.net/package/redis/6.0.0RC2))

### Sponsors :sparkling_heart:

- [Audiomack](https://audiomack.com)
- [Open LMS](https://openlms.net/)
- [BlueHost](https://bluehost.com)
- [Object Cache Pro for WordPress](https://objectcache.pro/)
- [Avtandil Kikabidze](https://github.com/akalongman)
- [Zaher Ghaibeh](https://github.com/zaherg)
- [BatchLabs](https://batch.com)
- [Stackhero](https://github.com/stackhero-io)
- [Florian Levis](https://github.com/Gounlaf)
- [Luis Zárate](https://github.com/jlzaratec)

### Fixed

- Fix arginfo for arguments that default to null
  [8d99b7d1](https://github.com/phpredis/phpredis/commit/8d99b7d1)
  ([Nicolas Grekas](https://github.com/nicolas-grekas))
- Fix C99 usages
  [54d9ca45](https://github.com/phpredis/phpredis/commit/54d9ca45)
  ([Remi Collet](https://github.com/remicollet))
- Raise minimal supported version to 7.2
  [e10b9a85](https://github.com/phpredis/phpredis/commit/e10b9a85)
  ([Remi Collet](https://github.com/remicollet))

## [6.0.0RC1] - 2023-08-01 ([GitHub](https://github.com/phpredis/phpredis/releases/6.0.0RC1), [PECL](https://pecl.php.net/package/redis/6.0.0RC1))

### Sponsors :sparkling_heart:

- [Audiomack](https://audiomack.com)
- [Open LMS](https://openlms.net/)
- [BlueHost](https://bluehost.com)
- [Object Cache Pro for WordPress](https://objectcache.pro/)
- [Avtandil Kikabidze](https://github.com/akalongman)
- [Zaher Ghaibeh](https://github.com/zaherg)
- [BatchLabs](https://batch.com)
- [Stackhero](https://github.com/stackhero-io)
- [Florian Levis](https://github.com/Gounlaf)
- [Luis Zárate](https://github.com/jlzaratec)

### Fixed

- Fix restoring keys when using compression
  [82e08723](https://github.com/phpredis/phpredis/commit/82e08723)
  ([Till Krüss](https://github.com/tillkruss))
- Fix missing auth in RedisSentinel stub
  [5db85561](https://github.com/phpredis/phpredis/commit/5db85561)
  ([Lu Fei](https://github.com/sy-records))
- Fix RedisSentinel pconnect check
  [42cbd88a](https://github.com/phpredis/phpredis/commit/42cbd88a)
  ([Pavlo Yatsukhnenko](https://github.com/yatsukhnenko))
- Fix NULL-pointer dereferences and handle possible UB
  [36457555](https://github.com/phpredis/phpredis/commit/36457555)
  ([Pavlo Yatsukhnenko](https://github.com/yatsukhnenko))
- Fix security alerts
  [ee210f86](https://github.com/phpredis/phpredis/commit/ee210f86),
  [fb6a297c](https://github.com/phpredis/phpredis/commit/fb6a297c)
  ([Pavlo Yatsukhnenko](https://github.com/yatsukhnenko)),
  ([Michael Grunder](https://github.com/michael-grunder))
- Fix segfault
  [55bf0202](https://github.com/phpredis/phpredis/commit/55bf0202)
  ([Pavlo Yatsukhnenko](https://github.com/yatsukhnenko))
- Fix default host length
  [c40f9d6c](https://github.com/phpredis/phpredis/commit/c40f9d6c)
  ([Pavlo Yatsukhnenko](https://github.com/yatsukhnenko))
- Fix redis session standalone stream ssl context
  [ed10f365](https://github.com/phpredis/phpredis/commit/ed10f365),
  [d1bc6727](https://github.com/phpredis/phpredis/commit/d1bc6727),
  [2ff11df5](https://github.com/phpredis/phpredis/commit/2ff11df5)
  ([patricio.dorantes](https://github.com/patricio.dorantes))
- Fix segfault with session+tls
  [a471c87a](https://github.com/phpredis/phpredis/commit/a471c87a)
  ([Pavlo Yatsukhnenko](https://github.com/yatsukhnenko))
- Fix non standards conforming prototypes.
  [b3ce0486](https://github.com/phpredis/phpredis/commit/b3ce0486)
  ([Michael Grunder](https://github.com/michael-grunder))
- Avoid registering the same replicas multiple times
  [f2bfd723](https://github.com/phpredis/phpredis/commit/f2bfd723)
  ([Marius Adam](https://github.com/mariusadam))
- Better unix:// or file:// detection.
  [d05d301b](https://github.com/phpredis/phpredis/commit/d05d301b)
  ([Michael Grunder](https://github.com/michael-grunder))
- Future proof our igbinary header check
  [69355faa](https://github.com/phpredis/phpredis/commit/69355faa)
  ([Michael Grunder](https://github.com/michael-grunder))
- Fix BITOP cross-slot bug
  [af13f951](https://github.com/phpredis/phpredis/commit/af13f951)
  ([Michael Grunder](https://github.com/michael-grunder))
- SENTINEL RESET returns a long.
  [0243dd9d](https://github.com/phpredis/phpredis/commit/0243dd9d)
  ([Michael Grunder](https://github.com/michael-grunder))
- Fix redis_sock_read_multibulk_multi_reply_loop logic
  [d9cb5946](https://github.com/phpredis/phpredis/commit/d9cb5946),
  [5a643b62](https://github.com/phpredis/phpredis/commit/5a643b62)
  ([Pavlo Yatsukhnenko](https://github.com/yatsukhnenko))
- Fix RPOP to unserialize/decompress data.
  [02c91d59](https://github.com/phpredis/phpredis/commit/02c91d59)
  ([Michael Grunder](https://github.com/michael-grunder))
- Fix testObject for redis 7.2
  [fea19b52](https://github.com/phpredis/phpredis/commit/fea19b52),
  [dcb95a3f](https://github.com/phpredis/phpredis/commit/dcb95a3f)
  ([Remi Collet](https://github.com/remicollet))
- Fix bug: the pipeline mode socket return an unexpected result after reconnecting
  [a3327d9d](https://github.com/phpredis/phpredis/commit/a3327d9d)
  ([thomaston](https://github.com/omigafu))
- Fix stub files
  [9aa5f387](https://github.com/phpredis/phpredis/commit/9aa5f387),
  [74cf49f5](https://github.com/phpredis/phpredis/commit/74cf49f5),
  [8b1eafe8](https://github.com/phpredis/phpredis/commit/8b1eafe8),
  [e392dd88](https://github.com/phpredis/phpredis/commit/e392dd88),
  [b5ea5fd7](https://github.com/phpredis/phpredis/commit/b5ea5fd7),
  [71758b09](https://github.com/phpredis/phpredis/commit/71758b09),
  [2a6dee5d](https://github.com/phpredis/phpredis/commit/2a6dee5d)
  ([Nicolas Grekas](https://github.com/nicolas-grekas)),
  ([Michael Grunder](https://github.com/michael-grunder))
- Update documentation
  [b64d93e1](https://github.com/phpredis/phpredis/commit/b64d93e1),
  [703d71b5](https://github.com/phpredis/phpredis/commit/703d71b5),
  [eba1c6d2](https://github.com/phpredis/phpredis/commit/eba1c6d2),
  [0f502c9e](https://github.com/phpredis/phpredis/commit/0f502c9e),
  [130b5d0b](https://github.com/phpredis/phpredis/commit/130b5d0b),
  [21c3ef94](https://github.com/phpredis/phpredis/commit/21c3ef94),
  [b7bf22d4](https://github.com/phpredis/phpredis/commit/b7bf22d4),
  [50151e7a](https://github.com/phpredis/phpredis/commit/50151e7a),
  [b9950727](https://github.com/phpredis/phpredis/commit/b9950727),
  [ab4ce4ab](https://github.com/phpredis/phpredis/commit/ab4ce4ab),
  [8d80ca5b](https://github.com/phpredis/phpredis/commit/8d80ca5b),
  [c4de8667](https://github.com/phpredis/phpredis/commit/c4de8667),
  [6982941b](https://github.com/phpredis/phpredis/commit/6982941b),
  [375d093d](https://github.com/phpredis/phpredis/commit/375d093d),
  [43da8dd9](https://github.com/phpredis/phpredis/commit/43da8dd9),
  [71344612](https://github.com/phpredis/phpredis/commit/71344612),
  [b9de0b97](https://github.com/phpredis/phpredis/commit/b9de0b97),
  [2d8a8a44](https://github.com/phpredis/phpredis/commit/2d8a8a44),
  [a2b0c86f](https://github.com/phpredis/phpredis/commit/a2b0c86f),
  [e0b24be1](https://github.com/phpredis/phpredis/commit/e0b24be1),
  [e609fbe8](https://github.com/phpredis/phpredis/commit/e609fbe8),
  [c4aef956](https://github.com/phpredis/phpredis/commit/c4aef956),
  [df50b2ad](https://github.com/phpredis/phpredis/commit/df50b2ad),
  [cc2383f0](https://github.com/phpredis/phpredis/commit/cc2383f0),
  [0dd2836f](https://github.com/phpredis/phpredis/commit/0dd2836f),
  [7d5db510](https://github.com/phpredis/phpredis/commit/7d5db510),
  [99340889](https://github.com/phpredis/phpredis/commit/99340889),
  [70a55f3e](https://github.com/phpredis/phpredis/commit/70a55f3e),
  [b04684d4](https://github.com/phpredis/phpredis/commit/b04684d4),
  [980ea6b1](https://github.com/phpredis/phpredis/commit/980ea6b1),
  [bb06ffa3](https://github.com/phpredis/phpredis/commit/bb06ffa3),
  [b8679d7a](https://github.com/phpredis/phpredis/commit/b8679d7a),
  [854f3aa4](https://github.com/phpredis/phpredis/commit/854f3aa4),
  [a5c47901](https://github.com/phpredis/phpredis/commit/a5c47901),
  [cf63e96e](https://github.com/phpredis/phpredis/commit/cf63e96e),
  [f05ba819](https://github.com/phpredis/phpredis/commit/f05ba819),
  [17db2328](https://github.com/phpredis/phpredis/commit/17db2328),
  [450904f7](https://github.com/phpredis/phpredis/commit/450904f7),
  [114f4d60](https://github.com/phpredis/phpredis/commit/114f4d60),
  [142bddf0](https://github.com/phpredis/phpredis/commit/142bddf0),
  [87fa36d6](https://github.com/phpredis/phpredis/commit/87fa36d6),
  [531177d4](https://github.com/phpredis/phpredis/commit/531177d4),
  [ecf65144](https://github.com/phpredis/phpredis/commit/ecf65144),
  [53d142d9](https://github.com/phpredis/phpredis/commit/53d142d9),
  [c14a9e3a](https://github.com/phpredis/phpredis/commit/c14a9e3a),
  [72f8eb25](https://github.com/phpredis/phpredis/commit/72f8eb25),
  [872b6931](https://github.com/phpredis/phpredis/commit/872b6931)
  ([Karina Kwiatek](https://github.com/raccube)),
  ([Nicolas Grekas](https://github.com/nicolas-grekas)),
  ([Muhammad Dyas Yaskur](https://github.com/dyaskur)),
  ([sergkash7](https://github.com/sergkash7)),
  ([Dawid Polak](https://github.com/DeyV)),
  ([Michael Grunder](https://github.com/michael-grunder)),
  ([Yurun](https://github.com/Yurunsoft)),
  ([twosee](https://github.com/twose)),
  ([Juha](https://github.com/ejuhjav)),
  ([Till Krüss](https://github.com/tillkruss))

### Changed

- Allow to pass null as iterator
  [14d121bb](https://github.com/phpredis/phpredis/commit/14d121bb)
  ([Pavlo Yatsukhnenko](https://github.com/yatsukhnenko))
- Add NOMKSTREAM option to XADD command.
  [f9436e25](https://github.com/phpredis/phpredis/commit/f9436e25)
  ([Pavlo Yatsukhnenko](https://github.com/yatsukhnenko))
- Don't allow reconnect on read response
  [5a269ab6](https://github.com/phpredis/phpredis/commit/5a269ab6)
  ([Pavlo Yatsukhnenko](https://github.com/yatsukhnenko))
- Reset multi/pipline transaction on pconnect close
  [0879770a](https://github.com/phpredis/phpredis/commit/0879770a)
  ([Pavlo Yatsukhnenko](https://github.com/yatsukhnenko))
- Use read_mbulk_header helper where possible
  [ca8b4c93](https://github.com/phpredis/phpredis/commit/ca8b4c93)
  ([Pavlo Yatsukhnenko](https://github.com/yatsukhnenko))
- Allow to pass null as auth argument
  [41517753](https://github.com/phpredis/phpredis/commit/41517753)
  ([Pavlo Yatsukhnenko](https://github.com/yatsukhnenko))
- Refactor redis_parse_client_list_response
  [68136a29](https://github.com/phpredis/phpredis/commit/68136a29),
  [aaa4c91a](https://github.com/phpredis/phpredis/commit/aaa4c91a),
  [1fb2935b](https://github.com/phpredis/phpredis/commit/1fb2935b),
  [cf2c052c](https://github.com/phpredis/phpredis/commit/cf2c052c)
  ([Pavlo Yatsukhnenko](https://github.com/yatsukhnenko))
- Refactor subscribe/unsubscribe
  [3c9e159c](https://github.com/phpredis/phpredis/commit/3c9e159c)
  ([Pavlo Yatsukhnenko](https://github.com/yatsukhnenko))
- Change PHPREDIS_CTX_PTR type
  [de3635da](https://github.com/phpredis/phpredis/commit/de3635da)
  ([Pavlo Yatsukhnenko](https://github.com/yatsukhnenko))
- Refactor redis_parse_info_response
  [982bd13b](https://github.com/phpredis/phpredis/commit/982bd13b)
  ([Pavlo Yatsukhnenko](https://github.com/yatsukhnenko))
- Allow IPv6 address within square brackets
  [c28ad7bb](https://github.com/phpredis/phpredis/commit/c28ad7bb)
  ([Pavlo Yatsukhnenko](https://github.com/yatsukhnenko))
- Allow multiple field-value pairs for hmset command.
  [e858e8e3](https://github.com/phpredis/phpredis/commit/e858e8e3)
  ([Pavlo Yatsukhnenko](https://github.com/yatsukhnenko))
- Refactor MINIT and use @generate-class-entries in stub files
  [3675f442](https://github.com/phpredis/phpredis/commit/3675f442)
  ([Remi Collet](https://github.com/remicollet))
- Use spl_ce_RuntimeException
  [3cd5ac1e](https://github.com/phpredis/phpredis/commit/3cd5ac1e),
  [a7e5ea64](https://github.com/phpredis/phpredis/commit/a7e5ea64)
  ([Remi Collet](https://github.com/remicollet))
- Regenerate arginfo using 8.2.0
  [a38e08da](https://github.com/phpredis/phpredis/commit/a38e08da)
  ([Remi Collet](https://github.com/remicollet))
- Refactor client command
  [a8d10291](https://github.com/phpredis/phpredis/commit/a8d10291)
  ([Pavlo Yatsukhnenko](https://github.com/yatsukhnenko))
- Pull COUNT/ANY parsing into a helper function
  [d67b2020](https://github.com/phpredis/phpredis/commit/d67b2020)
  ([Michael Grunder](https://github.com/michael-grunder))
- Return false or NULL on empty lpos response
  [39a01ac7](https://github.com/phpredis/phpredis/commit/39a01ac7)
  ([Michael Grunder](https://github.com/michael-grunder))
- BLPOP with a float timeout
  [a98605f2](https://github.com/phpredis/phpredis/commit/a98605f2),
  [dc9af529](https://github.com/phpredis/phpredis/commit/dc9af529)
  ([Michael Grunder](https://github.com/michael-grunder))
- Make sure we set an error for key based scans
  [98fda1b8](https://github.com/phpredis/phpredis/commit/98fda1b8)
  ([Michael Grunder](https://github.com/michael-grunder))
- Add back a default switch case for setoption handler
  [87464932](https://github.com/phpredis/phpredis/commit/87464932)
  ([Michael Grunder](https://github.com/michael-grunder))
- Update stubs so the tests pass in strict mode
  [bebd398c](https://github.com/phpredis/phpredis/commit/bebd398c)
  ([Michael Grunder](https://github.com/michael-grunder))
- Move where we generate our salt
  [d2044c9f](https://github.com/phpredis/phpredis/commit/d2044c9f)
  ([Michael Grunder](https://github.com/michael-grunder))
- Refactor XINFO handler
  [3b0d8b77](https://github.com/phpredis/phpredis/commit/3b0d8b77)
  ([Michael Grunder](https://github.com/michael-grunder))
- Refactor and fix XPENDING handler
  [457953f4](https://github.com/phpredis/phpredis/commit/457953f4)
  ([Michael Grunder](https://github.com/michael-grunder))
- Refactor FLUSHDB and update docs.
  [54a084e5](https://github.com/phpredis/phpredis/commit/54a084e5)
  ([Michael Grunder](https://github.com/michael-grunder))
- Add missing directed node command to docs and refactor stubs.
  [5ac92d25](https://github.com/phpredis/phpredis/commit/5ac92d25)
  ([Michael Grunder](https://github.com/michael-grunder))
- Refactor BITPOS and implement BIT/BYTE option.
  [4d8afd38](https://github.com/phpredis/phpredis/commit/4d8afd38)
  ([Michael Grunder](https://github.com/michael-grunder))
- INFO with multiple sections
  [44d03ca0](https://github.com/phpredis/phpredis/commit/44d03ca0)
  ([Michael Grunder](https://github.com/michael-grunder))
- Refactor SLOWLOG command
  [d87f1428](https://github.com/phpredis/phpredis/commit/d87f1428)
  ([Michael Grunder](https://github.com/michael-grunder))
- Refactor SORT and add SORT_RO command
  [8c7c5a3a](https://github.com/phpredis/phpredis/commit/8c7c5a3a)
  ([Michael Grunder](https://github.com/michael-grunder))
- Use ZEND_STRL in redis_commands.c
  [78de25a3](https://github.com/phpredis/phpredis/commit/78de25a3)
  ([Pavlo Yatsukhnenko](https://github.com/yatsukhnenko))
- Refactor PubSub command
  [2a0d1c1e](https://github.com/phpredis/phpredis/commit/2a0d1c1e)
  ([Pavlo Yatsukhnenko](https://github.com/yatsukhnenko))
- Refactor SLAVEOF handler
  [f2cef8be](https://github.com/phpredis/phpredis/commit/f2cef8be)
  ([Michael Grunder](https://github.com/michael-grunder))
- Refactor ACL command
  [504810a5](https://github.com/phpredis/phpredis/commit/504810a5)
  ([Pavlo Yatsukhnenko](https://github.com/yatsukhnenko))
- Use fast_zpp API
  [376d4d27](https://github.com/phpredis/phpredis/commit/376d4d27)
  ([Pavlo Yatsukhnenko](https://github.com/yatsukhnenko))
- Fix XAUTOCLAIM response handler
  [0b7bd83f](https://github.com/phpredis/phpredis/commit/0b7bd83f)
  ([Michael Grunder](https://github.com/michael-grunder))
- Refactor `command` command
  [ff863f3f](https://github.com/phpredis/phpredis/commit/ff863f3f)
  ([Pavlo Yatsukhnenko](https://github.com/yatsukhnenko))
- Refactor rawCommand and WAIT
  [79c9d224](https://github.com/phpredis/phpredis/commit/79c9d224)
  ([Michael Grunder](https://github.com/michael-grunder))
- Refactor SELECT command
  [86f15cca](https://github.com/phpredis/phpredis/commit/86f15cca)
  ([Michael Grunder](https://github.com/michael-grunder))
- Refactor SRANDMEMBER command.
  [f62363c2](https://github.com/phpredis/phpredis/commit/f62363c2)
  ([Michael Grunder](https://github.com/michael-grunder))
- Refactor OBJECT command.
  [acb5db76](https://github.com/phpredis/phpredis/commit/acb5db76)
  ([Michael Grunder](https://github.com/michael-grunder))
- Refactor gen_varkey_cmd
  [3efa59cb](https://github.com/phpredis/phpredis/commit/3efa59cb)
  ([Michael Grunder](https://github.com/michael-grunder))
- Refactor MGET command.
  [8cb6dd17](https://github.com/phpredis/phpredis/commit/8cb6dd17)
  ([Michael Grunder](https://github.com/michael-grunder))
- Refactor INFO and SCRIPT commands.
  [3574ef08](https://github.com/phpredis/phpredis/commit/3574ef08)
  ([Michael Grunder](https://github.com/michael-grunder))
- Refactor MSET and MSETNX commands.
  [6d104481](https://github.com/phpredis/phpredis/commit/6d104481)
  ([Michael Grunder](https://github.com/michael-grunder))
- Refactor HMSET command.
  [90eb0470](https://github.com/phpredis/phpredis/commit/90eb0470)
  ([Michael Grunder](https://github.com/michael-grunder))
- Refactor PFCOUNT command.
  [19fd7e0c](https://github.com/phpredis/phpredis/commit/19fd7e0c)
  ([Michael Grunder](https://github.com/michael-grunder))
- Refactor SMOVE command.
  [204a02c5](https://github.com/phpredis/phpredis/commit/204a02c5)
  ([Michael Grunder](https://github.com/michael-grunder))
- Rework ZRANGE argument handling.
  [aa0938a4](https://github.com/phpredis/phpredis/commit/aa0938a4)
  ([Michael Grunder](https://github.com/michael-grunder))
- Refactor a couple more command methods.
  [5b560ccf](https://github.com/phpredis/phpredis/commit/5b560ccf),
  [c8224b93](https://github.com/phpredis/phpredis/commit/c8224b93),
  [40e1b1bf](https://github.com/phpredis/phpredis/commit/40e1b1bf),
  [ccd419a4](https://github.com/phpredis/phpredis/commit/ccd419a4)
  ([Michael Grunder](https://github.com/michael-grunder))
- Refactor HMGET command
  [bb66a547](https://github.com/phpredis/phpredis/commit/bb66a547)
  ([Michael Grunder](https://github.com/michael-grunder))
- Refactor CLIENT command
  [77c4f7a3](https://github.com/phpredis/phpredis/commit/77c4f7a3)
  ([Pavlo Yatsukhnenko](https://github.com/yatsukhnenko))
- Refactor redis_long_response
  [f14a80db](https://github.com/phpredis/phpredis/commit/f14a80db)
  ([Pavlo Yatsukhnenko](https://github.com/yatsukhnenko))
- Synchronize Redis and RedisSentinel constructors
  [ebb2386e](https://github.com/phpredis/phpredis/commit/ebb2386e)
  ([Pavlo Yatsukhnenko](https://github.com/yatsukhnenko))
- Use redis_sock_connect on connect
  [f6c8b9c6](https://github.com/phpredis/phpredis/commit/f6c8b9c6)
  ([Pavlo Yatsukhnenko](https://github.com/yatsukhnenko))
- Auto-select db in redis_sock_server_open
  [6930a81c](https://github.com/phpredis/phpredis/commit/6930a81c)
  ([Pavlo Yatsukhnenko](https://github.com/yatsukhnenko))
- Use on-stack allocated valiables
  [7a055cad](https://github.com/phpredis/phpredis/commit/7a055cad)
  ([Pavlo Yatsukhnenko](https://github.com/yatsukhnenko))

### Added

- Add XAUTOCLAIM command
  [01f3342c](https://github.com/phpredis/phpredis/commit/01f3342c)
  ([Pavlo Yatsukhnenko](https://github.com/yatsukhnenko))
- Add SYNC arg to FLUSHALL and FLUSHDB, and ASYNC/SYNC arg to SCRIPT FLUSH
  [750b6cf3](https://github.com/phpredis/phpredis/commit/750b6cf3)
  ([Pavlo Yatsukhnenko](https://github.com/yatsukhnenko))
- Add reset command
  [947a2d38](https://github.com/phpredis/phpredis/commit/947a2d38)
  ([Pavlo Yatsukhnenko](https://github.com/yatsukhnenko))
- Add hRandField command
  [fe397371](https://github.com/phpredis/phpredis/commit/fe397371)
  ([Pavlo Yatsukhnenko](https://github.com/yatsukhnenko))
- Add PXAT/EXAT arguments to SET command.
  [0a160685](https://github.com/phpredis/phpredis/commit/0a160685)
  ([Pavlo Yatsukhnenko](https://github.com/yatsukhnenko))
- Add GETEX, GETDEL commands.
  [11861d95](https://github.com/phpredis/phpredis/commit/11861d95)
  ([Pavlo Yatsukhnenko](https://github.com/yatsukhnenko))
- Add FAILOVER command.
  [4b767be7](https://github.com/phpredis/phpredis/commit/4b767be7)
  ([Pavlo Yatsukhnenko](https://github.com/yatsukhnenko))
- Backoff settings in constructor
  [e6b3fe54](https://github.com/phpredis/phpredis/commit/e6b3fe54)
  ([Pavlo Yatsukhnenko](https://github.com/yatsukhnenko))
- Add the COUNT argument to LPOP and RPOP
  [df97cc35](https://github.com/phpredis/phpredis/commit/df97cc35)
  ([Pavlo Yatsukhnenko](https://github.com/yatsukhnenko))
- Unsubscribe from all channels
  [0f1ca0cc](https://github.com/phpredis/phpredis/commit/0f1ca0cc)
  ([Pavlo Yatsukhnenko](https://github.com/yatsukhnenko))
- Add lPos command.
  [687a5c78](https://github.com/phpredis/phpredis/commit/687a5c78)
  ([Pavlo Yatsukhnenko](https://github.com/yatsukhnenko))
- Add the ANY argument to GEOSEARCH and GEORADIUS
  [bf6f31e3](https://github.com/phpredis/phpredis/commit/bf6f31e3)
  ([Pavlo Yatsukhnenko](https://github.com/yatsukhnenko))
- Add 'BIT'/'BYTE' modifier to BITCOUNT + tests
  [a3d2f131](https://github.com/phpredis/phpredis/commit/a3d2f131)
  ([Michael Grunder](https://github.com/michael-grunder))
- Add missing configureoption entries in package.xml
  [59053f10](https://github.com/phpredis/phpredis/commit/59053f10)
  ([Michele Locati](https://github.com/mlocati))
- Implement CONFIG RESETSTAT
  [239678a0](https://github.com/phpredis/phpredis/commit/239678a0)
  ([Michael Grunder](https://github.com/michael-grunder))
- SINTERCARD and ZINTERCARD commands
  [64300508](https://github.com/phpredis/phpredis/commit/64300508)
  ([Michael Grunder](https://github.com/michael-grunder))
- LCS command
  [c0e839f6](https://github.com/phpredis/phpredis/commit/c0e839f6)
  ([Michael Grunder](https://github.com/michael-grunder))
- EXPIRETIME and PEXPIRETIME
  [f5b2a09b](https://github.com/phpredis/phpredis/commit/f5b2a09b)
  ([Michael Grunder](https://github.com/michael-grunder))
- [B]LMPOP and [B]ZMPOP commands
  [6ea978eb](https://github.com/phpredis/phpredis/commit/6ea978eb)
  ([Michael Grunder](https://github.com/michael-grunder))
- Implement new RESTORE options
  [9a3fe401](https://github.com/phpredis/phpredis/commit/9a3fe401)
  ([Michael Grunder](https://github.com/michael-grunder))
- Add new Redis 6.2.0 XTRIM options
  [6b34d17f](https://github.com/phpredis/phpredis/commit/6b34d17f)
  ([Michael Grunder](https://github.com/michael-grunder))
- Implement AUTH/AUTH2 arguments for MIGRATE
  [114d79d1](https://github.com/phpredis/phpredis/commit/114d79d1)
  ([Michael Grunder](https://github.com/michael-grunder))
- Implement CONFIG REWRITE
  [525958ea](https://github.com/phpredis/phpredis/commit/525958ea)
  ([Michael Grunder](https://github.com/michael-grunder))
- Implement Redis 7.0.0 [P]EXPIRE[AT] options
  [872ae107](https://github.com/phpredis/phpredis/commit/872ae107)
  ([Michael Grunder](https://github.com/michael-grunder))
- Variadic CONFIG GET/SET
  [36ef4bd8](https://github.com/phpredis/phpredis/commit/36ef4bd8),
  [a176f586](https://github.com/phpredis/phpredis/commit/a176f586)
  ([Michael Grunder](https://github.com/michael-grunder))
- EVAL_RO and EVALSHA_RO
  [f3a40830](https://github.com/phpredis/phpredis/commit/f3a40830)
  ([Michael Grunder](https://github.com/michael-grunder))
- Implement ZRANGESTORE and add ZRANGE options
  [71bcbcb9](https://github.com/phpredis/phpredis/commit/71bcbcb9)
  ([Michael Grunder](https://github.com/michael-grunder))
- XGROUP DELCONSUMER and ENTRIESREAD
  [1343f500](https://github.com/phpredis/phpredis/commit/1343f500)
  ([Michael Grunder](https://github.com/michael-grunder))
- Expose the transferred number of bytes
  [e0a88b7b](https://github.com/phpredis/phpredis/commit/e0a88b7b),
  [90828019](https://github.com/phpredis/phpredis/commit/90828019),
  [7a4cee2d](https://github.com/phpredis/phpredis/commit/7a4cee2d)
  ([Pavlo Yatsukhnenko](https://github.com/yatsukhnenko)),
  ([Michael Grunder](https://github.com/michael-grunder))
- TOUCH command
  [dc1f2398](https://github.com/phpredis/phpredis/commit/dc1f2398)
  ([Michael Grunder](https://github.com/michael-grunder))
- Redis Sentinel TLS support
  [f2bb2cdb](https://github.com/phpredis/phpredis/commit/f2bb2cdb)
  ([Pavlo Yatsukhnenko](https://github.com/yatsukhnenko))
- Add the CH, NX, XX arguments to GEOADD
  [2bb64038](https://github.com/phpredis/phpredis/commit/2bb64038),
  [e8f5b517](https://github.com/phpredis/phpredis/commit/e8f5b517)
  ([Pavlo Yatsukhnenko](https://github.com/yatsukhnenko))
- Implement SMISMEMBER for RedisCluster
  [abfac47b](https://github.com/phpredis/phpredis/commit/abfac47b)
  ([Michael Grunder](https://github.com/michael-grunder))
- Implement ssubscribe/sunsubscribe
  [7644736e](https://github.com/phpredis/phpredis/commit/7644736e)
  ([Pavlo Yatsukhnenko](https://github.com/yatsukhnenko))
- Implement BLMOVE and add LMOVE/BLMOVE to cluster.
  [121e9d9c](https://github.com/phpredis/phpredis/commit/121e9d9c)
  ([Michael Grunder](https://github.com/michael-grunder))
- Implement LPOS for RedisCluster
  [7121aaae](https://github.com/phpredis/phpredis/commit/7121aaae)
  ([Michael Grunder](https://github.com/michael-grunder))
- Implement GEOSEARCH and GEOSEARCHSTORE for RedisCluster.
  [fa5d1af9](https://github.com/phpredis/phpredis/commit/fa5d1af9)
  ([Michael Grunder](https://github.com/michael-grunder))
- Implement HRANDFIELD for RedisCluster
  [e222b85e](https://github.com/phpredis/phpredis/commit/e222b85e)
  ([Michael Grunder](https://github.com/michael-grunder))
- Implement COPY for RedisCluster
  [40a2c254](https://github.com/phpredis/phpredis/commit/40a2c254)
  ([Michael Grunder](https://github.com/michael-grunder))
- Implement new ZSET commands for cluster
  [27900f39](https://github.com/phpredis/phpredis/commit/27900f39)
  ([Michael Grunder](https://github.com/michael-grunder))
- Add cluster support for strict sessions and lazy write
  [b6cf6361](https://github.com/phpredis/phpredis/commit/b6cf6361)
  ([Michael Grunder](https://github.com/michael-grunder))
- Add function command
  [90a0e9cc](https://github.com/phpredis/phpredis/commit/90a0e9cc)
  ([Pavlo Yatsukhnenko](https://github.com/yatsukhnenko))
- Add FCALL/FCALL_RO commands
  [7c46ad2c](https://github.com/phpredis/phpredis/commit/7c46ad2c)
  ([Pavlo Yatsukhnenko](https://github.com/yatsukhnenko))

### Removed

- Remove unused macros
  [831d6118](https://github.com/phpredis/phpredis/commit/831d6118)
  ([Pavlo Yatsukhnenko](https://github.com/yatsukhnenko))

## [5.3.7] - 2021-02-15 ([GitHub](https://github.com/phpredis/phpredis/releases/5.3.7), [PECL](https://pecl.php.net/package/redis/5.3.7))

### Sponsors :sparkling_heart:

- [Audiomack](https://audiomack.com)
- [Open LMS](https://openlms.net/)
- [BlueHost](https://bluehost.com)
- [Object Cache Pro for WordPress](https://objectcache.pro/)
- [Avtandil Kikabidze](https://github.com/akalongman)
- [Zaher Ghaibeh](https://github.com/zaherg)
- [BatchLabs](https://batch.com)
- [Stackhero](https://github.com/stackhero-io)
- [Florian Levis](https://github.com/Gounlaf)
- [Luis Zárate](https://github.com/jlzaratec)

*There were no changes between 5.3.7 and 5.3.7RC2*

## [5.3.7RC2] - 2021-02-12 ([GitHub](https://github.com/phpredis/phpredis/releases/5.3.7RC2), [PECL](https://pecl.php.net/package/redis/5.3.7RC2))

### Sponsors :sparkling_heart:

- [Audiomack](https://audiomack.com)
- [Open LMS](https://openlms.net/)
- [BlueHost](https://bluehost.com)
- [Object Cache Pro for WordPress](https://objectcache.pro/)
- [Avtandil Kikabidze](https://github.com/akalongman)
- [Zaher Ghaibeh](https://github.com/zaherg)
- [BatchLabs](https://batch.com)
- [Stackhero](https://github.com/stackhero-io)
- [Florian Levis](https://github.com/Gounlaf)
- [Luis Zárate](https://github.com/jlzaratec)

*There were no changes between 5.3.7RC2 and 5.3.7RC1*

## [5.3.7RC1] - 2021-02-02 ([GitHub](https://github.com/phpredis/phpredis/releases/5.3.7RC1), [PECL](https://pecl.php.net/package/redis/5.3.7RC1))

### Sponsors :sparkling_heart:

- [Audiomack](https://audiomack.com)
- [Open LMS](https://openlms.net/)
- [BlueHost](https://bluehost.com)
- [Object Cache Pro for WordPress](https://objectcache.pro/)
- [Avtandil Kikabidze](https://github.com/akalongman)
- [Zaher Ghaibeh](https://github.com/zaherg)
- [BatchLabs](https://batch.com)
- [Stackhero](https://github.com/stackhero-io)
- [Florian Levis](https://github.com/Gounlaf)
- [Luis Zárate](https://github.com/jlzaratec)

### Fixed

- Fix RedisArray::[hsz]scan and tests
  [08a9d5db](https://github.com/phpredis/phpredis/commit/08a9d5db),
  [0264de18](https://github.com/phpredis/phpredis/commit/0264de18),
  ([Pavlo Yatsukhnenko](https://github.com/yatsukhnenko)),
  ([Michael Grunder](https://github.com/michael-grunder))
- Fix RedisArray::scan
  [8689ab1c](https://github.com/phpredis/phpredis/commit/8689ab1c)
  ([Pavlo Yatsukhnenko](https://github.com/yatsukhnenko))
- Fix LZF decompression logic
  [0719c1ec](https://github.com/phpredis/phpredis/commit/0719c1ec)
  ([Michael Grunder](https://github.com/michael-grunder))

## [5.3.6] - 2021-01-17 ([GitHub](https://github.com/phpredis/phpredis/releases/5.3.6), [PECL](https://pecl.php.net/package/redis/5.3.6))

### Sponsors :sparkling_heart:

- [Audiomack](https://audiomack.com)
- [Open LMS](https://openlms.net/)
- [BlueHost](https://bluehost.com)
- [Object Cache Pro for WordPress](https://objectcache.pro/)
- [Avtandil Kikabidze](https://github.com/akalongman)
- [Zaher Ghaibeh](https://github.com/zaherg)
- [BatchLabs](https://batch.com)
- [Stackhero](https://github.com/stackhero-io)
- [Florian Levis](https://github.com/Gounlaf)
- [Luis Zárate](https://github.com/jlzaratec)

### Fixed

- Fix a segfault in RedisArray::del
  [d2f2a7d9](https://github.com/phpredis/phpredis/commit/d2f2a7d9)
  ([Pavlo Yatsukhnenko](https://github.com/yatsukhnenko))

## [5.3.5] - 2021-12-18 ([GitHub](https://github.com/phpredis/phpredis/releases/5.3.5), [PECL](https://pecl.php.net/package/redis/5.3.5))

### Sponsors :sparkling_heart:

- [Audiomack](https://audiomack.com)
- [Open LMS](https://openlms.net/)
- [BlueHost](https://bluehost.com)
- [Object Cache Pro for WordPress](https://objectcache.pro/)
- [Avtandil Kikabidze](https://github.com/akalongman)
- [Zaher Ghaibeh](https://github.com/zaherg)
- [BatchLabs](https://batch.com)
- [Stackhero](https://github.com/stackhero-io)
- [Florian Levis](https://github.com/Gounlaf)
- [Luis Zárate](https://github.com/jlzaratec)

### Fixed

- Fixed typo in cluster_scan_resp
  [44affad2](https://github.com/phpredis/phpredis/commit/44affad2)

## [5.3.5RC1] - 2021-11-16 ([GitHub](https://github.com/phpredis/phpredis/releases/5.3.5RC1), [PECL](https://pecl.php.net/package/redis/5.3.5RC1))

### Sponsors :sparkling_heart:

- [Audiomack](https://audiomack.com)
- [Open LMS](https://openlms.net/)
- [BlueHost](https://bluehost.com)
- [Object Cache Pro for WordPress](https://objectcache.pro/)
- [Avtandil Kikabidze](https://github.com/akalongman)
- [Zaher Ghaibeh](https://github.com/zaherg)
- [BatchLabs](https://batch.com)
- [Stackhero](https://github.com/stackhero-io)
- [Florian Levis](https://github.com/Gounlaf)
- [Luis Zárate](https://github.com/jlzaratec)

### Fixed

- Fixed segfault in redis_setoption_handler
  [#2030](https://github.com/phpredis/phpredis/issues/2030)
  [692e4e84](https://github.com/phpredis/phpredis/commit/692e4e84)
  ([Pavlo Yatsukhnenko](https://github.com/yatsukhnenko))
- Fix masters array in the event of a cluster failover
  [bce692962](https://github.com/phpredis/phpredis/commit/bce692962)
  [#2025](https://github.com/phpredis/phpredis/pull/2025)
  ([Bar Shaul](https://github.com/barshaul))
- Fix 32bit type error
  [672dec87f](https://github.com/phpredis/phpredis/commit/672dec87f)
  ([#1956](https://github.com/phpredis/phpredis/issues/1956))
  ([Remi Collet](https://github.com/remicollet))
- Fix radix character in certain locales
  [#1893](https://github.com/phpredis/phpredis/issues/1893)
  [89a871e24](https://github.com/phpredis/phpredis/commit/89a871e24)
  ([Pavlo Yatsukhnenko](https://github.com/yatsukhnenko))
- ZSTD Validation fix
  [6a77ef5cd](https://github.com/phpredis/phpredis/commit/6a77ef5cd)
  ([Michael Grunder](https://github.com/michael-grunder))
- Remove superfluous typecast
  [b2871471f](https://github.com/phpredis/phpredis/commit/b2871471f)
  ([Remi Collet](https://github.com/remicollet))
- Updated documentation
  [f84168657](https://github.com/phpredis/phpredis/commit/f84168657),
  [d017788e7](https://github.com/phpredis/phpredis/commit/d017788e7),
  [20ac84710](https://github.com/phpredis/phpredis/commit/20ac84710),
  [0adf05260](https://github.com/phpredis/phpredis/commit/0adf05260),
  [aee29bf73](https://github.com/phpredis/phpredis/commit/aee29bf73),
  [09a095e72](https://github.com/phpredis/phpredis/commit/09a095e72),
  [12ffbf33a](https://github.com/phpredis/phpredis/commit/12ffbf33a),
  [ff331af98](https://github.com/phpredis/phpredis/commit/ff331af98),
  [a6bdb8731](https://github.com/phpredis/phpredis/commit/a6bdb8731),
  [305c15840](https://github.com/phpredis/phpredis/commit/305c15840),
  [1aa10e93a](https://github.com/phpredis/phpredis/commit/1aa10e93a),
  [d78b0c79d](https://github.com/phpredis/phpredis/commit/d78b0c79d),
  [c6d37c27c](https://github.com/phpredis/phpredis/commit/c6d37c27c),
  [a6303f5b9](https://github.com/phpredis/phpredis/commit/a6303f5b9),
  [d144bd2c7](https://github.com/phpredis/phpredis/commit/d144bd2c7),
  [a6fb815ef](https://github.com/phpredis/phpredis/commit/a6fb815ef),
  [9ef862bc6](https://github.com/phpredis/phpredis/commit/9ef862bc6)
  ([neodisco](https://github.com/neodisco), [Billy Wilson](https://github.com/wilsonwr),
  [Clément Tessier](https://github.com/ctessier), [wangqr](https://github.com/wangqr),
  [T. Todua](https://github.com/ttodua), [Naphat Deepar](https://github.com/feverxai),
  [dengliming](https://github.com/dengliming), [Poplary](https://github.com/poplary),
  [Maxime Cornet](https://github.com/xElysioN), [Michael Grunder](https://github.com/michael-grunder),
  [Emanuele Filannino](https://github.com/tatekan), [MiRacLe](https://github.com/MiRacLe-RPZ),
  [Michael Grunder](https://github.com/michael-grunder))
- Travis CI Fixes
  [a43f4586e](https://github.com/phpredis/phpredis/commit/a43f4586e),
  [4fde8178f](https://github.com/phpredis/phpredis/commit/4fde8178f),
  [7bd5415ac](https://github.com/phpredis/phpredis/commit/7bd5415ac),
  [fdb8c4bb7](https://github.com/phpredis/phpredis/commit/fdb8c4bb7),
  [d4f407470](https://github.com/phpredis/phpredis/commit/d4f407470)
  ([Pavlo Yatsukhnenko](https://github.com/yatsukhnenko))
- Minor fixes/cleanup
  [2e190adc1](https://github.com/phpredis/phpredis/commit/2e190adc1),
  [99975b592](https://github.com/phpredis/phpredis/commit/99975b592),
  [9d0879fa5](https://github.com/phpredis/phpredis/commit/9d0879fa5),
  [22b06457b](https://github.com/phpredis/phpredis/commit/22b06457b),
  ([Pavlo Yatsukhnenko](https://github.com/yatsukhnenko))
- Fix RedisArray constructor bug
  [85dc883ba](https://github.com/phpredis/phpredis/commit/85dc883ba)
  ([Pavlo Yatsukhnenko](https://github.com/yatsukhnenko))

### Changed

- Moved to GitHub Actions
  [4d2afa786](https://github.com/phpredis/phpredis/commit/4d2afa786),
  [502d09fd5](https://github.com/phpredis/phpredis/commit/502d09fd5)
  ([Pavlo Yatsukhnenko](https://github.com/yatsukhnenko))
- Use more appropriate array iteration macro
  [6008900c2](https://github.com/phpredis/phpredis/commit/6008900c2)
  ([Pavlo Yatsukhnenko](https://github.com/yatsukhnenko))
- Clean up session tests
  [ab25ae7f3](https://github.com/phpredis/phpredis/commit/ab25ae7f3)
  ([Michael Grunder](https://github.com/michael-grunder))
- RedisArray refactors
  [1250f0001](https://github.com/phpredis/phpredis/commit/1250f0001),
  [017b2ea7f](https://github.com/phpredis/phpredis/commit/017b2ea7f),
  [37ed3f079](https://github.com/phpredis/phpredis/commit/37ed3f079)
  ([Pavlo Yatsukhnenko](https://github.com/yatsukhnenko))
- Use zend_parse_parameters_none helper
  [a26b14dbe](https://github.com/phpredis/phpredis/commit/a26b14dbe)
  ([Remi Collet](https://github.com/remicollet))

### Added

- Support for various exponential backoff strategies
  [#1986](https://github.com/phpredis/phpredis/commit/#1986),
  [#1993](https://github.com/phpredis/phpredis/commit/#1993),
  [732eb8dcb](https://github.com/phpredis/phpredis/commit/732eb8dcb)
  [05129c3a3](https://github.com/phpredis/phpredis/commit/05129c3a3)
  [5bba6a7fc](https://github.com/phpredis/phpredis/commit/5bba6a7fc)
  ([Nathaniel Braun](https://github.com/nbraun-amazon))
- Added experimental support for detecting a dirty connection by
  trying to determine if the underlying stream is readable.
  [d68579562](https://github.com/phpredis/phpredis/commit/d68579562)
  [#2013](https://github.com/phpredis/phpredis/issues/2013)
  ([Michael Grunder](https://github.com/michael-grunder))
- Created distinct compression utility methods (pack/unpack)
  [#1939](https://github.com/phpredis/phpredis/issues/1939)
  [da2790aec](https://github.com/phpredis/phpredis/commit/da2790aec)
  ([Michael Grunder](https://github.com/michael-grunder))
- SMISMEMBER Command
  [#1894](https://github.com/phpredis/phpredis/commit/#1894)
  [ae2382472](https://github.com/phpredis/phpredis/commit/ae2382472),
  [ed283e1ab](https://github.com/phpredis/phpredis/commit/ed283e1ab),
  ([Pavlo Yatsukhnenko](https://github.com/yatsukhnenko))

## [5.3.4] - 2021-03-24 ([GitHub](https://github.com/phpredis/phpredis/releases/5.3.4), [PECL](https://pecl.php.net/package/redis/5.3.4))

### Sponsors :sparkling_heart:

- [Audiomack](https://audiomack.com)
- [Open LMS](https://openlms.net/)
- [BlueHost](https://bluehost.com)
- [Object Cache Pro for WordPress](https://objectcache.pro/)
- [Avtandil Kikabidze](https://github.com/akalongman)
- [Zaher Ghaibeh](https://github.com/zaherg)
- [BatchLabs](https://batch.com)

### Fixed

- Fix multi/pipeline segfault on Apple silicon [#1917](https://github.com/phpredis/phpredis/issues/1917)
  [e0796d48](https://github.com/phpredis/phpredis/commit/e0796d48af18adac2b93982474e7df8de79ec854)
  ([Michael Grunder](https://github.com/michael-grunder))
- Pass compression flag on HMGET in RedisCluster [#1945](https://github.com/phpredis/phpredis/issues/1945)
  [edc724e6](https://github.com/phpredis/phpredis/commit/edc724e6022620414abf4f90256522d03c3160fd)
  ([Adam Olley](https://github.com/aolley))
- Abide by ZSTD error return constants [#1936](https://github.com/phpredis/phpredis/issues/1936)
  [8400ed1c](https://github.com/phpredis/phpredis/pull/1937/commits/8400ed1cb23a22f70727cb60e88ca5397ee10d23)
  ([Michael Grunder](https://github.com/michael-grunder))
- Fix timing related CI session tests
  [9b986bf8](https://github.com/phpredis/phpredis/commit/9b986bf81859f5a5983cd148cb15ee6ce292d288)
  ([Michael Grunder](https://github.com/michael-grunder))

## [5.3.3] - 2021-02-01 ([GitHub](https://github.com/phpredis/phpredis/releases/5.3.3), [PECL](https://pecl.php.net/package/redis/5.3.3))

### Sponsors :sparkling_heart:

- [Audiomack](https://audiomack.com)
- [BlueHost](https://bluehost.com)
- [Redis Cache Pro for WordPress](https://wprediscache.com)
- [Avtandil Kikabidze](https://github.com/akalongman)
- [Oleg Babushkin](https://github.com/olbabushkin)
- [Zaher Ghaibeh](https://github.com/zaherg)
- [BatchLabs](https://batch.com)

### Fixed

- Fixed Windows includes for PHP 8
  [270b4db8](https://www.github.com/phpredis//phpredis/commit/270b4db821fcbe9fb881eef83e046f87587c4110)
  ([Jan-E](https://github.com/Jan-E))
- Fix hash_ops for PHP 8.0.1
  [87297cbb](https://www.github.com/phpredis/phpredis/commit/87297cbb4000c88b07e729b9379de321ead74aa2)
  ([defender-11](https://github.com/defender-11))
- Disable clone for Redis and RedisCluster objects.  Presently they segfault.
  [cd05a344](https://www.github.com/phpredis/phpredis/commit/87297cbb4000c88b07e729b9379de321ead74aa2)
  ([Michael Grunder](https://github.com/michael-grunder))

## [5.3.2] - 2020-10-22 ([GitHub](https://github.com/phpredis/phpredis/releases/5.3.2), [PECL](https://pecl.php.net/package/redis/5.3.2))

### Sponsors :sparkling_heart:

- [Audiomack](https://audiomack.com)
- [BlueHost](https://bluehost.com)
- [Redis Cache Pro for WordPress](https://wprediscache.com)
- [Avtandil Kikabidze](https://github.com/akalongman)
- [Oleg Babushkin](https://github.com/olbabushkin)

### Fixed

- Verify SET options are strings before testing them as strings
  [514bc371](https://github.com/phpredis/phpredis/commit/514bc37102c08c1ba7222212b125390f34c35803)
  ([Michael Grunder](https://github.com/michael-grunder))
- Fix cluster segfault when dealing with NULL multi bulk replies in RedisCluster
  [950e8de8](https://github.com/phpredis/phpredis/commit/950e8de807ba17ecfff62504a6ee7a959a5df714)
  ([Michael Grunder](https://github.com/michael-grunder),
   [Alex Offshore](https://github.com/offshore))
- Fix xReadGroup() must return message id
  [500916a4](https://github.com/phpredis/phpredis/commit/500916a4d052aa180aa8d27a9e147e64f3ee6303)
  ([Pavlo Yatsukhnenko](https://github.com/yatsukhnenko))
- Fix memory leak in rediscluster session handler
  [b2cffffc](https://github.com/phpredis/phpredis/commit/b2cffffc107541643bab7eb81751b497bc264639)
  ([Pavlo Yatsukhnenko](https://github.com/yatsukhnenko))
- Fix XInfo() returns false if the stream is empty
  [5719c9f7](https://github.com/phpredis/phpredis/commit/5719c9f7ff8ba4595c0f2d82e9549a604d925ed7),
  [566fdeeb](https://github.com/phpredis/phpredis/commit/566fdeeb19c8112ac83cf4e47be6928626aa7b37)
  ([Pavlo Yatsukhnenko](https://github.com/yatsukhnenko),
   [Michael Grunder](https://github.com/michael-grunder))

### Changed

- Use "%.17g" sprintf format for doubles as done in Redis server.
  [32be3006](https://github.com/phpredis/phpredis/commit/32be3006e6d5a9d58636efd53fe02aa22f18c496)
  ([Pavlo Yatsukhnenko](https://github.com/yatsukhnenko))
- Relax requirements on set's expire argument
  [36458071](https://github.com/phpredis/phpredis/commit/364580718891de94aac13dc352aa994d531d4272)
  ([Michael Grunder](https://github.com/michael-grunder))
- Refactor redis_sock_check_liveness
  [c5950644](https://github.com/phpredis/phpredis/commit/c5950644e92e61e0c3f38a8ab8a380f707102eb0)
  ([Pavlo Yatsukhnenko](https://github.com/yatsukhnenko))
- PHP8 compatibility
  [a7662da7](https://github.com/phpredis/phpredis/commit/a7662da7924dcbaa74f5f2c6e1dce06b19e64bfc),
  [f4a30cb2](https://github.com/phpredis/phpredis/commit/f4a30cb2bda7414b159bf8b1be69dad52ed6f008),
  [17848791](https://github.com/phpredis/phpredis/commit/178487919148a0f8f1ad4cae62847bc4ae82ee8c)
  ([Pavlo Yatsukhnenko](https://github.com/yatsukhnenko),
   [Remi Collet](https://github.com/remicollet))
- Update documentation
  [c9ed151d](https://github.com/phpredis/phpredis/commit/c9ed151dbae1532a98c0c9322c9401c47d1da149),
  [398c99d9](https://github.com/phpredis/phpredis/commit/398c99d9851b267d9aaaa42c097c5fe54d507a6e)
  ([Ali Alwash](https://github.com/aalwash),
   [Gregoire Pineau](https://github.com/lyrixx))

### Added

- Add `Redis::OPT_NULL_MULTIBULK_AS_NULL` setting to treat NULL multi bulk replies as `NULL` instead of `[]`.
  [950e8de8](https://github.com/phpredis/phpredis/commit/950e8de807ba17ecfff62504a6ee7a959a5df714)
  ([Michael Grunder](https://github.com/michael-grunder),
   [Alex Offshore](https://github.com/offshore))
- Allow to specify stream context for rediscluster session handler
  [a8daaff8](https://github.com/phpredis/phpredis/commit/a8daaff87a055bb6b4fb8702151915f56e144649),
  [4fbe7df7](https://github.com/phpredis/phpredis/commit/4fbe7df79b9b0e03f92e8323aed0bda9513bc20a)
  ([Pavlo Yatsukhnenko](https://github.com/yatsukhnenko))
- Add new parameter to RedisCluster to specify stream ssl/tls context.
  [f771ea16](https://github.com/phpredis/phpredis/commit/f771ea16b77f39fcca555bec2d952412265197aa),
  [72024afe](https://github.com/phpredis/phpredis/commit/72024afed3640230bbd1a017b2a374d12ab88e59)
  ([Pavlo Yatsukhnenko](https://github.com/yatsukhnenko))
- Add new parameter to RedisSentinel to specify auth information
  [81c502ae](https://github.com/phpredis/phpredis/commit/81c502ae7c0de65d63cd514ee59849c9d1b0b952)
  ([Pavlo Yatsukhnenko](https://github.com/yatsukhnenko))

## [5.3.1] - 2020-07-06 ([GitHub](https://github.com/phpredis/phpredis/releases/5.3.1), [PECL](https://pecl.php.net/package/redis/5.3.1))

### Sponsors :sparkling_heart:

- [Audiomack](https://audiomack.com)
- [BlueHost](https://bluehost.com)
- [Redis Cache Pro for WordPress](https://wprediscache.com)
- [Avtandil Kikabidze](https://github.com/akalongman)

### Fixed

- Properly clean up on session start failure
  [066cff6a](https://github.com/phpredis/phpredis/commit/066cff6adee03ce05ec5d57083eb7995dfa4344d)
  ([Michael Grunder](https://github.com/michael-grunder))
- Treat NULL as a failure for redis_extract_auth_info
  [49428a2f](https://github.com/phpredis/phpredis/commit/49428a2f7072dc30a52db4155aed3d382800b1a6),
  [14ac969d](https://github.com/phpredis/phpredis/commit/14ac969da29dbf7203f8db31988ca26b9b45f583)
  ([Michael Grunder](https://github.com/michael-grunder))
- Don't dereference a NULL zend_string or try to efree it
  [ff2e160f](https://github.com/phpredis/phpredis/commit/ff2e160f408efdc97676cffaa02093e65c2ad634),
  [7fed06f2](https://github.com/phpredis/phpredis/commit/7fed60f248e2249e6cac5c5c3090509aa47647fb)
  ([Michael Grunder](https://github.com/michael-grunder))
- Fix config.m4 messages and test for and include php_hash.h
  [83a1b7c5](https://github.com/phpredis/phpredis/commit/83a1b7c5e225abd94cd3459c52bf7d502dfb0979),
  [3c56289c](https://github.com/phpredis/phpredis/commit/3c56289c71516a7c0ac81713ef2786c2b9e52274),
  [08f202e7](https://github.com/phpredis/phpredis/commit/08f202e775037ccf849d7b933dddb467c9c2ee5f),
  ([Remi Collet](https://github.com/remicollet))

### Added

- Add openSUSE installation instructions
  [13a168f4](https://github.com/phpredis/phpredis/commit/13a168f42d6639a051d6f829d573dd81bcb97f3a)
  ([Pavlo Yatsukhnenko](https://github.com/yatsukhnenko))

### Removed

- Remove EOL Fedora installation instructions
  [b4779e6a](https://github.com/phpredis/phpredis/commit/b4779e6a919103bd65fa0e6a0c88e658e05a3e7c)
  ([Remi Collet](https://github.com/remicollet))

## [5.3.0] - 2020-06-30 ([GitHub](https://github.com/phpredis/phpredis/releases/tag/5.3.0), [PECL](https://pecl.php.net/package/redis/5.3.0))

### Sponsors :sparkling_heart:

- [Audiomack](https://audiomack.com)
- [BlueHost](https://bluehost.com)
- [Redis Cache Pro for WordPress](https://wprediscache.com)
- [Avtandil Kikabidze](https://github.com/akalongman)

*There were no changes between 5.3.0RC2 and 5.3.0*

## [5.3.0RC2] - 2020-06-26 ([GitHub](https://github.com/phpredis/phpredis/releases/tag/5.3.0RC2), [PECL](https://pecl.php.net/package/redis/5.3.0RC2))

### Sponsors :sparkling_heart:

- [Audiomack](https://audiomack.com)
- [BlueHost](https://bluehost.com)
- [Redis Cache Pro for WordPress](https://wprediscache.com/)
- [Avtandil Kikabidze](https://github.com/akalongman)

### Fixed

- Fix LZ4 configuration and use pkg-config if we have it
  [df398cb0](https://github.com/phpredis/phpredis/commit/df398cb07cd10d870c6805d5834703dc39590b0f)
  ([Remi Collet](https://github.com/remicollet))

- Make sure persistent pool ID is NULL terminated
  [0838b5bd](https://github.com/phpredis/phpredis/commit/0838b5bde7ef25d419868c7e705bf6c70d68ea20),
  [57bb95bf](https://github.com/phpredis/phpredis/commit/57bb95bf5a01a2adb74e2bf73bb285488e0d1586)
  ([Michael Grunder](https://github.com/michael-grunder))

### Changed

- Run LZ4 tests in Travis
  [3ba3f06d](https://github.com/phpredis/phpredis/commit/3ba3f06d51ff126eb51dd697381c0e56b38bbcf3)
  ([Michael Grunder](https://github.com/michael-grunder))

## [5.3.0RC1]

### Sponsors :sparkling_heart:

- [Audiomack.com](https://audiomack.com)
- [BlueHost](https://bluehost.com)
- [Redis Cache Pro for WordPress](https://wprediscache.com/)
- [Avtandil Kikabidze](https://github.com/akalongman)

### Added

- Support for Redis 6 ACLs
  [a311cc4e](https://github.com/phpredis/phpredis/commit/a311cc4ec3cecdbaf83ba66985efa82137e37cc0)
  ([Michael Grunder](https://github.com/michael-grunder))

- LZ4 Compression
  [04def9fb](https://github.com/phpredis/phpredis/commit/04def9fbe2194b3b711362de57260a6cd5216e69)
  ([Ilia Alshanetsky](https://github.com/iliaal),
   [Michael Grunder](https://github.com/michael-grunder))

- Support for new Redis 6 arguments (XINFO FULL, SET KEEPTTL)
  [a0c53e0b](https://github.com/phpredis/phpredis/commit/a0c53e0b30e0c6af15cc137415e7d65f6d1867f7),
  [f9c7bb57](https://github.com/phpredis/phpredis/commit/f9c7bb5788c39614c23e3bb9ec42ec8d6d5bbaa1)
  ([Victor Kislov](https://github.com/vityank),
   [Michael Grunder](https://github.com/michael-grunder))

- Support for TLS connections
  [890ee0e6](https://github.com/phpredis/phpredis/commit/890ee0e656e545b18179cf247db94a33179ce1ab),
  [b0671296](https://github.com/phpredis/phpredis/commit/b067129678264fc1c5c0f611ce1b192e05c14669)
  ([Pavlo Yatsukhnenko](https://github.com/yatsukhnenko))

- New option Redis::SCAN_PREFIX, Redis::SCAN_NOPREFIX
  [e80600e2](https://github.com/phpredis/phpredis/commit/e80600e244b8442cb7c86e99b067966cd59bf2ee)
  ([Pavlo Yatsukhnenko](https://github.com/yatsukhnenko))

- Configurable unit test authentication arguments
  [e37f38a3](https://github.com/phpredis/phpredis/commit/e37f38a39eb4bece8f49ebd0652112dc992084a0),
  [201a9759](https://github.com/phpredis/phpredis/commit/201a97599953a9621bb8eb02dc8d5f08d16499a3)
  ([Pavlo Yatsukhnenko](https://github.com/yatsukhnenko),
   [Michael Grunder](https://github.com/michael-grunder))

### Fixed

- Improved cluster slot caching mechanism to fix a couple of bugs and make it more efficient.
  [5ca4141c](https://github.com/phpredis/phpredis/commit/5ca4141c72e23816f146b49877a6a4b8098b34c6)
  ([Michael Grunder](https://github.com/michael-grunder))

- Stop calling Redis constructor when creating a RedisArray
  [e41e19a8](https://github.com/phpredis/phpredis/commit/e41e19a8342212ee9cfe35f622804c9870d05ec2)
  ([Pavlo Yatsukhnenko](https://github.com/yatsukhnenko))

- Use ZEND_LONG_FMT instead of system `long`
  [5bf88124](https://github.com/phpredis/phpredis/commit/5bf881244dd30b5310fcfcaf5bcd8f9e2675bb01)
  ([Michael Grunder](https://github.com/michael-grunder))

- Use long for SCAN iteration to fix potential overflow
  [f13f9b7c](https://github.com/phpredis/phpredis/commit/f13f9b7c7f5e3a7d286b412541199a408a0a98bd)
  ([Victor Kislov](https://github.com/vityank))

- Fix config.m4 to test for the variable $PHP_REDIS_JSON and not the literal PHP_REDIS_JSON
  [20a3dc72](https://github.com/phpredis/phpredis/commit/20a3dc7251cb0bf450ef2a1cfeeeaeaa10355cd2)
  ([Mizuki Nakano](https://github.com/mi-nakano))

- Fix compiler warnings
  [b9b383f4](https://github.com/phpredis/phpredis/commit/b9b383f49939484dcddf1a5edefdb9d753baa7f8),
  [215828e](https://github.com/phpredis/phpredis/commit/215828e3474dfd9ea72fdc6da67aa6bee2d95ddf)
  ([Remi Collet](https://github.com/remicollet), [Pavlo Yatsukhnenko](https://github.com/yatsukhnenko))

- Avoid use-after-free of RediSock
  [8c45816d](https://github.com/phpredis/phpredis/commit/8c45816dbf4746f6557f83332be874bd78b5ce34)
  ([Pavlo Yatsukhnenko](https://github.com/yatsukhnenko))

- Fixed ZADD arginfo
  [a8e2b021](https://github.com/phpredis/phpredis/commit/a8e2b021f9eb51ad3ed0cc89064e2f004c56f8ba)
  ([Pavlo Yatsukhnenko](https://github.com/yatsukhnenko))

### Changed

- Store AUTH information in flags RedisSock rather than duplicating information.
  [58dab564](https://github.com/phpredis/phpredis/commit/58dab5649fcc2cc63f5a29df83f783e154d7fa22)
  ([Pavlo Yatsukhnenko](https://github.com/yatsukhnenko))

- Refactor redis_sock_get_connection_pool logic.
  [73212e1](https://github.com/phpredis/phpredis/commit/73212e141403ec47441142fe1c7fd5fad24f6720)
  ([Pavlo Yatsukhnenko](https://github.com/yatsukhnenko))

- Updated documentation to show LPUSH and RPUSH are variadic and fixed DEL documentation.
  [92f8dde1](https://github.com/phpredis/phpredis/commit/92f8dde1c996d4e1c3d79226b888119307612c40)
  ([Michael Grunder](https://github.com/michael-grunder))

- Authenticate in redis_server_sock_open
  [4ef465b5](https://github.com/phpredis/phpredis/commit/4ef465b57325d2d93234fd66af06a7091ce7d1ea)
  ([Pavlo Yatsukhnenko](https://github.com/yatsukhnenko))

- Dynamically include json.so in unit tests based on configuration
  [0ce7ca2f](https://github.com/phpredis/phpredis/commit/0ce7ca2fb1eb2f3c445487957a49b70ad8d4ecb6)
  (([Michael Grunder](https://github.com/michael-grunder))

- Update save_path logic in Redis Cluster session unit tests
  [dd66fce](https://github.com/phpredis/phpredis/commit/dd66fceeb232f9e1fb0a26373949e810180dc5fc)
  ([Pavlo Yatsukhnenko](https://github.com/yatsukhnenko))

- Refactoring various bits of logic
  [bbcf32a3](https://github.com/phpredis/phpredis/commit/bbcf32a37fa856ba0b50b489ba05bd3d43800fcc),
  [a42cf189](https://github.com/phpredis/phpredis/commit/a42cf189a776fc43acf47ca519f1d7385cc27f2f),
  [460c8f29](https://github.com/phpredis/phpredis/commit/460c8f29239c263e15a093c9bcdb6fb24587ec7d),
  [b7f9df75](https://github.com/phpredis/phpredis/commit/b7f9df758b30187864012d5cd831dbbc5fa053d0),
  ([Pavlo Yatsukhnenko](https://github.com/yatsukhnenko))

- Use the portable `ZEND_LONG_FORMAT` family instead of C format specifiers
  [b9b383f4](https://github.com/phpredis/phpredis/commit/b9b383f4)
  ([Remi Collet](https://github.com/remicollet))

- PHP 8 compatibility
  [9ee94ca4](https://github.com/phpredis/phpredis/commit/9ee94ca4),
  [7e4c7b3e](https://github.com/phpredis/phpredis/commit/7e4c7b3e)
  ([Pavlo Yatsukhnenko](https://github.com/yatsukhnenko))

- Refactor PHPREDIS_GET_OBJECT macro
  [d5dadaf6](https://github.com/phpredis/phpredis/commit/d5dadaf6),
  [190c0d34](https://github.com/phpredis/phpredis/commit/190c0d34)
  ([Pavlo Yatsukhnenko](https://github.com/yatsukhnenko))

- Fix documentation to show lPush and rPush are variadic
  [6808cd6a](https://github.com/phpredis/phpredis/commit/6808cd6a)
  ([Michel Grunder](https://github.com/michael-grunder))

---

## [5.2.2] - 2020-05-05 ([GitHub](https://github.com/phpredis/phpredis/releases/tag/5.2.2), [PECL](https://pecl.php.net/package/redis/5.2.2))

### Sponsors :sparkling_heart:

- [Audiomack.com](https://audiomack.com)
- [Till Krüss](https://github.com/tillkruss)

### Changed

- Inexpensive liveness check, and making ECHO optional
  [56898f81](https://github.com/phpredis/phpredis/commit/56898f81)
  ([Pavlo Yatsukhnenko](https://github.com/yatsukhnenko))

- Move `AUTH` to `redis_sock_server_open`
  [80f2529b](https://github.com/phpredis/phpredis/commit/80f2529b)
  ([Pavlo Yatsukhnenko](https://github.com/yatsukhnenko))

---

## [5.2.1] - 2020-03-19 ([GitHub](https://github.com/phpredis/phpredis/releases/tag/5.2.1), [PECL](https://pecl.php.net/package/redis/5.2.1))

### Sponsors :sparkling_heart:

- [Audiomack.com](https://audiomack.com)
- [Till Krüss](https://github.com/tillkruss)

### Fixed

- Fix arginfo for Redis::zadd
  [a8e2b021](https://github.com/phpredis/phpredis/commit/a8e2b021)
  ([Pavlo Yatsukhnenko](https://github.com/yatsukhnenko))

- Fix segfault on closing persistent stream
  [b7f9df75](https://github.com/phpredis/phpredis/commit/b7f9df75)
  ([Pavlo Yatsukhnenko](https://github.com/yatsukhnenko))

---

## [5.2.0] - 2020-03-02 ([GitHub](https://github.com/phpredis/phpredis/releases/tag/5.2.0), [PECL](https://pecl.php.net/package/redis/5.2.0))

*There were no changes between 5.2.0RC2 and 5.2.0*

## [5.2.0RC2] - 2020-02-21 ([GitHub](https://github.com/phpredis/phpredis/releases/tag/5.2.0RC2), [PECL](https://pecl.php.net/package/redis/5.2.0RC2))

### Sponsors :sparkling_heart:

- [Audiomack.com](https://audiomack.com)
- [Till Krüss](https://github.com/tillkruss)

### Fixed

- Include RedisSentinelTest.php in package.xml!
  [eddbfc8f](https://github.com/phpredis/phpredis/commit/eddbfc8f)
  ([Michel Grunder](https://github.com/michael-grunder))

- Fix -Wmaybe-uninitialized warning
  [740b8c87](https://github.com/phpredis/phpredis/commit/740b8c87)
  ([Remi Collet](https://github.com/remicollet))

- Fix improper destructor when zipping values and scores
  [371ae7ae](https://github.com/phpredis/phpredis/commit/371ae7ae)

- Use php_rand instead of php_mt_rand for liveness challenge string
  [9ef2ed89](https://github.com/phpredis/phpredis/commit/9ef2ed89)
  ([Michael Grunder](https://github.com/michael-grunder))

## [5.2.0RC1] - 2020-02-15 ([GitHub](https://github.com/phpredis/phpredis/releases/tag/5.2.0RC1), [PECL](https://pecl.php.net/package/redis/5.2.0RC1))

### Sponsors :sparkling_heart:

- [Till Krüss](https://github.com/tillkruss)

### Added

- Added challenge/response mechanism to ensure persistent connections are not in a bad state
  [a5f95925](https://github.com/phpredis/phpredis/commit/a5f95925),
  [25cdaee6](https://github.com/phpredis/phpredis/commit/25cdaee6),
  [7b6072e0](https://github.com/phpredis/phpredis/commit/7b6072e0),
  [99ebd0cc](https://github.com/phpredis/phpredis/commit/99ebd0cc),
  [3243f426](https://github.com/phpredis/phpredis/commit/3243f426)
  ([Pavlo Yatsukhnenko](https://github.com/yatsukhnenko), [Michael Grunder](https://github.com/michael-grunder))

- Experimental support for RedisSentinel
  [90cb69f3](https://github.com/phpredis/phpredis/commit/90cb69f3),
  [c94e28f1](https://github.com/phpredis/phpredis/commit/c94e28f1),
  [46da22b0](https://github.com/phpredis/phpredis/commit/46da22b0),
  [5a609fa4](https://github.com/phpredis/phpredis/commit/5a609fa4),
  [383779ed](https://github.com/phpredis/phpredis/commit/383779ed)
  ([Pavlo Yatsukhnenko](https://github.com/yatsukhnenko))

### Fixed

- Fixed ASK redirection logic
  [ba73fbee](https://github.com/phpredis/phpredis/commit/ba73fbee)
  ([Michael Grunder](https://github.com/michael-grunder))

- Create specific 'test skipped' exception
  [c3d83d44](https://github.com/phpredis/phpredis/commit/c3d83d44)
  ([Michael Grunder](https://github.com/michael-grunder))

- Fixed memory leaks in RedisCluster
  [a107c9fc](https://github.com/phpredis/phpredis/commit/a107c9fc)
  ([Michael Grunder](https://github.com/michael-grunder))

- Fixes for session lifetime values that underflow or overflow
  [7a79ad9c](https://github.com/phpredis/phpredis/commit/7a79ad9c),
  [3c48a332](https://github.com/phpredis/phpredis/commit/3c48a332)
  ([Michael Grunder](https://github.com/michael-grunder))

- Enables slot caching for Redis Cluster
  [23b1a9d8](https://github.com/phpredis/phpredis/commit/23b1a9d8)
  ([Michael Booth](https://github.com/Michael03))

- Housekeeping (spelling, doc changes, etc)
  [23f9de30](https://github.com/phpredis/phpredis/commit/23f9de30),
  [d07a8df6](https://github.com/phpredis/phpredis/commit/d07a8df6),
  [2d39b48d](https://github.com/phpredis/phpredis/commit/2d39b48d),
  [0ef488fc](https://github.com/phpredis/phpredis/commit/0ef488fc),
  [2c35e435](https://github.com/phpredis/phpredis/commit/2c35e435),
  [f52bd8a8](https://github.com/phpredis/phpredis/commit/f52bd8a8),
  [2ddc5f21](https://github.com/phpredis/phpredis/commit/2ddc5f21),
  [1ff7dfb7](https://github.com/phpredis/phpredis/commit/1ff7dfb7),
  [db446138](https://github.com/phpredis/phpredis/commit/db446138)
  ([Pavlo Yatsukhnenko](https://github.com/yatsukhnenko),
   [Tyson Andre](https://github.com/TysonAndre), [Michael Grunder](https://github.com/michael-grunder),
   [Paul DelRe](https://github.com/pdelre), [Tyson Andre](https://github.com/TysonAndre))

### Changed

- Support TYPE argument for SCAN
  [8eb39a26](https://github.com/phpredis/phpredis/commit/8eb39a26)
  [b1724b84](https://github.com/phpredis/phpredis/commit/b1724b84)
  [53fb36c9](https://github.com/phpredis/phpredis/commit/53fb36c9)
  [544e641b](https://github.com/phpredis/phpredis/commit/544e641b)
  ([Pavlo Yatsukhnenko](https://github.com/yatsukhnenko))

---

## [5.1.1] - 2019-11-11 ([GitHub](https://github.com/phpredis/phpredis/releases/tag/5.1.1), [PECL](https://pecl.php.net/package/redis/5.1.1))

### Fixed

- Fix fail to connect to redis through unix socket
  [2bae8010](https://github.com/phpredis/phpredis/commit/2bae8010),
  [9f4ededa](https://github.com/phpredis/phpredis/commit/9f4ededa)
  ([Pavlo Yatsukhnenko](https://github.com/yatsukhnenko), [Michael Grunder](https://github.com/michael-grunder))

---

## [5.1.0] - 2019-10-31 ([GitHub](https://github.com/phpredis/phpredis/releases/tag/5.1.0), [PECL](https://pecl.php.net/package/redis/5.1.0))

### Added

- Add optional support for Zstd compression, using `--enable-redis-zstd`.
  This requires libzstd version >= 1.3.0
  [2abc61da](https://github.com/phpredis/phpredis/commit/2abc61da)
  ([Remi Collet](https://github.com/remicollet))
- Add documentation for zpopmin and zpopmax
  [99ec24b3](https://github.com/phpredis/phpredis/commit/99ec24b3),
  [4ab1f940](https://github.com/phpredis/phpredis/commit/4ab1f940)
  ([alexander-schranz](https://github.com/alexander-schranz))
- Allow to specify scheme for session handler.
  [53a8bcc7](https://github.com/phpredis/phpredis/commit/53a8bcc7)
  ([Pavlo Yatsukhnenko](https://github.com/yatsukhnenko))

### Changed

- Refactor redis_session
  [91a8e734](https://github.com/phpredis/phpredis/commit/91a8e734),
  [978c3074](https://github.com/phpredis/phpredis/commit/978c3074)
  ([Pavlo Yatsukhnenko](https://github.com/yatsukhnenko))
- Fix overallocation in RedisCluster directed node commands
  [cf93649](https://github.com/phpredis/phpredis/commit/cf93649)
  ([Michael Grunder](https://github.com/michael-grunder))
- Also attach slaves when caching cluster slots
  [0d6d3fdd](https://github.com/phpredis/phpredis/commit/0d6d3fdd),
  [b114fc26](https://github.com/phpredis/phpredis/commit/b114fc26)
  ([Michael Grunder](https://github.com/michael-grunder))
- Use zend_register_persistent_resource_ex for connection pooling
  [fdada7ae](https://github.com/phpredis/phpredis/commit/fdada7ae),
  [7c6c43a6](https://github.com/phpredis/phpredis/commit/7c6c43a6)
  ([Pavlo Yatsukhnenko](https://github.com/yatsukhnenko))

### Fixed

- Fix regression for multihost_distribute_call added in [112c77e3](https://github.com/phpredis/phpredis/commit/112c77e3)
  [fbe0f804](https://github.com/phpredis/phpredis/commit/fbe0f804)
  ([Pavlo Yatsukhnenko](https://github.com/yatsukhnenko))
- Revert "fix regression for conntecting to ports > 32767" added in [1f41da64](https://github.com/phpredis/phpredis/commit/1f41da64) and add another fix
  [17b139d8](https://github.com/phpredis/phpredis/commit/17b139d8),
  [7ef17ce1](https://github.com/phpredis/phpredis/commit/7ef17ce1)
  ([Pavlo Yatsukhnenko](https://github.com/yatsukhnenko))
- Fix regression for conntecting to unix sockets with relative path added in [1f41da64](https://github.com/phpredis/phpredis/commit/1f41da64)
  [17b139d8](https://github.com/phpredis/phpredis/commit/17b139d8),
  [7ef17ce1](https://github.com/phpredis/phpredis/commit/7ef17ce1)
  ([Pavlo Yatsukhnenko](https://github.com/yatsukhnenko))
- Fix unix-socket detection logic broken in [418428fa](https://github.com/phpredis/phpredis/commit/418428fa)
  [a080b73f](https://github.com/phpredis/phpredis/commit/a080b73f)
  ([Pavlo Yatsukhnenko](https://github.com/yatsukhnenko))
- Fix memory leak and bug with getLastError for redis_mbulk_reply_assoc and redis_mbulk_reply_zipped.
  [7f42d628](https://github.com/phpredis/phpredis/commit/7f42d628),
  [3a622a07](https://github.com/phpredis/phpredis/commit/3a622a07)
  ([Pavlo Yatsukhnenko](https://github.com/yatsukhnenko)),
  ([Michael Grunder](https://github.com/michael-grunder))
- Fix bug with password contain "#" for redis_session
  [2bb08680](https://github.com/phpredis/phpredis/commit/2bb08680)
  ([Pavlo Yatsukhnenko](https://github.com/yatsukhnenko))
- Missing null byte in PHP_MINFO_FUNCTION
  [8bc2240c](https://github.com/phpredis/phpredis/commit/8bc2240c)
  ([Remi Collet](https://github.com/remicollet))

### Removed

- Dead code generic_unsubscribe_cmd
  [8ee4abbc](https://github.com/phpredis/phpredis/commit/8ee4abbc)
  ([Pavlo Yatsukhnenko](https://github.com/yatsukhnenko))

---

## [5.0.2] - 2019-07-29 ([GitHub](https://github.com/phpredis/phpredis/releases/tag/5.0.2), [PECL](https://pecl.php.net/package/redis/5.0.2))

### Fixed

- Fix regression for conntecting to ports > 32767
  [1f41da64](https://github.com/phpredis/phpredis/commit/1f41da64),
  ([Owen Smith](https://github.com/orls))
- RedisCluster segfaults after second connection with cache_slots enabled
  [f52cd237](https://github.com/phpredis/phpredis/commit/f52cd237),
  [cb5d6b94](https://github.com/phpredis/phpredis/commit/cb5d6b94)
  ([Pavlo Yatsukhnenko](https://github.com/yatsukhnenko), [Michael Grunder](https://github.com/michael-grunder))

### Changed

- Cleanup TSRMLS_* usage
  [94380227](https://github.com/phpredis/phpredis/commit/94380227)
  ([Remi Collet](https://github.com/remicollet))
- Replace ulong with zend_ulong
  [b4eb158a](https://github.com/phpredis/phpredis/commit/b4eb158a)
  ([Remi Collet](https://github.com/remicollet))
- Replace uint with uint32_t
  [d6fc5c73](https://github.com/phpredis/phpredis/commit/d6fc5c73)
  ([Remi Collet](https://github.com/remicollet))

---

## [5.0.1] - 2019-07-12 ([GitHub](https://github.com/phpredis/phpredis/releases/tag/5.0.1), [PECL](https://pecl.php.net/package/redis/5.0.1))

### Fixed

- RedisCluster segfaults after second connection with cache_slots enabled
  [327cf0bd](https://github.com/phpredis/phpredis/commit/327cf0bd)
  ([Pavlo Yatsukhnenko](https://github.com/yatsukhnenko))

---

## [5.0.0] - 2019-07-02 ([GitHub](https://github.com/phpredis/phpredis/releases/tag/5.0.0), [PECL](https://pecl.php.net/package/redis/5.0.0))

This release contains important improvements and breaking changes.  The most
interesting are: drop PHP5 support, RedisCluster slots caching, JSON and msgpack
serializers, soft deprecation of non-Redis commands.

### Breaking Changes

- [Nullable xReadGroup COUNT and BLOCK arguments](#brk500-xreadgroup)
- [RedisArray exception now includes host information](#brk500-exception-host)
- [zRange now conforms to zRangeByScore to get scores](#brk500-zrange-withscores)

### Added
- Adds OPT_REPLY_LITERAL for rawCommand and EVAL [5cb30fb2](https://www.github.com/phpredis/phpredis/commit/5cb30fb2)
  ([Michael Grunder](https://github.com/michael-grunder))
- JSON serializer [98bd2886](https://www.github.com/phpredis/phpredis/commit/98bd2886),
  [96c57139](https://www.github.com/phpredis/phpredis/commit/96c57139),
  [235a27](https://www.github.com/phpredis/phpredis/commit/235a27)
  ([Pavlo Yatsukhnenko](https://github.com/yatsukhnenko), [Michael Grunder](https://github.com/michael-grunder))
- msgpack serializer [d5b8f833](https://www.github.com/phpredis/phpredis/commit/d5b8f833),
  [545250f3](https://www.github.com/phpredis/phpredis/commit/545250f3),
  [52bae8ab](https://www.github.com/phpredis/phpredis/commit/52bae8ab)
  ([@bgort](https://github.com/bgort), [Pavlo Yatsukhnenko](https://github.com/yatsukhnenko),
   [Michael Grunder](https://github.com/michael-grunder))
- Add support for STREAM to the type command [d7450b2f](https://www.github.com/phpredis/phpredis/commit/d7450b2f),
  [068ce978](https://www.github.com/phpredis/phpredis/commit/068ce978), [8a45d18c](https://www.github.com/phpredis/phpredis/commit/8a45d18c)
  ([Michael Grunder](https://github.com/michael-grunder), [Pavlo Yatsukhnenko](https://github.com/yatsukhnenko))
- Add Cluster slots caching [9f0d7bc0](https://www.github.com/phpredis/phpredis/commit/9f0d7bc0),
  [ea081e05](https://www.github.com/phpredis/phpredis/commit/ea081e05) ([Michael Grunder](https://github.com/michael-grunder))

### Changed

- <a id="brk500-exception-host">Add server address to exception message.  This changes the exception message from `read error on connection` to
  `read error on connection to <host>:<port>` or `read error on connection to </path/to/socket>` so code matching the exception string might break.
  [e8fb49be](https://www.github.com/phpredis/phpredis/commit/e8fb49be),
  [34d6403d](https://www.github.com/phpredis/phpredis/commit/34d6403d)
  ([Pavlo Yatsukhnenko](https://github.com/yatsukhnenko))
- Allow to specify server address as `schema://host` [418428fa](https://www.github.com/phpredis/phpredis/commit/418428fa)
  ([Pavlo Yatsukhnenko](https://github.com/yatsukhnenko)).
- Update Fedora installation instructions [90aa067c](https://www.github.com/phpredis/phpredis/commit/90aa067c)
  ([@remicollet](https://github.com/remicollet))
- Enable connection pooling by default [8206b147](https://www.github.com/phpredis/phpredis/commit/8206b147)
  ([Pavlo Yatsukhnenko](https://github.com/yatsukhnenko))
- <a id="brk500-ping-argument">Allow PING to take an optional argument. PING now returns `true` instead of "+PONG" [6e494170](https://www.github.com/phpredis/phpredis/commit/6e494170)
  ([Michael Grunder](https://github.com/michael-grunder))
- <a id="brk500-zrange-withscores">Allow ZRANGE to be called either with `true` or `['withscores' => true]`
  [19f3efcf](https://www.github.com/phpredis/phpredis/commit/19f3efcf) ([Michael Grunder](https://github.com/michael-grunder))
- Documentation improvements ([@alexander-schranz](https://github.com/alexander-schranz), [@cookieguru](https://github.com/cookieguru),
  [Pavlo Yatsukhnenko](https://github.com/yatsukhnenko), [Michael Grunder](https://github.com/michael-grunder))

### Deprecated

- Soft deprecate methods that aren't actually Redis commands [a81b4f2d](https://www.github.com/phpredis/phpredis/commit/a81b4f2d),
  [95c8aab9](https://www.github.com/phpredis/phpredis/commit/95c8aab9), [235a27](https://www.github.com/phpredis/phpredis/commit/235a27) ([@michael-grunder](https://github.com/michael-grunder), [@yatsukhnenko](https://github.com/weltling))
- Remove HAVE_SPL define [[55c5586c](https://www.github.com/phpredis/phpredis/commit/55c5586c)] ([@petk](https://github.com/petk))

### Removed

- Drop PHP5 support [[f9928642](https://www.github.com/phpredis/phpredis/commit/f9928642), [46a50c12](https://www.github.com/phpredis/phpredis/commit/46a50c12), [4601887d](https://www.github.com/phpredis/phpredis/commit/4601887d), [6ebb36ce](https://www.github.com/phpredis/phpredis/commit/6ebb36ce), [fdbe9d29](https://www.github.com/phpredis/phpredis/commit/fdbe9d29)] (Michael
  Grunder)

### Fixed

- Reworked PHP msgpack >= 2.0.3 version requirement. [6973478](https://www.github.com/phpredis/phpredis/commit/6973478)..[a537df8](https://www.github.com/phpredis/phpredis/commit/a537df8)
  ([@michael-grunder](https://github.com/michael-grunder)).
- Enable pooling for cluster slave nodes [17600dd1](https://www.github.com/phpredis/phpredis/commit/17600dd1) ([Michael Grunder](https://github.com/michael-grunder))
- xInfo response format [4852a510](https://www.github.com/phpredis/phpredis/commit/4852a510), [ac9dca0a](https://www.github.com/phpredis/phpredis/commit/ac9dca0a) ([Pavlo Yatsukhnenko](https://github.com/yatsukhnenko))
- <a id="brk500-xreadgroup">Make the XREADGROUP optional COUNT and BLOCK arguments nullable
  [0c17bd27](https://www.github.com/phpredis/phpredis/commit/0c17bd27)
  ([Michael Grunder](https://github.com/michael-grunder))
- Allow persistent_id to be passed as NULL with strict_types enabled [60223762](https://www.github.com/phpredis/phpredis/commit/60223762)
  ([Michael Grunder](https://github.com/michael-grunder))
- Fix TypeError when using built-in constants in `setOption` [4c7643ee](https://www.github.com/phpredis/phpredis/commit/4c7643ee)
  ([@JoyceBabu](https://github.com/JoyceBabu))
- Handle references in MGET [60d8b679](https://www.github.com/phpredis/phpredis/commit/60d8b679) ([Michael Grunder](https://github.com/michael-grunder))

---

## [4.3.0] - 2019-03-13 ([GitHub](https://github.com/phpredis/phpredis/releases/tag/4.3.0), [PECL](https://pecl.php.net/package/redis/4.3.0))

This is probably the last release with PHP 5 support!!!

### Added

- RedisArray auth [b5549cff](https://www.github.com/phpredis/phpredis/commit/b5549cff), [339cfa2b](https://www.github.com/phpredis/phpredis/commit/339cfa2b),
  [6b411aa8](https://www.github.com/phpredis/phpredis/commit/6b411aa8) ([Pavlo Yatsukhnenko](https://github.com/yatsukhnenko))
- Add ZPOPMAX and ZPOPMIN support [46f03561](https://www.github.com/phpredis/phpredis/commit/46f03561),
  [f89e941a](https://www.github.com/phpredis/phpredis/commit/f89e941a),
  [2ec7d91a](https://www.github.com/phpredis/phpredis/commit/2ec7d91a) (@mbezhanov, [Michael Grunder](https://github.com/michael-grunder))
- Implement GEORADIUS_RO and GEORADIUSBYMEMBER_RO [22d81a94](https://www.github.com/phpredis/phpredis/commit/22d81a94) ([Michael Grunder](https://github.com/michael-grunder))
- RedisCluster auth [c5994f2a](https://www.github.com/phpredis/phpredis/commit/c5994f2a) ([Pavlo Yatsukhnenko](https://github.com/yatsukhnenko))
- Different key hashing algorithms from hash extension [850027ff](https://www.github.com/phpredis/phpredis/commit/850027ff)
  ([Pavlo Yatsukhnenko](https://github.com/yatsukhnenko))

### Changed

- Proper persistent connections pooling implementation [a3703820](https://www.github.com/phpredis/phpredis/commit/a3703820),
  [c76e00fb](https://www.github.com/phpredis/phpredis/commit/c76e00fb), [0433dc03](https://www.github.com/phpredis/phpredis/commit/0433dc03),
  [c75b3b93](https://www.github.com/phpredis/phpredis/commit/c75b3b93) ([Pavlo Yatsukhnenko](https://github.com/yatsukhnenko))
- Use zend_string for storing key hashing algorithm [8cd165df](https://www.github.com/phpredis/phpredis/commit/8cd165df),
  [64e6a57f](https://www.github.com/phpredis/phpredis/commit/64e6a57f), [Pavlo Yatsukhnenko](https://github.com/yatsukhnenko)

- Add callback parameter to subscribe/psubscribe arginfo [0653ff31](https://www.github.com/phpredis/phpredis/commit/0653ff31),
  ([Pavlo Yatsukhnenko](https://github.com/yatsukhnenko))
- Xgroup updates [15995c06](https://www.github.com/phpredis/phpredis/commit/15995c06) ([Michael Grunder](https://github.com/michael-grunder))
- Use zend_string for pipeline_cmd [e98f5116](https://www.github.com/phpredis/phpredis/commit/e98f5116) ([Pavlo Yatsukhnenko](https://github.com/yatsukhnenko))
- Implement consistent hashing algorithm for RedisArray [bb32e6f3](https://www.github.com/phpredis/phpredis/commit/bb32e6f3), [71922bf1](https://www.github.com/phpredis/phpredis/commit/71922bf1)
  ([Pavlo Yatsukhnenko](https://github.com/yatsukhnenko))
- Use zend_string for storing RedisArray hosts [602740d3](https://www.github.com/phpredis/phpredis/commit/602740d3),
  [3e7e1c83](https://www.github.com/phpredis/phpredis/commit/3e7e1c83) ([Pavlo Yatsukhnenko](https://github.com/yatsukhnenko))
- Refactor redis_sock_read_bulk_reply [bc4dbc4b](https://www.github.com/phpredis/phpredis/commit/bc4dbc4b) ([Pavlo Yatsukhnenko](https://github.com/yatsukhnenko))

### Fixed

- Don't check the number affected keys in PS_UPDATE_TIMESTAMP_FUNC [b00060ce](https://www.github.com/phpredis/phpredis/commit/b00060ce)
  ([Pavlo Yatsukhnenko](https://github.com/yatsukhnenko))
- Cancel pipeline mode without executing commands [789256d7](https://www.github.com/phpredis/phpredis/commit/789256d7)
  ([Pavlo Yatsukhnenko](https://github.com/yatsukhnenko))
- Breaking the lock acquire loop in case of network problems [61889cd7](https://www.github.com/phpredis/phpredis/commit/61889cd7)
  ([@SkydiveMarius](https://github.com/SkydiveMarius))
- Update lzf_compress to be compatible with PECL lzf extension [b27fd430](https://www.github.com/phpredis/phpredis/commit/b27fd430)
  ([@jrchamp](https://github.com/jrchamp))
- Fix RedisCluster keys memory leak [3b56b7db](https://www.github.com/phpredis/phpredis/commit/3b56b7db) ([Michael Grunder](https://github.com/michael-grunder))
- Directly use return_value in RedisCluster::keys method [ad10a49e](https://www.github.com/phpredis/phpredis/commit/ad10a49e)
  ([Pavlo Yatsukhnenko](https://github.com/yatsukhnenko))
- Fix segfault in Redis Cluster with inconsistent configuration [72749916](https://www.github.com/phpredis/phpredis/commit/72749916),
  [6e455e2e](https://www.github.com/phpredis/phpredis/commit/6e455e2e) ([Pavlo Yatsukhnenko](https://github.com/yatsukhnenko))
- Masters info leakfix [91bd7426](https://www.github.com/phpredis/phpredis/commit/91bd7426) ([Michael Grunder](https://github.com/michael-grunder))
- Remove unused parameter lazy_connect from redis_sock_create [c0793e8b](https://www.github.com/phpredis/phpredis/commit/c0793e8b)
  ([Pavlo Yatsukhnenko](https://github.com/yatsukhnenko))
- Remove useless `ZEND_ACC_[C|D]TOR`. [bc9b5597](https://www.github.com/phpredis/phpredis/commit/bc9b5597) (@[twosee](https://github.com/twose))
- Documentation improvements ([yulonghu](https://github.com/yulonghu), [@alexander-schranz](https://github.com/alexander-schranz), [@hmc](https://github.com/hmczju),
  [Pavlo Yatsukhnenko](https://github.com/yatsukhnenko), [Michael Grunder](https://github.com/michael-grunder))

---

## [4.2.0] - 2018-11-08 ([GitHub](https://github.com/phpredis/phpredis/releases/tag/4.2.0), [PECL](https://pecl.php.net/package/redis/4.2.0))

The main feature of this release is new Streams API implemented by
[Michael Grunder](https://github.com/michael-grunder).

### Added

- Streams API [2c9e0572](https://www.github.com/phpredis/phpredis/commit/2c9e0572), [0b97ec37](https://www.github.com/phpredis/phpredis/commit/0b97ec37) ([Michael Grunder](https://github.com/michael-grunder))
- Display ini entries in output of phpinfo [908ac4b3](https://www.github.com/phpredis/phpredis/commit/908ac4b3) ([Pavlo Yatsukhnenko](https://github.com/yatsukhnenko))
- Persistent connections can be closed via close method + change reconnection
  logic [1d997873](https://www.github.com/phpredis/phpredis/commit/1d997873) ([Pavlo Yatsukhnenko](https://github.com/yatsukhnenko))

### Changed

- Optimize close method [2a1ef961](https://www.github.com/phpredis/phpredis/commit/2a1ef961) ([yulonghu](https://github.com/yulonghu))
- Use a ZSET instead of SET for EVAL tests [2e412373](https://www.github.com/phpredis/phpredis/commit/2e412373) ([Michael Grunder](https://github.com/michael-grunder))
- Modify session testing logic [bfd27471](https://www.github.com/phpredis/phpredis/commit/bfd27471) ([Michael Grunder](https://github.com/michael-grunder))
- Documentation improvements ([@michael-grunder](https://github.com/michael-grunder), [@elcheco](https://github.com/elcheco), [@lucascourot](https://github.com/lucascourot), [@nolimitdev](https://github.com/nolimitdev),
  [Michael Grunder](https://github.com/michael-grunder))

### Fixed

- Prevent potential infinite loop for sessions [4e2de158](https://www.github.com/phpredis/phpredis/commit/4e2de158) ([Pavlo Yatsukhnenko](https://github.com/yatsukhnenko))
- Fix coverty warnings [6f7ddd27](https://www.github.com/phpredis/phpredis/commit/6f7ddd27) ([Pavlo Yatsukhnenko](https://github.com/yatsukhnenko))
- Fix session memory leaks [071a1d54](https://www.github.com/phpredis/phpredis/commit/071a1d54), [92f14b14](https://www.github.com/phpredis/phpredis/commit/92f14b14) ([Pavlo Yatsukhnenko](https://github.com/yatsukhnenko),
  [Michael Grunder](https://github.com/michael-grunder))
- Fix XCLAIM on 32-bit installs [18dc2aac](https://www.github.com/phpredis/phpredis/commit/18dc2aac) ([Michael Grunder](https://github.com/michael-grunder))
- Build warning fixes [b5093910](https://www.github.com/phpredis/phpredis/commit/b5093910), [51027044](https://www.github.com/phpredis/phpredis/commit/51027044), [8b0f28cd](https://www.github.com/phpredis/phpredis/commit/8b0f28cd) ([Pavlo Yatsukhnenko](https://github.com/yatsukhnenko),
  [Remi Collet](https://github.com/remicollet), [twosee](https://github.com/twose))
- Fix incorrect arginfo for `Redis::sRem` and `Redis::multi` [25b043ce](https://www.github.com/phpredis/phpredis/commit/25b043ce)
  ([Pavlo Yatsukhnenko](https://github.com/yatsukhnenko))
- Treat a -1 response from cluster_check_response as a timeout. [27df9220](https://www.github.com/phpredis/phpredis/commit/27df9220),
  [07ef7f4e](https://www.github.com/phpredis/phpredis/commit/07ef7f4e), [d1172426](https://www.github.com/phpredis/phpredis/commit/d1172426) ([Michael Grunder](https://github.com/michael-grunder)).
- Missing space between command and args [0af2a7fe](https://www.github.com/phpredis/phpredis/commit/0af2a7fe) ([@remicollet](https://github.com/remicollet))
- Reset the socket after a timeout to make sure no wrong data is received
  [cd6ebc6d](https://www.github.com/phpredis/phpredis/commit/cd6ebc6d) ([@marcdejonge](https://github.com/marcdejonge))
- Allow '-' and '+' arguments and add tests for zLexCount and zRemRangeByLex
  [d4a08697](https://www.github.com/phpredis/phpredis/commit/d4a08697) ([Michael Grunder](https://github.com/michael-grunder))
- Fix printf format warnings [dcde9331](https://www.github.com/phpredis/phpredis/commit/dcde9331) ([Pavlo Yatsukhnenko](https://github.com/yatsukhnenko))
- Session module is required [58bd8cc8](https://www.github.com/phpredis/phpredis/commit/58bd8cc8) ([@remicollet](https://github.com/remicollet))
- Set default values for ini entries [e206ce9c](https://www.github.com/phpredis/phpredis/commit/e206ce9c) ([Pavlo Yatsukhnenko](https://github.com/yatsukhnenko))

---

## [4.0.0] - 2018-03-07 ([GitHub](https://github.com/phpredis/phpredis/releases/tag/4.0.0), [PECL](https://pecl.php.net/package/redis/4.0.0))

*WARNING:* THIS RELEASE CONTAINS BREAKING API CHANGES!

### Added

- Add proper ARGINFO for all methods. ([Pavlo Yatsukhnenko](https://github.com/yatsukhnenko), [Michael Grunder](https://github.com/michael-grunder))
- Let EXISTS take multiple keys [cccc39](https://www.github.com/phpredis/phpredis/commit/cccc39) ([Michael Grunder](https://github.com/michael-grunder))
- Implement SWAPDB and UNLINK commands [84f1f28b](https://www.github.com/phpredis/phpredis/commit/84f1f28b), [9e65c429](https://www.github.com/phpredis/phpredis/commit/9e65c429) ([Michael Grunder](https://github.com/michael-grunder))
- Add LZF compression (experimental) [e2c51251](https://www.github.com/phpredis/phpredis/commit/e2c51251), [8cb2d5bd](https://www.github.com/phpredis/phpredis/commit/8cb2d5bd), [8657557](https://www.github.com/phpredis/phpredis/commit/8657557) ([Pavlo Yatsukhnenko](https://github.com/yatsukhnenko))
- Allow mixing MULTI and PIPELINE modes (experimental) [5874b0](https://www.github.com/phpredis/phpredis/commit/5874b0) ([Pavlo Yatsukhnenko](https://github.com/yatsukhnenko))

### Changed

- Use zend_string as returning value for ra_extract_key and ra_call_extractor
  [9cd05911](https://www.github.com/phpredis/phpredis/commit/9cd05911) ([Pavlo Yatsukhnenko](https://github.com/yatsukhnenko))
- Return real connection error as exception [5b9c0c60](https://www.github.com/phpredis/phpredis/commit/5b9c0c60) ([Pavlo Yatsukhnenko](https://github.com/yatsukhnenko),
  [Michael Grunder](https://github.com/michael-grunder))
- Use zend_string for storing auth and prefix members [4b8336f7](https://www.github.com/phpredis/phpredis/commit/4b8336f7)
  ([Pavlo Yatsukhnenko](https://github.com/yatsukhnenko))
- Add tcp_keepalive option to redis sock [68c58513](https://www.github.com/phpredis/phpredis/commit/68c58513), [5101172a](https://www.github.com/phpredis/phpredis/commit/5101172a), [010336d5](https://www.github.com/phpredis/phpredis/commit/010336d5),
  [51e48729](https://www.github.com/phpredis/phpredis/commit/51e48729) ([@git-hulk](https://github.com/git-hulk), [Michael Grunder](https://github.com/michael-grunder))
- More robust GEORADIUS COUNT validation [f7edee5d](https://www.github.com/phpredis/phpredis/commit/f7edee5d) ([Michael Grunder](https://github.com/michael-grunder))
- Allow to use empty string as persistent_id [ec4fd1bd](https://www.github.com/phpredis/phpredis/commit/ec4fd1bd) ([Pavlo Yatsukhnenko](https://github.com/yatsukhnenko))
- Documentation improvements ([Michael Grunder](https://github.com/michael-grunder), [@TomA-R](https://github.com/TomA-R))

### Fixed

- Disallow using empty string as session name. [485db46f](https://www.github.com/phpredis/phpredis/commit/485db46f) ([Pavlo Yatsukhnenko](https://github.com/yatsukhnenko))
- The element of z_seeds may be a reference on php7 [367bc6aa](https://www.github.com/phpredis/phpredis/commit/367bc6aa), [1e63717a](https://www.github.com/phpredis/phpredis/commit/1e63717a)
  ([@janic716](https://github.com/janic716))
- Avoid connection in helper methods [91e9cfe1](https://www.github.com/phpredis/phpredis/commit/91e9cfe1) ([Pavlo Yatsukhnenko](https://github.com/yatsukhnenko))
- Don't use convert_to_string in redis_hmget_cmd [99335d6](https://www.github.com/phpredis/phpredis/commit/99335d6) ([Pavlo Yatsukhnenko](https://github.com/yatsukhnenko))
- PHP >=7.3.0 uses zend_string to store `php_url` elements [b566fb44](https://www.github.com/phpredis/phpredis/commit/b566fb44) ([@fmk](https://github.com/fmk))

---

## [3.1.5] - 2017-09-27 ([GitHub](https://github.com/phpredis/phpredis/releases/tag/3.1.5), [PECL](https://pecl.php.net/package/redis/3.1.5))

This is interim release which contains only bug fixes.

### Fixed

- Fix segfault when extending Redis class in PHP 5 [d23eff](https://www.github.com/phpredis/phpredis/commit/d23eff) ([Pavlo Yatsukhnenko](https://github.com/yatsukhnenko))
- Fix RedisCluster constructor with PHP 7 strict scalar type [5c21d7](https://www.github.com/phpredis/phpredis/commit/5c21d7)
  ([Pavlo Yatsukhnenko](https://github.com/yatsukhnenko))
- Allow to use empty string as persistent_id [344de5](https://www.github.com/phpredis/phpredis/commit/344de5) ([Pavlo Yatsukhnenko](https://github.com/yatsukhnenko))
- Fix cluster_init_seeds. [db1347](https://www.github.com/phpredis/phpredis/commit/db1347) ([@adlagares](https://github.com/adlagares))
- Fix z_seeds may be a reference [42581a](https://www.github.com/phpredis/phpredis/commit/42581a) ([@janic716](https://github.com/janic716))
- PHP >=7.3 uses zend_string for php_url elements [b566fb](https://www.github.com/phpredis/phpredis/commit/b566fb) ([@fmk](https://github.com/fmk))

---

## [3.1.4] - 2017-09-27 ([GitHub](https://github.com/phpredis/phpredis/releases/tag/3.1.4), [PECL](https://pecl.php.net/package/redis/3.1.4))

The primary new feature phpredis 3.1.4 is the ability to send MULTI .. EXEC
blocks in pipeline mode.  There are also many bugfixes and minor improvements
to the api, listed below.

### Added

- Allow mixing MULTI and PIPELINE modes (experimental)!  [5874b0](https://www.github.com/phpredis/phpredis/commit/5874b0)
  ([Pavlo Yatsukhnenko](https://github.com/yatsukhnenko))
- Added integration for coverty static analysis and fixed several warnings
  [faac8b0](https://www.github.com/phpredis/phpredis/commit/faac8b0), [eff7398](https://www.github.com/phpredis/phpredis/commit/eff7398), [4766c25](https://www.github.com/phpredis/phpredis/commit/4766c25), [0438ab4](https://www.github.com/phpredis/phpredis/commit/0438ab4), [1e0b065](https://www.github.com/phpredis/phpredis/commit/1e0b065), [733732a](https://www.github.com/phpredis/phpredis/commit/733732a), [26eeda5](https://www.github.com/phpredis/phpredis/commit/26eeda5), [735025](https://www.github.com/phpredis/phpredis/commit/735025),
   [42f1c9](https://www.github.com/phpredis/phpredis/commit/42f1c9), [af71d4](https://www.github.com/phpredis/phpredis/commit/af71d4) ([Pavlo Yatsukhnenko](https://github.com/yatsukhnenko)).
- Added arginfo introspection structures [81a0303](https://www.github.com/phpredis/phpredis/commit/81a0303), [d5609fc](https://www.github.com/phpredis/phpredis/commit/d5609fc), [e5660be](https://www.github.com/phpredis/phpredis/commit/e5660be), [3c60e1f](https://www.github.com/phpredis/phpredis/commit/3c60e1f),
  [50dcb15](https://www.github.com/phpredis/phpredis/commit/50dcb15), [6c2c6fa](https://www.github.com/phpredis/phpredis/commit/6c2c6fa), [212e323](https://www.github.com/phpredis/phpredis/commit/212e323), [e23be2c](https://www.github.com/phpredis/phpredis/commit/e23be2c), [682593d](https://www.github.com/phpredis/phpredis/commit/682593d), [f8de702](https://www.github.com/phpredis/phpredis/commit/f8de702), [4ef3acd](https://www.github.com/phpredis/phpredis/commit/4ef3acd), [f116be9](https://www.github.com/phpredis/phpredis/commit/f116be9),
  [5c111dd](https://www.github.com/phpredis/phpredis/commit/5c111dd), [9caa029](https://www.github.com/phpredis/phpredis/commit/9caa029), [0d69650](https://www.github.com/phpredis/phpredis/commit/0d69650), [6859828](https://www.github.com/phpredis/phpredis/commit/6859828), [024e593](https://www.github.com/phpredis/phpredis/commit/024e593), [3643ab6](https://www.github.com/phpredis/phpredis/commit/3643ab6), [f576fab](https://www.github.com/phpredis/phpredis/commit/f576fab), [122d41f](https://www.github.com/phpredis/phpredis/commit/122d41f),
  [a09d0e6](https://www.github.com/phpredis/phpredis/commit/a09d0e6) ([Tyson Andre](https://github.com/TysonAndre), [Pavlo Yatsukhnenko](https://github.com/yatsukhnenko)).
- Added a github issue template [61aba9](https://www.github.com/phpredis/phpredis/commit/61aba9) ([Pavlo Yatsukhnenko](https://github.com/yatsukhnenko))

### Changed

- Refactor redis_send_discard [ea15ce](https://www.github.com/phpredis/phpredis/commit/ea15ce) ([Pavlo Yatsukhnenko](https://github.com/yatsukhnenko))
- Improve redis array rehash [577a91](https://www.github.com/phpredis/phpredis/commit/577a91) ([Pavlo Yatsukhnenko](https://github.com/yatsukhnenko))
- Change redis array pure_cmds from zval to hashtable [a56ed7](https://www.github.com/phpredis/phpredis/commit/a56ed7) ([Pavlo Yatsukhnenko](https://github.com/yatsukhnenko))
- Use zend_string rather than char for various context fields (err, prefix, etc)
  [2bf7b2](https://www.github.com/phpredis/phpredis/commit/2bf7b2) ([Pavlo Yatsukhnenko](https://github.com/yatsukhnenko))
- Updated runtime exception handling [8dcaa4](https://www.github.com/phpredis/phpredis/commit/8dcaa4), [7c1407](https://www.github.com/phpredis/phpredis/commit/7c1407) ([Pavlo Yatsukhnenko](https://github.com/yatsukhnenko))

### Fixed

- Fixed link to redis cluster documentation [3b0b06](https://www.github.com/phpredis/phpredis/commit/3b0b06) ([Pavlo Yatsukhnenko](https://github.com/yatsukhnenko))
- Removed duplicate HGET in redis array hash table, formatting [d0b9c5](https://www.github.com/phpredis/phpredis/commit/d0b9c5)
  ([Pavlo Yatsukhnenko](https://github.com/yatsukhnenko)).
- Treat NULL bulk as success for session read [659450](https://www.github.com/phpredis/phpredis/commit/659450) ([Pavlo Yatsukhnenko](https://github.com/yatsukhnenko))
- Fix valgrind warnings [471ce07](https://www.github.com/phpredis/phpredis/commit/471ce07), [1ab89e1](https://www.github.com/phpredis/phpredis/commit/1ab89e1), [b624a8b](https://www.github.com/phpredis/phpredis/commit/b624a8b) ([Pavlo Yatsukhnenko](https://github.com/yatsukhnenko))
- Fix php5/php7 compatibility layer [1ab89e](https://www.github.com/phpredis/phpredis/commit/1ab89e), [4e3225](https://www.github.com/phpredis/phpredis/commit/4e3225) ([Pavlo Yatsukhnenko](https://github.com/yatsukhnenko))
- Fix typo in README.markdown [e47e44](https://www.github.com/phpredis/phpredis/commit/e47e44) ([Toby Schrapel](https://github.com/schrapel))
- Initialize gc member of zend_string [37f569](https://www.github.com/phpredis/phpredis/commit/37f569) ([Pavlo Yatsukhnenko](https://github.com/yatsukhnenko)).
- Don't try to set TCP_NODELAY on a unix socket and don't warn on multiple
  calls to pipeline [d11798](https://www.github.com/phpredis/phpredis/commit/d11798), [77aeba](https://www.github.com/phpredis/phpredis/commit/77aeba) ([Michael Grunder](https://github.com/michael-grunder))
- Various other library fixes [142b51](https://www.github.com/phpredis/phpredis/commit/142b51), [4452f6](https://www.github.com/phpredis/phpredis/commit/4452f6), [e672f4](https://www.github.com/phpredis/phpredis/commit/e672f4), [658ee3](https://www.github.com/phpredis/phpredis/commit/658ee3), [c9df77](https://www.github.com/phpredis/phpredis/commit/c9df77), [4a0a46](https://www.github.com/phpredis/phpredis/commit/4a0a46)
  ([Pavlo Yatsukhnenko](https://github.com/yatsukhnenko))

### Removed

- Remove unused PHP_RINIT and PHP_RSHUTDOWN functions [c760bf](https://www.github.com/phpredis/phpredis/commit/c760bf) ([Pavlo Yatsukhnenko](https://github.com/yatsukhnenko))

---

## [3.1.3] - 2017-07-15 ([GitHub](https://github.com/phpredis/phpredis/releases/tag/3.1.3), [PECL](https://pecl.php.net/package/redis/3.1.3))

This release contains two big improvements:

1. Adding a new printf like command construction function with additionally
   format specifiers specific to phpredis.
2. Implementation of custom objects for Redis and RedisArray which eliminates
   double hash lookup.

Also many small improvements and bug fixes were made.

### Added

- Add hStrLen command [c52077](https://www.github.com/phpredis/phpredis/commit/c52077), [fb88e1](https://www.github.com/phpredis/phpredis/commit/fb88e1) ([Pavlo Yatsukhnenko](https://github.com/yatsukhnenko))
- A printf like method to construct a Redis RESP command [a4a0ed](https://www.github.com/phpredis/phpredis/commit/a4a0ed), [d75081](https://www.github.com/phpredis/phpredis/commit/d75081),
  [bdd287](https://www.github.com/phpredis/phpredis/commit/bdd287), [0eaeae](https://www.github.com/phpredis/phpredis/commit/0eaeae), [b3d00d](https://www.github.com/phpredis/phpredis/commit/b3d00d) ([Michael Grunder](https://github.com/michael-grunder))
- Use custom objects instead of zend_list for storing Redis/RedisArray [a765f8](https://www.github.com/phpredis/phpredis/commit/a765f8),
  [8fa85a](https://www.github.com/phpredis/phpredis/commit/8fa85a) ([Pavlo Yatsukhnenko](https://github.com/yatsukhnenko))
- Add configureoption tag to package.xml [750963](https://www.github.com/phpredis/phpredis/commit/750963) ([Pavlo Yatsukhnenko](https://github.com/yatsukhnenko))

### Changed

- Add optional COUNT argument to sPop [d2e203](https://www.github.com/phpredis/phpredis/commit/d2e203) ([Michael Grunder](https://github.com/michael-grunder))
- Allow sInterStore to take one arg [26aec4](https://www.github.com/phpredis/phpredis/commit/26aec4), [4cd06b](https://www.github.com/phpredis/phpredis/commit/4cd06b) ([Michael Grunder](https://github.com/michael-grunder))
- Allow MIGRATE to accept multiple keys [9aa3db](https://www.github.com/phpredis/phpredis/commit/9aa3db) ([Michael Grunder](https://github.com/michael-grunder))
- Use crc32 table from PHP distro [f81694](https://www.github.com/phpredis/phpredis/commit/f81694) ([Pavlo Yatsukhnenko](https://github.com/yatsukhnenko))
- Throw exception for all non recoverable errors [e37239](https://www.github.com/phpredis/phpredis/commit/e37239) ([Pavlo Yatsukhnenko](https://github.com/yatsukhnenko))
- Increase read buffers size [520e06](https://www.github.com/phpredis/phpredis/commit/520e06) ([Pavlo Yatsukhnenko](https://github.com/yatsukhnenko))
- Better documentation [f0c25a](https://www.github.com/phpredis/phpredis/commit/f0c25a), [c5991f](https://www.github.com/phpredis/phpredis/commit/c5991f), [9ec9ae](https://www.github.com/phpredis/phpredis/commit/9ec9ae) ([Michael Grunder](https://github.com/michael-grunder))
- Better TravisCI integration [e37c08](https://www.github.com/phpredis/phpredis/commit/e37c08) ([Pavlo Yatsukhnenko](https://github.com/yatsukhnenko))

### Fixed

- Make sure redisCluster members are all initialized on (re)creation [162d88](https://www.github.com/phpredis/phpredis/commit/162d88)
- ([Michael Grunder](https://github.com/michael-grunder)).
- Fix Null Bulk String response parsing in cluster library [058753](https://www.github.com/phpredis/phpredis/commit/058753)
- ([Alberto Fernández](https://github.com/albertofem))
- Allow using numeric string in zInter command [ba0070](https://www.github.com/phpredis/phpredis/commit/ba0070) ([Pavlo Yatsukhnenko](https://github.com/yatsukhnenko))
- Use ZVAL_DEREF macros for dereference input variables [ad4596](https://www.github.com/phpredis/phpredis/commit/ad4596)
- ([Pavlo Yatsukhnenko](https://github.com/yatsukhnenko))
- Fix read_timeout [18149e](https://www.github.com/phpredis/phpredis/commit/18149e), [b56dc4](https://www.github.com/phpredis/phpredis/commit/b56dc4) ([Pavlo Yatsukhnenko](https://github.com/yatsukhnenko))
- Fix zval_get_string impl for PHP5 [4e56ba](https://www.github.com/phpredis/phpredis/commit/4e56ba) ([Pavlo Yatsukhnenko](https://github.com/yatsukhnenko))
- Fix Redis/RedisArray segfaults [be5c1f](https://www.github.com/phpredis/phpredis/commit/be5c1f), [635c3a](https://www.github.com/phpredis/phpredis/commit/635c3a), [1f8dde](https://www.github.com/phpredis/phpredis/commit/1f8dde), [43e1e0](https://www.github.com/phpredis/phpredis/commit/43e1e0)
- ([Pavlo Yatsukhnenko](https://github.com/yatsukhnenko))
- Fix memory leak and potential segfault [aa6ff77a](https://www.github.com/phpredis/phpredis/commit/aa6ff77a), [88efaa](https://www.github.com/phpredis/phpredis/commit/88efaa) ([Michael Grunder](https://github.com/michael-grunder))
- Assume "NULL bulk" reply as success (empty session data) [4a81e1](https://www.github.com/phpredis/phpredis/commit/4a81e1)
  ([Pavlo Yatsukhnenko](https://github.com/yatsukhnenko))
- Refactoring ([Pavlo Yatsukhnenko](https://github.com/yatsukhnenko), [Michael Grunder](https://github.com/michael-grunder))

---

## [3.1.2] - 2017-03-16 ([GitHub](https://github.com/phpredis/phpredis/releases/tag/3.1.2), [PECL](https://pecl.php.net/package/redis/3.1.2))

### Changes

- Re allow single array for sInterStore [6ef0c2](https://www.github.com/phpredis/phpredis/commit/6ef0c2), [d01966](https://www.github.com/phpredis/phpredis/commit/d01966) ([Michael Grunder](https://github.com/michael-grunder))
- Better TravisCI integration [4fd2f6](https://www.github.com/phpredis/phpredis/commit/4fd2f6) ([Pavlo Yatsukhnenko](https://github.com/yatsukhnenko))

### Fixed

- RedisArray segfault fix [564ce3](https://www.github.com/phpredis/phpredis/commit/564ce3) ([Pavlo Yatsukhnenko](https://github.com/yatsukhnenko))
- Small memory leak fix [645888b](https://www.github.com/phpredis/phpredis/commit/645888b) (Mike Grunder)
- Segfault fix when recreating RedisCluster objects [abf7d4](https://www.github.com/phpredis/phpredis/commit/abf7d4) ([Michael Grunder](https://github.com/michael-grunder))
- Fix for RedisCluster bulk response parsing [4121c4](https://www.github.com/phpredis/phpredis/commit/4121c4) ([Alberto Fernández](https://github.com/albertofem))

---

## [3.1.1] - 2017-02-01 ([GitHub](https://github.com/phpredis/phpredis/releases/tag/3.1.1), [PECL](https://pecl.php.net/package/redis/3.1.1))

This release contains mostly fixes for issues introduced when merging
the php 5 and 7 codebase into a single branch.

- Additional test updates for 32 bit systems ([@remicollet](https://github.com/remicollet))
- ARM rounding issue in tests ([@remicollet](https://github.com/remicollet))
- Use new zend_list_close instead of zend_list_delete when reconnecting.
- Refactoring of redis_boolean_response_impl and redis_sock_write ([@yatsukhnenko](https://github.com/weltling))
- Fixed a segfault in igbinary serialization ([@yatsukhnenko](https://github.com/weltling))
- Restore 2.2.8/3.0.0 functionality to distinguish between an error
  and simply empty session data. ([@remicollet](https://github.com/remicollet))
- Fix double to string conversion function ([@yatsukhnenko](https://github.com/weltling))
- Use PHP_FE_END definition when available ([@remicollet](https://github.com/remicollet))
- Fixed various 'static function declared but not used' warnings
- Fixes to various calls which were typecasting pointers to the
- wrong size. ([@remicollet](https://github.com/remicollet))
-
- Added php session unit test ([@yatsukhnenko](https://github.com/weltling))
- Added explicit module dependency for igbinary ([@remicollet](https://github.com/remicollet))
- Added phpinfo serialization information ([@remicollet](https://github.com/remicollet))

---

## [3.1.0] - 2016-12-14 ([GitHub](https://github.com/phpredis/phpredis/releases/3.1.0), [PECL](https://pecl.php.net/package/redis/3.1.0))

In this version of phpredis codebase was unified to work with all versions of php \o/
Also many bug fixes and some improvements has been made.

### Added

- Support the client to Redis Cluster just having one master ([andyli](https://github.com/andyli029)) [892e5646](https://www.github.com/phpredis/phpredis/commit/892e5646)
- Allow both long and strings that are longs for zrangebyscore offset/limit
  ([Michael Grunder](https://github.com/michael-grunder)) [bdcdd2aa](https://www.github.com/phpredis/phpredis/commit/bdcdd2aa)
- Process NX|XX, CH and INCR options in zAdd command [71c9f7c8](https://www.github.com/phpredis/phpredis/commit/71c9f7c8)
  ([Pavlo Yatsukhnenko](https://github.com/yatsukhnenko))

### Fixed

- Fix incrby/decrby for large integers ([Michael Grunder](https://github.com/michael-grunder)) [3a12758a](https://www.github.com/phpredis/phpredis/commit/3a12758a)
- Use static declarations for spl_ce_RuntimeException decl [a9857d69](https://www.github.com/phpredis/phpredis/commit/a9857d69)
  ([Jeremy Mikola](https://github.com/jmikola))
- Fixed method call problem causes session handler to display two times
  [24f86c49](https://www.github.com/phpredis/phpredis/commit/24f86c49) ([ZiHang Gao](https://github.com/cdoco)).
- PSETEX method returns '+OK' on success, not true [afcd8445](https://www.github.com/phpredis/phpredis/commit/afcd8445) ([sitri@ndxbn](https://github.com/ndxbn))
- Fix integer overflow for long (>32bit) increments in hIncrBy [58e1d799](https://www.github.com/phpredis/phpredis/commit/58e1d799)
  ([@iyesin](https://github.com/iyesin))
- Move zend_object handler to the end ([Michael Grunder](https://github.com/michael-grunder)) [34107966](https://www.github.com/phpredis/phpredis/commit/34107966)
- Using setOption on redis array causes immediate connection [f1a85b38](https://www.github.com/phpredis/phpredis/commit/f1a85b38)
  ([Pavlo Yatsukhnenko](https://github.com/yatsukhnenko))

---

## [3.0.0] - 2016-06-10 ([GitHub](https://github.com/phpredis/phpredis/releases/3.0.0), [PECL](https://pecl.php.net/package/redis/3.0.0))

This version of phpredis supports cluster and is intended for php versions
7.0.0 and higher. To compile cluster-enabled phpredis for older versions
of php, please use the 2.2.8 pecl package.

A huge thanks goes out to Sean DuBois for doing all the work required to get
phpredis working in php 7.0!

### Added

- PHP 7 Support [3159bd2](https://www.github.com/phpredis//phpredis/commit/3159bd2),
  [567dc2f](https://www.github.com/phpredis//phpredis/commit/567dc2f), [daa4d9f](https://www.github.com/phpredis//phpredis/commit/daa4d9f),
  [f2711e3](https://www.github.com/phpredis//phpredis/commit/f2711e3), [9cb9d07](https://www.github.com/phpredis//phpredis/commit/9cb9d07),
  [d51c89](https://www.github.com/phpredis//phpredis/commit/d51c89), [9ff8f49](https://www.github.com/phpredis//phpredis/commit/9ff8f49),
  [33bb629](https://www.github.com/phpredis//phpredis/commit/33bb629), [cbdf65a](https://www.github.com/phpredis//phpredis/commit/cbdf65a),
  [f30b7fd](https://www.github.com/phpredis//phpredis/commit/f30b7fd), [c687a51](https://www.github.com/phpredis//phpredis/commit/c687a51),
  [6b3e773](https://www.github.com/phpredis//phpredis/commit/6b3e773), [2bf8241](https://www.github.com/phpredis//phpredis/commit/2bf8241),
  [71bd3d](https://www.github.com/phpredis//phpredis/commit/71bd3d), [9221ca4](https://www.github.com/phpredis//phpredis/commit/9221ca4),
  [4e00df6](https://www.github.com/phpredis//phpredis/commit/4e00df6), [e2407ca](https://www.github.com/phpredis//phpredis/commit/e2407ca),
  [97fcfe6](https://www.github.com/phpredis//phpredis/commit/97fcfe6), [77e6200](https://www.github.com/phpredis//phpredis/commit/77e6200)
  [Sean DuBois](https://github.com/Sean-Der)
- Redis Cluster support
- IPv6 support

### Changed

- Allow SINTERSTORE to take a single array argument again
- Exception handling improvement [Jan-E](https://github.com/Jan-E) [314a2c3c](https://www.github.com/phpredis//phpredis/commit/314a2c3c)
- Allow '-' and '+' in ZRANGEBYLEX [Patrick Pokatilo](https://github.com/SHyx0rmZ) [8bfa2188](https://www.github.com/phpredis//phpredis/commit/8bfa2188)

### Fixed

- config.w32 fix [Jan-E](https://github.com/Jan-E) [495d308](https://www.github.com/phpredis//phpredis/commit/495d308), [c9e0b682](https://www.github.com/phpredis//phpredis/commit/c9e0b682)
- Unit test fix for max int value [Jan-E](https://github.com/Jan-E) [659ea2aa](https://www.github.com/phpredis//phpredis/commit/659ea2aa)
- unsigned long -> zend_ulong fix [Jan-E](https://github.com/Jan-E) [4d66e3d4](https://www.github.com/phpredis//phpredis/commit/4d66e3d4)
- Visual Stuio 14 fixes [Jan-E](https://github.com/Jan-E) [ea98401c](https://www.github.com/phpredis//phpredis/commit/ea98401c)
- Segfault fix when looking up our socket [ephemeralsnow](https://github.com/ephemeralsnow) [0126481a](https://www.github.com/phpredis//phpredis/commit/0126481a)
- Documentation fixes [Ares](https://github.com/ares333) [54b9a0ec](https://www.github.com/phpredis//phpredis/commit/54b9a0ec)
- php7 related memory leak fix [Stuart Carnie](https://github.com/stuartcarnie) [b75bf3b4](https://www.github.com/phpredis//phpredis/commit/b75bf3b4)
- Potential segfault fix in cluster session [Sergei Lomakov](https://github.com/sapfeer0k) [661fb5b1](https://www.github.com/phpredis//phpredis/commit/661fb5b1)
- php7 related serialization leak fix (Adam Harvey) [c40fc1d8](https://www.github.com/phpredis//phpredis/commit/c40fc1d8)

---

## [2.2.8] - 2016-06-02 ([GitHub](https://github.com/phpredis/phpredis/releases/2.2.8), [PECL](https://pecl.php.net/package/redis/2.2.8))

The main improvement in this version of phpredis is support for Redis
Cluster.  This version of phpredis is intended for versions of php older
than 7.

### Added

- Added randomization to our seed nodes to balance which instance is used
  to map the keyspace [32eb1c5f](https://www.github.com/phpredis/phpredis/commit/32eb1c5f) (Vitaliy Stepanyuk)
- Added support for IPv6 addresses

### Fixed

- PHP liveness checking workaround (Shafreeck Sea) [c18d58b9](https://www.github.com/phpredis/phpredis/commit/c18d58b9)
- Various documentation and code formatting and style fixes ([ares333](https://github.com/ares333),
  [sanpili](https://github.com/sanpili), [Bryan Nelson](https://github.com/bplus), [linfangrong](https://github.com/linfangrong), [Romero Malaquias](https://github.com/RomeroMalaquias), [Viktor Szépe](https://github.com/szepeviktor))
- Fix scan reply processing to use long instead of int to avoid overflow
  [mixiaojiong](https://github.com/mixiaojiong)).
- Fix potential segfault in Redis Cluster session storage [cc15aae](https://www.github.com/phpredis/phpredis/commit/cc15aae)
  ([Sergei Lomakov](https://github.com/sapfeer0k)).
- Fixed memory leak in discard function [17b1f427](https://www.github.com/phpredis/phpredis/commit/17b1f427)
- Sanity check for igbinary unserialization
  [3266b222](https://www.github.com/phpredis/phpredis/commit/3266b222), [528297a](https://www.github.com/phpredis/phpredis/commit/528297a) ([Maurus Cuelenaere](https://github.com/mcuelenaere)).
- Fix segfault occurring from unclosed socket connection for Redis Cluster
  [04196aee](https://www.github.com/phpredis/phpredis/commit/04196aee) ([CatKang](https://github.com/CatKang))
- Case insensitive zRangeByScore options
- Fixed dreaded size_t vs long long compiler warning

---

## [2.2.7] - 2015-03-03 ([GitHub](https://github.com/phpredis/phpredis/releases/tag/2.2.7), [PECL](https://pecl.php.net/package/redis/2.2.7))

### Added

- Implemented PFADD, PFMERGE, and PFCOUNT command handling
- Implemented ZRANGEBYLEX command (holding off on ZREVRANGEBYLEX
  as that won't be out until 3.0)
- Implemented getMode() so clients can detect whether we're in
  ATOMIC/MULTI/PIPELINE mode.
- Implemented rawCommand() so clients can send arbitrary things to
  the redis server
- Implemented DEBUG OBJECT ([@michael-grunder](https://github.com/michael-grunder), [@isage](https://github.com/isage))
- Added/abide by connect timeout for RedisArray

### Fixed

- Select to the last selected DB when phpredis reconnects
- Fix a possible invalid free in \_serialize()
- Added SAVE and BGSAVE to "distributable" commands for RedisArray
- Fixed invalid "argc" calculation in HLL commands ([@welting](https://github.com/weltling))
- Allow clients to break out of the subscribe loop and return context.
- Fixes a memory leak in SCAN when OPT_SCAN_RETRY id.
- Fix possible segfault when igbinary is enabled ([@remicollet](https://github.com/remicollet)).
- Add a couple of cases where we throw on an error (LOADING/NOAUTH/MASTERDOWN)
- Fix several issues with serialization NARY
- Fix missing TSRMLS_CC and a TSRMLS_DC/TSRMLS_CC typo ([@itcom](https://github.com/itcom))

---

## [2.2.5] - 2014-03-15 ([GitHub](https://github.com/phpredis/phpredis/releases/tag/2.2.5), [PECL](https://pecl.php.net/package/redis/2.2.5))

### Added

- Support for the BITPOS command
- Connection timeout option for RedisArray ([@MiketoString](https://github.com/MiketoString))
- A \_serialize method, to complement our existing \_unserialize method
- Support for the PUBSUB command
- Support for SCAN, SSCAN, HSCAN, and ZSCAN
- Support for the WAIT command

### Fixed

- Handle the COPY and REPLACE arguments for the MIGRATE command
- Fix syntax error in documentation for the SET command ([@mithunsatheesh](https://github.com/mithunsatheesh))
- Fix Homebrew documentation instructions ([@mathias](https://github.com/mathiasverraes))

---

## [2.2.4] - 2013-09-01 ([GitHub](https://github.com/phpredis/phpredis/releases/tag/2.2.4), [PECL](https://pecl.php.net/package/redis/2.2.4))

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

### Fixed

- C99 Compliance (or rather lack thereof) fix @mobli
- Added ZEND_ACC_CTOR and ZEND_ACC_DTOR [@euskadi31](https://github.com/euskadi31)
- Stop throwing and clearing an exception on connect failure @matmoi
- Fix a false positive unit test failure having to do with TTL returns
