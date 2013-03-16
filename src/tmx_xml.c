/* FIXME let the user choose if he links staticaly */
#define LIBXML_STATIC

/*
	XML Parser using the XMLReader API because maps may be huge
	see http://www.xmlsoft.org/xmlreader.html
	see http://www.xmlsoft.org/examples/index.html#reader1.c
	It uses top-appended linked lists, so if you have several layers
	in your map, your first layer in the xml will be the last in the
	linked list.
*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <libxml/xmlreader.h>
#include <libxml/xmlmemory.h>

#include "tmx.h"
#include "tmx_utils.h"

/*
	Memory functions
*/

void * tmx_malloc(size_t len) {
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

static int parse_property(xmlTextReaderPtr reader, tmx_property prop) {
	char *value;
	if ((value = (char*)xmlTextReaderGetAttribute(reader, "name"))) { /* name */
		prop->name = value;
	} else {
		tmx_err(E_MISSEL, "missing 'name' attribute in the 'property' element");
		return 0;
	}

	if ((value = (char*)xmlTextReaderGetAttribute(reader, "value"))) { /* source */
		prop->value = value;
	} else {
		tmx_err(E_MISSEL, "missing 'value' attribute in the 'property' element");
		return 0;
	}
	return 1;
}

static int parse_properties(xmlTextReaderPtr reader, tmx_property *prop_headadr) {
	tmx_property res;
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

			} else { /* Unknow element, skipping it's tree */
				if (xmlTextReaderNext(reader) != 1) return 0;
			}
		}
	} while (xmlTextReaderNodeType(reader) != XML_READER_TYPE_END_ELEMENT ||
	         xmlTextReaderDepth(reader) != curr_depth);
	return 1;
}

