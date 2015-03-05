# Install script for directory: D:/workspace/dcmtk-3.5.4-trunk/dcmnet/include/dcmtk/dcmnet

# Set the install prefix
IF(NOT DEFINED CMAKE_INSTALL_PREFIX)
  SET(CMAKE_INSTALL_PREFIX "D:/workspace/dcmtk-3.5.4-trunk/../dcmtk-3.5.4-win32-i386")
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
  FILE(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/dcmtk/dcmnet" TYPE FILE FILES
    "D:/workspace/dcmtk-3.5.4-trunk/dcmnet/include/dcmtk/dcmnet/assoc.h"
    "D:/workspace/dcmtk-3.5.4-trunk/dcmnet/include/dcmtk/dcmnet/cond.h"
    "D:/workspace/dcmtk-3.5.4-trunk/dcmnet/include/dcmtk/dcmnet/dcasccff.h"
    "D:/workspace/dcmtk-3.5.4-trunk/dcmnet/include/dcmtk/dcmnet/dcasccfg.h"
    "D:/workspace/dcmtk-3.5.4-trunk/dcmnet/include/dcmtk/dcmnet/dccfenmp.h"
    "D:/workspace/dcmtk-3.5.4-trunk/dcmnet/include/dcmtk/dcmnet/dccfpcmp.h"
    "D:/workspace/dcmtk-3.5.4-trunk/dcmnet/include/dcmtk/dcmnet/dccfprmp.h"
    "D:/workspace/dcmtk-3.5.4-trunk/dcmnet/include/dcmtk/dcmnet/dccfrsmp.h"
    "D:/workspace/dcmtk-3.5.4-trunk/dcmnet/include/dcmtk/dcmnet/dccftsmp.h"
    "D:/workspace/dcmtk-3.5.4-trunk/dcmnet/include/dcmtk/dcmnet/dccfuidh.h"
    "D:/workspace/dcmtk-3.5.4-trunk/dcmnet/include/dcmtk/dcmnet/dcmlayer.h"
    "D:/workspace/dcmtk-3.5.4-trunk/dcmnet/include/dcmtk/dcmnet/dcmsmap.h"
    "D:/workspace/dcmtk-3.5.4-trunk/dcmnet/include/dcmtk/dcmnet/dcmtrans.h"
    "D:/workspace/dcmtk-3.5.4-trunk/dcmnet/include/dcmtk/dcmnet/dcompat.h"
    "D:/workspace/dcmtk-3.5.4-trunk/dcmnet/include/dcmtk/dcmnet/dicom.h"
    "D:/workspace/dcmtk-3.5.4-trunk/dcmnet/include/dcmtk/dcmnet/dimse.h"
    "D:/workspace/dcmtk-3.5.4-trunk/dcmnet/include/dcmtk/dcmnet/diutil.h"
    "D:/workspace/dcmtk-3.5.4-trunk/dcmnet/include/dcmtk/dcmnet/dul.h"
    "D:/workspace/dcmtk-3.5.4-trunk/dcmnet/include/dcmtk/dcmnet/extneg.h"
    "D:/workspace/dcmtk-3.5.4-trunk/dcmnet/include/dcmtk/dcmnet/lst.h"
    )
ENDIF(NOT CMAKE_INSTALL_COMPONENT OR "${CMAKE_INSTALL_COMPONENT}" STREQUAL "Unspecified")

