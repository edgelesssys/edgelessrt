// Copyright (c) Edgeless Systems GmbH.
// Licensed under the MIT License.

#include <openenclave/ert_args.h>
#include <openenclave/host.h>
#include <openenclave/internal/final_action.h>
#include <openenclave/internal/trace.h>
#include <semaphore.h>
#include <cassert>
#include <cerrno>
#include <csignal>
#include <cstdlib>
#include <exception>
#include <iostream>
#include <stdexcept>
#include <system_error>
#include "../../host/enclave_thread_manager.h"
#include "emain_u.h"

using namespace std;
using namespace ert;
using namespace open_enclave;

static ert_args_t _args;

ert_args_t ert_get_args_ocall()
{
    assert(_args.argc > 0);
    return _args;
}

static void _init_args(int argc, char* argv[], char* envp[]) noexcept
{
    assert(argc > 0);
    assert(argv);
    assert(envp);

    _args.argc = argc;
    _args.argv = argv;
    _args.envp = envp;

    // count envp elements
    while (envp[_args.envc])
        ++_args.envc;

    _args.auxv = reinterpret_cast<const long*>(_args.envp + _args.envc + 1);

    // count auxv elements
    while (_args.auxv[2 * _args.auxc] || _args.auxv[2 * _args.auxc + 1])
        ++_args.auxc;
}

static int run(const char* path, bool simulate)
{
    assert(path);

    // The semaphore will be unlocked if the program should exit, either because
    // the enclave main thread returned or SIGINT occurred. (Semaphore is the
    // only synchronization primitive that can be used inside a signal handler.)
    static sem_t sem_exit;
    if (sem_init(&sem_exit, 0, 0) != 0)
        throw system_error(errno, system_category(), "sem_init");

    if (simulate)
        cout << "running in simulation mode\n";

    oe_enclave_t* enclave = nullptr;

    if (oe_create_emain_enclave(
            path,
            OE_ENCLAVE_TYPE_AUTO,
            OE_ENCLAVE_FLAG_DEBUG | (simulate ? OE_ENCLAVE_FLAG_SIMULATE : 0),
            nullptr,
            0,
            &enclave) != OE_OK ||
        !enclave)
        throw runtime_error("oe_create_enclave failed. (Set OE_SIMULATION=1 "
                            "for simulation mode.)");

    static int return_value = EXIT_FAILURE;

    {
        const FinalAction terminateEnclave([enclave] {
            signal(SIGINT, SIG_DFL);
            oe_terminate_enclave(enclave);
        });

        // SIGPIPE is received, among others, if a socket connection is lost. We
        // don't have signal handling inside the enclave yet and most
        // applications ignore the signal anyway and directly handle the errors
        // returned by the socket functions. Thus, we just ignore it.
        signal(SIGPIPE, SIG_IGN);

        // create enclave main thread
        host::EnclaveThreadManager::get_instance().create_thread(
            enclave, [](oe_enclave_t* e) {
                if (emain(e, &return_value) != OE_OK ||
                    sem_post(&sem_exit) != 0)
                    abort();
            });

        signal(SIGINT, [](int) {
            if (sem_post(&sem_exit) != 0)
                abort();
        });

        // wait until either the enclave main thread returned or SIGINT occurred
        while (sem_wait(&sem_exit) != 0)
            if (errno != EINTR)
                throw system_error(errno, system_category(), "sem_wait");
    }

    return return_value;
}

int main(int argc, char* argv[], char* envp[])
{
    if (argc < 2)
    {
        cout << "Usage: " << argv[0]
             << " enclave_image_path [enclave args...]\n"
                "Set OE_SIMULATION=1 for simulation mode.\n";
        return EXIT_FAILURE;
    }

    const char* const env_simulation = getenv("OE_SIMULATION");
    const bool simulation = env_simulation && *env_simulation == '1';

    _init_args(argc - 1, argv + 1, envp);

    try
    {
        return run(argv[1], simulation);
    }
    catch (const exception& e)
    {
        OE_TRACE_ERROR("%s", e.what());
    }

    return EXIT_FAILURE;
}
