# Try to find the package via pkg-config
PKG_CHECK_MODULES(CFITSIO cfitsio QUIET)

# Try to locate the package in the default path
# and in the path provided by pkg-config
FIND_PATH(CCFITS_INCLUDE_DIR NAMES CCfits PATHS ${CCFITS_INCLUDE_DIRS} PATH_SUFFIXES CCfits)
FIND_LIBRARY(CCFITS_LIBRARY NAMES CCfits PATHS ${CCFITS_LIBRARY_DIRS})

# Check if CFITSIOLIBARARY and CFITSIOINCLUDE_DIR is set
# Print a message otherwise
FIND_PACKAGE_HANDLE_STANDARD_ARGS(CCfits DEFAULT_MSG CCFITS_LIBRARY CCFITS_INCLUDE_DIR)

# Mark those variables to be displayed as 'advanced' in the GUI
MARK_AS_ADVANCED(CCFITS_LIBRARY CCFITS_INCLUDE_DIR)
