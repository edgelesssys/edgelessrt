// Copyright (c) Edgeless Systems GmbH.
// Licensed under the MIT License.

#include "threaded.h"
#include "../3rdparty/musl/musl/src/internal/libc.h"
#include "../3rdparty/musl/musl/src/internal/pthread_impl.h"
#include "../3rdparty/musl/musl/src/internal/stdio_impl.h"

// copied from musl/thread/pthread_create.c
static void init_file_lock(FILE* f)
{
    if (f && f->lock < 0)
        f->lock = 0;
}

// adapted from __pthread_create() in musl/thread/pthread_create.c
void musl_init_threaded(void)
{
    if (libc.threaded)
        return;
    for (FILE* f = *__ofl_lock(); f; f = f->next)
        init_file_lock(f);
    __ofl_unlock();
    init_file_lock(__stdin_used);
    init_file_lock(__stdout_used);
    init_file_lock(__stderr_used);
    libc.threaded = 1;
}

// Locks used internally in musl. These are adapted to spin instead of using
// futex (which is not supported in OE). We cannot use oe_spinlock because musl
// depends on implementation details. E.g., some functions "wake" a lock by
// directly modifying the value.

// adapted from musl/thread/__lock.c
void __lock(volatile int* l)
{
    if (!libc.threads_minus_1)
        return;
    /* fast path: INT_MIN for the lock, +1 for the congestion */
    int current = a_cas(l, 0, INT_MIN + 1);
    if (!current)
        return;
    /* spin loop */
    for (;;)
    {
        if (current < 0)
            current -= INT_MIN + 1;
        // assertion: current >= 0
        int val = a_cas(l, current, INT_MIN + (current + 1));
        if (val == current)
            return;
        current = val;
        __builtin_ia32_pause();
    }
}

// adapted from musl/thread/__lock.c
void __unlock(volatile int* l)
{
    /* Check l[0] to see if we are multi-threaded. */
    if (l[0] < 0)
    {
        if (a_fetch_add(l, -(INT_MIN + 1)) != (INT_MIN + 1))
        {
            //__wake(l, 1, 1);
        }
    }
}

// adapted from musl/stdio/__lockfile.c
int __lockfile(FILE* f)
{
    int owner = f->lock, tid = __pthread_self()->tid;
    if (owner == tid)
        return 0;
    while ((owner = a_cas(&f->lock, 0, tid)))
    {
        __builtin_ia32_pause();
    }
    return 1;
}

// adapted from musl/stdio/__lockfile.c
void __unlockfile(FILE* f)
{
    a_swap(&f->lock, 0);
}
