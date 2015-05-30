/*
	XML Parser using the XMLReader API because maps may be huge
	see http://www.xmlsoft.org/xmlreader.html
	see http://www.xmlsoft.org/examples/index.html#reader1.c
*/

#ifdef WANT_XML

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <libxml/xmlreader.h>
#include <libxml/xmlmemory.h>

#include "tmx.h"
#include "tmx_utils.h"
#include "tmx_array.h"

static void* tmx_malloc(size_t len) {
	return tmx_alloc_func(NULL, len);
}

/*
	 - Parsers -
	Each function is called when the XML reader is on an element
	with the same name.
	Each function return 1 on succes and 0 on failure.
	This parser is strict, the entry file MUST respect the file format.
	On failure tmx_errno is set and and an error message is generated.
*/

static void error_handler(void *arg, const char *msg, xmlParserSeverities severity, xmlTextReaderLocatorPtr locator) {
	if (severity == XML_PARSER_SEVERITY_ERROR) {
		tmx_err(E_XDATA, "xml parser: error at line %d: %s", xmlTextReaderLocatorLineNumber(locator), msg);
	}
}

static xmlTextReaderPtr create_parser(const char *filename) {
	xmlTextReaderPtr reader = NULL;
	if ((reader = xmlReaderForFile(filename, NULL, 0))) {

		xmlTextReaderSetErrorHandler(reader, error_handler, NULL);

		if (xmlTextReaderRead(reader) != 1) {
			xmlFreeTextReader(reader);
			reader = NULL;
		}
	} else {
		tmx_err(E_UNKN, "xml parser: unable to open %s", filename);
	}
	return reader;
}

static int parse_property(xmlTextReaderPtr reader, tmx_property *prop) {
	char *value;
	if ((value = (char*)xmlTextReaderGetAttribute(reader, (xmlChar*)"name"))) { /* name */
		prop->name = value;
	} else {
		tmx_err(E_MISSEL, "xml parser: missing 'name' attribute in the 'property' element");
		return 0;
	}

	if ((value = (char*)xmlTextReaderGetAttribute(reader, (xmlChar*)"value"))) { /* source */
		prop->value = value;
	} else {
		tmx_err(E_MISSEL, "xml parser: missing 'value' attribute in the 'property' element");
		return 0;
	}
	return 1;
}

static int parse_properties(xmlTextReaderPtr reader, tmx_property **prop_headadr) {
	tmx_property *res;
	int curr_depth;
	const char *name;

	curr_depth = xmlTextReaderDepth(reader);

	/* Parse each child */
	do {
		if (xmlTextReaderRead(reader) != 1) return 0; /* error_handler has been called */

		if (xmlTextReaderNodeType(reader) == XML_READER_TYPE_ELEMENT) {
			name = (char*)xmlTextReaderConstName(reader);
			if (!strcmp(name, "property")) {
				if (!(res = alloc_prop())) return 0;
				res->next = *prop_headadr;
				*prop_headadr = res;

				if (!parse_property(reader, res)) return 0;

			} else { /* Unknow element, skip its tree */
				if (xmlTextReaderNext(reader) != 1) return 0;
			}
		}
	} while (xmlTextReaderNodeType(reader) != XML_READER_TYPE_END_ELEMENT ||
	         xmlTextReaderDepth(reader) != curr_depth);
	return 1;
}

static int parse_frame(xmlTextReaderPtr reader, tmx_tileset *tileset, tmx_frame *frame) {
	char *value;
	if ((value = (char*)xmlTextReaderGetAttribute(reader, (xmlChar*)"tileid"))) { /* tileid */
		frame->gid = atoi(value) + tileset->firstgid;
		tmx_free_func(value);
	}
	else {
		tmx_err(E_MISSEL, "xml parser: missing 'tileid' attribute in the 'frame' element");
		return 0;
	}

	if ((value = (char*)xmlTextReaderGetAttribute(reader, (xmlChar*)"duration"))) { /* duration */
		frame->duration = atoi(value);
		tmx_free_func(value);
	}
	else {
		tmx_err(E_MISSEL, "xml parser: missing 'duration' attribute in the 'frame' element");
		return 0;
	}
	return 1;
}

