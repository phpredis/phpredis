/*
 * Author: Oleg Grenrus <oleg.grenrus@dynamoid.com>
 *
 * $Id: igbinary.h,v 1.5 2008/07/03 16:43:46 phadej Exp $
 */

#ifndef IGBINARY_H
#define IGBINARY_H

#include <stdint.h>

#include "php.h"

#define IGBINARY_VERSION "1.0.2"

/** Serialize zval.
 * Return buffer is allocated by this function with emalloc.
 * @param[out] ret Return buffer
 * @param[out] ret_len Size of return buffer
 * @param[in] z Variable to be serialized
 * @return 0 on success, 1 elsewhere.
 */
int igbinary_serialize(uint8_t **ret, size_t *ret_len, zval *z TSRMLS_DC);

/** Unserialize to zval.
 * @param[in] buf Buffer with serialized data.
 * @param[in] buf_len Buffer length.
 * @param[out] z Unserialized zval
 * @return 0 on success, 1 elsewhere.
 */
int igbinary_unserialize(const uint8_t *buf, size_t buf_len, zval **z TSRMLS_DC);

#endif /* IGBINARY_H */
