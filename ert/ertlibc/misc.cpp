// Copyright (c) Edgeless Systems GmbH.
// Licensed under the MIT License.

#include <openenclave/internal/random.h>
#include <climits>
#include <stdexcept>
#include <system_error>
#include "syscalls.h"

using namespace std;
using namespace ert;

ssize_t sc::getrandom(void* buf, size_t buflen, unsigned int /*flags*/)
{
    if (!buflen)
        return 0;
    if (!buf)
        throw system_error(EFAULT, system_category(), "getrandom: buf");

    if (buflen > SSIZE_MAX)
        buflen = SSIZE_MAX;

    if (oe_random_internal(buf, buflen) != OE_OK)
        throw runtime_error("oe_random_internal failed");

    return static_cast<ssize_t>(buflen);
}
