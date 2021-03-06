# Copyright (c) Edgeless Systems GmbH.
# Licensed under the MIT License.

cmake_minimum_required(VERSION 3.11)
project(sample)
find_package(OpenEnclave CONFIG REQUIRED)

if (NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Debug)
endif ()

add_subdirectory(../app app)

# Use the edger8r to generate C bindings from the EDL file.
add_custom_command(
  OUTPUT emain_t.c emain_u.c
  DEPENDS emain.edl
  COMMAND openenclave::oeedger8r ${CMAKE_SOURCE_DIR}/emain.edl --search-path
          ${OE_INCLUDEDIR} --search-path ${OE_INCLUDEDIR}/openenclave/edl/sgx)

add_executable(enclave emain.c emain_t.c)
target_include_directories(enclave PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
target_link_libraries(enclave openenclave::oeenclave hello
                      openenclave::oecryptombedtls)

# Generate key
add_custom_command(
  OUTPUT private.pem public.pem
  COMMAND openssl genrsa -out private.pem -3 3072
  COMMAND openssl rsa -in private.pem -pubout -out public.pem)

# Sign enclave
add_custom_command(
  OUTPUT enclave.signed
  DEPENDS enclave ../enclave.conf private.pem
  COMMAND openenclave::oesign sign -e $<TARGET_FILE:enclave> -c
          ${CMAKE_SOURCE_DIR}/../enclave.conf -k private.pem)

add_custom_target(sign ALL DEPENDS enclave.signed)

# Build the host
add_executable(helloworld_host host.cpp emain_u.c)
target_include_directories(helloworld_host PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
target_link_libraries(helloworld_host openenclave::oehost)
