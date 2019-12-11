add_subdirectory(host)

if (BUILD_ENCLAVES)
  add_subdirectory(enc)

  # expect ocall list in stdout (similar to tests/print)
  add_test(
    NAME tests/trace_ocalls
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMAND
      sh -c
      "OE_TRACE_OCALLS=1 host/trace_ocalls_host ./enc/trace_ocalls_enc | grep -P '3\tocall_my_ocall'"
  )
else ()

  add_enclave_test(tests/trace_ocalls trace_ocalls_host trace_ocalls_enc)
endif ()
