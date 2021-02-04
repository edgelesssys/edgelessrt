#include <openenclave/internal/tests.h>
#include <array>
#include <climits>
#include <cstdint>
#include <limits>
#include <string>
#include "../../ert/common/bitset.h"
#include "test_t.h"

using namespace std;

static string zeros(size_t n)
{
    return string(n, '0');
}

static string ones(size_t n)
{
    return string(n, '1');
}

// expect that *bitset* equals the bit string *expected*.
template <typename T>
static void expect(const T& bitset, const string& expected)
{
    const size_t word_size = CHAR_BIT * sizeof bitset.front();
    string actual;

    for (size_t i = 0; i < bitset.size() * word_size; ++i)
        actual += bitset[i / word_size] & (1ul << (i % word_size)) ? '1' : '0';

    OE_TEST(expected == actual);
}

template <typename T>
static void _test_find_unset_range(const T& bitset, size_t count, int expect)
{
    OE_TEST(
        ert_bitset_find_unset_range(
            bitset.data(),
            bitset.size() * sizeof bitset.front() * CHAR_BIT,
            count) == static_cast<size_t>(expect));
}

template <typename T>
static void _test_find_set_range(
    const T& bitset,
    size_t pos,
    int expect_idx,
    int expect_size)
{
    size_t count;
    OE_TEST(
        ert_bitset_find_set_range(
            bitset.data(),
            bitset.size() * sizeof bitset.front() * CHAR_BIT,
            pos,
            &count) == static_cast<size_t>(expect_idx));
    if (expect_idx != -1)
    {
        OE_TEST(count == static_cast<size_t>(expect_size));
    }
}

