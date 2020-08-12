# Try to find the package via pkg-config


# Try to locate the package in the default path
# and in the path provided by pkg-config
# Make sure to check for the right package
IF(Qt4_FOUND)
   PKG_CHECK_MODULES(QWT qwt QUIET)
   FIND_PATH(QWT_INCLUDE_DIR NAMES qwt.h PATHS ${QWT_INCLUDE_DIRS} PATH_SUFFIXES qwt-qt4)
   FIND_LIBRARY(QWT_LIBRARY NAMES qwt-qt4 PATHS ${QWT_LIBRARY_DIRS})
ELSE()
   PKG_CHECK_MODULES(QWT Qt5Qwt6 QUIET)
   FIND_PATH(QWT_INCLUDE_DIR NAMES qwt.h PATHS ${QWT_INCLUDE_DIRS} PATH_SUFFIXES qwt)
   FIND_LIBRARY(QWT_LIBRARY NAMES qwt-qt5 PATHS ${QWT_LIBRARY_DIRS})
ENDIF()

#IF(NOT QWT_INCLUDE_DIR OR NOT QWT_LIBRARY)
#   MESSAGE("Checking QWT")
#   FIND_PATH(QWT_INCLUDE_DIR NAMES qwt.h PATHS ${QWT_INCLUDE_DIRS} PATH_SUFFIXES qwt)
#   FIND_LIBRARY(QWT_LIBRARY NAMES libqwt.so.6abi1 PATHS ${QWT_LIBRARY_DIRS})
#   SET(Qwt 1)
#ENDIF()

# Check if QWT_LIBARARY and QWT_INCLUDE_DIR is set
# Print a message otherwise
#FIND_PACKAGE_HANDLE_STANDARD_ARGS(QWT DEFAULT_MSG QWT_LIBRARY QWT_INCLUDE_DIR)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(Qwt FOUND_VAR QWT_FOUND REQUIRED_VARS QWT_LIBRARY QWT_INCLUDE_DIR)

#IF(Qwt)
#   FIND_PACKAGE_MESSAGE(QwtHint "Found qwt: Qt specific version missing... using fallback." "[${Qwt}]")
#ENDIF()

# Mark those variables to be displayed as 'advanced' in the GUI
MARK_AS_ADVANCED(QWT_LIBRARY QWT_INCLUDE_DIR)

# Current status is
#                                                            Qt4                        Qt5
# libqwt5-qt4/libqwt5-qt4-dev                                 ok                  does not compile
# libqwt-headers/libqwt-qt5-6/libqwt-qt5-dev               core dump              viewer stretched
