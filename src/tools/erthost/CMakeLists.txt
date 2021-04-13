add_executable(erthost erthost.cpp emain_u.c)
target_include_directories(erthost PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
target_link_libraries(erthost -rdynamic openenclave::oehost oe_includes)
install(TARGETS erthost DESTINATION ${CMAKE_INSTALL_BINDIR})

add_library(ertdeventry OBJECT deventry.c emain_t.c)
target_include_directories(ertdeventry PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
target_link_libraries(
  ertdeventry
  PRIVATE $<BUILD_INTERFACE:oe_includes>
  INTERFACE ertlibc openenclave::oehostepoll openenclave::oehostfs
            openenclave::oehostresolver openenclave::oehostsock)
set_property(TARGET ertdeventry PROPERTY POSITION_INDEPENDENT_CODE ON)

add_library(ertmeshentry OBJECT meshentry.cpp emain_t.c)
target_include_directories(ertmeshentry PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
target_link_libraries(
  ertmeshentry
  PRIVATE $<BUILD_INTERFACE:oe_includes>
  INTERFACE ertttls ertlibc openenclave::oehostepoll openenclave::oehostfs
            openenclave::oehostresolver openenclave::oehostsock)
set_property(TARGET ertmeshentry PROPERTY POSITION_INDEPENDENT_CODE ON)

add_library(ertcalls OBJECT emain_t.c)
target_link_libraries(
  ertcalls
  PRIVATE $<BUILD_INTERFACE:oe_includes>
  INTERFACE ertlibc)

add_library(ertttls ttls.cpp)
target_link_libraries(
  ertttls
  PRIVATE $<BUILD_INTERFACE:oe_includes> ttls
  INTERFACE openenclave::oehostsock openenclave::mbedtls openenclave::mbedx509
            openenclave::mbedcrypto)
set_property(TARGET ertttls PROPERTY POSITION_INDEPENDENT_CODE ON)

install(
  TARGETS ertdeventry ertmeshentry ertcalls ertttls ttls
  EXPORT openenclave-targets
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/openenclave/enclave)

add_custom_command(
  OUTPUT emain_t.c emain_u.c
  DEPENDS emain.edl
  COMMAND
    openenclave::oeedger8r ${CMAKE_CURRENT_SOURCE_DIR}/emain.edl --search-path
    ${OEBUILDDIR}/openenclave-install/include --search-path
    ${OEBUILDDIR}/openenclave-install/include/openenclave/edl/sgx --search-path
    ${PROJECT_SOURCE_DIR}/ert/include)
