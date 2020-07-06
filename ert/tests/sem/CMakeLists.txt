add_custom_command(
  OUTPUT test_t.c
  DEPENDS ../test.edl edger8r
  COMMAND
    edger8r --trusted ${CMAKE_CURRENT_SOURCE_DIR}/../test.edl --search-path
    ${PROJECT_SOURCE_DIR}/include --search-path ${PLATFORM_EDL_DIR})

add_enclave_library(erttest_sem_lib enc.cpp test_t.c)
enclave_include_directories(erttest_sem_lib PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
enclave_link_libraries(erttest_sem_lib PRIVATE oe_includes)
set_property(TARGET erttest_sem_lib PROPERTY POSITION_INDEPENDENT_CODE ON)

add_enclave(TARGET erttest_sem SOURCES ../empty.c)
enclave_link_libraries(erttest_sem erttest_sem_lib ertlibc)

add_enclave_test(tests/ert/sem erttest_host erttest_sem)
