cmake_minimum_required(VERSION 3.0)

project(raylib_example VERSION 1.0.0 LANGUAGES C)

find_package(tmx REQUIRED)
find_package(raylib REQUIRED)
# The following libs should be transitively imported by raylib...
find_package(Threads REQUIRED)
find_package(X11 REQUIRED)

add_executable(raylib_example "raylib.c")
target_link_libraries(raylib_example PRIVATE tmx raylib Threads::Threads ZLIB::ZLIB ${LIBXML2_LIBRARIES} X11::X11)
