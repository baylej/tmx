/*
	TMX usage example with Allegro 5
	uses image and primitives addons
*/
#include <stdio.h>
#include <tmx.h>
#include <allegro5/allegro.h>
#include <allegro5/allegro_image.h>
#include <allegro5/allegro_primitives.h>

#define LINE_THICKNESS 2.5

#define fatal_error(str)  { fputs(str, stderr); goto errquit; }
#define fatal_error2(str) { fputs(str, stderr); return NULL; }

ALLEGRO_COLOR int_to_al_color(int color) {
	unsigned char r, g, b;

	r = (color >> 16) & 0xFF;
	g = (color >>  8) & 0xFF;
	b = (color)       & 0xFF;

	return al_map_rgb(r, g, b);
}

void* al_img_loader(const char *path) {
	ALLEGRO_BITMAP *res    = NULL;
	ALLEGRO_PATH   *alpath = NULL;
	
	if (!(alpath = al_create_path(path))) return NULL;
	
	al_set_new_bitmap_format(ALLEGRO_PIXEL_FORMAT_ANY_WITH_ALPHA);
	res = al_load_bitmap(al_path_cstr(alpath, ALLEGRO_NATIVE_PATH_SEP));
	
	al_destroy_path(alpath);
	
	return (void*)res;
}

/*
	Draw objects
*/
void draw_polyline(int **points, int x, int y, int pointsc, ALLEGRO_COLOR color) {
	int i;
	for (i=1; i<pointsc; i++) {
		al_draw_line(x+points[i-1][0], y+points[i-1][1], x+points[i][0], y+points[i][1], color, LINE_THICKNESS);
	}
}

void draw_polygone(int **points, int x, int y, int pointsc, ALLEGRO_COLOR color) {
	draw_polyline(points, x, y, pointsc, color);
	if (pointsc > 2) {
		al_draw_line(x+points[0][0], y+points[0][1], x+points[pointsc-1][0], y+points[pointsc-1][1], color, LINE_THICKNESS);
	}
}

void draw_objects(tmx_object head, ALLEGRO_COLOR color) {
	while (head) {
		if (head->visible) {
			if (head->shape == S_SQUARE) {
				al_draw_rectangle(head->x, head->y, head->x+head->width, head->y+head->height, color, LINE_THICKNESS);
			} else if (head->shape  == S_POLYGON) {
				draw_polygone(head->points, head->x, head->y, head->points_len, color);
			} else if (head->shape == S_POLYLINE) {
				draw_polyline(head->points, head->x, head->y, head->points_len, color);
			} else if (head->shape == S_ELLIPSE) {
				al_draw_ellipse(head->x + head->width/2.0, head->y + head->height/2.0, head->width/2.0, head->height/2.0, color, LINE_THICKNESS);
			}
		}
		head = head->next;
	}
}

/*
	Draw tiled layers
*/
int gid_extract_flags(unsigned int gid) {
	int res = 0;
	
	if (gid & TMX_FLIPPED_HORIZONTALLY) res |= ALLEGRO_FLIP_HORIZONTAL;
	if (gid & TMX_FLIPPED_VERTICALLY)   res |= ALLEGRO_FLIP_VERTICAL;
	/* FIXME allegro has no diagonal flip */
	return res;
}

int gid_clear_flags(unsigned int gid) {
	return gid & TMX_FLIP_BITS_REMOVAL;
}

/* returns the bitmap and the region associated with this gid, returns -1 if tile not found */
short get_bitmap_region(unsigned int gid, tmx_tileset ts, ALLEGRO_BITMAP **ts_bmp, unsigned int *x, unsigned int *y, unsigned int *w, unsigned int *h) {
	unsigned int tiles_x_count;
	unsigned int ts_w, id, tx, ty;
	gid = gid_clear_flags(gid);
	
	while (ts) {
		if (ts->firstgid <= gid) {
			if (!ts->next || ts->next->firstgid < ts->firstgid || ts->next->firstgid > gid) {
				id = gid - ts->firstgid; /* local id (for this image) */
				
				ts_w = ts->image->width  - 2 * (ts->margin) + ts->spacing;
				
				tiles_x_count = ts_w / (ts->tile_width  + ts->spacing);
				
				*ts_bmp = (ALLEGRO_BITMAP*)ts->image->resource_image;
				
				*w = ts->tile_width;  /* set bitmap's region width  */
				*h = ts->tile_height; /* set bitmap's region height */
				
				tx = id % tiles_x_count;
				ty = id / tiles_x_count;
				
				*x = ts->margin + (tx * ts->tile_width)  + (tx * ts->spacing); /* set bitmap's region */
				*y = ts->margin + (ty * ts->tile_height) + (ty * ts->spacing); /* x and y coordinates */
				return 0;
			}
		}
		ts = ts->next;
	}
	
	return -1;
}