static int parse_animation(xmlTextReaderPtr reader, tmx_tileset *tileset, tmx_frame **anim_headadr) {
	tmx_frame **res = anim_headadr;
	int curr_depth;
	const char *name;

	curr_depth = xmlTextReaderDepth(reader);

	/* Parse each child */
	do {
		if (xmlTextReaderRead(reader) != 1) return 0; /* error_handler has been called */

		if (xmlTextReaderNodeType(reader) == XML_READER_TYPE_ELEMENT) {
			name = (char*)xmlTextReaderConstName(reader);
			if (!strcmp(name, "frame")) {
				if (!(*res = alloc_frame())) return 0;

				if (!parse_frame(reader, tileset, *res)) return 0;

				res = &((*res)->next_frame);
			}
			else { /* Unknow element, skip its tree */
				if (xmlTextReaderNext(reader) != 1) return 0;
			}
		}
	} while (xmlTextReaderNodeType(reader) != XML_READER_TYPE_END_ELEMENT ||
		xmlTextReaderDepth(reader) != curr_depth);
	return 1;
}

static int parse_points(xmlTextReaderPtr reader, int ***ptsarrayadr, int *ptslenadr) {
	char *value, *v;
	int i;

	if (!(value = (char*)xmlTextReaderGetAttribute(reader, (xmlChar*)"points"))) { /* points */
		tmx_err(E_MISSEL, "xml parser: missing 'points' attribute in the 'object' element");
		return 0;
	}

	*ptslenadr = 1 + count_char_occurences(value, ' ');

	*ptsarrayadr = (int**)tmx_alloc_func(NULL, *ptslenadr * sizeof(int*)); /* points[i][x,y] */
	if (!(*ptsarrayadr)) {
		tmx_errno = E_ALLOC;
		return 0;
	}

	(*ptsarrayadr)[0] = (int*)tmx_alloc_func(NULL, *ptslenadr * 2 * sizeof(int));
	if (!(*ptsarrayadr)[0]) {
		tmx_free_func(*ptsarrayadr);
		tmx_errno = E_ALLOC;
		return 0;
	}

	for (i=1; i<*ptslenadr; i++) {
		(*ptsarrayadr)[i] = (*ptsarrayadr)[0]+(i*2);
	}

	v = value;
	for (i=0; i<*ptslenadr; i++) {
		if (sscanf(v, "%d,%d", (*ptsarrayadr)[i], (*ptsarrayadr)[i]+1) != 2) {
			tmx_err(E_XDATA, "xml parser: corrupted point list");
			return 0;
		}
		v = 1 + strchr(v, ' ');
	}

	tmx_free_func(value);
	return 1;
}

