# Install script for directory: ${CMAKE_SOURCE_DIR}/dcmsign/include/dcmtk/dcmsign

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
  FILE(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/dcmtk/dcmsign" TYPE FILE FILES
    "${CMAKE_SOURCE_DIR}/dcmsign/include/dcmtk/dcmsign/dcsignat.h"
    "${CMAKE_SOURCE_DIR}/dcmsign/include/dcmtk/dcmsign/sialgo.h"
    "${CMAKE_SOURCE_DIR}/dcmsign/include/dcmtk/dcmsign/siautopr.h"
    "${CMAKE_SOURCE_DIR}/dcmsign/include/dcmtk/dcmsign/sibrsapr.h"
    "${CMAKE_SOURCE_DIR}/dcmsign/include/dcmtk/dcmsign/sicert.h"
    "${CMAKE_SOURCE_DIR}/dcmsign/include/dcmtk/dcmsign/sicertvf.h"
    "${CMAKE_SOURCE_DIR}/dcmsign/include/dcmtk/dcmsign/sicreapr.h"
    "${CMAKE_SOURCE_DIR}/dcmsign/include/dcmtk/dcmsign/sidsa.h"
    "${CMAKE_SOURCE_DIR}/dcmsign/include/dcmtk/dcmsign/simac.h"
    "${CMAKE_SOURCE_DIR}/dcmsign/include/dcmtk/dcmsign/simaccon.h"
    "${CMAKE_SOURCE_DIR}/dcmsign/include/dcmtk/dcmsign/simd5.h"
    "${CMAKE_SOURCE_DIR}/dcmsign/include/dcmtk/dcmsign/sinullpr.h"
    "${CMAKE_SOURCE_DIR}/dcmsign/include/dcmtk/dcmsign/siprivat.h"
    "${CMAKE_SOURCE_DIR}/dcmsign/include/dcmtk/dcmsign/siripemd.h"
    "${CMAKE_SOURCE_DIR}/dcmsign/include/dcmtk/dcmsign/sirsa.h"
    "${CMAKE_SOURCE_DIR}/dcmsign/include/dcmtk/dcmsign/sisha1.h"
    "${CMAKE_SOURCE_DIR}/dcmsign/include/dcmtk/dcmsign/sisprof.h"
    "${CMAKE_SOURCE_DIR}/dcmsign/include/dcmtk/dcmsign/sitstamp.h"
    "${CMAKE_SOURCE_DIR}/dcmsign/include/dcmtk/dcmsign/sitypes.h"
    )
ENDIF(NOT CMAKE_INSTALL_COMPONENT OR "${CMAKE_INSTALL_COMPONENT}" STREQUAL "Unspecified")

