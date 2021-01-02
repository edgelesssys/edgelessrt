add_custom_command(
  OUTPUT test_u.c
  DEPENDS ../test.edl
  COMMAND openenclave::oeedger8r --untrusted
          ${CMAKE_CURRENT_SOURCE_DIR}/../test.edl ${DEFINE_OE_SGX})

add_executable(erttest_go_ra_host host.cpp test_u.c)
target_include_directories(erttest_go_ra_host
                           PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
target_link_libraries(erttest_go_ra_host openenclave::oehost oe_includes)

if (HAS_QUOTE_PROVIDER)
  target_compile_definitions(erttest_go_ra_host PRIVATE OE_LINK_SGX_DCAP_QL)
endif ()

add_subdirectory(enc)

add_test(NAME tests/ert/go_ra COMMAND erttest_go_ra_host
                                      $<TARGET_FILE:erttest_go_ra_enc>)
set_tests_properties(tests/ert/go_ra PROPERTIES SKIP_RETURN_CODE 2)
