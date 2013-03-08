#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "tmx.h"

/*
	Utility functions
*/

#define BIG_ENDIAN 'B'
#define LIT_ENDIAN 'L'
static char find_endianness() {
	int num=1;
	if(*(char*)&num==1)
		return LIT_ENDIAN;
	return BIG_ENDIAN;
}

/* BASE 64 */

static const char b64enc[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ" "abcdefghijklmnopqrstuvwxyz" "0123456789" "+/";

static char * b64_encode(const char* source, unsigned int length) {
	unsigned int i, mlen, r_pos;
	unsigned short dif, j;
	unsigned int frame = 0;
	char out[5];
	char *res;

	mlen = 4 * length/3 + 1; /* +1 : returns a null-terminated string */
	if (length%3) {
		mlen += 4;
	}

	res = (char*) malloc(mlen * sizeof(char));
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
			out[j] = b64enc[out[j]];
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

static char * b64_decode(const char* source) { /* NULL terminated string */
	return NULL;
}

/* returns true if c belongs to b64enc */
static short is_b64(char c) {
	if (c>='A' && c<='Z' || c>='a' && c<='z' || c>='0' && c<='9' || c=='+' || c=='/')
		return 1;
	return 0;
}

/*
	Public functions
*/

tmx_map tmx_load(const char * path) {
	/* TODO */
	return NULL;
}

void tmx_free(tmx_map map) {
	/* TODO */
}

/*
	Tests
*/

#ifdef DEBUG

int main(void) {
	puts(b64_encode("Hi!", 3));
	/*puts(b64_decode("SGkh"));*/
	system("pause");
	return 0;	
}

#endif