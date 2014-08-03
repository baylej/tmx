#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h> /* int32_t */
#include <errno.h>

#include "tmx.h"
#include "tmx_utils.h"

/*
	Globals
*/
void* (*tmx_alloc_func) (void *address, size_t len) = NULL;
void  (*tmx_free_func ) (void *address) = NULL;
void* (*rsc_img_load_func) (const char *p) = NULL;
void  (*rsc_img_free_func) (void *address) = NULL;
/*
	Public functions
*/
#ifndef WANT_XML
tmx_map *parse_xml(const char *path) {
	tmx_err(E_FONCT, "This library was not builded with the XML parser");
	return NULL;
}
#endif
#ifndef WANT_JSON
tmx_map *parse_json(const char *filename) {
	tmx_err(E_FONCT, "This library was not builded with the JSON parser");
	return NULL;
}
#endif

tmx_map *tmx_load(const char * path) {
	tmx_map *map = NULL;
	const char *extension;
	FILE *file;
	int fchar;

	if (!tmx_alloc_func) tmx_alloc_func = realloc;
	if (!tmx_free_func) tmx_free_func = free;

	/* is 'path' a JSON or a XML file ? */
	extension = strrchr(path, '.'); /* First using the file extension */
	if (!strcmp(extension, ".tmx") || !strcmp(extension, ".xml")) {
		map = parse_xml(path);
	} else if (!strcmp(extension, ".json")) {
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
		if (rsc_img_free_func) {
			rsc_img_free_func(i->resource_image);
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

static void free_ts(tmx_tileset *ts) {
	if (ts) {
		free_ts(ts->next);
		tmx_free_func(ts->name);
		free_image(ts->image);
		free_props(ts->properties);
		tmx_free_func(ts);
	}
}

void tmx_map_free(tmx_map *map) {
	if (map) {
		free_ts(map->ts_head);
		free_props(map->properties);
		free_layers(map->ly_head);
		tmx_free_func(map);
		*map = (tmx_map){0};
	}
}
