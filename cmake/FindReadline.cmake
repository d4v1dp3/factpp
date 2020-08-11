# Try to find the package via pkg-config
PKG_CHECK_MODULES(READLINE readline QUIET)

# Try to locate the package in the default path
# and in the path provided by pkg-config
FIND_PATH(READLINE_INCLUDE_DIR NAMES readline.h PATHS ${READLINE_INCLUDEDIR} PATH_SUFFIXES readline)
FIND_LIBRARY(READLINE_LIBRARY NAMES readline PATHS ${READLINE_LIBDIR})

# Check if AO_LIBARARY and AO_INCLUDE_DIR is set
# Print a message otherwise
FIND_PACKAGE_HANDLE_STANDARD_ARGS(Readline DEFAULT_MSG READLINE_LIBRARY READLINE_INCLUDE_DIR)

# Mark those variables to be displayed as 'advanced' in the GUI
MARK_AS_ADVANCED(READLINE_LIBRARY READLINE_INCLUDE_DIR)
