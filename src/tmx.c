#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h> /* int32_t */
#include <errno.h>

#include "tmx.h"
#include "tmx_utils.h"

/*
	Public globals
*/

void* (*tmx_alloc_func) (void *address, size_t len) = NULL;
void  (*tmx_free_func ) (void *address) = NULL;
void* (*tmx_img_load_func) (const char *p) = NULL;
void  (*tmx_img_free_func) (void *address) = NULL;

/*
	Public functions
*/

tmx_map* tmx_load(const char *path) {
	tmx_map *map = NULL;
	set_alloc_functions();
	map = parse_xml(NULL, path);
	map_post_parsing(&map);
	return map;
}

tmx_map* tmx_load_buffer(const char *buffer, int len) {
	tmx_map *map = NULL;
	set_alloc_functions();
	map = parse_xml_buffer(NULL, buffer, len);
	map_post_parsing(&map);
	return map;
}

tmx_map* tmx_load_fd(int fd) {
	tmx_map *map = NULL;
	set_alloc_functions();
	map = parse_xml_fd(NULL, fd);
	map_post_parsing(&map);
	return map;
}

tmx_map* tmx_load_callback(tmx_read_functor callback, void *userdata) {
	tmx_map *map = NULL;
	set_alloc_functions();
	map = parse_xml_callback(NULL, callback, userdata);
	map_post_parsing(&map);
	return map;
}

void tmx_map_free(tmx_map *map) {
	if (map) {
		free_ts_list(map->ts_head);
		free_props(map->properties);
		free_layers(map->ly_head);
		tmx_free_func(map->tiles);
		tmx_free_func(map);
	}
}

tmx_tile* tmx_get_tile(tmx_map *map, unsigned int gid) {
	if (!map) {
		tmx_err(E_INVAL, "tmx_get_tile: invalid argument: map is NULL");
		return NULL;
	}

	gid &= TMX_FLIP_BITS_REMOVAL;

	if (gid < map->tilecount) return map->tiles[gid];

	return NULL;
}

tmx_property* tmx_get_property(tmx_properties *hash, const char *key) {
	if (hash == NULL) {
		return NULL;
	}
	return (tmx_property*) hashtable_get((void*)hash, key);
}

struct property_foreach_data {
	tmx_property_functor callback;
	void *userdata;
};

static void property_foreach(void *val, void *userdata, const char *key UNUSED) {
	struct property_foreach_data *holder = ((struct property_foreach_data*)userdata);
	holder->callback((tmx_property*)val, holder->userdata);
}

void tmx_property_foreach(tmx_properties *hash, tmx_property_functor callback, void *userdata) {
	struct property_foreach_data holder;
	holder.callback = callback;
	holder.userdata = userdata;
	hashtable_foreach((void*)hash, property_foreach, &holder);
}

tmx_resource_manager* tmx_make_resource_manager() {
	return (tmx_resource_manager*)mk_hashtable(5);
}

void tmx_free_resource_manager(tmx_resource_manager *h) {
	free_hashtable((void*)h, resource_deallocator);
}

int tmx_load_tileset(tmx_resource_manager *rc_mgr, const char *path) {
	if (rc_mgr == NULL) return 0;
	return add_tileset(rc_mgr, path, parse_tsx_xml(path));
}

int tmx_load_tileset_buffer(tmx_resource_manager *rc_mgr, const char *buffer, int len, const char *key) {
	if (rc_mgr == NULL) return 0;
	return add_tileset(rc_mgr, key, parse_tsx_xml_buffer(buffer, len));
}

int tmx_load_tileset_fd(tmx_resource_manager *rc_mgr, int fd, const char *key) {
	if (rc_mgr == NULL) return 0;
	return add_tileset(rc_mgr, key, parse_tsx_xml_fd(fd));
}

int tmx_load_tileset_callback(tmx_resource_manager *rc_mgr, tmx_read_functor callback, void *userdata, const char *key) {
	if (rc_mgr == NULL) return 0;
	return add_tileset(rc_mgr, key, parse_tsx_xml_callback(callback, userdata));
}

int tmx_load_template(tmx_resource_manager *rc_mgr, const char *path) {
	if (rc_mgr == NULL) return 0;
	return add_template(rc_mgr, path, parse_tx_xml(rc_mgr, path));
}

int tmx_load_template_buffer(tmx_resource_manager *rc_mgr, const char *buffer, int len, const char *key) {
	if (rc_mgr == NULL) return 0;
	return add_template(rc_mgr, key, parse_tx_xml_buffer(rc_mgr, buffer, len));
}

int tmx_load_template_fd(tmx_resource_manager *rc_mgr, int fd, const char *key) {
	if (rc_mgr == NULL) return 0;
	return add_template(rc_mgr, key, parse_tx_xml_fd(rc_mgr, fd));
}

int tmx_load_template_callback(tmx_resource_manager *rc_mgr, tmx_read_functor callback, void *userdata, const char *key) {
	if (rc_mgr == NULL) return 0;
	return add_template(rc_mgr, key, parse_tx_xml_callback(rc_mgr, callback, userdata));
}

tmx_map* tmx_rcmgr_load(tmx_resource_manager *rc_mgr, const char *path) {
	tmx_map *map = NULL;
	set_alloc_functions();
	map = parse_xml(rc_mgr, path);
	map_post_parsing(&map);
	return map;
}

tmx_map* tmx_rcmgr_load_buffer(tmx_resource_manager *rc_mgr, const char *buffer, int len) {
	tmx_map *map = NULL;
	set_alloc_functions();
	map = parse_xml_buffer(rc_mgr, buffer, len);
	map_post_parsing(&map);
	return map;
}

tmx_map* tmx_rcmgr_load_fd(tmx_resource_manager *rc_mgr, int fd) {
	tmx_map *map = NULL;
	set_alloc_functions();
	map = parse_xml_fd(rc_mgr, fd);
	map_post_parsing(&map);
	return map;
}

tmx_map* tmx_rcmgr_load_callback(tmx_resource_manager *rc_mgr, tmx_read_functor callback, void *userdata) {
	tmx_map *map = NULL;
	set_alloc_functions();
	map = parse_xml_callback(rc_mgr, callback, userdata);
	map_post_parsing(&map);
	return map;
}
