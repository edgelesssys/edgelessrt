// Copyright (c) Edgeless Systems GmbH.
// Licensed under the MIT License.

#include <openenclave/ert.h>
#include <openenclave/internal/syscall/device.h>
#include <openenclave/internal/trace.h>
#include <cassert>
#include <cstdlib>
#include <stdexcept>
#include "filesystem.h"

using namespace std;
using namespace ert;

Memfs::Memfs(const std::string& devname)
    : impl_(make_unique<memfs::Filesystem>()), ops_(), devid_()
{
    if (devname.empty())
        throw invalid_argument("Memfs: empty devname");

    ops_.open = [](void* context, const char* pathname, bool must_exist) {
        try
        {
            return to_fs(context).open(pathname, must_exist);
        }
        catch (const exception& e)
        {
            OE_TRACE_ERROR("%s", e.what());
            return uintptr_t();
        }
    };

    ops_.close = [](void* context, uintptr_t handle) {
        try
        {
            to_fs(context).close(handle);
        }
        catch (const exception& e)
        {
            OE_TRACE_ERROR("%s", e.what());
        }
    };

    ops_.get_size = [](void* context, uintptr_t handle) {
        try
        {
            return to_fs(context).get_file(handle)->size();
        }
        catch (const exception& e)
        {
            OE_TRACE_ERROR("%s", e.what());
            return uint64_t();
        }
    };

    ops_.unlink = [](void* context, const char* pathname) {
        try
        {
            to_fs(context).unlink(pathname);
        }
        catch (const exception& e)
        {
            OE_TRACE_ERROR("%s", e.what());
        }
    };

    ops_.read = [](void* context,
                   uintptr_t handle,
                   void* buf,
                   uint64_t count,
                   uint64_t offset) {
        try
        {
            to_fs(context).get_file(handle)->read(buf, count, offset);
        }
        catch (const exception& e)
        {
            OE_TRACE_FATAL("%s", e.what());
            abort();
        }
    };

    ops_.write = [](void* context,
                    uintptr_t handle,
                    const void* buf,
                    uint64_t count,
                    uint64_t offset) {
        try
        {
            to_fs(context).get_file(handle)->write(buf, count, offset);
            return true;
        }
        catch (const exception& e)
        {
            OE_TRACE_ERROR("%s", e.what());
            return false;
        }
    };

    devid_ = oe_load_module_custom_file_system(devname.c_str(), &ops_, this);
    if (!devid_)
        throw runtime_error("Memfs: oe_load_module_custom_file_system failed");
}

Memfs::~Memfs()
{
    const int res = oe_device_table_remove(devid_);
    assert(res == 0);
    (void)res;
}

memfs::Filesystem& Memfs::to_fs(void* context)
{
    assert(context);
    return *static_cast<Memfs*>(context)->impl_;
}
