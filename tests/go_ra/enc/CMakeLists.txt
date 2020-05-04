add_custom_command(
  OUTPUT test_t.c
  DEPENDS ../test.edl edger8r
  COMMAND
    edger8r --trusted ${CMAKE_CURRENT_SOURCE_DIR}/../test.edl --search-path
    ${PROJECT_SOURCE_DIR}/include --search-path ${PLATFORM_EDL_DIR})

add_custom_command(
  OUTPUT gotest.a
  DEPENDS gotest.go
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
  COMMAND
    sh -c
    "CGO_CFLAGS='-I${CMAKE_SOURCE_DIR}/include -I${CMAKE_CURRENT_SOURCE_DIR}/include' GOROOT='${CMAKE_SOURCE_DIR}/3rdparty/go' '${CMAKE_SOURCE_DIR}/3rdparty/go/bin/go' build -buildmode=c-archive -o '${CMAKE_CURRENT_BINARY_DIR}/gotest.a' gotest.go"
)
add_custom_target(goratestlib DEPENDS gotest.a)

add_enclave(TARGET go_ra_enc CXX SOURCES enc.cpp test_t.c)
add_dependencies(go_ra_enc goratestlib)
target_include_directories(go_ra_enc PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
target_link_libraries(go_ra_enc ${CMAKE_CURRENT_BINARY_DIR}/gotest.a)
