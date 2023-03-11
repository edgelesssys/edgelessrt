#include <dlfcn.h>
#include <fcntl.h>
#include <openenclave/ert.h>
#include <openenclave/internal/tests.h>
#include <sys/mount.h>
#include <sys/random.h>
#include <sys/resource.h>
#include <sys/statfs.h>
#include <unistd.h>
#include <algorithm>
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

static void _test_getrandom()
{
    for (const int flags :
         {0, GRND_NONBLOCK, GRND_RANDOM, GRND_NONBLOCK | GRND_RANDOM})
    {
        array<uint8_t, 32> buf;
        for (size_t len = 1; len < buf.size(); ++len)
        {
            buf.fill(0);

            do
            {
                OE_TEST(
                    getrandom(
                        buf.data(), len, static_cast<unsigned int>(flags)) ==
                    static_cast<ssize_t>(len));

                // Test that not more than len bytes were written.
                OE_TEST(all_of(buf.cbegin() + len, buf.cend(), [](uint8_t x) {
                    return !x;
                }));

                // Test that last byte will eventually become nonzero.
            } while (len && !buf[len - 1]);
        }
    }
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

static void _test_rlimit()
{
    // test get for all resources
    for (int i = 0; i < RLIM_NLIMITS; ++i)
    {
        rlimit lim{};
        OE_TEST(getrlimit(i, &lim) == 0);
        OE_TEST(lim.rlim_cur <= lim.rlim_max);
    }

    const auto expect = [](int resource, rlim_t cur, rlim_t max) {
        rlimit lim{};
        OE_TEST(getrlimit(resource, &lim) == 0);
        OE_TEST(lim.rlim_cur == cur);
        OE_TEST(lim.rlim_max == max);
    };

    // test selected values
    {
        expect(RLIMIT_CPU, RLIM_INFINITY, RLIM_INFINITY);
        expect(RLIMIT_STACK, 64 * 4096, 64 * 4096);
        expect(RLIMIT_NPROC, 5 - 2, 5 - 2);
    }

    // test set
    {
        rlimit lim{};

        // cur > max is invalid
        lim.rlim_cur = 1;
        lim.rlim_max = 0;
        OE_TEST(setrlimit(RLIMIT_CPU, &lim) == -1);
        OE_TEST(errno == EINVAL);
        expect(RLIMIT_CPU, RLIM_INFINITY, RLIM_INFINITY);

        // reduce max
        lim.rlim_cur = 6;
        lim.rlim_max = 8;
        OE_TEST(setrlimit(RLIMIT_CPU, &lim) == 0);
        expect(RLIMIT_CPU, 6, 8);

        // can't increase max
        lim.rlim_cur = 6;
        lim.rlim_max = 9;
        OE_TEST(setrlimit(RLIMIT_CPU, &lim) == -1);
        OE_TEST(errno == EPERM);
        expect(RLIMIT_CPU, 6, 8);

        // increase cur
        lim.rlim_cur = 8;
        lim.rlim_max = 8;
        OE_TEST(setrlimit(RLIMIT_CPU, &lim) == 0);
        expect(RLIMIT_CPU, 8, 8);
    }

    // test invalid resource
    {
        rlimit lim{};
        OE_TEST(getrlimit(-1, &lim) == -1);
        OE_TEST(errno == EINVAL);
        OE_TEST(getrlimit(RLIM_NLIMITS, &lim) == -1);
        OE_TEST(errno == EINVAL);
        OE_TEST(setrlimit(-1, &lim) == -1);
        OE_TEST(errno == EINVAL);
        OE_TEST(setrlimit(RLIM_NLIMITS, &lim) == -1);
        OE_TEST(errno == EINVAL);
    }
}

static void _test_statfs()
{
    const size_t expected_blocks = 20000000;
    struct statfs buf = {};

    // statfs

    OE_TEST(statfs("/", &buf) == 0);
    OE_TEST(buf.f_blocks == expected_blocks);

    OE_TEST(statfs("/edg/hostfs/", &buf) == 0);
    OE_TEST(buf.f_files > 0);

    // statfs errors

    errno = 0;
    OE_TEST(statfs("", &buf) == -1);
    OE_TEST(errno == ENOENT);

    errno = 0;
    OE_TEST(statfs("/edg/hostfs/doesnotexist", &buf) == -1);
    OE_TEST(errno == ENOENT);

    // fstatfs

    OE_TEST(fstatfs(0, &buf) == 0);
    OE_TEST(buf.f_blocks == expected_blocks);

    OE_TEST(fstatfs(3, &buf) == 0);
    OE_TEST(buf.f_blocks == expected_blocks);

    // fstatfs errors

    errno = 0;
    OE_TEST(fstatfs(-1, &buf) == -1);
    OE_TEST(errno == EBADF);

    errno = 0;
    OE_TEST(fstatfs(-2, &buf) == -1);
    OE_TEST(errno == EBADF);
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
        OE_TEST(t1.tv_sec > 0);
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
    _test_getrandom();
    _test_readlink();
    _test_rlimit();
    _test_statfs();
    _test_syconf();
    _test_time();
}

OE_SET_ENCLAVE_SGX(
    1,    /* ProductID */
    1,    /* SecurityVersion */
    true, /* Debug */
    64,   /* NumHeapPages */
    64,   /* NumStackPages */
    5);   /* NumTCS */
