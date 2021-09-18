# Copyright (c) Open Enclave SDK contributors.
# Licensed under the MIT License.

if (UNIX)
  set(HOSTSRC "${DIR}/host/host_linux.c")
  set(EDL_FILE "${DIR}/linux/fs.edl")
else ()
  set(HOSTSRC "host_win32.c")
  set(EDL_FILE "../windows/fs.edl")
endif ()

add_custom_command(
  OUTPUT fs_u.h fs_u.c
  DEPENDS ${EDL_FILE}
  COMMAND
    openenclave::oeedger8r --untrusted ${EDL_FILE} --search-path
    ${PROJECT_SOURCE_DIR}/include ${DEFINE_OE_SGX} --search-path
    ${CMAKE_CURRENT_SOURCE_DIR})

add_executable(erttest_mmapfs_host ${HOSTSRC} fs_u.c
                                   ${DIR}/../tool/rmdir_linux.c)

target_include_directories(erttest_mmapfs_host
                           PRIVATE ${CMAKE_CURRENT_BINARY_DIR})

if (TEST_SGXFS)
  target_compile_definitions(erttest_mmapfs_host PRIVATE TEST_SGXFS=1)
  set(OESGXFSHOST oesgxfshost)
else ()
  set(OESGXFSHOST "")
endif ()

target_link_libraries(erttest_mmapfs_host ${OESGXFSHOST} openenclave::oehost
                      oe_includes)

set_source_files_properties(${DIR}/../tool/rmdir_linux.c
                            PROPERTIES COMPILE_FLAGS -Wno-unused-parameter)
