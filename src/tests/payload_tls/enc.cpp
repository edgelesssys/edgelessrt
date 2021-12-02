#include <elf.h>
#include <openenclave/ert.h>
#include <openenclave/internal/sgx/td.h>
#include <openenclave/internal/tests.h>
#include <cstdlib>
#include "test_t.h"

using namespace ert;

extern "C" __thread char ert_reserved_tls[16];

static thread_local uint64_t _x = 40;
static thread_local uint64_t _y;

OE_EXPORT extern "C" void __libc_start_main(int payload_main(...))
{
    OE_TEST(++_x == 41);
    OE_TEST(++_y == 1);
    OE_TEST(payload_main() == 32);
    OE_TEST(++_x == 42);
    OE_TEST(++_y == 2);
    exit(0);
}

void test_ecall()
{
    // Assert that the variable is located at the end of the TLS block.
    OE_TEST(
        reinterpret_cast<char*>(oe_sgx_get_td()) - ert_reserved_tls ==
        sizeof ert_reserved_tls);

    // get payload entry point
    const auto base = static_cast<const uint8_t*>(payload::get_base());
    OE_TEST(base);
    const auto& ehdr = *reinterpret_cast<const Elf64_Ehdr*>(base);
    OE_TEST(ehdr.e_entry);
    const auto entry = (void (*)())(base + ehdr.e_entry);

    entry();
    OE_TEST(false); // unreachable
}
