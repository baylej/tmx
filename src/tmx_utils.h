/* Private Header */

#pragma once

#ifndef TMXUTILS_H
#define TMXUTILS_H

/* From tmx_utils.c */
char* b64_encode(const char* source, unsigned int length);
char* b64_decode(const char* source, unsigned int *rlength);
#ifdef WANT_ZLIB
char* zlib_decompress(const char *source, unsigned int slength, unsigned int initial_capacity, unsigned int *rlength);
#endif

/* from tmx_xml.c */
#ifdef WANT_XML
tmx_map parse_xml(const char *filename);
#endif

/* from tmx_json.c */
#ifdef WANT_JSON
#endif

struct _tmp_tmxdata {
	enum enc{cvs, b64} encoding;
	enum cmp{none, zlib, gzip} compression;
	char *rawdata;
};

/*
	Node allocation
*/
tmx_property    alloc_prop(void);
tmx_image       alloc_image(void);
tmx_object      alloc_object(void);
tmx_objectgroup alloc_objgrp(void);
tmx_layer       alloc_layer(void);
tmx_tileset     alloc_tileset(void);
tmx_map         alloc_map(void);

/*
	Misc
*/
enum tmx_map_orient parse_orient(const char* orient_str);
int get_color_rgb(const char *c);
int count_char_occurences(const char *str, char c);

/*
	Error handling
*/
#if defined(WIN32) || defined(__WIN32__) || defined(_WIN32)
#define snprintf _snprintf
#endif

char custom_msg[256];
#define tmx_err(code, ...) tmx_errno = code; snprintf(custom_msg, 256, __VA_ARGS__)

#endif /* TMXUTILS_H */