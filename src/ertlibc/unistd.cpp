// Copyright (c) Edgeless Systems GmbH.
// Licensed under the MIT License.

#include <unistd.h>
#include <cerrno>
#include "syscalls.h"

using namespace ert;

ssize_t sc::readlink(const char* pathname, char* /*buf*/, size_t /*bufsiz*/)
{
    if (access(pathname, F_OK) != 0)
        return -errno;
    // symlinks not supported, so this must be a regular file
    return -EINVAL;
}
