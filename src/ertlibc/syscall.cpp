// Copyright (c) Edgeless Systems GmbH.
// Licensed under the MIT License.

#include "syscall.h"
#include <fcntl.h>
#include <openenclave/enclave.h>
#include <openenclave/internal/malloc.h>
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

long ert_syscall(long n, long x1, long x2, long x3, long x4, long x5, long x6)
{
    try
    {
        switch (n)
        {
            case SYS_sched_yield:
                __builtin_ia32_pause();
                return 0;
            case SYS_clock_gettime:
                return sc::clock_gettime(
                    static_cast<int>(x1), reinterpret_cast<timespec*>(x2));
            case SYS_gettid:
                return ert_thread_self()->tid;

            case SYS_readlink:
                return sc::readlink(
                    reinterpret_cast<char*>(x1), // pathname
                    reinterpret_cast<char*>(x2), // buf
                    x3);                         // bufsiz
            case SYS_readlinkat:
                return static_cast<int>(x1) == AT_FDCWD
                           ? sc::readlink(
                                 reinterpret_cast<char*>(x2), // pathname
                                 reinterpret_cast<char*>(x3), // buf
                                 x4)                          // bufsiz
                           : -EBADF;

            case SYS_exit_group:
                sc::exit_group(static_cast<int>(x1));
                return 0;
            case SYS_fchmodat:
                return 0; // noop for now
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

            case SYS_rt_sigaction:
                sc::rt_sigaction(
                    static_cast<int>(x1),                // signum
                    reinterpret_cast<k_sigaction*>(x2),  // act
                    reinterpret_cast<k_sigaction*>(x3)); // oldact
                return 0;

            case SYS_rt_sigprocmask:
                return 0; // Not supported. Silently ignore and return success.

            case SYS_sigaltstack:
                sc::sigaltstack(
                    reinterpret_cast<stack_t*>(x1),
                    reinterpret_cast<stack_t*>(x2));
                return 0;

            case 1000:
                return oe_get_report_v2(
                    OE_REPORT_FLAGS_REMOTE_ATTESTATION,
                    reinterpret_cast<uint8_t*>(x1),
                    static_cast<size_t>(x2),
                    reinterpret_cast<void*>(x3),
                    static_cast<size_t>(x4),
                    reinterpret_cast<uint8_t**>(x5),
                    reinterpret_cast<size_t*>(x6));

            case 1001:
                oe_free_report(reinterpret_cast<uint8_t*>(x1));
                return 0;

            case 1002:
                return oe_verify_report(
                    reinterpret_cast<uint8_t*>(x1),
                    static_cast<size_t>(x2),
                    reinterpret_cast<oe_report_t*>(x3));

            case 1003:
                return oe_get_seal_key_v2(
                    reinterpret_cast<uint8_t*>(x1),
                    static_cast<size_t>(x2),
                    reinterpret_cast<uint8_t**>(x3),
                    reinterpret_cast<size_t*>(x4));

            case 1004:
                oe_free_seal_key(
                    reinterpret_cast<uint8_t*>(x1),
                    reinterpret_cast<uint8_t*>(x2));
                return 0;

            case 1005:
                return oe_get_seal_key_by_policy_v2(
                    static_cast<oe_seal_policy_t>(x1),
                    reinterpret_cast<uint8_t**>(x2),
                    reinterpret_cast<size_t*>(x3),
                    reinterpret_cast<uint8_t**>(x4),
                    reinterpret_cast<size_t*>(x5));

            case 1006:
                return reinterpret_cast<long>(
                    oe_result_str(static_cast<oe_result_t>(x1)));
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
// the weak symbol in ert/libc/newlocale.c.
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

// The debug malloc check runs on enclave termination and prints errors for heap
// memory that has not been freed. As some global objects in (std)c++ use heap
// memory and don't free it by design, we cannot use this feature.
static int _init = [] {
    oe_disable_debug_malloc_check = true;
    return 0;
}();
