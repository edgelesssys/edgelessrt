#include <openenclave/host.h>
#include <openenclave/internal/tests.h>
#include <iostream>
#include "test_u.h"

using namespace std;

int main(int argc, char* argv[])
{
    if (argc != 2)
    {
        cout << "Usage: " << argv[0] << " ENCLAVE\n";
        return EXIT_FAILURE;
    }

    const char* const enclave_image = argv[1];
    const uint32_t flags = oe_get_create_flags();
    oe_enclave_t* enclave = nullptr;

    for (int i = 0; i < 100; ++i)
    {
        OE_TEST(
            oe_create_test_enclave(
                enclave_image,
                OE_ENCLAVE_TYPE_AUTO,
                flags,
                nullptr,
                0,
                &enclave) == OE_OK);
        OE_TEST(test_ecall(enclave) == OE_OK);
        OE_TEST(oe_terminate_enclave(enclave) == OE_OK);
    }

    for (int i = 0; i < 100; ++i)
    {
        OE_TEST(
            oe_create_test_enclave(
                enclave_image,
                OE_ENCLAVE_TYPE_AUTO,
                flags,
                nullptr,
                0,
                &enclave) == OE_OK);
        OE_TEST(test_child_of_child_ecall(enclave) == OE_OK);
        OE_TEST(oe_terminate_enclave(enclave) == OE_OK);
    }

    cout << "=== passed all tests (" << enclave_image << ")\n";

    return EXIT_SUCCESS;
}
