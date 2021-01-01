#include <openenclave/ert.h>
#include <openenclave/internal/tests.h>
#include <array>
#include <cstring>
#include <memory>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include "test_t.h"

using namespace std;

template <typename T>
static auto _host_alloc(size_t count)
{
    using TElement = remove_extent_t<T>;
    return unique_ptr<T, decltype(&oe_host_free)>(
        static_cast<TElement*>(oe_host_calloc(count, sizeof(TElement))),
        oe_host_free);
}

static auto _host_alloc_str(string_view s)
{
    auto result = _host_alloc<char>(s.size() + 1);
    memcpy(result.get(), s.data(), s.size());
    return result;
}

static void _test_copy_strings_from_host_to_enclave()
{
    const array strs{
        _host_alloc_str("foo"),
        _host_alloc_str("bar"),
        _host_alloc_str("baz"),
    };

    const auto host_array = _host_alloc<char*[]>(strs.size());
    for (size_t i = 0; i < strs.size(); ++i)
        host_array[i] = strs[i].get();

    char** enclave_array = nullptr;
    ert_copy_strings_from_host_to_enclave(host_array.get(), &enclave_array, 0);
    OE_TEST(oe_is_within_enclave(enclave_array, 1 * sizeof *enclave_array));
    OE_TEST(enclave_array[0] == nullptr);
    free(enclave_array);

    enclave_array = nullptr;
    ert_copy_strings_from_host_to_enclave(host_array.get(), &enclave_array, 1);
    OE_TEST(oe_is_within_enclave(enclave_array, 2 * sizeof *enclave_array));
    OE_TEST(oe_is_within_enclave(enclave_array[0], 4));
    OE_TEST(enclave_array[0] == "foo"s);
    OE_TEST(enclave_array[1] == nullptr);
    free(enclave_array);

    enclave_array = nullptr;
    ert_copy_strings_from_host_to_enclave(host_array.get(), &enclave_array, 2);
    OE_TEST(oe_is_within_enclave(enclave_array, 3 * sizeof *enclave_array));
    OE_TEST(enclave_array[0] == "foo"s);
    OE_TEST(enclave_array[1] == "bar"s);
    OE_TEST(enclave_array[2] == nullptr);
    free(enclave_array);

    enclave_array = nullptr;
    ert_copy_strings_from_host_to_enclave(host_array.get(), &enclave_array, 3);
    OE_TEST(oe_is_within_enclave(enclave_array, 4 * sizeof *enclave_array));
    OE_TEST(enclave_array[0] == "foo"s);
    OE_TEST(enclave_array[1] == "bar"s);
    OE_TEST(enclave_array[2] == "baz"s);
    OE_TEST(enclave_array[3] == nullptr);
    free(enclave_array);
}

void test_ecall()
{
    _test_copy_strings_from_host_to_enclave();
}

OE_SET_ENCLAVE_SGX(
    1,    /* ProductID */
    1,    /* SecurityVersion */
    true, /* Debug */
    64,   /* NumHeapPages */
    64,   /* NumStackPages */
    1);   /* NumTCS */
