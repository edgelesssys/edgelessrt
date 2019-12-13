// Copyright (c) Edgeless Systems GmbH.
// Licensed under the MIT License.

#include <openenclave/bits/types.h>
#include <openenclave/internal/calls.h>
#include <openenclave/internal/thread.h>
#include <openenclave/internal/time.h>
#include <openenclave/internal/trace.h>
#include <openenclave/internal/vdso.h>
#include "core_t.h"

#ifndef CLOCK_REALTIME
#define CLOCK_REALTIME 0
#define CLOCK_MONOTONIC 1
#define CLOCK_REALTIME_COARSE 5
#define CLOCK_MONOTONIC_COARSE 6
#endif

static oe_once_t _init_clock_once = OE_ONCE_INIT;
static const volatile uint32_t* _clock_seq;
static const volatile oe_vdso_timestamp_t* _clock_realtime_coarse;
static const volatile oe_vdso_timestamp_t* _clock_monotonic_coarse;

static void _init_clock(void)
{
    oe_result_t ret = OE_FAILURE;
    if (oe_get_clock_vdso_pointers_ocall(
            &ret,
            (uint32_t**)&_clock_seq,
            (void**)&_clock_realtime_coarse,
            (void**)&_clock_monotonic_coarse) != OE_OK ||
        ret != OE_OK)
    {
        _clock_seq = NULL;
        OE_TRACE_ERROR("oe_get_clock_vdso_pointers_ocall() failed");
        return;
    }

    if (!oe_is_outside_enclave((void*)_clock_seq, sizeof *_clock_seq) ||
        !oe_is_outside_enclave(
            (void*)_clock_realtime_coarse, sizeof *_clock_realtime_coarse) ||
        !oe_is_outside_enclave(
            (void*)_clock_monotonic_coarse, sizeof *_clock_monotonic_coarse))
        oe_abort();
}

static int _clock_gettime(int clk_id, struct oe_timespec* tp)
{
    oe_assert(tp);
    OE_TRACE_WARNING("clockid %ld was not served by vDSO", clk_id);
    int ret = -1;
    if (oe_clock_gettime_ocall(&ret, clk_id, tp) != OE_OK)
        return -1;
    return ret;
}

int oe_clock_gettime(int clk_id, struct oe_timespec* tp)
{
    if (!tp)
        return -1;

    if (oe_once(&_init_clock_once, _init_clock) != OE_OK)
        oe_abort();

    if (!_clock_seq)
        return _clock_gettime(clk_id, tp);

    oe_assert(_clock_realtime_coarse);
    oe_assert(_clock_monotonic_coarse);

    const volatile oe_vdso_timestamp_t* vdso_timestamp;
    bool monotonic = false;

    // always use coarse timestamps because the others need rdtsc
    switch (clk_id)
    {
        case CLOCK_REALTIME:
        case CLOCK_REALTIME_COARSE:
            vdso_timestamp = _clock_realtime_coarse;
            break;
        case CLOCK_MONOTONIC:
        case CLOCK_MONOTONIC_COARSE:
            vdso_timestamp = _clock_monotonic_coarse;
            monotonic = true;
            break;
        default:
            return _clock_gettime(clk_id, tp);
    }

    oe_vdso_timestamp_t timestamp;
    uint32_t seq;

    static oe_spinlock_t lock;
    if (monotonic)
        oe_spin_lock(&lock);

    // The kernel increments *_clock_seq before and after updating the
    // timestamps. seq is odd during the update.

    do
    {
        // Wait until a possibly running update is finished.
        while ((seq = __atomic_load_n(_clock_seq, __ATOMIC_SEQ_CST)) & 1)
            __builtin_ia32_pause();

        timestamp.sec = __atomic_load_n(&vdso_timestamp->sec, __ATOMIC_SEQ_CST);
        timestamp.nsec =
            __atomic_load_n(&vdso_timestamp->nsec, __ATOMIC_SEQ_CST);

        // Check that there has not been an update while reading the timestamp.
    } while (__atomic_load_n(_clock_seq, __ATOMIC_SEQ_CST) != seq);

    if (monotonic)
    {
        static oe_vdso_timestamp_t last_monotonic;

        // Abort if CLOCK_MONOTONIC is not monotonic.
        if (timestamp.sec < last_monotonic.sec ||
            (timestamp.sec == last_monotonic.sec &&
             timestamp.nsec < last_monotonic.nsec))
            oe_abort();

        last_monotonic = timestamp;
        oe_spin_unlock(&lock);
    }

    tp->tv_sec = timestamp.sec;
    tp->tv_nsec = timestamp.nsec;

    return 0;
}

uint64_t oe_get_time(void)
{
    // EDG: adapted from _time() in host/linux/time.c

    const uint64_t _SEC_TO_MSEC = 1000UL;
    const uint64_t _MSEC_TO_NSEC = 1000000UL;

    struct oe_timespec ts;

    if (oe_clock_gettime(CLOCK_REALTIME, &ts) != 0)
        return 0;

    return ((uint64_t)ts.tv_sec * _SEC_TO_MSEC) +
           ((uint64_t)ts.tv_nsec / _MSEC_TO_NSEC);
}

OE_WEAK oe_result_t
oe_clock_gettime_ocall(int* retval, int clk_id, struct oe_timespec* tp)
{
    (void)retval;
    (void)clk_id;
    (void)tp;
    return OE_UNSUPPORTED;
}

OE_WEAK oe_result_t oe_get_clock_vdso_pointers_ocall(
    oe_result_t* retval,
    uint32_t** seq,
    void** clock_realtime_coarse,
    void** clock_monotonic_coarse)
{
    (void)retval;
    (void)seq;
    (void)clock_realtime_coarse;
    (void)clock_monotonic_coarse;
    return OE_UNSUPPORTED;
}