static int parse_object(xmlTextReaderPtr reader, tmx_object *obj) {
	int curr_depth;
	const char *name;
	char *value;

	/* parses each attribute */
	if ((value = (char*)xmlTextReaderGetAttribute(reader, (xmlChar*)"x"))) { /* x */
		obj->x = atoi(value);
		tmx_free_func(value);
	} else {
		tmx_err(E_MISSEL, "xml parser: missing 'x' attribute in the 'object' element");
		return 0;
	}

	if ((value = (char*)xmlTextReaderGetAttribute(reader, (xmlChar*)"y"))) { /* y */
		obj->y = atoi(value);
		tmx_free_func(value);
	} else {
		tmx_err(E_MISSEL, "xml parser: missing 'y' attribute in the 'object' element");
		return 0;
	}

	if ((value = (char*)xmlTextReaderGetAttribute(reader, (xmlChar*)"name"))) { /* name */
		obj->name = value;
	}

	if ((value = (char*)xmlTextReaderGetAttribute(reader, (xmlChar*)"visible"))) { /* visible */
		obj->visible = (char)atoi(value);
		tmx_free_func(value);
	}

	if ((value = (char*)xmlTextReaderGetAttribute(reader, (xmlChar*)"gid"))) { /* gid */
		obj->shape = S_TILE;
		obj->gid = atoi(value);
		tmx_free_func(value);
	}

	if ((value = (char*)xmlTextReaderGetAttribute(reader, (xmlChar*)"height"))) { /* height */
		obj->shape = S_SQUARE;
		obj->height = atoi(value);
		tmx_free_func(value);
	}

	if ((value = (char*)xmlTextReaderGetAttribute(reader, (xmlChar*)"width"))) { /* width */
		obj->width = atoi(value);
		tmx_free_func(value);
	}

	/* If it has a child, then it's a polygon or a polyline or an ellipse */
	curr_depth = xmlTextReaderDepth(reader);
	if (!xmlTextReaderIsEmptyElement(reader)) {
		do {
			if (xmlTextReaderRead(reader) != 1) return 0; /* error_handler has been called */

			if (xmlTextReaderNodeType(reader) == XML_READER_TYPE_ELEMENT) {
				name = (char*)xmlTextReaderConstName(reader);
				if (!strcmp(name, "properties")) {
					if (!parse_properties(reader, &(obj->properties))) return 0;
				} else if (!strcmp(name, "ellipse")) {
					obj->shape = S_ELLIPSE;
				} else {
					if (!strcmp(name, "polygon")) {
						obj->shape = S_POLYGON;
					} else if (!strcmp(name, "polyline")) {
						obj->shape = S_POLYLINE;
					}
					/* Unknow element, skip its tree */
					else if (xmlTextReaderNext(reader) != 1) return 0;
					if (!parse_points(reader, &(obj->points), &(obj->points_len))) return 0;
				}
			}
		} while (xmlTextReaderNodeType(reader) != XML_READER_TYPE_END_ELEMENT ||
		         xmlTextReaderDepth(reader) != curr_depth);
	}
	return 1;
}

static int parse_data(xmlTextReaderPtr reader, int32_t **gidsadr, size_t gidscount) {
	char *value, *inner_xml;

	if (!(value = (char*)xmlTextReaderGetAttribute(reader, (xmlChar*)"encoding"))) { /* encoding */
		tmx_err(E_MISSEL, "xml parser: missing 'encoding' attribute in the 'data' element");
		return 0;
	}

	if (!(inner_xml = (char*)xmlTextReaderReadInnerXml(reader))) {
		tmx_err(E_XDATA, "xml parser: missing content in the 'data' element");
		tmx_free_func(value);
		return 0;
	}

	if (!strcmp(value, "base64")) {
		tmx_free_func(value);
		if (!(value = (char*)xmlTextReaderGetAttribute(reader, (xmlChar*)"compression"))) { /* compression */
			tmx_err(E_MISSEL, "xml parser: missing 'compression' attribute in the 'data' element");
			goto cleanup;
		}
		if (strcmp(value, "zlib") && strcmp(value, "gzip")) {
			tmx_err(E_ENCCMP, "xml parser: unsupported data compression: '%s'", value); /* unsupported compression */
			goto cleanup;
		}
		if (!data_decode(str_trim(inner_xml), B64Z, gidscount, gidsadr)) goto cleanup;

	} else if (!strcmp(value, "xml")) {
		tmx_err(E_ENCCMP, "xml parser: unimplemented data encoding: XML");
		goto cleanup;
	} else if (!strcmp(value, "csv")) {
		if (!data_decode(str_trim(inner_xml), CSV, gidscount, gidsadr)) goto cleanup;
	} else {
		tmx_err(E_ENCCMP, "xml parser: unknown data encoding: %s", value);
		goto cleanup;
	}
	tmx_free_func(value);
	tmx_free_func(inner_xml);
	return 1;

cleanup:
	tmx_free_func(value);
	tmx_free_func(inner_xml);
	return 0;
}

