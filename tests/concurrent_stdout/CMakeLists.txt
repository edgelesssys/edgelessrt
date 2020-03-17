add_subdirectory(host)

if (BUILD_ENCLAVES)
  add_subdirectory(enc)
endif ()

add_enclave_test(tests/concurrent_stdout concurrent_stdout_host
                 concurrent_stdout_enc)
