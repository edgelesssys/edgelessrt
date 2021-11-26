// Copyright (c) Edgeless Systems GmbH.
// Licensed under the MIT License.

#include <openenclave/internal/globals.h>
#include <sched.h>
#include <cerrno>
#include <system_error>
#include "ertlibc_t.h"
#include "syscalls.h"

using namespace std;
using namespace ert;

int sc::sched_getaffinity(pid_t pid, size_t size, cpu_set_t* set)
{
    if (!size)
        throw system_error(
            EINVAL, system_category(), "sched_getaffinity: size");
    if (!set)
        throw system_error(EFAULT, system_category(), "sched_getaffinity: set");
    if (pid)
        // getting affinity of other processes not supported
        throw system_error(ENOSYS, system_category(), "sched_getaffinity: pid");

    // Determine ncpu such that it's ideally the number of CPUs in the host
    // process's affinity mask, but satisfies 2 <= ncpu <= num_tcs - 2
    // - at least 2 to prevent callers to assume that this is a single-threaded
    //   process and doesn't need synchronization
    // - a bit lower than num_tcs to reduce the chance that the caller will
    //   create too many threads and causes OE_OUT_OF_THREADS
    static const size_t ncpu = []() -> size_t {
        const size_t num_tcs = oe_get_num_tcs();
        size_t count = 0;
        if (num_tcs <= 4 || ert_getaffinity_cpucount(&count) != OE_OK ||
            count <= 2)
            return 2;
        return min(count, num_tcs - 2);
    }();

    CPU_ZERO_S(size, set);
    for (size_t i = 0; i < ncpu; ++i)
        CPU_SET_S(i, size, set);

    return min(size, (ncpu + 7) / 8); // number of bytes written
}
