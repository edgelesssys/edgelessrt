list(
  APPEND
  PLATFORM_SDK_ONLY_SRC
  ${CMAKE_CURRENT_LIST_DIR}/calls.cpp
  ${CMAKE_CURRENT_LIST_DIR}/core.c
  ${CMAKE_CURRENT_LIST_DIR}/debug.cpp
  ${CMAKE_CURRENT_LIST_DIR}/enclave_thread_manager.cpp
  ${CMAKE_CURRENT_LIST_DIR}/mmapfs.cpp
  ${CMAKE_CURRENT_LIST_DIR}/ocall_tracer.cpp
  ${CMAKE_CURRENT_LIST_DIR}/restart.cpp
  ${CMAKE_CURRENT_LIST_DIR}/syscall.cpp
  ${CMAKE_CURRENT_LIST_DIR}/thread.cpp
  ${CMAKE_CURRENT_LIST_DIR}/vdso.cpp)

set(ERT_EDL_FILE ${ERTSRC}/include/openenclave/edl/ertlibc.edl)
add_custom_command(
  OUTPUT ertlibc_t.h
  DEPENDS ${ERT_EDL_FILE} edger8r
  COMMAND edger8r --header-only --untrusted ${ERT_EDL_FILE})
add_custom_target(ertlibc_untrusted_edl DEPENDS ertlibc_t.h)
