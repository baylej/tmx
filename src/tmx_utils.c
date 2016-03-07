/*
	Utility functions
*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h> /* is */

#include "tmx.h"
#include "tmx_utils.h"

/*
	BASE 64
*/

static const char b64enc[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ" "abcdefghijklmnopqrstuvwxyz" "0123456789" "+/";

char* b64_encode(const char *source, unsigned int length) {
	unsigned int i, mlen, r_pos;
	unsigned short dif, j;
	unsigned int frame = 0;
	char out[5];
	char *res;

	mlen = 4 * length/3 + 1; /* +1 : returns a null-terminated string */
	if (length%3) {
		mlen += 4;
	}

	res = (char*) tmx_alloc_func(NULL, mlen);
	if (!res) {
		tmx_errno = E_ALLOC;
		return NULL;
	}
	res[mlen-1] = '\0';
	out[4] = '\0';

	for (i=0; i<length; i+=3) {
		/*frame = 0; clean frame not needed because '>>' inserts '0' */
		dif = (length-i)/3 ? 3 : (length-i)%3; /* number of byte to read */
		for (j=0; j<dif; j++) {
			memcpy(((char*)&frame)+2-j, source+i+j, 1); /* copy 3 bytes in reverse order */
		}
		/*
		now 3 cases :
		. 3B red => 4chars
		. 2B red => 3chars + "="
		. 1B red => 2chars + "=="
		*/
		for (j=0; j<dif+1; j++) {
			out[j] = (char)((frame & 0xFC0000) >> 18); /* first 6 bits */
			out[j] = b64enc[(int)out[j]];
			frame = frame << 6; /* next 6b word */
		}
		if (dif == 1) {
			out[2] = out [3] = '=';
		} else if (dif == 2) {
			out [3] = '=';
		}
		r_pos = (i/3)*4;
		strcpy(res+r_pos, out);
	}
	return res;
}

static char b64_value(char c) {
	if (c>='A' && c<='Z') {
		return c - 'A';
	} else if (c>='a' && c<='z') {
		return c - 'a' + 26;
	} else if (c>='0' && c<='9') {
		return c - '0' + 52;
	} else if (c=='+') {
		return 62;
	} else if (c=='/') {
		return 63;
	} else if (c=='=') {
		return 0;
	}
	return -1;
}

char* b64_decode(const char *source, unsigned int *rlength) { /* NULL terminated string */
	char *res, v;
	short j;
	unsigned int i;
	unsigned int in = 0;
	unsigned int src_len = (unsigned int)(strlen(source));

	if (!source) {
		tmx_err(E_INVAL, "Base64: invalid argument: source is NULL");
		return NULL;
	}

	if (src_len%4) {
		tmx_err(E_BDATA, "Base64: invalid source");
		return NULL; /* invalid source */
	}

	*rlength = (src_len/4)*3;
	res = (char*) tmx_alloc_func(NULL, *rlength);
	if (!res) {
		tmx_errno = E_ALLOC;
		return NULL;
	}

	for (i=0; i<src_len; i+=4) {
		in = 0;

		for (j=0; j<4; j++) {
			v = b64_value(source[i+j]);
			if (v == -1) {
				tmx_err(E_BDATA, "Base64: invalid char '%c' in source", source[i+j]);
				goto cleanup;
			}
			in = in << 6;
			in += v; /* add 6b */
		}
		for (j=0; j<3; j++) {
			memcpy(res+(i/4)*3+j, ((char*)&in)+2-j, 1); /* copy 3 bytes in reverse order */
		}
	}

	if (source[src_len-1] == '=') {
		(*rlength)--;
	}
	if (source[src_len-2] == '=') {
		(*rlength)--;
	}

	return res;

cleanup:
	tmx_free_func(res);
	return NULL;
}

/*
	ZLib
*/

#ifdef WANT_ZLIB
#include <zlib.h>

void* z_alloc(void *opaque UNUSED, unsigned int items, unsigned int size) {
	return tmx_alloc_func(NULL, items *size);
}

void z_free(void *opaque UNUSED, void *address) {
	tmx_free_func(address);
}

char* zlib_decompress(const char *source, unsigned int slength, unsigned int rlength) {
	int ret;
	char *res = NULL;
	z_stream strm;

	if (!source) {
		tmx_err(E_INVAL, "zlib_decompress: invalid argument: source is NULL");
		return NULL;
	}

	strm.zalloc = z_alloc;
	strm.zfree = z_free;
	strm.opaque = Z_NULL;
	strm.next_in = (Bytef*)source;
	strm.avail_in = slength;

	res = (char*) tmx_alloc_func(NULL, rlength);
	if (!res) {
		tmx_errno = E_ALLOC;
		return NULL;
	}

	strm.next_out = (Bytef*)res;
	strm.avail_out = rlength;

	/* 15+32 to enable zlib and gzip decoding with automatic header detection */
	if ((ret=inflateInit2(&strm, 15 + 32)) != Z_OK) {
		tmx_err(E_UNKN, "zlib_decompress: inflateInit2 returned %d\n", ret);
		goto cleanup;
	}

	ret = inflate(&strm, Z_FINISH);
	inflateEnd(&strm);

	if (ret != Z_OK && ret != Z_STREAM_END) {
		tmx_err(E_ZDATA, "zlib_decompress: inflate returned %d\n", ret);
		goto cleanup;
	}

	if (strm.avail_out != 0) {
		tmx_err(E_ZDATA, "layer contains not enough tiles");
		goto cleanup;
	}
	if (strm.avail_in != 0) {
		/* FIXME There is remains in the source */
	}

	return res;
cleanup:
	tmx_free_func(res);
	return NULL;
}

#else

char* zlib_decompress(const char *source, unsigned int slength, unsigned int rlength) {
	tmx_err(E_FONCT, "This library was not built with the zlib/gzip support");
	return NULL;
}

#endif /* WANT_ZLIB */

/*
	Layer data decoders
*/

int data_decode(const char *source, enum enccmp_t type, size_t gids_count, int32_t **gids) {
	char *b64dec;
	unsigned int b64_len, i;

	if (type==CSV) {
		if (!(*gids = (int32_t*)tmx_alloc_func(NULL, gids_count * sizeof(int32_t)))) {
			tmx_errno = E_ALLOC;
			return 0;
		}
		for (i=0; i<gids_count; i++) {
			if (sscanf(source, "%d", (*gids)+i) != 1) {
				tmx_err(E_CDATA, "error in CVS while reading tile #%d", i);
				return 0;
			}
			if (!(source = strchr(source, ',')) && i!=gids_count-1) {
				tmx_err(E_CDATA, "error in CVS after reading tile #%d", i);
				return 0;
			}
			source++;
		}
	}
	else if (type==B64Z) {
		if (!(b64dec = b64_decode(source, &b64_len))) return 0;
		*gids = (int32_t*)zlib_decompress(b64dec, b64_len, (unsigned int)(gids_count*sizeof(int32_t)));
		tmx_free_func(b64dec);
		if (!(*gids)) return 0;
	}

	return 1;
}

/*
	Node allocation
*/

static void* node_alloc(size_t size) {
	void *res = tmx_alloc_func(NULL, size);
	if (res) {
		memset(res, 0, size);
	} else {
		tmx_errno = E_ALLOC;
	}
	return res;
}

tmx_property* alloc_prop(void) {
	return (tmx_property*)node_alloc(sizeof(tmx_property));
}

tmx_image* alloc_image(void) {
	return (tmx_image*)node_alloc(sizeof(tmx_image));
}

tmx_object* alloc_object(void) {
	tmx_object *res = (tmx_object*)node_alloc(sizeof(tmx_object));
	if (res) {
		res->visible = 1;
	}
	return res;
}

tmx_object_group* alloc_objgr(void) {
	return (tmx_object_group*)node_alloc(sizeof(tmx_object_group));
}

tmx_layer* alloc_layer(void) {
	tmx_layer *res = (tmx_layer*)node_alloc(sizeof(tmx_layer));
	if (res) {
		res->opacity = 1.0f;
		res->visible = 1;
	}
	return res;
}

tmx_tile* alloc_tiles(int count) {
	return (tmx_tile*)node_alloc(count * sizeof(tmx_tile));
}

tmx_tileset* alloc_tileset(void) {
	return (tmx_tileset*)node_alloc(sizeof(tmx_tileset));
}

tmx_map* alloc_map(void) {
	return (tmx_map*)node_alloc(sizeof(tmx_map));
}

/*
	Misc
*/

/* Sets tile->tileset and tile->ul_x,y */
int set_tiles_runtime_props(tmx_tileset *ts) {
	unsigned int i, j;
	unsigned int tiles_x_count, ts_w, tx, ty;

	if (ts == NULL) {
		tmx_err(E_INVAL, "set_tiles_runtime_props: invalid argument: ts is NULL");
		return 0;
	}

	/* reindex tiles (tiles are sorted, but not indexed correctly) */
	for (i=ts->tilecount-1, j=0; j<ts->tilecount; j++, i--) {
		if (ts->tiles[i].id > 0L && ts->tiles[i].id != i) {
			memcpy(ts->tiles+(ts->tiles[i].id), ts->tiles+i, sizeof(tmx_tile));
			memset(ts->tiles+i, 0, sizeof(tmx_tile));
		}
	}

	for (i=0; i<ts->tilecount; i++) {
		ts->tiles[i].id = i;
		ts->tiles[i].tileset = ts;

		/* set bitmap's region x and y coordinates */
		ts_w = ts->image->width - 2 * (ts->margin) + ts->spacing;

		tiles_x_count = ts_w / (ts->tile_width + ts->spacing);

		tx = i % tiles_x_count;
		ty = i / tiles_x_count;

		ts->tiles[i].ul_x = ts->margin + (tx * ts->tile_width)  + (tx * ts->spacing);
		ts->tiles[i].ul_y = ts->margin + (ty * ts->tile_height) + (ty * ts->spacing);
	}

	return 1;
}

/* Creates the array at map->tiles */
int mk_map_tile_array(tmx_map *map) {
	unsigned int i;
	tmx_tileset *ts, *max_ts;

	if (!map) {
		tmx_err(E_INVAL, "mk_map_tile_array: invalid argument: map is NULL");
		return 0;
	}

	/* Counts total tile count */
	ts = max_ts = map->ts_head;
	while (ts != NULL) {
		if (ts->firstgid > max_ts->firstgid) {
			max_ts = ts;
		}
		ts = ts->next;
	}
	if (max_ts->image) {
		map->tilecount = max_ts->firstgid + max_ts->tilecount;
	}
	else {
		/* Gets the last id, ts->tiles is sorted by id */
		map->tilecount = max_ts->firstgid + max_ts->tiles[max_ts->tilecount - 1].id + 1;
	}

	/* Allocates the GID indexed tile array */
	if (!(map->tiles = tmx_alloc_func(NULL, map->tilecount * sizeof(void*)))) {
		tmx_errno = E_ALLOC;
		return 0;
	}
	memset(map->tiles, 0, map->tilecount * sizeof(void*));

	/* Populates the array */
	map->tiles[0] = NULL; /* GIDs start from 1 */
	ts = map->ts_head;
	while (ts != NULL) {
		for (i=0; i<ts->tilecount; i++) {
			map->tiles[ts->firstgid + ts->tiles[i].id] = &(ts->tiles[i]);
		}
		ts = ts->next;
	}

	return 1;
}

/* "orthogonal" -> ORT */
enum tmx_map_orient parse_orient(const char *orient_str) {
	if (!strcmp(orient_str, "orthogonal")) {
		return O_ORT;
	}
	if (!strcmp(orient_str, "isometric")) {
		return O_ISO;
	}
	if (!strcmp(orient_str, "stagging")) {
		return O_STA;
	}
	if (!strcmp(orient_str, "hexagonal")) {
		return O_HEX;
	}
	return O_NONE;
}

/* "left-up" -> R_LEFTUP */
enum tmx_map_renderorder parse_renderorder(const char *renderorder) {
	if (renderorder == NULL || !strcmp(renderorder, "right-down")) {
		return R_RIGHTDOWN;
	}
	if (!strcmp(renderorder, "right-up")) {
		return R_RIGHTUP;
	}
	if (!strcmp(renderorder, "left-down")) {
		return R_LEFTDOWN;
	}
	if (!strcmp(renderorder, "left-up")) {
		return R_LEFTUP;
	}
	return R_NONE;
}

/* "index" -> G_INDEX */
enum tmx_objgr_draworder parse_objgr_draworder(const char *draworder) {
	if (draworder == NULL || !strcmp(draworder, "topdown")) {
		return G_TOPDOWN;
	}
	if (!strcmp(draworder, "index")) {
		return G_INDEX;
	}
	return G_NONE;
}

/* "even" -> SI_EVEN */
enum tmx_stagger_index parse_stagger_index(const char *staggerindex) {
	if (staggerindex == NULL || !strcmp(staggerindex, "odd")) {
		return SI_ODD;
	}
	if (!strcmp(staggerindex, "even")) {
		return SI_EVEN;
	}
	return SI_NONE;
}

/* "y" -> SA_Y */
enum tmx_stagger_axis parse_stagger_axis(const char *staggeraxis) {
	if (staggeraxis == NULL || !strcmp(staggeraxis, "y")) {
		return SA_Y;
	}
	if (!strcmp(staggeraxis, "columns")) {
		return SA_X;
	}
	return SA_NONE;
}

/* "#337FA2" -> 0x337FA2 */
int get_color_rgb(const char *c) {
	if (*c == '#') c++;
	return (int)strtol(c, NULL, 16);
}

int count_char_occurences(const char *str, char c) {
	int res = 0;
	while(*str != '\0') {
		if (*str == c) res++;
		str++;
	}
	return res;
}

/* trim 'str' to avoid blank characters at its beginning and end */
char* str_trim(char *str) {
	int end = (int)(strlen(str)-1);
	while (end>=0 && isspace((unsigned char) str[end])) end--;
	str[end+1] = '\0';

	while(isspace((unsigned char) str[0])) str++;
	return str;
}

/* duplicate a string */
char* tmx_strdup(const char *str) {
	char *res =  (char*)tmx_alloc_func(NULL, strlen(str)+1);
	strcpy(res, str);
	return res;
}

size_t dirpath_len(const char *str) {
	const char *lastslash  = strrchr(str, '/' );
	const char *lastbslash = strrchr(str, '\\');

	const char *last_path_sep = MAX(lastslash, lastbslash);
	if (last_path_sep == NULL) return 0;
	last_path_sep++; /* Keeps the trailing path separator */

	return (size_t) (last_path_sep - str);
}

/* ("C:\Maps\map.tmx", "tilesets\ts1.tsx") => "C:\Maps\tilesets\ts1.tsx" */
char* mk_absolute_path(const char *base_path, const char *rel_path) {
	/* if base_path is a directory, it MUST have a trailing path separator */
	size_t dp_len = dirpath_len(base_path);
	size_t rp_len = strlen(rel_path);
	size_t ap_len = dp_len + rp_len;

	char* res = (char*)tmx_alloc_func(NULL, ap_len+1);
	if (!res) {
		tmx_errno = E_ALLOC;
		return NULL;
	}

	memcpy(       res, base_path, dp_len);
	memcpy(res+dp_len,  rel_path, rp_len);
	*(res+ap_len) = '\0';

	return res;
}

/* resolves the path to the image, and delegates to the client code */
void* load_image(void **ptr, const char *base_path, const char *rel_path) {
	char *ap_img;
	if (tmx_img_load_func) {
		ap_img = mk_absolute_path(base_path, rel_path);
		if (!ap_img) return 0;
		*ptr = tmx_img_load_func(ap_img);
		tmx_free_func(ap_img);
		return(*ptr);
	}
	return (void*)1;
}
