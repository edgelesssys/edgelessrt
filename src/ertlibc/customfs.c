// Copyright (c) Edgeless Systems GmbH.
// Licensed under the MIT License.

#include <openenclave/corelibc/stdio.h>
#include <openenclave/corelibc/stdlib.h>
#include <openenclave/corelibc/string.h>
#include <openenclave/ert.h>
#include <openenclave/internal/hexdump.h>
#include <openenclave/internal/raise.h>
#include <openenclave/internal/safecrt.h>
#include <openenclave/internal/syscall/device.h>
#include <openenclave/internal/syscall/dirent.h>
#include <openenclave/internal/syscall/fcntl.h>
#include <openenclave/internal/syscall/iov.h>
#include <openenclave/internal/syscall/raise.h>
#include <openenclave/internal/syscall/sys/ioctl.h>
#include <openenclave/internal/syscall/sys/mount.h>
#include <openenclave/internal/thread.h>

#define FIONCLEX 0x5450
#define FIOCLEX 0x5451

#define FS_MAGIC 0x5f35f965
#define FILE_MAGIC 0xfe48c6fe

/* Mask to extract the access mode: O_RDONLY, O_WRONLY, O_RDWR. */
#define ACCESS_MODE_MASK 000000003

/* The file system device. */
typedef struct _device
{
    oe_device_t base;

    /* Must be FS_MAGIC. */
    uint32_t magic;

    /* True if this file system has been mounted. */
    bool is_mounted;

    /* The parameters that were passed to the mount() function. */
    struct
    {
        unsigned long flags;
        char source[OE_PATH_MAX];
        char target[OE_PATH_MAX];
    } mount;

    /* True if this device has been created by _fs_clone. */
    bool cloned;

    /* arbitrary value that is passed to the file operation functions */
    void* context;
} device_t;

/* Create by open(). */
typedef struct _file
{
    oe_fd_t base;

    /* Must be FILE_MAGIC. */
    uint32_t magic;

    /* The handle obtained from the custom fs. */
    void* handle;

    const oe_customfs_t* device;
    int flags;
} file_t;

static oe_spinlock_t _lock;

static oe_file_ops_t _get_file_ops(void);

/* Return true if the file system was mounted as read-only. */
OE_INLINE bool _is_read_only(const device_t* fs)
{
    return fs->mount.flags & OE_MS_RDONLY;
}

static device_t* _cast_device(const oe_device_t* device)
{
    device_t* ret = NULL;
    device_t* fs = (device_t*)device;

    if (fs == NULL || fs->magic != FS_MAGIC)
        goto done;

    ret = fs;

done:
    return ret;
}

static file_t* _cast_file(const oe_fd_t* desc)
{
    file_t* ret = NULL;
    file_t* file = (file_t*)desc;

    if (file == NULL || file->magic != FILE_MAGIC)
        OE_RAISE_ERRNO(OE_EINVAL);

    ret = file;

done:
    return ret;
}

/* Expand an enclave path to a host path. */
static int _make_host_path(
    const device_t* fs,
    const char* enclave_path,
    char host_path[OE_PATH_MAX])
{
    const size_t n = OE_PATH_MAX;
    int ret = -1;

    if (oe_strcmp(fs->mount.source, "/") == 0)
    {
        if (oe_strlcpy(host_path, enclave_path, OE_PATH_MAX) >= n)
            OE_RAISE_ERRNO(OE_ENAMETOOLONG);
    }
    else
    {
        if (oe_strlcpy(host_path, fs->mount.source, OE_PATH_MAX) >= n)
            OE_RAISE_ERRNO(OE_ENAMETOOLONG);

        if (oe_strcmp(enclave_path, "/") != 0)
        {
            if (oe_strlcat(host_path, "/", OE_PATH_MAX) >= n)
                OE_RAISE_ERRNO(OE_ENAMETOOLONG);

            if (oe_strlcat(host_path, enclave_path, OE_PATH_MAX) >= n)
                OE_RAISE_ERRNO(OE_ENAMETOOLONG);
        }
    }

    ret = 0;

done:
    return ret;
}

