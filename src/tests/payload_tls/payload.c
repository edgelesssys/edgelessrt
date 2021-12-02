#include <openenclave/bits/properties.h>

static __thread uint64_t x = 10;
static __thread uint64_t y = 20;

int main()
{
    return (++x) + (++y);
}

__attribute__((section(OE_INFO_SECTION_NAME))) volatile const char
    oe_enclave_properties_sgx[0x1000];
