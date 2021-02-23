add_custom_command(
  OUTPUT test_t.c
  DEPENDS ../test.edl
  COMMAND openenclave::oeedger8r --trusted
          ${CMAKE_CURRENT_SOURCE_DIR}/../test.edl ${DEFINE_OE_SGX})

add_executable(erttest_payload_data enc.cpp test_t.c)
target_include_directories(erttest_payload_data
                           PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
target_link_libraries(erttest_payload_data oeenclave ertlibc oe_includes)

add_executable(erttest_payload_data_payload payload.c)
set_property(TARGET erttest_payload_data_payload
             PROPERTY POSITION_INDEPENDENT_CODE ON)
target_link_libraries(erttest_payload_data_payload -pie oe_includes)
add_custom_command(
  TARGET erttest_payload_data_payload
  POST_BUILD
  COMMAND
    ${CMAKE_COMMAND} -E env GOROOT=${CMAKE_SOURCE_DIR}/../3rdparty/go
    ${CMAKE_SOURCE_DIR}/../3rdparty/go/bin/go run
    ${CMAKE_CURRENT_SOURCE_DIR}/patch.go erttest_payload_data_payload)

add_custom_command(OUTPUT private.pem COMMAND openssl genrsa -out private.pem
                                              -3 3072)
add_custom_command(
  OUTPUT signed
  DEPENDS erttest_payload_data erttest_payload_data_payload private.pem
          enclave.conf
  COMMAND
    openenclave::oesign sign -e $<TARGET_FILE:erttest_payload_data> -c
    ${CMAKE_CURRENT_SOURCE_DIR}/enclave.conf -k private.pem --payload
    $<TARGET_FILE:erttest_payload_data_payload>
  COMMAND touch signed)
add_custom_target(erttest_payload_data_sign ALL DEPENDS signed)

add_test(NAME tests/ert/payload_data
         COMMAND erttest_host erttest_payload_data:erttest_payload_data_payload)
