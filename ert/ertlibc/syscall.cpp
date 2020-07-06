// Copyright (c) Edgeless Systems GmbH.
// Licensed under the MIT License.

#include "syscall.h"
#include <openenclave/internal/time.h>
#include <openenclave/internal/trace.h>
#include <sys/syscall.h>
#include <cerrno>
#include <cstdlib>
#include <exception>
#include <stdexcept>
#include <system_error>
#include "ertthread.h"

using namespace std;

long ert_syscall(long n, long x1, long x2, long x3, long, long, long)
{
    try
    {
        switch (n)
        {
            case SYS_sched_yield:
                __builtin_ia32_pause();
                return 0;
            case SYS_clock_gettime:
                return oe_clock_gettime(
                    static_cast<int>(x1), reinterpret_cast<oe_timespec*>(x2));
            case SYS_gettid:
                return ert_thread_self()->tid;

            case SYS_mprotect:
            case SYS_madvise:
            case SYS_mlock:
            case SYS_munlock:
            case SYS_mlockall:
            case SYS_munlockall:
            case SYS_mlock2:
                // These can all be noops.
                return 0;

            case SYS_rt_sigprocmask:
            case SYS_sigaltstack:
                // Signals are not supported. Silently ignore and return
                // success.
                return 0;
        }
    }
    catch (const system_error& e)
    {
        OE_TRACE_ERROR("%s", e.what());
        return -e.code().value();
    }
    catch (const runtime_error& e)
    {
        OE_TRACE_ERROR("%s", e.what());
    }
    catch (const exception& e)
    {
        OE_TRACE_FATAL("%s", e.what());
        abort();
    }

    return -ENOSYS;
}
