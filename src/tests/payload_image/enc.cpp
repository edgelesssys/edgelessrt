#include <elf.h>
#include <openenclave/ert.h>
#include <openenclave/internal/tests.h>
#include "test_t.h"

using namespace ert;

void test_ecall()
{
    // get payload entry point
    const auto base = static_cast<const uint8_t*>(payload::get_base());
    OE_TEST(base);
    const auto& ehdr = *reinterpret_cast<const Elf64_Ehdr*>(base);
    OE_TEST(ehdr.e_entry);
}
