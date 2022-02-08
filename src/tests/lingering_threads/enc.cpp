#include <openenclave/internal/tests.h>
#include <atomic>
#include <mutex>
#include <thread>
#include "test_t.h"

using namespace std;

static atomic<bool*> _invalidated_by_atexit;

void test_ecall()
{
    // This is supposed to crash the lingering thread if it is not canceled
    // before atexit functions are called. If it is canceled shortly after
    // atexit, the test will be racy and sometimes succeed and sometimes not.
    bool invalidated_by_atexit = false;
    _invalidated_by_atexit = &invalidated_by_atexit;
    OE_TEST(atexit([] { _invalidated_by_atexit = nullptr; }) == 0);

    static mutex m;
    m.lock();
    thread([] { m.lock(); }).detach();

    thread([] {
        for (;;)
            this_thread::sleep_for(1h);
    }).detach();

    thread([] {
        for (; !*_invalidated_by_atexit;)
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
