// Copyright (c) Edgeless Systems GmbH.
// Licensed under the MIT License.

#include <openenclave/corelibc/stdlib.h>
#include <openenclave/internal/syscall/eventfd.h>
#include <openenclave/internal/syscall/raise.h>
#include "syscall_t.h"

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
