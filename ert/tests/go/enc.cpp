#include <openenclave/internal/tests.h>
#include <iostream>
#include "test_t.h"

#include <openenclave/internal/sgx/td.h>

using namespace std;

extern "C" const bool ert_enable_signals = true;
extern "C" const bool ert_enable_sigsegv = true;

extern "C" int gotest(bool simulate);

void test_ecall()
{
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
