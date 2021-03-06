set(EDL_FILE ../stdcxx.edl)

add_custom_command(
  OUTPUT stdcxx_t.h stdcxx_t.c
  DEPENDS ${EDL_FILE}
  COMMAND
    openenclave::oeedger8r --trusted ${EDL_FILE} --search-path
    ${PROJECT_SOURCE_DIR}/include ${DEFINE_OE_SGX} --search-path
    ${CMAKE_CURRENT_SOURCE_DIR})

add_enclave_library(erttest_stdcxx_lib OBJECT enc.cpp stdcxx_t.c)
enclave_include_directories(erttest_stdcxx_lib PRIVATE
                            ${CMAKE_CURRENT_BINARY_DIR})
enclave_link_libraries(erttest_stdcxx_lib PRIVATE oe_includes)
set_property(TARGET erttest_stdcxx_lib PROPERTY POSITION_INDEPENDENT_CODE ON)

add_enclave(TARGET erttest_stdcxx_enc SOURCES ${DIR}/enc/f.cpp)
enclave_link_libraries(erttest_stdcxx_enc erttest_stdcxx_lib ertlibc)
