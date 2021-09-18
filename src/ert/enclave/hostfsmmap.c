// Copyright (c) Edgeless Systems GmbH.
// Licensed under the MIT License.

#include <openenclave/corelibc/assert.h>
#include <openenclave/corelibc/errno.h>
#include <openenclave/corelibc/stdio.h>
#include <openenclave/corelibc/stdlib.h>
#include <openenclave/corelibc/string.h>
#include <openenclave/internal/ert/hostfs.h>
#include <openenclave/internal/safecrt.h>
#include <openenclave/internal/syscall/fcntl.h>
#include <openenclave/internal/syscall/iov.h>
#include <openenclave/internal/syscall/raise.h>
#include <openenclave/internal/syscall/sys/mount.h>
#include <openenclave/internal/thread.h>
#include <openenclave/internal/utils.h>
#include "../common/mmapfs.h"
#include "syscall_t.h"

typedef struct _file
{
    oe_fd_t base;
    oe_host_fd_t host_fd;
    int open_flags;
    // mmap base returned by host mmap()
    char* region_base;
    // position in file starting from region_base
    size_t file_position;
    // size of actual file
    size_t file_size;

    oe_spinlock_t lock;
} file_t;

static int _fs_fsync(oe_fd_t* desc)
{
    int ret = -1;
    file_t* file = (file_t*)desc;

    if (!file)
        OE_RAISE_ERRNO(OE_EINVAL);

    // noop for now
    ret = 0;

done:
    return ret;
}

static int _fs_dup(oe_fd_t* desc, oe_fd_t** new_file_out)
{
    int ret = -1;
    file_t* file = (file_t*)desc;

    if (!new_file_out)
        OE_RAISE_ERRNO(OE_EINVAL);

    *new_file_out = NULL;

    /* Check parameters. */
    if (!file)
        OE_RAISE_ERRNO(OE_EINVAL);

    OE_RAISE_ERRNO(OE_ENOSYS);

done:
    return ret;
}

// caller must hold the file lock
static ssize_t _mmap_file_read(
    const file_t* mmap_file,
    void* buf,
    size_t count,
    size_t offset)
{
    if (offset > mmap_file->file_size)
        return 0;

    const size_t max_count = mmap_file->file_size - offset;
    if (count > max_count)
        count = max_count;

    memcpy(buf, mmap_file->region_base + offset, count);

    return (ssize_t)count;
}

static ssize_t _fs_read(oe_fd_t* desc, void* buf, size_t count)
{
    ssize_t ret = -1;
    file_t* file = (file_t*)desc;

    /*
     * According to the POSIX specification, when the count is greater
     * than SSIZE_MAX, the result is implementation-defined. OE raises an
     * error in this case.
     * Refer to
     * https://pubs.opengroup.org/onlinepubs/9699919799/functions/read.html for
     * for more detail.
     */
    if (!file || count > OE_SSIZE_MAX)
        OE_RAISE_ERRNO(OE_EINVAL);

    oe_spin_lock(&file->lock);
    ret = _mmap_file_read(file, buf, count, file->file_position);
    file->file_position += (size_t)ret;
    oe_spin_unlock(&file->lock);

done:
    return ret;
}

static int _fs_getdents64(
    oe_fd_t* desc,
    struct oe_dirent* dirp,
    unsigned int count)
{
    (void)desc;
    (void)dirp;
    (void)count;
    OE_RAISE_ERRNO(OE_EINVAL);
done:
    return -1;
}

