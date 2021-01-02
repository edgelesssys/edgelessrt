set(DIR ${OESRCDIR}/tests/stdcxx)

add_subdirectory(host)
add_subdirectory(enc)

add_test(NAME tests/ert/stdcxx COMMAND erttest_stdcxx_host
                                       $<TARGET_FILE:erttest_stdcxx_enc> OE_OK)
