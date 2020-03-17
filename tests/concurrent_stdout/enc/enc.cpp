#include <openenclave/internal/tests.h>
#include <array>
#include <atomic>
#include <cstdio>
#include <functional>
#include <thread>
#include "test_t.h"

using namespace std;

static void loop(atomic<int>& ready, function<void()> func)
{
    ++ready;
    while (ready != 4)
        __builtin_ia32_pause();
    for (int i = 0; i < 10'000; ++i)
        func();
}

void test_ecall()
{
    atomic<int> ready = 0;

    array<thread, 4> threads{
        thread(loop, ref(ready), bind(fwrite, ".", 1, 1, stdout)),
        thread(loop, ref(ready), bind(fwrite, ".", 1, 1, stdout)),
        thread(loop, ref(ready), bind(putchar, '.')),
        thread(loop, ref(ready), bind(putchar, '.')),
    };

    for (auto& t : threads)
        t.join();
}

OE_SET_ENCLAVE_SGX(
    1,    /* ProductID */
    1,    /* SecurityVersion */
    true, /* Debug */
    64,   /* NumHeapPages */
    64,   /* NumStackPages */
    5);   /* NumTCS */
