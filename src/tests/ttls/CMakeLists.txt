add_custom_command(
  OUTPUT test_t.c
  DEPENDS ../test.edl
  COMMAND openenclave::oeedger8r --trusted
          ${CMAKE_CURRENT_SOURCE_DIR}/../test.edl ${DEFINE_OE_SGX})

add_enclave_library(erttest_ttls_lib OBJECT enc.cpp test_t.c)
enclave_include_directories(
  erttest_ttls_lib PRIVATE ${CMAKE_CURRENT_BINARY_DIR}
  ${CMAKE_SOURCE_DIR}/../3rdparty/ttls/include)
enclave_link_libraries(erttest_ttls_lib PRIVATE oe_includes)
set_property(TARGET erttest_ttls_lib PROPERTY POSITION_INDEPENDENT_CODE ON)

add_enclave(TARGET erttest_ttls SOURCES ../empty.c)
enclave_link_libraries(erttest_ttls erttest_ttls_lib ertttls ertlibc)

add_test(NAME tests/ert/ttls COMMAND erttest_host erttest_ttls)
