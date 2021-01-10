cmake_minimum_required(VERSION 3.5)

project(tmx VERSION 1.2.0 LANGUAGES C)

add_subdirectory(doc) # Sphinx documentation

# -- Options

set(API_VERSION "${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}")
set(BUILD_VERSION "${PROJECT_VERSION}")

option(WANT_ZLIB "use zlib (ability to decompress layers data) ?" On)
option(BUILD_SHARED_LIBS "Build shared libraries (dll / so)" Off)

set(EMSCRIPTEN False)
if(CMAKE_SYSTEM_NAME STREQUAL "Emscripten")
  set(EMSCRIPTEN True)
endif()

# -- Target

add_library(tmx
    "src/tmx.c"
    "src/tmx_utils.c"
    "src/tmx_err.c"
    "src/tmx_xml.c"
    "src/tmx_mem.c"
    "src/tmx_hash.c")
set(HEADERS "src/tmx.h")
set_target_properties(tmx PROPERTIES VERSION ${BUILD_VERSION})

# -- Dependencies

include(CheckIncludeFiles)
CHECK_INCLUDE_FILES("stdint.h" STDINT_H)
if(NOT STDINT_H)
    message(FATAL_ERROR "error: required header stdint.h not found")
endif()

if(WANT_ZLIB AND EMSCRIPTEN)
    target_link_options(tmx INTERFACE "SHELL:-s USE_ZLIB=1")
elseif(WANT_ZLIB)
    target_compile_definitions(tmx PRIVATE WANT_ZLIB)
    include(FindZLIB)
    find_package(ZLIB REQUIRED)
    target_link_libraries(tmx ZLIB::ZLIB)
else()
    message("Zlib not wanted")
endif()

include(FindLibXml2)
find_package(LibXml2 REQUIRED)
target_link_libraries(tmx LibXml2::LibXml2)

# -- Build

if(MSVC)
    # disable warning on _strncpy (spams the output)
    target_compile_definitions(tmx PRIVATE _CRT_SECURE_NO_WARNINGS)
endif()

if(BUILD_SHARED_LIBS)
    if(MSVC OR CYGWIN OR MINGW)
        target_compile_definitions(tmx
                                   PRIVATE   "TMXEXPORT=__declspec(dllexport)"
                                   INTERFACE "TMXEXPORT=__declspec(dllimport)")
    endif()

    set_target_properties(tmx PROPERTIES SOVERSION ${API_VERSION})
    set_target_properties(tmx PROPERTIES VERSION ${BUILD_VERSION})
endif()

# --  Installation

target_include_directories(tmx INTERFACE $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/src> $<INSTALL_INTERFACE:include>)

configure_file("tmxConfig.cmake.in" "tmxConfig.cmake" @ONLY)

include(CMakePackageConfigHelpers)
write_basic_package_version_file("tmxConfigVersion.cmake" COMPATIBILITY AnyNewerVersion)

include(GNUInstallDirs)

install(TARGETS tmx
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
