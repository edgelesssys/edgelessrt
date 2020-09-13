add_enclave_library(erttest_meshentry_nongo_app_lib enc.cpp)
enclave_link_libraries(erttest_meshentry_nongo_app_lib PRIVATE oe_includes)
set_property(TARGET erttest_meshentry_nongo_app_lib
             PROPERTY POSITION_INDEPENDENT_CODE ON)

add_enclave(TARGET erttest_meshentry_nongo_app SOURCES ../empty.c)
enclave_link_libraries(
  erttest_meshentry_nongo_app ertmeshentry erttest_meshentry_premainmock
  erttest_meshentry_nongo_app_lib)
add_dependencies(erttest_meshentry_nongo_app ertdevhost)
add_enclave_test(tests/ert/meshentry_nongo_app ertdevhost
                 erttest_meshentry_nongo_app)
