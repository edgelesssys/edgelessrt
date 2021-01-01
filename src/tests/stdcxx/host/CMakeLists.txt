# Copyright (c) Open Enclave SDK contributors.
# Licensed under the MIT License.

set(EDL_FILE ../stdcxx.edl)

add_custom_command(
  OUTPUT stdcxx_u.h stdcxx_u.c
  DEPENDS ${EDL_FILE} edger8r
  COMMAND
    edger8r --untrusted ${EDL_FILE} --search-path ${PROJECT_SOURCE_DIR}/include
    ${DEFINE_OE_SGX} --search-path ${CMAKE_CURRENT_SOURCE_DIR})

add_executable(erttest_stdcxx_host ${DIR}/host/host.cpp stdcxx_u.c)

target_include_directories(erttest_stdcxx_host
                           PRIVATE ${CMAKE_CURRENT_BINARY_DIR})

target_link_libraries(erttest_stdcxx_host oehost)
