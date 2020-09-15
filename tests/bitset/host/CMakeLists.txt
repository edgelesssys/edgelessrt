add_custom_command(
  OUTPUT test_u.c
  DEPENDS ../test.edl edger8r
  COMMAND edger8r --untrusted ${CMAKE_CURRENT_SOURCE_DIR}/../test.edl
          --search-path ${PROJECT_SOURCE_DIR}/include ${DEFINE_OE_SGX})

add_executable(bitset_host host.cpp test_u.c)
target_include_directories(bitset_host PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
target_link_libraries(bitset_host oehost)
