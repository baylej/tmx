/*
	JSON Parser using the yajl API
	see http://lloyd.github.com/yajl/
	It uses top-appended linked lists, so if you have several layers
	in your map, your first layer in the json will be the last in the
	linked list.
	This parser is less strict than the XML parser because it does not
	verify the presence of every needed keys.
	Not thread-safe, uses globals
*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#include <yajl/yajl_common.h>
#include <yajl/yajl_parse.h>

#include "tmx.h"
#include "tmx_utils.h"

/* 4kB buffer */
#define BUFFER_LEN 4096

/*
	Memory functions
*/
void * yajl_malloc(void *ctx, size_t size) {
	return tmx_alloc_func(NULL, size);
}

void * yajl_realloc(void *ctx, void *address, size_t size) {
	return tmx_alloc_func(address, size);
}

void yajl_freef(void *ctx, void *address) {
	tmx_free_func(address);
}

/*
	Tree
*/

enum tmx_str_type {
	TMX_NON = 0,
	TMX_MAP,
	TMX_LAY,
	TMX_OBJ,
	TMX_TST,
	TMX_PRP,
	TMX_DAT,
	TMX_PTS,
	J_ARRAY
};

union tmx_str_vals {
	void *addr;
	tmx_map map;
	tmx_layer lay;
	tmx_object obj;
	tmx_tileset tst;
	enum tmx_str_type arraytype;
};

struct _tmx_str {
	enum  tmx_str_type type;
	union tmx_str_vals vals;
};

#define J_MAX_DEPTH 8 /* Maximum possible depth in the JSON */
static int curr_depth = -1; /* Current depth in the JSON document */
static struct _tmx_str ancestry[J_MAX_DEPTH]; /* to remember the actual element's parents @ */
/* ancestry[0] is always a tmx_map object */

static char *last_key = NULL; /* store the map key here and waits for a value */
static tmx_map fres = NULL; /* this value is set at the end of the parsing */

static enum tmx_str_type get_struct_entity(const char *name) {
	if (name) {
		if (!strcmp(name, "tilesets"))
			return TMX_TST;
		if (!strcmp(name, "properties"))
			return TMX_PRP;
		if (!strcmp(name, "layers"))
			return TMX_LAY;
		if (!strcmp(name, "objects"))
			return TMX_OBJ;
		if (!strcmp(name, "map"))
			return TMX_MAP;
		if (!strcmp(name, "data"))
			return TMX_DAT;
		if (!strcmp(name, "polygon") || !strcmp(name, "polyline"))
			return TMX_PTS;
	}
	return TMX_NON;
}

/* DEBUG */
void dump_ancestry() {
	int i;
	printf("ancestry(%d)=\n", curr_depth+1);
	for (i=0; i<=curr_depth; i++) {
		printf("\t[%d]=%d\n", i, ancestry[i].type);
	}
	putchar('\n');
}

/* return NULL if an error occured */
static void* anc_append(enum tmx_str_type type) { /* return NULL if an error occured */
	void *res = NULL;
	if (curr_depth+1 == J_MAX_DEPTH) {
		tmx_err(E_JDATA, "json_parser: max depth overflow");
		return NULL; /* FIXME: error */
	}

	curr_depth++;
	switch(type) {
		case TMX_LAY: res = ancestry[curr_depth].vals.lay = alloc_layer();      break;
		case TMX_OBJ: res = ancestry[curr_depth].vals.obj = alloc_object();     break;
		case TMX_TST: res = ancestry[curr_depth].vals.tst = alloc_tileset();    break;
		case TMX_MAP: res = fres = ancestry[curr_depth].vals.map = alloc_map(); break;
		case J_ARRAY: ancestry[curr_depth].vals.arraytype = get_struct_entity(last_key);
		case TMX_PTS:
		case TMX_PRP: res = (void*)1; break;
		default: puts("error");
	}
	ancestry[curr_depth].type = type;
	dump_ancestry();
	return res;
}

/* void * anc_take() */
#define anc_take(v) ancestry[curr_depth].vals.v

/* enum tmx_str_type anc_take() */
#define anc_type() ancestry[curr_depth].type

/* int anc_drop() (return 0 if error) */
#define anc_drop() (!(curr_depth-- == 0))

/*
	arrays of unknown length
	/!\ JSON files from Tiled have their attributes badly placed
	ex : map.width can be the very last attribute in the document

	layer.data and object.points need this ugly hack
	cb_end_array trim the mem bloc
*/

