# Install script for directory: ${CMAKE_SOURCE_DIR}/dcmpstat/include/dcmtk/dcmpstat

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
  FILE(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/dcmtk/dcmpstat" TYPE FILE FILES
    "${CMAKE_SOURCE_DIR}/dcmpstat/include/dcmtk/dcmpstat/dcmpstat.h"
    "${CMAKE_SOURCE_DIR}/dcmpstat/include/dcmtk/dcmpstat/dvcache.h"
    "${CMAKE_SOURCE_DIR}/dcmpstat/include/dcmtk/dcmpstat/dviface.h"
    "${CMAKE_SOURCE_DIR}/dcmpstat/include/dcmtk/dcmpstat/dvpsab.h"
    "${CMAKE_SOURCE_DIR}/dcmpstat/include/dcmtk/dcmpstat/dvpsabl.h"
    "${CMAKE_SOURCE_DIR}/dcmpstat/include/dcmtk/dcmpstat/dvpsal.h"
    "${CMAKE_SOURCE_DIR}/dcmpstat/include/dcmtk/dcmpstat/dvpsall.h"
    "${CMAKE_SOURCE_DIR}/dcmpstat/include/dcmtk/dcmpstat/dvpscf.h"
    "${CMAKE_SOURCE_DIR}/dcmpstat/include/dcmtk/dcmpstat/dvpscu.h"
    "${CMAKE_SOURCE_DIR}/dcmpstat/include/dcmtk/dcmpstat/dvpscul.h"
    "${CMAKE_SOURCE_DIR}/dcmpstat/include/dcmtk/dcmpstat/dvpsda.h"
    "${CMAKE_SOURCE_DIR}/dcmpstat/include/dcmtk/dcmpstat/dvpsdal.h"
    "${CMAKE_SOURCE_DIR}/dcmpstat/include/dcmtk/dcmpstat/dvpsdef.h"
    "${CMAKE_SOURCE_DIR}/dcmpstat/include/dcmtk/dcmpstat/dvpsfs.h"
    "${CMAKE_SOURCE_DIR}/dcmpstat/include/dcmtk/dcmpstat/dvpsga.h"
    "${CMAKE_SOURCE_DIR}/dcmpstat/include/dcmtk/dcmpstat/dvpsgal.h"
    "${CMAKE_SOURCE_DIR}/dcmpstat/include/dcmtk/dcmpstat/dvpsgl.h"
    "${CMAKE_SOURCE_DIR}/dcmpstat/include/dcmtk/dcmpstat/dvpsgll.h"
    "${CMAKE_SOURCE_DIR}/dcmpstat/include/dcmtk/dcmpstat/dvpsgr.h"
    "${CMAKE_SOURCE_DIR}/dcmpstat/include/dcmtk/dcmpstat/dvpsgrl.h"
    "${CMAKE_SOURCE_DIR}/dcmpstat/include/dcmtk/dcmpstat/dvpshlp.h"
    "${CMAKE_SOURCE_DIR}/dcmpstat/include/dcmtk/dcmpstat/dvpsib.h"
    "${CMAKE_SOURCE_DIR}/dcmpstat/include/dcmtk/dcmpstat/dvpsibl.h"
    "${CMAKE_SOURCE_DIR}/dcmpstat/include/dcmtk/dcmpstat/dvpsmsg.h"
    "${CMAKE_SOURCE_DIR}/dcmpstat/include/dcmtk/dcmpstat/dvpsov.h"
    "${CMAKE_SOURCE_DIR}/dcmpstat/include/dcmtk/dcmpstat/dvpsovl.h"
    "${CMAKE_SOURCE_DIR}/dcmpstat/include/dcmtk/dcmpstat/dvpspl.h"
    "${CMAKE_SOURCE_DIR}/dcmpstat/include/dcmtk/dcmpstat/dvpspll.h"
    "${CMAKE_SOURCE_DIR}/dcmpstat/include/dcmtk/dcmpstat/dvpspr.h"
    "${CMAKE_SOURCE_DIR}/dcmpstat/include/dcmtk/dcmpstat/dvpsprt.h"
    "${CMAKE_SOURCE_DIR}/dcmpstat/include/dcmtk/dcmpstat/dvpsri.h"
    "${CMAKE_SOURCE_DIR}/dcmpstat/include/dcmtk/dcmpstat/dvpsril.h"
    "${CMAKE_SOURCE_DIR}/dcmpstat/include/dcmtk/dcmpstat/dvpsrs.h"
    "${CMAKE_SOURCE_DIR}/dcmpstat/include/dcmtk/dcmpstat/dvpsrsl.h"
    "${CMAKE_SOURCE_DIR}/dcmpstat/include/dcmtk/dcmpstat/dvpssp.h"
    "${CMAKE_SOURCE_DIR}/dcmpstat/include/dcmtk/dcmpstat/dvpsspl.h"
    "${CMAKE_SOURCE_DIR}/dcmpstat/include/dcmtk/dcmpstat/dvpssv.h"
    "${CMAKE_SOURCE_DIR}/dcmpstat/include/dcmtk/dcmpstat/dvpssvl.h"
    "${CMAKE_SOURCE_DIR}/dcmpstat/include/dcmtk/dcmpstat/dvpstat.h"
    "${CMAKE_SOURCE_DIR}/dcmpstat/include/dcmtk/dcmpstat/dvpstx.h"
    "${CMAKE_SOURCE_DIR}/dcmpstat/include/dcmtk/dcmpstat/dvpstxl.h"
    "${CMAKE_SOURCE_DIR}/dcmpstat/include/dcmtk/dcmpstat/dvpstyp.h"
    "${CMAKE_SOURCE_DIR}/dcmpstat/include/dcmtk/dcmpstat/dvpsvl.h"
    "${CMAKE_SOURCE_DIR}/dcmpstat/include/dcmtk/dcmpstat/dvpsvll.h"
    "${CMAKE_SOURCE_DIR}/dcmpstat/include/dcmtk/dcmpstat/dvpsvw.h"
    "${CMAKE_SOURCE_DIR}/dcmpstat/include/dcmtk/dcmpstat/dvpsvwl.h"
    "${CMAKE_SOURCE_DIR}/dcmpstat/include/dcmtk/dcmpstat/dvsighdl.h"
    )
ENDIF(NOT CMAKE_INSTALL_COMPONENT OR "${CMAKE_INSTALL_COMPONENT}" STREQUAL "Unspecified")

