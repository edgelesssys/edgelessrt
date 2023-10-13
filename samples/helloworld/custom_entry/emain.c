// Copyright (c) Edgeless Systems GmbH.
// Licensed under the MIT License.

#include <openenclave/ert.h>

int main(int argc, char* argv[], char* envp[]);

int emain(void)
{
    const int argc = ert_get_argc();
    char** const argv = ert_get_argv();
    char** const envp = ert_get_envp();

    // Do additional initialization here if needed.

    return main(argc, argv, envp);
}

ert_args_t ert_get_args(void)
{
    // Initialize arguments and environment here. In this example we hardcode
    // the arguments.
    static const char* const argv[] = {"./hello", "arg1", "arg2"};
    return (ert_args_t){.argc = 3, .argv = argv};
}
