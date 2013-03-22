# - Find Jansson
# Find the native Jansson includes and library.
# Once done this will define
#
#  JANSSON_INCLUDE_DIRS   - where to find jansson.h, etc.
#  JANSSON_LIBRARIES      - List of libraries when using Jansson.
#  JANSSON_FOUND          - True if Jansson found.

FIND_PATH(JANSSON_INCLUDE_DIR jansson_config.h)

SET(JANSSON_NAMES ${JANSSON_NAMES} jansson libjansson)
FIND_LIBRARY(JANSSON_LIBRARY NAMES ${JANSSON_NAMES} PATH)

MARK_AS_ADVANCED(JANSSON_INCLUDE_DIR JANSSON_LIBRARY)

# handle the QUIETLY and REQUIRED arguments and set JANSSON_FOUND to TRUE if 
# all listed variables are TRUE
INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(jansson  DEFAULT_MSG
                                  JANSSON_LIBRARY JANSSON_INCLUDE_DIR)

IF(JANSSON_FOUND)
    SET(JANSSON_INCLUDE_DIRS ${JANSSON_INCLUDE_DIR})
    SET(JANSSON_LIBRARIES ${JANSSON_LIBRARY})
ENDIF(JANSSON_FOUND)

