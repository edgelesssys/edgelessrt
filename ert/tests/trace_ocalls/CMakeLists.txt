add_custom_command(
  OUTPUT test_t.c test_u.c
  DEPENDS test.edl edger8r
  COMMAND edger8r ${CMAKE_CURRENT_SOURCE_DIR}/test.edl --search-path
          ${PROJECT_SOURCE_DIR}/include ${DEFINE_OE_SGX})

add_executable(erttest_trace_ocalls_host host.cpp test_u.c)
target_include_directories(erttest_trace_ocalls_host
                           PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
target_link_libraries(erttest_trace_ocalls_host -rdynamic oehost)

add_enclave(TARGET erttest_trace_ocalls SOURCES enc.cpp test_t.c)
enclave_include_directories(erttest_trace_ocalls PRIVATE
                            ${CMAKE_CURRENT_BINARY_DIR})

# expect ocall list in stdout (similar to tests/print)
add_test(
  NAME tests/ert/trace_ocalls
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  COMMAND
    sh -c
    "OE_TRACE_OCALLS=1 ./erttest_trace_ocalls_host ./erttest_trace_ocalls | grep -P '3\tocall_my_ocall'"
)
