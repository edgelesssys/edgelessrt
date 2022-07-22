# Use the edger8r to generate C bindings from the EDL file.
add_custom_command(
  OUTPUT helloworld_t.h helloworld_t.c helloworld_args.h
  DEPENDS ${CMAKE_SOURCE_DIR}/helloworld.edl
  COMMAND
    openenclave::oeedger8r --trusted ${CMAKE_SOURCE_DIR}/helloworld.edl
    --search-path ${OE_INCLUDEDIR} --search-path
    ${OE_INCLUDEDIR}/openenclave/edl/sgx)

add_executable(enclave enc.c ${CMAKE_CURRENT_BINARY_DIR}/helloworld_t.c)
target_include_directories(enclave PRIVATE ${CMAKE_CURRENT_BINARY_DIR})

# link the Go library and ertlibc
target_link_libraries(enclave openenclave::oeenclave openenclave::ertlibc
                      ${CMAKE_BINARY_DIR}/golib/hello.a)
add_dependencies(enclave hello)
