add_custom_command(
  OUTPUT test_t.c
  DEPENDS ../test.edl
  COMMAND openenclave::oeedger8r --trusted
          ${CMAKE_CURRENT_SOURCE_DIR}/../test.edl ${DEFINE_OE_SGX})

add_custom_command(
  OUTPUT gotest.a
  DEPENDS gotest.go
  COMMAND
    sh -c
    "GOROOT='${CMAKE_SOURCE_DIR}/../3rdparty/go' '${CMAKE_SOURCE_DIR}/../3rdparty/go/bin/go' build -buildmode=c-archive ${CMAKE_CURRENT_SOURCE_DIR}/gotest.go"
)
add_custom_target(gonettestlib DEPENDS gotest.a)

add_enclave_library(erttest_go_net_lib OBJECT enc.cpp test_t.c)
enclave_include_directories(erttest_go_net_lib PRIVATE
                            ${CMAKE_CURRENT_BINARY_DIR})
enclave_link_libraries(erttest_go_net_lib PRIVATE oe_includes)
set_property(TARGET erttest_go_net_lib PROPERTY POSITION_INDEPENDENT_CODE ON)

add_enclave(TARGET erttest_go_net SOURCES ../empty.c)
enclave_link_libraries(
  erttest_go_net erttest_go_net_lib ${CMAKE_CURRENT_BINARY_DIR}/gotest.a
  ertlibc openenclave::oehostepoll openenclave::oehostsock)
add_dependencies(erttest_go_net gonettestlib)

add_test(NAME tests/ert/go_net COMMAND erttest_host erttest_go_net)
