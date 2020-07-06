# Copyright (c) Open Enclave SDK contributors.
# Licensed under the MIT License.

# Include the libc tests wrappers:
include(../tests.cmake)

# Generate wrappers for libc tests.
foreach (test_file ${LIBC_TESTS})
  string(MAKE_C_IDENTIFIER ${test_file} test_name)
  list(APPEND test_dependencies ${CMAKE_CURRENT_BINARY_DIR}/${test_file})
  configure_file(${DIR}/enc/include.c.in ${test_file})

  string(APPEND forward_declarations
         "extern int ${test_name}(int argc, const char* argv[]);\n")
  string(APPEND test_table "{ \"${test_name}\", ${test_name} },\n")
endforeach ()

configure_file(${DIR}/enc/libc_tests.h.in libc_tests.h)

set(EDL_FILE ../libc.edl)

add_custom_command(
  OUTPUT libc_t.h libc_t.c
  DEPENDS ${EDL_FILE} edger8r
  COMMAND
    edger8r --trusted ${EDL_FILE} --search-path ${PROJECT_SOURCE_DIR}/include
    --search-path ${PLATFORM_EDL_DIR} --search-path ${CMAKE_CURRENT_SOURCE_DIR})

set(test_extras
    ${PROJECT_SOURCE_DIR}/3rdparty/musl/libc-test/src/common/vmfill.c
    ${PROJECT_SOURCE_DIR}/3rdparty/musl/libc-test/src/common/mtest.c
    ${PROJECT_SOURCE_DIR}/3rdparty/musl/libc-test/src/common/rand.c)

if (NOT OE_TRUSTZONE)
  # This file requires TLS relocations, which cannot be applied by OP-TEE's
  # ELF loader.
  list(
    APPEND test_extras
    ${PROJECT_SOURCE_DIR}/3rdparty/musl/libc-test/src/functional/tls_align_dso.c
  )
endif ()

add_enclave(
  TARGET
  erttest_libc_enc
  # Building the enclave by default when enabling LVI mitigation to
  # test linking against LVI-mitigated libraries.
  ADD_LVI_MITIGATION
  SOURCES
  ${DIR}/enc/enc.c
  ${test_dependencies}
  ${test_extras}
  ${LIBC_TESTS}
  libc_t.c)

enclave_compile_options(
  erttest_libc_enc
  PRIVATE
  -Wno-error
  -Wno-unknown-pragmas
  -Wno-unused-function
  -Wno-return-type
  -Wno-unused-variable
  -Wno-missing-braces
  -Wno-unused-parameter
  -Wno-float-conversion
  -Wno-sign-conversion
  -Wno-sign-compare
  -Wno-conversion
  -Wno-parentheses
  -Wno-unused-value
  -Wno-overflow)

if (OE_SGX)
  enclave_compile_options(erttest_libc_enc PRIVATE -Wno-macro-redefined
                          -Wno-literal-range -Wno-writable-strings)
elseif (OE_TRUSTZONE)
  enclave_compile_options(erttest_libc_enc PRIVATE -Wno-bool-compare)
endif ()

enclave_include_directories(
  erttest_libc_enc PRIVATE ${CMAKE_CURRENT_BINARY_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${PROJECT_SOURCE_DIR}/3rdparty/musl/libc-test/src/common)

enclave_link_libraries(erttest_libc_enc ertlibc oehostfs oehostsock)
