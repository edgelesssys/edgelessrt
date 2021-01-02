add_custom_command(
  OUTPUT test_t.c
  DEPENDS ../test.edl
  COMMAND openenclave::oeedger8r --trusted
          ${CMAKE_CURRENT_SOURCE_DIR}/../test.edl ${DEFINE_OE_SGX})

add_enclave_library(erttest_args_enc_custom_lib OBJECT enc.cpp test_t.c)
enclave_include_directories(erttest_args_enc_custom_lib PRIVATE
                            ${CMAKE_CURRENT_BINARY_DIR})
enclave_link_libraries(erttest_args_enc_custom_lib PRIVATE oe_includes)
set_property(TARGET erttest_args_enc_custom_lib
             PROPERTY POSITION_INDEPENDENT_CODE ON)

add_enclave(TARGET erttest_args_enc_custom SOURCES ../../empty.c)
enclave_link_libraries(erttest_args_enc_custom erttest_args_enc_custom_lib
                       ertlibc)
