#ifndef REDIS_BACKOFF_H
#define REDIS_BACKOFF_H

/* {{{ struct RedisBackoff */
struct RedisBackoff {
    unsigned int algorithm;        /* index of algorithm function, returns backoff in microseconds*/
    zend_ulong   base;             /* base backoff in microseconds */
    zend_ulong   cap;              /* max backoff in microseconds */
    zend_ulong   previous_backoff; /* previous backoff in microseconds */
};
/* }}} */

void redis_initialize_backoff(struct RedisBackoff *self, unsigned long retry_interval);
void redis_backoff_reset(struct RedisBackoff *self);
zend_ulong redis_backoff_compute(struct RedisBackoff *self, unsigned int retry_index);

#endif
