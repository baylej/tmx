cmake_minimum_required(VERSION 2.8)

#-----------#
# Configure
#-----------#

project(tmx C)

option(INSTALL_TARGET "adds a 'Install' target to the build system ?" off)
SET(BUILD_SHARED_LIBS OFF)
SET(CMAKE_BUILD_TYPE Release)

#-----------#
#    Env
#-----------#

#-----------#
#   Build
#-----------#

file(GLOB sources "${PROJECT_SOURCE_DIR}/src/*.c")
add_library(${PROJECT_NAME} ${sources})

#-----------#
#  Install
#-----------#

# Install target only on non-win systems
if(INSTALL_TARGET)
install(TARGETS ${PROJECT_NAME} 
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib)
install(DIRECTORY ${PROJECT_SOURCE_DIR}/src/ DESTINATION include/${PROJECT_NAME} FILES_MATCHING PATTERN "*.h")
endif()