static void* _get_context(const file_t* file)
{
    oe_assert(file);
    return ((device_t*)file->device)->context;
}

static bool _readable(const file_t* file)
{
    oe_assert(file);
    const int acc = file->flags & ACCESS_MODE_MASK;
    return acc == OE_O_RDONLY || acc == OE_O_RDWR;
}

static bool _writable(const file_t* file)
{
    oe_assert(file);
    const int acc = file->flags & ACCESS_MODE_MASK;
    return acc == OE_O_WRONLY || acc == OE_O_RDWR;
}

static int _err_int(int result)
{
    oe_assert(-4096 < result && result <= 0);
    if (result >= 0)
        return result;
    oe_errno = -result;
    return -1;
}

static ssize_t _err_ssize(ssize_t result)
{
    oe_assert(-4096 < result);
    if (result >= 0)
        return result;
    oe_errno = -result;
    return -1;
}

/* Called by oe_mount(). */
static int _fs_mount(
    oe_device_t* device,
    const char* source,
    const char* target,
    const char* filesystemtype,
    unsigned long flags,
    const void* data)
{
    (void)filesystemtype;

    int ret = -1;
    device_t* fs = _cast_device(device);

    /* Fail if required parameters are null. */
    if (!fs || !source || !target)
        OE_RAISE_ERRNO(OE_EINVAL);

    /* Fail if this file system is already mounted. */
    if (fs->is_mounted)
        OE_RAISE_ERRNO(OE_EBUSY);

    /* The data parameter is not supported for host file systems. */
    if (data)
        OE_RAISE_ERRNO(OE_EINVAL);

    /* Remember whether this is a read-only mount. */
    if ((flags & OE_MS_RDONLY))
        fs->mount.flags = flags;

    /* ---------------------------------------------------------------------
     * Only support absolute paths. Hostfs is treated as an external
     * filesystem. As such, it does not make sense to resolve relative paths
     * using the enclave's current working directory.
     * ---------------------------------------------------------------------
     */
    if (source && source[0] != '/')
        OE_RAISE_ERRNO(OE_EINVAL);

    /* Save the source parameter (will be needed to form host paths). */
    oe_strlcpy(fs->mount.source, source, sizeof(fs->mount.source));

    /* Save the target parameter (checked by the umount2() function). */
    oe_strlcpy(fs->mount.target, target, sizeof(fs->mount.target));

    /* Set the flag indicating that this file system is mounted. */
    fs->is_mounted = true;

    ret = 0;

done:
    return ret;
}

/* Called by oe_umount2(). */
static int _fs_umount2(oe_device_t* device, const char* target, int flags)
{
    int ret = -1;
    device_t* fs = _cast_device(device);

    OE_UNUSED(flags);

    /* Fail if any required parameters are null. */
    if (!fs || !target)
        OE_RAISE_ERRNO(OE_EINVAL);

    /* Fail if this file system is not mounted. */
    if (!fs->is_mounted)
        OE_RAISE_ERRNO(OE_EINVAL);

    /* Cross check target parameter with the one passed to mount(). */
    if (oe_strcmp(target, fs->mount.target) != 0)
        OE_RAISE_ERRNO(OE_ENOENT);

    /* Clear the cached mount parameters. */
    oe_memset_s(&fs->mount, sizeof(fs->mount), 0, sizeof(fs->mount));

    /* Set the flag indicating that this file system is mounted. */
    fs->is_mounted = false;

    ret = 0;

done:
    return ret;
}

/* Called by oe_mount() to make a copy of this device. */
static int _fs_clone(oe_device_t* device, oe_device_t** new_device)
{
    int ret = -1;
    device_t* fs = _cast_device(device);
    oe_customfs_t* new_fs = NULL;

    if (!fs || !new_device)
        OE_RAISE_ERRNO(OE_EINVAL);

    if (!(new_fs = oe_calloc(1, sizeof *new_fs)))
        OE_RAISE_ERRNO(OE_ENOMEM);

    *new_fs = *(oe_customfs_t*)device;
    ((device_t*)new_fs)->cloned = true;
    *new_device = (oe_device_t*)new_fs;

    ret = 0;

done:
    return ret;
}

