/*
	JSON Parser using Jansson
	see http://www.digip.org/jansson/
	Loads the full JSON file in jansson's data structure
	and then creates the tmx data structure.
*/

#ifdef WANT_JSON

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#include <jansson.h>

#include "tmx.h"
#include "tmx_utils.h"

static void* json_malloc(size_t size) {
	return tmx_alloc_func(NULL, size);
}

/*
	 - Parsers -
	Each function return 1 on succes and 0 on failure.
	This parser is strict, the entry file MUST respect the file format.
	On failure tmx_errno is set and and an error message is generated.
*/

static int pjson_properties(json_t *prp_el, tmx_property **prp_headaddr) {
	tmx_property *p;
	const char *key;
	json_t     *val;

	json_object_foreach(prp_el, key, val) {
		if (!json_is_string(val)) {
			tmx_err(E_JDATA, "json parser: property %s has a non-string value", key);
			return 0;
		}
		if (!(p = alloc_prop())) return 0;
		p->next = *prp_headaddr;
		*prp_headaddr = p;

		p->name  = tmx_strdup(key);
		p->value = tmx_strdup(json_string_value(val));
	}

	return 1;
}

/* unlike other parsers, this one's first argument is an array, not an element */
static int pjson_points(json_t *pts_ar, int ***pts_araddr, int *ptslen_addr) {
	json_error_t err;
	json_t *tmp;
	int i;

	*ptslen_addr = (int)(json_array_size(pts_ar));
	if (!(*pts_araddr = (int**)tmx_alloc_func(NULL, *ptslen_addr * sizeof(int*)))) {
		tmx_errno = E_ALLOC;
		return 0;
	}
	if (!(*pts_araddr[0] = (int*)tmx_alloc_func(NULL, *ptslen_addr * 2 * sizeof(int)))) {
		tmx_errno = E_ALLOC;
		return 0;
	}
	for (i=1; i<*ptslen_addr; i++) {
		(*pts_araddr)[i] = (*pts_araddr)[0]+(i*2);
	}

	for (i=0; i<*ptslen_addr; i++) {
		tmp = json_array_get(pts_ar, i);
		if (json_unpack_ex(tmp, &err, 0, "{s:i, s:i}", "x", (*pts_araddr)[i], "y", (*pts_araddr)[i]+1)) {
			tmx_err(E_MISSEL, "json parser: (point) %s", err.text);
			return 0;
		}
	}

	return 1;
}

static int pjson_objects(json_t *obj_el, tmx_object **obj_headaddr) {
	json_error_t err;
	json_t *tmp;
	tmx_object *o;
	char *name;

	if (!(o = alloc_object())) return 0;
	o->next = *obj_headaddr;
	*obj_headaddr = o;

	if (json_unpack_ex(obj_el, &err, 0, "{s:i, s:i, s:i, s:i, s:b, s:s}",
	                   "height",  &(o->height),  "width", &(o->width),
	                   "x",       &(o->x),       "y",     &(o->y),
	                   "visible", &(o->visible), "name",   &name)) {
		tmx_err(E_MISSEL, "json parser: (tileset) %s", err.text);
		return 0;
	}
	if (!(o->name = tmx_strdup(name))) return 0;

	if ((tmp = json_object_get(obj_el, "polyline")) && json_is_array(tmp)) {
		o->shape = S_POLYLINE;
		if (!(pjson_points(tmp, &(o->points), &(o->points_len)))) return 0;
	} else if ((tmp = json_object_get(obj_el, "polygon")) && json_is_array(tmp)) {
		o->shape = S_POLYGON;
		if (!(pjson_points(tmp, &(o->points), &(o->points_len)))) return 0;
	} else if ((tmp = json_object_get(obj_el, "ellipse")) && json_is_boolean(tmp))  {
		o->shape = S_ELLIPSE;
	} else if ((tmp = json_object_get(obj_el, "gid")) && json_is_number(tmp)) {
		o->shape = S_TILE;
		o->gid = (int)json_integer_value(tmp);
	} else {
		o->shape = S_SQUARE;
	}

	if ((tmp = json_object_get(obj_el, "properties")) && json_is_object(tmp)) {
		if (!pjson_properties(tmp, &(o->properties))) return 0;
	}

	return 1;
}

