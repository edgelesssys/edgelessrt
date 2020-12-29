add_custom_command(
  OUTPUT test_t.c
  DEPENDS ../test.edl edger8r
  COMMAND edger8r --trusted ${CMAKE_CURRENT_SOURCE_DIR}/../test.edl
          --search-path ${PROJECT_SOURCE_DIR}/include ${DEFINE_OE_SGX})

add_enclave(TARGET erttest_customfs SOURCES enc.c main.c test_t.c)
enclave_link_libraries(erttest_customfs ertlibc)

add_enclave_test(tests/ert/customfs erttest_host erttest_customfs)
