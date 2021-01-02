# Copyright (c) Open Enclave SDK contributors.
# Licensed under the MIT License.

set(EDL_FILE ../threadcxx.edl)

add_custom_command(
  OUTPUT threadcxx_u.h threadcxx_u.c
  DEPENDS ${EDL_FILE}
  COMMAND
    openenclave::oeedger8r --untrusted ${EDL_FILE} --search-path
    ${PROJECT_SOURCE_DIR}/include ${DEFINE_OE_SGX} --search-path
    ${CMAKE_CURRENT_SOURCE_DIR})

add_executable(erttest_threadcxx_host ${DIR}/host/host.cpp threadcxx_u.c)

target_include_directories(erttest_threadcxx_host
                           PRIVATE ${CMAKE_CURRENT_BINARY_DIR})

target_link_libraries(erttest_threadcxx_host openenclave::oehost oe_includes)
