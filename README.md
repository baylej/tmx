# TMX C Loader

---

## About

Loads **.tmx** tiled maps in you games.

## Dependencies

This project depends on [Zlib](http://zlib.net/) and [LibXml2](http://xmlsoft.org).
Binaries for windows are available :
 * [MSVS 10](http://bayle.jonathan.free.fr/f/deps-msvc10.zip)
 * [MinGW 4.6.2](http://bayle.jonathan.free.fr/f/deps-mingw4_6_2.zip)

## Compiling
This project uses [cmake](http://cmake.org) as a *build system* builder.
You can either use cmake or cmake-gui
You can disable XML support by setting **WANT_XML** to *off*

### Exemple with cmake on linux :

    mkdir build
    cd build
    cmake ..
    make && make install

## Usage

    tmx map = tmx_load("path/map.tmx");
    ...
    tmx_free(map);
