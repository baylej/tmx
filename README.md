# TMX C Loader

---

## About

Loads **.tmx** tiled maps in you games.


## Compile
This project uses [cmake](http://cmake.org) as a *build system* builder.
You can either use cmake or cmake-gui

### Exemple with cmake on linux :

    mkdir build
    cd build
    cmake ..
    make && make install

## Usage

    tmx map = tmx_load("path/map.tmx");
    ...
    tmx_free(map);
