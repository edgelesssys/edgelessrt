#include <openenclave/internal/ringbuffer.h>
#include <openenclave/internal/tests.h>
#include <array>
#include <cstring>
#include "test_t.h"

using namespace std;

void test_ecall()
{
    // free()-like functions should accept null
    oe_ringbuffer_free(nullptr);

    array<char, 8> buf;

    const auto rb = oe_ringbuffer_alloc(8);
    OE_TEST(rb);
    OE_TEST(oe_ringbuffer_empty(rb));

    OE_TEST(oe_ringbuffer_read(rb, buf.data(), 1) == 0);
    OE_TEST(oe_ringbuffer_write(rb, "a", 1) == 1);
    OE_TEST(!oe_ringbuffer_empty(rb));
    OE_TEST(oe_ringbuffer_read(rb, buf.data(), 2) == 1);
    OE_TEST(oe_ringbuffer_empty(rb));
    OE_TEST(buf[0] == 'a');
    OE_TEST(oe_ringbuffer_read(rb, buf.data(), 1) == 0);
    OE_TEST(oe_ringbuffer_write(rb, "bc", 2) == 2);
    OE_TEST(oe_ringbuffer_read(rb, buf.data(), 1) == 1);
    OE_TEST(buf[0] == 'b');
    OE_TEST(oe_ringbuffer_write(rb, "d", 1) == 1);
    OE_TEST(oe_ringbuffer_read(rb, buf.data(), 2) == 2);
    OE_TEST(memcmp(buf.data(), "cd", 2) == 0);
    OE_TEST(oe_ringbuffer_read(rb, buf.data(), 1) == 0);
    OE_TEST(oe_ringbuffer_write(rb, "efghijklm", 9) == 8);
    OE_TEST(oe_ringbuffer_read(rb, buf.data(), 2) == 2);
    OE_TEST(memcmp(buf.data(), "ef", 2) == 0);
    OE_TEST(oe_ringbuffer_write(rb, "no", 2) == 2);
    OE_TEST(oe_ringbuffer_read(rb, buf.data(), 8) == 8);
    OE_TEST(memcmp(buf.data(), "ghijklno", 8) == 0);

    oe_ringbuffer_free(rb);
}

OE_SET_ENCLAVE_SGX(
    1,    /* ProductID */
    1,    /* SecurityVersion */
    true, /* Debug */
    64,   /* NumHeapPages */
    64,   /* NumStackPages */
    1);   /* NumTCS */
