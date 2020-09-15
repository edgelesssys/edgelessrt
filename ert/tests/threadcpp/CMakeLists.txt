add_custom_command(
  OUTPUT test_t.c
  DEPENDS ../test.edl edger8r
  COMMAND edger8r --trusted ${CMAKE_CURRENT_SOURCE_DIR}/../test.edl
          --search-path ${PROJECT_SOURCE_DIR}/include ${DEFINE_OE_SGX})

add_enclave_library(erttest_threadcpp_lib OBJECT enc.cpp test_t.c)
enclave_include_directories(erttest_threadcpp_lib PRIVATE
                            ${CMAKE_CURRENT_BINARY_DIR})
enclave_link_libraries(erttest_threadcpp_lib PRIVATE oe_includes)
set_property(TARGET erttest_threadcpp_lib PROPERTY POSITION_INDEPENDENT_CODE ON)

add_enclave(TARGET erttest_threadcpp SOURCES ../empty.c)
enclave_link_libraries(erttest_threadcpp erttest_threadcpp_lib ertlibc)

add_enclave_test(tests/ert/threadcpp erttest_host erttest_threadcpp)
