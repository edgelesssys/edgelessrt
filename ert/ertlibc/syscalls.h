// Copyright (c) Edgeless Systems GmbH.
// Licensed under the MIT License.

#pragma once

#include <sched.h>
#include <sys/types.h>
#include <ctime>

struct k_sigaction;

namespace ert::sc
{
int clock_gettime(clockid_t clk_id, struct timespec* tp);
ssize_t getrandom(void* buf, size_t buflen, unsigned int flags);
void rt_sigaction(int signum, const k_sigaction* act, k_sigaction* oldact);
int sched_getaffinity(pid_t pid, size_t size, cpu_set_t* set);
} // namespace ert::sc
