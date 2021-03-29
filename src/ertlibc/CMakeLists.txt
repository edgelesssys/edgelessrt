add_library(
  ertlibc
  customfs.c
  dynlink.cpp
  locale.cpp
  memfs.cpp
  misc.cpp
  new_thread.cpp
  pthread.cpp
  sched.cpp
  signal.cpp
  signal_manager.cpp
  stdio.cpp
  stdlib.cpp
  stubs.cpp
  syscall.cpp
  time.cpp
  util.cpp
  __vasprintf_chk.c)

set_property(TARGET ertlibc PROPERTY POSITION_INDEPENDENT_CODE ON)
target_include_directories(ertlibc PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
target_compile_definitions(ertlibc PRIVATE OE_BUILD_ENCLAVE)
target_link_libraries(
  ertlibc
  INTERFACE -static -Wl,--eh-frame-hdr -Wl,-u,ert_syscall stdc++
            openenclave::ertlibunwind openenclave::oecryptombedtls
  PRIVATE $<BUILD_INTERFACE:oe_includes> $<BUILD_INTERFACE:mystikos_ramfs>)

set(EDL_FILE ${CMAKE_SOURCE_DIR}/ert/include/openenclave/edl/ertlibc.edl)
add_custom_command(
  OUTPUT ertlibc_t.h
  DEPENDS ${EDL_FILE}
  COMMAND openenclave::oeedger8r --header-only --trusted ${EDL_FILE})
add_custom_target(ertlibc_trusted_edl DEPENDS ertlibc_t.h)
add_dependencies(ertlibc ertlibc_trusted_edl)

install(
  TARGETS ertlibc
  EXPORT openenclave-targets
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/openenclave/enclave)
