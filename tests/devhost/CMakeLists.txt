add_enclave(TARGET devhost_enc CXX SOURCES enc.cpp)
target_link_libraries(devhost_enc oeenclave oecore ertdeventry)
add_dependencies(devhost_enc ertdevhost)
add_test(
  NAME tests/devhost
  COMMAND
    sh -c
    "OE_DEVHOST_TEST=test ${CMAKE_BINARY_DIR}/tools/ertdevhost/ertdevhost ./devhost_enc arg1 arg2"
)
