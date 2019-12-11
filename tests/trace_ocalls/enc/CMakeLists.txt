add_custom_command(
  OUTPUT test_t.c
  DEPENDS ../test.edl edger8r
  COMMAND
    edger8r --trusted ${CMAKE_CURRENT_SOURCE_DIR}/../test.edl --search-path
    ${PROJECT_SOURCE_DIR}/include --search-path ${PLATFORM_EDL_DIR})

add_enclave(TARGET trace_ocalls_enc CXX SOURCES enc.cpp test_t.c)
target_include_directories(trace_ocalls_enc PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
