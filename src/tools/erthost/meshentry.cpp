// Copyright (c) Edgeless Systems GmbH.
// Licensed under the MIT License.

#include <openenclave/ert.h>
#include <openenclave/internal/trace.h>
#include <sys/mount.h>
#include <array>
#include <cassert>
#include <cstdlib>
#include <cstring>
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
        oe_load_module_host_resolver() != OE_OK ||
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

    cout << "[meshentry] invoking premain\n";
    int argc = 0;
    char** argv = nullptr;
    ert_meshentry_premain(&argc, &argv);

    ert_init_ttls(getenv("MARBLE_TTLS_CONFIG"));

    cout << "[meshentry] invoking main\n";
    return main(argc, argv, environ);
}

ert_args_t ert_get_args()
{
    //
    // Get env vars from the host.
    //

    ert_args_t args{};
    if (ert_get_args_ocall(&args) != OE_OK || args.envc < 0)
        abort();

    char** env = nullptr;
    ert_copy_strings_from_host_to_enclave(
        args.envp, &env, static_cast<size_t>(args.envc));

    assert(env);

    //
    // Keep all env vars that begin with EDG_
    //

    size_t edg_count = 0;

    for (size_t i = 0; env[i]; ++i)
    {
        if (memcmp(env[i], "EDG_", 4) == 0)
        {
            env[edg_count] = env[i];
            ++edg_count;
        }
    }

    env[edg_count] = nullptr;

    ert_args_t result{};
    result.envp = env;
    result.envc = static_cast<int>(edg_count);

    // We need a dummy value for argv[0] in case it is accessed before premain.
    static const array argv{"./marble"};
    result.argv = argv.data();
    result.argc = argv.size();

    return result;
}
