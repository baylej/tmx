cmake_minimum_required(VERSION 2.8)

#-----------#
# Configure
#-----------#

project(tmx C)

option(INSTALL_TARGET "adds a 'Install' target to the build system ?" on)
option(WANT_ZLIB "use zlib (ability to decompress layers data) ?" on)
option(WANT_XML "use libXml2 (ability to read .tmx maps) ?" on)
option(WANT_JSON "use JSON (ability to read .json maps) ?" on)
set(BUILD_SHARED_LIBS OFF)
set(CMAKE_BUILD_TYPE Release)
set(DEPENDENCIES_DIR "" CACHE PATH "Dependencies root directory, contains include and lib subdirectories")

#-----------#
#    Env
#-----------#

# Search in the `cmake' directory for additional CMake modules.
list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake)

# variable containing sources to compile
set(SOURCES "src/tmx.c" "src/tmx_utils.c" "src/tmx_err.c")

if(DEPENDENCIES_DIR)
    message(STATUS "Adding ${DEPENDENCIES_DIR} to CMAKE_FIND_ROOT_PATH")
    list(APPEND CMAKE_FIND_ROOT_PATH "${DEPENDENCIES_DIR}")
endif(DEPENDENCIES_DIR)

include(CheckIncludeFiles)
CHECK_INCLUDE_FILES("stdint.h" STDINT_H)
if(NOT STDINT_H)
    message(FATAL_ERROR "error: required header stdint.h not found")
endif(NOT STDINT_H)

include(TestBigEndian)
TEST_BIG_ENDIAN(SYS_BE)
if(SYS_BE)
    add_definitions(-DSYS_BIG_ENDIAN)
endif(SYS_BE)

if(WANT_ZLIB)
    add_definitions(-DWANT_ZLIB)
    include(FindZLIB)
    find_package(ZLIB REQUIRED)
    include_directories(${ZLIB_INCLUDE_DIRS})
else(WANT_ZLIB)
    message("Zlib not wanted")
endif(WANT_ZLIB)

if(WANT_XML)
    add_definitions(-DWANT_XML)
    set(SOURCES ${SOURCES} "src/tmx_xml.c")
    include(FindLibXml2)
    find_package(LibXml2 REQUIRED)
    include_directories(${LIBXML2_INCLUDE_DIRS})
else(WANT_XML)
    message("LibXml not wanted")
endif(WANT_XML)

if(WANT_JSON)
    add_definitions(-DWANT_JSON)
    set(SOURCES ${SOURCES} "src/tmx_json.c")
    include(FindJansson)
    find_package(Jansson REQUIRED)
    include_directories(${JANSSON_INCLUDE_DIRS})
else(WANT_JSON)
    message("Json not wanted")
endif(WANT_JSON)

if(WIN32)
    # disable warning on _strncpy (spams the output)
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
endif(WIN32)

#-----------#
#   Build
#-----------#

add_library(${PROJECT_NAME} ${SOURCES})
target_link_libraries(${PROJECT_NAME} ${LIBS})

#-----------#
#  Install
#-----------#

if(INSTALL_TARGET)
install(TARGETS ${PROJECT_NAME} 
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib)
install(DIRECTORY ${PROJECT_SOURCE_DIR}/src/ DESTINATION include/${PROJECT_NAME} FILES_MATCHING PATTERN "tmx.h")
endif(INSTALL_TARGET)