// caller must hold the file lock
static bool _extend(file_t* mmap_file, size_t count, size_t offset)
{
    bool ret = false;

    size_t region_size = OE_MMAP_FILE_CHUNK_SIZE;
    if (mmap_file->file_size != 0)
        region_size = oe_round_up_to_multiple(
            mmap_file->file_size, OE_MMAP_FILE_CHUNK_SIZE);

    if (offset >= region_size || count > region_size - offset)
    {
        size_t new_region_size =
            oe_round_up_to_multiple(count + offset, OE_MMAP_FILE_CHUNK_SIZE);

        void* new_region = NULL;
        if (ert_mmapfs_extend_ocall(
                &new_region,
                mmap_file->host_fd,
                mmap_file->region_base,
                mmap_file->file_size,
                new_region_size) != OE_OK)
            OE_RAISE_ERRNO(OE_EINVAL);

        if (new_region == (void*)-1 || new_region == NULL)
            OE_RAISE_ERRNO(oe_errno);

        if (!oe_is_outside_enclave(new_region, new_region_size))
            oe_abort();

        mmap_file->region_base = new_region;
    }

    ret = true;

done:
    return ret;
}

// caller must hold the file lock
static ssize_t _mmap_file_write(
    file_t* mmap_file,
    const void* buf,
    size_t count,
    size_t offset)
{
    ssize_t ret = -1;

    if ((mmap_file->open_flags & ACCESS_MODE_MASK) != OE_O_RDWR)
        OE_RAISE_ERRNO(OE_EBADF); // fd was not opened for writing

    if (!_extend(mmap_file, count, offset))
        OE_RAISE_ERRNO(oe_errno);

    memcpy(mmap_file->region_base + offset, buf, count);

    if (offset + count > mmap_file->file_size)
        mmap_file->file_size = offset + count; // EOF

    ret = (ssize_t)count;

done:
    return ret;
}

static ssize_t _fs_write(oe_fd_t* desc, const void* buf, size_t count)
{
    ssize_t ret = -1;
    file_t* file = (file_t*)desc;

    /*
     * Check parameters.
     * According to the POSIX specification, when the count is greater
     * than SSIZE_MAX, the result is implementation-defined. OE raises an
     * error in this case.
     * Refer to
     * https://pubs.opengroup.org/onlinepubs/9699919799/functions/write.html for
     * for more detail.
     */
    if (!file || (count && !buf) || count > OE_SSIZE_MAX)
        OE_RAISE_ERRNO(OE_EINVAL);

    oe_spin_lock(&file->lock);
    ret = _mmap_file_write(file, buf, count, file->file_position);
    if (ret > 0)
        file->file_position += (size_t)ret;
    oe_spin_unlock(&file->lock);

done:
    return ret;
}

static ssize_t _fs_readv(oe_fd_t* desc, const struct oe_iovec* iov, int iovcnt)
{
    ssize_t ret = -1;
    file_t* file = (file_t*)desc;

    if (!file || (!iov && iovcnt) || iovcnt < 0 || iovcnt > OE_IOV_MAX)
        OE_RAISE_ERRNO(OE_EINVAL);

    oe_spin_lock(&file->lock);

    if (file->file_position >= file->file_size)
    {
        oe_spin_unlock(&file->lock);
        return 0;
    }

    size_t max_count = file->file_size - file->file_position;
    if (max_count > OE_SSIZE_MAX)
        max_count = OE_SSIZE_MAX;

    const char* const src = file->region_base + file->file_position;
    size_t bytes_read = 0;

    for (int i = 0; i < iovcnt; ++i)
    {
        size_t len = iov[i].iov_len;
        if (bytes_read + len > max_count)
        {
            len = max_count - bytes_read;
            if (!len)
                break;
        }

        memcpy(iov[i].iov_base, src + bytes_read, len);
        bytes_read += len;
    }

    file->file_position += bytes_read;

    oe_spin_unlock(&file->lock);

    ret = (ssize_t)bytes_read;

done:
    return ret;
}

