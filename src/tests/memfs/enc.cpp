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
    const auto myfs = "myfs";

    // run tests from OE
    for (int i = 0; i < 3; ++i) // ensure Memfs can be un-/reloaded
    {
        const Memfs memfs(myfs);
        OE_TEST(mount("/", "/", myfs, 0, nullptr) == 0);
        fd_file_system fs;
        test_common(fs, "");
        test_pio(fs, "");
        stream_file_system sfs;
        test_common(sfs, "");
        test_pio(fs, "");
        OE_TEST(umount("/") == 0);
    }

    // test different mount sources
    {
        const Memfs memfs(myfs);

        OE_TEST(mount("/", "/", myfs, 0, nullptr) == 0);
        OE_TEST(mkdir("/s0", 0) == 0);
        OE_TEST(mkdir("/s1", 0) == 0);
        OE_TEST(umount("/") == 0);

        OE_TEST(mount("/s0", "/t0", myfs, 0, nullptr) == 0);
        OE_TEST(mount("/s1", "/t1", myfs, 0, nullptr) == 0);
        OE_TEST(mount("/s0", "/t0a", myfs, 0, nullptr) == 0);

        FILE* f = fopen("/t0/foo", "w");
        OE_TEST(f);
        OE_TEST(fclose(f) == 0);

        // expect file exists on other mount point with same source
        f = fopen("/t0a/foo", "r");
        OE_TEST(f);
        OE_TEST(fclose(f) == 0);

        // expect file does not exist on a mount point with different source
        OE_TEST(!fopen("/t1/foo", "r"));
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
