// reuse existing test from OE
// existing test links against musl which has strlc??, but we link against glibc
#define strlcat strncat
#define strlcpy strncpy
#include "../tests/syscall/fs/enc/enc.cpp"
#undef strlcat
#undef strlcpy

#include <openenclave/ert.h>
#include "test_t.h"

using namespace ert;

void test_ecall()
{
    for (int i = 0; i < 3; ++i) // ensure Memfs can be un-/reloaded
    {
        const Memfs memfs("myfs");
        OE_TEST(mount("/", "/", "myfs", 0, nullptr) == 0);
        fd_file_system fs;
        test_common(fs, "");
        test_pio(fs, "");
        stream_file_system sfs;
        test_common(sfs, "");
        test_pio(fs, "");
        OE_TEST(umount("/") == 0);
    }
}

// existing test links against these, but the parts we use do not call them
extern "C" int oe_cpio_pack(const char*, const char*)
{
    OE_TEST(false);
}
extern "C" int oe_cpio_unpack(const char*, const char*)
{
    OE_TEST(false);
}
extern "C" int oe_cmp(const char*, const char*)
{
    OE_TEST(false);
}
