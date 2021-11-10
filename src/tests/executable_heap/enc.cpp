#include <openenclave/internal/tests.h>
#include <vector>
#include "test_t.h"

using namespace std;

static int _f()
{
    return 2;
}

void test_ecall()
{
    const auto _f_ptr = reinterpret_cast<const byte*>(_f);
    vector<byte> f_on_heap(_f_ptr, _f_ptr + 16);
    OE_TEST(reinterpret_cast<int (*)()>(f_on_heap.data())() == 2);
}
