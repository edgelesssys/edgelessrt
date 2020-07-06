// Copyright (c) Edgeless Systems GmbH.
// Licensed under the MIT License.

#pragma once

#include <time.h>

int ert_futex(
    const int* uaddr,
    int op,
    int val,
    const struct timespec* timeout,
    const int* uaddr2);
