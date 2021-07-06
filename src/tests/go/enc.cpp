#include <openenclave/ert.h>
#include <openenclave/internal/sgx/td.h>
#include <openenclave/internal/tests.h>
#include <sys/mount.h>
#include <iostream>
#include "test_t.h"

using namespace std;
using namespace ert;

extern "C" const bool ert_enable_signals = true;
extern "C" const bool ert_enable_sigsegv = true;

extern "C" int gotest(bool simulate);

void test_ecall()
{
    const auto myfs = "myfs";
    const Memfs memfs(myfs);
    OE_TEST(mount("/", "/", myfs, 0, nullptr) == 0);

    const int r = gotest(oe_sgx_get_td()->simulate);
    cout << "gotest() returned " << r << '\n';
    OE_TEST(r == 42);
}

OE_SET_ENCLAVE_SGX(
    1,      /* ProductID */
    1,      /* SecurityVersion */
    true,   /* Debug */
    131072, /* NumHeapPages */
    64,     /* NumStackPages */
    16);    /* NumTCS */
