// Copyright (c) Edgeless Systems GmbH.
// Licensed under the MIT License.

#pragma once

#include <openenclave/bits/types.h>

typedef struct _oe_ringbuffer
{
    size_t _front;
    size_t _back;
    size_t _capacity;
    bool _full;
    uint8_t _buf[];
} oe_ringbuffer_t;

OE_EXTERNC_BEGIN

oe_ringbuffer_t* oe_ringbuffer_alloc(size_t size);
void oe_ringbuffer_free(oe_ringbuffer_t* rb);
size_t oe_ringbuffer_read(oe_ringbuffer_t* rb, void* buffer, size_t size);
size_t oe_ringbuffer_write(
    oe_ringbuffer_t* rb,
    const void* buffer,
    size_t size);

OE_EXTERNC_END