static int pjson_layer(json_t *lay_el, tmx_layer **lay_headaddr, const char *filename) {
	json_error_t err;
	json_t *tmp;
	tmx_layer *lay;
	char *type, *name;
	int i;

	if (!(lay = alloc_layer())) return 0;
	lay->next = *lay_headaddr;
	*lay_headaddr = lay;

	if (json_unpack_ex(lay_el, &err, 0, "{s:b, s:F, s:i, s:i, s:s, s:s}",
	                   "visible", &(lay->visible),  "opacity", &(lay->opacity), // Something changed on this line
	                   "x",       &(lay->x_offset), "y",       &(lay->y_offset), // Something changed on this line
	                   "type",    &type,            "name",    &name)) { // Something changed on this line
		tmx_err(E_MISSEL, "json parser: (layer) %s", err.text);
		return 0;
	}
	if (!(lay->name = tmx_strdup(name))) return 0;
	if (!strcmp(type, "tilelayer")) {
		lay->type = L_LAYER;
	} else if (!strcmp(type, "objectgroup")) {
		lay->type = L_OBJGR;
	} else if (!strcmp(type, "imagelayer")) {
		lay->type = L_IMAGE;
	} else {
		tmx_err(E_JDATA, "json parser: unknown layer type: %s", type);
		return 0;
	}

	if (!json_unpack(lay_el, "{s:s}", "color", &name)) lay->color = get_color_rgb(name);

	if (lay->type == L_LAYER) {
		/* FIXME verify data length = map.height * map.width */
		if ((tmp = json_object_get(lay_el, "data")) && json_is_array(tmp)) {
			if (!(lay->content.gids = (int32_t*)tmx_alloc_func(NULL, json_array_size(tmp) * sizeof(int32_t)))) {
				tmx_errno = E_ALLOC;
				return 0;
			}
			for (i=0; i<(int)json_array_size(tmp); i++) {
				lay->content.gids[i] = (int32_t)json_integer_value(json_array_get(tmp, i));
			}
		}
	} else if (lay->type == L_IMAGE) {
		lay->content.image = alloc_image();
		if (lay->content.image && !json_unpack(lay_el, "{s:s}", "image", &name)) {
			lay->content.image->source = tmx_strdup(name);
			if (!load_image(&(lay->content.image->resource_image), filename, name)) {
				tmx_err(E_UNKN, "json parser: an error occured in the delegated image loading function");
				return 0;
			}
		} else return 0;
	} else {
		if ((tmp = json_object_get(lay_el, "objects")) && json_is_array(tmp)) {
			for (i=0; i<(int)json_array_size(tmp); i++) {
				if (!pjson_objects(json_array_get(tmp, i), &(lay->content.head))) return 0;
			}
		}
	}

	if ((tmp = json_object_get(lay_el, "properties")) && json_is_object(tmp)) {
		if (!pjson_properties(tmp, &(lay->properties))) return 0;
	}

	return 1;
}

static int pjson_tile(json_t *tile_el, tmx_tile **tile_headaddr) {
	tmx_tile *tile;
	const char *key;
	json_t *val;

	json_object_foreach(tile_el, key, val) {
		if (!(tile = alloc_tile())) return 0;
		tile->next = *tile_headaddr;
		*tile_headaddr = tile;

		tile->id = atoi(key);

		if (json_is_object(val)) {
			if (!pjson_properties(val, &(tile->properties))) return 0;
		}
	}

	return 1;
}


/* This is a rather slow solution - anyone with more spare time, feel free to rework the whole structure of
   the tiles list to make this faster */
static tmx_tile* pjson_tiles_find_or_alloc(tmx_tile **tile_headaddr, unsigned int id) {
	tmx_tile *tile = *tile_headaddr;

	if (tile == NULL) {
		if (!(*tile_headaddr = alloc_tile())) return NULL;
		(*tile_headaddr)->id = id;
		return *tile_headaddr;
	}
	else {
		while (tile != NULL) {
			if (tile->id == id) return tile;
			if (tile->next != NULL)
				tile = tile->next;
			else {
				if (!(tile->next = alloc_tile())) return NULL;
				tile->next->id = id;
				return tile->next;
			}
		}
	}

	return NULL;
}

