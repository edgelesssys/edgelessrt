add_enclave_library(erttest_customentry_lib enc.cpp)
enclave_link_libraries(erttest_customentry_lib PRIVATE oe_includes)
set_property(TARGET erttest_customentry_lib PROPERTY POSITION_INDEPENDENT_CODE
                                                     ON)

add_enclave(TARGET erttest_customentry SOURCES ../empty.c)
enclave_link_libraries(erttest_customentry ertcalls erttest_customentry_lib)
add_dependencies(erttest_customentry erthost)
add_enclave_test(tests/ert/customentry erthost erttest_customentry)
