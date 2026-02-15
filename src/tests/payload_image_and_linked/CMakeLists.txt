add_custom_command(
  OUTPUT test_t.c
  DEPENDS ../test.edl
  COMMAND openenclave::oeedger8r --trusted
          ${CMAKE_CURRENT_SOURCE_DIR}/../test.edl ${DEFINE_OE_SGX})

add_executable(erttest_payload_image_and_linked enc.cpp test_t.c)
target_include_directories(erttest_payload_image_and_linked
                           PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
target_link_libraries(erttest_payload_image_and_linked oeenclave ertlibc
                      oe_includes erttest_payload_image_and_linked_linked)

add_library(erttest_payload_image_and_linked_linked SHARED linked.c)
set_property(TARGET erttest_payload_image_and_linked_linked
             PROPERTY POSITION_INDEPENDENT_CODE ON)
target_link_libraries(erttest_payload_image_and_linked_linked -pie oe_includes)

add_executable(erttest_payload_image_and_linked_payload payload.c)
set_property(TARGET erttest_payload_image_and_linked_payload
             PROPERTY POSITION_INDEPENDENT_CODE ON)
target_link_libraries(erttest_payload_image_and_linked_payload -pie oe_includes)

add_custom_command(OUTPUT private.pem COMMAND openssl genrsa -out private.pem
                                              -3 3072)
add_custom_command(
  OUTPUT signed
  DEPENDS erttest_payload_image_and_linked
          erttest_payload_image_and_linked_linked
          erttest_payload_image_and_linked_payload private.pem enclave.conf
  COMMAND
    openenclave::oesign sign -e $<TARGET_FILE:erttest_payload_image_and_linked>
    -c ${CMAKE_CURRENT_SOURCE_DIR}/enclave.conf -k private.pem --payload
    $<TARGET_FILE:erttest_payload_image_and_linked_payload>
  COMMAND touch signed)
add_custom_target(erttest_payload_image_and_linked_sign ALL DEPENDS signed)

add_test(
  NAME tests/ert/payload_image_and_linked
  COMMAND
    erttest_host
    erttest_payload_image_and_linked:erttest_payload_image_and_linked_payload)
