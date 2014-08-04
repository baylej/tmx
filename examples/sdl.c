/*
	TMX usage example with SDL 2
	requires SDL_image
*/
#include <stdio.h>
#include <tmx.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_events.h>
#include <SDL2/SDL_keyboard.h>
#include <SDL2/SDL_keycode.h>
#include <SDL2/SDL_image.h>

#define fatal_error(str)  { fputs(str, stderr); goto errquit; }
#define fatal_error2(str) { fputs(str, stderr);  return NULL; }

#define DISPLAY_H 480
#define DISPLAY_W 640

void set_color(SDL_Renderer* ren, int color) {
	unsigned char r, g, b;

	r = (color >> 16) & 0xFF;
	g = (color >>  8) & 0xFF;
	b = (color)       & 0xFF;

	SDL_SetRenderDrawColor(ren, r, g, b, SDL_ALPHA_OPAQUE);
}

void draw_polyline(SDL_Renderer* ren, int **points, int x, int y, int pointsc) {
	int i;
	for (i=1; i<pointsc; i++) {
		SDL_RenderDrawLine(ren, x+points[i-1][0], y+points[i-1][1], x+points[i][0], y+points[i][1]);
	}
}

void draw_polygon(SDL_Renderer* ren, int **points, int x, int y, int pointsc) {
	draw_polyline(ren, points, x, y, pointsc);
	if (pointsc > 2) {
		SDL_RenderDrawLine(ren, x+points[0][0], y+points[0][1], x+points[pointsc-1][0], y+points[pointsc-1][1]);
	}
}

void draw_objects(SDL_Renderer* ren, tmx_object *head, int color) {
	SDL_Rect rect;
	set_color(ren, color);
	/* FIXME line thickness */
	while (head) {
		if (head->visible) {
			if (head->shape == S_SQUARE) {
				rect.x =     head->x;  rect.y =      head->y;
				rect.w = head->width;  rect.h = head->height;
				SDL_RenderDrawRect(ren, &rect);
			} else if (head->shape  == S_POLYGON) {
				draw_polygon(ren, head->points, head->x, head->y, head->points_len);
			} else if (head->shape == S_POLYLINE) {
				draw_polyline(ren, head->points, head->x, head->y, head->points_len);
			} else if (head->shape == S_ELLIPSE) {
				/* FIXME: no function in SDL2 */
			}
		}
		head = head->next;
	}
}

int gid_clear_flags(unsigned int gid) {
	return gid & TMX_FLIP_BITS_REMOVAL;
}

