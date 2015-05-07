# Install script for directory: D:/workspace/dcmtk-3.5.4/dcmimgle/include/dcmtk/dcmimgle

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
  FILE(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/dcmtk/dcmimgle" TYPE FILE FILES
    "D:/workspace/dcmtk-3.5.4/dcmimgle/include/dcmtk/dcmimgle/dcmimage.h"
    "D:/workspace/dcmtk-3.5.4/dcmimgle/include/dcmtk/dcmimgle/dibaslut.h"
    "D:/workspace/dcmtk-3.5.4/dcmimgle/include/dcmtk/dcmimgle/diciefn.h"
    "D:/workspace/dcmtk-3.5.4/dcmimgle/include/dcmtk/dcmimgle/dicielut.h"
    "D:/workspace/dcmtk-3.5.4/dcmimgle/include/dcmtk/dcmimgle/dicrvfit.h"
    "D:/workspace/dcmtk-3.5.4/dcmimgle/include/dcmtk/dcmimgle/didislut.h"
    "D:/workspace/dcmtk-3.5.4/dcmimgle/include/dcmtk/dcmimgle/didispfn.h"
    "D:/workspace/dcmtk-3.5.4/dcmimgle/include/dcmtk/dcmimgle/didocu.h"
    "D:/workspace/dcmtk-3.5.4/dcmimgle/include/dcmtk/dcmimgle/diflipt.h"
    "D:/workspace/dcmtk-3.5.4/dcmimgle/include/dcmtk/dcmimgle/digsdfn.h"
    "D:/workspace/dcmtk-3.5.4/dcmimgle/include/dcmtk/dcmimgle/digsdlut.h"
    "D:/workspace/dcmtk-3.5.4/dcmimgle/include/dcmtk/dcmimgle/diimage.h"
    "D:/workspace/dcmtk-3.5.4/dcmimgle/include/dcmtk/dcmimgle/diinpx.h"
    "D:/workspace/dcmtk-3.5.4/dcmimgle/include/dcmtk/dcmimgle/diinpxt.h"
    "D:/workspace/dcmtk-3.5.4/dcmimgle/include/dcmtk/dcmimgle/diluptab.h"
    "D:/workspace/dcmtk-3.5.4/dcmimgle/include/dcmtk/dcmimgle/dimo1img.h"
    "D:/workspace/dcmtk-3.5.4/dcmimgle/include/dcmtk/dcmimgle/dimo2img.h"
    "D:/workspace/dcmtk-3.5.4/dcmimgle/include/dcmtk/dcmimgle/dimocpt.h"
    "D:/workspace/dcmtk-3.5.4/dcmimgle/include/dcmtk/dcmimgle/dimoflt.h"
    "D:/workspace/dcmtk-3.5.4/dcmimgle/include/dcmtk/dcmimgle/dimoimg.h"
    "D:/workspace/dcmtk-3.5.4/dcmimgle/include/dcmtk/dcmimgle/dimoipxt.h"
    "D:/workspace/dcmtk-3.5.4/dcmimgle/include/dcmtk/dcmimgle/dimomod.h"
    "D:/workspace/dcmtk-3.5.4/dcmimgle/include/dcmtk/dcmimgle/dimoopx.h"
    "D:/workspace/dcmtk-3.5.4/dcmimgle/include/dcmtk/dcmimgle/dimoopxt.h"
    "D:/workspace/dcmtk-3.5.4/dcmimgle/include/dcmtk/dcmimgle/dimopx.h"
    "D:/workspace/dcmtk-3.5.4/dcmimgle/include/dcmtk/dcmimgle/dimopxt.h"
    "D:/workspace/dcmtk-3.5.4/dcmimgle/include/dcmtk/dcmimgle/dimorot.h"
    "D:/workspace/dcmtk-3.5.4/dcmimgle/include/dcmtk/dcmimgle/dimosct.h"
    "D:/workspace/dcmtk-3.5.4/dcmimgle/include/dcmtk/dcmimgle/diobjcou.h"
    "D:/workspace/dcmtk-3.5.4/dcmimgle/include/dcmtk/dcmimgle/diovdat.h"
    "D:/workspace/dcmtk-3.5.4/dcmimgle/include/dcmtk/dcmimgle/diovlay.h"
    "D:/workspace/dcmtk-3.5.4/dcmimgle/include/dcmtk/dcmimgle/diovlimg.h"
    "D:/workspace/dcmtk-3.5.4/dcmimgle/include/dcmtk/dcmimgle/diovpln.h"
    "D:/workspace/dcmtk-3.5.4/dcmimgle/include/dcmtk/dcmimgle/dipixel.h"
    "D:/workspace/dcmtk-3.5.4/dcmimgle/include/dcmtk/dcmimgle/diplugin.h"
    "D:/workspace/dcmtk-3.5.4/dcmimgle/include/dcmtk/dcmimgle/dipxrept.h"
    "D:/workspace/dcmtk-3.5.4/dcmimgle/include/dcmtk/dcmimgle/diregbas.h"
    "D:/workspace/dcmtk-3.5.4/dcmimgle/include/dcmtk/dcmimgle/dirotat.h"
    "D:/workspace/dcmtk-3.5.4/dcmimgle/include/dcmtk/dcmimgle/discalet.h"
    "D:/workspace/dcmtk-3.5.4/dcmimgle/include/dcmtk/dcmimgle/displint.h"
    "D:/workspace/dcmtk-3.5.4/dcmimgle/include/dcmtk/dcmimgle/ditranst.h"
    "D:/workspace/dcmtk-3.5.4/dcmimgle/include/dcmtk/dcmimgle/diutils.h"
    )
ENDIF(NOT CMAKE_INSTALL_COMPONENT OR "${CMAKE_INSTALL_COMPONENT}" STREQUAL "Unspecified")

