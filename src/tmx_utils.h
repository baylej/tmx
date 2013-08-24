/* Private Header */

#pragma once

#ifndef TMXUTILS_H
#define TMXUTILS_H

/* From tmx_utils.c */
enum enccmp_t {CSV, B64Z};
int data_decode(const char *source, enum enccmp_t type, size_t gids_count, int32_t **gids);

/* from tmx_xml.c */
tmx_map parse_xml(const char *filename);

/* from tmx_json.c */
tmx_map parse_json(const char *filename);

/*
	Node allocation
*/
tmx_property    alloc_prop(void);
tmx_image       alloc_image(void);
tmx_object      alloc_object(void);
tmx_layer       alloc_layer(void);
tmx_tileset     alloc_tileset(void);
tmx_map         alloc_map(void);

/*
	Misc
*/
enum tmx_map_orient parse_orient(const char* orient_str);
int get_color_rgb(const char *c);
int count_char_occurences(const char *str, char c);
char* str_trim(char *str);
char* tmx_strdup(const char *str);

/*
	Error handling
*/
#if defined(WIN32) || defined(__WIN32__) || defined(_WIN32)
#define snprintf _snprintf
#endif

char custom_msg[256];
#define tmx_err(code, ...) tmx_errno = code; snprintf(custom_msg, 256, __VA_ARGS__)

#endif /* TMXUTILS_H */