struct _unkn_larray{
	void *addr;
	unsigned int length, count; /*arrays length VS elements count*/
	union {
		int32_t *gids;
		int *points;
	} val;
} unkn_array;

static int _ularray_appd(struct _unkn_larray *a, void *addr, enum tmx_str_type t, void *v) {
	int32_t *gids = NULL;
	int *points = NULL;

	if (t==TMX_NON || a->addr != addr) {
		a->addr = addr;
		a->count = a->length = 0;
		a->val.gids = NULL;
		if (t==TMX_NON) return 1;
	}

	if (a->count+1 > a->length) {
		if (t == TMX_DAT) {
			a->length += 500; /* reallocs 500 more tiles */
			gids = (int32_t*)tmx_alloc_func(a->val.gids, a->length*sizeof(int32_t));
			if (gids && gids != a->val.gids) a->val.gids = gids;
		} else if (t == TMX_PTS) {
			a->length += 5; /* realloc 5 more points */
			points = (int*)tmx_alloc_func(a->val.points, a->length*2*sizeof(int));
			if (points && points != a->val.points) a->val.points = points;
		}
		if (gids==NULL && points==NULL) {
			tmx_errno = E_ALLOC;
			return 0;
		}
	}

	if (t == TMX_DAT) {
		memmove(a->val.gids+a->count, v, sizeof(int32_t));
	} else if (t == TMX_PTS) {
		memmove(a->val.points+a->count*2, v, 2*sizeof(int));
	}
	a->count++;

	return 1;
}

static int _ularray_trim(struct _unkn_larray *a, enum tmx_str_type t) {
	int32_t *gids = NULL;
	int *points = NULL;

	if (t == TMX_DAT && a->val.gids) {
		gids = (int32_t*)tmx_alloc_func(a->val.gids, a->count);
		if (gids && gids != a->val.gids) a->val.gids = gids;
	} else if (t == TMX_PTS && a->val.points) {
		points = (int*)tmx_alloc_func(a->val.points, a->count*2);
		if (points && points != a->val.points) a->val.points = points;
	}
	return (gids!=NULL || points!=NULL);
}

/*
	Setters
	parses the key and set the value in the right field
	return 0 if an error occured
*/

static int set_property(tmx_property *propaddr, const char *value, size_t value_len) {
	tmx_property p;

	if (!(p = alloc_prop())) {
		tmx_errno = E_ALLOC;
		return 0;
	}

	p->name  = tmx_strdup((char*)last_key);
	p->value = tmx_strndup((char*)value, value_len);

	if (!p->name || !p->value) goto cleanup;

	p->next = *propaddr;
	*propaddr = p;

	return 1;
cleanup:
	tmx_free_func(p->name);
	tmx_free_func(p->value);
	tmx_free_func(p);
	return 0;
}

static int set_layer(tmx_layer layeraddr, const char *value, size_t value_len) {
	int v;
	if (!strcmp(last_key, "name")) {
		layeraddr->name = tmx_strndup((char*)value, value_len);
		if (!(layeraddr->name)) {
			tmx_errno = E_ALLOC;
			return 0;
		}
	} else if (!strcmp(last_key, "color")) { 
		layeraddr->color = get_color_rgb(value);
	} else if (!strcmp(last_key, "type")) { 
		if (!strncmp(value, "tilelayer", value_len))
			layeraddr->type = L_LAYER;
		else if (!strncmp(value, "objectgroup", value_len))
			layeraddr->type = L_OBJGR;
	} else if (!strcmp(last_key, "opacity")) { 
		layeraddr->opacity = atof(value);
	} else if (!strcmp(last_key, "visible")) { 
		layeraddr->visible = (value[0] == 't');
	}
	else if (!strcmp(last_key, "data")) { 
		v = atoi(value);
		_ularray_appd(&unkn_array, layeraddr, TMX_DAT, &v);
	}

	return 1;
}

