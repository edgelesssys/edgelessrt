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
#include "syscall_u.h"

oe_host_fd_t ert_mmapfs_open_ocall(
    const char* pathname,
    int flags,
    oe_mode_t mode,
    void** addr,
    size_t* file_size)
{
    errno = 0;

    int fd = open(pathname, flags, mode);
    if (fd < 0)
        return fd;

    struct stat file_stat;
    if (fstat(fd, &file_stat) == -1)
    {
        if (close(fd) == -1)
            OE_TRACE_ERROR("close failed");

        return -1;
    }

    size_t len = (size_t)file_stat.st_size;
    *file_size = len;
    if (len == 0)
        len = OE_MMAP_FILE_CHUNK_SIZE;

    len = oe_round_up_to_multiple(len, OE_MMAP_FILE_CHUNK_SIZE);

    int mmap_prot = PROT_READ;
    // extend file in case we want to write/append
    // fd needs ~O_TRUNC & O_RDWR
    if ((flags & O_ACCMODE) == O_RDWR)
    {
        if (ftruncate(fd, (ssize_t)len) == -1)
        {
            if (close(fd) == -1)
                OE_TRACE_ERROR("close failed");
            return -1;
        }

        mmap_prot |= PROT_WRITE;
    }

    *addr = mmap(NULL, len, mmap_prot, MAP_SHARED, fd, 0);
    if (*addr == MAP_FAILED)
    {
        if (close(fd) == -1)
            OE_TRACE_ERROR("close failed");

        return -1;
    }

    return fd;
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

    if (munmap(region, region_size) == -1)
        return false;

    if ((flags & O_ACCMODE) == O_RDWR)
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

    // make sure we extend the file on disk appropriately
    if (ftruncate((int)fd, (off_t)new_region_size) == -1)
        return NULL;

    return mremap(region, cur_region_size, new_region_size, MAP_SHARED);
}
