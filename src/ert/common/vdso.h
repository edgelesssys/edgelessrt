// Copyright (c) Edgeless Systems GmbH.
// Licensed under the MIT License.

#pragma once

#include <openenclave/bits/types.h>

typedef struct _oe_vdso_timestamp
{
    int64_t sec;
    int64_t nsec;
} oe_vdso_timestamp_t;
