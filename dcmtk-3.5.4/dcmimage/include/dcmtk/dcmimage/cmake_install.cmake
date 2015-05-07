# Install script for directory: D:/workspace/dcmtk-3.5.4/dcmimage/include/dcmtk/dcmimage

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
  FILE(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/dcmtk/dcmimage" TYPE FILE FILES
    "D:/workspace/dcmtk-3.5.4/dcmimage/include/dcmtk/dcmimage/diargimg.h"
    "D:/workspace/dcmtk-3.5.4/dcmimage/include/dcmtk/dcmimage/diargpxt.h"
    "D:/workspace/dcmtk-3.5.4/dcmimage/include/dcmtk/dcmimage/dicmyimg.h"
    "D:/workspace/dcmtk-3.5.4/dcmimage/include/dcmtk/dcmimage/dicmypxt.h"
    "D:/workspace/dcmtk-3.5.4/dcmimage/include/dcmtk/dcmimage/dicocpt.h"
    "D:/workspace/dcmtk-3.5.4/dcmimage/include/dcmtk/dcmimage/dicoflt.h"
    "D:/workspace/dcmtk-3.5.4/dcmimage/include/dcmtk/dcmimage/dicoimg.h"
    "D:/workspace/dcmtk-3.5.4/dcmimage/include/dcmtk/dcmimage/dicomot.h"
    "D:/workspace/dcmtk-3.5.4/dcmimage/include/dcmtk/dcmimage/dicoopx.h"
    "D:/workspace/dcmtk-3.5.4/dcmimage/include/dcmtk/dcmimage/dicoopxt.h"
    "D:/workspace/dcmtk-3.5.4/dcmimage/include/dcmtk/dcmimage/dicopx.h"
    "D:/workspace/dcmtk-3.5.4/dcmimage/include/dcmtk/dcmimage/dicopxt.h"
    "D:/workspace/dcmtk-3.5.4/dcmimage/include/dcmtk/dcmimage/dicorot.h"
    "D:/workspace/dcmtk-3.5.4/dcmimage/include/dcmtk/dcmimage/dicosct.h"
    "D:/workspace/dcmtk-3.5.4/dcmimage/include/dcmtk/dcmimage/dihsvimg.h"
    "D:/workspace/dcmtk-3.5.4/dcmimage/include/dcmtk/dcmimage/dihsvpxt.h"
    "D:/workspace/dcmtk-3.5.4/dcmimage/include/dcmtk/dcmimage/dipalimg.h"
    "D:/workspace/dcmtk-3.5.4/dcmimage/include/dcmtk/dcmimage/dipalpxt.h"
    "D:/workspace/dcmtk-3.5.4/dcmimage/include/dcmtk/dcmimage/dipipng.h"
    "D:/workspace/dcmtk-3.5.4/dcmimage/include/dcmtk/dcmimage/dipitiff.h"
    "D:/workspace/dcmtk-3.5.4/dcmimage/include/dcmtk/dcmimage/diqtcmap.h"
    "D:/workspace/dcmtk-3.5.4/dcmimage/include/dcmtk/dcmimage/diqtctab.h"
    "D:/workspace/dcmtk-3.5.4/dcmimage/include/dcmtk/dcmimage/diqtfs.h"
    "D:/workspace/dcmtk-3.5.4/dcmimage/include/dcmtk/dcmimage/diqthash.h"
    "D:/workspace/dcmtk-3.5.4/dcmimage/include/dcmtk/dcmimage/diqthitl.h"
    "D:/workspace/dcmtk-3.5.4/dcmimage/include/dcmtk/dcmimage/diqthitm.h"
    "D:/workspace/dcmtk-3.5.4/dcmimage/include/dcmtk/dcmimage/diqtid.h"
    "D:/workspace/dcmtk-3.5.4/dcmimage/include/dcmtk/dcmimage/diqtpbox.h"
    "D:/workspace/dcmtk-3.5.4/dcmimage/include/dcmtk/dcmimage/diqtpix.h"
    "D:/workspace/dcmtk-3.5.4/dcmimage/include/dcmtk/dcmimage/diqtstab.h"
    "D:/workspace/dcmtk-3.5.4/dcmimage/include/dcmtk/dcmimage/diqttype.h"
    "D:/workspace/dcmtk-3.5.4/dcmimage/include/dcmtk/dcmimage/diquant.h"
    "D:/workspace/dcmtk-3.5.4/dcmimage/include/dcmtk/dcmimage/diregist.h"
    "D:/workspace/dcmtk-3.5.4/dcmimage/include/dcmtk/dcmimage/dirgbimg.h"
    "D:/workspace/dcmtk-3.5.4/dcmimage/include/dcmtk/dcmimage/dirgbpxt.h"
    "D:/workspace/dcmtk-3.5.4/dcmimage/include/dcmtk/dcmimage/diybrimg.h"
    "D:/workspace/dcmtk-3.5.4/dcmimage/include/dcmtk/dcmimage/diybrpxt.h"
    "D:/workspace/dcmtk-3.5.4/dcmimage/include/dcmtk/dcmimage/diyf2img.h"
    "D:/workspace/dcmtk-3.5.4/dcmimage/include/dcmtk/dcmimage/diyf2pxt.h"
    "D:/workspace/dcmtk-3.5.4/dcmimage/include/dcmtk/dcmimage/diyp2img.h"
    "D:/workspace/dcmtk-3.5.4/dcmimage/include/dcmtk/dcmimage/diyp2pxt.h"
    )
ENDIF(NOT CMAKE_INSTALL_COMPONENT OR "${CMAKE_INSTALL_COMPONENT}" STREQUAL "Unspecified")