static int pjson_tiles_images(json_t *tile_el, tmx_tile **tile_headaddr, const char *filename) {
	tmx_tile *tile = NULL;
	const char *key;
	json_t *val;
	json_error_t err;
	char *img;

	json_object_foreach(tile_el, key, val) {
		unsigned int key_value = atoi(key);
		tile = pjson_tiles_find_or_alloc(tile_headaddr, key_value);
		if (json_is_object(val) && tile != NULL) {
			if (!(tile->image = alloc_image())) return 0;
			if (json_unpack_ex(val, &err, 0, "{s:i, s:i, s:s}", "imageheight", &(tile->image->height), "imagewidth", &(tile->image->width), "image", &img))
				json_unpack_ex(val, &err, 0, "{s:s}", "image", &img);

			if (img != NULL) {
				if (!(tile->image->source = tmx_strdup(img))) return 0;
				if (!load_image(&(tile->image->resource_image), filename, img)) {
					tmx_err(E_UNKN, "json parser: an error occured in the delegated image loading function");
					return 0;
				}
			}
			else {
				tmx_err(E_MISSEL, "json parser: couldn't read image source file string for tmx_tile");
				return 0;
			}

			tile = tile->next;
		}
		else {
			tmx_err(E_ALLOC, "json parser: couldn't allocate or find tmx_tile for image collection tileset");
			return 0;
		}
	}

	return 1;
}

static int pjson_tileset(json_t *tls_el, tmx_tileset **tst_headaddr, const char *filename) {
	json_error_t err;
	json_t *tmp;
	tmx_tileset *ts;
	char *img, *name;
	unsigned int image_width, image_height;

	if (!(ts = alloc_tileset()))      return 0;
	ts->next = *tst_headaddr;
	*tst_headaddr = ts;

	if (!(json_unpack_ex(tls_el, &err, 0, "{s:i, s:i, s:i, s:i, s:i, s:i, s:i, s:s, s:s}",
	                   "spacing",     &(ts->spacing),       "margin",     &(ts->margin),
	                   "tileheight",  &(ts->tile_height),   "tilewidth",  &(ts->tile_width),
					   "imageheight", &(image_height),		"imagewidth", &(image_width),
	                   "firstgid",    &(ts->firstgid),      "image",      &img,
	                   "name",        &name))) {
		if (!(ts->image = alloc_image())) return 0;
		ts->image->width = image_width;
		ts->image->height = image_height;
		if (!(ts->name = tmx_strdup(name)))         return 0;
		if (!(ts->image->source = tmx_strdup(img))) return 0;
		if (!load_image(&(ts->image->resource_image), filename, img)) {
			tmx_err(E_UNKN, "json parser: an error occured in the delegated image loading function");
			return 0;
		}

		if ((tmp = json_object_get(tls_el, "properties")) && json_is_object(tmp)) {
			if (!pjson_properties(tmp, &(ts->properties))) return 0;
		}

		if ((tmp = json_object_get(tls_el, "tileproperties")) && json_is_object(tmp)) {
			if (!pjson_tile(tmp, &(ts->tiles))) return 0;
		}

		if ((tmp = json_object_get(tls_el, "tileoffset")) && json_is_object(tmp)) {
			const char *key;
			json_t *val;
			json_object_foreach(tmp, key, val) {
				// TODO: Add some error handling here
				if (!strcmp(key, "x")) ts->x_offset = (int)json_integer_value(val);
				if (!strcmp(key, "y")) ts->y_offset = (int)json_integer_value(val);
			}
		}

		char* trans_string;

		if (!(json_unpack_ex(tls_el, &err, 0, "{s:s}", "transparentcolor", &trans_string))) {
			if (trans_string != NULL) {
				ts->image->uses_trans = 1;
				if (trans_string[0] == '#') {
					ts->image->trans = (int)(strtoul(&(trans_string[1]), NULL, 16));
				}
				else
					ts->image->trans = (int)(strtoul(&(trans_string[0]), NULL, 16));
			}
		}
	}
	else if (!(json_unpack_ex(tls_el, &err, 0, "{s:i, s:i, s:i, s:i, s:i, s:s}",
							"spacing", &(ts->spacing), "margin", &(ts->margin),
							"tileheight", &(ts->tile_height), "tilewidth", &(ts->tile_width),
							"firstgid", &(ts->firstgid),
							"name", &name))) {
		ts->image = NULL;

		if (!(ts->name = tmx_strdup(name)))         return 0;

		if ((tmp = json_object_get(tls_el, "properties")) && json_is_object(tmp)) {
			if (!pjson_properties(tmp, &(ts->properties))) return 0;
		}

		if ((tmp = json_object_get(tls_el, "tileproperties")) && json_is_object(tmp)) {
			if (!pjson_tile(tmp, &(ts->tiles))) return 0;
		}

		if ((tmp = json_object_get(tls_el, "tiles")) && json_is_object(tmp)) {
			if (!pjson_tiles_images(tmp, &(ts->tiles), filename)) return 0;
		}

		if ((tmp = json_object_get(tls_el, "tileoffset")) && json_is_object(tmp)) {
			const char *key;
			json_t *val;
			json_object_foreach(tmp, key, val) {
				// TODO: Add some error handling here
				if (!strcmp(key, "x")) ts->x_offset = (int)json_integer_value(val);
				if (!strcmp(key, "y")) ts->y_offset = (int)json_integer_value(val);
			}
		}
	}
	else {

		tmx_err(E_MISSEL, "json parser: (tileset) %s", err.text);
		return 0;
	}

	return 1;
}

