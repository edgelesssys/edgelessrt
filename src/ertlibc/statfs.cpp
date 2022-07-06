// Copyright (c) Edgeless Systems GmbH.
// Licensed under the MIT License.

#include <cerrno>
#include <cstring>
#include <stdexcept>
#include <string_view>
#include "ertlibc_t.h"
#include "syscalls.h"

using namespace std;
using namespace ert;

int sc::fstatfs(int fd, struct statfs* buf)
{
    if (fd < 0)
        return -EBADF;
    if (!buf)
        return -EFAULT;

    memset(buf, 0, sizeof *buf);
    auto& b = *buf;

    // return sane dummy values that should satisfy most callers
    b.f_type = 0x858458f6; // RAMFS_MAGIC
    b.f_bsize = 4096;
    b.f_blocks = 20000000;
    b.f_bfree = 10000000;
    b.f_bavail = 10000000;
    b.f_namelen = 255;
    b.f_frsize = 4096;

    return 0;
}

int sc::statfs(const char* path, struct statfs* buf)
{
    if (!path || !buf)
        return -EFAULT;
    if (!*path)
        return -ENOENT;

    // By default, statfs returns dummy values (see fstatfs).
    // Caller can request statfs from host by using this prefix.
    const string_view prefix = "/edg/hostfs/";
    const string_view spath = path;
    if (spath.compare(0, prefix.size(), prefix) != 0)
        return sc::fstatfs(0, buf);

    // see ertlibc.edl
    static_assert(sizeof(struct statfs) == 15 * sizeof(uint64_t));
    int ret = -1;
    if (ert_statfs_ocall(
            &ret, path + prefix.size() - 1, reinterpret_cast<uint64_t*>(buf)) !=
        OE_OK)
        throw logic_error("ert_statfs_ocall");
    if (ret != 0)
    {
        if (errno)
            return -errno;
        return ret;
    }

    // sanity checks
    auto& b = *buf;
    if (b.f_bsize < 0 || b.f_namelen < 0 || b.f_frsize < 0 ||
        !(b.f_bavail <= b.f_bfree && b.f_bfree <= b.f_blocks) ||
        b.f_ffree > b.f_files)
        return -ENOSYS;

    return 0;
}
