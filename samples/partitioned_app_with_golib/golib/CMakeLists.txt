# Use ertgo to compile the Go code to a static library.
add_custom_target(hello ertgo build -buildmode=c-archive
                        ${CMAKE_CURRENT_SOURCE_DIR}/hello.go)
