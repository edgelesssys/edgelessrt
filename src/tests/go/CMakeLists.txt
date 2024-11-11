add_custom_command(
  OUTPUT test_t.c
  DEPENDS ../test.edl
  COMMAND openenclave::oeedger8r --trusted
          ${CMAKE_CURRENT_SOURCE_DIR}/../test.edl ${DEFINE_OE_SGX})

add_custom_command(
  OUTPUT gotest.a
  DEPENDS gotest.go removeall.go
  COMMAND
    sh -c
    "GOROOT='${CMAKE_SOURCE_DIR}/../3rdparty/go' '${CMAKE_SOURCE_DIR}/../3rdparty/go/bin/go' build -buildmode=c-archive -o gotest.a ${CMAKE_CURRENT_SOURCE_DIR}/gotest.go ${CMAKE_CURRENT_SOURCE_DIR}/removeall.go"
)
add_custom_target(gotestlib DEPENDS gotest.a)

add_enclave_library(erttest_go_lib OBJECT enc.cpp test_t.c)
enclave_include_directories(erttest_go_lib PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
enclave_link_libraries(erttest_go_lib PRIVATE oe_includes)
set_property(TARGET erttest_go_lib PROPERTY POSITION_INDEPENDENT_CODE ON)

add_enclave(TARGET erttest_go SOURCES ../empty.c)
enclave_link_libraries(erttest_go erttest_go_lib
                       ${CMAKE_CURRENT_BINARY_DIR}/gotest.a ertlibc)
add_dependencies(erttest_go gotestlib)

add_test(NAME tests/ert/go COMMAND erttest_host erttest_go)
