# Try to find the package via pkg-config
PKG_CHECK_MODULES(MYSQLPP mysqlpp QUIET)

# Try to locate the package in the default path
# and in the path provided by pkg-config
FIND_PATH(MYSQLPP_INCLUDE_DIR NAMES mysql++.h PATHS ${MYSQLPP_INCLUDE_DIRS} PATH_SUFFIXES mysql++)
FIND_LIBRARY(MYSQLPP_LIBRARY NAMES mysqlpp PATHS ${MYSQLPP_LIBRARY_DIRS})

# Check if MYSQLPP_LIBARARY and MYSQLPP_INCLUDE_DIR is set
# Print a message otherwise
FIND_PACKAGE_HANDLE_STANDARD_ARGS(MySQL++ DEFAULT_MSG MYSQLPP_LIBRARY MYSQLPP_INCLUDE_DIR)

# Mark those variables to be displayed as 'advanced' in the GUI
MARK_AS_ADVANCED(MYSQLPP_LIBRARY MYSQLPP_INCLUDE_DIR)
