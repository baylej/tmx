/**
 * TMX usage example with Allegro 5
 * uses image and primitives addons
 */
#include <stdio.h>
#include <tmx/tmx.h>
#include <allegro5/allegro.h>
#include <allegro5/allegro_image.h>
#include <allegro5/allegro_primitives.h>

#define LINE_THICKNESS 2.5

#define fatal_error(str)  { fputs(str, stderr); goto errquit; }
#define fatal_error2(str) { fputs(str, stderr); return NULL; }

tmx_map load_map_and_images(const char *file) {
	tmx_map res = NULL;
	tmx_tileset tilesets = NULL;
	tmx_layer layers = NULL;
	ALLEGRO_PATH *head, *tail = NULL;
	
	rsc_img_free_func = (void(*)(void*))al_destroy_bitmap; /* set TMX resource image free function */
	
	/* Load the map */
	if (!(head = al_create_path(file))) fatal_error2("failed to create a path");
	res = tmx_load(al_path_cstr(head, ALLEGRO_NATIVE_PATH_SEP));
	if (!res) {
		tmx_perror("failed to open the map!");
		return NULL;
	}
	
	/* Load tilesets images (currently not supporting .tsx files) */
	al_set_path_filename(head, NULL);
	tilesets = res->ts_head;
	while (tilesets) {
		if (tail) al_destroy_path(tail);
		if (!(tail = al_create_path(tilesets->image->source))) fatal_error("failed to create a path");
		if (!(al_rebase_path(head, tail))) {
			fputs("it seems the tileset image path is invalid!", stderr);
		} else {
			tilesets->image->resource_image = (void*) al_load_bitmap(al_path_cstr(tail, ALLEGRO_NATIVE_PATH_SEP));
			if (!(tilesets->image->resource_image)) fatal_error("failed to load image!");
		}
		tilesets = tilesets->next;
	}

	/* Load layers images */
	layers = res->ly_head;
	while (layers) {
		if (layers->type == L_IMAGE) {
			if (tail) al_destroy_path(tail);
			if (!(tail = al_create_path(layers->content.image->source))) fatal_error("failed to create a path");
			if (!(al_rebase_path(head, tail))) {
				fputs("it seems the tileset image path is invalid!", stderr);
			} else {
				layers->content.image->resource_image = (void*) al_load_bitmap(al_path_cstr(tail, ALLEGRO_NATIVE_PATH_SEP));
				if (!(layers->content.image->resource_image)) fatal_error("failed to load image!");
			}
		}
		layers = layers->next;
	}
	
	if (tail) al_destroy_path(tail);
	al_destroy_path(head);
	return res;
errquit:
	if (tail) al_destroy_path(tail);
	al_destroy_path(head);
	tmx_free(&res);
	return NULL;
}

ALLEGRO_COLOR int_to_al_color(int color) {
	unsigned char r, g, b;

	r = (color >> 16) & 0xFF;
	g = (color >>  8) & 0xFF;
	b = (color)       & 0xFF;

	return al_map_rgb(r, g, b);
}

void draw_objects(tmx_object head, ALLEGRO_COLOR color) {
	while (head) {
		if (head->visible) {
			if (head->shape == S_SQUARE) {
				al_draw_rectangle(head->x, head->y, head->x+head->width, head->y+head->height, color, LINE_THICKNESS);
			} else if (head->shape  == S_POLYGON) {
				/* TODO al_draw_polyline */
			} else if (head->shape == S_POLYLINE) {
				/* TODO al_draw_polygon */
			} else if (head->shape == S_ELLIPSE) {
				al_draw_ellipse(head->x, head->y, head->width/2.0, head->height/2.0, color, LINE_THICKNESS);
			}
		}
		head = head->next;
	}
}

ALLEGRO_BITMAP* render_map(tmx_map map) {
	ALLEGRO_BITMAP *res = NULL;
	tmx_layer layers = map->ly_head;
	int w, h;
	
	if (map->orient != O_ORT) fatal_error("only orthogonal orient currently supported in this example!");
	
	w = map->width  * map->tile_width;  /* Map's width and height */
	h = map->height * map->tile_height;
	if (!(res = al_create_bitmap(w, h))) fatal_error("failed to create bitmap!");
	
	al_set_target_bitmap(res);
	
	al_clear_to_color(int_to_al_color(map->backgroundcolor));
	
	while (layers) {
		if (layers->visible) {
			if (layers->type == L_OBJGR) {
				draw_objects(layers->content.head, int_to_al_color(layers->color));
			} else if (layers->type == L_IMAGE) {
				al_draw_bitmap((ALLEGRO_BITMAP*)layers->content.image->resource_image, 0, 0, 0);
			} else if (layers->type == L_LAYER) {
				/* TODO */
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

	/* Load and render the map */
	if (!(map = load_map_and_images(argv[1]))) return -1;
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
