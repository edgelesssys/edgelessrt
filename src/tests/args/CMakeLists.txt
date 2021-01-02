add_custom_command(
  OUTPUT test_u.c
  DEPENDS test.edl
  COMMAND openenclave::oeedger8r --untrusted
          ${CMAKE_CURRENT_SOURCE_DIR}/test.edl ${DEFINE_OE_SGX})

add_executable(erttest_args_host host.cpp test_u.c)
target_include_directories(erttest_args_host
                           PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
target_link_libraries(erttest_args_host openenclave::oehost oe_includes)

add_subdirectory(enc_default)
add_subdirectory(enc_custom)

add_test(NAME tests/ert/args
         COMMAND erttest_args_host $<TARGET_FILE:erttest_args_enc_default>
                 $<TARGET_FILE:erttest_args_enc_custom>)
