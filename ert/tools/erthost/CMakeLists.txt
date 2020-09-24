add_executable(erthost erthost.cpp emain_u.c)
target_include_directories(erthost PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
target_link_libraries(erthost -rdynamic oehost)
install(TARGETS erthost DESTINATION ${CMAKE_INSTALL_BINDIR})

add_library(ertdeventry OBJECT deventry.c emain_t.c)
target_include_directories(ertdeventry PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
target_link_libraries(
  ertdeventry
  PRIVATE oelibc
  INTERFACE ertlibc oehostepoll oehostfs oehostresolver oehostsock)
install(
  TARGETS ertdeventry
  EXPORT openenclave-targets
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/openenclave/enclave)

add_library(ertmeshentry OBJECT meshentry.cpp emain_t.c)
target_include_directories(ertmeshentry PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
target_link_libraries(
  ertmeshentry
  PRIVATE oe_includes
  INTERFACE ertlibc oehostepoll oehostfs oehostsock oecustomfs)
set_property(TARGET ertmeshentry PROPERTY POSITION_INDEPENDENT_CODE ON)
install(
  TARGETS ertmeshentry
  EXPORT openenclave-targets
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/openenclave/enclave)

add_custom_command(
  OUTPUT emain_t.c emain_u.c
  DEPENDS emain.edl
  COMMAND
    edger8r ${CMAKE_CURRENT_SOURCE_DIR}/emain.edl --search-path
    ${CMAKE_SOURCE_DIR}/include --search-path
    ${CMAKE_SOURCE_DIR}/include/openenclave/edl/sgx)
