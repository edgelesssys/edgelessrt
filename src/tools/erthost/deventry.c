// Copyright (c) Edgeless Systems GmbH.
// Licensed under the MIT License.

#include <openenclave/corelibc/stdio.h>
#include <openenclave/ert.h>
#include <openenclave/internal/trace.h>
#include <stdlib.h>
#include <sys/mount.h>
#include <unistd.h>

int main(int argc, char* argv[], char* envp[]);

int emain(void)
{
    oe_printf("[deventry] running in development mode\n");

    const int argc = ert_get_argc();
    char** const argv = ert_get_argv();
    char** const envp = ert_get_envp();

    if (oe_load_module_host_epoll() != OE_OK ||
        oe_load_module_host_file_system() != OE_OK ||
        oe_load_module_host_resolver() != OE_OK ||
        oe_load_module_host_socket_interface() != OE_OK)
    {
        OE_TRACE_FATAL("oe_load_module_host failed");
        return 1;
    }

    if (mount("/", "/", OE_HOST_FILE_SYSTEM, 0, NULL) != 0)
    {
        OE_TRACE_FATAL("mount OE_HOST_FILE_SYSTEM failed");
        return 1;
    }

    const char* const cwd = getenv("EDG_CWD");
    if (!cwd || !*cwd || chdir(cwd) != 0)
        OE_TRACE_ERROR("cannot set cwd");

    oe_printf("[deventry] invoking main\n");
    return main(argc, argv, envp);
}

ert_args_t ert_get_args(void)
{
    // Get args from host. This is just for testing and not secure:
    // - The host controls commandline arguments and environment variables.
    // These are provided to the application without any checks.
    // - The ocall receives raw pointers. It will not be checked if they really
    // point outside the enclave memory.
    ert_args_t args = {};
    if (ert_get_args_ocall(&args) != OE_OK)
        oe_abort();
    return args;
}
