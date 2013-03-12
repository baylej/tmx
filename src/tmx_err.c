#include <stdlib.h>
#include <stdio.h>

#include "tmx.h"
#include "tmx_utils.h"

int tmx_errno = E_NONE;

static char *errmsgs[] = {
	"No error",
	"Memory alloc failed",
	"Missing privileges to access the file",
};

char custom_msg[256];

const char* tmx_strerr(void) {
	char *msg;
	switch(tmx_errno) {
		case E_NONE:   msg = errmsgs[0]; break;
		case E_ALLOC:  msg = errmsgs[1]; break;
		case E_ACCESS: msg = errmsgs[2]; break;
		default: msg = custom_msg;
	}
	return msg;
}

void tmx_perror(const char *pos) {
	const char *msg = tmx_strerr();
	fprintf(stderr, "%s: %s\n", pos, msg);
}