void draw_layer(tmx_layer layer, tmx_tileset ts, unsigned int width, unsigned int height, unsigned int tile_width, unsigned int tile_height) {
	unsigned long i, j;
	unsigned int x, y, w, h, flags;
	float op;
	ALLEGRO_BITMAP *tileset;
	op = layer->opacity;
	for (i=0; i<height; i++) {
		for (j=0; j<width; j++) {
			if (!get_bitmap_region(layer->content.gids[(i*width)+j], ts, &tileset, &x, &y, &w, &h)) {
				flags = gid_extract_flags(layer->content.gids[(i*width)+j]);
				al_draw_tinted_bitmap_region(tileset, al_map_rgba_f(op, op, op, op), x, y, w, h, j*tile_width, i*tile_height, flags);
			}
		}
	}
}

/*
	Render map
*/
ALLEGRO_BITMAP* render_map(tmx_map map) {
	ALLEGRO_BITMAP *res = NULL;
	tmx_layer layers = map->ly_head;
	unsigned long w, h;
	
	if (map->orient != O_ORT) fatal_error("only orthogonal orient currently supported in this example!");
	
	w = map->width  * map->tile_width;  /* Bitmap's width and height */
	h = map->height * map->tile_height;
	if (!(res = al_create_bitmap(w, h))) fatal_error("failed to create bitmap!");
	
	al_set_target_bitmap(res);
	
	al_clear_to_color(int_to_al_color(map->backgroundcolor));
	
	while (layers) {
		if (layers->visible) {
			if (layers->type == L_OBJGR) {
				draw_objects(layers->content.head, int_to_al_color(layers->color));
			} else if (layers->type == L_IMAGE) {
				if (layers->opacity < 1.) {
					float op = layers->opacity;
					al_draw_tinted_bitmap((ALLEGRO_BITMAP*)layers->content.image->resource_image, al_map_rgba_f(op, op, op, op), 0, 0, 0);
				}
				al_draw_bitmap((ALLEGRO_BITMAP*)layers->content.image->resource_image, 0, 0, 0);
			} else if (layers->type == L_LAYER) {
				draw_layer(layers, map->ts_head, map->width, map->height, map->tile_width, map->tile_height);
			}
		}
		layers = layers->next;
	}
	
	al_set_target_backbuffer(al_get_current_display());
	
	return res;
	
errquit:
	al_destroy_bitmap(res);
	return NULL;
}

/*
	MAIN
	Creates a display
	Loads the map and map's resources
	Renders the map
*/
int main(int argc, char **argv) {
	ALLEGRO_DISPLAY *display = NULL;
	ALLEGRO_TIMER *timer = NULL;
	ALLEGRO_EVENT_QUEUE *equeue = NULL;
	ALLEGRO_EVENT ev;
	tmx_map map = NULL;
	ALLEGRO_BITMAP *bmp_map = NULL;

	if (argc != 2) fatal_error("This program expects 1 argument");

	if (!al_init())	fatal_error("failed to initialize allegro!");

	display = al_create_display(640, 480);
	if (!display) fatal_error("failed to create display!");
	al_set_window_title(display, "Allegro Game");
	
	if (!al_init_image_addon()) fatal_error("failed to initialise ImageIO!");
	if (!al_init_primitives_addon()) fatal_error("failed to initialise Primitives!");
	if (!al_install_keyboard()) fatal_error("failed to install keyboard!");
	
	rsc_img_load_func = al_img_loader;
	rsc_img_free_func = (void (*)(void*))al_destroy_bitmap;

	/* Load and render the map */
	if (!(map = tmx_load(argv[1]))) fatal_error(tmx_strerr());
	if (!(bmp_map = render_map(map))) return -1;
	al_resize_display(display, map->width  * map->tile_width, map->height * map->tile_height); /* DELME */

	equeue = al_create_event_queue();
	if (!equeue) fatal_error("failed to create event queue!");

	timer = al_create_timer(1.0/30.0);
	if (!timer) fatal_error("failed to create timer!");

	al_register_event_source(equeue, al_get_display_event_source(display));
	al_register_event_source(equeue, al_get_timer_event_source(timer));
	al_register_event_source(equeue, al_get_keyboard_event_source());
 
	al_start_timer(timer);

	while(al_wait_for_event(equeue, &ev), 1) {
		if (ev.type == ALLEGRO_EVENT_DISPLAY_CLOSE) break;
		if (ev.type == ALLEGRO_EVENT_TIMER) {
			if (bmp_map) { /* TODO */
				al_draw_bitmap(bmp_map, 0, 0, 0);
			}
			al_flip_display();
		} else if (ev.type == ALLEGRO_EVENT_KEY_UP) {
			if (ev.keyboard.keycode == ALLEGRO_KEY_Q) break;
			switch (ev.keyboard.keycode) {
				case ALLEGRO_KEY_LEFT: break; /* FIXME */
				case ALLEGRO_KEY_RIGHT: break;
				case ALLEGRO_KEY_UP: break;
				case ALLEGRO_KEY_DOWN: break;
			}
		}
	}
	
	al_destroy_timer(timer);
	al_destroy_event_queue(equeue);
	al_destroy_display(display);
 
	return 0;

errquit:
	return -1;
}
