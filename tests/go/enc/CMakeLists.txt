add_custom_command(
  OUTPUT test_t.c
  DEPENDS ../test.edl edger8r
  COMMAND
    edger8r --trusted ${CMAKE_CURRENT_SOURCE_DIR}/../test.edl --search-path
    ${PROJECT_SOURCE_DIR}/include --search-path ${PLATFORM_EDL_DIR})

add_custom_command(
  OUTPUT gotest.a
  DEPENDS gotest.go
  COMMAND
    sh -c
    "GOROOT='${CMAKE_SOURCE_DIR}/3rdparty/go' '${CMAKE_SOURCE_DIR}/3rdparty/go/bin/go' build -buildmode=c-archive ${CMAKE_CURRENT_SOURCE_DIR}/gotest.go"
)
add_custom_target(gotestlib DEPENDS gotest.a)

add_enclave(TARGET go_enc CXX SOURCES enc.cpp test_t.c)
add_dependencies(go_enc gotestlib)
target_include_directories(go_enc PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
target_link_libraries(go_enc ${CMAKE_CURRENT_BINARY_DIR}/gotest.a)