static ssize_t _fs_writev(oe_fd_t* desc, const struct oe_iovec* iov, int iovcnt)
{
    ssize_t ret = -1;
    bool locked = false;
    file_t* file = (file_t*)desc;

    if (!file || !iov || iovcnt < 0 || iovcnt > OE_IOV_MAX)
        OE_RAISE_ERRNO(OE_EINVAL);

    if ((file->open_flags & ACCESS_MODE_MASK) != OE_O_RDWR)
        OE_RAISE_ERRNO(OE_EBADF); // fd was not opened for writing

    // calculate the sum of bytes to be written
    size_t count = 0;
    for (int i = 0; i < iovcnt; ++i)
        count += iov[i].iov_len;

    locked = true;
    oe_spin_lock(&file->lock);

    // extend the file if needed
    if (!_extend(file, count, file->file_position))
        OE_RAISE_ERRNO(oe_errno);

    char* const dst = file->region_base + file->file_position;
    size_t bytes_written = 0;

    for (int i = 0; i < iovcnt; ++i)
    {
        const size_t len = iov[i].iov_len;
        if (len > OE_SSIZE_MAX)
        {
            if (bytes_written)
                break;
            OE_RAISE_ERRNO(OE_EFBIG);
        }

        memcpy(dst + bytes_written, iov[i].iov_base, len);
        bytes_written += len;
    }

    file->file_position += bytes_written;
    if (file->file_position > file->file_size)
        file->file_size = file->file_position; // EOF

    ret = (ssize_t)bytes_written;

done:
    if (locked)
        oe_spin_unlock(&file->lock);
    return ret;
}

static oe_off_t _fs_lseek(oe_fd_t* desc, oe_off_t offset, int whence)
{
    oe_off_t ret = -1;
    bool locked = false;
    file_t* file = (file_t*)desc;

    if (!file)
        OE_RAISE_ERRNO(OE_EINVAL);

    locked = true;
    oe_spin_lock(&file->lock);

    // TODO: SEEK_DATA, SEEK_HOLE
    switch (whence)
    {
        case OE_SEEK_SET:
            break;
        case OE_SEEK_CUR:
            offset += (oe_off_t)file->file_position;
            break;
        case OE_SEEK_END:
            offset += (oe_off_t)file->file_size;
            break;
        default:
            OE_RAISE_ERRNO(OE_EINVAL);
    }

    if (offset < 0)
        OE_RAISE_ERRNO(OE_EINVAL);

    file->file_position = (size_t)offset;
    ret = offset;

done:
    if (locked)
        oe_spin_unlock(&file->lock);
    return ret;
}

static ssize_t _fs_pread(
    oe_fd_t* desc,
    void* buf,
    size_t count,
    oe_off_t offset)
{
    ssize_t ret = -1;
    file_t* file = (file_t*)desc;

    /*
     * According to the POSIX specification, when the count is greater
     * than SSIZE_MAX, the result is implementation-defined. OE raises an
     * error in this case.
     * Refer to
     * https://pubs.opengroup.org/onlinepubs/9699919799/functions/pread.html for
     * for more detail.
     */
    if (!file || count > OE_SSIZE_MAX)
        OE_RAISE_ERRNO(OE_EINVAL);

    oe_spin_lock(&file->lock);
    ret = _mmap_file_read(file, buf, count, (size_t)offset);
    oe_spin_unlock(&file->lock);

done:
    return ret;
}

static ssize_t _fs_pwrite(
    oe_fd_t* desc,
    const void* buf,
    size_t count,
    oe_off_t offset)
{
    ssize_t ret = -1;
    file_t* file = (file_t*)desc;

    /*
     * According to the POSIX specification, when the count is greater
     * than SSIZE_MAX, the result is implementation-defined. OE raises an
     * error in this case.
     * Refer to
     * https://pubs.opengroup.org/onlinepubs/9699919799/functions/pwrite.html
     * for more detail.
     */
    if (!file || count > OE_SSIZE_MAX)
        OE_RAISE_ERRNO(OE_EINVAL);

    oe_spin_lock(&file->lock);
    ret = _mmap_file_write(file, buf, count, (size_t)offset);
    oe_spin_unlock(&file->lock);

done:
    return ret;
}

