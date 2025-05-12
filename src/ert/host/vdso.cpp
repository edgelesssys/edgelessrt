// Copyright (c) Edgeless Systems GmbH.
// Licensed under the MIT License.

#include "../common/vdso.h"
#include <openenclave/internal/trace.h>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <fstream>
#include <regex>
#include <sstream>
#include <stdexcept>
#include <string>
#include "core_u.h"

using namespace std;

// from Linux kernel include/vdso/datapage.h, kernel < v6.10
struct vdso_data
{
    uint32_t seq; // timebase sequence counter
    uint32_t clock_mode;
    uint64_t cycle_last;
    uint64_t mask;
    uint64_t reserved0;

    oe_vdso_timestamp_t reserved1[5];
    oe_vdso_timestamp_t realtime_coarse;
    oe_vdso_timestamp_t monotonic_coarse;
};

// from Linux kernel include/vdso/datapage.h, kernel >= v6.10
struct vdso_data_new
{
    uint32_t seq; // timebase sequence counter
    uint32_t clock_mode;
    uint64_t cycle_last;
    uint64_t max_cycles; // new in v6.10 if CONFIG_GENERIC_VDSO_OVERFLOW_PROTECT
    uint64_t mask;
    uint64_t reserved0;

    oe_vdso_timestamp_t reserved1[5];
    oe_vdso_timestamp_t realtime_coarse;
    oe_vdso_timestamp_t monotonic_coarse;
};

// from arch/x86/include/asm/vvar.h, valid for at least kernel v3.0 to v6.0
static const size_t _vvar_vdso_data_offset = 128;

static byte* _get_vvar()
{
    // vvar is the data section of vdso

    const regex vvar_regex(
        "([0-9a-f]+)-[0-9a-f]+ r--p 00000000 00:00 0 +\\[vvar]");

    ifstream f;
    f.exceptions(ios::badbit);
    f.open("/proc/self/maps");

    for (string line; getline(f, line);)
    {
        smatch match;
        if (regex_match(line, match, vvar_regex))
            return reinterpret_cast<byte*>(stoul(match[1], nullptr, 16));
    }

    throw runtime_error("vvar not found in /proc/self/maps");
}

static void _get_clock_vdso_pointers(
    uint32_t*& seq,
    oe_vdso_timestamp_t*& clock_realtime_coarse,
    oe_vdso_timestamp_t*& clock_monotonic_coarse)
{
    const auto vdsodata =
        reinterpret_cast<vdso_data*>(_get_vvar() + _vvar_vdso_data_offset);
    seq = &vdsodata->seq;

    // get current monotonic time as reference value
    timespec tp{};
    if (clock_gettime(CLOCK_MONOTONIC_COARSE, &tp) != 0)
        throw system_error(errno, system_category(), "clock_gettime");

    // try older vdso_data struct
    auto sec =
        __atomic_load_n(&vdsodata->monotonic_coarse.sec, __ATOMIC_SEQ_CST);
    if (abs(sec - tp.tv_sec) <= 1)
    {
        clock_realtime_coarse = &vdsodata->realtime_coarse;
        clock_monotonic_coarse = &vdsodata->monotonic_coarse;
        return;
    }

    // try newer vdso_data struct
    const auto vdsodata_new = reinterpret_cast<vdso_data_new*>(vdsodata);
    sec =
        __atomic_load_n(&vdsodata_new->monotonic_coarse.sec, __ATOMIC_SEQ_CST);
    if (abs(sec - tp.tv_sec) <= 1)
    {
        clock_realtime_coarse = &vdsodata_new->realtime_coarse;
        clock_monotonic_coarse = &vdsodata_new->monotonic_coarse;
        return;
    }

    throw runtime_error("kernel doesn't provide vDSO clock");
}

extern "C" oe_result_t oe_get_clock_vdso_pointers_ocall(
    uint32_t** seq,
    void** clock_realtime_coarse,
    void** clock_monotonic_coarse)
{
    assert(seq);
    assert(clock_realtime_coarse);
    assert(clock_monotonic_coarse);

    const char* const disable = getenv("ERT_DISABLE_VDSO_CLOCK");
    if (disable && *disable == '1')
        return OE_UNSUPPORTED;

    try
    {
        _get_clock_vdso_pointers(
            *seq,
            *reinterpret_cast<oe_vdso_timestamp_t**>(clock_realtime_coarse),
            *reinterpret_cast<oe_vdso_timestamp_t**>(clock_monotonic_coarse));
    }
    catch (const exception& e)
    {
        OE_TRACE_ERROR("%s", e.what());
        return OE_FAILURE;
    }

    return OE_OK;
}
