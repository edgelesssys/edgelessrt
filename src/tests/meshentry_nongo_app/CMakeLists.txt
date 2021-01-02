add_enclave_library(erttest_meshentry_nongo_app_lib enc.cpp)
enclave_link_libraries(erttest_meshentry_nongo_app_lib PRIVATE oe_includes)
set_property(TARGET erttest_meshentry_nongo_app_lib
             PROPERTY POSITION_INDEPENDENT_CODE ON)

add_enclave(TARGET erttest_meshentry_nongo_app SOURCES ../empty.c)
enclave_link_libraries(
  erttest_meshentry_nongo_app ertmeshentry erttest_meshentry_premainmock
  erttest_meshentry_nongo_app_lib)
add_dependencies(erttest_meshentry_nongo_app erthost)
add_test(NAME tests/ert/meshentry_nongo_app COMMAND erthost
                                                    erttest_meshentry_nongo_app)
set_enclave_tests_properties(tests/ert/meshentry_nongo_app PROPERTIES
                             ENVIRONMENT EDG_FOO=bar;ABC_FOO=bar)
