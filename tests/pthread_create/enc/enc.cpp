#include <openenclave/corelibc/sched.h>
#include <openenclave/enclave.h>
#include <openenclave/internal/tests.h>
#include <pthread.h>
#include <array>
#include <atomic>
#include <cerrno>
#include "test_t.h"

using namespace std;

static void _sleep()
{
    timespec t{0, 100'000'000};
    while (nanosleep(&t, &t))
        ;
}

static void _test_invalid_arguments()
{
    const auto start_routine = [](void*) -> void* { return nullptr; };

    pthread_t thread{};
    void* ret = nullptr;

    OE_TEST(pthread_create(nullptr, nullptr, start_routine, nullptr) == EINVAL);
    OE_TEST(pthread_create(&thread, nullptr, nullptr, nullptr) == EINVAL);
    OE_TEST(pthread_join(pthread_t(), &ret) == EINVAL);
}

static void _test_created_thread_runs_concurrently()
{
    const auto start_routine = [](void* arg) {
        auto& a = *static_cast<atomic<bool>*>(arg);
        while (!a)
            oe_sched_yield();
        a = false;
        return reinterpret_cast<void*>(2);
    };

    pthread_t thread{};
    atomic<bool> a = false;

    OE_TEST(pthread_create(&thread, nullptr, start_routine, &a) == 0);

    a = true;

    intptr_t ret = 0;
    OE_TEST(pthread_join(thread, reinterpret_cast<void**>(&ret)) == 0);
    OE_TEST(ret == 2);
    OE_TEST(!a);
}

static void _test_join_retval_nullptr()
{
    const auto start_routine = [](void*) { return reinterpret_cast<void*>(2); };
    pthread_t thread{};

    OE_TEST(pthread_create(&thread, nullptr, start_routine, nullptr) == 0);

    OE_TEST(pthread_join(thread, nullptr) == 0);
}

static void _test_join_before_thread_ends()
{
    const auto start_routine = [](void* arg) {
        auto& a = *static_cast<atomic<bool>*>(arg);
        a = true;
        _sleep();
        return reinterpret_cast<void*>(2);
    };

    pthread_t thread{};
    atomic<bool> a = false;

    OE_TEST(pthread_create(&thread, nullptr, start_routine, &a) == 0);

    while (!a)
        oe_sched_yield();

    intptr_t ret = 0;
    OE_TEST(pthread_join(thread, reinterpret_cast<void**>(&ret)) == 0);
    OE_TEST(ret == 2);
}

static void _test_join_after_thread_ends()
{
    const auto start_routine = [](void* arg) {
        auto& a = *static_cast<atomic<bool>*>(arg);
        a = true;
        return reinterpret_cast<void*>(2);
    };

    pthread_t thread{};
    atomic<bool> a = false;

    OE_TEST(pthread_create(&thread, nullptr, start_routine, &a) == 0);

    while (!a)
        oe_sched_yield();

    _sleep();

    intptr_t ret = 0;
    OE_TEST(pthread_join(thread, reinterpret_cast<void**>(&ret)) == 0);
    OE_TEST(ret == 2);
}

static void _test_multiple_threads()
{
    const auto start_routine = [](void* arg) -> void* {
        auto& a = *static_cast<atomic<int>*>(arg);
        ++a;
        _sleep();
        return nullptr;
    };

    array<pthread_t, 8> threads{};
    atomic<int> count = 0;

    for (pthread_t& t : threads)
        OE_TEST(pthread_create(&t, nullptr, start_routine, &count) == 0);

    for (pthread_t t : threads)
        OE_TEST(pthread_join(t, nullptr) == 0);

    OE_TEST(count == threads.size());
}

void test_ecall()
{
    _test_invalid_arguments();
    _test_created_thread_runs_concurrently();
    _test_join_retval_nullptr();
    _test_join_before_thread_ends();
    _test_join_after_thread_ends();
    _test_multiple_threads();
}

OE_SET_ENCLAVE_SGX(
    1,    /* ProductID */
    1,    /* SecurityVersion */
    true, /* Debug */
    64,   /* NumHeapPages */
    64,   /* NumStackPages */
    9);   /* NumTCS */
