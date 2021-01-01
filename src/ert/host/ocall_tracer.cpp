// Copyright (c) Edgeless Systems GmbH.
// Licensed under the MIT License.

#include <dlfcn.h>
#include <openenclave/internal/backtrace.h>
#include <openenclave/internal/elf.h>
#include <openenclave/internal/trace.h>
#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <exception>
#include <fstream>
#include <iomanip>
#include <ios>
#include <iostream>
#include <map>
#include <memory>
#include <mutex>
#include <ostream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>
#include "../common/final_action.h"
#include "../host/sgx/enclave.h"
#include "debug.h"
#include "platform_u.h"

using namespace std;
using namespace open_enclave;

// Each thread has its own backtrace buffer. It is filled by the enclave before
// making the ocall (see oe_ocall() in enclave/core/sgx/calls.c). The first
// element is the size of the backtrace.
static thread_local vector<void*> _backtrace(1 + OE_BACKTRACE_MAX);

extern "C" void** oe_sgx_get_backtrace_buffer_ocall()
{
    return _backtrace.data();
}

namespace
{
class OcallTracer final
{
  public:
    OcallTracer() noexcept;
    ~OcallTracer();
    void trace(oe_enclave_t* enclave, const void* func);

  private:
    bool enabled_;
    mutex mutex_;

    // count ocalls and unique backtraces
    unordered_map<const void*, uint64_t> ocalls_;
    map<pair<oe_enclave_t*, vector<const void*>>, uint64_t> backtraces_;

    // enclaves' base addr and path
    unordered_map<const oe_enclave_t*, pair<uint64_t, string>> enclaves_;

    void dump_ocalls(ostream& out) const;
    void dump_backtraces(ostream& out) const;
} _tracer;
} // namespace

extern "C" void ert_trace_ocall(oe_enclave_t* enclave, const void* func)
{
    try
    {
        _tracer.trace(enclave, func);
    }
    catch (const exception& e)
    {
        OE_TRACE_FATAL("%s", e.what());
        abort();
    }
}

OcallTracer::OcallTracer() noexcept
{
    const char* const trace_ocalls = getenv("OE_TRACE_OCALLS");
    enabled_ = trace_ocalls && *trace_ocalls == '1';
}

OcallTracer::~OcallTracer()
{
    if (!enabled_)
        return;

    try
    {
        cout << "\n"
                "------\n"
                "ocalls\n"
                "------\n";

        dump_ocalls(cout);

        cout << "------\n";

        if (backtraces_.empty())
            return;

        cout << "dumping backtraces.txt ... " << flush;

        ofstream f;
        f.exceptions(ios::badbit | ios::failbit | ios::eofbit);
        f.open("backtraces.txt");
        dump_backtraces(f);

        cout << "done\n";
    }
    catch (const exception& e)
    {
        OE_TRACE_ERROR("%s", e.what());
    }
}

void OcallTracer::trace(oe_enclave_t* enclave, const void* func)
{
    assert(enclave);
    assert(func);

    if (!enabled_)
        return;

    const auto backtrace_size = reinterpret_cast<intptr_t>(_backtrace[0]);
    assert(0 <= backtrace_size && backtrace_size <= OE_BACKTRACE_MAX);

    // Set size to 0 so that the next trace will not take an old backtrace in
    // case the enclave doesn't make a new one.
    _backtrace[0] = nullptr;

    vector<const void*> backtrace(
        _backtrace.cbegin() + 1, _backtrace.cbegin() + 1 + backtrace_size);
    const pair enclaveAndBacktrace(enclave, move(backtrace));

    const lock_guard lock(mutex_);
    enclaves_.try_emplace(enclave, enclave->addr, enclave->path);
    ++ocalls_[func];
    if (!enclaveAndBacktrace.second.empty())
        ++backtraces_[enclaveAndBacktrace];
}

template <typename TMap>
static auto _to_sorted_vector(const TMap& m)
{
    vector<pair<typename TMap::key_type, typename TMap::mapped_type>> result(
        m.cbegin(), m.cend());
    sort(result.begin(), result.end(), [](const auto& lhs, const auto& rhs) {
        return lhs.second > rhs.second;
    });
    return result;
}

void OcallTracer::dump_ocalls(ostream& out) const
{
    for (const auto [func, count] : _to_sorted_vector(ocalls_))
    {
        out << count << '\t';

        Dl_info info{};
        if (dladdr(func, &info) && info.dli_sname && *info.dli_sname)
            out << info.dli_sname;
        else
            out << func;

        out << '\n';
    }
}

void OcallTracer::dump_backtraces(ostream& out) const
{
    elf64_t elf = ELF64_INIT;

    const oe_enclave_t* loaded_enclave = nullptr;
    const FinalAction unload([&] {
        if (loaded_enclave)
            elf64_unload(&elf);
    });

    out << setfill('0');

    for (const auto& [enclave_and_backtrace, count] :
         _to_sorted_vector(backtraces_))
    {
        const auto& [enclave, backtrace] = enclave_and_backtrace;
        const auto& [enclave_addr, path] = enclaves_.at(enclave);

        // Load enclave elf if not already loaded.
        if (enclave != loaded_enclave)
        {
            if (loaded_enclave)
            {
                elf64_unload(&elf);
                loaded_enclave = nullptr;
            }
            elf = ELF64_INIT;
            if (elf64_load(path.c_str(), &elf) != 0)
                throw runtime_error("elf64_load failed");
            loaded_enclave = enclave;
        }

        out << count << '\n';

        // Get backtrace symbols. Must be free()d.
        const unique_ptr<char*, void (*)(void*)> pBacktrace(
            ert_backtrace_symbols(
                &elf,
                enclave_addr,
                backtrace.data(),
                static_cast<int>(backtrace.size())),
            free);

        if (pBacktrace)
        {
            out << hex;
            for (size_t i = 0; i < backtrace.size(); ++i)
                out << setw(8)
                    << reinterpret_cast<uint64_t>(backtrace[i]) - enclave_addr
                    << "  " << pBacktrace.get()[i] << '\n';
            out << dec;
        }

        out << '\n';
    }
}
