#include <openenclave/internal/tests.h>
#include <condition_variable>
#include <mutex>
#include <thread>
#include "test_t.h"

using namespace std;

namespace
{
struct A
{
    mutex m;
    condition_variable cv;
    bool exit = false;
    thread t{&A::f, this};

    ~A()
    {
        {
            const lock_guard l(m);
            exit = true;
            cv.notify_one();
        }
        t.join();
    }

    void f()
    {
        unique_lock l(m);
        cv.wait(l, [this] { return exit; });
    }
};
} // namespace

void test_ecall()
{
    // sanity test
    A();

    // destructor will be executed on enclave exit
    static A a;
}

OE_SET_ENCLAVE_SGX(
    1,    /* ProductID */
    1,    /* SecurityVersion */
    true, /* Debug */
    64,   /* NumHeapPages */
    64,   /* NumStackPages */
    2);   /* NumTCS */
