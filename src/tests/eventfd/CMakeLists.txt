add_custom_command(
  OUTPUT test_t.c
  DEPENDS ../test.edl edger8r
  COMMAND edger8r --trusted ${CMAKE_CURRENT_SOURCE_DIR}/../test.edl
          --search-path ${PROJECT_SOURCE_DIR}/include ${DEFINE_OE_SGX})

add_enclave_library(erttest_eventfd_lib OBJECT enc.cpp test_t.c)
enclave_include_directories(erttest_eventfd_lib PRIVATE
                            ${CMAKE_CURRENT_BINARY_DIR})
enclave_link_libraries(erttest_eventfd_lib PRIVATE oe_includes)
set_property(TARGET erttest_eventfd_lib PROPERTY POSITION_INDEPENDENT_CODE ON)

add_enclave(TARGET erttest_eventfd SOURCES ../empty.c)
enclave_link_libraries(erttest_eventfd erttest_eventfd_lib ertlibc)

add_enclave_test(tests/ert/eventfd erttest_host erttest_eventfd)
