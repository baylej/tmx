/*
	TSX.H - TSX C LOADER
	Copyright (c) 2017, Bayle Jonathan <baylej@github>

	Functions to load tileset (.tsx) files in a tileset manager

	Optional functionality
	Include <tmx.h> first
*/

#pragma once

#ifndef TSX_H
#define TSX_H

#ifdef __cplusplus
extern "C" {
#endif

/*
	Tileset Manager functions
*/

/* Tileset Manager type (private hashtable) */
typedef void tmx_tileset_manager;

/* Creates a Tileset Manager that holds a hashtable of loaded tilesets
   Only external tilesets (in .TSX files) are indexed in a tileset manager
   This is particularly useful to only load once tilesets needed by many maps
   The key is the `source` attribute of a tileset element */
TMXEXPORT tmx_tileset_manager* tmx_make_tileset_manager();

/* Frees the tilesetManager and all its loaded Tilesets
   All maps holding a pointer to external tileset loaded by the given manager
   now hold a pointer to freed memory */
TMXEXPORT void tmx_free_tileset_manager(tmx_tileset_manager *ts_mgr);

/* Loads a tileset from file at `path` and stores it into given tileset manager
   `path` will be used as the key
   Returns 1 on success */
TMXEXPORT int tmx_load_tileset(tmx_tileset_manager *ts_mgr, const char *path);

/* Loads a tileset from a buffer and stores it into given tileset manager
   Returns 1 on success */
TMXEXPORT int tmx_load_tileset_buffer(tmx_tileset_manager *ts_mgr, const char *buffer, int len, const char *key);

/* Loads a tileset from a file descriptor and stores it into given tileset manager
   The file descriptor will not be closed
   Returns 1 on success */
TMXEXPORT int tmx_load_tileset_fd(tmx_tileset_manager *ts_mgr, int fd, const char *key);

/* Loads a tileset using the given read callback and stores it into given tileset manager
   Returns 1 on success */
TMXEXPORT int tmx_load_tileset_callback(tmx_tileset_manager *ts_mgr, tmx_read_functor callback, void *userdata, const char *key);

/*
	Load map using a Tileset Manager
*/

/* Same as tmx_load (tmx.h) but with a Tileset Manager. */
TMXEXPORT tmx_map* tmx_tsmgr_load(tmx_tileset_manager *ts_mgr, const char *path);

/* Same as tmx_load_buffer (tmx.h) but with a Tileset Manager. */
TMXEXPORT tmx_map* tmx_tsmgr_load_buffer(tmx_tileset_manager *ts_mgr, const char *buffer, int len);

/* Same as tmx_load_fd (tmx.h) but with a Tileset Manager. */
TMXEXPORT tmx_map* tmx_tsmgr_load_fd(tmx_tileset_manager *ts_mgr, int fd);

/* Same as tmx_load_callback (tmx.h) but with a Tileset Manager. */
TMXEXPORT tmx_map* tmx_tsmgr_load_callback(tmx_tileset_manager *ts_mgr, tmx_read_functor callback, void *userdata);

#ifdef __cplusplus
}
#endif

#endif /* TSX_H */
