// Copyright (c) Edgeless Systems GmbH.
// Licensed under the MIT License.

#include <errno.h>
#include <limits.h>
#include <openenclave/internal/defs.h>
#include <openenclave/internal/thread.h>
#include <semaphore.h>
#include <stddef.h>
#include <string.h>
#include "atomic.h"

// musl uses the first 3 elements of sem_t, we use the rest for wait/wake
// metadata.
static const int _index_oe_sem_t = 3;

// adapted from musl's sem_init.c
int sem_init(sem_t* sem, int pshared, unsigned value)
{
    OE_STATIC_ASSERT(
        sizeof(sem_t) - offsetof(sem_t, __val[_index_oe_sem_t]) >=
        sizeof(oe_sem_t));

    if (value > SEM_VALUE_MAX)
    {
        errno = EINVAL;
        return -1;
    }
    memset(sem, 0, sizeof *sem);
    sem->__val[0] = value;
    sem->__val[1] = 0;
    sem->__val[2] = pshared ? 0 : 128;
    return 0;
}

// adapted from musl's sem_post.c
int sem_post(sem_t* sem)
{
    int val, waiters;
    do
    {
        val = sem->__val[0];
        waiters = sem->__val[1];
        if (val == SEM_VALUE_MAX)
        {
            errno = EOVERFLOW;
            return -1;
        }
    } while (a_cas(sem->__val, val, val + 1 + (val < 0)) != val);
    if (val < 0 || waiters)
        oe_sem_wake((oe_sem_t*)(&sem->__val[_index_oe_sem_t]));
    return 0;
}

// adapted from musl's sem_timedwait.c
int sem_timedwait(sem_t* restrict sem, const struct timespec* restrict at)
{
    if (!sem_trywait(sem))
        return 0;

    int spins = 100;
    while (spins-- && sem->__val[0] <= 0 && !sem->__val[1])
        a_spin();

    while (sem_trywait(sem))
    {
        oe_result_t r;
        a_inc(sem->__val + 1);
        a_cas(sem->__val, 0, -1);
        r = oe_sem_wait(
            (oe_sem_t*)(&sem->__val[_index_oe_sem_t]),
            sem->__val,
            (struct oe_timespec*)at);
        a_dec(sem->__val + 1);
        if (r != OE_OK)
        {
            errno = r == OE_TIMEDOUT ? ETIMEDOUT : EINVAL;
            return -1;
        }
    }
    return 0;
}
