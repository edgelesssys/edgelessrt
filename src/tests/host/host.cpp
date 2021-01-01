#include <openenclave/host.h>
#include <openenclave/internal/tests.h>
#include <iostream>
#include <string>
#include "test_u.h"

using namespace std;

int main(int argc, char* argv[])
{
    if (argc < 2)
    {
        cout << "Usage: " << argv[0] << " ENCLAVE\n";
        return EXIT_FAILURE;
    }

    const char* const enclave_image = argv[1];
    const uint32_t flags = oe_get_create_flags();

    if ((flags & OE_ENCLAVE_FLAG_SIMULATE) && argc == 3 &&
        argv[2] == "--skip-simulate"s)
    {
        cout << "=== Skipped unsupported test in simulation mode ("
             << enclave_image << ")\n";
        return 2;
    }

    oe_enclave_t* enclave = nullptr;

    OE_TEST(
        oe_create_test_enclave(
            enclave_image, OE_ENCLAVE_TYPE_AUTO, flags, nullptr, 0, &enclave) ==
        OE_OK);
    OE_TEST(test_ecall(enclave) == OE_OK);
    OE_TEST(oe_terminate_enclave(enclave) == OE_OK);

    cout << "=== passed all tests (" << enclave_image << ")\n";

    return EXIT_SUCCESS;
}
