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

    oe_enclave_t* enclave = nullptr;

    OE_TEST(
        oe_create_test_enclave(
            argv[1],
            OE_ENCLAVE_TYPE_AUTO,
            OE_ENCLAVE_FLAG_SIMULATE,
            nullptr,
            0,
            &enclave) == OE_OK);
    OE_TEST(test_ecall(enclave) == OE_OK);
    OE_TEST(oe_terminate_enclave(enclave) == OE_OK);

    cout << "=== passed all tests (" << argv[0] << ")\n";

    return EXIT_SUCCESS;
}
