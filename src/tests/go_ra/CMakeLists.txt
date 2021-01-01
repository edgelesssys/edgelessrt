add_custom_command(
  OUTPUT test_u.c
  DEPENDS ../test.edl edger8r
  COMMAND edger8r --untrusted ${CMAKE_CURRENT_SOURCE_DIR}/../test.edl
          --search-path ${PROJECT_SOURCE_DIR}/include ${DEFINE_OE_SGX})

add_executable(erttest_go_ra_host host.cpp test_u.c)
target_include_directories(erttest_go_ra_host
                           PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
target_link_libraries(erttest_go_ra_host oehost)

if (HAS_QUOTE_PROVIDER)
  target_compile_definitions(erttest_go_ra_host PRIVATE OE_LINK_SGX_DCAP_QL)
endif ()

if (BUILD_ENCLAVES)
  add_subdirectory(enc)
endif ()

add_enclave_test(tests/ert/go_ra erttest_go_ra_host erttest_go_ra_enc)
set_tests_properties(tests/ert/go_ra PROPERTIES SKIP_RETURN_CODE 2)
