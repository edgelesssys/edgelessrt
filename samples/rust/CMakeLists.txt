# Copyright (c) Edgeless Systems GmbH.
# Licensed under the MIT License.

cmake_minimum_required(VERSION 3.11)
project(sample)
find_package(OpenEnclave CONFIG REQUIRED)

if (NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Debug)
endif ()
if (CMAKE_BUILD_TYPE STREQUAL Debug)
  set(CARGOTARGET debug)
else ()
  set(CARGOTARGET release)
  set(CARGOFLAGS --release)
endif ()

# Use cargo to compile the Rust app.
add_custom_target(
  app
  CARGO_TARGET_DIR=${CMAKE_BINARY_DIR}/target cargo build ${CARGOFLAGS}
  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/app)

add_executable(enclave main.c)
add_dependencies(enclave app)

target_link_libraries(enclave openenclave::oeenclave openenclave::ertdeventry
                      ${CMAKE_BINARY_DIR}/target/${CARGOTARGET}/libmthread.a)

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
