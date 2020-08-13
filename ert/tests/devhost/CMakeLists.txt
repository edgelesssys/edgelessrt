add_enclave_library(erttest_devhost_lib enc.cpp)
enclave_link_libraries(erttest_devhost_lib PRIVATE oe_includes)
set_property(TARGET erttest_devhost_lib PROPERTY POSITION_INDEPENDENT_CODE ON)

add_enclave(TARGET erttest_devhost SOURCES ../empty.c)
enclave_link_libraries(erttest_devhost ertdeventry erttest_devhost_lib)
add_dependencies(erttest_devhost ertdevhost)
add_test(
  NAME tests/ert/devhost
  COMMAND
    sh -c
    "OE_DEVHOST_TEST=test ${CMAKE_BINARY_DIR}/ert/tools/devhost/ertdevhost ./erttest_devhost arg1 arg2"
)