static int parse_image(xmlTextReaderPtr reader, tmx_image **img_adr, short strict, const char *filename) {
	tmx_image *res;
	char *value;

	if (!(res = alloc_image())) return 0;
	*img_adr = res;

	if ((value = (char*)xmlTextReaderGetAttribute(reader, (xmlChar*)"source"))) { /* source */
		res->source = value;
		if (!(load_image(&(res->resource_image), filename, value))) {
			tmx_err(E_UNKN, "xml parser: an error occured in the delegated image loading function");
			return 0;
		}
	} else {
		tmx_err(E_MISSEL, "xml parser: missing 'source' attribute in the 'image' element");
		return 0;
	}

	if ((value = (char*)xmlTextReaderGetAttribute(reader, (xmlChar*)"height"))) { /* height */
		res->height = atoi(value);
		tmx_free_func(value);
	} else if (strict) {
		tmx_err(E_MISSEL, "xml parser: missing 'height' attribute in the 'image' element");
		return 0;
	}

	if ((value = (char*)xmlTextReaderGetAttribute(reader, (xmlChar*)"width"))) { /* width */
		res->width = atoi(value);
		tmx_free_func(value);
	} else if (strict) {
		tmx_err(E_MISSEL, "xml parser: missing 'width' attribute in the 'image' element");
		return 0;
	}

	if ((value = (char*)xmlTextReaderGetAttribute(reader, (xmlChar*)"trans"))) { /* trans */
		res->trans = get_color_rgb(value);
		res->uses_trans = 1;
		tmx_free_func(value);
	}

	return 1;
}

/* parse layers and objectgroups */
static int parse_layer(xmlTextReaderPtr reader, tmx_layer **layer_headadr, int map_h, int map_w, enum tmx_layer_type type, const char *filename) {
	tmx_layer *res;
	tmx_object *obj;
	int curr_depth;
	const char *name;
	char *value;

	curr_depth = xmlTextReaderDepth(reader);

	if (!(res = alloc_layer())) return 0;
	res->type = type;
	while(*layer_headadr) {
		layer_headadr = &((*layer_headadr)->next);
	}
	*layer_headadr = res;

	/* parses each attribute */
	if ((value = (char*)xmlTextReaderGetAttribute(reader, (xmlChar*)"name"))) { /* name */
		res->name = value;
	} else {
		tmx_err(E_MISSEL, "xml parser: missing 'name' attribute in the 'layer' element");
		return 0;
	}

	if ((value = (char*)xmlTextReaderGetAttribute(reader, (xmlChar*)"visible"))) { /* visible */
		res->visible = (char)atoi(value);
		tmx_free_func(value);
	}

	if ((value = (char*)xmlTextReaderGetAttribute(reader, (xmlChar*)"opacity"))) { /* opacity */
		res->opacity = (float)strtod(value, NULL);
		tmx_free_func(value);
	}

	if ((value = (char*)xmlTextReaderGetAttribute(reader, (xmlChar*)"color"))) { /* color */
		res->color = get_color_rgb(value);
		tmx_free_func(value);
	}

	if ((value = (char*)xmlTextReaderGetAttribute(reader, (xmlChar*)"x"))) { /* x_offset */
		res->x_offset = (int)atoi(value);
		tmx_free_func(value);
	}

	if ((value = (char*)xmlTextReaderGetAttribute(reader, (xmlChar*)"y"))) { /* y_offset */
		res->y_offset = (int)atoi(value);
		tmx_free_func(value);
	}

	do {
		if (xmlTextReaderRead(reader) != 1) return 0; /* error_handler has been called */

		if (xmlTextReaderNodeType(reader) == XML_READER_TYPE_ELEMENT) {
			name = (char*)xmlTextReaderConstName(reader);
			if (!strcmp(name, "properties")) {
				if (!parse_properties(reader, &(res->properties))) return 0;
			} else if (!strcmp(name, "data")) {
				if (!parse_data(reader, &(res->content.gids), map_h * map_w)) return 0;
			} else if (!strcmp(name, "image")) {
				if (!parse_image(reader, &(res->content.image), 0, filename)) return 0;
			} else if (!strcmp(name, "object")) {
				if (!(obj = alloc_object())) return 0;

				obj->next = res->content.head;
				res->content.head = obj;

				if (!parse_object(reader, obj)) return 0;
			} else {
				/* Unknow element, skip its tree */
				if (xmlTextReaderNext(reader) != 1) return 0;
			}
		}
	} while (xmlTextReaderNodeType(reader) != XML_READER_TYPE_END_ELEMENT ||
	         xmlTextReaderDepth(reader) != curr_depth);

	return 1;
}

