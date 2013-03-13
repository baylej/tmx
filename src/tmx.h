/*
	TMH.H - TMX C LOADER
	Copyright (c) 2013, Bayle Jonathan <baylej@github>

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
	Configuration
*/
/* Custom realloc and free function, for memalloc debugging purposes
   Please modify these values if once before you use tmx_load */
void* (*tmx_alloc_func) (void *address, size_t len); /* realloc */
void  (*tmx_free_func ) (void *address);             /* free */

/*
	Data Structures
*/

enum tmx_map_orient{O_NONE, O_ORT, O_ISO}; /* T_STA : stagging (0.9) */
enum tmx_shape {S_NONE, S_SQUARE, S_POLYGON, S_POLYLINE}; /* ellipse(0.9) */

typedef struct _tmx_prop { /* <properties> and <property> */
	char *name;
	char *value;
	struct _tmx_prop *next;
} * tmx_property;

typedef struct _tmx_img { /* <image> */
	char *source;
	int trans; /* bytes : RGB */
	unsigned long width, height;
	//char *format; /* (0.9) */
	//char *data; /* (0.9) */
} * tmx_image;

typedef struct _tmx_ts { /* <tileset> and <tileoffset> */
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

typedef struct _tmx_layer { /* <layer> and it's <data> */
	char *name;
	float opacity;
	char visible; /* 0 == false */
	int32_t *gids;
	tmx_property properties;
	struct _tmx_layer *next;
} * tmx_layer;

typedef struct _tmx_obj { /* <object> */
	char *name;
	enum tmx_shape shape;
	unsigned long x, y;
	unsigned long width, height;
	int gid;
	int **points; /* point[i][x,y]; x=0 y=1 */
	int points_len;
	//char visible; /* 0 == false (0.9) */
	struct _tmx_obj *next;
} * tmx_object;

typedef struct _tmx_objgrp { /* <objectgroup> */
	char *name;
	int color; /* bytes : RGB */
	float opacity;
	char visible; /* 0 == false */
	tmx_object head;
	struct _tmx_objgrp *next;
} * tmx_objectgroup;

typedef struct _tmx_map { /* <map> (Head of the data structure) */
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

/* Load a map and return the head of the data structure
   returns NULL if an error occured and set tmx_errno */
tmx_map tmx_load(const char * path);
/* Free the map data structure */
void tmx_free(tmx_map *map);

/*
	Error handling
	each time a function fails, tmx_errno is set
*/

int tmx_errno;

/* print the error message prefixed with the parameter */
void tmx_perror(const char*);
/* return the error message for the current value of `tmx_errno` */
const char* tmx_strerr(void); /* FIXME errno parameter ? (as strerror) */

/* possible values for `tmx_errno` */
enum _tmx_error_codes {
	/* Syst */
	E_NONE   = 0,     /* No error so far */
	E_UNKN   = 1,     /* See the message for more details */
	E_INVAL  = 2,     /* Invalid argument */
	E_ALLOC  = 8,     /* Mem alloc */
	/* I/O */
	E_ACCESS = 10,    /* privileges needed */
	E_NOENT  = 11,    /* File not found */
	E_FORMAT = 12,    /* Unsupproted/Unknown file format */
	E_FONCT  = 13,    /* Fonctionnality not enbled */
	E_BDATA  = 20,    /* B64 bad data */
	E_ZDATA  = 21,    /* Zlib corrupted data */
	E_XDATA  = 22,    /* XML corrupted data */
	E_JDATA  = 23,    /* JSON corrupted data */
	E_MISSEL = 30     /* Missing element, incomplete source */
};

#endif /* TMX_H */