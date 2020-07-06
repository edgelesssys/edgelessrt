add_subdirectory(ertlibc)
add_subdirectory(libc)

set(PLATFORM_EDL_DIR ${PROJECT_SOURCE_DIR}/include/openenclave/edl/sgx)

add_subdirectory(tests/host)
add_subdirectory(tests/libc)
add_subdirectory(tests/misc_syscalls)
add_subdirectory(tests/stdcxx)
add_subdirectory(tests/template)
add_subdirectory(tests/threadcpp)
add_subdirectory(tests/threadcxx)
