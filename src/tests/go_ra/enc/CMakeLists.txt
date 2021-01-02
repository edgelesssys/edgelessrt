add_custom_command(
  OUTPUT test_t.c
  DEPENDS ../../test.edl
  COMMAND openenclave::oeedger8r --trusted
          ${CMAKE_CURRENT_SOURCE_DIR}/../../test.edl ${DEFINE_OE_SGX})

add_custom_command(
  OUTPUT gotest.a
  DEPENDS gotest.go
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
  COMMAND
    sh -c
    "CGO_CFLAGS='-I${OEBUILDDIR}/openenclave-install/include -I${CMAKE_CURRENT_SOURCE_DIR}/include' GOROOT='${CMAKE_SOURCE_DIR}/../3rdparty/go' '${CMAKE_SOURCE_DIR}/../3rdparty/go/bin/go' build -buildmode=c-archive -o '${CMAKE_CURRENT_BINARY_DIR}/gotest.a' gotest.go"
)
add_custom_target(goratestlib DEPENDS gotest.a)

add_enclave_library(erttest_go_ra_enc_lib OBJECT enc.cpp test_t.c)
enclave_include_directories(erttest_go_ra_enc_lib PRIVATE
                            ${CMAKE_CURRENT_BINARY_DIR})
enclave_link_libraries(erttest_go_ra_enc_lib PRIVATE oe_includes)
set_property(TARGET erttest_go_ra_enc_lib PROPERTY POSITION_INDEPENDENT_CODE ON)

add_enclave(TARGET erttest_go_ra_enc SOURCES ../../empty.c)
enclave_link_libraries(erttest_go_ra_enc erttest_go_ra_enc_lib
                       ${CMAKE_CURRENT_BINARY_DIR}/gotest.a ertlibc)
add_dependencies(erttest_go_ra_enc goratestlib)
