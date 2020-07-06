// Copyright (c) Edgeless Systems GmbH.
// Licensed under the MIT License.

#pragma once

#include <openenclave/bits/defs.h>

typedef struct _ert_thread
{
    struct _oe_new_thread* new_thread;
    long tid;
} ert_thread_t;

OE_EXTERNC_BEGIN

const ert_thread_t* ert_thread_self();
void ert_musl_init_threaded(void);

OE_EXTERNC_END
