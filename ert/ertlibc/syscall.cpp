// Copyright (c) Edgeless Systems GmbH.
// Licensed under the MIT License.

#include "syscall.h"
#include <openenclave/internal/time.h>
#include <openenclave/internal/trace.h>
#include <sys/syscall.h>
#include <cerrno>
#include <clocale>
#include <cstdlib>
#include <cstring>
#include <exception>
#include <stdexcept>
#include <system_error>
#include "ertthread.h"
#include "locale.h"
#include "syscalls.h"

using namespace std;
using namespace ert;

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

            case SYS_sched_getaffinity:
                return sc::sched_getaffinity(
                    static_cast<pid_t>(x1),
                    static_cast<size_t>(x2),
                    reinterpret_cast<cpu_set_t*>(x3));

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

// This function is defined in this source file to guarantee that it overrides
// the weak symbol in ert/libc/locale.c.
extern "C" locale_t __newlocale(int mask, const char* locale, locale_t loc)
{
    if (!(mask > 0 && !loc && locale && strcmp(locale, "C") == 0))
        return newlocale(mask, locale, loc);

    // Caller may be stdc++. We must return a struct that satisfies glibc
    // internals (see glibc/locale/global-locale.c). The following struct is
    // also compatible with musl.
    static const __locale_struct c_locale{
        {}, reinterpret_cast<const unsigned short*>(nl_C_LC_CTYPE_class + 128)};
    return const_cast<locale_t>(&c_locale);
}
