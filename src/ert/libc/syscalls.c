// Copyright (c) Edgeless Systems GmbH.
// Licensed under the MIT License.

#include <errno.h>
#include <fcntl.h>
#include <openenclave/corelibc/assert.h>
#include <openenclave/internal/syscall/hook.h>
#include <openenclave/internal/syscall/sys/syscall.h>
#include <openenclave/internal/thread.h>
#include <openenclave/internal/time.h>
#include <openenclave/internal/trace.h>
#include <stdlib.h>
#include <sys/random.h>
#include <sys/socket.h>
#include <time.h>
#include "../../ertlibc/syscall.h"
#include "ertfutex.h"
#include "mman.h"

static oe_syscall_hook_t _hook;
static oe_spinlock_t _lock;

long __syscall(long n, long x1, long x2, long x3, long x4, long x5, long x6)
{
    // These syscalls must always be available for libc.
    switch (n)
    {
        case OE_SYS_futex:
            return ert_futex(
                (int*)x1, x2, x3, (struct timespec*)x4, (int*)x5, x6);
        case OE_SYS_set_thread_area:
            // Called by __init_tp (see pthread.c). Can be a noop as TLS is
            // handled by OE.
            return 0;
        case OE_SYS_set_tid_address:
        {
            // Called by __init_tp and returns the tid. Must be unique as musl
            // uses it for thread locking.
            static long tid;
            return __atomic_add_fetch(&tid, 1, __ATOMIC_SEQ_CST);
        }
        case OE_SYS_mmap:
            return (long)ert_mmap((void*)x1, x2, x3, x4, x5, x6);
        case OE_SYS_munmap:
            return ert_munmap((void*)x1, x2);
    }

    // Try hook
    oe_spin_lock(&_lock);
    const oe_syscall_hook_t hook = _hook;
    oe_spin_unlock(&_lock);
    if (hook)
    {
        long ret = -ENOSYS;
        if (hook(n, x1, x2, x3, x4, x5, x6, &ret) == OE_OK)
        {
            // hook returns result and errno like libc syscall(), but musl
            // expects them like raw syscall, so we must adjust the values.
            if (ret >= 0)
                return ret;
            if (ret == -1 && errno > 0)
                return -errno;
            OE_TRACE_FATAL(
                "unexpected syscall hook result: n=%lu ret=%ld errno=%d",
                n,
                ret,
                errno);
            abort();
        }
    }

    // Try libertlibc
    long ret = ert_syscall(n, x1, x2, x3, x4, x5, x6);
    if (ret != -ENOSYS)
        return ret;

    // Try liboesyscall
    const long org_n = n;
    switch (n)
    {
        case OE_SYS_lstat:
            n = OE_SYS_stat;
            break;
        case OE_SYS_newfstatat:
            if (x4 == AT_SYMLINK_NOFOLLOW)
                x4 = 0;
            break;
        case OE_SYS_accept4:
            n = OE_SYS_accept;
            break;
        case OE_SYS_getrandom:
            // clear flags because oe_getrandom fails if any are set and it
            // should be fine to ignore them
            x3 = 0;
            break;
    }
    errno = 0;
    ret = oe_syscall(n, x1, x2, x3, x4, x5, x6);
    // oe_syscall returns result and errno like libc syscall(), but musl expects
    // them like raw syscall, so we must adjust the values.
    if (ret == -1 && errno > 0)
    {
        if (errno == ENOSYS)
            OE_TRACE_WARNING("unhandled syscall: n=%lu", n);
        return -errno;
    }
    if (ret < 0)
    {
        OE_TRACE_FATAL(
            "unexpected oe_syscall result: n=%lu ret=%ld errno=%d",
            n,
            ret,
            errno);
        abort();
    }

    switch (org_n)
    {
        case OE_SYS_accept4:
            if (x4 & SOCK_NONBLOCK)
                oe_syscall(OE_SYS_fcntl, ret, F_SETFL, O_NONBLOCK);
            break;
    }
    return ret;
}

long __syscall_cp(long n, long x1, long x2, long x3, long x4, long x5, long x6)
{
    return __syscall(n, x1, x2, x3, x4, x5, x6);
}

// copied from libc/syscalls.c
static long _syscall_clock_gettime(long x1, long x2)
{
    static const uint64_t _SEC_TO_MSEC = 1000UL;
    static const uint64_t _MSEC_TO_NSEC = 1000000UL;

    clockid_t clock_id = (clockid_t)x1;
    struct timespec* tp = (struct timespec*)x2;
    int ret = -1;
    uint64_t msec;

    if (!tp)
        goto done;

    if (clock_id != CLOCK_REALTIME)
    {
        /* Only supporting CLOCK_REALTIME */
        oe_assert("clock_gettime(): panic" == NULL);
        goto done;
    }

    if ((msec = oe_get_time()) == (uint64_t)-1)
        goto done;

    tp->tv_sec = msec / _SEC_TO_MSEC;
    tp->tv_nsec = (msec % _SEC_TO_MSEC) * _MSEC_TO_NSEC;

    ret = 0;

done:

    return ret;
}

// Fallback if libertlibc is not linked.
__attribute__((__weak__)) long
ert_syscall(long n, long x1, long x2, long x3, long x4, long x5, long x6)
{
    if (n == OE_SYS_clock_gettime)
        return _syscall_clock_gettime(x1, x2);

    return -ENOSYS;
}

// copied from libc/syscalls.c
void oe_register_syscall_hook(oe_syscall_hook_t hook)
{
    oe_spin_lock(&_lock);
    _hook = hook;
    oe_spin_unlock(&_lock);
}
