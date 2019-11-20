#include <openenclave/host.h>
#include <openenclave/internal/tests.h>
#include <cstring>
#include <iostream>
#include "test_u.h"

using namespace std;

int main(int argc, char* argv[])
{
    if (argc != 3)
    {
        cout << "Usage: " << argv[0]
             << " ARGS_DEFAULT_ENCLAVE ARGS_CUSTOM_ENCLAVE\n";
        return EXIT_FAILURE;
    }

    const uint32_t flags = oe_get_create_flags();
    oe_enclave_t* enclave = nullptr;

    OE_TEST(
        oe_create_test_enclave(
            argv[1], OE_ENCLAVE_TYPE_AUTO, flags, nullptr, 0, &enclave) ==
        OE_OK);
    OE_TEST(test_ecall(enclave) == OE_OK);
    OE_TEST(oe_terminate_enclave(enclave) == OE_OK);

    OE_TEST(
        oe_create_test_enclave(
            argv[2], OE_ENCLAVE_TYPE_AUTO, flags, nullptr, 0, &enclave) ==
        OE_OK);
    OE_TEST(test_ecall(enclave) == OE_OK);
    OE_TEST(oe_terminate_enclave(enclave) == OE_OK);

    cout << "=== passed all tests (" << argv[0] << ")\n";

    return EXIT_SUCCESS;
}

void get_str_ocall(char* s)
{
    strcpy(s, "foo");
}
