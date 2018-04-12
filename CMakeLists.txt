cmake_minimum_required(VERSION 3.0)

#-----------#
# Configure
#-----------#

project(tmx VERSION 1.0.0 LANGUAGES C)
set(API_VERSION "${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}")
set(BUILD_VERSION "${PROJECT_VERSION}")

option(WANT_ZLIB "use zlib (ability to decompress layers data) ?" on)
option(BUILD_SHARED_LIBS "Build shared libraries (dll / so)" off)

#-----------#
#    Env
#-----------#

set(SOURCES
    "src/tmx.c"
    "src/tmx_utils.c"
    "src/tmx_err.c"
    "src/tmx_xml.c"
    "src/tmx_mem.c"
    "src/tmx_hash.c")
set(HEADERS "src/tmx.h")

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
    list(APPEND libs ${ZLIB_LIBRARIES})
else(WANT_ZLIB)
    message("Zlib not wanted")
endif(WANT_ZLIB)

include(FindLibXml2)
find_package(LibXml2 REQUIRED)
include_directories(${LIBXML2_INCLUDE_DIR})
list(APPEND libs ${LIBXML2_LIBRARIES})

if(MSVC)
    # disable warning on _strncpy (spams the output)
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
endif(MSVC)

configure_file("tmxConfig.cmake.in" "tmxConfig.cmake" @ONLY)

include(CMakePackageConfigHelpers)
write_basic_package_version_file("tmxConfigVersion.cmake" COMPATIBILITY SameMajorVersion)

#-----------#
#   Build
#-----------#

list(APPEND targets tmx)

add_library(tmx STATIC ${SOURCES})
set_target_properties(tmx PROPERTIES VERSION ${BUILD_VERSION})

set_property(TARGET tmx
             APPEND PROPERTY INTERFACE_INCLUDE_DIRECTORIES
             $<INSTALL_INTERFACE:include>)

if(BUILD_SHARED_LIBS)
    list(APPEND targets tmx_shared)

    add_library(tmx_shared SHARED ${SOURCES})
    target_link_libraries(tmx_shared PRIVATE ${libs})

    if(MSVC OR CYGWIN OR MINGW)
        target_compile_definitions(tmx_shared
                                   PRIVATE   "TMXEXPORT=__declspec(dllexport)"
                                   INTERFACE "TMXEXPORT=__declspec(dllimport)")
    endif()

    set_target_properties(tmx_shared PROPERTIES OUTPUT_NAME tmx)
    set_target_properties(tmx_shared PROPERTIES SOVERSION ${API_VERSION})
    set_target_properties(tmx_shared PROPERTIES VERSION ${BUILD_VERSION})

    set_property(TARGET tmx_shared
                 APPEND PROPERTY INTERFACE_INCLUDE_DIRECTORIES
                 $<INSTALL_INTERFACE:include>)
endif(BUILD_SHARED_LIBS)

#-----------#
#  Install
#-----------#

include(GNUInstallDirs)

install(TARGETS ${targets}
        EXPORT tmx_exports
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})
install(FILES ${HEADERS} DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

install(EXPORT tmx_exports
        FILE "tmxExports.cmake"
        DESTINATION "lib/cmake/tmx")
install(FILES "${CMAKE_BINARY_DIR}/tmxConfig.cmake" "${CMAKE_BINARY_DIR}/tmxConfigVersion.cmake"
        DESTINATION "lib/cmake/tmx")
