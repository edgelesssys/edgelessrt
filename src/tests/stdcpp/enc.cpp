#include <openenclave/internal/tests.h>
#include <locale>
#include "test_t.h"

using namespace std;

void test_ecall()
{
    // this tests __newlocale in src/ertlibc/syscall.cpp
    OE_TEST('a' == tolower('A', locale()));
    OE_TEST('A' == toupper('a', locale()));
}

OE_SET_ENCLAVE_SGX(
    1,    /* ProductID */
    1,    /* SecurityVersion */
    true, /* Debug */
    64,   /* NumHeapPages */
    64,   /* NumStackPages */
    1);   /* NumTCS */
