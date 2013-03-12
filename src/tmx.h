/*
	Stuctures storing the map and functions prototypes
	
	See : (I'm using names from this documentation)
	https://github.com/bjorn/tiled/wiki/TMX-Map-Format
*/

#pragma once

#ifndef TMX_H
#define TMX_H

#include <stdint.h>

#define FLIPPED_HORIZONTALLY_FLAG 0x80000000;
#define FLIPPED_VERTICALLY_FLAG   0x40000000;
#define FLIPPED_DIAGONALLY_FLAG   0x20000000;

/*
	Data Structures
*/

enum tmx_map_orient{T_ORT = 1, T_ISO}; /* T_STA : stagging (0.9) */

typedef struct _tmx_prop {
	char *name;
	char *value;
	struct _tmx_prop *next;
} * tmx_property;

typedef struct _tmx_img {
	char *source;
	int trans; /* bytes : RGB */
	unsigned long width, height;
	//char *format; /* (0.9) */
	//char *data; /* (0.9) */
} * tmx_image;

typedef struct _tmx_ts {
	unsigned int firstgid;
	char *name;
	//char *source; /* not supporting this */
	unsigned int tile_width, tile_height;
	unsigned int spacing, margin;
	unsigned int x_offset, y_offset; /* tileoffset */
	/* TODO: can contain: terraintypes(0.9), tile(0.9) */
	tmx_image image;
	//tmx_property properties; /* FIXME No way to set props in "QT Tiled" */
	struct _tmx_ts *next;
} * tmx_tileset;

/* TODO: terrains(0.9) and imagelayer(0.9) */

typedef struct _tmx_layer {
	char *name;
	float opacity;
	char visible; /* 0 == false */
	int32_t *gids;
	tmx_property properties;
	struct _tmx_layer *next;
} * tmx_layer;

typedef struct _tmx_obj {
	char *name;
	enum tmx_shape {square, polygon, polyline} shape; /* ellipse(0.9) */
	unsigned long x, y;
	unsigned long width, height;
	int gid;
	//char visible; /* 0 == false (0.9) */
	struct _tmx_obj *next;
} * tmx_object;

typedef struct _tmx_objgrp {
	char *name;
	int color; /* bytes : RGB */
	float opacity;
	char visible; /* 0 == false */
	tmx_object head;
	struct _tmx_objgrp *next;
} * tmx_objectgroup;

typedef struct _tmx_map {
	enum tmx_map_orient orient;
	unsigned int width, height;
	unsigned int tile_width, tile_height;
	//int backgroundcolor; /* (0.9) bytes : RGB */
	
	tmx_property properties;
	tmx_tileset ts_head;
	tmx_layer ly_head;
	tmx_objectgroup ob_head;
	/* TODO: can contain: imagelayer(0.9) */
} * tmx_map;

/*
	Functions
*/

tmx_map tmx_load(const char * path);
void tmx_free(tmx_map *map);

/*
	Error handling
	each time a function fails, tmx_errno is set
*/

int tmx_errno;

void tmx_perror(const char*);
const char* tmx_strerr(void); /* DO NOT FREE */

enum _tmx_error_codes {
	/* Syst */
	E_NONE   = 0,     /* No error so far */
	E_UNKN   = 1,     /* See the message for more details */
	E_INVAL  = 2,     /* Invalid argument */
	E_ALLOC  = 8,     /* Mem alloc */
	/* I/O */
	E_ACCESS = 10,    /* privileges needed */
	E_NOENT  = 11,    /* File not found */
	E_BDATA  = 20,    /* B64 bad data */
	E_ZDATA  = 21,    /* Zlib corrupted data */
	E_XDATA  = 22,    /* XML corrupted data */
	E_JDATA  = 23,    /* JSON corrupted data */
	E_MISSEL = 30     /* Missing element, incomplete source */
};

#endif /* TMX_H */