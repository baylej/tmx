/* FIXME let the user choose if he links staticaly */
#define LIBXML_STATIC

/*
	XML Parser using the XMLReader API because maps are huge
	see http://www.xmlsoft.org/xmlreader.html
	see http://www.xmlsoft.org/examples/index.html#reader1.c
*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <libxml/xmlreader.h>
#include <libxml/debugXML.h>

#include "tmx.h"
#include "tmx_utils.h"

static const char *known_nodes[] = 
{
	"tileset", "tileoffset", "image",
	"layer", "data",
	"imagelayer",
	"properties", "property",
	"objectgroup", "object", "polygon", "polyline", "ellipse",
	"terraintypes", "terrain", "tile",
NULL};
;

static void parse_layer(xmlTextReaderPtr reader) {

}

static void parse_objectgroup(xmlTextReaderPtr reader) {

}

static void parse_properties(xmlTextReaderPtr reader) {

}

static int parse_image(xmlTextReaderPtr reader, tmx_image *img_adr) {
	tmx_image res;
	char *value;

	if (!(res = alloc_image())) {
		tmx_errno = E_ALLOC;
		return 0;
	}

	if ((value = (char*)xmlTextReaderGetAttribute(reader, "source"))) { /* x offset */
		(*img_adr)->source = value;
	} else {
		tmx_err(E_MISSEL, "missing 'source' attribute in the 'image' element");
		goto cleanup;
	}

	if ((value = (char*)xmlTextReaderGetAttribute(reader, "height"))) { /* height */
		(*img_adr)->height = atoi(value);
		free(value);
	} else {
		tmx_err(E_MISSEL, "missing 'height' attribute in the 'image' element");
		goto cleanup;
	}

	if ((value = (char*)xmlTextReaderGetAttribute(reader, "width"))) { /* width */
		(*img_adr)->width = atoi(value);
		free(value);
	} else {
		tmx_err(E_MISSEL, "missing 'width' attribute in the 'image' element");
		goto cleanup;
	}

	*img_adr = res;
	return 1;

cleanup:
	free(res);
	return 0;
}

