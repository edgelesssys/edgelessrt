# Copyright (c) Edgeless Systems GmbH.
# Licensed under the MIT License.

cmake_minimum_required(VERSION 3.11)
find_package(OpenEnclave CONFIG REQUIRED)

if (NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Debug)
endif ()

add_subdirectory(app)

# CMake needs at least one source file even if it contains no code.
add_executable(enclave enc.c)

target_link_libraries(
  enclave
  -static
  -Wl,--eh-frame-hdr # needed for exception handling
  openenclave::oeenclave
  openenclave::oecore
  openenclave::ertdeventry
  hello
  stdc++
  openenclave::oelibcxx # required because it includes libunwind
)

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