static int parse_tileoffset(xmlTextReaderPtr reader, int *x, int *y) {
	char *value;
	if ((value = (char*)xmlTextReaderGetAttribute(reader, (xmlChar*)"x"))) { /* x offset */
		*x = atoi(value);
		tmx_free_func(value);
	} else {
		tmx_err(E_MISSEL, "xml parser: missing 'x' attribute in the 'tileoffset' element");
		return 0;
	}

	if ((value = (char*)xmlTextReaderGetAttribute(reader, (xmlChar*)"y"))) { /* y offset */
		*y = atoi(value);
		tmx_free_func(value);
	} else {
		tmx_err(E_MISSEL, "xml parser: missing 'y' attribute in the 'tileoffset' element");
		return 0;
	}

	return 1;
}

static int parse_tile(tmx_tileset *tileset, xmlTextReaderPtr reader, tmx_sorted_array *tiles_array, const char *filename) {
	tmx_tile res;
	memset(&res, 0, sizeof(res));
	int curr_depth;
	const char *name;
	char *value;

	curr_depth = xmlTextReaderDepth(reader);

	if ((value = (char*)xmlTextReaderGetAttribute(reader, (xmlChar*)"id"))) { /* id */
		res.gid = atoi(value) + tileset->firstgid;
		tmx_free_func(value);
	}
	else {
		tmx_err(E_MISSEL, "xml parser: missing 'id' attribute in the 'tile' element");
		return 0;
	}

	tmx_tile *new_tile = NULL;
	if (!(new_tile = tmx_sa_insert(tiles_array, &res))) return 0;

	tileset->num_tiles++;	
	if (tileset->tiles == NULL || res.gid < (unsigned int)(tileset->tiles))
		tileset->tiles = (void*)(res.gid); /* Write tile gid into tiles pointer - we will use this to get the actual pointer later */

	do {
		if (xmlTextReaderRead(reader) != 1) return 0; /* error_handler has been called */

		if (xmlTextReaderNodeType(reader) == XML_READER_TYPE_ELEMENT) {
			name = (char*)xmlTextReaderConstName(reader);
			if (!strcmp(name, "properties")) {
				if (!parse_properties(reader, &(new_tile->properties))) return 0;
			}
			else if (!strcmp(name, "image")) {
				if (!parse_image(reader, &(new_tile->image), 0, filename)) return 0;
			}
			else if (!strcmp(name, "animation")) {
				if (!parse_animation(reader, tileset, &(new_tile->animation))) return 0;
			}
			else {
				/* Unknow element, skip its tree */
				if (xmlTextReaderNext(reader) != 1) return 0;
			}
		}
	} while (xmlTextReaderNodeType(reader) != XML_READER_TYPE_END_ELEMENT ||
		xmlTextReaderDepth(reader) != curr_depth);

	return 1;
}

