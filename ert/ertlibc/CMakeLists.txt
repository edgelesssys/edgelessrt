add_enclave_library(ertlibunwind
                    ${CMAKE_SOURCE_DIR}/3rdparty/libcxx/__dso_handle.cpp)
enclave_link_libraries(ertlibunwind PRIVATE libunwind)

add_enclave_library(
  ertlibc
  syscall.cpp)

set_enclave_property(TARGET ertlibc PROPERTY POSITION_INDEPENDENT_CODE ON)
enclave_link_libraries(
  ertlibc
  INTERFACE
  -static
  -Wl,--eh-frame-hdr
  -Wl,-u,ert_syscall
  stdc++
  ertlibunwind
  PRIVATE
  oe_includes)

install_enclaves(
  TARGETS
  ertlibc
  ertlibunwind
  EXPORT
  openenclave-targets
  ARCHIVE
  DESTINATION
  ${CMAKE_INSTALL_LIBDIR}/openenclave/enclave)