/* Called by oe_umount() to release this device. */
static int _fs_release(oe_device_t* device)
{
    int ret = -1;
    device_t* fs = _cast_device(device);

    if (!fs)
        OE_RAISE_ERRNO(OE_EINVAL);

    if (fs->cloned)
        oe_free(fs);
    ret = 0;

done:
    return ret;
}

static oe_fd_t* _fs_open(
    oe_device_t* device,
    const char* pathname,
    int flags,
    oe_mode_t mode)
{
    oe_fd_t* ret = NULL;
    device_t* fs = _cast_device(device);
    file_t* file = NULL;
    char host_path[OE_PATH_MAX];

    /* Fail if any required parameters are null. */
    if (!fs || !pathname)
        OE_RAISE_ERRNO(OE_EINVAL);

    /* Fail if attempting to write to a read-only file system. */
    if (_is_read_only(fs) && (flags & ACCESS_MODE_MASK) != OE_O_RDONLY)
        OE_RAISE_ERRNO(OE_EPERM);

    /* Create new file struct. */
    {
        if (!(file = oe_calloc(1, sizeof(file_t))))
            OE_RAISE_ERRNO(OE_ENOMEM);

        file->base.type = OE_FD_TYPE_FILE;
        file->magic = FILE_MAGIC;
        file->base.ops.file = _get_file_ops();
        file->device = (oe_customfs_t*)device;
        file->flags = flags;
    }

    /* Ask the host to open the file. */
    {
        if (_make_host_path(fs, pathname, host_path) != 0)
            OE_RAISE_ERRNO_MSG(oe_errno, "pathname=%s", pathname);

        oe_spin_lock(&_lock);
        const int retval = file->device->open(
            fs->context, host_path, flags, mode, NULL, &file->handle);
        oe_spin_unlock(&_lock);
        if (retval)
            OE_RAISE_ERRNO(-retval);
    }

    ret = &file->base;
    file = NULL;

done:

    if (file)
        oe_free(file);

    return ret;
}

static int _fs_fsync(oe_fd_t* desc)
{
    int ret = -1;
    file_t* file = _cast_file(desc);

    if (!file)
        OE_RAISE_ERRNO(OE_EINVAL);

    // noop
    ret = 0;

done:
    return ret;
}

