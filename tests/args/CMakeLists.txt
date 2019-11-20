add_subdirectory(host)

if (BUILD_ENCLAVES)
  add_subdirectory(enc_default)
  add_subdirectory(enc_custom)
endif ()

add_enclave_test(tests/args args_host args_enc_default
                 $<TARGET_FILE:args_enc_custom>)
