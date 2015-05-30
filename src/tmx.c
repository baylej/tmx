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
	const char *extension;
	FILE *file;
	int fchar;
	tmx_sorted_array tiles_array;

	if (!tmx_alloc_func) tmx_alloc_func = realloc;
	if (!tmx_free_func) tmx_free_func = free;

	if (tmx_sa_initialize(&tiles_array, sizeof(tmx_tile), 16, &tmx_compare_tiles) < 0) {
		tmx_errno = E_ALLOC;
		return NULL;
	}

	/* is 'path' a JSON or a XML file ? */
	extension = strrchr(path, '.'); /* First using the file extension */
	if (extension && (!strcmp(extension, ".tmx") || !strcmp(extension, ".xml"))) {
		map = parse_xml(&tiles_array, path);
	} else if (extension && !strcmp(extension, ".json")) {
		map = parse_json(&tiles_array, path);
	} else {
		/* open the file and check with the first character */
		if ((file = fopen(path, "r"))) {
			fchar = fgetc(file);
			fclose(file);
			if (fchar == '<') {
				map = parse_xml(&tiles_array, path);
			} else if (fchar == '{') {
				map = parse_json(&tiles_array, path);
			} else {
				tmx_errno = E_FORMAT;
			}
		} else {
			if (errno == EACCES) {
				tmx_errno = E_ACCESS;
			} else if (errno == ENOENT) {
				tmx_errno = E_NOENT;
			} else {
				tmx_err(E_UNKN, "%s", strerror(errno));
			}
		}
	}

	if (map == NULL)
		tmx_sa_free(&tiles_array);
	else {
		/* Optimize tiles array and add tiles to their respective tilesets */
		tmx_sa_resize(&tiles_array, tiles_array.num_elements);
		map->tiles = (tmx_tile*)(tiles_array.data);
		map->num_tiles = tiles_array.num_elements;

		tmx_tileset *ts = map->ts_head;

		while (ts) {
			if (ts->tiles != NULL) {
				tmx_tile tile_to_find;
				memset(&tile_to_find, 0, sizeof(tile_to_find));
				tile_to_find.gid = (unsigned int)(ts->tiles); /* We previously stored gid of tile in ts->tiles */

				ts->tiles = (tmx_tile*)(tmx_sa_find(&tiles_array, &tile_to_find));
			}

			ts = ts->next;
		}
	}

	return map;
}

static void free_props(tmx_property *p) {
	if (p) {
		free_props(p->next);
		tmx_free_func(p->name);
		tmx_free_func(p->value);
		tmx_free_func(p);
	}
}

static void free_obj(tmx_object *o) {
	if (o) {
		free_obj(o->next);
		tmx_free_func(o->name);
		if (o->points) tmx_free_func(*(o->points));
		tmx_free_func(o->points);
		tmx_free_func(o);
	}
}

static void free_image(tmx_image *i) {
	if (i) {
		tmx_free_func(i->source);
		if (tmx_img_free_func) {
			tmx_img_free_func(i->resource_image);
		}
		tmx_free_func(i);
	}
}

static void free_layers(tmx_layer *l) {
	if (l) {
		free_layers(l->next);
		tmx_free_func(l->name);
		if (l->type == L_LAYER)
			tmx_free_func(l->content.gids);
		else if (l->type == L_OBJGR)
			free_obj(l->content.head);
		else if (l->type == L_IMAGE) {
			free_image(l->content.image);
		}
		free_props(l->properties);
		tmx_free_func(l);
	}
}

static void free_animation(tmx_frame *anim) {
	if (anim) {
		free_animation(anim->next_frame);
		tmx_free_func(anim);
	}
}

static void free_ts(tmx_tileset *ts) {
	if (ts) {
		free_ts(ts->next);
		tmx_free_func(ts->name);
		free_image(ts->image);
		free_props(ts->properties);
		ts->tiles = NULL;
		ts->num_tiles = 0;
		tmx_free_func(ts);
	}
}

void tmx_map_free(tmx_map *map) {
	if (map) {
		if (map->tiles) {
			for (unsigned int i = 0; i < map->num_tiles; i++) {
				free_animation(((tmx_tile*)(map->tiles))[i].animation);
				((tmx_tile*)(map->tiles))[i].animation = NULL;
			}
			tmx_free_func(map->tiles);
		}
		free_ts(map->ts_head);
		free_props(map->properties);
		free_layers(map->ly_head);
		tmx_free_func(map);
	}
}

tmx_tileset* tmx_get_tileset(tmx_map *map, unsigned int gid, unsigned int *x, unsigned int *y) {
	unsigned int tiles_x_count;
	unsigned int ts_w, id, tx, ty;
	tmx_tileset *ts;

	if (!map) {
		tmx_err(E_INVAL, "tmx_get_tile: invalid argument: map is NULL");
		return NULL;
	}

	if (!x || !y) {
		tmx_err(E_INVAL, "tmx_get_tile: invalid argument: x or y is NULL");
		return NULL;
	}

	gid &= TMX_FLIP_BITS_REMOVAL;
	ts = map->ts_head;

	while (ts) {
		if (ts->firstgid <= gid) {
			if (!ts->next || ts->next->firstgid < ts->firstgid || ts->next->firstgid > gid) {
				id = gid - ts->firstgid; /* local id (for this image) */

				ts_w = ts->image->width  - 2 * (ts->margin) + ts->spacing;

				tiles_x_count = ts_w / (ts->tile_width  + ts->spacing);

				tx = id % tiles_x_count;
				ty = id / tiles_x_count;

				*x = ts->margin + (tx * ts->tile_width)  + (tx * ts->spacing); /* set bitmap's region */
				*y = ts->margin + (ty * ts->tile_height) + (ty * ts->spacing); /* x and y coordinates */
				return ts;
			}
		}
		ts = ts->next;
	}

	return NULL;
}

tmx_tile* tmx_get_tile(tmx_map *map, unsigned int gid) {
	tmx_sorted_array sorted_array;
	tmx_tile tile_to_find;
	tmx_tile *found_tile = NULL;

	if (gid == 0) return NULL;

	if (!map) {
		tmx_err(E_INVAL, "tmx_get_tile: invalid argument: map is NULL");
		return NULL;
	}

	gid &= TMX_FLIP_BITS_REMOVAL;

	sorted_array.data = map->tiles;
	sorted_array.element_size = sizeof(tmx_tile);
	sorted_array.num_elements = map->num_tiles;
	sorted_array.max_num_elements = map->num_tiles;
	sorted_array.compare_func = &tmx_compare_tiles;

	memset(&tile_to_find, 0, sizeof(tile_to_find));
	tile_to_find.gid = gid;

	found_tile = tmx_sa_find(&sorted_array, &tile_to_find);

	return found_tile;
}