/* returns the bitmap and the region associated with this gid, returns -1 if tile not found */
short get_bitmap_region(unsigned int gid, tmx_tileset *ts, SDL_Surface **ts_bmp, unsigned int *x, unsigned int *y, unsigned int *w, unsigned int *h) {
	unsigned int tiles_x_count;
	unsigned int ts_w, id, tx, ty;
	gid = gid_clear_flags(gid);
	
	while (ts) {
		if (ts->firstgid <= gid) {
			if (!ts->next || ts->next->firstgid < ts->firstgid || ts->next->firstgid > gid) {
				id = gid - ts->firstgid; /* local id (for this image) */
				
				ts_w = ts->image->width  - 2 * (ts->margin) + ts->spacing;
				
				tiles_x_count = ts_w / (ts->tile_width  + ts->spacing);
				
				*ts_bmp = (SDL_Surface*)ts->image->resource_image;
				
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

void draw_layer(SDL_Renderer* ren, tmx_layer *layer, tmx_tileset *ts, unsigned int width, unsigned int height, unsigned int tile_width, unsigned int tile_height) {
	unsigned long i, j;
	unsigned int x, y, w, h;
	float op;
	SDL_Surface *tileset;
	SDL_Texture *tex_ts;
	SDL_Rect srcrect, dstrect;
	op = layer->opacity;
	for (i=0; i<height; i++) {
		for (j=0; j<width; j++) {
			if (!get_bitmap_region(layer->content.gids[(i*width)+j], ts, &tileset, &x, &y, &w, &h)) {
				/* TODO Opacity and Flips */
				srcrect.w = w;  srcrect.h = h;  srcrect.x = x;             srcrect.y = y;
				dstrect.w = w;  dstrect.h = h;  dstrect.x = j*tile_width;  dstrect.y = i*tile_height;
				tex_ts = SDL_CreateTextureFromSurface(ren, tileset);
				SDL_RenderCopy(ren, tex_ts, &srcrect, &dstrect);
				SDL_DestroyTexture(tex_ts);
			}
		}
	}
}

void draw_image_layer(SDL_Renderer* ren, tmx_image *img) {
	SDL_Surface *bmp; 
	SDL_Texture *tex;
	SDL_Rect dim;
	
	bmp =  (SDL_Surface*)img->resource_image;
	
	dim.x = dim.y = 0;
	dim.w = bmp->w;
	dim.h = bmp->h;
	
	if ((tex = SDL_CreateTextureFromSurface(ren, bmp))) {
		SDL_RenderCopy(ren, tex, NULL, &dim);
		SDL_DestroyTexture(tex);
	}
	
}

SDL_Texture* render_map(SDL_Renderer* ren, tmx_map *map) {
	SDL_Texture *res;
	tmx_layer *layers = map->ly_head;
	int w, h;
	
	w = map->width  * map->tile_width;  /* Bitmap's width and height */
	h = map->height * map->tile_height;
	
	if (!(res = SDL_CreateTexture(ren, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, w, h)))
		fatal_error2(SDL_GetError());
	SDL_SetRenderTarget(ren, res);
	
	set_color(ren, map->backgroundcolor);
	SDL_RenderClear(ren);
	
	while (layers) {
		if (layers->visible) {
			if (layers->type == L_OBJGR) {
				draw_objects(ren, layers->content.head, layers->color);
			} else if (layers->type == L_IMAGE) {
				draw_image_layer(ren, layers->content.image);
			} else if (layers->type == L_LAYER) {
				draw_layer(ren, layers, map->ts_head, map->width, map->height, map->tile_width, map->tile_height);
			}
		}
		layers = layers->next;
	}
	
	SDL_SetRenderTarget(ren, NULL);
	return res;
}

Uint32 timer_func(Uint32 interval, void *param) {
	SDL_Event event;
	SDL_UserEvent userevent;
	
	userevent.type = SDL_USEREVENT;
	userevent.code = 0;
	userevent.data1 = NULL;
	userevent.data2 = NULL;
	
	event.type = SDL_USEREVENT;
	event.user = userevent;
	
	SDL_PushEvent(&event);
	return(interval);
}

int main(int argc, char **argv) {
	tmx_map *map = NULL;
	SDL_Window *win;
	SDL_Renderer *ren;
	SDL_Event e;
	SDL_Texture *map_bmp;
	SDL_Rect map_rect;
	SDL_TimerID timer_id;
	int x_delta, y_delta;
	int key_state[2] = {0, 0};
	
	if (argc != 2) fatal_error("This program expects 1 argument");
	
	if (SDL_Init(SDL_INIT_VIDEO|SDL_INIT_EVENTS|SDL_INIT_TIMER) != 0)
		fatal_error(SDL_GetError());
	atexit(SDL_Quit);
	
	if (!(win = SDL_CreateWindow("SDL Game", 0, 0, DISPLAY_W, DISPLAY_H, SDL_WINDOW_SHOWN)))
		fatal_error(SDL_GetError());
	
	if (!(ren = SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC)))
		fatal_error(SDL_GetError());
	
	SDL_EventState(SDL_MOUSEMOTION, SDL_DISABLE);
	
	/* You probably want to create a fuction that creates a SDL_Texture directly here */
	rsc_img_load_func = (void* (*)(const char*))IMG_Load;
	rsc_img_free_func = (void  (*)(void*))      SDL_FreeSurface;
	
	if (!(map = tmx_load(argv[1]))) fatal_error(tmx_strerr());
	
	map_rect.w = map->width  * map->tile_width;
	map_rect.h = map->height * map->tile_height;
	map_rect.x = 0;  map_rect.y = 0;
	
	x_delta = DISPLAY_W - map_rect.w;
	y_delta = DISPLAY_H - map_rect.h;
	
	if (!(map_bmp = render_map(ren, map)))
		fatal_error(SDL_GetError());
	
	timer_id = SDL_AddTimer(30, timer_func, NULL);
	
	while (SDL_WaitEvent(&e)){
		
		if (e.type == SDL_QUIT) break;
		
		else if (e.type == SDL_KEYUP) {
			switch (e.key.keysym.scancode) {
				case SDL_SCANCODE_LEFT:
				case SDL_SCANCODE_RIGHT: key_state[0] = 0; break;
				case SDL_SCANCODE_UP:
				case SDL_SCANCODE_DOWN:  key_state[1] = 0; break;
			}
		}
		
		else if (e.type == SDL_KEYDOWN) {
			
			if (e.key.keysym.scancode == SDL_SCANCODE_Q) break;
			switch (e.key.keysym.scancode) {
				case SDL_SCANCODE_LEFT:  key_state[0] =  4; break;
				case SDL_SCANCODE_RIGHT: key_state[0] = -4; break;
				case SDL_SCANCODE_UP:    key_state[1] =  4; break;
				case SDL_SCANCODE_DOWN:  key_state[1] = -4; break;
			}
		}
		
		else if (e.type == SDL_USEREVENT) {
			
			map_rect.x += key_state[0];
			map_rect.y += key_state[1];
			if (x_delta > 0) {
				map_rect.x = x_delta/2;
			} else {
				if (map_rect.x < x_delta) map_rect.x = x_delta;
				if (map_rect.x > 0) map_rect.x = 0;
			}
			if (y_delta > 0) {
				map_rect.y = y_delta/2;
			} else {
				if (map_rect.y < y_delta) map_rect.y = y_delta;
				if (map_rect.y > 0) map_rect.y = 0;
			}
			
			SDL_RenderClear(ren);
			SDL_RenderCopy(ren, map_bmp, NULL, &map_rect);
			SDL_RenderPresent(ren);
		}
	}
	
	tmx_map_free(map);
	
	SDL_RemoveTimer(timer_id);
	SDL_DestroyTexture(map_bmp);
	SDL_DestroyRenderer(ren);
	SDL_DestroyWindow(win);
	SDL_Quit();
	
	return 0;
	
errquit:
	return -1;
}

