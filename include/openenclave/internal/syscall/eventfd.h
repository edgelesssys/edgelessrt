// Copyright (c) Edgeless Systems GmbH.
// Licensed under the MIT License.

#pragma once

#include <openenclave/bits/defs.h>
#include <openenclave/internal/syscall/types.h>

typedef uint64_t oe_eventfd_t;

OE_EXTERNC_BEGIN

int oe_eventfd(unsigned int initval, int flags);
oe_host_fd_t oe_host_eventfd(unsigned int initval, int flags);
int oe_host_eventfd_read(oe_host_fd_t fd, oe_eventfd_t* value);
int oe_host_eventfd_write(oe_host_fd_t fd, oe_eventfd_t value);

OE_EXTERNC_END
