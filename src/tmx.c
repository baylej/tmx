#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h> /* int32_t */
#include <errno.h>

#include "tmx.h"
#include "tsx.h"
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
