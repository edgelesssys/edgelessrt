#include <openenclave/internal/tests.h>
#include "test_t.h"

void test_ecall()
{
    OE_TEST(my_ocall() == OE_OK);
    OE_TEST(my_ocall() == OE_OK);
    OE_TEST(my_ocall() == OE_OK);
}

OE_SET_ENCLAVE_SGX(
    1,    /* ProductID */
    1,    /* SecurityVersion */
    true, /* Debug */
    64,   /* NumHeapPages */
    64,   /* NumStackPages */
    1);   /* NumTCS */