/* parses a tileset within the tmx file or in a dedicated tsx file */
static int parse_tileset_sub(xmlTextReaderPtr reader, tmx_tileset *ts_addr, tmx_sorted_array *tiles_array, const char *filename) {
	int curr_depth;
	const char *name;
	char *value;

	curr_depth = xmlTextReaderDepth(reader);

	/* parses each attribute */
	if ((value = (char*)xmlTextReaderGetAttribute(reader, (xmlChar*)"name"))) { /* name */
		ts_addr->name = value;
	} else {
		tmx_err(E_MISSEL, "xml parser: missing 'name' attribute in the 'tileset' element");
		return 0;
	}

	if ((value = (char*)xmlTextReaderGetAttribute(reader, (xmlChar*)"tilewidth"))) { /* tile_width */
		ts_addr->tile_width = atoi(value);
		tmx_free_func(value);
	} else {
		tmx_err(E_MISSEL, "xml parser: missing 'tilewidth' attribute in the 'tileset' element");
		return 0;
	}

	if ((value = (char*)xmlTextReaderGetAttribute(reader, (xmlChar*)"tileheight"))) { /* tile_height */
		ts_addr->tile_height = atoi(value);
		tmx_free_func(value);
	} else {
		tmx_err(E_MISSEL, "xml parser: missing 'tileheight' attribute in the 'tileset' element");
		return 0;
	}

	if ((value = (char*)xmlTextReaderGetAttribute(reader, (xmlChar*)"spacing"))) { /* spacing */
		ts_addr->spacing = atoi(value);
		tmx_free_func(value);
	}

	if ((value = (char*)xmlTextReaderGetAttribute(reader, (xmlChar*)"margin"))) { /* margin */
		ts_addr->margin = atoi(value);
		tmx_free_func(value);
	}

	/* Parse each child */
	do {
		if (xmlTextReaderRead(reader) != 1) return 0; /* error_handler has been called */

		if (xmlTextReaderNodeType(reader) == XML_READER_TYPE_ELEMENT) {
			name = (char*)xmlTextReaderConstName(reader);
			if (!strcmp(name, "image")) {
				if (!parse_image(reader, &(ts_addr->image), 1, filename)) return 0;
			} else if (!strcmp(name, "tileoffset")) {
				if (!parse_tileoffset(reader, &(ts_addr->x_offset), &(ts_addr->y_offset))) return 0;
			} else if (!strcmp(name, "properties")) {
				if (!parse_properties(reader, &(ts_addr->properties))) return 0;
			} else if (!strcmp(name, "tile")) {
				if (!parse_tile(ts_addr, reader, tiles_array, filename)) return 0;
			} else {
				/* Unknown element, skip its tree */
				if (xmlTextReaderNext(reader) != 1) return 0;
			}
		}
	} while (xmlTextReaderNodeType(reader) != XML_READER_TYPE_END_ELEMENT ||
	         xmlTextReaderDepth(reader) != curr_depth);

	return 1;
}

static int parse_tileset(xmlTextReaderPtr reader, tmx_tileset **ts_headadr, tmx_sorted_array *tiles_array, const char *filename) {
	tmx_tileset *res = NULL;
	int ret;
	char *value, *ab_path;
	xmlTextReaderPtr sub_reader;

	if (!(res = alloc_tileset())) return 0;
	res->next = *ts_headadr;
	*ts_headadr = res;

	/* parses each attribute */
	if ((value = (char*)xmlTextReaderGetAttribute(reader, (xmlChar*)"firstgid"))) { /* fisrtgid */
		res->firstgid = atoi(value);
		tmx_free_func(value);
	} else {
		tmx_err(E_MISSEL, "xml parser: missing 'firstgid' attribute in the 'tileset' element");
		return 0;
	}

	if ((value = (char*)xmlTextReaderGetAttribute(reader, (xmlChar*)"source"))) { /* source */
		if (!(ab_path = mk_absolute_path(filename, value))) return 0;
		tmx_free_func(value);
		if (!(sub_reader = create_parser(ab_path))) return 0; /* opens */
		ret = parse_tileset_sub(sub_reader, res, tiles_array, ab_path); /* and parses the tsx file */
		xmlFreeTextReader(sub_reader);
		tmx_free_func(ab_path);
		return ret;
	}

	ret = parse_tileset_sub(reader, res, tiles_array, filename);
	return ret;
}

