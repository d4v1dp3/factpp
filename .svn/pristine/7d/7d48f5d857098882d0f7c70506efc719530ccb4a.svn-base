# Try to find the package via pkg-config
PKG_CHECK_MODULES(CFITSIO cfitsio QUIET)

# Try to locate the package in the default path
# and in the path provided by pkg-config
FIND_PATH(CFITSIO_INCLUDE_DIR NAMES fitsio.h PATHS ${CFITSIO_INCLUDE_DIRS})
FIND_LIBRARY(CFITSIO_LIBRARY NAMES cfitsio PATHS ${CFITSIO_LIBRARY_DIRS})

# Check if CFITSIOLIBARARY and CFITSIOINCLUDE_DIR is set
# Print a message otherwise
FIND_PACKAGE_HANDLE_STANDARD_ARGS(Cfitsio DEFAULT_MSG CFITSIO_LIBRARY CFITSIO_INCLUDE_DIR)

# Mark those variables to be displayed as 'advanced' in the GUI
MARK_AS_ADVANCED(CFITSIO_LIBRARY CFITSIO_INCLUDE_DIR)
