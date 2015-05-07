# Install script for directory: ${CMAKE_SOURCE_DIR}/dcmimage/include/dcmtk/dcmimage

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
  FILE(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/dcmtk/dcmimage" TYPE FILE FILES
    "${CMAKE_SOURCE_DIR}/dcmimage/include/dcmtk/dcmimage/diargimg.h"
    "${CMAKE_SOURCE_DIR}/dcmimage/include/dcmtk/dcmimage/diargpxt.h"
    "${CMAKE_SOURCE_DIR}/dcmimage/include/dcmtk/dcmimage/dicmyimg.h"
    "${CMAKE_SOURCE_DIR}/dcmimage/include/dcmtk/dcmimage/dicmypxt.h"
    "${CMAKE_SOURCE_DIR}/dcmimage/include/dcmtk/dcmimage/dicocpt.h"
    "${CMAKE_SOURCE_DIR}/dcmimage/include/dcmtk/dcmimage/dicoflt.h"
    "${CMAKE_SOURCE_DIR}/dcmimage/include/dcmtk/dcmimage/dicoimg.h"
    "${CMAKE_SOURCE_DIR}/dcmimage/include/dcmtk/dcmimage/dicomot.h"
    "${CMAKE_SOURCE_DIR}/dcmimage/include/dcmtk/dcmimage/dicoopx.h"
    "${CMAKE_SOURCE_DIR}/dcmimage/include/dcmtk/dcmimage/dicoopxt.h"
    "${CMAKE_SOURCE_DIR}/dcmimage/include/dcmtk/dcmimage/dicopx.h"
    "${CMAKE_SOURCE_DIR}/dcmimage/include/dcmtk/dcmimage/dicopxt.h"
    "${CMAKE_SOURCE_DIR}/dcmimage/include/dcmtk/dcmimage/dicorot.h"
    "${CMAKE_SOURCE_DIR}/dcmimage/include/dcmtk/dcmimage/dicosct.h"
    "${CMAKE_SOURCE_DIR}/dcmimage/include/dcmtk/dcmimage/dihsvimg.h"
    "${CMAKE_SOURCE_DIR}/dcmimage/include/dcmtk/dcmimage/dihsvpxt.h"
    "${CMAKE_SOURCE_DIR}/dcmimage/include/dcmtk/dcmimage/dipalimg.h"
    "${CMAKE_SOURCE_DIR}/dcmimage/include/dcmtk/dcmimage/dipalpxt.h"
    "${CMAKE_SOURCE_DIR}/dcmimage/include/dcmtk/dcmimage/dipipng.h"
    "${CMAKE_SOURCE_DIR}/dcmimage/include/dcmtk/dcmimage/dipitiff.h"
    "${CMAKE_SOURCE_DIR}/dcmimage/include/dcmtk/dcmimage/diqtcmap.h"
    "${CMAKE_SOURCE_DIR}/dcmimage/include/dcmtk/dcmimage/diqtctab.h"
    "${CMAKE_SOURCE_DIR}/dcmimage/include/dcmtk/dcmimage/diqtfs.h"
    "${CMAKE_SOURCE_DIR}/dcmimage/include/dcmtk/dcmimage/diqthash.h"
    "${CMAKE_SOURCE_DIR}/dcmimage/include/dcmtk/dcmimage/diqthitl.h"
    "${CMAKE_SOURCE_DIR}/dcmimage/include/dcmtk/dcmimage/diqthitm.h"
    "${CMAKE_SOURCE_DIR}/dcmimage/include/dcmtk/dcmimage/diqtid.h"
    "${CMAKE_SOURCE_DIR}/dcmimage/include/dcmtk/dcmimage/diqtpbox.h"
    "${CMAKE_SOURCE_DIR}/dcmimage/include/dcmtk/dcmimage/diqtpix.h"
    "${CMAKE_SOURCE_DIR}/dcmimage/include/dcmtk/dcmimage/diqtstab.h"
    "${CMAKE_SOURCE_DIR}/dcmimage/include/dcmtk/dcmimage/diqttype.h"
    "${CMAKE_SOURCE_DIR}/dcmimage/include/dcmtk/dcmimage/diquant.h"
    "${CMAKE_SOURCE_DIR}/dcmimage/include/dcmtk/dcmimage/diregist.h"
    "${CMAKE_SOURCE_DIR}/dcmimage/include/dcmtk/dcmimage/dirgbimg.h"
    "${CMAKE_SOURCE_DIR}/dcmimage/include/dcmtk/dcmimage/dirgbpxt.h"
    "${CMAKE_SOURCE_DIR}/dcmimage/include/dcmtk/dcmimage/diybrimg.h"
    "${CMAKE_SOURCE_DIR}/dcmimage/include/dcmtk/dcmimage/diybrpxt.h"
    "${CMAKE_SOURCE_DIR}/dcmimage/include/dcmtk/dcmimage/diyf2img.h"
    "${CMAKE_SOURCE_DIR}/dcmimage/include/dcmtk/dcmimage/diyf2pxt.h"
    "${CMAKE_SOURCE_DIR}/dcmimage/include/dcmtk/dcmimage/diyp2img.h"
    "${CMAKE_SOURCE_DIR}/dcmimage/include/dcmtk/dcmimage/diyp2pxt.h"
    )
ENDIF(NOT CMAKE_INSTALL_COMPONENT OR "${CMAKE_INSTALL_COMPONENT}" STREQUAL "Unspecified")

