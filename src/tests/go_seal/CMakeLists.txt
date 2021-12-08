add_custom_command(
  OUTPUT test_t.c
  DEPENDS ../test.edl
  COMMAND openenclave::oeedger8r --trusted
          ${CMAKE_CURRENT_SOURCE_DIR}/../test.edl ${DEFINE_OE_SGX})

add_custom_command(
  OUTPUT gotest.a
  DEPENDS gotest.go
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
  COMMAND
    sh -c
    "CGO_CFLAGS='-I${OEBUILDDIR}/openenclave-install/include' GOROOT='${CMAKE_SOURCE_DIR}/../3rdparty/go' '${CMAKE_SOURCE_DIR}/../3rdparty/go/bin/go' build -buildmode=c-archive -o '${CMAKE_CURRENT_BINARY_DIR}/gotest.a' gotest.go"
)
add_custom_target(gosealtestlib DEPENDS gotest.a)

add_enclave_library(erttest_go_seal_lib OBJECT enc.cpp test_t.c)
enclave_include_directories(erttest_go_seal_lib PRIVATE
                            ${CMAKE_CURRENT_BINARY_DIR})
enclave_link_libraries(erttest_go_seal_lib PRIVATE oe_includes)
set_property(TARGET erttest_go_seal_lib PROPERTY POSITION_INDEPENDENT_CODE ON)

add_enclave(TARGET erttest_go_seal SOURCES ../empty.c)
enclave_link_libraries(erttest_go_seal erttest_go_seal_lib
                       ${CMAKE_CURRENT_BINARY_DIR}/gotest.a ertlibc)
add_dependencies(erttest_go_seal gosealtestlib)

add_test(NAME tests/ert/go_seal COMMAND erttest_host erttest_go_seal)
