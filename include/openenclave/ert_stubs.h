// Copyright (c) Edgeless Systems GmbH.
// Licensed under the MIT License.

#pragma once

#include <errno.h>
#include "bits/defs.h"

OE_EXTERNC void ert_stub_trace(const char* msg);

#define ERT_STUB_ERRNO(x, ret, err) \
    OE_EXTERNC int x()              \
    {                               \
        ERT_STUB_trace(#x);         \
        errno = err;                \
        return ret;                 \
    }

#define ERT_STUB_ERRNO_SILENT(x, ret, err) \
    OE_EXTERNC int x()                     \
    {                                      \
        errno = err;                       \
        return ret;                        \
    }

#define ERT_STUB(x, ret) ERT_STUB_ERRNO(x, ret, 0)
#define ERT_STUB_SILENT(x, ret) ERT_STUB_ERRNO_SILENT(x, ret, 0)
