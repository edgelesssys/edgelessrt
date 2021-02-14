#include <openenclave/bits/properties.h>

int main()
{
}

__attribute__((section(OE_INFO_SECTION_NAME))) volatile const char
    oe_enclave_properties_sgx[0x1000];
