set(DIR ${OESRCDIR}/tests/libc)

add_subdirectory(host)
add_subdirectory(enc)

add_test(NAME tests/ert/libc COMMAND erttest_libc_host
                                     $<TARGET_FILE:erttest_libc_enc>)
