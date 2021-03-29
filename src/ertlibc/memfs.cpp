// Copyright (c) Edgeless Systems GmbH.
// Licensed under the MIT License.

#include <openenclave/ert.h>
#include <openenclave/internal/syscall/device.h>
#include <cassert>
#include <stdexcept>

extern "C"
{
#include <myst/ramfs.h>
}

using namespace std;
using namespace ert;

Memfs::Memfs(const std::string& devname) : fs_(), ops_(), devid_()
{
    if (devname.empty())
        throw invalid_argument("Memfs: empty devname");

    if (myst_init_ramfs(reinterpret_cast<myst_fs_t**>(&fs_)) != 0)
        throw runtime_error("Memfs: myst_init_ramfs failed");
    assert(fs_);

    auto& fs = *static_cast<myst_fs_t*>(fs_);

#define set(op) ops_.op = reinterpret_cast<decltype(ops_.op)>(fs.fs_##op)
    set(open);
    set(close);
    set(read);
    set(write);
    set(readv);
    set(writev);
    set(pread);
    set(pwrite);
    set(lseek);
    set(fstat);
    set(ftruncate);
    set(getdents64);
    set(mkdir);
    set(rmdir);
    set(link);
    set(unlink);
    set(rename);
#undef set

    devid_ = oe_load_module_custom_file_system(devname.c_str(), &ops_, fs_);
    if (!devid_)
    {
        fs.fs_release(&fs);
        throw runtime_error("Memfs: oe_load_module_custom_file_system failed");
    }
}

Memfs::~Memfs()
{
    int res = oe_device_table_remove(devid_);
    assert(res == 0);

    const auto fs = static_cast<myst_fs_t*>(fs_);
    res = fs->fs_release(fs);
    assert(res == 0);
}
