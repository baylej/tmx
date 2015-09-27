# This is a minimal CMakeLists.txt to link with TMX and its dependencies

cmake_minimum_required(VERSION 3.0)

project(dumper VERSION 0.10.1 LANGUAGES C)

# `tmx` package creates two imported targets: `tmx` and `tmx_shared`
# Imported targets requires cmake version 3.0+
find_package(tmx REQUIRED)

add_executable(dumper "dumper.c")

# Links with the static library
target_link_libraries(dumper tmx ZLIB::ZLIB ${LIBXML2_LIBRARIES})

