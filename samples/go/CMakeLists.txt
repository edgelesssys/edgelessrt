# Copyright (c) Edgeless Systems GmbH.
# Licensed under the MIT License.

cmake_minimum_required(VERSION 3.11)
project(sample)
find_package(OpenEnclave CONFIG REQUIRED)

if (NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Debug)
endif ()

# Use ertgo to compile the Go code to a static library.
add_custom_target(
  hello ertgo build -buildmode=c-archive ${CMAKE_SOURCE_DIR}/app/hello.go
        ${CMAKE_SOURCE_DIR}/app/invokemain.go)

add_executable(enclave main.c)
add_dependencies(enclave hello)

target_link_libraries(enclave openenclave::oeenclave openenclave::ertdeventry
                      ${CMAKE_BINARY_DIR}/hello.a)

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
