add_custom_command(
  OUTPUT test_t.c
  DEPENDS ../test.edl
  COMMAND openenclave::oeedger8r --trusted
          ${CMAKE_CURRENT_SOURCE_DIR}/../test.edl ${DEFINE_OE_SGX})

# Both main image and payload image use the local-exec TLS model. This means that TLS memory
# would overlap. To fix this, we reserve space in the main image that we won't touch.
# The reserved space must be at the end of the TLS block, so the lib must be linked last.
add_library(reserved_tls reserved_tls.c)
add_library(reserved_tls_last INTERFACE)
target_link_libraries(reserved_tls_last INTERFACE reserved_tls)

add_executable(erttest_payload_tls enc.cpp test_t.c)
target_include_directories(erttest_payload_tls
                           PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
target_link_libraries(erttest_payload_tls oeenclave ertlibc oe_includes
                      reserved_tls_last)

add_executable(erttest_payload_tls_payload payload.c)
set_property(TARGET erttest_payload_tls_payload
             PROPERTY POSITION_INDEPENDENT_CODE ON)
target_link_libraries(erttest_payload_tls_payload -pie oe_includes)

add_custom_command(OUTPUT private.pem COMMAND openssl genrsa -out private.pem
                                              -3 3072)
add_custom_command(
  OUTPUT signed
  DEPENDS erttest_payload_tls erttest_payload_tls_payload private.pem
          enclave.conf
  COMMAND
    openenclave::oesign sign -e $<TARGET_FILE:erttest_payload_tls> -c
    ${CMAKE_CURRENT_SOURCE_DIR}/enclave.conf -k private.pem --payload
    $<TARGET_FILE:erttest_payload_tls_payload>
  COMMAND touch signed)
add_custom_target(erttest_payload_tls_sign ALL DEPENDS signed)

add_test(NAME tests/ert/payload_tls
         COMMAND erttest_host erttest_payload_tls:erttest_payload_tls_payload)
