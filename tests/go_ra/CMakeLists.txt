add_subdirectory(host)

if (BUILD_ENCLAVES)
  add_subdirectory(enc)
endif ()

add_enclave_test(tests/go_ra go_ra_host go_ra_enc)
set_tests_properties(tests/go_ra PROPERTIES SKIP_RETURN_CODE 2)