static int set_tileset(tmx_tileset tilesetaddr, const char *value, size_t value_len) {
	if (!(tilesetaddr->image)) {
		if (!(tilesetaddr->image = alloc_image())) return 0;
	}

	if (!strcmp(last_key, "name")) {
		tilesetaddr->name = tmx_strndup((char*)value, value_len);
		if (!(tilesetaddr->name)) {
			tmx_errno = E_ALLOC;
			return 0;
		}
	} else if (!strcmp(last_key, "image")) {
		tilesetaddr->image->source = tmx_strndup((char*)value, value_len);
		if (!(tilesetaddr->image->source)) {
			tmx_errno = E_ALLOC;
			return 0;
		}
	} else if (!strcmp(last_key, "imageheight")) {
		tilesetaddr->image->height = atoi(value);
	} else if (!strcmp(last_key, "imagewidth")) { /* TODO: image.trans */
		tilesetaddr->image->width = atoi(value);
	} else if (!strcmp(last_key, "firstgid")) {
		tilesetaddr->firstgid = atoi(value);
	} else if (!strcmp(last_key, "tileheight")) {
		tilesetaddr->tile_height = atoi(value);
	} else if (!strcmp(last_key, "tilewidth")) {
		tilesetaddr->tile_width = atoi(value);
	} else if (!strcmp(last_key, "spacing")) {
		tilesetaddr->spacing = atoi(value);
	} else if (!strcmp(last_key, "margin")) {
		tilesetaddr->margin = atoi(value);
	} /* TODO: offset X,Y */
	return 1;
}

static int set_point(tmx_object o, const char *value, size_t value_len) {
	static int v[2];
	static char x_s=0, y_s=0;

	switch (last_key[0]) {
		case 'x': x_s=1; v[0]=atoi(value); break;
		case 'y': y_s=1; v[1]=atoi(value); break;
		default: return 0;
	}

	if (x_s && y_s) {
		_ularray_appd(&unkn_array, o, TMX_PTS, v);
		x_s = y_s = 0;
	}

	return 1;
}

static int set_object(tmx_object objectaddr, const char *value, size_t value_len) {
	if (!strcmp(last_key, "name") && value_len > 0) {
		objectaddr->name = tmx_strndup((char*)value, value_len);
		if (!(objectaddr->name)) {
			tmx_errno = E_ALLOC;
			return 0;
		}
	} else if (!strcmp(last_key, "y")) {
		objectaddr->y = atoi(value);
	} else if (!strcmp(last_key, "x")) {
		objectaddr->x = atoi(value);
	} else if (!strcmp(last_key, "height")) {
		objectaddr->height = atoi(value);
	} else if (!strcmp(last_key, "width")) {
		objectaddr->width = atoi(value);
	}

	return 1;
}

static int set_map(tmx_map mapaddr, const char *value, size_t value_len) {
	char orient_s[11];
	enum tmx_map_orient orient_v;

	if (!strcmp(last_key, "orientation")) {
		if (value_len > 10) {
			tmx_err(E_JDATA, "parse_json: invalid orientation");
			return 0;
		}
		memcpy(orient_s, value, value_len);
		orient_s[value_len] = '\0';
		orient_v = parse_orient(orient_s);
		if (orient_v == O_NONE) {
			tmx_err(E_JDATA, "parse_json: unsupported orientation %s", orient_s);
			return 0;
		}
		mapaddr->orient = orient_v;
	} else if (!strcmp(last_key, "height")) {
		mapaddr->height = atoi(value);
	} else if (!strcmp(last_key, "width")) {
		mapaddr->width = atoi(value);
	} else if (!strcmp(last_key, "tileheight")) {
		mapaddr->tile_height = atoi(value);
	} else if (!strcmp(last_key, "tilewidth")) {
		mapaddr->tile_width = atoi(value);
	}
	return 1;
}

static int set(const char *value, size_t value_len) {
	int i, res;

	i = curr_depth;
	if (ancestry[i].type == J_ARRAY) { /* get the type of the array */
		if (--i == -1) {
			tmx_err(E_UNKN, "json_parser: wrong type of root element (array)");
			return 0;
		}
	}
	if (ancestry[i].type == TMX_PTS) {
		if (i-2 != -1 && ancestry[i-2].type == TMX_OBJ) {
			res = set_point(ancestry[i-2].vals.obj, value, value_len);
		}
	} else if (ancestry[i].type == TMX_PRP) { /* properties */
		if (--i == -1) {
			tmx_err(E_UNKN, "json_parser: wrong type of root element (property)");
			return 0;
		}
		switch(ancestry[i].type) {
			case TMX_MAP: res = set_property(&(ancestry[i].vals.map->properties), value, value_len); break;
			case TMX_LAY: res = set_property(&(ancestry[i].vals.lay->properties), value, value_len); break;
			case TMX_OBJ: res = set_property(&(ancestry[i].vals.obj->properties), value, value_len); break;
			//case TMX_TST: res = set_property(&(ancestry[i].vals.tst->properties), value, value_len); break; /* FIXME */
			default: tmx_err(E_JDATA, "json_parser: unallowed 'properties' in an element of type %d", ancestry[i].type);
			         res = 0;
		}
	} else {

		switch(ancestry[i].type) {
			case TMX_MAP: res = set_map    (ancestry[i].vals.map, value, value_len); break;
			case TMX_LAY: res = set_layer  (ancestry[i].vals.lay, value, value_len); break;
			case TMX_OBJ: res = set_object (ancestry[i].vals.obj, value, value_len); break;
			case TMX_TST: res = set_tileset(ancestry[i].vals.tst, value, value_len); break;
			default: tmx_err(E_JDATA, "json_parser: cannot set values in an element of type %d", ancestry[i].type);
			         res = 0;
		}
	}
	return res;
}

