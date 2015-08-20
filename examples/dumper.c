#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <tmx.h>

#define str_bool(b) (b==0? "false": "true")

void print_orient(enum tmx_map_orient orient) {
	switch(orient) {
		case O_NONE: printf("none");  break;
		case O_ORT:  printf("ortho"); break;
		case O_ISO:  printf("isome"); break;
		case O_STA:  printf("stagg"); break;
		default: printf("unknown");
	}
}

void print_renderorder(enum tmx_map_renderorder ro) {
	switch(ro) {
		case R_NONE:      printf("none");      break;
		case R_RIGHTDOWN: printf("rightdown"); break;
		case R_RIGHTUP:   printf("rightup");   break;
		case R_LEFTDOWN:  printf("leftdown");  break;
		case R_LEFTUP:    printf("leftup");    break;
		default: printf("unknown");
	}
}

void print_draworder(enum tmx_objgr_draworder dro) {
	switch(dro) {
		case G_NONE:    printf("none");    break;
		case G_INDEX:   printf("index");   break;
		case G_TOPDOWN: printf("topdown"); break;
		default: printf("unknown");
	}
}

void mk_padding(char pad[11], int depth) {
	if (depth>10) depth=10;
	memset(pad, '\t', depth);
	pad[depth] = '\0';
}

void dump_prop(tmx_property *p, int depth) {
	char padding[11]; mk_padding(padding, depth);

	printf("\n%s" "properties={", padding);
	if (!p) {
		printf(" (NULL) }");
	} else {
		while (p) {
			printf("\n%s\t" "'%s'='%s'", padding, p->name, p->value);
			p = p->next;
		}
		printf("\n" "%s}", padding);
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
	for (i=0; i<pl; i++) {
		printf(" (%f, %f)", p[i][0], p[i][1]);
	}
}

void dump_objects(tmx_object *o, int depth) {
	char padding[11]; mk_padding(padding, depth);

	printf("\n%s" "object={", padding);
	if (!o) {
		printf(" (NULL) }");
	} else {
		printf("\n%s\t" "name='%s'", padding, o->name);
		printf("\n%s\t" "shape=", padding);  print_shape(o->shape);
		printf("\n%s\t" "x=%f", padding, o->x);
		printf("\n%s\t" "y=%f", padding, o->y);
		printf("\n%s\t" "number of points='%d'", padding, o->points_len);
		printf("\n%s\t" "rotation=%f", padding, o->rotation);
		printf("\n%s\t" "visible=%s", padding, str_bool(o->visible));
		if (o->points_len) {
			printf("\n%s\t" "points=", padding);
			dump_points(o->points, o->points_len);
		}
		dump_prop(o->properties, depth+1);
		printf("\n%s}", padding);
	}

	if (o && o->next) {
		dump_objects(o->next, depth);
	}
}

void dump_image(tmx_image *i, int depth) {
	char padding[11]; mk_padding(padding, depth);

	printf("\n%s" "image={", padding);
	if (i) {
		printf("\n%s\t" "source='%s'", padding, i->source);
		printf("\n%s\t" "height=%lu", padding, i->height);
		printf("\n%s\t" "width=%lu", padding, i->width);
		printf("\n%s\t" "uses_trans=%s", padding, str_bool(i->uses_trans));
		printf("\n%s\t" "trans=#%.6X", padding, i->trans);
		printf("\n%s}", padding);
	} else {
		printf(" (NULL) }");
	}
}

void dump_tile(tmx_tile *t) {
	printf("\n\t" "tile={");
	if (t) {
		printf("\n\t\t" "id=%u", t->id);
		dump_image(t->image, 2);
		dump_prop(t->properties, 2);
		dump_objects(t->collision, 2);
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
		printf("\n\t" "name=%s", t->name);
		printf("\n\t" "firstgid=%u", t->firstgid);
		printf("\n\t" "tile_height=%d", t->tile_height);
		printf("\n\t" "tile_width=%d", t->tile_width);
		printf("\n\t" "firstgid=%d", t->firstgid);
		printf("\n\t" "margin=%d", t->margin);
		printf("\n\t" "spacing=%d", t->spacing);
		printf("\n\t" "x_offset=%d", t->x_offset);
		printf("\n\t" "y_offset=%d", t->y_offset);
		dump_image(t->image, 1);
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
		printf("\n\t" "name='%s'", l->name);
		printf("\n\t" "visible=%s", str_bool(l->visible));
		printf("\n\t" "opacity='%f'", l->opacity);
		if (l->type == L_LAYER && l->content.gids) {
			printf("\n\t" "type=Layer" "\n\t" "tiles=");
			for (i=0; i<tc; i++) {
				printf("%d,", l->content.gids[i] & TMX_FLIP_BITS_REMOVAL);
			}
		} else if (l->type == L_OBJGR) {
			printf("\n\t" "color=#%.6X", l->content.objgr->color);
			printf("\n\t" "draworder="); print_draworder(l->content.objgr->draworder);
			printf("\n\t" "type=ObjectGroup");
			dump_objects(l->content.objgr->head, 1);
		} else if (l->type == L_IMAGE) {
			printf("\n\t" "x_offset=%d", l->x_offset);
			printf("\n\t" "y_offset=%d", l->y_offset);
			printf("\n\t" "type=ImageLayer");
			dump_image(l->content.image, 1);
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
		printf("\n\t" "orient="); print_orient(m->orient);
		printf("\n\t" "renderorder=%d", m->renderorder);
		printf("\n\t" "height=%d", m->height);
		printf("\n\t" "width=%d", m->width);
		printf("\n\t" "theight=%d", m->tile_height);
		printf("\n\t" "twidth=%d", m->tile_width);
		printf("\n\t" "bgcol=#%.6X", m->backgroundcolor);
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
		fprintf(stderr, "usage: %s <map.tmx>\n", argv[0]);
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

