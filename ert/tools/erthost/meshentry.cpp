// Copyright (c) Edgeless Systems GmbH.
// Licensed under the MIT License.

#include <openenclave/ert.h>
#include <openenclave/internal/trace.h>
#include <sys/mount.h>
#include <iostream>

using namespace std;
using namespace ert;

static const auto _memfs_name = "edg_memfs";

int main(int argc, char* argv[], char* envp[]);
extern "C" void ert_meshentry_premain(int* argc, char*** argv);
extern "C" char** environ;

int emain()
{
    if (oe_load_module_host_epoll() != OE_OK ||
        oe_load_module_host_file_system() != OE_OK ||
        oe_load_module_host_socket_interface() != OE_OK)
    {
        OE_TRACE_FATAL("oe_load_module_host failed");
        return 1;
    }

    if (mount("/", "/edg/hostfs", OE_HOST_FILE_SYSTEM, 0, NULL) != 0)
    {
        OE_TRACE_FATAL("mount hostfs failed");
        return 1;
    }

    const Memfs memfs(_memfs_name);

    cout << "invoking premain\n";
    int argc = 0;
    char** argv = nullptr;
    ert_meshentry_premain(&argc, &argv);

    cout << "invoking main\n";
    return main(argc, argv, environ);
}
