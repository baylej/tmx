/* Private Header */

#pragma once

#ifndef TMXUTILS_H
#define TMXUTILS_H

/* UNUSED macro to suppress `unused parameter` warnings with GCC and CLANG */
#ifdef __GNUC__
#define UNUSED __attribute__((__unused__))
#else
#define UNUSED
#endif

/*
	Parser implementations
*/
enum enccmp_t {CSV, B64Z};
int data_decode(const char *source, enum enccmp_t type, size_t gids_count, int32_t **gids);
tmx_map* parse_xml(const char *filename); /* tmx_xml.c */

/*
	Node allocation
*/
tmx_property*     alloc_prop(void);
tmx_image*        alloc_image(void);
tmx_object*       alloc_object(void);
tmx_object_group* alloc_objgr(void);
tmx_layer*        alloc_layer(void);
tmx_tile*         alloc_tiles(int count);
tmx_tileset*      alloc_tileset(void);
tmx_map*          alloc_map(void);
tmx_tile*         alloc_tile(void);

/*
	Misc
*/
#define MAX(a,b) (a<b) ? b: a;
int set_tiles_runtime_props(tmx_tileset *ts);
int mk_map_tile_array(tmx_map *map);
enum tmx_map_orient parse_orient(const char *orient_str);
enum tmx_map_renderorder parse_renderorder(const char *renderorder);
enum tmx_objgr_draworder parse_objgr_draworder(const char *draworder);
enum tmx_stagger_index parse_stagger_index(const char *staggerindex);
enum tmx_stagger_axis parse_stagger_axis(const char *staggeraxis);
int get_color_rgb(const char *c);
int count_char_occurences(const char *str, char c);
char* str_trim(char *str);
char* tmx_strdup(const char *str);

/*
	FS
*/
size_t dirpath_len(const char *str);
char* mk_absolute_path(const char *base_path, const char *rel_path);
void* load_image(void **ptr, const char *base_path, const char *rel_path);

/*
	Error handling
*/
#if defined(WIN32) || defined(__WIN32__) || defined(_WIN32)
#define snprintf _snprintf
#endif

char custom_msg[256];
#define tmx_err(code, ...) tmx_errno = code; snprintf(custom_msg, 256, __VA_ARGS__)

#endif /* TMXUTILS_H */
