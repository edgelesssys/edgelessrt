// Copyright (c) Edgeless Systems GmbH.
// Licensed under the MIT License.

#include <openenclave/corelibc/stdlib.h>
#include <openenclave/internal/syscall/eventfd.h>
#include <openenclave/internal/syscall/fdtable.h>
#include <openenclave/internal/syscall/raise.h>
#include "syscall_t.h"

typedef struct _efd
{
    oe_fd_t base;
    oe_host_fd_t hostfd;
} efd_t;

static ssize_t _efd_read(oe_fd_t* desc, void* buf, size_t count)
{
    oe_assert(desc);
    ssize_t result = -1;

    if (count && !buf)
        OE_RAISE_ERRNO(OE_EINVAL);

    if (oe_syscall_read_ocall(&result, ((efd_t*)desc)->hostfd, buf, count) !=
        OE_OK)
    {
        result = -1;
        OE_RAISE_ERRNO(OE_EINVAL);
    }

done:
    return result;
}

static ssize_t _efd_write(oe_fd_t* desc, const void* buf, size_t count)
{
    oe_assert(desc);
    ssize_t result = -1;

    if (count && !buf)
        OE_RAISE_ERRNO(OE_EINVAL);

    if (oe_syscall_write_ocall(&result, ((efd_t*)desc)->hostfd, buf, count) !=
        OE_OK)
    {
        result = -1;
        OE_RAISE_ERRNO(OE_EINVAL);
    }

done:
    return result;
}

static ssize_t _efd_readv(oe_fd_t* desc, const struct oe_iovec* iov, int iovcnt)
{
    (void)desc;
    (void)iov;
    (void)iovcnt;
    OE_RAISE_ERRNO(OE_ENOSYS);
done:
    return -1;
}

static ssize_t _efd_writev(
    oe_fd_t* desc,
    const struct oe_iovec* iov,
    int iovcnt)
{
    (void)desc;
    (void)iov;
    (void)iovcnt;
    OE_RAISE_ERRNO(OE_ENOSYS);
done:
    return -1;
}

static int _efd_dup(oe_fd_t* desc, oe_fd_t** new_fd)
{
    (void)desc;
    (void)new_fd;
    OE_RAISE_ERRNO(OE_ENOSYS);
done:
    return -1;
}

static int _efd_ioctl(oe_fd_t* desc, unsigned long request, uint64_t arg)
{
    (void)desc;
    (void)request;
    (void)arg;
    OE_RAISE_ERRNO(OE_ENOSYS);
done:
    return -1;
}

static int _efd_fcntl(oe_fd_t* desc, int cmd, uint64_t arg)
{
    (void)desc;
    (void)cmd;
    (void)arg;
    OE_RAISE_ERRNO(OE_ENOSYS);
done:
    return -1;
}

static int _efd_close(oe_fd_t* desc)
{
    oe_assert(desc);
    int result = -1;

    if (oe_syscall_close_ocall(&result, ((efd_t*)desc)->hostfd) != OE_OK)
    {
        result = -1;
        OE_RAISE_ERRNO(OE_EINVAL);
    }

    oe_free(desc);

done:
    return result;
}

static oe_host_fd_t _efd_get_host_fd(oe_fd_t* desc)
{
    return ((efd_t*)desc)->hostfd;
}

static oe_fd_ops_t _ops = {
    .read = _efd_read,
    .write = _efd_write,
    .readv = _efd_readv,
    .writev = _efd_writev,
    .dup = _efd_dup,
    .ioctl = _efd_ioctl,
    .fcntl = _efd_fcntl,
    .close = _efd_close,
    .get_host_fd = _efd_get_host_fd,
};

int oe_eventfd(unsigned int initval, int flags)
{
    int result = -1;

    efd_t* efd = oe_calloc(1, sizeof *efd);
    if (!efd)
        OE_RAISE_ERRNO(OE_ENOMEM);

    oe_host_fd_t hostfd = -1;
    if (oe_syscall_eventfd_ocall(&hostfd, initval, flags) != OE_OK)
        OE_RAISE_ERRNO(OE_EINVAL);
    if (hostfd < 0)
        goto done;

    efd->hostfd = hostfd;
    efd->base.ops.fd = _ops;

    result = oe_fdtable_assign((oe_fd_t*)efd);
    if (result < 0)
    {
        int ret;
        oe_syscall_close_ocall(&ret, hostfd);
        goto done;
    }

    efd = NULL;

done:
    oe_free(efd);

    return result;
}

oe_host_fd_t oe_host_eventfd(unsigned int initval, int flags)
{
    oe_host_fd_t ret = -1;
    if (oe_syscall_eventfd_ocall(&ret, initval, flags) != OE_OK)
        OE_RAISE_ERRNO(OE_EINVAL);
done:
    return ret;
}

int oe_host_eventfd_read(oe_host_fd_t fd, oe_eventfd_t* value)
{
    oe_assert(fd >= 0);
    oe_assert(value);
    int ret = -1;
    ssize_t res = -1;
    if (oe_syscall_read_ocall(&res, fd, value, sizeof *value) != OE_OK)
        OE_RAISE_ERRNO(OE_EINVAL);
    ret = res == sizeof *value ? 0 : -1;
done:
    return ret;
}

int oe_host_eventfd_write(oe_host_fd_t fd, oe_eventfd_t value)
{
    oe_assert(fd >= 0);
    int ret = -1;
    ssize_t res = -1;
    if (oe_syscall_write_ocall(&res, fd, &value, sizeof value) != OE_OK)
        OE_RAISE_ERRNO(OE_EINVAL);
    ret = res == sizeof value ? 0 : -1;
done:
    return ret;
}
