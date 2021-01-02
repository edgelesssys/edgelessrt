add_custom_command(
  OUTPUT erttest_meshentry_premainmock.a
  DEPENDS main.go premain/premain.go
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
  COMMAND
    sh -c
    "GOROOT='${CMAKE_SOURCE_DIR}/../3rdparty/go' '${CMAKE_SOURCE_DIR}/../3rdparty/go/bin/go' build -buildmode=c-archive -o '${CMAKE_CURRENT_BINARY_DIR}/erttest_meshentry_premainmock.a'"
)
add_custom_target(erttest_meshentry_premainmock_gobuild
                  DEPENDS erttest_meshentry_premainmock.a)

add_library(erttest_meshentry_premainmock STATIC IMPORTED GLOBAL)
set_property(
  TARGET erttest_meshentry_premainmock
  PROPERTY IMPORTED_LOCATION
           ${CMAKE_CURRENT_BINARY_DIR}/erttest_meshentry_premainmock.a)
add_dependencies(erttest_meshentry_premainmock
                 erttest_meshentry_premainmock_gobuild)
