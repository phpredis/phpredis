#ifndef REDIS_SENTINEL_H
#define REDIS_SENTINEL_H

#ifdef PHP_WIN32
#include "win32/php_stdint.h"
#else
#include <stdint.h>
#endif
#include "common.h"

PHP_METHOD(RedisSentinel, __construct);
PHP_METHOD(RedisSentinel, __destruct);
PHP_METHOD(RedisSentinel, connect);
PHP_METHOD(RedisSentinel, ping);
PHP_METHOD(RedisSentinel, masters);
PHP_METHOD(RedisSentinel, master);
PHP_METHOD(RedisSentinel, slaves);
PHP_METHOD(RedisSentinel, getMasterAddrByName);
PHP_METHOD(RedisSentinel, reset);
PHP_METHOD(RedisSentinel, failover);
PHP_METHOD(RedisSentinel, getMaster);

#endif
