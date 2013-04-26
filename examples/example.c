#include <stdlib.h>
#include <stdio.h>
#include <tmx.h>

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

void dump_points(int **p, int pl) {
	int i;
	printf("\n\tpoints=");
	for (i=0; i<pl; i++) {
		printf(" (%d, %d)", p[i][0], p[i][1]);
	}
}

void dump_objects(tmx_object o) {
	printf("object={");
	if (!o) {
		fputs("\n(NULL)", stdout);
	} else {
		printf("\n\tname='%s'", o->name);
		printf("\n\tshape=");  print_shape(o->shape);
		printf("\n\t x ='%d'", o->x);
		printf("\n\t y ='%d'", o->y);
		printf("\n\tnumber of points='%d'", o->points_len);
		if (o->points_len) dump_points(o->points, o->points_len);
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
		if (t->properties) dump_prop(t->properties);
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
		} else if (l->type == L_IMAGE) {
			printf("\n\ttype=ImageLayer");
			printf("\n\tsource='%s'", l->content.image->source);
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
		printf("\n\tbgcol=%d", m->backgroundcolor);
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

	if (argc != 2) {
		fprintf(stderr, "usage: %s <map.(tmx|xml|json)>\n", argv[0]);
		return EXIT_FAILURE;
	}

	tmx_alloc_func = dbg_alloc; /* alloc/free dbg */
	tmx_free_func  = dbg_free;
	
	m = tmx_load(argv[1]);
	if (!m) tmx_perror("error");

	dump_map(m);
	tmx_free(&m);

	printf("%d mem alloc not freed\n", mal_vs_free_count);

	puts("press <Enter> to quit\n");
	getchar();
	return EXIT_SUCCESS;
}