/*
	Chaining functions
	the linked-list part of the data structure
*/

static void chain_tileset(tmx_tileset ts) {
	tmx_map map = ancestry[0].vals.map;

	ts->next = map->ts_head;
	map->ts_head = ts;
}

static void chain_layer(tmx_layer ly) {
	tmx_layer *lyadr = &(ancestry[0].vals.map->ly_head);

	/* tail-chain */
	if (!(*lyadr)) {
		(*lyadr) = ly;
	} else {
		while((*lyadr)->next) {
			lyadr = &((*lyadr)->next);
		}

		(*lyadr)->next = ly;
	}
}

static int chain_object(tmx_object o, tmx_layer parent) {
	if (parent->type == L_NONE) {
		parent->type = L_OBJGR;
	}
	if (parent->type != L_OBJGR) {
		tmx_err(E_INVAL, "json_parser: invalid parameter, layer is not of type objectgroup");
		return 0;
	}
	o->next = parent->content.head;
	parent->content.head = o;
	return 1;
}

/*
	Callback functions
	called by the event-driven JSON parser
	return 0 if an error occured
*/

static int cb_boolean(void * ctx, int boolVal) {
	char b_val;
	b_val = boolVal ? 't': 'f';
	return set(&b_val, 1);
}

static int cb_number(void * ctx, const char *numberVal, size_t numberLen) {
	return set(numberVal, numberLen);
}

static int cb_string(void * ctx, const unsigned char *stringVal, size_t stringLen) {
	return set(stringVal, stringLen);
}

/* raised if the right-value is '{' */
static int cb_start_map(void * ctx) {
	if (anc_type() == J_ARRAY) { /* if the parent is an array */
		printf("new obj of type %d\n", anc_take(arraytype));
		return (anc_append(anc_take(arraytype)) != NULL); /* alloc with the type of the array */
	} else {
		printf("new obj: %s\n", last_key); /* DEBUG */
		return (anc_append(get_struct_entity(last_key)) != NULL);
	}
}

/* raised by a left-value : it's a key */
static int cb_map_key(void * ctx, const unsigned char *key, size_t stringLen) {
	tmx_free_func(last_key);
	last_key = tmx_strndup((char*)key, stringLen);
	return (last_key != NULL);
}

/* raised by a '}' (end of an object) */
static int cb_end_map(void * ctx) {
	void *val = (void*)anc_take(map);
	enum tmx_str_type type = anc_type();

	anc_drop();

	/* chain the current object to it's parent */
	if (type == TMX_LAY) {
		chain_layer((tmx_layer)val);
	} else if (type == TMX_TST) {
		chain_tileset((tmx_tileset)val);
	} else if (type == TMX_PRP || type == TMX_PTS) {
		;/* Nothing to do */
	} else if (type == TMX_OBJ && anc_type()==J_ARRAY) {
		chain_object((tmx_object)val, ancestry[curr_depth-1].vals.lay);
	} else if (type != TMX_MAP) {
		tmx_err(E_UNKN, "json_parser: unexpected element of type %d at map end", type);
		return 0;
	}

	return 1;
}

/* raised if the right-value is '[' */
static int cb_start_array(void * ctx) {
	return (anc_append(J_ARRAY) != NULL);
}

