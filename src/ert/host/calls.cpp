// Copyright (c) Edgeless Systems GmbH.
// Licensed under the MIT License.

#include <openenclave/internal/trace.h>
#include <sched.h>
#include <sys/statfs.h>
#include <cassert>
#include <exception>
#include "enclave_thread_manager.h"
#include "ertlibc_u.h"

using namespace std;
using namespace ert;

// This function is supposed to be run in a new thread created by the
// EnclaveThreadManager. The thread may be canceled. In the implementation of
// glibc, the canceled thread throws the internal exception type __forced_unwind
// to end itself. This exception must not be caught by, e.g., noexcept or
// catch(...).
static void _invoke_create_thread_ecall(oe_enclave_t* enclave)
{
    assert(enclave);
    if (ert_create_thread_ecall(enclave) != OE_OK)
        abort();
}

bool ert_create_thread_ocall(oe_enclave_t* enclave)
{
    assert(enclave);

    try
    {
        host::EnclaveThreadManager::get_instance().create_thread(
            enclave, _invoke_create_thread_ecall);
    }
    catch (const exception& e)
    {
        OE_TRACE_ERROR("%s", e.what());
        return false;
    }

    return true;
}

int ert_clock_gettime_ocall(
    clockid_t clk_id,
    ert_clock_gettime_ocall_timespec* tp)
{
    return clock_gettime(clk_id, reinterpret_cast<timespec*>(tp));
}

void ert_exit_ocall(int status)
{
    exit(status);
}

size_t ert_getaffinity_cpucount()
{
    cpu_set_t cpuset{};
    if (sched_getaffinity(0, sizeof cpuset, &cpuset) != 0)
        return 0;
    return static_cast<size_t>(CPU_COUNT(&cpuset));
}

int ert_statfs_ocall(const char* path, uint64_t* buf)
{
    return statfs(path, reinterpret_cast<struct statfs*>(buf));
}
