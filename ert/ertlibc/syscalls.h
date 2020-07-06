// Copyright (c) Edgeless Systems GmbH.
// Licensed under the MIT License.

#pragma once

#include <sched.h>

namespace ert::sc
{
ssize_t getrandom(void* buf, size_t buflen, unsigned int flags);
int sched_getaffinity(pid_t pid, size_t size, cpu_set_t* set);
} // namespace ert::sc
