add_subdirectory(host)

if (BUILD_ENCLAVES)
  add_subdirectory(enc)
endif ()

add_enclave_test(tests/lingering_threads lingering_threads_host
                 lingering_threads_enc)
