set(DIR ${CMAKE_SOURCE_DIR}/tests/stdcxx)

add_subdirectory(host)

if (BUILD_ENCLAVES)
  add_subdirectory(enc)
endif ()

add_enclave_test(tests/ert/stdcxx erttest_stdcxx_host erttest_stdcxx_enc OE_OK)
