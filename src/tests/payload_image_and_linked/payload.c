#include <openenclave/bits/properties.h>

static int x = 1;

int main()
{
    return x;
}

__attribute__((constructor)) void init_enclave()
{
    ++x;
}

__attribute__((section(OE_INFO_SECTION_NAME))) volatile const char
    oe_enclave_properties_sgx[0x1000];
