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

// from Linux kernel include/vdso/datapage.h
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

    // TODO This heuristic check is not guaranteed to catch the error. There
    // could be (future) kernels that have some other data at these offsets.
    if (!vdsodata->realtime_coarse.sec || !vdsodata->monotonic_coarse.sec)
        throw runtime_error("kernel doesn't provide vDSO clock");

    seq = &vdsodata->seq;
    clock_realtime_coarse = &vdsodata->realtime_coarse;
    clock_monotonic_coarse = &vdsodata->monotonic_coarse;
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