void test_ecall()
{
    // max value for pos and count
    const size_t max_value = 512;

    // Allocate a bit more than required so that we can test that the functions
    // do not write out of bounds.
    const size_t bit_count = 2 * max_value + 2 * 64;
    array<uint64_t, bit_count / 64> bitset;

    //
    // Test ert_bitset_(re)set_range
    //
    for (size_t pos = 0; pos < 3; ++pos)
        for (size_t count = 0; count < 3; ++count)
        {
            bitset.fill(0);
            ert_bitset_set_range(bitset.data(), pos, count);
            expect(
                bitset,
                zeros(pos) + ones(count) + zeros(bit_count - pos - count));

            bitset.fill(numeric_limits<uint64_t>::max());
            ert_bitset_set_range(bitset.data(), pos, count);
            expect(bitset, ones(bit_count));

            bitset.fill(0);
            ert_bitset_reset_range(bitset.data(), pos, count);
            expect(bitset, zeros(bit_count));

            bitset.fill(numeric_limits<uint64_t>::max());
            ert_bitset_reset_range(bitset.data(), pos, count);
            expect(
                bitset,
                ones(pos) + zeros(count) + ones(bit_count - pos - count));
        }

    //
    // Test ert_bitset_find_unset_range
    //

    bitset.fill(numeric_limits<uint64_t>::max());
    _test_find_unset_range(bitset, 1, -1);
    _test_find_unset_range(bitset, 2, -1);
    _test_find_unset_range(bitset, 62, -1);
    _test_find_unset_range(bitset, 63, -1);
    _test_find_unset_range(bitset, 64, -1);
    _test_find_unset_range(bitset, 65, -1);
    _test_find_unset_range(bitset, 126, -1);
    _test_find_unset_range(bitset, 127, -1);
    _test_find_unset_range(bitset, 128, -1);
    _test_find_unset_range(bitset, 129, -1);
    _test_find_unset_range(bitset, bit_count - 2, -1);
    _test_find_unset_range(bitset, bit_count - 1, -1);
    _test_find_unset_range(bitset, bit_count, -1);
    _test_find_unset_range(bitset, bit_count + 1, -1);
    _test_find_unset_range(bitset, bit_count + 2, -1);

    bitset.fill(0);
    _test_find_unset_range(bitset, 1, 0);
    _test_find_unset_range(bitset, 2, 0);
    _test_find_unset_range(bitset, 62, 0);
    _test_find_unset_range(bitset, 63, 0);
    _test_find_unset_range(bitset, 64, 0);
    _test_find_unset_range(bitset, 65, 0);
    _test_find_unset_range(bitset, 126, 0);
    _test_find_unset_range(bitset, 127, 0);
    _test_find_unset_range(bitset, 128, 0);
    _test_find_unset_range(bitset, 129, 0);
    _test_find_unset_range(bitset, bit_count - 2, 0);
    _test_find_unset_range(bitset, bit_count - 1, 0);
    _test_find_unset_range(bitset, bit_count, 0);
    _test_find_unset_range(bitset, bit_count + 1, -1);
    _test_find_unset_range(bitset, bit_count + 2, -1);

    bitset[0] = 1;
    bitset[1] = 6;
    bitset[4] = 0x8000'0000'0000'0001;
    _test_find_unset_range(bitset, 1, 1);
    _test_find_unset_range(bitset, 2, 1);
    _test_find_unset_range(bitset, 63, 1);
    _test_find_unset_range(bitset, 64, 1);
    _test_find_unset_range(bitset, 65, 67);
    _test_find_unset_range(bitset, 66, 67);
    _test_find_unset_range(bitset, 188, 67);
    _test_find_unset_range(bitset, 189, 67);
    _test_find_unset_range(bitset, 190, 320);
    _test_find_unset_range(bitset, 191, 320);
    _test_find_unset_range(bitset, bit_count - 321, 320);
    _test_find_unset_range(bitset, bit_count - 320, 320);
    _test_find_unset_range(bitset, bit_count - 319, -1);
    _test_find_unset_range(bitset, bit_count - 318, -1);

    //
    // Test ert_bitset_find_set_range
    //

    bitset.fill(numeric_limits<uint64_t>::max());
    _test_find_set_range(bitset, 0, 0, bit_count);
    _test_find_set_range(bitset, 1, 1, bit_count - 1);
    _test_find_set_range(bitset, 2, 2, bit_count - 2);
    _test_find_set_range(bitset, 62, 62, bit_count - 62);
    _test_find_set_range(bitset, 63, 63, bit_count - 63);
    _test_find_set_range(bitset, 64, 64, bit_count - 64);
    _test_find_set_range(bitset, 65, 65, bit_count - 65);
    _test_find_set_range(bitset, 126, 126, bit_count - 126);
    _test_find_set_range(bitset, 127, 127, bit_count - 127);
    _test_find_set_range(bitset, 128, 128, bit_count - 128);
    _test_find_set_range(bitset, 129, 129, bit_count - 129);
    _test_find_set_range(bitset, bit_count - 2, bit_count - 2, 2);
    _test_find_set_range(bitset, bit_count - 1, bit_count - 1, 1);

    bitset.fill(0);
    _test_find_set_range(bitset, 0, -1, -1);
    _test_find_set_range(bitset, 1, -1, -1);
    _test_find_set_range(bitset, 2, -1, -1);
    _test_find_set_range(bitset, 62, -1, -1);
    _test_find_set_range(bitset, 63, -1, -1);
    _test_find_set_range(bitset, 64, -1, -1);
    _test_find_set_range(bitset, 65, -1, -1);
    _test_find_set_range(bitset, 126, -1, -1);
    _test_find_set_range(bitset, 127, -1, -1);
    _test_find_set_range(bitset, 128, -1, -1);
    _test_find_set_range(bitset, 129, -1, -1);
    _test_find_set_range(bitset, bit_count - 2, -1, -1);
    _test_find_set_range(bitset, bit_count - 1, -1, -1);

    bitset[0] = 1;
    bitset[1] = 6;
    bitset[4] = 0x7000'0000'0000'0001;
    _test_find_set_range(bitset, 0, 0, 1);
    _test_find_set_range(bitset, 1, 65, 2);
    _test_find_set_range(bitset, 2, 65, 2);
    _test_find_set_range(bitset, 63, 65, 2);
    _test_find_set_range(bitset, 64, 65, 2);
    _test_find_set_range(bitset, 65, 65, 2);
    _test_find_set_range(bitset, 66, 66, 1);
    _test_find_set_range(bitset, 67, 256, 1);
    _test_find_set_range(bitset, 256, 256, 1);
    _test_find_set_range(bitset, 257, 316, 3);
    _test_find_set_range(bitset, 316, 316, 3);
    _test_find_set_range(bitset, 317, 317, 2);
    _test_find_set_range(bitset, 318, 318, 1);
    _test_find_set_range(bitset, 319, -1, -1);
}

OE_SET_ENCLAVE_SGX(
    1,    /* ProductID */
    1,    /* SecurityVersion */
    true, /* Debug */
    64,   /* NumHeapPages */
    64,   /* NumStackPages */
    1);   /* NumTCS */
