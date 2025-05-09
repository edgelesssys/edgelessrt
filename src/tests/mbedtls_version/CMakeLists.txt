add_custom_command(
  OUTPUT test_t.c
  DEPENDS ../test.edl
  COMMAND openenclave::oeedger8r --trusted
          ${CMAKE_CURRENT_SOURCE_DIR}/../test.edl ${DEFINE_OE_SGX})

add_enclave_library(erttest_mbedtls_version_lib OBJECT enc.cpp test_t.c)
enclave_include_directories(erttest_mbedtls_version_lib PRIVATE
                            ${CMAKE_CURRENT_BINARY_DIR})
enclave_link_libraries(erttest_mbedtls_version_lib PRIVATE oe_includes)
set_property(TARGET erttest_mbedtls_version_lib
             PROPERTY POSITION_INDEPENDENT_CODE ON)

add_enclave(TARGET erttest_mbedtls_version SOURCES ../empty.c)
enclave_link_libraries(erttest_mbedtls_version erttest_mbedtls_version_lib
                       ertlibc)

add_test(NAME tests/ert/mbedtls_version COMMAND erttest_host
                                                erttest_mbedtls_version)
