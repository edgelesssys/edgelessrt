set(DIR ${CMAKE_SOURCE_DIR}/tests/libc)

add_subdirectory(host)

if (BUILD_ENCLAVES)
  add_subdirectory(enc)
endif ()

add_enclave_test(tests/ert/libc erttest_libc_host erttest_libc_enc)
