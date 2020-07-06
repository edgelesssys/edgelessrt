#include <openenclave/internal/tests.h>
#include <pthread.h>
#include <atomic>
#include <future>
#include <mutex>
#include <thread>
#include <vector>
#include "test_t.h"

using namespace std;

static void _test_threadid()
{
    thread::id id1;
    pthread_t handle1{};

    thread t([&] {
        id1 = this_thread::get_id();
        handle1 = pthread_self();
    });

    const auto id2 = t.get_id();
    OE_TEST(id2 != this_thread::get_id());

    const auto handle2 = t.native_handle();
    OE_TEST(handle2 != pthread_self());

    OE_TEST(t.joinable());
    t.join();
    OE_TEST(!t.joinable());

    OE_TEST(id1 == id2);
    OE_TEST(handle1 == handle2);
}

static void _test_misc()
{
    this_thread::yield();
    OE_TEST(this_thread::get_id() != thread::id());
    OE_TEST(thread::hardware_concurrency() >= 2);

    const thread t;
    OE_TEST(!t.joinable());

    // future::wait_for uses futex without FUTEX_PRIVATE flag
    auto f = async(launch::async, [] {
        this_thread::sleep_for(10ms);
        return 2;
    });
    f.wait_for(1s);
    OE_TEST(f.get() == 2);
}

static void _test_once()
{
    const int thread_count = 4;
    once_flag of;
    atomic<int> ready = 0;
    atomic<int> called = 0;
    vector<thread> threads;

    for (int i = 0; i < thread_count; ++i)
        threads.emplace_back([&] {
            ++ready;
            while (ready != thread_count)
                __builtin_ia32_pause();
            call_once(of, [&] { ++called; });
        });

    for (auto& t : threads)
        t.join();

    OE_TEST(called == 1);
}

void test_ecall()
{
    _test_threadid();
    _test_misc();
    _test_once();
}

OE_SET_ENCLAVE_SGX(
    1,    /* ProductID */
    1,    /* SecurityVersion */
    true, /* Debug */
    64,   /* NumHeapPages */
    64,   /* NumStackPages */
    6);   /* NumTCS */
