set(DIR ${OESRCDIR}/tests/syscall/fs)
add_subdirectory(host)
add_subdirectory(enc)

add_test(NAME tests/ert/mmapfs
         COMMAND erttest_mmapfs_host $<TARGET_FILE:erttest_mmapfs_enc> ""
                 ${CMAKE_CURRENT_BINARY_DIR}/tmp)
