// Copyright (c) Edgeless Systems GmbH.
// Licensed under the MIT License.

#include <openenclave/internal/bitset.h>
#include "common.h"

#define UINTPTR_BITS (OE_CHAR_BIT * sizeof(uintptr_t))

void oe_bitset_set_range(void* bitset, size_t pos, size_t count)
{
    oe_assert(bitset);

    uintptr_t* p = (uintptr_t*)bitset + pos / UINTPTR_BITS;
    size_t remaining = count;

    // handle first word
    size_t bits_per_word = UINTPTR_BITS - (pos % UINTPTR_BITS);
    uintptr_t mask = OE_UINTPTR_MAX << (pos % UINTPTR_BITS);

    while (remaining >= bits_per_word)
    {
        *p |= mask;
        remaining -= bits_per_word;
        bits_per_word = UINTPTR_BITS;
        mask = OE_UINTPTR_MAX;
        ++p;
    }

    if (!remaining)
        return;

    // handle last word
    mask &= OE_UINTPTR_MAX >> (UINTPTR_BITS - (pos + count) % UINTPTR_BITS);
    *p |= mask;
}

void oe_bitset_reset_range(void* bitset, size_t pos, size_t count)
{
    oe_assert(bitset);

    uintptr_t* p = (uintptr_t*)bitset + pos / UINTPTR_BITS;
    size_t remaining = count;

    // handle first word
    size_t bits_per_word = UINTPTR_BITS - (pos % UINTPTR_BITS);
    uintptr_t mask = OE_UINTPTR_MAX << (pos % UINTPTR_BITS);

    while (remaining >= bits_per_word)
    {
        *p &= ~mask;
        remaining -= bits_per_word;
        bits_per_word = UINTPTR_BITS;
        mask = OE_UINTPTR_MAX;
        ++p;
    }

    if (!remaining)
        return;

    // handle last word
    mask &= OE_UINTPTR_MAX >> (UINTPTR_BITS - (pos + count) % UINTPTR_BITS);
    *p &= ~mask;
}

// invert = 0 to search for 1 bit
// invert = OE_UINTPTR_MAX to search for 0 bit
// returns bitset_size if not found
static size_t _find_next_bit(
    const uintptr_t* bitset,
    size_t bitset_size,
    size_t pos,
    uintptr_t invert)
{
    oe_assert(bitset);
    oe_assert(invert == 0 || invert == OE_UINTPTR_MAX);

    if (pos >= bitset_size)
        return bitset_size;

    uintptr_t word = bitset[pos / UINTPTR_BITS] ^ invert;

    // handle first word
    word &= OE_UINTPTR_MAX << (pos % UINTPTR_BITS);
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

size_t oe_bitset_find_unset_range(
    const void* bitset,
    size_t bitset_size,
    size_t count)
{
    oe_assert(bitset);
    oe_assert(count);

    for (size_t i = 0;;)
    {
        // find next 0 bit
        const size_t pos0 =
            _find_next_bit(bitset, bitset_size, i, OE_UINTPTR_MAX);

        const size_t end = pos0 + count;
        if (end > bitset_size)
            return OE_SIZE_MAX;

        // find next 1 bit
        const size_t pos1 = _find_next_bit(bitset, bitset_size, pos0, 0);

        if (pos1 >= end)
            return pos0;

        i = pos1 + 1;
    }
}
