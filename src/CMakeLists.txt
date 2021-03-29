cmake_minimum_required(VERSION 3.11)
project(ertcore)

find_package(
  OpenEnclave REQUIRED PATHS
  ${OEBUILDDIR}/openenclave-install/lib/openenclave/cmake NO_DEFAULT_PATH)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_EXTENSIONS OFF)
add_subdirectory(../3rdparty/mystikos mystikos)
add_compile_options(-Wall -Wextra -Werror -Wno-missing-field-initializers)
if (CMAKE_C_COMPILER_ID MATCHES GNU)
  add_compile_options(-fno-builtin)
endif ()

include(GNUInstallDirs)

add_library(oe_includes INTERFACE)
target_include_directories(
  oe_includes INTERFACE ert/include ${OESRCDIR}/include
                        ${OEBUILDDIR}/openenclave/output/include)

set(TTLS_NOTEST True)
add_subdirectory(../3rdparty/ttls ttls)
target_include_directories(
  ttls PUBLIC ${OEBUILDDIR}/openenclave/3rdparty
              ${OESRCDIR}/3rdparty/mbedtls/mbedtls/include)

add_subdirectory(ertlibc)
add_subdirectory(meshpremain)
add_subdirectory(tools/erthost)
add_subdirectory(tools/ertgo)

if (BUILD_TESTS)
  enable_testing()
  add_subdirectory(tests)
endif ()

install(
  FILES ${PROJECT_SOURCE_DIR}/ert/include/openenclave/ert.h
        ${PROJECT_SOURCE_DIR}/ert/include/openenclave/ert_args.h
        ${PROJECT_SOURCE_DIR}/ert/include/openenclave/ert_stubs.h
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/openenclave)
install(FILES ${PROJECT_SOURCE_DIR}/ert/include/openenclave/edl/ertlibc.edl
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/openenclave/edl)

# install ert-targets.cmake and patch openenclave-config.cmake such that it includes ert-config.cmake
install(
  EXPORT openenclave-targets
  NAMESPACE openenclave::
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/openenclave/cmake
  FILE ert-targets.cmake)
file(
  COPY ${OEBUILDDIR}/openenclave-install/lib/openenclave/cmake/openenclave-config.cmake
  DESTINATION .)
file(APPEND ${CMAKE_CURRENT_BINARY_DIR}/openenclave-config.cmake
     "include(\${CMAKE_CURRENT_LIST_DIR}/ert-config.cmake)")
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/openenclave-config.cmake
              ert-config.cmake
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/openenclave/cmake)
