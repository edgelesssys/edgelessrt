add_custom_command(
  OUTPUT test_t.c
  DEPENDS ../test.edl
  COMMAND openenclave::oeedger8r --trusted
          ${CMAKE_CURRENT_SOURCE_DIR}/../test.edl ${DEFINE_OE_SGX})

add_enclave(TARGET erttest_customfs SOURCES enc.c main.c test_t.c)
enclave_link_libraries(erttest_customfs ertlibc oe_includes)

add_test(NAME tests/ert/customfs COMMAND erttest_host erttest_customfs)
