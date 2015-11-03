# This is a minimal CMakeLists.txt to link with TMX and its dependencies

cmake_minimum_required(VERSION 3.0)

project(dumper VERSION 0.10.1 LANGUAGES C)

# `tmx` package creates two imported targets: `tmx` and `tmx_shared`
# Imported targets requires cmake version 3.0+
find_package(tmx REQUIRED)

# get SDL2 libs and include
set(SDL2_CONFIG_EXECUTABLE sdl2-config)
execute_process (
  COMMAND ${SDL2_CONFIG_EXECUTABLE} --libs
  OUTPUT_VARIABLE LIBSDL2_LIBRARIES OUTPUT_STRIP_TRAILING_WHITESPACE
  )
execute_process (
  COMMAND ${SDL2_CONFIG_EXECUTABLE} --cflags
  OUTPUT_VARIABLE LIBSDL2_CFLAGS OUTPUT_STRIP_TRAILING_WHITESPACE
  )

set(CMAKE_C_FLAGS ${LIBSDL2_CFLAGS})

add_executable(sdl "sdl.c")

# Links with the static library
target_link_libraries(sdl tmx ZLIB::ZLIB ${LIBXML2_LIBRARIES} ${LIBSDL2_LIBRARIES} -lSDL2_image)