/* raised by a ']' */
static int cb_end_array(void * ctx) { /* FIXME : make functions */
	unsigned int i;
	if (anc_take(arraytype) == TMX_DAT) {
		_ularray_trim(&unkn_array, TMX_DAT);
		((tmx_layer)unkn_array.addr)->content.gids = unkn_array.val.gids;
	} else if (anc_take(arraytype) == TMX_PTS) {
		_ularray_trim(&unkn_array, TMX_PTS);
		((tmx_object)unkn_array.addr)->points_len = unkn_array.count;
		((tmx_object)unkn_array.addr)->points = tmx_alloc_func(NULL, sizeof(int*));
		if (!(((tmx_object)unkn_array.addr)->points)) {
			tmx_errno = E_ALLOC;
			return 0;
		}
		((tmx_object)unkn_array.addr)->points[0] = unkn_array.val.points;
		printf("array has %d points", unkn_array.count);
		for (i=1; i<unkn_array.count; i++) {
			((tmx_object)unkn_array.addr)->points[i] = ((tmx_object)unkn_array.addr)->points[0]+(i*2);
		}
		_ularray_appd(&unkn_array, NULL, TMX_NON, NULL);
	}
	return anc_drop();
}

/*
	Public function
*/

tmx_map parse_json(const char *filename) {
	yajl_alloc_funcs mem_f;
	yajl_callbacks cb_f;
	yajl_handle handle = NULL;
	yajl_status ret;
	char *yajl_errstr;

	FILE *file = NULL;
	unsigned char buffer[BUFFER_LEN];
	size_t red;

	//if (!(res = (tmx_map)anc_append(TMX_MAP))) return NULL;
	if (!(last_key = (char*)tmx_alloc_func(NULL, 4))) {
		tmx_errno = E_ALLOC;
		return NULL;
	}
	sprintf(last_key, "map");

	/* set memory alloc/free functions pointers */
	mem_f.ctx = NULL;
	mem_f.malloc = yajl_malloc;
	mem_f.realloc = yajl_realloc;
	mem_f.free = yajl_freef;

	/* set callback functions */
	cb_f.yajl_integer = NULL;
	cb_f.yajl_double = NULL;
	cb_f.yajl_null = NULL;
	cb_f.yajl_boolean = cb_boolean;
	cb_f.yajl_number = cb_number;
	cb_f.yajl_string = cb_string;
	cb_f.yajl_start_array = cb_start_array;
	cb_f.yajl_end_array = cb_end_array;
	cb_f.yajl_start_map = cb_start_map;
	cb_f.yajl_map_key = cb_map_key;
	cb_f.yajl_end_map = cb_end_map;

	if (!(handle = yajl_alloc(&cb_f, &mem_f, NULL))) {
		yajl_errstr = (char*)yajl_get_error(handle, 0, NULL, 0);
		tmx_err(E_UNKN, yajl_errstr);
		yajl_free_error(handle, (unsigned char*)yajl_errstr);
		goto cleanup;
	}
	yajl_config(handle, yajl_dont_validate_strings, 1); /* disable utf8 checking */

	/* Open and parse the file */
	if (!(file = fopen(filename, "r"))) {
		if (errno == EACCES) {
			tmx_errno = E_ACCESS;
		} else if (errno == ENOENT) {
			tmx_errno = E_NOENT;
		} else {
			tmx_err(E_UNKN, strerror(errno));
		}
		goto cleanup;
	}

	while (red = fread(buffer, 1, BUFFER_LEN, file), red > 0) {
		
		ret = yajl_parse(handle, buffer, red);
		if (ret == yajl_status_client_canceled) goto cleanup;
		if (ret == yajl_status_error) {
			yajl_errstr = (char*)yajl_get_error(handle, 0, NULL, 0);
			tmx_err(E_JDATA, yajl_errstr);
			yajl_free_error(handle, (unsigned char*)yajl_errstr);
			goto cleanup;
		}
	}

	ret = yajl_complete_parse(handle);
	if (ret == yajl_status_client_canceled) goto cleanup;
	if (ret == yajl_status_error) {
		yajl_errstr = (char*)yajl_get_error(handle, 0, NULL, 0);
		tmx_err(E_JDATA, yajl_errstr);
		yajl_free_error(handle, (unsigned char*)yajl_errstr);
		goto cleanup;
	}

	/* call cleanup function */
	yajl_free(handle);
	fclose(file);
	tmx_free_func(last_key);

	return fres;
cleanup:
	yajl_free(handle);
	tmx_free(&fres);
	fclose(file);
	return NULL;
}
