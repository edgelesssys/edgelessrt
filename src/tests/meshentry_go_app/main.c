#include <openenclave/internal/properties.h>

void invokemain(void);

int main()
{
    invokemain();
    return 0;
}

OE_SET_ENCLAVE_SGX(
    1,    /* ProductID */
    1,    /* SecurityVersion */
    true, /* Debug */
    4096, /* NumHeapPages */
    64,   /* NumStackPages */
    16);  /* NumTCS */
