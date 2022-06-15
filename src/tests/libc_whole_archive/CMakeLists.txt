add_custom_command(
  OUTPUT test_t.c
  DEPENDS ../test.edl
  COMMAND openenclave::oeedger8r --trusted
          ${CMAKE_CURRENT_SOURCE_DIR}/../test.edl ${DEFINE_OE_SGX})

add_enclave_library(erttest_libc_whole_archive_lib OBJECT enc.cpp test_t.c)
enclave_include_directories(erttest_libc_whole_archive_lib PRIVATE
                            ${CMAKE_CURRENT_BINARY_DIR})
enclave_link_libraries(erttest_libc_whole_archive_lib PRIVATE oe_includes)
set_property(TARGET erttest_libc_whole_archive_lib
             PROPERTY POSITION_INDEPENDENT_CODE ON)

add_enclave(TARGET erttest_libc_whole_archive SOURCES ../empty.c)
enclave_link_libraries(
  erttest_libc_whole_archive erttest_libc_whole_archive_lib ertlibc
  -Wl,--whole-archive openenclave::oelibc -Wl,--no-whole-archive)

add_test(NAME tests/ert/libc_whole_archive COMMAND erttest_host
                                                   erttest_libc_whole_archive)
