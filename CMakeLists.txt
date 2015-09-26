cmake_minimum_required(VERSION 3.0)

#-----------#
# Configure
#-----------#

project(tmx VERSION 0.11.0 LANGUAGES C)
set(API_VERSION "${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}")
set(BUILD_VERSION "${PROJECT_VERSION}")

option(WANT_ZLIB "use zlib (ability to decompress layers data) ?" on)
option(BUILD_SHARED_LIBS "Build shared libraries (dll / so)" off)

#-----------#
#    Env
#-----------#

set(SOURCES "src/tmx.c" "src/tmx_utils.c" "src/tmx_err.c" "src/tmx_xml.c")
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
    list(APPEND libs ${ZLIB_LIBRARY})
else(WANT_ZLIB)
    message("Zlib not wanted")
endif(WANT_ZLIB)

add_definitions(-DWANT_XML)
include(FindLibXml2)
find_package(LibXml2 REQUIRED)
include_directories(${LIBXML2_INCLUDE_DIR})
list(APPEND libs ${LIBXML2_LIBRARIES})

if(MSVC)
    # disable warning on _strncpy (spams the output)
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
endif(MSVC)

configure_file("${PROJECT_NAME}Config.cmake.in" "${PROJECT_NAME}Config.cmake" @ONLY)

include(CMakePackageConfigHelpers)
write_basic_package_version_file("${PROJECT_NAME}ConfigVersion.cmake" COMPATIBILITY SameMajorVersion)

#-----------#
#   Build
#-----------#

list(APPEND targets ${PROJECT_NAME})

add_library(${PROJECT_NAME} STATIC ${SOURCES})
set_target_properties(${PROJECT_NAME} PROPERTIES VERSION ${BUILD_VERSION})
target_link_libraries(${PROJECT_NAME} ${libs})
target_compile_definitions(${PROJECT_NAME} PUBLIC "TMXEXPORT=")

set_property(TARGET ${PROJECT_NAME}
             APPEND PROPERTY INTERFACE_INCLUDE_DIRECTORIES
             $<INSTALL_INTERFACE:include>)

if(BUILD_SHARED_LIBS)
    list(APPEND targets ${PROJECT_NAME}_shared)

    add_library(${PROJECT_NAME}_shared SHARED ${SOURCES})
    target_link_libraries(${PROJECT_NAME}_shared ${libs})

    if(MSVC OR CYGWIN OR MINGW)
        target_compile_definitions(${PROJECT_NAME}_shared
                                   PRIVATE   "TMXEXPORT=__declspec(dllexport)"
                                   INTERFACE "TMXEXPORT=__declspec(dllimport)")
    else()
        target_compile_definitions(${PROJECT_NAME}_shared PUBLIC "TMXEXPORT=")
    endif()

    set_target_properties(${PROJECT_NAME}_shared PROPERTIES OUTPUT_NAME ${PROJECT_NAME})
    set_target_properties(${PROJECT_NAME}_shared PROPERTIES SOVERSION ${API_VERSION})
    set_target_properties(${PROJECT_NAME}_shared PROPERTIES VERSION ${BUILD_VERSION})

    set_property(TARGET ${PROJECT_NAME}_shared
                 APPEND PROPERTY INTERFACE_INCLUDE_DIRECTORIES
                 $<INSTALL_INTERFACE:include>)
endif(BUILD_SHARED_LIBS)

#-----------#
#  Install
#-----------#

include(GNUInstallDirs)

install(TARGETS ${targets}
        EXPORT ${PROJECT_NAME}_exports
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})
install(FILES ${HEADERS} DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

install(EXPORT ${PROJECT_NAME}_exports
        FILE "${PROJECT_NAME}Exports.cmake"
        DESTINATION "lib/cmake/${PROJECT_NAME}")
install(FILES "${CMAKE_BINARY_DIR}/${PROJECT_NAME}Config.cmake" "${CMAKE_BINARY_DIR}/${PROJECT_NAME}ConfigVersion.cmake"
        DESTINATION "lib/cmake/${PROJECT_NAME}")
