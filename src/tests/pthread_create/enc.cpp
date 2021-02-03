#include <openenclave/corelibc/sched.h>
#include <openenclave/enclave.h>
#include <openenclave/internal/tests.h>
#include <pthread.h>
#include <array>
#include <atomic>
#include <cerrno>
#include "test_t.h"

using namespace std;

namespace
{
thread_local int _tl_i = 2;

struct TlsCtorDtorTest
{
    static atomic<int> ctor_count;
    static atomic<int> dtor_count;
    int i;

    TlsCtorDtorTest() : i(4)
    {
        ++ctor_count;
    }

    ~TlsCtorDtorTest()
    {
        ++dtor_count;
    }

    static void reset_count()
    {
        ctor_count = 0;
        dtor_count = 0;
    }

    static void expect_count(int count)
    {
        OE_TEST(ctor_count == count);
        OE_TEST(dtor_count == count);
    }
} thread_local _tl_ctor_dtor;

atomic<int> TlsCtorDtorTest::ctor_count, TlsCtorDtorTest::dtor_count;

void _test_tls()
{
    // test that TLS variables are reinitialized
    OE_TEST(++_tl_i == 3);
    OE_TEST(++_tl_ctor_dtor.i == 5);
}
} // namespace

static int _test_create_thread_during_initialization = [] {
    const auto start_routine = [](void*) -> void* { return nullptr; };

    pthread_t thread{};
    OE_TEST(pthread_create(&thread, nullptr, start_routine, nullptr) == 0);
    OE_TEST(pthread_join(thread, nullptr) == 0);

    return 0;
}();

static void _sleep()
{
    timespec t{0, 100'000'000};
    while (nanosleep(&t, &t))
        ;
}

static void _test_invalid_arguments()
{
    OE_TEST(pthread_join(pthread_t(), nullptr) == ESRCH);
    OE_TEST(pthread_detach(pthread_t()) == ESRCH);
    OE_TEST(pthread_cancel(pthread_t()) == ESRCH);
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
        _test_tls();
        _sleep();
        return nullptr;
    };

    array<pthread_t, 8> threads{};
    atomic<int> count = 0;
    TlsCtorDtorTest::reset_count();

    for (pthread_t& t : threads)
        OE_TEST(pthread_create(&t, nullptr, start_routine, &count) == 0);

    for (pthread_t t : threads)
        OE_TEST(pthread_join(t, nullptr) == 0);

    OE_TEST(count == threads.size());

    // The current implementation of join does not wait until TLS has been
    // unwound.
    _sleep();
    TlsCtorDtorTest::expect_count(threads.size());
}

static void _test_detach()
{
    const auto start_routine = [](void*) -> void* {
        _test_tls();
        return nullptr;
    };

    TlsCtorDtorTest::reset_count();

    for (int i = 0; i < 2; ++i)
    {
        for (int j = 0; j < 8; ++j)
        {
            pthread_t t{};
            OE_TEST(pthread_create(&t, nullptr, start_routine, nullptr) == 0);
            OE_TEST(pthread_detach(t) == 0);
        }

        _sleep();

        // If pthread_detach() did not work, the next 8 threads would exceed
        // the TCS limit.
    }

    // threads should be finished by now
    TlsCtorDtorTest::expect_count(16);
}

static void _test_detached()
{
    const auto start_routine = [](void*) -> void* {
        _test_tls();
        return nullptr;
    };

    pthread_attr_t attr{};
    OE_TEST(pthread_attr_init(&attr) == 0);
    OE_TEST(pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED) == 0);

    TlsCtorDtorTest::reset_count();

    for (int i = 0; i < 2; ++i)
    {
        for (int j = 0; j < 8; ++j)
        {
            pthread_t t{};
            OE_TEST(pthread_create(&t, &attr, start_routine, nullptr) == 0);
        }

        _sleep();

        // If PTHREAD_CREATE_DETACHED did not work, the next 8 threads would
        // exceed the TCS limit
    }

    // threads should be finished by now
    TlsCtorDtorTest::expect_count(16);

    OE_TEST(pthread_attr_destroy(&attr) == 0);
}

static void _test_exit()
{
    const auto start_routine = [](void*) -> void* {
        pthread_exit(reinterpret_cast<void*>(2));
        OE_TEST(false);
    };

    pthread_t thread{};
    OE_TEST(pthread_create(&thread, nullptr, start_routine, nullptr) == 0);

    intptr_t ret = 0;
    OE_TEST(pthread_join(thread, reinterpret_cast<void**>(&ret)) == 0);
    OE_TEST(ret == 2);
}

static void _test_cancel()
{
    const auto start_routine = [](void*) -> void* {
        for (;;)
        {
            oe_sched_yield();
            pthread_testcancel();
        }
        OE_TEST(false);
    };

    pthread_t thread{};
    OE_TEST(pthread_create(&thread, nullptr, start_routine, nullptr) == 0);
    OE_TEST(pthread_cancel(thread) == 0);

    void* ret = nullptr;
    OE_TEST(pthread_join(thread, &ret) == 0);
    OE_TEST(ret == PTHREAD_CANCELED);
}

static void _test_attr()
{
    pthread_attr_t attr{};
    OE_TEST(pthread_getattr_np(pthread_self(), &attr) == 0);

    size_t guardsize = 0;
    OE_TEST(pthread_attr_getguardsize(&attr, &guardsize) == 0);
    OE_TEST(guardsize == OE_PAGE_SIZE);

    void* stackaddr = nullptr;
    size_t stacksize = 0;
    OE_TEST(pthread_attr_getstack(&attr, &stackaddr, &stacksize) == 0);

    // see NumStackPages at the end of this file
    OE_TEST(stacksize == 64 * OE_PAGE_SIZE);

    const auto stackptr = static_cast<const uint8_t*>(stackaddr);
    const uint8_t stackvar = 0;
    OE_TEST(stackptr < &stackvar && &stackvar < stackptr + stacksize);
}

static void _test_mutexattr()
{
    pthread_mutexattr_t attr{};
    OE_TEST(pthread_mutexattr_init(&attr) == 0);
    OE_TEST(pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_ADAPTIVE_NP) == 0);
}

void test_ecall()
{
    _test_invalid_arguments();
    _test_created_thread_runs_concurrently();
    _test_join_retval_nullptr();
    _test_join_before_thread_ends();
    _test_join_after_thread_ends();
    _test_multiple_threads();
    _test_detach();
    _test_detached();
    _test_exit();
    _test_cancel();
    _test_attr();
    _test_mutexattr();
}

OE_SET_ENCLAVE_SGX(
    1,    /* ProductID */
    1,    /* SecurityVersion */
    true, /* Debug */
    64,   /* NumHeapPages */
    64,   /* NumStackPages */
    9);   /* NumTCS */
