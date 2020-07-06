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

add_enclave_library(go_ra_enc_lib enc.cpp test_t.c)
enclave_include_directories(go_ra_enc_lib PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
enclave_link_libraries(go_ra_enc_lib PRIVATE oe_includes)
set_property(TARGET go_ra_enc_lib PROPERTY POSITION_INDEPENDENT_CODE ON)

add_enclave(TARGET go_ra_enc SOURCES ${CMAKE_SOURCE_DIR}/ert/tests/empty.c)
enclave_link_libraries(go_ra_enc go_ra_enc_lib
                       ${CMAKE_CURRENT_BINARY_DIR}/gotest.a ertlibc)
add_dependencies(go_ra_enc goratestlib)
