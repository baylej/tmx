cmake_minimum_required(VERSION 2.8)

#-----------#
# Configure
#-----------#

project(tmx C)
set(API_VERSION "0.10")
set(BUILD_VERSION "0.10.1")

option(WANT_ZLIB "use zlib (ability to decompress layers data) ?" on)
option(WANT_XML "use libXml2 (ability to read .tmx maps) ?" on)
option(WANT_JSON "use JSON (ability to read .json maps) ?" on)
option(BUILD_SHARED_LIBS "Build shared libraries (dll / so)" off)

#-----------#
#    Env
#-----------#

# Search in the `cmake' directory for additional CMake modules.
list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake")

# variable containing sources to compile
set(SOURCES "src/tmx.c" "src/tmx_utils.c" "src/tmx_err.c" "src/tmx_xml.c" "src/tmx_json.c")

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
    include_directories(${ZLIB_INCLUDE_DIR})
    list(APPEND libs ${ZLIB_LIBRARY})
else(WANT_ZLIB)
    message("Zlib not wanted")
endif(WANT_ZLIB)

if(WANT_XML)
    add_definitions(-DWANT_XML)
    include(FindLibXml2)
    find_package(LibXml2 REQUIRED)
    include_directories(${LIBXML2_INCLUDE_DIR})
    list(APPEND libs ${LIBXML2_LIBRARIES})
else(WANT_XML)
    message("LibXml not wanted")
endif(WANT_XML)

if(WANT_JSON)
    add_definitions(-DWANT_JSON)
    include(FindJansson)
    find_package(Jansson REQUIRED)
    include_directories(${JANSSON_INCLUDE_DIR})
    list(APPEND libs ${JANSSON_LIBRARY})
else(WANT_JSON)
    message("Json not wanted")
endif(WANT_JSON)

if(MSVC)
    # disable warning on _strncpy (spams the output)
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
endif(MSVC)

#-----------#
#   Build
#-----------#

list(APPEND targets ${PROJECT_NAME})

add_library(${PROJECT_NAME} STATIC ${SOURCES})
set_target_properties(${PROJECT_NAME} PROPERTIES VERSION ${BUILD_VERSION})

if(BUILD_SHARED_LIBS)
  list(APPEND targets ${PROJECT_NAME}_shared)

  add_library(${PROJECT_NAME}_shared SHARED ${SOURCES})
  target_link_libraries(${PROJECT_NAME}_shared ${libs})

  set_target_properties(${PROJECT_NAME}_shared PROPERTIES OUTPUT_NAME ${PROJECT_NAME})
  set_target_properties(${PROJECT_NAME}_shared PROPERTIES SOVERSION ${API_VERSION})
  set_target_properties(${PROJECT_NAME}_shared PROPERTIES VERSION ${BUILD_VERSION})
endif(BUILD_SHARED_LIBS)

#-----------#
#  Install
#-----------#

install(TARGETS ${targets}
        RUNTIME DESTINATION "bin"
        LIBRARY DESTINATION "lib"
        ARCHIVE DESTINATION "lib")
install(FILES "src/tmx.h" DESTINATION "include")
