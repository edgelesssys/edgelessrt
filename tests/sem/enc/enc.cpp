#include <openenclave/corelibc/sched.h>
#include <openenclave/internal/tests.h>
#include <semaphore.h>
#include <atomic>
#include <chrono>
#include <thread>
#include "test_t.h"

using namespace std;

// Calls sem_wait() and sem_post() concurrently to test for race condition bugs.
void test_ecall()
{
    const int count = 1'000'000;
    atomic<bool> ready = false;
    sem_t s{};
    OE_TEST(sem_init(&s, 0, 0) == 0);
    const auto timeout = chrono::steady_clock::now() + 10s;

    thread t([&] {
        for (int i = 0; i < count; ++i)
        {
            ready = true;
            OE_TEST(sem_wait(&s) == 0);
        }
    });

    for (int i = 0; i < count; ++i)
    {
        while (!ready.exchange(false))
        {
            OE_TEST(chrono::steady_clock::now() < timeout);
            oe_sched_yield();
        }
        OE_TEST(sem_post(&s) == 0);
    }

    t.join();
}

OE_SET_ENCLAVE_SGX(
    1,    /* ProductID */
    1,    /* SecurityVersion */
    true, /* Debug */
    64,   /* NumHeapPages */
    64,   /* NumStackPages */
    2);   /* NumTCS */
