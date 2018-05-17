#ifndef REDIS_SESSION_H
#define REDIS_SESSION_H
#ifdef PHP_SESSION
#include "ext/session/php_session.h"

PS_OPEN_FUNC(redis);
PS_CLOSE_FUNC(redis);
PS_READ_FUNC(redis);
PS_WRITE_FUNC(redis);
PS_DESTROY_FUNC(redis);
PS_GC_FUNC(redis);
PS_CREATE_SID_FUNC(redis);

#if (PHP_MAJOR_VERSION >= 7)
PS_VALIDATE_SID_FUNC(redis);
PS_UPDATE_TIMESTAMP_FUNC(redis);
#endif

PS_OPEN_FUNC(rediscluster);
PS_CLOSE_FUNC(rediscluster);
PS_READ_FUNC(rediscluster);
PS_WRITE_FUNC(rediscluster);
PS_DESTROY_FUNC(rediscluster);
PS_GC_FUNC(rediscluster);

#endif
#endif
