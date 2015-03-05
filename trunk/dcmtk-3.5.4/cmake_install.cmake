# Install script for directory: D:/workspace/dcmtk-3.5.4

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
  FILE(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/share/doc" TYPE FILE FILES
    "D:/workspace/dcmtk-3.5.4-trunk/COPYRIGHT"
    "D:/workspace/dcmtk-3.5.4-trunk/FAQ"
    "D:/workspace/dcmtk-3.5.4-trunk/HISTORY"
    )
ENDIF(NOT CMAKE_INSTALL_COMPONENT OR "${CMAKE_INSTALL_COMPONENT}" STREQUAL "Unspecified")

IF(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for each subdirectory.
  INCLUDE("D:/workspace/dcmtk-3.5.4-trunk/config/cmake_install.cmake")
  INCLUDE("D:/workspace/dcmtk-3.5.4-trunk/ofstd/cmake_install.cmake")
  INCLUDE("D:/workspace/dcmtk-3.5.4-trunk/dcmdata/cmake_install.cmake")
  INCLUDE("D:/workspace/dcmtk-3.5.4-trunk/dcmtls/cmake_install.cmake")
  INCLUDE("D:/workspace/dcmtk-3.5.4-trunk/dcmnet/cmake_install.cmake")
  INCLUDE("D:/workspace/dcmtk-3.5.4-trunk/dcmqrdb/cmake_install.cmake")
  INCLUDE("D:/workspace/dcmtk-3.5.4-trunk/dcmimgle/cmake_install.cmake")
  INCLUDE("D:/workspace/dcmtk-3.5.4-trunk/dcmimage/cmake_install.cmake")
  INCLUDE("D:/workspace/dcmtk-3.5.4-trunk/dcmjpeg/cmake_install.cmake")
  INCLUDE("D:/workspace/dcmtk-3.5.4-trunk/dcmsign/cmake_install.cmake")
  INCLUDE("D:/workspace/dcmtk-3.5.4-trunk/dcmsr/cmake_install.cmake")
  INCLUDE("D:/workspace/dcmtk-3.5.4-trunk/dcmpstat/cmake_install.cmake")
  INCLUDE("D:/workspace/dcmtk-3.5.4-trunk/dcmwlm/cmake_install.cmake")
  INCLUDE("D:/workspace/dcmtk-3.5.4-trunk/doxygen/cmake_install.cmake")

ENDIF(NOT CMAKE_INSTALL_LOCAL_ONLY)

IF(CMAKE_INSTALL_COMPONENT)
  SET(CMAKE_INSTALL_MANIFEST "install_manifest_${CMAKE_INSTALL_COMPONENT}.txt")
ELSE(CMAKE_INSTALL_COMPONENT)
  SET(CMAKE_INSTALL_MANIFEST "install_manifest.txt")
ENDIF(CMAKE_INSTALL_COMPONENT)

FILE(WRITE "D:/workspace/dcmtk-3.5.4-trunk/${CMAKE_INSTALL_MANIFEST}" "")
FOREACH(file ${CMAKE_INSTALL_MANIFEST_FILES})
  FILE(APPEND "D:/workspace/dcmtk-3.5.4-trunk/${CMAKE_INSTALL_MANIFEST}" "${file}\n")
ENDFOREACH(file)
