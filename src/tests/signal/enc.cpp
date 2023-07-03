#include <openenclave/internal/tests.h>
#include <array>
#include <cerrno>
#include <csignal>
#include "../../ertlibc/signal.h"
#include "test_t.h"

using namespace std;

extern "C" const bool ert_enable_signals = true;
extern "C" const bool ert_enable_sigsegv = true;

static constexpr uint64_t _guard = 0xED6E1E55ED6E1E55;

static struct
{
    uint64_t guard0 = _guard;
    array<uint8_t, ERT_SIGSTKSZ> stack;
    uint64_t guard1 = _guard;
} _signalstack;

static const void* _addr_within_signalstack;

static int f()
{
    return 2;
}

static void handler(int sig, siginfo_t* info, void* ucontext)
{
    OE_TEST(sig == SIGSEGV);
    OE_TEST(info);
    _addr_within_signalstack = &sig;

    // test_ecall called nullptr. Set rip to a function so that thread execution
    // can continue.
    static_cast<ucontext_t*>(ucontext)->uc_mcontext.gregs[REG_RIP] =
        reinterpret_cast<greg_t>(f);
}

void test_ecall()
{
    // Register signal handler.
    struct sigaction sa = {};
    sa.sa_flags = SA_SIGINFO;
    sa.sa_sigaction = handler;
    OE_TEST(sigaction(SIGSEGV, &sa, nullptr) == 0);

    // Call nullptr. The signal handler will fix this.
    int (*const volatile p)() = nullptr;
    OE_TEST(p() == 2);

    // Set alternative signal stack.
    stack_t st{};
    st.ss_sp = _signalstack.stack.data();
    st.ss_size = _signalstack.stack.size();
    OE_TEST(sigaltstack(&st, nullptr) == 0);

    OE_TEST(p() == 2);

    // Test that alternative signal stack has been used and no overflow
    // happened.
    OE_TEST(
        &_signalstack.stack.front() < _addr_within_signalstack &&
        _addr_within_signalstack < &_signalstack.stack.back());
    OE_TEST(_signalstack.guard0 == _guard);
    OE_TEST(_signalstack.guard1 == _guard);

    // Disable alternative signal stack.
    st.ss_flags = SS_DISABLE;
    OE_TEST(sigaltstack(&st, nullptr) == 0);
    OE_TEST(p() == 2);
    OE_TEST(
        !(&_signalstack.stack.front() <= _addr_within_signalstack &&
          _addr_within_signalstack <= &_signalstack.stack.back()));

    // Test invalid arguments.
    errno = 0;
    OE_TEST(sigaction(0, &sa, nullptr) == -1 && errno == EINVAL);
    errno = 0;
    OE_TEST(sigaction(-1, &sa, nullptr) == -1 && errno == EINVAL);
    errno = 0;
    OE_TEST(sigaction(-2, &sa, nullptr) == -1 && errno == EINVAL);
    st.ss_flags = SS_ONSTACK;
    errno = 0;
    OE_TEST(sigaltstack(&st, nullptr) == -1 && errno == EINVAL);
    st.ss_flags = 0;
    st.ss_size = ERT_MINSIGSTKSZ - 1;
    errno = 0;
    OE_TEST(sigaltstack(&st, nullptr) == -1 && errno == ENOMEM);

    // Get old sigaction.
    struct sigaction oldsa = {};
    OE_TEST(sigaction(SIGSEGV, nullptr, &oldsa) == 0);
    OE_TEST(oldsa.sa_flags & SA_SIGINFO);
    OE_TEST(oldsa.sa_sigaction == handler);

    // Get old sigaltstack.
    stack_t oldst{};
    OE_TEST(sigaltstack(nullptr, &oldst) == 0);
    OE_TEST(oldst.ss_flags == SS_DISABLE);
}

OE_SET_ENCLAVE_SGX(
    1,    /* ProductID */
    1,    /* SecurityVersion */
    true, /* Debug */
    64,   /* NumHeapPages */
    64,   /* NumStackPages */
    1);   /* NumTCS */
