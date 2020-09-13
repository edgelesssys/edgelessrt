// Copyright (c) Edgeless Systems GmbH.
// Licensed under the MIT License.

// clang-format off
#include <openenclave/enclave.h>
// clang-format on

#include <openenclave/internal/syscall/device.h>
#include <openenclave/internal/thread.h>
#include <openenclave/internal/syscall/dirent.h>
#include <openenclave/internal/syscall/sys/mount.h>
#include <openenclave/corelibc/stdio.h>
#include <openenclave/corelibc/stdlib.h>
#include <openenclave/corelibc/string.h>
#include <openenclave/internal/syscall/fcntl.h>
#include <openenclave/internal/syscall/sys/ioctl.h>
#include <openenclave/internal/syscall/raise.h>
#include <openenclave/internal/syscall/iov.h>
#include <openenclave/internal/raise.h>
#include <openenclave/internal/hexdump.h>
#include <openenclave/internal/safecrt.h>

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
    uintptr_t handle;

    const oe_customfs_t* device;
    oe_spinlock_t lock;
    uint64_t offset;
    bool readonly;
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

static void* _get_context(const file_t* file)
{
    oe_assert(file);
    return ((device_t*)file->device)->context;
}

// caller must hold the file lock
static ssize_t _read(
    const file_t* file,
    void* buf,
    uint64_t count,
    uint64_t offset)
{
    oe_assert(file);
    oe_assert(buf);

    void* const context = _get_context(file);

    const uint64_t size = file->device->get_size(context, file->handle);
    if (offset >= size)
        return 0;

    const uint64_t max_count = size - offset;
    if (count > max_count)
        count = max_count;
    if (count > OE_SSIZE_MAX)
        count = OE_SSIZE_MAX;

    file->device->read(context, file->handle, buf, count, offset);
    return (ssize_t)count;
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

static oe_fd_t* _fs_open_file(
    oe_device_t* device,
    const char* pathname,
    int flags,
    oe_mode_t mode)
{
    (void)mode;

    oe_fd_t* ret = NULL;
    device_t* fs = _cast_device(device);
    file_t* file = NULL;

    /* Fail if any required parameters are null. */
    if (!fs || !pathname)
        OE_RAISE_ERRNO(OE_EINVAL);

    /* Fail if attempting to write to a read-only file system. */
    const bool readonly = (flags & ACCESS_MODE_MASK) == OE_O_RDONLY;
    if (_is_read_only(fs) && !readonly)
        OE_RAISE_ERRNO(OE_EPERM);

    /* Create new file struct. */
    {
        if (!(file = oe_calloc(1, sizeof(file_t))))
            OE_RAISE_ERRNO(OE_ENOMEM);

        file->base.type = OE_FD_TYPE_FILE;
        file->magic = FILE_MAGIC;
        file->base.ops.file = _get_file_ops();
        file->device = (oe_customfs_t*)device;
        file->readonly = readonly;
    }

    oe_spin_lock(&_lock);
    if (flags & OE_O_TRUNC)
        file->device->unlink(fs->context, pathname);

    /* Ask the host to open the file. */
    file->handle =
        file->device->open(fs->context, pathname, !(flags & OE_O_CREAT));
    oe_spin_unlock(&_lock);
    if (!file->handle)
        OE_RAISE_ERRNO(OE_EINVAL);

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

    if (!file)
        OE_RAISE_ERRNO(OE_EINVAL);

    oe_spin_lock(&file->lock);

    ret = _read(file, buf, count, file->offset);
    oe_assert(ret >= 0);
    file->offset += (uint64_t)ret;

    oe_spin_unlock(&file->lock);

done:
    return ret;
}

/* Called by oe_getdents64() to handle the getdents64 system call. */
static int _fs_getdents64(
    oe_fd_t* desc,
    struct oe_dirent* dirp,
    unsigned int count)
{
    (void)count;

    int ret = -1;
    file_t* file = _cast_file(desc);

    if (!file || !dirp)
        OE_RAISE_ERRNO(OE_EINVAL);

    OE_RAISE_ERRNO(OE_ENOSYS);

done:
    return ret;
}

static ssize_t _fs_write(oe_fd_t* desc, const void* buf, size_t count)
{
    ssize_t ret = -1;
    bool locked = false;
    file_t* file = _cast_file(desc);

    /* Check parameters. */
    if (!file || (count && !buf))
        OE_RAISE_ERRNO(OE_EINVAL);

    if (file->readonly)
        OE_RAISE_ERRNO(OE_EBADF);
    if (count > OE_SSIZE_MAX)
        OE_RAISE_ERRNO(OE_EFBIG);

    locked = true;
    oe_spin_lock(&file->lock);

    if (!file->device->write(
            _get_context(file), file->handle, buf, count, file->offset))
        OE_RAISE_ERRNO(OE_ENOSPC);
    file->offset += count;
    ret = (ssize_t)count;

done:
    if (locked)
        oe_spin_unlock(&file->lock);
    return ret;
}

static ssize_t _fs_readv(oe_fd_t* desc, const struct oe_iovec* iov, int iovcnt)
{
    ssize_t ret = -1;
    file_t* file = _cast_file(desc);

    if (!file || (!iov && iovcnt) || iovcnt < 0 || iovcnt > OE_IOV_MAX)
        OE_RAISE_ERRNO(OE_EINVAL);

    void* const context = _get_context(file);
    uint64_t bytes_read = 0;

    oe_spin_lock(&file->lock);

    const uint64_t size = file->device->get_size(context, file->handle);
    if (file->offset >= size)
    {
        oe_spin_unlock(&file->lock);
        return 0;
    }

    uint64_t max_count = size - file->offset;
    if (max_count > OE_SSIZE_MAX)
        max_count = OE_SSIZE_MAX;

    for (int i = 0; i < iovcnt; ++i)
    {
        size_t len = iov[i].iov_len;
        if (bytes_read + len > max_count)
        {
            len = max_count - bytes_read;
            if (!len)
                break;
        }

        file->device->read(
            context,
            file->handle,
            iov[i].iov_base,
            len,
            file->offset + bytes_read);
        bytes_read += len;
    }

    file->offset += bytes_read;

    oe_spin_unlock(&file->lock);

    ret = (ssize_t)bytes_read;

done:
    return ret;
}

static ssize_t _fs_writev(oe_fd_t* desc, const struct oe_iovec* iov, int iovcnt)
{
    ssize_t ret = -1;
    bool locked = false;
    file_t* file = _cast_file(desc);

    if (!file || !iov || iovcnt < 0 || iovcnt > OE_IOV_MAX)
        OE_RAISE_ERRNO(OE_EINVAL);

    if (file->readonly)
        OE_RAISE_ERRNO(OE_EBADF);

    void* const context = _get_context(file);
    uint64_t bytes_written = 0;

    locked = true;
    oe_spin_lock(&file->lock);

    for (int i = 0; i < iovcnt; ++i)
    {
        const size_t len = iov[i].iov_len;
        if (len > OE_SSIZE_MAX)
        {
            if (bytes_written)
                break;
            OE_RAISE_ERRNO(OE_EFBIG);
        }

        if (!file->device->write(
                context,
                file->handle,
                iov[i].iov_base,
                len,
                file->offset + bytes_written))
        {
            if (bytes_written)
                break;
            OE_RAISE_ERRNO(OE_ENOSPC);
        }

        bytes_written += len;
    }

    file->offset += bytes_written;
    ret = (ssize_t)bytes_written;

done:
    if (locked)
        oe_spin_unlock(&file->lock);
    return ret;
}

static oe_off_t _fs_lseek_file(oe_fd_t* desc, oe_off_t offset, int whence)
{
    oe_off_t ret = -1;
    bool locked = false;
    file_t* file = _cast_file(desc);

    if (!file)
        OE_RAISE_ERRNO(OE_EINVAL);

    locked = true;
    oe_spin_lock(&file->lock);

    switch (whence)
    {
        case OE_SEEK_SET:
            break;
        case OE_SEEK_CUR:
            offset += (oe_off_t)file->offset;
            break;
        case OE_SEEK_END:
            offset += (oe_off_t)file->device->get_size(
                _get_context(file), file->handle);
            break;
        default:
            OE_RAISE_ERRNO(OE_EINVAL);
    }

    if (offset < 0)
        OE_RAISE_ERRNO(OE_EINVAL);
    file->offset = (uint64_t)offset;
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
    file_t* file = _cast_file(desc);

    if (!file || offset < 0)
        OE_RAISE_ERRNO(OE_EINVAL);

    oe_spin_lock(&file->lock);
    ret = _read(file, buf, count, (uint64_t)offset);
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
    bool locked = false;
    file_t* file = _cast_file(desc);

    if (!file || offset < 0)
        OE_RAISE_ERRNO(OE_EINVAL);

    if (file->readonly)
        OE_RAISE_ERRNO(OE_EBADF);
    if (count > OE_SSIZE_MAX)
        OE_RAISE_ERRNO(OE_EFBIG);

    locked = true;
    oe_spin_lock(&file->lock);

    if (!file->device->write(
            _get_context(file), file->handle, buf, count, (uint64_t)offset))
        OE_RAISE_ERRNO(OE_ENOSPC);
    ret = (ssize_t)count;

done:
    if (locked)
        oe_spin_unlock(&file->lock);
    return ret;
}

static int _fs_close_file(oe_fd_t* desc)
{
    int ret = -1;
    file_t* file = _cast_file(desc);

    if (!file)
        OE_RAISE_ERRNO(OE_EINVAL);

    file->device->close(_get_context(file), file->handle);
    oe_free(file);

    ret = 0;

done:
    return ret;
}

static int _fs_ioctl(oe_fd_t* desc, unsigned long request, uint64_t arg)
{
    (void)request;
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
    (void)cmd;
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

static void _stat(const oe_customfs_t* fs, uintptr_t handle, oe_stat_t* buf)
{
    oe_assert(fs);
    oe_assert(handle);
    oe_assert(buf);
    const uint64_t size = fs->get_size(((device_t*)fs)->context, handle);
    buf->st_size = size < OE_SSIZE_MAX ? (oe_off_t)size : OE_SSIZE_MAX;
    buf->st_mode = OE_S_IFREG;
}

static int _fs_stat(oe_device_t* device, const char* pathname, oe_stat_t* buf)
{
    int ret = -1;
    device_t* fs = _cast_device(device);
    int retval = -1;

    if (buf)
        oe_memset_s(buf, sizeof(*buf), 0, sizeof(*buf));

    if (!fs || !pathname || !buf)
        OE_RAISE_ERRNO(OE_EINVAL);

    const oe_customfs_t* const customfs = (oe_customfs_t*)device;

    const uintptr_t handle = customfs->open(fs->context, pathname, true);
    if (handle)
    {
        _stat(customfs, handle, buf);
        customfs->close(fs->context, handle);
        retval = 0;
    }
    else
        oe_errno = OE_ENOENT;

    ret = retval;

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

    _stat(file->device, file->handle, buf);
    ret = 0;

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

    if (!fs || !oldpath || !newpath)
        OE_RAISE_ERRNO(OE_EINVAL);

    /* Fail if attempting to write to a read-only file system. */
    if (_is_read_only(fs))
        OE_RAISE_ERRNO(OE_EPERM);

    OE_RAISE_ERRNO(OE_ENOSYS);

done:

    return ret;
}

static int _fs_unlink(oe_device_t* device, const char* pathname)
{
    int ret = -1;
    device_t* fs = _cast_device(device);

    if (!fs)
        OE_RAISE_ERRNO(OE_EINVAL);

    /* Fail if attempting to write to a read-only file system. */
    if (_is_read_only(fs))
        OE_RAISE_ERRNO(OE_EPERM);

    oe_spin_lock(&_lock);
    ((oe_customfs_t*)device)->unlink(fs->context, pathname);
    oe_spin_unlock(&_lock);

    ret = 0;
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

    if (!fs || !oldpath || !newpath)
        OE_RAISE_ERRNO(OE_EINVAL);

    if (_is_read_only(fs))
        OE_RAISE_ERRNO(OE_EPERM);

    OE_RAISE_ERRNO(OE_ENOSYS);

done:

    return ret;
}

static int _fs_truncate(oe_device_t* device, const char* path, oe_off_t length)
{
    (void)path;
    (void)length;

    int ret = -1;
    device_t* fs = _cast_device(device);

    if (!fs)
        OE_RAISE_ERRNO(OE_EINVAL);

    if (_is_read_only(fs))
        OE_RAISE_ERRNO(OE_EPERM);

    OE_RAISE_ERRNO(OE_ENOSYS);

done:

    return ret;
}

static int _fs_mkdir(oe_device_t* device, const char* pathname, oe_mode_t mode)
{
    (void)pathname;
    (void)mode;

    int ret = -1;
    device_t* fs = _cast_device(device);

    if (!fs)
        OE_RAISE_ERRNO(OE_EINVAL);

    /* Fail if attempting to write to a read-only file system. */
    if (_is_read_only(fs))
        OE_RAISE_ERRNO(OE_EPERM);

    OE_RAISE_ERRNO(OE_ENOSYS);

done:

    return ret;
}

static int _fs_rmdir(oe_device_t* device, const char* pathname)
{
    (void)pathname;

    int ret = -1;
    device_t* fs = _cast_device(device);

    if (!fs)
        OE_RAISE_ERRNO(OE_EINVAL);

    /* Fail if attempting to write to a read-only file system. */
    if (_is_read_only(fs))
        OE_RAISE_ERRNO(OE_EPERM);

    OE_RAISE_ERRNO(OE_ENOSYS);

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
    .fd.close = _fs_close_file,
    .fd.get_host_fd = _fs_get_host_fd,
    .lseek = _fs_lseek_file,
    .pread = _fs_pread,
    .pwrite = _fs_pwrite,
    .getdents64 = _fs_getdents64,
    .fstat = _fs_fstat,
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
    dev->base.ops.fs.open = _fs_open_file;
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
