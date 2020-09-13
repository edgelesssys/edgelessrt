add_custom_command(
  OUTPUT gotest.a
  DEPENDS main.go mesh.go
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
  COMMAND
    sh -c
    "GOROOT='${CMAKE_SOURCE_DIR}/3rdparty/go' '${CMAKE_SOURCE_DIR}/3rdparty/go/bin/go' build -buildmode=c-archive -o '${CMAKE_CURRENT_BINARY_DIR}/gotest.a' main.go mesh.go"
)
add_custom_target(erttest_meshentry_go_app_lib DEPENDS gotest.a)

add_enclave(TARGET erttest_meshentry_go_app SOURCES main.c)
enclave_link_libraries(erttest_meshentry_go_app ertmeshentry
                       ${CMAKE_CURRENT_BINARY_DIR}/gotest.a)
add_dependencies(erttest_meshentry_go_app ertdevhost
                 erttest_meshentry_go_app_lib)
add_enclave_test(tests/ert/meshentry_go_app ertdevhost erttest_meshentry_go_app)
