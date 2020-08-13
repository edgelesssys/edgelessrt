add_enclave_library(ertlibunwind
                    ${CMAKE_SOURCE_DIR}/3rdparty/libcxx/__dso_handle.cpp)
enclave_link_libraries(ertlibunwind PRIVATE libunwind)

add_enclave_library(
  ertlibc
  dynlink.cpp
  locale.cpp
  misc.cpp
  new_thread.cpp
  pthread.cpp
  sched.cpp
  stdio.cpp
  stdlib.cpp
  stubs.cpp
  syscall.cpp
  time.cpp
  __vasprintf_chk.c)

set_enclave_property(TARGET ertlibc PROPERTY POSITION_INDEPENDENT_CODE ON)
enclave_include_directories(ertlibc PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
enclave_compile_definitions(ertlibc PRIVATE OE_BUILD_ENCLAVE)
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

set(EDL_FILE ${CMAKE_SOURCE_DIR}/include/openenclave/edl/ertlibc.edl)
add_custom_command(
  OUTPUT ertlibc_t.h
  DEPENDS ${EDL_FILE} edger8r
  COMMAND edger8r --header-only --trusted ${EDL_FILE})
add_custom_target(ertlibc_trusted_edl DEPENDS ertlibc_t.h)
add_enclave_dependencies(ertlibc ertlibc_trusted_edl)

install_enclaves(
  TARGETS
  ertlibc
  ertlibunwind
  EXPORT
  openenclave-targets
  ARCHIVE
  DESTINATION
  ${CMAKE_INSTALL_LIBDIR}/openenclave/enclave)
