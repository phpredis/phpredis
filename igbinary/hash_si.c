/*
 * Author: Oleg Grenrus <oleg.grenrus@dynamoid.com>
 *
 * $Id: hash_si.c,v 1.5 2008/07/01 17:02:18 phadej Exp $
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <assert.h>

#include "hash.h"
#include "hash_function.h"

/* {{{ nextpow2 */
/** Next power of 2.
 * @param n Integer.
 * @return next to n power of 2 .
 */
inline static uint32_t nextpow2(uint32_t n) {
	uint32_t m = 1;
	while (m < n) {
		m = m << 1;
	}

	return m;
}
/* }}} */
/* {{{ hash_si_init */
int hash_si_init(struct hash_si *h, size_t size) {
	size = nextpow2(size);

	h->size = size;
	h->used = 0;
	h->data = (struct hash_si_pair *) malloc(sizeof(struct hash_si_pair) * size);
	if (h->data == NULL) {
		return 1;
	}

	memset(h->data, 0, sizeof(struct hash_si_pair) * size);
	
	return 0;
}
/* }}} */
/* {{{ hash_si_deinit */
void hash_si_deinit(struct hash_si *h) {
	int i;
	
	for (i = 0; i < h->size; i++) {
		if (h->data[i].key != NULL) {
			free(h->data[i].key);
		}
	}

	free(h->data);

	h->size = 0;
	h->used = 0;
}
/* }}} */
/* {{{ _hash_si_find */
/** Returns index of key, or where it should be.
 * @param h Pointer to hash_si struct.
 * @param key Pointer to key.
 * @param key_len Key length.
 * @return index.
 */
inline static size_t _hash_si_find(struct hash_si *h, const char *key, size_t key_len) {
	uint32_t hv;
	size_t size;
	
	assert(h != NULL);
	
	size = h->size;
	hv = hash_function(key, key_len, 0) & (h->size-1);
	
	while (size > 0 &&
		h->data[hv].key != NULL &&
		(h->data[hv].key_len != key_len || memcmp(h->data[hv].key, key, key_len) != 0)) {	
		/* linear prob */
		hv = (hv + 1) & (h->size-1);
		size--;
	}
	
	return hv;
}
/* }}} */
/* {{{ hash_si_remove */
int hash_si_remove(struct hash_si *h, const char *key, size_t key_len, uint32_t *value) {
	uint32_t hv;
	uint32_t j, k;
	
	assert(h != NULL);
	
	hv = _hash_si_find(h, key, key_len);

	/* dont exists */
	if (h->data[hv].key == NULL) {
		return 1;
	}

	h->used--;
		
	free(h->data[hv].key);

	if (value != NULL) 
		*value = h->data[hv].value;

	j = (hv + 1) & (h->size-1);	
	while (h->data[j].key != NULL) {
		k = hash_function(h->data[j].key, strlen(h->data[j].key), 0) & (h->size-1);
		if ((j > hv && (k <= hv || k > j)) || (j < hv && (k <= hv && k > j))) {
			h->data[hv].key = h->data[j].key;
			h->data[hv].key_len = h->data[j].key_len;
			h->data[hv].value = h->data[j].value;
		
			hv = j;
		}
		j = (j + 1) & (h->size-1);	
	}
	h->data[hv].key = NULL;
		

	return 0;
/*
 *     loop
 *            j := (j+1) modulo num_slots
 *	            if slot[j] is unoccupied
 *		             exit loop
 *			          k := hash(slot[j].key) modulo num_slots
 *			          if (j > i and (k <= i or k > j)) or
 *			            (j < i and (k <= i and k > j)) (note 2)
 *			               slot[i] := slot[j]
 *			                i := j
 *			      mark slot[i] as unoccupied				
 *
 * For all records in a cluster, there must be no vacant slots between their natural
 * hash position and their current position (else lookups will terminate before finding
 * the record). At this point in the pseudocode, i is a vacant slot that might be
 * invalidating this property for subsequent records in the cluster. j is such a
 * subsequent record. k is the raw hash where the record at j would naturally land in
 * the hash table if there were no collisions. This test is asking if the record at j
 * is invalidly positioned with respect to the required properties of a cluster now
 * that i is vacant.
 *
 * Another technique for removal is simply to mark the slot as deleted. However
 * this eventually requires rebuilding the table simply to remove deleted records.
 * The methods above provide O(1) updating and removal of existing records, with
 * occasional rebuilding if the high water mark of the table size grows.
 */
}
/* }}} */
/* {{{ hash_si_rehash */
/** Rehash/resize hash_si.
 * @param h Pointer to hash_si struct.
 */
inline static void hash_si_rehash(struct hash_si *h) {
	uint32_t hv;
	int i;
	struct hash_si newh;
		
	assert(h != NULL);
	
	hash_si_init(&newh, h->size * 2);
	
	for (i = 0; i < h->size; i++) {
		if (h->data[i].key != NULL) {
			hv = _hash_si_find(&newh, h->data[i].key, h->data[i].key_len);
			newh.data[hv].key = h->data[i].key;
			newh.data[hv].key_len = h->data[i].key_len;
			newh.data[hv].value = h->data[i].value;
		}
	}	
	
	free(h->data);
	h->data = newh.data;
	h->size *= 2;
}
/* }}} */
/* {{{ hash_si_insert */
int hash_si_insert(struct hash_si *h, const char *key, size_t key_len, uint32_t value) {
	uint32_t hv;

	if (h->size / 4 * 3 < h->used + 1) {
		hash_si_rehash(h);
	}
	
	hv = _hash_si_find(h, key, key_len);
	
	if (h->data[hv].key == NULL) {
		h->data[hv].key = (char *) malloc(key_len + 1);
		if (h->data[hv].key == NULL) {
			return 1;
		}
		memcpy(h->data[hv].key, key, key_len);
		h->data[hv].key[key_len] = '\0';
		h->data[hv].key_len = key_len;

		h->used++;
	} else {
		return 2;
	}
	
	h->data[hv].value = value;
	
	return 0;
}
/* }}} */
/* {{{ hash_si_find */
int hash_si_find(struct hash_si *h, const char *key, size_t key_len, uint32_t *value) {
	uint32_t hv;

	assert(h != NULL);
	
	hv = _hash_si_find(h, key, key_len);
	
	if (h->data[hv].key == NULL) {
		return 1;
	} else {
		*value = h->data[hv].value;
		return 0;
	}
}
/* }}} */
/* {{{ hash_si_traverse */
void hash_si_traverse(struct hash_si *h, int (*traverse_function) (const char *key, size_t key_len, uint32_t value)) {
	int i;

	assert(h != NULL && traverse_function != NULL);
	
	for (i = 0; i < h->size; i++) {
		if (h->data[i].key != NULL && traverse_function(h->data[i].key, h->data[i].key_len, h->data[i].value) != 1) {
			return;
		}
	}
}
/* }}} */
/* {{{ hash_si_size */
size_t hash_si_size(struct hash_si *h) {
	assert(h != NULL);

	return h->used;
}
/* }}} */
/* {{{ hash_si_capacity */
size_t hash_si_capacity(struct hash_si *h) {
	assert(h != NULL);

	return h->size;
}
/* }}} */

/*
 * Local variables:
 * tab-width: 2
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
