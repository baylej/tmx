#ifndef TMX_XML_H
#define TMX_XML_H

#ifdef __cplusplus
extern "C" {
#endif

#include "tmx.h"

#include <libxml/xmlreader.h>

/*
   - Loaders -
	 This functions override the default XML loaders if you want to use an external filesystem instead.
 */
TMXEXPORT extern xmlTextReaderPtr (*tmx_xml_reader_load_func) (const char *path, const char *encoding, int options);
TMXEXPORT extern void  (*tmx_xml_reader_free_func) (xmlTextReaderPtr reader);

#ifdef __cplusplus
}
#endif

#endif
