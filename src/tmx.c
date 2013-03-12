#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h> /* int32_t */

#include "tmx.h"
#include "tmx_utils.h"

/*
	Public functions
*/

tmx_map tmx_load(const char * path) {
	/*
		TODO
		find wich parser to use (XML or JSON)
		Decode and decompress data
		fix data if SYS_BIG_ENDIAN (windows is always little endian) use <endian.h> on posix systems
	*/
	return NULL;
}

static void free_ts(tmx_tileset ts) {
	if (ts->next) free_ts(ts->next);
	free(ts->name);
	free(ts->image);
	free(ts);
}

void tmx_free(tmx_map *map) {
	/* TODO */
	if ((*map)->ts_head)
		free_ts((*map)->ts_head);
	free(*map);
	*map = NULL;
}
