/*
	Stuctures storing the map and functions prototypes
	
	See : (I'm using names from this documentation)
	https://github.com/bjorn/tiled/wiki/TMX-Map-Format
*/


#ifndef TMX_H
#define TMX_H

#define FLIPPED_HORIZONTALLY_FLAG 0x80000000;
#define FLIPPED_VERTICALLY_FLAG   0x40000000;
#define FLIPPED_DIAGONALLY_FLAG   0x20000000;

/*
	Data Structures
*/

typedef struct {
	enum tmx_map_orient{ORT, ISO, STA} orient;
	unsigned int width;
	unsigned int height;
	unsigned int tile_width;
	unsigned int tile_height;
	int backgroundcolor; /* bytes : RGB */
	/* TODO: can contain: properties, tileset, layer, objectgroup, imagelayer */
} * tmx_map;

typedef struct {
	unsigned int firstgid;
	char *name;
	void *source; /* FIXME data type */
	unsigned int tile_width;
	unsigned int tile_height;
	unsigned int spacing;
	unsigned int margin;
	unsigned int x_offset; /* tileoffset */
	unsigned int y_offset;
	/* TODO: can contain: tileoffset, properties, image, terraintypes, tile */
} * tmx_tileset;

typedef struct {
	void *format; /* FIXME data type */
	void *source; /* FIXME data type */
	int trans; /* bytes : RGB */
	unsigned long width;
	unsigned long height;
	/* TODO: can contain: data */
} * tmx_image;

/* TODO: terrains */

typedef struct {
	char *name;
	float opacity;
	char visible; /* 0 == false */
	/* TODO: can contain: properties, data */
} * tmx_layer;

typedef struct {
	enum tmx_enc {csv, b64} encoding;
	enum tmx_cmp {none, zip, gzip} compression;
	char *rawdata;
} * tmx_data;

typedef struct {
	char *name;
	char *value;
} * tmx_property;

/*
	Functions
*/


#endif /* TMX_H */