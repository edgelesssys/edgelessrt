# Copyright (c) Edgeless Systems GmbH.
# Licensed under the MIT License.

cmake_minimum_required(VERSION 3.11)
find_package(OpenEnclave CONFIG REQUIRED)

if (NOT GOLIB)
  message(
    FATAL_ERROR
      "You must set the path of the vault lib, e.g., -DGOLIB=../vault/main.a")
endif ()
get_filename_component(GOLIB ${GOLIB} ABSOLUTE BASE_DIR ${CMAKE_BINARY_DIR})

if (NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Debug)
endif ()

add_executable(enclave main.c)

target_link_libraries(enclave openenclave::oeenclave openenclave::oecore
                      openenclave::ertdeventry ${GOLIB})

# Generate key
add_custom_command(
  OUTPUT private.pem public.pem
  COMMAND openssl genrsa -out private.pem -3 3072
  COMMAND openssl rsa -in private.pem -pubout -out public.pem)

# Sign enclave
add_custom_command(
  OUTPUT enclave.signed
  DEPENDS enclave enclave.conf private.pem
  COMMAND openenclave::oesign sign -e $<TARGET_FILE:enclave> -c
          ${CMAKE_SOURCE_DIR}/enclave.conf -k private.pem)

add_custom_target(sign ALL DEPENDS enclave.signed)
