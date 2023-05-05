cmake_minimum_required(VERSION 3.11)

if (NOT DEFINED ENV{CC} AND NOT DEFINED CMAKE_C_COMPILER)
  find_program(CMAKE_C_COMPILER clang-11 clang-10 clang)
endif ()
if (NOT DEFINED ENV{CXX} AND NOT DEFINED CMAKE_CXX_COMPILER)
  find_program(CMAKE_CXX_COMPILER clang++-11 clang++-10 clang++)
endif ()

project(edgelessrt)
file(STRINGS "VERSION" OE_VERSION_WITH_V)
string(REGEX REPLACE "^v" "" OE_VERSION ${OE_VERSION_WITH_V})
set(PROJECT_VERSION ${OE_VERSION})

if (NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Debug)
endif ()
if (CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
  set(CMAKE_INSTALL_PREFIX
      /opt/edgelessrt
      CACHE PATH "" FORCE)
endif ()

if (IS_DIRECTORY "${PROJECT_SOURCE_DIR}/.git")
  # Install Git pre-commit hook
  file(COPY scripts/pre-commit scripts/commit-msg
       DESTINATION "${PROJECT_SOURCE_DIR}/.git/hooks")
endif ()

option(BUILD_TESTS "" ON)
configure_file(src/CTestTestfile.cmake . COPYONLY)

include(ExternalProject)
include(GNUInstallDirs)

# update required submodules
execute_process(
  COMMAND git submodule update --init 3rdparty/go 3rdparty/mystikos/mystikos
          3rdparty/ttls WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
if (NOT OEDEV OR NOT EXISTS
                 ${CMAKE_SOURCE_DIR}/3rdparty/openenclave/openenclave/.git)
  execute_process(
    COMMAND git submodule update --init 3rdparty/openenclave/openenclave
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
endif ()
execute_process(
  COMMAND git submodule update --init tools/oeedger8r-cpp
          3rdparty/mbedtls/mbedtls 3rdparty/musl/musl 3rdparty/musl/libc-test
  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/3rdparty/openenclave/openenclave)

add_subdirectory(3rdparty/openenclave)
add_subdirectory(docs/refman)

ExternalProject_Add(
  ertcore
  DEPENDS openenclave
  SOURCE_DIR ${CMAKE_SOURCE_DIR}/src
  BINARY_DIR ertcore
  CMAKE_ARGS -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
             -DCMAKE_INSTALL_PREFIX=${CMAKE_BINARY_DIR}/ertcore-install
             -DCMAKE_C_COMPILER=${CMAKE_C_COMPILER}
             -DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}
             -DBUILD_TESTS=${BUILD_TESTS}
             -DOESRCDIR=${OESRCDIR}
             -DOEBUILDDIR=${OEBUILDDIR}
             -DOE_VERSION=${OE_VERSION}
  BUILD_ALWAYS ON
  USES_TERMINAL_BUILD ON)

# install OE first and then ERT on top such that it can overwrite files
install(
  DIRECTORY ${CMAKE_BINARY_DIR}/3rdparty/openenclave/openenclave-install/
  DESTINATION .
  USE_SOURCE_PERMISSIONS
  PATTERN samples EXCLUDE
  # exclude openenclaverc because overwrite is not reliable for this file
  PATTERN openenclaverc EXCLUDE)
install(
  DIRECTORY ${CMAKE_BINARY_DIR}/ertcore-install/
  DESTINATION .
  USE_SOURCE_PERMISSIONS)

# generate and install the openenclaverc script
configure_file(src/openenclaverc.in openenclaverc @ONLY)
install(FILES ${CMAKE_BINARY_DIR}/openenclaverc
        DESTINATION ${CMAKE_INSTALL_DATADIR}/openenclave)

install(
  DIRECTORY samples
  DESTINATION ${CMAKE_INSTALL_DATADIR}/openenclave
  PATTERN build EXCLUDE)
install(
  DIRECTORY 3rdparty/go/
  DESTINATION go
  USE_SOURCE_PERMISSIONS)

set(CPACK_PACKAGE_CONTACT "contact@edgeless.systems​")
set(CPACK_PACKAGE_VERSION ${OE_VERSION})
set(CPACK_PACKAGING_INSTALL_PREFIX ${CMAKE_INSTALL_PREFIX})
set(CPACK_DEBIAN_PACKAGE_DEPENDS
    "libsgx-enclave-common (>=2.3.100.46354-1), libsgx-dcap-ql (>=1.0.100.46460-1.0)"
)
set(CPACK_DEBIAN_FILE_NAME DEB-DEFAULT)
set(CPACK_DEBIAN_ARCHIVE_TYPE gnutar)
include(CPack)