static int _fs_close(oe_fd_t* desc)
{
    int ret = -1;
    file_t* file = (file_t*)desc;

    if (!file)
        OE_RAISE_ERRNO(OE_EINVAL);

    bool success = false;
    if (ert_mmapfs_close_ocall(
            &success,
            file->host_fd,
            file->open_flags,
            file->region_base,
            file->file_size) != OE_OK)
        OE_RAISE_ERRNO(OE_EINVAL);

    if (!success)
        OE_RAISE_ERRNO(oe_errno);

    file->region_base = NULL;

    ret = 0;
    oe_free(file);

done:
    return ret;
}

static int _fs_ioctl(oe_fd_t* desc, unsigned long request, uint64_t arg)
{
    (void)request;
    (void)arg;
    int ret = -1;
    file_t* file = (file_t*)desc;

    if (!file)
        OE_RAISE_ERRNO(OE_EINVAL);

    /*
     * MUSL uses the TIOCGWINSZ ioctl request to determine whether the file
     * descriptor refers to a terminal device. This request cannot be handled
     * by Windows hosts, so the error is handled on the enclave side. This is
     * the correct behavior since host files are not terminal devices.
     */
    switch (request)
    {
        default:
            OE_RAISE_ERRNO(OE_ENOTTY);
    }

    OE_RAISE_ERRNO(OE_ENOSYS);

done:
    return ret;
}

static int _fs_fcntl(oe_fd_t* desc, int cmd, uint64_t arg)
{
    (void)cmd;
    (void)arg;
    int ret = -1;
    file_t* file = (file_t*)desc;

    if (!file)
        OE_RAISE_ERRNO(OE_EINVAL);

    if (cmd == OE_F_SETLK)
        return 0; // can be a noop as we do not support multiple processes

    OE_RAISE_ERRNO(OE_ENOSYS);

done:
    return ret;
}

static int _fs_fstat(oe_fd_t* desc, struct oe_stat_t* buf)
{
    int ret = -1;
    file_t* file = (file_t*)desc;
    int retval = -1;

    if (buf)
        oe_memset_s(buf, sizeof(*buf), 0, sizeof(*buf));

    if (!file || !buf)
        OE_RAISE_ERRNO(OE_EINVAL);

    if (oe_syscall_fstat_ocall(&retval, file->host_fd, buf) != OE_OK)
        OE_RAISE_ERRNO(OE_EINVAL);

    ret = retval;

done:

    return ret;
}

static int _fs_ftruncate(oe_fd_t* desc, oe_off_t length)
{
    (void)length;
    int ret = -1;
    file_t* const file = (file_t*)desc;

    if (!file)
        OE_RAISE_ERRNO(OE_EINVAL);

    if (length < 0)
        OE_RAISE_ERRNO(OE_EINVAL);

    if ((file->open_flags & ACCESS_MODE_MASK) != OE_O_RDWR)
        OE_RAISE_ERRNO(OE_EBADF); // fd was not opened for writing

    oe_spin_lock(&file->lock);
    file->file_size = (size_t)length;
    oe_spin_unlock(&file->lock);

    ret = 0;

done:
    return ret;
}

static oe_host_fd_t _fs_get_host_fd(oe_fd_t* desc)
{
    file_t* file = (file_t*)desc;

    return file ? file->host_fd : -1;
}

static oe_file_ops_t _file_ops = {
    .fd.read = _fs_read,
    .fd.write = _fs_write,
    .fd.readv = _fs_readv,
    .fd.writev = _fs_writev,
    .fd.dup = _fs_dup,
    .fd.ioctl = _fs_ioctl,
    .fd.fcntl = _fs_fcntl,
    .fd.close = _fs_close,
    .fd.get_host_fd = _fs_get_host_fd,
    .lseek = _fs_lseek,
    .pread = _fs_pread,
    .pwrite = _fs_pwrite,
    .getdents64 = _fs_getdents64,
    .fstat = _fs_fstat,
    .ftruncate = _fs_ftruncate,
    .fsync = _fs_fsync,
    .fdatasync = _fs_fsync,
};

