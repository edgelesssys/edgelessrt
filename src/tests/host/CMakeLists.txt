add_custom_command(
  OUTPUT test_u.c
  DEPENDS ../test.edl
  COMMAND openenclave::oeedger8r --untrusted
          ${CMAKE_CURRENT_SOURCE_DIR}/../test.edl ${DEFINE_OE_SGX})

add_executable(erttest_host host.cpp test_u.c)
target_include_directories(erttest_host PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
target_link_libraries(erttest_host openenclave::oehost oe_includes)
