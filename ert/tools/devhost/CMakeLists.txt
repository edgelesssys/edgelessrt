add_executable(ertdevhost devhost.cpp emain_u.c)
target_include_directories(ertdevhost PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
target_link_libraries(ertdevhost -rdynamic oehost)
install(TARGETS ertdevhost DESTINATION ${CMAKE_INSTALL_BINDIR})

add_library(ertdeventry OBJECT emain.c emain_t.c)
target_include_directories(ertdeventry PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
target_link_libraries(
  ertdeventry
  PRIVATE oelibc
  INTERFACE ertlibc oehostepoll oehostfs oehostresolver oehostsock)
install(
  TARGETS ertdeventry
  EXPORT openenclave-targets
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/openenclave/enclave)

add_custom_command(
  OUTPUT emain_t.c emain_u.c
  DEPENDS emain.edl
  COMMAND
    edger8r ${CMAKE_CURRENT_SOURCE_DIR}/emain.edl --search-path
    ${CMAKE_SOURCE_DIR}/include --search-path
    ${CMAKE_SOURCE_DIR}/include/openenclave/edl/sgx)
