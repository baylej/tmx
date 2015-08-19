/*
	TMX.H - TMX C LOADER
	Copyright (c) 2013-2014, Bayle Jonathan <baylej@github>

	Data Stuctures storing the map, and functions prototypes

	See : (I'm using names from this documentation)
	http://doc.mapeditor.org/reference/tmx-map-format/
*/

#pragma once

#ifndef TMX_H
#define TMX_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define TMX_FLIPPED_HORIZONTALLY 0x80000000
#define TMX_FLIPPED_VERTICALLY   0x40000000
#define TMX_FLIPPED_DIAGONALLY   0x20000000
#define TMX_FLIP_BITS_REMOVAL    0x1FFFFFFF

/*
	Configuration
*/
/* Custom realloc and free function, for memalloc debugging purposes
   Please modify these values if once before you use tmx_load */
extern void* (*tmx_alloc_func) (void *address, size_t len); /* realloc */
extern void  (*tmx_free_func ) (void *address);             /* free */

/* load/free tmx_image->resource_image, you should set this if you want
   the library to load/free images */
extern void* (*tmx_img_load_func) (const char *path);
extern void  (*tmx_img_free_func) (void *address);

/*
	Data Structures
*/

enum tmx_map_orient {O_NONE, O_ORT, O_ISO, O_STA};
enum tmx_map_renderorder {R_NONE, R_RIGHTDOWN, R_RIGHTUP, R_LEFTDOWN, R_LEFTUP};
enum tmx_layer_type {L_NONE, L_LAYER, L_OBJGR, L_IMAGE};
enum tmx_objgr_draworder {G_NONE, G_INDEX, G_TOPDOWN};
enum tmx_shape {S_NONE, S_SQUARE, S_POLYGON, S_POLYLINE, S_ELLIPSE, S_TILE};

typedef struct _tmx_prop { /* <properties> and <property> */
	char *name;
	char *value;
	struct _tmx_prop *next;
} tmx_property;

typedef struct _tmx_img { /* <image> */
	char *source;
	int trans; /* bytes : RGB */
	int uses_trans;
	unsigned long width, height;
	/*char *format; Not currently implemented in QtTiled
	char *data;*/
	void *resource_image;
} tmx_image;

typedef struct _tmx_tile { /* <tile> */
	unsigned int id;
	tmx_image* image;
	tmx_property *properties;
	struct _tmx_tile *next;
} tmx_tile;

typedef struct _tmx_ts { /* <tileset> and <tileoffset> */
	unsigned int firstgid;
	char *name;
	unsigned int tile_width, tile_height;
	unsigned int spacing, margin;
	int x_offset, y_offset; /* tileoffset */
	/* terraintypes(0.9) is for the QtTiled terrain feature */
	tmx_image *image;
	tmx_property *properties;
	tmx_tile *tiles;
	struct _tmx_ts *next;
} tmx_tileset;

typedef struct _tmx_obj { /* <object> */
	char *name;
	enum tmx_shape shape;
	double x, y;
	double width, height;
	int gid;
	double **points; /* point[i][x,y]; x=0 y=1 */
	int points_len;
	int visible; /* 0 == false */
	double rotation;
	tmx_property *properties;
	struct _tmx_obj *next;
} tmx_object;

typedef struct _tmx_objgr { /* <objectgroup> */
	int color; /* bytes : RGB */
	enum tmx_objgr_draworder draworder;
	tmx_object *head;
} tmx_object_group;

typedef struct _tmx_layer { /* <layer> or <imagelayer> or <objectgroup> */
	char *name;
	double opacity;
	int visible; /* 0 == false */
	int x_offset, y_offset; /* For image layers */

	enum tmx_layer_type type;
	union layer_content {
		int32_t *gids;
		tmx_object_group *objgr;
		tmx_image *image;
	} content;

	void *user_data; /* not freed by tmx_free ! */
	tmx_property *properties;
	struct _tmx_layer *next;
} tmx_layer;

typedef struct _tmx_map { /* <map> (Head of the data structure) */
	enum tmx_map_orient orient;
	unsigned int width, height;
	unsigned int tile_width, tile_height;
	int backgroundcolor; /* bytes : RGB */
	enum tmx_map_renderorder renderorder;

	tmx_property *properties;
	tmx_tileset *ts_head;
	tmx_layer *ly_head;
} tmx_map;

/*
	Functions
*/

/* Load a map and return the head of the data structure
   returns NULL if an error occured and set tmx_errno */
tmx_map *tmx_load(const char *path);

/* Free the map data structure */
void tmx_map_free(tmx_map *map);

/* returns the tileset and the upper-left coordinate on the tileset
   of the tile associated with this gid, returns NULL if it fails */
tmx_tileset* tmx_get_tileset(tmx_map *map, unsigned int gid, unsigned int *x, unsigned int *y);

/* returns the tile associated with this gid, returns NULL if it fails */
tmx_tile* tmx_get_tile(tmx_map *map, unsigned int gid);

/*
	Error handling
	each time a function fails, tmx_errno is set
*/

/* possible values for `tmx_errno` */
typedef enum _tmx_error_codes {
	/* Syst */
	E_NONE   = 0,     /* No error so far */
	E_UNKN   = 1,     /* See the message for more details */
	E_INVAL  = 2,     /* Invalid argument */
	E_ALLOC  = 8,     /* Mem alloc */
	/* I/O */
	E_ACCESS = 10,    /* privileges needed */
	E_NOENT  = 11,    /* File not found */
	E_FORMAT = 12,    /* Unsupproted/Unknown file format */
	E_ENCCMP = 13,    /* Unsupproted/Unknown data encoding/compression */
	E_FONCT  = 16,    /* Fonctionnality not enabled */
	E_BDATA  = 20,    /* B64 bad data */
	E_ZDATA  = 21,    /* Zlib corrupted data */
	E_XDATA  = 22,    /* XML corrupted data */
	E_JDATA  = 23,    /* JSON corrupted data */
	E_CDATA  = 24,    /* CSV corrupted data */
	E_MISSEL = 30     /* Missing element, incomplete source */
} tmx_error_codes;

extern tmx_error_codes tmx_errno;

/* print the error message prefixed with the parameter */
void tmx_perror(const char*);
/* return the error message for the current value of `tmx_errno` */
const char* tmx_strerr(void); /* FIXME errno parameter ? (as strerror) */

#ifdef __cplusplus
}
#endif

#endif /* TMX_H */
