#ifndef REDIS_SIMDJSON_H
#define REDIS_SIMDJSON_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct redis_zval redis_zval;

enum redisJsonFlags : uint32_t {
    RJ_OBJECT           = 1u << 0,
    RJ_BIGINT_AS_STRING = 1u << 1,
};

int redis_json_to_zval_ex(redis_zval *dst_, const char *json, size_t len,
                          uint32_t flags, uint32_t max_depth);

#ifdef __cplusplus
}
#endif

#endif /* REDIS_SIMDJSON_H */