static bool _mmap_file_open(
    file_t* mmap_file,
    const char* pathname,
    int flags,
    oe_mode_t mode)
{
    bool ret = false;

    if (flags & OE_O_WRONLY)
    {
        flags &= ~OE_O_WRONLY; // ftruncate chokes on (O_WRONLY|O_RDWR)
        flags |= OE_O_RDWR;    // mmap does not open O_WRONLY fds.
    }

    mmap_file->open_flags = flags;

    if (ert_mmapfs_open_ocall(
            &mmap_file->host_fd,
            pathname,
            flags,
            mode,
            (void**)&mmap_file->region_base,
            &mmap_file->file_size) != OE_OK)
        OE_RAISE_ERRNO(OE_EINVAL);

    if (mmap_file->host_fd < 0)
        OE_RAISE_ERRNO(oe_errno);

    if (flags & OE_O_APPEND)
        mmap_file->file_position = mmap_file->file_size;

    ret = true;

done:
    return ret;
}

static oe_fd_t* _fs_open(
    oe_device_t* device,
    const char* pathname,
    int flags,
    oe_mode_t mode)
{
    // It is valid to open a directory without O_DIRECTORY flag, so we cannot
    // know if pathname is a directory or a file. Try to open pathname as a
    // directory first.
    oe_fd_t* ret =
        _hostfs_open_directory(device, pathname, flags | OE_O_DIRECTORY);
    if (ret || (flags & OE_O_DIRECTORY))
        return ret;

    // Not a directory, open as a file.

    device_t* fs = (device_t*)device;
    file_t* file = NULL;
    char host_path[OE_PATH_MAX];

    /* Fail if any required parameters are null. */
    if (!fs || !pathname)
        OE_RAISE_ERRNO(OE_EINVAL);

    /* Fail if attempting to write to a read-only file system. */
    if ((fs->mount.flags & OE_MS_RDONLY) &&
        (flags & ACCESS_MODE_MASK) != OE_O_RDONLY)
        OE_RAISE_ERRNO(OE_EPERM);

    /* Create new file struct. */
    {
        if (!(file = oe_calloc(1, sizeof(file_t))))
            OE_RAISE_ERRNO(OE_ENOMEM);

        file->base.type = OE_FD_TYPE_FILE;
        file->base.ops.file = _file_ops;
    }

    /* Ask the host to open the file. */
    {
        if (_hostfs_make_host_path(fs, pathname, host_path) != 0)
            OE_RAISE_ERRNO_MSG(oe_errno, "pathname=%s", pathname);

        if (!_mmap_file_open(file, host_path, flags, mode))
            goto done;

        oe_assert(file->region_base != NULL);
        if (!oe_is_outside_enclave(file->region_base, file->file_size))
            oe_abort();
    }

    ret = &file->base;
    file = NULL;

done:

    if (file)
        oe_free(file);

    return ret;
}

device_t oe_hostfs_mmap = {
    .base.type = OE_DEVICE_TYPE_FILE_SYSTEM,
    .base.name = OE_HOST_FILE_SYSTEM_MMAP,
    .base.ops.fs =
        {
            .base.release = _hostfs_release,
            .clone = _hostfs_clone,
            .mount = _hostfs_mount,
            .umount2 = _hostfs_umount2,
            .open = _fs_open,
            .stat = _hostfs_stat,
            .access = _hostfs_access,
            .link = _hostfs_link,
            .unlink = _hostfs_unlink,
            .rename = _hostfs_rename,
            .truncate = _hostfs_truncate,
            .mkdir = _hostfs_mkdir,
            .rmdir = _hostfs_rmdir,
        },
    .magic = FS_MAGIC,
    .mount =
        {
            .source = {'/'},
        },
};
