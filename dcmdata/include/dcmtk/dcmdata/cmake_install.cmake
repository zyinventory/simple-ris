# Install script for directory: D:/workspace/dcmtk-3.5.4/dcmdata/include/dcmtk/dcmdata

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
  FILE(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/dcmtk/dcmdata" TYPE FILE FILES
    "D:/workspace/dcmtk-3.5.4/dcmdata/include/dcmtk/dcmdata/cmdlnarg.h"
    "D:/workspace/dcmtk-3.5.4/dcmdata/include/dcmtk/dcmdata/dcbytstr.h"
    "D:/workspace/dcmtk-3.5.4/dcmdata/include/dcmtk/dcmdata/dcchrstr.h"
    "D:/workspace/dcmtk-3.5.4/dcmdata/include/dcmtk/dcmdata/dccodec.h"
    "D:/workspace/dcmtk-3.5.4/dcmdata/include/dcmtk/dcmdata/dcdatset.h"
    "D:/workspace/dcmtk-3.5.4/dcmdata/include/dcmtk/dcmdata/dcddirif.h"
    "D:/workspace/dcmtk-3.5.4/dcmdata/include/dcmtk/dcmdata/dcdebug.h"
    "D:/workspace/dcmtk-3.5.4/dcmdata/include/dcmtk/dcmdata/dcdefine.h"
    "D:/workspace/dcmtk-3.5.4/dcmdata/include/dcmtk/dcmdata/dcdeftag.h"
    "D:/workspace/dcmtk-3.5.4/dcmdata/include/dcmtk/dcmdata/dcdicdir.h"
    "D:/workspace/dcmtk-3.5.4/dcmdata/include/dcmtk/dcmdata/dcdicent.h"
    "D:/workspace/dcmtk-3.5.4/dcmdata/include/dcmtk/dcmdata/dcdict.h"
    "D:/workspace/dcmtk-3.5.4/dcmdata/include/dcmtk/dcmdata/dcdirrec.h"
    "D:/workspace/dcmtk-3.5.4/dcmdata/include/dcmtk/dcmdata/dcelem.h"
    "D:/workspace/dcmtk-3.5.4/dcmdata/include/dcmtk/dcmdata/dcerror.h"
    "D:/workspace/dcmtk-3.5.4/dcmdata/include/dcmtk/dcmdata/dcfilefo.h"
    "D:/workspace/dcmtk-3.5.4/dcmdata/include/dcmtk/dcmdata/dchashdi.h"
    "D:/workspace/dcmtk-3.5.4/dcmdata/include/dcmtk/dcmdata/dcistrma.h"
    "D:/workspace/dcmtk-3.5.4/dcmdata/include/dcmtk/dcmdata/dcistrmb.h"
    "D:/workspace/dcmtk-3.5.4/dcmdata/include/dcmtk/dcmdata/dcistrmf.h"
    "D:/workspace/dcmtk-3.5.4/dcmdata/include/dcmtk/dcmdata/dcistrmz.h"
    "D:/workspace/dcmtk-3.5.4/dcmdata/include/dcmtk/dcmdata/dcitem.h"
    "D:/workspace/dcmtk-3.5.4/dcmdata/include/dcmtk/dcmdata/dclist.h"
    "D:/workspace/dcmtk-3.5.4/dcmdata/include/dcmtk/dcmdata/dcmetinf.h"
    "D:/workspace/dcmtk-3.5.4/dcmdata/include/dcmtk/dcmdata/dcobject.h"
    "D:/workspace/dcmtk-3.5.4/dcmdata/include/dcmtk/dcmdata/dcofsetl.h"
    "D:/workspace/dcmtk-3.5.4/dcmdata/include/dcmtk/dcmdata/dcostrma.h"
    "D:/workspace/dcmtk-3.5.4/dcmdata/include/dcmtk/dcmdata/dcostrmb.h"
    "D:/workspace/dcmtk-3.5.4/dcmdata/include/dcmtk/dcmdata/dcostrmf.h"
    "D:/workspace/dcmtk-3.5.4/dcmdata/include/dcmtk/dcmdata/dcostrmz.h"
    "D:/workspace/dcmtk-3.5.4/dcmdata/include/dcmtk/dcmdata/dcovlay.h"
    "D:/workspace/dcmtk-3.5.4/dcmdata/include/dcmtk/dcmdata/dcpcache.h"
    "D:/workspace/dcmtk-3.5.4/dcmdata/include/dcmtk/dcmdata/dcpixel.h"
    "D:/workspace/dcmtk-3.5.4/dcmdata/include/dcmtk/dcmdata/dcpixseq.h"
    "D:/workspace/dcmtk-3.5.4/dcmdata/include/dcmtk/dcmdata/dcpxitem.h"
    "D:/workspace/dcmtk-3.5.4/dcmdata/include/dcmtk/dcmdata/dcrleccd.h"
    "D:/workspace/dcmtk-3.5.4/dcmdata/include/dcmtk/dcmdata/dcrlecce.h"
    "D:/workspace/dcmtk-3.5.4/dcmdata/include/dcmtk/dcmdata/dcrlecp.h"
    "D:/workspace/dcmtk-3.5.4/dcmdata/include/dcmtk/dcmdata/dcrledec.h"
    "D:/workspace/dcmtk-3.5.4/dcmdata/include/dcmtk/dcmdata/dcrledrg.h"
    "D:/workspace/dcmtk-3.5.4/dcmdata/include/dcmtk/dcmdata/dcrleenc.h"
    "D:/workspace/dcmtk-3.5.4/dcmdata/include/dcmtk/dcmdata/dcrleerg.h"
    "D:/workspace/dcmtk-3.5.4/dcmdata/include/dcmtk/dcmdata/dcrlerp.h"
    "D:/workspace/dcmtk-3.5.4/dcmdata/include/dcmtk/dcmdata/dcsequen.h"
    "D:/workspace/dcmtk-3.5.4/dcmdata/include/dcmtk/dcmdata/dcstack.h"
    "D:/workspace/dcmtk-3.5.4/dcmdata/include/dcmtk/dcmdata/dcswap.h"
    "D:/workspace/dcmtk-3.5.4/dcmdata/include/dcmtk/dcmdata/dctag.h"
    "D:/workspace/dcmtk-3.5.4/dcmdata/include/dcmtk/dcmdata/dctagkey.h"
    "D:/workspace/dcmtk-3.5.4/dcmdata/include/dcmtk/dcmdata/dctk.h"
    "D:/workspace/dcmtk-3.5.4/dcmdata/include/dcmtk/dcmdata/dctypes.h"
    "D:/workspace/dcmtk-3.5.4/dcmdata/include/dcmtk/dcmdata/dcuid.h"
    "D:/workspace/dcmtk-3.5.4/dcmdata/include/dcmtk/dcmdata/dcvm.h"
    "D:/workspace/dcmtk-3.5.4/dcmdata/include/dcmtk/dcmdata/dcvr.h"
    "D:/workspace/dcmtk-3.5.4/dcmdata/include/dcmtk/dcmdata/dcvrae.h"
    "D:/workspace/dcmtk-3.5.4/dcmdata/include/dcmtk/dcmdata/dcvras.h"
    "D:/workspace/dcmtk-3.5.4/dcmdata/include/dcmtk/dcmdata/dcvrat.h"
    "D:/workspace/dcmtk-3.5.4/dcmdata/include/dcmtk/dcmdata/dcvrcs.h"
    "D:/workspace/dcmtk-3.5.4/dcmdata/include/dcmtk/dcmdata/dcvrda.h"
    "D:/workspace/dcmtk-3.5.4/dcmdata/include/dcmtk/dcmdata/dcvrds.h"
    "D:/workspace/dcmtk-3.5.4/dcmdata/include/dcmtk/dcmdata/dcvrdt.h"
    "D:/workspace/dcmtk-3.5.4/dcmdata/include/dcmtk/dcmdata/dcvrfd.h"
    "D:/workspace/dcmtk-3.5.4/dcmdata/include/dcmtk/dcmdata/dcvrfl.h"
    "D:/workspace/dcmtk-3.5.4/dcmdata/include/dcmtk/dcmdata/dcvris.h"
    "D:/workspace/dcmtk-3.5.4/dcmdata/include/dcmtk/dcmdata/dcvrlo.h"
    "D:/workspace/dcmtk-3.5.4/dcmdata/include/dcmtk/dcmdata/dcvrlt.h"
    "D:/workspace/dcmtk-3.5.4/dcmdata/include/dcmtk/dcmdata/dcvrobow.h"
    "D:/workspace/dcmtk-3.5.4/dcmdata/include/dcmtk/dcmdata/dcvrof.h"
    "D:/workspace/dcmtk-3.5.4/dcmdata/include/dcmtk/dcmdata/dcvrpn.h"
    "D:/workspace/dcmtk-3.5.4/dcmdata/include/dcmtk/dcmdata/dcvrpobw.h"
    "D:/workspace/dcmtk-3.5.4/dcmdata/include/dcmtk/dcmdata/dcvrsh.h"
    "D:/workspace/dcmtk-3.5.4/dcmdata/include/dcmtk/dcmdata/dcvrsl.h"
    "D:/workspace/dcmtk-3.5.4/dcmdata/include/dcmtk/dcmdata/dcvrss.h"
    "D:/workspace/dcmtk-3.5.4/dcmdata/include/dcmtk/dcmdata/dcvrst.h"
    "D:/workspace/dcmtk-3.5.4/dcmdata/include/dcmtk/dcmdata/dcvrtm.h"
    "D:/workspace/dcmtk-3.5.4/dcmdata/include/dcmtk/dcmdata/dcvrui.h"
    "D:/workspace/dcmtk-3.5.4/dcmdata/include/dcmtk/dcmdata/dcvrul.h"
    "D:/workspace/dcmtk-3.5.4/dcmdata/include/dcmtk/dcmdata/dcvrulup.h"
    "D:/workspace/dcmtk-3.5.4/dcmdata/include/dcmtk/dcmdata/dcvrus.h"
    "D:/workspace/dcmtk-3.5.4/dcmdata/include/dcmtk/dcmdata/dcvrut.h"
    "D:/workspace/dcmtk-3.5.4/dcmdata/include/dcmtk/dcmdata/dcxfer.h"
    )
ENDIF(NOT CMAKE_INSTALL_COMPONENT OR "${CMAKE_INSTALL_COMPONENT}" STREQUAL "Unspecified")

