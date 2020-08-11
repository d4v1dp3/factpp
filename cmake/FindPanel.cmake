# Try to find the package via pkg-config
PKG_CHECK_MODULES(READLINE readline QUIET)

# Try to locate the package in the default path
# and in the path provided by pkg-config
FIND_PATH(PANEL_INCLUDE_DIR NAMES panel.h PATHS ${PANEL_INCLUDEDIR})
FIND_LIBRARY(PANEL_LIBRARY NAMES panel PATHS ${PANEL_LIBDIR})

# Check if AO_LIBARARY and AO_INCLUDE_DIR is set
# Print a message otherwise
FIND_PACKAGE_HANDLE_STANDARD_ARGS(Panel DEFAULT_MSG PANEL_LIBRARY PANEL_INCLUDE_DIR)

# Mark those variables to be displayed as 'advanced' in the GUI
MARK_AS_ADVANCED(PANEL_LIBRARY PANEL_INCLUDE_DIR)
