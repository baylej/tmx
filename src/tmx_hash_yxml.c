/*
	Hashtable

	Self-contained chaining hashtable. 
*/

#include <stdlib.h>
#include <string.h>

#include "tmx_utils.h"

typedef struct ht_entry {
	char *key;
	void *value;
	struct ht_entry *next;
} ht_entry;

typedef struct {
	ht_entry **buckets;
	unsigned int size;
} ht_table;

static unsigned int ht_hash(const char *key, unsigned int size) {
	/* djb2 */
	unsigned long h = 5381;
	while (*key) {
		h = ((h << 5) + h) + (unsigned char)*key;
		key++;
	}
	return (unsigned int)(h % size);
}

void* mk_hashtable(unsigned int initial_size) {
	ht_table *ht;
	if (initial_size == 0) initial_size = 8;
	ht = (ht_table*)tmx_alloc_func(NULL, sizeof(ht_table));
	if (!ht) return NULL;
	ht->size = initial_size;
	ht->buckets = (ht_entry**)tmx_alloc_func(NULL, initial_size * sizeof(ht_entry*));
	if (!ht->buckets) {
		tmx_free_func(ht);
		return NULL;
	}
	memset(ht->buckets, 0, initial_size * sizeof(ht_entry*));
	return (void*)ht;
}

void hashtable_set(void *hashtable, const char *key, void *val, hashtable_entry_deallocator deallocator) {
	ht_table *ht = (ht_table*)hashtable;
	unsigned int idx = ht_hash(key, ht->size);
	ht_entry *e = ht->buckets[idx];

	/* Update existing entry if key matches */
	while (e) {
		if (!strcmp(e->key, key)) {
			if (deallocator && e->value) {
				deallocator(e->value, e->key);
			}
			e->value = val;
			return;
		}
		e = e->next;
	}

	/* Insert new entry */
	e = (ht_entry*)tmx_alloc_func(NULL, sizeof(ht_entry));
	if (!e) return;
	e->key = tmx_strdup(key);
	e->value = val;
	e->next = ht->buckets[idx];
	ht->buckets[idx] = e;
}

void* hashtable_get(void *hashtable, const char *key) {
	ht_table *ht = (ht_table*)hashtable;
	unsigned int idx = ht_hash(key, ht->size);
	ht_entry *e = ht->buckets[idx];

	while (e) {
		if (!strcmp(e->key, key)) {
			return e->value;
		}
		e = e->next;
	}
	return NULL;
}

void hashtable_rm(void *hashtable, const char *key, hashtable_entry_deallocator deallocator) {
	ht_table *ht = (ht_table*)hashtable;
	unsigned int idx = ht_hash(key, ht->size);
	ht_entry *e = ht->buckets[idx];
	ht_entry *prev = NULL;

	while (e) {
		if (!strcmp(e->key, key)) {
			if (prev) {
				prev->next = e->next;
			} else {
				ht->buckets[idx] = e->next;
			}
			if (deallocator) {
				deallocator(e->value, e->key);
			}
			tmx_free_func(e->key);
			tmx_free_func(e);
			return;
		}
		prev = e;
		e = e->next;
	}
}

void free_hashtable(void *hashtable, hashtable_entry_deallocator deallocator) {
	ht_table *ht;
	ht_entry *e, *next;
	unsigned int i;

	if (!hashtable) return;
	ht = (ht_table*)hashtable;

	for (i = 0; i < ht->size; i++) {
		e = ht->buckets[i];
		while (e) {
			next = e->next;
			if (deallocator) {
				deallocator(e->value, e->key);
			}
			tmx_free_func(e->key);
			tmx_free_func(e);
			e = next;
		}
	}
	tmx_free_func(ht->buckets);
	tmx_free_func(ht);
}

void hashtable_foreach(void *hashtable, hashtable_foreach_functor functor, void *userdata) {
	ht_table *ht = (ht_table*)hashtable;
	ht_entry *e;
	unsigned int i;

	for (i = 0; i < ht->size; i++) {
		e = ht->buckets[i];
		while (e) {
			functor(e->value, userdata, e->key);
			e = e->next;
		}
	}
}
