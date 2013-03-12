#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "tmx.h"
#include "tmx_utils.h"

/*
	Utility functions
*/

/*
	Endianness
	If the Host is BigEndian, we have to convert layers data from LittleEndian
*/

#define T_BIG_ENDIAN 'B'
#define T_LIT_ENDIAN 'L'
static char find_endianness() {
	int num=1;
	if(*(char*)&num==1)
		return T_LIT_ENDIAN;
	return T_BIG_ENDIAN;
}

/* TODO : function that do LE --> BE */

/* BASE 64 */

static const char b64enc[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ" "abcdefghijklmnopqrstuvwxyz" "0123456789" "+/";

char* b64_encode(const char* source, unsigned int length) {
	unsigned int i, mlen, r_pos;
	unsigned short dif, j;
	unsigned int frame = 0;
	char out[5];
	char *res;

	mlen = 4 * length/3 + 1; /* +1 : returns a null-terminated string */
	if (length%3) {
		mlen += 4;
	}

	res = (char*) malloc(mlen);
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
			out[j] = (frame & 0xFC0000) >> 18; /* first 6 bits */
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

char* b64_decode(const char* source, unsigned int *rlength) { /* NULL terminated string */
	char *res, v;
	short j;
	unsigned int i;
	unsigned int in = 0;
	unsigned int src_len = strlen(source);

	if (!source) {
		tmx_err(E_INVAL, "Base64: invalid argument: source is NULL");
		return NULL;
	}

	if (src_len%4) {
		tmx_err(E_BDATA, "Base64: invalid source");
		return NULL; /* invalid source */
	}

	*rlength = (src_len/4)*3;
	res = (char*) malloc(*rlength);
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
	free(res);
	return NULL;
}

/*
	ZLib
*/

#ifdef WANT_ZLIB
#include <zlib.h>

/* in in out */
char* zlib_compress(const char *source, unsigned int slength, unsigned int *rlength) {
	return NULL;
}

/* in in in out */
char* zlib_decompress(const char *source, unsigned int slength, unsigned int initial_capacity, unsigned int *rlength) {
	int ret;
	char *res = NULL, *tmp;
	z_stream strm;

	if (!source) {
		tmx_err(E_INVAL, "zlib_decompress: invalid argument: source is NULL");
		return NULL;
	}

	strm.zalloc = Z_NULL;
	strm.zfree = Z_NULL;
	strm.opaque = Z_NULL;
	strm.next_in = (Bytef*)source;
	strm.avail_in = strlen(source);
	
	if (initial_capacity) {
		*rlength = initial_capacity;
	} else {
		*rlength = slength * 2;
	}
	res = (char*) malloc(*rlength);
	if (!res) {
		tmx_errno = E_ALLOC;
		return NULL;
	}

	strm.next_out = (Bytef*)res;
	strm.avail_out = *rlength;
	
	/* 15+32 to enable zlib and gzip decoding with automatic header detection */
	if ((ret=inflateInit2(&strm, 15 + 32)) != Z_OK) {
		tmx_err(E_UNKN, "zlib_decompress: inflateInit2 returned %d\n", ret);
		goto cleanup;
	}

	do {
		ret = inflate(&strm, Z_SYNC_FLUSH);
		if (ret == Z_BUF_ERROR) { /* too tiny buffer */
			strm.avail_out = *rlength;
			*rlength *= 2;
			if (!(tmp = (char*)realloc(res, *rlength))) {
				tmx_errno = E_ALLOC;
				inflateEnd(&strm);
				goto cleanup;
			}
			res = tmp;
		} else if (ret != Z_STREAM_END && ret != Z_OK) {
			tmx_err(E_ZDATA, "zlib_decompress: inflate returned %d\n", ret);
			inflateEnd(&strm);
			goto cleanup;
		}
	} while (ret != Z_STREAM_END);

	if (strm.avail_in != 0) {
		/* FIXME There is remains in the source */
	}
	*rlength -= strm.avail_out;
	inflateEnd(&strm);

	return res;
cleanup:
	free(res);
	return NULL;
}

/*
	Node allocation
*/

tmx_property alloc_prop(void) {
	tmx_property res = (tmx_property)malloc(sizeof(struct _tmx_prop));
	if (res) {
		memset(res, 0, sizeof(struct _tmx_prop));
	} else {
		tmx_errno = E_ALLOC;
	}
	return res;
}

tmx_image alloc_image(void) {
	tmx_image res = (tmx_image)malloc(sizeof(struct _tmx_img));
	if (res) {
		memset(res, 0, sizeof(struct _tmx_img));
	} else {
		tmx_errno = E_ALLOC;
	}
	return res;
}

tmx_object alloc_object(void) {
	tmx_object res = (tmx_object)malloc(sizeof(struct _tmx_obj));
	if (res) {
		memset(res, 0, sizeof(struct _tmx_obj));
	} else {
		tmx_errno = E_ALLOC;
	}
	return res;
}

tmx_objectgroup alloc_objgrp(void) {
	tmx_objectgroup res = (tmx_objectgroup)malloc(sizeof(struct _tmx_objgrp));
	if (res) {
		memset(res, 0, sizeof(struct _tmx_objgrp));
		res->opacity = 1.0f;
		res->visible = 1;
	} else {
		tmx_errno = E_ALLOC;
	}
	return res;
}

tmx_layer alloc_layer(void) {
	tmx_layer res = (tmx_layer)malloc(sizeof(struct _tmx_layer));
	if (res) {
		memset(res, 0, sizeof(struct _tmx_layer));
		res->opacity = 1.0f;
		res->visible = 1;
	} else {
		tmx_errno = E_ALLOC;
	}
	return res;
}

tmx_tileset alloc_tileset(void) {
	tmx_tileset res = (tmx_tileset)malloc(sizeof(struct _tmx_ts));
	if (res) {
		memset(res, 0, sizeof(struct _tmx_ts));
	} else {
		tmx_errno = E_ALLOC;
	}
	return res;
}

tmx_map alloc_map(void) {
	tmx_map res = (tmx_map)malloc(sizeof(struct _tmx_map));
	if (res) {
		memset(res, 0, sizeof(struct _tmx_map));
	} else {
		tmx_errno = E_ALLOC;
	}
	return res;
}

/*
	Misc
*/

/* "orthogonal" -> ORT */
enum tmx_map_orient parse_orient(const char* orient_str) {
	if (!strcmp(orient_str, "orthogonal")) {
		return T_ORT;
	}
	if (!strcmp(orient_str, "isometric")) {
		return T_ISO;
	}
	return -1;
}

/* "#337FA2" -> 0x337FA2 */
int get_color_rgb(const char *c) {
	return 0; /* TODO */
}

#endif /* WANT_ZLIB */