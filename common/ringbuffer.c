// Copyright (c) Edgeless Systems GmbH.
// Licensed under the MIT License.

#include <openenclave/internal/ringbuffer.h>
#include "common.h"

oe_ringbuffer_t* oe_ringbuffer_alloc(size_t size)
{
    oe_ringbuffer_t* const result = oe_calloc(1, sizeof *result + size);
    if (result)
        result->_capacity = size;
    return result;
}

void oe_ringbuffer_free(oe_ringbuffer_t* rb)
{
    oe_free(rb);
}

static size_t _read(oe_ringbuffer_t* rb, void* buffer, size_t size)
{
    if (!size || (rb->_front == rb->_back && !rb->_full))
        return 0;

    const size_t end = rb->_back > rb->_front ? rb->_back : rb->_capacity;
    size_t n = end - rb->_front;
    if (n > size)
        n = size;

    memcpy(buffer, rb->_buf + rb->_front, n);

    rb->_front = (rb->_front + n) % rb->_capacity;
    rb->_full = false;

    return n;
}

size_t oe_ringbuffer_read(oe_ringbuffer_t* rb, void* buffer, size_t size)
{
    oe_assert(rb);
    oe_assert(buffer || !size);
    const size_t n1 = _read(rb, buffer, size);
    const size_t n2 = _read(rb, (uint8_t*)buffer + n1, size - n1);
    return n1 + n2;
}

static size_t _write(oe_ringbuffer_t* rb, const void* buffer, size_t size)
{
    if (!size || rb->_full)
        return 0;

    const size_t end = rb->_front > rb->_back ? rb->_front : rb->_capacity;
    size_t n = end - rb->_back;
    if (n > size)
        n = size;

    memcpy(rb->_buf + rb->_back, buffer, n);

    rb->_back = (rb->_back + n) % rb->_capacity;
    rb->_full = rb->_back == rb->_front;

    return n;
}

size_t oe_ringbuffer_write(oe_ringbuffer_t* rb, const void* buffer, size_t size)
{
    oe_assert(rb);
    oe_assert(buffer || !size);
    const size_t n1 = _write(rb, buffer, size);
    const size_t n2 = _write(rb, (uint8_t*)buffer + n1, size - n1);
    return n1 + n2;
}
