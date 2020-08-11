# Try to find the package via pkg-config
PKG_CHECK_MODULES(V8 v8 QUIET)

# Try to locate the package in the default path
# and in the path provided by pkg-config
FIND_PATH(V8_INCLUDE_DIR NAMES v8.h PATHS ${V8_INCLUDE_DIRS})
FIND_LIBRARY(V8_LIBRARY NAMES v8 PATHS ${V8_LIBRARY_DIRS})

# Check if V8_LIBARARY and V8_INCLUDE_DIR is set
# Print a message otherwise
FIND_PACKAGE_HANDLE_STANDARD_ARGS(V8 DEFAULT_MSG V8_LIBRARY V8_INCLUDE_DIR)

# Mark those variables to be displayed as 'advanced' in the GUI
MARK_AS_ADVANCED(V8_LIBRARY V8_INCLUDE_DIR)
