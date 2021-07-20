#include "common.h"

#include <ext/standard/php_rand.h>

#if PHP_VERSION_ID >= 70100
#include <ext/standard/php_mt_rand.h>
#else
static zend_long php_mt_rand_range(zend_long min, zend_long max) {
    zend_long number = php_rand();
    RAND_RANGE(number, min, max, PHP_RAND_MAX);
    return number;
}
#endif

#include "backoff.h"

static zend_ulong random_range(zend_ulong min, zend_ulong max) {
    if (max < min) {
        return php_mt_rand_range(max, min);
    }

    return php_mt_rand_range(min, max);
}

static zend_ulong redis_default_backoff(struct RedisBackoff *self, unsigned int retry_index) {
    zend_ulong backoff = retry_index ? self->base : random_range(0, self->base);
    return MIN(self->cap, backoff);
}

static zend_ulong redis_constant_backoff(struct RedisBackoff *self, unsigned int retry_index) {
    zend_ulong backoff = self->base;
    return MIN(self->cap, backoff);
}

static zend_ulong redis_uniform_backoff(struct RedisBackoff *self, unsigned int retry_index) {
    zend_ulong backoff = random_range(0, self->base);
    return MIN(self->cap, backoff);
}

static zend_ulong redis_exponential_backoff(struct RedisBackoff *self, unsigned int retry_index) {
    zend_ulong pow = MIN(retry_index, 10);
    zend_ulong backoff = self->base * (1 << pow);
    return MIN(self->cap, backoff);
}

static zend_ulong redis_full_jitter_backoff(struct RedisBackoff *self, unsigned int retry_index) {
    zend_ulong pow = MIN(retry_index, 10);
    zend_ulong backoff = self->base * (1 << pow);
    zend_ulong cap = MIN(self->cap, backoff);
    return random_range(0, cap);
}

static zend_ulong redis_equal_jitter_backoff(struct RedisBackoff *self, unsigned int retry_index) {
    zend_ulong pow = MIN(retry_index, 10);
    zend_ulong backoff = self->base * (1 << pow);
    zend_ulong temp = MIN(self->cap, backoff);
    return temp / 2 + random_range(0, temp) / 2;
}

static zend_ulong redis_decorrelated_jitter_backoff(struct RedisBackoff *self, unsigned int retry_index) {
    self->previous_backoff = random_range(self->base, self->previous_backoff * 3);
    return MIN(self->cap, self->previous_backoff);
}

typedef zend_ulong (*redis_backoff_algorithm)(struct RedisBackoff *self, unsigned int retry_index);

static redis_backoff_algorithm redis_backoff_algorithms[REDIS_BACKOFF_ALGORITHMS] = {
    redis_default_backoff,
    redis_decorrelated_jitter_backoff,
    redis_full_jitter_backoff,
    redis_equal_jitter_backoff,
    redis_exponential_backoff,
    redis_uniform_backoff,
    redis_constant_backoff,
};

void redis_initialize_backoff(struct RedisBackoff *self, unsigned long retry_interval) {
    self->algorithm = 0; // default backoff
    self->base = retry_interval;
    self->cap = retry_interval;
    self->previous_backoff = 0;
}

void redis_backoff_reset(struct RedisBackoff *self) {
    self->previous_backoff = 0;
}

zend_ulong redis_backoff_compute(struct RedisBackoff *self, unsigned int retry_index) {
    return redis_backoff_algorithms[self->algorithm](self, retry_index);
}
