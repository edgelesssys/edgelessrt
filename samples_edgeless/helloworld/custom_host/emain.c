// Copyright (c) Edgeless Systems GmbH.
// Licensed under the MIT License.

#include <openenclave/enclave.h>
#include <openenclave/enclave_args.h>
#include "emain_t.h"

int main(int argc, char* argv[], char* envp[]);

void emain(void)
{
    const int argc = oe_get_argc();
    char** const argv = oe_get_argv();
    char** const envp = oe_get_envp();

    // Do additional initialization here if needed.

    main(argc, argv, envp);
}

oe_args_t oe_get_args(void)
{
    // Initialize arguments and environment here. In this example we hardcode
    // the arguments.
    static const char* const argv[] = {"./hello", "arg1", "arg2"};
    return (oe_args_t){.argc = 3, .argv = argv};
}
