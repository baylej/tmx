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

	if (!tmx_alloc_func) tmx_alloc_func = realloc;
	if (!tmx_free_func) tmx_free_func = free;

	/* is 'path' a JSON or a XML file ? */
	extension = strrchr(path, '.'); /* First using the file extension */
	if (extension && (!strcmp(extension, ".tmx") || !strcmp(extension, ".xml"))) {
		map = parse_xml(path);
	} else if (extension && !strcmp(extension, ".json")) {
		map = parse_json(path);
	} else {
		/* open the file and check with the first character */
		if ((file = fopen(path, "r"))) {
			fchar = fgetc(file);
			fclose(file);
			if (fchar == '<') {
				map = parse_xml(path);
			} else if (fchar == '{') {
				map = parse_json(path);
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

static void free_tiles(tmx_tile *t) {
	if (t) {
		free_tiles(t->next);
		free_props(t->properties);
		free_image(t->image);
		tmx_free_func(t);
	}
}

static void free_ts(tmx_tileset *ts) {
	if (ts) {
		free_ts(ts->next);
		tmx_free_func(ts->name);
		free_image(ts->image);
		free_props(ts->properties);
		free_tiles(ts->tiles);
		tmx_free_func(ts);
	}
}

void tmx_map_free(tmx_map *map) {
	if (map) {
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
	int id;
	tmx_tileset *ts;
	tmx_tile* t;

	if (!map) {
		tmx_err(E_INVAL, "tmx_get_tile: invalid argument: map is NULL");
		return NULL;
	}

	gid &= TMX_FLIP_BITS_REMOVAL;
	ts = map->ts_head;

	while (ts) {
		if (ts->firstgid <= gid) {
			if (!ts->next || ts->next->firstgid < ts->firstgid || ts->next->firstgid > gid) {
				id = gid - ts->firstgid; /* local id (for this tile) */

				t = ts->tiles;

				while (t) {
					if (t->id == id) {
						return t;
					}
					t = t->next;
				}
			}
		}
		ts = ts->next;
	}

	return NULL;
}
