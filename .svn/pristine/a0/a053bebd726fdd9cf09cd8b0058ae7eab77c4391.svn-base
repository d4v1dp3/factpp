# *********************************
# ************ cmake **************
# *********************************
CMAKE_MINIMUM_REQUIRED(VERSION 3.0 FATAL_ERROR)
CMAKE_POLICY(SET CMP0043 NEW)
CMAKE_POLICY(SET CMP0048 NEW)

# Enable debug symbols by default
# must be done before project() statement
SET(CMAKE_BUILD_TYPE_INIT Release) 
# (you can also set it on the command line: -D CMAKE_BUILD_TYPE=Debug)

# Enable CLANG as default compilers, to change this use
# cmake -DCMAKE_C_COMPILER=cc -DCMAKE_CXX_COMPILER=c++
# Note that if compilers are switched on the fly,
# the cache is deleted
IF(NOT DEFINED CMAKE_C_COMPILER)
	FIND_PROGRAM(CMAKE_C_COMPILER   NAMES $ENV{CC}  clang   gcc cc  PATHS ENV PATH NO_DEFAULT_PATH DOC "C Compiler")
ENDIF()
IF(NOT DEFINED CMAKE_CXX_COMPILER)
	FIND_PROGRAM(CMAKE_CXX_COMPILER NAMES $ENV{CXX} clang++ g++ c++ PATHS ENV PATH NO_DEFAULT_PATH DOC "C++ Compiler")
ENDIF()


#SET(CMAKE_VERBOSE_MAKEFILE ON)


# *********************************
# ************ Project ************
# *********************************

STRING(TIMESTAMP VERSION %y.%U.%w.%H UTC)

MESSAGE(STATUS "FACT++ V${VERSION}")

PROJECT(fact++ VERSION ${VERSION})

INCLUDE(GNUInstallDirs)

IF ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
    SET(GCC_MIN 4.4.7)
    IF (CMAKE_C_COMPILER_VERSION VERSION_LESS GCC_MIN OR
	CMAKE_CXX_COMPILER_VERSION VERSION_LESS GCC_MIN)
        MESSAGE(FATAL_ERROR "\n  gcc/g++ ${GCC_MIN} or higher is required.\n  You are running version ${CMAKE_C_COMPILER_VERSION}/${CMAKE_CXX_COMPILER_VERSION}.")
    ENDIF()
ELSEIF ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
    SET(CLANG_MIN 3.8)
    IF (CMAKE_C_COMPILER_VERSION VERSION_LESS CLANG_MIN OR
        CMAKE_CXX_COMPILER_VERSION VERSION_LESS CLANG_MIN)
        MESSAGE(FATAL_ERROR "\n  clang/clang++ ${CLANG_MIN} or higher is required.\n  You are running version ${CMAKE_C_COMPILER_VERSION}/${CMAKE_CXX_COMPILER_VERSION}.")
    ENDIF()
ELSE()
    MESSAGE(WARNING "You are using an unsupported compiler! Compilation has only been tested only with clang/clang++ and gcc/g++.")
ENDIF()

# ======== For config.h.in =========
SET(PACKAGE_MAINTAINER "Thomas Bretz")
SET(PACKAGE_BUGREPORT  "tbretz@physik.rwth-aachen.de")
SET(PACKAGE_URL        "https://www.fact-project.org")

FIND_PROGRAM(LSB_RELEASE_EXE NAMES lsb_release)
IF (DEFINED LSB_RELEASE_EXE)
   EXECUTE_PROCESS(COMMAND ${LSB_RELEASE_EXE} -d OUTPUT_VARIABLE LSB_RELEASE OUTPUT_STRIP_TRAILING_WHITESPACE)
   MESSAGE(STATUS ${LSB_RELEASE})
ENDIF()

# *********************************
# ******** FAMOUS / IceACT ********
# *********************************

#SET(OBSERVATORY "ORM" CACHE STRING "Select the observatory location (ORM, HAWC, SPM, RWTH: see nova.h)")

IF (NOT TOOLS_ONLY AND NOT VIEWER_ONLY)

IF(EXISTS "${CMAKE_SOURCE_DIR}/famous")
	MESSAGE(STATUS "Compilation of 'famous' enabled.")
        SET(FOUND_FAMOUS ON)
ENDIF()

IF(EXISTS "${CMAKE_SOURCE_DIR}/iceact")
	MESSAGE(STATUS "Compilation of 'iceact' enabled.")
        SET(FOUND_ICEACT ON)
ENDIF()

ENDIF(NOT TOOLS_ONLY AND NOT VIEWER_ONLY)


# *********************************
# ********* Requirements **********
# *********************************

# =========== pkg-config ============
FIND_PACKAGE(PkgConfig)
FIND_PACKAGE(PackageHandleStandardArgs)


# ========== executables ==========
FIND_PROGRAM(HELP2MAN_EXECUTABLE  help2man)
FIND_PROGRAM(ROFF2HTML_EXECUTABLE roff2html)
FIND_PROGRAM(ROFF2PDF_EXECUTABLE  roff2pdf)
FIND_PROGRAM(GROFF_EXECUTABLE     groff)

FIND_PACKAGE_HANDLE_STANDARD_ARGS(help2man  DEFAULT_MSG HELP2MAN_EXECUTABLE)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(roff2html DEFAULT_MSG ROFF2HTML_EXECUTABLE)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(roff2pdf  DEFAULT_MSG ROFF2PDF_EXECUTABLE)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(groff     DEFAULT_MSG GROFF_EXECUTABLE)

IF (NOT HELP2MAN_EXECUTABLE OR NOT ROFF2HTML_EXECUTABLE OR NOT ROFF2PDF_EXECUTABLE OR NOT GROFF_EXECUTABLE)
MESSAGE(STATUS "Documentation turned off (some executables missing).")
ENDIF()

IF (NOT TOOLS_ONLY AND NOT VIEWER_ONLY)

FIND_PROGRAM(JSDOC_EXECUTABLE    jsdoc)
FIND_PROGRAM(MAILX_EXECUTABLE    mailx)
FIND_PROGRAM(CURL_EXECUTABLE     curl)

FIND_PACKAGE_HANDLE_STANDARD_ARGS(curl     DEFAULT_MSG CURL_EXECUTABLE)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(mailx    DEFAULT_MSG MAILX_EXECUTABLE)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(jsdoc    DEFAULT_MSG JSDOC_EXECUTABLE)

IF (curl_FOUND)
   ADD_DEFINITIONS(-DHAVE_CURL)
ENDIF()
IF (mailx_FOUND)
   ADD_DEFINITIONS(-DHAVE_MAILX)
ENDIF()

ENDIF (NOT TOOLS_ONLY AND NOT VIEWER_ONLY)

#IF(NOT SOME_COND)
#  MESSAGE(FATAL_ERROR "At least one required executable was not found")
#ENDIF()

# =========== libraries =============

