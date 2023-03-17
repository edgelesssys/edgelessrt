#include <openenclave/internal/tests.h>
#include <iostream>
#include "test_t.h"

using namespace std;

extern "C" int gotest();

void test_ecall()
{
    const int r = gotest();
    cout << "gotest() returned " << r << '\n';
    OE_TEST(r == 42);
}

OE_SET_ENCLAVE_SGX(
    1234, /* ProductID */
    2,    /* SecurityVersion */
    true, /* Debug */
    4096, /* NumHeapPages */
    64,   /* NumStackPages */
    16);  /* NumTCS */
