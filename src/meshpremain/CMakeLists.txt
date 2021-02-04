add_custom_command(
  OUTPUT libertmeshpremain.a
  DEPENDS main.go
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
  COMMAND
    sh -c
    "GOROOT='${CMAKE_SOURCE_DIR}/../3rdparty/go' '${CMAKE_SOURCE_DIR}/../3rdparty/go/bin/go' build -buildmode=c-archive -o ${CMAKE_CURRENT_BINARY_DIR}/libertmeshpremain.a"
)

add_custom_target(ertmeshpremain ALL DEPENDS libertmeshpremain.a)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/libertmeshpremain.a
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/openenclave/enclave)
