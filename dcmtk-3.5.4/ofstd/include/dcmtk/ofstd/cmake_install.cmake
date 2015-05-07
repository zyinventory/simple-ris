# Install script for directory: D:/workspace/dcmtk-3.5.4/ofstd/include/dcmtk/ofstd

# Set the install prefix
IF(NOT DEFINED CMAKE_INSTALL_PREFIX)
  SET(CMAKE_INSTALL_PREFIX "D:/workspace/dcmtk-3.5.4/../dcmtk-3.5.4-win32-i386")
ENDIF(NOT DEFINED CMAKE_INSTALL_PREFIX)
STRING(REGEX REPLACE "/$" "" CMAKE_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}")

# Set the install configuration name.
IF(NOT DEFINED CMAKE_INSTALL_CONFIG_NAME)
  IF(BUILD_TYPE)
    STRING(REGEX REPLACE "^[^A-Za-z0-9_]+" ""
           CMAKE_INSTALL_CONFIG_NAME "${BUILD_TYPE}")
  ELSE(BUILD_TYPE)
    SET(CMAKE_INSTALL_CONFIG_NAME "Release")
  ENDIF(BUILD_TYPE)
  MESSAGE(STATUS "Install configuration: \"${CMAKE_INSTALL_CONFIG_NAME}\"")
ENDIF(NOT DEFINED CMAKE_INSTALL_CONFIG_NAME)

# Set the component getting installed.
IF(NOT CMAKE_INSTALL_COMPONENT)
  IF(COMPONENT)
    MESSAGE(STATUS "Install component: \"${COMPONENT}\"")
    SET(CMAKE_INSTALL_COMPONENT "${COMPONENT}")
  ELSE(COMPONENT)
    SET(CMAKE_INSTALL_COMPONENT)
  ENDIF(COMPONENT)
ENDIF(NOT CMAKE_INSTALL_COMPONENT)

IF(NOT CMAKE_INSTALL_COMPONENT OR "${CMAKE_INSTALL_COMPONENT}" STREQUAL "Unspecified")
  FILE(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/dcmtk/ofstd" TYPE FILE FILES
    "D:/workspace/dcmtk-3.5.4/ofstd/include/dcmtk/ofstd/ofalgo.h"
    "D:/workspace/dcmtk-3.5.4/ofstd/include/dcmtk/ofstd/ofbmanip.h"
    "D:/workspace/dcmtk-3.5.4/ofstd/include/dcmtk/ofstd/ofcast.h"
    "D:/workspace/dcmtk-3.5.4/ofstd/include/dcmtk/ofstd/ofcmdln.h"
    "D:/workspace/dcmtk-3.5.4/ofstd/include/dcmtk/ofstd/ofconapp.h"
    "D:/workspace/dcmtk-3.5.4/ofstd/include/dcmtk/ofstd/ofcond.h"
    "D:/workspace/dcmtk-3.5.4/ofstd/include/dcmtk/ofstd/ofconfig.h"
    "D:/workspace/dcmtk-3.5.4/ofstd/include/dcmtk/ofstd/ofconsol.h"
    "D:/workspace/dcmtk-3.5.4/ofstd/include/dcmtk/ofstd/ofcrc32.h"
    "D:/workspace/dcmtk-3.5.4/ofstd/include/dcmtk/ofstd/ofdate.h"
    "D:/workspace/dcmtk-3.5.4/ofstd/include/dcmtk/ofstd/ofdatime.h"
    "D:/workspace/dcmtk-3.5.4/ofstd/include/dcmtk/ofstd/offname.h"
    "D:/workspace/dcmtk-3.5.4/ofstd/include/dcmtk/ofstd/ofglobal.h"
    "D:/workspace/dcmtk-3.5.4/ofstd/include/dcmtk/ofstd/oflist.h"
    "D:/workspace/dcmtk-3.5.4/ofstd/include/dcmtk/ofstd/oflogfil.h"
    "D:/workspace/dcmtk-3.5.4/ofstd/include/dcmtk/ofstd/ofoset.h"
    "D:/workspace/dcmtk-3.5.4/ofstd/include/dcmtk/ofstd/ofset.h"
    "D:/workspace/dcmtk-3.5.4/ofstd/include/dcmtk/ofstd/ofsetit.h"
    "D:/workspace/dcmtk-3.5.4/ofstd/include/dcmtk/ofstd/ofstack.h"
    "D:/workspace/dcmtk-3.5.4/ofstd/include/dcmtk/ofstd/ofstd.h"
    "D:/workspace/dcmtk-3.5.4/ofstd/include/dcmtk/ofstd/ofstdinc.h"
    "D:/workspace/dcmtk-3.5.4/ofstd/include/dcmtk/ofstd/ofstream.h"
    "D:/workspace/dcmtk-3.5.4/ofstd/include/dcmtk/ofstd/ofstring.h"
    "D:/workspace/dcmtk-3.5.4/ofstd/include/dcmtk/ofstd/ofthread.h"
    "D:/workspace/dcmtk-3.5.4/ofstd/include/dcmtk/ofstd/oftime.h"
    "D:/workspace/dcmtk-3.5.4/ofstd/include/dcmtk/ofstd/oftimer.h"
    "D:/workspace/dcmtk-3.5.4/ofstd/include/dcmtk/ofstd/oftypes.h"
    "D:/workspace/dcmtk-3.5.4/ofstd/include/dcmtk/ofstd/ofuoset.h"
    )
ENDIF(NOT CMAKE_INSTALL_COMPONENT OR "${CMAKE_INSTALL_COMPONENT}" STREQUAL "Unspecified")

