// Copyright (c) Open Enclave SDK contributors.
// Licensed under the MIT License.

#include <errno.h>
#include <openenclave/enclave.h>
#include <openenclave/internal/defs.h>
#include <unistd.h>

long sysconf(int name)
{
    switch (name)
    {
        case _SC_PAGESIZE:
            return OE_PAGE_SIZE;
        default:
            errno = EINVAL;
            return -1;
    }
}
