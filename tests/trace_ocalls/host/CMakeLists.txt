add_custom_command(
  OUTPUT test_u.c
  DEPENDS ../test.edl edger8r
  COMMAND
    edger8r --untrusted ${CMAKE_CURRENT_SOURCE_DIR}/../test.edl --search-path
    ${PROJECT_SOURCE_DIR}/include --search-path ${PLATFORM_EDL_DIR})

add_executable(trace_ocalls_host host.cpp test_u.c)
target_include_directories(trace_ocalls_host
                           PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
target_link_libraries(trace_ocalls_host -rdynamic oehost)
