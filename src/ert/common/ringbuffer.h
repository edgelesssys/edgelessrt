// Copyright (c) Edgeless Systems GmbH.
// Licensed under the MIT License.

#pragma once

#include <openenclave/bits/types.h>

typedef struct _ert_ringbuffer
{
    size_t _front;
    size_t _back;
    size_t _capacity;
    bool _full;
    uint8_t _buf[];
} ert_ringbuffer_t;

OE_EXTERNC_BEGIN

ert_ringbuffer_t* ert_ringbuffer_alloc(size_t size);
void ert_ringbuffer_free(ert_ringbuffer_t* rb);
size_t ert_ringbuffer_read(ert_ringbuffer_t* rb, void* buffer, size_t size);
size_t ert_ringbuffer_write(
    ert_ringbuffer_t* rb,
    const void* buffer,
    size_t size);
bool ert_ringbuffer_empty(const ert_ringbuffer_t* rb);

OE_EXTERNC_END
