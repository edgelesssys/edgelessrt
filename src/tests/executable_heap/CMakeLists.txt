add_custom_command(
  OUTPUT test_t.c
  DEPENDS ../test.edl
  COMMAND openenclave::oeedger8r --trusted
          ${CMAKE_CURRENT_SOURCE_DIR}/../test.edl ${DEFINE_OE_SGX})

add_enclave_library(erttest_executable_heap_lib OBJECT enc.cpp test_t.c)
enclave_include_directories(erttest_executable_heap_lib PRIVATE
                            ${CMAKE_CURRENT_BINARY_DIR})
enclave_link_libraries(erttest_executable_heap_lib PRIVATE oe_includes)
set_property(TARGET erttest_executable_heap_lib
             PROPERTY POSITION_INDEPENDENT_CODE ON)

add_custom_command(OUTPUT private.pem COMMAND openssl genrsa -out private.pem
                                              -3 3072)

#
# build erttest_executable_heap
#

add_enclave(TARGET erttest_executable_heap SOURCES ../empty.c)
enclave_link_libraries(erttest_executable_heap erttest_executable_heap_lib
                       ertlibc)

add_custom_command(
  OUTPUT executable_signed
  DEPENDS erttest_executable_heap private.pem executable.conf
  COMMAND openenclave::oesign sign -e $<TARGET_FILE:erttest_executable_heap> -c
          ${CMAKE_CURRENT_SOURCE_DIR}/executable.conf -k private.pem
  COMMAND touch executable_signed)

#
# build erttest_nonexecutable_heap
#

add_enclave(TARGET erttest_nonexecutable_heap SOURCES ../empty.c)
enclave_link_libraries(erttest_nonexecutable_heap erttest_executable_heap_lib
                       ertlibc)

add_custom_command(
  OUTPUT nonexecutable_signed
  DEPENDS erttest_nonexecutable_heap private.pem nonexecutable.conf
  COMMAND openenclave::oesign sign -e $<TARGET_FILE:erttest_nonexecutable_heap>
          -c ${CMAKE_CURRENT_SOURCE_DIR}/nonexecutable.conf -k private.pem
  COMMAND touch nonexecutable_signed)

#
# define tests
#

add_custom_target(erttest_executable_heap_sign ALL DEPENDS executable_signed
                                                           nonexecutable_signed)

add_test(NAME tests/ert/executable_heap
         COMMAND erttest_host $<TARGET_FILE:erttest_executable_heap>.signed)

add_test(
  NAME tests/ert/nonexecutable_heap
  COMMAND
    sh -c
    "$<TARGET_FILE:erttest_host> $<TARGET_FILE:erttest_nonexecutable_heap>.signed ; test $? -eq 139"
)
