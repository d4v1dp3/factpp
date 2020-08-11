# Try to find the package via pkg-config
PKG_CHECK_MODULES(HIGHLIGHT source-highlight QUIET)

# Try to locate the package in the default path
# and in the path provided by pkg-config
FIND_PATH(HIGHLIGHT_INCLUDE_DIR NAMES sourcehighlight.h PATHS ${HIGHLIGHT_INCLUDE_DIRS} PATH_SUFFIXES srchilite)
FIND_LIBRARY(HIGHLIGHT_LIBRARY NAMES source-highlight PATHS ${HIGHLIGHT_LIBRARY_DIRS})

# Check if LIBARARY and INCLUDE_DIR is set
# Print a message otherwise
FIND_PACKAGE_HANDLE_STANDARD_ARGS(Highlight DEFAULT_MSG HIGHLIGHT_LIBRARY HIGHLIGHT_INCLUDE_DIR)

# Mark those variables to be displayed as 'advanced' in the GUI
IF(HIGHLIGHT_FOUND)
   MARK_AS_ADVANCED(HIGHLIGHT_LIBRARY HIGHLIGHT_INCLUDE_DIR)
ELSE()
   UNSET(HIGHLIGHT_LIBRARY     CACHE)
   UNSET(HIGHLIGHT_INCLUDE_DIR CACHE)
ENDIF()
