# Copyright (c) Open Enclave SDK contributors.
# Licensed under the MIT License.

set(EDL_FILE ../test_customfs.edl)

add_custom_command(
  OUTPUT test_customfs_t.h test_customfs_t.c
  DEPENDS ${EDL_FILE} edger8r
  COMMAND
    edger8r --trusted ${EDL_FILE} --search-path ${PROJECT_SOURCE_DIR}/include
    ${DEFINE_OE_SGX} --search-path ${CMAKE_CURRENT_SOURCE_DIR} --search-path
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../device/edl)

add_enclave(TARGET customfs_enc SOURCES enc.c main.c
            ${CMAKE_CURRENT_BINARY_DIR}/test_customfs_t.c)

enclave_link_libraries(customfs_enc oelibc oecustomfs oeenclave)
