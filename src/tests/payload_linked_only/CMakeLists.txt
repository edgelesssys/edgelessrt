add_custom_command(
  OUTPUT test_t.c
  DEPENDS ../test.edl
  COMMAND openenclave::oeedger8r --trusted
          ${CMAKE_CURRENT_SOURCE_DIR}/../test.edl ${DEFINE_OE_SGX})

add_executable(erttest_payload_linked_only enc.cpp test_t.c)
target_include_directories(erttest_payload_linked_only
                           PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
target_link_libraries(erttest_payload_linked_only oeenclave ertlibc oe_includes
                      erttest_payload_linked_only_linked)

add_library(erttest_payload_linked_only_linked SHARED linked.c)
set_property(TARGET erttest_payload_linked_only_linked
             PROPERTY POSITION_INDEPENDENT_CODE ON)
target_link_libraries(erttest_payload_linked_only_linked -pie oe_includes)

add_custom_command(OUTPUT private.pem COMMAND openssl genrsa -out private.pem
                                              -3 3072)
add_custom_command(
  OUTPUT signed
  DEPENDS erttest_payload_linked_only erttest_payload_linked_only_linked
          private.pem enclave.conf
  COMMAND openenclave::oesign sign -e $<TARGET_FILE:erttest_payload_linked_only>
          -c ${CMAKE_CURRENT_SOURCE_DIR}/enclave.conf -k private.pem
  COMMAND touch signed)
add_custom_target(erttest_payload_linked_only_sign ALL DEPENDS signed)

add_test(NAME tests/ert/payload_linked_only
         COMMAND erttest_host erttest_payload_linked_only.signed)
