/*
	Dynamic array implementation with binary search
*/

#include "tmx_array.h"
#include "tmx.h"
#include "tmx_utils.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>


int tmx_sa_initialize(tmx_sorted_array *array_struct, unsigned int element_size, unsigned int start_capacity, tmx_compare_func compare_func) {
	if (!tmx_alloc_func) tmx_alloc_func = realloc;
	if (!tmx_free_func) tmx_free_func = free;

	if (array_struct == NULL) {
		tmx_err(E_INVAL, "tmx_sorted_array: parameter 'array_struct' must not be NULL");
		return -1;
	}

	if (element_size == 0) {
		tmx_err(E_INVAL, "tmx_sorted_array: parameter 'element_size' must be > 0");
		return -1;
	}

	if (compare_func == NULL) {
		tmx_err(E_INVAL, "tmx_sorted_array: parameter 'compare_func' must not be NULL");
		return -1;
	}

	unsigned int capacity = start_capacity;
	if (capacity == 0)
		capacity = 1;	// Start capacity should be at least 1

	array_struct->data = tmx_alloc_func(NULL, capacity * element_size);

	if (array_struct->data == NULL) {
		tmx_err(E_ALLOC, "tmx_sorted_array: couldn't allocate memory for array");
		return -1;
	}

	array_struct->element_size = element_size;
	array_struct->num_elements = 0;
	array_struct->max_num_elements = capacity;
	array_struct->compare_func = compare_func;

	return 0;
}

void tmx_sa_free(tmx_sorted_array *array_struct) {
	if (array_struct != 0) {
		if (array_struct->data != NULL)
			tmx_free_func(array_struct->data);

		array_struct->data = NULL;
		array_struct->element_size = 0;
		array_struct->num_elements = 0;
		array_struct->max_num_elements = 0;
		array_struct->compare_func = NULL;
	}
}

void *tmx_sa_get(tmx_sorted_array *array_struct, unsigned int index) {
	// NOTE: Should we put error checking here at the cost of slightly slower access?
	// NOTE: This function should probably be inlined for (potentially) faster access
	return (char *)(array_struct->data) + (index * array_struct->element_size);
}

void *tmx_sa_insert(tmx_sorted_array *array_struct, void *element) {
	if (array_struct == NULL) {
		tmx_err(E_INVAL, "tmx_sorted_array: parameter 'array_struct' must not be NULL");
		return NULL;
	}

	if (array_struct->data == NULL) {
		tmx_err(E_INVAL, "tmx_sorted_array: parameter 'array_struct->data' must not be NULL");
		return NULL;
	}

	if (array_struct->compare_func == NULL) {
		tmx_err(E_INVAL, "tmx_sorted_array: parameter 'compare_func' must not be NULL");
		return NULL;
	}

	// Do binary search to find position for insertion
	unsigned int insert_pos = 0;

	if (array_struct->num_elements == 0)
		insert_pos = 0;
	else {
		unsigned int range_start = 0;
		unsigned int range_end = array_struct->num_elements - 1;
		int found = 0;
		unsigned int current_pos = (range_end - range_start) / 2;

		while (range_end - range_start > 0 && found == 0 && current_pos < array_struct->num_elements) {
			int result = array_struct->compare_func(element, tmx_sa_get(array_struct, current_pos));

			if (result < 0) {
				range_end = current_pos;
				current_pos = range_start + ((range_end - range_start) / 2);
			}
			else if (result > 0) {
				range_start = current_pos + 1;
				current_pos = range_start + ((range_end - range_start) / 2);
			}
			else {
				found = 1;
			}
		}

		int pos_result = array_struct->compare_func(element, tmx_sa_get(array_struct, current_pos));

		if (pos_result < 0)
			insert_pos = current_pos;
		else
			insert_pos = current_pos + 1;
	}

	// Position found - check if array has enough capacity and resize if necessary
	if (array_struct->num_elements >= array_struct->max_num_elements) {
		tmx_sa_resize(array_struct, array_struct->max_num_elements * 2);

		if (array_struct->num_elements >= array_struct->max_num_elements) {
			tmx_err(E_ALLOC, "tmx_sorted_array: couldn't insert element into array because resizing array failed");
			return NULL;
		}
	}

	// Move elements down by one location to make room for new element
	for (int i = array_struct->num_elements - 1; i >= (int)insert_pos; i--) {
		memcpy((char*)(array_struct->data) + (array_struct->element_size * (i + 1)), (char*)(array_struct->data) + (array_struct->element_size * i), array_struct->element_size);
	}

	// Insert
	memcpy((char*)(array_struct->data) + (array_struct->element_size * insert_pos), element, array_struct->element_size);

	array_struct->num_elements++;

	return (void*)((char*)(array_struct->data) + (array_struct->element_size * insert_pos));
}

