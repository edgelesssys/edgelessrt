// Copyright (c) Edgeless Systems GmbH.
// Licensed under the MIT License.

#pragma once

#include <sched.h>
#include <sys/statfs.h>
#include <sys/types.h>
#include <csignal>
#include <ctime>

struct k_sigaction;

namespace ert::sc
{
int clock_gettime(clockid_t clk_id, struct timespec* tp);
void exit_group(int status);
int fstatfs(int fd, struct statfs* buf);
ssize_t readlink(const char* pathname, char* buf, size_t bufsiz);
void rt_sigaction(int signum, const k_sigaction* act, k_sigaction* oldact);
int sched_getaffinity(pid_t pid, size_t size, cpu_set_t* set);
void sigaltstack(const stack_t* ss, stack_t* oss);
int statfs(const char* path, struct statfs* buf);
} // namespace ert::sc
