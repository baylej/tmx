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

/*
	Public functions
*/
#ifndef WANT_XML
static tmx_map parse_xml(const char *path) {
	tmx_err(E_FONCT, "This library was not builded with the XML parser");
}
#endif

tmx_map tmx_load(const char * path) {
	tmx_map map = NULL;
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
		/* TODO */
	} else {
		/* open the file and check with the first character */
		if ((file = fopen(path, "r"))) {
			fchar = fgetc(file);
			fclose(file);
			if (fchar == '<') {
				map = parse_xml(path);
			} else if (fchar == '{') {
				/* TODO */
			} else {
				tmx_errno = E_FORMAT;
			}
		} else {
			/* TODO error access or file not found ? */
			if (errno == EACCES) {
				tmx_errno = E_ACCESS;
			} else if (errno == ENOENT) {
				tmx_errno = E_NOENT;
			} else {
				tmx_err(E_UNKN, strerror(errno));
			}
		}
	}

	return map;
}

static void free_obj(tmx_object o) {
	if (o->next) free_obj(o->next);
	tmx_free_func(o->name);
	if (o->points) tmx_free_func(*(o->points));
	tmx_free_func(o->points);
	tmx_free_func(o);
}

static void free_objgrp(tmx_objectgroup o) {
	if (o->head) free_obj(o->head);
	tmx_free_func(o->name);
	tmx_free_func(o);
}

static void free_props(tmx_property p) {
	if (p->next) free_props(p->next);
	tmx_free_func(p->name);
	tmx_free_func(p->value);
	tmx_free_func(p);
}

static void free_ts(tmx_tileset ts) {
	if (ts->next) free_ts(ts->next);
	tmx_free_func(ts->name);
	if (ts->image) {
		tmx_free_func(ts->image->source);
		tmx_free_func(ts->image);
	}
	tmx_free_func(ts);
}

void tmx_free(tmx_map *map) {
	/* TODO */
	if ((*map)->ts_head)
		free_ts((*map)->ts_head);
	if ((*map)->ob_head)
		free_objgrp((*map)->ob_head);
	if ((*map)->properties)
		free_props((*map)->properties);
	tmx_free_func(*map);
	*map = NULL;
}

#ifdef DEBUG

void dump_objects(tmx_object o) {
	printf("object={");
	if (!o) {
		fputs("\n(NULL)", stdout);
	} else {
		printf("\n\tname='%s'", o->name);
		printf("\n\t x ='%d'", o->x);
		printf("\n\t y ='%d'", o->y);
		printf("\n\tnumber of points='%d'", o->points_len);
	}
	puts("\n}");

	if (o->next) dump_objects(o->next);
}

void dump_objgrps(tmx_objectgroup o) {
	printf("objectgroup={");
	if (!o) {
		fputs("\n(NULL)", stdout);
	} else {
		printf("\n\tname='%s'", o->name);
		printf("\n\tcolor='0x%x'", o->color);
		printf("\n\tvisible='%d'", o->visible);
		printf("\n\topacity='%f'", o->opacity);
	}
	puts("\n}");

	if (o->head) dump_objects(o->head);
	if (o->next) dump_objgrps(o->next);
}

void dump_prop(tmx_property p) {
	printf("properties={");
	if (!p) {
		fputs("\n(NULL)", stdout);
	} else {
		while (p) {
			printf("\n\tname='%s'", p->name);
			printf("\t\tvalue='%s'", p->value);
			p = p->next;
		}
	}
	puts("\n}");
}

void dump_image(tmx_image i) {
	printf("image={");
	if (i) {
		printf("\n\tsource='%s'", i->source);
		printf("\n\theight=%d", i->height);
		printf("\n\twidth=%d", i->width);
	} else {
		fputs("\n(NULL)", stdout);
	}
	puts("\n}");
}

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

	if (t) {
		dump_image(t->image);
	}
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
		dump_prop(m->properties);
		dump_objgrps(m->ob_head);
	}
}

int main(int argc, char *argv[]) {
	tmx_map m;
	m = tmx_load("test.tmx");
	if (!m) tmx_perror("parse_xml(text.xml)");
	dump_map(m);
	tmx_free(&m);
	getchar();
	return 0;
}

#endif