static tmx_map *parse_root_map(xmlTextReaderPtr reader, tmx_sorted_array *tiles_array, const char *filename) {
	tmx_map *res = NULL;
	int curr_depth;
	const char *name;
	char *value;

	name = (char*) xmlTextReaderConstName(reader);
	curr_depth = xmlTextReaderDepth(reader);

	if (strcmp(name, "map")) {
		tmx_err(E_XDATA, "xml parser: root is not a 'map' element");
		return NULL;
	}

	if (!(res = alloc_map())) return NULL;

	/* parses each attribute */
	if ((value = (char*)xmlTextReaderGetAttribute(reader, (xmlChar*)"orientation"))) { /* orientation */
		if (res->orient = parse_orient(value), res->orient == O_NONE) {
			tmx_err(E_XDATA, "xml parser: unsupported 'orientation' '%s'", value);
			goto cleanup;
		}
		tmx_free_func(value);
	} else {
		tmx_err(E_MISSEL, "xml parser: missing 'orientation' attribute in the 'map' element");
		goto cleanup;
	}

	if ((value = (char*)xmlTextReaderGetAttribute(reader, (xmlChar*)"height"))) { /* height */
		res->height = atoi(value);
		tmx_free_func(value);
	} else {
		tmx_err(E_MISSEL, "xml parser: missing 'height' attribute in the 'map' element");
		goto cleanup;
	}

	if ((value = (char*)xmlTextReaderGetAttribute(reader, (xmlChar*)"width"))) { /* width */
		res->width = atoi(value);
		tmx_free_func(value);
	} else {
		tmx_err(E_MISSEL, "xml parser: missing 'width' attribute in the 'map' element");
		goto cleanup;
	}

	if ((value = (char*)xmlTextReaderGetAttribute(reader, (xmlChar*)"tileheight"))) { /* tileheight */
		res->tile_height = atoi(value);
		tmx_free_func(value);
	} else {
		tmx_err(E_MISSEL, "xml parser: missing 'tileheight' attribute in the 'map' element");
		goto cleanup;
	}

	if ((value = (char*)xmlTextReaderGetAttribute(reader, (xmlChar*)"tilewidth"))) { /* tilewidth */
		res->tile_width = atoi(value);
		tmx_free_func(value);
	} else {
		tmx_err(E_MISSEL, "xml parser: missing 'tilewidth' attribute in the 'map' element");
		goto cleanup;
	}

	if ((value = (char*)xmlTextReaderGetAttribute(reader, (xmlChar*)"backgroundcolor"))) { /* backgroundcolor */
		res->backgroundcolor = get_color_rgb(value);
		tmx_free_func(value);
	}

	/* Parse each child */
	do {
		if (xmlTextReaderRead(reader) != 1) goto cleanup; /* error_handler has been called */

		if (xmlTextReaderNodeType(reader) == XML_READER_TYPE_ELEMENT) {
			name = (char*)xmlTextReaderConstName(reader);
			if (!strcmp(name, "tileset")) {
				if (!parse_tileset(reader, &(res->ts_head), tiles_array, filename)) goto cleanup;
			} else if (!strcmp(name, "layer")) {
				if (!parse_layer(reader, &(res->ly_head), res->height, res->width, L_LAYER, filename)) goto cleanup;
			} else if (!strcmp(name, "objectgroup")) {
				if (!parse_layer(reader, &(res->ly_head), res->height, res->width, L_OBJGR, filename)) goto cleanup;
			} else if (!strcmp(name, "imagelayer")) {
				if (!parse_layer(reader, &(res->ly_head), res->height, res->width, L_IMAGE, filename)) goto cleanup;
			} else if (!strcmp(name, "properties")) {
				if (!parse_properties(reader, &(res->properties))) goto cleanup;
			} else {
				/* Unknow element, skip its tree */
				if (xmlTextReaderNext(reader) != 1) goto cleanup;
			}
		}
	} while (xmlTextReaderNodeType(reader) != XML_READER_TYPE_END_ELEMENT ||
	         xmlTextReaderDepth(reader) != curr_depth);
	return res;
cleanup:
	tmx_map_free(res);
	return NULL;
}

tmx_map *parse_xml(tmx_sorted_array *tiles_array, const char *filename) {
	xmlTextReaderPtr reader;
	tmx_map *res = NULL;

	xmlMemSetup((xmlFreeFunc)tmx_free_func, (xmlMallocFunc)tmx_malloc, (xmlReallocFunc)tmx_alloc_func, (xmlStrdupFunc)tmx_strdup);

	if ((reader = create_parser(filename))) {
		res = parse_root_map(reader, tiles_array, filename);
		xmlFreeTextReader(reader);
	}

	return res;
}

#else

#include <stdio.h>
#include "tmx.h"
#include "tmx_utils.h"

tmx_map *parse_xml(const char *path) {
	tmx_err(E_FONCT, "This library was not built with the XML parser");
	return NULL;
}

#endif /* WANT_XML */
