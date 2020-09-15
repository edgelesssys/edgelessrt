add_custom_command(
  OUTPUT test_t.c
  DEPENDS ../test.edl edger8r
  COMMAND edger8r --trusted ${CMAKE_CURRENT_SOURCE_DIR}/../test.edl
          --search-path ${PROJECT_SOURCE_DIR}/include ${DEFINE_OE_SGX})

add_enclave(TARGET args_enc_default CXX SOURCES enc.cpp test_t.c)
target_include_directories(args_enc_default PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
