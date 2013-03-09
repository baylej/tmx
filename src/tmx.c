#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "tmx.h"

/* From tmx_utils.c */
extern char* b64_encode(const char* source, unsigned int length);
extern char* b64_decode(const char* source, unsigned int *rlength);
char* zlib_decompress(const char *source, unsigned int slength, unsigned int initial_capacity, unsigned int *rlength);

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

int main(int argc, char *argv[]) {
	unsigned int l, rl;
	char *c1, *c2;
	//FILE *dump;

	/*puts((c=b64_encode("Hello", 5)));
	free(c);*/
	c1 = b64_decode("eJy9ld0KgDAIhbfWVr3/C8dggYh61KiLQ+Dfd2aLeiml/aDONGNjidZVQVq8At58niQ2FKbGt3KalygvK84bH/OawsvOuYi8PK3fmsVzWt0mvD/kk8eQB8RD+0C7spge3rHk8cNruT9rn2iGxHzDQ3vz5iWeZ/fZu+/lRc5pfS8RXuQckfsZ6fVwtdqNMHuCa0nqlf5/j/akkN/JuQE3eggd", &l);
	if (!c1) return -1;
	/*if ((dump=fopen("c1.dump", "wb"))) {
		fwrite(c1, 1, l, dump);
		fclose(dump);
	}*/
	printf("c1[5]=0x%x\n", c1[5]);

	c2 = zlib_decompress(c1, l, 0, &rl);
	if (!c2) return -1;

	printf ("unencoded uncompressed data is %u bytes long\n", rl);

	if (!(rl%4)) {
		printf ("it contains %u ints\n", rl/4u);
	}

	//free(c1);
	free(c2);
	getchar();
	return 0;
}

#endif