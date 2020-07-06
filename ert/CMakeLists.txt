add_subdirectory(ertlibc)
add_subdirectory(libc)

set(PLATFORM_EDL_DIR ${PROJECT_SOURCE_DIR}/include/openenclave/edl/sgx)

add_subdirectory(tests/libc)
