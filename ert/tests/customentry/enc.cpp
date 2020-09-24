#include <openenclave/ert.h>
#include <openenclave/internal/tests.h>

int emain()
{
    ert_args_t args{};
    OE_TEST(ert_get_args_ocall(&args) == OE_OK);
    return 0;
}

OE_SET_ENCLAVE_SGX(
    1,    /* ProductID */
    1,    /* SecurityVersion */
    true, /* Debug */
    64,   /* NumHeapPages */
    64,   /* NumStackPages */
    1);   /* NumTCS */
