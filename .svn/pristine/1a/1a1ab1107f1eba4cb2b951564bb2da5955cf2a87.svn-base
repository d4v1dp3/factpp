# Try to find the package via pkg-config
PKG_CHECK_MODULES(NOVA nova QUIET)

# Try to locate the package in the default path
# and in the path provided by pkg-config
FIND_PATH(NOVA_INCLUDE_DIR NAMES libnova.h PATHS ${NOVA_INCLUDE_DIRS} PATH_SUFFIXES libnova)
FIND_LIBRARY(NOVA_LIBRARY NAMES nova PATHS ${NOVA_LIBRARY_DIRS})

# Check if NOVA_LIBARARY and NOVA_INCLUDE_DIR is set
# Print a message otherwise
FIND_PACKAGE_HANDLE_STANDARD_ARGS(Nova DEFAULT_MSG NOVA_LIBRARY NOVA_INCLUDE_DIR)

# Mark those variables to be displayed as 'advanced' in the GUI
MARK_AS_ADVANCED(NOVA_LIBRARY NOVA_INCLUDE_DIR)
