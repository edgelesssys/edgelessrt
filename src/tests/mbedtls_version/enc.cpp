#include <openenclave/internal/tests.h>
#include <array>
#include <cstring>
#include "test_t.h"

extern "C" void mbedtls_version_get_string(char*);

using namespace std;

void test_ecall()
{
    array<char, 9> ver;
    mbedtls_version_get_string(ver.data());
    OE_TEST(strcmp(ver.data(), "2.28.10") == 0);
}

OE_SET_ENCLAVE_SGX(
    1,    /* ProductID */
    1,    /* SecurityVersion */
    true, /* Debug */
    64,   /* NumHeapPages */
    64,   /* NumStackPages */
    1);   /* NumTCS */