SET(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake" ${CMAKE_MODULE_PATH})

# FIXME: Need to check which package finders accept REQUIERED. For all others implement checks

FIND_PACKAGE(Threads  REQUIRED)  # Threads::Threads
FIND_PACKAGE(ZLIB     REQUIRED)  # ZLIB::ZLIB

SET(CURSES_NEED_NCURSES ON)
FIND_PACKAGE(Curses   REQUIRED)  # CURSES_FOUND
# Newer boost versions use timeout which is defined in ncurses.h
# see for example: https://stackoverflow.com/questions/55265851/getting-errors-when-trying-to-compile-program-using-boost-and-ncurses-libraries
ADD_DEFINITIONS(-DNCURSES_NOMACROS)
# FIXME : Require  panel

IF(NOT TOOLS_ONLY OR VIEWER_ONLY)
FIND_PACKAGE(OpenGL   REQUIRED)  # OpenGL::GL OpenGL::GLU / OPENGL_FOUND OPENGL_GLU_FOUND
ENDIF(NOT TOOLS_ONLY OR VIEWER_ONLY)

IF(NOT TOOLS_ONLY)

# FIND_PACKAGE(DBus)    => skaypeclient
# FIND_PACKAGE(Motif)   => did   # MOTIF_FOUND

FIND_PACKAGE(Readline REQUIRED)
FIND_PACKAGE(Panel    REQUIRED)
FIND_PACKAGE(V8       REQUIRED)
FIND_PACKAGE(CCfits   REQUIRED)  # First as Cfitsio is a dependency
FIND_PACKAGE(Cfitsio  REQUIRED)
FIND_PACKAGE(OpenSSL  REQUIRED)  # Required for boost::asio::ssl

ENDIF(NOT TOOLS_ONLY)

FIND_PACKAGE(Nova     REQUIRED)

FIND_PACKAGE(Highlight)
IF (HIGHLIGHT_FOUND)
   ADD_DEFINITIONS(-DHAVE_HIGHLIGHT)
ENDIF()


#INCLUDE_DIRECTORIES("/usr/include/mysql")
# dbus-1 dbus-glib-1
# doxygen

#MESSAGE("")
#MESSAGE(${V8_LIBRARY})
#MESSAGE(${MYSQLPP_LIBRARY})
#MESSAGE(${PANEL_LIBRARY})
#MESSAGE(${CURSES_LIBRARIES})
#MESSAGE(${CCFITS_LIBRARY})
#MESSAGE(${CFITSIO_LIBRARY})
#MESSAGE(${MYSQL_LIBRARY})
#MESSAGE(${NOVA_LIBRARY})
#MESSAGE(${READLINE_LIBRARY})
#MESSAGE(${OPENSSL_SSL_LIBRARY})
#MESSAGE("")

# =============== MySQL ============

INCLUDE(GetPrerequisites)

FIND_PACKAGE(MySQL++ REQUIRED)

IF(NOT DEFINED MYSQL_LIBRARY AND NOT CMAKE_VERSION VERSION_LESS 3.6)
	GET_PREREQUISITES(${MYSQLPP_LIBRARY} MYSQL_LIBRARY 0 0 "" "")
	LIST(FILTER MYSQL_LIBRARY INCLUDE REGEX .*mysqlclient.*)
        IF (DEFINED MYSQL_LIBRARY)
        	FIND_PACKAGE_MESSAGE(MysqlLib "MySQL++ links to ${MYSQL_LIBRARY}" "[${MYSQL_LIBRARY}]")
 	ENDIF()
ENDIF()

FIND_PACKAGE(MySQL REQUIRED)

INCLUDE_DIRECTORIES(${MYSQL_INCLUDE_DIR} ${MYSQLPP_INCLUDE_DIR})


# =============== BOOST ============
# 1.47.0 is the newest version available at ISDC
FIND_PACKAGE(Boost 1.47.0 REQUIRED
        date_time
        filesystem
        program_options
        regex
        system
        thread)

IF(CMAKE_CXX_COMPILER_ID MATCHES "Clang" AND Boost_VERSION LESS 106000 AND NOT NO_BOOST_HACK)
    MESSAGE(STATUS "  BOOST_REGEX_NO_EXTERNAL_TEMPLATES")
    SET(CMAKE_CXX_FLAGS	"${CMAKE_CXX_FLAGS} -DBOOST_REGEX_NO_EXTERNAL_TEMPLATES=1")
ENDIF()

# At least print some information for reference
FIND_PACKAGE_HANDLE_STANDARD_ARGS(Boost FOUND_VAR Boost_FOUND REQUIRED_VARS Boost_INCLUDE_DIR Boost_LIBRARIES)

# =============== Qt4/Qt5 ==============
IF(NOT TOOLS_ONLY OR VIEWER_ONLY)

   IF (NOT FORCE_QT5)
      FIND_PACKAGE(Qt4 COMPONENTS QtCore QtGui QtOpenGL QtSql QtXml QUIET)
   ENDIF()

   IF(Qt4_FOUND)
      FIND_PACKAGE_MESSAGE(Qt4Found "Found Qt4 [Core Gui OpenGL Sql Xml]" "[${Qt4_FOUND}]")
      SET(Qt Qt4)
      SET(QtCore QtCore)
      SET(QtGui QtGui)
      SET(QtOpenGL QtOpenGL)
      SET(QtSql QtSql)
      SET(QtXml QtXml)
      ADD_DEFINITIONS(-DHAVE_QT4)
   ELSE()
      IF (NOT FORCE_QT5)
         FIND_PACKAGE_MESSAGE(Qt4NotFound "Qt4 [or one of: Core Gui OpenGL Sql Xml] NOT found, trying Qt5!" "[${Qt4_FOUND}]")
      ENDIF()
      FIND_PACKAGE(Qt5 COMPONENTS Core Gui OpenGL Sql Xml REQUIRED)
      FIND_PACKAGE_MESSAGE(Qt5Found2 "Found Qt5: fact GUI will be disabled" "[${Qt5_FOUND}]")
      SET(Qt Qt5)
      SET(QtCore Core)
      SET(QtGui Gui)
      SET(QtOpenGL OpenGL)
      SET(QtSql Sql)
      SET(QtXml Xml)
   ENDIF()

   FIND_PACKAGE(Qwt REQUIRED)

ENDIF(NOT TOOLS_ONLY OR VIEWER_ONLY)

# ============== ROOT ================

IF(NOT NO_ROOT AND NOT VIEWER_ONLY)

   FIND_PROGRAM(ROOTCONFIG_EXECUTABLE NAMES root-config)
   FIND_PACKAGE_HANDLE_STANDARD_ARGS(root-config DEFAULT_MSG ROOTCONFIG_EXECUTABLE)
   IF(NOT ROOTCONFIG_EXECUTABLE)
      MESSAGE(FATAL_ERROR "Please make sure `root-config` is accessible in your path. In case of an installation from source, you might need to source\n . [path-to-root]/bin/thisroot.sh")
   ENDIF()

   # You need to tell CMake where to find the ROOT installation. This can be done in a number of ways:
   #   - ROOT built with classic configure/make use the provided $ROOTSYS/etc/cmake/FindROOT.cmake
   #   - ROOT built with CMake. Add in CMAKE_PREFIX_PATH the installation prefix for ROOT
   IF(EXISTS $ENV{ROOTSYS}/ROOTConfig.cmake)
      IF(NOT DEFINED ENV{CMAKE_PREFIX_PATH})
         LIST(APPEND CMAKE_PREFIX_PATH $ENV{ROOTSYS})
      ENDIF()
   ENDIF()
   SET(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "$ENV{ROOTSYS}/etc/cmake")

   #---Locate the ROOT package and defines a number of variables (e.g. ROOT_INCLUDE_DIRS)
   #   SET(ROOT_CONFIG_DEBUG 1)
   FIND_PACKAGE(ROOT REQUIRED COMPONENTS TreePlayer OPTIONAL_COMPONENTS GQt)

   IF(NOT DEFINED ROOT_CONFIG_EXECUTABLE)
      FIND_PACKAGE_MESSAGE(RootFound "Found ROOT: ${ROOT_INCLUDE_DIRS}" "[${ROOT_INCLUDE_DIRS}][${ROOT_LIBRARY_DIR}][${ROOT_VERSION}]")
   ENDIF()

   IF (NOT TOOLS_ONLY AND NOT VIEWER_ONLY AND NOT ROOT_GQt_LIBRARY)
      IF(ROOT_VERSION VERSION_LESS 6.18)
         MESSAGE(FATAL_ERROR "Root version [${ROOT_VERSION}] less than 6.18, but component GQt not found.")
      ELSE()
         MESSAGE(WARNING "Root version [${ROOT_VERSION}] does not support GQt, GUI [fact] turned off.")
      ENDIF()
   ENDIF()

   #---Define useful ROOT functions and macros (e.g. ROOT_GENERATE_DICTIONARY)
   FIND_PACKAGE_MESSAGE(RootInclude "Setup ROOT: [${ROOT_VERSION}] ${ROOT_USE_FILE}" "[${ROOT_VERSION}][${ROOT_USE_FILE}]")
   IF(DEFINED ROOT_USE_FILE)
      INCLUDE(${ROOT_USE_FILE})
   ELSE()
      # From RootUseFile.cmake (root 6)
      INCLUDE_DIRECTORIES(${ROOT_INCLUDE_DIRS})
      LINK_DIRECTORIES(${ROOT_LIBRARY_DIR})
      ADD_DEFINITIONS(${ROOT_DEFINITIONS})
      SET(CMAKE_CXX_FLAGS     "${CMAKE_CXX_FLAGS} ${ROOT_CXX_FLAGS}")
      SET(CMAKE_C_FLAGS       "${CMAKE_C_FLAGS} ${ROOT_C_FLAGS}")
      SET(CMAKE_Fortran_FLAGS "${CMAKE_Fortran_FLAGS} ${ROOT_fortran_FLAGS}")
   ENDIF()

   # Is this is a local or a global installation?
   IF(DEFINED ENV{ROOTSYS})
      IF(ROOTSYS AND NOT ROOTSYS STREQUAL "$ENV{ROOTSYS}")
         MESSAGE(WARNING
            " \n"
            " ROOT Location changed!\n"
            " Present  ROOTSYS: $ENV{ROOTSYS}\n"
            " Previous ROOTSYS: ${ROOTSYS}")
         FILE(REMOVE thisroot.sh)
      ENDIF()

      SET(ROOTSYS $ENV{ROOTSYS} CACHE INTERNAL "Internal variable to check for consistency in ROOTSYS")

      # For convenience and easy tracking
      ADD_CUSTOM_COMMAND(OUTPUT thisroot.sh COMMAND ln -sf ${ROOTSYS}/bin/thisroot.sh VERBATIM)
      ADD_CUSTOM_TARGET(thisroot ALL DEPENDS thisroot.sh)
   ENDIF()

   IF(ROOTVER AND NOT ROOTVER VERSION_EQUAL ROOT_VERSION)
      MESSAGE(WARNING
         " \n"
         " ROOT Version changed!\n"
         " Present:  ${ROOT_VERSION}\n"
         " Previous: ${ROOTVER}")
   ENDIF()

   SET(ROOTVER ${ROOT_VERSION} CACHE INTERNAL "Internal variable to check for consistency of root version")

   ADD_DEFINITIONS(-DHAVE_ROOT)

ENDIF(NOT NO_ROOT AND NOT VIEWER_ONLY)

# Note that in root 5.34/38 (and maybe others?) the ROOT_xxx_FOUND macros are broken
IF (NOT ROOT_GQt_FOUND)
   FIND_LIBRARY(ROOT_GQt_LIBRARY NAMES GQt PATHS ${ROOT_LIBRARY_DIR})
   IF (ROOT_GQt_LIBRARY)
      FIND_PACKAGE_MESSAGE(RootQGtLib "Found libGQt: ${ROOT_GQt_LIBRARY}" "[${ROOT_GQt_LIBRARY}]")
      SET(ROOT_GQt_FOUND 1)
   ELSE()
      FIND_PACKAGE_MESSAGE(RootQGt "ROOT: libGQt not found, fact GUI disabled!" "[${ROOT_GQt_FOUND}]")
   ENDIF()
ENDIF()

# -lGui -lCore -lCint -lRIO -lNet -lHist -lGraf -lGraf3d -lGpad -lTree
# -lRint -lPostscript -lMatrix -lPhysics -lMathCore -lThread
# -lASImage -lMinuit -lHistPainter -lThread

#MESSAGE("INCL_DIRS: ${ROOT_INCLUDE_DIRS}")
#MESSAGE("LIBS:      ${ROOT_LIBRARIES}")
#MESSAGE("DEFS:      ${ROOT_DEFINITIONS}")
#MESSAGE("CXXFLAGS:  ${ROOT_CXX_FLAGS}")
#MESSAGE("CXFLAGS:   ${ROOT_CC_FLAGS}")
#MESSAGE("FOUND:     ${ROOT_FOUND}")
#MESSAGE("FILE:      ${ROOT_USE_FILE}")
#MESSAGE("DIR:       ${CMAKE_CURRENT_SOURCE_DIR}")
#ROOT_<library>_LIBRARY	PATH	Full path for each of the ROOT libraries listed in COMPONENTS
#ROOT_<command>_CMD	PATH	Full path for each ROOT executable (rootcling, root, hadd, etc.)
#ROOT_<option>_FOUND	BOOL	True for each enabled build option (e.g. cocoa, python, xrootd, etc.)

# function ROOT_GENERATE_DICTIONARY( dictionary
#                                    header1 header2 ...
#                                    LINKDEF linkdef1 ...
#                                    OPTIONS opt1...)

# *********************************
# ********** Observatory **********
# *********************************

SET(OBSERVATORY "ORM" CACHE STRING "Select the observatory location (ORM, HAWC, SPM, RWTH: see nova.h)")

IF(NOT OBSERVATORY STREQUAL "ORM"  AND
   NOT OBSERVATORY STREQUAL "HAWC" AND
   NOT OBSERVATORY STREQUAL "SPM"  AND
   NOT OBSERVATORY STREQUAL "RWTH")
   MESSAGE(FATAL_ERROR "OBSERVATORY=${OBSERVATORY} invalid. Only ORM, HAWC, SPM and RWTH allowd!")
ENDIF()

FIND_PACKAGE_MESSAGE(PresetObservatory "Preset Observatory: ${OBSERVATORY}" "[${OBSERVATORY}]")

# *********************************
# *********** Compiler ************
# *********************************

SET(CMAKE_CXX_STANDARD 11)

#ADD_DEFINITIONS(-D__CMAKE__)

SET(CMAKE_CXX_FLAGS
	"${CMAKE_CXX_FLAGS} -Wall -Winit-self -Wpointer-arith \
         -Wcast-align -Wformat=2 -Woverloaded-virtual \
         -Wnon-virtual-dtor -Wshadow -Wno-unknown-pragmas") # -Wextra

# -D__MARS__
SET(CMAKE_CXX_FLAGS
	"${CMAKE_CXX_FLAGS} \
         -D_REENTRANT -D__LINUX__ \
         -DHAVE_ZLIB -DHAVE_V8 -DHAVE_SQL -DHAVE_NOVA -DHAVE_FITS -DPRESET_OBSERVATORY=k${OBSERVATORY} \
         -DMARSVER=\\\"${VERSION}\\\" \
         -DPACKAGE_VERSION=\\\"${VERSION}\\\" \
         -DREVISION=\\\"${REVISION}\\\" \
         -DPACKAGE_NAME=\\\"FACT++\\\" \
         -DPACKAGE_STRING=\\\"FACT++\\\" \
         -DPACKAGE_URL=\\\"${PACKAGE_URL}\\\" \
         -DPACKAGE_BUGREPORT=\\\"${PACKAGE_BUGREPORT}\\\""
         )

#         -DBOOST_DISABLE_ASSERTS \
#         -DBOOST_NO_STD_LOCALE=1 \
#         -DBOOST_REXEG_NO_FWD=1 \
#         -DBOOST_NO_MEMBER_TEMPLATES=1 \
#         -DBOOST_HAS_ABI_HEADERS=1 \

# Flags required for Dim
SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -pedantic -DMIPSEL -DPROTOCOL=1 -Dunix -Dlinux")


# -------------------------------------------------------
# Inlclude the main source AND build dir itself
SET(CMAKE_INCLUDE_CURRENT_DIR ON)
SET(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/lib)

INCLUDE_DIRECTORIES(pal)
INCLUDE_DIRECTORIES(erfa/src)
INCLUDE_DIRECTORIES(dim/dim)
INCLUDE_DIRECTORIES(externals)
INCLUDE_DIRECTORIES(src)

# ********************************************************
# ********************** Libraries ***********************
# ********************************************************
IF (NOT TOOLS_ONLY AND NOT VIEWER_ONLY)

# =================== libPointing.so ======================
ADD_LIBRARY(Pointing SHARED
	pal/pal.h
	pal/palDtt.c 		pal/palDat.c 		pal/palMappa.c
	pal/palPrenut.c		pal/palEvp.c 		pal/palAoppa.c
	pal/palAoppat.c		pal/palRefco.c 		pal/palRefro.c
	pal/pal1Atmt.c 		pal/palDrange.c		pal/palOne2One.c
	pal/pal1Atms.c 		pal/palMapqkz.c 	pal/palAopqk.c
	pal/palRefz.c 		pal/palAmpqk.c 		pal/palRdplan.c
	pal/palDt.c 		pal/palPvobs.c 		pal/palNut.c
	pal/palDmoon.c 		pal/palPlanet.c 	pal/palNutc.c
	pal/palDeuler.c
	erfa/src/gd2gc.c	erfa/src/p06e.c		erfa/src/c2s.c
	erfa/src/eform.c	erfa/src/s2c.c		erfa/src/pas.c
	erfa/src/pmat06.c	erfa/src/epv00.c	erfa/src/plan94.c
	erfa/src/anpm.c		erfa/src/obl06.c	erfa/src/dat.c
        erfa/src/af2a.c		erfa/src/rxr.c		erfa/src/gmst06.c
	erfa/src/sepp.c		erfa/src/rz.c		erfa/src/zp.c
	erfa/src/rxpv.c		erfa/src/pn.c		erfa/src/cr.c
	erfa/src/seps.c		erfa/src/ry.c		erfa/src/pdp.c
	erfa/src/pnm06a.c	erfa/src/hfk5z.c	erfa/src/epj2jd.c
	erfa/src/pv2s.c		erfa/src/tf2a.c		erfa/src/pm.c
	erfa/src/sxp.c		erfa/src/a2af.c		erfa/src/rxp.c
	erfa/src/pxp.c		erfa/src/fk5hip.c	erfa/src/fw2m.c
	erfa/src/rx.c		erfa/src/tf2d.c		erfa/src/cal2jd.c
	erfa/src/cp.c		erfa/src/nut06a.c	erfa/src/rm2v.c
	erfa/src/nut00a.c	erfa/src/ee06a.c	erfa/src/fk5hz.c
	erfa/src/epb2jd.c	erfa/src/refco.c	erfa/src/a2tf.c
	erfa/src/fapa03.c	erfa/src/gst06a.c	erfa/src/faf03.c
	erfa/src/faur03.c	erfa/src/faju03.c	erfa/src/fal03.c
	erfa/src/fasa03.c	erfa/src/fame03.c 	erfa/src/fave03.c
	erfa/src/fama03.c 	erfa/src/faom03.c 	erfa/src/gst06.c
	erfa/src/jd2cal.c	erfa/src/gd2gce.c	erfa/src/anp.c
	erfa/src/fae03.c	erfa/src/ir.c		erfa/src/pfw06.c
	erfa/src/bpn2xy.c	erfa/src/eors.c		erfa/src/s06.c
	erfa/src/trxp.c		erfa/src/era00.c	erfa/src/epj.c
	erfa/src/d2tf.c		erfa/src/epb.c		erfa/src/rv2m.c
	erfa/src/pap.c		erfa/src/fad03.c	erfa/src/pmp.c
	erfa/src/tr.c		erfa/src/falp03.c)
TARGET_LINK_LIBRARIES(Pointing PUBLIC m)

# ======================= libDim.so ======================
ADD_LIBRARY(Dim SHARED
#	dim/dim/dic.h
#	dim/dim/dim_common.h
#	dim/dim/dim.h
#	dim/dim/dim_tcpip.h
#	dim/dim/dis.h
	dim/src/dic.c
	dim/src/dis.c
	dim/src/dna.c
	dim/src/sll.c
	dim/src/dll.c
	dim/src/hash.c
	dim/src/swap.c
	dim/src/copy_swap.c
	dim/src/open_dns.c
	dim/src/conn_handler.c
	dim/src/tcpip.c
	dim/src/dtq.c
	dim/src/dim_thr.c
	dim/src/utilities.c)

# ================= libDimExtension.so ===================
ADD_LIBRARY(DimExtension SHARED
#        src/DimSetup.h
	src/DimSetup.cc)
TARGET_LINK_LIBRARIES(DimExtension PUBLIC
        Boost::system)

# ===================== libDim++.so ======================
ADD_LIBRARY(Dim++ SHARED
#       dim/dim/dic.hxx
#       dim/dim/dis.hxx
#       dim/dim/dim_core.hxx
#       dim/dim/dim.hxx
#       dim/dim/dim_tcpip.h
#       dim/dim/dllist.hxx
#       dim/dim/sllist.hxx
#       dim/dim/tokenstring.hxx
#       src/Dim.h
	dim/src/diccpp.cxx
	dim/src/dimcpp.cxx
	dim/src/discpp.cxx
	dim/src/tokenstring.cxx)
TARGET_LINK_LIBRARIES(Dim++ PUBLIC Dim)

# ================= libStateMachine.so ===================
ADD_LIBRARY(StateMachine SHARED
	src/WindowLog.h       src/WindowLog.cc
	src/Readline.h        src/Readline.cc
	src/ReadlineColor.h   src/ReadlineColor.cc
	src/ReadlineWindow.h  src/ReadlineWindow.cc
	src/Console.h         src/Console.cc
	src/Shell.h           src/Shell.cc
	src/EventImp.h        src/EventImp.cc
	src/Event.h           src/Event.cc
	src/State.h           src/State.cc
	src/Description.h     src/Description.cc
	src/MessageImp.h      src/MessageImp.cc
	src/Converter.h       src/Converter.cc
	src/StateMachineImp.h src/StateMachineImp.cc
	src/StateMachine.h    src/StateMachine.cc
	src/EventDim.h
	src/MessageDim.h         src/MessageDim.cc
	src/StateMachineDim.h    src/StateMachineDim.cc
	src/DimServerList.h      src/DimServerList.cc
	src/DimServiceInfoList.h src/DimServiceInfoList.cc
#	src/DimNetworkList.h     src/DimNetwork.cc
	src/ServiceList.h        src/ServiceList.cc
	src/DimErrorRedirecter.h
	src/DimErrorRedirecter.cc
	src/DimDescriptionService.h
	src/DimDescriptionService.cc
	src/Connection.h      src/Connection.cc
	src/ConnectionUSB.h   src/ConnectionUSB.cc
	src/ConnectionSSL.h   src/ConnectionSSL.cc
#	FACT.h ByteOrder.h
	src/DimWriteStatistics.h src/DimWriteStatistics.cc)
TARGET_LINK_LIBRARIES(StateMachine PUBLIC
	readline ${CURSES_LIBRARIES} panel
        Tools Time Dim++
	Boost::regex
	Boost::thread
        Boost::filesystem
        OpenSSL::SSL)

ENDIF (NOT TOOLS_ONLY AND NOT VIEWER_ONLY)

# ================= libConfiguration.so ==================
ADD_LIBRARY(Configuration SHARED
	src/Configuration.h src/Configuration.cc
	src/FACT.h src/FACT.cc)
TARGET_LINK_LIBRARIES(Configuration PUBLIC
	Boost::regex
        Boost::system
        Boost::filesystem
        Boost::program_options
        ${MYSQL_LIBRARY} ${MYSQLPP_LIBRARY})

# ===================== libTime.so =======================
ADD_LIBRARY(Time SHARED
	src/Time.h src/Time.cc)
TARGET_LINK_LIBRARIES(Time PUBLIC ${NOVA_LIBRARY})

# ===================== libTools.so ======================
ADD_LIBRARY(Tools SHARED
	src/tools.h src/tools.cc)



# ********************************************************
# ********************** Manuals *************************
# ********************************************************

IF (NOT VIEWER_ONLY)

FILE(MAKE_DIRECTORY ${PROJECT_BINARY_DIR}/man1)
FILE(MAKE_DIRECTORY ${PROJECT_BINARY_DIR}/html)
FILE(MAKE_DIRECTORY ${PROJECT_BINARY_DIR}/pdf)

MACRO(MANPAGE program description)
   # https://samthursfield.wordpress.com/2015/11/21/cmake-dependencies-between-targets-and-files-and-custom-commands/
   IF (HELP2MAN_EXECUTABLE AND ROFF2HTML_EXECUTABLE AND ROFF2PDF_EXECUTABLE AND GROFF_EXECUTABLE)
        ADD_CUSTOM_COMMAND(
        	OUTPUT
                   man1/${program}.1
                DEPENDS
                   ${program}
  		COMMAND
                   ${HELP2MAN_EXECUTABLE}
                   ${PROJECT_BINARY_DIR}/${program}
  	  	   -n "${description}"
  	  	   --no-info
  	  	   --output=man1/${program}.1
		COMMAND
                   ${GROFF_EXECUTABLE}
                   -mandoc man1/${program}.1
                   -T html > html/${program}.html
		COMMAND
                   ${GROFF_EXECUTABLE}
                   -mandoc man1/${program}.1
                   -T pdf > pdf/${program}.pdf
                COMMENT
                   "Generating man-page ${program}.1"
                VERBATIM)

	ADD_CUSTOM_TARGET(${program}.man ALL DEPENDS man1/${program}.1)

	INSTALL(FILES ${PROJECT_BINARY_DIR}/man1/${program}.1 DESTINATION "${CMAKE_INSTALL_MANDIR}/man1")
   ENDIF()
ENDMACRO()


# ********************************************************
# ********************* Resources ************************
# ********************************************************

FILE(MAKE_DIRECTORY ${PROJECT_BINARY_DIR}/res)

FUNCTION(ADD_RESOURCES)
    FOREACH(resource ${ARGV})
        STRING(MAKE_C_IDENTIFIER ${resource} identifier)
        ADD_CUSTOM_COMMAND(
            OUTPUT  "${PROJECT_BINARY_DIR}/res/${identifier}.o"
            COMMAND ${CMAKE_LINKER} --relocatable --format binary --output "${PROJECT_BINARY_DIR}/res/${identifier}.o" ${resource}
            DEPENDS ${resource}
            WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
        )
    ENDFOREACH()
ENDFUNCTION()



# ********************************************************
# *********************** Helpers ************************
# ********************************************************

SET(HELP++LIBS Tools Configuration Time)

# --------------------------------------------------------

#ADD_EXECUTABLE(fitsloader src/fitsloader.cc)
#TARGET_LINK_LIBRARIES(fitsloader ${HELP++LIBS})
#MANPAGE(fitsloader "")

#ADD_EXECUTABLE(fitscheck src/fitscheck.cc)
#TARGET_LINK_LIBRARIES(fitscheck ${HELP++LIBS} ZLIB::ZLIB)
#MANPAGE(fitscheck "")

#ADD_EXECUTABLE(fitsCompressor src/fitsCompressor.cc)
#TARGET_LINK_LIBRARIES(fitsCompressor ${HELP++LIBS} ZLIB::ZLIB)
#MANPAGE(fitsCompressor "")

#ADD_EXECUTABLE(fitsselect src/fitsselect.cc)
#TARGET_LINK_LIBRARIES(fitsselect ${HELP++LIBS} ZLIB::ZLIB)
#MANPAGE(fitsselect "")

#ADD_EXECUTABLE(readfits src/readfits.cc)
#TARGET_LINK_LIBRARIES(readfits ${HELP++LIBS} ZLIB::ZLIB)
#MANPAGE(readfits "")

IF(NOT NO_ROOT)
   ADD_EXECUTABLE(fitsdump src/fitsdump.cc)
   TARGET_LINK_LIBRARIES(fitsdump ${HELP++LIBS} ${ROOT_LIBRARIES} ZLIB::ZLIB)
   MANPAGE(fitsdump "FACT++ - fitsdump - Read and dump contents of a FITStable")

   ADD_EXECUTABLE(rootifysql src/rootifysql.cc)
   TARGET_LINK_LIBRARIES(rootifysql ${HELP++LIBS} ${ROOT_LIBRARIES})
   MANPAGE(rootifysql "FACT++ - rootifysql - Write result of a SQL query into a root-file")
   ADD_EXECUTABLE(root2sql src/root2sql.cc)
   TARGET_LINK_LIBRARIES(root2sql ${HELP++LIBS} ${ROOT_LIBRARIES})
   MANPAGE(root2sql "FACT++ - root2sql - Fill contents of a root-tree into a MySQL database")

   ADD_EXECUTABLE(root2csv src/root2csv.cc)
   TARGET_LINK_LIBRARIES(root2csv ${HELP++LIBS} ${ROOT_LIBRARIES})
   MANPAGE(root2csv "FACT++ - root2csv - Convert a root-tree to a csv file")

   ADD_EXECUTABLE(csv2root src/csv2root.cc)
   TARGET_LINK_LIBRARIES(csv2root ${HELP++LIBS} ${ROOT_LIBRARIES})
   MANPAGE(csv2root "FACT++ - csv2root - Convert a csv file to a root-tree")
ENDIF()

ADD_EXECUTABLE(fits2sql src/fits2sql.cc)
TARGET_LINK_LIBRARIES(fits2sql ${HELP++LIBS}  ZLIB::ZLIB)
MANPAGE(fits2sql "FACT++ - fits2sql - Fill contents of a FITS table into a MySQL database")

ADD_EXECUTABLE(corsika2sql src/corsika2sql.cc)
TARGET_LINK_LIBRARIES(corsika2sql ${HELP++LIBS})
MANPAGE(corsika2sql "Fills a Corsika Input Card into a SQL table")

ADD_EXECUTABLE(showlog src/showlog.cc src/WindowLog.cc)
TARGET_LINK_LIBRARIES(showlog Threads::Threads ${HELP++LIBS} ${CURSES_LIBRARIES})
MANPAGE(showlog "")

ADD_EXECUTABLE(zfits src/zfits.cc)
TARGET_LINK_LIBRARIES(zfits ${HELP++LIBS} ZLIB::ZLIB)
MANPAGE(zfits "")

IF(NOT NO_ROOT)
   ADD_EXECUTABLE(calcsource src/calcsource.cc)
   TARGET_LINK_LIBRARIES(calcsource ${HELP++LIBS} ${ROOT_LIBRARIES})
   MANPAGE(calcsource "")

   ADD_EXECUTABLE(calcsourcemc src/calcsourcemc.cc)
   TARGET_LINK_LIBRARIES(calcsourcemc ${HELP++LIBS} ${ROOT_LIBRARIES})
   MANPAGE(calcsourcemc "")
ENDIF()

ADD_EXECUTABLE(spectrum src/spectrum.cc src/WindowLog.cc res/spectrum_data_sql.o res/spectrum_simulation_sql.o res/spectrum_analysis_sql.o res/spectrum_spectrum_sql.o res/spectrum_summary_sim_sql.o res/spectrum_summary_est_sql.o)
TARGET_LINK_LIBRARIES(spectrum Threads::Threads ${CURSES_LIBRARIES} ${HIGHLIGHT_LIBRARY} ${HELP++LIBS} ${ROOT_LIBRARIES})
ADD_RESOURCES(spectrum/analysis.sql spectrum/data.sql spectrum/simulation.sql spectrum/spectrum.sql spectrum/summary-est.sql spectrum/summary-sim.sql)
MANPAGE(spectrum "")

ADD_EXECUTABLE(getevent src/getevent.cc)
TARGET_LINK_LIBRARIES(getevent ${HELP++LIBS} ZLIB::ZLIB)
MANPAGE(getevent "")

ENDIF(NOT VIEWER_ONLY)

IF (NOT TOOLS_ONLY AND NOT VIEWER_ONLY)

IF(NOT NO_ROOT)
   ADD_EXECUTABLE(makeplots src/makeplots.cc)
   TARGET_LINK_LIBRARIES(makeplots  ${HELP++LIBS} ${ROOT_LIBRARIES})
   MANPAGE(makeplots "")
ENDIF()

ADD_EXECUTABLE(makedata src/makedata.cc)
TARGET_LINK_LIBRARIES(makedata ${HELP++LIBS})
MANPAGE(makedata "")

ADD_EXECUTABLE(makeschedule src/makeschedule.cc)
TARGET_LINK_LIBRARIES(makeschedule ${HELP++LIBS})
MANPAGE(makeschedule "")

ADD_EXECUTABLE(moon src/moon.cc)
TARGET_LINK_LIBRARIES(moon ${HELP++LIBS})
MANPAGE(moon "")

# ********************************************************
# ********************* Operation ************************
# ********************************************************

SET(FACT++LIBS StateMachine Configuration DimExtension)

# --------------------------------------------------------

ADD_EXECUTABLE(dns src/dns.c)
TARGET_LINK_LIBRARIES(dns ${FACT++LIBS})
MANPAGE(dns "")

ADD_EXECUTABLE(agilentctrl src/agilentctrl.cc)
TARGET_LINK_LIBRARIES(agilentctrl ${FACT++LIBS})
MANPAGE(agilentctrl "")

ADD_EXECUTABLE(biasctrl src/biasctrl.cc)
TARGET_LINK_LIBRARIES(biasctrl ${FACT++LIBS})
MANPAGE(biasctrl "")

ADD_EXECUTABLE(biastemp src/biastemp.cc)
TARGET_LINK_LIBRARIES(biastemp ${FACT++LIBS})
MANPAGE(biastemp "")

ADD_EXECUTABLE(gudectrl src/gudectrl.cc)
TARGET_LINK_LIBRARIES(gudectrl ${FACT++LIBS})
MANPAGE(gudectrl "")

ADD_EXECUTABLE(gtcdust src/gtcdust.cc)
TARGET_LINK_LIBRARIES(gtcdust ${FACT++LIBS})
MANPAGE(gtcdust "")

ADD_EXECUTABLE(chatserv src/chatserv.cc)
TARGET_LINK_LIBRARIES(chatserv ${FACT++LIBS}) # does not require libpanel
MANPAGE(chatserv "")

ADD_EXECUTABLE(chatclient src/chatclient.cc)
TARGET_LINK_LIBRARIES(chatclient ${FACT++LIBS}) # does not require libpanel
MANPAGE(chatclient "")

#ADD_EXECUTABLE(cosyctrl src/cosyctrl.cc)# src/LocalControl.h
#TARGET_LINK_LIBRARIES(cosyctrl
#	StateMachine Tools Configuration Time Dim++ Dim DimExtension
#        ${Boost_LIBRARIES} readline ${CURSES_LIBRARIES})

ADD_EXECUTABLE(datalogger src/datalogger.cc
	src/DimState.cc
	src/FitsFile.cc src/Fits.cc)
TARGET_LINK_LIBRARIES(datalogger ${FACT++LIBS} cfitsio CCfits)
MANPAGE(datalogger "")

ADD_EXECUTABLE(dimctrl src/dimctrl.cc
	src/StateMachineDimControl.cc
        src/RemoteControl.cc
        src/InterpreterV8.cc
        src/DimState.cc)
TARGET_LINK_LIBRARIES(dimctrl ${FACT++LIBS} Pointing ZLIB::ZLIB v8)
MANPAGE(dimctrl "")

ADD_CUSTOM_TARGET(dimserver ALL
	COMMAND ln -sf dimctrl dimserver
        COMMENT "Creating symlink for dimserver"
	DEPENDS dimctrl)

ADD_EXECUTABLE(drivectrl src/drivectrl.cc)
TARGET_LINK_LIBRARIES(drivectrl ${FACT++LIBS} Pointing)
MANPAGE(drivectrl "")

ADD_EXECUTABLE(evtserver src/evtserver.cc)
TARGET_LINK_LIBRARIES(evtserver ${FACT++LIBS} ZLIB::ZLIB)
MANPAGE(evtserver "")

ADD_EXECUTABLE(fadctrl src/fadctrl.cc src/HeadersFAD.cc
	src/EventBuilder.cc
	src/FitsFile.cc
	src/DataCalib.cc
	src/DataWriteRaw.cc
	src/DataWriteFits.cc
	src/DataWriteFits2.cc
	src/DataProcessorImp.cc)
TARGET_LINK_LIBRARIES(fadctrl ${FACT++LIBS} cfitsio CCfits ZLIB::ZLIB)
MANPAGE(fadctrl "")

ADD_EXECUTABLE(feedback src/feedback.cc)
TARGET_LINK_LIBRARIES(feedback ${FACT++LIBS})
MANPAGE(feedback "")

ADD_EXECUTABLE(fscctrl src/fscctrl.cc src/HeadersFTM.cc)
TARGET_LINK_LIBRARIES(fscctrl ${FACT++LIBS})
MANPAGE(fscctrl "")

ADD_EXECUTABLE(ftmctrl src/ftmctrl.cc src/HeadersFTM.cc)
TARGET_LINK_LIBRARIES(ftmctrl ${FACT++LIBS})
MANPAGE(ftmctrl "")

ADD_EXECUTABLE(gcn src/gcn.cc)
TARGET_LINK_LIBRARIES(gcn ${FACT++LIBS} ${Qt}::${QtXml})
MANPAGE(gcn "")

ADD_EXECUTABLE(scheduler src/scheduler.cc)
TARGET_LINK_LIBRARIES(scheduler ${FACT++LIBS})
MANPAGE(scheduler "")

ADD_EXECUTABLE(gpsctrl src/gpsctrl.cc)
TARGET_LINK_LIBRARIES(gpsctrl ${FACT++LIBS})
MANPAGE(gpsctrl "")

ADD_EXECUTABLE(lidctrl src/lidctrl.cc)
TARGET_LINK_LIBRARIES(lidctrl ${FACT++LIBS} ${Qt}::${QtXml})
MANPAGE(lidctrl "")

ADD_EXECUTABLE(magiclidar src/magiclidar.cc)
TARGET_LINK_LIBRARIES(magiclidar ${FACT++LIBS})
MANPAGE(magiclidar "")

ADD_EXECUTABLE(magicweather src/magicweather.cc)
TARGET_LINK_LIBRARIES(magicweather ${FACT++LIBS})
MANPAGE(magicweather "")

ADD_EXECUTABLE(mcp src/mcp.cc)
TARGET_LINK_LIBRARIES(mcp ${FACT++LIBS})
MANPAGE(mcp "")

ADD_EXECUTABLE(pfminictrl src/pfminictrl.cc)
TARGET_LINK_LIBRARIES(pfminictrl ${FACT++LIBS})
MANPAGE(pfminictrl "")

ADD_EXECUTABLE(pwrctrl src/pwrctrl.cc src/HeadersPower.cc)
TARGET_LINK_LIBRARIES(pwrctrl ${FACT++LIBS} ${Qt}::${QtXml})
MANPAGE(pwrctrl "")

ADD_EXECUTABLE(rainsensor src/rainsensor.cc)
TARGET_LINK_LIBRARIES(rainsensor ${FACT++LIBS})
MANPAGE(rainsensor "")

ADD_EXECUTABLE(ratecontrol src/ratecontrol.cc)
TARGET_LINK_LIBRARIES(ratecontrol ${FACT++LIBS})
MANPAGE(ratecontrol "")

ADD_EXECUTABLE(ratescan src/ratescan.cc)
TARGET_LINK_LIBRARIES(ratescan ${FACT++LIBS})
MANPAGE(ratescan "")

#ADD_EXECUTABLE(scheduler src/scheduler.cc)
#TARGET_LINK_LIBRARIES(scheduler ${FACT++LIBS})
#MANPAGE(scheduler "")

#ADD_EXECUTABLE(skypeclient src/skypeclient.cc)
#TARGET_LINK_LIBRARIES(skypeclient ${FACT++LIBS})
#MANPAGE(skypeclient "")

ADD_EXECUTABLE(sqmctrl src/sqmctrl.cc)
TARGET_LINK_LIBRARIES(sqmctrl ${FACT++LIBS})
MANPAGE(sqmctrl "")

ADD_EXECUTABLE(smartfact src/smartfact.cc)
TARGET_LINK_LIBRARIES(smartfact ${FACT++LIBS}) # HAVE_NOVA
MANPAGE(smartfact "")

ADD_EXECUTABLE(temperature src/temperature.cc)
TARGET_LINK_LIBRARIES(temperature ${FACT++LIBS})
MANPAGE(temperature "")

ADD_EXECUTABLE(timecheck src/timecheck.cc)
TARGET_LINK_LIBRARIES(timecheck ${FACT++LIBS})
MANPAGE(timecheck "")

ADD_EXECUTABLE(tngweather src/tngweather.cc)
TARGET_LINK_LIBRARIES(tngweather ${FACT++LIBS} ${Qt}::${QtXml})
MANPAGE(tngweather "")

#ADD_EXECUTABLE(triggerschedule src/triggerschedule.cc)
#TARGET_LINK_LIBRARIES(triggerschedule ${FACT++LIBS})
#MANPAGE(triggerschedule "")

ENDIF(NOT TOOLS_ONLY AND NOT VIEWER_ONLY)

# ********************************************************
# ****************** GUI Applications ********************
# ********************************************************

IF (NOT TOOLS_ONLY OR VIEWER_ONLY)

INCLUDE_DIRECTORIES(${QWT_INCLUDE_DIR} gui gui/RawEventsViewer)

# --------------------------------------------------------

IF(QT4_FOUND)
   QT4_WRAP_UI(VIEWER_QT_HEADERS gui/RawEventsViewer/viewer.ui)
   QT4_WRAP_CPP(VIEWER_QT_SOURCES
	gui/RawEventsViewer/RawEventsViewer.h
	gui/BasicGlCamera.h
	gui/QCameraWidget.h
	gui/Q3DCameraWidget.h)
ELSE()
   QT5_WRAP_UI(VIEWER_QT_HEADERS gui/RawEventsViewer/viewer.ui)
   QT5_WRAP_CPP(VIEWER_QT_SOURCES
	gui/RawEventsViewer/RawEventsViewer.h
	gui/BasicGlCamera.h
	gui/QCameraWidget.h
	gui/Q3DCameraWidget.h)
ENDIF()

ADD_EXECUTABLE(viewer
	gui/RawEventsViewer/RawEventsViewer.cc
	gui/BasicGlCamera.cc
	gui/QCameraWidget.cc
	gui/Q3DCameraWidget.cc
        ${VIEWER_QT_HEADERS}
        ${VIEWER_QT_SOURCES})

TARGET_LINK_LIBRARIES(viewer
	${Boost_LIBRARIES} Configuration
        ${Qt}::${QtGui} ${Qt}::${QtOpenGL}
        ${OPENGL_LIBRARIES} # OpenGL::GL OpenGL::GLU
        ${QWT_LIBRARY} ZLIB::ZLIB)

#ENDIF(NOT TOOLS_ONLY OR VIEWER_ONLY)

# --------------------------------------------------------

IF(NOT VIEWER_ONLY AND ROOT_GQt_FOUND AND Qt4_FOUND)

QT4_ADD_RESOURCES(FACT_QT_RESOURCES gui/design.qrc)

QT4_WRAP_UI(FACT_QT_HEADERS gui/design.ui)

QT4_WRAP_CPP(FACT_QT_SOURCES
	gui/DockWindow.h
	gui/MainWindow.h
	gui/QCameraWidget.h
	gui/BasicGlCamera.h)

ADD_EXECUTABLE(fact gui/fact.cc
	gui/DockWindow.cc
	gui/MainWindow.cc
	gui/QCameraWidget.cc
	gui/BasicGlCamera.cc
	gui/HtmlDelegate.cc
	gui/CheckBoxDelegate.cc
        src/DimNetwork.cc
        ${FACT_QT_HEADERS}
        ${FACT_QT_SOURCES}
        ${FACT_QT_RESOURCES})

TARGET_LINK_LIBRARIES(fact
	${FACT++LIBS}
        ${ROOT_LIBRARIES}
        ${Qt}::${QtGui} ${Qt}::${QtOpenGL} ${Qt}::${QtSql}
        ${OPENGL_LIBRARIES} # OpenGL::GL OpenGL::GLU
	${QWT_LIBRARY} ZLIB::ZLIB)


#dclient5_SOURCES = src/dclient5.cc src/LocalControl.h
#dclient5_LDADD   = libDim++.la libDim.la libStateMachine.la libTime.la libTools.la \
#    libConfiguration.la

#argv_SOURCES = src/argv.cc
#argv_LDADD = libConfiguration.la

#time_SOURCES = src/time.cc
#time_LDADD = libTime.la

#test_SOURCES = src/test.cc
#test_LDADD = libStateMachine.la libTools.la libTime.la libDim++.la libDim.la libConfiguration.la

ENDIF(NOT VIEWER_ONLY AND ROOT_GQt_FOUND AND Qt4_FOUND)
ENDIF(NOT TOOLS_ONLY OR VIEWER_ONLY)


# ********************************************************
# ********************** FAMOUS **************************
# ********************************************************

IF(FOUND_FAMOUS)
	INCLUDE_DIRECTORIES(famous/ui)
	INCLUDE_DIRECTORIES(famous/src)

	ADD_EXECUTABLE(famousbias famous/src/famousbias.cc)
	TARGET_LINK_LIBRARIES(famousbias ${FACT++LIBS})
	MANPAGE(famousbias "")

	ADD_EXECUTABLE(famousui famous/ui/famousui.cc)
	TARGET_LINK_LIBRARIES(famousui ${FACT++LIBS})
	MANPAGE(famousui "")

	ADD_EXECUTABLE(ftuctrl famous/src/ftuctrl.cc)
	TARGET_LINK_LIBRARIES(ftuctrl ${FACT++LIBS})
	MANPAGE(ftuctrl "")

	ADD_EXECUTABLE(miniftmctrl famous/src/miniftmctrl.cc)
	TARGET_LINK_LIBRARIES(miniftmctrl ${FACT++LIBS})
	MANPAGE(miniftmctrl "")

	ADD_EXECUTABLE(fpgaftmctrl famous/src/fpgaftmctrl.cc)
	TARGET_LINK_LIBRARIES(fpgaftmctrl ${FACT++LIBS})
	MANPAGE(fpgaftmctrl "")

	ADD_EXECUTABLE(famousrate famous/src/famousrate.cc)
	TARGET_LINK_LIBRARIES(famousrate ${FACT++LIBS})
	MANPAGE(famousrate "")

	ADD_EXECUTABLE(famousscan famous/src/famousscan.cc)
	TARGET_LINK_LIBRARIES(famousscan ${FACT++LIBS})
	MANPAGE(famousscan "")

	ADD_EXECUTABLE(beaglectrl famous/src/beaglectrl.cc)
	TARGET_LINK_LIBRARIES(beaglectrl ${FACT++LIBS} ${Qt}::${QtXml})
 	MANPAGE(beaglectrl "")
ENDIF()



# ********************************************************
# ********************** IceACT **************************
# ********************************************************

IF(FOUND_ICEACT)
	ADD_EXECUTABLE(iceactpsu iceact/src/psuctrl.cc)
	TARGET_LINK_LIBRARIES(iceactpsu ${FACT++LIBS})
	MANPAGE(iceactpsu "")

	ADD_EXECUTABLE(tmonctrl iceact/src/tmonctrl.cc)
	TARGET_LINK_LIBRARIES(tmonctrl ${FACT++LIBS})
	MANPAGE(tmonctrl "")
ENDIF()


# *********************************
# ********** Installation *********
# *********************************

# ======== Setup directories ======

# MESSAGE("")
# MESSAGE("PREFIX:         "${CMAKE_INSTALL_PREFIX})
# MESSAGE("")
# MESSAGE("BINDIR:         "${CMAKE_INSTALL_FULL_BINDIR})
# MESSAGE("SBINDIR:        "${CMAKE_INSTALL_FULL_SBINDIR})
# MESSAGE("LIBEXECDIR:     "${CMAKE_INSTALL_FULL_LIBEXECDIR})
# MESSAGE("SHAREDSTATEDIR: "${CMAKE_INSTALL_FULL_SHAREDSTATEDIR})
# MESSAGE("LOCALSTATEDIR:  "${CMAKE_INSTALL_FULL_LOCALSTATEDIR})
# MESSAGE("RUNSTATEDIR:    "${CMAKE_INSTALL_FULL_RUNSTATEDIR})
# MESSAGE("LIBDIR:         "${CMAKE_INSTALL_FULL_LIBDIR})
# MESSAGE("INCLUDEDIR:     "${CMAKE_INSTALL_FULL_INCLUDEDIR})
# MESSAGE("DATAROOTDIR:    "${CMAKE_INSTALL_FULL_DATAROOTDIR})
# MESSAGE("DATADIR:        "${CMAKE_INSTALL_FULL_DATADIR})
# MESSAGE("INFODIR:        "${CMAKE_INSTALL_FULL_INFODIR})
# MESSAGE("LOCALDIR:       "${CMAKE_INSTALL_FULL_LOCALDIR})
# MESSAGE("MANDIR:         "${CMAKE_INSTALL_FULL_MANDIR})
# MESSAGE("DOCDIR:         "${CMAKE_INSTALL_FULL_DOCDIR})
# MESSAGE("")
# MESSAGE("BINDIR:         "${CMAKE_INSTALL_BINDIR})
# MESSAGE("SBINDIR:        "${CMAKE_INSTALL_SBINDIR})
# MESSAGE("LIBEXECDIR:     "${CMAKE_INSTALL_LIBEXECDIR})
# MESSAGE("SHAREDSTATEDIR: "${CMAKE_INSTALL_SHAREDSTATEDIR})
# MESSAGE("LOCALSTATEDIR:  "${CMAKE_INSTALL_LOCALSTATEDIR})
# MESSAGE("RUNSTATEDIR:    "${CMAKE_INSTALL_RUNSTATEDIR})
# MESSAGE("LIBDIR:         "${CMAKE_INSTALL_LIBDIR})
# MESSAGE("INCLUDEDIR:     "${CMAKE_INSTALL_INCLUDEDIR})
# MESSAGE("DATAROOTDIR:    "${CMAKE_INSTALL_DATAROOTDIR})
# MESSAGE("DATADIR:        "${CMAKE_INSTALL_DATADIR})
# MESSAGE("INFODIR:        "${CMAKE_INSTALL_INFODIR})
# MESSAGE("LOCALDIR:       "${CMAKE_INSTALL_LOCALDIR})
# MESSAGE("MANDIR:         "${CMAKE_INSTALL_MANDIR})
# MESSAGE("DOCDIR:         "${CMAKE_INSTALL_DOCDIR})
# MESSAGE("")

SET(PKG_SHARE    "${CMAKE_INSTALL_DATAROOTDIR}/${PROJECT_NAME}")
SET(PKG_APPS     "${CMAKE_INSTALL_DATAROOTDIR}/applications")
SET(PKG_PIXMAPS  "${CMAKE_INSTALL_DATAROOTDIR}/pixmaps")

# ====== Installation targets =====

IF (NOT VIEWER_ONLY)

   INSTALL(TARGETS  fits2sql       DESTINATION "${CMAKE_INSTALL_BINDIR}")
   INSTALL(TARGETS  zfits          DESTINATION "${CMAKE_INSTALL_BINDIR}")
   INSTALL(TARGETS  showlog        DESTINATION "${CMAKE_INSTALL_BINDIR}")

   IF(NOT NO_ROOT)
      INSTALL(TARGETS  rootifysql     DESTINATION "${CMAKE_INSTALL_BINDIR}")
      INSTALL(TARGETS  root2sql       DESTINATION "${CMAKE_INSTALL_BINDIR}")
      INSTALL(TARGETS  root2csv       DESTINATION "${CMAKE_INSTALL_BINDIR}")
      INSTALL(TARGETS  csv2root       DESTINATION "${CMAKE_INSTALL_BINDIR}")
      INSTALL(TARGETS  fitsdump       DESTINATION "${CMAKE_INSTALL_BINDIR}")
      INSTALL(TARGETS  calcsource     DESTINATION "${CMAKE_INSTALL_BINDIR}")
      INSTALL(TARGETS  calcsourcemc   DESTINATION "${CMAKE_INSTALL_BINDIR}")
   ENDIF()

ENDIF(NOT VIEWER_ONLY)

INSTALL(TARGETS  Configuration  DESTINATION "${CMAKE_INSTALL_LIBDIR}")
INSTALL(TARGETS  Time           DESTINATION "${CMAKE_INSTALL_LIBDIR}")
INSTALL(TARGETS  Tools          DESTINATION "${CMAKE_INSTALL_LIBDIR}")


#INSTALL(DIRECTORY ${PROJECT_BINARY_DIR}/html DESTINATION  "${PKG_SHARE}" FILES_MATCHING PATTERN "*.html")
#INSTALL(DIRECTORY ${PROJECT_BINARY_DIR}/pdf  DESTINATION  "${PKG_SHARE}" FILES_MATCHING PATTERN "*.pdf")

# *********************************
# ************ Packaging **********
# *********************************

# ======== General packaging ======
# https://cmake.org/cmake/help/latest/module/CPack.html

INCLUDE(InstallRequiredSystemLibraries)

# SET(CPACK_STRIP_FILES ON) # Strip debug sysmbold from files

SET(CPACK_PACKAGE_VERSION       "${fact++_VERSION}")
SET(CPACK_PACKAGE_CONTACT       "${PACKAGE_BUGREPORT}")
SET(CPACK_PACKAGE_MAINTAINER    "${PACKAGE_MAINTAINER}")
SET(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_SOURCE_DIR}/COPYING")
#SET(CPACK_PACKAGE_VENDOR "Me, myself, and I")


SET(CPACK_SOURCE_IGNORE_FILES "/old/;/build/;/[.].*/;/autom4te[.]cache/;.*~;.log$")

# Maybe the text displayed in synaptic?
SET(CPACK_PACKAGE_DESCRIPTION_FILE "${CMAKE_SOURCE_DIR}/README")
SET(CPACK_PACKAGE_DESCRIPTION_SUMMARY "FACT++ - Toolset")
#CPACK_RESOURCE_FILE_README     ReadMe file to be embedded in the installer.

SET(CPACK_GENERATOR        TBZ2 DEB) # TGZ
SET(CPACK_SOURCE_GENERATOR TBZ2)     # TGZ
#SET(CPACK_PROJECT_CONFIG_FILE ${CMAKE_SOURCE_DIR}/cmake/Package.txt)
#SET(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/Copyright.txt")


# ======== Debian packaging =======
# https://cmake.org/cmake/help/latest/module/CPackDeb.html
# https://www.debian.org/doc/debian-policy

#SET(CPACK_DEBIAN_PACKAGE_DEPENDS "libao4 (>=1.0), libsfm-system2.3v5 (>=2.3), libsfml-graphicsv2.3v5 (>=2.3), libsfml-audio2.3v5 (>=2.3), libsfm-window2.3v5 (>2.3)")
SET(CPACK_DEBIAN_PACKAGE_SHLIBDEPS 1) # generate better (atomatic?) dependecies
SET(CPACK_DEBIAN_PACKAGE_GENERATE_SHLIBS_POLICY ">=")
SET(CPACK_DEBIAN_PACKAGE_CONTROL_STRICT_PERMISSION TRUE)
SET(CPACK_DEBIAN_PACKAGE_SECTION utils)
SET(CPACK_DEBIAN_PACKAGE_HOMEPAGE "${PACKAGE_URL}")
SET(CPACK_DEBIAN_PACKAGE_DESCRIPTION
"${CPACK_PACKAGE_DESCRIPTION_SUMMARY}
 This package contains some tools which are part of FACT++ to deal with databases, fits files and root-files.")
#indent the long description with spaces

#CPACK_INSTALL_CMAKE_PROJECTS
#List of four values that specify what project to install. Build directory, Project Name, Project Component, Directory.

INCLUDE(CPack)
