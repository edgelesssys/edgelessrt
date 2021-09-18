#include <openenclave/internal/syscall/device.h>
#undef OE_DEVICE_NAME_HOST_FILE_SYSTEM
#define OE_DEVICE_NAME_HOST_FILE_SYSTEM OE_HOST_FILE_SYSTEM_MMAP
#include "../tests/syscall/fs/enc/enc.cpp"
