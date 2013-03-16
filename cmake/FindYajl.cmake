# - Find yajl
# Find the native YAJL includes and library.
# Once done this will define
#
#  YAJL_INCLUDE_DIRS   - where to find yajl/yajl.h, etc.
#  YAJL_LIBRARIES      - List of libraries when using yajl.
#  YAJL_FOUND          - True if yajl found.

FIND_PATH(YAJL_INCLUDE_DIR yajl/yajl_common.h)

SET(YAJL_NAMES ${YAJL_NAMES} yajl libyajl yajl_s)
FIND_LIBRARY(YAJL_LIBRARY NAMES ${YAJL_NAMES} PATH)

# handle the QUIETLY and REQUIRED arguments and set YAJL_FOUND to TRUE if 
# all listed variables are TRUE
INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(yajl  DEFAULT_MSG
                                  YAJL_LIBRARY YAJL_INCLUDE_DIR)

IF(YAJL_FOUND)
    SET(YAJL_INCLUDE_DIRS ${YAJL_INCLUDE_DIR})
    SET(YAJL_LIBRARIES ${YAJL_LIBRARY})
ENDIF(YAJL_FOUND)
