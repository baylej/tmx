#include <stdlib.h>
#include <stdio.h>
#include <tmx.h>

#define str_bool(b) (b==0? "false": "true")

void dump_prop(tmx_property *p, int depth) {
	int i; char padding[5];
	for (i=0; i<depth; i++) {
		padding[i] = '\t';
	}
	padding[depth] = '\0';

	printf("\n%sproperties={", padding);
	if (!p) {
		printf(" (NULL) }");
	} else {
		while (p) {
			printf("\n%s\t'%s'='%s'", padding, p->name, p->value);
			p = p->next;
		}
		printf("\n%s}", padding);
	}
}

void print_shape(enum tmx_shape shape) {
	switch(shape) {
		case S_NONE:     printf("none");     break;
		case S_SQUARE:   printf("square");   break;
		case S_ELLIPSE:  printf("ellipse");  break;
		case S_POLYGON:  printf("polygon");  break;
		case S_POLYLINE: printf("polyline"); break;
		default: printf("unknown");
	}
}

void dump_points(double **p, int pl) {
	int i;
	printf("\n\t\tpoints=");
	for (i=0; i<pl; i++) {
		printf(" (%lf, %lf)", p[i][0], p[i][1]);
	}
}

void dump_objects(tmx_object *o) {
	printf("\n\tobject={");
	if (!o) {
		printf(" (NULL) }");
	} else {
		printf("\n\t\tname='%s'", o->name);
		printf("\n\t\tshape=");  print_shape(o->shape);
		printf("\n\t\tx=%lf", o->x);
		printf("\n\t\ty=%lf", o->y);
		printf("\n\t\tnumber of points='%d'", o->points_len);
		printf("\n\t\tvisible=%s", str_bool(o->visible));
		if (o->points_len) dump_points(o->points, o->points_len);
		dump_prop(o->properties, 2);
		printf("\n\t}");
	}

	if (o && o->next) {
		dump_objects(o->next);
	}
}

void dump_image(tmx_image *i) {
	printf("\n\timage={");
	if (i) {
		printf("\n\t\tsource='%s'", i->source);
		printf("\n\t\theight=%lu", i->height);
		printf("\n\t\twidth=%lu", i->width);
		printf("\n\t\tuses_trans=%s", str_bool(i->uses_trans));
		printf("\n\t\ttrans=#%.6X", i->trans);
		printf("\n\t}");
	} else {
		printf(" (NULL) }");
	}
}

void dump_tile(tmx_tile *t) {
	printf("\n\ttile={");
	if (t) {
		printf("\n\t\tid=%u", t->id);
		//dump_image(t->image);
		dump_prop(t->properties, 2);
		printf("\n\t}");
	} else {
		printf(" (NULL) }");
	}

	if (t && t->next) {
		dump_tile(t->next);
	}
}

void dump_tileset(tmx_tileset *t) {
	printf("\ntileset={");
	if (t) {
		printf("\n\tname=%s", t->name);
		printf("\n\tfirstgid=%u", t->firstgid);
		printf("\n\ttile_height=%d", t->tile_height);
		printf("\n\ttile_width=%d", t->tile_width);
		printf("\n\tfirstgid=%d", t->firstgid);
		printf("\n\tmargin=%d", t->margin);
		printf("\n\tspacing=%d", t->spacing);
		printf("\n\tx_offset=%d", t->x_offset);
		printf("\n\ty_offset=%d", t->y_offset);
		dump_image(t->image);
		dump_tile(t->tiles);
		dump_prop(t->properties, 1);
		printf("\n}");
	} else {
		printf(" (NULL) }");
	}

	if (t && t->next) {
		dump_tileset(t->next);
	}
}

void dump_layer(tmx_layer *l, unsigned int tc) {
	unsigned int i;
	printf("\nlayer={");
	if (!l) {
		printf(" (NULL) }");
	} else {
		printf("\n\tname='%s'", l->name);
		printf("\n\tcolor=#%.6X", l->color);
		printf("\n\tvisible=%s", str_bool(l->visible));
		printf("\n\topacity='%f'", l->opacity);
		if (l->type == L_LAYER && l->content.gids) {
			printf("\n\ttype=Layer\n\ttiles=");
			for (i=0; i<tc; i++) {
				printf("%d,", l->content.gids[i] & TMX_FLIP_BITS_REMOVAL);
			}
		} else if (l->type == L_OBJGR) {
			printf("\n\ttype=ObjectGroup");
			dump_objects(l->content.head);
		} else if (l->type == L_IMAGE) {
			printf("\n\tx_offset=%d", l->x_offset);
			printf("\n\ty_offset=%d", l->y_offset);
			printf("\n\ttype=ImageLayer");
			dump_image(l->content.image);
		}
		dump_prop(l->properties, 1);
		printf("\n}");
	}

	if (l) {
		if (l->next) dump_layer(l->next, tc);
	}
}

void dump_map(tmx_map *m) {
	fputs("map={", stdout);
	if (m) {
		printf("\n\torient=%d", m->orient);
		printf("\n\trenderorder=%d", m->renderorder);
		printf("\n\theight=%d", m->height);
		printf("\n\twidth=%d", m->width);
		printf("\n\ttheight=%d", m->tile_height);
		printf("\n\ttwidth=%d", m->tile_width);
		printf("\n\tbgcol=#%.6X", m->backgroundcolor);
	} else {
		fputs("\n(NULL)", stdout);
	}
	puts("\n}");

	if (m) {
		dump_tileset(m->ts_head);
		dump_layer(m->ly_head, m->height * m->width);
		dump_prop(m->properties, 0);
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
	tmx_map *m;

	if (argc != 2) {
		fprintf(stderr, "usage: %s <map.(tmx|xml|json)>\n", argv[0]);
		return EXIT_FAILURE;
	}

	tmx_alloc_func = dbg_alloc; /* alloc/free dbg */
	tmx_free_func  = dbg_free;

	m = tmx_load(argv[1]);
	if (!m) tmx_perror("error");

	dump_map(m);
	tmx_map_free(m);

	printf("\n%d mem alloc not freed\n", mal_vs_free_count);

#ifdef PAUSE
	puts("press <Enter> to quit\n");
	getchar();
#endif

	return EXIT_SUCCESS;
}

