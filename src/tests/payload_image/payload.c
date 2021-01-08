#include <openenclave/bits/properties.h>
#include <stdio.h>

int main()
{
    puts("hello from payload");
    return 2;
}

// This will add the .oeinfo section to the payload executable. oesign will fill
// in valid values.
OE_SET_ENCLAVE_SGX(0, 0, false, 0, 0, 0);
