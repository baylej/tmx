#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h> /* int32_t */
#include <errno.h>

#include "tmx.h"
#include "tsx.h"
#include "tmx_utils.h"

/*
	Public functions
*/

tmx_tileset_manager* tmx_make_tileset_manager() {
	return (tmx_tileset_manager*)mk_hashtable(5);
}

void tmx_free_tileset_manager(tmx_tileset_manager *h) {
	free_hashtable((void*)h, tileset_deallocator);
}

int tmx_load_tileset(tmx_tileset_manager *ts_mgr, const char *path) {
	tmx_tileset *ts;

	if (ts_mgr == NULL) return 0;

	ts = parse_tsx_xml(path);
	if (ts) {
		hashtable_set((void*)ts_mgr, path, (void*)ts, tileset_deallocator);
		return 1;
	}

	return 0;
}

int tmx_load_tileset_buffer(tmx_tileset_manager *ts_mgr, const char *buffer, int len, const char *key) {
	tmx_tileset *ts;

	if (ts_mgr == NULL) return 0;

	ts = parse_tsx_xml_buffer(buffer, len);
	if (ts) {
		hashtable_set((void*)ts_mgr, key, (void*)ts, tileset_deallocator);
		return 1;
	}

	return 0;
}

int tmx_load_tileset_fd(tmx_tileset_manager *ts_mgr, int fd, const char *key) {
	tmx_tileset *ts;

	if (ts_mgr == NULL) return 0;

	ts = parse_tsx_xml_fd(fd);
	if (ts) {
		hashtable_set((void*)ts_mgr, key, (void*)ts, tileset_deallocator);
		return 1;
	}

	return 0;
}

int tmx_load_tileset_callback(tmx_tileset_manager *ts_mgr, tmx_read_functor callback, void *userdata, const char *key) {
	tmx_tileset *ts;

	if (ts_mgr == NULL) return 0;

	ts = parse_tsx_xml_callback(callback, userdata);
	if (ts) {
		hashtable_set((void*)ts_mgr, key, (void*)ts, tileset_deallocator);
		return 1;
	}

	return 0;
}

tmx_map* tmx_tsmgr_load(tmx_tileset_manager *ts_mgr, const char *path) {
	tmx_map *map = NULL;
	set_alloc_functions();
	map = parse_xml(ts_mgr, path);
	map_post_parsing(&map);
	return map;
}

tmx_map* tmx_tsmgr_load_buffer(tmx_tileset_manager *ts_mgr, const char *buffer, int len) {
	tmx_map *map = NULL;
	set_alloc_functions();
	map = parse_xml_buffer(ts_mgr, buffer, len);
	map_post_parsing(&map);
	return map;
}

tmx_map* tmx_tsmgr_load_fd(tmx_tileset_manager *ts_mgr, int fd) {
	tmx_map *map = NULL;
	set_alloc_functions();
	map = parse_xml_fd(ts_mgr, fd);
	map_post_parsing(&map);
	return map;
}

tmx_map* tmx_tsmgr_load_callback(tmx_tileset_manager *ts_mgr, tmx_read_functor callback, void *userdata) {
	tmx_map *map = NULL;
	set_alloc_functions();
	map = parse_xml_callback(ts_mgr, callback, userdata);
	map_post_parsing(&map);
	return map;
}
