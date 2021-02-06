// Copyright (c) Edgeless Systems GmbH.
// Licensed under the MIT License.

#include "bitset.h"
#include <assert.h>
#include <limits.h>
#include <stdint.h>

#define UINTPTR_BITS (CHAR_BIT * sizeof(uintptr_t))

void ert_bitset_set_range(void* bitset, size_t pos, size_t count)
{
    assert(bitset);

    uintptr_t* p = (uintptr_t*)bitset + pos / UINTPTR_BITS;
    size_t remaining = count;

    // handle first word
    size_t bits_per_word = UINTPTR_BITS - (pos % UINTPTR_BITS);
    uintptr_t mask = UINTPTR_MAX << (pos % UINTPTR_BITS);

    while (remaining >= bits_per_word)
    {
        *p |= mask;
        remaining -= bits_per_word;
        bits_per_word = UINTPTR_BITS;
        mask = UINTPTR_MAX;
        ++p;
    }

    if (!remaining)
        return;

    // handle last word
    mask &= UINTPTR_MAX >> (UINTPTR_BITS - (pos + count) % UINTPTR_BITS);
    *p |= mask;
}

void ert_bitset_reset_range(void* bitset, size_t pos, size_t count)
{
    assert(bitset);

    uintptr_t* p = (uintptr_t*)bitset + pos / UINTPTR_BITS;
    size_t remaining = count;

    // handle first word
    size_t bits_per_word = UINTPTR_BITS - (pos % UINTPTR_BITS);
    uintptr_t mask = UINTPTR_MAX << (pos % UINTPTR_BITS);

    while (remaining >= bits_per_word)
    {
        *p &= ~mask;
        remaining -= bits_per_word;
        bits_per_word = UINTPTR_BITS;
        mask = UINTPTR_MAX;
        ++p;
    }

    if (!remaining)
        return;

    // handle last word
    mask &= UINTPTR_MAX >> (UINTPTR_BITS - (pos + count) % UINTPTR_BITS);
    *p &= ~mask;
}

// invert = 0 to search for 1 bit
// invert = UINTPTR_MAX to search for 0 bit
// returns bitset_size if not found
static size_t _find_next_bit(
    const uintptr_t* bitset,
    size_t bitset_size,
    size_t pos,
    uintptr_t invert)
{
    assert(bitset);
    assert(invert == 0 || invert == UINTPTR_MAX);

    if (pos >= bitset_size)
        return bitset_size;

    uintptr_t word = bitset[pos / UINTPTR_BITS] ^ invert;

    // handle first word
    word &= UINTPTR_MAX << (pos % UINTPTR_BITS);
    pos -= pos % UINTPTR_BITS;

    while (!word)
    {
        pos += UINTPTR_BITS;
        if (pos >= bitset_size)
            return bitset_size;

        word = bitset[pos / UINTPTR_BITS] ^ invert;
    }

    // find first bit in word
    pos += (size_t)__builtin_ctzl(word);
    if (pos >= bitset_size)
        return bitset_size;

    return pos;
}

size_t ert_bitset_find_unset_range(
    const void* bitset,
    size_t bitset_size,
    size_t count)
{
    assert(bitset);
    assert(count);

    for (size_t i = 0;;)
    {
        // find next 0 bit
        const size_t pos0 = _find_next_bit(bitset, bitset_size, i, UINTPTR_MAX);

        const size_t end = pos0 + count;
        if (end > bitset_size)
            return SIZE_MAX;

        // find next 1 bit
        const size_t pos1 = _find_next_bit(bitset, bitset_size, pos0, 0);

        if (pos1 >= end)
            return pos0;

        i = pos1 + 1;
    }
}

size_t ert_bitset_find_set_range(
    const void* bitset,
    size_t bitset_size,
    size_t pos,
    size_t* count)
{
    assert(bitset);
    assert(count);

    // find next 1 bit
    const size_t pos0 = _find_next_bit(bitset, bitset_size, pos, 0);
    if (pos0 == bitset_size)
    {
        return SIZE_MAX;
    }

    // find next 0 bit
    const size_t pos1 = _find_next_bit(bitset, bitset_size, pos0, UINTPTR_MAX);

    // return count
    *count = pos1 - pos0;

    return pos0;
}
