cmake_minimum_required(VERSION 3.1)

project(al5_example VERSION 1.0.0 LANGUAGES C)

if(MSVC)
  message(FATAL_ERROR "MSVC not currently supported")
endif()

find_package(tmx REQUIRED)

find_package(PkgConfig REQUIRED)
pkg_search_module(ALLEGRO5 REQUIRED allegro-5)
pkg_search_module(ALLEGRO5_IMAGE REQUIRED allegro_image-5)
pkg_search_module(ALLEGRO5_PRIM REQUIRED allegro_primitives-5)

add_executable(al5_example "allegro.c")
target_include_directories(al5_example PRIVATE ${ALLEGRO5_INCLUDE_DIRS})
target_link_libraries(al5_example PRIVATE tmx ${ALLEGRO5_IMAGE_LIBRARIES} ${ALLEGRO5_PRIM_LIBRARIES} ZLIB::ZLIB ${LIBXML2_LIBRARIES})
