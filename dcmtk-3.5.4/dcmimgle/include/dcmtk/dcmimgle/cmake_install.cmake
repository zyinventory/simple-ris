# Install script for directory: ${CMAKE_SOURCE_DIR}/dcmimgle/include/dcmtk/dcmimgle

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
  FILE(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/dcmtk/dcmimgle" TYPE FILE FILES
    "${CMAKE_SOURCE_DIR}/dcmimgle/include/dcmtk/dcmimgle/dcmimage.h"
    "${CMAKE_SOURCE_DIR}/dcmimgle/include/dcmtk/dcmimgle/dibaslut.h"
    "${CMAKE_SOURCE_DIR}/dcmimgle/include/dcmtk/dcmimgle/diciefn.h"
    "${CMAKE_SOURCE_DIR}/dcmimgle/include/dcmtk/dcmimgle/dicielut.h"
    "${CMAKE_SOURCE_DIR}/dcmimgle/include/dcmtk/dcmimgle/dicrvfit.h"
    "${CMAKE_SOURCE_DIR}/dcmimgle/include/dcmtk/dcmimgle/didislut.h"
    "${CMAKE_SOURCE_DIR}/dcmimgle/include/dcmtk/dcmimgle/didispfn.h"
    "${CMAKE_SOURCE_DIR}/dcmimgle/include/dcmtk/dcmimgle/didocu.h"
    "${CMAKE_SOURCE_DIR}/dcmimgle/include/dcmtk/dcmimgle/diflipt.h"
    "${CMAKE_SOURCE_DIR}/dcmimgle/include/dcmtk/dcmimgle/digsdfn.h"
    "${CMAKE_SOURCE_DIR}/dcmimgle/include/dcmtk/dcmimgle/digsdlut.h"
    "${CMAKE_SOURCE_DIR}/dcmimgle/include/dcmtk/dcmimgle/diimage.h"
    "${CMAKE_SOURCE_DIR}/dcmimgle/include/dcmtk/dcmimgle/diinpx.h"
    "${CMAKE_SOURCE_DIR}/dcmimgle/include/dcmtk/dcmimgle/diinpxt.h"
    "${CMAKE_SOURCE_DIR}/dcmimgle/include/dcmtk/dcmimgle/diluptab.h"
    "${CMAKE_SOURCE_DIR}/dcmimgle/include/dcmtk/dcmimgle/dimo1img.h"
    "${CMAKE_SOURCE_DIR}/dcmimgle/include/dcmtk/dcmimgle/dimo2img.h"
    "${CMAKE_SOURCE_DIR}/dcmimgle/include/dcmtk/dcmimgle/dimocpt.h"
    "${CMAKE_SOURCE_DIR}/dcmimgle/include/dcmtk/dcmimgle/dimoflt.h"
    "${CMAKE_SOURCE_DIR}/dcmimgle/include/dcmtk/dcmimgle/dimoimg.h"
    "${CMAKE_SOURCE_DIR}/dcmimgle/include/dcmtk/dcmimgle/dimoipxt.h"
    "${CMAKE_SOURCE_DIR}/dcmimgle/include/dcmtk/dcmimgle/dimomod.h"
    "${CMAKE_SOURCE_DIR}/dcmimgle/include/dcmtk/dcmimgle/dimoopx.h"
    "${CMAKE_SOURCE_DIR}/dcmimgle/include/dcmtk/dcmimgle/dimoopxt.h"
    "${CMAKE_SOURCE_DIR}/dcmimgle/include/dcmtk/dcmimgle/dimopx.h"
    "${CMAKE_SOURCE_DIR}/dcmimgle/include/dcmtk/dcmimgle/dimopxt.h"
    "${CMAKE_SOURCE_DIR}/dcmimgle/include/dcmtk/dcmimgle/dimorot.h"
    "${CMAKE_SOURCE_DIR}/dcmimgle/include/dcmtk/dcmimgle/dimosct.h"
    "${CMAKE_SOURCE_DIR}/dcmimgle/include/dcmtk/dcmimgle/diobjcou.h"
    "${CMAKE_SOURCE_DIR}/dcmimgle/include/dcmtk/dcmimgle/diovdat.h"
    "${CMAKE_SOURCE_DIR}/dcmimgle/include/dcmtk/dcmimgle/diovlay.h"
    "${CMAKE_SOURCE_DIR}/dcmimgle/include/dcmtk/dcmimgle/diovlimg.h"
    "${CMAKE_SOURCE_DIR}/dcmimgle/include/dcmtk/dcmimgle/diovpln.h"
    "${CMAKE_SOURCE_DIR}/dcmimgle/include/dcmtk/dcmimgle/dipixel.h"
    "${CMAKE_SOURCE_DIR}/dcmimgle/include/dcmtk/dcmimgle/diplugin.h"
    "${CMAKE_SOURCE_DIR}/dcmimgle/include/dcmtk/dcmimgle/dipxrept.h"
    "${CMAKE_SOURCE_DIR}/dcmimgle/include/dcmtk/dcmimgle/diregbas.h"
    "${CMAKE_SOURCE_DIR}/dcmimgle/include/dcmtk/dcmimgle/dirotat.h"
    "${CMAKE_SOURCE_DIR}/dcmimgle/include/dcmtk/dcmimgle/discalet.h"
    "${CMAKE_SOURCE_DIR}/dcmimgle/include/dcmtk/dcmimgle/displint.h"
    "${CMAKE_SOURCE_DIR}/dcmimgle/include/dcmtk/dcmimgle/ditranst.h"
    "${CMAKE_SOURCE_DIR}/dcmimgle/include/dcmtk/dcmimgle/diutils.h"
    )
ENDIF(NOT CMAKE_INSTALL_COMPONENT OR "${CMAKE_INSTALL_COMPONENT}" STREQUAL "Unspecified")

