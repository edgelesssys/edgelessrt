target_include_directories(
  oe_includes INTERFACE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>)

target_sources(oecore PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/common/ringbuffer.c
                              ${CMAKE_CURRENT_SOURCE_DIR}/enclave/args.c)
target_sources(oesyscall PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/enclave/eventfd.c)
target_sources(oehostfs
               PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/enclave/hostfsmmap.c)
target_sources(oehostsock
               PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/enclave/internalsock.c)
target_sources(oesign PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/oesign/eradump.cpp
                              ${CMAKE_CURRENT_SOURCE_DIR}/oesign/oesignerid.c)

add_subdirectory(libc)

add_enclave_library(ertlibunwind
                    ${CMAKE_SOURCE_DIR}/3rdparty/libcxx/__dso_handle.cpp)
enclave_link_libraries(ertlibunwind PRIVATE libunwind)
install_enclaves(
  TARGETS
  ertlibunwind
  EXPORT
  openenclave-targets
  ARCHIVE
  DESTINATION
  ${CMAKE_INSTALL_LIBDIR}/openenclave/enclave)
