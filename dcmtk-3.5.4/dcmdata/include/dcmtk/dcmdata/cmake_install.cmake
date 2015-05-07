# Install script for directory: ${CMAKE_SOURCE_DIR}/dcmdata/include/dcmtk/dcmdata

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
  FILE(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/dcmtk/dcmdata" TYPE FILE FILES
    "${CMAKE_SOURCE_DIR}/dcmdata/include/dcmtk/dcmdata/cmdlnarg.h"
    "${CMAKE_SOURCE_DIR}/dcmdata/include/dcmtk/dcmdata/dcbytstr.h"
    "${CMAKE_SOURCE_DIR}/dcmdata/include/dcmtk/dcmdata/dcchrstr.h"
    "${CMAKE_SOURCE_DIR}/dcmdata/include/dcmtk/dcmdata/dccodec.h"
    "${CMAKE_SOURCE_DIR}/dcmdata/include/dcmtk/dcmdata/dcdatset.h"
    "${CMAKE_SOURCE_DIR}/dcmdata/include/dcmtk/dcmdata/dcddirif.h"
    "${CMAKE_SOURCE_DIR}/dcmdata/include/dcmtk/dcmdata/dcdebug.h"
    "${CMAKE_SOURCE_DIR}/dcmdata/include/dcmtk/dcmdata/dcdefine.h"
    "${CMAKE_SOURCE_DIR}/dcmdata/include/dcmtk/dcmdata/dcdeftag.h"
    "${CMAKE_SOURCE_DIR}/dcmdata/include/dcmtk/dcmdata/dcdicdir.h"
    "${CMAKE_SOURCE_DIR}/dcmdata/include/dcmtk/dcmdata/dcdicent.h"
    "${CMAKE_SOURCE_DIR}/dcmdata/include/dcmtk/dcmdata/dcdict.h"
    "${CMAKE_SOURCE_DIR}/dcmdata/include/dcmtk/dcmdata/dcdirrec.h"
    "${CMAKE_SOURCE_DIR}/dcmdata/include/dcmtk/dcmdata/dcelem.h"
    "${CMAKE_SOURCE_DIR}/dcmdata/include/dcmtk/dcmdata/dcerror.h"
    "${CMAKE_SOURCE_DIR}/dcmdata/include/dcmtk/dcmdata/dcfilefo.h"
    "${CMAKE_SOURCE_DIR}/dcmdata/include/dcmtk/dcmdata/dchashdi.h"
    "${CMAKE_SOURCE_DIR}/dcmdata/include/dcmtk/dcmdata/dcistrma.h"
    "${CMAKE_SOURCE_DIR}/dcmdata/include/dcmtk/dcmdata/dcistrmb.h"
    "${CMAKE_SOURCE_DIR}/dcmdata/include/dcmtk/dcmdata/dcistrmf.h"
    "${CMAKE_SOURCE_DIR}/dcmdata/include/dcmtk/dcmdata/dcistrmz.h"
    "${CMAKE_SOURCE_DIR}/dcmdata/include/dcmtk/dcmdata/dcitem.h"
    "${CMAKE_SOURCE_DIR}/dcmdata/include/dcmtk/dcmdata/dclist.h"
    "${CMAKE_SOURCE_DIR}/dcmdata/include/dcmtk/dcmdata/dcmetinf.h"
    "${CMAKE_SOURCE_DIR}/dcmdata/include/dcmtk/dcmdata/dcobject.h"
    "${CMAKE_SOURCE_DIR}/dcmdata/include/dcmtk/dcmdata/dcofsetl.h"
    "${CMAKE_SOURCE_DIR}/dcmdata/include/dcmtk/dcmdata/dcostrma.h"
    "${CMAKE_SOURCE_DIR}/dcmdata/include/dcmtk/dcmdata/dcostrmb.h"
    "${CMAKE_SOURCE_DIR}/dcmdata/include/dcmtk/dcmdata/dcostrmf.h"
    "${CMAKE_SOURCE_DIR}/dcmdata/include/dcmtk/dcmdata/dcostrmz.h"
    "${CMAKE_SOURCE_DIR}/dcmdata/include/dcmtk/dcmdata/dcovlay.h"
    "${CMAKE_SOURCE_DIR}/dcmdata/include/dcmtk/dcmdata/dcpcache.h"
    "${CMAKE_SOURCE_DIR}/dcmdata/include/dcmtk/dcmdata/dcpixel.h"
    "${CMAKE_SOURCE_DIR}/dcmdata/include/dcmtk/dcmdata/dcpixseq.h"
    "${CMAKE_SOURCE_DIR}/dcmdata/include/dcmtk/dcmdata/dcpxitem.h"
    "${CMAKE_SOURCE_DIR}/dcmdata/include/dcmtk/dcmdata/dcrleccd.h"
    "${CMAKE_SOURCE_DIR}/dcmdata/include/dcmtk/dcmdata/dcrlecce.h"
    "${CMAKE_SOURCE_DIR}/dcmdata/include/dcmtk/dcmdata/dcrlecp.h"
    "${CMAKE_SOURCE_DIR}/dcmdata/include/dcmtk/dcmdata/dcrledec.h"
    "${CMAKE_SOURCE_DIR}/dcmdata/include/dcmtk/dcmdata/dcrledrg.h"
    "${CMAKE_SOURCE_DIR}/dcmdata/include/dcmtk/dcmdata/dcrleenc.h"
    "${CMAKE_SOURCE_DIR}/dcmdata/include/dcmtk/dcmdata/dcrleerg.h"
    "${CMAKE_SOURCE_DIR}/dcmdata/include/dcmtk/dcmdata/dcrlerp.h"
    "${CMAKE_SOURCE_DIR}/dcmdata/include/dcmtk/dcmdata/dcsequen.h"
    "${CMAKE_SOURCE_DIR}/dcmdata/include/dcmtk/dcmdata/dcstack.h"
    "${CMAKE_SOURCE_DIR}/dcmdata/include/dcmtk/dcmdata/dcswap.h"
    "${CMAKE_SOURCE_DIR}/dcmdata/include/dcmtk/dcmdata/dctag.h"
    "${CMAKE_SOURCE_DIR}/dcmdata/include/dcmtk/dcmdata/dctagkey.h"
    "${CMAKE_SOURCE_DIR}/dcmdata/include/dcmtk/dcmdata/dctk.h"
    "${CMAKE_SOURCE_DIR}/dcmdata/include/dcmtk/dcmdata/dctypes.h"
    "${CMAKE_SOURCE_DIR}/dcmdata/include/dcmtk/dcmdata/dcuid.h"
    "${CMAKE_SOURCE_DIR}/dcmdata/include/dcmtk/dcmdata/dcvm.h"
    "${CMAKE_SOURCE_DIR}/dcmdata/include/dcmtk/dcmdata/dcvr.h"
    "${CMAKE_SOURCE_DIR}/dcmdata/include/dcmtk/dcmdata/dcvrae.h"
    "${CMAKE_SOURCE_DIR}/dcmdata/include/dcmtk/dcmdata/dcvras.h"
    "${CMAKE_SOURCE_DIR}/dcmdata/include/dcmtk/dcmdata/dcvrat.h"
    "${CMAKE_SOURCE_DIR}/dcmdata/include/dcmtk/dcmdata/dcvrcs.h"
    "${CMAKE_SOURCE_DIR}/dcmdata/include/dcmtk/dcmdata/dcvrda.h"
    "${CMAKE_SOURCE_DIR}/dcmdata/include/dcmtk/dcmdata/dcvrds.h"
    "${CMAKE_SOURCE_DIR}/dcmdata/include/dcmtk/dcmdata/dcvrdt.h"
    "${CMAKE_SOURCE_DIR}/dcmdata/include/dcmtk/dcmdata/dcvrfd.h"
    "${CMAKE_SOURCE_DIR}/dcmdata/include/dcmtk/dcmdata/dcvrfl.h"
    "${CMAKE_SOURCE_DIR}/dcmdata/include/dcmtk/dcmdata/dcvris.h"
    "${CMAKE_SOURCE_DIR}/dcmdata/include/dcmtk/dcmdata/dcvrlo.h"
    "${CMAKE_SOURCE_DIR}/dcmdata/include/dcmtk/dcmdata/dcvrlt.h"
    "${CMAKE_SOURCE_DIR}/dcmdata/include/dcmtk/dcmdata/dcvrobow.h"
    "${CMAKE_SOURCE_DIR}/dcmdata/include/dcmtk/dcmdata/dcvrof.h"
    "${CMAKE_SOURCE_DIR}/dcmdata/include/dcmtk/dcmdata/dcvrpn.h"
    "${CMAKE_SOURCE_DIR}/dcmdata/include/dcmtk/dcmdata/dcvrpobw.h"
    "${CMAKE_SOURCE_DIR}/dcmdata/include/dcmtk/dcmdata/dcvrsh.h"
    "${CMAKE_SOURCE_DIR}/dcmdata/include/dcmtk/dcmdata/dcvrsl.h"
    "${CMAKE_SOURCE_DIR}/dcmdata/include/dcmtk/dcmdata/dcvrss.h"
    "${CMAKE_SOURCE_DIR}/dcmdata/include/dcmtk/dcmdata/dcvrst.h"
    "${CMAKE_SOURCE_DIR}/dcmdata/include/dcmtk/dcmdata/dcvrtm.h"
    "${CMAKE_SOURCE_DIR}/dcmdata/include/dcmtk/dcmdata/dcvrui.h"
    "${CMAKE_SOURCE_DIR}/dcmdata/include/dcmtk/dcmdata/dcvrul.h"
    "${CMAKE_SOURCE_DIR}/dcmdata/include/dcmtk/dcmdata/dcvrulup.h"
    "${CMAKE_SOURCE_DIR}/dcmdata/include/dcmtk/dcmdata/dcvrus.h"
    "${CMAKE_SOURCE_DIR}/dcmdata/include/dcmtk/dcmdata/dcvrut.h"
    "${CMAKE_SOURCE_DIR}/dcmdata/include/dcmtk/dcmdata/dcxfer.h"
    )
ENDIF(NOT CMAKE_INSTALL_COMPONENT OR "${CMAKE_INSTALL_COMPONENT}" STREQUAL "Unspecified")

