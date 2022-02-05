#include <openenclave/internal/defs.h>
#include <openenclave/internal/tests.h>
#include <sys/mman.h>
#include <algorithm>
#include <cstdint>
#include <cstring>
#include "test_t.h"

using namespace std;

static void _test_filled(const uint8_t* p, size_t size, uint8_t value)
{
    OE_TEST(p && p != MAP_FAILED);
    OE_TEST(all_of(p, p + size, [value](uint8_t x) { return x == value; }));
}

void test_ecall()
{
    const char* const a = new char('a');
    OE_TEST(*a == 'a');

    const int flags = MAP_ANON | MAP_PRIVATE;

    const auto p =
        static_cast<uint8_t*>(mmap(nullptr, 1, PROT_READ, flags, -1, 0));
    _test_filled(
        p, OE_PAGE_SIZE, 0); // requested size is rounded up to page size
    OE_TEST(munmap(p, 1) == 0);
    OE_TEST(munmap(p, 1) == 0); // unmapping unmapped pages is not an error

    // test invalid args
    OE_TEST(munmap(p + 1, 1) == -1);
    OE_TEST(munmap(p, 0) == -1);
    OE_TEST(munmap(nullptr, 1) == -1);
    OE_TEST(mmap(nullptr, 0, PROT_READ, flags, -1, 0) == MAP_FAILED);
    OE_TEST(mmap(p + 1, 1, PROT_READ, flags | MAP_FIXED, -1, 0) == MAP_FAILED);
    OE_TEST(mmap(nullptr, 1, PROT_READ, MAP_PRIVATE, 0, 0) == MAP_FAILED);
    OE_TEST(mmap(nullptr, 1, PROT_READ, flags, -1, 1) == MAP_FAILED);

    auto p2 = static_cast<uint8_t*>(mmap(
        p, 3 * OE_PAGE_SIZE, PROT_READ | PROT_WRITE, flags | MAP_FIXED, -1, 0));
    OE_TEST(p2 == p);

    _test_filled(p, 3 * OE_PAGE_SIZE, 0);
    memset(p, 2, 3 * OE_PAGE_SIZE);
    _test_filled(p, 3 * OE_PAGE_SIZE, 2);

    p2 = static_cast<uint8_t*>(mmap(
        p + OE_PAGE_SIZE, 1, PROT_READ | PROT_WRITE, flags | MAP_FIXED, -1, 0));
    OE_TEST(p2 == p + OE_PAGE_SIZE);

    // mmap cleared one page
    _test_filled(p, OE_PAGE_SIZE, 2);
    _test_filled(p + OE_PAGE_SIZE, OE_PAGE_SIZE, 0);
    _test_filled(p + 2 * OE_PAGE_SIZE, OE_PAGE_SIZE, 2);

    // mmap did not overwrite heap allocation. (Ensures that malloc uses mmap
    // internally instead of directly using the enclave heap memory.)
    OE_TEST(*a == 'a');
    delete a;
}

OE_SET_ENCLAVE_SGX(
    1,    /* ProductID */
    1,    /* SecurityVersion */
    true, /* Debug */
    64,   /* NumHeapPages */
    64,   /* NumStackPages */
    1);   /* NumTCS */