/* returns NULL on fail */
static tmx_map* pjson_map(json_t *map_el, const char *filename) {
	json_error_t err;
	json_t *tmp;
	tmx_map *res;
	char *col, *orient;
	int i;

	if (!(res = alloc_map())) return NULL;

	if (json_unpack_ex(map_el, &err, 0, "{s:i, s:i, s:i, s:i, s:s}",
	                   "height",      &(res->height),      "width",     &(res->width),
	                   "tileheight",  &(res->tile_height), "tilewidth", &(res->tile_width),
	                   "orientation", &orient)) {
		tmx_err(E_MISSEL, "json parser: %s", err.text);
		goto cleanup;
	}

	if ((res->orient = parse_orient(orient)) == O_NONE) {
		tmx_err(E_JDATA, "json parser: unsupported 'orientation' '%s'", orient);
		goto cleanup;
	}

	if (!json_unpack(map_el, "{s:s}", "backgroundcolor", &col)) res->backgroundcolor = get_color_rgb(col);

	if ((tmp = json_object_get(map_el, "tilesets")) && json_is_array(tmp)) {
		for (i=0; i<(int)json_array_size(tmp); i++) {
			if (!pjson_tileset(json_array_get(tmp, i), &(res->ts_head), filename)) goto cleanup;
		}
	}

	if ((tmp = json_object_get(map_el, "layers")) && json_is_array(tmp)) {
		i = (int)(json_array_size(tmp));
		for (--i; i>=0 ; i--) { /* tail appending */
			if (!pjson_layer(json_array_get(tmp, i), &(res->ly_head), filename)) goto cleanup;
		}
	}

	if ((tmp = json_object_get(map_el, "properties")) && json_is_object(tmp)) {
		if (!pjson_properties(tmp, &(res->properties))) return 0;
	}

	return res;
cleanup:
	tmx_map_free(res);
	return NULL;
}

tmx_map* parse_json(const char *filename) {
	tmx_map *res = NULL;
	json_t *parsed = NULL;
	json_error_t err;

	/* set memory alloc/free functions pointers */
	json_set_alloc_funcs(json_malloc, tmx_free_func);

	if (!(parsed = json_load_file(filename, 0, &err))) {
		tmx_err(E_JDATA, "json parser: (%s:%d,%d) %s", filename, err.line, err.column, err.text);
		return NULL;
	}

	res = pjson_map(parsed, filename);

	json_decref(parsed);

	return res;
}

#else

#include <stdio.h>
#include "tmx.h"
#include "tmx_utils.h"

tmx_map* parse_json(const char *filename) {
	tmx_err(E_FONCT, "This library was not built with the JSON parser");
	return NULL;
}

#endif /* WANT_JSON */