static int _fs_dup(oe_fd_t* desc, oe_fd_t** new_file_out)
{
    int ret = -1;
    file_t* file = _cast_file(desc);

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

static ssize_t _fs_read(oe_fd_t* desc, void* buf, size_t count)
{
    ssize_t ret = -1;
    file_t* file = _cast_file(desc);

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

    if (!_readable(file))
        OE_RAISE_ERRNO(OE_EBADF);

    /* Call the host to perform the read(). */
    oe_spin_lock(&_lock);
    ret = file->device->read(_get_context(file), file->handle, buf, count);
    oe_spin_unlock(&_lock);
    ret = _err_ssize(ret);

    /*
     * Guard the special case that a host sets an arbitrarily large value.
     * The returned value should not exceed count.
     */
    if (ret > (ssize_t)count)
    {
        ret = -1;
        OE_RAISE_ERRNO(OE_EINVAL);
    }

done:
    return ret;
}

/* Called by oe_getdents64() to handle the getdents64 system call. */
static int _fs_getdents64(
    oe_fd_t* desc,
    struct oe_dirent* dirp,
    unsigned int count)
{
    int ret = -1;
    file_t* file = _cast_file(desc);

    if (!file || !dirp)
        OE_RAISE_ERRNO(OE_EINVAL);

    oe_spin_lock(&_lock);
    ret =
        file->device->getdents64(_get_context(file), file->handle, dirp, count);
    oe_spin_unlock(&_lock);
    ret = _err_ssize(ret);

done:
    return ret;
}

static ssize_t _fs_write(oe_fd_t* desc, const void* buf, size_t count)
{
    ssize_t ret = -1;
    file_t* file = _cast_file(desc);

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

    if (!_writable(file))
        OE_RAISE_ERRNO(OE_EBADF);

    /* Call the host. */
    oe_spin_lock(&_lock);
    ret = file->device->write(_get_context(file), file->handle, buf, count);
    oe_spin_unlock(&_lock);
    ret = _err_ssize(ret);

    /*
     * Guard the special case that a host sets an arbitrarily large value.
     * The returned value should not exceed count.
     */
    if (ret > (ssize_t)count)
    {
        ret = -1;
        OE_RAISE_ERRNO(OE_EINVAL);
    }

done:
    return ret;
}

static ssize_t _fs_readv(oe_fd_t* desc, const struct oe_iovec* iov, int iovcnt)
{
    ssize_t ret = -1;
    file_t* file = _cast_file(desc);

    if (!file || (!iov && iovcnt) || iovcnt < 0 || iovcnt > OE_IOV_MAX)
        OE_RAISE_ERRNO(OE_EINVAL);

    if (!_readable(file))
        OE_RAISE_ERRNO(OE_EBADF);

    oe_spin_lock(&_lock);
    ret = file->device->readv(_get_context(file), file->handle, iov, iovcnt);
    oe_spin_unlock(&_lock);
    ret = _err_ssize(ret);

done:
    return ret;
}

static ssize_t _fs_writev(oe_fd_t* desc, const struct oe_iovec* iov, int iovcnt)
{
    ssize_t ret = -1;
    file_t* file = _cast_file(desc);

    if (!file || !iov || iovcnt < 0 || iovcnt > OE_IOV_MAX)
        OE_RAISE_ERRNO(OE_EINVAL);

    if (!_writable(file))
        OE_RAISE_ERRNO(OE_EBADF);

    oe_spin_lock(&_lock);
    ret = file->device->writev(_get_context(file), file->handle, iov, iovcnt);
    oe_spin_unlock(&_lock);
    ret = _err_ssize(ret);

done:
    return ret;
}

static oe_off_t _fs_lseek(oe_fd_t* desc, oe_off_t offset, int whence)
{
    oe_off_t ret = -1;
    file_t* file = _cast_file(desc);

    if (!file)
        OE_RAISE_ERRNO(OE_EINVAL);

    oe_spin_lock(&_lock);
    ret = file->device->lseek(_get_context(file), file->handle, offset, whence);
    oe_spin_unlock(&_lock);
    ret = _err_ssize(ret);

done:
    return ret;
}

static ssize_t _fs_pread(
    oe_fd_t* desc,
    void* buf,
    size_t count,
    oe_off_t offset)
{
    ssize_t ret = -1;
    file_t* file = _cast_file(desc);

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

    if (!_readable(file))
        OE_RAISE_ERRNO(OE_EBADF);

    oe_spin_lock(&_lock);
    ret = file->device->pread(
        _get_context(file), file->handle, buf, count, offset);
    oe_spin_unlock(&_lock);
    ret = _err_ssize(ret);

    /*
     * Guard the special case that a host sets an arbitrarily large value.
     * The returned value should not exceed count.
     */
    if (ret > (ssize_t)count)
    {
        ret = -1;
        OE_RAISE_ERRNO(OE_EINVAL);
    }

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
    file_t* file = _cast_file(desc);

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

    if (!_writable(file))
        OE_RAISE_ERRNO(OE_EBADF);

    oe_spin_lock(&_lock);
    ret = file->device->pwrite(
        _get_context(file), file->handle, buf, count, offset);
    oe_spin_unlock(&_lock);
    ret = _err_ssize(ret);

    /*
     * Guard the special case that a host sets an arbitrarily large value.
     * The returned value should not exceed count.
     */
    if (ret > (ssize_t)count)
    {
        ret = -1;
        OE_RAISE_ERRNO(OE_EINVAL);
    }

done:
    return ret;
}

static int _fs_close(oe_fd_t* desc)
{
    int ret = -1;
    file_t* file = _cast_file(desc);

    if (!file)
        OE_RAISE_ERRNO(OE_EINVAL);

    oe_spin_lock(&_lock);
    ret = file->device->close(_get_context(file), file->handle);
    oe_spin_unlock(&_lock);
    ret = _err_int(ret);

    if (ret == 0)
        oe_free(file);

done:
    return ret;
}

static int _fs_ioctl(oe_fd_t* desc, unsigned long request, uint64_t arg)
{
    (void)arg;

    int ret = -1;
    file_t* file = _cast_file(desc);

    if (!file)
        OE_RAISE_ERRNO(OE_EINVAL);

    switch (request)
    {
        case FIONCLEX:
        case FIOCLEX:
            // can be a noop as we do not support exec()
            ret = 0;
            break;
        default:
            OE_RAISE_ERRNO(OE_ENOTTY);
    }

done:
    return ret;
}

static int _fs_fcntl(oe_fd_t* desc, int cmd, uint64_t arg)
{
    (void)arg;

    int ret = -1;
    file_t* file = _cast_file(desc);

    if (!file)
        OE_RAISE_ERRNO(OE_EINVAL);

    switch (cmd)
    {
        case OE_F_GETFD:
        case OE_F_SETFD:
            // can be a noop as we do not support exec()
            ret = 0;
            break;

        default:
            OE_RAISE_ERRNO(OE_EINVAL);
    }

done:
    return ret;
}

static int _fstat_unlocked(
    const oe_customfs_t* fs,
    void* handle,
    oe_stat_t* statbuf)
{
    oe_assert(fs);
    oe_assert(handle);
    oe_assert(statbuf);

    // struct stat contains unused fields at the end which oe_stat_t does not,
    // so we must wrap it.
    struct
    {
        oe_stat_t buf;
        long unused[3];
    } buf = {.buf = *statbuf};

    const int ret = fs->fstat(((device_t*)fs)->context, handle, &buf);
    *statbuf = buf.buf;
    return _err_int(ret);
}

static int _fs_stat(oe_device_t* device, const char* pathname, oe_stat_t* buf)
{
    int ret = -1;
    device_t* fs = _cast_device(device);
    char host_path[OE_PATH_MAX];

    if (buf)
        oe_memset_s(buf, sizeof(*buf), 0, sizeof(*buf));

    if (!fs || !pathname || !buf)
        OE_RAISE_ERRNO(OE_EINVAL);

    if (_make_host_path(fs, pathname, host_path) != 0)
        OE_RAISE_ERRNO(oe_errno);

    const oe_customfs_t* const customfs = (oe_customfs_t*)device;
    void* handle = NULL;

    oe_spin_lock(&_lock);
    if (customfs->open(fs->context, host_path, OE_O_RDONLY, 0, NULL, &handle) ==
        0)
    {
        ret = _fstat_unlocked(customfs, handle, buf);
        customfs->close(fs->context, handle);
    }
    else
        oe_errno = OE_ENOENT;
    oe_spin_unlock(&_lock);

done:

    return ret;
}

static int _fs_fstat(oe_fd_t* desc, struct oe_stat_t* buf)
{
    int ret = -1;
    file_t* file = _cast_file(desc);

    if (buf)
        oe_memset_s(buf, sizeof(*buf), 0, sizeof(*buf));

    if (!file || !buf)
        OE_RAISE_ERRNO(OE_EINVAL);

    oe_spin_lock(&_lock);
    ret = _fstat_unlocked(file->device, file->handle, buf);
    oe_spin_unlock(&_lock);

done:

    return ret;
}

static int _fs_access(oe_device_t* device, const char* pathname, int mode)
{
    int ret = -1;
    device_t* fs = _cast_device(device);
    const uint32_t MASK = (OE_R_OK | OE_W_OK | OE_X_OK);

    if (!fs || !pathname || ((uint32_t)mode & ~MASK))
        OE_RAISE_ERRNO(OE_EINVAL);

    OE_RAISE_ERRNO(OE_ENOSYS);

done:

    return ret;
}

static int _fs_link(
    oe_device_t* device,
    const char* oldpath,
    const char* newpath)
{
    int ret = -1;
    device_t* fs = _cast_device(device);
    char host_oldpath[OE_PATH_MAX];
    char host_newpath[OE_PATH_MAX];

    if (!fs || !oldpath || !newpath)
        OE_RAISE_ERRNO(OE_EINVAL);

    /* Fail if attempting to write to a read-only file system. */
    if (_is_read_only(fs))
        OE_RAISE_ERRNO(OE_EPERM);

    if (_make_host_path(fs, oldpath, host_oldpath) != 0)
        OE_RAISE_ERRNO(oe_errno);

    if (_make_host_path(fs, newpath, host_newpath) != 0)
        OE_RAISE_ERRNO(oe_errno);

    oe_spin_lock(&_lock);
    ret =
        ((oe_customfs_t*)device)->link(fs->context, host_oldpath, host_newpath);
    oe_spin_unlock(&_lock);
    ret = _err_int(ret);

done:

    return ret;
}

static int _fs_unlink(oe_device_t* device, const char* pathname)
{
    int ret = -1;
    device_t* fs = _cast_device(device);
    char host_path[OE_PATH_MAX];

    if (!fs)
        OE_RAISE_ERRNO(OE_EINVAL);

    /* Fail if attempting to write to a read-only file system. */
    if (_is_read_only(fs))
        OE_RAISE_ERRNO(OE_EPERM);

    if (_make_host_path(fs, pathname, host_path) != 0)
        OE_RAISE_ERRNO(oe_errno);

    oe_spin_lock(&_lock);
    ret = ((oe_customfs_t*)device)->unlink(fs->context, host_path);
    oe_spin_unlock(&_lock);
    ret = _err_int(ret);

done:

    return ret;
}

static int _fs_rename(
    oe_device_t* device,
    const char* oldpath,
    const char* newpath)
{
    int ret = -1;
    device_t* fs = _cast_device(device);
    char host_oldpath[OE_PATH_MAX];
    char host_newpath[OE_PATH_MAX];

    if (!fs || !oldpath || !newpath)
        OE_RAISE_ERRNO(OE_EINVAL);

    if (_is_read_only(fs))
        OE_RAISE_ERRNO(OE_EPERM);

    if (_make_host_path(fs, oldpath, host_oldpath) != 0)
        OE_RAISE_ERRNO(oe_errno);

    if (_make_host_path(fs, newpath, host_newpath) != 0)
        OE_RAISE_ERRNO(oe_errno);

    oe_spin_lock(&_lock);
    ret = ((oe_customfs_t*)device)
              ->rename(fs->context, host_oldpath, host_newpath);
    oe_spin_unlock(&_lock);
    ret = _err_int(ret);

done:

    return ret;
}

static int _fs_truncate(oe_device_t* device, const char* path, oe_off_t length)
{
    int ret = -1;
    device_t* fs = _cast_device(device);
    char host_path[OE_PATH_MAX];

    if (!fs)
        OE_RAISE_ERRNO(OE_EINVAL);

    if (_is_read_only(fs))
        OE_RAISE_ERRNO(OE_EPERM);

    if (_make_host_path(fs, path, host_path) != 0)
        OE_RAISE_ERRNO(oe_errno);

    const oe_customfs_t* const customfs = (oe_customfs_t*)device;
    void* handle = NULL;

    oe_spin_lock(&_lock);
    if (customfs->open(fs->context, host_path, OE_O_WRONLY, 0, NULL, &handle) ==
        0)
    {
        ret = customfs->ftruncate(fs->context, handle, length);
        customfs->close(fs->context, handle);
        ret = _err_int(ret);
    }
    else
        oe_errno = OE_ENOENT;
    oe_spin_unlock(&_lock);

done:

    return ret;
}

static int _fs_ftruncate(oe_fd_t* desc, oe_off_t length)
{
    int ret = -1;
    const file_t* const file = _cast_file(desc);

    if (!file)
        OE_RAISE_ERRNO(OE_EINVAL);

    if (!_writable(file))
        OE_RAISE_ERRNO(OE_EBADF);

    oe_spin_lock(&_lock);
    ret = file->device->ftruncate(_get_context(file), file->handle, length);
    oe_spin_unlock(&_lock);
    ret = _err_int(ret);

done:
    return ret;
}

static int _fs_mkdir(oe_device_t* device, const char* pathname, oe_mode_t mode)
{
    int ret = -1;
    device_t* fs = _cast_device(device);
    char host_path[OE_PATH_MAX];

    if (!fs)
        OE_RAISE_ERRNO(OE_EINVAL);

    /* Fail if attempting to write to a read-only file system. */
    if (_is_read_only(fs))
        OE_RAISE_ERRNO(OE_EPERM);

    if (_make_host_path(fs, pathname, host_path) != 0)
        OE_RAISE_ERRNO(oe_errno);

    oe_spin_lock(&_lock);
    ret = ((oe_customfs_t*)device)->mkdir(fs->context, host_path, mode);
    oe_spin_unlock(&_lock);
    ret = _err_int(ret);

done:

    return ret;
}

static int _fs_rmdir(oe_device_t* device, const char* pathname)
{
    int ret = -1;
    device_t* fs = _cast_device(device);
    char host_path[OE_PATH_MAX];

    if (!fs)
        OE_RAISE_ERRNO(OE_EINVAL);

    /* Fail if attempting to write to a read-only file system. */
    if (_is_read_only(fs))
        OE_RAISE_ERRNO(OE_EPERM);

    if (_make_host_path(fs, pathname, host_path) != 0)
        OE_RAISE_ERRNO(oe_errno);

    oe_spin_lock(&_lock);
    ret = ((oe_customfs_t*)device)->rmdir(fs->context, host_path);
    oe_spin_unlock(&_lock);
    ret = _err_int(ret);

done:

    return ret;
}

static oe_host_fd_t _fs_get_host_fd(oe_fd_t* desc)
{
    // currently only called by epoll
    (void)desc;
    OE_RAISE_ERRNO(OE_ENOSYS);
done:
    return -1;
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

static oe_file_ops_t _get_file_ops(void)
{
    return _file_ops;
};

uint64_t oe_load_module_custom_file_system(
    const char* devname,
    oe_customfs_t* ops,
    void* context)
{
    oe_assert(devname && *devname);
    oe_assert(ops);

    OE_STATIC_ASSERT(sizeof(device_t) == sizeof ops->reserved);
    device_t* const dev = (device_t*)ops->reserved;
    memset(dev, 0, sizeof *dev);

    dev->base.type = OE_DEVICE_TYPE_FILE_SYSTEM;
    dev->base.name = devname;

    dev->base.ops.device.release = _fs_release;
    dev->base.ops.fs.clone = _fs_clone;
    dev->base.ops.fs.mount = _fs_mount;
    dev->base.ops.fs.umount2 = _fs_umount2;
    dev->base.ops.fs.open = _fs_open;
    dev->base.ops.fs.stat = _fs_stat;
    dev->base.ops.fs.access = _fs_access;
    dev->base.ops.fs.link = _fs_link;
    dev->base.ops.fs.unlink = _fs_unlink;
    dev->base.ops.fs.rename = _fs_rename;
    dev->base.ops.fs.truncate = _fs_truncate;
    dev->base.ops.fs.mkdir = _fs_mkdir;
    dev->base.ops.fs.rmdir = _fs_rmdir;

    dev->context = context;
    dev->magic = FS_MAGIC;

    const uint64_t devid = oe_device_table_get_custom_devid();
    if (oe_device_table_set(devid, &dev->base) != 0)
        return 0;

    return devid;
}
