set(EDL_FILE ../threadcxx.edl)

add_custom_command(
  OUTPUT threadcxx_t.h threadcxx_t.c
  DEPENDS ${EDL_FILE} edger8r
  COMMAND
    edger8r --trusted ${EDL_FILE} --search-path ${PROJECT_SOURCE_DIR}/include
    ${DEFINE_OE_SGX} --search-path ${CMAKE_CURRENT_SOURCE_DIR})

add_enclave_library(erttest_threadcxx_lib OBJECT ${DIR}/enc/enc.cpp
                    ${DIR}/enc/cond_tests.cpp threadcxx_t.c)
enclave_include_directories(erttest_threadcxx_lib PRIVATE
                            ${CMAKE_CURRENT_BINARY_DIR})
enclave_link_libraries(erttest_threadcxx_lib PRIVATE oe_includes)
set_property(TARGET erttest_threadcxx_lib PROPERTY POSITION_INDEPENDENT_CODE ON)

add_enclave(TARGET erttest_threadcxx_enc SOURCES ../../empty.c)
enclave_link_libraries(erttest_threadcxx_enc erttest_threadcxx_lib ertlibc)
