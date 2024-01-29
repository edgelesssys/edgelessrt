#include <openenclave/bits/properties.h>
#include <stdio.h>

int main()
{
    puts("hello from payload");

    // test that ertlibc contains this symbol
    void __res_init();
    __res_init();

    return 2;
}

__attribute__((section(OE_INFO_SECTION_NAME))) volatile const char
    oe_enclave_properties_sgx[0x1000];
