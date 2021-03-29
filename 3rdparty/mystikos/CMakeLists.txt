add_library(
  mystikos_ramfs OBJECT
  myst.cpp mystikos/kernel/ramfs.c mystikos/utils/buf.c mystikos/utils/paths.c
  mystikos/utils/round.c mystikos/utils/strings.c)

target_include_directories(mystikos_ramfs PUBLIC mystikos/include)
set_property(TARGET mystikos_ramfs PROPERTY POSITION_INDEPENDENT_CODE ON)