static int parse_tileoffset(xmlTextReaderPtr reader, unsigned int *x, unsigned int *y) {
	char *value;
	if ((value = (char*)xmlTextReaderGetAttribute(reader, "x"))) { /* x offset */
		*x = atoi(value);
		free(value);
	} else {
		tmx_err(E_MISSEL, "missing 'x' attribute in the 'tileoffset' element");
		return 0;
	}

	if ((value = (char*)xmlTextReaderGetAttribute(reader, "y"))) { /* y offset */
		*y = atoi(value);
		free(value);
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

	if (!(res = alloc_tileset())) {
		tmx_errno = E_ALLOC;
		return 0;
	}

	/* parses each attribute */
	if ((value = (char*)xmlTextReaderGetAttribute(reader, "name"))) { /* name */
		res->name = value;
	} else {
		tmx_err(E_MISSEL, "missing 'name' attribute in the 'tileset' element");
		goto cleanup;
	}

	if ((value = (char*)xmlTextReaderGetAttribute(reader, "firstgid"))) { /* fisrtgid */
		res->firstgid = atoi(value);
		free(value);
	} else {
		tmx_err(E_MISSEL, "missing 'firstgid' attribute in the 'tileset' element");
		goto cleanup;
	}

	if ((value = (char*)xmlTextReaderGetAttribute(reader, "tilewidth"))) { /* tile_width */
		res->tile_width = atoi(value);
		free(value);
	} else {
		tmx_err(E_MISSEL, "missing 'tilewidth' attribute in the 'tileset' element");
		goto cleanup;
	}

	if ((value = (char*)xmlTextReaderGetAttribute(reader, "tileheight"))) { /* tile_height */
		res->tile_height = atoi(value);
		free(value);
	} else {
		tmx_err(E_MISSEL, "missing 'tileheight' attribute in the 'tileset' element");
		goto cleanup;
	}

	if ((value = (char*)xmlTextReaderGetAttribute(reader, "spacing"))) { /* spacing */
		res->spacing = atoi(value);
		free(value);
	}

	if ((value = (char*)xmlTextReaderGetAttribute(reader, "margin"))) { /* margin */
		res->margin = atoi(value);
		free(value);
	}

	/* Parse each child */
	do {
		if (xmlTextReaderRead(reader) != 1) {
			goto cleanup; /* error_handler has been called */
		}
		if (xmlTextReaderNodeType(reader) == XML_READER_TYPE_ELEMENT) {
			name = (char*)xmlTextReaderConstName(reader);
			if (!strcmp(name, "image")) {
				/* TODO */
			} else if (!strcmp(name, "tileoffset")) {
				if (!parse_tileoffset(reader, &(res->x_offset), &(res->y_offset))) {
					goto cleanup;
				}
			} else {
				/* Unknow element, skipping it's tree */
				if (xmlTextReaderNext(reader) != 1) {
					goto cleanup;
				}
			}
		}
	} while (xmlTextReaderNodeType(reader) != XML_READER_TYPE_END_ELEMENT &&
	         xmlTextReaderDepth(reader) != curr_depth);
	if ((*ts_headadr)) { /* Insert the new element */
		res->next = *ts_headadr;
	}
	*ts_headadr = res;
	return 1;

cleanup:
	free(res);
	return 0;
}

static tmx_map parse_root_map(xmlTextReaderPtr reader) {
	tmx_map res = NULL;
	int curr_depth, ret;
	const char *name;
	char *value;
	
	name = (char*) xmlTextReaderConstName(reader);
	curr_depth = xmlTextReaderDepth(reader);

	if (strcmp(name, "map")) {
		tmx_err(E_XDATA, "xml parser: root is not a 'map' element");
		return NULL;
	}

	if (!(res = alloc_map())) {
		tmx_errno = E_ALLOC;
		return NULL;
	}

	/* parses each attribute */
	if ((value = (char*)xmlTextReaderGetAttribute(reader, "orientation"))) { /* orientation */
		tmx_err(E_XDATA, "unsupported 'orientation' '%'", value);
		if (ret = parse_orient(value), ret == -1) {
			goto cleanup;
		}
		res->orient = (enum tmx_map_orient)ret;
		free(value);
	} else {
		tmx_err(E_MISSEL, "missing 'orientation' attribute in the 'map' element");
		goto cleanup;
	}

	if ((value = (char*)xmlTextReaderGetAttribute(reader, "height"))) { /* height */
		res->height = atoi(value);
		free(value);
	} else {
		tmx_err(E_MISSEL, "missing 'height' attribute in the 'map' element");
		goto cleanup;
	}

	if ((value = (char*)xmlTextReaderGetAttribute(reader, "width"))) { /* width */
		res->width = atoi(value);
		free(value);
	} else {
		tmx_err(E_MISSEL, "missing 'width' attribute in the 'map' element");
		goto cleanup;
	}

	if ((value = (char*)xmlTextReaderGetAttribute(reader, "tileheight"))) { /* tileheight */
		res->tile_height = atoi(value);
		free(value);
	} else {
		tmx_err(E_MISSEL, "missing 'tileheight' attribute in the 'map' element");
		goto cleanup;
	}

	if ((value = (char*)xmlTextReaderGetAttribute(reader, "tilewidth"))) { /* tilewidth */
		res->tile_width = atoi(value);
		free(value);
	} else {
		tmx_err(E_MISSEL, "missing 'tilewidth' attribute in the 'map' element");
		goto cleanup;
	}

	/* Parse each child */
	do {
		if (xmlTextReaderRead(reader) != 1) {
			goto cleanup; /* error_handler has been called */
		}
		if (xmlTextReaderNodeType(reader) == XML_READER_TYPE_ELEMENT) {
			name = (char*)xmlTextReaderConstName(reader);
			if (!strcmp(name, "tileset")) {
				if (!parse_tileset(reader, &(res->ts_head))) goto cleanup;
			} else if (!strcmp(name, "layer")) {
				/* TODO */
			} else if (!strcmp(name, "objectgroup")) {
				/* TODO */
			} else if (!strcmp(name, "properties")) {
				/* TODO */
			} else {
				/* Unknow element, skipping it's tree */
				if (xmlTextReaderNext(reader) != 1) {
					goto cleanup;
				}
			}
		}
	} while (xmlTextReaderNodeType(reader) != XML_READER_TYPE_END_ELEMENT &&
	         xmlTextReaderDepth(reader) != curr_depth);
	return res;
cleanup:
	tmx_free(&res);
	return NULL;
}

void error_handler(void *arg, const char *msg, xmlParserSeverities severity, xmlTextReaderLocatorPtr locator) {
	if (severity == XML_PARSER_SEVERITY_ERROR) { /* FIXME : use msg ? */
		tmx_err(E_XDATA, "xml parser: error at line #%d", xmlTextReaderLocatorLineNumber(locator));
	}
}

tmx_map parse_xml(const char *filename) {
	xmlTextReaderPtr reader;
	tmx_map res = NULL;

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

#ifdef DEBUG

void dump_tileset(tmx_tileset t) {
	printf("tileset(%s)={", t->name);
	if (t) {
		printf("\n\ttile_height=%d", t->tile_height);
		printf("\n\ttile_width=%d", t->tile_width);
		printf("\n\tfirstgid=%d", t->firstgid);
		printf("\n\tmargin=%d", t->margin);
		printf("\n\tspacing=%d", t->spacing);
	} else {
		fputs("\n(NULL)", stdout);
	}
	puts("\n}");
}

void dump_map(tmx_map m) {
	fputs("map={", stdout);
	if (m) {
		printf("\n\torient=%d", m->orient);
		printf("\n\theight=%d", m->height);
		printf("\n\twidth=%d", m->width);
		printf("\n\ttheight=%d", m->tile_height);
		printf("\n\ttwidth=%d", m->tile_width);
	} else {
		fputs("\n(NULL)", stdout);
	}
	puts("\n}");

	if (m) {
		dump_tileset(m->ts_head);
	}
}

int main(int argc, char *argv[]) {
	tmx_map m;
	m = parse_xml("test.tmx");
	if (!m) tmx_perror("wtf ?");
	dump_map(m);
	tmx_free(&m);
	getchar();
	return 0;
}

#endif