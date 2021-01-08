#include <elf.h>
#include <openenclave/ert.h>
#include <openenclave/internal/tests.h>
#include <cstdio>
#include <cstdlib>
#include "test_t.h"

using namespace ert;

static void _start_main(int payload_main(...))
{
    OE_TEST(payload_main() == 2);
    exit(3);
}

void test_ecall()
{
    payload::apply_relocations(_start_main);

    // get payload entry point
    const auto base = static_cast<const uint8_t*>(payload::get_base());
    OE_TEST(base);
    const auto& ehdr = *reinterpret_cast<const Elf64_Ehdr*>(base);
    OE_TEST(ehdr.e_entry);
    const auto entry = (void (*)())(base + ehdr.e_entry);

    entry();
    OE_TEST(false); // unreachable
}

// make sure puts will be linked
extern const auto my_puts = puts;
