#include <openenclave/ert.h>
#include <openenclave/internal/globals.h>
#include <openenclave/internal/safemath.h>
#include <cassert>
#include <cstdlib>
#include <cstring>
#include <vector>
#include "ertlibc_t.h"

using namespace std;
using namespace ert;

extern "C" volatile uint64_t _reloc_rva, _reloc_size;

static void _check(oe_result_t result)
{
    if (result != OE_OK)
        abort();
}

void ert_copy_strings_from_host_to_enclave(
    const char* const* host_array,
    char*** enclave_array,
    size_t count)
{
    assert(enclave_array);

    if (!oe_is_outside_enclave(host_array, count * sizeof *host_array))
        abort();

    // Copy host array so that the host cannot manipulate it while we are
    // working on it.
    const vector<const char*> host_array_copy(host_array, host_array + count);

    // Get the lengths of all strings of the host array.
    vector<size_t> lens(count);
    size_t lens_sum = 0;
    for (size_t i = 0; i < count; ++i)
    {
        const size_t len = strlen(host_array_copy[i]);
        if (!oe_is_outside_enclave(host_array_copy[i], len))
            abort();
        lens[i] = len;
        _check(oe_safe_add_sizet(lens_sum, len, &lens_sum));
    }

    // Allocate all required memory in one chunk.
    size_t size = 0;
    // number of array elements + terminating nullptr element
    _check(oe_safe_add_sizet(count, 1, &size));
    // size of array
    _check(oe_safe_mul_sizet(size, sizeof **enclave_array, &size));
    // space for the strings
    _check(oe_safe_add_sizet(size, lens_sum, &size));
    // space for the strings' null terminators
    _check(oe_safe_add_sizet(size, count, &size));

    const auto arr = static_cast<char**>(calloc(1, size));
    if (!arr)
        abort();
    auto str = reinterpret_cast<char*>(arr + count + 1);

    // Copy all strings to the enclave memory.
    for (size_t i = 0; i < count; ++i)
    {
        memcpy(str, host_array_copy[i], lens[i]);
        arr[i] = str;
        str += lens[i] + 1;
    }

    *enclave_array = arr;
}

void ert_restart_host_process()
{
    if (ert_restart_host_process_ocall() != OE_OK)
        abort();
}

const void* payload::get_base() noexcept
{
    return static_cast<const uint8_t*>(__oe_get_enclave_base()) +
           oe_get_module_info()->base_rva;
}

std::pair<const void*, size_t> payload::get_data() noexcept
{
    // payload data resides after relocs
    return {
        static_cast<const uint8_t*>(__oe_get_enclave_base()) + _reloc_rva +
            _reloc_size,
        oe_get_module_info()->payload_data_size};
}
