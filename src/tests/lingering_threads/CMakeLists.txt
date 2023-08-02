add_custom_command(
  OUTPUT test_t.c test_u.c
  DEPENDS ../test.edl
  COMMAND openenclave::oeedger8r ${CMAKE_CURRENT_SOURCE_DIR}/test.edl
          ${DEFINE_OE_SGX})

add_executable(erttest_lingering_threads_host host.cpp test_u.c)
target_include_directories(erttest_lingering_threads_host
                           PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
target_link_libraries(erttest_lingering_threads_host openenclave::oehost
                      oe_includes)

add_enclave_library(erttest_lingering_threads_lib OBJECT enc.cpp test_t.c)
enclave_compile_definitions(erttest_lingering_threads_lib PRIVATE
                            DEBUG_ENCLAVE=true)
enclave_include_directories(erttest_lingering_threads_lib PRIVATE
                            ${CMAKE_CURRENT_BINARY_DIR})
enclave_link_libraries(erttest_lingering_threads_lib PRIVATE oe_includes)
set_property(TARGET erttest_lingering_threads_lib
             PROPERTY POSITION_INDEPENDENT_CODE ON)

add_enclave(TARGET erttest_lingering_threads SOURCES ../empty.c)
enclave_link_libraries(erttest_lingering_threads erttest_lingering_threads_lib
                       ertlibc)

add_test(NAME tests/ert/lingering_threads COMMAND erttest_lingering_threads_host
                                                  erttest_lingering_threads)

#
# nondebug test
#
# This covers a bug in _invoke_create_thread_ecall() in src/ert/host/calls.cpp that only
# occurred in nondebug builds. (The stack stitching in OE's __oe_host_stack_bridge() function
# that is applied to debug enclaves seems to stop the stack unwinding and thus hides the bug.)
#

add_executable(erttest_lingering_threads_nondebug_host nondebug_host.cpp
                                                       test_u.c)
target_include_directories(erttest_lingering_threads_nondebug_host
                           PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
target_link_libraries(erttest_lingering_threads_nondebug_host
                      openenclave::oehost oe_includes)

add_enclave_library(erttest_lingering_threads_nondebug_lib OBJECT enc.cpp
                    test_t.c)
enclave_compile_definitions(erttest_lingering_threads_nondebug_lib PRIVATE
                            DEBUG_ENCLAVE=false)
enclave_include_directories(erttest_lingering_threads_nondebug_lib PRIVATE
                            ${CMAKE_CURRENT_BINARY_DIR})
enclave_link_libraries(erttest_lingering_threads_nondebug_lib PRIVATE
                       oe_includes)
set_property(TARGET erttest_lingering_threads_nondebug_lib
             PROPERTY POSITION_INDEPENDENT_CODE ON)

add_enclave(TARGET erttest_lingering_threads_nondebug SOURCES ../empty.c)
enclave_link_libraries(erttest_lingering_threads_nondebug
                       erttest_lingering_threads_nondebug_lib ertlibc)

add_test(tests/ert/lingering_threads_nondebug
         erttest_lingering_threads_nondebug_host
         erttest_lingering_threads_nondebug)
