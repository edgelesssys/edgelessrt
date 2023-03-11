// Copyright (c) Edgeless Systems GmbH.
// Licensed under the MIT License.

#include <openenclave/internal/thread.h>
#include <openenclave/internal/trace.h>
#include "../common/vdso.h"
#include "ertlibc_t.h"
#include "syscalls.h"

using namespace ert;

static oe_once_t _init_clock_once = OE_ONCE_INIT;
static const volatile uint32_t* _clock_seq;
static const volatile oe_vdso_timestamp_t* _clock_realtime_coarse;
static const volatile oe_vdso_timestamp_t* _clock_monotonic_coarse;
static oe_spinlock_t _monotonic_lock;

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

static void _ensure_range(oe_vdso_timestamp_t timestamp)
{
    const auto ns = timestamp.nsec;
    if (!(0 <= ns && ns < 1'000'000'000))
        oe_abort();
}

static void _ensure_monotonic_unlocked(oe_vdso_timestamp_t& timestamp)
{
    static oe_vdso_timestamp_t last_monotonic;

    // Ensure CLOCK_MONOTONIC can't go backwards.
    if (timestamp.sec < last_monotonic.sec ||
        (timestamp.sec == last_monotonic.sec &&
         timestamp.nsec < last_monotonic.nsec))
        timestamp = last_monotonic;
    else
        last_monotonic = timestamp;
}

static int _clock_gettime(int clk_id, struct timespec* tp)
{
    oe_assert(tp);
    OE_TRACE_WARNING("clockid %ld was not served by vDSO", clk_id);
    int ret = -1;
    oe_vdso_timestamp_t timestamp{};

    const bool monotonic =
        clk_id == CLOCK_MONOTONIC || clk_id == CLOCK_MONOTONIC_COARSE;
    if (monotonic)
        oe_spin_lock(&_monotonic_lock);

    if (ert_clock_gettime_ocall(
            &ret,
            clk_id,
            reinterpret_cast<ert_clock_gettime_ocall_timespec*>(&timestamp)) !=
        OE_OK)
    {
        if (monotonic)
            oe_spin_unlock(&_monotonic_lock);
        return -1;
    }

    _ensure_range(timestamp);

    if (monotonic)
    {
        _ensure_monotonic_unlocked(timestamp);
        oe_spin_unlock(&_monotonic_lock);
    }

    tp->tv_sec = timestamp.sec;
    tp->tv_nsec = timestamp.nsec;
    return ret;
}

int sc::clock_gettime(int clk_id, struct timespec* tp)
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

    if (monotonic)
        oe_spin_lock(&_monotonic_lock);

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

    _ensure_range(timestamp);

    if (monotonic)
    {
        _ensure_monotonic_unlocked(timestamp);
        oe_spin_unlock(&_monotonic_lock);
    }

    tp->tv_sec = timestamp.sec;
    tp->tv_nsec = timestamp.nsec;

    return 0;
}
