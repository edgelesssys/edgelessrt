add_custom_command(
  OUTPUT test_u.c
  DEPENDS ../test.edl edger8r
  COMMAND
    edger8r --untrusted ${CMAKE_CURRENT_SOURCE_DIR}/../test.edl --search-path
    ${PROJECT_SOURCE_DIR}/include --search-path ${PLATFORM_EDL_DIR})

add_executable(lingering_threads_host host.cpp test_u.c)
target_include_directories(lingering_threads_host
                           PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
target_link_libraries(lingering_threads_host oehost)
