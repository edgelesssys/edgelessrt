add_enclave_library(devhost_enc_lib enc.cpp)
enclave_link_libraries(devhost_enc_lib PRIVATE oe_includes)
set_property(TARGET devhost_enc_lib PROPERTY POSITION_INDEPENDENT_CODE ON)

add_enclave(TARGET devhost_enc SOURCES ${CMAKE_SOURCE_DIR}/ert/tests/empty.c)
target_link_libraries(devhost_enc ertdeventry devhost_enc_lib)
add_dependencies(devhost_enc ertdevhost)
add_test(
  NAME tests/devhost
  COMMAND
    sh -c
    "OE_DEVHOST_TEST=test ${CMAKE_BINARY_DIR}/tools/ertdevhost/ertdevhost ./devhost_enc arg1 arg2"
)
