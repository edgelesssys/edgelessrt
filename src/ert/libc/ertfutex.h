// Copyright (c) Edgeless Systems GmbH.
// Licensed under the MIT License.

#pragma once

#include <openenclave/bits/defs.h>
#include <time.h>

OE_EXTERNC_BEGIN

int ert_futex(
    const int* uaddr,
    int op,
    int val,
    const struct timespec* timeout,
    const int* uaddr2,
    int val3);

void ert_futex_remove_tcs(const void* tcs);
void ert_futex_wake_tcs(const void* tcs);

OE_EXTERNC_END
