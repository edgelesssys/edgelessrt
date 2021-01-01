// Copyright (c) Edgeless Systems GmbH.
// Licensed under the MIT License.

#pragma once

#include <openenclave/bits/defs.h>

OE_EXTERNC long
ert_syscall(long n, long x1, long x2, long x3, long x4, long x5, long x6);
