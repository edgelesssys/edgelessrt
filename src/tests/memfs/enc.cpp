// reuse existing test from OE

#include <sys/syscall.h>

// existing test mounts hostfs, we want our memfs named myfs
#include <openenclave/internal/syscall/device.h>
#undef OE_DEVICE_NAME_HOST_FILE_SYSTEM
#define OE_DEVICE_NAME_HOST_FILE_SYSTEM "myfs"

#define ERT_TEST_MEMFS

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
        test_common(fs, "/");
        test_pio(fs, "/");
        stream_file_system sfs;
        test_common(sfs, "/");
        test_pio(sfs, "/");
        OE_TEST(umount("/") == 0);
        test_dup_case1("");
    }

    // test mystikos workaround for pread beyond end of file
    {
        const Memfs memfs(myfs);
        OE_TEST(mount("/", "/", myfs, 0, nullptr) == 0);
        const int fd = open("foo", O_RDONLY | O_CREAT, 0);
        OE_TEST(fd >= 0);
        uint8_t b = 0;
        OE_TEST(pread(fd, &b, 1, 1) == 0);
        OE_TEST(close(fd) == 0);
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
