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
		case O_HEX:  printf("hexag"); break;
		default: printf("unknown");
	}
}

void print_stagger_index(enum tmx_stagger_index index) {
	switch(index) {
		case SI_NONE: printf("none"); break;
		case SI_EVEN: printf("even"); break;
		case SI_ODD:  printf("odd");  break;
		default: printf("unknown");
	}
}

void print_stagger_axis(enum tmx_stagger_axis axis) {
	switch(axis) {
		case SA_NONE: printf("none"); break;
		case SA_X:    printf("x");    break;
		case SA_Y:    printf("y");    break;
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
	if (depth>0) memset(pad, '\t', depth);
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
		printf("\n%s\t" "id=%u", padding, o->id);
		printf("\n%s\t" "name='%s'", padding, o->name);
		printf("\n%s\t" "type='%s'", padding, o->type);
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

void dump_tile(tmx_tile *t, unsigned int tilecount) {
	unsigned int i, j;
	for (i=0; i<tilecount; i++) {
		printf("\n\t" "tile={");
		printf("\n\t\t" "id=%u", t[i].id);
		printf("\n\t\t" "upper-left=(%u,%u)", t[i].ul_x, t[i].ul_y);
		dump_image(t[i].image, 2);
		dump_prop(t[i].properties, 2);
		dump_objects(t[i].collision, 2);

		if (t[i].animation) {
			printf("\n\t\t" "animation={");
			for (j=0; j<t[i].animation_len; j++) {
				printf("\n\t\t\t" "tile=%3u (%6ums)", t[i].animation[j].tile_id, t[i].animation[j].duration);
			}
			printf("\n\t\t}");
		}

		printf("\n\t}");
	}
}

void dump_tileset(tmx_tileset *t) {
	printf("\ntileset={");
	if (t) {
		printf("\n\t" "name=%s", t->name);
		printf("\n\t" "tilecount=%u", t->tilecount);
		printf("\n\t" "firstgid=%u", t->firstgid);
		printf("\n\t" "tile_height=%u", t->tile_height);
		printf("\n\t" "tile_width=%u", t->tile_width);
		printf("\n\t" "firstgid=%u", t->firstgid);
		printf("\n\t" "margin=%u", t->margin);
		printf("\n\t" "spacing=%u", t->spacing);
		printf("\n\t" "x_offset=%d", t->x_offset);
		printf("\n\t" "y_offset=%d", t->y_offset);
		dump_image(t->image, 1);
		dump_tile(t->tiles, t->tilecount);
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
		printf("\n\t" "offsetx=%d", l->offsetx);
		printf("\n\t" "offsety=%d", l->offsety);
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
		printf("\n\t" "height=%u", m->height);
		printf("\n\t" "width=%u", m->width);
		printf("\n\t" "theight=%u", m->tile_height);
		printf("\n\t" "twidth=%u", m->tile_width);
		printf("\n\t" "bgcol=#%.6X", m->backgroundcolor);
		printf("\n\t" "staggerindex="); print_stagger_index(m->stagger_index);
		printf("\n\t" "staggeraxis="); print_stagger_axis(m->stagger_axis);
		printf("\n\t" "hexsidelength=%d", m->hexsidelength);
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

