# Try to find the package via pkg-config
PKG_CHECK_MODULES(DBus dbus-1)

# Try to locate the package in the default path
# and in the path provided by pkg-config
FIND_PATH(DBUS_INCLUDE_DIR NAMES dbus/dbus-glib-lowlevel.h PATHS ${DBUS_INCLUDEDIR})
FIND_LIBRARY(DBUS_LIBRARY NAMES dbus-1 PATHS ${DBUS_LIBDIR})

# Check if AO_LIBARARY and AO_INCLUDE_DIR is set
# Print a message otherwise
FIND_PACKAGE_HANDLE_STANDARD_ARGS(libdbus-1 DEFAULT_MSG DBUS_LIBRARY DBUS_INCLUDE_DIR)

# Mark those variables to be displayed as 'advanced' in the GUI
MARK_AS_ADVANCED(DBUS_LIBRARY DBUS_INCLUDE_DIR)

# handle success
IF(DBUS_FOUND AND NOT DBUS_FIND_QUIETLY)
    MESSAGE(STATUS "Found DBus ${DBUS_VERSION} in ${DBUS_INCLUDE_DIR}")
ENDIF()
