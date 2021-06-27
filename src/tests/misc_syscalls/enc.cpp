#include <dlfcn.h>
#include <openenclave/internal/tests.h>
#include <unistd.h>
#include <chrono>
#include <ctime>
#include "test_t.h"

using namespace std;

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
