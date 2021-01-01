add_custom_command(
  OUTPUT test_u.c
  DEPENDS test.edl edger8r
  COMMAND edger8r --untrusted ${CMAKE_CURRENT_SOURCE_DIR}/test.edl
          --search-path ${PROJECT_SOURCE_DIR}/include ${DEFINE_OE_SGX})

add_executable(erttest_args_host host.cpp test_u.c)
target_include_directories(erttest_args_host
                           PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
target_link_libraries(erttest_args_host oehost)

if (BUILD_ENCLAVES)
  add_subdirectory(enc_default)
  add_subdirectory(enc_custom)
endif ()

add_enclave_test(tests/ert/args erttest_args_host erttest_args_enc_default
                 $<TARGET_FILE:erttest_args_enc_custom>)
