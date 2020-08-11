# This file will be configured to contain variables for CPack. These variables
# should be set in the CMake list file of the project before CPack module is
# included. The list of available CPACK_xxx variables and their associated
# documentation may be obtained using
#  cpack --help-variable-list
#
# Some variables are common to all generators (e.g. CPACK_PACKAGE_NAME)
# and some are specific to a generator
# (e.g. CPACK_NSIS_EXTRA_INSTALL_COMMANDS). The generator specific variables
# usually begin with CPACK_<GENNAME>_xxxx.


set(CPACK_BUILD_SOURCE_DIRS "/home/macj/fact/FACT++;/home/macj/fact/FACT++/build")
set(CPACK_CMAKE_GENERATOR "Unix Makefiles")
set(CPACK_COMPONENT_UNSPECIFIED_HIDDEN "TRUE")
set(CPACK_COMPONENT_UNSPECIFIED_REQUIRED "TRUE")
set(CPACK_DEBIAN_PACKAGE_CONTROL_STRICT_PERMISSION "TRUE")
set(CPACK_DEBIAN_PACKAGE_DESCRIPTION "FACT++ - Toolset
 This package contains some tools which are part of FACT++ to deal with databases, fits files and root-files.")
set(CPACK_DEBIAN_PACKAGE_GENERATE_SHLIBS_POLICY ">=")
set(CPACK_DEBIAN_PACKAGE_HOMEPAGE "https://www.fact-project.org")
set(CPACK_DEBIAN_PACKAGE_SECTION "utils")
set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS "1")
set(CPACK_DEFAULT_PACKAGE_DESCRIPTION_FILE "/usr/share/cmake/Templates/CPack.GenericDescription.txt")
set(CPACK_DEFAULT_PACKAGE_DESCRIPTION_SUMMARY "fact++ built using CMake")
set(CPACK_GENERATOR "TBZ2;DEB")
set(CPACK_INSTALL_CMAKE_PROJECTS "/home/macj/fact/FACT++/build;fact++;ALL;/")
set(CPACK_INSTALL_PREFIX "/usr/local")
set(CPACK_MODULE_PATH "/home/macj/fact/FACT++/cmake;/home/macj/rootv61804/etc/cmake")
set(CPACK_NSIS_DISPLAY_NAME "fact++ 20.31.6.3")
set(CPACK_NSIS_INSTALLER_ICON_CODE "")
set(CPACK_NSIS_INSTALLER_MUI_ICON_CODE "")
set(CPACK_NSIS_INSTALL_ROOT "$PROGRAMFILES")
set(CPACK_NSIS_PACKAGE_NAME "fact++ 20.31.6.3")
set(CPACK_NSIS_UNINSTALL_NAME "Uninstall")
set(CPACK_OUTPUT_CONFIG_FILE "/home/macj/fact/FACT++/build/CPackConfig.cmake")
set(CPACK_PACKAGE_CONTACT "tbretz@physik.rwth-aachen.de")
set(CPACK_PACKAGE_DEFAULT_LOCATION "/")
set(CPACK_PACKAGE_DESCRIPTION_FILE "/home/macj/fact/FACT++/README")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "FACT++ - Toolset")
set(CPACK_PACKAGE_FILE_NAME "fact++-20.31.6.3-Linux")
set(CPACK_PACKAGE_INSTALL_DIRECTORY "fact++ 20.31.6.3")
set(CPACK_PACKAGE_INSTALL_REGISTRY_KEY "fact++ 20.31.6.3")
set(CPACK_PACKAGE_MAINTAINER "Thomas Bretz")
set(CPACK_PACKAGE_NAME "fact++")
set(CPACK_PACKAGE_RELOCATABLE "true")
set(CPACK_PACKAGE_VENDOR "Humanity")
set(CPACK_PACKAGE_VERSION "20.31.6.3")
set(CPACK_PACKAGE_VERSION_MAJOR "20")
set(CPACK_PACKAGE_VERSION_MINOR "31")
set(CPACK_PACKAGE_VERSION_PATCH "6")
set(CPACK_RESOURCE_FILE_LICENSE "/home/macj/fact/FACT++/COPYING")
set(CPACK_RESOURCE_FILE_README "/usr/share/cmake/Templates/CPack.GenericDescription.txt")
set(CPACK_RESOURCE_FILE_WELCOME "/usr/share/cmake/Templates/CPack.GenericWelcome.txt")
set(CPACK_SET_DESTDIR "OFF")
set(CPACK_SOURCE_GENERATOR "TBZ2")
set(CPACK_SOURCE_IGNORE_FILES "/old/;/build/;/[.].*/;/autom4te[.]cache/;.*~;.log$")
set(CPACK_SOURCE_OUTPUT_CONFIG_FILE "/home/macj/fact/FACT++/build/CPackSourceConfig.cmake")
set(CPACK_SYSTEM_NAME "Linux")
set(CPACK_TOPLEVEL_TAG "Linux")
set(CPACK_WIX_SIZEOF_VOID_P "8")

if(NOT CPACK_PROPERTIES_FILE)
  set(CPACK_PROPERTIES_FILE "/home/macj/fact/FACT++/build/CPackProperties.cmake")
endif()

if(EXISTS ${CPACK_PROPERTIES_FILE})
  include(${CPACK_PROPERTIES_FILE})
endif()
