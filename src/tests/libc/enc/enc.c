// same as OE's tests/libc/enc/enc.c, but with 16 instead of 4 NumTCS

#include <openenclave/bits/properties.h>

OE_SET_ENCLAVE_SGX(
    1,    /* ProductID */
    1,    /* SecurityVersion */
    true, /* Debug */
    512,  /* NumHeapPages */
    256,  /* NumStackPages */
    16);  /* NumTCS */

#undef OE_SET_ENCLAVE_SGX
#define OE_SET_ENCLAVE_SGX(...)

#include "../tests/libc/enc/enc.c"
