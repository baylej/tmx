# TMX C Loader

---

## About

A portable C library to load [tiled](http://mapeditor.org) maps in your games.

## Dependencies

This project depends on [Zlib](http://zlib.net/) and [LibXml2](http://xmlsoft.org).

## Compiling

This project uses [cmake](http://cmake.org) as a *build system* builder.
You can either use cmake, ccmake or cmake-gui.

### Example :

    mkdir build
    cd build
    cmake ..
    make && make install

## Usage

```c
#include <tmx.h>

int main(void) {
  tmx_map *map = tmx_load("path/map.tmx");
  if (!map) {
    tmx_perror("tmx_load");
    return 1;
  }
  /* ... */
  tmx_map_free(map);
  return 0;
}
```

See the dumper example (`examples/dumper/dumper.c`) for an in-depth usage of TMX.

### Help

See the [Wiki](https://github.com/baylej/tmx/wiki/).
