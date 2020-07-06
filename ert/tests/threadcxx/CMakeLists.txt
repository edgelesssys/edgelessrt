set(DIR ${CMAKE_SOURCE_DIR}/tests/threadcxx)

add_subdirectory(host)

if (BUILD_ENCLAVES)
  add_subdirectory(enc)
endif ()

add_enclave_test(tests/ert/threadcxx erttest_threadcxx_host
                 erttest_threadcxx_enc)
