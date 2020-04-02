#include <openenclave/internal/tests.h>
#include <mutex>
#include <thread>
#include "test_t.h"

using namespace std;

void test_ecall()
{
    static mutex m;
    m.lock();
    thread([] { m.lock(); }).detach();

    thread([] {
        for (;;)
            this_thread::sleep_for(1h);
    }).detach();

    thread([] {
        for (;;)
            this_thread::sleep_for(1ms);
    }).detach();

    thread([] {}).detach();

    this_thread::sleep_for(100ms);

    // The actual test happens when the host calls oe_terminate_enclave().
    // Threads are expected to be successfully canceled.
}

OE_SET_ENCLAVE_SGX(
    1,    /* ProductID */
    1,    /* SecurityVersion */
    true, /* Debug */
    64,   /* NumHeapPages */
    64,   /* NumStackPages */
    5);   /* NumTCS */
