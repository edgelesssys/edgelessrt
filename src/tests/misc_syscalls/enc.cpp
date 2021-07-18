#include <dlfcn.h>
#include <fcntl.h>
#include <openenclave/ert.h>
#include <openenclave/internal/tests.h>
#include <sys/mount.h>
#include <unistd.h>
#include <array>
#include <cerrno>
#include <chrono>
#include <climits>
#include <ctime>
#include "test_t.h"

using namespace std;
using namespace ert;

extern "C" __attribute__((visibility("default"))) unsigned int edgeless()
{
    return 0xED6E1E55;
}

static void _test_dynlink()
{
    dlerror(); // clear error if any

    // open main module
    void* const handle = dlopen(nullptr, RTLD_LAZY);
    OE_TEST(handle);

    // bind now should also succeed
    OE_TEST(dlopen(nullptr, RTLD_NOW) == handle);

    // opening any other module should be redirected to main module
    OE_TEST(dlopen("foo", RTLD_LAZY) == handle);
    OE_TEST(dlopen("bar", RTLD_NOW) == handle);

    // test existing symbol
    const auto func =
        reinterpret_cast<unsigned int (*)()>(dlsym(handle, "edgeless"));
    OE_TEST(func);
    OE_TEST(func() == 0xED6E1E55);

    // test nonexistent symbol
    OE_TEST(!dlerror());
    OE_TEST(!dlsym(handle, "edgy"));
    OE_TEST(dlerror());
    OE_TEST(!dlerror());
}

static void _test_readlink()
{
    array<char, PATH_MAX> buf{};
    const auto myfs = "myfs";
    const Memfs memfs(myfs);
    OE_TEST(mount("/", "/", myfs, 0, nullptr) == 0);

    OE_TEST(readlink("foo", buf.data(), buf.size()) == -1);
    OE_TEST(errno == ENOENT);
    OE_TEST(readlink("/", buf.data(), buf.size()) == -1);
    OE_TEST(errno == EINVAL);

    OE_TEST(readlinkat(AT_FDCWD, "foo", buf.data(), buf.size()) == -1);
    OE_TEST(errno == ENOENT);
    OE_TEST(readlinkat(AT_FDCWD, "/", buf.data(), buf.size()) == -1);
    OE_TEST(errno == EINVAL);

    OE_TEST(readlinkat(0, "foo", buf.data(), buf.size()) == -1);
    OE_TEST(errno == EBADF);
    OE_TEST(readlinkat(0, "/", buf.data(), buf.size()) == -1);
    OE_TEST(errno == EBADF);

    OE_TEST(umount("/") == 0);
    OE_TEST(!buf.front());
}

static void _test_syconf()
{
    OE_TEST(sysconf(_SC_PAGESIZE) == OE_PAGE_SIZE);
    OE_TEST(sysconf(_SC_NPROCESSORS_CONF) >= 2);
}

static void _test_time()
{
    {
        timespec t1{};
        timespec t2{};
        OE_TEST(clock_gettime(CLOCK_MONOTONIC, &t1) == 0);
        OE_TEST(clock_gettime(CLOCK_MONOTONIC, &t2) == 0);
        OE_TEST(
            t1.tv_sec < t2.tv_sec ||
            (t1.tv_sec == t2.tv_sec && t1.tv_nsec <= t2.tv_nsec));
    }
    {
        const auto t1 = chrono::steady_clock::now();
        const auto t2 = chrono::steady_clock::now();
        OE_TEST(t1 <= t2);
    }
}

void test_ecall()
{
    _test_dynlink();
    _test_readlink();
    _test_syconf();
    _test_time();
}

OE_SET_ENCLAVE_SGX(
    1,    /* ProductID */
    1,    /* SecurityVersion */
    true, /* Debug */
    64,   /* NumHeapPages */
    64,   /* NumStackPages */
    1);   /* NumTCS */
