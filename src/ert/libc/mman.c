// Copyright (c) Edgeless Systems GmbH.
// Licensed under the MIT License.

/*
This mmap implementation manages the whole enclave heap memory. The first pages
are reserved for a bitmap that saves the state of all other pages: 1 if the page
is in use, 0 otherwise.
malloc calls mmap to reserve enclave heap space.
*/

#include "mman.h"
#include <assert.h>
#include <errno.h>
#include <limits.h>
#include <openenclave/internal/globals.h>
#include <openenclave/internal/thread.h>
#include <openenclave/internal/utils.h>
#include <stdint.h>
#include <string.h>
#include "../common/bitset.h"

static oe_spinlock_t _lock = OE_SPINLOCK_INITIALIZER;
static void* _bitset;
static void* _base;
static size_t _size;

static void _init()
{
    const size_t full_size = __oe_get_heap_size();
    const size_t bitmap_size =
        oe_round_up_to_page_size(full_size / (CHAR_BIT * OE_PAGE_SIZE));
    _bitset = (void*)__oe_get_heap_base();
    _base = (uint8_t*)_bitset + bitmap_size;
    _size = full_size - bitmap_size;
    memset(_bitset, 0, bitmap_size);
}

static bool _length_in_range(size_t length)
{
    return 0 < length && length < _size;
}

static bool _addr_in_range(void* addr, size_t length)
{
    return _base <= addr && (uint8_t*)addr + length <= (uint8_t*)_base + _size;
}

static size_t _to_pos(const void* addr)
{
    return (size_t)((uint8_t*)addr - (uint8_t*)_base) / OE_PAGE_SIZE;
}

static void* _map(size_t length)
{
    assert(length && length % OE_PAGE_SIZE == 0);

    // Naive approach: reserve the first unset range we can find.

    const size_t count = length / OE_PAGE_SIZE;
    const size_t pos =
        ert_bitset_find_unset_range(_bitset, _size / OE_PAGE_SIZE, count);

    if (pos == SIZE_MAX)
        return (void*)-ENOMEM;

    ert_bitset_set_range(_bitset, pos, count);
    void* const result = (uint8_t*)_base + pos * OE_PAGE_SIZE;
    memset(result, 0, length);
    return result;
}

static void* _map_fixed(void* addr, size_t length)
{
    if (!_addr_in_range(addr, length))
        return (void*)-ENOMEM;

    // MAP_FIXED discards overlapped part of existing mappings
    ert_bitset_set_range(_bitset, _to_pos(addr), length / OE_PAGE_SIZE);
    memset(addr, 0, length);
    return addr;
}

void* ert_mmap(
    void* addr,
    size_t length,
    int prot,
    int flags,
    int fd,
    off_t offset)
{
    // check for invalid args
    if (!length || !addr != !(flags & MAP_FIXED) ||
        (uintptr_t)addr % OE_PAGE_SIZE)
        return (void*)-EINVAL;

    // check for unsupported args
    if (offset || (fd != -1 && !(flags & MAP_ANON)))
        return (void*)-ENOSYS;

    length = oe_round_up_to_page_size(length);
    void* result = MAP_FAILED;

    oe_spin_lock(&_lock);

    if (!_base)
        _init();

    if (!_length_in_range(length))
    {
        oe_spin_unlock(&_lock);
        return (void*)-ENOMEM;
    }

    if (flags & MAP_FIXED)
        result = _map_fixed(addr, length);
    else
        result = _map(length);

    oe_spin_unlock(&_lock);

    return result;
}

int ert_munmap(void* addr, size_t length)
{
    int result = -EINVAL;
    length = oe_round_up_to_page_size(length);

    oe_spin_lock(&_lock);

    if (_length_in_range(length) && _addr_in_range(addr, length) &&
        (uintptr_t)addr % OE_PAGE_SIZE == 0)
    {
        ert_bitset_reset_range(_bitset, _to_pos(addr), length / OE_PAGE_SIZE);
        result = 0;
    }

    oe_spin_unlock(&_lock);

    return result;
}
