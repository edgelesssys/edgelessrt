set(ERTHOSTDIR ${CMAKE_SOURCE_DIR}/ert/host)

list(APPEND PLATFORM_SDK_ONLY_SRC ${ERTHOSTDIR}/core.c ${ERTHOSTDIR}/thread.cpp)

set(ERT_EDL_FILE ${CMAKE_SOURCE_DIR}/include/openenclave/edl/ertlibc.edl)
add_custom_command(
  OUTPUT ertlibc_t.h
  DEPENDS ${ERT_EDL_FILE} edger8r
  COMMAND edger8r --header-only --untrusted ${ERT_EDL_FILE})
add_custom_target(ertlibc_untrusted_edl DEPENDS ertlibc_t.h)
