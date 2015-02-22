# Install script for directory: D:/workspace/dcmtk-3.5.4/dcmqrdb/include/dcmtk/dcmqrdb

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
  FILE(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/dcmtk/dcmqrdb" TYPE FILE FILES
    "D:/workspace/dcmtk-3.5.4/dcmqrdb/include/dcmtk/dcmqrdb/dcmqrcbf.h"
    "D:/workspace/dcmtk-3.5.4/dcmqrdb/include/dcmtk/dcmqrdb/dcmqrcbg.h"
    "D:/workspace/dcmtk-3.5.4/dcmqrdb/include/dcmtk/dcmqrdb/dcmqrcbm.h"
    "D:/workspace/dcmtk-3.5.4/dcmqrdb/include/dcmtk/dcmqrdb/dcmqrcbs.h"
    "D:/workspace/dcmtk-3.5.4/dcmqrdb/include/dcmtk/dcmqrdb/dcmqrcnf.h"
    "D:/workspace/dcmtk-3.5.4/dcmqrdb/include/dcmtk/dcmqrdb/dcmqrdba.h"
    "D:/workspace/dcmtk-3.5.4/dcmqrdb/include/dcmtk/dcmqrdb/dcmqrdbi.h"
    "D:/workspace/dcmtk-3.5.4/dcmqrdb/include/dcmtk/dcmqrdb/dcmqrdbs.h"
    "D:/workspace/dcmtk-3.5.4/dcmqrdb/include/dcmtk/dcmqrdb/dcmqridx.h"
    "D:/workspace/dcmtk-3.5.4/dcmqrdb/include/dcmtk/dcmqrdb/dcmqropt.h"
    "D:/workspace/dcmtk-3.5.4/dcmqrdb/include/dcmtk/dcmqrdb/dcmqrptb.h"
    "D:/workspace/dcmtk-3.5.4/dcmqrdb/include/dcmtk/dcmqrdb/dcmqrsrv.h"
    "D:/workspace/dcmtk-3.5.4/dcmqrdb/include/dcmtk/dcmqrdb/dcmqrtis.h"
    )
ENDIF(NOT CMAKE_INSTALL_COMPONENT OR "${CMAKE_INSTALL_COMPONENT}" STREQUAL "Unspecified")

