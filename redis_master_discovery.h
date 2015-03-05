#ifndef REDIS_MASTER_DISCOVERY_H
#define REDIS_MASTER_DISCOVERY_H

#ifdef PHP_WIN32
#include "win32/php_stdint.h"
#else
#include <stdint.h>
#endif
#include "common.h"

PHP_METHOD(RedisMasterDiscovery, __construct);
PHP_METHOD(RedisMasterDiscovery, __destruct);
PHP_METHOD(RedisMasterDiscovery, addSentinel);
PHP_METHOD(RedisMasterDiscovery, getSentinels);
PHP_METHOD(RedisMasterDiscovery, getMasterAddr);

#endif
