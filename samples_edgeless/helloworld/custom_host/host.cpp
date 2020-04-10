// Copyright (c) Edgeless Systems GmbH.
// Licensed under the MIT License.

#include <openenclave/host.h>
#include <cstdlib>
#include <iostream>
#include "emain_u.h"

using namespace std;

int main(int argc, char* argv[])
{
    if (argc != 2)
    {
        cout << "Usage: " << argv[0]
             << " enclave_image_path\n"
                "Set OE_SIMULATION=1 for simulation mode.\n";
        return EXIT_FAILURE;
    }

    const char* const env_simulation = getenv("OE_SIMULATION");
    const bool simulate = env_simulation && *env_simulation == '1';

    oe_enclave_t* enclave = nullptr;

    // create the enclave
    if (oe_create_emain_enclave(
            argv[1],
            OE_ENCLAVE_TYPE_AUTO,
            OE_ENCLAVE_FLAG_DEBUG | (simulate ? OE_ENCLAVE_FLAG_SIMULATE : 0),
            nullptr,
            0,
            &enclave) != OE_OK ||
        !enclave)
    {
        cout << "oe_create_enclave failed\n";
        return EXIT_FAILURE;
    }

    // call the enclave entry function
    if (emain(enclave) != OE_OK)
        cout << "ecall failed\n";

    oe_terminate_enclave(enclave);
}
