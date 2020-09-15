add_custom_command(
  OUTPUT test_u.c
  DEPENDS ../test.edl edger8r
  COMMAND edger8r --untrusted ${CMAKE_CURRENT_SOURCE_DIR}/../test.edl
          --search-path ${PROJECT_SOURCE_DIR}/include ${DEFINE_OE_SGX})

add_executable(go_ra_host host.cpp test_u.c)
target_include_directories(go_ra_host PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
target_link_libraries(go_ra_host oehost)

if (HAS_QUOTE_PROVIDER)
  target_compile_definitions(go_ra_host PRIVATE OE_LINK_SGX_DCAP_QL)
endif ()
