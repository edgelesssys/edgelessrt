# Copyright (c) Open Enclave SDK contributors.
# Licensed under the MIT License.

add_enclave_library(oecustomfs STATIC customfs.c)

maybe_build_using_clangw(oecustomfs)

enclave_include_directories(oecustomfs PRIVATE ${CMAKE_BINARY_DIR}/syscall
                            ${PROJECT_SOURCE_DIR}/include/openenclave/corelibc)

enclave_link_libraries(oecustomfs oesyscall)

install_enclaves(
  TARGETS
  oecustomfs
  EXPORT
  openenclave-targets
  ARCHIVE
  DESTINATION
  ${CMAKE_INSTALL_LIBDIR}/openenclave/enclave)
