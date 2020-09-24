add_enclave_library(erttest_deventry_lib enc.cpp)
enclave_link_libraries(erttest_deventry_lib PRIVATE oe_includes)
set_property(TARGET erttest_deventry_lib PROPERTY POSITION_INDEPENDENT_CODE ON)

add_enclave(TARGET erttest_deventry SOURCES ../empty.c)
enclave_link_libraries(erttest_deventry ertdeventry erttest_deventry_lib)
add_dependencies(erttest_deventry erthost)
add_test(
  NAME tests/ert/deventry
  COMMAND
    sh -c
    "ERT_DEVENTRY_TEST=test ${CMAKE_BINARY_DIR}/ert/tools/erthost/erthost ./erttest_deventry arg1 arg2"
)
