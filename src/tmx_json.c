/*
	JSON Parser using the yajl API
	see http://lloyd.github.com/yajl/
	It uses top-appended linked lists, so if you have several layers
	in your map, your first layer in the json will be the last in the
	linked list.
*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#include <yajl/yajl_common.h>
#include <yajl/yajl_parse.h>

#include "tmx.h"
#include "tmx_utils.h"

/* 4kB buffer */
#define BUFFER_LEN 4096

/*
	Memory functions
*/
void * yajl_malloc(void *ctx, size_t size) {
	return tmx_alloc_func(NULL, size);
}

void * yajl_realloc(void *ctx, void *address, size_t size) {
	return tmx_alloc_func(address, size);
}

void yajl_freef(void *ctx, void *address) {
	tmx_free_func(address);
}

/*
	Callback functions
*/

int cb_null(void * ctx) {
	return 1;
}

int cb_boolean(void * ctx, int boolVal) {
	return 1;
}

int cb_integer(void * ctx, long long integerVal) {
	return 1;
}

int cb_double(void * ctx, double doubleVal) {
	return 1;
}

int cb_number(void * ctx, const char * numberVal, size_t numberLen) {
	return 1;
}

int cb_string(void * ctx, const unsigned char * stringVal, size_t stringLen) {
	return 1;
}

int cb_start_map(void * ctx) {
	return 1;
}

int cb_map_key(void * ctx, const unsigned char * key, size_t stringLen) {
	return 1;
}

int cb_end_map(void * ctx) {
	return 1;
}

int cb_start_array(void * ctx) {
	return 1;
}

int cb_end_array(void * ctx) {
	return 1;
}

/*
	Public function
*/

tmx_map parse_json(const char *filename) {
	tmx_map res = NULL;

	yajl_alloc_funcs mem_f;
	yajl_callbacks cb_f;
	yajl_handle handle = NULL;
	yajl_status ret;
	char *yajl_errstr;

	FILE *file = NULL;
	unsigned char buffer[BUFFER_LEN];
	size_t red;

	if (!(res = alloc_map())) return NULL;

	/* set memory alloc/free functions pointers */
	mem_f.ctx = NULL;
	mem_f.malloc = yajl_malloc;
	mem_f.realloc = yajl_realloc;
	mem_f.free = yajl_freef;

	/* set callback functions */
	cb_f.yajl_boolean = cb_boolean;
	cb_f.yajl_double = cb_double;
	cb_f.yajl_end_array = cb_end_array;
	cb_f.yajl_end_map = cb_end_map;
	cb_f.yajl_integer = cb_integer;
	cb_f.yajl_map_key = cb_map_key;
	cb_f.yajl_null = cb_null;
	cb_f.yajl_number = cb_number;
	cb_f.yajl_start_array = cb_start_array;
	cb_f.yajl_start_map = cb_start_map;
	cb_f.yajl_string = cb_string;

	if (!(handle = yajl_alloc(&cb_f, &mem_f, NULL))) goto cleanup;
	yajl_config(handle, yajl_dont_validate_strings, 1); /* disable utf8 checking */

	/* Open and parse the file */
	if (!(file = fopen(filename, "r"))) {
		if (errno == EACCES) {
			tmx_errno = E_ACCESS;
		} else if (errno == ENOENT) {
			tmx_errno = E_NOENT;
		} else {
			tmx_err(E_UNKN, strerror(errno));
		}
		goto cleanup;
	}

	while (red = fread(buffer, 1, BUFFER_LEN, file), red > 0) {
		
		ret = yajl_parse(handle, buffer, red);
		if (ret == yajl_status_client_canceled) goto cleanup;
		if (ret == yajl_status_error) {
			yajl_errstr = (char*)yajl_get_error(handle, 0, NULL, 0);
			tmx_err(E_JDATA, yajl_errstr);
			yajl_free_error(handle, (unsigned char*)yajl_errstr);
			goto cleanup;
		}
	}

	ret = yajl_complete_parse(handle);
	if (ret == yajl_status_client_canceled) goto cleanup;
	if (ret == yajl_status_error) {
		yajl_errstr = (char*)yajl_get_error(handle, 0, NULL, 0);
		tmx_err(E_JDATA, yajl_errstr);
		yajl_free_error(handle, (unsigned char*)yajl_errstr);
		goto cleanup;
	}

	/* call cleanup function */
	yajl_free(handle);
	fclose(file);

	return res;
cleanup:
	yajl_free(handle);
	tmx_free_func(res);
	fclose(file);
	return NULL;
}
