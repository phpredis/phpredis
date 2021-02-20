#ifndef REDIS_SENTINEL_H
#define REDIS_SENTINEL_H

#include "sentinel_library.h"

#define PHP_REDIS_SENTINEL_VERSION "0.1"

PHP_METHOD(RedisSentinel, __construct);
PHP_METHOD(RedisSentinel, ckquorum);
PHP_METHOD(RedisSentinel, failover);
PHP_METHOD(RedisSentinel, flushconfig);
PHP_METHOD(RedisSentinel, getMasterAddrByName);
PHP_METHOD(RedisSentinel, master);
PHP_METHOD(RedisSentinel, masters);
PHP_METHOD(RedisSentinel, myid);
PHP_METHOD(RedisSentinel, ping);
PHP_METHOD(RedisSentinel, reset);
PHP_METHOD(RedisSentinel, sentinels);
PHP_METHOD(RedisSentinel, slaves);

#endif /* REDIS_SENTINEL_H */
