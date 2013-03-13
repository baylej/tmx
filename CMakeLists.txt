cmake_minimum_required(VERSION 2.8)

#-----------#
# Configure
#-----------#

project(tmx C)

option(INSTALL_TARGET "adds a 'Install' target to the build system ?" off)
option(WANT_ZLIB "use zlib (ability to decompress layers data) ?" on)
option(WANT_XML "use libXml2 (ability to read .tmx maps) ?" on)
option(WANT_JSON "use JSON (ability to read .json maps) ?" on)
SET(BUILD_SHARED_LIBS OFF)
SET(CMAKE_BUILD_TYPE Release)

#-----------#
#    Env
#-----------#

set(SOURCES "src/tmx.c" "src/tmx_utils.c" "src/tmx_err.c")

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
    set(LIBS ${LIBS} ${ZLIB_LIBRARIES})
else(WANT_ZLIB)
    message("Zlib not wanted")
endif(WANT_ZLIB)

if(WANT_XML)
    add_definitions(-DWANT_XML)
    set(SOURCES ${SOURCES} "src/tmx_xml.c")
    include(FindLibXml2)
    find_package(LibXml2 REQUIRED)
    include_directories(${LIBXML2_INCLUDE_DIRS})
    set(LIBS ${LIBS} ${LIBXML2_LIBRARIES})
else(WANT_XML)
    message("LibXml not wanted")
endif(WANT_XML)

if(WANT_JSON)
    add_definitions(-DWANT_JSON)
    list(APPEND ${SOURCES} "src/tmx_xml.c")
    # TODO
else(WANT_ZLIB)
    message("Json not wanted")
endif(WANT_JSON)

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
endif()

