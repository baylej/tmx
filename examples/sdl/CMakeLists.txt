cmake_minimum_required(VERSION 3.1)

project(sdl_example VERSION 1.0.0 LANGUAGES C)

if(MSVC)
  message(FATAL_ERROR "MSVC not currently supported")
endif()

find_package(tmx REQUIRED)
find_package(PkgConfig REQUIRED)
#pkg_search_module(SDL2 REQUIRED IMPORTED_TARGET sdl2 SDL2_image) does not work for unknown reasons
pkg_search_module(SDL2_IMAGE REQUIRED IMPORTED_TARGET SDL2_image) # transitively includes SDL2

add_executable(sdl_example "sdl.c")
target_link_libraries(sdl_example tmx PkgConfig::SDL2_IMAGE ZLIB::ZLIB ${LIBXML2_LIBRARIES})
