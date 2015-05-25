/* Dynamic array implementation with binary search */

#pragma once

#ifndef TMXARRAY_H
#define TMXARRAY_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Comparison function for tmx_sorted_arry 
   Return -1 if first_elem < second_elem
   Return 1 if first_elem > second_elem
   Return 0 if first_elem == second_elem */
typedef int(*tmx_compare_func)(void *first_elem, void *second_elem);

typedef struct _tmx_sorted_array {
	void *data;
	unsigned int element_size;
	unsigned int num_elements;
	unsigned int max_num_elements;
	tmx_compare_func compare_func;
} tmx_sorted_array;

/* Initialize a tmx_sorted_array by specifying element size and
   start capacity (in elements), returns -1 on failure */
int tmx_sa_initialize(tmx_sorted_array *array_struct, unsigned int element_size, unsigned int start_capacity, tmx_compare_func compare_func);

/* Free all resources allocated by a tmx_sorted_array */
void tmx_sa_free(tmx_sorted_array *array_struct);

/* Get element at index 'index'
   (Does no boundary checks for performance) */
void *tmx_sa_get(tmx_sorted_array *array_struct, unsigned int index);

/* Uses binary insertion to sort a new element into array
   Uses array_struct.compare_func to determine order */
void tmx_sa_insert(tmx_sorted_array *array_struct, void *element);

/* Uses binary search to return element on 
   which array_struct.compare_func returns 0
   Returns NULL if not found */
void *tmx_sa_find(tmx_sorted_array *array_struct, void *element);

/* Resizes an array, new_size must be >= array_struct.num_elements */
void tmx_sa_resize(tmx_sorted_array *array_struct, unsigned int new_size);

#ifdef __cplusplus
}
#endif

#endif /* TMXARRAY_H */
