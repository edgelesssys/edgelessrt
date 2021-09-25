// Copyright (c) Edgeless Systems GmbH.
// Licensed under the MIT License.

#include "../common/mmapfs.h"
#include <fcntl.h>
#include <openenclave/internal/syscall/types.h>
#include <openenclave/internal/trace.h>
#include <openenclave/internal/utils.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <cerrno>
#include <cstddef>
#include <string_view>
#include "fd.h"
#include "syscall_u.h"

using namespace std;
using namespace ert;

static bool _writable(int flags)
{
    return (flags & O_ACCMODE) == O_RDWR;
}

oe_host_fd_t ert_mmapfs_open_ocall(
    const char* pathname,
    int flags,
    oe_mode_t mode,
    void** addr,
    size_t* file_size)
{
    errno = 0;

    FileDescriptor fd = open(pathname, flags, mode);
    if (!fd)
        return fd;

    struct stat file_stat;
    if (fstat(fd, &file_stat) == -1)
        return -1;

    *file_size = static_cast<size_t>(file_stat.st_size);

    constexpr string_view file_size_tag = "EDG_SIZE";
    static_assert(sizeof ert_mmapfs_file_size_t::tag == file_size_tag.size());

    // recover real file size from appended ert_mmapfs_file_size_t if file has
    // not been closed properly
    if (*file_size >= sizeof(ert_mmapfs_file_size_t))
    {
        if (lseek(
                fd,
                -static_cast<off_t>(sizeof(ert_mmapfs_file_size_t)),
                SEEK_END) < 0)
            return -1;

        ert_mmapfs_file_size_t stored_file_size{};
        if (read(fd, &stored_file_size, sizeof stored_file_size) !=
            sizeof stored_file_size)
            return -1;

        if (file_size_tag.compare(
                0,
                file_size_tag.size(),
                stored_file_size.tag,
                file_size_tag.size()) == 0)
        {
            if (stored_file_size.size > *file_size - sizeof stored_file_size)
                return -1;
            *file_size = stored_file_size.size;
        }
    }

    // calculate map length
    size_t len = *file_size;
    if (len == 0)
        len = OE_MMAP_FILE_CHUNK_SIZE;
    len = oe_round_up_to_multiple(len, OE_MMAP_FILE_CHUNK_SIZE);
    const size_t pos_file_size = len;

    int mmap_prot = PROT_READ;
    // extend file in case we want to write/append
    // fd needs ~O_TRUNC & O_RDWR
    if (_writable(flags))
    {
        len += sizeof(ert_mmapfs_file_size_t);
        if (ftruncate(fd, (ssize_t)len) == -1)
            return -1;
        mmap_prot |= PROT_WRITE;
    }

    *addr = mmap(NULL, len, mmap_prot, MAP_SHARED, fd, 0);
    if (*addr == MAP_FAILED)
        return -1;

    if (_writable(flags))
    {
        // append file size, which will be kept up-to-date by the enclave
        auto& stored_file_size = *reinterpret_cast<ert_mmapfs_file_size_t*>(
            static_cast<byte*>(*addr) + pos_file_size);
        memcpy(
            stored_file_size.tag, file_size_tag.data(), file_size_tag.size());
        stored_file_size.size = *file_size;
    }

    return fd.release();
}

bool ert_mmapfs_close_ocall(
    oe_host_fd_t fd,
    int flags,
    void* region,
    size_t file_size)
{
    errno = 0;
    size_t region_size = OE_MMAP_FILE_CHUNK_SIZE;
    if (file_size != 0)
        region_size =
            oe_round_up_to_multiple(file_size, OE_MMAP_FILE_CHUNK_SIZE);
    if (_writable(flags))
        region_size += sizeof(ert_mmapfs_file_size_t);

    if (munmap(region, region_size) == -1)
        return false;

    if (_writable(flags))
    {
        if (ftruncate((int)fd, (off_t)file_size) == -1)
        {
            // file size stays aligned to MMAP_FILE_CHUNK_SIZE
            OE_TRACE_ERROR("ftruncate host fd failed");
        }
    }

    if (close((int)fd) == -1)
        OE_TRACE_ERROR("close on host fd failed");

    return true;
}

void* ert_mmapfs_extend_ocall(
    oe_host_fd_t fd,
    void* region,
    size_t file_size,
    size_t new_region_size)
{
    errno = 0;
    size_t cur_region_size =
        oe_round_up_to_multiple(file_size, OE_MMAP_FILE_CHUNK_SIZE);
    new_region_size += sizeof(ert_mmapfs_file_size_t);

    // make sure we extend the file on disk appropriately
    if (ftruncate((int)fd, (off_t)new_region_size) == -1)
        return NULL;

    const auto result =
        mremap(region, cur_region_size, new_region_size, MREMAP_MAYMOVE);
    if (result == MAP_FAILED)
        return result;

    // copy the appended file size to the new end
    const auto p = static_cast<byte*>(result);
    memcpy(
        p + new_region_size - sizeof(ert_mmapfs_file_size_t),
        p + cur_region_size,
        sizeof(ert_mmapfs_file_size_t));

    return result;
}
