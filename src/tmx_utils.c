#include <stdlib.h>
#include <stdio.h>
#include <string.h>

/*
	Utility functions
*/

#define T_BIG_ENDIAN 'B'
#define T_LIT_ENDIAN 'L'
static char find_endianness() {
	int num=1;
	if(*(char*)&num==1)
		return T_LIT_ENDIAN;
	return T_BIG_ENDIAN;
}

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
	if (!res) return NULL;
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

	if (src_len%4) {
		return NULL; /* invalid source */
	}

	*rlength = (src_len/4)*3;
	res = (char*) malloc(*rlength);
	if (!res) return NULL;

	for (i=0; i<src_len; i+=4) {
		in = 0;

		for (j=0; j<4; j++) {
			v = b64_value(source[i+j]);
			if (v == -1) {
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
	strm.zalloc = Z_NULL;
	strm.zfree = Z_NULL;
	strm.opaque = Z_NULL;
	strm.next_in = (Bytef*)source;
	strm.avail_in = strlen(source);
	
	if (initial_capacity) {
		*rlength = initial_capacity;
	} else {
		*rlength = slength;
		/**rlength = slength * 2; 50% cmp rate ? */
	}
	res = (char*) malloc(*rlength);
	if (!res) {
		fprintf(stderr, "zlib_decompress(%s:%d): mem alloc returned NULL\n", __FILE__, __LINE__);
		return NULL;
	}

	strm.next_out = (Bytef*)res;
	strm.avail_out = *rlength;
	
	/* 15+32 to enable zlib and gzip decoding with automatic header detection */
	if ((ret=inflateInit2(&strm, 15 + 32)) != Z_OK) {
		fprintf(stderr, "zlib_decompress(%s:%d): inflateInit2 returned %d\n", __FILE__, __LINE__, ret);
		goto cleanup;
	}

	do {
		ret = inflate(&strm, Z_SYNC_FLUSH);
		if (ret == Z_BUF_ERROR) { /* too tiny buffer */
			strm.avail_out = *rlength;
			*rlength *= 2;
			if (!(tmp = (char*)realloc(res, *rlength))) {
				fprintf(stderr, "zlib_decompress(%s:%d): mem alloc returned NULL\n", __FILE__, __LINE__);
				inflateEnd(&strm);
				goto cleanup;
			}
			res = tmp;
		} else if (ret != Z_STREAM_END && ret != Z_OK) {
			fprintf(stderr, "zlib_decompress(%s:%d): inflate returned %d\n", __FILE__, __LINE__, ret);
			inflateEnd(&strm);
			goto cleanup;
		}
	} while (ret != Z_STREAM_END);

	if (strm.avail_in != 0) {
		/* FIXME There is remains in the source */
		fprintf(stderr, "zlib_decompress(%s:%d): remains(%d) in the source\n", __FILE__, __LINE__, strm.avail_in);
	}
	*rlength -= strm.avail_out;
	inflateEnd(&strm);

	return res;
cleanup:
	free(res);
	return NULL;
}