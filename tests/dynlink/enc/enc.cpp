#include <dlfcn.h>
#include <openenclave/internal/tests.h>
#include "test_t.h"

extern "C" __attribute__((visibility("default"))) unsigned int edgeless()
{
    return 0xED6E1E55;
}

void test_ecall()
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

OE_SET_ENCLAVE_SGX(
    1,    /* ProductID */
    1,    /* SecurityVersion */
    true, /* Debug */
    64,   /* NumHeapPages */
    64,   /* NumStackPages */
    1);   /* NumTCS */
