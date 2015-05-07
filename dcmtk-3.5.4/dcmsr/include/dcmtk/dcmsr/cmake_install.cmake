# Install script for directory: ${CMAKE_SOURCE_DIR}/dcmsr/include/dcmtk/dcmsr

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
  FILE(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/dcmtk/dcmsr" TYPE FILE FILES
    "${CMAKE_SOURCE_DIR}/dcmsr/include/dcmtk/dcmsr/dsrbascc.h"
    "${CMAKE_SOURCE_DIR}/dcmsr/include/dcmtk/dcmsr/dsrchecc.h"
    "${CMAKE_SOURCE_DIR}/dcmsr/include/dcmtk/dcmsr/dsrcitem.h"
    "${CMAKE_SOURCE_DIR}/dcmsr/include/dcmtk/dcmsr/dsrcodtn.h"
    "${CMAKE_SOURCE_DIR}/dcmsr/include/dcmtk/dcmsr/dsrcodvl.h"
    "${CMAKE_SOURCE_DIR}/dcmsr/include/dcmtk/dcmsr/dsrcomcc.h"
    "${CMAKE_SOURCE_DIR}/dcmsr/include/dcmtk/dcmsr/dsrcomtn.h"
    "${CMAKE_SOURCE_DIR}/dcmsr/include/dcmtk/dcmsr/dsrcomvl.h"
    "${CMAKE_SOURCE_DIR}/dcmsr/include/dcmtk/dcmsr/dsrcontn.h"
    "${CMAKE_SOURCE_DIR}/dcmsr/include/dcmtk/dcmsr/dsrcsidl.h"
    "${CMAKE_SOURCE_DIR}/dcmsr/include/dcmtk/dcmsr/dsrdattn.h"
    "${CMAKE_SOURCE_DIR}/dcmsr/include/dcmtk/dcmsr/dsrdoc.h"
    "${CMAKE_SOURCE_DIR}/dcmsr/include/dcmtk/dcmsr/dsrdoctn.h"
    "${CMAKE_SOURCE_DIR}/dcmsr/include/dcmtk/dcmsr/dsrdoctr.h"
    "${CMAKE_SOURCE_DIR}/dcmsr/include/dcmtk/dcmsr/dsrdtitn.h"
    "${CMAKE_SOURCE_DIR}/dcmsr/include/dcmtk/dcmsr/dsrenhcc.h"
    "${CMAKE_SOURCE_DIR}/dcmsr/include/dcmtk/dcmsr/dsrimgfr.h"
    "${CMAKE_SOURCE_DIR}/dcmsr/include/dcmtk/dcmsr/dsrimgtn.h"
    "${CMAKE_SOURCE_DIR}/dcmsr/include/dcmtk/dcmsr/dsrimgvl.h"
    "${CMAKE_SOURCE_DIR}/dcmsr/include/dcmtk/dcmsr/dsriodcc.h"
    "${CMAKE_SOURCE_DIR}/dcmsr/include/dcmtk/dcmsr/dsrkeycc.h"
    "${CMAKE_SOURCE_DIR}/dcmsr/include/dcmtk/dcmsr/dsrmamcc.h"
    "${CMAKE_SOURCE_DIR}/dcmsr/include/dcmtk/dcmsr/dsrnumtn.h"
    "${CMAKE_SOURCE_DIR}/dcmsr/include/dcmtk/dcmsr/dsrnumvl.h"
    "${CMAKE_SOURCE_DIR}/dcmsr/include/dcmtk/dcmsr/dsrpnmtn.h"
    "${CMAKE_SOURCE_DIR}/dcmsr/include/dcmtk/dcmsr/dsrprocc.h"
    "${CMAKE_SOURCE_DIR}/dcmsr/include/dcmtk/dcmsr/dsrreftn.h"
    "${CMAKE_SOURCE_DIR}/dcmsr/include/dcmtk/dcmsr/dsrscogr.h"
    "${CMAKE_SOURCE_DIR}/dcmsr/include/dcmtk/dcmsr/dsrscotn.h"
    "${CMAKE_SOURCE_DIR}/dcmsr/include/dcmtk/dcmsr/dsrscovl.h"
    "${CMAKE_SOURCE_DIR}/dcmsr/include/dcmtk/dcmsr/dsrsoprf.h"
    "${CMAKE_SOURCE_DIR}/dcmsr/include/dcmtk/dcmsr/dsrstrvl.h"
    "${CMAKE_SOURCE_DIR}/dcmsr/include/dcmtk/dcmsr/dsrtcodt.h"
    "${CMAKE_SOURCE_DIR}/dcmsr/include/dcmtk/dcmsr/dsrtcosp.h"
    "${CMAKE_SOURCE_DIR}/dcmsr/include/dcmtk/dcmsr/dsrtcotn.h"
    "${CMAKE_SOURCE_DIR}/dcmsr/include/dcmtk/dcmsr/dsrtcoto.h"
    "${CMAKE_SOURCE_DIR}/dcmsr/include/dcmtk/dcmsr/dsrtcovl.h"
    "${CMAKE_SOURCE_DIR}/dcmsr/include/dcmtk/dcmsr/dsrtextn.h"
    "${CMAKE_SOURCE_DIR}/dcmsr/include/dcmtk/dcmsr/dsrtimtn.h"
    "${CMAKE_SOURCE_DIR}/dcmsr/include/dcmtk/dcmsr/dsrtlist.h"
    "${CMAKE_SOURCE_DIR}/dcmsr/include/dcmtk/dcmsr/dsrtncsr.h"
    "${CMAKE_SOURCE_DIR}/dcmsr/include/dcmtk/dcmsr/dsrtree.h"
    "${CMAKE_SOURCE_DIR}/dcmsr/include/dcmtk/dcmsr/dsrtypes.h"
    "${CMAKE_SOURCE_DIR}/dcmsr/include/dcmtk/dcmsr/dsruidtn.h"
    "${CMAKE_SOURCE_DIR}/dcmsr/include/dcmtk/dcmsr/dsrwavch.h"
    "${CMAKE_SOURCE_DIR}/dcmsr/include/dcmtk/dcmsr/dsrwavtn.h"
    "${CMAKE_SOURCE_DIR}/dcmsr/include/dcmtk/dcmsr/dsrwavvl.h"
    "${CMAKE_SOURCE_DIR}/dcmsr/include/dcmtk/dcmsr/dsrxmlc.h"
    "${CMAKE_SOURCE_DIR}/dcmsr/include/dcmtk/dcmsr/dsrxmld.h"
    "${CMAKE_SOURCE_DIR}/dcmsr/include/dcmtk/dcmsr/dsrxrdcc.h"
    )
ENDIF(NOT CMAKE_INSTALL_COMPONENT OR "${CMAKE_INSTALL_COMPONENT}" STREQUAL "Unspecified")

