# Install script for directory: ${CMAKE_SOURCE_DIR}/ofstd/include/dcmtk/ofstd

# Set the install prefix
IF(NOT DEFINED CMAKE_INSTALL_PREFIX)
  SET(CMAKE_INSTALL_PREFIX "${CMAKE_SOURCE_DIR}/../dcmtk-3.5.4-win32-i386")
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
    "${CMAKE_SOURCE_DIR}/ofstd/include/dcmtk/ofstd/ofalgo.h"
    "${CMAKE_SOURCE_DIR}/ofstd/include/dcmtk/ofstd/ofbmanip.h"
    "${CMAKE_SOURCE_DIR}/ofstd/include/dcmtk/ofstd/ofcast.h"
    "${CMAKE_SOURCE_DIR}/ofstd/include/dcmtk/ofstd/ofcmdln.h"
    "${CMAKE_SOURCE_DIR}/ofstd/include/dcmtk/ofstd/ofconapp.h"
    "${CMAKE_SOURCE_DIR}/ofstd/include/dcmtk/ofstd/ofcond.h"
    "${CMAKE_SOURCE_DIR}/ofstd/include/dcmtk/ofstd/ofconfig.h"
    "${CMAKE_SOURCE_DIR}/ofstd/include/dcmtk/ofstd/ofconsol.h"
    "${CMAKE_SOURCE_DIR}/ofstd/include/dcmtk/ofstd/ofcrc32.h"
    "${CMAKE_SOURCE_DIR}/ofstd/include/dcmtk/ofstd/ofdate.h"
    "${CMAKE_SOURCE_DIR}/ofstd/include/dcmtk/ofstd/ofdatime.h"
    "${CMAKE_SOURCE_DIR}/ofstd/include/dcmtk/ofstd/offname.h"
    "${CMAKE_SOURCE_DIR}/ofstd/include/dcmtk/ofstd/ofglobal.h"
    "${CMAKE_SOURCE_DIR}/ofstd/include/dcmtk/ofstd/oflist.h"
    "${CMAKE_SOURCE_DIR}/ofstd/include/dcmtk/ofstd/oflogfil.h"
    "${CMAKE_SOURCE_DIR}/ofstd/include/dcmtk/ofstd/ofoset.h"
    "${CMAKE_SOURCE_DIR}/ofstd/include/dcmtk/ofstd/ofset.h"
    "${CMAKE_SOURCE_DIR}/ofstd/include/dcmtk/ofstd/ofsetit.h"
    "${CMAKE_SOURCE_DIR}/ofstd/include/dcmtk/ofstd/ofstack.h"
    "${CMAKE_SOURCE_DIR}/ofstd/include/dcmtk/ofstd/ofstd.h"
    "${CMAKE_SOURCE_DIR}/ofstd/include/dcmtk/ofstd/ofstdinc.h"
    "${CMAKE_SOURCE_DIR}/ofstd/include/dcmtk/ofstd/ofstream.h"
    "${CMAKE_SOURCE_DIR}/ofstd/include/dcmtk/ofstd/ofstring.h"
    "${CMAKE_SOURCE_DIR}/ofstd/include/dcmtk/ofstd/ofthread.h"
    "${CMAKE_SOURCE_DIR}/ofstd/include/dcmtk/ofstd/oftime.h"
    "${CMAKE_SOURCE_DIR}/ofstd/include/dcmtk/ofstd/oftimer.h"
    "${CMAKE_SOURCE_DIR}/ofstd/include/dcmtk/ofstd/oftypes.h"
    "${CMAKE_SOURCE_DIR}/ofstd/include/dcmtk/ofstd/ofuoset.h"
    )
ENDIF(NOT CMAKE_INSTALL_COMPONENT OR "${CMAKE_INSTALL_COMPONENT}" STREQUAL "Unspecified")

