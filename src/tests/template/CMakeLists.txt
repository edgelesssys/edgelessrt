add_custom_command(
  OUTPUT test_t.c
  DEPENDS ../test.edl
  COMMAND openenclave::oeedger8r --trusted
          ${CMAKE_CURRENT_SOURCE_DIR}/../test.edl ${DEFINE_OE_SGX})

add_enclave_library(erttest_template_lib OBJECT enc.cpp test_t.c)
enclave_include_directories(erttest_template_lib PRIVATE
                            ${CMAKE_CURRENT_BINARY_DIR})
enclave_link_libraries(erttest_template_lib PRIVATE oe_includes)
set_property(TARGET erttest_template_lib PROPERTY POSITION_INDEPENDENT_CODE ON)

add_enclave(TARGET erttest_template SOURCES ../empty.c)
enclave_link_libraries(erttest_template erttest_template_lib ertlibc)

add_test(NAME tests/ert/template COMMAND erttest_host erttest_template)
