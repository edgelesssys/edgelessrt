#pragma once

#include <openenclave/corelibc/limits.h>
#include <openenclave/internal/syscall/device.h>

#define FS_MAGIC 0x5f35f964

/* Mask to extract the access mode: O_RDONLY, O_WRONLY, O_RDWR. */
#define ACCESS_MODE_MASK 000000003

/* The host file system device. */
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
} device_t;

extern device_t oe_hostfs_mmap;

int _hostfs_make_host_path(
    const device_t* fs,
    const char* enclave_path,
    char host_path[OE_PATH_MAX]);
int _hostfs_mount(
    oe_device_t* device,
    const char* source,
    const char* target,
    const char* filesystemtype,
    unsigned long flags,
    const void* data);
int _hostfs_umount2(oe_device_t* device, const char* target, int flags);
int _hostfs_clone(oe_device_t* device, oe_device_t** new_device);
int _hostfs_release(oe_device_t* device);
oe_fd_t* _hostfs_open_directory(
    oe_device_t* device,
    const char* pathname,
    int flags);
int _hostfs_stat(
    oe_device_t* device,
    const char* pathname,
    struct oe_stat_t* buf);
int _hostfs_access(oe_device_t* device, const char* pathname, int mode);
int _hostfs_link(oe_device_t* device, const char* oldpath, const char* newpath);
int _hostfs_unlink(oe_device_t* device, const char* pathname);
int _hostfs_rename(
    oe_device_t* device,
    const char* oldpath,
    const char* newpath);
int _hostfs_truncate(oe_device_t* device, const char* path, oe_off_t length);
int _hostfs_mkdir(oe_device_t* device, const char* pathname, oe_mode_t mode);
int _hostfs_rmdir(oe_device_t* device, const char* pathname);
