add_custom_command(
  OUTPUT gotest.a
  DEPENDS main.go mesh.go
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
  COMMAND
    sh -c
    "GOROOT='${CMAKE_SOURCE_DIR}/../3rdparty/go' '${CMAKE_SOURCE_DIR}/../3rdparty/go/bin/go' build -buildmode=c-archive -o '${CMAKE_CURRENT_BINARY_DIR}/gotest.a' main.go mesh.go"
)
add_custom_target(erttest_meshentry_go_app_lib DEPENDS gotest.a)

add_enclave(TARGET erttest_meshentry_go_app SOURCES main.c)
enclave_link_libraries(erttest_meshentry_go_app ertmeshentry
                       ${CMAKE_CURRENT_BINARY_DIR}/gotest.a oe_includes)
add_dependencies(erttest_meshentry_go_app erthost erttest_meshentry_go_app_lib)
add_test(NAME tests/ert/meshentry_go_app COMMAND erthost
                                                 erttest_meshentry_go_app)
set_enclave_tests_properties(tests/ert/meshentry_go_app PROPERTIES ENVIRONMENT
                             EDG_FOO=bar;ABC_FOO=bar)
