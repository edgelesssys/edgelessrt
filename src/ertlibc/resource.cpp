// Copyright (c) Edgeless Systems GmbH.
// Licensed under the MIT License.

#include <openenclave/bits/properties.h>
#include <openenclave/internal/globals.h>
#include <sys/resource.h>
#include <algorithm>
#include <array>
#include <cerrno>
#include <mutex>
#include <system_error>
#include "spinlock.h"
#include "syscalls.h"

using namespace std;
using namespace ert;

static array<rlimit, RLIM_NLIMITS> _get_initial_limits() noexcept
{
    array<rlimit, RLIM_NLIMITS> li{};

    // Fill initial values. Unless otherwise noted, these are based on the
    // system defaults of Ubuntu 20.04.

    li.fill({RLIM_INFINITY, RLIM_INFINITY});

    const size_t stack = oe_get_num_stack_pages() * OE_PAGE_SIZE;
    li[RLIMIT_STACK] = {stack, stack};

    li[RLIMIT_CORE].rlim_cur = 0;

    // 2 <= nproc <= num_tcs - 2 for same reasons as sched_getaffinity in
    // sched.cpp
    const size_t nproc =
        max(min(oe_get_num_tcs(), static_cast<size_t>(OE_SGX_MAX_TCS)), 4ul) -
        2;
    li[RLIMIT_NPROC] = {nproc, nproc};

    // reserve some for possible use by host process, EGo runtime, etc.
    li[RLIMIT_NOFILE] = {1024 - 32, 1048576};

    li[RLIMIT_MEMLOCK] = {67108864, 67108864};

    // Not sure about SIGPENDING. It's the same value as NPROC on Ubuntu,
    // but I don't want to reduce it that much.
    li[RLIMIT_SIGPENDING] = {OE_SGX_MAX_TCS, OE_SGX_MAX_TCS};

    li[RLIMIT_MSGQUEUE] = {819200, 819200};
    li[RLIMIT_NICE] = {0, 0};
    li[RLIMIT_RTPRIO] = {0, 0};

    return li;
}

/*
prlimit is the implementation for the prlimit64, getrlimit, and setrlimit
syscalls. The functions should behave like their actual Linux counterparts. This
is mainly for compatibility with existing code calling these functions. The
limits are NOT actually enforced.
*/
int sc::prlimit(
    pid_t pid,
    int resource,
    const rlimit* new_limit,
    rlimit* old_limit)
{
    if (pid)
        throw system_error(
            ESRCH, system_category(), "prlimit: pid not supported");
    if (!(0 <= resource && resource < RLIM_NLIMITS))
        throw system_error(
            EINVAL, system_category(), "prlimit: resource out of range");

    static Spinlock spinlock;
    const lock_guard lock(spinlock);

    static array<rlimit, RLIM_NLIMITS> limits = _get_initial_limits();

    auto& limit = limits[resource];

    if (old_limit)
        *old_limit = limit;

    if (!new_limit)
        return 0;

    if (new_limit->rlim_cur > new_limit->rlim_max)
        throw system_error(EINVAL, system_category(), "prlimit: cur > max");
    if (new_limit->rlim_max > limit.rlim_max)
        throw system_error(
            EPERM, system_category(), "prlimit: tried to raise hard limit");

    limit = *new_limit;
    return 0;
}
