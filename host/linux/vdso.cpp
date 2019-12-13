// Copyright (c) Edgeless Systems GmbH.
// Licensed under the MIT License.

#include <openenclave/internal/trace.h>
#include <openenclave/internal/vdso.h>
#include <sys/utsname.h>
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

// v3.15 -v5.2: arch/x86/include/asm/vgtod.h
// v5.3 - v5.4: include/vdso/datapage.h
struct vdso_data
{
    uint32_t seq; // timebase sequence counter
    uint32_t clock_mode;
    uint64_t cycle_last;
    uint64_t mask;
    uint64_t reserved;

    oe_vdso_timestamp_t t0;
    oe_vdso_timestamp_t t1;
    oe_vdso_timestamp_t v_3_15_to_v4_19_realtime_coarse;
    oe_vdso_timestamp_t v_3_15_to_v4_19_monotonic_coarse;
    oe_vdso_timestamp_t t4;
    oe_vdso_timestamp_t v4_20_realtime_coarse;
    oe_vdso_timestamp_t v4_20_monotonic_coarse;
};

// from arch/x86/include/asm/vvar.h, valid for at least kernel v3.0 to v5.4
static const size_t _vvar_vdso_data_offset = 128;

namespace
{
class KernelVersion final
{
  public:
    KernelVersion()
    {
        // Get kernel version from OS.

        utsname u{};
        if (uname(&u) != 0 || !*u.release)
            throw runtime_error("uname() failed");

        istringstream release(u.release);
        release.exceptions(ios::badbit | ios::failbit | ios::eofbit);

        char dot = 0;
        release >> major_ >> dot >> minor_;

        if (!major_ || dot != '.')
            throw runtime_error("cannot parse utsname::release: "s + u.release);
    }

    KernelVersion(unsigned int major, unsigned int minor) noexcept
        : major_(major), minor_(minor)
    {
        assert(major);
    }

    bool operator<(KernelVersion rhs) const noexcept
    {
        return major_ < rhs.major_ ||
               (major_ == rhs.major_ && minor_ < rhs.minor_);
    }

  private:
    unsigned int major_;
    unsigned int minor_;
};
} // namespace

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

    // TODO get pointers dynamically indepentent of kernel version

    const KernelVersion kernel_version;
    if (kernel_version < KernelVersion(3, 15))
        throw runtime_error("Linux kernel below 3.15 is not supported");

    seq = &vdsodata->seq;

    if (kernel_version < KernelVersion(4, 20))
    {
        clock_realtime_coarse = &vdsodata->v_3_15_to_v4_19_realtime_coarse;
        clock_monotonic_coarse = &vdsodata->v_3_15_to_v4_19_monotonic_coarse;
    }
    else
    {
        clock_realtime_coarse = &vdsodata->v4_20_realtime_coarse;
        clock_monotonic_coarse = &vdsodata->v4_20_monotonic_coarse;
    }
}

extern "C" oe_result_t oe_get_clock_vdso_pointers_ocall(
    uint32_t** seq,
    void** clock_realtime_coarse,
    void** clock_monotonic_coarse)
{
    assert(seq);
    assert(clock_realtime_coarse);
    assert(clock_monotonic_coarse);

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
