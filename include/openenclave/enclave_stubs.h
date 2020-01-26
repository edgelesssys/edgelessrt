// Copyright (c) Edgeless Systems GmbH.
// Licensed under the MIT License.

#pragma once

#include <errno.h>
#include "bits/defs.h"

OE_EXTERNC void oe_stub_trace(const char* msg);

#define OE_STUB_ERRNO(x, ret, err) \
    OE_EXTERNC int x()             \
    {                              \
        oe_stub_trace(#x);         \
        errno = err;               \
        return ret;                \
    }

#define OE_STUB_ERRNO_SILENT(x, ret, err) \
    OE_EXTERNC int x()                    \
    {                                     \
        errno = err;                      \
        return ret;                       \
    }

#define OE_STUB(x, ret) OE_STUB_ERRNO(x, ret, 0)
#define OE_STUB_SILENT(x, ret) OE_STUB_ERRNO_SILENT(x, ret, 0)
