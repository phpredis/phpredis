/*
 * Author: Oleg Grenrus <oleg.grenrus@dynamoid.com>
 *
 * $Id: hash_function.h,v 1.4 2008/07/01 17:02:18 phadej Exp $
 */

#ifndef HASH_FUNCTION_H
#define HASH_FUNCTION_H

#include <stdint.h>     /* defines uint32_t etc */

/**
 * Hash function
 *
 * At this moment lookup3 by Bob Jerkins
 *
 * @param key key
 * @param length key length
 * @param initval hash init val
 * @return hash value of key
 * @see http://burtleburtle.net/bob/hash/index.html
 * @author Bob Jerkins <bob_jenkins@burtleburtle.net>
 */
uint32_t hash_function(const void *key, size_t length, uint32_t initval);

#endif /* HASH_FUNCTION_H */
