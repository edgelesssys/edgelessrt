// Copyright (c) Edgeless Systems GmbH.
// Licensed under the MIT License.

#include <sched.h>
#include <cerrno>
#include <system_error>
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

    CPU_ZERO_S(size, set);

    // Return at least 2 CPUs so that Go does not assume a single-threaded
    // process.
    CPU_SET_S(0, size, set);
    CPU_SET_S(1, size, set);

    return 1;
}
