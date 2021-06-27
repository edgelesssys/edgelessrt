#include <openenclave/ert.h>
#include <openenclave/internal/tests.h>
#include <cstring>
#include "test_t.h"

using namespace ert;

void test_ecall()
{
    const auto data = payload::get_data();
    OE_TEST(data.second == 3);
    OE_TEST(memcmp(data.first, "foo", 3) == 0);
}

OE_EXPORT extern "C" void __libc_start_main()
{
}
