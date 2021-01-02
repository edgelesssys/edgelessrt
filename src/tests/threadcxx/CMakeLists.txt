set(DIR ${OESRCDIR}/tests/threadcxx)

add_subdirectory(host)
add_subdirectory(enc)

add_test(NAME tests/ert/threadcxx COMMAND erttest_threadcxx_host
                                          $<TARGET_FILE:erttest_threadcxx_enc>)
