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
	return NULL;
}
#endif
#ifndef WANT_JSON
tmx_map parse_json(const char *filename) {
	tmx_err(E_FONCT, "This library was not builded with the JSON parser");
	return NULL;
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
		map = parse_json(path);
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

static void free_props(tmx_property p) {
	if (p) {
		free_props(p->next);
		tmx_free_func(p->name);
		tmx_free_func(p->value);
		tmx_free_func(p);
	}
}

static void free_obj(tmx_object o) {
	if (o) {
		free_obj(o->next);
		tmx_free_func(o->name);
		if (o->points) tmx_free_func(*(o->points));
		tmx_free_func(o->points);
		tmx_free_func(o);
	}
}

static void free_layers(tmx_layer l) {
	if (l) {
		free_layers(l->next);
		tmx_free_func(l->name);
		if (l->type == L_LAYER)
			tmx_free_func(l->content.gids);
		else if (l->type == L_OBJGR)
			free_obj(l->content.head);
		free_props(l->properties);
		tmx_free_func(l);
	}
}

static void free_ts(tmx_tileset ts) {
	if (ts) {
		free_ts(ts->next);
		tmx_free_func(ts->name);
		if (ts->image) tmx_free_func(ts->image->source);
		tmx_free_func(ts->image);
		tmx_free_func(ts);
	}
}

void tmx_free(tmx_map *map) {
	if (*map) {
		free_ts((*map)->ts_head);
		free_props((*map)->properties);
		free_layers((*map)->ly_head);
		tmx_free_func(*map);
		*map = NULL;
	}
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

	if (o)
		if (o->next) dump_objects(o->next);
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
	printf("tileset={");
	if (t) {
		printf("\n\tname=%s", t->name);
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
		if (t->image) dump_image(t->image);
		if (t->next) dump_tileset(t->next);
	}
}

void dump_layer(tmx_layer l, unsigned int tc) {
	unsigned int i;
	printf("layer={");
	if (!l) {
		fputs("\n(NULL)", stdout);
	} else {
		printf("\n\tname='%s'", l->name);
		printf("\n\tvisible='%d'", l->visible);
		printf("\n\topacity='%f'", l->opacity);
		if (l->type == L_LAYER && l->content.gids) {
			printf("\n\ttype=Layer\n\ttiles=");
			for (i=0; i<tc; i++)
				printf("%d,", l->content.gids[i] & TMX_FLIP_BITS_REMOVAL);
		} else if (l->type == L_OBJGR) {
			printf("\n\ttype=ObjectGroup");
		}
	}
	puts("\n}");

	if (l) {
		if (l->type == L_OBJGR && l->content.head) dump_objects(l->content.head);
		if (l->properties) dump_prop(l->properties);
		if (l->next) dump_layer(l->next, tc);
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
		dump_layer(m->ly_head, m->height * m->width);
	}
}

static int mal_vs_free_count = 0;

void* dbg_alloc(void *address, size_t len) {
	if (!address) mal_vs_free_count++; /* !realloc */
	return realloc(address, len);
}

void dbg_free(void *address) {
	if (address) mal_vs_free_count--;
	free(address);
}

int main(int argc, char *argv[]) {
	tmx_map m;

	tmx_alloc_func = dbg_alloc; /* alloc/free dbg */
	tmx_free_func  = dbg_free;

	m = tmx_load("test.tmx");
	//if (!m) tmx_perror("parse_json(test.json)");
	if (!m) tmx_perror("parse_xml(test.tmx)");
	dump_map(m);
	tmx_free(&m);

	printf("%d mem alloc not freed\n", mal_vs_free_count);

	getchar();
	return 0;
}

#endif