#include <openenclave/internal/tests.h>
#include "test_t.h"

extern "C" int linked_test();

void test_ecall()
{
    OE_TEST(linked_test() == 2);
}