void *tmx_sa_find(tmx_sorted_array *array_struct, void *element) {
	if (array_struct == NULL) {
		tmx_err(E_INVAL, "tmx_sorted_array: parameter 'array_struct' must not be NULL");
		return NULL;
	}

	if (array_struct->data == NULL) {
		tmx_err(E_INVAL, "tmx_sorted_array: parameter 'array_struct->data' must not be NULL");
		return NULL;
	}

	if (array_struct->compare_func == NULL) {
		tmx_err(E_INVAL, "tmx_sorted_array: parameter 'compare_func' must not be NULL");
		return NULL;
	}

	if (array_struct->num_elements == 0)
		return NULL;
	
	// Do binary search to find element quickly
	if (array_struct->num_elements == 0)
		return NULL;
	else {
		unsigned int range_start = 0;
		unsigned int range_end = array_struct->num_elements - 1;
		unsigned int current_pos = (range_end - range_start) / 2;
		void *current_element = NULL;

		while (range_end - range_start > 0 && current_pos < array_struct->num_elements) {
			current_element = tmx_sa_get(array_struct, current_pos);
			int result = array_struct->compare_func(element, current_element);

			if (result < 0) {
				range_end = current_pos;
				current_pos = range_start + ((range_end - range_start) / 2);
			}
			else if (result > 0) {
				range_start = current_pos + 1;
				current_pos = range_start + ((range_end - range_start) / 2);
			}
			else {
				return current_element;
			}
		}

		current_element = tmx_sa_get(array_struct, current_pos);
		int pos_result = array_struct->compare_func(element, current_element);

		if (pos_result == 0)
			return current_element;
	}

	return NULL;
}

void *tmx_sa_find_or_insert(tmx_sorted_array *array_struct, void *element) {
	if (array_struct == NULL) {
		tmx_err(E_INVAL, "tmx_sorted_array: parameter 'array_struct' must not be NULL");
		return NULL;
	}

	if (array_struct->data == NULL) {
		tmx_err(E_INVAL, "tmx_sorted_array: parameter 'array_struct->data' must not be NULL");
		return NULL;
	}

	if (array_struct->compare_func == NULL) {
		tmx_err(E_INVAL, "tmx_sorted_array: parameter 'compare_func' must not be NULL");
		return NULL;
	}

	// Do binary search to find element quickly
	unsigned int insert_pos = 0;

	if (array_struct->num_elements == 0)
		insert_pos = 0;
	else {
		unsigned int range_start = 0;
		unsigned int range_end = array_struct->num_elements - 1;
		unsigned int current_pos = (range_end - range_start) / 2;
		void *current_element = NULL;

		while (range_end - range_start > 0 && current_pos < array_struct->num_elements) {
			current_element = tmx_sa_get(array_struct, current_pos);
			int result = array_struct->compare_func(element, current_element);

			if (result < 0) {
				range_end = current_pos;
				current_pos = range_start + ((range_end - range_start) / 2);
			}
			else if (result > 0) {
				range_start = current_pos + 1;
				current_pos = range_start + ((range_end - range_start) / 2);
			}
			else {
				return current_element;
			}
		}

		current_element = tmx_sa_get(array_struct, current_pos);
		int pos_result = array_struct->compare_func(element, current_element);

		if (pos_result < 0)
			insert_pos = current_pos;
		else if (pos_result > 0)
			insert_pos = current_pos + 1;
		else
			return current_element;
	}

	// Element not found, need to insert new element - check if array has enough capacity and resize if necessary
	if (array_struct->num_elements >= array_struct->max_num_elements) {
		tmx_sa_resize(array_struct, array_struct->max_num_elements * 2);

		if (array_struct->num_elements >= array_struct->max_num_elements) {
			tmx_err(E_ALLOC, "tmx_sorted_array: couldn't insert element into array because resizing array failed");
			return NULL;
		}
	}

	// Move elements down by one location to make room for new element
	for (int i = array_struct->num_elements - 1; i >= (int)insert_pos; i--) {
		memcpy((char*)(array_struct->data) + (array_struct->element_size * (i + 1)), (char*)(array_struct->data) + (array_struct->element_size * i), array_struct->element_size);
	}

	// Insert
	memcpy((char*)(array_struct->data) + (array_struct->element_size * insert_pos), element, array_struct->element_size);

	array_struct->num_elements++;

	return (void*)((char*)(array_struct->data) + (array_struct->element_size * insert_pos));
}

void tmx_sa_resize(tmx_sorted_array *array_struct, unsigned int new_size) {
	if (array_struct == NULL) {
		tmx_err(E_INVAL, "tmx_sorted_array: parameter 'array_struct' must not be NULL");
		return;
	}

	if (array_struct->data == NULL) {
		tmx_err(E_INVAL, "tmx_sorted_array: parameter 'array_struct->data' must not be NULL");
		return;
	}

	if (new_size != array_struct->max_num_elements) {
		if (new_size < array_struct->num_elements || new_size == 0) {
			tmx_err(E_INVAL, "tmx_sorted_array: parameter 'new_size' must be >= 'array_struct->num_elements' and > 0");
			return;
		}

		void *temp_array = tmx_alloc_func(array_struct->data, new_size * array_struct->element_size);
		if (temp_array != NULL)
			array_struct->data = temp_array;
		else {
			tmx_err(E_ALLOC, "tmx_sorted_array: couldn't reallocate array memory for resizing");
			return;
		}

		array_struct->max_num_elements = new_size;
	}
}