static int parse_data(xmlTextReaderPtr reader, int32_t **gidsadr, size_t gidscount) {
	char *value, *inner_xml;

	if (!(value = (char*)xmlTextReaderGetAttribute(reader, "encoding"))) { /* encoding */
		tmx_err(E_MISSEL, "missing 'encoding' attribute in the 'data' element");
		return 0;
	}

	if (!(inner_xml = (char*)xmlTextReaderReadInnerXml(reader))) {
		tmx_err(E_XDATA, "missing content in the 'data' element", value);
		tmx_free_func(value);
		return 0;
	}

	if (!strcmp(value, "base64")) {
		tmx_free_func(value);
		if (!(value = (char*)xmlTextReaderGetAttribute(reader, "compression"))) { /* compression */
			tmx_err(E_MISSEL, "missing 'compression' attribute in the 'data' element");
			goto cleanup;
		}
		if (strcmp(value, "zlib") && strcmp(value, "gzip")) {
			tmx_err(E_ENCCMP, "unsupported data compression: '%s'", value); /* unsupported compression */
			goto cleanup;
		} 
		if (!data_decode(str_trim(inner_xml), B64Z, gidscount, gidsadr)) goto cleanup;

	} else if (!strcmp(value, "xml")) {
		tmx_err(E_ENCCMP, "unimplemented data encoding: XML");
		goto cleanup;
	} else if (!strcmp(value, "csv")) {
		if (!data_decode(str_trim(inner_xml), CSV, gidscount, gidsadr)) goto cleanup;
	} else {
		tmx_err(E_ENCCMP, "unknown data encoding: %s", value);
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

static int parse_layer(xmlTextReaderPtr reader, tmx_layer *layer_headadr, int map_height, int map_width) {
	tmx_layer res;
	int curr_depth;
	const char *name;
	char *value;

	curr_depth = xmlTextReaderDepth(reader);

	if (!(res = alloc_layer())) return 0;
	res->type = L_LAYER;
	res->next = *layer_headadr;
	*layer_headadr = res;

	/* parses each attribute */
	if ((value = (char*)xmlTextReaderGetAttribute(reader, "name"))) { /* name */
		res->name = value;
	} else {
		tmx_err(E_MISSEL, "missing 'name' attribute in the 'layer' element");
		return 0;
	}

	if ((value = (char*)xmlTextReaderGetAttribute(reader, "visible"))) { /* visible */
		res->visible = value[0]=='t' ? 1: 0; /* if (visible=="true") visible <-- 1 */
		tmx_free_func(value);
	}
	
	if ((value = (char*)xmlTextReaderGetAttribute(reader, "opacity"))) { /* opacity */
		res->opacity = (float)strtod(value, NULL);
		tmx_free_func(value);
	}

	do {
		if (xmlTextReaderRead(reader) != 1) return 0; /* error_handler has been called */
		
		if (xmlTextReaderNodeType(reader) == XML_READER_TYPE_ELEMENT) {
			name = (char*)xmlTextReaderConstName(reader);
			if (!strcmp(name, "properties")) {
				if (!parse_properties(reader, &(res->properties))) return 0;
			} else if (!strcmp(name, "data")) {
				if (!parse_data(reader, &(res->content.gids), map_height * map_width)) return 0;
			} else {
				/* Unknow element, skipping it's tree */
				if (xmlTextReaderNext(reader) != 1) return 0;
			}
		}
	} while (xmlTextReaderNodeType(reader) != XML_READER_TYPE_END_ELEMENT ||
	         xmlTextReaderDepth(reader) != curr_depth);

	return 1;
}

static int parse_points(xmlTextReaderPtr reader, int ***ptsarrayadr, int *ptslenadr) {
	char *value;
	int i;

	if (!(value = (char*)xmlTextReaderGetAttribute(reader, "points"))) { /* points */
		tmx_err(E_MISSEL, "missing 'points' attribute in the 'object' element");
		return 0;
	}

	*ptslenadr = count_char_occurences(value, ',');

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

	for (i=0; i<*ptslenadr; i++) {
		if (sscanf(value, "%d,%d", (*ptsarrayadr)[i], (*ptsarrayadr)[i]+1) != 2) {
			tmx_free_func((*ptsarrayadr)[0]);
			tmx_free_func(*ptsarrayadr);
			return 0;
		}
	}

	tmx_free_func(value);
	return 1;
}

static int parse_object(xmlTextReaderPtr reader, tmx_object obj) {
	int curr_depth;
	const char *name;
	char *value;

	/* parses each attribute */
	if ((value = (char*)xmlTextReaderGetAttribute(reader, "x"))) { /* x */
		obj->x = atoi(value);
		tmx_free_func(value);
	} else {
		tmx_err(E_MISSEL, "missing 'x' attribute in the 'object' element");
		return 0;
	}

	if ((value = (char*)xmlTextReaderGetAttribute(reader, "y"))) { /* y */
		obj->y = atoi(value);
		tmx_free_func(value);
	} else {
		tmx_err(E_MISSEL, "missing 'y' attribute in the 'object' element");
		return 0;
	}

	if ((value = (char*)xmlTextReaderGetAttribute(reader, "name"))) { /* name */
		obj->name = value;
	}

	if ((value = (char*)xmlTextReaderGetAttribute(reader, "gid"))) { /* gid */
		obj->gid = atoi(value);
		tmx_free_func(value);
	}

	if ((value = (char*)xmlTextReaderGetAttribute(reader, "height"))) { /* height */
		obj->height = atoi(value);
		tmx_free_func(value);
	}

	if ((value = (char*)xmlTextReaderGetAttribute(reader, "width"))) { /* width */
		obj->width = atoi(value);
		tmx_free_func(value);
	}

	/* If it has a child, then it's a polygon or a polyline */
	if (xmlTextReaderIsEmptyElement(reader)) {
		obj->shape = S_SQUARE;
	} else {
		curr_depth = xmlTextReaderDepth(reader);
		do {
			if (xmlTextReaderRead(reader) != 1) return 0; /* error_handler has been called */
			
			if (xmlTextReaderNodeType(reader) == XML_READER_TYPE_ELEMENT) {
				name = (char*)xmlTextReaderConstName(reader);
				if (!strcmp(name, "polygon")) {
					obj->shape = S_POLYGON;
				} else if (!strcmp(name, "polyline")) {
					obj->shape = S_POLYLINE;
				} else { /* Unknow element, skipping it's tree */
					if (xmlTextReaderNext(reader) != 1) return 0;
				}
				if (!parse_points(reader, &(obj->points), &(obj->points_len))) return 0;
			}
		} while (xmlTextReaderNodeType(reader) != XML_READER_TYPE_END_ELEMENT ||
		         xmlTextReaderDepth(reader) != curr_depth);
	}
	return 1;
}

static int parse_objectgroup(xmlTextReaderPtr reader, tmx_layer *layers_headadr) {
	tmx_layer objgrp;
	tmx_object res;
	int curr_depth;
	const char *name;
	char *value;

	curr_depth = xmlTextReaderDepth(reader);

	if (!(objgrp = alloc_layer())) return 0;
	objgrp->type = L_OBJGR;
	objgrp->next = *layers_headadr;
	*layers_headadr = objgrp;

	/* parses each attribute */
	if ((value = (char*)xmlTextReaderGetAttribute(reader, "name"))) { /* name */
		objgrp->name = value;
	} else {
		tmx_err(E_MISSEL, "missing 'name' attribute in the 'objectgroup' element");
		return 0;
	}

	if ((value = (char*)xmlTextReaderGetAttribute(reader, "visible"))) { /* visible */
		objgrp->visible = value[0]=='t' ? 1: 0; /* if (visible=="true") visible <-- 1 */
		tmx_free_func(value);
	}

	if ((value = (char*)xmlTextReaderGetAttribute(reader, "color"))) { /* color */
		objgrp->color = get_color_rgb(value);
		tmx_free_func(value);
	}

	if ((value = (char*)xmlTextReaderGetAttribute(reader, "opacity"))) { /* opacity */
		objgrp->opacity = (float)strtod(value, NULL);
		tmx_free_func(value);
	}

	/* Parse each child */
	do {
		if (xmlTextReaderRead(reader) != 1) return 0;

		if (xmlTextReaderNodeType(reader) == XML_READER_TYPE_ELEMENT) {
			name = (char*)xmlTextReaderConstName(reader);
			if (!strcmp(name, "object")) {
				if (!(res = alloc_object())) return 0;
				
				res->next = objgrp->content.head;
				objgrp->content.head = res;

				if (!parse_object(reader, res)) return 0;

			} else { /* Unknow element, skipping it's tree */
				if (xmlTextReaderNext(reader) != 1) return 0;
			}
		}
	} while (xmlTextReaderNodeType(reader) != XML_READER_TYPE_END_ELEMENT ||
	         xmlTextReaderDepth(reader) != curr_depth);

	return 1;
}

static int parse_image(xmlTextReaderPtr reader, tmx_image *img_adr) {
	tmx_image res;
	char *value;

	if (!(res = alloc_image())) return 0;
	*img_adr = res;

	if ((value = (char*)xmlTextReaderGetAttribute(reader, "source"))) { /* source */
		res->source = value;
	} else {
		tmx_err(E_MISSEL, "missing 'source' attribute in the 'image' element");
		return 0;
	}

	if ((value = (char*)xmlTextReaderGetAttribute(reader, "height"))) { /* height */
		res->height = atoi(value);
		tmx_free_func(value);
	} else {
		tmx_err(E_MISSEL, "missing 'height' attribute in the 'image' element");
		return 0;
	}

	if ((value = (char*)xmlTextReaderGetAttribute(reader, "width"))) { /* width */
		res->width = atoi(value);
		tmx_free_func(value);
	} else {
		tmx_err(E_MISSEL, "missing 'width' attribute in the 'image' element");
		return 0;
	}

	if ((value = (char*)xmlTextReaderGetAttribute(reader, "trans"))) { /* trans */
		res->trans = get_color_rgb(value);
		tmx_free_func(value);
	}

	return 1;
}

static int parse_tileoffset(xmlTextReaderPtr reader, unsigned int *x, unsigned int *y) {
	char *value;
	if ((value = (char*)xmlTextReaderGetAttribute(reader, "x"))) { /* x offset */
		*x = atoi(value);
		tmx_free_func(value);
	} else {
		tmx_err(E_MISSEL, "missing 'x' attribute in the 'tileoffset' element");
		return 0;
	}

	if ((value = (char*)xmlTextReaderGetAttribute(reader, "y"))) { /* y offset */
		*y = atoi(value);
		tmx_free_func(value);
	} else {
		tmx_err(E_MISSEL, "missing 'y' attribute in the 'tileoffset' element");
		return 0;
	}

	return 1;
}

static int parse_tileset(xmlTextReaderPtr reader, tmx_tileset *ts_headadr) {
	tmx_tileset res = NULL;
	int curr_depth;
	const char *name;
	char *value;

	curr_depth = xmlTextReaderDepth(reader);

	if (!(res = alloc_tileset())) return 0;
	res->next = *ts_headadr;
	*ts_headadr = res;

	/* parses each attribute */
	if ((value = (char*)xmlTextReaderGetAttribute(reader, "name"))) { /* name */
		res->name = value;
	} else {
		tmx_err(E_MISSEL, "missing 'name' attribute in the 'tileset' element");
		return 0;
	}

	if ((value = (char*)xmlTextReaderGetAttribute(reader, "firstgid"))) { /* fisrtgid */
		res->firstgid = atoi(value);
		tmx_free_func(value);
	} else {
		tmx_err(E_MISSEL, "missing 'firstgid' attribute in the 'tileset' element");
		return 0;
	}

	if ((value = (char*)xmlTextReaderGetAttribute(reader, "tilewidth"))) { /* tile_width */
		res->tile_width = atoi(value);
		tmx_free_func(value);
	} else {
		tmx_err(E_MISSEL, "missing 'tilewidth' attribute in the 'tileset' element");
		return 0;
	}

	if ((value = (char*)xmlTextReaderGetAttribute(reader, "tileheight"))) { /* tile_height */
		res->tile_height = atoi(value);
		tmx_free_func(value);
	} else {
		tmx_err(E_MISSEL, "missing 'tileheight' attribute in the 'tileset' element");
		return 0;
	}

	if ((value = (char*)xmlTextReaderGetAttribute(reader, "spacing"))) { /* spacing */
		res->spacing = atoi(value);
		tmx_free_func(value);
	}

	if ((value = (char*)xmlTextReaderGetAttribute(reader, "margin"))) { /* margin */
		res->margin = atoi(value);
		tmx_free_func(value);
	}

	/* Parse each child */
	do {
		if (xmlTextReaderRead(reader) != 1) return 0; /* error_handler has been called */
		
		if (xmlTextReaderNodeType(reader) == XML_READER_TYPE_ELEMENT) {
			name = (char*)xmlTextReaderConstName(reader);
			if (!strcmp(name, "image")) {
				if (!parse_image(reader, &(res->image))) return 0;
			} else if (!strcmp(name, "tileoffset")) {
				if (!parse_tileoffset(reader, &(res->x_offset), &(res->y_offset))) return 0;
			} else {
				/* Unknow element, skipping it's tree */
				if (xmlTextReaderNext(reader) != 1) return 0;
			}
		}
	} while (xmlTextReaderNodeType(reader) != XML_READER_TYPE_END_ELEMENT ||
	         xmlTextReaderDepth(reader) != curr_depth);

	return 1;
}

static tmx_map parse_root_map(xmlTextReaderPtr reader) {
	tmx_map res = NULL;
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
	if ((value = (char*)xmlTextReaderGetAttribute(reader, "orientation"))) { /* orientation */
		if (res->orient = parse_orient(value), res->orient == O_NONE) {
			tmx_err(E_XDATA, "unsupported 'orientation' '%s'", value);
			goto cleanup;
		}
		tmx_free_func(value);
	} else {
		tmx_err(E_MISSEL, "missing 'orientation' attribute in the 'map' element");
		goto cleanup;
	}

	if ((value = (char*)xmlTextReaderGetAttribute(reader, "height"))) { /* height */
		res->height = atoi(value);
		tmx_free_func(value);
	} else {
		tmx_err(E_MISSEL, "missing 'height' attribute in the 'map' element");
		goto cleanup;
	}

	if ((value = (char*)xmlTextReaderGetAttribute(reader, "width"))) { /* width */
		res->width = atoi(value);
		tmx_free_func(value);
	} else {
		tmx_err(E_MISSEL, "missing 'width' attribute in the 'map' element");
		goto cleanup;
	}

	if ((value = (char*)xmlTextReaderGetAttribute(reader, "tileheight"))) { /* tileheight */
		res->tile_height = atoi(value);
		tmx_free_func(value);
	} else {
		tmx_err(E_MISSEL, "missing 'tileheight' attribute in the 'map' element");
		goto cleanup;
	}

	if ((value = (char*)xmlTextReaderGetAttribute(reader, "tilewidth"))) { /* tilewidth */
		res->tile_width = atoi(value);
		tmx_free_func(value);
	} else {
		tmx_err(E_MISSEL, "missing 'tilewidth' attribute in the 'map' element");
		goto cleanup;
	}

	/* Parse each child */
	do {
		if (xmlTextReaderRead(reader) != 1) goto cleanup; /* error_handler has been called */
		
		if (xmlTextReaderNodeType(reader) == XML_READER_TYPE_ELEMENT) {
			name = (char*)xmlTextReaderConstName(reader);
			if (!strcmp(name, "tileset")) {
				if (!parse_tileset(reader, &(res->ts_head))) goto cleanup;
			} else if (!strcmp(name, "layer")) {
				if (!parse_layer(reader, &(res->ly_head), res->height, res->width)) goto cleanup;
			} else if (!strcmp(name, "objectgroup")) {
				if (!parse_objectgroup(reader, &(res->ly_head))) goto cleanup;
			} else if (!strcmp(name, "properties")) {
				if (!parse_properties(reader, &(res->properties))) goto cleanup;
			} else {
				/* Unknow element, skipping it's tree */
				if (xmlTextReaderNext(reader) != 1) goto cleanup;
			}
		}
	} while (xmlTextReaderNodeType(reader) != XML_READER_TYPE_END_ELEMENT ||
	         xmlTextReaderDepth(reader) != curr_depth);
	return res;
cleanup:
	tmx_free(&res);
	return NULL;
}

static void error_handler(void *arg, const char *msg, xmlParserSeverities severity, xmlTextReaderLocatorPtr locator) {
	if (severity == XML_PARSER_SEVERITY_ERROR) { /* FIXME : use msg ? free msg ? */
		tmx_err(E_XDATA, "xml parser: error at line #%d", xmlTextReaderLocatorLineNumber(locator));
	}
}

tmx_map parse_xml(const char *filename) {
	xmlTextReaderPtr reader;
	tmx_map res = NULL;

	xmlMemSetup((xmlFreeFunc)tmx_free_func, (xmlMallocFunc)tmx_malloc, (xmlReallocFunc)tmx_alloc_func, (xmlStrdupFunc)tmx_strdup);

	if ((reader = xmlReaderForFile(filename, NULL, 0))) {

		xmlTextReaderSetErrorHandler(reader, error_handler, NULL);

		if (xmlTextReaderRead(reader) == 1) {
			res = parse_root_map(reader);
		}

		xmlFreeTextReader(reader);
	} else {
		tmx_err(E_UNKN, "xml parser: unable to open %s", filename);
	}
	xmlCleanupParser();
	return res;
}
